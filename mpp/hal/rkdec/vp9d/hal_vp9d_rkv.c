/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vp9d_rkv"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_vp9d_debug.h"
#include "hal_vp9d_ctx.h"
#include "hal_vp9d_com.h"
#include "hal_vp9d_rkv.h"
#include "hal_vp9d_rkv_reg.h"
#include "vp9d_syntax.h"


typedef struct Vp9dRkvCtx_t {
    Vp9dRegBuf      g_buf[MAX_GEN_REG];
    MppBuffer       probe_base;
    MppBuffer       count_base;
    MppBuffer       segid_cur_base;
    MppBuffer       segid_last_base;
    void*           hw_regs;
    RK_S32          mv_base_addr;
    RK_U32          mv_base_offset;
    RK_S32          pre_mv_base_addr;
    RK_U32          pre_mv_base_offset;
    Vp9dLastInfo    ls_info;
    /*
     * swap between segid_cur_base & segid_last_base
     * 0  used segid_cur_base as last
     * 1  used segid_last_base as
     */
    RK_U32          last_segid_flag;
} Vp9dRkvCtx;

static MPP_RET hal_vp9d_alloc_res(HalVp9dCtx *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;

    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            hw_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].probe_base, PROB_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].count_base, COUNT_SIZE);
            if (ret) {
                mpp_err("vp9 count_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].segid_cur_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_cur_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].segid_last_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_last_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        hw_ctx->hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->probe_base, PROB_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->count_base, COUNT_SIZE);
        if (ret) {
            mpp_err("vp9 count_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->segid_cur_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_cur_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->segid_last_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_last_base get buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(HalVp9dCtx *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    HalVp9dCtx *p_hal = hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;

    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (hw_ctx->g_buf[i].probe_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].probe_base);
                if (ret) {
                    mpp_err("vp9 probe_base put buffer failed\n");
                    return ret;
                }
            }
            if (hw_ctx->g_buf[i].count_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].count_base);
                if (ret) {
                    mpp_err("vp9 count_base put buffer failed\n");
                    return ret;
                }
            }
            if (hw_ctx->g_buf[i].segid_cur_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].segid_cur_base);
                if (ret) {
                    mpp_err("vp9 segid_cur_base put buffer failed\n");
                    return ret;
                }
            }
            if (hw_ctx->g_buf[i].segid_last_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].segid_last_base);
                if (ret) {
                    mpp_err("vp9 segid_last_base put buffer failed\n");
                    return ret;
                }
            }
            if (hw_ctx->g_buf[i].hw_regs) {
                mpp_free(hw_ctx->g_buf[i].hw_regs);
                hw_ctx->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (hw_ctx->probe_base) {
            ret = mpp_buffer_put(hw_ctx->probe_base);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
        }
        if (hw_ctx->count_base) {
            ret = mpp_buffer_put(hw_ctx->count_base);
            if (ret) {
                mpp_err("vp9 count_base put buffer failed\n");
                return ret;
            }
        }
        if (hw_ctx->segid_cur_base) {
            ret = mpp_buffer_put(hw_ctx->segid_cur_base);
            if (ret) {
                mpp_err("vp9 segid_cur_base put buffer failed\n");
                return ret;
            }
        }
        if (hw_ctx->segid_last_base) {
            ret = mpp_buffer_put(hw_ctx->segid_last_base);
            if (ret) {
                mpp_err("vp9 segid_last_base put buffer failed\n");
                return ret;
            }
        }
        if (hw_ctx->hw_regs) {
            mpp_free(hw_ctx->hw_regs);
            hw_ctx->hw_regs = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    MEM_CHECK(ret, p_hal->hw_ctx = mpp_calloc_size(void, sizeof(Vp9dRkvCtx)));
    Vp9dRkvCtx *ctx = (Vp9dRkvCtx *)p_hal->hw_ctx;

    mpp_log("hal_vp9d_rkv_init in");
    ctx->mv_base_addr = -1;
    ctx->pre_mv_base_addr = -1;
    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, vp9_hor_align);
    mpp_slots_set_prop(p_hal->slots, SLOTS_VER_ALIGN, vp9_ver_align);

    if (p_hal->group == NULL) {
        ret = mpp_buffer_group_get_internal(&p_hal->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("vp9 mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    ret = hal_vp9d_alloc_res(p_hal);
    if (ret) {
        mpp_err("hal_vp9d_alloc_res failed\n");
        return ret;
    }

    ctx->last_segid_flag = 1;

    (void) cfg;
    return ret = MPP_OK;
__FAILED:
    return ret = MPP_NOK;
}

MPP_RET hal_vp9d_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;

    hal_vp9d_release_res(p_hal);

    if (p_hal->group) {
        ret = mpp_buffer_group_put(p_hal->group);
        if (ret) {
            mpp_err("vp9d group free buffer failed\n");
            return ret;
        }
    }
    MPP_FREE(p_hal->hw_ctx);
    return ret = MPP_OK;
}

MPP_RET hal_vp9d_rkv_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32   i;
    RK_U8    bit_depth = 0;
    RK_U32   pic_h[3] = { 0 };
    RK_U32   ref_frame_width_y;
    RK_U32   ref_frame_height_y;
    RK_S32   stream_len = 0, aglin_offset = 0;
    RK_U32   y_hor_virstride, uv_hor_virstride, y_virstride, uv_virstride, yuv_virstride;
    RK_U8   *bitstream = NULL;
    MppBuffer streambuf = NULL;
    RK_U32 sw_y_hor_virstride;
    RK_U32 sw_uv_hor_virstride;
    RK_U32 sw_y_virstride;
    RK_U32 sw_uv_virstride;
    RK_U32 sw_yuv_virstride ;
    RK_U8  ref_idx = 0;
    RK_U32 *reg_ref_base = 0;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;

    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!hw_ctx->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                hw_ctx->probe_base = hw_ctx->g_buf[i].probe_base;
                hw_ctx->count_base = hw_ctx->g_buf[i].count_base;
                hw_ctx->segid_cur_base = hw_ctx->g_buf[i].segid_cur_base;
                hw_ctx->segid_last_base = hw_ctx->g_buf[i].segid_last_base;
                hw_ctx->hw_regs = hw_ctx->g_buf[i].hw_regs;
                hw_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("vp9 fast mode buf all used\n");
            return MPP_ERR_NOMEM;
        }
    }
    VP9_REGS *vp9_hw_regs = (VP9_REGS*)hw_ctx->hw_regs;
    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_output_probe(mpp_buffer_get_ptr(hw_ctx->probe_base), task->dec.syntax.data);
    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
    memset(hw_ctx->hw_regs, 0, sizeof(VP9_REGS));
    vp9_hw_regs->swreg2_sysctrl.sw_dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->swreg5_stream_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = vp9_hw_regs->swreg5_stream_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;
    pic_h[0] = vp9_ver_align(pic_param->height); //p_cm->height;
    pic_h[1] = vp9_ver_align(pic_param->height) / 2; //(p_cm->height + 1) / 2;
    pic_h[2] = pic_h[1];

    sw_y_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_uv_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_y_virstride = pic_h[0] * sw_y_hor_virstride;

    sw_uv_virstride = pic_h[1] * sw_uv_hor_virstride;
    sw_yuv_virstride = sw_y_virstride + sw_uv_virstride;

    vp9_hw_regs->swreg3_picpar.sw_y_hor_virstride = sw_y_hor_virstride;
    vp9_hw_regs->swreg3_picpar.sw_uv_hor_virstride = sw_uv_hor_virstride;
    vp9_hw_regs->swreg8_y_virstride.sw_y_virstride = sw_y_virstride;
    vp9_hw_regs->swreg9_yuv_virstride.sw_yuv_virstride = sw_yuv_virstride;

    if (!pic_param->intra_only && pic_param->frame_type &&
        !pic_param->error_resilient_mode && hw_ctx->ls_info.last_show_frame) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
        hw_ctx->pre_mv_base_offset = hw_ctx->mv_base_offset;
    }


    mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    vp9_hw_regs->swreg7_decout_base =  mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->swreg4_strm_rlc_base = mpp_buffer_get_fd(streambuf);

    vp9_hw_regs->swreg6_cabactbl_prob_base = mpp_buffer_get_fd(hw_ctx->probe_base);
    vp9_hw_regs->swreg14_vp9_count_base  = mpp_buffer_get_fd(hw_ctx->count_base);

    if (hw_ctx->last_segid_flag) {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
    } else {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
    }

    if (pic_param->stVP9Segments.enabled && pic_param->stVP9Segments.update_map) {
        hw_ctx->last_segid_flag = !hw_ctx->last_segid_flag;
    }

    hw_ctx->mv_base_addr = vp9_hw_regs->swreg7_decout_base;
    hw_ctx->mv_base_offset = mpp_get_ioctl_version() ? sw_yuv_virstride << 4 : sw_yuv_virstride;
    if (hw_ctx->pre_mv_base_addr < 0) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
        hw_ctx->pre_mv_base_offset = hw_ctx->mv_base_offset;
    }
    vp9_hw_regs->swreg52_vp9_refcolmv_base = hw_ctx->pre_mv_base_addr;
    mpp_dev_set_reg_offset(p_hal->dev, 52, hw_ctx->pre_mv_base_offset);

    vp9_hw_regs->swreg10_vp9_cprheader_offset.sw_vp9_cprheader_offset = 0; //no use now.
    reg_ref_base = &vp9_hw_regs->swreg11_vp9_referlast_base;
    for (i = 0; i < 3; i++) {
        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];
        pic_h[0] = vp9_ver_align(ref_frame_height_y);
        pic_h[1] = vp9_ver_align(ref_frame_height_y) / 2;
        y_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        uv_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        y_virstride = y_hor_virstride * pic_h[0];
        uv_virstride = uv_hor_virstride * pic_h[1];
        yuv_virstride = y_virstride + uv_virstride;

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            mpp_buf_slot_get_prop(p_hal->slots, pic_param->ref_frame_map[ref_idx].Index7Bits, SLOT_BUFFER, &framebuf);
        }

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            switch (i) {
            case 0: {

                vp9_hw_regs->swreg17_vp9_frame_size_last.sw_framewidth_last = ref_frame_width_y;
                vp9_hw_regs->swreg17_vp9_frame_size_last.sw_frameheight_last = ref_frame_height_y;
                vp9_hw_regs->swreg37_vp9_lastf_hor_virstride.sw_vp9_lastfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg37_vp9_lastf_hor_virstride.sw_vp9_lastfuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg48_vp9_last_ystride.sw_vp9_lastfy_virstride = y_virstride;
                vp9_hw_regs->swreg51_vp9_lastref_yuvstride.sw_vp9_lastref_yuv_virstride = yuv_virstride;
                break;
            }
            case 1: {
                vp9_hw_regs->swreg18_vp9_frame_size_golden.sw_framewidth_golden = ref_frame_width_y;
                vp9_hw_regs->swreg18_vp9_frame_size_golden.sw_frameheight_golden = ref_frame_height_y;
                vp9_hw_regs->swreg38_vp9_goldenf_hor_virstride.sw_vp9_goldenfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg38_vp9_goldenf_hor_virstride.sw_vp9_goldenuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg49_vp9_golden_ystride.sw_vp9_goldeny_virstride = y_virstride;
                break;
            }
            case 2: {
                vp9_hw_regs->swreg19_vp9_frame_size_altref.sw_framewidth_alfter = ref_frame_width_y;
                vp9_hw_regs->swreg19_vp9_frame_size_altref.sw_frameheight_alfter = ref_frame_height_y;
                vp9_hw_regs->swreg39_vp9_altreff_hor_virstride.sw_vp9_altreffy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->swreg39_vp9_altreff_hor_virstride.sw_vp9_altreffuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->swreg50_vp9_altrefy_ystride.sw_vp9_altrefy_virstride = y_virstride;
                break;
            }
            default:
                break;

            }

            /*0 map to 11*/
            /*1 map to 12*/
            /*2 map to 13*/
            if (framebuf != NULL) {
                reg_ref_base[i] = mpp_buffer_get_fd(framebuf);
            } else {
                mpp_log("ref buff address is no valid used out as base slot index 0x%x", pic_param->ref_frame_map[ref_idx].Index7Bits);
                reg_ref_base[i] = vp9_hw_regs->swreg7_decout_base; //set
            }
        } else {
            reg_ref_base[i] = vp9_hw_regs->swreg7_decout_base; //set
        }
    }

    for (i = 0; i < 8; i++) {
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta_en              = (hw_ctx->ls_info.feature_mask[i]) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta                 = hw_ctx->ls_info.feature_data[i][0];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfitler_value_en      = (hw_ctx->ls_info.feature_mask[i] >> 1) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfilter_value         = hw_ctx->ls_info.feature_data[i][1];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo_en                   = (hw_ctx->ls_info.feature_mask[i] >> 2) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo                      = hw_ctx->ls_info.feature_data[i][2];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_skip_en                  = (hw_ctx->ls_info.feature_mask[i] >> 3) & 0x1;
    }


    vp9_hw_regs->swreg20_27_vp9_segid_grp[0].sw_vp9segid_abs_delta                              = hw_ctx->ls_info.abs_delta_last;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_tx_mode                                    = pic_param->txmode;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_frame_reference_mode                   = pic_param->refmode;

    vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe               = 0;

    if (!intraFlag) {
        for (i = 0; i < 4; i++)
            vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe           |= (hw_ctx->ls_info.last_ref_deltas[i] & 0x7f) << (7 * i);

        for (i = 0; i < 2; i++)
            vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                |= (hw_ctx->ls_info.last_mode_deltas[i] & 0x7f) << (7 * i);


    } else {
        hw_ctx->ls_info.segmentation_enable_flag_last = 0;
        hw_ctx->ls_info.last_intra_only = 1;
    }

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                        = 0;

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_segmentation_enable_lstframe                  = hw_ctx->ls_info.segmentation_enable_flag_last;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_show_frame                          = hw_ctx->ls_info.last_show_frame;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_intra_only                          = hw_ctx->ls_info.last_intra_only;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_widthheight_eqcur                   = (pic_param->width == hw_ctx->ls_info.last_width) && (pic_param->height == hw_ctx->ls_info.last_height);

    vp9_hw_regs->swreg36_vp9_lasttile_size.sw_vp9_lasttile_size                             =  stream_len - pic_param->first_partition_size;


    if (!intraFlag) {
        vp9_hw_regs->swreg29_vp9_lref_scale.sw_vp9_lref_hor_scale = pic_param->mvscale[0][0];
        vp9_hw_regs->swreg29_vp9_lref_scale.sw_vp9_lref_ver_scale = pic_param->mvscale[0][1];
        vp9_hw_regs->swreg30_vp9_gref_scale.sw_vp9_gref_hor_scale = pic_param->mvscale[1][0];
        vp9_hw_regs->swreg30_vp9_gref_scale.sw_vp9_gref_ver_scale = pic_param->mvscale[1][1];
        vp9_hw_regs->swreg31_vp9_aref_scale.sw_vp9_aref_hor_scale = pic_param->mvscale[2][0];
        vp9_hw_regs->swreg31_vp9_aref_scale.sw_vp9_aref_ver_scale = pic_param->mvscale[2][1];
        // vp9_hw_regs.swreg33_vp9_info_lastframe.sw_vp9_color_space_lastkeyframe = p_cm->color_space_last;
    }


    //reuse reg64, and it will be written by hardware to show performance.
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle = 0;
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle |= pic_param->width;
    vp9_hw_regs->swreg64_performance_cycle.sw_performance_cycle |= pic_param->height << 16;

    vp9_hw_regs->swreg1_int.sw_dec_e         = 1;
    vp9_hw_regs->swreg1_int.sw_dec_timeout_e = 1;

    //last info  update
    hw_ctx->ls_info.abs_delta_last = pic_param->stVP9Segments.abs_delta;
    for (i = 0 ; i < 4; i ++) {
        hw_ctx->ls_info.last_ref_deltas[i] = pic_param->ref_deltas[i];
    }

    for (i = 0 ; i < 2; i ++) {
        hw_ctx->ls_info.last_mode_deltas[i] = pic_param->mode_deltas[i];
    }

    for (i = 0; i < 8; i++) {
        hw_ctx->ls_info.feature_data[i][0] = pic_param->stVP9Segments.feature_data[i][0];
        hw_ctx->ls_info.feature_data[i][1] = pic_param->stVP9Segments.feature_data[i][1];
        hw_ctx->ls_info.feature_data[i][2] = pic_param->stVP9Segments.feature_data[i][2];
        hw_ctx->ls_info.feature_data[i][3] = pic_param->stVP9Segments.feature_data[i][3];
        hw_ctx->ls_info.feature_mask[i]  = pic_param->stVP9Segments.feature_mask[i];
    }
    if (!hw_ctx->ls_info.segmentation_enable_flag_last)
        hw_ctx->ls_info.segmentation_enable_flag_last = pic_param->stVP9Segments.enabled;

    hw_ctx->ls_info.last_show_frame = pic_param->show_frame;
    hw_ctx->ls_info.last_width = pic_param->width;
    hw_ctx->ls_info.last_height = pic_param->height;
    hw_ctx->ls_info.last_intra_only = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_dbg_par("stVP9Segments.enabled %d show_frame %d  width %d  height %d last_intra_only %d",
                     pic_param->stVP9Segments.enabled, pic_param->show_frame,
                     pic_param->width, pic_param->height,
                     hw_ctx->ls_info.last_intra_only);

    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;
    VP9_REGS *hw_regs = (VP9_REGS *)hw_ctx->hw_regs;
    MppDev dev = p_hal->dev;

    if (p_hal->fast_mode) {
        RK_S32 index =  task->dec.reg_index;
        hw_regs = (VP9_REGS *)hw_ctx->g_buf[index].hw_regs;
    }

    mpp_assert(hw_regs);

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(VP9_REGS) / 4; i++)
            mpp_log("set regs[%02d]: %08X\n", i, *p++);
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;
        RK_U32 reg_size = sizeof(VP9_REGS);

        wr_cfg.reg = hw_ctx->hw_regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = hw_ctx->hw_regs;
        rd_cfg.size = reg_size;
        rd_cfg.offset = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    (void)task;
    return ret;
}

MPP_RET hal_vp9d_rkv_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;
    VP9_REGS *hw_regs = (VP9_REGS *)hw_ctx->hw_regs;

    if (p_hal->fast_mode)
        hw_regs = (VP9_REGS *)hw_ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(VP9_REGS) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i, *p++);
    }

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        !hw_regs->swreg1_int.sw_dec_rdy_sta) {
        MppFrame mframe = NULL;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

    if (p_hal->dec_cb && task->dec.flags.wait_done) {
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;

        hal_vp9d_update_counts(mpp_buffer_get_ptr(hw_ctx->count_base), task->dec.syntax.data);

        mpp_callback(p_hal->dec_cb, DEC_PARSER_CALLBACK, &pic_param->counts);
    }
    if (p_hal->fast_mode) {
        hw_ctx->g_buf[task->dec.reg_index].use_flag = 0;
    }

    (void)task;
    return ret;
}

MPP_RET hal_vp9d_rkv_reset(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    Vp9dRkvCtx *hw_ctx = (Vp9dRkvCtx*)p_hal->hw_ctx;

    hal_vp9d_enter();

    memset(&hw_ctx->ls_info, 0, sizeof(hw_ctx->ls_info));
    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    hw_ctx->last_segid_flag = 1;

    hal_vp9d_leave();

    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_flush(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    Vp9dRkvCtx *hw_ctx = p_hal->hw_ctx;

    hal_vp9d_enter();

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;

    hal_vp9d_leave();

    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_control(void *hal, MpiCmd cmd_type, void *param)
{
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        /* commit buffer stride */
        RK_U32 width = mpp_frame_get_width((MppFrame)param);
        RK_U32 height = mpp_frame_get_height((MppFrame)param);

        mpp_frame_set_hor_stride((MppFrame)param, vp9_hor_align(width));
        mpp_frame_set_ver_stride((MppFrame)param, vp9_ver_align(height));
    } break;
    default: {
    } break;
    }
    (void)hal;

    return MPP_OK;
}

const MppHalApi hal_vp9d_rkv = {
    .name = "vp9d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(HalVp9dCtx),
    .flag = 0,
    .init = hal_vp9d_rkv_init,
    .deinit = hal_vp9d_rkv_deinit,
    .reg_gen = hal_vp9d_rkv_gen_regs,
    .start = hal_vp9d_rkv_start,
    .wait = hal_vp9d_rkv_wait,
    .reset = hal_vp9d_rkv_reset,
    .flush = hal_vp9d_rkv_flush,
    .control = hal_vp9d_rkv_control,
};
