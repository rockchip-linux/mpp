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
#include "vdpu383_com.h"
#include "vdpu383_avs2d.h"
#include "hal_avs2d_global.h"
#include "hal_avs2d_vdpu383.h"
#include "mpp_dec_cb_param.h"

#define VDPU383_FAST_REG_SET_CNT    (3)
#define MAX_REF_NUM                 (8)
#define AVS2_383_SHPH_SIZE          (208)            /* bytes */
#define AVS2_383_SCALIST_SIZE       (80)             /* bytes */
#define VDPU34x_TOTAL_REG_CNT       (278)

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

typedef struct avs2d_buf_t {
    RK_U32                  valid;
    RK_U32                  offset_shph;
    RK_U32                  offset_sclst;
    Vdpu383Avs2dRegSet      *regs;
} Avs2dRkvBuf_t;

typedef struct avs2d_reg_ctx_t {
    Avs2dRkvBuf_t           reg_buf[VDPU383_FAST_REG_SET_CNT];

    RK_U32                  shph_offset;
    RK_U32                  sclst_offset;

    Vdpu383Avs2dRegSet      *regs;

    RK_U8                   shph_dat[AVS2_383_SHPH_SIZE];
    RK_U8                   scalist_dat[AVS2_383_SCALIST_SIZE];

    MppBuffer               bufs;
    RK_S32                  bufs_fd;
    void                    *bufs_ptr;

    MppBuffer               rcb_buf[VDPU383_FAST_REG_SET_CNT];
    RK_S32                  rcb_buf_size;
    Vdpu383RcbInfo          rcb_info[RCB_BUF_COUNT];
    RK_U32                  reg_out[VDPU34x_TOTAL_REG_CNT];

} Avs2dRkvRegCtx_t;

MPP_RET hal_avs2d_vdpu383_deinit(void *hal);
static RK_U32 avs2d_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static RK_U32 avs2d_len_align(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 16));
}

static MPP_RET prepare_header(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    RK_U32 i, j;
    BitputCtx_t bp;
    RK_U64 *bit_buf = (RK_U64 *)data;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    AlfParams_Avs2d *alfp = &syntax->alfp;
    RefParams_Avs2d *refp = &syntax->refp;
    WqmParams_Avs2d *wqmp = &syntax->wqmp;

    memset(data, 0, len);

    mpp_set_bitput_ctx(&bp, bit_buf, len);

    //!< sequence header syntax
    mpp_put_bits(&bp, pp->chroma_format_idc, 2);
    mpp_put_bits(&bp, pp->pic_width_in_luma_samples, 16);
    mpp_put_bits(&bp, pp->pic_height_in_luma_samples, 16);
    mpp_put_bits(&bp, pp->bit_depth_luma_minus8, 3);
    mpp_put_bits(&bp, pp->bit_depth_chroma_minus8, 3);
    mpp_put_bits(&bp, pp->lcu_size, 3);
    mpp_put_bits(&bp, pp->progressive_sequence, 1);
    mpp_put_bits(&bp, pp->field_coded_sequence, 1);

    mpp_put_bits(&bp, pp->secondary_transform_enable_flag, 1);
    mpp_put_bits(&bp, pp->sample_adaptive_offset_enable_flag, 1);
    mpp_put_bits(&bp, pp->adaptive_loop_filter_enable_flag, 1);
    mpp_put_bits(&bp, pp->pmvr_enable_flag, 1);
    mpp_put_bits(&bp, pp->cross_slice_loopfilter_enable_flag, 1);

    //!< picture header syntax
    mpp_put_bits(&bp, pp->picture_type, 3);
    mpp_put_bits(&bp, refp->ref_pic_num, 3);
    mpp_put_bits(&bp, pp->scene_reference_enable_flag, 1);
    mpp_put_bits(&bp, pp->bottom_field_picture_flag, 1);
    mpp_put_bits(&bp, pp->fixed_picture_qp, 1);
    mpp_put_bits(&bp, pp->picture_qp, 7);
    mpp_put_bits(&bp, pp->loop_filter_disable_flag, 1);
    mpp_put_bits(&bp, pp->alpha_c_offset, 5);
    mpp_put_bits(&bp, pp->beta_offset, 5);

    //!< weight quant param
    mpp_put_bits(&bp, wqmp->chroma_quant_param_delta_cb, 6);
    mpp_put_bits(&bp, wqmp->chroma_quant_param_delta_cr, 6);
    mpp_put_bits(&bp, wqmp->pic_weight_quant_enable_flag, 1);

    //!< alf param
    mpp_put_bits(&bp, alfp->enable_pic_alf_y, 1);
    mpp_put_bits(&bp, alfp->enable_pic_alf_cb, 1);
    mpp_put_bits(&bp, alfp->enable_pic_alf_cr, 1);

    mpp_put_bits(&bp, alfp->alf_filter_num_minus1, 4);
    for (i = 0; i < 16; i++)
        mpp_put_bits(&bp, alfp->alf_coeff_idx_tab[i], 4);

    for (i = 0; i < 16; i++)
        for (j = 0; j < 9; j++)
            mpp_put_bits(&bp, alfp->alf_coeff_y[i][j], 7);

    for (j = 0; j < 9; j++)
        mpp_put_bits(&bp, alfp->alf_coeff_cb[j], 7);

    for (j = 0; j < 9; j++)
        mpp_put_bits(&bp, alfp->alf_coeff_cr[j], 7);

    /* other flags */
    mpp_put_bits(&bp, pp->multi_hypothesis_skip_enable_flag, 1);
    mpp_put_bits(&bp, pp->dual_hypothesis_prediction_enable_flag, 1);
    mpp_put_bits(&bp, pp->weighted_skip_enable_flag, 1);
    mpp_put_bits(&bp, pp->asymmetrc_motion_partitions_enable_flag, 1);
    mpp_put_bits(&bp, pp->nonsquare_quadtree_transform_enable_flag, 1);
    mpp_put_bits(&bp, pp->nonsquare_intra_prediction_enable_flag, 1);

    //!< picture reference params
    mpp_put_bits(&bp, pp->cur_poc, 32);
    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, (i < refp->ref_pic_num) ? refp->ref_poc_list[i] : 0, 32);
    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, (i < refp->ref_pic_num) ? pp->field_coded_sequence : 0, 1);
    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, (i < refp->ref_pic_num) ? pp->bottom_field_picture_flag : 0, 1);
    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, (i < refp->ref_pic_num), 1);

    return MPP_OK;
}

static MPP_RET prepare_scalist(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    WqmParams_Avs2d *wqmp = &syntax->wqmp;
    RK_U32 i = 0;
    RK_U32 n = 0;

    if (!wqmp->pic_weight_quant_enable_flag)
        return MPP_OK;

    memset(data, 0, len);

    /* dump by block4x4, vectial direction */
    for (i = 0; i < 4; i++) {
        data[n++] = wqmp->wq_matrix[0][i + 0];
        data[n++] = wqmp->wq_matrix[0][i + 4];
        data[n++] = wqmp->wq_matrix[0][i + 8];
        data[n++] = wqmp->wq_matrix[0][i + 12];
    }

    /* block8x8 */
    {
        RK_S32 blk4_x = 0, blk4_y = 0;

        /* dump by block4x4, vectial direction */
        for (blk4_x = 0; blk4_x < 8; blk4_x += 4) {
            for (blk4_y = 0; blk4_y < 8; blk4_y += 4) {
                RK_S32 pos = blk4_y * 8 + blk4_x;

                for (i = 0; i < 4; i++) {
                    data[n++] = wqmp->wq_matrix[1][pos + i + 0];
                    data[n++] = wqmp->wq_matrix[1][pos + i + 8];
                    data[n++] = wqmp->wq_matrix[1][pos + i + 16];
                    data[n++] = wqmp->wq_matrix[1][pos + i + 24];
                }
            }
        }
    }

    return MPP_OK;
}

static RK_S32 get_frame_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd = mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

static RK_S32 get_packet_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->packet_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd =  mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

static void init_ctrl_regs(Vdpu383Avs2dRegSet *regs)
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

static void avs2d_refine_rcb_size(Vdpu383RcbInfo *rcb_info,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    (void) height;
    Avs2dSyntax_t *syntax = dxva;
    RK_U8 ctu_size = 1 << syntax->pp.lcu_size;
    RK_U8 bit_depth = syntax->pp.bit_depth_chroma_minus8 + 8;
    RK_U32 rcb_bits = 0;
    RK_U32 filterd_row_append = 8192;

    width = MPP_ALIGN(width, ctu_size);

    /* RCB_STRMD_ROW && RCB_STRMD_TILE_ROW*/
    if (width > 8192)
        rcb_bits = ((width + 63) / 64) * 112;
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_TILE_ROW].size = 0;

    /* RCB_INTER_ROW && RCB_INTER_TILE_ROW*/
    rcb_bits = ((width + 7) / 8) * 166;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTER_TILE_ROW].size = 0;

    /* RCB_INTRA_ROW && RCB_INTRA_TILE_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 3; //TODO:
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTRA_TILE_ROW].size = 0;

    /* RCB_FILTERD_ROW && RCB_FILTERD_TILE_ROW*/
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_bits = MPP_ALIGN(width, 64) * (30 * bit_depth + 9);
    rcb_info[RCB_FILTERD_ROW].size = MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FILTERD_PROTECT_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FILTERD_TILE_ROW].size = 0;

    /* RCB_FILTERD_TILE_COL */
    rcb_info[RCB_FILTERD_TILE_COL].size = 0;
}

static void hal_avs2d_rcb_info_update(void *hal, Vdpu383Avs2dRegSet *regs)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx_t *reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;
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

static MPP_RET fill_registers(Avs2dHalCtx_t *p_hal, Vdpu383Avs2dRegSet *regs, HalTaskInfo *task)
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
            regs->avs2d_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
            fbd_offset = regs->avs2d_paras.reg68_hor_virstride * MPP_ALIGN(ver_virstride, 64) * 4;
            regs->avs2d_addrs.reg193_fbc_payload_offset = fbd_offset;
        } else if (is_tile) {
            regs->ctrl_regs.reg9.tile_e = 1;
            regs->avs2d_paras.reg68_hor_virstride = hor_virstride * 6 / 16;
            regs->avs2d_paras.reg70_y_virstride = (y_virstride + uv_virstride) / 16;
        } else {
            regs->ctrl_regs.reg9.fbc_e = 0;
            regs->ctrl_regs.reg9.tile_e = 0;
            regs->avs2d_paras.reg68_hor_virstride = hor_virstride / 16;
            regs->avs2d_paras.reg69_raster_uv_hor_virstride = hor_virstride / 16;
            regs->avs2d_paras.reg70_y_virstride = y_virstride / 16;
        }
    }

    // set current
    {
        RK_S32 fd = get_frame_fd(p_hal, task_dec->output);

        mpp_assert(fd >= 0);

        regs->avs2d_addrs.reg168_decout_base = fd;
        regs->avs2d_addrs.reg192_payload_st_cur_base = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, task_dec->output);
        regs->avs2d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        AVS2D_HAL_TRACE("cur frame index %d, fd %d, colmv fd %d", task_dec->output, fd, regs->avs2d_addrs.reg216_colmv_cur_base);

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
                    regs->avs2d_addrs.reg170_185_ref_base[i] = get_frame_fd(p_hal, slot_idx);
                    regs->avs2d_addrs.reg195_210_payload_st_ref_base[i] = get_frame_fd(p_hal, slot_idx);
                    mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                    regs->avs2d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
                }
            }
        }

        if (p_hal->syntax.refp.scene_ref_enable && p_hal->syntax.refp.scene_ref_slot_idx >= 0) {
            MppFrame scene_ref = NULL;
            RK_S32 slot_idx = p_hal->syntax.refp.scene_ref_slot_idx;
            RK_S32 replace_idx = p_hal->syntax.refp.scene_ref_replace_pos;

            mpp_buf_slot_get_prop(p_hal->frame_slots, slot_idx, SLOT_FRAME_PTR, &scene_ref);

            if (scene_ref) {
                regs->avs2d_addrs.reg170_185_ref_base[replace_idx] = get_frame_fd(p_hal, slot_idx);
                regs->avs2d_addrs.reg195_210_payload_st_ref_base[replace_idx] = regs->avs2d_addrs.reg170_185_ref_base[replace_idx];
                mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                regs->avs2d_addrs.reg217_232_colmv_ref_base[replace_idx] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        }

        regs->avs2d_addrs.reg169_error_ref_base = regs->avs2d_addrs.reg170_185_ref_base[0];
        regs->avs2d_addrs.reg194_payload_st_error_ref_base = regs->avs2d_addrs.reg195_210_payload_st_ref_base[0];
    }

    // set rlc
    regs->common_addr.reg128_strm_base = get_packet_fd(p_hal, task_dec->input);
    AVS2D_HAL_TRACE("packet fd %d from slot %d", regs->common_addr.reg128_strm_base, task_dec->input);

    regs->avs2d_paras.reg66_stream_len = MPP_ALIGN(mpp_packet_get_length(task_dec->input_packet), 16) + 64;

    {
        //scale down config
        mpp_buf_slot_get_prop(p_hal->frame_slots, task_dec->output,
                              SLOT_FRAME_PTR, &mframe);
        if (mpp_frame_get_thumbnail_en(mframe)) {
            regs->common_addr.reg133_scale_down_base = regs->avs2d_addrs.reg168_decout_base;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->avs2d_paras);
        } else {
            regs->ctrl_regs.reg9.scale_down_en = 0;
        }
    }

    return ret;
}

MPP_RET hal_avs2d_vdpu383_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, loop;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx_t *reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == reg_ctx);

    //!< malloc buffers
    loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }

        MPP_FREE(reg_ctx->reg_buf[i].regs);
    }

    if (reg_ctx->bufs) {
        mpp_buffer_put(reg_ctx->bufs);
        reg_ctx->bufs = NULL;
    }

    if (p_hal->cmv_bufs) {
        hal_bufs_deinit(p_hal->cmv_bufs);
        p_hal->cmv_bufs = NULL;
    }

    MPP_FREE(p_hal->reg_ctx);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_vdpu383_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, loop;
    Avs2dRkvRegCtx_t *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Avs2dRkvRegCtx_t)));
    reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;

    //!< malloc buffers
    loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, AVS2_ALL_TBL_BUF_SIZE(loop)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);

    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383Avs2dRegSet, 1);
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
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, avs2d_ver_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, avs2d_len_align);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    (void)cfg;
    return ret;
__FAILED:
    hal_avs2d_vdpu383_deinit(p_hal);
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

static RK_S32 calc_mv_size(RK_S32 pic_w, RK_S32 pic_h, RK_S32 ctu_w)
{
    RK_S32 seg_w = 64 * 16 * 16 / ctu_w; // colmv_block_size = 16, colmv_per_bytes = 16
    RK_S32 seg_cnt_w = MPP_ALIGN(pic_w, seg_w) / seg_w;
    RK_S32 seg_cnt_h = MPP_ALIGN(pic_h, ctu_w) / ctu_w;
    RK_S32 mv_size   = seg_cnt_w * seg_cnt_h * 64 * 16;

    return mv_size;
}

static MPP_RET set_up_colmv_buf(void *hal)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    RK_U32 ctu_size = 1 << (p_hal->syntax.pp.lcu_size);
    RK_S32 mv_size = calc_mv_size(pp->pic_width_in_luma_samples,
                                  pp->pic_height_in_luma_samples * (1 + pp->field_coded_sequence),
                                  ctu_size);

    AVS2D_HAL_TRACE("mv_size %d", mv_size);

    if (p_hal->cmv_bufs == NULL || p_hal->mv_size < (RK_U32)mv_size) {
        size_t size = mv_size;

        if (p_hal->cmv_bufs) {
            hal_bufs_deinit(p_hal->cmv_bufs);
            p_hal->cmv_bufs = NULL;
        }

        hal_bufs_init(&p_hal->cmv_bufs);
        if (p_hal->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            ret = MPP_ERR_INIT;
            goto __RETURN;
        }

        p_hal->mv_size = mv_size;
        p_hal->mv_count = mpp_buf_slot_get_count(p_hal->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

__RETURN:
    return ret;
}

MPP_RET hal_avs2d_vdpu383_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dRkvRegCtx_t *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Vdpu383Avs2dRegSet *regs = NULL;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);
    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->base.disable_error) {
        ret = MPP_NOK;
        goto __RETURN;
    }

    ret = set_up_colmv_buf(p_hal);
    if (ret)
        goto __RETURN;

    reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;

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

    prepare_header(p_hal, reg_ctx->shph_dat, sizeof(reg_ctx->shph_dat) / 8);
    prepare_scalist(p_hal, reg_ctx->scalist_dat, sizeof(reg_ctx->scalist_dat));

    ret = fill_registers(p_hal, regs, task);

    if (ret)
        goto __RETURN;

    {
        memcpy(reg_ctx->bufs_ptr + reg_ctx->shph_offset, reg_ctx->shph_dat, sizeof(reg_ctx->shph_dat));
        memcpy(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, reg_ctx->scalist_dat, sizeof(reg_ctx->scalist_dat));

        regs->common_addr.reg131_gbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 131, reg_ctx->shph_offset);
        regs->avs2d_paras.reg67_global_len = AVS2_383_SHPH_SIZE;

        regs->common_addr.reg132_scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 132, reg_ctx->sclst_offset);
    }

    // set rcb
    {
        hal_avs2d_rcb_info_update(p_hal, regs);
        vdpu383_setup_rcb(&regs->common_addr, p_hal->dev, p_hal->fast_mode ?
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
    Vdpu383Avs2dRegSet *regs = NULL;
    Avs2dRkvRegCtx_t *reg_ctx;
    MppDev dev = NULL;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    AVS2D_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == p_hal);

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->base.disable_error) {
        goto __RETURN;
    }

    reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;
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

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->avs2d_paras;
        wr_cfg.size = sizeof(regs->avs2d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->avs2d_addrs;
        wr_cfg.size = sizeof(regs->avs2d_addrs);
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

        if (avs2d_hal_debug & AVS2D_HAL_DBG_REG) {
            memset(reg_ctx->reg_out, 0, sizeof(reg_ctx->reg_out));
            rd_cfg.reg = reg_ctx->reg_out;
            rd_cfg.size = sizeof(reg_ctx->reg_out);
            rd_cfg.offset = 0;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu383_set_rcbinfo(dev, (Vdpu383RcbInfo*)reg_ctx->rcb_info);

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

static RK_U8 fetch_data(RK_U32 fmt, RK_U8 *line, RK_U32 num)
{
    RK_U32 offset = 0;
    RK_U32 value = 0;

    if (fmt == MPP_FMT_YUV420SP_10BIT) {
        offset = (num * 2) & 7;
        value = (line[num * 10 / 8] >> offset) |
                (line[num * 10 / 8 + 1] << (8 - offset));

        value = (value & 0x3ff) >> 2;
    } else if (fmt == MPP_FMT_YUV420SP) {
        value = line[num];
    }

    return value;
}

static MPP_RET hal_avs2d_vdpu383_dump_yuv(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    MppFrameFormat fmt = MPP_FMT_YUV420SP;
    RK_U32 vir_w = 0;
    RK_U32 vir_h = 0;
    RK_U32 i = 0;
    RK_U32 j = 0;
    FILE *fp_stream = NULL;
    char name[50];
    MppBuffer buffer = NULL;
    MppFrame frame;
    void *base = NULL;

    ret = mpp_buf_slot_get_prop(p_hal->frame_slots, task->dec.output, SLOT_FRAME_PTR, &frame);

    if (ret != MPP_OK || frame == NULL)
        mpp_log_f("failed to get frame slot %d", task->dec.output);

    ret = mpp_buf_slot_get_prop(p_hal->frame_slots, task->dec.output, SLOT_BUFFER, &buffer);

    if (ret != MPP_OK || buffer == NULL)
        mpp_log_f("failed to get frame buffer slot %d", task->dec.output);

    AVS2D_HAL_TRACE("frame slot %d, fd %d\n", task->dec.output, mpp_buffer_get_fd(buffer));
    base = mpp_buffer_get_ptr(buffer);
    vir_w = mpp_frame_get_hor_stride(frame);
    vir_h = mpp_frame_get_ver_stride(frame);
    fmt = mpp_frame_get_fmt(frame);
    snprintf(name, sizeof(name), "/data/tmp/rkv_out_%dx%d_nv12_%03d.yuv", vir_w, vir_h,
             p_hal->frame_no);
    fp_stream = fopen(name, "wb");

    if (fmt != MPP_FMT_YUV420SP_10BIT) {
        fwrite(base, 1, vir_w * vir_h * 3 / 2, fp_stream);
    } else {
        RK_U8 tmp = 0;
        for (i = 0; i < vir_h; i++) {
            for (j = 0; j < vir_w; j++) {
                tmp = fetch_data(fmt, base, j);
                fwrite(&tmp, 1, 1, fp_stream);
            }
            base += vir_w;
        }

        for (i = 0; i < vir_h / 2; i++) {
            for (j = 0; j < vir_w; j++) {
                tmp = fetch_data(fmt, base, j);
                fwrite(&tmp, 1, 1, fp_stream);
            }
            base += vir_w;
        }
    }
    fclose(fp_stream);

    return ret;
}

MPP_RET hal_avs2d_vdpu383_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx_t *reg_ctx;
    Vdpu383Avs2dRegSet *regs;

    INP_CHECK(ret, NULL == p_hal);
    reg_ctx = (Avs2dRkvRegCtx_t *)p_hal->reg_ctx;
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
        hal_avs2d_vdpu383_dump_yuv(hal, task);

    AVS2D_HAL_TRACE("read irq_status 0x%08x\n", regs->ctrl_regs.reg19);

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

    memset(&regs->ctrl_regs.reg19, 0, sizeof(RK_U32));
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
    .ctx_size = sizeof(Avs2dRkvRegCtx_t),
    .flag     = 0,
    .init     = hal_avs2d_vdpu383_init,
    .deinit   = hal_avs2d_vdpu383_deinit,
    .reg_gen  = hal_avs2d_vdpu383_gen_regs,
    .start    = hal_avs2d_vdpu383_start,
    .wait     = hal_avs2d_vdpu383_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
};
