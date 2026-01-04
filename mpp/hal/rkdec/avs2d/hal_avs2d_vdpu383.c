/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_avs2d_vdpu383"

#include <string.h>
#include <stdio.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_debug.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "avs2d_syntax.h"
#include "hal_avs2d_global.h"
#include "hal_avs2d_vdpu383.h"
#include "mpp_dec_cb_param.h"
#include "vdpu_com.h"
#include "vdpu38x_com.h"
#include "vdpu383_com.h"
#include "hal_avs2d_ctx.h"
#include "hal_avs2d_com.h"

#define MAX_REF_NUM                 (8)
#define AVS2_383_SHPH_SIZE          (208)            /* bytes */
#define AVS2_383_SCALIST_SIZE       (80)             /* bytes */

#define AVS2_383_SHPH_ALIGNED_SIZE          (MPP_ALIGN(AVS2_383_SHPH_SIZE, SZ_4K))
#define AVS2_383_SCALIST_ALIGNED_SIZE       (MPP_ALIGN(AVS2_383_SCALIST_SIZE, SZ_4K))
#define AVS2_383_STREAM_INFO_SET_SIZE       (AVS2_383_SHPH_ALIGNED_SIZE + \
                                            AVS2_383_SCALIST_ALIGNED_SIZE)
#define AVS2_ALL_TBL_BUF_SIZE(cnt)          (AVS2_383_STREAM_INFO_SET_SIZE * (cnt))
#define AVS2_SHPH_OFFSET(pos)               (AVS2_383_STREAM_INFO_SET_SIZE * (pos))
#define AVS2_SCALIST_OFFSET(pos)            (AVS2_SHPH_OFFSET(pos) + AVS2_383_SHPH_ALIGNED_SIZE)

#define COLMV_COMPRESS_EN       (1)
#define COLMV_BLOCK_SIZE        (16)
#define COLMV_BYTES             (16)

static MPP_RET vdpu383_avs2d_rcb_calc(void *context, RK_U32 *total_size)
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
    if (ctx->pic_w > 8192)
        cur_bit_size = MPP_DIVUP(64, in_tl_row) * 112;
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_ON_ROW, 142, cur_bit_size);

    /* RCB_INTER_IN_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(8, in_tl_row) * 166;
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_IN_ROW, 144, cur_bit_size);

    /* RCB_INTER_ON_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(8, on_tl_row) * 166;
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
    cur_bit_size = MPP_ROUNDUP(64, in_tl_row) * (1.6 * bit_depth + 0.5)
                   * (12 + 5 * cur_uv_para + 1.5 * ctx->alf_en);
    cur_bit_size = cur_bit_size / 2 + fltd_row_append;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    // save space mode : half for RCB_FLTD_IN_ROW, half for RCB_FLTD_PROT_IN_ROW
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW,  154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.6 * bit_depth + 0.5)
                   * (12 + 5 * cur_uv_para + 1.5 * ctx->alf_en);
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

static MPP_RET fill_registers(Avs2dHalCtx_t *p_hal, Vdpu383RegSet *regs, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    MppFrame mframe = NULL;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    RefParams_Avs2d *refp = &syntax->refp;
    HalDecTask *task_dec  = &task->dec;
    MppBufSlots frm_slots = p_hal->cfg->frame_slots;

    RK_U32 is_fbc = 0;
    RK_U32 is_tile = 0;
    HalBuf *mv_buf = NULL;

    mpp_buf_slot_get_prop(frm_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);
    is_fbc = MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe));
    is_tile = MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe));

    //!< caculate the yuv_frame_size
    {
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;

        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        uv_virstride = hor_virstride * ver_virstride / 2;
        AVS2D_HAL_TRACE("is_fbc %d y_virstride %d, hor_virstride %d, ver_virstride %d\n",
                        is_fbc, y_virstride, hor_virstride, ver_virstride);

        if (is_fbc) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset;

            regs->ctrl_regs.reg9.fbc_e = 1;
            regs->comm_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
            fbd_offset = regs->comm_paras.reg68_hor_virstride * MPP_ALIGN(ver_virstride, 64) * 4;
            regs->comm_addrs.reg193_fbc_payload_offset = fbd_offset;
        } else if (is_tile) {
            regs->ctrl_regs.reg9.tile_e = 1;
            regs->comm_paras.reg68_hor_virstride = hor_virstride * 6 / 16;
            regs->comm_paras.reg70_y_virstride = (y_virstride + uv_virstride) / 16;
        } else {
            regs->ctrl_regs.reg9.fbc_e = 0;
            regs->ctrl_regs.reg9.tile_e = 0;
            regs->comm_paras.reg68_hor_virstride = hor_virstride / 16;
            regs->comm_paras.reg69_raster_uv_hor_virstride = hor_virstride / 16;
            regs->comm_paras.reg70_y_virstride = y_virstride / 16;
        }
    }

    // set current
    {
        RK_S32 fd = hal_avs2d_get_frame_fd(p_hal, task_dec->output);

        mpp_assert(fd >= 0);

        regs->comm_addrs.reg168_decout_base = fd;
        regs->comm_addrs.reg192_payload_st_cur_base = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, task_dec->output);
        regs->comm_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        AVS2D_HAL_TRACE("cur frame index %d, fd %d, colmv fd %d", task_dec->output, fd, regs->comm_addrs.reg216_colmv_cur_base);

        // TODO: set up error_ref_base
        // regs->avs2d_addr.reg169_err_ref_base.base = regs->avs2d_addr.reg216_colmv_cur_base.base;
    }

    // set reference
    {
        RK_S32 valid_slot = -1;

        AVS2D_HAL_TRACE("num of ref %d", refp->ref_pic_num);

        for (i = 0; i < refp->ref_pic_num; i++) {
            if (task_dec->refer[i] < 0)
                continue;

            valid_slot = i;
            break;
        }

        for (i = 0; i < MAX_REF_NUM; i++) {
            if (i < refp->ref_pic_num) {
                MppFrame frame_ref = NULL;

                RK_S32 slot_idx = task_dec->refer[i] < 0 ? task_dec->refer[valid_slot] : task_dec->refer[i];

                if (slot_idx < 0) {
                    AVS2D_HAL_TRACE("missing ref, could not found valid ref");
                    task->dec.flags.ref_err = 1;
                    return ret = MPP_ERR_UNKNOW;
                }

                mpp_buf_slot_get_prop(frm_slots, slot_idx, SLOT_FRAME_PTR, &frame_ref);

                if (frame_ref) {
                    regs->comm_addrs.reg170_185_ref_base[i] = hal_avs2d_get_frame_fd(p_hal, slot_idx);
                    regs->comm_addrs.reg195_210_payload_st_ref_base[i] = hal_avs2d_get_frame_fd(p_hal, slot_idx);
                    mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                    regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
                }
            }
        }

        if (p_hal->syntax.refp.scene_ref_enable && p_hal->syntax.refp.scene_ref_slot_idx >= 0) {
            MppFrame scene_ref = NULL;
            RK_S32 slot_idx = p_hal->syntax.refp.scene_ref_slot_idx;
            RK_S32 replace_idx = p_hal->syntax.refp.scene_ref_replace_pos;

            mpp_buf_slot_get_prop(frm_slots, slot_idx, SLOT_FRAME_PTR, &scene_ref);

            if (scene_ref) {
                regs->comm_addrs.reg170_185_ref_base[replace_idx] = hal_avs2d_get_frame_fd(p_hal, slot_idx);
                regs->comm_addrs.reg195_210_payload_st_ref_base[replace_idx] = regs->comm_addrs.reg170_185_ref_base[replace_idx];
                mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                regs->comm_addrs.reg217_232_colmv_ref_base[replace_idx] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        }

        regs->comm_addrs.reg169_error_ref_base = regs->comm_addrs.reg170_185_ref_base[0];
        regs->comm_addrs.reg194_payload_st_error_ref_base = regs->comm_addrs.reg195_210_payload_st_ref_base[0];
    }

    // set rlc
    regs->comm_addrs.reg128_strm_base = hal_avs2d_get_packet_fd(p_hal, task_dec->input);
    AVS2D_HAL_TRACE("packet fd %d from slot %d", regs->comm_addrs.reg128_strm_base, task_dec->input);

    regs->comm_paras.reg66_stream_len = MPP_ALIGN(mpp_packet_get_length(task_dec->input_packet), 16) + 64;

    {
        //scale down config
        mpp_buf_slot_get_prop(frm_slots, task_dec->output,
                              SLOT_FRAME_PTR, &mframe);
        if (mpp_frame_get_thumbnail_en(mframe)) {
            regs->comm_addrs.reg133_scale_down_base = regs->comm_addrs.reg168_decout_base;
            vdpu383_setup_down_scale(mframe, p_hal->cfg->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
        } else {
            regs->ctrl_regs.reg9.scale_down_en = 0;
        }
    }

    return ret;
}

MPP_RET hal_avs2d_vdpu383_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, loop;
    Avs2dRkvRegCtx *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    mpp_env_get_u32("hal_avs2d_debug", &hal_avs2d_debug, 0);

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);

    p_hal->cfg = cfg;
    cfg->support_fast_mode = 1;
    p_hal->fast_mode = cfg->cfg->base.fast_parse && cfg->support_fast_mode;

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Avs2dRkvRegCtx)));
    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    reg_ctx->shph_dat = mpp_calloc(RK_U8, AVS2_383_SHPH_SIZE);
    reg_ctx->scalist_dat = mpp_calloc(RK_U8, AVS2_383_SCALIST_SIZE);
    loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->cfg->buf_group, &reg_ctx->bufs, AVS2_ALL_TBL_BUF_SIZE(loop)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->cfg->dev);

    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383RegSet, 1);
        vdpu383_init_ctrl_regs(reg_ctx->reg_buf[i].regs, MPP_VIDEO_CodingAVS2);
        reg_ctx->reg_buf[i].offset_shph = AVS2_SHPH_OFFSET(i);
        reg_ctx->reg_buf[i].offset_sclst = AVS2_SCALIST_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->shph_offset = reg_ctx->reg_buf[0].offset_shph;
        reg_ctx->sclst_offset = reg_ctx->reg_buf[0].offset_sclst;
    }

    mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

    vdpu38x_rcb_calc_init((Vdpu38xRcbCtx **)&reg_ctx->rcb_ctx);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    (void)cfg;
    return ret;
__FAILED:
    hal_avs2d_vdpu_deinit(p_hal);
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_vdpu383_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dRkvRegCtx *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Vdpu383RegSet *regs = NULL;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);
    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->cfg->base.disable_error) {
        ret = MPP_NOK;
        goto __RETURN;
    }

    memcpy(&p_hal->syntax, task->dec.syntax.data, sizeof(Avs2dSyntax_t));
    ret = hal_avs2d_set_up_colmv_buf(p_hal);
    if (ret)
        goto __RETURN;

    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;

    if (p_hal->fast_mode) {
        RK_U32 i = 0;

        for (i = 0; i <  MPP_ARRAY_ELEMS(reg_ctx->reg_buf); i++) {
            if (!reg_ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                regs = reg_ctx->reg_buf[i].regs;
                reg_ctx->shph_offset = reg_ctx->reg_buf[i].offset_shph;
                reg_ctx->sclst_offset = reg_ctx->reg_buf[i].offset_sclst;
                reg_ctx->regs = reg_ctx->reg_buf[i].regs;
                reg_ctx->reg_buf[i].valid = 1;
                break;
            }
        }

        mpp_assert(regs);
    }

    regs = reg_ctx->regs;
    memset(regs, 0, sizeof(Vdpu383RegSet));
    vdpu383_init_ctrl_regs(regs, MPP_VIDEO_CodingAVS2);

    hal_avs2d_vdpu38x_prepare_header(p_hal, reg_ctx->shph_dat, AVS2_383_SHPH_SIZE / 8);
    hal_avs2d_vdpu38x_prepare_scalist(p_hal, reg_ctx->scalist_dat, AVS2_383_SCALIST_SIZE / 8);

    ret = fill_registers(p_hal, regs, task);

    if (ret)
        goto __RETURN;

    {
        memcpy(reg_ctx->bufs_ptr + reg_ctx->shph_offset, reg_ctx->shph_dat, AVS2_383_SHPH_SIZE);
        memcpy(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, reg_ctx->scalist_dat, AVS2_383_SCALIST_SIZE);

        regs->comm_addrs.reg131_gbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->cfg->dev, 131, reg_ctx->shph_offset);
        regs->comm_paras.reg67_global_len = AVS2_383_SHPH_SIZE;

        regs->comm_addrs.reg132_scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->cfg->dev, 132, reg_ctx->sclst_offset);
    }

    vdpu38x_avs2d_rcb_setup(p_hal, task, &regs->comm_addrs.rcb_regs,
                            vdpu383_avs2d_rcb_calc);
    vdpu383_setup_statistic(&regs->ctrl_regs);
    mpp_buffer_sync_end(reg_ctx->bufs);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_vdpu383_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Vdpu383RegSet *regs = NULL;
    Avs2dRkvRegCtx *reg_ctx;
    MppDev dev = NULL;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    AVS2D_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == p_hal);

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->cfg->base.disable_error) {
        goto __RETURN;
    }

    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    regs = (p_hal->fast_mode != 0) ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;
    dev = p_hal->cfg->dev;

    p_hal->frame_no++;

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

        if (hal_avs2d_debug & AVS2D_HAL_DBG_REG) {
            memset(reg_ctx->reg_out, 0, sizeof(reg_ctx->reg_out));
            rd_cfg.reg = reg_ctx->reg_out;
            rd_cfg.size = sizeof(reg_ctx->reg_out);
            rd_cfg.offset = 0;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu38x_rcb_set_info(reg_ctx->rcb_ctx, dev);

        // send request to hardware
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }

    } while (0);

__RETURN:
    AVS2D_HAL_TRACE("Out.");
    return ret;
}

MPP_RET hal_avs2d_vdpu383_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx;
    Vdpu383RegSet *regs;

    INP_CHECK(ret, NULL == p_hal);
    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    regs = (p_hal->fast_mode != 0) ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->cfg->base.disable_error) {
        AVS2D_HAL_DBG(AVS2D_HAL_DBG_ERROR, "found task error.\n");
        ret = MPP_NOK;
        goto __RETURN;
    } else {
        ret = mpp_dev_ioctl(p_hal->cfg->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    }

    if (hal_avs2d_debug & AVS2D_HAL_DBG_OUT)
        hal_avs2d_vdpu_dump_yuv(hal, task);

    AVS2D_HAL_TRACE("read irq_status 0x%08x\n", regs->ctrl_regs.reg15);

    if (p_hal->cfg->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)regs;

        if ((!regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
            regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
            regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
            regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
            regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
            regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
            regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        task->dec.flags.ref_info_valid = 0;

        AVS2D_HAL_TRACE("hal frame %d hard_err= %d", p_hal->frame_no, param.hard_err);

        mpp_callback(p_hal->cfg->dec_cb, &param);
    }

    memset(&regs->ctrl_regs.reg15, 0, sizeof(RK_U32));
    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

const MppHalApi hal_avs2d_vdpu383 = {
    .name     = "avs2d_vdpu383",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVS2,
    .ctx_size = sizeof(Avs2dHalCtx_t),
    .flag     = 0,
    .init     = hal_avs2d_vdpu383_init,
    .deinit   = hal_avs2d_vdpu_deinit,
    .reg_gen  = hal_avs2d_vdpu383_gen_regs,
    .start    = hal_avs2d_vdpu383_start,
    .wait     = hal_avs2d_vdpu383_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
    .client   = VPU_CLIENT_RKVDEC,
    .soc_type = {
        ROCKCHIP_SOC_RK3576,
        ROCKCHIP_SOC_BUTT
    },
};

MPP_DEC_HAL_API_REGISTER(hal_avs2d_vdpu383)
