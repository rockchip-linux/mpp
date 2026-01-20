/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_vp9d_vdpu384b"

#include "mpp_buffer_impl.h"

#include "vp9d_syntax.h"
#include "hal_vp9d_debug.h"
#include "vdpu38x_com.h"
#include "hal_vp9d_com.h"
#include "hal_vp9d_ctx.h"
#include "hal_vp9d_vdpu384b.h"

#define GBL_SIZE        (MPP_ALIGN(1340 + 64, 128) / 8)  // byte, 1340 bit + Reserve 64

#ifdef DUMP_VDPU38X_DATAS
static RK_U32 cur_last_segid_flag;
static MppBuffer cur_last_prob_base;
#endif

static MPP_RET hal_vp9d_alloc_res(HalVp9dCtx *hal)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu38xVp9dCtx *hw_ctx = (Vdpu38xVp9dCtx*)p_hal->hw_ctx;
    MppBufferGroup group = p_hal->cfg->buf_group;
    RK_S32 ret = 0;
    RK_S32 i = 0;

    /* alloc common buffer */
    for (i = 0; i < VP9_CONTEXT; i++) {
        ret = mpp_buffer_get(group, &hw_ctx->prob_loop_base[i], PROB_SIZE);
        if (ret) {
            mpp_err("vp9 probe_loop_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->prob_loop_base[i], p_hal->cfg->dev);
    }
    ret = mpp_buffer_get(group, &hw_ctx->prob_default_base, PROB_SIZE);
    if (ret) {
        mpp_err("vp9 probe_default_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->prob_default_base, p_hal->cfg->dev);

    ret = mpp_buffer_get(group, &hw_ctx->segid_cur_base, MAX_SEGMAP_SIZE);
    if (ret) {
        mpp_err("vp9 segid_cur_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->segid_cur_base, p_hal->cfg->dev);
    ret = mpp_buffer_get(group, &hw_ctx->segid_last_base, MAX_SEGMAP_SIZE);
    if (ret) {
        mpp_err("vp9 segid_last_base get buffer failed\n");
        return ret;
    }
    mpp_buffer_attach_dev(hw_ctx->segid_last_base, p_hal->cfg->dev);

    /* alloc buffer for fast mode or normal */
    if (p_hal->fast_mode) {
        for (i = 0; i < VDPU_FAST_REG_SET_CNT; i++) {
            hw_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu38xRegSet));
            ret = mpp_buffer_get(group,
                                 &hw_ctx->g_buf[i].global_base, GBL_SIZE);
            mpp_buffer_attach_dev(hw_ctx->g_buf[i].global_base, p_hal->cfg->dev);
            if (ret) {
                mpp_err("vp9 global_base get buffer failed\n");
                return ret;
            }
        }
    } else {
        hw_ctx->hw_regs = mpp_calloc_size(void, sizeof(Vdpu38xRegSet));
        ret = mpp_buffer_get(group, &hw_ctx->global_base, PROB_SIZE);
        if (ret) {
            mpp_err("vp9 global_base get buffer failed\n");
            return ret;
        }
        mpp_buffer_attach_dev(hw_ctx->global_base, p_hal->cfg->dev);
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu384b_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;

    mpp_env_get_u32("hal_vp9d_debug", &hal_vp9d_debug, 0);

    cfg->support_fast_mode = 1;
    p_hal->cfg = cfg;
    p_hal->client_type = VPU_CLIENT_RKVDEC;
    p_hal->fast_mode = cfg->cfg->base.fast_parse && cfg->support_fast_mode;

    MEM_CHECK(ret, p_hal->hw_ctx = mpp_calloc_size(void, sizeof(Vdpu38xVp9dCtx) + GBL_SIZE));
    Vdpu38xVp9dCtx *hw_ctx = (Vdpu38xVp9dCtx*)p_hal->hw_ctx;

    hw_ctx->mv_base_addr = -1;
    hw_ctx->pre_mv_base_addr = -1;
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_VER_ALIGN, mpp_align_64);

    ret = hal_vp9d_alloc_res(p_hal);
    if (ret) {
        mpp_err("hal_vp9d_alloc_res failed\n");
        goto __FAILED;
    }

    hw_ctx->last_segid_flag = 1;

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu38x_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 0;
    }

    vdpu38x_rcb_calc_init((Vdpu38xRcbCtx **)&hw_ctx->rcb_ctx);

    return ret;
__FAILED:
    hal_vp9d_vdpu38x_deinit(hal);
    return ret;
}

static MPP_RET vdpu384b_vp9d_rcb_calc(void *context, RK_U32 *total_size)
{
    Vdpu38xRcbCtx *ctx = (Vdpu38xRcbCtx *)context;
    RK_FLOAT cur_bit_size = 0;
    RK_U32 cur_uv_para = 0;
    RK_U32 bit_depth = ctx->bit_depth;
    RK_U32 in_tl_row = 0;
    RK_U32 on_tl_row = 0;
    RK_U32 on_tl_col = 0;
    Vdpu38xFmt rcb_fmt;

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
    if (ctx->pic_w > 4096)
        cur_bit_size = MPP_DIVUP(64, on_tl_row) * 250;
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_ON_ROW, 142, cur_bit_size);

    /* RCB_INTER_IN_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(64, in_tl_row) * 2368;
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_IN_ROW, 144, cur_bit_size);

    /* RCB_INTER_ON_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(64, on_tl_row) * 2368;
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
    cur_bit_size = MPP_ROUNDUP(64, in_tl_row) * (1.2 * bit_depth + 0.5)
                   * (17.5 + 8 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW,  154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.2 * bit_depth + 0.5)
                   * (17.5 + 8 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_ROW, 156, cur_bit_size);

    /* RCB_FLTD_ON_COL */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_col_uv_coef_map[rcb_fmt];
    if (ctx->tile_dir == 0)
        cur_bit_size = MPP_ROUNDUP(64, on_tl_col) * (1.6 * bit_depth + 0.5)
                       * (17.75 + 8 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_COL, 158, cur_bit_size);

    /* RCB_FLTD_UPSC_ON_COL */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_UPSC_ON_COL, 160, cur_bit_size);

    *total_size = vdpu38x_rcb_get_total_size(ctx);

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu384b_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32 i;
    RK_U8  bit_depth = 0;
    RK_U32 ref_frame_width_y;
    RK_U32 ref_frame_height_y;
    RK_S32 stream_len = 0, aglin_offset = 0;
    RK_U32 y_hor_virstride, uv_hor_virstride, y_virstride;
    RK_U8  *bitstream = NULL;
    MppBuffer streambuf = NULL;
    RK_U8  ref_idx = 0;
    RK_U8  ref_frame_idx = 0;
    RK_U32 *reg_ref_base = NULL;
    RK_U32 *reg_payload_ref_base = NULL;
    RK_S32 intraFlag = 0;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;
    HalBuf *origin_buf = NULL;

    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu38xVp9dCtx *hw_ctx = (Vdpu38xVp9dCtx*)p_hal->hw_ctx;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
    MppHalCfg *cfg = p_hal->cfg;
    Vdpu38xRegSet *regs = NULL;
    RK_S32 mv_size = pic_param->width * pic_param->height / 2;
    RK_U32 frame_ctx_id = pic_param->frame_context_idx;
    MppFrame mframe;
    MppFrame ref_frame = NULL;

    if (p_hal->fast_mode) {
        for (i = 0; i < VDPU_FAST_REG_SET_CNT; i++) {
            if (!hw_ctx->g_buf[i].use_flag) {
                task->dec.reg_index = i;
                hw_ctx->global_base = hw_ctx->g_buf[i].global_base;
                hw_ctx->hw_regs = hw_ctx->g_buf[i].hw_regs;
                hw_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == VDPU_FAST_REG_SET_CNT) {
            mpp_err("vp9 fast mode buf all used\n");
            return MPP_ERR_NOMEM;
        }
    }
    regs = (Vdpu38xRegSet*)hw_ctx->hw_regs;
    memset(regs, 0, sizeof(Vdpu38xRegSet));

#ifdef DUMP_VDPU38X_DATAS
    {
        memset(vdpu38x_dump_cur_dir, 0, sizeof(vdpu38x_dump_cur_dir));
        sprintf(vdpu38x_dump_cur_dir, "vp9/Frame%04d", vdpu38x_dump_cur_frm);
        if (access(vdpu38x_dump_cur_dir, 0)) {
            if (mkdir(vdpu38x_dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", vdpu38x_dump_cur_dir);
        }
        vdpu38x_dump_cur_frm++;
    }
#endif

    /* uncompress header data */
    vdpu38x_vp9d_uncomp_hdr(p_hal, pic_param, (RK_U64 *)hw_ctx->header_data, GBL_SIZE / 8);
    memcpy(mpp_buffer_get_ptr(hw_ctx->global_base), hw_ctx->header_data, GBL_SIZE);
    mpp_buffer_sync_end(hw_ctx->global_base);
    regs->comm_paras.reg67_global_len = GBL_SIZE / 16;
    regs->comm_addrs.reg131_gbl_base = mpp_buffer_get_fd(hw_ctx->global_base);

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
        hw_ctx->mv_count = mpp_buf_slot_get_count(cfg->frame_slots);
        hal_bufs_setup(hw_ctx->cmv_bufs, hw_ctx->mv_count, 1, &size);
    }

    mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        hw_ctx->origin_bufs == NULL) {
        vdpu38x_setup_scale_origin_bufs(mframe, &hw_ctx->origin_bufs,
                                        mpp_buf_slot_get_count(cfg->frame_slots));
    }

    stream_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);

    intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    /* init kf_probe */
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
            regs->comm_addrs.reg184_lastprob_base =
                mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
#ifdef DUMP_VDPU38X_DATAS
            cur_last_prob_base = hw_ctx->prob_loop_base[frame_ctx_id];
#endif
        } else {
            /*
             * When configuring the default prob, the hardware actually uses
             * the internal buffer of the hardware.
             */
            regs->comm_addrs.reg184_lastprob_base = mpp_buffer_get_fd(hw_ctx->prob_default_base);
            hw_ctx->prob_ctx_valid[frame_ctx_id] |= pic_param->refresh_frame_context;
#ifdef DUMP_VDPU38X_DATAS
            cur_last_prob_base = hw_ctx->prob_default_base;
#endif
        }
        regs->comm_addrs.reg185_updateprob_base =
            mpp_buffer_get_fd(hw_ctx->prob_loop_base[frame_ctx_id]);
    }
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "cabac_last_probe.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(cur_last_prob_base),
                                  8 * 152 * 16, 128, 0, 0);
    }
#endif

    regs->comm_paras.reg66_stream_len = ((stream_len + 15) & (~15)) + 0x80;

    mpp_buf_slot_get_prop(cfg->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    bitstream = mpp_buffer_get_ptr(streambuf);
    aglin_offset = regs->comm_paras.reg66_stream_len - stream_len;
    if (aglin_offset > 0) {
        memset((void *)(bitstream + stream_len), 0, aglin_offset);
    }

    //--- caculate the yuv_frame_size and mv_size
    bit_depth = pic_param->BitDepthMinus8Luma + 8;

    {
        MppFrameFormat fmt;
        RK_U32 sw_hor_virstride = 0;
        RK_U32 sw_ver_virstride = 0;
        RK_U32 sw_y_virstride = 0;
        RK_U32 sw_uv_virstride = 0;
        RK_U32 fbc_head_stride = 0;
        RK_U32 fbc_pld_stride = 0;
        RK_U32 fbc_offset = 0;
        RK_U32 tile4x4_coeff = 0;

        mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        fmt = mpp_frame_get_fmt(mframe);
        sw_hor_virstride = mpp_frame_get_hor_stride(mframe);
        sw_ver_virstride = mpp_frame_get_ver_stride(mframe);
        sw_y_virstride = sw_ver_virstride * sw_hor_virstride;
        sw_uv_virstride = sw_ver_virstride * sw_hor_virstride / 2;
        if (MPP_FRAME_FMT_IS_AFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            regs->ctrl_regs.reg9.pp_output_mode = 3;
            regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
            regs->comm_paras.reg80_error_ref_hor_virstride = fbc_head_stride;
        } else if (MPP_FRAME_FMT_IS_RKFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            regs->ctrl_regs.reg9.pp_output_mode = 2;
            regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
            regs->comm_paras.reg80_error_ref_hor_virstride = fbc_head_stride;
        } else if (MPP_FRAME_FMT_IS_TILE(fmt)) {
            if (vdpu38x_get_tile4x4_h_stride_coeff(fmt, &tile4x4_coeff)) {
                mpp_err("get tile 4x4 coeff failed\n");
                return MPP_NOK;
            }
            regs->ctrl_regs.reg9.pp_output_mode = 1;
            regs->comm_paras.reg68_pp_m_hor_stride = sw_hor_virstride * tile4x4_coeff >> 4;
            regs->comm_paras.reg70_pp_m_y_virstride = (sw_y_virstride + sw_uv_virstride) >> 4;
        } else {
            regs->ctrl_regs.reg9.pp_output_mode = 0;
            regs->comm_paras.reg68_pp_m_hor_stride = sw_hor_virstride >> 4;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = sw_hor_virstride >> 4;
            regs->comm_paras.reg70_pp_m_y_virstride = sw_y_virstride >> 4;
        }
        /* error stride */
        regs->comm_paras.reg80_error_ref_hor_virstride = regs->comm_paras.reg68_pp_m_hor_stride;
        regs->comm_paras.reg81_error_ref_raster_uv_hor_virstride = regs->comm_paras.reg69_pp_m_uv_hor_stride;
        regs->comm_paras.reg82_error_ref_virstride = regs->comm_paras.reg70_pp_m_y_virstride;
    }
    if (!pic_param->intra_only && pic_param->frame_type &&
        !pic_param->error_resilient_mode && hw_ctx->ls_info.last_show_frame) {
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;
    }

    mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
    mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output, SLOT_BUFFER, &framebuf);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
        origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, task->dec.output);
        framebuf = origin_buf->buf[0];
    }
    regs->comm_addrs.reg168_decout_base = mpp_buffer_get_fd(framebuf);
    regs->comm_addrs.reg169_error_ref_base = mpp_buffer_get_fd(framebuf);
    regs->comm_addrs.reg192_payload_st_cur_base = mpp_buffer_get_fd(framebuf);
    regs->comm_addrs.reg194_payload_st_error_ref_base = mpp_buffer_get_fd(framebuf);
    regs->comm_addrs.reg128_strm_base = mpp_buffer_get_fd(streambuf);
    regs->comm_addrs.reg129_stream_buf_st_base = mpp_buffer_get_fd(streambuf);
    regs->comm_addrs.reg130_stream_buf_end_base = mpp_buffer_get_fd(streambuf);
    mpp_dev_set_reg_offset(cfg->dev, 130, mpp_buffer_get_size(streambuf));

    {
        RK_U32 strm_offset = pic_param->uncompressed_header_size_byte_aligned;

        regs->comm_paras.reg65_strm_start_bit = 8 * (strm_offset & 0xf);
        mpp_dev_set_reg_offset(cfg->dev, 128, strm_offset & 0xfffffff0);
    }

    if (hw_ctx->last_segid_flag) {
        regs->comm_addrs.reg181_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
        regs->comm_addrs.reg182_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
    } else {
        regs->comm_addrs.reg181_segidlast_base = mpp_buffer_get_fd(hw_ctx->segid_cur_base);
        regs->comm_addrs.reg182_segidcur_base = mpp_buffer_get_fd(hw_ctx->segid_last_base);
    }
#ifdef DUMP_VDPU38X_DATAS
    cur_last_segid_flag = hw_ctx->last_segid_flag;
    {
        char *cur_fname = "stream_in.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(streambuf)
                                  + pic_param->uncompressed_header_size_byte_aligned,
                                  8 * (((stream_len + 15) & (~15)) + 0x80), 128, 0, 0);
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

    regs->comm_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);

    hw_ctx->mv_base_addr = regs->comm_addrs.reg216_colmv_cur_base;
    if (hw_ctx->pre_mv_base_addr < 0)
        hw_ctx->pre_mv_base_addr = hw_ctx->mv_base_addr;

    // vp9 only one colmv
    regs->comm_addrs.reg217_232_colmv_ref_base[0] = hw_ctx->pre_mv_base_addr;

    reg_ref_base = regs->comm_addrs.reg170_185_ref_base;
    reg_payload_ref_base = regs->comm_addrs.reg195_210_payload_st_ref_base;
    for (i = 0; i < 3; i++) {
        ref_idx = pic_param->frame_refs[i].Index7Bits;
        ref_frame_idx = pic_param->ref_frame_map[ref_idx].Index7Bits;
        ref_frame_width_y = pic_param->ref_frame_coded_width[ref_idx];
        ref_frame_height_y = pic_param->ref_frame_coded_height[ref_idx];
        if (ref_frame_idx < 0x7f)
            mpp_buf_slot_get_prop(cfg->frame_slots, ref_frame_idx, SLOT_FRAME_PTR, &ref_frame);
        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            vdpu38x_get_fbc_off(mframe, &y_hor_virstride, &uv_hor_virstride, &y_virstride);
        } else {
            if (ref_frame)
                y_hor_virstride = uv_hor_virstride = (mpp_frame_get_hor_stride(ref_frame) >> 4);
            else
                y_hor_virstride = uv_hor_virstride = (mpp_align_128_odd_plus_64((ref_frame_width_y * bit_depth) >> 3) >> 4);
        }
        if (ref_frame)
            y_virstride = y_hor_virstride * mpp_frame_get_ver_stride(ref_frame);
        else
            y_virstride = y_hor_virstride * mpp_align_64(ref_frame_height_y);

        if (ref_frame_idx < 0x7f) {
            mpp_buf_slot_get_prop(cfg->frame_slots, ref_frame_idx, SLOT_BUFFER, &framebuf);
            if (hw_ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, ref_frame_idx);
                framebuf = origin_buf->buf[0];
            }

            switch (i) {
            case 0: {
                regs->comm_paras.reg83_ref0_hor_virstride = y_hor_virstride;
                regs->comm_paras.reg84_ref0_raster_uv_hor_virstride = uv_hor_virstride;
                regs->comm_paras.reg85_ref0_virstride = y_virstride;
            } break;
            case 1: {
                regs->comm_paras.reg86_ref1_hor_virstride = y_hor_virstride;
                regs->comm_paras.reg87_ref1_raster_uv_hor_virstride = uv_hor_virstride;
                regs->comm_paras.reg88_ref1_virstride = y_virstride;
            } break;
            case 2: {
                regs->comm_paras.reg89_ref2_hor_virstride = y_hor_virstride;
                regs->comm_paras.reg90_ref2_raster_uv_hor_virstride = uv_hor_virstride;
                regs->comm_paras.reg91_ref2_virstride = y_virstride;
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
                reg_ref_base[i] = regs->comm_addrs.reg168_decout_base;
                reg_payload_ref_base[i] = regs->comm_addrs.reg168_decout_base;
            }
            mv_buf = hal_bufs_get_buf(hw_ctx->cmv_bufs, ref_frame_idx);
        } else {
            reg_ref_base[i] = regs->comm_addrs.reg168_decout_base;
            reg_payload_ref_base[i] = regs->comm_addrs.reg168_decout_base;
        }
    }

    vdpu384b_init_ctrl_regs(regs, MPP_VIDEO_CodingVP9);

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

    vdpu38x_vp9d_rcb_setup(hal, pic_param, task, &regs->comm_addrs.rcb_regs,
                           vdpu384b_vp9d_rcb_calc);
    vdpu38x_setup_statistic(&regs->ctrl_regs);
    // whether need update counts
    if (pic_param->refresh_frame_context && !pic_param->parallelmode) {
        task->dec.flags.wait_done = 1;
    }

    {
        //scale down config
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output,
                              SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(cfg->frame_slots, task->dec.output,
                              SLOT_FRAME_PTR, &mframe);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->comm_addrs.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            origin_buf = hal_bufs_get_buf(hw_ctx->origin_bufs, task->dec.output);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->comm_addrs.reg168_decout_base = fd;
            regs->comm_addrs.reg169_error_ref_base = fd;
            regs->comm_addrs.reg192_payload_st_cur_base = fd;
            regs->comm_addrs.reg194_payload_st_error_ref_base = fd;
            vdpu38x_setup_down_scale(mframe, cfg->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->comm_addrs.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            vdpu38x_setup_down_scale(mframe, cfg->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return MPP_OK;
}

static MPP_RET hal_vp9d_vdpu384b_start(void *hal, HalTaskInfo *task)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu38xVp9dCtx *hw_ctx = (Vdpu38xVp9dCtx*)p_hal->hw_ctx;
    Vdpu38xRegSet *hw_regs = (Vdpu38xRegSet *)hw_ctx->hw_regs;
    MppDev dev = p_hal->cfg->dev;
    MPP_RET ret = MPP_OK;

    if (p_hal->fast_mode) {
        RK_S32 index = task->dec.reg_index;

        hw_regs = (Vdpu38xRegSet *)hw_ctx->g_buf[index].hw_regs;
    }

    mpp_assert(hw_regs);

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->ctrl_regs;
        wr_cfg.size = sizeof(hw_regs->ctrl_regs);
        wr_cfg.offset = VDPU38X_OFF_CTRL_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->comm_paras;
        wr_cfg.size = sizeof(hw_regs->comm_paras);
        wr_cfg.offset = VDPU38X_OFF_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->comm_addrs;
        wr_cfg.size = sizeof(hw_regs->comm_addrs);
        wr_cfg.offset = VDPU38X_OFF_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(hw_regs->ctrl_regs.reg15);
        rd_cfg.offset = VDPU38X_OFF_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
            rd_cfg.reg = &hw_regs->statistic_regs;
            rd_cfg.size = sizeof(hw_regs->statistic_regs);
            rd_cfg.offset = VDPU38X_OFF_COM_STATISTIC_REGS_VDPU384B;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        // rcb info for sram
        vdpu38x_rcb_set_info(hw_ctx->rcb_ctx, dev);

        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}

static MPP_RET hal_vp9d_vdpu384b_wait(void *hal, HalTaskInfo *task)
{
    HalVp9dCtx *p_hal = (HalVp9dCtx*)hal;
    Vdpu38xVp9dCtx *hw_ctx = (Vdpu38xVp9dCtx*)p_hal->hw_ctx;
    Vdpu38xRegSet *hw_regs = (Vdpu38xRegSet *)hw_ctx->hw_regs;
    MPP_RET ret = MPP_OK;

    if (p_hal->fast_mode)
        hw_regs = (Vdpu38xRegSet *)hw_ctx->g_buf[task->dec.reg_index].hw_regs;

    mpp_assert(hw_regs);

    ret = mpp_dev_ioctl(p_hal->cfg->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "cabac_update_probe.dat";
        DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)task->dec.syntax.data;
        RK_U32 frame_ctx_id = pic_param->frame_context_idx;
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                  (void *)mpp_buffer_get_ptr(hw_ctx->prob_loop_base[frame_ctx_id]),
                                  8 * 152 * 16, 128, 0, 0);
    }
    {
        char *cur_fname = "segid_last.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        if (!cur_last_segid_flag)
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                      (void *)mpp_buffer_get_ptr(hw_ctx->segid_cur_base),
                                      8 * 1559 * 8, 64, 0, 0);
        else
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                      (void *)mpp_buffer_get_ptr(hw_ctx->segid_last_base),
                                      8 * 1559 * 8, 64, 0, 0);
    }
    {
        char *cur_fname = "segid_cur.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        if (cur_last_segid_flag)
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                      (void *)mpp_buffer_get_ptr(hw_ctx->segid_cur_base),
                                      8 * 1559 * 8, 64, 0, 0);
        else
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                      (void *)mpp_buffer_get_ptr(hw_ctx->segid_last_base),
                                      8 * 1559 * 8, 64, 0, 0);
    }
#endif

    if (hal_vp9d_debug & HAL_VP9D_DBG_REG) {
        RK_U32 *p = (RK_U32 *)hw_regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(hw_regs->ctrl_regs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + VDPU38X_OFF_CTRL_REGS, *p++);
        for (i = 0; i < sizeof(hw_regs->comm_paras) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + VDPU38X_OFF_CODEC_PARAS_REGS, *p++);
        for (i = 0; i < sizeof(hw_regs->comm_addrs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + VDPU38X_OFF_COMMON_ADDR_REGS, *p++);
        for (i = 0; i < sizeof(hw_regs->statistic_regs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + VDPU38X_OFF_COM_STATISTIC_REGS_VDPU384B, *p++);

        mpp_assert(hw_regs->statistic_regs.reg312.rcb_rd_sum_chk ==
                   hw_regs->statistic_regs.reg312.rcb_wr_sum_chk);
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

        mpp_buf_slot_get_prop(p_hal->cfg->frame_slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

    if (p_hal->fast_mode) {
        hw_ctx->g_buf[task->dec.reg_index].use_flag = 0;
    }

    (void)task;
    return ret;
}

const MppHalApi hal_vp9d_vdpu384b = {
    .name     = "vp9d_vdpu384b",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingVP9,
    .ctx_size = sizeof(HalVp9dCtx),
    .flag     = 0,
    .init     = hal_vp9d_vdpu384b_init,
    .deinit   = hal_vp9d_vdpu38x_deinit,
    .reg_gen  = hal_vp9d_vdpu384b_gen_regs,
    .start    = hal_vp9d_vdpu384b_start,
    .wait     = hal_vp9d_vdpu384b_wait,
    .reset    = hal_vp9d_vdpu38x_reset,
    .flush    = hal_vp9d_vdpu38x_flush,
    .control  = hal_vp9d_vdpu38x_control,
    .client   = VPU_CLIENT_RKVDEC,
    .soc_type = {
        ROCKCHIP_SOC_RK3572,
        ROCKCHIP_SOC_RK3538,
        ROCKCHIP_SOC_RK3539,
        ROCKCHIP_SOC_BUTT
    },
};

MPP_DEC_HAL_API_REGISTER(hal_vp9d_vdpu384b)
