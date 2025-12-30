/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_avs2d_vdpu384b"

#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "vdpu38x_com.h"
#include "hal_avs2d_global.h"
#include "hal_avs2d_vdpu384b.h"
#include "mpp_dec_cb_param.h"
#include "hal_avs2d_ctx.h"
#include "hal_avs2d_com.h"

#define MAX_REF_NUM                         (8)
#define AVS2_384B_SHPH_SIZE                 (MPP_ALIGN(1614 + 64, 128) / 8) // byte, 1614 bit + Reserve 64
#define AVS2_384B_SCALIST_SIZE              (80)             /* bytes */

#define AVS2_384B_SHPH_ALIGNED_SIZE         (MPP_ALIGN(AVS2_384B_SHPH_SIZE, SZ_4K))
#define AVS2_384B_SCALIST_ALIGNED_SIZE      (MPP_ALIGN(AVS2_384B_SCALIST_SIZE, SZ_4K))
#define AVS2_384B_STREAM_INFO_SET_SIZE      (AVS2_384B_SHPH_ALIGNED_SIZE + \
                                            AVS2_384B_SCALIST_ALIGNED_SIZE)
#define AVS2_ALL_TBL_BUF_SIZE(cnt)          (AVS2_384B_STREAM_INFO_SET_SIZE * (cnt))
#define AVS2_SHPH_OFFSET(pos)               (AVS2_384B_STREAM_INFO_SET_SIZE * (pos))
#define AVS2_SCALIST_OFFSET(pos)            (AVS2_SHPH_OFFSET(pos) + AVS2_384B_SHPH_ALIGNED_SIZE)

#define COLMV_COMPRESS_EN                   (1)
#define COLMV_BLOCK_SIZE                    (16)
#define COLMV_BYTES                         (16)

static MPP_RET vdpu384b_rcb_avs2_calc_rcb_bufs(void *context, RK_U32 *total_size)
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
    cur_bit_size = MPP_ROUNDUP(64, in_tl_row) * (1.2 * bit_depth + 0.5)
                   * (12 + 5 * cur_uv_para + 1.5 * ctx->alf_en);
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_IN_ROW, 152, cur_bit_size);

    /* RCB_FLTD_PROT_IN_ROW */
    cur_bit_size = 0;
    vdpu38x_rcb_reg_info_update(ctx, RCB_FLTD_PROT_IN_ROW,  154, cur_bit_size);

    /* RCB_FLTD_ON_ROW */
    cur_bit_size = 0;
    cur_uv_para = vdpu38x_filter_row_uv_coef_map[rcb_fmt];
    cur_bit_size = MPP_ROUNDUP(64, on_tl_row) * (1.2 * bit_depth + 0.5)
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

static void vdpu384b_avs2_rcb_setup(void *hal, Vdpu38xRegSet *regs, HalTaskInfo *task)
{
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp = &syntax->pp;
    Avs2dRkvRegCtx *reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    RK_S32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    MppBuffer rcb_buf = NULL;
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;

    /* update rcb info */
    {
        RcbTileInfo tl_info;
        MppFrame mframe;
        MppFrameFormat mpp_fmt;
        Vdpu38xFmt rcb_fmt;

        mpp_buf_slot_get_prop(p_hal->frame_slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_fmt = mpp_frame_get_fmt(mframe);
        rcb_fmt = vdpu38x_fmt_mpp2hal(mpp_fmt);

        vdpu38x_rcb_reset(reg_ctx->rcb_ctx);

        /* update general info */
        vdpu38x_rcb_set_pic_w(reg_ctx->rcb_ctx, pp->pic_width_in_luma_samples);
        vdpu38x_rcb_set_pic_h(reg_ctx->rcb_ctx, pp->pic_height_in_luma_samples);
        vdpu38x_rcb_set_fmt(reg_ctx->rcb_ctx, rcb_fmt);
        vdpu38x_rcb_set_bit_depth(reg_ctx->rcb_ctx, pp->bit_depth_luma_minus8 + 8);

        /* update cur spec info */
        vdpu38x_rcb_set_alf_en(reg_ctx->rcb_ctx, pp->adaptive_loop_filter_enable_flag);

        /* add tile info */
        /* Simplify the calculation. */
        tl_info.lt_x = 0;
        tl_info.lt_y = 0;
        tl_info.w = pp->pic_width_in_luma_samples;
        tl_info.h = pp->pic_height_in_luma_samples;
        vdpu38x_rcb_set_tile_dir(reg_ctx->rcb_ctx, 0);
        vdpu38x_rcb_add_tile_info(reg_ctx->rcb_ctx, &tl_info);
        vdpu38x_rcb_register_calc_handle(reg_ctx->rcb_ctx, vdpu384b_rcb_avs2_calc_rcb_bufs);
    }

    vdpu38x_rcb_calc_exec(reg_ctx->rcb_ctx, &reg_ctx->rcb_buf_size);

    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }

        ret = mpp_buffer_get(p_hal->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        if (ret)
            mpp_err_f("AVS2D mpp_buffer_group_get failed\n");

        reg_ctx->rcb_buf[i] = rcb_buf;
    }

    rcb_buf = p_hal->fast_mode ?
              reg_ctx->rcb_buf[task->dec.reg_index] : reg_ctx->rcb_buf[0];
    vdpu38x_setup_rcb(reg_ctx->rcb_ctx, &regs->comm_addrs, p_hal->dev, rcb_buf);
}

static MPP_RET fill_registers(Avs2dHalCtx_t *p_hal, Vdpu38xRegSet *regs, HalTaskInfo *task)
{
    MppFrame mframe = NULL;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    RefParams_Avs2d *refp = &syntax->refp;
    HalDecTask *task_dec  = &task->dec;
    HalBuf *mv_buf = NULL;
    RK_U32 i;
    MPP_RET ret = MPP_OK;

    mpp_buf_slot_get_prop(p_hal->frame_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);

    //!< caculate the yuv_frame_size
    {
        MppFrameFormat fmt;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;
        RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
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
            regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride * tile4x4_coeff / 16;
            regs->comm_paras.reg70_pp_m_y_virstride = (y_virstride + uv_virstride) / 16;
        } else {
            regs->ctrl_regs.reg9.pp_output_mode = 0;
            regs->comm_paras.reg68_pp_m_hor_stride = hor_virstride / 16;
            regs->comm_paras.reg69_pp_m_uv_hor_stride = hor_virstride / 16;
            regs->comm_paras.reg70_pp_m_y_virstride = y_virstride / 16;
        }
        /* error stride */
        regs->comm_paras.reg80_error_ref_hor_virstride = regs->comm_paras.reg68_pp_m_hor_stride;
        regs->comm_paras.reg81_error_ref_raster_uv_hor_virstride = regs->comm_paras.reg69_pp_m_uv_hor_stride;
        regs->comm_paras.reg82_error_ref_virstride = regs->comm_paras.reg70_pp_m_y_virstride;
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
    regs->comm_addrs.reg129_stream_buf_st_base = hal_avs2d_get_packet_fd(p_hal, task_dec->input);
    regs->comm_addrs.reg130_stream_buf_end_base = hal_avs2d_get_packet_fd(p_hal, task_dec->input);
    {
        MppBuffer mbuffer = NULL;

        mpp_buf_slot_get_prop(p_hal->packet_slots, task_dec->input, SLOT_BUFFER, &mbuffer);
        mpp_dev_set_reg_offset(p_hal->dev, 130, mpp_buffer_get_size(mbuffer));
    }
#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "stream_in.dat";
        MppBuffer strm_in_buf = NULL;

        mpp_buf_slot_get_prop(p_hal->packet_slots, task_dec->input, SLOT_BUFFER, &strm_in_buf);
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)mpp_buffer_get_ptr(strm_in_buf),
                                  8 * regs->comm_paras.reg66_stream_len, 128, 0, 0);
    }
#endif

    {
        //scale down config
        mpp_buf_slot_get_prop(p_hal->frame_slots, task_dec->output,
                              SLOT_FRAME_PTR, &mframe);
        if (mpp_frame_get_thumbnail_en(mframe)) {
            regs->comm_addrs.reg133_scale_down_base = regs->comm_addrs.reg168_decout_base;
            vdpu38x_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->comm_paras);
        } else {
            regs->ctrl_regs.reg9.scale_down_en = 0;
        }
    }

    return ret;
}

MPP_RET hal_avs2d_vdpu384b_init(void *hal, MppHalCfg *cfg)
{
    Avs2dRkvRegCtx *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    RK_U32 i, loop;
    MPP_RET ret = MPP_OK;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Avs2dRkvRegCtx)));
    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    reg_ctx->shph_dat = mpp_calloc(RK_U8, AVS2_384B_SHPH_SIZE);
    reg_ctx->scalist_dat = mpp_calloc(RK_U8, AVS2_384B_SCALIST_SIZE);
    loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, AVS2_ALL_TBL_BUF_SIZE(loop)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);

    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu38xRegSet, 1);
        vdpu384b_init_ctrl_regs(reg_ctx->reg_buf[i].regs, MPP_VIDEO_CodingAVS2);
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

MPP_RET hal_avs2d_vdpu384b_gen_regs(void *hal, HalTaskInfo *task)
{
    Avs2dRkvRegCtx *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Vdpu38xRegSet *regs = NULL;
    MPP_RET ret = MPP_OK;

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
    memset(regs, 0, sizeof(Vdpu38xRegSet));
    vdpu384b_init_ctrl_regs(regs, MPP_VIDEO_CodingAVS2);

#ifdef DUMP_VDPU38X_DATAS
    {
        memset(vdpu38x_dump_cur_dir, 0, sizeof(vdpu38x_dump_cur_dir));
        sprintf(vdpu38x_dump_cur_dir, "avs2/Frame%04d", vdpu38x_dump_cur_frm);
        if (access(vdpu38x_dump_cur_dir, 0)) {
            if (mkdir(vdpu38x_dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", vdpu38x_dump_cur_dir);
        }
        vdpu38x_dump_cur_frm++;
    }
#endif

    hal_avs2d_vdpu38x_prepare_header(p_hal, reg_ctx->shph_dat, AVS2_384B_SHPH_SIZE / 8);
    hal_avs2d_vdpu38x_prepare_scalist(p_hal, reg_ctx->scalist_dat, AVS2_384B_SCALIST_SIZE / 8);

    ret = fill_registers(p_hal, regs, task);

    if (ret)
        goto __RETURN;

    {
        memcpy(reg_ctx->bufs_ptr + reg_ctx->shph_offset, reg_ctx->shph_dat, AVS2_384B_SHPH_SIZE);
        memcpy(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, reg_ctx->scalist_dat, AVS2_384B_SCALIST_SIZE);

        regs->comm_addrs.reg131_gbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 131, reg_ctx->shph_offset);
        regs->comm_paras.reg67_global_len = AVS2_384B_SHPH_SIZE;

        regs->comm_addrs.reg132_scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 132, reg_ctx->sclst_offset);
    }

    vdpu384b_avs2_rcb_setup(p_hal, regs, task);
    vdpu38x_setup_statistic(&regs->ctrl_regs);
    mpp_buffer_sync_end(reg_ctx->bufs);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_vdpu384b_start(void *hal, HalTaskInfo *task)
{
    Vdpu38xRegSet *regs = NULL;
    Avs2dRkvRegCtx *reg_ctx;
    MppDev dev = NULL;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    MPP_RET ret = MPP_OK;

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
            rd_cfg.reg = &regs->statistic_regs;
            rd_cfg.size = sizeof(regs->statistic_regs);
            rd_cfg.offset = OFFSET_COM_STATISTIC_REGS_VDPU384B;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        /* rcb info for sram */
        vdpu38x_set_rcbinfo(dev, (VdpuRcbInfo*)reg_ctx->rcb_info);

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

MPP_RET hal_avs2d_vdpu384b_wait(void *hal, HalTaskInfo *task)
{
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx;
    Vdpu38xRegSet *regs;
    MPP_RET ret = MPP_OK;

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

    if (avs2d_hal_debug & AVS2D_HAL_DBG_REG) {
        RK_U32 *p = (RK_U32 *)regs;
        RK_U32 i = 0;

        for (i = 0; i < sizeof(regs->ctrl_regs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + OFFSET_CTRL_REGS, *p++);
        for (i = 0; i < sizeof(regs->comm_paras) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + OFFSET_CODEC_PARAS_REGS, *p++);
        for (i = 0; i < sizeof(regs->comm_addrs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + OFFSET_COMMON_ADDR_REGS, *p++);
        for (i = 0; i < sizeof(regs->statistic_regs) / 4; i++)
            mpp_log("get regs[%02d]: %08X\n", i + OFFSET_COM_STATISTIC_REGS_VDPU384B, *p++);

        mpp_assert(regs->statistic_regs.reg312.rcb_rd_sum_chk ==
                   regs->statistic_regs.reg312.rcb_wr_sum_chk);
    }

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

const MppHalApi hal_avs2d_vdpu384b = {
    .name     = "avs2d_vdpu384b",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVS2,
    .ctx_size = sizeof(Avs2dRkvRegCtx),
    .flag     = 0,
    .init     = hal_avs2d_vdpu384b_init,
    .deinit   = hal_avs2d_vdpu_deinit,
    .reg_gen  = hal_avs2d_vdpu384b_gen_regs,
    .start    = hal_avs2d_vdpu384b_start,
    .wait     = hal_avs2d_vdpu384b_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
};
