/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vp9d_vdpu382"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_hal.h"

#include "hal_bufs.h"
#include "hal_vp9d_debug.h"
#include "hal_vp9d_com.h"
#include "hal_vp9d_vdpu382.h"
#include "hal_vp9d_ctx.h"
#include "vdpu382_vp9d.h"
#include "vp9d_syntax.h"

#define HW_PROB 1
#define VP9_CONTEXT 4
#define VP9_CTU_SIZE 64
#define PROB_SIZE_ALIGN_TO_4K MPP_ALIGN(PROB_SIZE, SZ_4K)
#define COUNT_SIZE_ALIGN_TO_4K MPP_ALIGN(COUNT_SIZE, SZ_4K)
#define MAX_SEGMAP_SIZE_ALIGN_TO_4K MPP_ALIGN(MAX_SEGMAP_SIZE, SZ_4K)

#define VDPU382_OFFSET_COUNT (PROB_SIZE_ALIGN_TO_4K)
#define VDPU382_PROBE_BUFFER_SIZE (PROB_SIZE_ALIGN_TO_4K + COUNT_SIZE_ALIGN_TO_4K)

typedef struct Vdpu382Vp9dCtx_t {
    Vp9dRegBuf      g_buf[MAX_GEN_REG];
    MppBuffer       probe_base;
    MppBuffer       seg_base;
    RK_U32          offset_count;
    RK_U32          offset_segid_cur;
    RK_U32          offset_segid_last;
    MppBuffer       prob_default_base;
    void*           hw_regs;
    RK_S32          mv_base_addr;
    RK_S32          pre_mv_base_addr;
    Vp9dLastInfo    ls_info;
    /*
     * swap between segid_cur_base & segid_last_base
     * 0  used segid_cur_base as last
     * 1  used segid_last_base as
     */
    RK_U32          last_segid_flag;
    RK_S32          width;
    RK_S32          height;
    /* rcb buffers info */
    RK_S32          rcb_buf_size;
    Vdpu382RcbInfo  rcb_info[RCB_BUF_COUNT];
    MppBuffer       rcb_buf;
    RK_U32          num_row_tiles;
    RK_U32          bit_depth;
    /* colmv buffers info */
    HalBufs         cmv_bufs;
    RK_S32          mv_size;
    RK_S32          mv_count;
    RK_U32          prob_ctx_valid[VP9_CONTEXT];
    MppBuffer       prob_loop_base[VP9_CONTEXT];
    RK_U32          prob_ref_poc[VP9_CONTEXT];
    RK_U32          col_ref_poc;
    RK_U32          segid_ref_poc;
} Vdpu382Vp9dCtx;

static MPP_RET hal_vp9d_alloc_res(HalVp9dCtx *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    hw_ctx->offset_count = VDPU382_OFFSET_COUNT;
    hw_ctx->offset_segid_cur = 0;
    hw_ctx->offset_segid_last = MAX_SEGMAP_SIZE_ALIGN_TO_4K;
    /* alloc common buffer */
    for (i = 0; i < VP9_CONTEXT; i++) {
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->prob_loop_base[i], PROB_SIZE);
        if (ret) {
            mpp_err("vp9 probe_loop_base get buffer failed\n");
            return ret;
        }
    }
    ret = mpp_buffer_get(p_hal->group, &hw_ctx->prob_default_base, PROB_SIZE);
    if (ret) {
        mpp_err("vp9 probe_default_base get buffer failed\n");
        return ret;
    }
    /* alloc buffer for fast mode or normal */
    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            hw_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu382Vp9dRegSet));
            ret = mpp_buffer_get(p_hal->group, &hw_ctx->g_buf[i].probe_base, VDPU382_PROBE_BUFFER_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        hw_ctx->hw_regs = mpp_calloc_size(void, sizeof(Vdpu382Vp9dRegSet));
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->probe_base, VDPU382_PROBE_BUFFER_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
    }

    ret = mpp_buffer_get(p_hal->group, &hw_ctx->seg_base, MAX_SEGMAP_SIZE_ALIGN_TO_4K * 2);
    if (ret) {
        mpp_err("vp9 segid_base get buffer failed\n");
        return ret;
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(HalVp9dCtx *hal)
{
    RK_S32 i = 0;
    RK_S32 ret = 0;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;

    if (hw_ctx->prob_default_base) {
        ret = mpp_buffer_put(hw_ctx->prob_default_base);
        if (ret) {
            mpp_err("vp9 probe_wr_base put buffer failed\n");
            return ret;
        }
    }
    for (i = 0; i < VP9_CONTEXT; i++) {
        if (hw_ctx->prob_loop_base[i]) {
            ret = mpp_buffer_put(hw_ctx->prob_loop_base[i]);
            if (ret) {
                mpp_err("vp9 probe_base put buffer failed\n");
                return ret;
            }
        }
    }
    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (hw_ctx->g_buf[i].probe_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].probe_base);
                if (ret) {
                    mpp_err("vp9 probe_base put buffer failed\n");
                    return ret;
                }
            }
            if (hw_ctx->g_buf[i].hw_regs) {
                mpp_free(hw_ctx->g_buf[i].hw_regs);
                hw_ctx->g_buf[i].hw_regs = NULL;
            }
            if (hw_ctx->g_buf[i].rcb_buf) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].rcb_buf);
                if (ret) {
                    mpp_err("vp9 rcb_buf[%d] put buffer failed\n", i);
                    return ret;
                }
            }
        }
    } else {
        if (hw_ctx->probe_base) {
            ret = mpp_buffer_put(hw_ctx->probe_base);
            if (ret) {
                mpp_err("vp9 probe_base put buffer failed\n");
                return ret;
            }
        }

        if (hw_ctx->hw_regs) {
            mpp_free(hw_ctx->hw_regs);
            hw_ctx->hw_regs = NULL;
        }
        if (hw_ctx->rcb_buf) {
            ret = mpp_buffer_put(hw_ctx->rcb_buf);
            if (ret) {
                mpp_err("vp9 rcb_buf put buffer failed\n");
                return ret;
            }
        }
    }

    if (hw_ctx->cmv_bufs) {
        ret = hal_bufs_deinit(hw_ctx->cmv_bufs);
        if (ret) {
            mpp_err("vp9 cmv bufs deinit buffer failed\n");
            return ret;
        }
    }

    if (hw_ctx->seg_base) {
        ret = mpp_buffer_put(hw_ctx->seg_base);
        if (ret) {
            mpp_err("vp9 seg_base put buffer failed\n");
            return ret;
        }
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu382_deinit(void *hal)
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

static MPP_RET hal_vp9d_vdpu382_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    MEM_CHECK(ret, p_hal->hw_ctx = mpp_calloc_size(void, sizeof(Vdpu382Vp9dCtx)));
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, vp9_hor_align);
    mpp_slots_set_prop(p_hal->slots, SLOTS_VER_ALIGN, vp9_ver_align);

    if (p_hal->group == NULL) {
        ret = mpp_buffer_group_get_internal(&p_hal->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("vp9 mpp_buffer_group_get failed\n");
            goto __FAILED;
        }
    }

    ret = hal_vp9d_alloc_res(p_hal);
    if (ret) {
        mpp_err("hal_vp9d_alloc_res failed\n");
        goto __FAILED;
    }

    hw_ctx->last_segid_flag = 1;

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu382_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 0;
    }

    return ret;
__FAILED:
    hal_vp9d_vdpu382_deinit(hal);
    return ret;
}

static void vp9d_refine_rcb_size(Vdpu382RcbInfo *rcb_info,
                                 Vdpu382Vp9dRegSet *vp9_hw_regs,
                                 RK_S32 width, RK_S32 height, void* data)
{
    RK_U32 rcb_bits = 0;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)data;
    RK_U32 num_tiles_col = 1 << pic_param->log2_tile_cols;
    RK_U32 bit_depth = pic_param->BitDepthMinus8Luma + 8;
    RK_U32 ext_align_size = num_tiles_col * 64 * 8;

    width = MPP_ALIGN(width, VP9_CTU_SIZE);
    height = MPP_ALIGN(height, VP9_CTU_SIZE);
    /* RCB_STRMD_ROW */
    if (width >= 4096)
        rcb_bits = MPP_ALIGN(width, 64) * 232 + ext_align_size;
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_TRANSD_ROW */
    if (width >= 8192)
        rcb_bits = (MPP_ALIGN(width - 8192, 4) << 1) + ext_align_size;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_TRANSD_COL */
    if ((height >= 8192) && (num_tiles_col > 1))
        rcb_bits = (MPP_ALIGN(height - 8192, 4) << 1);
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_COL].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTER_ROW */
    rcb_bits = width * 36 + ext_align_size;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTER_COL */
    rcb_info[RCB_INTER_COL].size = 0;

    /* RCB_INTRA_ROW */
    rcb_bits = width * 2 * 11 + ext_align_size;
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_DBLK_ROW */
    rcb_bits = width * (0.5 + 16 * bit_depth) + num_tiles_col * 192 * bit_depth + ext_align_size;
    rcb_info[RCB_DBLK_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_SAO_ROW */
    rcb_info[RCB_SAO_ROW].size = 0;

    /* RCB_FBC_ROW */
    if (vp9_hw_regs->common.reg012.fbc_e) {
        rcb_bits = 8 * width * bit_depth + ext_align_size;
    } else
        rcb_bits = 0;
    rcb_info[RCB_FBC_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_FILT_COL */
    if (num_tiles_col > 1) {
        if (vp9_hw_regs->common.reg012.fbc_e) {
            rcb_bits = height * (4 + 24 *  bit_depth);
        } else
            rcb_bits = height * (4 + 16 *  bit_depth);
    } else
        rcb_bits = 0;
    rcb_info[RCB_FILT_COL].size = MPP_RCB_BYTES(rcb_bits);
}

static void hal_vp9d_rcb_info_update(void *hal,  Vdpu382Vp9dRegSet *hw_regs, void *data)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)data;
    RK_U32 num_tiles = pic_param->log2_tile_rows;
    RK_U32 bit_depth = pic_param->BitDepthMinus8Luma + 8;
    RK_S32 height = vp9_ver_align(pic_param->height);
    RK_S32 width  = vp9_ver_align(pic_param->width);

    if (hw_ctx->num_row_tiles != num_tiles ||
        hw_ctx->bit_depth != bit_depth ||
        hw_ctx->width != width ||
        hw_ctx->height != height) {

        hw_ctx->rcb_buf_size = vdpu382_get_rcb_buf_size(hw_ctx->rcb_info, width, height);
        vp9d_refine_rcb_size(hw_ctx->rcb_info, hw_regs, width, height, pic_param);

        if (p_hal->fast_mode) {
            RK_U32 i;

            for (i = 0; i < MPP_ARRAY_ELEMS(hw_ctx->g_buf); i++) {
                MppBuffer rcb_buf = hw_ctx->g_buf[i].rcb_buf;

                if (rcb_buf) {
                    mpp_buffer_put(rcb_buf);
                    hw_ctx->g_buf[i].rcb_buf = NULL;
                }
                mpp_buffer_get(p_hal->group, &rcb_buf, hw_ctx->rcb_buf_size);
                hw_ctx->g_buf[i].rcb_buf = rcb_buf;
            }
        } else {
            MppBuffer rcb_buf = hw_ctx->rcb_buf;

            if (rcb_buf) {
                mpp_buffer_put(rcb_buf);
                rcb_buf = NULL;
            }
            mpp_buffer_get(p_hal->group, &rcb_buf, hw_ctx->rcb_buf_size);
            hw_ctx->rcb_buf = rcb_buf;
        }

        hw_ctx->num_row_tiles  = num_tiles;
        hw_ctx->bit_depth      = bit_depth;
        hw_ctx->width          = width;
        hw_ctx->height         = height;
    }
}


static MPP_RET hal_vp9d_vdpu382_setup_colmv_buf(void *hal, HalTaskInfo *task)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
    RK_U32 width = pic_param->width;
    RK_U32 height = pic_param->height;
    RK_S32 mv_size = 0, colmv_size = 8, colmv_byte = 16;
    RK_U32 compress = p_hal->hw_info ? p_hal->hw_info->cap_colmv_compress : 1;

    mv_size = vdpu382_get_colmv_size(width, height, VP9_CTU_SIZE, colmv_byte, colmv_size, compress);
    if (hw_ctx->cmv_bufs == NULL || hw_ctx->mv_size < mv_size) {
        size_t size = mv_size;

        if (hw_ctx->cmv_bufs) {
            hal_bufs_deinit(hw_ctx->cmv_bufs);
            hw_ctx->cmv_bufs = NULL;
        }

        hal_bufs_init(&hw_ctx->cmv_bufs);
        if (hw_ctx->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_ERR_NOMEM;
        }
        hw_ctx->mv_size = mv_size;
        hw_ctx->mv_count = mpp_buf_slot_get_count(p_hal ->slots);
        hal_bufs_setup(hw_ctx->cmv_bufs, hw_ctx->mv_count, 1, &size);
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu382_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32   i;
    RK_U8    bit_depth = 0;
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
    RK_U8  ref_frame_idx = 0;
    RK_U32 *reg_ref_base = 0;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;
    RK_U32 fbc_en = 0;

    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
    RK_U32 frame_ctx_id = pic_param->frame_context_idx;

    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!hw_ctx->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                hw_ctx->probe_base = hw_ctx->g_buf[i].probe_base;

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

    if (hal_vp9d_vdpu382_setup_colmv_buf(hal, task))
        return MPP_ERR_NOMEM;

    Vdpu382Vp9dRegSet *vp9_hw_regs = (Vdpu382Vp9dRegSet*)hw_ctx->hw_regs;
    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
    memset(hw_ctx->hw_regs, 0, sizeof(Vdpu382Vp9dRegSet));
#if HW_PROB
    hal_vp9d_prob_flag_delta(mpp_buffer_get_ptr(hw_ctx->probe_base), task->dec.syntax.data);
    mpp_buffer_sync_end(hw_ctx->probe_base);
    if (intraFlag) {
        hal_vp9d_prob_default(mpp_buffer_get_ptr(hw_ctx->prob_default_base), task->dec.syntax.data);
        mpp_buffer_sync_end(hw_ctx->prob_default_base);
    }

    /* config reg103 */
    vp9_hw_regs->vp9d_param.reg103.prob_update_en   = 1;
    vp9_hw_regs->vp9d_param.reg103.intra_only_flag  = intraFlag;
    if (!intraFlag) {
        vp9_hw_regs->vp9d_param.reg103.txfmmode_rfsh_en = (pic_param->txmode == 4) ? 1 : 0;
        vp9_hw_regs->vp9d_param.reg103.interp_filter_switch_en = pic_param->interp_filter == 4 ? 1 : 0;
    }
    vp9_hw_regs->vp9d_param.reg103.ref_mode_rfsh_en     = 1;
    vp9_hw_regs->vp9d_param.reg103.single_ref_rfsh_en   = 1;
    vp9_hw_regs->vp9d_param.reg103.comp_ref_rfsh_en     = 1;
    vp9_hw_regs->vp9d_param.reg103.inter_coef_rfsh_flag = 0;
    vp9_hw_regs->vp9d_param.reg103.refresh_en           =
        !pic_param->error_resilient_mode && !pic_param->parallelmode;
    vp9_hw_regs->vp9d_param.reg103.prob_save_en             = pic_param->refresh_frame_context;
    vp9_hw_regs->vp9d_param.reg103.allow_high_precision_mv  = pic_param->allow_high_precision_mv;
    vp9_hw_regs->vp9d_param.reg103.last_key_frame_flag      = hw_ctx->ls_info.last_intra_only;

    /* set info for multi core */
    {
        MppFrame mframe = NULL;

        vp9_hw_regs->common.reg028.sw_poc_arb_flag = 1;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        vp9_hw_regs->vp9d_param.reg65.cur_poc = mframe ? mpp_frame_get_poc(mframe) : 0;
        // last poc
        ref_idx = pic_param->frame_refs[0].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        if (ref_frame_idx < 0x7f) {
            mframe = NULL;
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_FRAME_PTR, &mframe);
            vp9_hw_regs->vp9d_param.reg95.last_poc = mframe ? mpp_frame_get_poc(mframe) : 0;
        }
        // golden poc
        ref_idx = pic_param->frame_refs[1].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        if (ref_frame_idx < 0x7f) {
            mframe = NULL;
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_FRAME_PTR, &mframe);
            vp9_hw_regs->vp9d_param.reg96.golden_poc = mframe ? mpp_frame_get_poc(mframe) : 0;
        }
        // altref poc
        ref_idx = pic_param->frame_refs[2].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        if (ref_frame_idx < 0x7f) {
            mframe = NULL;
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_FRAME_PTR, &mframe);
            vp9_hw_regs->vp9d_param.reg97.altref_poc = mframe ? mpp_frame_get_poc(mframe) : 0;
        }
        // colref poc
        vp9_hw_regs->vp9d_param.reg98.col_ref_poc =
            hw_ctx->col_ref_poc ? hw_ctx->col_ref_poc : vp9_hw_regs->vp9d_param.reg65.cur_poc;
        if (pic_param->show_frame && !pic_param->show_existing_frame)
            hw_ctx->col_ref_poc = vp9_hw_regs->vp9d_param.reg65.cur_poc;
        // segment id ref poc
        vp9_hw_regs->vp9d_param.reg100.segid_ref_poc = hw_ctx->segid_ref_poc;

        vp9_hw_regs->vp9d_addr.reg169_segidcur_base = mpp_buffer_get_fd(hw_ctx->seg_base);
        vp9_hw_regs->vp9d_addr.reg168_segidlast_base = mpp_buffer_get_fd(hw_ctx->seg_base);
        if (hw_ctx->last_segid_flag) {
            mpp_dev_set_reg_offset(p_hal->dev, 168, hw_ctx->offset_segid_last);
            mpp_dev_set_reg_offset(p_hal->dev, 169, hw_ctx->offset_segid_cur);
        } else {
            mpp_dev_set_reg_offset(p_hal->dev, 168, hw_ctx->offset_segid_cur);
            mpp_dev_set_reg_offset(p_hal->dev, 169, hw_ctx->offset_segid_last);
        }

        if ((pic_param->stVP9Segments.enabled && pic_param->stVP9Segments.update_map) ||
            (hw_ctx->ls_info.last_width != pic_param->width) ||
            (hw_ctx->ls_info.last_height != pic_param->height) ||
            intraFlag || pic_param->error_resilient_mode) {
            hw_ctx->segid_ref_poc = vp9_hw_regs->vp9d_param.reg65.cur_poc;
            hw_ctx->last_segid_flag = !hw_ctx->last_segid_flag;
            vp9_hw_regs->vp9d_param.reg100.segid_ref_poc = 0;
            vp9_hw_regs->vp9d_param.reg75.vp9_segment_id_update = 1;
        } else
            vp9_hw_regs->vp9d_param.reg75.vp9_segment_id_update = 0;
    }

    /* config last prob base and update write base */
    {

        if (intraFlag || pic_param->error_resilient_mode) {
            if (intraFlag
                || pic_param->error_resilient_mode
                || (pic_param->reset_frame_context == 3)) {
                memset(hw_ctx->prob_ctx_valid, 0, sizeof(hw_ctx->prob_ctx_valid));
            } else if (pic_param->reset_frame_context == 2) {
                hw_ctx->prob_ctx_valid[frame_ctx_id] = 0;
            }
        }

#if VP9_DUMP
        {
            static RK_U32 file_cnt = 0;
            char file_name[128];
            RK_U32 i = 0;
            sprintf(file_name, "/data/vp9/prob_last_%d.txt", file_cnt);
            FILE *fp = fopen(file_name, "wb");
            RK_U32 *tmp = NULL;
            if (hw_ctx->prob_ctx_valid[frame_ctx_id]) {
                tmp = (RK_U32 *)mpp_buffer_get_ptr(hw_ctx->prob_loop_base[pic_param->frame_context_idx]);
            } else {
                tmp = (RK_U32 *)mpp_buffer_get_ptr(hw_ctx->prob_default_base);
            }
            for (i = 0; i < PROB_SIZE / 4; i += 2) {
                fprintf(fp, "%08x%08x\n", tmp[i + 1], tmp[i]);
            }
            file_cnt++;
            fflush(fp);
            fclose(fp);
        }
#endif

        if (hw_ctx->prob_ctx_valid[frame_ctx_id]) {
            vp9_hw_regs->vp9d_addr.reg162_last_prob_base =
                mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
            vp9_hw_regs->common.reg028.swreg_vp9_rd_prob_idx = frame_ctx_id + 1;
            vp9_hw_regs->vp9d_param.reg99.prob_ref_poc = hw_ctx->prob_ref_poc[frame_ctx_id];
        } else {
            vp9_hw_regs->vp9d_addr.reg162_last_prob_base = mpp_buffer_get_fd(hw_ctx->prob_default_base);
            hw_ctx->prob_ctx_valid[frame_ctx_id] |= pic_param->refresh_frame_context;
            vp9_hw_regs->common.reg028.swreg_vp9_rd_prob_idx = 0;
            vp9_hw_regs->vp9d_param.reg99.prob_ref_poc = 0;
            if (pic_param->refresh_frame_context)
                hw_ctx->prob_ref_poc[frame_ctx_id] = vp9_hw_regs->vp9d_param.reg65.cur_poc;
        }
        vp9_hw_regs->vp9d_addr.reg172_update_prob_wr_base =
            mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
        vp9_hw_regs->common.reg028.swreg_vp9_wr_prob_idx = frame_ctx_id + 1;

    }
    vp9_hw_regs->vp9d_addr.reg160_delta_prob_base = mpp_buffer_get_fd(hw_ctx->probe_base);
#else
    hal_vp9d_output_probe(mpp_buffer_get_ptr(hw_ctx->probe_base), task->dec.syntax.data);
    mpp_buffer_sync_end(hw_ctx->probe_base);
#endif
    vp9_hw_regs->common.reg012.colmv_compress_en = p_hal->hw_info ? p_hal->hw_info->cap_colmv_compress : 1;
    vp9_hw_regs->common.reg013.cur_pic_is_idr = !pic_param->frame_type;
    vp9_hw_regs->common.reg009.dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->common.reg016_str_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(p_hal ->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = vp9_hw_regs->common.reg016_str_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;

    {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        fbc_en = MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe));

        if (fbc_en) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 h = MPP_ALIGN(mpp_frame_get_height(mframe), 64);
            RK_U32 fbd_offset = MPP_ALIGN(fbc_hdr_stride * (h + 16) / 16, SZ_4K);

            vp9_hw_regs->common.reg012.fbc_e = 1;
            vp9_hw_regs->common.reg018.y_hor_virstride = fbc_hdr_stride >> 4;
            vp9_hw_regs->common.reg019.uv_hor_virstride = fbc_hdr_stride >> 4;
            vp9_hw_regs->common.reg020_fbc_payload_off.payload_st_offset = fbd_offset >> 4;
        } else {
            sw_y_hor_virstride = mpp_frame_get_hor_stride(mframe) >> 4;
            sw_uv_hor_virstride = sw_y_hor_virstride;
            sw_y_virstride = mpp_frame_get_ver_stride(mframe) * sw_y_hor_virstride;

            vp9_hw_regs->common.reg012.fbc_e = 0;
            vp9_hw_regs->common.reg018.y_hor_virstride = sw_y_hor_virstride;
            vp9_hw_regs->common.reg019.uv_hor_virstride = sw_uv_hor_virstride;
            vp9_hw_regs->common.reg020_y_virstride.y_virstride = sw_y_virstride;
        }
    }
    if (!pic_param->intra_only && pic_param->frame_type &&
        !pic_param->error_resilient_mode && hw_ctx->ls_info.last_show_frame) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
    }

    mpp_buf_slot_get_prop(p_hal ->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    vp9_hw_regs->common_addr.reg130_decout_base =  mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->common_addr.reg128_rlc_base = mpp_buffer_get_fd(streambuf);
    vp9_hw_regs->common_addr.reg129_rlcwrite_base = mpp_buffer_get_fd(streambuf);

    vp9_hw_regs->vp9d_addr.reg167_count_prob_base = mpp_buffer_get_fd(hw_ctx->probe_base);
    mpp_dev_set_reg_offset(p_hal->dev, 167, hw_ctx->offset_count);

    //set cur colmv base
    mv_buf = hal_bufs_get_buf(hw_ctx->cmv_bufs, task->dec.output);
    vp9_hw_regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
    hw_ctx->mv_base_addr = vp9_hw_regs->common_addr.reg131_colmv_cur_base;
    if (hw_ctx->pre_mv_base_addr < 0) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
    }
    vp9_hw_regs->vp9d_addr.reg170_ref_colmv_base = hw_ctx->pre_mv_base_addr;

    vp9_hw_regs->vp9d_param.reg64.cprheader_offset = 0;
    reg_ref_base = (RK_U32*)&vp9_hw_regs->vp9d_addr.reg164_ref_last_base;
    for (i = 0; i < 3; i++) {
        MppFrame frame = NULL;

        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];

        if (ref_frame_idx < 0x7f)
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_FRAME_PTR, &frame);

        if (fbc_en && frame) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(frame);
            RK_U32 h = MPP_ALIGN(mpp_frame_get_height(frame), 64);
            RK_U32 fbd_offset = MPP_ALIGN(fbc_hdr_stride * (h + 16) / 16, SZ_4K);

            y_hor_virstride = uv_hor_virstride = fbc_hdr_stride >> 4;
            y_virstride = fbd_offset;
        } else {
            if (frame) {
                y_hor_virstride = uv_hor_virstride = mpp_frame_get_hor_stride(frame) >> 4;
                y_virstride = y_hor_virstride * mpp_frame_get_ver_stride(frame);
            } else {
                y_hor_virstride = uv_hor_virstride = (vp9_hor_align((ref_frame_width_y * bit_depth) >> 3) >> 4);
                y_virstride = y_hor_virstride * vp9_ver_align(ref_frame_height_y);
            }
        }

        if (pic_param->ref_frame_map[ref_idx].Index7Bits < 0x7f) {
            mpp_buf_slot_get_prop(p_hal ->slots, pic_param->ref_frame_map[ref_idx].Index7Bits, SLOT_BUFFER, &framebuf);
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
            mv_buf = hal_bufs_get_buf(hw_ctx->cmv_bufs, pic_param->ref_frame_map[ref_idx].Index7Bits);
            vp9_hw_regs->vp9d_addr.reg181_196_ref_colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        } else {
            reg_ref_base[i] = vp9_hw_regs->common_addr.reg130_decout_base;
            vp9_hw_regs->vp9d_addr.reg181_196_ref_colmv_base[i] = vp9_hw_regs->common_addr.reg131_colmv_cur_base;
        }
    }

    for (i = 0; i < 8; i++) {
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_qp_delta_en         = (hw_ctx->ls_info.feature_mask[i]) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_qp_delta            = hw_ctx->ls_info.feature_data[i][0];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_loopfitler_value_en = (hw_ctx->ls_info.feature_mask[i] >> 1) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_loopfilter_value    = hw_ctx->ls_info.feature_data[i][1];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_referinfo_en              = (hw_ctx->ls_info.feature_mask[i] >> 2) & 0x1;
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_referinfo                 = hw_ctx->ls_info.feature_data[i][2];
        vp9_hw_regs->vp9d_param.reg67_74[i].segid_frame_skip_en             = (hw_ctx->ls_info.feature_mask[i] >> 3) & 0x1;
    }

    vp9_hw_regs->vp9d_param.reg67_74[0].segid_abs_delta = hw_ctx->ls_info.abs_delta_last;
    vp9_hw_regs->vp9d_param.reg76.tx_mode               = pic_param->txmode;
    vp9_hw_regs->vp9d_param.reg76.frame_reference_mode  = pic_param->refmode;
    vp9_hw_regs->vp9d_param.reg94.ref_deltas_lastframe  = 0;

    if (!intraFlag) {
        for (i = 0; i < 4; i++)
            vp9_hw_regs->vp9d_param.reg94.ref_deltas_lastframe   |= (hw_ctx->ls_info.last_ref_deltas[i] & 0x7f) << (7 * i);

        for (i = 0; i < 2; i++)
            vp9_hw_regs->vp9d_param.reg75.mode_deltas_lastframe  |= (hw_ctx->ls_info.last_mode_deltas[i] & 0x7f) << (7 * i);
    } else {
        hw_ctx->ls_info.segmentation_enable_flag_last = 0;
        hw_ctx->ls_info.last_intra_only = 1;
    }

    vp9_hw_regs->vp9d_param.reg75.segmentation_enable_lstframe     = hw_ctx->ls_info.segmentation_enable_flag_last;
    vp9_hw_regs->vp9d_param.reg75.last_show_frame                  = hw_ctx->ls_info.last_show_frame;
    vp9_hw_regs->vp9d_param.reg75.last_intra_only                  = hw_ctx->ls_info.last_intra_only;
    vp9_hw_regs->vp9d_param.reg75.last_widthheight_eqcur           = (pic_param->width == hw_ctx->ls_info.last_width) && (pic_param->height == hw_ctx->ls_info.last_height);
    vp9_hw_regs->vp9d_param.reg78.lasttile_size                    = stream_len - pic_param->first_partition_size;


    if (!intraFlag) {
        vp9_hw_regs->vp9d_param.reg88.lref_hor_scale = pic_param->mvscale[0][0];
        vp9_hw_regs->vp9d_param.reg89.lref_ver_scale = pic_param->mvscale[0][1];
        vp9_hw_regs->vp9d_param.reg90.gref_hor_scale = pic_param->mvscale[1][0];
        vp9_hw_regs->vp9d_param.reg91.gref_ver_scale = pic_param->mvscale[1][1];
        vp9_hw_regs->vp9d_param.reg92.aref_hor_scale = pic_param->mvscale[2][0];
        vp9_hw_regs->vp9d_param.reg93.aref_ver_scale = pic_param->mvscale[2][1];
    }

    vp9_hw_regs->common.reg010.dec_e            = 1;
    vp9_hw_regs->common.reg011.buf_empty_en     = 1;
    vp9_hw_regs->common.reg011.dec_clkgate_e    = 1;
    vp9_hw_regs->common.reg011.err_head_fill_e  = 1;
    vp9_hw_regs->common.reg011.err_colmv_fill_e = 1;

    vp9_hw_regs->common.reg026.inter_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.filterd_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.strmd_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.mcp_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.busifd_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.dec_ctrl_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.intra_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.mc_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.transd_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.sram_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.cru_auto_gating_e = 1;
    vp9_hw_regs->common.reg026.reg_cfg_gating_en = 1;

    vp9_hw_regs->common.reg032_timeout_threshold = 0x3ffff;

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

    hal_vp9d_rcb_info_update(hal, vp9_hw_regs, pic_param);
    {
        MppBuffer rcb_buf = NULL;

        rcb_buf = p_hal->fast_mode ? hw_ctx->g_buf[task->dec.reg_index].rcb_buf : hw_ctx->rcb_buf;
        vdpu382_setup_rcb(&vp9_hw_regs->common_addr, p_hal->dev, rcb_buf, hw_ctx->rcb_info);
    }

    {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        if (mpp_frame_get_thumbnail_en(mframe)) {
            vp9_hw_regs->vp9d_addr.reg198_scale_down_luma_base =
                vp9_hw_regs->common_addr.reg130_decout_base;
            vp9_hw_regs->vp9d_addr.reg199_scale_down_chorme_base =
                vp9_hw_regs->common_addr.reg130_decout_base;
            vdpu382_setup_down_scale(mframe, p_hal->dev, &vp9_hw_regs->common);
        } else {
            vp9_hw_regs->vp9d_addr.reg198_scale_down_luma_base = 0;
            vp9_hw_regs->vp9d_addr.reg199_scale_down_chorme_base = 0;
            vp9_hw_regs->common.reg012.scale_down_en = 0;
        }
    }
    vdpu382_setup_statistic(&vp9_hw_regs->common, &vp9_hw_regs->statistic);

    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu382_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    Vdpu382Vp9dRegSet *hw_regs = (Vdpu382Vp9dRegSet *)hw_ctx->hw_regs;
    MppDev dev = p_hal->dev;

    if (p_hal->fast_mode) {
        RK_S32 index =  task->dec.reg_index;
        hw_regs = (Vdpu382Vp9dRegSet *)hw_ctx->g_buf[index].hw_regs;
    }

    mpp_assert(hw_regs);


#if VP9_DUMP
    {
        static RK_U32 file_cnt = 0;
        char file_name[128];
        sprintf(file_name, "/data/vp9_regs/reg_%d.txt", file_cnt);
        FILE *fp = fopen(file_name, "wb");
        RK_U32 i = 0;
        RK_U32 *tmp = NULL;
        tmp = (RK_U32 *)&hw_regs->common;
        for (i = 0; i < sizeof(hw_regs->common) / 4; i++) {
            fprintf(fp, "reg[%d] 0x%08x\n", i + 8, tmp[i]);
        }
        fprintf(fp, "\n");
        tmp = (RK_U32 *)&hw_regs->vp9d_param;
        for (i = 0; i < sizeof(hw_regs->vp9d_param) / 4; i++) {
            fprintf(fp, "reg[%d] 0x%08x\n", i + 64, tmp[i]);
        }
        fprintf(fp, "\n");
        tmp = (RK_U32 *)&hw_regs->common_addr;
        for (i = 0; i < sizeof(hw_regs->common_addr) / 4; i++) {
            fprintf(fp, "reg[%d] 0x%08x\n", i + 128, tmp[i]);
        }
        fprintf(fp, "\n");
        tmp = (RK_U32 *)&hw_regs->vp9d_addr;
        for (i = 0; i < sizeof(hw_regs->vp9d_addr) / 4; i++) {
            fprintf(fp, "reg[%d] 0x%08x\n", i + 160, tmp[i]);
        }
        file_cnt++;
        fflush(fp);
        fclose(fp);
    }
#endif

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

        wr_cfg.reg = &hw_regs->statistic;
        wr_cfg.size = sizeof(hw_regs->statistic);
        wr_cfg.offset = OFFSET_STATISTIC_REGS;

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
        /* rcb info for sram */
        vdpu382_set_rcbinfo(dev, hw_ctx->rcb_info);
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    (void)task;
    return ret;
}

static MPP_RET hal_vp9d_vdpu382_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;
    Vdpu382Vp9dRegSet *hw_regs = (Vdpu382Vp9dRegSet *)hw_ctx->hw_regs;

    if (p_hal->fast_mode)
        hw_regs = (Vdpu382Vp9dRegSet *)hw_ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(Vdpu382Vp9dRegSet) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i, *p++);
    }

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        !hw_regs->irq_status.reg224.dec_rdy_sta) {
        MppFrame mframe = NULL;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }
#if !HW_PROB
    if (p_hal->dec_cb && task->dec.flags.wait_done) {
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;

        mpp_buffer_sync_end(hw_ctx->count_base);
        hal_vp9d_update_counts(mpp_buffer_get_ptr(hw_ctx->count_base), task->dec.syntax.data);
        mpp_callback(p_hal->dec_cb, &pic_param->counts);
    }
#endif
    if (p_hal->fast_mode) {
        hw_ctx->g_buf[task->dec.reg_index].use_flag = 0;
    }

    (void)task;
    return ret;
}

static MPP_RET hal_vp9d_vdpu382_reset(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;

    hal_vp9d_enter();

    memset(&hw_ctx->ls_info, 0, sizeof(hw_ctx->ls_info));
    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    hw_ctx->last_segid_flag = 1;
    memset(&hw_ctx->prob_ref_poc, 0, sizeof(hw_ctx->prob_ref_poc));
    hw_ctx->col_ref_poc = 0;
    hw_ctx->segid_ref_poc = 0;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu382_flush(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu382Vp9dCtx *hw_ctx = (Vdpu382Vp9dCtx*)p_hal->hw_ctx;

    hal_vp9d_enter();

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu382_control(void *hal, MpiCmd cmd_type, void *param)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO : {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);

        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu382_afbc_align_calc(p_hal->slots, (MppFrame)param, 0);
        } else {
            mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, vp9_hor_align);
        }
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

const MppHalApi hal_vp9d_vdpu382 = {
    .name = "vp9d_vdpu382",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(Vdpu382Vp9dCtx),
    .flag = 0,
    .init = hal_vp9d_vdpu382_init,
    .deinit = hal_vp9d_vdpu382_deinit,
    .reg_gen = hal_vp9d_vdpu382_gen_regs,
    .start = hal_vp9d_vdpu382_start,
    .wait = hal_vp9d_vdpu382_wait,
    .reset = hal_vp9d_vdpu382_reset,
    .flush = hal_vp9d_vdpu382_flush,
    .control = hal_vp9d_vdpu382_control,
};
