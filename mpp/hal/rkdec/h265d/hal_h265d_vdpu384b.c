/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h265d_vdpu384b"

#include "mpp_buffer_impl.h"

#include "vdpu38x_com.h"
#include "h265d_syntax.h"
#include "hal_h265d_debug.h"
#include "hal_h265d_ctx.h"
#include "hal_h265d_com.h"
#include "hal_h265d_vdpu384b.h"

#define PPS_SIZE                        (112 * 64) //(96x64)

#define SPSPPS_ALIGNED_SIZE             (MPP_ALIGN(2182 + 64, 128) / 8) // byte, 2182 bit + Reserve 64
#define SCALIST_ALIGNED_SIZE            (MPP_ALIGN(81 * 1360, SZ_4K))
#define INFO_BUFFER_SIZE                (SPSPPS_ALIGNED_SIZE + SCALIST_ALIGNED_SIZE)
#define ALL_BUFFER_SIZE(cnt)            (INFO_BUFFER_SIZE *cnt)

#define SPSPPS_OFFSET(pos)              (INFO_BUFFER_SIZE * pos)
#define SCALIST_OFFSET(pos)             (SPSPPS_OFFSET(pos) + SPSPPS_ALIGNED_SIZE)

#define pocdistance(a, b)               (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))

static MPP_RET hal_h265d_vdpu384b_init(void *hal, MppHalCfg *cfg)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;

    mpp_slots_set_prop(reg_ctx->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(reg_ctx->slots, SLOTS_VER_ALIGN, mpp_align_8);

    reg_ctx->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    if (reg_ctx->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_ctx->scaling_rk = mpp_calloc(scalingFactor_t, 1);
    reg_ctx->pps_buf = mpp_calloc(RK_U8, SPSPPS_ALIGNED_SIZE);
    reg_ctx->pps_buf_sz = SPSPPS_ALIGNED_SIZE;

    if (reg_ctx->scaling_rk == NULL) {
        mpp_err("scaling_rk alloc fail");
        return MPP_ERR_MALLOC;
    }

    if (reg_ctx->group == NULL) {
        ret = mpp_buffer_group_get_internal(&reg_ctx->group, MPP_BUFFER_TYPE_ION);
        if (ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    {
        RK_U32 max_cnt = reg_ctx->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
        RK_U32 i = 0;

        //!< malloc buffers
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->bufs, ALL_BUFFER_SIZE(max_cnt));
        if (ret) {
            mpp_err("h265d mpp_buffer_get failed\n");
            return ret;
        }

        reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
        for (i = 0; i < max_cnt; i++) {
            reg_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu38xRegSet));
            reg_ctx->offset_spspps[i] = SPSPPS_OFFSET(i);
            reg_ctx->offset_sclst[i] = SCALIST_OFFSET(i);
        }

        mpp_buffer_attach_dev(reg_ctx->bufs, reg_ctx->dev);
    }

    if (!reg_ctx->fast_mode) {
        reg_ctx->hw_regs = reg_ctx->g_buf[0].hw_regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu38x_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

    vdpu38x_rcb_calc_init((Vdpu38xRcbCtx **)&reg_ctx->rcb_ctx);

    return MPP_OK;
}

static MPP_RET vdpu384b_h265d_rcb_calc(void *context, RK_U32 *total_size)
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
    vdpu38x_rcb_reg_info_update(ctx, RCB_STRMD_ON_ROW, 142, cur_bit_size);

    /* RCB_INTER_IN_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(8, in_tl_row) * 174;
    vdpu38x_rcb_reg_info_update(ctx, RCB_INTER_IN_ROW, 144, cur_bit_size);

    /* RCB_INTER_ON_ROW */
    cur_bit_size = 0;
    cur_bit_size = MPP_DIVUP(8, on_tl_row) * 174;
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
    cur_bit_size = MPP_ROUNDUP(64, in_tl_row) * (1.2 * bit_depth + 0.5 )
                   * (7.5 + 5 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW,  154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.2 * bit_depth + 0.5)
                   * (7.5 + 5 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_ROW, 156, cur_bit_size);

    /* RCB_FLTD_ON_COL */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_col_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.6 * bit_depth + 0.5)
                   * (16.5 + 5.5 * cur_uv_para);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_ON_COL, 158, cur_bit_size);

    /* RCB_FLTD_UPSC_ON_COL */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_UPSC_ON_COL, 160, cur_bit_size);

    *total_size = vdpu38x_rcb_get_total_size(ctx);

    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu384b_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_S32 i = 0;
    RK_S32 log2_min_cb_size;
    RK_S32 width, height;
    Vdpu38xRegSet *hw_regs;
    RK_S32 ret = MPP_SUCCESS;
    MppBuffer streambuf = NULL;
    RK_S32 aglin_offset = 0;
    RK_S32 valid_ref = -1;
    MppBuffer framebuf = NULL;
    HalBuf *mv_buf = NULL;
    RK_S32 fd = -1;
    RK_U32 mv_size = 0;
    RK_S32 distance = INT_MAX;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;

    (void) fd;
    if (syn->dec.flags.parse_err ||
        (syn->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    h265d_dxva2_picture_context_t *dxva_ctx = (h265d_dxva2_picture_context_t *)syn->dec.syntax.data;
    HalBuf *origin_buf = NULL;

    if (reg_ctx->fast_mode) {
        for (i = 0; i < VDPU_FAST_REG_SET_CNT; i++) {
            if (!reg_ctx->g_buf[i].use_flag) {
                syn->dec.reg_index = i;

                reg_ctx->spspps_offset = reg_ctx->offset_spspps[i];
                reg_ctx->sclst_offset = reg_ctx->offset_sclst[i];

                reg_ctx->hw_regs = reg_ctx->g_buf[i].hw_regs;
                reg_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == VDPU_FAST_REG_SET_CNT) {
            mpp_err("hevc rps buf all used");
            return MPP_ERR_NOMEM;
        }
    }

    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

#ifdef DUMP_VDPU38X_DATAS
    {
        memset(vdpu38x_dump_cur_dir, 0, sizeof(vdpu38x_dump_cur_dir));
        sprintf(vdpu38x_dump_cur_dir, "hevc/Frame%04d", vdpu38x_dump_cur_frm);
        if (access(vdpu38x_dump_cur_dir, 0)) {
            if (mkdir(vdpu38x_dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", vdpu38x_dump_cur_dir);
        }
        vdpu38x_dump_cur_frm++;
    }
#endif

    /* output pps */
    hw_regs = (Vdpu38xRegSet*)reg_ctx->hw_regs;
    memset(hw_regs, 0, sizeof(Vdpu38xRegSet));

    if (NULL == reg_ctx->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }


    log2_min_cb_size = dxva_ctx->pp.log2_min_luma_coding_block_size_minus3 + 3;
    width = (dxva_ctx->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_ctx->pp.PicHeightInMinCbsY << log2_min_cb_size);
    mv_size = hal_h265d_avs2d_calc_mv_size(width, height, 1 << log2_min_cb_size) * 2;

    if (reg_ctx->cmv_bufs == NULL || reg_ctx->mv_size < mv_size) {
        size_t size = mv_size;

        if (reg_ctx->cmv_bufs) {
            hal_bufs_deinit(reg_ctx->cmv_bufs);
            reg_ctx->cmv_bufs = NULL;
        }

        hal_bufs_init(&reg_ctx->cmv_bufs);
        if (reg_ctx->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_ERR_NULL_PTR;
        }

        reg_ctx->mv_size = mv_size;
        reg_ctx->mv_count = mpp_buf_slot_get_count(reg_ctx->slots);
        hal_bufs_setup(reg_ctx->cmv_bufs, reg_ctx->mv_count, 1, &size);
    }

    {
        MppFrame mframe = NULL;
        MppFrameFormat fmt = 0;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;
        RK_U32 uv_virstride_tile = 0;
        RK_U32 chroma_fmt_idc = dxva_ctx->pp.chroma_format_idc;
        RK_U32 fbc_hdr_stride = 0;
        RK_U32 fbc_head_stride = 0;
        RK_U32 fbc_pld_stride = 0;
        RK_U32 fbc_offset = 0;
        RK_U32 tile4x4_coeff = 0;

        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        /* for 8K downscale mode*/
        if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
            reg_ctx->origin_bufs == NULL) {
            vdpu38x_setup_scale_origin_bufs(mframe, &reg_ctx->origin_bufs,
                                            mpp_buf_slot_get_count(reg_ctx->slots));
        }

        fmt = mpp_frame_get_fmt(mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        uv_virstride = hor_virstride;
        y_virstride = ver_virstride * hor_virstride;
        fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);

        if (chroma_fmt_idc == 3)
            uv_virstride *= 2;
        if (chroma_fmt_idc == 3 || chroma_fmt_idc == 2)
            uv_virstride_tile = uv_virstride * ver_virstride;
        else
            uv_virstride_tile = uv_virstride * ver_virstride / 2;
        if (MPP_FRAME_FMT_IS_AFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            hw_regs->ctrl_regs.reg9.pp_output_mode = 3;
            hw_regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            hw_regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            hw_regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
        } else if (MPP_FRAME_FMT_IS_RKFBC(fmt)) {
            vdpu38x_get_fbc_off(mframe, &fbc_head_stride, &fbc_pld_stride, &fbc_offset);

            hw_regs->ctrl_regs.reg9.pp_output_mode = 2;
            hw_regs->comm_paras.reg68_pp_m_hor_stride = fbc_head_stride;
            hw_regs->comm_paras.reg69_pp_m_uv_hor_stride = fbc_pld_stride;
            hw_regs->comm_addrs.reg193_fbc_payload_offset = fbc_offset;
        } else if (MPP_FRAME_FMT_IS_TILE(fmt)) {
            if (vdpu38x_get_tile4x4_h_stride_coeff(fmt, &tile4x4_coeff)) {
                mpp_err("get tile 4x4 coeff failed\n");
                return MPP_NOK;
            }
            hw_regs->ctrl_regs.reg9.pp_output_mode = 1;
            hw_regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride * tile4x4_coeff / 16;
            hw_regs->comm_paras.reg70_pp_m_y_virstride = (y_virstride + uv_virstride_tile) / 16;
        } else {
            hw_regs->ctrl_regs.reg9.pp_output_mode = 0;
            hw_regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride >> 4;
            hw_regs->comm_paras.reg69_pp_m_uv_hor_stride = uv_virstride >> 4;
            hw_regs->comm_paras.reg70_pp_m_y_virstride = y_virstride >> 4;
        }
        /* error */
        hw_regs->comm_paras.reg80_error_ref_hor_virstride = hw_regs->comm_paras.reg68_pp_m_hor_stride;
        hw_regs->comm_paras.reg81_error_ref_raster_uv_hor_virstride = hw_regs->comm_paras.reg69_pp_m_uv_hor_stride;
        hw_regs->comm_paras.reg82_error_ref_virstride = hw_regs->comm_paras.reg70_pp_m_y_virstride;
    }
    mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);

    if (reg_ctx->origin_bufs) {
        origin_buf = hal_bufs_get_buf(reg_ctx->origin_bufs,
                                      dxva_ctx->pp.CurrPic.Index7Bits);
        framebuf = origin_buf->buf[0];
    }

    hw_regs->comm_addrs.reg168_decout_base = mpp_buffer_get_fd(framebuf); //just index need map
    hw_regs->comm_addrs.reg169_error_ref_base = mpp_buffer_get_fd(framebuf);
    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/
    if (!hw_regs->comm_addrs.reg168_decout_base)
        return 0;

    fd =  mpp_buffer_get_fd(framebuf);
    hw_regs->comm_addrs.reg168_decout_base = fd;
    hw_regs->comm_addrs.reg192_payload_st_cur_base = fd;
    mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_ctx->pp.CurrPic.Index7Bits);

    hw_regs->comm_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "colmv_cur_frame.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path,
                                  (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                                  mpp_buffer_get_size(mv_buf->buf[0]), 64, 0, 0);
    }
#endif

    mpp_buf_slot_get_prop(reg_ctx->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);
    if ( dxva_ctx->bitstream == NULL) {
        dxva_ctx->bitstream = mpp_buffer_get_ptr(streambuf);
    }

#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "stream_in_128bit.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(streambuf),
                                  mpp_buffer_get_size(streambuf), 128, 0, 0);
    }
#endif

    hw_regs->comm_addrs.reg128_strm_base = mpp_buffer_get_fd(streambuf);
    hw_regs->comm_paras.reg66_stream_len = ((dxva_ctx->bitstream_size + 15) & (~15)) + 64;
    hw_regs->comm_addrs.reg129_stream_buf_st_base = mpp_buffer_get_fd(streambuf);
    hw_regs->comm_addrs.reg130_stream_buf_end_base = mpp_buffer_get_fd(streambuf);
    mpp_dev_set_reg_offset(reg_ctx->dev, 130, mpp_buffer_get_size(streambuf));
    aglin_offset =  hw_regs->comm_paras.reg66_stream_len - dxva_ctx->bitstream_size;
    if (aglin_offset > 0)
        memset((void *)(dxva_ctx->bitstream + dxva_ctx->bitstream_size), 0, aglin_offset);

    vdpu384b_init_ctrl_regs(hw_regs, MPP_VIDEO_CodingHEVC);

    valid_ref = hw_regs->comm_addrs.reg168_decout_base;
    reg_ctx->error_index[syn->dec.reg_index] = dxva_ctx->pp.CurrPic.Index7Bits;

    hw_regs->comm_addrs.reg169_error_ref_base = valid_ref;
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_ctx->pp.RefPicList); i++) {
        if (dxva_ctx->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_ctx->pp.RefPicList[i].bPicEntry != 0x7f) {

            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(reg_ctx->slots,
                                  dxva_ctx->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);
            mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);
            if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                origin_buf = hal_bufs_get_buf(reg_ctx->origin_bufs,
                                              dxva_ctx->pp.RefPicList[i].Index7Bits);
                framebuf = origin_buf->buf[0];
            }
            if (framebuf != NULL) {
                hw_regs->comm_addrs.reg170_185_ref_base[i] = mpp_buffer_get_fd(framebuf);
                hw_regs->comm_addrs.reg195_210_payload_st_ref_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->comm_addrs.reg170_185_ref_base[i];
                if ((pocdistance(dxva_ctx->pp.PicOrderCntValList[i], dxva_ctx->pp.current_poc) < distance)
                    && (!mpp_frame_get_errinfo(mframe))) {

                    distance = pocdistance(dxva_ctx->pp.PicOrderCntValList[i], dxva_ctx->pp.current_poc);
                    hw_regs->comm_addrs.reg169_error_ref_base = hw_regs->comm_addrs.reg170_185_ref_base[i];
                    reg_ctx->error_index[syn->dec.reg_index] = dxva_ctx->pp.RefPicList[i].Index7Bits;
                    hw_regs->ctrl_regs.reg16.error_proc_disable = 1;
                }
            } else {
                hw_regs->comm_addrs.reg170_185_ref_base[i] = valid_ref;
                hw_regs->comm_addrs.reg195_210_payload_st_ref_base[i] = valid_ref;
            }

            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_ctx->pp.RefPicList[i].Index7Bits);
            hw_regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }
    }

    if ((reg_ctx->error_index[syn->dec.reg_index] == dxva_ctx->pp.CurrPic.Index7Bits) &&
        !dxva_ctx->pp.IntraPicFlag && !reg_ctx->cfg->base.disable_error) {
        h265h_dbg(H265H_DBG_TASK_ERR, "current frm may be err, should skip process");
        syn->dec.flags.ref_err = 1;
        return MPP_OK;
    }

    /* pps */
    hw_regs->comm_addrs.reg131_gbl_base = reg_ctx->bufs_fd;
    hw_regs->comm_paras.reg67_global_len = SPSPPS_ALIGNED_SIZE / 16;
    mpp_dev_set_reg_offset(reg_ctx->dev, 131, reg_ctx->spspps_offset);

    hal_h265d_vdpu38x_output_pps_packet(hal, syn->dec.syntax.data,
                                        &hw_regs->comm_addrs.reg132_scanlist_addr);

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_ctx->pp.RefPicList); i++) {

        if (dxva_ctx->pp.RefPicList[i].bPicEntry != 0xff &&
            dxva_ctx->pp.RefPicList[i].bPicEntry != 0x7f) {
            MppFrame mframe = NULL;

            mpp_buf_slot_get_prop(reg_ctx->slots,
                                  dxva_ctx->pp.RefPicList[i].Index7Bits,
                                  SLOT_BUFFER, &framebuf);

            mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.RefPicList[i].Index7Bits,
                                  SLOT_FRAME_PTR, &mframe);

            if (framebuf == NULL || mpp_frame_get_errinfo(mframe)) {
                mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, reg_ctx->error_index[syn->dec.reg_index]);
                hw_regs->comm_addrs.reg170_185_ref_base[i] = hw_regs->comm_addrs.reg169_error_ref_base;
                hw_regs->comm_addrs.reg195_210_payload_st_ref_base[i] = hw_regs->comm_addrs.reg169_error_ref_base;
                hw_regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        } else {
            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, reg_ctx->error_index[syn->dec.reg_index]);
            hw_regs->comm_addrs.reg170_185_ref_base[i] = hw_regs->comm_addrs.reg169_error_ref_base;
            hw_regs->comm_addrs.reg195_210_payload_st_ref_base[i] = hw_regs->comm_addrs.reg169_error_ref_base;
            hw_regs->comm_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }
    }

    vdpu38x_h265d_rcb_setup(hal, dxva_ctx, syn, width, height,
                            &hw_regs->comm_addrs.rcb_regs,
                            vdpu384b_h265d_rcb_calc);
    vdpu38x_setup_statistic(&hw_regs->ctrl_regs);
    mpp_buffer_sync_end(reg_ctx->bufs);

    {
        //scale down config
        MppFrame mframe = NULL;
        MppBuffer mbuffer = NULL;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                              SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            hw_regs->comm_addrs.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            origin_buf = hal_bufs_get_buf(reg_ctx->origin_bufs, dxva_ctx->pp.CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            hw_regs->comm_addrs.reg168_decout_base = fd;
            hw_regs->comm_addrs.reg192_payload_st_cur_base = fd;
            hw_regs->comm_addrs.reg169_error_ref_base = fd;
            vdpu38x_setup_down_scale(mframe, reg_ctx->dev, &hw_regs->ctrl_regs, (void*)&hw_regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            hw_regs->comm_addrs.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            vdpu38x_setup_down_scale(mframe, reg_ctx->dev, &hw_regs->ctrl_regs, (void*)&hw_regs->comm_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            hw_regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return ret;
}

static MPP_RET hal_h265d_vdpu384b_start(void *hal, HalTaskInfo *task)
{
    RK_U8* p = NULL;
    Vdpu38xRegSet *hw_regs = NULL;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_S32 index =  task->dec.reg_index;
    RK_U32 i;
    MPP_RET ret = MPP_OK;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_ctx->fast_mode) {
        p = (RK_U8*)reg_ctx->g_buf[index].hw_regs;
        hw_regs = ( Vdpu38xRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        p = (RK_U8*)reg_ctx->hw_regs;
        hw_regs = ( Vdpu38xRegSet *)reg_ctx->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_h265d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->ctrl_regs;
        wr_cfg.size = sizeof(hw_regs->ctrl_regs);
        wr_cfg.offset = VDPU38X_OFF_CTRL_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->comm_paras;
        wr_cfg.size = sizeof(hw_regs->comm_paras);
        wr_cfg.offset = VDPU38X_OFF_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->comm_addrs;
        wr_cfg.size = sizeof(hw_regs->comm_addrs);
        wr_cfg.offset = VDPU38X_OFF_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(hw_regs->ctrl_regs.reg15);
        rd_cfg.offset = VDPU38X_OFF_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        if (hal_h265d_debug & H265H_DBG_REG) {
            rd_cfg.reg = &hw_regs->statistic_regs;
            rd_cfg.size = sizeof(hw_regs->statistic_regs);
            rd_cfg.offset = VDPU38X_OFF_COM_STATISTIC_REGS_VDPU384B;
            ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu38x_set_rcbinfo(reg_ctx->dev, (VdpuRcbInfo*)reg_ctx->rcb_info);

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}


static MPP_RET hal_h265d_vdpu384b_wait(void *hal, HalTaskInfo *task)
{
    RK_S32 index =  task->dec.reg_index;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_U8* p = NULL;
    Vdpu38xRegSet *hw_regs = NULL;
    RK_S32 i;
    MPP_RET ret = MPP_OK;

    if (reg_ctx->fast_mode) {
        hw_regs = ( Vdpu38xRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        hw_regs = ( Vdpu38xRegSet *)reg_ctx->hw_regs;
    }

    p = (RK_U8*)hw_regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        goto ERR_PROC;
    }

    ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

ERR_PROC:
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        (!hw_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
        hw_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
        hw_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta) {
        if (!reg_ctx->fast_mode) {
            if (reg_ctx->dec_cb)
                mpp_callback(reg_ctx->dec_cb, &task->dec);
        } else {
            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.output,
                                  SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                reg_ctx->fast_mode_err_found = 1;
                mpp_frame_set_errinfo(mframe, 1);
            }
        }
    } else {
        if (reg_ctx->fast_mode && reg_ctx->fast_mode_err_found) {
            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                if (task->dec.refer[i] >= 0) {
                    MppFrame frame_ref = NULL;

                    mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.refer[i],
                                          SLOT_FRAME_PTR, &frame_ref);
                    h265h_dbg(H265H_DBG_FAST_ERR, "refer[%d] %d frame %p\n",
                              i, task->dec.refer[i], frame_ref);
                    if (frame_ref && mpp_frame_get_errinfo(frame_ref)) {
                        MppFrame frame_out = NULL;
                        mpp_buf_slot_get_prop(reg_ctx->slots, task->dec.output,
                                              SLOT_FRAME_PTR, &frame_out);
                        mpp_frame_set_errinfo(frame_out, 1);
                        break;
                    }
                }
            }
        }
    }
    if (hal_h265d_debug & H265H_DBG_REG) {
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

    if (reg_ctx->fast_mode) {
        reg_ctx->g_buf[index].use_flag = 0;
    }

    return ret;
}

const MppHalApi hal_h265d_vdpu384b = {
    .name = "h265d_vdpu384b",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_vdpu384b_init,
    .deinit = hal_h265d_vdpu38x_deinit,
    .reg_gen = hal_h265d_vdpu384b_gen_regs,
    .start = hal_h265d_vdpu384b_start,
    .wait = hal_h265d_vdpu384b_wait,
    .reset = hal_h265d_vdpu_reset,
    .flush = hal_h265d_vdpu_flush,
    .control = hal_h265d_vdpu38x_control,
};
