/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_vp9d_vdpu383"

#include <string.h>

#include "mpp_debug.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buffer_impl.h"
#include "mpp_bitput.h"
#include "mpp_compat_impl.h"

#include "hal_vp9d_debug.h"
#include "hal_vp9d_com.h"
#include "hal_vp9d_vdpu383.h"
#include "hal_vp9d_ctx.h"
#include "vdpu383_vp9d.h"
#include "vp9d_syntax.h"

#define HW_PROB         1
#define VP9_CONTEXT     4
#define VP9_CTU_SIZE    64

#define GBL_SIZE        2 * (MPP_ALIGN(1299, 128) / 8)

#define EIGHTTAP        0
#define EIGHTTAP_SMOOTH 1
#define EIGHTTAP_SHARP  2
#define BILINEAR        3

const RK_U8 literal_to_filter[] = { EIGHTTAP_SMOOTH, EIGHTTAP,
                                    EIGHTTAP_SHARP, BILINEAR
                                  };

typedef struct Vdpu383Vp9dCtx_t {
    Vp9dRegBuf      g_buf[MAX_GEN_REG];
    MppBuffer       global_base;
    MppBuffer       probe_base;
    MppBuffer       count_base;
    MppBuffer       segid_cur_base;
    MppBuffer       segid_last_base;
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
    Vdpu383RcbInfo  rcb_info[RCB_BUF_COUNT];
    MppBuffer       rcb_buf;
    RK_U32          num_row_tiles;
    RK_U32          bit_depth;
    /* colmv buffers info */
    HalBufs         cmv_bufs;
    RK_S32          mv_size;
    RK_S32          mv_count;
    HalBufs         origin_bufs;
    RK_U32          prob_ctx_valid[VP9_CONTEXT];
    MppBuffer       prob_loop_base[VP9_CONTEXT];
    /* uncompress header data */
    RK_U8           header_data[168];
} Vdpu383Vp9dCtx;

#ifdef DUMP_VDPU383_DATAS
static RK_U32 cur_last_segid_flag;
static MppBuffer cur_last_prob_base;
#endif

static MPP_RET vdpu383_setup_scale_origin_bufs(Vdpu383Vp9dCtx *ctx, MppFrame mframe)
{
    /* for 8K FrameBuf scale mode */
    size_t origin_buf_size = 0;

    origin_buf_size = mpp_frame_get_buf_size(mframe);

    if (!origin_buf_size) {
        mpp_err_f("origin_bufs get buf size failed\n");
        return MPP_NOK;
    }
    if (ctx->origin_bufs) {
        hal_bufs_deinit(ctx->origin_bufs);
        ctx->origin_bufs = NULL;
    }
    hal_bufs_init(&ctx->origin_bufs);
    if (!ctx->origin_bufs) {
        mpp_err_f("origin_bufs thumb init fail\n");
        return MPP_ERR_NOMEM;
    }
    hal_bufs_setup(ctx->origin_bufs, 16, 1, &origin_buf_size);

    return MPP_OK;
}
static MPP_RET hal_vp9d_alloc_res(HalVp9dCtx *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    RK_S32 ret = 0;
    RK_S32 i = 0;

    /* alloc common buffer */
    for (i = 0; i < VP9_CONTEXT; i++) {
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->prob_loop_base[i], PROB_SIZE);
        if (ret) {
            mpp_err("vp9 probe_loop_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->prob_loop_base[i], p_hal->dev);
    }
    ret = mpp_buffer_get(p_hal->group, &hw_ctx->prob_default_base, PROB_SIZE);
    if (ret) {
        mpp_err("vp9 probe_default_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->prob_default_base, p_hal->dev);

    ret = mpp_buffer_get(p_hal->group, &hw_ctx->segid_cur_base, MAX_SEGMAP_SIZE);
    if (ret) {
        mpp_err("vp9 segid_cur_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->segid_cur_base, p_hal->dev);
    ret = mpp_buffer_get(p_hal->group, &hw_ctx->segid_last_base, MAX_SEGMAP_SIZE);
    if (ret) {
        mpp_err("vp9 segid_last_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->segid_last_base, p_hal->dev);

    /* alloc buffer for fast mode or normal */
    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            hw_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu383Vp9dRegSet));
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].global_base, GBL_SIZE);
            mpp_buffer_attach_dev(hw_ctx->g_buf[i].global_base, p_hal->dev);
            if (ret) {
                mpp_err("vp9 global_base get buffer failed\n");
                return ret;
            }
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].probe_base, PROB_KF_SIZE);
            if (ret) {
                mpp_err("vp9 probe_base get buffer failed\n");
                return ret;
            }
            mpp_buffer_attach_dev(hw_ctx->g_buf[i].probe_base, p_hal->dev);
            ret = mpp_buffer_get(p_hal->group,
                                 &hw_ctx->g_buf[i].count_base, COUNT_SIZE);
            if (ret) {
                mpp_err("vp9 count_base get buffer failed\n");
                return ret;
            }
            mpp_buffer_attach_dev(hw_ctx->g_buf[i].count_base, p_hal->dev);
        }
    } else {
        hw_ctx->hw_regs = mpp_calloc_size(void, sizeof(Vdpu383Vp9dRegSet));
        ret = mpp_buffer_get(p_hal->group, &hw_ctx->global_base, PROB_SIZE);
        if (ret) {
            mpp_err("vp9 global_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->global_base, p_hal->dev);

        ret = mpp_buffer_get(p_hal->group, &hw_ctx->probe_base, PROB_KF_SIZE);
        if (ret) {
            mpp_err("vp9 probe_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->probe_base, p_hal->dev);

        ret = mpp_buffer_get(p_hal->group, &hw_ctx->count_base, COUNT_SIZE);
        if (ret) {
            mpp_err("vp9 count_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->count_base, p_hal->dev);
    }
    return MPP_OK;
}

static MPP_RET hal_vp9d_release_res(HalVp9dCtx *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    RK_S32 ret = 0;
    RK_S32 i = 0;

    if (hw_ctx->prob_default_base) {
        ret = mpp_buffer_put(hw_ctx->prob_default_base);
        if (ret) {
            mpp_err("vp9 probe_wr_base get buffer failed\n");
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
    for (i = 0; i < VP9_CONTEXT; i++) {
        if (hw_ctx->prob_loop_base[i]) {
            ret = mpp_buffer_put(hw_ctx->prob_loop_base[i]);
            if (ret) {
                mpp_err("vp9 prob_loop_base put buffer failed\n");
                return ret;
            }
        }
    }
    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (hw_ctx->g_buf[i].global_base) {
                ret = mpp_buffer_put(hw_ctx->g_buf[i].global_base);
                if (ret) {
                    mpp_err("vp9 global_base put buffer failed\n");
                    return ret;
                }
            }
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
        if (hw_ctx->global_base) {
            ret = mpp_buffer_put(hw_ctx->global_base);
            if (ret) {
                mpp_err("vp9 global_base get buffer failed\n");
                return ret;
            }
        }
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
    if (hw_ctx->origin_bufs) {
        ret = hal_bufs_deinit(hw_ctx->origin_bufs);
        if (ret) {
            mpp_err("thumb vp9 origin_bufs deinit buffer failed\n");
            return ret;
        }
        hw_ctx->origin_bufs = NULL;
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu383_deinit(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx *)hal;
    MPP_RET ret = MPP_OK;

    hal_vp9d_release_res(p_hal);

    if (p_hal->group) {
        ret = mpp_buffer_group_put(p_hal->group);
        if (ret) {
            mpp_err("vp9d group free buffer failed\n");
            return ret;
        }
    }
    MPP_FREE(p_hal->hw_ctx);

    return ret;
}

static MPP_RET hal_vp9d_vdpu383_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    MEM_CHECK(ret, p_hal->hw_ctx = mpp_calloc_size(void, sizeof(Vdpu383Vp9dCtx)));
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    (void) cfg;

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
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
        cfg->hal_fbc_adj_cfg->func = vdpu383_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 0;
    }

    return ret;
__FAILED:
    hal_vp9d_vdpu383_deinit(hal);
    return ret;
}

static void vp9d_refine_rcb_size(Vdpu383RcbInfo *rcb_info,
                                 RK_S32 width, RK_S32 height, void* data)
{
    RK_U32 rcb_bits = 0;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)data;
    RK_U32 tile_row_num = 1 << pic_param->log2_tile_rows;
    RK_U32 tile_col_num = 1 << pic_param->log2_tile_cols;
    RK_U32 bit_depth = pic_param->BitDepthMinus8Luma + 8;
    RK_U32 ext_row_align_size = tile_row_num * 64 * 8;
    RK_U32 ext_col_align_size = tile_col_num * 64 * 8;
    RK_U32 filterd_row_append = 8192;

    width = MPP_ALIGN(width, VP9_CTU_SIZE);
    height = MPP_ALIGN(height, VP9_CTU_SIZE);
    /* RCB_STRMD_ROW && RCB_STRMD_TILE_ROW*/
    if (width > 4096)
        rcb_bits = ((width + 63) / 64) * 250;
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = 0;
    rcb_info[RCB_STRMD_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);

    /* RCB_INTER_ROW && RCB_INTER_TILE_ROW*/
    rcb_bits = ((width + 63) / 64) * 2368;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTER_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTER_TILE_ROW].size = 0;

    /* RCB_INTRA_ROW && RCB_INTRA_TILE_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 3; //TODO:
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTRA_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTRA_TILE_ROW].size = 0;

    /* RCB_FILTERD_ROW && RCB_FILTERD_TILE_ROW*/
    // save space mode : half for RCB_FILTERD_ROW, half for RCB_FILTERD_PROTECT_ROW
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_bits = (RK_U32)(MPP_ALIGN(width, 64) * (41 * bit_depth + 13));
    rcb_info[RCB_FILTERD_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FILTERD_PROTECT_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_FILTERD_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_FILTERD_TILE_ROW].size = 0;

    /* RCB_FILTERD_TILE_COL */
    if (tile_col_num > 1) {
        rcb_bits = (RK_U32)(MPP_ALIGN(height, 64) * (42 * bit_depth + 13)) + ext_col_align_size;
        rcb_info[RCB_FILTERD_TILE_COL].size = MPP_RCB_BYTES(rcb_bits);
    } else {
        rcb_info[RCB_FILTERD_TILE_COL].size = 0;
    }

}

static void hal_vp9d_rcb_info_update(void *hal, Vdpu383Vp9dRegSet *hw_regs, void *data)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)data;
    RK_U32 num_tiles = pic_param->log2_tile_rows;
    RK_U32 bit_depth = pic_param->BitDepthMinus8Luma + 8;
    RK_S32 height = vp9_ver_align(pic_param->height);
    RK_S32 width  = vp9_ver_align(pic_param->width);
    (void) hw_regs;

    if (hw_ctx->num_row_tiles != num_tiles ||
        hw_ctx->bit_depth != bit_depth ||
        hw_ctx->width != width ||
        hw_ctx->height != height) {

        hw_ctx->rcb_buf_size = vdpu383_get_rcb_buf_size(hw_ctx->rcb_info, width, height);
        // TODO: refine rcb buffer size
        vp9d_refine_rcb_size(hw_ctx->rcb_info, width, height, pic_param);

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

static void
set_tile_offset(RK_S32 *start, RK_S32 *end, RK_S32 idx, RK_S32 log2_n, RK_S32 n)
{
    RK_S32 sb_start = ( idx      * n) >> log2_n;
    RK_S32 sb_end   = ((idx + 1) * n) >> log2_n;

    *start = MPP_MIN(sb_start, n) << 3;
    *end   = MPP_MIN(sb_end,   n) << 3;
}

static MPP_RET prepare_uncompress_header(HalVp9dCtx *p_hal, DXVA_PicParams_VP9 *pp,
                                         RK_U64 *data, RK_U32 len)
{
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    BitputCtx_t bp;
    RK_S32 i, j;

    mpp_set_bitput_ctx(&bp, data, len);

    mpp_put_bits(&bp, pp->frame_type, 1);
    mpp_put_bits(&bp, pp->error_resilient_mode, 1);
    mpp_put_bits(&bp, pp->BitDepthMinus8Luma, 3);
    mpp_put_bits(&bp, 1, 2); // yuv420
    mpp_put_bits(&bp, pp->width, 16);
    mpp_put_bits(&bp, pp->height, 16);

    mpp_put_bits(&bp, (!pp->frame_type || pp->intra_only), 1);
    mpp_put_bits(&bp, pp->ref_frame_sign_bias[1], 1);
    mpp_put_bits(&bp, pp->ref_frame_sign_bias[2], 1);
    mpp_put_bits(&bp, pp->ref_frame_sign_bias[3], 1);

    mpp_put_bits(&bp, pp->allow_high_precision_mv, 1);
    /* sync with cmodel */
    if (!pp->frame_type || pp->intra_only)
        mpp_put_bits(&bp, 0, 3);
    else {
        if (pp->interp_filter == 4) /* FILTER_SWITCHABLE */
            mpp_put_bits(&bp, pp->interp_filter, 3);
        else
            mpp_put_bits(&bp, literal_to_filter[pp->interp_filter], 3);
    }
    mpp_put_bits(&bp, pp->parallelmode, 1);
    mpp_put_bits(&bp, pp->refresh_frame_context, 1);

    /* loop filter */
    mpp_put_bits(&bp, pp->filter_level, 6);
    mpp_put_bits(&bp, pp->sharpness_level, 3);
    mpp_put_bits(&bp, pp->mode_ref_delta_enabled, 1);
    mpp_put_bits(&bp, pp->mode_ref_delta_update, 1);

    mpp_put_bits(&bp, pp->ref_deltas[0], 7);
    mpp_put_bits(&bp, pp->ref_deltas[1], 7);
    mpp_put_bits(&bp, pp->ref_deltas[2], 7);
    mpp_put_bits(&bp, pp->ref_deltas[3], 7);
    mpp_put_bits(&bp, pp->mode_deltas[0], 7);
    mpp_put_bits(&bp, pp->mode_deltas[1], 7);

    mpp_put_bits(&bp, pp->base_qindex, 8);
    mpp_put_bits(&bp, pp->y_dc_delta_q, 5);
    mpp_put_bits(&bp, pp->uv_dc_delta_q, 5);
    mpp_put_bits(&bp, pp->uv_ac_delta_q, 5);
    mpp_put_bits(&bp, (!pp->base_qindex && !pp->y_dc_delta_q && !pp->uv_dc_delta_q && !pp->uv_ac_delta_q), 1);

    for (i = 0; i < 3; i++) {
        mpp_put_bits(&bp, pp->stVP9Segments.pred_probs[i], 8);
    }
    for (i = 0; i < 7; i++) {
        mpp_put_bits(&bp, pp->stVP9Segments.tree_probs[i], 8);
    }
    mpp_put_bits(&bp, pp->stVP9Segments.enabled, 1);
    mpp_put_bits(&bp, pp->stVP9Segments.update_map, 1);
    mpp_put_bits(&bp, pp->stVP9Segments.temporal_update, 1);
    mpp_put_bits(&bp, pp->stVP9Segments.abs_delta, 1);

    {
        RK_U32 use_prev_frame_mvs = !pp->error_resilient_mode &&
                                    pp->width == hw_ctx->ls_info.last_width &&
                                    pp->height == hw_ctx->ls_info.last_height &&
                                    !hw_ctx->ls_info.last_intra_only &&
                                    hw_ctx->ls_info.last_show_frame;
        mpp_put_bits(&bp, use_prev_frame_mvs, 1);
    }

    for ( i = 0; i < 8; i++ )
        for ( j = 0; j < 4; j++ )
            mpp_put_bits(&bp, (pp->stVP9Segments.feature_mask[i] >> j) & 0x1, 1);

    for ( i = 0; i < 8; i++ ) {
        mpp_put_bits(&bp, pp->stVP9Segments.feature_data[i][0], 9);
        mpp_put_bits(&bp, pp->stVP9Segments.feature_data[i][1], 7);
        mpp_put_bits(&bp, pp->stVP9Segments.feature_data[i][2], 2);
    }

    mpp_put_bits(&bp, pp->first_partition_size, 16);

    /* refer frame width and height */
    {
        RK_S32 ref_idx = pp->frame_refs[0].Index7Bits;
        mpp_put_bits(&bp, pp->ref_frame_coded_width[ref_idx], 16);
        mpp_put_bits(&bp, pp->ref_frame_coded_height[ref_idx], 16);
        ref_idx = pp->frame_refs[1].Index7Bits;
        mpp_put_bits(&bp, pp->ref_frame_coded_width[ref_idx], 16);
        mpp_put_bits(&bp, pp->ref_frame_coded_height[ref_idx], 16);
        ref_idx = pp->frame_refs[2].Index7Bits;
        mpp_put_bits(&bp, pp->ref_frame_coded_width[ref_idx], 16);
        mpp_put_bits(&bp, pp->ref_frame_coded_height[ref_idx], 16);
    }

    /* last frame info */
    mpp_put_bits(&bp, hw_ctx->ls_info.last_mode_deltas[0], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.last_mode_deltas[1], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.last_ref_deltas[0], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.last_ref_deltas[1], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.last_ref_deltas[2], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.last_ref_deltas[3], 7);
    mpp_put_bits(&bp, hw_ctx->ls_info.segmentation_enable_flag_last, 1);

    mpp_put_bits(&bp, hw_ctx->ls_info.last_show_frame, 1);
    mpp_put_bits(&bp, pp->intra_only, 1);
    {
        RK_U32 last_widthheight_eqcur = pp->width == hw_ctx->ls_info.last_width &&
                                        pp->height == hw_ctx->ls_info.last_height;

        mpp_put_bits(&bp, last_widthheight_eqcur, 1);
    }
    mpp_put_bits(&bp, hw_ctx->ls_info.color_space_last, 3);

    mpp_put_bits(&bp, !hw_ctx->ls_info.last_frame_type, 1);
    mpp_put_bits(&bp, 0, 1);
    mpp_put_bits(&bp, 1, 1);
    mpp_put_bits(&bp, 1, 1);
    mpp_put_bits(&bp, 1, 1);

    mpp_put_bits(&bp, pp->mvscale[0][0], 16);
    mpp_put_bits(&bp, pp->mvscale[0][1], 16);
    mpp_put_bits(&bp, pp->mvscale[1][0], 16);
    mpp_put_bits(&bp, pp->mvscale[1][1], 16);
    mpp_put_bits(&bp, pp->mvscale[2][0], 16);
    mpp_put_bits(&bp, pp->mvscale[2][1], 16);

    /* tile cols and rows */
    {
        RK_S32 tile_width[64] = {0};
        RK_S32 tile_height[4] = {0};
        RK_S32 tile_cols = 1 << pp->log2_tile_cols;
        RK_S32 tile_rows = 1 << pp->log2_tile_rows;

        mpp_put_bits(&bp, tile_cols, 7);
        mpp_put_bits(&bp, tile_rows, 3);

        for (i = 0; i < tile_cols; ++i) { // tile_col
            RK_S32 tile_col_start = 0;
            RK_S32 tile_col_end = 0;

            set_tile_offset(&tile_col_start, &tile_col_end,
                            i, pp->log2_tile_cols, MPP_ALIGN(pp->width, 64) / 64);
            tile_width[i] = (tile_col_end - tile_col_start + 7) / 8;
        }

        for (j = 0; j < tile_rows; ++j) { // tile_row
            RK_S32 tile_row_start = 0;
            RK_S32 tile_row_end = 0;

            set_tile_offset(&tile_row_start, &tile_row_end,
                            j, pp->log2_tile_rows, MPP_ALIGN(pp->height, 64) / 64);
            tile_height[j] = (tile_row_end - tile_row_start + 7) / 8;
        }

        for (i = 0; i < 64; i++)
            mpp_put_bits(&bp, tile_width[i], 10);

        for (j = 0; j < 4; j++)
            mpp_put_bits(&bp, tile_height[j], 10);
    }

    mpp_put_align(&bp, 64, 0);//128

#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "global_cfg.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)bp.pbuf, 64 * (bp.index - 1) + bp.bitpos, 64, 0);
    }
#endif

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu383_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32 i;
    RK_U8  bit_depth = 0;
    RK_U32 ref_frame_width_y;
    RK_U32 ref_frame_height_y;
    RK_S32 stream_len = 0, aglin_offset = 0;
    RK_U32 y_hor_virstride, uv_hor_virstride, y_virstride;
    RK_U8  *bitstream = NULL;
    MppBuffer streambuf = NULL;
    RK_U32 sw_y_hor_virstride;
    RK_U32 sw_uv_hor_virstride;
    RK_U32 sw_y_virstride;
    RK_U32 sw_uv_virstride;
    RK_U8  ref_idx = 0;
    RK_U8  ref_frame_idx = 0;
    RK_U32 *reg_ref_base = NULL;
    RK_U32 *reg_payload_ref_base = NULL;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;
    RK_U32 fbc_en = 0;
    HalBuf *origin_buf = NULL;

    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
    Vdpu383Vp9dRegSet *vp9_hw_regs = NULL;
    RK_S32 mv_size = pic_param->width * pic_param->height / 2;
    RK_U32 frame_ctx_id = pic_param->frame_context_idx;
    MppFrame mframe;
    MppFrame ref_frame = NULL;

    if (p_hal->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!hw_ctx->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                hw_ctx->global_base = hw_ctx->g_buf[i].global_base;
                hw_ctx->probe_base = hw_ctx->g_buf[i].probe_base;
                hw_ctx->count_base = hw_ctx->g_buf[i].count_base;
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
    vp9_hw_regs = (Vdpu383Vp9dRegSet*)hw_ctx->hw_regs;
    memset(vp9_hw_regs, 0, sizeof(Vdpu383Vp9dRegSet));

#ifdef DUMP_VDPU383_DATAS
    {
        memset(dump_cur_dir, 0, sizeof(dump_cur_dir));
        sprintf(dump_cur_dir, "vp9/Frame%04d", dump_cur_frame);
        if (access(dump_cur_dir, 0)) {
            if (mkdir(dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", dump_cur_dir);
        }
        dump_cur_frame++;
    }
#endif

    /* uncompress header data */
    prepare_uncompress_header(p_hal, pic_param, (RK_U64 *)hw_ctx->header_data, sizeof(hw_ctx->header_data) / 8);
    memcpy(mpp_buffer_get_ptr(hw_ctx->global_base), hw_ctx->header_data, sizeof(hw_ctx->header_data));
    mpp_buffer_sync_end(hw_ctx->global_base);
    vp9_hw_regs->vp9d_paras.reg67_global_len = GBL_SIZE / 16;
    vp9_hw_regs->common_addr.reg131_gbl_base = mpp_buffer_get_fd(hw_ctx->global_base);

    if (hw_ctx->cmv_bufs == NULL || hw_ctx->mv_size < mv_size) {
        size_t size = mv_size;

        if (hw_ctx->cmv_bufs) {
            hal_bufs_deinit(hw_ctx->cmv_bufs);
            hw_ctx->cmv_bufs = NULL;
        }

        hal_bufs_init(&hw_ctx->cmv_bufs);
        if (hw_ctx->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_NOK;
        }
        hw_ctx->mv_size = mv_size;
        hw_ctx->mv_count = mpp_buf_slot_get_count(p_hal ->slots);
        hal_bufs_setup(hw_ctx->cmv_bufs, hw_ctx->mv_count, 1, &size);
    }

    mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        hw_ctx->origin_bufs == NULL) {
        vdpu383_setup_scale_origin_bufs(hw_ctx, mframe);
    }

    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);

    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
#if HW_PROB
    // hal_vp9d_prob_flag_delta(mpp_buffer_get_ptr(hw_ctx->probe_base), task->dec.syntax.data);
    /* init kf_probe */
    hal_vp9d_prob_kf(mpp_buffer_get_ptr(hw_ctx->probe_base));
    mpp_buffer_sync_end(hw_ctx->probe_base);
    if (intraFlag) {
        hal_vp9d_prob_default(mpp_buffer_get_ptr(hw_ctx->prob_default_base), task->dec.syntax.data);
        mpp_buffer_sync_end(hw_ctx->prob_default_base);
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

        if (hw_ctx->prob_ctx_valid[frame_ctx_id]) {
            vp9_hw_regs->vp9d_addrs.reg184_lastprob_base =
                mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
#ifdef DUMP_VDPU383_DATAS
            { cur_last_prob_base = hw_ctx->prob_loop_base[frame_ctx_id]; }
#endif
        } else {
            vp9_hw_regs->vp9d_addrs.reg184_lastprob_base = mpp_buffer_get_fd(hw_ctx->prob_default_base);
            hw_ctx->prob_ctx_valid[frame_ctx_id] |= pic_param->refresh_frame_context;
#ifdef DUMP_VDPU383_DATAS
            { cur_last_prob_base = hw_ctx->prob_default_base; }
#endif
        }
        vp9_hw_regs->vp9d_addrs.reg185_updateprob_base =
            mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
    }
    vp9_hw_regs->vp9d_addrs.reg183_kfprob_base = mpp_buffer_get_fd(hw_ctx->probe_base);
#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "cabac_last_probe.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(cur_last_prob_base),
                          8 * 152 * 16, 128, 0);
    }
    {
        char *cur_fname = "cabac_kf_probe.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(hw_ctx->probe_base),
                          8 * PROB_KF_SIZE, 128, 0);
    }
#endif
#else
#endif

    vp9_hw_regs->vp9d_paras.reg66_stream_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = vp9_hw_regs->vp9d_paras.reg66_stream_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;

    {
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        fbc_en = MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe));

        if (fbc_en) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 h = MPP_ALIGN(mpp_frame_get_height(mframe), 64);
            RK_U32 fbd_offset;

            vp9_hw_regs->ctrl_regs.reg9.fbc_e = 1;
            vp9_hw_regs->vp9d_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
            fbd_offset = vp9_hw_regs->vp9d_paras.reg68_hor_virstride * h * 4;
            vp9_hw_regs->vp9d_addrs.reg193_fbc_payload_offset = fbd_offset;
            /* error stride */
            vp9_hw_regs->vp9d_paras.reg80_error_ref_hor_virstride = fbc_hdr_stride / 64;
        } else {
            sw_y_hor_virstride = mpp_frame_get_hor_stride(mframe) >> 4;
            sw_uv_hor_virstride = sw_y_hor_virstride;
            sw_y_virstride = mpp_frame_get_ver_stride(mframe) * sw_y_hor_virstride;
            sw_uv_virstride = sw_y_virstride / 2;

            vp9_hw_regs->ctrl_regs.reg9.fbc_e = 0;
            if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                vp9_hw_regs->ctrl_regs.reg9.tile_e = 1;
                vp9_hw_regs->vp9d_paras.reg68_hor_virstride = sw_y_hor_virstride * 6;
                vp9_hw_regs->vp9d_paras.reg70_y_virstride = sw_y_virstride + sw_uv_virstride;
            } else {
                vp9_hw_regs->ctrl_regs.reg9.tile_e = 0;
                vp9_hw_regs->vp9d_paras.reg68_hor_virstride = sw_y_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg69_raster_uv_hor_virstride = sw_uv_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg70_y_virstride = sw_y_virstride;
            }
            /* error stride */
            vp9_hw_regs->vp9d_paras.reg80_error_ref_hor_virstride = sw_y_hor_virstride;
            vp9_hw_regs->vp9d_paras.reg81_error_ref_raster_uv_hor_virstride = sw_uv_hor_virstride;
            vp9_hw_regs->vp9d_paras.reg82_error_ref_virstride = sw_y_virstride;
        }
    }
    if (!pic_param->intra_only && pic_param->frame_type &&
        !pic_param->error_resilient_mode && hw_ctx->ls_info.last_show_frame) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
    }

    mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
    mpp_buf_slot_get_prop(p_hal ->slots, task->dec.output, SLOT_BUFFER, &framebuf);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
        origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, task->dec.output);
        framebuf = origin_buf->buf[0];
    }
    vp9_hw_regs->vp9d_addrs.reg168_decout_base = mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->vp9d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->vp9d_addrs.reg192_payload_st_cur_base = mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->vp9d_addrs.reg194_payload_st_error_ref_base = mpp_buffer_get_fd(framebuf);
    vp9_hw_regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(streambuf);

    {
        RK_U32 strm_offset = pic_param->uncompressed_header_size_byte_aligned;

        vp9_hw_regs->vp9d_paras.reg65_strm_start_bit = 8 * (strm_offset & 0xf);
        mpp_dev_set_reg_offset(p_hal->dev, 128, strm_offset & 0xfffffff0);
    }

    if (hw_ctx->last_segid_flag) {
        vp9_hw_regs->vp9d_addrs.reg181_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
        vp9_hw_regs->vp9d_addrs.reg182_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
    } else {
        vp9_hw_regs->vp9d_addrs.reg181_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
        vp9_hw_regs->vp9d_addrs.reg182_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
    }
#ifdef DUMP_VDPU383_DATAS
    cur_last_segid_flag = hw_ctx->last_segid_flag;
    {
        char *cur_fname = "stream_in.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(streambuf)
                          + pic_param->uncompressed_header_size_byte_aligned,
                          8 * (((stream_len + 15) & (~15)) + 0x80), 128, 0);
    }
#endif
    /* set last segid flag */
    if ((pic_param->stVP9Segments.enabled && pic_param->stVP9Segments.update_map) ||
        (pic_param->width != hw_ctx->ls_info.last_width || pic_param->height != hw_ctx->ls_info.last_height) ||
        intraFlag || pic_param->error_resilient_mode) {
        hw_ctx->last_segid_flag = !hw_ctx->last_segid_flag;
    }
    //set cur colmv base
    mv_buf = hal_bufs_get_buf(hw_ctx->cmv_bufs, task->dec.output);

    vp9_hw_regs->vp9d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);

    hw_ctx->mv_base_addr = vp9_hw_regs->vp9d_addrs.reg216_colmv_cur_base;
    if (hw_ctx->pre_mv_base_addr < 0)
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;

    // vp9 only one colmv
    vp9_hw_regs->vp9d_addrs.reg217_232_colmv_ref_base[0] = hw_ctx->pre_mv_base_addr;

    reg_ref_base = vp9_hw_regs->vp9d_addrs.reg170_185_ref_base;
    reg_payload_ref_base = vp9_hw_regs->vp9d_addrs.reg195_210_payload_st_ref_base;
    for (i = 0; i < 3; i++) {
        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];
        if (ref_frame_idx < 0x7f)
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_FRAME_PTR, &ref_frame);
        if (fbc_en) {
            y_hor_virstride = uv_hor_virstride = MPP_ALIGN(ref_frame_width_y, 64) / 64;
            if (*compat_ext_fbc_hdr_256_odd)
                y_hor_virstride = uv_hor_virstride = (MPP_ALIGN(ref_frame_width_y, 256) | 256) / 64;
        } else {
            if (ref_frame)
                y_hor_virstride = uv_hor_virstride = (mpp_frame_get_hor_stride(ref_frame) >> 4);
            else
                y_hor_virstride = uv_hor_virstride = (mpp_align_128_odd_plus_64((ref_frame_width_y * bit_depth) >> 3) >> 4);
        }
        if (ref_frame)
            y_virstride = y_hor_virstride * mpp_frame_get_ver_stride(ref_frame);
        else
            y_virstride = y_hor_virstride * vp9_ver_align(ref_frame_height_y);

        if (ref_frame_idx < 0x7f) {
            mpp_buf_slot_get_prop(p_hal ->slots, ref_frame_idx, SLOT_BUFFER, &framebuf);
            if (hw_ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, ref_frame_idx);
                framebuf = origin_buf->buf[0];
            }

            switch (i) {
            case 0: {
                vp9_hw_regs->vp9d_paras.reg83_ref0_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg84_ref0_raster_uv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg85_ref0_virstride = y_virstride;
            } break;
            case 1: {
                vp9_hw_regs->vp9d_paras.reg86_ref1_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg87_ref1_raster_uv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg88_ref1_virstride = y_virstride;
            } break;
            case 2: {
                vp9_hw_regs->vp9d_paras.reg89_ref2_hor_virstride = y_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg90_ref2_raster_uv_hor_virstride = uv_hor_virstride;
                vp9_hw_regs->vp9d_paras.reg91_ref2_virstride = y_virstride;
            } break;
            default:
                break;
            }

            /*0 map to 11*/
            /*1 map to 12*/
            /*2 map to 13*/
            if (framebuf != NULL) {
                reg_ref_base[i] = mpp_buffer_get_fd(framebuf);
                reg_payload_ref_base[i] = mpp_buffer_get_fd(framebuf);
            } else {
                mpp_log("ref buff address is no valid used out as base slot index 0x%x", ref_frame_idx);
                reg_ref_base[i] = vp9_hw_regs->vp9d_addrs.reg168_decout_base;
                reg_payload_ref_base[i] = vp9_hw_regs->vp9d_addrs.reg168_decout_base;
            }
            mv_buf = hal_bufs_get_buf(hw_ctx->cmv_bufs, ref_frame_idx);
        } else {
            reg_ref_base[i] = vp9_hw_regs->vp9d_addrs.reg168_decout_base;
            reg_payload_ref_base[i] = vp9_hw_regs->vp9d_addrs.reg168_decout_base;
        }
    }

    /* common register setting */
    vp9_hw_regs->ctrl_regs.reg8_dec_mode = 2; //set as vp9 dec
    vp9_hw_regs->ctrl_regs.reg9.buf_empty_en = 0;

    vp9_hw_regs->ctrl_regs.reg10.strmd_auto_gating_e      = 1;
    vp9_hw_regs->ctrl_regs.reg10.inter_auto_gating_e      = 1;
    vp9_hw_regs->ctrl_regs.reg10.intra_auto_gating_e      = 1;
    vp9_hw_regs->ctrl_regs.reg10.transd_auto_gating_e     = 1;
    vp9_hw_regs->ctrl_regs.reg10.recon_auto_gating_e      = 1;
    vp9_hw_regs->ctrl_regs.reg10.filterd_auto_gating_e    = 1;
    vp9_hw_regs->ctrl_regs.reg10.bus_auto_gating_e        = 1;
    vp9_hw_regs->ctrl_regs.reg10.ctrl_auto_gating_e       = 1;
    vp9_hw_regs->ctrl_regs.reg10.rcb_auto_gating_e        = 1;
    vp9_hw_regs->ctrl_regs.reg10.err_prc_auto_gating_e    = 1;

    vp9_hw_regs->ctrl_regs.reg16.error_proc_disable = 1;
    vp9_hw_regs->ctrl_regs.reg16.error_spread_disable = 0;
    vp9_hw_regs->ctrl_regs.reg16.roi_error_ctu_cal_en = 0;

    vp9_hw_regs->ctrl_regs.reg20_cabac_error_en_lowbits = 0xffffffdf;
    vp9_hw_regs->ctrl_regs.reg21_cabac_error_en_highbits = 0x3fffffff;

    vp9_hw_regs->ctrl_regs.reg13_core_timeout_threshold = 0x3ffff;

    //last info update
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
        hw_ctx->ls_info.feature_mask[i] = pic_param->stVP9Segments.feature_mask[i];
    }
    if (!hw_ctx->ls_info.segmentation_enable_flag_last)
        hw_ctx->ls_info.segmentation_enable_flag_last = pic_param->stVP9Segments.enabled;

    hw_ctx->ls_info.last_show_frame = pic_param->show_frame;
    hw_ctx->ls_info.last_width = pic_param->width;
    hw_ctx->ls_info.last_height = pic_param->height;
    hw_ctx->ls_info.last_frame_type = pic_param->frame_type;

    if (intraFlag)
        hw_ctx->ls_info.last_intra_only = 1;

    hw_ctx->ls_info.last_intra_only = (!pic_param->frame_type || pic_param->intra_only);
    hal_vp9d_dbg_par("stVP9Segments.enabled %d show_frame %d width %d height %d last_intra_only %d",
                     pic_param->stVP9Segments.enabled, pic_param->show_frame,
                     pic_param->width, pic_param->height,
                     hw_ctx->ls_info.last_intra_only);

    hal_vp9d_rcb_info_update(hal, vp9_hw_regs, pic_param);
    {
        MppBuffer rcb_buf = NULL;

        rcb_buf = p_hal->fast_mode ? hw_ctx->g_buf[task->dec.reg_index].rcb_buf : hw_ctx->rcb_buf;
        vdpu383_setup_rcb(&vp9_hw_regs->common_addr, p_hal->dev, rcb_buf, hw_ctx->rcb_info);
    }
    vdpu383_setup_statistic(&vp9_hw_regs->ctrl_regs);
    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    {
        //scale down config
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output,
                              SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output,
                              SLOT_FRAME_PTR, &mframe);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            vp9_hw_regs->common_addr.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, task->dec.output);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            vp9_hw_regs->vp9d_addrs.reg168_decout_base = fd;
            vp9_hw_regs->vp9d_addrs.reg169_error_ref_base = fd;
            vp9_hw_regs->vp9d_addrs.reg192_payload_st_cur_base = fd;
            vp9_hw_regs->vp9d_addrs.reg194_payload_st_error_ref_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &vp9_hw_regs->ctrl_regs,
                                     (void *)&vp9_hw_regs->vp9d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            vp9_hw_regs->common_addr.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            vdpu383_setup_down_scale(mframe, p_hal->dev, &vp9_hw_regs->ctrl_regs,
                                     (void *)&vp9_hw_regs->vp9d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            vp9_hw_regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu383_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    Vdpu383Vp9dRegSet *hw_regs = (Vdpu383Vp9dRegSet *)hw_ctx->hw_regs;
    MppDev dev = p_hal->dev;

    if (p_hal->fast_mode) {
        RK_S32 index = task->dec.reg_index;

        hw_regs = (Vdpu383Vp9dRegSet *)hw_ctx->g_buf[index].hw_regs;
    }

    mpp_assert(hw_regs);

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->ctrl_regs;
        wr_cfg.size = sizeof(hw_regs->ctrl_regs);
        wr_cfg.offset = OFFSET_CTRL_REGS;
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

        wr_cfg.reg = &hw_regs->vp9d_paras;
        wr_cfg.size = sizeof(hw_regs->vp9d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->vp9d_addrs;
        wr_cfg.size = sizeof(hw_regs->vp9d_addrs);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(hw_regs->ctrl_regs.reg15);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        // rcb info for sram
        vdpu383_set_rcbinfo(dev, hw_ctx->rcb_info);

        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}

static MPP_RET hal_vp9d_vdpu383_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;
    Vdpu383Vp9dRegSet *hw_regs = (Vdpu383Vp9dRegSet *)hw_ctx->hw_regs;

    if (p_hal->fast_mode)
        hw_regs = (Vdpu383Vp9dRegSet *)hw_ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);
#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "cabac_update_probe.dat";
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
        RK_U32 frame_ctx_id = pic_param->frame_context_idx;
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path,
                          (void *)mpp_buffer_get_ptr(hw_ctx->prob_loop_base[frame_ctx_id]),
                          8 * 152 * 16, 128, 0);
    }
    {
        char *cur_fname = "segid_last.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        if (!cur_last_segid_flag)
            dump_data_to_file(dump_cur_fname_path,
                              (void *)mpp_buffer_get_ptr(hw_ctx->segid_cur_base),
                              8 * 1559 * 8, 64, 0);
        else
            dump_data_to_file(dump_cur_fname_path,
                              (void *)mpp_buffer_get_ptr(hw_ctx->segid_last_base),
                              8 * 1559 * 8, 64, 0);
    }
    {
        char *cur_fname = "segid_cur.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        if (cur_last_segid_flag)
            dump_data_to_file(dump_cur_fname_path,
                              (void *)mpp_buffer_get_ptr(hw_ctx->segid_cur_base),
                              8 * 1559 * 8, 64, 0);
        else
            dump_data_to_file(dump_cur_fname_path,
                              (void *)mpp_buffer_get_ptr(hw_ctx->segid_last_base),
                              8 * 1559 * 8, 64, 0);
    }
#endif

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(Vdpu383Vp9dRegSet) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i, *p++);
    }

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        (!hw_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
        hw_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta) {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

#if !HW_PROB
    if (p_hal->dec_cb && task->dec.flags.wait_done) {
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
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

static MPP_RET hal_vp9d_vdpu383_reset(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;

    hal_vp9d_enter();

    memset(&hw_ctx->ls_info, 0, sizeof(hw_ctx->ls_info));
    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    hw_ctx->last_segid_flag = 1;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu383_flush(void *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu383Vp9dCtx *hw_ctx = (Vdpu383Vp9dCtx*)p_hal->hw_ctx;

    hal_vp9d_enter();

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;

    hal_vp9d_leave();

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu383_control(void *hal, MpiCmd cmd_type, void *param)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO : {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);

        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu383_afbc_align_calc(p_hal->slots, (MppFrame)param, 0);
        } else {
            mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
        }
    } break;
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO: {
        vdpu383_update_thumbnail_frame_info((MppFrame)param);
    } break;
    default : {
    } break;
    }

    return MPP_OK;
}

const MppHalApi hal_vp9d_vdpu383 = {
    .name = "vp9d_vdpu383",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(Vdpu383Vp9dCtx),
    .flag = 0,
    .init = hal_vp9d_vdpu383_init,
    .deinit = hal_vp9d_vdpu383_deinit,
    .reg_gen = hal_vp9d_vdpu383_gen_regs,
    .start = hal_vp9d_vdpu383_start,
    .wait = hal_vp9d_vdpu383_wait,
    .reset = hal_vp9d_vdpu383_reset,
    .flush = hal_vp9d_vdpu383_flush,
    .control = hal_vp9d_vdpu383_control,
};
