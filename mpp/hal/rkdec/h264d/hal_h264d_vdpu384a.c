/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h264d_vdpu384a"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "hal_h264d_global.h"
#include "hal_h264d_vdpu384a.h"
#include "vdpu384a_h264d.h"
#include "mpp_dec_cb_param.h"
#include "vdpu_com.h"
#include "vdpu38x_com.h"
#include "hal_h264d_ctx.h"
#include "hal_h264d_com.h"

/* Number registers for the decoder */
#define DEC_VDPU384A_REGISTERS               276

#define VDPU384A_SPSPPS_SIZE                 (MPP_ALIGN(2266 + 64, 128) / 8) /* byte, 2266 bit + Reserve 64 */
#define VDPU384A_SCALING_LIST_SIZE           (6*16+2*64 + 128)   /* bytes */
#define VDPU384A_ERROR_INFO_SIZE             (256*144*4)         /* bytes */

#define VDPU384A_ERROR_INFO_ALIGNED_SIZE     (0)
#define VDPU384A_SPSPPS_ALIGNED_SIZE         (MPP_ALIGN(VDPU384A_SPSPPS_SIZE, SZ_4K))
#define VDPU384A_SCALING_LIST_ALIGNED_SIZE   (MPP_ALIGN(VDPU384A_SCALING_LIST_SIZE, SZ_4K))
#define VDPU384A_STREAM_INFO_SET_SIZE        (VDPU384A_SPSPPS_ALIGNED_SIZE + \
                                             VDPU384A_SCALING_LIST_ALIGNED_SIZE)

#define VDPU384A_ERROR_INFO_OFFSET           (0)
#define VDPU384A_STREAM_INFO_OFFSET_BASE     (VDPU384A_ERROR_INFO_OFFSET + VDPU384A_ERROR_INFO_ALIGNED_SIZE)
#define VDPU384A_SPSPPS_OFFSET(pos)          (VDPU384A_STREAM_INFO_OFFSET_BASE + (VDPU384A_STREAM_INFO_SET_SIZE * pos))
#define VDPU384A_SCALING_LIST_OFFSET(pos)    (VDPU384A_SPSPPS_OFFSET(pos) + VDPU384A_SPSPPS_ALIGNED_SIZE)
#define VDPU384A_INFO_BUFFER_SIZE(cnt)       (VDPU384A_STREAM_INFO_OFFSET_BASE + (VDPU384A_STREAM_INFO_SET_SIZE * cnt))

static MPP_RET set_registers(H264dHalCtx_t *p_hal, Vdpu384aH264dRegSet *regs, HalTaskInfo *task)
{
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    HalBuf *mv_buf = NULL;
    HalBuf *origin_buf = NULL;
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;

    // memset(regs, 0, sizeof(Vdpu384aH264dRegSet));
    regs->h264d_paras.reg66_stream_len = p_hal->strm_len;

    //!< caculate the yuv_frame_size
    {
        MppFrame mframe = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        uv_virstride = hor_virstride * ver_virstride / 2;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset;

            fbd_offset = fbc_hdr_stride * MPP_ALIGN(ver_virstride, 64) / 16;

            regs->ctrl_regs.reg9.dpb_data_sel = 0;
            regs->ctrl_regs.reg9.dpb_output_dis = 0;
            regs->ctrl_regs.reg9.pp_m_output_mode = 0;

            regs->h264d_paras.reg68_dpb_hor_virstride = fbc_hdr_stride / 64;
            regs->h264d_addrs.reg193_dpb_fbc64x4_payload_offset = fbd_offset;
            regs->h264d_paras.reg80_error_ref_hor_virstride = regs->h264d_paras.reg68_dpb_hor_virstride;
        } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
            regs->ctrl_regs.reg9.dpb_data_sel = 1;
            regs->ctrl_regs.reg9.dpb_output_dis = 1;
            regs->ctrl_regs.reg9.pp_m_output_mode = 2;

            regs->h264d_paras.reg77_pp_m_hor_stride = hor_virstride * 6 / 16;
            regs->h264d_paras.reg79_pp_m_y_virstride = (y_virstride + uv_virstride) / 16;
            regs->h264d_paras.reg80_error_ref_hor_virstride = regs->h264d_paras.reg77_pp_m_hor_stride;
        } else {
            regs->ctrl_regs.reg9.dpb_data_sel = 1;
            regs->ctrl_regs.reg9.dpb_output_dis = 1;
            regs->ctrl_regs.reg9.pp_m_output_mode = 1;

            regs->h264d_paras.reg77_pp_m_hor_stride = hor_virstride / 16;
            regs->h264d_paras.reg78_pp_m_uv_hor_stride = hor_virstride / 16;
            regs->h264d_paras.reg79_pp_m_y_virstride = y_virstride / 16;
            regs->h264d_paras.reg80_error_ref_hor_virstride = regs->h264d_paras.reg77_pp_m_hor_stride;
        }
        regs->h264d_paras.reg81_error_ref_raster_uv_hor_virstride = regs->h264d_paras.reg78_pp_m_uv_hor_stride;
        regs->h264d_paras.reg82_error_ref_virstride = regs->h264d_paras.reg79_pp_m_y_virstride;
    }
    //!< set current
    {
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        /* output rkfbc64 */
        // regs->h264d_addrs.reg168_dpb_decout_base = fd;
        /* output raster/tile4x4 */
        regs->common_addr.reg135_pp_m_decout_base = fd;
        regs->h264d_addrs.reg192_dpb_payload64x4_st_cur_base = fd;

        //colmv_cur_base
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, pp->CurrPic.Index7Bits);
        regs->h264d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        regs->h264d_addrs.reg169_error_ref_base = fd;
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
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_FRAME_PTR, &mframe);
            if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
                mbuffer = origin_buf->buf[0];
            }

            if (pp->FrameNumList[i] < pp->frame_num &&
                pp->FrameNumList[i] > min_frame_num &&
                (!mpp_frame_get_errinfo(mframe))) {
                min_frame_num = pp->FrameNumList[i];
                regs->h264d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
            }

            fd = mpp_buffer_get_fd(mbuffer);
            regs->h264d_addrs.reg170_185_ref_base[i] = fd;
            regs->h264d_addrs.reg195_210_payload_st_ref_base[i] = fd;
            mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
            regs->h264d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }

        if (pp->RefFrameList[15].bPicEntry != 0xff) {
            ref_index = pp->RefFrameList[15].Index7Bits;
        } else {
            ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
        }

        mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        if (mpp_frame_get_thumbnail_en(mframe) == 2) {
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, ref_index);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
        }
        regs->h264d_addrs.reg170_185_ref_base[15] = fd;
        regs->h264d_addrs.reg195_210_payload_st_ref_base[15] = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
        regs->h264d_addrs.reg217_232_colmv_ref_base[15] = mpp_buffer_get_fd(mv_buf->buf[0]);
    }
    {
        MppBuffer mbuffer = NULL;

        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        regs->common_addr.reg129_stream_buf_st_base = mpp_buffer_get_fd(mbuffer);
        regs->common_addr.reg130_stream_buf_end_base = mpp_buffer_get_fd(mbuffer);
        mpp_dev_set_reg_offset(p_hal->dev, 130, mpp_buffer_get_size(mbuffer));
        // regs->h264d_paras.reg65_strm_start_bit = 2 * 8;
#ifdef DUMP_VDPU384A_DATAS
        {
            char *cur_fname = "stream_in.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                              8 * p_hal->strm_len, 128, 0);
        }
#endif
    }

    {
        //scale down config
        MppFrame mframe = NULL;
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        fd = mpp_buffer_get_fd(mbuffer);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->common_addr.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, pp->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            /* output rkfbc64 */
            // regs->h264d_addrs.reg168_dpb_decout_base = fd;
            /* output raster/tile4x4 */
            regs->common_addr.reg135_pp_m_decout_base = fd;
            regs->h264d_addrs.reg192_dpb_payload64x4_st_cur_base = fd;
            regs->h264d_addrs.reg169_error_ref_base = fd;
            vdpu384a_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs, (void*)&regs->h264d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->common_addr.reg133_scale_down_base = fd;
            vdpu384a_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs, (void*)&regs->h264d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return MPP_OK;
}

static MPP_RET init_ctrl_regs(Vdpu384aH264dRegSet *regs)
{
    Vdpu384aCtrlReg *ctrl_regs = &regs->ctrl_regs;

    ctrl_regs->reg8_dec_mode = 1;  //!< h264
    ctrl_regs->reg9.low_latency_en = 0;

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

    ctrl_regs->reg11.rd_outstanding = 32;
    ctrl_regs->reg11.wr_outstanding = 250;

    ctrl_regs->reg13_core_timeout_threshold = 0xffffff;

    ctrl_regs->reg16.error_proc_disable = 1;
    ctrl_regs->reg16.error_spread_disable = 0;
    ctrl_regs->reg16.roi_error_ctu_cal_en = 0;

    ctrl_regs->reg20_cabac_error_en_lowbits = 0xfffedfff;
    ctrl_regs->reg21_cabac_error_en_highbits = 0x0ffbf9ff;

    /* performance */
    ctrl_regs->reg28.axi_perf_work_e = 1;
    ctrl_regs->reg28.axi_cnt_type = 1;
    ctrl_regs->reg28.rd_latency_id = 11;

    ctrl_regs->reg29.addr_align_type = 2;
    ctrl_regs->reg29.ar_cnt_id_type = 0;
    ctrl_regs->reg29.aw_cnt_id_type = 0;
    ctrl_regs->reg29.ar_count_id = 0xa;
    ctrl_regs->reg29.aw_count_id = 0;
    ctrl_regs->reg29.rd_band_width_mode = 0;

    return MPP_OK;
}

MPP_RET vdpu384a_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    (void) cfg;

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Vdpu3xxH264dRegCtx)));
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;

    //!< malloc buffers
    reg_ctx->spspps = mpp_calloc(RK_U8, VDPU384A_SPSPPS_SIZE);
    reg_ctx->sclst = mpp_calloc(RK_U8, VDPU384A_SCALING_LIST_SIZE);
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs,
                                   VDPU384A_INFO_BUFFER_SIZE(max_cnt)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    reg_ctx->offset_errinfo = VDPU384A_ERROR_INFO_OFFSET;
    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu384aH264dRegSet, 1);
        init_ctrl_regs(reg_ctx->reg_buf[i].regs);
        reg_ctx->offset_spspps[i] = VDPU384A_SPSPPS_OFFSET(i);
        reg_ctx->offset_sclst[i] = VDPU384A_SCALING_LIST_OFFSET(i);
    }

    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv420);

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu384a_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu3xx_h264d_deinit(hal);

    return ret;
}

static void h264d_refine_rcb_size(H264dHalCtx_t *p_hal, VdpuRcbInfo *rcb_info,
                                  RK_S32 width, RK_S32 height)
{
    RK_U32 rcb_bits = 0;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;
    RK_U32 row_uv_para = 1; // for yuv420/yuv422
    RK_U32 filterd_row_append = 8192;

    // vdpu384a h264d support yuv400/yuv420/yuv422
    if (chroma_format_idc == 0)
        row_uv_para = 0;

    width = MPP_ALIGN(width, H264_CTU_SIZE);
    height = MPP_ALIGN(height, H264_CTU_SIZE);
    /* RCB_STRMD_IN_ROW && RCB_STRMD_ON_ROW*/
    if (width > 4096)
        rcb_bits = ((width + 15) / 16) * 158 * (mbaff ? 2 : 1);
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_ON_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_IN_ROW && RCB_INTER_ON_ROW*/
    rcb_bits = ((width + 3) / 4) * 92 * (mbaff ? 2 : 1);
    rcb_info[RCB_INTER_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTER_ON_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTRA_IN_ROW && RCB_INTRA_ON_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2) * (mbaff ? 2 : 1);
    if (chroma_format_idc == 1 || chroma_format_idc == 2)
        rcb_bits = rcb_bits * 5 / 2; //TODO:

    rcb_info[RCB_INTRA_IN_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_INTRA_ON_ROW].size = 0;
    /* RCB_FLTD_IN_ROW && RCB_FLTD_PROT_IN_ROW*/
    // save space mode : half for RCB_FLTD_IN_ROW, half for RCB_FLTD_PROT_IN_ROW
    rcb_bits = width * 13 * ((6 + 3 * row_uv_para) * (mbaff ? 2 : 1) + 2 * row_uv_para + 1.5);
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_info[RCB_FLTD_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FLTD_PROT_IN_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);

    rcb_info[RCB_FLTD_ON_ROW].size = 0;
    /* RCB_FLTD_ON_COL */
    rcb_info[RCB_FLTD_ON_COL].size = 0;

}

static void hal_h264d_rcb_info_update(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t*)hal;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);

    if ( ctx->bit_depth != bit_depth ||
         ctx->chroma_format_idc != chroma_format_idc ||
         ctx->mbaff != mbaff ||
         ctx->width != width ||
         ctx->height != height) {
        RK_U32 i;
        RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(ctx->reg_buf) : 1;

        ctx->rcb_buf_size = vdpu384a_get_rcb_buf_size(ctx->rcb_info, width, height);
        h264d_refine_rcb_size(hal, ctx->rcb_info, width, height);
        /* vdpu384a_check_rcb_buf_size(ctx->rcb_info, width, height); */
        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf = ctx->rcb_buf[i];

            if (rcb_buf) {
                mpp_buffer_put(rcb_buf);
                ctx->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(p_hal->buf_group, &rcb_buf, ctx->rcb_buf_size);
            ctx->rcb_buf[i] = rcb_buf;
        }
        ctx->bit_depth      = bit_depth;
        ctx->width          = width;
        ctx->height         = height;
        ctx->mbaff          = mbaff;
        ctx->chroma_format_idc = chroma_format_idc;
    }
}

MPP_RET vdpu384a_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu384aH264dRegSet *regs = ctx->regs;
    MppFrame mframe;
    RK_S32 mv_size = MPP_ALIGN(width, 64) * MPP_ALIGN(height, 16); // 16 byte unit

    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

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
        p_hal->mv_count = mpp_buf_slot_get_count(p_hal->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

    mpp_buf_slot_get_prop(p_hal->frame_slots, p_hal->pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        ctx->origin_bufs == NULL) {
        vdpu38x_setup_scale_origin_bufs(mframe, &ctx->origin_bufs);
    }

    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                regs = ctx->reg_buf[i].regs;

                ctx->spspps_offset = ctx->offset_spspps[i];
                ctx->sclst_offset = ctx->offset_sclst[i];
                ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

#ifdef DUMP_VDPU384A_DATAS
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

    vdpu38x_h264d_prepare_spspps(p_hal, (RK_U64 *)ctx->spspps, VDPU384A_SPSPPS_SIZE / 8);
    vdpu38x_h264d_prepare_scanlist(p_hal, ctx->sclst, VDPU384A_SCALING_LIST_SIZE);
    set_registers(p_hal, regs, task);

    //!< copy spspps datas
    memcpy((char *)ctx->bufs_ptr + ctx->spspps_offset, (char *)ctx->spspps, VDPU384A_SPSPPS_SIZE);

    regs->common_addr.reg131_gbl_base = ctx->bufs_fd;
    regs->h264d_paras.reg67_global_len = VDPU384A_SPSPPS_SIZE / 16; // 128 bit as unit
    mpp_dev_set_reg_offset(p_hal->dev, 131, ctx->spspps_offset);

    if (p_hal->pp->scaleing_list_enable_flag) {
        memcpy((char *)ctx->bufs_ptr + ctx->sclst_offset, (void *)ctx->sclst, VDPU384A_SCALING_LIST_SIZE);
        regs->common_addr.reg132_scanlist_addr = ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 132, ctx->sclst_offset);
    } else {
        regs->common_addr.reg132_scanlist_addr = 0;
    }

    hal_h264d_rcb_info_update(p_hal);
    vdpu384a_setup_rcb(&regs->common_addr, p_hal->dev, p_hal->fast_mode ?
                       ctx->rcb_buf[task->dec.reg_index] : ctx->rcb_buf[0],
                       ctx->rcb_info);
    vdpu384a_setup_statistic(&regs->ctrl_regs);
    mpp_buffer_sync_end(ctx->bufs);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu384a_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu384aH264dRegSet *regs = p_hal->fast_mode ?
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

        wr_cfg.reg = &regs->h264d_paras;
        wr_cfg.size = sizeof(regs->h264d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->h264d_addrs;
        wr_cfg.size = sizeof(regs->h264d_addrs);
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
        vdpu384a_set_rcbinfo(dev, (VdpuRcbInfo*)reg_ctx->rcb_info);

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

MPP_RET vdpu384a_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu384aH264dRegSet *p_regs = p_hal->fast_mode ?
                                  reg_ctx->reg_buf[task->dec.reg_index].regs :
                                  reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->dec_cb) {
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

        mpp_callback(p_hal->dec_cb, &param);
    }
    memset(&p_regs->ctrl_regs.reg19, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_h264d_vdpu384a = {
    .name     = "h264d_vdpu384a",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(Vdpu3xxH264dRegCtx),
    .flag     = 0,
    .init     = vdpu384a_h264d_init,
    .deinit   = vdpu3xx_h264d_deinit,
    .reg_gen  = vdpu384a_h264d_gen_regs,
    .start    = vdpu384a_h264d_start,
    .wait     = vdpu384a_h264d_wait,
    .reset    = vdpu_h264d_reset,
    .flush    = vdpu_h264d_flush,
    .control  = vdpu38x_h264d_control,
};
