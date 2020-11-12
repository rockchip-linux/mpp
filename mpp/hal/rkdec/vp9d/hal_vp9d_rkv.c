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

static MPP_RET hal_vp9d_alloc_res(HalVp9dCtx *reg_cxt)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            reg_cxt->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].probe_base, PROBE_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].count_base, COUNT_SIZE);
            if (ret) {
                mpp_err("vp9 count_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].segid_cur_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_cur_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_cxt->group,
                                 &reg_cxt->g_buf[i].segid_last_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_last_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        reg_cxt->hw_regs = mpp_calloc_size(void, sizeof(VP9_REGS));
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->probe_base, PROBE_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->count_base, COUNT_SIZE);
        if (ret) {
            mpp_err("vp9 count_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->segid_cur_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_cur_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_cxt->group, &reg_cxt->segid_last_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_last_base get buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(HalVp9dCtx *reg_cxt)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_cxt->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (reg_cxt->g_buf[i].probe_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].probe_base);
                if (ret) {
                    mpp_err("vp9 probe_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].count_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].count_base);
                if (ret) {
                    mpp_err("vp9 count_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].segid_cur_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].segid_cur_base);
                if (ret) {
                    mpp_err("vp9 segid_cur_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].segid_last_base) {
                ret = mpp_buffer_put(reg_cxt->g_buf[i].segid_last_base);
                if (ret) {
                    mpp_err("vp9 segid_last_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_cxt->g_buf[i].hw_regs) {
                mpp_free(reg_cxt->g_buf[i].hw_regs);
                reg_cxt->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (reg_cxt->probe_base) {
            ret = mpp_buffer_put(reg_cxt->probe_base);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->count_base) {
            ret = mpp_buffer_put(reg_cxt->count_base);
            if (ret) {
                mpp_err("vp9 count_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->segid_cur_base) {
            ret = mpp_buffer_put(reg_cxt->segid_cur_base);
            if (ret) {
                mpp_err("vp9 segid_cur_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->segid_last_base) {
            ret = mpp_buffer_put(reg_cxt->segid_last_base);
            if (ret) {
                mpp_err("vp9 segid_last_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_cxt->hw_regs) {
            mpp_free(reg_cxt->hw_regs);
            reg_cxt->hw_regs = NULL;
        }
    }
    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *ctx = (HalVp9dCtx *)hal;

    mpp_log("hal_vp9d_rkv_init in");
    ctx->mv_base_addr = -1;
    ctx->pre_mv_base_addr = -1;
    mpp_slots_set_prop(ctx->slots, SLOTS_HOR_ALIGN, vp9_hor_align);
    mpp_slots_set_prop(ctx->slots, SLOTS_VER_ALIGN, vp9_ver_align);

    if (ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("vp9 mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    ret = hal_vp9d_alloc_res(ctx);
    if (ret) {
        mpp_err("hal_vp9d_alloc_res failed\n");
        return ret;
    }

    ctx->last_segid_flag = 1;

    (void) cfg;
    return ret = MPP_OK;
}

MPP_RET hal_vp9d_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *ctx = (HalVp9dCtx *)hal;

    hal_vp9d_release_res(ctx);

    if (ctx->group) {
        ret = mpp_buffer_group_put(ctx->group);
        if (ret) {
            mpp_err("vp9d group free buffer failed\n");
            return ret;
        }
    }
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
    HalVp9dCtx *reg_cxt = (HalVp9dCtx*)hal;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;

    if (reg_cxt ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_cxt->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                reg_cxt->probe_base = reg_cxt->g_buf[i].probe_base;
                reg_cxt->count_base = reg_cxt->g_buf[i].count_base;
                reg_cxt->segid_cur_base = reg_cxt->g_buf[i].segid_cur_base;
                reg_cxt->segid_last_base = reg_cxt->g_buf[i].segid_last_base;
                reg_cxt->hw_regs = reg_cxt->g_buf[i].hw_regs;
                reg_cxt->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("vp9 fast mode buf all used\n");
            return MPP_ERR_NOMEM;
        }
    }
    VP9_REGS *vp9_hw_regs = (VP9_REGS*)reg_cxt->hw_regs;
    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_output_probe(mpp_buffer_get_ptr(reg_cxt->probe_base), task->dec.syntax.data);
    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
    memset(reg_cxt->hw_regs, 0, sizeof(VP9_REGS));
    vp9_hw_regs->swreg2_sysctrl.sw_dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->swreg5_stream_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(reg_cxt->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
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
        !pic_param->error_resilient_mode && reg_cxt->ls_info.last_show_frame) {
        reg_cxt->pre_mv_base_addr = reg_cxt->mv_base_addr;
    }


    mpp_buf_slot_get_prop(reg_cxt->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    vp9_hw_regs->swreg7_decout_base =  mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->swreg4_strm_rlc_base = mpp_buffer_get_fd(streambuf);

    vp9_hw_regs->swreg6_cabactbl_prob_base = mpp_buffer_get_fd(reg_cxt->probe_base);
    vp9_hw_regs->swreg14_vp9_count_base  = mpp_buffer_get_fd(reg_cxt->count_base);

    if (reg_cxt->last_segid_flag) {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(reg_cxt->segid_last_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(reg_cxt->segid_cur_base);
    } else {
        vp9_hw_regs->swreg15_vp9_segidlast_base = mpp_buffer_get_fd(reg_cxt->segid_cur_base);
        vp9_hw_regs->swreg16_vp9_segidcur_base = mpp_buffer_get_fd(reg_cxt->segid_last_base);
    }

    if (pic_param->stVP9Segments.enabled && pic_param->stVP9Segments.update_map) {
        reg_cxt->last_segid_flag = !reg_cxt->last_segid_flag;
    }

    reg_cxt->mv_base_addr = vp9_hw_regs->swreg7_decout_base | ((sw_yuv_virstride << 4) << 6);
    if (reg_cxt->pre_mv_base_addr < 0) {
        reg_cxt->pre_mv_base_addr = reg_cxt->mv_base_addr;
    }
    vp9_hw_regs->swreg52_vp9_refcolmv_base = reg_cxt->pre_mv_base_addr;

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
            mpp_buf_slot_get_prop(reg_cxt->slots, pic_param->ref_frame_map[ref_idx].Index7Bits, SLOT_BUFFER, &framebuf);
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
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta_en              = (reg_cxt->ls_info.feature_mask[i]) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_qp_delta                 = reg_cxt->ls_info.feature_data[i][0];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfitler_value_en      = (reg_cxt->ls_info.feature_mask[i] >> 1) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_loopfilter_value         = reg_cxt->ls_info.feature_data[i][1];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo_en                   = (reg_cxt->ls_info.feature_mask[i] >> 2) & 0x1;
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_referinfo                      = reg_cxt->ls_info.feature_data[i][2];
        vp9_hw_regs->swreg20_27_vp9_segid_grp[i].sw_vp9segid_frame_skip_en                  = (reg_cxt->ls_info.feature_mask[i] >> 3) & 0x1;
    }


    vp9_hw_regs->swreg20_27_vp9_segid_grp[0].sw_vp9segid_abs_delta                              = reg_cxt->ls_info.abs_delta_last;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_tx_mode                                    = pic_param->txmode;

    vp9_hw_regs->swreg28_vp9_cprheader_config.sw_vp9_frame_reference_mode                   = pic_param->refmode;

    vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe               = 0;

    if (!intraFlag) {
        for (i = 0; i < 4; i++)
            vp9_hw_regs->swreg32_vp9_ref_deltas_lastframe.sw_vp9_ref_deltas_lastframe           |= (reg_cxt->ls_info.last_ref_deltas[i] & 0x7f) << (7 * i);

        for (i = 0; i < 2; i++)
            vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                |= (reg_cxt->ls_info.last_mode_deltas[i] & 0x7f) << (7 * i);


    } else {
        reg_cxt->ls_info.segmentation_enable_flag_last = 0;
        reg_cxt->ls_info.last_intra_only = 1;
    }

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_mode_deltas_lastframe                        = 0;

    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_segmentation_enable_lstframe                  = reg_cxt->ls_info.segmentation_enable_flag_last;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_show_frame                          = reg_cxt->ls_info.last_show_frame;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_intra_only                          = reg_cxt->ls_info.last_intra_only;
    vp9_hw_regs->swreg33_vp9_info_lastframe.sw_vp9_last_widthheight_eqcur                   = (pic_param->width == reg_cxt->ls_info.last_width) && (pic_param->height == reg_cxt->ls_info.last_height);

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
    reg_cxt->ls_info.abs_delta_last = pic_param->stVP9Segments.abs_delta;
    for (i = 0 ; i < 4; i ++) {
        reg_cxt->ls_info.last_ref_deltas[i] = pic_param->ref_deltas[i];
    }

    for (i = 0 ; i < 2; i ++) {
        reg_cxt->ls_info.last_mode_deltas[i] = pic_param->mode_deltas[i];
    }

    for (i = 0; i < 8; i++) {
        reg_cxt->ls_info.feature_data[i][0] = pic_param->stVP9Segments.feature_data[i][0];
        reg_cxt->ls_info.feature_data[i][1] = pic_param->stVP9Segments.feature_data[i][1];
        reg_cxt->ls_info.feature_data[i][2] = pic_param->stVP9Segments.feature_data[i][2];
        reg_cxt->ls_info.feature_data[i][3] = pic_param->stVP9Segments.feature_data[i][3];
        reg_cxt->ls_info.feature_mask[i]  = pic_param->stVP9Segments.feature_mask[i];
    }
    if (!reg_cxt->ls_info.segmentation_enable_flag_last)
        reg_cxt->ls_info.segmentation_enable_flag_last = pic_param->stVP9Segments.enabled;

    reg_cxt->ls_info.last_show_frame = pic_param->show_frame;
    reg_cxt->ls_info.last_width = pic_param->width;
    reg_cxt->ls_info.last_height = pic_param->height;
    reg_cxt->ls_info.last_intra_only = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_dbg_par("stVP9Segments.enabled %d show_frame %d  width %d  height %d last_intra_only %d",
                     pic_param->stVP9Segments.enabled, pic_param->show_frame,
                     pic_param->width, pic_param->height,
                     reg_cxt->ls_info.last_intra_only);

    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *ctx = (HalVp9dCtx *)hal;
    VP9_REGS *hw_regs = (VP9_REGS *)ctx->hw_regs;
    MppDev dev = ctx->dev;

    if (ctx->fast_mode) {
        RK_S32 index =  task->dec.reg_index;
        hw_regs = (VP9_REGS *)ctx->g_buf[index].hw_regs;
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

        wr_cfg.reg = ctx->hw_regs;
        wr_cfg.size = reg_size;
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = ctx->hw_regs;
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
    HalVp9dCtx *ctx = (HalVp9dCtx *)hal;
    VP9_REGS *hw_regs = ctx->hw_regs;

    if (ctx->fast_mode)
        hw_regs = (VP9_REGS *)ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
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
        mpp_buf_slot_get_prop(ctx->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

    if (ctx->int_cb.callBack && task->dec.flags.wait_done) {
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
        hal_vp9d_update_counts(mpp_buffer_get_ptr(ctx->count_base), task->dec.syntax.data);
        ctx->int_cb.callBack(ctx->int_cb.opaque, (void*)&pic_param->counts);
    }
    if (ctx->fast_mode) {
        ctx->g_buf[task->dec.reg_index].use_flag = 0;
    }

    (void)task;
    return ret;
}

MPP_RET hal_vp9d_rkv_reset(void *hal)
{
    HalVp9dCtx *reg_cxt = (HalVp9dCtx *)hal;

    hal_vp9d_enter();

    memset(&reg_cxt->ls_info, 0, sizeof(reg_cxt->ls_info));
    reg_cxt->mv_base_addr = -1;
    reg_cxt->pre_mv_base_addr = -1;
    reg_cxt->last_segid_flag = 1;

    hal_vp9d_leave();

    return MPP_OK;
}

MPP_RET hal_vp9d_rkv_flush(void *hal)
{
    HalVp9dCtx *reg_cxt = (HalVp9dCtx *)hal;

    hal_vp9d_enter();

    reg_cxt->mv_base_addr = -1;
    reg_cxt->pre_mv_base_addr = -1;

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
