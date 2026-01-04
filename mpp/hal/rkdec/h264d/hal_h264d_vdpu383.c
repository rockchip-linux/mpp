/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h264d_vdpu383"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "hal_h264d_global.h"
#include "hal_h264d_vdpu383.h"
#include "mpp_dec_cb_param.h"
#include "vdpu_com.h"
#include "vdpu38x_com.h"
#include "vdpu383_com.h"
#include "hal_h264d_ctx.h"
#include "hal_h264d_com.h"

/* Number registers for the decoder */
#define DEC_VDPU383_REGISTERS               276

#define VDPU383_CABAC_TAB_SIZE              (928*4 + 128)       /* bytes */
#define VDPU383_SPSPPS_SIZE                 (168 + 128)     /* bytes */
#define VDPU383_RPS_SIZE                    (128 + 128 + 128)   /* bytes */
#define VDPU383_SCALING_LIST_SIZE           (6*16+2*64 + 128)   /* bytes */
#define VDPU383_ERROR_INFO_SIZE             (256*144*4)         /* bytes */

#define VDPU383_CABAC_TAB_ALIGNED_SIZE      (MPP_ALIGN(VDPU383_CABAC_TAB_SIZE, SZ_4K))
#define VDPU383_ERROR_INFO_ALIGNED_SIZE     (0)
#define VDPU383_SPSPPS_ALIGNED_SIZE         (MPP_ALIGN(VDPU383_SPSPPS_SIZE, SZ_4K))
#define VDPU383_RPS_ALIGNED_SIZE            (MPP_ALIGN(VDPU383_RPS_SIZE, SZ_4K))
#define VDPU383_SCALING_LIST_ALIGNED_SIZE   (MPP_ALIGN(VDPU383_SCALING_LIST_SIZE, SZ_4K))
#define VDPU383_STREAM_INFO_SET_SIZE        (VDPU383_SPSPPS_ALIGNED_SIZE + \
                                             VDPU383_RPS_ALIGNED_SIZE + \
                                             VDPU383_SCALING_LIST_ALIGNED_SIZE)

#define VDPU383_CABAC_TAB_OFFSET            (0)
#define VDPU383_ERROR_INFO_OFFSET           (VDPU383_CABAC_TAB_OFFSET + VDPU383_CABAC_TAB_ALIGNED_SIZE)
#define VDPU383_STREAM_INFO_OFFSET_BASE     (VDPU383_ERROR_INFO_OFFSET + VDPU383_ERROR_INFO_ALIGNED_SIZE)
#define VDPU383_SPSPPS_OFFSET(pos)          (VDPU383_STREAM_INFO_OFFSET_BASE + (VDPU383_STREAM_INFO_SET_SIZE * pos))
#define VDPU383_RPS_OFFSET(pos)             (VDPU383_SPSPPS_OFFSET(pos) + VDPU383_SPSPPS_ALIGNED_SIZE)
#define VDPU383_SCALING_LIST_OFFSET(pos)    (VDPU383_RPS_OFFSET(pos) + VDPU383_RPS_ALIGNED_SIZE)
#define VDPU383_INFO_BUFFER_SIZE(cnt)       (VDPU383_STREAM_INFO_OFFSET_BASE + (VDPU383_STREAM_INFO_SET_SIZE * cnt))

#define VDPU383_SPS_PPS_LEN            (MPP_ALIGN(1338, 128) / 8) // byte, 1338 bit

static MPP_RET prepare_framerps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    RK_U32 max_frame_num = 0;
    RK_U16 frame_num_wrap = 0;
    RK_U32 tmp = 0;

    BitputCtx_t bp;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;

    mpp_set_bitput_ctx(&bp, data, len);

    max_frame_num = 1 << (pp->log2_max_frame_num_minus4 + 4);
    for (i = 0; i < 16; i++) {
        if ((pp->NonExistingFrameFlags >> i) & 0x01) {
            frame_num_wrap = 0;
        } else {
            if (pp->RefFrameList[i].AssociatedFlag) {
                frame_num_wrap = pp->FrameNumList[i];
            } else {
                frame_num_wrap = (pp->FrameNumList[i] > pp->frame_num) ?
                                 (pp->FrameNumList[i] - max_frame_num) : pp->FrameNumList[i];
            }
        }
        mpp_put_bits(&bp, frame_num_wrap, 16);
    }

    tmp = 0;
    for (i = 0; i < 16; i++) {
        tmp |= (RK_U32)pp->RefPicLayerIdList[i] << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);

    for (i = 0; i < 32; i++) {
        tmp = 0;
        dpb_valid = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;

        tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
        tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
        if (dpb_valid)
            tmp |= (RK_U32)(voidx & 0x1) << 6;
        mpp_put_bits(&bp, tmp, 7);
    }

    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            tmp = 0;
            dpb_valid = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;
            tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
            tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
            if (dpb_valid)
                tmp |= (RK_U32)(voidx & 0x1) << 6;
            mpp_put_bits(&bp, tmp, 7);
        }
    }

    mpp_put_align(&bp, 128, 0);

#ifdef DUMP_VDPU383_DATAS
    {
        char *cur_fname = "rps.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)bp.pbuf, 64 * bp.index + bp.bitpos, 64, 0);
    }
#endif

    return MPP_OK;
}

static MPP_RET set_registers(H264dHalCtx_t *p_hal, Vdpu383RegSet *regs, HalTaskInfo *task)
{
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    HalBuf *mv_buf = NULL;
    HalBuf *origin_buf = NULL;
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    MppHalCfg *cfg = p_hal->cfg;

    // memset(regs, 0, sizeof(Vdpu383RegSet));
    regs->comm_paras.reg66_stream_len = p_hal->strm_len;

    //!< caculate the yuv_frame_size
    {
        MppFrame mframe = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;

        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        uv_virstride = hor_virstride * ver_virstride / 2;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset;

            fbd_offset = fbc_hdr_stride * MPP_ALIGN(ver_virstride, 64) / 16;

            regs->ctrl_regs.reg9.fbc_e = 1;
            regs->comm_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
            regs->comm_addrs.reg193_fbc_payload_offset = fbd_offset;
        } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
            regs->ctrl_regs.reg9.tile_e = 1;
            regs->comm_paras.reg68_hor_virstride = hor_virstride * 6 / 16;
            regs->comm_paras.reg70_y_virstride = (y_virstride + uv_virstride) / 16;
        } else {
            regs->ctrl_regs.reg9.fbc_e = 0;
            regs->comm_paras.reg68_hor_virstride = hor_virstride / 16;
            regs->comm_paras.reg69_raster_uv_hor_virstride = hor_virstride / 16;
            regs->comm_paras.reg70_y_virstride = y_virstride / 16;
        }
    }
    //!< set current
    {
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;

        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        regs->comm_addrs.reg168_decout_base = fd;
        regs->comm_addrs.reg192_payload_st_cur_base = fd;

        //colmv_cur_base
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, pp->CurrPic.Index7Bits);
        regs->comm_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        regs->comm_addrs.reg169_error_ref_base = fd;
    }
    //!< set reference
    {
        RK_S32 i = 0;
        RK_S32 fd = -1;
        RK_S32 ref_index = -1;
        RK_S32 near_index = -1;
        MppBuffer mbuffer = NULL;
        RK_U32 min_frame_num  = 0;
        MppFrame mframe = NULL;

        for (i = 0; i < 15; i++) {
            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                ref_index = pp->RefFrameList[i].Index7Bits;
                near_index = pp->RefFrameList[i].Index7Bits;
            } else {
                ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
            }
            /* mark 3 to differ from current frame */
            mpp_buf_slot_get_prop(cfg->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
            mpp_buf_slot_get_prop(cfg->frame_slots, ref_index, SLOT_FRAME_PTR, &mframe);
            if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
                mbuffer = origin_buf->buf[0];
            }

            if (pp->FrameNumList[i] < pp->frame_num &&
                pp->FrameNumList[i] > min_frame_num &&
                (!mpp_frame_get_errinfo(mframe))) {
                min_frame_num = pp->FrameNumList[i];
                regs->comm_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
            }

            fd = mpp_buffer_get_fd(mbuffer);
            regs->comm_addrs.reg170_185_ref_base[i] = fd;
            regs->comm_addrs.reg195_210_payload_st_ref_base[i] = fd;
            mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
            regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }

        if (pp->RefFrameList[15].bPicEntry != 0xff) {
            ref_index = pp->RefFrameList[15].Index7Bits;
        } else {
            ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
        }

        mpp_buf_slot_get_prop(cfg->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        if (mpp_frame_get_thumbnail_en(mframe) == 2) {
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
        }
        regs->comm_addrs.reg170_185_ref_base[15] = fd;
        regs->comm_addrs.reg195_210_payload_st_ref_base[15] = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
        regs->comm_addrs.reg217_232_colmv_ref_base[15] = mpp_buffer_get_fd(mv_buf->buf[0]);
    }
    {
        MppBuffer mbuffer = NULL;
        Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;

        mpp_buf_slot_get_prop(cfg->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->comm_addrs.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        // regs->comm_paras.reg65_strm_start_bit = 2 * 8;
#ifdef DUMP_VDPU383_DATAS
        {
            char *cur_fname = "stream_in.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                              8 * p_hal->strm_len, 128, 0);
        }
#endif

        regs->comm_addrs.reg130_cabactbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(cfg->dev, 130, reg_ctx->offset_cabac);
    }

    {
        //scale down config
        MppFrame mframe = NULL;
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(cfg->frame_slots, pp->CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        fd = mpp_buffer_get_fd(mbuffer);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->comm_addrs.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, pp->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->comm_addrs.reg168_decout_base = fd;
            regs->comm_addrs.reg192_payload_st_cur_base = fd;
            regs->comm_addrs.reg169_error_ref_base = fd;
            vdpu383_setup_down_scale(mframe, cfg->dev, &regs->ctrl_regs, (void*)&regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->comm_addrs.reg133_scale_down_base = fd;
            vdpu383_setup_down_scale(mframe, cfg->dev, &regs->ctrl_regs, (void*)&regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return MPP_OK;
}

MPP_RET vdpu383_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    mpp_env_get_u32("hal_h264d_debug", &hal_h264d_debug, 0);

    INP_CHECK(ret, NULL == p_hal);

    p_hal->cfg = cfg;
    cfg->support_fast_mode = 1;
    p_hal->fast_mode = cfg->cfg->base.fast_parse && cfg->support_fast_mode;

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Vdpu3xxH264dRegCtx)));
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    RK_U32 max_cnt = (p_hal->fast_mode != 0) ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;

    //!< malloc buffers
    reg_ctx->spspps = mpp_calloc(RK_U8, VDPU383_SPS_PPS_LEN);
    reg_ctx->rps = mpp_calloc(RK_U8, VDPU383_RPS_SIZE);
    reg_ctx->sclst = mpp_calloc(RK_U8, VDPU383_SCALING_LIST_SIZE);
    FUN_CHECK(ret = mpp_buffer_get(cfg->buf_group, &reg_ctx->bufs,
                                   VDPU383_INFO_BUFFER_SIZE(max_cnt)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    reg_ctx->offset_cabac = VDPU383_CABAC_TAB_OFFSET;
    reg_ctx->offset_errinfo = VDPU383_ERROR_INFO_OFFSET;
    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383RegSet, 1);
        vdpu383_init_ctrl_regs(reg_ctx->reg_buf[i].regs, MPP_VIDEO_CodingAVC);
        reg_ctx->offset_spspps[i] = VDPU383_SPSPPS_OFFSET(i);
        reg_ctx->offset_rps[i] = VDPU383_RPS_OFFSET(i);
        reg_ctx->offset_sclst[i] = VDPU383_SCALING_LIST_OFFSET(i);
    }

    mpp_buffer_attach_dev(reg_ctx->bufs, cfg->dev);

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->rps_offset = reg_ctx->offset_rps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    //!< copy cabac table bytes
    memcpy((char *)reg_ctx->bufs_ptr + reg_ctx->offset_cabac,
           (void *)rkv_cabac_table, sizeof(rkv_cabac_table));

    mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu38x_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

    vdpu38x_rcb_calc_init((Vdpu38xRcbCtx **)&reg_ctx->rcb_ctx);

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu3xx_h264d_deinit(hal);

    return ret;
}

static MPP_RET vdpu383_h264d_rcb_calc(void *context, RK_U32 *total_size)
{
    Vdpu38xRcbCtx *ctx = (Vdpu38xRcbCtx *)context;
    RK_FLOAT cur_bit_size = 0;
    RK_U32 cur_uv_para = 0;
    RK_U32 bit_depth = ctx->bit_depth;
    RK_U32 in_tl_row = 0;
    RK_U32 on_tl_row = 0;
    RK_U32 on_tl_col = 0;
    Vdpu38xFmt rcb_fmt;
    RK_U32 fltd_row_append = ctx->pic_w > 4096 ? 256 * 16 * 8 : 864 * 16 * 8;

    /* vdpu383/vdpu384a/vdpu384b fix 10bit */
    bit_depth = 10;

    vdpu38x_rcb_get_len(ctx, VDPU38X_RCB_IN_TILE_ROW, &in_tl_row);
    vdpu38x_rcb_get_len(ctx, VDPU38X_RCB_ON_TILE_ROW, &on_tl_row);
    vdpu38x_rcb_get_len(ctx, VDPU38X_RCB_ON_TILE_COL, &on_tl_col);
    rcb_fmt = vdpu38x_rcb_get_fmt(ctx);

    /* RCB_STRMD_IN_ROW */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_IN_ROW, 140, cur_bit_size);

    /* RCB_STRMD_ON_ROW */
    cur_bit_size = 0;
    /*
     * For all spec, the hardware connects all in-tile rows of strmd to the on-tile.
     * Therefore, only strmd on-tile needs to be configured, and there is no need to
     * configure strmd in-tile.
     *
     * Versions with issues: rk3576(383), swan1126b (384a), shark/robin (384b).
     */
    if (ctx->pic_w > 4096)
        cur_bit_size = MPP_DIVUP(16, in_tl_row) * 154 * (1 + ctx->mbaff_flag);
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_ON_ROW, 142, cur_bit_size);

    /* RCB_INTER_IN_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(4, in_tl_row) * 92 * (1 + ctx->mbaff_flag);
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_IN_ROW, 144, cur_bit_size);

    /* RCB_INTER_ON_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(4, on_tl_row) * 92 * (1 + ctx->mbaff_flag);
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_ON_ROW, 146, cur_bit_size);

    /* RCB_INTRA_IN_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_intra_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(512, (in_tl_row * (bit_depth + 2)
                                     * (1 + ctx->mbaff_flag) * cur_uv_para));
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTRA_IN_ROW, 148, cur_bit_size);

    /* RCB_INTRA_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_intra_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(512, (on_tl_row * (bit_depth + 2)
                                     * (1 + ctx->mbaff_flag) * cur_uv_para));
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTRA_ON_ROW, 150, cur_bit_size);

    /* RCB_FLTD_IN_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(16, in_tl_row) * (1.6 * bit_depth + 0.5)
                   * (( 6 + 3 * cur_uv_para) * (1 + ctx->mbaff_flag)
                      + 2 * cur_uv_para + 1.5);
    cur_bit_size = cur_bit_size / 2 + fltd_row_append;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    // save space mode : half for RCB_FLTD_IN_ROW, half for RCB_FLTD_PROT_IN_ROW
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW, 154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(16, on_tl_row) * (1.6 * bit_depth + 0.5)
                   * (( 6 + 3 * cur_uv_para) * (1 + ctx->mbaff_flag)
                      + 2 * cur_uv_para + 1.5);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_ROW, 156, cur_bit_size);

    /* RCB_FLTD_ON_COL */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_COL, 158, cur_bit_size);

    /* RCB_FLTD_UPSC_ON_COL */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_UPSC_ON_COL, 160, cur_bit_size);

    *total_size = vdpu38x_rcb_get_total_size(ctx);

    return MPP_OK;
}

MPP_RET vdpu383_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu383RegSet *regs = ctx->regs;
    MppHalCfg *cfg = p_hal->cfg;
    RK_S32 width, height, mv_size;
    MppFrame mframe;

    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !cfg->cfg->base.disable_error)) {
        goto __RETURN;
    }

    hal_h264d_explain_input_buffer(hal, &task->dec);

    width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);
    mv_size = MPP_ALIGN(width, 64) * MPP_ALIGN(height, 16); // 16 byte unit

    /* if is field mode is enabled enlarge colmv buffer and disable colmv compression */
    if (!p_hal->pp->frame_mbs_only_flag)
        mv_size *= 2;

    if (p_hal->cmv_bufs == NULL || p_hal->mv_size < mv_size) {
        size_t size = mv_size;

        if (p_hal->cmv_bufs) {
            hal_bufs_deinit(p_hal->cmv_bufs);
            p_hal->cmv_bufs = NULL;
        }

        hal_bufs_init(&p_hal->cmv_bufs);
        if (p_hal->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            goto __RETURN;
        }
        p_hal->mv_size = mv_size;
        p_hal->mv_count = mpp_buf_slot_get_count(cfg->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

    mpp_buf_slot_get_prop(cfg->frame_slots, p_hal->pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        ctx->origin_bufs == NULL) {
        vdpu38x_setup_scale_origin_bufs(mframe, &ctx->origin_bufs,
                                        mpp_buf_slot_get_count(cfg->frame_slots));
    }

    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                regs = ctx->reg_buf[i].regs;

                ctx->spspps_offset = ctx->offset_spspps[i];
                ctx->rps_offset = ctx->offset_rps[i];
                ctx->sclst_offset = ctx->offset_sclst[i];
                ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

#ifdef DUMP_VDPU383_DATAS
    {
        memset(dump_cur_dir, 0, sizeof(dump_cur_dir));
        sprintf(dump_cur_dir, "avc/Frame%04d", dump_cur_frame);
        if (access(dump_cur_dir, 0)) {
            if (mkdir(dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", dump_cur_dir);
        }
        dump_cur_frame++;
    }
#endif

    vdpu38x_h264d_prepare_spspps(p_hal, (RK_U64 *)ctx->spspps, VDPU383_SPS_PPS_LEN / 8);
    prepare_framerps(p_hal, (RK_U64 *)ctx->rps, VDPU383_RPS_SIZE / 8);
    vdpu38x_h264d_prepare_scanlist(p_hal, ctx->sclst, VDPU383_SCALING_LIST_SIZE);
    set_registers(p_hal, regs, task);

    //!< copy spspps datas
    memcpy((char *)ctx->bufs_ptr + ctx->spspps_offset, (char *)ctx->spspps, VDPU383_SPS_PPS_LEN);

    regs->comm_addrs.reg131_gbl_base = ctx->bufs_fd;
    regs->comm_paras.reg67_global_len = VDPU383_SPS_PPS_LEN / 16; // 128 bit as unit
    mpp_dev_set_reg_offset(cfg->dev, 131, ctx->spspps_offset);

    memcpy((char *)ctx->bufs_ptr + ctx->rps_offset, (void *)ctx->rps, VDPU383_RPS_SIZE);
    regs->comm_addrs.reg129_rps_base = ctx->bufs_fd;
    mpp_dev_set_reg_offset(cfg->dev, 129, ctx->rps_offset);

    if (p_hal->pp->scaleing_list_enable_flag) {
        memcpy((char *)ctx->bufs_ptr + ctx->sclst_offset, (void *)ctx->sclst, VDPU383_SCALING_LIST_SIZE);
        regs->comm_addrs.reg132_scanlist_addr = ctx->bufs_fd;
        mpp_dev_set_reg_offset(cfg->dev, 132, ctx->sclst_offset);
    } else {
        regs->comm_addrs.reg132_scanlist_addr = 0;
    }

    vdpu38x_h264d_rcb_setup(p_hal, task, &regs->comm_addrs.rcb_regs,
                            vdpu383_h264d_rcb_calc);
    vdpu383_setup_statistic(&regs->ctrl_regs);
    mpp_buffer_sync_end(ctx->bufs);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->cfg->base.disable_error)) {
        goto __RETURN;
    }

    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu383RegSet *regs = (p_hal->fast_mode != 0) ?
                          reg_ctx->reg_buf[task->dec.reg_index].regs :
                          reg_ctx->regs;
    MppDev dev = p_hal->cfg->dev;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->ctrl_regs;
        wr_cfg.size = sizeof(regs->ctrl_regs);
        wr_cfg.offset = VDPU38X_OFF_CTRL_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->comm_paras;
        wr_cfg.size = sizeof(regs->comm_paras);
        wr_cfg.offset = VDPU38X_OFF_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->comm_addrs;
        wr_cfg.size = sizeof(regs->comm_addrs);
        wr_cfg.offset = VDPU38X_OFF_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(regs->ctrl_regs.reg15);
        rd_cfg.offset = VDPU38X_OFF_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu38x_rcb_set_info(reg_ctx->rcb_ctx, dev);

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

MPP_RET vdpu383_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu383RegSet *p_regs = (p_hal->fast_mode != 0) ?
                            reg_ctx->reg_buf[task->dec.reg_index].regs :
                            reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->cfg->base.disable_error)) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->cfg->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->cfg->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_regs;

        if ((!p_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
            p_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
            p_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        mpp_callback(p_hal->cfg->dec_cb, &param);
    }
    memset(&p_regs->ctrl_regs.reg19, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_h264d_vdpu383 = {
    .name     = "h264d_vdpu383",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(H264dHalCtx_t),
    .flag     = 0,
    .init     = vdpu383_h264d_init,
    .deinit   = vdpu3xx_h264d_deinit,
    .reg_gen  = vdpu383_h264d_gen_regs,
    .start    = vdpu383_h264d_start,
    .wait     = vdpu383_h264d_wait,
    .reset    = vdpu_h264d_reset,
    .flush    = vdpu_h264d_flush,
    .control  = vdpu38x_h264d_control,
    .client   = VPU_CLIENT_RKVDEC,
    .soc_type = {
        ROCKCHIP_SOC_RK3576,
        ROCKCHIP_SOC_BUTT
    },
};

MPP_DEC_HAL_API_REGISTER(hal_h264d_vdpu383)
