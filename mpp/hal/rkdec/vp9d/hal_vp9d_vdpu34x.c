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

#define MODULE_TAG "hal_vp9d_vdpu34x"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_hal.h"

#include "hal_bufs.h"
#include "hal_vp9d_debug.h"
#include "hal_vp9d_com.h"
#include "hal_vp9d_vdpu34x.h"
#include "vdpu34x_vp9d.h"
#include "vp9d_syntax.h"

#define MAX_GEN_REG 3

typedef struct Vp9dLastInfo_t {
    RK_S32      abs_delta_last;
    RK_S8       last_ref_deltas[4];
    RK_S8       last_mode_deltas[2];
    RK_U8       segmentation_enable_flag_last;
    RK_U8       last_show_frame;
    RK_U8       last_intra_only;
    RK_U32      last_width;
    RK_U32      last_height;
    RK_S16      feature_data[8][4];
    RK_U8       feature_mask[8];
} Vp9dLastInfo;

typedef struct Vp9dRegBuf_t {
    RK_S32      use_flag;
    MppBuffer   probe_base;
    MppBuffer   count_base;
    MppBuffer   segid_cur_base;
    MppBuffer   segid_last_base;
    void        *hw_regs;
} Vp9dRegBuf;

typedef struct Vdpu34xVp9dCtx_t {
    /* for hal api call back */
    const MppHalApi *api;

    /* for hardware info */
    MppClientType   client_type;
    RK_U32          hw_id;
    MppDev          dev;

    MppBufSlots     slots;
    MppBufSlots     packet_slots;
    MppBufferGroup  group;
    Vp9dRegBuf      g_buf[MAX_GEN_REG];
    MppBuffer       probe_base;
    MppBuffer       count_base;
    MppBuffer       segid_cur_base;
    MppBuffer       segid_last_base;
    void*           hw_regs;
    IOInterruptCB   int_cb;
    RK_S32          mv_base_addr;
    RK_S32          pre_mv_base_addr;
    Vp9dLastInfo    ls_info;
    /*
     * swap between segid_cur_base & segid_last_base
     * 0  used segid_cur_base as last
     * 1  used segid_last_base as
     */
    RK_U32          last_segid_flag;
    RK_U32          fast_mode;
    RK_S32          width;
    RK_S32          height;
    RK_S32          rcb_buf_size;
    RK_S32          rcb_size[RCB_BUF_COUNT];
    RK_S32          rcb_offset[RCB_BUF_COUNT];
    MppBuffer       rcb_buf;
    HalBufs         cmv_bufs;
    RK_S32          mv_size;
    RK_S32          mv_count;
} Vdpu34xVp9dCtx;

static MPP_RET hal_vp9d_alloc_res(Vdpu34xVp9dCtx *reg_ctx)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_ctx->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            reg_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu34xVp9dRegSet));
            ret = mpp_buffer_get(reg_ctx->group,
                                 &reg_ctx->g_buf[i].probe_base, PROBE_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_ctx->group,
                                 &reg_ctx->g_buf[i].count_base, COUNT_SIZE);
            if (ret) {
                mpp_err("vp9 count_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_ctx->group,
                                 &reg_ctx->g_buf[i].segid_cur_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_cur_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(reg_ctx->group,
                                 &reg_ctx->g_buf[i].segid_last_base, MAX_SEGMAP_SIZE);
            if (ret) {
                mpp_err("vp9 segid_last_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        reg_ctx->hw_regs = mpp_calloc_size(void, sizeof(Vdpu34xVp9dRegSet));
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->probe_base, PROBE_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->count_base, COUNT_SIZE);
        if (ret) {
            mpp_err("vp9 count_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->segid_cur_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_cur_base get buffer failed\n");
            return ret;
        }
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->segid_last_base, MAX_SEGMAP_SIZE);
        if (ret) {
            mpp_err("vp9 segid_last_base get buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(Vdpu34xVp9dCtx *reg_ctx)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;

    if (reg_ctx->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (reg_ctx->g_buf[i].probe_base) {
                ret = mpp_buffer_put(reg_ctx->g_buf[i].probe_base);
                if (ret) {
                    mpp_err("vp9 probe_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_ctx->g_buf[i].count_base) {
                ret = mpp_buffer_put(reg_ctx->g_buf[i].count_base);
                if (ret) {
                    mpp_err("vp9 count_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_ctx->g_buf[i].segid_cur_base) {
                ret = mpp_buffer_put(reg_ctx->g_buf[i].segid_cur_base);
                if (ret) {
                    mpp_err("vp9 segid_cur_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_ctx->g_buf[i].segid_last_base) {
                ret = mpp_buffer_put(reg_ctx->g_buf[i].segid_last_base);
                if (ret) {
                    mpp_err("vp9 segid_last_base put buffer failed\n");
                    return ret;
                }
            }
            if (reg_ctx->g_buf[i].hw_regs) {
                mpp_free(reg_ctx->g_buf[i].hw_regs);
                reg_ctx->g_buf[i].hw_regs = NULL;
            }
        }
    } else {
        if (reg_ctx->probe_base) {
            ret = mpp_buffer_put(reg_ctx->probe_base);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
        }
        if (reg_ctx->count_base) {
            ret = mpp_buffer_put(reg_ctx->count_base);
            if (ret) {
                mpp_err("vp9 count_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_ctx->segid_cur_base) {
            ret = mpp_buffer_put(reg_ctx->segid_cur_base);
            if (ret) {
                mpp_err("vp9 segid_cur_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_ctx->segid_last_base) {
            ret = mpp_buffer_put(reg_ctx->segid_last_base);
            if (ret) {
                mpp_err("vp9 segid_last_base put buffer failed\n");
                return ret;
            }
        }
        if (reg_ctx->hw_regs) {
            mpp_free(reg_ctx->hw_regs);
            reg_ctx->hw_regs = NULL;
        }
    }
    if (reg_ctx->rcb_buf) {
        ret = mpp_buffer_put(reg_ctx->rcb_buf);
        if (ret) {
            mpp_err("vp9 rcb_buf put buffer failed\n");
            return ret;
        }
    }

    if (reg_ctx->rcb_buf) {
        ret = hal_bufs_deinit(reg_ctx->cmv_bufs);
        if (ret) {
            mpp_err("vp9 cmv bufs deinit buffer failed\n");
            return ret;
        }
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu34x_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Vdpu34xVp9dCtx *ctx = (Vdpu34xVp9dCtx *)hal;

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

static MPP_RET hal_vp9d_vdpu34x_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    Vdpu34xVp9dCtx *ctx = (Vdpu34xVp9dCtx *)hal;

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

static MPP_RET hal_vp9d_vdpu34x_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32   i;
    RK_U8    bit_depth = 0;
    RK_U32   pic_h[3] = { 0 };
    RK_U32   ref_frame_width_y;
    RK_U32   ref_frame_height_y;
    RK_S32   stream_len = 0, aglin_offset = 0;
    RK_U32   y_hor_virstride, uv_hor_virstride, y_virstride;
    RK_U8   *bitstream = NULL;
    MppBuffer streambuf = NULL;
    RK_U32 sw_y_hor_virstride;
    RK_U32 sw_uv_hor_virstride;
    RK_U32 sw_y_virstride;
    RK_U8  ref_idx = 0;
    RK_U32 *reg_ref_base = 0;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;

    Vdpu34xVp9dCtx *reg_ctx = (Vdpu34xVp9dCtx*)hal;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
    RK_S32 mv_size = pic_param->width * pic_param->height / 2;

    if (reg_ctx ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_ctx->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                reg_ctx->probe_base = reg_ctx->g_buf[i].probe_base;
                reg_ctx->count_base = reg_ctx->g_buf[i].count_base;
                reg_ctx->segid_cur_base = reg_ctx->g_buf[i].segid_cur_base;
                reg_ctx->segid_last_base = reg_ctx->g_buf[i].segid_last_base;
                reg_ctx->hw_regs = reg_ctx->g_buf[i].hw_regs;
                reg_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("vp9 fast mode buf all used\n");
            return MPP_ERR_NOMEM;
        }
    }

    if (reg_ctx->cmv_bufs == NULL || reg_ctx->mv_size < mv_size) {
        size_t size = mv_size;

        if (reg_ctx->cmv_bufs) {
            hal_bufs_deinit(reg_ctx->cmv_bufs);
            reg_ctx->cmv_bufs = NULL;
        }

        hal_bufs_init(&reg_ctx->cmv_bufs);
        if (reg_ctx->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_NOK;
        }
        reg_ctx->mv_size = mv_size;
        reg_ctx->mv_count = mpp_buf_slot_get_count(reg_ctx->slots);
        hal_bufs_setup(reg_ctx->cmv_bufs, reg_ctx->mv_count, 1, &size);
    }

    Vdpu34xVp9dRegSet *vp9_hw_regs = (Vdpu34xVp9dRegSet*)reg_ctx->hw_regs;
    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_output_probe(mpp_buffer_get_ptr(reg_ctx->probe_base), task->dec.syntax.data);
    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
    memset(reg_ctx->hw_regs, 0, sizeof(Vdpu34xVp9dRegSet));
    vp9_hw_regs->common.reg013.cur_pic_is_idr = intraFlag;
    vp9_hw_regs->common.reg009.dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->common.reg016_str_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(reg_ctx->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = vp9_hw_regs->common.reg016_str_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;
    pic_h[0] = vp9_ver_align(pic_param->height);
    pic_h[1] = vp9_ver_align(pic_param->height) / 2;
    pic_h[2] = pic_h[1];

    sw_y_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_uv_hor_virstride = (vp9_hor_align((pic_param->width * bit_depth) >> 3) >> 4);
    sw_y_virstride = pic_h[0] * sw_y_hor_virstride;

    vp9_hw_regs->common.reg018.y_hor_virstride = sw_y_hor_virstride;
    vp9_hw_regs->common.reg019.uv_hor_virstride = sw_uv_hor_virstride;
    vp9_hw_regs->common.reg020_y_virstride.y_virstride = sw_y_virstride;

    if (!pic_param->intra_only && pic_param->frame_type &&
        !pic_param->error_resilient_mode && reg_ctx->ls_info.last_show_frame) {
        reg_ctx->pre_mv_base_addr = reg_ctx->mv_base_addr;
    }

    mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    vp9_hw_regs->common_addr.reg130_decout_base =  mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->common_addr.reg128_rlc_base = mpp_buffer_get_fd(streambuf);
    vp9_hw_regs->common_addr.reg129_rlcwrite_base = mpp_buffer_get_fd(streambuf);

    vp9_hw_regs->vp9d_addr.reg197_cabactbl_base = mpp_buffer_get_fd(reg_ctx->probe_base);
    vp9_hw_regs->vp9d_addr.reg167_count_prob_base  = mpp_buffer_get_fd(reg_ctx->count_base);

    if (reg_ctx->last_segid_flag) {
        vp9_hw_regs->vp9d_addr.reg168_segidlast_base = mpp_buffer_get_fd(reg_ctx->segid_last_base);
        vp9_hw_regs->vp9d_addr.reg169_segidcur_base = mpp_buffer_get_fd(reg_ctx->segid_cur_base);
    } else {
        vp9_hw_regs->vp9d_addr.reg168_segidlast_base = mpp_buffer_get_fd(reg_ctx->segid_cur_base);
        vp9_hw_regs->vp9d_addr.reg169_segidcur_base = mpp_buffer_get_fd(reg_ctx->segid_last_base);
    }

    if (pic_param->stVP9Segments.enabled && pic_param->stVP9Segments.update_map) {
        reg_ctx->last_segid_flag = !reg_ctx->last_segid_flag;
    }
    //set cur colmv base
    mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, task->dec.output);
    vp9_hw_regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
    reg_ctx->mv_base_addr = vp9_hw_regs->common_addr.reg131_colmv_cur_base;
    if (reg_ctx->pre_mv_base_addr < 0) {
        reg_ctx->pre_mv_base_addr = reg_ctx->mv_base_addr;
    }
    vp9_hw_regs->vp9d_addr.reg170_ref_colmv_base = reg_ctx->pre_mv_base_addr;

    vp9_hw_regs->vp9d_param.reg64.cprheader_offset = 0;
    reg_ref_base = (RK_U32*)&vp9_hw_regs->vp9d_addr.reg164_ref_last_base;
    for (i = 0; i < 3; i++) {
        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];
        pic_h[0] = vp9_ver_align(ref_frame_height_y);
        pic_h[1] = vp9_ver_align(ref_frame_height_y) / 2;
        y_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        uv_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
        y_virstride = y_hor_virstride * pic_h[0];

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            mpp_buf_slot_get_prop(reg_ctx->slots, pic_param->ref_frame_map[ref_idx].Index7Bits, SLOT_BUFFER, &framebuf);
        }

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            switch (i) {
            case 0: {
                vp9_hw_regs->vp9d_param.reg106.framewidth_last = ref_frame_width_y;
                vp9_hw_regs->vp9d_param.reg107.frameheight_last = ref_frame_height_y;
                vp9_hw_regs->vp9d_param.reg79.lastfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_param.reg80.lastfuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_param.reg85.lastfy_virstride = y_virstride;
            } break;
            case 1: {
                vp9_hw_regs->vp9d_param.reg108.framewidth_golden = ref_frame_width_y;
                vp9_hw_regs->vp9d_param.reg109.frameheight_golden = ref_frame_height_y;
                vp9_hw_regs->vp9d_param.reg81.goldenfy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_param.reg82.goldenfuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_param.reg86.goldeny_virstride = y_virstride;
            } break;
            case 2: {
                vp9_hw_regs->vp9d_param.reg110.framewidth_alfter = ref_frame_width_y;
                vp9_hw_regs->vp9d_param.reg111.frameheight_alfter = ref_frame_height_y;
                vp9_hw_regs->vp9d_param.reg83.altreffy_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_param.reg84.altreffuv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_param.reg87.altrefy_virstride = y_virstride;
            } break;
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
                reg_ref_base[i] = vp9_hw_regs->common_addr.reg130_decout_base;
            }
            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, pic_param->ref_frame_map[ref_idx].Index7Bits);
            vp9_hw_regs->vp9d_addr.reg181_196_ref_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        } else {
            reg_ref_base[i] = vp9_hw_regs->common_addr.reg130_decout_base;
            vp9_hw_regs->vp9d_addr.reg181_196_ref_colmv_base[i] = vp9_hw_regs->common_addr.reg131_colmv_cur_base;
        }
    }

    for (i = 0; i < 8; i++) {
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_qp_delta_en            = (reg_ctx->ls_info.feature_mask[i]) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_qp_delta               = reg_ctx->ls_info.feature_data[i][0];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_loopfitler_value_en    = (reg_ctx->ls_info.feature_mask[i] >> 1) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_loopfilter_value       = reg_ctx->ls_info.feature_data[i][1];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_referinfo_en                 = (reg_ctx->ls_info.feature_mask[i] >> 2) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_referinfo                    = reg_ctx->ls_info.feature_data[i][2];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_skip_en                = (reg_ctx->ls_info.feature_mask[i] >> 3) & 0x1;
    }

    vp9_hw_regs->vp9d_param.reg67_74[0].segid_abs_delta             = reg_ctx->ls_info.abs_delta_last;
    vp9_hw_regs->vp9d_param.reg76.tx_mode                       = pic_param->txmode;
    vp9_hw_regs->vp9d_param.reg76.frame_reference_mode          = pic_param->refmode;
    vp9_hw_regs->vp9d_param.reg94.ref_deltas_lastframe   = 0;

    if (!intraFlag) {
        for (i = 0; i < 4; i++)
            vp9_hw_regs->vp9d_param.reg94.ref_deltas_lastframe   |= (reg_ctx->ls_info.last_ref_deltas[i] & 0x7f) << (7 * i);

        for (i = 0; i < 2; i++)
            vp9_hw_regs->vp9d_param.reg75.mode_deltas_lastframe        |= (reg_ctx->ls_info.last_mode_deltas[i] & 0x7f) << (7 * i);
    } else {
        reg_ctx->ls_info.segmentation_enable_flag_last = 0;
        reg_ctx->ls_info.last_intra_only = 1;
    }

    vp9_hw_regs->vp9d_param.reg75.mode_deltas_lastframe            = 0;
    vp9_hw_regs->vp9d_param.reg75.segmentation_enable_lstframe     = reg_ctx->ls_info.segmentation_enable_flag_last;
    vp9_hw_regs->vp9d_param.reg75.last_show_frame                  = reg_ctx->ls_info.last_show_frame;
    vp9_hw_regs->vp9d_param.reg75.last_intra_only                  = reg_ctx->ls_info.last_intra_only;
    vp9_hw_regs->vp9d_param.reg75.last_widthheight_eqcur           = (pic_param->width == reg_ctx->ls_info.last_width) && (pic_param->height == reg_ctx->ls_info.last_height);
    vp9_hw_regs->vp9d_param.reg78.lasttile_size                     =  stream_len - pic_param->first_partition_size;


    if (!intraFlag) {
        vp9_hw_regs->vp9d_param.reg88.lref_hor_scale = pic_param->mvscale[0][0];
        vp9_hw_regs->vp9d_param.reg89.lref_ver_scale = pic_param->mvscale[0][1];
        vp9_hw_regs->vp9d_param.reg90.gref_hor_scale = pic_param->mvscale[1][0];
        vp9_hw_regs->vp9d_param.reg91.gref_ver_scale = pic_param->mvscale[1][1];
        vp9_hw_regs->vp9d_param.reg92.aref_hor_scale = pic_param->mvscale[2][0];
        vp9_hw_regs->vp9d_param.reg93.aref_ver_scale = pic_param->mvscale[2][1];
    }

    vp9_hw_regs->common.reg010.dec_e                = 1;
    vp9_hw_regs->common.reg011.dec_timeout_e    = 1;
    vp9_hw_regs->common.reg011.buf_empty_en     = 1;

    //last info  update
    reg_ctx->ls_info.abs_delta_last = pic_param->stVP9Segments.abs_delta;
    for (i = 0 ; i < 4; i ++) {
        reg_ctx->ls_info.last_ref_deltas[i] = pic_param->ref_deltas[i];
    }

    for (i = 0 ; i < 2; i ++) {
        reg_ctx->ls_info.last_mode_deltas[i] = pic_param->mode_deltas[i];
    }

    for (i = 0; i < 8; i++) {
        reg_ctx->ls_info.feature_data[i][0] = pic_param->stVP9Segments.feature_data[i][0];
        reg_ctx->ls_info.feature_data[i][1] = pic_param->stVP9Segments.feature_data[i][1];
        reg_ctx->ls_info.feature_data[i][2] = pic_param->stVP9Segments.feature_data[i][2];
        reg_ctx->ls_info.feature_data[i][3] = pic_param->stVP9Segments.feature_data[i][3];
        reg_ctx->ls_info.feature_mask[i]  = pic_param->stVP9Segments.feature_mask[i];
    }
    if (!reg_ctx->ls_info.segmentation_enable_flag_last)
        reg_ctx->ls_info.segmentation_enable_flag_last = pic_param->stVP9Segments.enabled;

    reg_ctx->ls_info.last_show_frame = pic_param->show_frame;
    reg_ctx->ls_info.last_width = pic_param->width;
    reg_ctx->ls_info.last_height = pic_param->height;
    reg_ctx->ls_info.last_intra_only = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_dbg_par("stVP9Segments.enabled %d show_frame %d  width %d  height %d last_intra_only %d",
                     pic_param->stVP9Segments.enabled, pic_param->show_frame,
                     pic_param->width, pic_param->height,
                     reg_ctx->ls_info.last_intra_only);
    {
        RK_S32 height = vp9_ver_align(pic_param->height);
        RK_S32 width  = vp9_ver_align(pic_param->width);
        MppBuffer rcb_buf = reg_ctx->rcb_buf;

        if (width != reg_ctx->width || height != reg_ctx->height) {
            if (rcb_buf) {
                mpp_buffer_put(rcb_buf);
                rcb_buf = NULL;
            }

            reg_ctx->rcb_buf_size = get_rcb_buf_size(reg_ctx->rcb_size, reg_ctx->rcb_offset,
                                                     width, height);

            mpp_buffer_get(reg_ctx->group, &rcb_buf, reg_ctx->rcb_buf_size);
            reg_ctx->rcb_buf = rcb_buf;
            reg_ctx->width = width;
            reg_ctx->height = height;
        }

        vdpu34x_setup_rcb(&vp9_hw_regs->common_addr, rcb_buf, reg_ctx->rcb_offset);
    }

    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu34x_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Vdpu34xVp9dCtx *ctx = (Vdpu34xVp9dCtx *)hal;
    Vdpu34xVp9dRegSet *hw_regs = (Vdpu34xVp9dRegSet *)ctx->hw_regs;
    MppDev dev = ctx->dev;

    if (ctx->fast_mode) {
        RK_S32 index =  task->dec.reg_index;
        hw_regs = (Vdpu34xVp9dRegSet *)ctx->g_buf[index].hw_regs;
    }

    mpp_assert(hw_regs);

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)ctx->hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(Vdpu34xVp9dRegSet) / 4; i++)
            mpp_log("set regs[%02d]: %08X\n", i, *p++);
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->common;
        wr_cfg.size = sizeof(hw_regs->common);
        wr_cfg.offset = OFFSET_COMMON_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->vp9d_param;
        wr_cfg.size = sizeof(hw_regs->vp9d_param);
        wr_cfg.offset = OFFSET_CODEC_PARAMS_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->common_addr;
        wr_cfg.size = sizeof(hw_regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->vp9d_addr;
        wr_cfg.size = sizeof(hw_regs->vp9d_addr);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->irq_status;
        rd_cfg.size = sizeof(hw_regs->irq_status);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;

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

static MPP_RET hal_vp9d_vdpu34x_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Vdpu34xVp9dCtx *ctx = (Vdpu34xVp9dCtx *)hal;
    Vdpu34xVp9dRegSet *hw_regs = (Vdpu34xVp9dRegSet *)ctx->hw_regs;

    if (ctx->fast_mode)
        hw_regs = (Vdpu34xVp9dRegSet *)ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(Vdpu34xVp9dRegSet) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i, *p++);
    }

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        !hw_regs->irq_status.reg224.dec_rdy_sta) {
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

static MPP_RET hal_vp9d_vdpu34x_reset(void *hal)
{
    Vdpu34xVp9dCtx *reg_ctx = (Vdpu34xVp9dCtx *)hal;

    hal_vp9d_enter();

    memset(&reg_ctx->ls_info, 0, sizeof(reg_ctx->ls_info));
    reg_ctx->mv_base_addr = -1;
    reg_ctx->pre_mv_base_addr = -1;
    reg_ctx->last_segid_flag = 1;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu34x_flush(void *hal)
{
    Vdpu34xVp9dCtx *reg_ctx = (Vdpu34xVp9dCtx *)hal;

    hal_vp9d_enter();

    reg_ctx->mv_base_addr = -1;
    reg_ctx->pre_mv_base_addr = -1;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu34x_control(void *hal, MpiCmd cmd_type, void *param)
{
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO : {
        /* commit buffer stride */
        RK_U32 width = mpp_frame_get_width((MppFrame)param);
        RK_U32 height = mpp_frame_get_height((MppFrame)param);

        mpp_frame_set_hor_stride((MppFrame)param, vp9_hor_align(width));
        mpp_frame_set_ver_stride((MppFrame)param, vp9_ver_align(height));
    } break;
    default : {
    } break;
    }

    (void)hal;

    return MPP_OK;
}

const MppHalApi hal_vp9d_vdpu34x = {
    .name = "vp9d_vdpu34x",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(Vdpu34xVp9dCtx),
    .flag = 0,
    .init = hal_vp9d_vdpu34x_init,
    .deinit = hal_vp9d_vdpu34x_deinit,
    .reg_gen = hal_vp9d_vdpu34x_gen_regs,
    .start = hal_vp9d_vdpu34x_start,
    .wait = hal_vp9d_vdpu34x_wait,
    .reset = hal_vp9d_vdpu34x_reset,
    .flush = hal_vp9d_vdpu34x_flush,
    .control = hal_vp9d_vdpu34x_control,
};
