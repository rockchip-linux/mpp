/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "h264e_com"

#include <string.h>
#include <math.h>

#include "mpp_common.h"
#include "mpp_mem.h"

#include "hal_h264e_com.h"

static const RK_S32 h264e_rkv_csp_idx_map[H264E_RKV_CSP_BUTT] = {
    H264E_CSP2_BGRA, H264E_CSP2_BGR,  H264E_CSP2_RGB,
    H264E_CSP2_NONE, H264E_CSP2_NV16, H264E_CSP2_I422,
    H264E_CSP2_NV12, H264E_CSP2_I420, H264E_CSP2_NONE,
    H264E_CSP2_NONE
};

static const RK_S32 h264e_vpu_csp_idx_map[H264E_VPU_CSP_BUTT] = {
    H264E_CSP2_I420, H264E_CSP2_NV12, H264E_CSP2_NONE,
    H264E_CSP2_NONE, H264E_CSP2_RGB, H264E_CSP2_BGR,
    H264E_CSP2_RGB, H264E_CSP2_BGR, H264E_CSP2_RGB,
    H264E_CSP2_BGR, H264E_CSP2_RGB, H264E_CSP2_BGR,
    H264E_CSP2_RGB, H264E_CSP2_BGR,
};

/* default quant matrices */
static const RK_U8 h264e_cqm_jvt4i[16] = {
    6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42

};
static const RK_U8 h264e_cqm_jvt4p[16] = {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34

};
static const RK_U8 h264e_cqm_jvt8i[64] = {
    6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42

};
static const RK_U8 h264e_cqm_jvt8p[64] = {
    9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35

};

static const RK_U8 h264e_cqm_flat16[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16

};

const RK_U8 * const h264e_cqm_jvt[8] = {
    h264e_cqm_jvt4i, h264e_cqm_jvt4p,
    h264e_cqm_jvt4i, h264e_cqm_jvt4p,
    h264e_cqm_jvt8i, h264e_cqm_jvt8p,
    h264e_cqm_jvt8i, h264e_cqm_jvt8p

};

/* zigzags are transposed with respect to the tables in the standard */
const RK_U8 h264e_zigzag_scan4[2][16] = {
    {
        // frame
        0,  4,  1,  2,  5,  8, 12,  9,  6,  3,  7, 10, 13, 14, 11, 15
    },
    {
        // field
        0,  1,  4,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    }

};
const RK_U8 h264e_zigzag_scan8[2][64] = {
    {
        0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
        33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
        28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
        23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63

    },
    {
        0,  1,  2,  8,  9,  3,  4, 10, 16, 11,  5,  6,  7, 12, 17, 24,
        18, 13, 14, 15, 19, 25, 32, 26, 20, 21, 22, 23, 27, 33, 40, 34,
        28, 29, 30, 31, 35, 41, 48, 42, 36, 37, 38, 39, 43, 49, 50, 44,
        45, 46, 47, 51, 56, 57, 52, 53, 54, 55, 58, 59, 60, 61, 62, 63

    }

};

void h264e_rkv_set_format(H264eHwCfg *hw_cfg, MppEncPrepCfg *prep_cfg)
{
    MppFrameFormat format = (MppFrameFormat)prep_cfg->format;
    switch (format) {
    case MPP_FMT_YUV420P: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV420P;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV420SP: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV420SP;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV420SP_10BIT: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV420SP_VU: { //TODO: to be confirmed
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV420SP;
        hw_cfg->uv_rb_swap = 1;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422P: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV422P;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422SP: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV422SP;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422SP_10BIT: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422SP_VU: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUV422SP;
        hw_cfg->uv_rb_swap = 1;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422_YUYV: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_YUYV422;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_YUV422_UYVY: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_UYVY422;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_RGB565: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGR565;
        hw_cfg->uv_rb_swap = 1;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_BGR565: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGR565;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_RGB555: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_BGR555: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_RGB444: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_BGR444: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_RGB888: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGR888;
        hw_cfg->uv_rb_swap = 1;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_BGR888: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGR888;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_RGB101010: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_BGR101010: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
        break;
    }
    case MPP_FMT_ARGB8888: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGRA8888;
        hw_cfg->uv_rb_swap = 1;
        hw_cfg->alpha_swap = 1;
        break;
    }
    case MPP_FMT_ABGR8888: {
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_BGRA8888;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 1;
        break;
    }
    default: {
        h264e_hal_err("unvalid src color space: %d", format);
        hw_cfg->input_format = (RK_U32)H264E_RKV_CSP_NONE;
        hw_cfg->uv_rb_swap = 0;
        hw_cfg->alpha_swap = 0;
    }
    }
}


void h264e_vpu_set_format(H264eHwCfg *hw_cfg, MppEncPrepCfg *prep_cfg)
{
    MppFrameFormat format = (MppFrameFormat)prep_cfg->format;
    hw_cfg->input_format = 0;
    hw_cfg->r_mask_msb = 0;
    hw_cfg->g_mask_msb = 0;
    hw_cfg->b_mask_msb = 0;

    switch (format) {
    case MPP_FMT_YUV420P: {
        hw_cfg->input_format = H264E_VPU_CSP_YUV420P;
        break;
    }
    case MPP_FMT_YUV420SP: {
        hw_cfg->input_format = H264E_VPU_CSP_YUV420SP;
        break;
    }
    case MPP_FMT_YUV420SP_10BIT: {
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV420SP_VU: { //TODO: to be confirmed
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422P: {
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422SP: {
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422SP_10BIT: {
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422SP_VU: {
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
        break;
    }
    case MPP_FMT_YUV422_YUYV: {
        hw_cfg->input_format = H264E_VPU_CSP_YUYV422;
        break;
    }
    case MPP_FMT_YUV422_UYVY: {
        hw_cfg->input_format = H264E_VPU_CSP_UYVY422;
        break;
    }
    case MPP_FMT_RGB565: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB565;
        hw_cfg->r_mask_msb = 15;
        hw_cfg->g_mask_msb = 10;
        hw_cfg->b_mask_msb = 4;
        break;
    }
    case MPP_FMT_BGR565: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB565;
        hw_cfg->r_mask_msb = 4;
        hw_cfg->g_mask_msb = 10;
        hw_cfg->b_mask_msb = 15;
        break;
    }
    case MPP_FMT_RGB555: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB555;
        hw_cfg->r_mask_msb = 14;
        hw_cfg->g_mask_msb = 9;
        hw_cfg->b_mask_msb = 4;
        break;
    }
    case MPP_FMT_BGR555: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB555;
        hw_cfg->r_mask_msb = 14;
        hw_cfg->g_mask_msb = 9;
        hw_cfg->b_mask_msb = 4;
        break;
    }
    case MPP_FMT_RGB444: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB444;
        hw_cfg->r_mask_msb = 11;
        hw_cfg->g_mask_msb = 7;
        hw_cfg->b_mask_msb = 3;
        break;
    }
    case MPP_FMT_BGR444: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB444;
        hw_cfg->r_mask_msb = 11;
        hw_cfg->g_mask_msb = 7;
        hw_cfg->b_mask_msb = 3;
        break;
    }
    case MPP_FMT_RGB888: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB888;
        hw_cfg->r_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->b_mask_msb = 7;
        break;
    }
    case MPP_FMT_BGR888: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB888;
        hw_cfg->r_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->b_mask_msb = 7;
        break;
    }
    case MPP_FMT_RGB101010: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB101010;
        hw_cfg->r_mask_msb = 29;
        hw_cfg->g_mask_msb = 19;
        hw_cfg->b_mask_msb = 9;
        break;
    }
    case MPP_FMT_BGR101010: {
        hw_cfg->input_format = H264E_VPU_CSP_RGB101010;
        hw_cfg->r_mask_msb = 29;
        hw_cfg->g_mask_msb = 19;
        hw_cfg->b_mask_msb = 9;
        break;
    }
    case MPP_FMT_ARGB8888: {
        hw_cfg->input_format = H264E_VPU_CSP_ARGB8888;
        hw_cfg->b_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->r_mask_msb = 7;
        break;
    }
    case MPP_FMT_ABGR8888: {
        hw_cfg->input_format = H264E_VPU_CSP_ARGB8888;
        hw_cfg->r_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->b_mask_msb = 7;
        break;
    }
    default: {
        mpp_err_f("invalid color space: %d", format);
        hw_cfg->input_format = H264E_VPU_CSP_NONE;
    }
    }
}


static RK_S32 h264e_set_format(H264eHwCfg *hw_cfg, MppEncPrepCfg *prep_cfg)
{
    RK_S32 csp = 0;

    if (hw_cfg->hw_type == H264E_RKV) {
        h264e_rkv_set_format(hw_cfg, prep_cfg);
        csp = h264e_rkv_csp_idx_map[hw_cfg->input_format] & H264E_CSP2_MASK;
    } else {
        h264e_vpu_set_format(hw_cfg, prep_cfg);
        csp = h264e_vpu_csp_idx_map[hw_cfg->input_format] & H264E_CSP2_MASK;
    }

    return csp;
}

static void h264e_set_vui(H264eVuiParam *vui)
{
    vui->i_sar_height   = 0;
    vui->i_sar_width    = 0;
    vui->i_overscan     = 0;
    vui->i_vidformat    = 5;
    vui->b_fullrange    = 0;
    vui->i_colorprim    = 2;
    vui->i_transfer     = 2;
    vui->i_colmatrix    = -1;
    vui->i_chroma_loc   = 0;
}

static void h264e_set_ref(H264eRefParam *ref)
{
    ref->i_frame_reference = H264E_NUM_REFS;
    ref->i_dpb_size = H264E_NUM_REFS;
    ref->i_ref_pos = 1;
    ref->i_long_term_en = H264E_LONGTERM_REF_EN;
    ref->hw_longterm_mode = 0;
    ref->i_long_term_internal = 0;
}

void h264e_set_param(H264eHalParam *p, RK_S32 hw_type)
{
    H264eVuiParam *vui = &p->vui;
    H264eRefParam *ref = &p->ref;

    p->hw_type = hw_type;
    p->constrained_intra = 0;
    h264e_set_vui(vui);
    h264e_set_ref(ref);
}

MPP_RET h264e_set_sps(H264eHalContext *ctx, H264eSps *sps)
{
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    MppEncH264Cfg *codec = &ctx->cfg->codec.h264;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    MppEncRcCfg *rc = &ctx->cfg->rc;
    MppEncH264VuiCfg *vui = &codec->vui;
    MppEncH264RefCfg *ref = &codec->ref;
    RK_S32 max_frame_num = 0;

    /* default settings */
    RK_S32 i_bframe_pyramid = 0;
    RK_S32 i_bframe = 0;
    RK_S32 b_intra_refresh = 0;
    RK_S32 Log2MaxFrameNum = 16;
    RK_S32 Sw_log2_max_pic_order_cnt_lsb_minus4 = 12;
    RK_S32 i_overscan = 0;
    RK_S32 i_vidformat = 5;
    RK_S32 b_fullrange = vui->b_fullrange;
    RK_S32 i_colorprim = 2;
    RK_S32 i_transfer = 2;
    RK_S32 i_colmatrix = -1;
    RK_S32 i_chroma_loc = 0;
    RK_S32 b_interlaced = 0;
    RK_S32 b_fake_interlaced = 0;
    RK_S32 crop_rect_left = 0;
    RK_S32 crop_rect_right = 0;
    RK_S32 crop_rect_top = 0;
    RK_S32 crop_rect_bottom = 0;
    RK_S32 i_timebase_num = 1;
    RK_S32 i_timebase_den = rc->fps_out_num / rc->fps_out_denorm;
    RK_S32 b_vfr_input = 0;
    RK_S32 i_nal_hrd = 0;
    RK_S32 b_pic_struct = 0;
    RK_S32 analyse_mv_range = 128; //TODO: relative to level_idc, transplant x264_validate_levels.
    RK_S32 csp = 0;
    RK_S32 i_dpb_size = ref->i_dpb_size;
    RK_S32 i_frame_reference = ref->i_frame_reference;

    csp = h264e_set_format(hw_cfg, prep);
    sps->i_id = 0;
    sps->i_mb_width = ( prep->width + 15 ) / 16;
    sps->i_mb_height = ( prep->height + 15 ) / 16;
    sps->i_chroma_format_idc = H264E_CHROMA_420; // for rkv and vpu, only support YUV420

    sps->b_qpprime_y_zero_transform_bypass = 0; //param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant == 0;
    sps->i_profile_idc = codec->profile;

    sps->b_constraint_set0  = 0;
    sps->b_constraint_set1  = 0;
    sps->b_constraint_set2  = 0;
    sps->b_constraint_set3  = 0;

    sps->i_level_idc = codec->level;
    if ( codec->level == 9 && ( sps->i_profile_idc == H264_PROFILE_BASELINE || sps->i_profile_idc == H264_PROFILE_MAIN ) ) {
        mpp_log_f("warnings: for profile %d, change level from 9(that is 1b) to 11", sps->i_profile_idc);
        sps->i_level_idc      = 11;
    }

    sps->vui.i_num_reorder_frames = i_bframe_pyramid ? 2 : i_bframe ? 1 : 0;
    /* extra slot with pyramid so that we don't have to override the
     * order of forgetting old pictures */
    sps->vui.i_max_dec_frame_buffering =
        sps->i_num_ref_frames = H264E_HAL_MIN(H264E_REF_MAX, H264E_HAL_MAX4(i_frame_reference, 1 + sps->vui.i_num_reorder_frames,
                                                                            i_bframe_pyramid ? 4 : 1, i_dpb_size)); //TODO: multi refs
    sps->i_num_ref_frames -= i_bframe_pyramid == H264E_B_PYRAMID_STRICT; //TODO: multi refs
    sps->keyframe_max_interval = rc->gop;
    if (sps->keyframe_max_interval == 1) {
        sps->i_num_ref_frames = 0;
        sps->vui.i_max_dec_frame_buffering = 0;
    }

    /* number of refs + current frame */
    max_frame_num = sps->vui.i_max_dec_frame_buffering * (!!i_bframe_pyramid + 1) + 1;
    /* Intra refresh cannot write a recovery time greater than max frame num-1 */
    if (b_intra_refresh ) {
        RK_S32 time_to_recovery = H264E_HAL_MIN( sps->i_mb_width - 1, sps->keyframe_max_interval) + i_bframe - 1;
        max_frame_num = H264E_HAL_MAX( max_frame_num, time_to_recovery + 1 );
    }

    sps->i_log2_max_frame_num = Log2MaxFrameNum;//fix by gsl  org 16, at least 4
    while ( (1 << sps->i_log2_max_frame_num) <= max_frame_num  )
        sps->i_log2_max_frame_num++;

    if (hw_cfg->hw_type == H264E_RKV)
        sps->i_poc_type = 0;
    else // VPU
        sps->i_poc_type = 2;

    if ( sps->i_poc_type == 0  ) {
        RK_S32 max_delta_poc = (i_bframe + 2) * (!!i_bframe_pyramid + 1) * 2;
        sps->i_log2_max_poc_lsb = Sw_log2_max_pic_order_cnt_lsb_minus4 + 4;
        while ( (1 << sps->i_log2_max_poc_lsb) <= max_delta_poc * 2  )
            sps->i_log2_max_poc_lsb++;

    }

    sps->vui.b_vui = 1;

    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->b_frame_mbs_only = !(b_interlaced || b_fake_interlaced);
    if ( !sps->b_frame_mbs_only  )
        sps->i_mb_height = ( sps->i_mb_height + 1  ) & ~1;
    sps->b_mb_adaptive_frame_field = b_interlaced;
    sps->b_direct8x8_inference = 1;

    sps->crop.i_left   = crop_rect_left;
    sps->crop.i_top    = crop_rect_top;
    sps->crop.i_right  = crop_rect_right + sps->i_mb_width * 16 - prep->width;
    sps->crop.i_bottom = (crop_rect_bottom + sps->i_mb_height * 16 - prep->height) >> !sps->b_frame_mbs_only;
    sps->b_crop = sps->crop.i_left  || sps->crop.i_top ||
                  sps->crop.i_right || sps->crop.i_bottom;

    sps->vui.b_aspect_ratio_info_present = 0;
    if (vui->i_sar_width > 0 && vui->i_sar_height > 0 ) {
        sps->vui.b_aspect_ratio_info_present = 1;
        sps->vui.i_sar_width = vui->i_sar_width;
        sps->vui.i_sar_height = vui->i_sar_height;
    }

    sps->vui.b_overscan_info_present = i_overscan > 0 && i_overscan <= 2;
    if ( sps->vui.b_overscan_info_present )
        sps->vui.b_overscan_info = ( i_overscan == 2 ? 1 : 0 );

    sps->vui.b_signal_type_present = 0;
    sps->vui.i_vidformat = ( i_vidformat >= 0 && i_vidformat <= 5 ? i_vidformat : 5 );
    sps->vui.b_fullrange = ( b_fullrange >= 0 && b_fullrange <= 1 ? b_fullrange :
                             ( csp >= H264E_CSP2_BGR ? 1 : 0 ) );
    sps->vui.b_color_description_present = 0;

    sps->vui.i_colorprim = ( i_colorprim >= 0 && i_colorprim <=  9 ? i_colorprim : 2 );
    sps->vui.i_transfer  = ( i_transfer  >= 0 && i_transfer  <= 15 ? i_transfer  : 2 );
    sps->vui.i_colmatrix = ( i_colmatrix >= 0 && i_colmatrix <= 10 ? i_colmatrix :
                             ( csp >= H264E_CSP2_BGR ? 0 : 2 ) );
    if ( sps->vui.i_colorprim != 2 ||
         sps->vui.i_transfer  != 2 ||
         sps->vui.i_colmatrix != 2 ) {
        sps->vui.b_color_description_present = 1;
    }

    if ( sps->vui.i_vidformat != 5 ||
         sps->vui.b_fullrange ||
         sps->vui.b_color_description_present ) {
        sps->vui.b_signal_type_present = 1;
    }

    /* FIXME: not sufficient for interlaced video */
    sps->vui.b_chroma_loc_info_present = i_chroma_loc > 0 && i_chroma_loc <= 5 &&
                                         sps->i_chroma_format_idc == H264E_CHROMA_420;
    if ( sps->vui.b_chroma_loc_info_present  ) {
        sps->vui.i_chroma_loc_top = i_chroma_loc;
        sps->vui.i_chroma_loc_bottom = i_chroma_loc;

    }

    sps->vui.b_timing_info_present = i_timebase_num > 0 && i_timebase_den > 0;

    if (sps->vui.b_timing_info_present ) {
        sps->vui.i_num_units_in_tick = i_timebase_num;
        sps->vui.i_time_scale = i_timebase_den * 2;
        sps->vui.b_fixed_frame_rate = !b_vfr_input;

    }

    sps->vui.b_vcl_hrd_parameters_present = 0; // we don't support VCL HRD
    sps->vui.b_nal_hrd_parameters_present = !!i_nal_hrd;
    sps->vui.b_pic_struct_present = b_pic_struct;

    // NOTE: HRD related parts of the SPS are initialised in x264_ratecontrol_init_reconfigurable
    sps->vui.b_bitstream_restriction = sps->keyframe_max_interval > 1;
    if ( sps->vui.b_bitstream_restriction  ) {
        sps->vui.b_motion_vectors_over_pic_boundaries = 1;
        sps->vui.i_max_bytes_per_pic_denom = 0;
        sps->vui.i_max_bits_per_mb_denom = 0;
        sps->vui.i_log2_max_mv_length_horizontal =
            sps->vui.i_log2_max_mv_length_vertical =
                (RK_S32)log2f((float)H264E_HAL_MAX(1, analyse_mv_range * 4 - 1))
                + 1;
    }

    return MPP_OK;
}
MPP_RET h264e_set_pps(H264eHalContext *ctx, H264ePps *pps, H264eSps *sps)
{
    MppEncH264Cfg *codec = &ctx->cfg->codec.h264;
    RK_S32 k = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 b_interlaced = 0;
    RK_S32 analyse_weighted_pred = 0;
    RK_S32 analyse_b_weighted_bipred = 0;
    RK_S32 Sw_deblock_filter_ctrl_present_flag = 1;
    RK_S32 b_cqm_preset = 0;

    pps->i_id = 0;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = codec->entropy_coding_mode;

    pps->b_pic_order = !i_avcintra_class && b_interlaced;
    pps->i_num_slice_groups = 1;

    pps->i_num_ref_idx_l0_default_active = 1;
    pps->i_num_ref_idx_l1_default_active = 1;

    pps->b_weighted_pred = analyse_weighted_pred > 0;
    pps->i_weighted_bipred_idc = analyse_b_weighted_bipred ? 2 : 0;

    /* pps.pic_init_qp is not included in user interface, just fix it */
    pps->i_pic_init_qp = 26;
    pps->i_pic_init_qs = pps->i_pic_init_qp; // only for SP/SI slices

    pps->b_transform_8x8_mode = codec->transform8x8_mode;
    pps->i_chroma_qp_index_offset = codec->chroma_cb_qp_offset;
    pps->i_second_chroma_qp_index_offset = codec->chroma_cr_qp_offset;
    pps->b_deblocking_filter_control = Sw_deblock_filter_ctrl_present_flag;
    pps->b_constrained_intra_pred = codec->constrained_intra_pred_mode;
    pps->b_redundant_pic_cnt = 0;

    if (sps->i_profile_idc < H264_PROFILE_HIGH) {
        if (pps->b_transform_8x8_mode) {
            pps->b_transform_8x8_mode = 0;
            mpp_log_f("warning: for profile %d b_transform_8x8_mode should be 0",
                      sps->i_profile_idc);
        }
        if (pps->i_second_chroma_qp_index_offset) {
            pps->i_second_chroma_qp_index_offset = 0;
            mpp_log_f("warning: for profile %d i_second_chroma_qp_index_offset should be 0",
                      sps->i_profile_idc);
        }
    }
    if (sps->i_profile_idc == H264_PROFILE_BASELINE && pps->b_cabac) {
        mpp_log_f("warning: for profile baseline b_cabac should be 0");
        pps->b_cabac = 0;
    }

    pps->b_cqm_preset = b_cqm_preset;

    switch (pps->b_cqm_preset) {
    case H264E_CQM_FLAT:
        for (k = 0; k < 8; k++)
            pps->scaling_list[k] = h264e_cqm_flat16;
        break;
    case H264E_CQM_JVT:
        for (k = 0; k < 8; k++)
            pps->scaling_list[k] = h264e_cqm_jvt[k];
        break;
    case H264E_CQM_CUSTOM:
        /* match the transposed DCT & zigzag */
        h264e_hal_err("CQM_CUSTOM mode is not supported now");
        return MPP_NOK;
    default:
        h264e_hal_err("invalid cqm_preset mode %d", b_cqm_preset);
        return MPP_NOK;
    }

    return MPP_OK;
}

void h264e_sei_pack2str(char *str, H264eHalContext *ctx, RcSyntax *rc_syn)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_U32 prep_change = prep->change & MPP_ENC_PREP_CFG_CHANGE_INPUT;
    RK_U32 codec_change = codec->change;
    RK_U32 rc_change = rc->change;
    RK_S32 len = H264E_SEI_BUF_SIZE - H264E_UUID_LENGTH;

    if (prep_change || codec_change || rc_change)
        H264E_HAL_SPRINT(str, len, "frm %d cfg: ", ctx->frame_cnt);

    /* prep cfg */
    if (prep_change) {
        H264E_HAL_SPRINT(str, len, "[prep] ");
        if (prep_change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            H264E_HAL_SPRINT(str, len, "w=%d ", prep->width);
            H264E_HAL_SPRINT(str, len, "h=%d ", prep->height);
            H264E_HAL_SPRINT(str, len, "fmt=%d ", prep->format);
            H264E_HAL_SPRINT(str, len, "h_strd=%d ", prep->hor_stride);
            H264E_HAL_SPRINT(str, len, "v_strd=%d ", prep->ver_stride);
        }
    }

    /* codec cfg */
    if (codec_change) {
        H264E_HAL_SPRINT(str, len, "[codec] ");
        H264E_HAL_SPRINT(str, len, "profile=%d ", codec->profile);
        H264E_HAL_SPRINT(str, len, "level=%d ", codec->level);
        H264E_HAL_SPRINT(str, len, "b_cabac=%d ", codec->entropy_coding_mode);
        H264E_HAL_SPRINT(str, len, "cabac_idc=%d ", codec->cabac_init_idc);
        H264E_HAL_SPRINT(str, len, "t8x8=%d ", codec->transform8x8_mode);
        H264E_HAL_SPRINT(str, len, "constrain_intra=%d ", codec->constrained_intra_pred_mode);
        H264E_HAL_SPRINT(str, len, "dblk=%d:%d:%d ", codec->deblock_disable,
                         codec->deblock_offset_alpha, codec->deblock_offset_beta);
        H264E_HAL_SPRINT(str, len, "cbcr_qp_offset=%d:%d ", codec->chroma_cb_qp_offset,
                         codec->chroma_cr_qp_offset);
        H264E_HAL_SPRINT(str, len, "qp_max=%d ", codec->qp_max);
        H264E_HAL_SPRINT(str, len, "qp_min=%d ", codec->qp_min);
        H264E_HAL_SPRINT(str, len, "qp_max_step=%d ", codec->qp_max_step);
    }

    /* rc cfg */
    if (rc_change) {
        H264E_HAL_SPRINT(str, len, "[rc] ");
        H264E_HAL_SPRINT(str, len, "fps_in=%d:%d:%d ", rc->fps_in_num, rc->fps_in_denorm, rc->fps_in_flex);
        H264E_HAL_SPRINT(str, len, "fps_out=%d:%d:%d ", rc->fps_out_num, rc->fps_out_denorm, rc->fps_out_flex);
        H264E_HAL_SPRINT(str, len, "gop=%d ", rc->gop);
    }
    /* record detailed RC parameter
     * Start to write parameter when the first frame is encoded,
     * because we can get all parameter only when it's encoded.
     */
    if (rc_syn && rc_syn->rc_head && (ctx->frame_cnt > 0)) {
        RecordNode *pos, *m;
        MppLinReg *lin_reg;

        list_for_each_entry_safe(pos, m, rc_syn->rc_head, RecordNode, list) {
            if (ctx->frame_cnt == pos->frm_cnt) {
                H264E_HAL_SPRINT(str, len, "[frm %d]detailed param ", pos->frm_cnt);
                H264E_HAL_SPRINT(str, len, "tgt_bits=%d:%d:%d:%d ",
                                 pos->tgt_bits, pos->real_bits,
                                 pos->bit_min, pos->bit_max);
                H264E_HAL_SPRINT(str, len, "tgt_qp=%d:%d:%d:%d ",
                                 pos->set_qp, pos->real_qp,
                                 pos->qp_min, pos->qp_max);

                H264E_HAL_SPRINT(str, len, "per_pic=%d intra=%d inter=%d ",
                                 pos->bits_per_pic,
                                 pos->bits_per_intra, pos->bits_per_inter);
                H264E_HAL_SPRINT(str, len, "acc_intra=%d inter=%d last_fps=%d ",
                                 pos->acc_intra_bits_in_fps,
                                 pos->acc_inter_bits_in_fps,
                                 pos->last_fps_bits);
                H264E_HAL_SPRINT(str, len, "qp_sum=%d sse_sum=%lld ",
                                 pos->qp_sum, pos->sse_sum);

                lin_reg = &pos->lin_reg;
                H264E_HAL_SPRINT(str, len, "size=%d n=%d i=%d ",
                                 lin_reg->size, lin_reg->n, lin_reg->i);
                H264E_HAL_SPRINT(str, len, "a=%0.2f b=%0.2f c=%0.2f ",
                                 lin_reg->a, lin_reg->b, lin_reg->c);
                H264E_HAL_SPRINT(str, len, "weight_len=%d wlen=%d ",
                                 lin_reg->weight_mode, pos->wlen);

                /* frame type is intra */
                if (pos->frm_type == INTRA_FRAME)
                    H264E_HAL_SPRINT(str, len, "fps=%d gop=%d I=%0.2f ", pos->fps,
                                     pos->gop, pos->last_intra_percent);

                break;
            }
        }
    }
    /* frame type is intra */
    if (rc_syn && (hw_cfg->frame_type == 2)) {
        H264E_HAL_SPRINT(str, len, "[frm %d] ", ctx->frame_cnt);
        H264E_HAL_SPRINT(str, len, "rc_mode=%d ", rc->rc_mode);
        H264E_HAL_SPRINT(str, len, "quality=%d ", rc->quality);
        H264E_HAL_SPRINT(str, len, "bps=%d:%d:%d ", rc->bps_target, rc->bps_min, rc->bps_max);
    }
}

