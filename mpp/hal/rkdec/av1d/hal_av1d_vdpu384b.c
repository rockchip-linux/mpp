/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_av1d_vdpu384b"

#include "mpp_buffer_impl.h"

#include "hal_av1d_com.h"
#include "hal_av1d_ctx.h"
#include "vdpu38x_com.h"

#define VDPU384B_UNCMPS_HEADER_SIZE          (MPP_ALIGN(5160 + 64, 128) / 8) // byte, 5160 bit + Reserve 64
#define VDPU384B_UNCMPS_HEADER_OFFSET_BASE   (0)
#define VDPU384B_INFO_ELEM_SIZE              (VDPU384B_UNCMPS_HEADER_SIZE)
#define VDPU384B_UNCMPS_HEADER_OFFSET(idx)   (VDPU384B_INFO_ELEM_SIZE * idx + VDPU384B_UNCMPS_HEADER_OFFSET_BASE)
#define VDPU384B_INFO_BUF_SIZE(cnt)          (VDPU384B_INFO_ELEM_SIZE * cnt)

static MPP_RET hal_av1d_alloc_res(void *hal)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    void *cdf_ptr;
    Vdpu38xAv1dRegCtx *reg_ctx = NULL;
    RK_U32 i = 0;
    MPP_RET ret = MPP_OK;

    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, p_hal->api->ctx_size));
    reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, MPP_ALIGN(VDPU384B_INFO_BUF_SIZE(max_cnt), SZ_2K)));
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);


    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu38xRegSet, 1);
        memset(reg_ctx->reg_buf[i].regs, 0, sizeof(Vdpu38xRegSet));
        reg_ctx->uncmps_offset[i] = VDPU384B_UNCMPS_HEADER_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->offset_uncomps = reg_ctx->uncmps_offset[0];
    }

    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->cdf_rd_def_base, MPP_ALIGN(sizeof(g_av1d_default_prob), 2048)));
    mpp_buffer_attach_dev(reg_ctx->cdf_rd_def_base, p_hal->dev);
    cdf_ptr = mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base);
    memcpy(cdf_ptr, g_av1d_default_prob, sizeof(g_av1d_default_prob));
    mpp_buffer_sync_end(reg_ctx->cdf_rd_def_base);

__RETURN:
    return ret;
__FAILED:
    return ret;
}

MPP_RET vdpu384b_av1d_init(void *hal, MppHalCfg *cfg)
{
    (void) cfg;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu38xAv1dRegCtx *reg_ctx = NULL;
    MPP_RET ret = MPP_OK;

    INP_CHECK(ret, NULL == p_hal);

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

static MPP_RET vdpu384b_rcb_av1_calc_rcb_bufs(void *context, RK_U32 *total_size)
{
    Vdpu38xRcbCtx *ctx = (Vdpu38xRcbCtx *)context;
    RK_FLOAT cur_bit_size = 0;
    RK_U32 cur_uv_para = 0;
    RK_U32 bit_depth = ctx->bit_depth;
    RK_U32 in_tl_row = 0;
    RK_U32 on_tl_row = 0;
    RK_U32 on_tl_col = 0;
    Vdpu38xFmt rcb_fmt;

    /* vdpu384b fix 10bit */
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
     * Versions with issues: swan1126b (384a version), shark/robin (384b version).
     */
    cur_bit_size = MPP_DIVUP(8, in_tl_row) * 100;
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_ON_ROW, 142, cur_bit_size);

    /* RCB_INTER_IN_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(64, in_tl_row) * 2752;
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_IN_ROW, 144, cur_bit_size);

    /* RCB_INTER_ON_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(64, on_tl_row) * 2752;
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
                   * (12.5 + 6 * cur_uv_para + 1.5 * ctx->lr_en);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW,  154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.2 * bit_depth + 0.5)
                   * (12.5 + 6 * cur_uv_para + 1.5 * ctx->lr_en);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_ROW, 156, cur_bit_size);

    /* RCB_FLTD_ON_COL */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_col_uv_coef_map[rcb_fmt];
    if (ctx->tile_dir == 0)
        cur_bit_size = MPP_ROUNDUP(64, on_tl_col) * (1.6 * bit_depth + 0.5) *
                       (14 + 7 * cur_uv_para + (14 + 12.5 * cur_uv_para) * ctx->lr_en +
                        ( ctx->upsc_en ? (8.5 + 7 * cur_uv_para) : (5 + 1 * cur_uv_para)));
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_COL, 158, cur_bit_size);

    /* RCB_FLTD_UPSC_ON_COL */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(64, on_tl_col) * bit_depth * 22;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_UPSC_ON_COL, 160, cur_bit_size);

    *total_size = vdpu38x_rcb_get_total_size(ctx);

    return MPP_OK;
}

static void vdpu384b_av1d_rcb_setup(Av1dHalCtx *p_hal, Vdpu38xRegSet *regs,
                                    HalTaskInfo *task, DXVA_PicParams_AV1 *dxva)
{
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    MppBuffer rcb_buf;
    RK_U32 i;

    /* update rcb info */
    {
        RcbTileInfo tl_info;
        MppFrame mframe;
        MppFrameFormat mpp_fmt;
        Vdpu38xFmt rcb_fmt;
        RK_U32 uses_lr = 0;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_fmt = mpp_frame_get_fmt(mframe);
        rcb_fmt = vdpu38x_fmt_mpp2hal(mpp_fmt);
        for (i = 0; i < (dxva->format.mono_chrome ? 1 : 3); i++)
            uses_lr |= (dxva->loop_filter.frame_restoration_type[i] != AV1_RESTORE_NONE) ? 1 : 0;

        vdpu38x_rcb_reset(reg_ctx->rcb_ctx);

        /* update general info */
        vdpu38x_rcb_set_pic_w(reg_ctx->rcb_ctx, dxva->width);
        vdpu38x_rcb_set_pic_h(reg_ctx->rcb_ctx, dxva->height);
        vdpu38x_rcb_set_fmt(reg_ctx->rcb_ctx, rcb_fmt);
        vdpu38x_rcb_set_bit_depth(reg_ctx->rcb_ctx, dxva->bitdepth);

        /* update cur spec info */
        vdpu38x_rcb_set_lr_en(reg_ctx->rcb_ctx, uses_lr ? 1 : 0);
        vdpu38x_rcb_set_upsc_en(reg_ctx->rcb_ctx, dxva->use_superres);

        /* add tile info */
        /* Simplify the calculation. */
        tl_info.lt_x = 0;
        tl_info.lt_y = 0;
        tl_info.w = dxva->width;
        tl_info.h = dxva->height;
        vdpu38x_rcb_set_tile_dir(reg_ctx->rcb_ctx, 0);
        vdpu38x_rcb_add_tile_info(reg_ctx->rcb_ctx, &tl_info);
        vdpu38x_rcb_register_calc_handle(reg_ctx->rcb_ctx, vdpu384b_rcb_av1_calc_rcb_bufs);
    }

    vdpu38x_rcb_calc_exec(reg_ctx->rcb_ctx, &reg_ctx->rcb_buf_size);

    for (i = 0; i < max_cnt; i++) {
        rcb_buf = reg_ctx->rcb_bufs[i];

        if (rcb_buf) {
            mpp_buffer_put(rcb_buf);
            reg_ctx->rcb_bufs[i] = NULL;
        }
        mpp_buffer_get(p_hal->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        reg_ctx->rcb_bufs[i] = rcb_buf;
    }

    rcb_buf = p_hal->fast_mode ? reg_ctx->rcb_bufs[task->dec.reg_index]
              : reg_ctx->rcb_bufs[0];
    vdpu38x_setup_rcb(reg_ctx->rcb_ctx, &regs->comm_addrs, p_hal->dev,
                      rcb_buf);

    return;
}

MPP_RET vdpu384b_av1d_gen_regs(void *hal, HalTaskInfo *task)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu38xAv1dRegCtx *ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu38xRegSet *regs;
    DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
    HalBuf *origin_buf = NULL;
    MppFrame mframe;
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

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
        vdpu38x_setup_scale_origin_bufs(mframe, &ctx->origin_bufs,
                                        mpp_buf_slot_get_count(p_hal->slots));
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

    vdpu384b_init_ctrl_regs(regs, MPP_VIDEO_CodingAV1);
    vdpu38x_setup_statistic(&regs->ctrl_regs);

    /* set reg -> pkt data */
    {
        MppBuffer mbuffer = NULL;

        /* uncompress header data */
        vdpu38x_av1d_uncomp_hdr(p_hal, dxva, (RK_U64 *)ctx->header_data, VDPU384B_UNCMPS_HEADER_SIZE / 8);
        memcpy((char *)ctx->bufs_ptr, (void *)ctx->header_data, VDPU384B_UNCMPS_HEADER_SIZE);
        regs->comm_paras.reg67_global_len = VDPU384B_UNCMPS_HEADER_SIZE / 16; // 128 bit as unit
        regs->comm_addrs.reg131_gbl_base = ctx->bufs_fd;
        // mpp_dev_set_reg_offset(p_hal->dev, 131, ctx->offset_uncomps);
#ifdef DUMP_VDPU38X_DATAS
        {
            char *cur_fname = "global_cfg.dat";
            memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
            sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
            vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, ctx->bufs_ptr,
                                      8 * regs->comm_paras.reg67_global_len * 16, 128, 0, 0);
        }
#endif
        // input strm
        p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
        regs->comm_paras.reg66_stream_len = MPP_ALIGN(p_hal->strm_len + 15, 128);
        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->comm_addrs.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        regs->comm_addrs.reg129_stream_buf_st_base = mpp_buffer_get_fd(mbuffer);
        regs->comm_addrs.reg130_stream_buf_end_base = mpp_buffer_get_fd(mbuffer);
        mpp_dev_set_reg_offset(p_hal->dev, 130, mpp_buffer_get_size(mbuffer));
        regs->comm_paras.reg65_strm_start_bit = (ctx->offset_uncomps & 0xf) * 8; // bit start to decode
        mpp_dev_set_reg_offset(p_hal->dev, 128, ctx->offset_uncomps & 0xfffffff0);
        /* error */
        regs->comm_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
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

    vdpu384b_av1d_rcb_setup(p_hal, regs, task, dxva);

    /* set reg -> para (stride, len) */
    {
        MppFrameFormat fmt;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;
        RK_U32 mapped_idx = 0;
        RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
        RK_U32 h = MPP_ALIGN(mpp_frame_get_height(mframe), 64);
        RK_U32 fbc_head_stride = 0;
        RK_U32 fbc_pld_stride = 0;
        RK_U32 fbc_offset = 0;
        RK_U32 tile4x4_coeff = 0;

        fmt = mpp_frame_get_fmt(mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        uv_virstride = hor_virstride * ver_virstride / 2;
        if (MPP_FRAME_FMT_IS_AFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            regs->ctrl_regs.reg9.pp_output_mode = 3;
            regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
        } else if (MPP_FRAME_FMT_IS_RKFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            regs->ctrl_regs.reg9.pp_output_mode = 2;
            regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
        } else if (MPP_FRAME_FMT_IS_TILE(fmt)) {
            if (vdpu38x_get_tile4x4_h_stride_coeff(fmt, &tile4x4_coeff)) {
                mpp_err("get tile 4x4 coeff failed\n");
                return MPP_NOK;
            }
            regs->ctrl_regs.reg9.pp_output_mode = 1;
            regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride * tile4x4_coeff >> 4;
            regs->comm_paras.reg70_pp_m_y_virstride = (y_virstride + uv_virstride) >> 4;
        } else {
            regs->ctrl_regs.reg9.pp_output_mode = 0;
            regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride >> 4;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = hor_virstride >> 4;
            regs->comm_paras.reg70_pp_m_y_virstride = y_virstride >> 4;
        }
        /* error */
        regs->comm_paras.reg80_error_ref_hor_virstride = regs->comm_paras.reg68_pp_m_hor_stride;
        regs->comm_paras.reg81_error_ref_raster_uv_hor_virstride = regs->comm_paras.reg69_pp_m_uv_hor_stride;
        regs->comm_paras.reg82_error_ref_virstride = regs->comm_paras.reg70_pp_m_y_virstride;

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_FRAME_PTR, &mframe);
                if (mframe) {
                    hor_virstride = mpp_frame_get_hor_stride(mframe);
                    ver_virstride = mpp_frame_get_ver_stride(mframe);
                    y_virstride = hor_virstride * ver_virstride;
                    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
                        vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);
                        regs->comm_paras.ref_stride[mapped_idx].hor_y_stride = fbc_head_stride;
                        regs->comm_paras.ref_stride[mapped_idx].hor_uv_stride = fbc_pld_stride;
                    } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                        hor_virstride = MPP_ALIGN(hor_virstride * 6, 16);
                        y_virstride += y_virstride / 2;
                        regs->comm_paras.ref_stride[mapped_idx].hor_y_stride = hor_virstride >> 4;
                        regs->comm_paras.ref_stride[mapped_idx].hor_uv_stride = hor_virstride >> 4;
                        regs->comm_paras.ref_stride[mapped_idx].y_stride = y_virstride >> 4;
                    } else {
                        regs->comm_paras.ref_stride[mapped_idx].hor_y_stride = hor_virstride >> 4;
                        regs->comm_paras.ref_stride[mapped_idx].hor_uv_stride = hor_virstride >> 4;
                        regs->comm_paras.ref_stride[mapped_idx].y_stride = y_virstride >> 4;
                    }
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
        regs->comm_addrs.reg168_decout_base = mpp_buffer_get_fd(mbuffer);
        regs->comm_addrs.reg192_payload_st_cur_base = mpp_buffer_get_fd(mbuffer);

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_BUFFER, &mbuffer);
                if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                    origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->frame_refs[mapped_idx].Index);
                    mbuffer = origin_buf->buf[0];
                }
                if (mbuffer) {
                    RK_U32 *reg_ref_base = NULL;
                    RK_U32 *reg_pld_base = NULL;

                    reg_ref_base = regs->comm_addrs.reg170_185_ref_base;
                    reg_ref_base[mapped_idx] = mpp_buffer_get_fd(mbuffer);
                    reg_pld_base = regs->comm_addrs.reg195_210_payload_st_ref_base;
                    reg_pld_base[mapped_idx] = mpp_buffer_get_fd(mbuffer);
                }
            }
        }

        HalBuf *mv_buf = NULL;
        vdpu38x_av1d_colmv_setup(p_hal, dxva);
        mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->CurrPic.Index7Bits);
        regs->comm_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_VDPU38X_DATAS
        memset(mpp_buffer_get_ptr(mv_buf->buf[0]), 0, mpp_buffer_get_size(mv_buf->buf[0]));
#endif
        for (i = 0; i < NUM_REF_FRAMES; i++) {
            if (dxva->frame_refs[i].Index != (CHAR)0xff && dxva->frame_refs[i].Index != 0x7f) {
                mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->frame_refs[i].Index);
                regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
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
                               &regs->comm_addrs.reg178_av1_coef_rd_base,
                               &regs->comm_addrs.reg179_av1_coef_wr_base,
                               &regs->comm_addrs.reg181_av1_segid_last_base,
                               &regs->comm_addrs.reg182_av1_segid_cur_base,
                               &regs->comm_addrs.reg184_av1_noncoef_rd_base,
                               &regs->comm_addrs.reg185_av1_noncoef_wr_base);
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
            regs->comm_addrs.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->comm_addrs.reg168_decout_base = fd;
            regs->comm_addrs.reg192_payload_st_cur_base = fd;
            regs->comm_addrs.reg169_error_ref_base = fd;
            vdpu38x_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->comm_addrs.reg133_scale_down_base = fd;
            vdpu38x_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
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

MPP_RET vdpu384b_av1d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MppDev dev;

    INP_CHECK(ret, NULL == p_hal);
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }

    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu38xRegSet *regs = p_hal->fast_mode ?
                          reg_ctx->reg_buf[task->dec.reg_index].regs :
                          reg_ctx->regs;
    dev = p_hal->dev;
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

        if (hal_av1d_debug & AV1D_DBG_REG) {
            rd_cfg.reg = &regs->statistic_regs;
            rd_cfg.size = sizeof(regs->statistic_regs);
            rd_cfg.offset = VDPU38X_OFF_COM_STATISTIC_REGS_VDPU384B;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu38x_set_rcbinfo(dev, (VdpuRcbInfo*)reg_ctx->rcb_buf_info);

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

MPP_RET vdpu384b_av1d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu38xAv1dRegCtx *reg_ctx = (Vdpu38xAv1dRegCtx *)p_hal->reg_ctx;
    Vdpu38xRegSet *p_regs = p_hal->fast_mode ?
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
    if (hal_av1d_debug & AV1D_DBG_REG) {
        mpp_assert(p_regs->statistic_regs.reg312.rcb_rd_sum_chk ==
                   p_regs->statistic_regs.reg312.rcb_wr_sum_chk);
    }

__SKIP_HARD:
    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_av1d_vdpu384b = {
    .name       = "av1d_vdpu384b",
    .type       = MPP_CTX_DEC,
    .coding     = MPP_VIDEO_CodingAV1,
    .ctx_size   = sizeof(Vdpu38xAv1dRegCtx) + VDPU384B_UNCMPS_HEADER_SIZE,
    .flag       = 0,
    .init       = vdpu384b_av1d_init,
    .deinit     = vdpu38x_av1d_deinit,
    .reg_gen    = vdpu384b_av1d_gen_regs,
    .start      = vdpu384b_av1d_start,
    .wait       = vdpu384b_av1d_wait,
    .reset      = vdpu38x_av1d_reset,
    .flush      = vdpu38x_av1d_flush,
    .control    = vdpu38x_av1d_control,
};
