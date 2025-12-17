/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_av1d_vdpu383"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "vdpu_com.h"
#include "hal_av1d_common.h"
#include "hal_av1d_ctx.h"
#include "hal_av1d_com.h"
#include "vdpu383_av1d.h"
#include "vdpu383_com.h"
#include "vdpu38x_com.h"

#include "av1d_syntax.h"
#include "film_grain_noise_table.h"
#include "av1d_syntax.h"

#define VDPU383_UNCMPS_HEADER_SIZE            (MPP_ALIGN(5160, 128) / 8 + 16) // byte, 5160 bit, reverse 128 bits

// bits len
#define VDPU383_RCB_STRMD_ROW_LEN             (MPP_ALIGN(dxva->width, 8) / 8 * 100)
#define VDPU383_RCB_STRMD_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 8) / 8 * 100)
#define VDPU383_RCB_INTER_ROW_LEN             (MPP_ALIGN(dxva->width, 64) / 64 * 2752)
#define VDPU383_RCB_INTER_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 64) / 64 * 2752)
#define VDPU383_RCB_INTRA_ROW_LEN             (MPP_ALIGN(dxva->width, 512) * 12 * 3)
#define VDPU383_RCB_INTRA_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 512) * 12 * 3)
#define VDPU383_RCB_FILTERD_ROW_LEN           (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_PROTECT_ROW_LEN   (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_TILE_ROW_LEN      (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_TILE_COL_LEN      (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 7 * 3 + (14 + 13 * 3) + (9 + 7 * 3)))
#define VDPU383_RCB_FILTERD_AV1_UP_TL_COL_LEN (MPP_ALIGN(dxva->width, 64) * 10 * 22)

#define VDPU383_UNCMPS_HEADER_OFFSET_BASE            (0)
#define VDPU383_INFO_ELEM_SIZE (VDPU383_UNCMPS_HEADER_SIZE)
#define VDPU383_UNCMPS_HEADER_OFFSET(idx)            (VDPU383_INFO_ELEM_SIZE * idx + VDPU383_UNCMPS_HEADER_OFFSET_BASE)
#define VDPU383_INFO_BUF_SIZE(cnt) (VDPU383_INFO_ELEM_SIZE * cnt)

#define SET_REF_HOR_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg83_ref0_hor_virstride = value; break;\
        case 1: regs.reg86_ref1_hor_virstride = value; break;\
        case 2: regs.reg89_ref2_hor_virstride = value; break;\
        case 3: regs.reg92_ref3_hor_virstride = value; break;\
        case 4: regs.reg95_ref4_hor_virstride = value; break;\
        case 5: regs.reg98_ref5_hor_virstride = value; break;\
        case 6: regs.reg101_ref6_hor_virstride = value; break;\
        case 7: regs.reg104_ref7_hor_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_RASTER_UV_HOR_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg84_ref0_raster_uv_hor_virstride = value; break;\
        case 1: regs.reg87_ref1_raster_uv_hor_virstride = value; break;\
        case 2: regs.reg90_ref2_raster_uv_hor_virstride = value; break;\
        case 3: regs.reg93_ref3_raster_uv_hor_virstride = value; break;\
        case 4: regs.reg96_ref4_raster_uv_hor_virstride = value; break;\
        case 5: regs.reg99_ref5_raster_uv_hor_virstride = value; break;\
        case 6: regs.reg102_ref6_raster_uv_hor_virstride = value; break;\
        case 7: regs.reg105_ref7_raster_uv_hor_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg85_ref0_virstride = value; break;\
        case 1: regs.reg88_ref1_virstride = value; break;\
        case 2: regs.reg91_ref2_virstride = value; break;\
        case 3: regs.reg94_ref3_virstride = value; break;\
        case 4: regs.reg97_ref4_virstride = value; break;\
        case 5: regs.reg100_ref5_virstride = value; break;\
        case 6: regs.reg103_ref6_virstride = value; break;\
        case 7: regs.reg106_ref7_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_BASE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg170_av1_last_base      = value; break; \
        case 1: regs.reg171_av1golden_base     = value; break; \
        case 2: regs.reg172_av1alfter_base     = value; break; \
        case 3: regs.reg173_refer3_base        = value; break; \
        case 4: regs.reg174_refer4_base        = value; break; \
        case 5: regs.reg175_refer5_base        = value; break; \
        case 6: regs.reg176_refer6_base        = value; break; \
        case 7: regs.reg177_refer7_base        = value; break; \
        default: break;}\
    }while(0)

#define SET_FBC_PAYLOAD_REF_BASE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg195_payload_st_ref0_base    = value; break; \
        case 1: regs.reg196_payload_st_ref1_base    = value; break; \
        case 2: regs.reg197_payload_st_ref2_base    = value; break; \
        case 3: regs.reg198_payload_st_ref3_base    = value; break; \
        case 4: regs.reg199_payload_st_ref4_base    = value; break; \
        case 5: regs.reg200_payload_st_ref5_base    = value; break; \
        case 6: regs.reg201_payload_st_ref6_base    = value; break; \
        case 7: regs.reg202_payload_st_ref7_base    = value; break; \
        default: break;}\
    }while(0)

static MPP_RET hal_av1d_alloc_res(void *hal)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;
    void *cdf_ptr;
    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, p_hal->api->ctx_size));
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, MPP_ALIGN(VDPU383_INFO_BUF_SIZE(max_cnt), SZ_2K)));
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);


    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383Av1dRegSet, 1);
        memset(reg_ctx->reg_buf[i].regs, 0, sizeof(Vdpu383Av1dRegSet));
        reg_ctx->uncmps_offset[i] = VDPU383_UNCMPS_HEADER_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->offset_uncomps = reg_ctx->uncmps_offset[0];
    }

    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->cdf_rd_def_base,
                                  MPP_ALIGN(sizeof(g_av1d_default_prob), SZ_2K)));
    mpp_buffer_attach_dev(reg_ctx->cdf_rd_def_base, p_hal->dev);
    cdf_ptr = mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base);
    memcpy(cdf_ptr, g_av1d_default_prob, sizeof(g_av1d_default_prob));
    mpp_buffer_sync_end(reg_ctx->cdf_rd_def_base);

__RETURN:
    return ret;
__FAILED:
    return ret;
}

MPP_RET vdpu383_av1d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu38xAv1dRegCtx *reg_ctx = NULL;
    INP_CHECK(ret, NULL == p_hal);
    (void) cfg;

    FUN_CHECK(hal_av1d_alloc_res(hal));

    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(p_hal->slots, SLOTS_VER_ALIGN, mpp_align_8);
    mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

    reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    vdpu38x_rcb_calc_init((Vdpu38xRcbCtx **)&reg_ctx->rcb_ctx);

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu38x_av1d_deinit(hal);

    return ret;
}

static RK_S32 update_size_offset(VdpuRcbInfo *info, RK_U32 reg_idx,
                                 RK_S32 offset, RK_S32 len, RK_S32 rcb_buf_idx)
{
    RK_S32 buf_size = 0;

    buf_size = MPP_RCB_BYTES(len);
    info[rcb_buf_idx].reg_idx = reg_idx;
    info[rcb_buf_idx].offset = offset;
    info[rcb_buf_idx].size = buf_size;

    return buf_size;
}

static void av1d_refine_rcb_size(VdpuRcbInfo *rcb_info,
                                 RK_S32 width, RK_S32 height, void* data)
{
    RK_U32 rcb_bits = 0;
    DXVA_PicParams_AV1 *pic_param = (DXVA_PicParams_AV1*)data;
    RK_U32 tile_row_num = pic_param->tiles.rows;
    RK_U32 tile_col_num = pic_param->tiles.cols;
    RK_U32 bit_depth = pic_param->bitdepth;
    RK_U32 sb_size = pic_param->coding.use_128x128_superblock ? 128 : 64;
    RK_U32 ext_row_align_size = tile_row_num * 64 * 8;
    RK_U32 ext_col_align_size = tile_col_num * 64 * 8;
    RK_U32 filterd_row_append = 8192;

    width = MPP_ALIGN(width, sb_size);
    height = MPP_ALIGN(height, sb_size);
    /* RCB_STRMD_IN_ROW && RCB_STRMD_ON_ROW*/
    rcb_bits = ((width + 7) / 8) * 100;
    rcb_info[RCB_STRMD_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_ON_ROW].size = 0;

    /* RCB_INTER_IN_ROW && RCB_INTER_ON_ROW*/
    rcb_bits = ((width + 63) / 64) * 2752;
    rcb_info[RCB_INTER_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTER_ON_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTER_ON_ROW].size = 0;

    /* RCB_INTRA_IN_ROW && RCB_INTRA_ON_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 3; //TODO:
    rcb_info[RCB_INTRA_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTRA_ON_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTRA_ON_ROW].size = 0;

    /* RCB_FLTD_IN_ROW && RCB_FLTD_ON_ROW*/
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_bits = (RK_U32)(MPP_ALIGN(width, 64) * (32 * bit_depth + 10));
    rcb_info[RCB_FLTD_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FLTD_PROT_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_FLTD_ON_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_FLTD_ON_ROW].size = 0;

    /* RCB_FLTD_ON_COL */
    if (tile_col_num > 1) {
        rcb_bits = (MPP_ALIGN(height, 64) * (101 * bit_depth + 32)) + ext_col_align_size;
        rcb_info[RCB_FLTD_ON_COL].size = MPP_RCB_BYTES(rcb_bits);
    } else {
        rcb_info[RCB_FLTD_ON_COL].size = 0;
    }

}

static void vdpu383_av1d_rcb_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    RK_U32 offset = 0;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i;

    offset += update_size_offset(reg_ctx->rcb_buf_info, 140, offset, VDPU383_RCB_STRMD_ROW_LEN,             RCB_STRMD_IN_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 142, offset, VDPU383_RCB_STRMD_TILE_ROW_LEN,        RCB_STRMD_ON_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 144, offset, VDPU383_RCB_INTER_ROW_LEN,             RCB_INTER_IN_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 146, offset, VDPU383_RCB_INTER_TILE_ROW_LEN,        RCB_INTER_ON_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 148, offset, VDPU383_RCB_INTRA_ROW_LEN,             RCB_INTRA_IN_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 150, offset, VDPU383_RCB_INTRA_TILE_ROW_LEN,        RCB_INTRA_ON_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 152, offset, VDPU383_RCB_FILTERD_ROW_LEN,           RCB_FLTD_IN_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 154, offset, VDPU383_RCB_FILTERD_PROTECT_ROW_LEN,   RCB_FLTD_PROT_IN_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 156, offset, VDPU383_RCB_FILTERD_TILE_ROW_LEN,      RCB_FLTD_ON_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 158, offset, VDPU383_RCB_FILTERD_TILE_COL_LEN,      RCB_FLTD_ON_COL);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 160, offset, VDPU383_RCB_FILTERD_AV1_UP_TL_COL_LEN, RCB_FLTD_UPSC_ON_COL);
    reg_ctx->rcb_buf_size = offset;

    av1d_refine_rcb_size(reg_ctx->rcb_buf_info, dxva->width, dxva->height, dxva);

    for (i = 0; i < max_cnt; i++) {
        MppBuffer rcb_buf = reg_ctx->rcb_bufs[i];

        if (rcb_buf) {
            mpp_buffer_put(rcb_buf);
            reg_ctx->rcb_bufs[i] = NULL;
        }
        mpp_buffer_get(p_hal->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        reg_ctx->rcb_bufs[i] = rcb_buf;
    }

    return;
}

static void vdpu383_av1d_rcb_reg_cfg(Av1dHalCtx *p_hal, MppBuffer buf)
{
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs = reg_ctx->regs;
    RK_U32 fd = mpp_buffer_get_fd(buf);
    RK_U32 i;

    regs->common_addr.reg140_rcb_strmd_row_offset           = fd;
    regs->common_addr.reg142_rcb_strmd_tile_row_offset      = fd;
    regs->common_addr.reg144_rcb_inter_row_offset           = fd;
    regs->common_addr.reg146_rcb_inter_tile_row_offset      = fd;
    regs->common_addr.reg148_rcb_intra_row_offset           = fd;
    regs->common_addr.reg150_rcb_intra_tile_row_offset      = fd;
    regs->common_addr.reg152_rcb_filterd_row_offset         = fd;
    regs->common_addr.reg154_rcb_filterd_protect_row_offset = fd;
    regs->common_addr.reg156_rcb_filterd_tile_row_offset    = fd;
    regs->common_addr.reg158_rcb_filterd_tile_col_offset    = fd;
    regs->common_addr.reg160_rcb_filterd_av1_upscale_tile_col_offset = fd;

    regs->common_addr.reg141_rcb_strmd_row_len            = reg_ctx->rcb_buf_info[RCB_STRMD_IN_ROW].size;
    regs->common_addr.reg143_rcb_strmd_tile_row_len       = reg_ctx->rcb_buf_info[RCB_STRMD_ON_ROW].size;
    regs->common_addr.reg145_rcb_inter_row_len            = reg_ctx->rcb_buf_info[RCB_INTER_IN_ROW].size;
    regs->common_addr.reg147_rcb_inter_tile_row_len       = reg_ctx->rcb_buf_info[RCB_INTER_ON_ROW].size;
    regs->common_addr.reg149_rcb_intra_row_len            = reg_ctx->rcb_buf_info[RCB_INTRA_IN_ROW].size;
    regs->common_addr.reg151_rcb_intra_tile_row_len       = reg_ctx->rcb_buf_info[RCB_INTRA_ON_ROW].size;
    regs->common_addr.reg153_rcb_filterd_row_len          = reg_ctx->rcb_buf_info[RCB_FLTD_IN_ROW].size;
    regs->common_addr.reg155_rcb_filterd_protect_row_len  = reg_ctx->rcb_buf_info[RCB_FLTD_PROT_IN_ROW].size;
    regs->common_addr.reg157_rcb_filterd_tile_row_len     = reg_ctx->rcb_buf_info[RCB_FLTD_ON_ROW].size;
    regs->common_addr.reg159_rcb_filterd_tile_col_len     = reg_ctx->rcb_buf_info[RCB_FLTD_ON_COL].size;
    regs->common_addr.reg161_rcb_filterd_av1_upscale_tile_col_len  = reg_ctx->rcb_buf_info[RCB_FLTD_UPSC_ON_COL].size;

    for (i = 0; i < RCB_BUF_CNT; i++)
        mpp_dev_set_reg_offset(p_hal->dev, reg_ctx->rcb_buf_info[i].reg_idx, reg_ctx->rcb_buf_info[i].offset);
}

MPP_RET vdpu383_av1d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu38xAv1dRegCtx *ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs;
    DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
    RK_U32 i = 0;
    HalBuf *origin_buf = NULL;
    MppFrame mframe;

    INP_CHECK(ret, NULL == p_hal);

    ctx->refresh_frame_flags = dxva->refresh_frame_flags;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        mpp_err_f("parse err %d ref err %d\n",
                  task->dec.flags.parse_err, task->dec.flags.ref_err);
        goto __RETURN;
    }

    mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        MPP_MAX(dxva->width, dxva->height) > 4096 && ctx->origin_bufs == NULL) {
        vdpu38x_setup_scale_origin_bufs(mframe, &ctx->origin_bufs);
    }

    if (p_hal->fast_mode) {
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                ctx->regs = ctx->reg_buf[i].regs;
                ctx->reg_buf[i].valid = 1;
                ctx->offset_uncomps = ctx->uncmps_offset[i];
                break;
            }
        }
    }
    regs = ctx->regs;
    memset(regs, 0, sizeof(*regs));
    p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);

#ifdef DUMP_VDPU38X_DATAS
    {
        memset(vdpu38x_dump_cur_dir, 0, sizeof(vdpu38x_dump_cur_dir));
        sprintf(vdpu38x_dump_cur_dir, "av1/Frame%04d", vdpu38x_dump_cur_frm);
        if (access(vdpu38x_dump_cur_dir, 0)) {
            if (mkdir(vdpu38x_dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", vdpu38x_dump_cur_dir);
        }
        vdpu38x_dump_cur_frm++;
    }
#endif

    /* set reg -> ctrl reg */
    {
        regs->ctrl_regs.reg8_dec_mode          = 4; // av1
        regs->ctrl_regs.reg9.fbc_e             = 0;
        regs->ctrl_regs.reg9.buf_empty_en      = 0;

        regs->ctrl_regs.reg10.strmd_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.inter_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.intra_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.transd_auto_gating_e   = 1;
        regs->ctrl_regs.reg10.recon_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.filterd_auto_gating_e  = 1;
        regs->ctrl_regs.reg10.bus_auto_gating_e      = 1;
        regs->ctrl_regs.reg10.ctrl_auto_gating_e     = 1;
        regs->ctrl_regs.reg10.rcb_auto_gating_e      = 1;
        regs->ctrl_regs.reg10.err_prc_auto_gating_e  = 1;

        // regs->ctrl_regs.reg11.dec_timeout_dis        = 1;

        regs->ctrl_regs.reg13_core_timeout_threshold  = 0x3fffff;

        regs->ctrl_regs.reg16.error_proc_disable     = 1;
        regs->ctrl_regs.reg16.error_spread_disable   = 0;
        regs->ctrl_regs.reg16.roi_error_ctu_cal_en   = 0;

        regs->ctrl_regs.reg20_cabac_error_en_lowbits  = 0xffffffdf;
        regs->ctrl_regs.reg21_cabac_error_en_highbits = 0x3fffffff;

        regs->ctrl_regs.reg28.axi_perf_work_e = 1;
        regs->ctrl_regs.reg28.axi_cnt_type    = 1;
        regs->ctrl_regs.reg28.rd_latency_id   = 11;

        regs->ctrl_regs.reg29.addr_align_type     = 1;
        regs->ctrl_regs.reg29.ar_cnt_id_type      = 0;
        regs->ctrl_regs.reg29.aw_cnt_id_type      = 1;
        regs->ctrl_regs.reg29.ar_count_id         = 17;
        regs->ctrl_regs.reg29.aw_count_id         = 0;
        regs->ctrl_regs.reg29.rd_band_width_mode  = 0;

        regs->ctrl_regs.reg30.axi_wr_qos = 0;
        regs->ctrl_regs.reg30.axi_rd_qos = 0;
    }

    /* set reg -> pkt data */
    {
        MppBuffer mbuffer = NULL;

        /* uncompress header data */
        vdpu38x_av1d_uncomp_hdr(p_hal, dxva, (RK_U64 *)ctx->header_data, VDPU383_UNCMPS_HEADER_SIZE / 8);
        memcpy((char *)ctx->bufs_ptr, (void *)ctx->header_data, VDPU383_UNCMPS_HEADER_SIZE);
        regs->av1d_paras.reg67_global_len = VDPU383_UNCMPS_HEADER_SIZE / 16; // 128 bit as unit
        regs->common_addr.reg131_gbl_base = ctx->bufs_fd;
        // mpp_dev_set_reg_offset(p_hal->dev, 131, ctx->offset_uncomps);
#ifdef DUMP_VDPU38X_DATAS
        {
            char *cur_fname = "global_cfg.dat";
            memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
            sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, ctx->bufs_ptr,
                                      8 * regs->av1d_paras.reg67_global_len * 16, 64, 0, 0);
        }
#endif
        // input strm
        p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
        regs->av1d_paras.reg66_stream_len = MPP_ALIGN(p_hal->strm_len + 15, 128);
        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        regs->av1d_paras.reg65_strm_start_bit = (ctx->offset_uncomps & 0xf) * 8; // bit start to decode
        mpp_dev_set_reg_offset(p_hal->dev, 128, ctx->offset_uncomps & 0xfffffff0);
        /* error */
        regs->av1d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
#ifdef DUMP_VDPU38X_DATAS
        {
            char *cur_fname = "stream_in.dat";
            memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
            sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer)
                                      + ctx->offset_uncomps,
                                      8 * p_hal->strm_len, 128, 0, 0);
        }
        {
            char *cur_fname = "stream_in_no_offset.dat";
            memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
            sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                                      8 * p_hal->strm_len, 128, 0, 0);
        }
#endif
    }

    /* set reg -> rcb */
    vdpu383_av1d_rcb_setup(p_hal, dxva);
    vdpu383_av1d_rcb_reg_cfg(p_hal, p_hal->fast_mode ? ctx->rcb_bufs[task->dec.reg_index] : ctx->rcb_bufs[0]);

    /* set reg -> para (stride, len) */
    {
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;
        RK_U32 mapped_idx;

        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        if (mframe) {
            hor_virstride = mpp_frame_get_hor_stride(mframe);
            ver_virstride = mpp_frame_get_ver_stride(mframe);
            y_virstride = hor_virstride * ver_virstride;
            uv_virstride = hor_virstride * ver_virstride / 2;

            if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
                RK_U32 fbd_offset;
                RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
                RK_U32 h = MPP_ALIGN(mpp_frame_get_height(mframe), 64);

                regs->ctrl_regs.reg9.fbc_e = 1;
                regs->av1d_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
                fbd_offset = regs->av1d_paras.reg68_hor_virstride * h * 4;
                regs->av1d_addrs.reg193_fbc_payload_offset = fbd_offset;
            } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                regs->ctrl_regs.reg9.tile_e = 1;
                regs->av1d_paras.reg68_hor_virstride = MPP_ALIGN(hor_virstride * 6, 16) >> 4;
                regs->av1d_paras.reg70_y_virstride = (y_virstride + uv_virstride) >> 4;
            } else {
                regs->ctrl_regs.reg9.fbc_e = 0;
                regs->av1d_paras.reg68_hor_virstride = hor_virstride >> 4;
                regs->av1d_paras.reg69_raster_uv_hor_virstride = hor_virstride >> 4;
                regs->av1d_paras.reg70_y_virstride = y_virstride >> 4;
            }
            /* error */
            regs->av1d_paras.reg80_error_ref_hor_virstride = regs->av1d_paras.reg68_hor_virstride;
            regs->av1d_paras.reg81_error_ref_raster_uv_hor_virstride = regs->av1d_paras.reg69_raster_uv_hor_virstride;
            regs->av1d_paras.reg82_error_ref_virstride = regs->av1d_paras.reg70_y_virstride;
        }

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_FRAME_PTR, &mframe);
                if (mframe) {
                    hor_virstride = mpp_frame_get_hor_stride(mframe);
                    ver_virstride = mpp_frame_get_ver_stride(mframe);
                    y_virstride = hor_virstride * ver_virstride;
                    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
                        hor_virstride = mpp_frame_get_fbc_hdr_stride(mframe) / 4;
                    } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                        hor_virstride = MPP_ALIGN(hor_virstride * 6, 16);
                        y_virstride += y_virstride / 2;
                    }
                    SET_REF_HOR_VIRSTRIDE(regs->av1d_paras, mapped_idx, hor_virstride >> 4);
                    SET_REF_RASTER_UV_HOR_VIRSTRIDE(regs->av1d_paras, mapped_idx, hor_virstride >> 4);
                    SET_REF_VIRSTRIDE(regs->av1d_paras, mapped_idx, y_virstride >> 4);
                }
            }
        }
    }

    /* set reg -> para (ref, fbc, colmv) */
    {
        MppBuffer mbuffer = NULL;
        RK_U32 mapped_idx;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &mbuffer);
        regs->av1d_addrs.reg168_decout_base = mpp_buffer_get_fd(mbuffer);
        regs->av1d_addrs.reg192_payload_st_cur_base = mpp_buffer_get_fd(mbuffer);

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_BUFFER, &mbuffer);
                if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                    origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->frame_refs[mapped_idx].Index);
                    mbuffer = origin_buf->buf[0];
                }
                if (mbuffer) {
                    SET_REF_BASE(regs->av1d_addrs, mapped_idx, mpp_buffer_get_fd(mbuffer));
                    SET_FBC_PAYLOAD_REF_BASE(regs->av1d_addrs, mapped_idx, mpp_buffer_get_fd(mbuffer));
                }
            }
        }

        HalBuf *mv_buf = NULL;
        vdpu38x_av1d_colmv_setup(p_hal, dxva);
        mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->CurrPic.Index7Bits);
        regs->av1d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_VDPU38X_DATAS
        memset(mpp_buffer_get_ptr(mv_buf->buf[0]), 0, mpp_buffer_get_size(mv_buf->buf[0]));
#endif
        for (i = 0; i < NUM_REF_FRAMES; i++) {
            if (dxva->frame_refs[i].Index != (CHAR)0xff && dxva->frame_refs[i].Index != 0x7f) {
                mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->frame_refs[i].Index);
                regs->av1d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_VDPU38X_DATAS
                {
                    char *cur_fname = "colmv_ref_frame";
                    memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
                    sprintf(vdpu38x_dump_cur_fname_path, "%s/%s%d.dat", vdpu38x_dump_cur_dir, cur_fname, i);
                    vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                                              8 * mpp_buffer_get_size(mv_buf->buf[0]), 64, 0, 0);
                }
#endif
            }
        }
    }

    vdpu38x_av1d_cdf_setup(p_hal, dxva);
    vdpu38x_av1d_set_cdf_segid(p_hal, dxva,
                               &regs->av1d_addrs.reg178_av1_coef_rd_base,
                               &regs->av1d_addrs.reg179_av1_coef_wr_base,
                               &regs->av1d_addrs.reg181_av1_rd_segid_base,
                               &regs->av1d_addrs.reg182_av1_wr_segid_base,
                               &regs->av1d_addrs.reg184_av1_noncoef_rd_base,
                               &regs->av1d_addrs.reg185_av1_noncoef_wr_base);
    mpp_buffer_sync_end(ctx->bufs);

    {
        //scale down config
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        fd = mpp_buffer_get_fd(mbuffer);

        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->common_addr.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->av1d_addrs.reg168_decout_base = fd;
            regs->av1d_addrs.reg192_payload_st_cur_base = fd;
            regs->av1d_addrs.reg169_error_ref_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->av1d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->common_addr.reg133_scale_down_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->av1d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    INP_CHECK(ret, NULL == p_hal);
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }

    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs = p_hal->fast_mode ?
                              reg_ctx->reg_buf[task->dec.reg_index].regs :
                              reg_ctx->regs;
    MppDev dev = p_hal->dev;
    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->ctrl_regs;
        wr_cfg.size = sizeof(regs->ctrl_regs);
        wr_cfg.offset = OFFSET_CTRL_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->av1d_paras;
        wr_cfg.size = sizeof(regs->av1d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->av1d_addrs;
        wr_cfg.size = sizeof(regs->av1d_addrs);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(regs->ctrl_regs.reg15);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu383_set_rcbinfo(dev, (VdpuRcbInfo*)reg_ctx->rcb_buf_info);

        /* send request to hardware */
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *p_regs = p_hal->fast_mode ?
                                reg_ctx->reg_buf[task->dec.reg_index].regs :
                                reg_ctx->regs;
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "colmv_cur_frame.dat";
        DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
        HalBuf *mv_buf = NULL;
        mv_buf = hal_bufs_get_buf(reg_ctx->colmv_bufs, dxva->CurrPic.Index7Bits);
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                                  8 * mpp_buffer_get_size(mv_buf->buf[0]), 64, 0, 0);
    }
    {
        char *cur_fname = "decout.dat";
        MppBuffer mbuffer = NULL;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &mbuffer);
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                                  8 * mpp_buffer_get_size(mbuffer), 128, 0, 0);
    }
#endif

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "cabac_cdf_out.dat";
        HalBuf *cdf_buf = NULL;
        DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        cdf_buf = hal_bufs_get_buf(reg_ctx->cdf_segid_bufs, dxva->CurrPic.Index7Bits);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(cdf_buf->buf[0]),
                                  (NON_COEF_CDF_SIZE + COEF_CDF_SIZE) * 8, 128, 0, 0);
    }
#endif

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        (!p_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
        p_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta) {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

__SKIP_HARD:
    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_av1d_vdpu383 = {
    .name       = "av1d_vdpu383",
    .type       = MPP_CTX_DEC,
    .coding     = MPP_VIDEO_CodingAV1,
    .ctx_size   = sizeof(Vdpu38xAv1dRegCtx) + VDPU383_UNCMPS_HEADER_SIZE,
    .flag       = 0,
    .init       = vdpu383_av1d_init,
    .deinit     = vdpu38x_av1d_deinit,
    .reg_gen    = vdpu383_av1d_gen_regs,
    .start      = vdpu383_av1d_start,
    .wait       = vdpu383_av1d_wait,
    .reset      = vdpu38x_av1d_reset,
    .flush      = vdpu38x_av1d_flush,
    .control    = vdpu38x_av1d_control,
};
