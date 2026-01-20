/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpu38x_com"

#include <string.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"
#include "mpp_frame_impl.h"

#include "vdpu_com.h"
#include "vdpu38x_com.h"

const RK_U32 vdpu38x_rcb_type2loc_map[RCB_BUF_CNT] = {
    [RCB_STRMD_IN_ROW]     = VDPU38X_RCB_IN_TILE_ROW,
    [RCB_STRMD_ON_ROW]     = VDPU38X_RCB_ON_TILE_ROW,
    [RCB_INTER_IN_ROW]     = VDPU38X_RCB_IN_TILE_ROW,
    [RCB_INTER_ON_ROW]     = VDPU38X_RCB_ON_TILE_ROW,
    [RCB_INTRA_IN_ROW]     = VDPU38X_RCB_IN_TILE_ROW,
    [RCB_INTRA_ON_ROW]     = VDPU38X_RCB_ON_TILE_ROW,
    [RCB_FLTD_IN_ROW]      = VDPU38X_RCB_IN_TILE_ROW,
    [RCB_FLTD_PROT_IN_ROW] = VDPU38X_RCB_IN_TILE_ROW,
    [RCB_FLTD_ON_ROW]      = VDPU38X_RCB_ON_TILE_ROW,
    [RCB_FLTD_ON_COL]      = VDPU38X_RCB_ON_TILE_COL,
    [RCB_FLTD_UPSC_ON_COL] = VDPU38X_RCB_ON_TILE_COL,
};

const RK_U32 vdpu38x_intra_uv_coef_map[MPP_HAL_FMT_BUTT] = {
    [MPP_HAL_FMT_YUV400] = 1,
    [MPP_HAL_FMT_YUV420] = 2,
    [MPP_HAL_FMT_YUV422] = 2,
    [MPP_HAL_FMT_YUV444] = 3,
};

const RK_U32 vdpu38x_filter_row_uv_coef_map[MPP_HAL_FMT_BUTT] = {
    [MPP_HAL_FMT_YUV400] = 0,
    [MPP_HAL_FMT_YUV420] = 1,
    [MPP_HAL_FMT_YUV422] = 1,
    [MPP_HAL_FMT_YUV444] = 3,
};

const RK_U32 vdpu38x_filter_col_uv_coef_map[MPP_HAL_FMT_BUTT] = {
    [MPP_HAL_FMT_YUV400] = 0,
    [MPP_HAL_FMT_YUV420] = 1,
    [MPP_HAL_FMT_YUV422] = 3,
    [MPP_HAL_FMT_YUV444] = 3,
};

MPP_RET vdpu38x_rcb_calc_init(Vdpu38xRcbCtx **ctx)
{
    Vdpu38xRcbCtx *p = NULL;

    p = (Vdpu38xRcbCtx *)mpp_calloc(Vdpu38xRcbCtx, 1);
    if (!p) {
        mpp_loge_f("malloc rcb ctx failed\n");
        return MPP_ERR_MALLOC;
    }
    p->tile_infos = (RcbTileInfo *)mpp_calloc(RcbTileInfo, 1);
    if (!p) {
        mpp_loge_f("malloc tile infos failed\n");
        return MPP_ERR_MALLOC;
    }
    p->tile_info_cap = 1;
    p->fmt = MPP_HAL_FMT_BUTT;
    *ctx = p;

    return MPP_OK;
}

MPP_RET vdpu38x_rcb_calc_deinit(Vdpu38xRcbCtx *ctx)
{
    if (NULL != ctx) {
        MPP_FREE(ctx->tile_infos);
        MPP_FREE(ctx);
    }

    return MPP_OK;
}

MPP_RET vdpu38x_rcb_reset(Vdpu38xRcbCtx *ctx)
{
    ctx->pic_w = 0;
    ctx->pic_h = 0;
    /* tile info */
    ctx->tile_num = 0;
    ctx->tile_dir = 0;
    /* general */
    ctx->fmt = MPP_HAL_FMT_BUTT;
    ctx->bit_depth = 0;
    ctx->buf_sz = 0;
    /* h264 */
    ctx->mbaff_flag = 0;
    /* avs2 */
    ctx->alf_en = 0;
    /* av1 */
    ctx->lr_en = 0;
    ctx->upsc_en = 0;

    memset(ctx->buf_info, 0, sizeof(VdpuRcbInfo) * RCB_BUF_CNT);

    return MPP_OK;
}

MPP_RET vdpu38x_rcb_add_tile_info(Vdpu38xRcbCtx *ctx, RcbTileInfo *tile_info)
{
    RcbTileInfo *tl_infos = NULL;
    RcbTileInfo *p = NULL;

    tl_infos = ctx->tile_infos;
    if (ctx->tile_num >= ctx->tile_info_cap) {
        ctx->tile_info_cap += 4;
        tl_infos = (RcbTileInfo *)mpp_realloc(tl_infos, RcbTileInfo, ctx->tile_info_cap);
        if (!tl_infos) {
            mpp_loge_f("realloc failed\n");
            return MPP_ERR_NOMEM;
        }
        ctx->tile_infos = tl_infos;
    }
    p = &tl_infos[ctx->tile_num++];
    memcpy(p, tile_info, sizeof(RcbTileInfo));

    return MPP_OK;
}

MPP_RET vdpu38x_rcb_dump_tile_info(Vdpu38xRcbCtx *ctx)
{
    RcbTileInfo *p = ctx->tile_infos;
    RK_U32 i;

    for (i = 0; i < ctx->tile_num; i++) {
        mpp_logi("tile %d: idx %d lt(%d,%d) w %d h %d\n",
                 i, p[i].idx, p[i].lt_x, p[i].lt_y, p[i].w, p[i].h);
    }

    return MPP_OK;
}

Vdpu38xFmt vdpu38x_fmt_mpp2hal(MppFrameFormat mpp_fmt)
{
    switch (mpp_fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV400 : {
        return MPP_HAL_FMT_YUV400;
    } break;
    case MPP_FMT_YUV420SP :
    case MPP_FMT_YUV420SP_10BIT :
    case MPP_FMT_YUV420P :
    case MPP_FMT_YUV420SP_VU : {
        return MPP_HAL_FMT_YUV420;
    } break;
    case MPP_FMT_YUV422SP :
    case MPP_FMT_YUV422SP_10BIT :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP_VU :
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY : {
        return MPP_HAL_FMT_YUV444;
    } break;
    case MPP_FMT_YUV444SP :
    case MPP_FMT_YUV444P :
    case MPP_FMT_YUV444SP_10BIT : {
        return MPP_HAL_FMT_YUV444;
    } break;
    default : {
        mpp_loge_f("hal format not support %d\n", mpp_fmt);
    } break;
    }

    return MPP_HAL_FMT_BUTT;
}

#define VDPU38X_RCB_ACCESSORS(type, field) \
    type vdpu38x_rcb_get_##field(Vdpu38xRcbCtx *ctx) \
    { \
        return ((Vdpu38xRcbCtx*)ctx)->field; \
    } \
    void vdpu38x_rcb_set_##field(Vdpu38xRcbCtx *ctx, type v) \
    { \
        ((Vdpu38xRcbCtx*)ctx)->field = v; \
    }

VDPU38X_RCB_ACCESSORS(RK_U32, pic_w)
VDPU38X_RCB_ACCESSORS(RK_U32, pic_h)
VDPU38X_RCB_ACCESSORS(RK_U32, tile_dir)
VDPU38X_RCB_ACCESSORS(Vdpu38xFmt, fmt)
VDPU38X_RCB_ACCESSORS(RK_U32, bit_depth)
VDPU38X_RCB_ACCESSORS(RK_U32, mbaff_flag)
VDPU38X_RCB_ACCESSORS(RK_U32, alf_en)
VDPU38X_RCB_ACCESSORS(RK_U32, lr_en)
VDPU38X_RCB_ACCESSORS(RK_U32, upsc_en)

MPP_RET vdpu38x_rcb_get_len(Vdpu38xRcbCtx *ctx, Vdpu38xTileLoc loc, RK_U32 *len)
{
    RcbTileInfo *tile_p = NULL;
    RK_U32 i = 0;
    RK_U32 res = 0;
    RK_U32 ret = MPP_OK;

    tile_p = ctx->tile_infos;

    if (loc == VDPU38X_RCB_IN_TILE_ROW) {
        for (i = 0, res = 0; i < ctx->tile_num; i++)
            res = res < tile_p[i].w ? tile_p[i].w : res;
    } else if (loc == VDPU38X_RCB_IN_TILE_COL) {
        mpp_loge_f("invalid tile loc %d\n", loc);
        ret = MPP_NOK;
    } else if (loc == VDPU38X_RCB_ON_TILE_ROW) {
        res = ctx->pic_w;
    } else if (loc == VDPU38X_RCB_ON_TILE_COL) {
        if (ctx->tile_dir == 0) { /* left to right  */
            for (i = 0, res = 0; i < ctx->tile_num; i++)
                res = res < tile_p[i].h ? tile_p[i].h : res;
        } else { /* top to bottom */
            res = ctx->pic_h;
        }
    } else {
        mpp_loge_f("invalid tile loc %d\n", loc);
        ret = MPP_NOK;
    }
    *len = res;

    return ret;
}

MPP_RET vdpu38x_rcb_get_extra_size(Vdpu38xRcbCtx *ctx, Vdpu38xTileLoc loc, RK_U32 *extra_sz)
{
    RK_U32 tl_row_num = 0;
    RK_U32 tl_col_num = 0;
    RK_U32 buf_size = 0;
    RK_U32 i;

    for (i = 0; i < ctx->tile_num; i++) {
        if (ctx->tile_infos[i].lt_y == 0)
            tl_row_num++;
        if (ctx->tile_infos[i].lt_x == 0)
            tl_col_num++;
    }

    if (loc == VDPU38X_RCB_ON_TILE_ROW)
        buf_size = (tl_row_num - 1) * 64;
    else if (loc == VDPU38X_RCB_ON_TILE_COL)
        buf_size = (tl_col_num - 1) * 64;
    else
        buf_size = 0;

    *extra_sz = buf_size;

    return MPP_OK;
}

RK_U32 vdpu38x_rcb_reg_info_update(Vdpu38xRcbCtx *ctx, Vdpu38xRcbType type, RK_U32 idx,
                                   RK_FLOAT sz)
{
    RK_U32 extra_sz = 0;
    RK_U32 result = 0;
    Vdpu38xTileLoc loc = vdpu38x_rcb_type2loc_map[type];

    vdpu38x_rcb_get_extra_size(ctx, loc, &extra_sz);
    result = MPP_RCB_BYTES(sz) + extra_sz;
    ctx->buf_info[type].reg_idx = idx;
    ctx->buf_info[type].offset = ctx->buf_sz;
    ctx->buf_info[type].size = result;
    ctx->buf_sz += result;

    return result;
}

RK_U32 vdpu38x_rcb_get_total_size(Vdpu38xRcbCtx *ctx)
{
    return ctx->buf_sz;
}

MPP_RET vdpu38x_rcb_register_calc_handle(Vdpu38xRcbCtx *ctx, Vdpu38xRcbCalc_f func)
{
    ctx->calc_func = func;

    return MPP_OK;
}

MPP_RET vdpu38x_rcb_calc_exec(Vdpu38xRcbCtx *ctx, RK_U32 *total_sz)
{
    if (NULL == ctx->calc_func) {
        mpp_logi("error: The compute function is not registered\n");
        return MPP_NOK;
    }

    return ctx->calc_func(ctx, total_sz);
}

void vdpu38x_setup_rcb(Vdpu38xRcbCtx *ctx, Vdpu38xRcbRegSet *regs, MppDev dev,
                       MppBuffer buf)
{
    VdpuRcbInfo *info = ctx->buf_info;
    RK_U32 i;

    regs->strmd_in_row_off     = mpp_buffer_get_fd(buf);
    regs->strmd_on_row_off     = mpp_buffer_get_fd(buf);
    regs->inter_in_row_off     = mpp_buffer_get_fd(buf);
    regs->inter_on_row_off     = mpp_buffer_get_fd(buf);
    regs->intra_in_row_off     = mpp_buffer_get_fd(buf);
    regs->intra_on_row_off     = mpp_buffer_get_fd(buf);
    regs->fltd_in_row_off      = mpp_buffer_get_fd(buf);
    regs->fltd_prot_in_row_off = mpp_buffer_get_fd(buf);
    regs->fltd_on_row_off      = mpp_buffer_get_fd(buf);
    regs->fltd_on_col_off      = mpp_buffer_get_fd(buf);
    regs->fltd_upsc_on_col_off = mpp_buffer_get_fd(buf);

    regs->strmd_in_row_len     = info[RCB_STRMD_IN_ROW].size;
    regs->strmd_on_row_len     = info[RCB_STRMD_ON_ROW].size;
    regs->inter_in_row_len     = info[RCB_INTER_IN_ROW].size;
    regs->inter_on_row_len     = info[RCB_INTER_ON_ROW].size;
    regs->intra_in_row_len     = info[RCB_INTRA_IN_ROW].size;
    regs->intra_on_row_len     = info[RCB_INTRA_ON_ROW].size;
    regs->fltd_in_row_len      = info[RCB_FLTD_IN_ROW].size;
    regs->fltd_prot_in_row_len = info[RCB_FLTD_PROT_IN_ROW].size;
    regs->fltd_on_row_len      = info[RCB_FLTD_ON_ROW].size;
    regs->fltd_on_col_len      = info[RCB_FLTD_ON_COL].size;
    regs->fltd_upsc_on_col_len = info[RCB_FLTD_UPSC_ON_COL].size;

    for (i = 0; i < RCB_BUF_CNT; i++) {
        if (info[i].offset)
            mpp_dev_set_reg_offset(dev, info[i].reg_idx, info[i].offset);
    }
}

RK_S32 vdpu38x_rcb_set_info(Vdpu38xRcbCtx *ctx, MppDev dev)
{
    MppDevRcbInfoCfg rcb_cfg;
    VdpuRcbSetMode set_rcb_mode = RCB_SET_BY_PRIORITY_MODE;
    static const RK_U32 rcb_priority[RCB_BUF_CNT] = {
        RCB_FLTD_IN_ROW,
        RCB_INTER_IN_ROW,
        RCB_INTRA_IN_ROW,
        RCB_STRMD_IN_ROW,
        RCB_INTER_ON_ROW,
        RCB_INTRA_ON_ROW,
        RCB_STRMD_ON_ROW,
        RCB_FLTD_ON_ROW,
        RCB_FLTD_ON_COL,
        RCB_FLTD_UPSC_ON_COL,
        RCB_FLTD_PROT_IN_ROW,
    };
    RK_U32 i;
    /*
     * RCB_SET_BY_SIZE_SORT_MODE: by size sort
     * RCB_SET_BY_PRIORITY_MODE: by priority
     */

    switch (set_rcb_mode) {
    case RCB_SET_BY_SIZE_SORT_MODE : {
        VdpuRcbInfo info[RCB_BUF_CNT];

        memcpy(info, ctx->buf_info, sizeof(info));
        qsort(info, MPP_ARRAY_ELEMS(info),
              sizeof(info[0]), vdpu_compare_rcb_size);

        for (i = 0; i < MPP_ARRAY_ELEMS(info); i++) {
            rcb_cfg.reg_idx = info[i].reg_idx;
            rcb_cfg.size = info[i].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            } else
                break;
        }
    } break;
    case RCB_SET_BY_PRIORITY_MODE : {
        VdpuRcbInfo *info = ctx->buf_info;
        RK_U32 index = 0;

        for (i = 0; i < MPP_ARRAY_ELEMS(rcb_priority); i ++) {
            index = rcb_priority[i];

            rcb_cfg.reg_idx = info[index].reg_idx;
            rcb_cfg.size = info[index].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            }
        }
    } break;
    default:
        break;
    }

    return 0;
}

MPP_RET vdpu38x_rcb_dump_rcb_result(Vdpu38xRcbCtx *ctx)
{
    VdpuRcbInfo *info = ctx->buf_info;
    static const char rcb_descs[RCB_BUF_CNT][32] = {
        "RCB_STRMD_IN_ROW",
        "RCB_STRMD_ON_ROW",
        "RCB_INTER_IN_ROW",
        "RCB_INTER_ON_ROW",
        "RCB_INTRA_IN_ROW",
        "RCB_INTRA_ON_ROW",
        "RCB_FLTD_IN_ROW",
        "RCB_FLTD_PROT_IN_ROW",
        "RCB_FLTD_ON_ROW",
        "RCB_FLTD_ON_COL",
        "RCB_FLTD_UPSC_ON_COL",
    };
    RK_U32 i;

    for (i = 0; i < RCB_BUF_CNT; i++) {
        mpp_logi("rcb buf %2d: desc %-24s reg_idx %3d size %-8d offset %-4d\n",
                 i, rcb_descs[i], info[i].reg_idx, info[i].size, info[i].offset);
    }

    return MPP_OK;
}

MPP_RET vdpu38x_get_fbc_off(MppFrame mframe, RK_U32 *head_stride, RK_U32 *pld_stride, RK_U32 *pld_offset)
{
    MppFrameFormat fmt;
    RK_U32 fbc_unit_w = 0;
    RK_U32 fbc_unit_h = 0;
    RK_U32 bit_depth;
    Vdpu38xFmt fmt_type;
    static const RK_FLOAT fmt_coeff[MPP_HAL_FMT_BUTT] = {1, 1.5, 2, 3};
    RK_U32 ver_virstride = 0;
    RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
    RK_U32 fbc_unit_bit_sz;
    RK_U32 head_vir_w; // byte
    RK_U32 pld_real_w; // byte
    RK_U32 pld_vir_w; // byte

    ver_virstride = mpp_frame_get_ver_stride(mframe);
    fmt = mpp_frame_get_fmt(mframe);

    if (MPP_FRAME_FMT_IS_AFBC(fmt)) {
        fbc_unit_w = 32;
        fbc_unit_h = 8;
    } else if (MPP_FRAME_FMT_IS_RKFBC(fmt)) {
        fbc_unit_w = 64;
        fbc_unit_h = 4;
    }

    bit_depth = MPP_FRAME_FMT_IS_YUV_10BIT(fmt) ? 10 : 8;
    fmt_type = vdpu38x_fmt_mpp2hal(fmt);

    /* head stride */
    head_vir_w = MPP_ALIGN(fbc_hdr_stride, fbc_unit_w) / fbc_unit_w * 16;
    *head_stride =  head_vir_w >> 4;

    /* pld stride */
    fbc_unit_bit_sz = MPP_ALIGN((int)ceilf(fbc_unit_w * fbc_unit_h * fmt_coeff[fmt_type] * bit_depth), 128);
    pld_real_w = fbc_hdr_stride / fbc_unit_w * fbc_unit_bit_sz >> 3;
    pld_vir_w = MPP_ALIGN(pld_real_w, 16);
    *pld_stride = pld_vir_w >> 4;

    /* pld offset*/
    *pld_offset = head_vir_w * (MPP_ALIGN(ver_virstride, fbc_unit_h) / fbc_unit_h);

    return MPP_OK;
}

MPP_RET vdpu38x_get_tile4x4_h_stride_coeff(MppFrameFormat fmt, RK_U32 *coeff)
{
    RK_U32 val = 0;
    MPP_RET ret = MPP_OK;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV400 : {
        val = 4;
    } break;
    case MPP_FMT_YUV420SP :
    case MPP_FMT_YUV420SP_10BIT :
    case MPP_FMT_YUV420P :
    case MPP_FMT_YUV420SP_VU : {
        val = 6;
    } break;
    case MPP_FMT_YUV422SP:
    case MPP_FMT_YUV422SP_10BIT:
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP_VU :
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY : {
        val = 8;
    } break;
    case MPP_FMT_YUV444SP :
    case MPP_FMT_YUV444P :
    case MPP_FMT_YUV444SP_10BIT : {
        val = 12;
    } break;
    default : {
        mpp_err_f("format not support %d\n", fmt);
        ret = MPP_NOK;
    } break;
    }

    *coeff = val;

    return ret;
}

void vdpu384b_init_ctrl_regs(Vdpu38xRegSet *regs, MppCodingType codec_t)
{
    Vdpu38xCtrlReg *ctrl_regs = &regs->ctrl_regs;

    switch (codec_t) {
    case MPP_VIDEO_CodingAVC : {
        ctrl_regs->reg8_dec_mode = 1;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0xfff3ffff;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        ctrl_regs->reg8_dec_mode = 0;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffdfff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0xfffbf9ff;
    } break;
    case MPP_VIDEO_CodingAVS2 : {
        ctrl_regs->reg8_dec_mode = 3;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffdf;
        ctrl_regs->reg21_cabac_error_en_highbits = 0xffffffff;
    } break;
    case MPP_VIDEO_CodingVP9 : {
        ctrl_regs->reg8_dec_mode = 2;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0xffffffff;
    } break;
    case MPP_VIDEO_CodingAV1 : {
        ctrl_regs->reg8_dec_mode = 4;
        ctrl_regs->reg20_cabac_error_en_lowbits  = 0xffffffff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0xffffffff;
    } break;
    default : {
        mpp_err("not support codec type %d\n", codec_t);
    } break;
    }

    ctrl_regs->reg9.collect_info_dis = 1;

    ctrl_regs->reg10.strmd_auto_gating_dis      = 0;
    ctrl_regs->reg10.inter_auto_gating_dis      = 0;
    ctrl_regs->reg10.intra_auto_gating_dis      = 0;
    ctrl_regs->reg10.transd_auto_gating_dis     = 0;
    ctrl_regs->reg10.recon_auto_gating_dis      = 0;
    ctrl_regs->reg10.filterd_auto_gating_dis    = 0;
    ctrl_regs->reg10.bus_auto_gating_dis        = 0;
    ctrl_regs->reg10.ctrl_auto_gating_dis       = 0;
    ctrl_regs->reg10.rcb_auto_gating_dis        = 0;
    ctrl_regs->reg10.err_prc_auto_gating_dis    = 0;
    ctrl_regs->reg10.cache_auto_gating_dis      = 0;

    ctrl_regs->reg11.rd_outstanding = 32;
    ctrl_regs->reg11.wr_outstanding = 250;

    ctrl_regs->reg13_core_timeout_threshold = 0xffffff;

    ctrl_regs->reg16.error_proc_disable = 1;
    ctrl_regs->reg16.error_spread_disable = 0;
    ctrl_regs->reg16.roi_error_ctu_cal_en = 0;

    return;
}

void vdpu38x_setup_statistic(Vdpu38xCtrlReg *ctrl_regs)
{
    ctrl_regs->reg28.axi_perf_work_e = 1;
    ctrl_regs->reg28.axi_cnt_type = 1;
    ctrl_regs->reg28.rd_latency_id = 11;

    ctrl_regs->reg29.addr_align_type     = 1;
    ctrl_regs->reg29.ar_cnt_id_type      = 0;
    ctrl_regs->reg29.aw_cnt_id_type      = 1;
    ctrl_regs->reg29.ar_count_id         = 17;
    ctrl_regs->reg29.aw_count_id         = 0;
    ctrl_regs->reg29.rd_band_width_mode  = 0;

    /* set hurry */
    ctrl_regs->reg30.axi_wr_qos = 0;
    ctrl_regs->reg30.axi_rd_qos = 0;
}

void vdpu38x_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand)
{
    RK_U32 ver_stride = 0;
    RK_U32 img_height = mpp_frame_get_height(frame);
    RK_U32 img_width = mpp_frame_get_width(frame);
    RK_U32 hdr_stride = (*compat_ext_fbc_hdr_256_odd) ?
                        (MPP_ALIGN(img_width, 256) | 256) :
                        (MPP_ALIGN(img_width, 64));

    mpp_slots_set_prop(slots, SLOTS_HOR_ALIGN, mpp_align_64);
    mpp_slots_set_prop(slots, SLOTS_VER_ALIGN, mpp_align_16);

    mpp_frame_set_fbc_hdr_stride(frame, hdr_stride);

    ver_stride = mpp_align_16(img_height);
    if (*compat_ext_fbc_buf_size) {
        ver_stride += expand;
    }
    mpp_frame_set_ver_stride(frame, ver_stride);
}

void vdpu38x_update_thumbnail_frame_info(MppFrame frame)
{
    RK_U32 down_scale_height = mpp_frame_get_height(frame) >> 1;
    RK_U32 down_scale_width = mpp_frame_get_width(frame) >> 1;
    RK_U32 down_scale_ver = MPP_ALIGN(down_scale_height, 16);
    RK_U32 down_scale_hor = MPP_ALIGN(down_scale_width, 16);
    RK_U32 down_scale_buf_size = 0;

    if (!MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(frame))) {
        down_scale_hor = mpp_align_128_odd_plus_64(down_scale_hor);
        down_scale_ver = mpp_frame_get_ver_stride(frame) >> 1;
    }

    down_scale_buf_size = down_scale_hor * down_scale_ver *  3 / 2;
    /*
     *  no matter what format, scale down image will output as 8bit raster format;
     */
    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
    mpp_frame_set_width(frame, down_scale_width);
    mpp_frame_set_height(frame, down_scale_height);
    mpp_frame_set_hor_stride(frame, down_scale_hor);
    mpp_frame_set_ver_stride(frame, down_scale_ver);
    mpp_frame_set_buf_size(frame, down_scale_buf_size);
}

void vdpu38x_setup_down_scale(MppFrame frame, MppDev dev, Vdpu38xCtrlReg *com, void* comParas)
{
    RK_U32 down_scale_height = mpp_frame_get_height(frame) >> 1;
    RK_U32 down_scale_width = mpp_frame_get_width(frame) >> 1;
    RK_U32 down_scale_ver = MPP_ALIGN(down_scale_height, 16);
    RK_U32 down_scale_hor = MPP_ALIGN(down_scale_width, 16);

    Vdpu38xRegCommParas* paras = (Vdpu38xRegCommParas*)comParas;
    MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    MppMeta meta = mpp_frame_get_meta(frame);
    RK_U32 down_scale_y_offset = 0;
    RK_U32 down_scale_uv_offset = 0;
    RK_U32 down_scale_y_virstride = down_scale_ver * down_scale_hor;
    RK_U32 downscale_buf_size;

    if (!MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(frame))) {
        down_scale_hor = mpp_align_128_odd_plus_64(down_scale_hor);
        down_scale_ver = mpp_frame_get_ver_stride(frame) >> 1;
        down_scale_y_virstride = down_scale_ver * down_scale_hor;
    }
    /*
     *  no matter what format, scale down image will output as 8bit raster format;
     *  down_scale image buffer size was already added to the buf_size of mpp_frame,
     *  which was calculated in mpp_buf_slot.cpp: (size = original_size + scaledown_size)
     */
    switch ((fmt & MPP_FRAME_FMT_MASK)) {
    case MPP_FMT_YUV400 : {
        downscale_buf_size = down_scale_y_virstride;
    } break;
    case MPP_FMT_YUV420SP_10BIT :
    case MPP_FMT_YUV420SP : {
        downscale_buf_size = down_scale_y_virstride * 3 / 2;
    } break;
    case MPP_FMT_YUV422SP_10BIT :
    case MPP_FMT_YUV422SP : {
        downscale_buf_size = down_scale_y_virstride * 2;
    } break;
    case MPP_FMT_YUV444SP : {
        downscale_buf_size = down_scale_y_virstride * 3;
    } break;
    default : {
        downscale_buf_size = down_scale_y_virstride * 3 / 2;
    } break;
    }
    downscale_buf_size = MPP_ALIGN(downscale_buf_size, 16);

    down_scale_y_offset = MPP_ALIGN((mpp_frame_get_buf_size(frame) - downscale_buf_size), 16);
    down_scale_uv_offset = down_scale_y_offset + down_scale_y_virstride;

    com->reg9.scale_down_en = 1;
    com->reg9.av1_fgs_en = 0;
    paras->reg71_scl_ref_hor_virstride = down_scale_hor >> 4;
    paras->reg72_scl_ref_raster_uv_hor_virstride = down_scale_hor >> 4;
    if ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV444SP)
        paras->reg72_scl_ref_raster_uv_hor_virstride = down_scale_hor >> 3;
    paras->reg73_scl_ref_virstride = down_scale_y_virstride >> 4;
    if (mpp_frame_get_thumbnail_en(frame) == MPP_FRAME_THUMBNAIL_MIXED) {
        mpp_dev_set_reg_offset(dev, 133, down_scale_y_offset);
        mpp_meta_set_s32(meta, KEY_DEC_TBN_Y_OFFSET, down_scale_y_offset);
        mpp_meta_set_s32(meta, KEY_DEC_TBN_UV_OFFSET, down_scale_uv_offset);
    }
}

MPP_RET vdpu38x_setup_scale_origin_bufs(MppFrame mframe, HalBufs *org_bufs, RK_S32 max_cnt)
{
    /* for 8K FrameBuf scale mode */
    size_t origin_buf_size = 0;

    origin_buf_size = mpp_frame_get_buf_size(mframe);

    if (!origin_buf_size) {
        mpp_err_f("origin_bufs get buf size failed\n");
        return MPP_NOK;
    }
    if (*org_bufs) {
        hal_bufs_deinit(*org_bufs);
        *org_bufs = NULL;
    }
    hal_bufs_init(org_bufs);
    if (!(*org_bufs)) {
        mpp_err_f("org_bufs init fail\n");
        return MPP_ERR_NOMEM;
    }
    hal_bufs_setup(*org_bufs, max_cnt > 0 ? max_cnt : 16, 1, &origin_buf_size);

    return MPP_OK;
}

RK_S32 hal_h265d_avs2d_calc_mv_size(RK_S32 pic_w, RK_S32 pic_h, RK_S32 ctu_w)
{
    RK_S32 seg_w = 64 * 16 * 16 / ctu_w; // colmv_block_size = 16, colmv_per_bytes = 16
    RK_S32 seg_cnt_w = MPP_ALIGN(pic_w, seg_w) / seg_w;
    RK_S32 seg_cnt_h = MPP_ALIGN(pic_h, ctu_w) / ctu_w;
    RK_S32 mv_size   = seg_cnt_w * seg_cnt_h * 64 * 16;

    return mv_size;
}

#ifdef DUMP_VDPU38X_DATAS
RK_U32 vdpu38x_dump_cur_frm = 0;
char vdpu38x_dump_cur_dir[128];
char vdpu38x_dump_cur_fname_path[512];

MPP_RET vdpu38x_flip_string(char *str)
{
    RK_U32 len = strlen(str);
    RK_U32 i, j;

    for (i = 0, j = len - 1; i <= j; i++, j--) {
        // swapping characters
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }

    return MPP_OK;
}

MPP_RET vdpu38x_dump_data_to_file(char *fname_path, void *data, RK_U32 data_bit_size,
                                  RK_U32 line_bits, RK_U32 big_end, RK_U32 append)
{
    RK_U8 *buf_p = (RK_U8 *)data;
    RK_U8 cur_data;
    RK_U32 i;
    RK_U32 loop_cnt;
    FILE *dump_fp = NULL;
    char line_tmp[256];
    RK_U32 str_idx = 0;

    if (append)
        dump_fp = fopen(fname_path, "aw+");
    else
        dump_fp = fopen(fname_path, "w+");
    if (!dump_fp) {
        mpp_err_f("open file: %s error!\n", fname_path);
        return MPP_NOK;
    }
    if (append)
        fseek(dump_fp, 0, SEEK_END);

    if ((data_bit_size % 4 != 0) || (line_bits % 8 != 0)) {
        mpp_err_f("line bits not align to 4! data_bit_size: %d  line_bits: %d\n",
                  data_bit_size, line_bits);
        return MPP_NOK;
    }

    loop_cnt = data_bit_size / 8;
    for (i = 0; i < loop_cnt; i++) {
        cur_data = buf_p[i];

        sprintf(&line_tmp[str_idx++], "%0x", cur_data & 0xf);
        if ((i * 8 + 4) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                vdpu38x_flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
        sprintf(&line_tmp[str_idx++], "%0x", (cur_data >> 4) & 0xf);
        if ((i * 8 + 8) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                vdpu38x_flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
    }

    // last line
    if (data_bit_size % 4) {
        cur_data = buf_p[i];
        sprintf(&line_tmp[str_idx++], "%0x", cur_data & 0xf);
        if ((i * 8 + 8) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                vdpu38x_flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
    }
    if (data_bit_size % line_bits) {
        loop_cnt = (line_bits - (data_bit_size % line_bits)) / 4;
        for (i = 0; i < loop_cnt; i++)
            sprintf(&line_tmp[str_idx++], "%0x", 0);
        line_tmp[str_idx++] = '\0';
        str_idx = 0;
        if (!big_end)
            vdpu38x_flip_string(line_tmp);
        fprintf(dump_fp, "%s\n", line_tmp);
    }

    fflush(dump_fp);
    fclose(dump_fp);

    return MPP_OK;
}
#endif
