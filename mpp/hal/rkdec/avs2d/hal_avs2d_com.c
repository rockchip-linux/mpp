/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_avs2d_com"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_platform.h"
#include "mpp_common.h"
#include "mpp_log.h"
#include "vdpu38x_com.h"
#include "hal_avs2d_global.h"
#include "hal_avs2d_ctx.h"
#include "mpp_bitput.h"

MPP_RET hal_avs2d_vdpu_deinit(void *hal)
{
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    RK_U32 i, loop;
    MPP_RET ret = MPP_OK;

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
    vdpu38x_rcb_calc_deinit(reg_ctx->rcb_ctx);

    if (reg_ctx->bufs) {
        mpp_buffer_put(reg_ctx->bufs);
        reg_ctx->bufs = NULL;
    }

    if (p_hal->cmv_bufs) {
        hal_bufs_deinit(p_hal->cmv_bufs);
        p_hal->cmv_bufs = NULL;
    }

    MPP_FREE(reg_ctx->shph_dat);
    MPP_FREE(reg_ctx->scalist_dat);

    MPP_FREE(p_hal->reg_ctx);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_vdpu38x_prepare_header(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    AlfParams_Avs2d *alfp = &syntax->alfp;
    RefParams_Avs2d *refp = &syntax->refp;
    WqmParams_Avs2d *wqmp = &syntax->wqmp;
    RK_U64 *bit_buf = (RK_U64 *)data;
    BitputCtx_t bp;
    RK_U32 i, j;

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

    mpp_put_align(&bp, 64, 0);

#ifdef DUMP_VDPU38X_DATAS
    {
        char *cur_fname = "global_cfg.dat";
        memset(vdpu38x_dump_cur_fname_path, 0, sizeof(vdpu38x_dump_cur_fname_path));
        sprintf(vdpu38x_dump_cur_fname_path, "%s/%s", vdpu38x_dump_cur_dir, cur_fname);
        vdpu38x_dump_data_to_file(vdpu38x_dump_cur_fname_path, (void *)bp.pbuf,
                                  64 * bp.index + bp.bitpos, 128, 0, 0);
    }
#endif

    return MPP_OK;
}

MPP_RET hal_avs2d_vdpu38x_prepare_scalist(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
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

RK_S32 hal_avs2d_get_frame_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->frame_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd = mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

RK_S32 hal_avs2d_get_packet_fd(Avs2dHalCtx_t *p_hal, RK_S32 idx)
{
    RK_S32 ret_fd = 0;
    MppBuffer mbuffer = NULL;

    mpp_buf_slot_get_prop(p_hal->packet_slots, idx, SLOT_BUFFER, &mbuffer);
    ret_fd =  mpp_buffer_get_fd(mbuffer);

    return ret_fd;
}

MPP_RET hal_avs2d_set_up_colmv_buf(void *hal)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    RK_U32 ctu_size = 1 << (p_hal->syntax.pp.lcu_size);
    RK_S32 mv_size = hal_h265d_avs2d_calc_mv_size(pp->pic_width_in_luma_samples,
                                                  pp->pic_height_in_luma_samples *
                                                  (1 + pp->field_coded_sequence),
                                                  ctu_size);

    AVS2D_HAL_TRACE("mv_size %d", mv_size);

    if (p_hal->mv_size < (RK_U32)mv_size) {
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

RK_U8 hal_avs2d_fetch_data(RK_U32 fmt, RK_U8 *line, RK_U32 num)
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

MPP_RET hal_avs2d_vdpu_dump_yuv(void *hal, HalTaskInfo *task)
{
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    MppFrameFormat fmt = MPP_FMT_YUV420SP;
    RK_U32 vir_w = 0;
    RK_U32 vir_h = 0;
    FILE *fp_stream = NULL;
    char name[50];
    MppBuffer buffer = NULL;
    MppFrame frame;
    void *base = NULL;
    RK_U32 i, j;
    MPP_RET ret = MPP_OK;

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
    /* if format is fbc, write fbc header first */
    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        RK_U32 header_size = 0;

        header_size = vir_w * vir_h / 16;
        fwrite(base, 1, header_size, fp_stream);
        base += header_size;
    }

    if (fmt != MPP_FMT_YUV420SP_10BIT) {
        fwrite(base, 1, vir_w * vir_h * 3 / 2, fp_stream);
    } else {
        RK_U8 tmp = 0;
        for (i = 0; i < vir_h; i++) {
            for (j = 0; j < vir_w; j++) {
                tmp = hal_avs2d_fetch_data(fmt, base, j);
                fwrite(&tmp, 1, 1, fp_stream);
            }
            base += vir_w;
        }

        for (i = 0; i < vir_h / 2; i++) {
            for (j = 0; j < vir_w; j++) {
                tmp = hal_avs2d_fetch_data(fmt, base, j);
                fwrite(&tmp, 1, 1, fp_stream);
            }
            base += vir_w;
        }
    }
    fclose(fp_stream);

    return ret;
}
