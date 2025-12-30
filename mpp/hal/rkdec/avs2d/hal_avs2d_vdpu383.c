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

static void init_ctrl_regs(Vdpu383RegSet *regs)
{
    Vdpu383CtrlReg *ctrl_regs = &regs->ctrl_regs;

    ctrl_regs->reg8_dec_mode = 3;  // AVS2
    ctrl_regs->reg9.buf_empty_en = 1;

    ctrl_regs->reg10.strmd_auto_gating_e      = 1;
    ctrl_regs->reg10.inter_auto_gating_e      = 1;
    ctrl_regs->reg10.intra_auto_gating_e      = 1;
    ctrl_regs->reg10.transd_auto_gating_e     = 1;
    ctrl_regs->reg10.recon_auto_gating_e      = 1;
    ctrl_regs->reg10.filterd_auto_gating_e    = 1;
    ctrl_regs->reg10.bus_auto_gating_e        = 1;
    ctrl_regs->reg10.ctrl_auto_gating_e       = 1;
    ctrl_regs->reg10.rcb_auto_gating_e        = 1;
    ctrl_regs->reg10.err_prc_auto_gating_e    = 1;

    ctrl_regs->reg13_core_timeout_threshold = 0xffffff;

    ctrl_regs->reg16.error_proc_disable = 1;
    ctrl_regs->reg16.error_spread_disable = 0;
    ctrl_regs->reg16.roi_error_ctu_cal_en = 0;

    ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffff;
    ctrl_regs->reg21_cabac_error_en_highbits = 0x3fffffff;

    /* performance */
    ctrl_regs->reg28.axi_perf_work_e = 1;
    ctrl_regs->reg28.axi_cnt_type = 1;
    ctrl_regs->reg28.rd_latency_id = 0xb;
    ctrl_regs->reg28.rd_latency_thr = 0;

    ctrl_regs->reg29.addr_align_type = 2;
    ctrl_regs->reg29.ar_cnt_id_type = 0;
    ctrl_regs->reg29.aw_cnt_id_type = 0;
    ctrl_regs->reg29.ar_count_id = 0xa;
    ctrl_regs->reg29.aw_count_id = 0;
    ctrl_regs->reg29.rd_band_width_mode = 0;
}

static void avs2d_refine_rcb_size(VdpuRcbInfo *rcb_info,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    (void) height;
    Avs2dSyntax_t *syntax = dxva;
    RK_U8 ctu_size = 1 << syntax->pp.lcu_size;
    RK_U8 bit_depth = syntax->pp.bit_depth_chroma_minus8 + 8;
    RK_U32 rcb_bits = 0;
    RK_U32 filterd_row_append = 8192;

    width = MPP_ALIGN(width, ctu_size);

    /* RCB_STRMD_IN_ROW && RCB_STRMD_ON_ROW*/
    if (width > 8192)
        rcb_bits = ((width + 63) / 64) * 112;
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_ON_ROW].size = 0;

    /* RCB_INTER_IN_ROW && RCB_INTER_ON_ROW*/
    rcb_bits = ((width + 7) / 8) * 166;
    rcb_info[RCB_INTER_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTER_ON_ROW].size = 0;

    /* RCB_INTRA_IN_ROW && RCB_INTRA_ON_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 3; //TODO:
    rcb_info[RCB_INTRA_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTRA_ON_ROW].size = 0;

    /* RCB_FLTD_IN_ROW && RCB_FLTD_ON_ROW*/
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_bits = MPP_ALIGN(width, 64) * (30 * bit_depth + 9);
    rcb_info[RCB_FLTD_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FLTD_PROT_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FLTD_ON_ROW].size = 0;

    /* RCB_FLTD_ON_COL */
    rcb_info[RCB_FLTD_ON_COL].size = 0;
}

static void hal_avs2d_rcb_info_update(void *hal, Vdpu383RegSet *regs)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    RK_S32 width = p_hal->syntax.pp.pic_width_in_luma_samples;
    RK_S32 height = p_hal->syntax.pp.pic_height_in_luma_samples;
    RK_S32 i = 0;
    RK_S32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    (void) regs;

    reg_ctx->rcb_buf_size = vdpu383_get_rcb_buf_size(reg_ctx->rcb_info, width, height);
    avs2d_refine_rcb_size(reg_ctx->rcb_info, width, height, (void *)&p_hal->syntax);

    for (i = 0; i < loop; i++) {
        MppBuffer rcb_buf = NULL;

        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }

        ret = mpp_buffer_get(p_hal->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        if (ret)
            mpp_err_f("AVS2D mpp_buffer_group_get failed\n");

        reg_ctx->rcb_buf[i] = rcb_buf;
    }
}

static MPP_RET fill_registers(Avs2dHalCtx_t *p_hal, Vdpu383RegSet *regs, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    MppFrame mframe = NULL;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    RefParams_Avs2d *refp = &syntax->refp;
    HalDecTask *task_dec  = &task->dec;

    RK_U32 is_fbc = 0;
    RK_U32 is_tile = 0;
    HalBuf *mv_buf = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);
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

                mpp_buf_slot_get_prop(p_hal->frame_slots, slot_idx, SLOT_FRAME_PTR, &frame_ref);

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

            mpp_buf_slot_get_prop(p_hal->frame_slots, slot_idx, SLOT_FRAME_PTR, &scene_ref);

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
        mpp_buf_slot_get_prop(p_hal->frame_slots, task_dec->output,
                              SLOT_FRAME_PTR, &mframe);
        if (mpp_frame_get_thumbnail_en(mframe)) {
            regs->comm_addrs.reg133_scale_down_base = regs->comm_addrs.reg168_decout_base;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
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

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Avs2dRkvRegCtx)));
    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    reg_ctx->shph_dat = mpp_calloc(RK_U8, AVS2_383_SHPH_SIZE);
    reg_ctx->scalist_dat = mpp_calloc(RK_U8, AVS2_383_SCALIST_SIZE);
    loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, AVS2_ALL_TBL_BUF_SIZE(loop)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);

    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383RegSet, 1);
        init_ctrl_regs(reg_ctx->reg_buf[i].regs);
        reg_ctx->reg_buf[i].offset_shph = AVS2_SHPH_OFFSET(i);
        reg_ctx->reg_buf[i].offset_sclst = AVS2_SCALIST_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->shph_offset = reg_ctx->reg_buf[0].offset_shph;
        reg_ctx->sclst_offset = reg_ctx->reg_buf[0].offset_sclst;
    }

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

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
        !p_hal->cfg->base.disable_error) {
        ret = MPP_NOK;
        goto __RETURN;
    }

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
    init_ctrl_regs(regs);

    hal_avs2d_vdpu38x_prepare_header(p_hal, reg_ctx->shph_dat, AVS2_383_SHPH_SIZE / 8);
    hal_avs2d_vdpu38x_prepare_scalist(p_hal, reg_ctx->scalist_dat, AVS2_383_SCALIST_SIZE / 8);

    ret = fill_registers(p_hal, regs, task);

    if (ret)
        goto __RETURN;

    {
        memcpy(reg_ctx->bufs_ptr + reg_ctx->shph_offset, reg_ctx->shph_dat, AVS2_383_SHPH_SIZE);
        memcpy(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, reg_ctx->scalist_dat, AVS2_383_SCALIST_SIZE);

        regs->comm_addrs.reg131_gbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 131, reg_ctx->shph_offset);
        regs->comm_paras.reg67_global_len = AVS2_383_SHPH_SIZE;

        regs->comm_addrs.reg132_scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 132, reg_ctx->sclst_offset);
    }

    // set rcb
    {
        hal_avs2d_rcb_info_update(p_hal, regs);
        vdpu383_setup_rcb(&regs->comm_addrs, p_hal->dev, p_hal->fast_mode ?
                          reg_ctx->rcb_buf[task->dec.reg_index] : reg_ctx->rcb_buf[0],
                          reg_ctx->rcb_info);

    }

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
        !p_hal->cfg->base.disable_error) {
        goto __RETURN;
    }

    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    regs = p_hal->fast_mode ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;
    dev = p_hal->dev;

    p_hal->frame_no++;

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

        wr_cfg.reg = &regs->comm_paras;
        wr_cfg.size = sizeof(regs->comm_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->comm_addrs;
        wr_cfg.size = sizeof(regs->comm_addrs);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
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

        if (avs2d_hal_debug & AVS2D_HAL_DBG_REG) {
            memset(reg_ctx->reg_out, 0, sizeof(reg_ctx->reg_out));
            rd_cfg.reg = reg_ctx->reg_out;
            rd_cfg.size = sizeof(reg_ctx->reg_out);
            rd_cfg.offset = 0;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu383_set_rcbinfo(dev, (VdpuRcbInfo*)reg_ctx->rcb_info);

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
    regs = p_hal->fast_mode ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->base.disable_error) {
        AVS2D_HAL_DBG(AVS2D_HAL_DBG_ERROR, "found task error.\n");
        ret = MPP_NOK;
        goto __RETURN;
    } else {
        ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret)
            mpp_err_f("poll cmd failed %d\n", ret);
    }

    if (avs2d_hal_debug & AVS2D_HAL_DBG_OUT)
        hal_avs2d_vdpu_dump_yuv(hal, task);

    AVS2D_HAL_TRACE("read irq_status 0x%08x\n", regs->ctrl_regs.reg15);

    if (p_hal->dec_cb) {
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

        mpp_callback(p_hal->dec_cb, &param);
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
    .ctx_size = sizeof(Avs2dRkvRegCtx),
    .flag     = 0,
    .init     = hal_avs2d_vdpu383_init,
    .deinit   = hal_avs2d_vdpu_deinit,
    .reg_gen  = hal_avs2d_vdpu383_gen_regs,
    .start    = hal_avs2d_vdpu383_start,
    .wait     = hal_avs2d_vdpu383_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
};
