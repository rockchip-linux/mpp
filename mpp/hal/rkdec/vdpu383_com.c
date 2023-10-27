/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpu383_com"

#include <string.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"

#include "vdpu383_com.h"

static RK_U32 rcb_coeff[RCB_BUF_COUNT] = {
    [RCB_STRMD_ROW]                 = 3,
    [RCB_STRMD_TILE_ROW]            = 3,
    [RCB_INTER_ROW]                 = 6,
    [RCB_INTER_TILE_ROW]            = 6,
    [RCB_INTRA_ROW]                 = 8,
    [RCB_INTRA_TILE_ROW]            = 8,
    [RCB_FILTERD_ROW]               = 90,
    [RCB_FILTERD_PROTECT_ROW]       = 0,
    [RCB_FILTERD_TILE_ROW]          = 90,
    [RCB_FILTERD_TILE_COL]          = 90,
    [RCB_FILTERD_AV1_UP_TILE_COL]   = 0,
};

static RK_S32 update_size_offset(Vdpu383RcbInfo *info, RK_U32 reg_idx,
                                 RK_S32 offset, RK_S32 len, RK_S32 idx)
{
    RK_S32 buf_size = 0;

    buf_size = 2 * MPP_ALIGN(len * rcb_coeff[idx], RCB_ALLINE_SIZE);
    info[idx].reg_idx = reg_idx;
    info[idx].offset = offset;
    info[idx].size = buf_size;

    return buf_size;
}

RK_S32 vdpu383_get_rcb_buf_size(Vdpu383RcbInfo *info, RK_S32 width, RK_S32 height)
{
    RK_S32 offset = 0;

    offset += update_size_offset(info, 140, offset, width, RCB_STRMD_ROW);
    offset += update_size_offset(info, 142, offset, width, RCB_STRMD_TILE_ROW);
    offset += update_size_offset(info, 144, offset, width, RCB_INTER_ROW);
    offset += update_size_offset(info, 146, offset, width, RCB_INTER_TILE_ROW);
    offset += update_size_offset(info, 148, offset, width, RCB_INTRA_ROW);
    offset += update_size_offset(info, 150, offset, width, RCB_INTRA_TILE_ROW);
    offset += update_size_offset(info, 152, offset, width, RCB_FILTERD_ROW);
    offset += update_size_offset(info, 154, offset, width, RCB_FILTERD_PROTECT_ROW);
    offset += update_size_offset(info, 156, offset, width, RCB_FILTERD_TILE_ROW);
    offset += update_size_offset(info, 158, offset, height, RCB_FILTERD_TILE_COL);
    offset += update_size_offset(info, 160, offset, height, RCB_FILTERD_AV1_UP_TILE_COL);

    return offset;
}

void vdpu383_setup_rcb(Vdpu383RegCommonAddr *reg, MppDev dev,
                       MppBuffer buf, Vdpu383RcbInfo *info)
{
    MppDevRegOffsetCfg trans_cfg;
    RK_U32 i;

    reg->reg140_rcb_strmd_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg142_rcb_strmd_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg144_rcb_inter_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg146_rcb_inter_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg148_rcb_intra_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg150_rcb_intra_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg152_rcb_filterd_row_offset         = mpp_buffer_get_fd(buf);
    reg->reg154_rcb_filterd_protect_row_offset = mpp_buffer_get_fd(buf);
    reg->reg156_rcb_filterd_tile_row_offset    = mpp_buffer_get_fd(buf);
    reg->reg158_rcb_filterd_tile_col_offset    = mpp_buffer_get_fd(buf);
    reg->reg160_rcb_filterd_av1_upscale_tile_col_offset = mpp_buffer_get_fd(buf);

    reg->reg141_rcb_strmd_row_len            =  info[RCB_STRMD_ROW].size          ;
    reg->reg143_rcb_strmd_tile_row_len       =  info[RCB_STRMD_TILE_ROW].size     ;
    reg->reg145_rcb_inter_row_len            =  info[RCB_INTER_ROW].size          ;
    reg->reg147_rcb_inter_tile_row_len       =  info[RCB_INTER_TILE_ROW].size     ;
    reg->reg149_rcb_intra_row_len            =  info[RCB_INTRA_ROW].size          ;
    reg->reg151_rcb_intra_tile_row_len       =  info[RCB_INTRA_TILE_ROW].size     ;
    reg->reg153_rcb_filterd_row_len          =  info[RCB_FILTERD_ROW].size        ;
    reg->reg155_rcb_filterd_protect_row_len  =  info[RCB_FILTERD_PROTECT_ROW].size;
    reg->reg157_rcb_filterd_tile_row_len     =  info[RCB_FILTERD_TILE_ROW].size   ;
    reg->reg159_rcb_filterd_tile_col_len     =  info[RCB_FILTERD_TILE_COL].size   ;
    reg->reg161_rcb_filterd_av1_upscale_tile_col_len = info[RCB_FILTERD_AV1_UP_TILE_COL].size;

    for (i = 0; i < RCB_BUF_COUNT; i++) {
        if (info[i].offset) {
            trans_cfg.reg_idx = info[i].reg_idx;
            trans_cfg.offset = info[i].offset;
            mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
        }
    }
}

RK_S32 vdpu383_compare_rcb_size(const void *a, const void *b)
{
    RK_S32 val = 0;
    Vdpu383RcbInfo *p0 = (Vdpu383RcbInfo *)a;
    Vdpu383RcbInfo *p1 = (Vdpu383RcbInfo *)b;

    val = (p0->size > p1->size) ? -1 : 1;

    return val;
}

void vdpu383_setup_statistic(Vdpu383CtrlReg *ctrl_regs)
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

void vdpu383_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand)
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

RK_S32 vdpu383_set_rcbinfo(MppDev dev, Vdpu383RcbInfo *rcb_info)
{
    MppDevRcbInfoCfg rcb_cfg;
    RK_U32 i;
    /*
     * RCB_SET_BY_SIZE_SORT_MODE: by size sort
     * RCB_SET_BY_PRIORITY_MODE: by priority
     */

    for (i = 0; i < RCB_BUF_COUNT; i++) {
        rcb_cfg.reg_idx = rcb_info[i].reg_idx;
        rcb_cfg.size = rcb_info[i].size;
        if (rcb_cfg.size > 0) {
            mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
        }
    }

    return 0;
}
