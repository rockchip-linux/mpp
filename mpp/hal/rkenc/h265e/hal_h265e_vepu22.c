/*
 * Copyright 2017 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "hal_h265e_vepu22"

#include <string.h>

#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "h265e_syntax.h"
#include "hal_h265e_base.h"
#include "hal_h265e_vepu22_def.h"
#include "hal_h265e_vepu22.h"


#define H265E_MAX_ROI_LEVEL           (8)
#define H265E_LOG2_CTB_SIZE           (6)
#define H265E_CTB_SIZE                (1 << H265E_LOG2_CTB_SIZE)
#define H265E_MAX_CTU_NUM             0x4000      // CTU num for max resolution = 8192x8192/(64x64)

// this value must sync with kernel
#define MPP_H265E_BASE                     0x1000
#define H265E_SET_PARAMETER               (MPP_H265E_BASE+6)
#define H265E_GET_HEADER                  (MPP_H265E_BASE+7)
#define H265E_RESET                       (MPP_H265E_BASE+8)

typedef enum {
    CODEOPT_ENC_HEADER_IMPLICIT = (1 << 0), /* A flag to encode (a) headers (VPS, SPS, PPS) implicitly for generating bitstreams conforming to spec. */
    CODEOPT_ENC_VCL             = (1 << 1), /* A flag to encode VCL nal unit explicitly */
    CODEOPT_ENC_VPS             = (1 << 2), /* A flag to encode VPS nal unit explicitly */
    CODEOPT_ENC_SPS             = (1 << 3), /* A flag to encode SPS nal unit explicitly */
    CODEOPT_ENC_PPS             = (1 << 4), /* A flag to encode PPS nal unit explicitly */
    CODEOPT_ENC_AUD             = (1 << 5), /* A flag to encode AUD nal unit explicitly */
    CODEOPT_ENC_EOS             = (1 << 6), /* A flag to encode EOS nal unit explicitly */
    CODEOPT_ENC_EOB             = (1 << 7), /* A flag to encode EOB nal unit explicitly */
    CODEOPT_ENC_RESERVED        = (1 << 8), /* reserved */
    CODEOPT_ENC_VUI             = (1 << 9), /* A flag to encode VUI nal unit explicitly */
} H265E_PIC_CODE_OPTION;


#if 0
/*
* check mutli slice parameters
* h265 encoder support independ slice and depend slice.
* There are must independ slice when set multi slice,and independ_slice_mode must greater or equit than depend_slice_mode.
* There are only ctu mode(value = 1) in independ_slice_mode, and ctu mode(value = 1) and byte size(value = 2)
* in depend_slice_mode.
*/
static MPP_RET vepu22_checkout_multi_slice(HalH265eCfg* cfg)
{
    if (cfg == NULL) {
        mpp_err_f("error: param is null\n");
        return MPP_NOK;
    }

    hal_h265e_dbg_func("enter\n");
    if (cfg->independ_slice_mode == 0 && cfg->depend_slice_mode != 0) {
        mpp_err_f("error: independ_slice_mode = 0,depend_slice_mode must be 0\n");
        return -1;
    } else if (cfg->independ_slice_mode == 1 && cfg->depend_slice_mode == 1) {
        if (cfg->independ_slice_mode_arg < cfg->independ_slice_mode_arg) {
            mpp_err_f("error: independ_slice_mode & depend_slice_mode is both"
                      " '1'(multi-slice with ctu count),"
                      "  must be independ_slice_mode_arg >= independ_slice_mode_arg\n");
            return MPP_NOK;
        }
    }

    if (cfg->independ_slice_mode != 0) {
        if (cfg->independ_slice_mode_arg > 65535) {
            mpp_err_f("error: If independ_slice_mode is not 0, "
                      "must be independ_slice_mode_arg <= 0xFFFF\n");
            return MPP_NOK;
        }
    }

    if (cfg->depend_slice_mode != 0) {
        if (cfg->independ_slice_mode_arg > 65535) {
            mpp_err_f("error: If depend_slice_mode is not 0, "
                      "must be independ_slice_mode_arg <= 0xFFFF\n");
            return MPP_NOK;
        }
    }

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_check_ctu_parameter(HalH265eCtx* ctx)
{
    hal_h265e_dbg_func("enter\n");

    HalH265eCfg* cfg = (HalH265eCfg*)ctx->hw_cfg;
    // rc_enbale and ctu'qp can't both open
    if (cfg->rc_enable && cfg->ctu.ctu_qp_enable) {
        mpp_err_f("error: rc_mode and ctu qp enale can't both open,close ctu qp enable\n");
        cfg->ctu.ctu_qp_enable = 0;
    }

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}
#endif

MPP_RET vepu22_need_pre_process(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    int h_stride = prep->hor_stride;

    /*
     * the stride of luma and chroma must align 16,
     * so if input format is NV12 or NV21, no need to process,
     * and if the input format is YU12 or YV12, and the stride of
     * input is align 32(stride of luma is align 32, stride of chroma is align 16),
     * there is no need to proces also.
     *
     * return MPP_NOK means need pre process(fomrat translate)
     * return MPP_OK means need do nothing
     */
    if ((prep->format == MPP_FMT_YUV420SP) || (prep->format == MPP_FMT_YUV420SP_VU)) {
        return MPP_OK;
    } else if (h_stride == MPP_ALIGN(prep->hor_stride, 32)) {
        return MPP_OK;
    }

    RgaCtx rga = ctx->rga_ctx;
    if (rga == NULL) {
        MPP_RET ret = rga_init(&rga);
        if (ret) {
            mpp_err("init rga context failed %d\n", ret);
        } else {
            ctx->rga_ctx = rga;
        }
    }

    return ctx->rga_ctx ? MPP_NOK : MPP_OK;
}

static RK_U8 vepu22_get_endian(int endian)
{
    switch (endian) {
    case H265E_LITTLE_ENDIAN:
        endian = 0x00;
        break;

    case H265E_BIG_ENDIAN:
        endian = 0x0f;
        break;

    default:
        mpp_err_f("error: endian = %d not support", endian);
        return -1;
    }

    return (endian & 0x0f);
}

/*
* set vui parameter
* flags in struct vui every bit have meaning
*/
static MPP_RET vepu22_set_default_vui_parameter(H265e_VUI* vui)
{
    if (vui == NULL) {
        mpp_err_f("error: param is null\n");
        return MPP_NOK;
    }

    hal_h265e_dbg_func("enter\n");

    vui->flags = 0;
    /*
    * A flag whether to insertaspect_ratio_info_present_flag syntax of VUI parameters
    * enable frame rate encode to vui
    */
    vui->flags |= 1 << 3;
    vui->aspect_ratio_idc = 0;
    vui->sar_size = 0;
    vui->over_scan_appropriate = 0;

    vui->signal = 0;
    vui->chroma_sample_loc = 0;
    vui->disp_win_left_right = 0;
    vui->disp_win_top_bottom = 0;

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_set_rc_default_parameter(HalH265eCfg* cfg)
{
    hal_h265e_dbg_func("enter\n");

    cfg->min_qp = 8;
    cfg->max_qp = 51;
    cfg->max_delta_qp = 10;
    cfg->initial_rc_qp = 63;
    cfg->init_buf_levelx8 = 1;
    cfg->intra_qp_offset = 0;
    cfg->hvs_qp_enable = 0;// 1;
    cfg->cu_level_rc_enable = 1;
    cfg->trans_rate = 0;
    cfg->initial_delay = 100;

    // close rc enable default
    cfg->rc_enable = 0;

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

/*
* set h265 encoder default parameters
*/
static MPP_RET vepu22_set_default_hw_cfg(HalH265eCfg* cfg)
{
    RK_U32 i = 0;

    hal_h265e_dbg_func("enter\n");

    cfg->cfg_option = H265E_OPT_ALL_PARAM;
    cfg->cfg_mask = H265E_CFG_CHANGE_SET_PARAM_ALL;
    // support profile main ,level 4.1
    cfg->profile = H265E_Profile_Main;
    cfg->level = H265E_MAIN_LEVEL41;
    cfg->tier = H265E_MAIN_TILER;

    // only YUV_420 8bit
    cfg->chroma_idc = 0;
    cfg->lossless_enable = 0;
    cfg->const_intra_pred_flag = 0;

    cfg->chroma_cb_qp_offset = 0;
    cfg->chroma_cr_qp_offset = 0;

    cfg->width = 0;
    cfg->height = 0;
    cfg->bit_depth = 8;
    cfg->src_format = H265E_SRC_YUV_420;
    cfg->frame_rate = 30;
    cfg->frame_skip = 0;

    cfg->src_endian = vepu22_get_endian(H265E_BIG_ENDIAN);
    cfg->bs_endian = vepu22_get_endian(H265E_BIG_ENDIAN);
    // only support value 0,detail see register SFB_OPTION (0x0000010C)
    cfg->fb_endian = vepu22_get_endian(H265E_LITTLE_ENDIAN);

    cfg->codeOption.implicitHeaderEncode = 1;
    cfg->codeOption.encodeVCL = 1;
    cfg->codeOption.encodeVPS = 1;
    cfg->codeOption.encodeSPS = 1;
    cfg->codeOption.encodePPS = 1;
    // fbc may type: only support COMPRESSED_FRAME_MAP
    cfg->fbc_map_type = COMPRESSED_FRAME_MAP;

    cfg->use_cur_as_longterm_pic = 0;
    cfg->use_longterm_ref = 0;
    cfg->use_long_term  = 0;

    /* for CMD_ENC_SEQ_INTRA_PARAM */
    cfg->decoding_refresh_type = 1;
    cfg->intra_period         = 32;
    cfg->intra_qp             = 26;

    /* for CMD_ENC_SEQ_CONF_WIN_TOP_BOT/LEFT_RIGHT */
    cfg->conf_win_top    = 0;
    cfg->conf_win_bot    = 0;
    cfg->conf_win_left   = 0;
    cfg->conf_win_right  = 0;

    /* for CMD_ENC_SEQ_INDEPENDENT_SLICE */
    cfg->independ_slice_mode     = 0;
    cfg->independ_slice_mode_arg = 0;

    /* for CMD_ENC_SEQ_DEPENDENT_SLICE */
    cfg->depend_slice_mode     = 0;
    cfg->depend_slice_mode_arg = 0;

    /* for CMD_ENC_SEQ_INTRA_REFRESH_PARAM */
    cfg->intra_in_inter_slice_enable = 0;
    cfg->intra_refresh_mode  = 0;
    cfg->intra_refresh_arg   = 0;
    cfg->use_recommend_param = 0;// 1;

    cfg->sei.prefix_sei_nal_enable   = 0;
    cfg->sei.suffix_sei_nal_enable   = 0;

    cfg->hrd_rbsp_in_vps = 0;
    cfg->hrd_rbsp_in_vui = 0;
    cfg->vui_rbsp        = 0;

    /* for CMD_ENC_PARAM */
    if (cfg->use_recommend_param == 0) {
        cfg->scaling_list_enable        = 0; // ScalingList
        cfg->cu_size_mode               = 0x07; // enable CU8x8, CU16x16, CU32x32
        cfg->tmvp_enable                = 1; // EnTemporalMVP
        cfg->wpp_enable                 = 0;
        cfg->max_num_merge              = 2;
        cfg->dynamic_merge_8x8_enable   = 1;
        cfg->dynamic_merge_16x16_enable = 1;
        cfg->dynamic_merge_32x32_enable = 1;
        cfg->disable_deblk              = 0;
        cfg->lf_cross_slice_boundary_enable = 1;
        cfg->beta_offset_div2           = 0;
        cfg->tc_offset_div2             = 0;
        cfg->skip_intra_trans           = 0;
        cfg->sao_enable                 = 1;
        cfg->intra_qp_offset            = 1;
        cfg->intra_nxn_enable           = 0;
    }

    /* for CMD_ENC_RC_PARAM */
    cfg->ctu.roi_enable   = 0;
    cfg->ctu.roi_delta_qp = 3;
    cfg->ctu.map_endian   = 0;
    cfg->intra_qp_offset  = 0;
    cfg->bit_alloc_mode   = 0;

    for (i = 0; i < H265E_MAX_GOP_NUM; i++) {
        cfg->fixed_bit_ratio[i] = 1;
    }

    cfg->hvs_qp_enable       = 0;// 1;
    cfg->hvs_qp_scale_enable = 0;
    cfg->hvs_qp_scale        = 0;

    /* for CMD_ENC_CUSTOM_GOP_PARAM */
    cfg->gop_idx = PRESET_IDX_CUSTOM_GOP;
    cfg->gop.custom_gop_size     = 8; // 1~8
    cfg->gop.use_derive_lambda_weight = 1; // DeriveLambdaWeight
    if (cfg->intra_period == 1) {
        cfg->gop_idx = PRESET_IDX_ALL_I;
    } else {
        cfg->gop_idx = PRESET_IDX_CUSTOM_GOP;
    }

    if (cfg->gop_idx == PRESET_IDX_CUSTOM_GOP) {
        for (i = 0; i < cfg->gop.custom_gop_size; i++) {
            if (i == 0) {
                cfg->gop.pic[i].ref_poc_l0 = 0;
            } else {
                cfg->gop.pic[i].ref_poc_l0 = i - 1;
            }
            cfg->gop.pic[i].type  = H265E_PIC_TYPE_P;
            //not set pocOffset to 0,because the first I frame's will be take the pocOffset 0
            cfg->gop.pic[i].offset      = i + 1;
            cfg->gop.pic[i].qp          = cfg->intra_qp;
            cfg->gop.pic[i].ref_poc_l1  = 0;
            cfg->gop.pic[i].temporal_id = 0;
            cfg->gop.gop_pic_lambda[i]  = 0;
        }
    }

    // for VUI / time information.
    cfg->num_ticks_poc_diff_one = 0;
    cfg->time_scale             =  cfg->frame_rate * 1000;
    cfg->num_units_in_tick      =  1000;

    cfg->vui.flags = 0;                // when vuiParamFlags == 0, VPU doesn't encode VUI

    cfg->nr_y_enable        = 0;
    cfg->nr_cb_enable       = 0;
    cfg->nr_cr_enable       = 0;
    cfg->nr_noise_est_enable = 1;
    cfg->nr_intra_weight_y = 7;
    cfg->nr_intra_weight_cb = 7;
    cfg->nr_intra_weight_cr = 7;
    cfg->nr_inter_weight_y = 4;
    cfg->nr_inter_weight_cb = 4;
    cfg->nr_inter_weight_cr = 4;

    cfg->intra_min_qp       = 8;
    cfg->intra_max_qp       = 51;

    vepu22_set_default_vui_parameter(&cfg->vui);
    vepu22_set_rc_default_parameter(cfg);

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_clear_cfg_mask(HalH265eCtx* cfg)
{
    HalH265eCfg *hw_cfg = (HalH265eCfg *)cfg->hw_cfg;

    hw_cfg->cfg_mask = 0;
    hw_cfg->cfg_option = 0;

    return MPP_OK;
}

/*
* check prep parameter is valid
*/
static MPP_RET vepu22_check_prep_parameter(MppEncPrepCfg *set)
{
    if (set == NULL) {
        mpp_err_f("parameter is NULL\n");
        return MPP_NOK;
    }

    if (set->width < 256 && set->width > 1920) {
        mpp_err_f("ERROR: invalid width %d is not in range [256..1920]\n", set->width);
        return MPP_NOK;
    }

    if (set->height < 128 && set->height > 1080) {
        mpp_err_f("ERROR: invalid height %d is not in range [128..1080]\n", set->height);
        return MPP_NOK;
    }

    if (set->hor_stride < set->width) {
        mpp_err_f("WARNING: hor_stride(%d) < cfg->width(%d),force hor_stride = width\n",
                  set->hor_stride, set->width);
        set->hor_stride = set->width;
    }

    if (set->ver_stride < set->height) {
        mpp_err_f("WARNING: ver_stride(%d) < cfg->height(%d),force ver_stride = height\n",
                  set->ver_stride, set->height);
        set->ver_stride = set->height;
    }

    if (set->format !=  MPP_FMT_YUV420SP &&
        set->format !=  MPP_FMT_YUV420P &&
        set->format != MPP_FMT_YUV420SP_VU) {
        mpp_err_f("ERROR: invalid format %d is not supportted\n", set->format);
        return MPP_NOK;
    }

    return MPP_OK;
}

static MPP_RET vepu22_force_frame_type(H265E_PIC_TYPE type, EncInfo* info)
{
    if (info == NULL) {
        mpp_err_f("error: info = %p\n", info);
        return MPP_NOK;
    }

    if (type > H265E_PIC_TYPE_CRA ||
        type == H265E_PIC_TYPE_B) {
        mpp_err_f("error: type = %d not support\n");
        return MPP_NOK;
    }

    info->force_frame_type_enable = 1;
    info->force_frame_type = type;

    return MPP_OK;
}


static MPP_RET vepu22_set_frame_type(H265eSyntax *syntax, EncInfo* info)
{
    if (syntax == NULL || info == NULL) {
        mpp_err_f("error: syntax = %p,info = %p\n", syntax, info);
        return MPP_NOK;
    }
    if (syntax->idr_request > 0) {
        vepu22_force_frame_type(H265E_PIC_TYPE_IDR, info);
        syntax->idr_request --;
    } else {
        info->force_frame_type_enable = 0;
        info->force_frame_type = 0;
    }

    return MPP_OK;
}

static MPP_RET vepu22_set_ctu_qp(HalH265eCtx* ctx)
{
    EncInfo *en_info = (EncInfo *)ctx->en_info;
    HalH265eCfg* hw_cfg = (HalH265eCfg *)ctx->hw_cfg;

    en_info->ctu_qp_fd = 0;
    if (ctx->ctu != NULL && hw_cfg->ctu.ctu_qp_enable > 0) {
        en_info->ctu_qp_fd = mpp_buffer_get_fd(ctx->ctu);
    }
    return MPP_OK;
}

static MPP_RET vepu22_set_roi_region(HalH265eCtx *ctx)
{
    EncInfo *en_info = (EncInfo *)ctx->en_info;
    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;

    en_info->roi_fd = 0;
    if (ctx->roi != NULL && hw_cfg->ctu.roi_enable > 0) {
        en_info->roi_fd = mpp_buffer_get_fd(ctx->roi);
    }
    return MPP_OK;
}

static RK_U8 vepu22_get_yuv_format(HalH265eCtx* ctx)
{
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    MppFrameFormat format = prep->format;
    RK_U8 output_format = H265E_SRC_YUV_420_YV12;
    switch (format) {
    case MPP_FMT_YUV420SP:
        output_format = H265E_SRC_YUV_420_NV12;
        break;

    case MPP_FMT_YUV420P:
        output_format = H265E_SRC_YUV_420_YU12;
        if (vepu22_need_pre_process(ctx) != MPP_OK) {
            output_format = H265E_SRC_YUV_420_NV12;
        }
        break;

    case MPP_FMT_YUV420SP_VU:
        output_format = H265E_SRC_YUV_420_NV21;
        break;

    default:
        output_format = H265E_SRC_YUV_420_YV12;
        if (vepu22_need_pre_process(ctx) != MPP_OK) {
            output_format = H265E_SRC_YUV_420_NV12;
        }
        break;
    }

    return output_format;
}

static MPP_RET vepu22_check_roi_parameter(void *cfg)
{
    MppEncH265RoiCfg* roi = (MppEncH265RoiCfg*)cfg;

    if (roi->num) {
        if (roi->delta_qp < 1 || roi->delta_qp > 51) {
            mpp_err_f("error: syntax->roi.delta_qp = %d must between [1,51]\n", roi->delta_qp);
            return MPP_NOK;
        }
    }

    return MPP_OK;
}

/*
 * set ROI's map
 * user can set some ROI regions, the ROI regions will have small
 * qp value when encode.The ROI regions use ctu size as a unit.
 * ROI's map is a buffer cotain define roi's level. The buffer index is
 * the coordinates with CTU as the unit. For example:
 * there are two roi' region, the value of level are 1 and 2,
 * this function first convert the region's pixel coordinates to
 * CTU coordinates, and set those ctu's level value.
 *****************************************************************
 *   *   *   *   *   *   *   *   *   *   *   *   * 2 * 2 * 2 * 2 *
 *****************************************************************
 *   *   *   *   *1  *1  *1  *1  *1  *   *   *   * 2 * 2 * 2 * 2 *
 *****************************************************************
 *   *   *   *   *1  *1  *1  *1  *1  *   *   *   * 2 * 2 * 2 * 2 *
 *****************************************************************
 *   *   *   *   *1  *1  *1  *1  *1  *   *   *   *   *   *   *   *
 *****************************************************************
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 *****************************************************************
 *
 * NOTE: ROI enable only work when Rate Control is enable
 */
static MPP_RET vepu22_set_roi_region_parameter(HalH265eCtx* ctx, void *para)
{
    H265e_CTU *ctu = NULL;
    MppEncH265RoiCfg *roi = (MppEncH265RoiCfg*)para;
    MppEncH265RoiCfg local_roi;
    H265eRoiRegion *region = NULL;
    H265eRoiRegion *region1 = NULL;
    H265eRect* rect = NULL;
    RK_S32 i = 0;
    RK_S32 x = 0, y = 0;
    RK_S32 left = 0, right = 0;
    RK_S32 top = 0, bottom = 0;
    RK_S32 width, height;
    RK_S32 map_width;//, map_height;
    RK_S32 roi_number = 0;
    RK_S32 roi_id = 0;
    RK_U8 *region_buf = NULL;
    MPP_RET ret = MPP_OK;
    HalH265eCfg *hw_cfg = NULL;

    hal_h265e_dbg_func("enter\n");
    if (ctx == NULL || roi == NULL) {
        mpp_err_f("error: parameter is invalid");
        return MPP_NOK;
    }

    hw_cfg = (HalH265eCfg *)ctx->hw_cfg;
    hal_h265e_dbg_func("cfg_mask = 0x%x\n", hw_cfg->cfg_mask);
    if (vepu22_check_roi_parameter(roi) != MPP_OK) {
        return MPP_OK;
    }

    width = hw_cfg->width;
    height = hw_cfg->height;
    ctu = &hw_cfg->ctu;
    map_width = MPP_ALIGN(width, 64) >> 6;
//   map_height = MPP_ALIGN(height, 64) >> 6;
    memset(&local_roi, 0, sizeof(MppEncH265RoiCfg));


    for (i = 0; i < roi->num; i++) {
        // check ROI's Level
        region = &roi->region[i];
        if (region->level > H265E_MAX_ROI_LEVEL) {
            return MPP_NOK;
        }

        // check ROI's region, the ROI's region is used CTU as a Unit.
        rect = &(region->rect);
        if (rect->left < 0 || rect->top < 0) {
            return MPP_NOK;
        }

        if (rect->left > rect->right) {
            return MPP_NOK;
        }

        if (rect->top > rect->bottom) {
            return MPP_NOK;
        }

        if ((rect->left >= width) || (rect->top >= height)) {
            mpp_err("rect->left = %d,rect->top = %d\n", rect->left, rect->top);
            mpp_err("rect->left must less than %d,rect->top must less than %d\n", width, height);
            return MPP_NOK;
        }
        hal_h265e_dbg_input("rect->method = %d\n", roi->method);
        if (roi->method == H264E_METHOD_COORDINATE) {
            if ((rect->right > (width + H265E_CTB_SIZE - 1)) ||
                (rect->bottom > (height + H265E_CTB_SIZE - 1))) {
                mpp_err("rect->right = %d,rect->top = %d\n", rect->right, rect->bottom);
                mpp_err("rect->right must less than %d,rect->bottom must less than %d\n",
                        (width + H265E_CTB_SIZE - 1), (height + H265E_CTB_SIZE - 1));
                return MPP_NOK;
            }

            // covert to unit of ctu
            hal_h265e_dbg_input("rect->left = %d,rect->right = %d,rect->top = %d,rect->bottom = %d\n",
                                rect->left, rect->right, rect->top, rect->bottom);
            left = (RK_S32)((rect->left + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            right = (RK_S32)((rect->right + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            top = (RK_S32)((rect->top + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            bottom = (RK_S32)((rect->bottom + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
        } else {
            left = rect->left;
            right = rect->right;
            top = rect->top;
            bottom = rect->bottom;
        }
        region1 = &local_roi.region[roi_number];
        region1->level = region->level;
        region1->rect.left = left;
        region1->rect.right = right;
        region1->rect.top = top;
        region1->rect.bottom = bottom;

        roi_number ++;
    }

    if (roi_number > 0) {
        if (ctx->roi == NULL) {
            ret = mpp_buffer_get(ctx->buf_grp, &ctx->roi, H265E_MAX_CTU_NUM);
            if (ctx->roi == NULL) {
                mpp_err("failed to get buffer for roi buffer ret %d\n", ret);
                return MPP_NOK;
            }
        }

        region_buf = mpp_buffer_get_ptr(ctx->roi);
        memset(region_buf, 0, H265E_MAX_CTU_NUM);

        //set roi map. roi_entry[i] has higher priority than roi_entry[i+1]

        hal_h265e_dbg_func("roi_number = %d\n", roi_number);
        for (roi_id = (roi_number - 1); roi_id >= 0; roi_id--) {
            region1 = &local_roi.region[roi_id];
            rect = &region1->rect;
            for (y = rect->top; y <= rect->bottom; y++) {
                for (x = rect->left; x <= rect->right; x++) {
                    /*
                       * every roi_entry[i] mean a ctu, if a ctu belong to a roi,
                       * then set roi's level to buffer
                       */
                    region_buf[y * map_width + x] = (RK_U8)region1->level;
                }
            }
        }

        ctu->roi_enable = 1;
        ctu->map_endian = vepu22_get_endian(H265E_BIG_ENDIAN);
        ctu->map_stride = map_width;
        ctu->roi_delta_qp = roi->delta_qp;

        if (hw_cfg->hvs_qp_enable == 1) {
            hw_cfg->hvs_qp_enable = 0;
            hal_h265e_dbg_func("roi_enable and hvs_qp_enable can't both 1,close hvs_qp_enable");
        }

    } else {
        ctu->roi_enable = 0;
    }

    hw_cfg->cfg_mask |= H265E_CFG_RC_PARAM_CHANGE;
    hw_cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_set_roi(HalH265eCtx* ctx, void *param)
{
    MppEncH265RoiCfg *roi = (MppEncH265RoiCfg*)param;
    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;
    /*
     * if already init, do not change value of roi_enable.
     * ctu_qp_enable only can be set before encoder init.
     * if encoder already init, can only change the ctu's region and qp value
     */
    if (ctx->init == 1) {
        if (roi->num == 0 && hw_cfg->ctu.roi_enable == 1) {
            return MPP_NOK;
        } else if (roi->num == 1 && hw_cfg->ctu.roi_enable == 0) {
            return MPP_NOK;
        }

        return vepu22_set_roi_region_parameter(ctx, param);
    } else {
        return vepu22_set_roi_region_parameter(ctx, param);
    }
}

/*
 * set ctu's qp map
 * user can set some regions's qp, the regions use ctu size as a unit
 * ctu map is a buffer cotain define qp value. The buffer index is
 * the coordinates with CTU as the unit. For example:
 * there are two ctu' region, the value of qp are 51 and 10,
 * this function first convert the region's pixel coordinates to
 * CTU coordinates, and set those ctu's qp value.
 *****************************************************************
 *   *   *   *   *   *   *   *   *   *   *   *   * 51* 51* 51* 51*
 *****************************************************************
 *   *   *   *   *10 *10 *10 *10 *10 *   *   *   * 51* 51* 51* 51*
 *****************************************************************
 *   *   *   *   *10 *10 *10 *10 *10 *   *   *   * 51* 51* 51* 51*
 *****************************************************************
 *   *   *   *   *10 *10 *10 *10 *10 *   *   *   *   *   *   *   *
 *****************************************************************
 *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *   *
 *****************************************************************
 *
 * NOTE: ctu qp enable can't set when Rate Control is disable
 */
static MPP_RET vepu22_set_ctu_parameter(HalH265eCtx* ctx, void *param)
{
    MppEncH265CtuCfg *ctu_region = (MppEncH265CtuCfg*)param;
    H265e_CTU *ctu_cfg = NULL;
    H265eRect* rect = NULL;
    RK_S32 i = 0;
    RK_S32 x = 0, y = 0;
    RK_S32 width, height;
    RK_S32 map_width;//, map_height;
    RK_U8 *ctu_qp_buf = NULL;
    H265eCtu* ctu = NULL;
    MPP_RET ret = MPP_OK;
    RK_S32 valid = 0;
    RK_S32 left = 0, right = 0;
    RK_S32 top = 0, bottom = 0;
    HalH265eCfg *hw_cfg = NULL;

    hal_h265e_dbg_func("enter\n");
    if (ctx == NULL || param == NULL) {
        mpp_err_f("error: parameter is invalid");
        return MPP_NOK;
    }

    if (ctx->ctu == NULL) {
        ret = mpp_buffer_get(ctx->buf_grp, &ctx->ctu, H265E_MAX_CTU_NUM);
        if (ctx->ctu == NULL) {
            mpp_err("failed to get buffer for roi buffer ret %d\n", ret);
            return MPP_NOK;
        }
    }
    hw_cfg = (HalH265eCfg *)ctx->hw_cfg;

    ctu_qp_buf = mpp_buffer_get_ptr(ctx->ctu);
    if (ctu_qp_buf == NULL) {
        mpp_err_f("error: ctu_qp_buf is NULL");
        return MPP_NOK;
    }

    memset(ctu_qp_buf, hw_cfg->intra_qp, H265E_MAX_CTU_NUM);

    width = hw_cfg->width;
    height = hw_cfg->height;
    map_width = MPP_ALIGN(width, 64) >> 6;
    ctu_cfg = &hw_cfg->ctu;
//   map_height = MPP_ALIGN(height, 64) >> 6;

    for (i = 0; i < ctu_region->num; i++) {
        // check ROI's region, the ROI's region is used CTU as a Unit.
        ctu = &(ctu_region->ctu[i]);
        rect = &(ctu->rect);
        if (rect->left < 0 || rect->top < 0) {
            return MPP_NOK;
        }

        if (rect->left > rect->right) {
            return MPP_NOK;
        }

        if (rect->top > rect->bottom) {
            return MPP_NOK;
        }

        if ((rect->left >= width) || (rect->top >= height)) {
            mpp_err_f("rect->left = %d,rect->top = %d\n", rect->left, rect->top);
            mpp_err_f("rect->left must less than %d,rect->top must less than %d\n",
                      width, height);
            return MPP_NOK;
        }


        if (ctu_region->method == H264E_METHOD_COORDINATE) {
            if ((rect->right > (width + H265E_CTB_SIZE - 1)) ||
                (rect->bottom > (height + H265E_CTB_SIZE - 1))) {
                mpp_err_f("rect->right = %d,rect->top = %d\n", rect->right, rect->bottom);
                mpp_err_f("rect->right must less than %d,rect->bottom must less than %d\n",
                          (width + H265E_CTB_SIZE - 1), (height + H265E_CTB_SIZE - 1));
                return MPP_NOK;
            }

            /*
             * if method = H264E_METHOD_COORDINATE,then the value of rect is
             * in coordinates unit, so we must translate the values to ctu size
             */
            left = (RK_S32)((rect->left + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            right = (RK_S32)((rect->right + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            top = (RK_S32)((rect->top + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
            bottom = (RK_S32)((rect->bottom + H265E_CTB_SIZE - 1) >> H265E_LOG2_CTB_SIZE);
        } else {
            left = rect->left;
            right = rect->right;
            top = rect->top;
            bottom = rect->bottom;
        }

        for (y = top; y <= bottom; y++) {
            for (x = left; x <= right; x++) {
                ctu_qp_buf[y * map_width + x] = ctu->qp;
                valid = 1;
            }
        }
    }

    if (valid > 0) {
        ctu_cfg->ctu_qp_enable = 1;
        ctu_cfg->map_endian = vepu22_get_endian(H265E_BIG_ENDIAN);
        ctu_cfg->map_stride = map_width;
        hal_h265e_dbg_func("ctu_cfg->ctu_qp_enable = 1\n");
    } else {
        ctu_cfg->ctu_qp_enable = 0;
    }

    hw_cfg->cfg_mask |= H265E_CFG_PARAM_CHANGE;
    hw_cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_set_ctu(HalH265eCtx* ctx, void *param)
{
    MppEncH265CtuCfg *ctu = (MppEncH265CtuCfg*)param;
    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;
    /*
     * if already init, do not change value of ctu_qp_enable.
     * ctu_qp_enable only can be set before encoder init.
     * if encoder already init, can only change the ctu's region and qp value
     */
    if (ctx->init == 1) {
        if (ctu->num == 0 && hw_cfg->ctu.ctu_qp_enable == 1) {
            return MPP_NOK;
        } else if (ctu->num == 1 && hw_cfg->ctu.ctu_qp_enable == 0) {
            return MPP_NOK;
        }

        return vepu22_set_ctu_parameter(ctx, param);
    } else {
        return vepu22_set_ctu_parameter(ctx, param);
    }
}

static MPP_RET vepu22_check_rc_parameter(HalH265eCtx *ctx)
{
    hal_h265e_dbg_func("enter\n");

    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;

    if (hw_cfg->min_qp < 0 || hw_cfg->min_qp > 51) {
        mpp_err_f("error: hw_cfg->min_qp = %d is invalid", hw_cfg->min_qp);
        return MPP_NOK;
    }

    if (hw_cfg->max_qp < 0 || hw_cfg->max_qp > 51) {
        mpp_err_f("error: hw_cfg->max_qp = %d is invalid", hw_cfg->max_qp);
        return MPP_NOK;
    }

    if (hw_cfg->initial_rc_qp < 0 || hw_cfg->initial_rc_qp > 63) {
        mpp_err_f("error: syntax->inital_rc_qp = %d is invalid\n",
                  hw_cfg->initial_rc_qp);
        return MPP_NOK;
    }

    if ((hw_cfg->initial_rc_qp < 52) &&
        ((hw_cfg->initial_rc_qp > hw_cfg->max_qp) ||
         (hw_cfg->initial_rc_qp < hw_cfg->min_qp))) {
        mpp_err_f("error: nital_rc_qp < 52 must between min_qp = %d...max_qp = %d\n",
                  hw_cfg->min_qp, hw_cfg->max_qp);
        return MPP_NOK;
    }

    if (hw_cfg->intra_qp_offset < -10 || hw_cfg->intra_qp_offset > 10) {
        mpp_err_f("error: intra_qp_offset = %d must between [-10,10]",
                  hw_cfg->intra_qp_offset);
        return MPP_NOK;
    }

    if (hw_cfg->cu_level_rc_enable != 1 && hw_cfg->cu_level_rc_enable != 0) {
        mpp_err_f("error: cu_level_rc_enable = %d is invalid",
                  hw_cfg->cu_level_rc_enable);
        return MPP_NOK;
    }

    if (hw_cfg->init_buf_levelx8 == 1) {
        if (hw_cfg->hvs_qp_enable != 1 && hw_cfg->hvs_qp_enable != 0) {
            mpp_err_f("error: syntax->hvs_qp_enable = %d is invalid",
                      hw_cfg->hvs_qp_enable);
            return MPP_NOK;
        }

        if (hw_cfg->hvs_qp_enable) {
            if (hw_cfg->max_delta_qp < 0 || hw_cfg->max_delta_qp > 51) {
                mpp_err_f("error: syntax->max_delta_qp = %d is invalid",
                          hw_cfg->max_delta_qp);
                return MPP_NOK;
            }

            if (hw_cfg->hvs_qp_scale_enable != 1 && hw_cfg->hvs_qp_scale_enable != 0) {
                mpp_err_f("error: syntax->hvs_qp_scale_enable = %d is invalid",
                          hw_cfg->hvs_qp_scale_enable);
                return MPP_NOK;
            }

            if (hw_cfg->hvs_qp_scale_enable == 1) {
                if (hw_cfg->hvs_qp_scale < 0 || hw_cfg->hvs_qp_scale > 4) {
                    mpp_err_f("error: syntax->hvs_qp_scale = %d is invalid",
                              hw_cfg->hvs_qp_scale);
                    return MPP_NOK;
                }
            }
        }
    }

    //  if (syntax->bit_alloc_mode < 0 && param->bit_alloc_mode > 2)
    //     return RETCODE_INVALID_PARAM;

    if (hw_cfg->init_buf_levelx8 < 0 || hw_cfg->init_buf_levelx8 > 8) {
        mpp_err_f("error: syntax->init_buf_level = %d must between [0,8]",
                  hw_cfg->init_buf_levelx8);
        return MPP_NOK;
    }

    if (hw_cfg->initial_delay < 10 || hw_cfg->initial_delay > 3000 ) {
        mpp_err_f("error: syntax->inital_delay = %d must between [10,3000]",
                  hw_cfg->initial_delay);
        return MPP_NOK;
    }

    if (hw_cfg->ctu.roi_enable && hw_cfg->rc_enable == 0) {
        mpp_err_f("error: roi_enable = 1 must when rc_enable = 1,close roi_enable");
        hw_cfg->ctu.roi_enable = 0;
    }

    if (hw_cfg->ctu.roi_enable && hw_cfg->rc_enable && hw_cfg->hvs_qp_enable > 1) {
        // can not use both ROI and hvsQp
        mpp_err_f("error: roi and hvs_qp_enable can't use both,close roi_enable");
        hw_cfg->ctu.roi_enable = 0;
    }

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_check_encode_parameter(HalH265eCtx* ctx)
{
    RK_S32 intra_period_gop_step_size = 0;
    RK_U32 i = 0;
    RK_S32 low_delay = 0;
    MPP_RET ret = MPP_OK;

    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;
    hal_h265e_dbg_func("in\n");

    hal_h265e_dbg_func("cfg = %p,cfg->gop_idx = %d\n", hw_cfg, hw_cfg->gop_idx);
    static uint32_t presetGopSize[] = {
        1,  /* Custom GOP, Not used */
        1,  /* All Intra */
        1,  /* IPP Cyclic GOP size 1 */
        1,  /* IBB Cyclic GOP size 1 (not support)*/
        2,  /* IBP Cyclic GOP size 2 */
        4,  /* IBBBP (not support)*/
        4,
        4,
        8,
    };
    hal_h265e_dbg_func("hw_cfg->gop_idx = %d\n", hw_cfg->gop_idx);
    // check low-delay gop structure
    if (hw_cfg->gop_idx == PRESET_IDX_CUSTOM_GOP) { // common gop
        RK_U32 min_offset = 0;
        if (hw_cfg->gop.custom_gop_size > 1) {
            min_offset = hw_cfg->gop.pic[0].offset;
            low_delay = 1;
            for (i = 1; i < hw_cfg->gop.custom_gop_size; i++) {
                if (min_offset > hw_cfg->gop.pic[i].offset) {
                    low_delay = 0;
                    break;
                } else {
                    min_offset = hw_cfg->gop.pic[i].offset;
                }
            }
        }
    } else if (hw_cfg->gop_idx == PRESET_IDX_IPP ||
               hw_cfg->gop_idx == PRESET_IDX_IPPPP) {
        // low-delay case
        low_delay = 1;
    }
    hal_h265e_dbg_func("low_delay = %d,hw_cfg->gop_idx = %d\n",
                       low_delay, hw_cfg->gop_idx);
    if (low_delay) {
        intra_period_gop_step_size = 1;
    } else {
        if (hw_cfg->gop_idx == PRESET_IDX_CUSTOM_GOP) {
            intra_period_gop_step_size = hw_cfg->gop.custom_gop_size ;
            hal_h265e_dbg_func("PRESET_IDX_CUSTOM_GOP,intra_period_gop_step_size = %d\n",
                               intra_period_gop_step_size);
        } else {
            intra_period_gop_step_size = presetGopSize[hw_cfg->gop_idx];
            hal_h265e_dbg_func("presetGopSize[%d],intra_period_gop_step_size = %d\n",
                               hw_cfg->gop_idx, intra_period_gop_step_size);
        }
    }
    hal_h265e_dbg_func("intra_period = %d,intra_period_gop_step_size = %d\n",
                       hw_cfg->intra_period, intra_period_gop_step_size);
    hal_h265e_dbg_func("decoding_refresh_type = %d",
                       hw_cfg->decoding_refresh_type);
    if (!low_delay &&
        (hw_cfg->intra_period != 0) &&
        ((hw_cfg->intra_period % intra_period_gop_step_size) != 0)
        && (hw_cfg->decoding_refresh_type != 0)) {
        mpp_err_f("CFG FAIL : Not support intra period[%d] for the gop structure\n",
                  hw_cfg->intra_period);
        mpp_err_f("RECOMMEND CONFIG PARAMETER : Intra period = %d\n",
                  intra_period_gop_step_size * (hw_cfg->intra_period / intra_period_gop_step_size));
        return MPP_NOK;
    }

    if (!low_delay && (hw_cfg->intra_period != 0) &&
        ((hw_cfg->intra_period % intra_period_gop_step_size) == 1) &&
        (hw_cfg->decoding_refresh_type == 0)) {
        mpp_err_f("CFG FAIL : Not support decoding refresh type[%d] for closed gop structure\n",
                  hw_cfg->decoding_refresh_type);
        mpp_err_f("RECOMMEND CONFIG PARAMETER : Decoding refresh type = IDR\n");
        return MPP_NOK;
    }

    if (hw_cfg->gop_idx == PRESET_IDX_CUSTOM_GOP) {
        for (i = 0; i < hw_cfg->gop.custom_gop_size; i++) {
            if (hw_cfg->gop.pic[i].temporal_id >= H265E_MAX_NUM_TEMPORAL_LAYER) {
                mpp_err_f("CFG FAIL : temporalId %d exceeds MAX_NUM_TEMPORAL_LAYER\n", \
                          hw_cfg->gop.pic[i].temporal_id);
                mpp_err_f("RECOMMEND CONFIG PARAMETER :"
                          "Adjust temporal ID under MAX_NUM_TEMPORAL_LAYER(7) in GOP structure\n");
                return MPP_NOK;
            }
        }
    }

    if (hw_cfg->use_recommend_param == 0) {
        // Intra
        if (hw_cfg->intra_in_inter_slice_enable == 0 && hw_cfg->intra_refresh_mode != 0) {
            mpp_err_f("CFG FAIL : If intraInInterSliceEnable is '0',"
                      "Intra refresh mode must be '0'\n");
            mpp_err_f("RECOMMEND CONFIG PARAMETER : intraRefreshMode = 0\n");
            return MPP_NOK;
        }

        // RDO
        {
            int align_32_width_flag  = hw_cfg->width % 32;
            int align_16_width_flag  = hw_cfg->width % 16;
            int align_8_width_flag   = hw_cfg->width % 8;
            int align_32_height_flag = hw_cfg->height % 32;
            int align_16_height_flag = hw_cfg->height % 16;
            int align_8_height_flag  = hw_cfg->height % 8;

            if ( ((hw_cfg->cu_size_mode & 0x1) == 0) && ((align_8_width_flag != 0) || (align_8_height_flag != 0)) ) {
                mpp_err_f("CFG FAIL : Picture width and height must be aligned with 8 pixels when enable CU8x8 of cuSizeMode \n");
                mpp_err_f("RECOMMEND CONFIG PARAMETER : cuSizeMode |= 0x1 (CU8x8)\n");
                return MPP_NOK;
            } else if (((hw_cfg->cu_size_mode & 0x1) == 0) && ((hw_cfg->cu_size_mode & 0x2) == 0) &&
                       ((align_16_width_flag != 0) || (align_16_height_flag != 0)) ) {
                mpp_err_f("CFG FAIL : Picture width and height must be aligned with 16 pixels when enable CU16x16 of cuSizeMode\n");
                mpp_err_f("RECOMMEND CONFIG PARAMETER : cuSizeMode |= 0x2 (CU16x16)\n");
                return MPP_NOK;
            } else if (((hw_cfg->cu_size_mode & 0x1) == 0) && ((hw_cfg->cu_size_mode & 0x2) == 0)
                       && ((hw_cfg->cu_size_mode & 0x4) == 0) && ((align_32_width_flag != 0) || (align_32_height_flag != 0)) ) {
                mpp_err_f("CFG FAIL : Picture width and height must be aligned with 32 pixels when enable CU32x32 of cuSizeMode\n");
                mpp_err_f("RECOMMEND CONFIG PARAMETER : cuSizeMode |= 0x4 (CU32x32)\n");
                return MPP_NOK;
            }
        }

        // multi-slice & wpp
        if (hw_cfg->wpp_enable == 1 && (hw_cfg->independ_slice_mode != 0 || hw_cfg->depend_slice_mode != 0)) {
            mpp_err_f("CFG FAIL : If WaveFrontSynchro(WPP) '1', the option of multi-slice must be '0'\n");
            mpp_err_f("RECOMMEND CONFIG PARAMETER : independSliceMode = 0, dependSliceMode = 0\n");
            return MPP_NOK;
        }
    }

    // check multi slice
    {
        if (hw_cfg->independ_slice_mode  == 0 && hw_cfg->depend_slice_mode != 0) {
            mpp_err_f("CFG FAIL : If independSliceMode is '0', dependSliceMode must be '0'\n");
            mpp_err_f("RECOMMEND CONFIG PARAMETER : independSliceMode = 1, independSliceModeArg = TotalCtuNum\n");
            return MPP_NOK;
        } else if ((hw_cfg->independ_slice_mode == 1) && (hw_cfg->depend_slice_mode == 1) ) {
            if (hw_cfg->independ_slice_mode_arg < hw_cfg->depend_slice_mode_arg) {
                mpp_err_f("CFG FAIL : If independSliceMode & dependSliceMode is both '1'(multi-slice with ctu count),"
                          "must be independSliceModeArg >= dependSliceModeArg\n");
                mpp_err_f("RECOMMEND CONFIG PARAMETER : dependSliceMode = 0\n");
                return MPP_NOK;
            }
        }

        if (hw_cfg->independ_slice_mode != 0) {
            if (hw_cfg->independ_slice_mode_arg > 65535) {
                mpp_err_f("CFG FAIL : If independSliceMode is not 0,"
                          "must be independSliceModeArg <= 0xFFFF\n");
                return MPP_NOK;
            }
        }

        if (hw_cfg->depend_slice_mode != 0) {
            if (hw_cfg->depend_slice_mode_arg > 65535) {
                mpp_err_f("CFG FAIL : If dependSliceMode is not 0,"
                          "must be dependSliceModeArg <= 0xFFFF\n");
                return MPP_NOK;
            }
        }
    }

    if (hw_cfg->conf_win_top % 2) {
        mpp_err_f("CFG FAIL : conf_win_top : %d value is not available."
                  "The value should be equal to multiple of 2.\n", hw_cfg->conf_win_top);
        return MPP_NOK;
    }

    if (hw_cfg->conf_win_bot % 2) {
        mpp_err_f("CFG FAIL : conf_win_bot : %d value is not available."
                  "The value should be equal to multiple of 2.\n", hw_cfg->conf_win_bot);
        return MPP_NOK;
    }

    if (hw_cfg->conf_win_left % 2) {
        mpp_err_f("CFG FAIL : conf_win_left : %d value is not available."
                  "The value should be equal to multiple of 2.\n", hw_cfg->conf_win_left);
        return MPP_NOK;
    }

    if (hw_cfg->conf_win_right % 2) {
        mpp_err_f( "CFG FAIL : conf_win_right : %d value is not available."
                   "The value should be equal to multiple of 2.\n", hw_cfg->conf_win_right);
        return MPP_NOK;
    }

    if (hw_cfg->rc_enable) {
        ret = vepu22_check_rc_parameter(ctx);
    }
    hal_h265e_dbg_func("leave\n");
    return ret;
}

static MPP_RET vepu22_set_prep_cfg(HalH265eCtx* ctx)
{
    hal_h265e_dbg_func("enter\n");

    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    /*
     * don't allow change width,height and format of input
     * if want to change these, must close h265 encoder and reopen it
     */
    if (!ctx->init) {
        hw_cfg->width = prep->width;
        hw_cfg->height = prep->height;
        hw_cfg->width_stride = prep->hor_stride;
        hw_cfg->height_stride = prep->ver_stride;
        hw_cfg->src_format = vepu22_get_yuv_format(ctx);
        hal_h265e_dbg_input("width = %d,height = %d",
                            hw_cfg->width, hw_cfg->height);
        hal_h265e_dbg_input("width_stride = %d,height_stride = %d",
                            hw_cfg->width_stride, hw_cfg->height_stride);
        hal_h265e_dbg_input("src_format = %d", hw_cfg->src_format);
    }

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_check_rc_cfg_change(HalH265eCtx* ctx, MppEncRcCfg* set)
{
    hal_h265e_dbg_func("enter");

    hal_h265e_dbg_func("set = %p\n", set);
    hal_h265e_dbg_input("rc_mode = %d,quality = %d,bps_target = %d\n",
                        set->rc_mode, set->quality, set->bps_target);
    hal_h265e_dbg_input("fps_in_flex = %d,fps_out_flex = %d\n",
                        set->fps_in_flex, set->fps_out_flex);
    hal_h265e_dbg_input("fps_in_denorm = %d,fps_out_flex = %d\n",
                        set->fps_in_num, set->fps_in_denorm);
    hal_h265e_dbg_input("fps_out_num = %d,fps_out_denorm = %d\n",
                        set->fps_out_num, set->fps_out_denorm);
    hal_h265e_dbg_input("gop = %d,skip_cnt = %d\n", set->gop, set->skip_cnt);
    if (!ctx->init) {
        set->change = MPP_ENC_RC_CFG_CHANGE_ALL;
        ctx->option |= H265E_SET_RC_CFG;
        hal_h265e_dbg_input("init = 0, cfg all");
    }

    hal_h265e_dbg_func("leave");
    return MPP_OK;
}

static MPP_RET vepu22_set_rc_cfg(HalH265eCtx* ctx)
{
    hal_h265e_dbg_func("enter\n");

    HalH265eCfg *cfg = (HalH265eCfg*)ctx->hw_cfg;
    MppEncRcCfg *rc = &ctx->set->rc;
    RK_U32  change = rc->change;
    // config parameters of rc if first open
    if (!ctx->init) {
        change = MPP_ENC_RC_CFG_CHANGE_ALL;
    }

    hal_h265e_dbg_input("rc_mode = %d,quality = %d,bps_target = %d\n",
                        rc->rc_mode, rc->quality, rc->bps_target);
    hal_h265e_dbg_input("fps_in_flex = %d,fps_out_flex = %d\n",
                        rc->fps_in_flex, rc->fps_out_flex);
    hal_h265e_dbg_input("fps_in_denorm = %d,fps_out_flex = %d\n",
                        rc->fps_in_num, rc->fps_in_denorm);
    hal_h265e_dbg_input("fps_out_num = %d,fps_out_denorm = %d\n",
                        rc->fps_out_num, rc->fps_out_denorm);
    hal_h265e_dbg_input("gop = %d,skip_cnt = %d\n", rc->gop, rc->skip_cnt);

    /* the first time to set rc cfg*/
    if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
        hal_h265e_dbg_input("FPS change\n");
        cfg->frame_rate = rc->fps_out_num / rc->fps_out_denorm;
        cfg->time_scale = cfg->frame_rate * cfg->num_units_in_tick;
        cfg->cfg_mask |= (H265E_CFG_SET_TIME_SCALE | H265E_CFG_SET_NUM_UNITS_IN_TICK);
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    int rc_change = ((change & MPP_ENC_RC_CFG_CHANGE_RC_MODE)
                     || (change & MPP_ENC_RC_CFG_CHANGE_BPS));
    if (rc_change) {
        hal_h265e_dbg_input("rc change = 0x%x\n", change);
        cfg->rc_enable = 0;
        switch (rc->rc_mode) {
        case MPP_ENC_RC_MODE_VBR:
            // if rc_mode is VBR and quality == MPP_ENC_RC_QUALITY_CQP, this mode is const QP
            if (rc->quality == MPP_ENC_RC_QUALITY_CQP) {
                cfg->rc_enable = 0; // fix qp
                cfg->bit_rate = 0;
                cfg->trans_rate = 0;
            } else {
                cfg->rc_enable = 1;
                cfg->bit_rate = rc->bps_target;
                // set trans_rate larger than bit_rate
                cfg->trans_rate = cfg->bit_rate + 10000;
            }
            break;
        case MPP_ENC_RC_MODE_CBR:
            cfg->rc_enable = 1;
            cfg->bit_rate = rc->bps_target;
            cfg->trans_rate = cfg->bit_rate;
            break;
        default:
            mpp_err_f("rc_mode = %d not support", rc->rc_mode);
        }

        cfg->cfg_mask |= (H265E_CFG_RC_PARAM_CHANGE | H265E_CFG_RC_TARGET_RATE_CHANGE
                          | H265E_CFG_RC_TRANS_RATE_CHANGE);
        //cfg->cfg_mask |=  (H265E_CFG_RC_TARGET_RATE_CHANGE | H265E_CFG_RC_MIN_MAX_QP_CHANGE);
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }


    hal_h265e_dbg_input("rc_enable = %d\n", cfg->rc_enable);
    hal_h265e_dbg_input("bit_rate = %d,trans_rate = %d\n", cfg->bit_rate, cfg->trans_rate);
    hal_h265e_dbg_func("leave,enter cfg->cfg_mask = 0x%x,cfg->cfg_option = 0x%x\n",
                       cfg->cfg_mask, cfg->cfg_option);
    return MPP_OK;
}

static MPP_RET vepu22_check_code_cfg_change(HalH265eCtx* ctx, MppEncH265Cfg* set)
{
    if (ctx == NULL || set == NULL) {
        return MPP_NOK;
    }
    hal_h265e_dbg_func("enter");
    MppEncH265Cfg* cfg = &ctx->cfg->codec.h265;
    if (!ctx->init) {
        ctx->option |= H265E_SET_CODEC_CFG;
        if (set->change & MPP_ENC_H265_CFG_INTRA_QP_CHANGE) {
            cfg->intra_qp = set->intra_qp;
        }
        hal_h265e_dbg_input("init = 0, cfg all,change = 0x%x", set->change);
    } else {
        hal_h265e_dbg_func("enter cfg->cfg_mask = 0x%x,cfg->cfg_option = 0x%x",
                           ((HalH265eCfg *)ctx->hw_cfg)->cfg_mask,
                           ((HalH265eCfg *)ctx->hw_cfg)->cfg_option);
        hal_h265e_dbg_func(" change = %d", set->change);
        hal_h265e_dbg_input("hw_cfg->intra_qp = %d,const_intra_pred = %d",
                            set->intra_qp, set->const_intra_pred);
        hal_h265e_dbg_input("intra_refresh_mode = %d,intra_refresh_arg = %d\n",
                            set->intra_refresh_mode, set->intra_refresh_arg);
        hal_h265e_dbg_input("independ_slice_mode = %d,independ_slice_mode_arg = %d\n",
                            set->independ_slice_mode, set->independ_slice_arg);
        hal_h265e_dbg_input("depend_slice_mode = %d,depend_slice_mode_arg = %d\n",
                            set->depend_slice_mode, set->depend_slice_arg);

        if (set->change & MPP_ENC_H265_CFG_INTRA_QP_CHANGE) {
            cfg->intra_qp = set->intra_qp;
        }

        if (set->change & MPP_ENC_H265_CFG_RC_QP_CHANGE) {
            cfg->max_qp = set->max_qp;
            cfg->min_qp = set->min_qp;
            cfg->max_delta_qp = set->max_delta_qp;
        }

        if (set->change & MPP_ENC_H265_CFG_INTRA_REFRESH_CHANGE) {
            cfg->intra_refresh_mode = set->intra_refresh_mode;
            cfg->intra_refresh_arg = set->intra_refresh_arg;
        }

        if (set->change & MPP_ENC_H265_CFG_INDEPEND_SLICE_CHANGE) {
            cfg->independ_slice_mode = set->independ_slice_mode;
            cfg->independ_slice_arg = set->independ_slice_arg;
        }

        if (set->change & MPP_ENC_H265_CFG_DEPEND_SLICE_CHANGE) {
            cfg->depend_slice_mode = set->depend_slice_mode;
            cfg->depend_slice_arg = set->depend_slice_arg;
        }

        cfg->change = set->change;
    }
    hal_h265e_dbg_func("leave");
    return MPP_OK;
}

static MPP_RET vepu22_set_codec_cfg(HalH265eCtx* ctx)
{
    MppEncH265Cfg* codec = &ctx->cfg->codec.h265;
    HalH265eCfg* cfg = (HalH265eCfg*)ctx->hw_cfg;
    RK_U32 change = codec->change;
    // config parameters of rc if first open
    if (!ctx->init) {
        change = MPP_ENC_H265_CFG_CHANGE_ALL;
    }

    hal_h265e_dbg_func("enter change = %d", codec->change);
    hal_h265e_dbg_input("hw_cfg->intra_qp = %d,const_intra_pred = %d",
                        cfg->intra_qp, codec->const_intra_pred);
    hal_h265e_dbg_input("intra_refresh_mode = %d,intra_refresh_arg = %d\n",
                        codec->intra_refresh_mode, codec->intra_refresh_arg);
    hal_h265e_dbg_input("independ_slice_mode = %d,independ_slice_mode_arg = %d\n",
                        codec->independ_slice_mode, codec->independ_slice_arg);
    hal_h265e_dbg_input("depend_slice_mode = %d,depend_slice_mode_arg = %d\n",
                        codec->depend_slice_mode, codec->depend_slice_arg);

    if (change & MPP_ENC_H265_CFG_INTRA_REFRESH_CHANGE) {
        cfg->intra_refresh_mode = codec->intra_refresh_mode;
        cfg->intra_refresh_arg = codec->intra_refresh_arg;
        cfg->intra_in_inter_slice_enable = (cfg->intra_refresh_mode != 0) ? 1 : 0;

        cfg->cfg_mask |= (H265E_CFG_PARAM_CHANGE | H265E_CFG_INTRA_REFRESH_CHANGE);
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    if (change & MPP_ENC_H265_CFG_INDEPEND_SLICE_CHANGE) {
        cfg->independ_slice_mode = codec->independ_slice_mode;
        cfg->independ_slice_mode_arg  = codec->independ_slice_arg;

        cfg->cfg_mask |= H265E_CFG_INDEPENDENT_SLICE_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    if (change & MPP_ENC_H265_CFG_DEPEND_SLICE_CHANGE) {
        cfg->depend_slice_mode = codec->depend_slice_mode;
        cfg->depend_slice_mode_arg = codec->depend_slice_arg;

        cfg->cfg_mask |= H265E_CFG_DEPENDENT_SLICE_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    if (change & MPP_ENC_H265_CFG_DEPEND_SLICE_CHANGE) {
        cfg->depend_slice_mode = codec->depend_slice_mode;
        cfg->depend_slice_mode_arg = codec->depend_slice_arg;

        cfg->cfg_mask |= H265E_CFG_DEPENDENT_SLICE_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }
    cfg->sao_enable = codec->sao_enable;
    if (change & MPP_ENC_H265_CFG_FRAME_RATE_CHANGE) {
        cfg->time_scale = cfg->frame_rate * 1000;
        cfg->cfg_mask |= H265E_CFG_FRAME_RATE_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    cfg->initial_rc_qp = cfg->max_qp;
    cfg->const_intra_pred_flag = codec->const_intra_pred;

    if (change & MPP_ENC_H265_CFG_INTRA_QP_CHANGE) {
        hal_h265e_dbg_input("intra qp %d change %d", cfg->intra_qp, codec->intra_qp);
        cfg->intra_qp = codec->intra_qp;

        cfg->cfg_mask |= H265E_CFG_INTRA_PARAM_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }

    if (change & MPP_ENC_H265_CFG_RC_QP_CHANGE) {
        cfg->max_qp = codec->max_qp;
        cfg->min_qp = codec->min_qp;
        cfg->max_delta_qp = codec->max_delta_qp;

        cfg->cfg_mask |= H265E_CFG_RC_MIN_MAX_QP_CHANGE;
        cfg->cfg_option |= H265E_PARAM_CHANEGED_COMMON;
    }
    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_set_gop_cfg(HalH265eCtx* ctx)
{
    hal_h265e_dbg_func("enter\n");

    HalH265eCfg* hw_cfg = (HalH265eCfg*)ctx->hw_cfg;
    MppEncRcCfg* rc = &ctx->set->rc;
    MppEncH265Cfg* codec = &ctx->cfg->codec.h265;

    hal_h265e_dbg_input("gop = %d,const_intra_pred = %d\n",
                        rc->gop, codec->const_intra_pred);
    hal_h265e_dbg_input("intra_qp = %d,gop_delta_qp = %d\n",
                        codec->intra_qp, codec->gop_delta_qp);
    if (!ctx->init || (rc->change & MPP_ENC_RC_CFG_CHANGE_GOP) ||
        (codec->change & MPP_ENC_H265_CFG_INTRA_QP_CHANGE)) {
        hw_cfg->cfg_mask |= (H265E_CFG_INTRA_PARAM_CHANGE | H265E_CFG_GOP_PARAM_CHANGE);
        hal_h265e_dbg_input("%d,hw_cfg->cfg_mask = %d\n", __LINE__, hw_cfg->cfg_mask);
        hw_cfg->cfg_option |= (H265E_PARAM_CHANEGED_COMMON | H265E_PARAM_CHANEGED_CUSTOM_GOP);

        hw_cfg->intra_period = rc->gop;

        /* not change custom_gop_size */
        // hw_cfg->gop.custom_gop_size = H265E_MAX_GOP_NUM;
        //(rc->gop > H265E_MAX_GOP_NUM) ? H265E_MAX_GOP_NUM : rc->gop;
        if (hw_cfg->intra_period == 1) {
            hw_cfg->gop_idx = PRESET_IDX_ALL_I;
        } else {
            hw_cfg->gop_idx = PRESET_IDX_CUSTOM_GOP;
        }

        hal_h265e_dbg_input("%d,hw_cfg->cfg_mask = %d\n", __LINE__, hw_cfg->cfg_mask);
        hal_h265e_dbg_input("hw_cfg->gop_idx= %d,hw_cfg->gop.custom_gop_size = %d\n",
                            hw_cfg->gop_idx, hw_cfg->gop.custom_gop_size);
        if (hw_cfg->gop_idx == PRESET_IDX_CUSTOM_GOP) {
            RK_U32 i = 0;
            for (i = 0; i < hw_cfg->gop.custom_gop_size; i++) {
                if (i == 0) {
                    hw_cfg->gop.pic[i].ref_poc_l0 = 0;
                } else {
                    hw_cfg->gop.pic[i].ref_poc_l0 = i - 1;
                }
                hw_cfg->gop.pic[i].type  = H265E_PIC_TYPE_P;
                //not set pocOffset to 0,because the first I frame's will be take the pocOffset 0
                hw_cfg->gop.pic[i].offset      = i + 1;
                hw_cfg->gop.pic[i].qp          = codec->intra_qp + codec->gop_delta_qp;
                hw_cfg->gop.pic[i].ref_poc_l1  = 0;
                hw_cfg->gop.pic[i].temporal_id = 0;
                hw_cfg->gop.gop_pic_lambda[i]  = 0;
            }
        }

    }

    hal_h265e_dbg_input("gop_idx = %d,intra_period = %d\n",
                        hw_cfg->gop_idx, hw_cfg->intra_period);
    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET vepu22_dump_cfg(HalH265eCtx* ctx)
{
    HalH265eCfg* hw_cfg = (HalH265eCfg*)ctx->hw_cfg;
    hal_h265e_dbg_func("enter\n");

    hal_h265e_dbg_input("profile = %d,level = %d,tier = %d\n",
                        hw_cfg->profile, hw_cfg->level, hw_cfg->tier);
    hal_h265e_dbg_input("chroma_idc = %d\n", hw_cfg->chroma_idc);
    hal_h265e_dbg_input("width = %d,height = %d\n", hw_cfg->width, hw_cfg->height);
    hal_h265e_dbg_input("width_stride = %d,height_stride = %d\n",
                        hw_cfg->width_stride, hw_cfg->height_stride);
    hal_h265e_dbg_input("bit_depth = %d,src_format = %d,src_endian = %d\n",
                        hw_cfg->bit_depth, hw_cfg->src_format, hw_cfg->src_endian );
    hal_h265e_dbg_input("bs_endian = %d,fb_endian = %d\n",
                        hw_cfg->bs_endian, hw_cfg->fb_endian);
    hal_h265e_dbg_input("frame_rate = %d,frame_skip = %d\n",
                        hw_cfg->frame_rate, hw_cfg->frame_skip);
    hal_h265e_dbg_input("111 bit_rate = %d,trans_rate = %d\n",
                        hw_cfg->bit_rate, hw_cfg->trans_rate);
    hal_h265e_dbg_input("fbc_map_type = %d,line_buf_int_en = %d\n",
                        hw_cfg->fbc_map_type, hw_cfg->line_buf_int_en);
    hal_h265e_dbg_input("slice_int_enable = %d,ring_buffer_enable = %d\n",
                        hw_cfg->slice_int_enable, hw_cfg->ring_buffer_enable);
    hal_h265e_dbg_input("lossless_enable = %d,const_intra_pred_flag = %d\n",
                        hw_cfg->lossless_enable, hw_cfg->const_intra_pred_flag);
    hal_h265e_dbg_input("chroma_cb_qp_offset = %d,chroma_cr_qp_offset = %d\n",
                        hw_cfg->chroma_cb_qp_offset, hw_cfg->chroma_cr_qp_offset);
    hal_h265e_dbg_input("gop_idx = %d,decoding_refresh_type = %d\n",
                        hw_cfg->gop_idx, hw_cfg->decoding_refresh_type);
    hal_h265e_dbg_input("intra_qp = %d,intra_period = %d\n",
                        hw_cfg->intra_qp, hw_cfg->intra_period);
    hal_h265e_dbg_input("conf_win_top = %d,conf_win_bot = %d,conf_win_left = %d,conf_win_right = %d\n",
                        hw_cfg->conf_win_top, hw_cfg->conf_win_bot, hw_cfg->conf_win_left, hw_cfg->conf_win_right);
    hal_h265e_dbg_input("independ_slice_mode = %d,independ_slice_mode_arg = %d\n",
                        hw_cfg->independ_slice_mode, hw_cfg->independ_slice_mode_arg);
    hal_h265e_dbg_input("depend_slice_mode = %d,depend_slice_mode_arg = %d\n",
                        hw_cfg->depend_slice_mode, hw_cfg->depend_slice_mode_arg);
    hal_h265e_dbg_input("intra_refresh_mode = %d,intra_refresh_arg = %d\n",
                        hw_cfg->intra_refresh_mode, hw_cfg->intra_refresh_arg);
    hal_h265e_dbg_input("use_recommend_param = %d,scaling_list_enable = %d\n",
                        hw_cfg->use_recommend_param, hw_cfg->scaling_list_enable);
    hal_h265e_dbg_input("cu_size_mode = %d,tmvp_enable = %d\n",
                        hw_cfg->cu_size_mode, hw_cfg->tmvp_enable);
    hal_h265e_dbg_input("wpp_enable = %d,max_num_merge = %d\n",
                        hw_cfg->wpp_enable, hw_cfg->max_num_merge);
    hal_h265e_dbg_input("dynamic_merge_8x8_enable = %d,dynamic_merge_16x16_enable = %d\n",
                        hw_cfg->dynamic_merge_8x8_enable, hw_cfg->dynamic_merge_16x16_enable,
                        hw_cfg->dynamic_merge_32x32_enable);
    hal_h265e_dbg_input("disable_deblk = %d,lf_cross_slice_boundary_enable = %d\n",
                        hw_cfg->disable_deblk, hw_cfg->lf_cross_slice_boundary_enable);
    hal_h265e_dbg_input("beta_offset_div2 = %d,tc_offset_div2 = %d\n",
                        hw_cfg->beta_offset_div2, hw_cfg->tc_offset_div2);
    hal_h265e_dbg_input("skip_intra_trans = %d,sao_enable = %d\n",
                        hw_cfg->skip_intra_trans, hw_cfg->sao_enable);
    hal_h265e_dbg_input("intra_in_inter_slice_enable = %d,intra_nxn_enable = %d\n",
                        hw_cfg->intra_in_inter_slice_enable, hw_cfg->intra_nxn_enable);
    hal_h265e_dbg_input("intra_qp_offset = %d,init_buf_levelx8 = %d\n",
                        hw_cfg->intra_qp_offset, hw_cfg->init_buf_levelx8);
    hal_h265e_dbg_input("bit_alloc_mode = %d,rc_enable = %d\n",
                        hw_cfg->bit_alloc_mode, hw_cfg->rc_enable);
    hal_h265e_dbg_input("cu_level_rc_enable = %d,hvs_qp_enable = %d\n",
                        hw_cfg->cu_level_rc_enable, hw_cfg->hvs_qp_enable);
    hal_h265e_dbg_input("hvs_qp_scale_enable = %d,hvs_qp_scale = %d\n",
                        hw_cfg->hvs_qp_scale_enable, hw_cfg->hvs_qp_scale);
    hal_h265e_dbg_input("min_qp = %d,max_qp = %d,max_delta_qp = %d\n",
                        hw_cfg->min_qp, hw_cfg->max_qp, hw_cfg->max_delta_qp);
    hal_h265e_dbg_input("num_units_in_tick = %d,time_scale = %d\n",
                        hw_cfg->num_units_in_tick, hw_cfg->time_scale);
    hal_h265e_dbg_input("initial_rc_qp = %d,num_ticks_poc_diff_one = %d\n",
                        hw_cfg->initial_rc_qp, hw_cfg->num_ticks_poc_diff_one);
    hal_h265e_dbg_input("nr_noise_est_enable = %d\n",
                        hw_cfg->nr_noise_est_enable);
    hal_h265e_dbg_input("intra_min_qp = %d,intra_max_qp = %d\n",
                        hw_cfg->intra_min_qp, hw_cfg->intra_max_qp);
    hal_h265e_dbg_input("roi_enable = %d,roi_delta_qp = %d\n",
                        hw_cfg->ctu.roi_enable, hw_cfg->ctu.roi_delta_qp);
    hal_h265e_dbg_input("ctu_qp_enable = %d\n", hw_cfg->ctu.ctu_qp_enable);
    hal_h265e_dbg_input("ROI/CTU map_endian = %d,map_stride = %d\n",
                        hw_cfg->ctu.map_endian, hw_cfg->ctu.map_stride);
    hal_h265e_dbg_input("initial_delay = %d,hrd_rbsp_in_vps = %d\n",
                        hw_cfg->initial_delay, hw_cfg->hrd_rbsp_in_vps);
    hal_h265e_dbg_input("hrd_rbsp_in_vui = %d\n",
                        hw_cfg->hrd_rbsp_in_vui);
    hal_h265e_dbg_input("use_long_term = %d,use_cur_as_longterm_pic = %d,use_longterm_ref = %d\n",
                        hw_cfg->use_long_term, hw_cfg->use_cur_as_longterm_pic, hw_cfg->use_longterm_ref);
    hal_h265e_dbg_input("cfg_option = %d,cfg_mask = %d \n",
                        hw_cfg->cfg_option, hw_cfg->cfg_mask);

    hal_h265e_dbg_func("leave\n");
    return MPP_OK;
}

MPP_RET vepu22_pre_process(void *hal, HalTaskInfo *task)
{
    RK_S32 ret = MPP_NOK;

    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    HalEncTask *info = &task->enc;

    int h_stride = prep->hor_stride;
    int v_stride = prep->ver_stride;
    int size = h_stride * v_stride * 3 / 2;
    int width = prep->width;
    int height = prep->height;

    MppBuffer src_buf = info->input;
    MppBuffer dst_buf = NULL;
    MppFrame src_frm = NULL;
    MppFrame dst_frm = NULL;

    // check need pre prcoess?
    if (vepu22_need_pre_process(hal) == MPP_OK) {
        return MPP_NOK;
    }

    if (ctx->pre_buf == NULL) {
        mpp_assert(size);
        mpp_buffer_get(ctx->buf_grp, &ctx->pre_buf, size);
        hal_h265e_dbg_func("mpp_buffer_get,ctx = %p size = %d,pre fd = %d", ctx,  \
                           size, mpp_buffer_get_fd(ctx->pre_buf));
    }
    mpp_assert(ctx->pre_buf != NULL);
    dst_buf = ctx->pre_buf;

    RgaCtx rga = ctx->rga_ctx;
    mpp_assert(rga != NULL);
    ret = mpp_frame_init(&src_frm);
    if (ret) {
        mpp_err("failed to init src frame\n");
        goto END;
    }

    ret = mpp_frame_init(&dst_frm);
    if (ret) {
        mpp_err("failed to init dst frame\n");
        goto END;
    }

    mpp_frame_set_buffer(src_frm, src_buf);
    mpp_frame_set_width(src_frm, width);
    mpp_frame_set_height(src_frm, height);
    mpp_frame_set_hor_stride(src_frm, h_stride);
    mpp_frame_set_ver_stride(src_frm, v_stride);
    mpp_frame_set_fmt(src_frm, prep->format);

    mpp_frame_set_buffer(dst_frm, dst_buf);
    mpp_frame_set_width(dst_frm, width);
    mpp_frame_set_height(dst_frm, height);
    mpp_frame_set_hor_stride(dst_frm, h_stride);
    mpp_frame_set_ver_stride(dst_frm, v_stride);
    mpp_frame_set_fmt(dst_frm, MPP_FMT_YUV420SP);

    ret = rga_control(rga, RGA_CMD_INIT, NULL);
    if (ret) {
        mpp_err("rga cmd init failed %d\n", ret);
        goto END;
    }

    ret = rga_control(rga, RGA_CMD_SET_SRC, src_frm);
    if (ret) {
        mpp_err("rga cmd setup source failed %d\n", ret);
        goto END;
    }

    ret = rga_control(rga, RGA_CMD_SET_DST, dst_frm);
    if (ret) {
        mpp_err("rga cmd setup destination failed %d\n", ret);
        goto END;
    }

    ret = rga_control(rga, RGA_CMD_RUN_SYNC, NULL);
    if (ret) {
        mpp_err("rga cmd process copy failed %d\n", ret);
        goto END;
    }

    ret = MPP_OK;
END:
    if (src_frm) {
        mpp_frame_deinit(&src_frm);
    }

    if (dst_frm) {
        mpp_frame_deinit(&dst_frm);
    }

    hal_h265e_dbg_func("format convert:src YUV: %d -----> dst YUV: %d", prep->format, MPP_FMT_YUV420SP);
    return ret;
}

static MPP_RET vepu22_set_cfg(HalH265eCtx* ctx)
{
    MPP_RET ret = MPP_OK;
    hal_h265e_dbg_func("enter hal");
    if (ctx == NULL) {
        mpp_err_f("error: ctx == NULL");
        return MPP_NOK;
    }

    if (vepu22_set_prep_cfg(ctx) != MPP_OK) {
        mpp_err_f("error: h265e_init_hal_prep_cfg fail\n");
        return MPP_NOK;
    }

    if (vepu22_set_rc_cfg(ctx) != MPP_OK) {
        mpp_err_f("error: h265e_hal_set_rc_cfg fail\n");
        return MPP_NOK;
    }

    if (vepu22_set_codec_cfg(ctx) != MPP_OK) {
        mpp_err_f("error: h265e_hal_set_codec_cfg fail\n");
        return MPP_NOK;
    }

    if (vepu22_set_gop_cfg(ctx) != MPP_OK) {
        mpp_err_f("error: h265e_hal_set_gop_cfg fail\n");
        return MPP_NOK;
    }

    if (vepu22_check_encode_parameter(ctx) != MPP_OK) {
        mpp_err_f("check encode prameter fail\n");
        return MPP_NOK;
    }

    vepu22_dump_cfg(ctx);


    if (ctx->dev_ctx) {
        ret = mpp_device_send_reg_with_id(ctx->dev_ctx, H265E_SET_PARAMETER,
                                          ctx->hw_cfg, sizeof(struct hal_h265e_cfg));
        hal_h265e_dbg_input("hal_h265e_send_cmd ret = %d\n", ret);
        if (ret) {
            mpp_err_f("error: hal_h265e_set_param 0x%x fail\n", ret);
            return MPP_NOK;
        }
    }

    vepu22_clear_cfg_mask(ctx);
    hal_h265e_dbg_func("leave hal");
    return ret;
}

MPP_RET hal_h265e_vepu22_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalH265eCtx* ctx = (HalH265eCtx*)hal;

    hal_h265e_dbg_func("enter hal\n", hal);
    ctx->buf_grp = NULL;
    ctx->roi = NULL;
    ctx->ctu = NULL;
    ctx->pre_buf = NULL;

    /* pointer to cfg define in controller*/
    ctx->cfg = cfg->cfg;
    ctx->set = cfg->set;
    ctx->int_cb = cfg->hal_int_cb;
    ctx->option = H265E_SET_CFG_INIT;
    ctx->init = 0;
    ctx->rga_ctx = NULL;

    ctx->hw_cfg = mpp_calloc_size(void, sizeof(HalH265eCfg));
    if (ctx->hw_cfg == NULL) {
        mpp_err("failed to malloc ctx->hw_cfg %d\n", ctx->hw_cfg);
        ret = MPP_ERR_MALLOC;
        goto FAIL;
    }
    vepu22_set_default_hw_cfg(ctx->hw_cfg);

    ctx->en_info = mpp_calloc_size(void, sizeof(EncInfo));
    if (ctx->en_info == NULL) {
        mpp_err("failed to malloc ctx->en_info %d\n", ctx->en_info);
        ret = MPP_ERR_MALLOC;
        goto FAIL;
    }

    if (HAL_H265E_DBG_WRITE_IN_STREAM & hal_h265e_debug) {
        ctx->mInFile = fopen("/data/h265e/h265e_yuv.bin", "wb");
        if (ctx->mInFile == NULL) {
            mpp_err_f("open /data/h265e_yuv.bin fail\n");
        }
    }

    if (HAL_H265E_DBG_WRITE_OUT_STREAM & hal_h265e_debug) {
        ctx->mOutFile = fopen("/data/h265e/h265e_bs.bin", "wb");
        if (ctx->mOutFile == NULL) {
            mpp_err_f("open /data/h265e/h265e_bs.bin fail\n");
        }
    }

    ret = mpp_buffer_group_get_internal(&ctx->buf_grp, MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("failed to malloc buf_grp from ion ret %d\n", ret);
        goto FAIL;
    }

    //!< mpp_device_init
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,             /* type */
        .coding = MPP_VIDEO_CodingHEVC,  /* coding */
        .platform = 0,                   /* platform */
        .pp_enable = 0,                  /* pp_enable */
    };

    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err("mpp_device_init failed ret: %d\n", ret);
        goto FAIL;
    }
    hal_h265e_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
FAIL:
    if (ctx->hw_cfg != NULL) {
        mpp_free(ctx->hw_cfg);
        ctx->hw_cfg = NULL;
    }

    if (ctx->en_info != NULL) {
        mpp_free(ctx->en_info);
        ctx->en_info = NULL;
    }

    if (ctx->roi != NULL) {
        mpp_buffer_put(ctx->roi);
        ctx->roi = NULL;
    }

    if (ctx->ctu != NULL) {
        mpp_buffer_put(ctx->ctu);
        ctx->ctu = NULL;
    }

    if (ctx->pre_buf != NULL) {
        mpp_buffer_put(ctx->pre_buf);
        ctx->pre_buf = NULL;
    }

    if (ctx->buf_grp != NULL) {
        mpp_buffer_group_put(ctx->buf_grp);
        ctx->buf_grp = NULL;
    }

    if (ctx->mInFile != NULL) {
        fclose(ctx->mInFile);
        ctx->mInFile = NULL;
    }

    if (ctx->mOutFile != NULL) {
        fclose(ctx->mOutFile);
        ctx->mOutFile = NULL;
    }

    if (ctx->rga_ctx != NULL) {
        rga_deinit(ctx->rga_ctx);
        ctx->rga_ctx = NULL;
    }

    return MPP_NOK;
}

MPP_RET hal_h265e_vepu22_deinit(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;

    hal_h265e_dbg_func("enter hal %p\n", hal);

    if (ctx->hw_cfg != NULL) {
        mpp_free(ctx->hw_cfg);
        ctx->hw_cfg = NULL;
    }

    if (ctx->en_info != NULL) {
        mpp_free(ctx->en_info);
        ctx->en_info = NULL;
    }

    if (ctx->roi != NULL) {
        mpp_buffer_put(ctx->roi);
        ctx->roi = NULL;
    }

    if (ctx->ctu != NULL) {
        mpp_buffer_put(ctx->ctu);
        ctx->ctu = NULL;
    }

    if (ctx->pre_buf != NULL) {
        mpp_buffer_put(ctx->pre_buf);
        ctx->pre_buf = NULL;
    }
    if (ctx->buf_grp != NULL) {
        mpp_buffer_group_put(ctx->buf_grp);
        ctx->buf_grp = NULL;
    }

    if (ctx->dev_ctx) {
        mpp_device_deinit(ctx->dev_ctx);
        ctx->dev_ctx = NULL;
    }

    if (ctx->rga_ctx != NULL) {
        rga_deinit(ctx->rga_ctx);
        ctx->rga_ctx = NULL;
    }

    if (ctx->mInFile != NULL) {
        fflush(ctx->mInFile);
        fclose(ctx->mInFile);
        ctx->mInFile = NULL;
    }

    if (ctx->mOutFile != NULL) {
        fflush(ctx->mOutFile);
        fclose(ctx->mOutFile);
        ctx->mOutFile = NULL;
    }
    hal_h265e_dbg_func("leave hal %p\n", hal);
    return MPP_OK;
}

MPP_RET hal_h265e_vepu22_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;

    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    HalEncTask *info = &task->enc;
    MppBuffer input  = info->input;
    MppBuffer output = info->output;
    H265eSyntax *syntax = (H265eSyntax *)info->syntax.data;
    EncInfo* en_info = (EncInfo*)ctx->en_info;

    hal_h265e_dbg_func("enter hal %p\n", hal);
    memset(en_info, 0, sizeof(EncInfo));

    if (vepu22_pre_process(hal, task) == MPP_OK) {
        input = ctx->pre_buf;
    }
    en_info->src_addr = mpp_buffer_get_fd(input);
    en_info->src_size = mpp_buffer_get_size(input);
    if (ctx->mInFile != NULL) {
        RK_U8*buffer = mpp_buffer_get_ptr(input);
        if (buffer != NULL && en_info->src_size > 0) {
            fwrite(buffer, en_info->src_size, 1, ctx->mInFile);
            fflush(ctx->mInFile);
            hal_h265e_dbg_input("H265E: write yuv's data to file,size = %d",
                                en_info->src_size);
        }
    }

    /*
    * Minimum size of bitstream buffer : 96KBytes
    */
    en_info->bs_addr = mpp_buffer_get_fd(output);
    en_info->bs_size = mpp_buffer_get_size(output);

    vepu22_set_roi_region(ctx);
    vepu22_set_ctu_qp(ctx);
    vepu22_set_frame_type(syntax, en_info);
    hal_h265e_dbg_func("leave hal %p, frame type = %d,enable = %d\n",
                       hal, en_info->force_frame_type, en_info->force_frame_type_enable);

    return ret;
}

MPP_RET hal_h265e_vepu22_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalH265eCtx* ctx = (HalH265eCtx*)hal;

    hal_h265e_dbg_func("enter hal %p\n", hal);

    ret = mpp_device_send_reg(ctx->dev_ctx, (RK_U32*)ctx->en_info,
                              sizeof(EncInfo) / sizeof(RK_U32));
    if (ret) {
        ret = MPP_ERR_VPUHW;
        mpp_err_f("h265 encoder FlushRegs fail. \n");
    }

    hal_h265e_dbg_func("leave hal %p\n", hal);

    (void)task;
    return ret;
}

MPP_RET hal_h265e_vepu22_wait(void *hal, HalTaskInfo *task)
{
    RK_S32 ret = MPP_NOK;

    HalH265eCtx* ctx = (HalH265eCtx*)hal;
    HalEncTask *info = &task->enc;
    H265eFeedback feedback;
    H265eVepu22Result result;
    MppBuffer output = info->output;

    hal_h265e_dbg_func("enter hal %p\n", hal);

    ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32*)&result,
                              (RK_U32)(sizeof(H265eVepu22Result) / sizeof(RK_U32)));
    if (ret) {
        mpp_err_f("leave hal hardware returns error:%d\n", ret);
        return MPP_ERR_VPUHW;
    }

    info->is_intra = 0;
    feedback.status = result.fail_reason;
    if (feedback.status == 0) {
        void* buffer = NULL;
        int size = 0;

        feedback.bs_size = result.bs_size;
        feedback.pic_type = result.pic_type;
        feedback.avg_ctu_qp = result.avg_ctu_qp;
        feedback.gop_idx = result.gop_idx;
        feedback.poc = result.poc;
        feedback.src_idx = result.src_idx;
        feedback.enc_pic_cnt = result.enc_pic_cnt;

        if (feedback.pic_type == H265E_PIC_TYPE_I ||
            feedback.pic_type == H265E_PIC_TYPE_IDR ||
            feedback.pic_type == H265E_PIC_TYPE_CRA) {
            info->is_intra = 1;
        } else {
            info->is_intra = 0;
        }
        buffer = mpp_buffer_get_ptr(output);
        size = result.bs_size;
        if (ctx->mOutFile != NULL && size > 0) {
            fwrite(buffer, size, 1, ctx->mOutFile);
            fflush(ctx->mOutFile);
            hal_h265e_dbg_input("H265E: write data bs to file,size = %d", size);
        }
    } else {
        // error
        feedback.bs_size = 0;
    }

    ctx->int_cb.callBack(ctx->int_cb.opaque, &feedback);
    task->enc.length = feedback.bs_size;
    hal_h265e_dbg_func("leave hal %p,status = %d,size = %d\n",
                       hal, feedback.status, feedback.bs_size);

    return MPP_OK;
}

MPP_RET hal_h265e_vepu22_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalH265eCtx* ctx = (HalH265eCtx*)hal;

    hal_h265e_dbg_func("enter hal %p\n", hal);

    if (ctx->dev_ctx) {
        ret = mpp_device_send_reg_with_id(ctx->dev_ctx, H265E_RESET, NULL, 0);
        if (ret) {
            ret = MPP_ERR_VPUHW;
            mpp_err_f("failed to send reset cmd\n");
        }
    }

    hal_h265e_dbg_func("leave hal %p\n", hal);

    return ret;
}

MPP_RET hal_h265e_vepu22_flush(void *hal)
{
    HalH265eCtx* ctx = (HalH265eCtx*)hal;

    hal_h265e_dbg_func("enter hal %p\n", hal);
    hal_h265e_dbg_func("leave hal %p\n", hal);

    (void)ctx;
    return MPP_OK;
}

MPP_RET hal_h265e_vepu22_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = 0;

    HalH265eCtx *ctx = (HalH265eCtx*)hal;
    HalH265eCfg *hw_cfg = (HalH265eCfg *)ctx->hw_cfg;

    hal_h265e_dbg_func("enter hal %p,cmd = %d\n", hal, cmd_type);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
        hal_h265e_dbg_input("MPP_ENC_SET_EXTRA_INFO\n");
        break;
    }

    case MPP_ENC_GET_EXTRA_INFO: {
        hal_h265e_dbg_input("MPP_ENC_GET_EXTRA_INFO\n");
        MppPacket *pkt_out  = (MppPacket *)param;
        *pkt_out = NULL;
        break;
    }

    case MPP_ENC_SET_PREP_CFG: {
        MppEncPrepCfg *set = &ctx->set->prep;
        hal_h265e_dbg_input("MPP_ENC_SET_PREP_CFG: change = %d\n",
                            set->change);
        ret = vepu22_check_prep_parameter(set);
        ctx->option |= H265E_SET_PREP_CFG;
        break;
    }

    case MPP_ENC_SET_RC_CFG : {
        MppEncRcCfg* rc = (MppEncRcCfg*)&ctx->set->rc;
        hal_h265e_dbg_input("MPP_ENC_SET_RC_CFG,change = %d,rc.gop = %d\n",
                            rc->change, rc->gop);
        ret = vepu22_check_rc_cfg_change(ctx, rc);
        change = rc->change;
        break;
    }

    case MPP_ENC_SET_CODEC_CFG : {
        MppEncH265Cfg *src = &ctx->set->codec.h265;
        vepu22_check_code_cfg_change(ctx, src);
        change = ctx->cfg->codec.h265.change;
        hal_h265e_dbg_input("MPP_ENC_SET_CODEC_CFG,change = %d\n", change);
        break;
    }

    case MPP_ENC_SET_CTU_QP: {
        hal_h265e_dbg_input("MPP_ENC_SET_CTU_QP\n");
        ret = vepu22_set_ctu(ctx, param);
        break;
    }

    case MPP_ENC_SET_ROI_CFG: {
        hal_h265e_dbg_input("MPP_ENC_SET_ROI_CFG\n");
        ret = vepu22_set_roi(ctx, param);
        break;
    }
    }
    hal_h265e_dbg_input("ctx->option = 0x%x H265E_SET_ALL_CFG = 0x%x\n",
                        ctx->option, H265E_SET_ALL_CFG);
    if ((ctx->init == 0) && (ctx->option & H265E_SET_ALL_CFG) == H265E_SET_ALL_CFG) {
        ret = vepu22_set_cfg(ctx);
        ctx->option = 0;
        ctx->init = 1;
    } else if (ctx->init == 1) {
        if ((hw_cfg->cfg_mask != 0) || (change != 0)) {
            vepu22_set_cfg(ctx);
        }
    }

    hal_h265e_dbg_func("leave hal %p\n", hal);

    return ret;
}


