/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "hal_avs2d_rkv"

#include <string.h>
#include <stdio.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_debug.h"
#include "mpp_bitput.h"

#include "avs2d_syntax.h"
#include "hal_avs2d_rkv.h"
#include "mpp_dec_cb_param.h"
#include "vdpu34x_avs2d.h"
#include "hal_avs2d_ctx.h"
#include "hal_avs2d_com.h"

#define MAX_REF_NUM                 (8)
#define AVS2_RKV_SHPH_SIZE          (1408 / 8)       /* bytes */
#define AVS2_RKV_SCALIST_SIZE       (80 + 128)       /* bytes */

#define AVS2_RKV_SHPH_ALIGNED_SIZE          (MPP_ALIGN(AVS2_RKV_SHPH_SIZE, SZ_4K))
#define AVS2_RKV_SCALIST_ALIGNED_SIZE       (MPP_ALIGN(AVS2_RKV_SCALIST_SIZE, SZ_4K))
#define AVS2_RKV_STREAM_INFO_SET_SIZE       (AVS2_RKV_SHPH_ALIGNED_SIZE + \
                                            AVS2_RKV_SCALIST_ALIGNED_SIZE)
#define AVS2_ALL_TBL_BUF_SIZE(cnt)          (AVS2_RKV_STREAM_INFO_SET_SIZE * (cnt))
#define AVS2_SHPH_OFFSET(pos)               (AVS2_RKV_STREAM_INFO_SET_SIZE * (pos))
#define AVS2_SCALIST_OFFSET(pos)            (AVS2_SHPH_OFFSET(pos) + AVS2_RKV_SHPH_ALIGNED_SIZE)

#define COLMV_COMPRESS_EN       (1)
#define COLMV_BLOCK_SIZE        (16)
#define COLMV_BYTES             (16)


MPP_RET hal_avs2d_rkv_deinit(void *hal);

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
    mpp_put_bits(&bp, pp->multi_hypothesis_skip_enable_flag, 1);
    mpp_put_bits(&bp, pp->dual_hypothesis_prediction_enable_flag, 1);
    mpp_put_bits(&bp, pp->weighted_skip_enable_flag, 1);
    mpp_put_bits(&bp, pp->asymmetrc_motion_partitions_enable_flag, 1);
    mpp_put_bits(&bp, pp->nonsquare_quadtree_transform_enable_flag, 1);
    mpp_put_bits(&bp, pp->nonsquare_intra_prediction_enable_flag, 1);
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

    if (alfp->enable_pic_alf_y) {
        RK_U32 alf_filter_num = alfp->alf_filter_num_minus1 + 1;
        mpp_put_bits(&bp, alfp->alf_filter_num_minus1, 4);

        for (i = 0; i < 16; i++)
            mpp_put_bits(&bp, alfp->alf_coeff_idx_tab[i], 4);

        for (i = 0; i < alf_filter_num; i++) {
            for (j = 0; j < 9; j++) {
                mpp_put_bits(&bp, alfp->alf_coeff_y[i][j], 7);
            }
        }
    }

    if (alfp->enable_pic_alf_cb) {
        for (j = 0; j < 9; j++)
            mpp_put_bits(&bp, alfp->alf_coeff_cb[j], 7);
    }

    if (alfp->enable_pic_alf_cr) {
        for (j = 0; j < 9; j++)
            mpp_put_bits(&bp, alfp->alf_coeff_cr[j], 7);
    }

    mpp_put_align(&bp, 128, 0);

    return MPP_OK;
}

static MPP_RET prepare_scalist(Avs2dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    RK_U32 i, j;
    RK_U32 size_id, block_size;
    BitputCtx_t bp;
    RK_U64 *bit_buf = (RK_U64 *)data;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    WqmParams_Avs2d *wqmp = &syntax->wqmp;

    if (!wqmp->pic_weight_quant_enable_flag)
        return MPP_OK;

    memset(data, 0, len);

    mpp_set_bitput_ctx(&bp, bit_buf, len);

    for (size_id = 0; size_id < 2; size_id++) {
        block_size = MPP_MIN(1 << (size_id + 2), 8);
        for (i = 0; i < block_size; i++) {
            for (j = 0 ; j < block_size; j++)
                //!< row col reversed
                mpp_put_bits(&bp, wqmp->wq_matrix[size_id][size_id * j + i], 8);
        }
    }

    return MPP_OK;
}

static MPP_RET init_common_regs(Vdpu34xAvs2dRegSet *regs)
{
    Vdpu34xRegCommon *common = &regs->common;

    common->reg009.dec_mode = 3;  // AVS2
    common->reg015.rlc_mode = 0;

    common->reg011.buf_empty_en = 1;
    common->reg011.dec_timeout_e = 1;

    common->reg010.dec_e = 1;

    common->reg013.h26x_error_mode = 0;
    common->reg013.colmv_error_mode = 0;
    common->reg013.h26x_streamd_error_mode = 0;
    common->reg021.inter_error_prc_mode = 0;
    common->reg021.error_deb_en = 0;
    common->reg021.error_intra_mode = 0;

    if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) {
        common->reg024.cabac_err_en_lowbits = 0;
        common->reg025.cabac_err_en_highbits = 0;
        common->reg026.swreg_block_gating_e = 0xfffef;
    } else {
        common->reg024.cabac_err_en_lowbits = 0xffffffdf;
        common->reg025.cabac_err_en_highbits = 0x3dffffff;
        common->reg026.swreg_block_gating_e = 0xfffff;
    }

    common->reg026.reg_cfg_gating_en = 1;
    common->reg032_timeout_threshold = 0x3fffff;

    common->reg011.dec_clkgate_e = 1;
    common->reg011.dec_e_strmd_clkgate_dis = 0;
    common->reg011.dec_timeout_e = 1;

    common->reg013.timeout_mode = 1;
    common->reg013.stmerror_waitdecfifo_empty = 1;
    common->reg012.colmv_compress_en = COLMV_COMPRESS_EN;
    common->reg012.wr_ddr_align_en = 1;
    common->reg012.info_collect_en = 1;
    common->reg012.error_info_en = 0;

    return MPP_OK;
}

//TODO calc rcb buffer size;
/*
static void avs2d_refine_rcb_size(VdpuRcbInfo *rcb_info,
                                  Vdpu34xAvs2dRegSet *hw_regs,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    (void) rcb_info;
    (void) hw_regs;
    (void) width;
    (void) height;
    (void) dxva;
    return;
}
*/

static void hal_avs2d_rcb_info_update(void *hal, Vdpu34xAvs2dRegSet *hw_regs)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    RK_S32 width = p_hal->syntax.pp.pic_width_in_luma_samples;
    RK_S32 height = p_hal->syntax.pp.pic_height_in_luma_samples;
    RK_S32 i = 0;
    RK_S32 loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    (void) hw_regs;

    reg_ctx->rcb_buf_size = vdpu34x_get_rcb_buf_size(reg_ctx->rcb_info, width, height);
    //avs2d_refine_rcb_size(reg_ctx->rcb_info, hw_regs, width, height, (void *)&p_hal->syntax);

    for (i = 0; i < loop; i++) {
        MppBuffer rcb_buf = NULL;

        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }

        ret = mpp_buffer_get(p_hal->cfg->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        if (ret)
            mpp_err_f("AVS2D mpp_buffer_get failed\n");

        reg_ctx->rcb_buf[i] = rcb_buf;
    }
}

static MPP_RET fill_registers(Avs2dHalCtx_t *p_hal, Vdpu34xAvs2dRegSet *p_regs, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    MppFrame mframe = NULL;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    RefParams_Avs2d *refp = &syntax->refp;
    HalDecTask *task_dec  = &task->dec;
    Vdpu34xRegCommon *common = &p_regs->common;
    RK_U32 is_fbc = 0;
    HalBuf *mv_buf = NULL;
    MppBufSlots frm_slots = p_hal->cfg->frame_slots;

    mpp_buf_slot_get_prop(frm_slots, task_dec->output, SLOT_FRAME_PTR, &mframe);
    is_fbc = MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe));

    //!< caculate the yuv_frame_size
    {
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;

        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;
        AVS2D_HAL_TRACE("is_fbc %d y_virstride %d, hor_virstride %d, ver_virstride %d\n", is_fbc, y_virstride, hor_virstride, ver_virstride);

        if (is_fbc) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset = MPP_ALIGN(fbc_hdr_stride * (ver_virstride + 16) / 16, SZ_4K);

            common->reg012.fbc_e = 1;
            common->reg018.y_hor_virstride = fbc_hdr_stride / 16;
            common->reg019.uv_hor_virstride = fbc_hdr_stride / 16;
            common->reg020_fbc_payload_off.payload_st_offset = fbd_offset >> 4;
        } else {
            common->reg012.fbc_e = 0;
            common->reg018.y_hor_virstride = hor_virstride / 16;
            common->reg019.uv_hor_virstride = hor_virstride / 16;
            common->reg020_y_virstride.y_virstride = y_virstride / 16;
        }
        common->reg013.cur_pic_is_idr = (pp->picture_type == 0 || pp->picture_type == 4 || pp->picture_type == 5);
    }

    // set current
    {
        RK_S32 fd = -1;
        p_regs->avs2d_param.reg65_cur_top_poc = mpp_frame_get_poc(mframe);
        p_regs->avs2d_param.reg66_cur_bot_poc = 0;
        fd = hal_avs2d_get_frame_fd(p_hal, task_dec->output);
        mpp_assert(fd >= 0);
        p_regs->common_addr.reg130_decout_base = fd;
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, task_dec->output);
        p_regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        AVS2D_HAL_TRACE("cur frame index %d, fd %d, colmv fd %d", task_dec->output, fd, p_regs->common_addr.reg131_colmv_cur_base);
    }

    // set reference
    {
        RK_U64 ref_flag = 0;
        RK_S32 valid_slot = -1;
        RK_U32 *ref_low = (RK_U32 *)&p_regs->avs2d_param.reg99;
        RK_U32 *ref_hight = (RK_U32 *)&p_regs->avs2d_param.reg100;
        RK_U32 err_ref_base = 0;

        AVS2D_HAL_TRACE("num of ref %d", refp->ref_pic_num);

        for (i = 0; i < refp->ref_pic_num; i++) {
            if (task_dec->refer[i] < 0)
                continue;

            valid_slot = i;
            break;
        }

        for (i = 0; i < refp->ref_pic_num; i++) {
            MppFrame frame_ref = NULL;

            RK_S32 slot_idx = task_dec->refer[i] < 0 ? task_dec->refer[valid_slot] : task_dec->refer[i];

            if (slot_idx < 0) {
                AVS2D_HAL_DBG(AVS2D_HAL_DBG_ERROR, "missing ref, could not found valid ref");
                task->dec.flags.ref_err = 1;
                return ret = MPP_ERR_UNKNOW;
            }

            mpp_buf_slot_get_prop(frm_slots, slot_idx, SLOT_FRAME_PTR, &frame_ref);

            if (frame_ref) {
                RK_U32 frm_flag = 1 << 3;

                if (pp->bottom_field_picture_flag)
                    frm_flag |= 1 << 2;

                if (pp->field_coded_sequence)
                    frm_flag |= 1;

                ref_flag |= frm_flag << (i * 8);

                p_regs->avs2d_addr.ref_base[i] = hal_avs2d_get_frame_fd(p_hal, slot_idx);
                mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                p_regs->avs2d_addr.colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);

                p_regs->avs2d_param.reg67_098_ref_poc[i] = mpp_frame_get_poc(frame_ref);

                if (!err_ref_base && !mpp_frame_get_errinfo(frame_ref))
                    err_ref_base = p_regs->avs2d_addr.ref_base[i];

                AVS2D_HAL_TRACE("ref_base[%d] index=%d, fd = %d, colmv %d, poc %d",
                                i, slot_idx, p_regs->avs2d_addr.ref_base[i],
                                p_regs->avs2d_addr.colmv_base[i], p_regs->avs2d_param.reg67_098_ref_poc[i]);
            }
        }

        if (p_hal->syntax.refp.scene_ref_enable && p_hal->syntax.refp.scene_ref_slot_idx >= 0) {
            MppFrame scene_ref = NULL;
            RK_S32 slot_idx = p_hal->syntax.refp.scene_ref_slot_idx;
            RK_S32 replace_idx = p_hal->syntax.refp.scene_ref_replace_pos;

            mpp_buf_slot_get_prop(frm_slots, slot_idx, SLOT_FRAME_PTR, &scene_ref);

            if (scene_ref) {
                p_regs->avs2d_addr.ref_base[replace_idx] = hal_avs2d_get_frame_fd(p_hal, slot_idx);
                mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, slot_idx);
                p_regs->avs2d_addr.colmv_base[replace_idx] = mpp_buffer_get_fd(mv_buf->buf[0]);
                p_regs->avs2d_param.reg67_098_ref_poc[replace_idx] = mpp_frame_get_poc(scene_ref);
            }
        }

        *ref_low = (RK_U32) (ref_flag & 0xffffffff);
        *ref_hight = (RK_U32) ((ref_flag >> 32) & 0xffffffff);

        p_regs->common_addr.reg132_error_ref_base = err_ref_base;
    }

    // set rlc
    {
        p_regs->common_addr.reg128_rlc_base = hal_avs2d_get_packet_fd(p_hal, task_dec->input);
        AVS2D_HAL_TRACE("packet fd %d from slot %d", p_regs->common_addr.reg128_rlc_base, task_dec->input);
        p_regs->common_addr.reg129_rlcwrite_base = p_regs->common_addr.reg128_rlc_base;
        common->reg016_str_len = MPP_ALIGN(mpp_packet_get_length(task_dec->input_packet), 16) + 64;
    }

    return ret;
}

MPP_RET hal_avs2d_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, loop;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == reg_ctx);

    //!< malloc buffers
    loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
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

    MPP_FREE(reg_ctx->shph_dat);
    MPP_FREE(reg_ctx->scalist_dat);

    MPP_FREE(p_hal->reg_ctx);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

MPP_RET hal_avs2d_rkv_init(void *hal, MppHalCfg *cfg)
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
    reg_ctx->shph_dat = mpp_calloc(RK_U8, AVS2_RKV_SHPH_SIZE);
    reg_ctx->scalist_dat = mpp_calloc(RK_U8, AVS2_RKV_SCALIST_SIZE);
    loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    FUN_CHECK(ret = mpp_buffer_get(cfg->buf_group, &reg_ctx->bufs, AVS2_ALL_TBL_BUF_SIZE(loop)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);

    for (i = 0; i < loop; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu34xAvs2dRegSet, 1);
        init_common_regs(reg_ctx->reg_buf[i].regs);
        reg_ctx->reg_buf[i].offset_shph = AVS2_SHPH_OFFSET(i);
        reg_ctx->reg_buf[i].offset_sclst = AVS2_SCALIST_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->shph_offset = reg_ctx->reg_buf[0].offset_shph;
        reg_ctx->sclst_offset = reg_ctx->reg_buf[0].offset_sclst;
    }

    if (MPP_FRAME_FMT_IS_FBC(cfg->cfg->base.out_fmt))
        mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_64);
    else
        mpp_slots_set_prop(cfg->frame_slots, SLOTS_HOR_ALIGN, mpp_align_16);

    mpp_slots_set_prop(cfg->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(cfg->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);

    return ret;
__FAILED:
    hal_avs2d_rkv_deinit(p_hal);
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

static MPP_RET set_up_colmv_buf(void *hal)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dSyntax_t *syntax = &p_hal->syntax;
    PicParams_Avs2d *pp   = &syntax->pp;
    RK_U32 mv_size = 0;
    RK_U32 ctu_size = 1 << (p_hal->syntax.pp.lcu_size);
    RK_U32 width = p_hal->syntax.pp.pic_width_in_luma_samples;
    RK_U32 height = p_hal->syntax.pp.pic_height_in_luma_samples;

    mv_size = vdpu34x_get_colmv_size(width, height, ctu_size, COLMV_BYTES,
                                     COLMV_BLOCK_SIZE, COLMV_COMPRESS_EN);
    if (pp->field_coded_sequence)
        mv_size *= 2;
    AVS2D_HAL_TRACE("mv_size %d", mv_size);

    if (p_hal->cmv_bufs == NULL || p_hal->mv_size < mv_size) {
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
        p_hal->mv_count = mpp_buf_slot_get_count(p_hal->cfg->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

__RETURN:
    return ret;
}

MPP_RET hal_avs2d_rkv_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dRkvRegCtx *reg_ctx;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Vdpu34xAvs2dRegSet *regs = NULL;
    MppHalCfg *cfg = p_hal->cfg;

    AVS2D_HAL_TRACE("In.");

    INP_CHECK(ret, NULL == p_hal);

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !cfg->cfg->base.disable_error) {
        ret = MPP_NOK;
        goto __RETURN;
    }

    memcpy(&p_hal->syntax, task->dec.syntax.data, sizeof(Avs2dSyntax_t));
    ret = set_up_colmv_buf(p_hal);
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
    memset(regs, 0, sizeof(Vdpu34xAvs2dRegSet));
    init_common_regs(regs);

    prepare_header(p_hal, reg_ctx->shph_dat, AVS2_RKV_SHPH_SIZE / 8);
    prepare_scalist(p_hal, reg_ctx->scalist_dat, AVS2_RKV_SCALIST_SIZE / 8);

    ret = fill_registers(p_hal, regs, task);

    if (ret)
        goto __RETURN;

    {
        memcpy(reg_ctx->bufs_ptr + reg_ctx->shph_offset, reg_ctx->shph_dat, AVS2_RKV_SHPH_SIZE);
        memcpy(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, reg_ctx->scalist_dat, AVS2_RKV_SCALIST_SIZE);
        regs->common.reg012.scanlist_addr_valid_en = 1;

        regs->avs2d_addr.head_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(cfg->dev, 161, reg_ctx->shph_offset);

        regs->avs2d_param.reg105.head_len = AVS2_RKV_SHPH_SIZE / 16;
        regs->avs2d_param.reg105.head_len -= (regs->avs2d_param.reg105.head_len > 0) ? 1 : 0;

        regs->avs2d_addr.scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(cfg->dev, 180, reg_ctx->sclst_offset);
    }

    if (hal_avs2d_debug & AVS2D_HAL_DBG_IN) {
        FILE *fp_shph = NULL;
        char name[50];
        snprintf(name, sizeof(name), "/data/tmp/rkv_shph_%03d.bin", p_hal->frame_no);
        fp_shph = fopen(name, "wb");
        fwrite(reg_ctx->bufs_ptr + reg_ctx->shph_offset, 1, AVS2_RKV_SHPH_SIZE, fp_shph);
        fclose(fp_shph);
    }

    if (hal_avs2d_debug & AVS2D_HAL_DBG_IN) {
        FILE *fp_scalist = NULL;
        char name[50];
        snprintf(name, sizeof(name), "/data/tmp/rkv_scalist_%03d.bin", p_hal->frame_no);
        fp_scalist = fopen(name, "wb");
        fwrite(reg_ctx->bufs_ptr + reg_ctx->sclst_offset, 1, AVS2_RKV_SCALIST_SIZE, fp_scalist);
        fclose(fp_scalist);
    }

    // set rcb
    {
        hal_avs2d_rcb_info_update(p_hal, regs);
        vdpu34x_setup_rcb(&regs->common_addr, cfg->dev, (p_hal->fast_mode != 0) ?
                          reg_ctx->rcb_buf[task->dec.reg_index] : reg_ctx->rcb_buf[0],
                          reg_ctx->rcb_info);

    }

    if (hal_avs2d_debug & AVS2D_HAL_DBG_IN) {
        FILE *fp_rcb = NULL;
        char name[50];
        void *base = NULL;
        snprintf(name, sizeof(name), "/data/tmp/rkv_rcb_%03d.bin", p_hal->frame_no);
        fp_rcb = fopen(name, "wb");
        base = mpp_buffer_get_ptr(reg_ctx->rcb_buf[0]);
        fwrite(base, 1, reg_ctx->rcb_buf_size, fp_rcb);
        fclose(fp_rcb);

    }

    vdpu34x_setup_statistic(&regs->common, &regs->statistic);
    mpp_buffer_sync_end(reg_ctx->bufs);

    /* enable reference frame usage feedback */
    regs->statistic.reg265.perf_cnt0_sel = 42;
    regs->statistic.reg266_perf_cnt0 = 0;

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

static MPP_RET hal_avs2d_rkv_dump_reg_write(void *hal, Vdpu34xAvs2dRegSet *regs)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    FILE *fp_reg = NULL;
    RK_U32 i = 0;
    char name[50];
    snprintf(name, sizeof(name), "/data/tmp/rkv_reg_write_%03d.txt", p_hal->frame_no);
    fp_reg = fopen(name , "w+");

    fprintf(fp_reg, "********Frame num %d\n", p_hal->frame_no);
    for (i = 0; i < 8; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i, 0);

    for (i = 0; i < sizeof(Vdpu34xRegCommon) / sizeof(RK_U32); i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_COMMON_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->common)[i]);

    for (i = 0; i < 63 - 32; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i + 33, 0);

    for (i = 0; i < sizeof(Vdpu34xRegAvs2dParam) / sizeof(RK_U32); i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_CODEC_PARAMS_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->avs2d_param)[i]);

    for (i = 0; i < 127 - 112; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i + 113, 0);

    for (i = 0; i < sizeof(Vdpu34xRegCommonAddr) / sizeof(RK_U32); i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_COMMON_ADDR_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->common_addr)[i]);

    for (i = 0; i < 159 - 142; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i + 143, 0);


    for (i = 0; i < sizeof(Vdpu34xRegAvs2dAddr) / sizeof(RK_U32); i++ )
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_CODEC_ADDR_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->avs2d_addr)[i]);

    for (i = 0; i < 223 - 197; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i + 198, 0);

    for (i = 0; i < sizeof(Vdpu34xRegIrqStatus) / sizeof(RK_U32); i++ )
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_INTERRUPT_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->irq_status)[i]);

    for (i = 0; i < 255 - 237; i++)
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", i + 238, 0);

    for (i = 0; i < sizeof(Vdpu34xRegStatistic) / sizeof(RK_U32); i++ )
        fprintf(fp_reg, "Write reg[%03d] : 0x%08x\n", (RK_U32)(i + VDPU34X_OFF_STATISTIC_REGS / sizeof(RK_U32)),
                ((RK_U32 *)&regs->statistic)[i]);

    fclose(fp_reg);
    return ret;
}

static MPP_RET hal_avs2d_rkv_dump_stream(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    FILE *fp_stream = NULL;
    char name[50];
    MppBuffer buffer = NULL;
    void *base = NULL;
    mpp_buf_slot_get_prop(p_hal->cfg->packet_slots, task->dec.input, SLOT_BUFFER, &buffer);
    base = mpp_buffer_get_ptr(buffer);
    snprintf(name, sizeof(name), "/data/tmp/rkv_stream_in_%03d.bin", p_hal->frame_no);
    fp_stream = fopen(name, "wb");
    fwrite(base, 1, mpp_packet_get_length(task->dec.input_packet), fp_stream);
    fclose(fp_stream);

    return ret;
}

MPP_RET hal_avs2d_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Vdpu34xAvs2dRegSet *regs = NULL;
    Avs2dRkvRegCtx *reg_ctx;
    MppDev dev = NULL;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;

    AVS2D_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == p_hal);

    if ((task->dec.flags.parse_err || task->dec.flags.ref_err) &&
        !p_hal->cfg->cfg->base.disable_error) {
        ret = MPP_NOK;
        goto __RETURN;
    }

    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    regs = (p_hal->fast_mode != 0) ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;
    dev = p_hal->cfg->dev;

    p_hal->frame_no++;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->common;
        wr_cfg.size = sizeof(regs->common);
        wr_cfg.offset = VDPU34X_OFF_COMMON_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->avs2d_param;
        wr_cfg.size = sizeof(regs->avs2d_param);
        wr_cfg.offset = VDPU34X_OFF_CODEC_PARAMS_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = VDPU34X_OFF_COMMON_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->avs2d_addr;
        wr_cfg.size = sizeof(regs->avs2d_addr);
        wr_cfg.offset = VDPU34X_OFF_CODEC_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->statistic;
        wr_cfg.size = sizeof(regs->statistic);
        wr_cfg.offset = VDPU34X_OFF_STATISTIC_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->irq_status;
        rd_cfg.size = sizeof(regs->irq_status);
        rd_cfg.offset = VDPU34X_OFF_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);

        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->avs2d_param;
        rd_cfg.size = sizeof(regs->avs2d_param);
        rd_cfg.offset = VDPU34X_OFF_CODEC_PARAMS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);

        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->statistic;
        rd_cfg.size = sizeof(regs->statistic);
        rd_cfg.offset = VDPU34X_OFF_STATISTIC_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);

        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        if (hal_avs2d_debug & AVS2D_HAL_DBG_REG) {
            memset(reg_ctx->reg_out, 0, sizeof(reg_ctx->reg_out));
            rd_cfg.reg = reg_ctx->reg_out;
            rd_cfg.size = sizeof(reg_ctx->reg_out);
            rd_cfg.offset = 0;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        }

        // rcb info for sram
        vdpu34x_set_rcbinfo(dev, reg_ctx->rcb_info);

        if (hal_avs2d_debug & AVS2D_HAL_DBG_IN)
            hal_avs2d_rkv_dump_stream(hal, task);

        if (hal_avs2d_debug & AVS2D_HAL_DBG_REG)
            hal_avs2d_rkv_dump_reg_write(hal, regs);

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

MPP_RET hal_avs2d_rkv_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dHalCtx_t *p_hal = (Avs2dHalCtx_t *)hal;
    Avs2dRkvRegCtx *reg_ctx;
    Vdpu34xAvs2dRegSet *p_regs;

    INP_CHECK(ret, NULL == p_hal);
    reg_ctx = (Avs2dRkvRegCtx *)p_hal->reg_ctx;
    p_regs = (p_hal->fast_mode != 0) ? reg_ctx->reg_buf[task->dec.reg_index].regs : reg_ctx->regs;

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

    if (hal_avs2d_debug & AVS2D_HAL_DBG_REG) {
        FILE *fp_reg = NULL;
        RK_U32 i = 0;
        char name[50];
        snprintf(name, sizeof(name), "/data/tmp/rkv_reg_read_%03d.txt", p_hal->frame_no);
        fp_reg = fopen(name , "w+");

        for (i = 0; i < 278; i++)
            fprintf(fp_reg, "%08x\n", reg_ctx->reg_out[i]);

        fclose(fp_reg);
    }

    AVS2D_HAL_TRACE("read reg[224] 0x%08x\n", p_regs->irq_status.reg224);

    if (p_hal->cfg->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_regs;

        if (p_regs->irq_status.reg224.dec_error_sta ||
            (!p_regs->irq_status.reg224.dec_rdy_sta) ||
            p_regs->irq_status.reg224.buf_empty_sta ||
            p_regs->irq_status.reg226.strmd_error_status ||
            p_regs->irq_status.reg227.colmv_error_ref_picidx ||
            p_regs->irq_status.reg225.strmd_detect_error_flag)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        task->dec.flags.ref_used = p_regs->statistic.reg266_perf_cnt0;
        task->dec.flags.ref_info_valid = 1;

        if (task->dec.flags.ref_miss) {
            RK_U32 ref_hw_usage = p_regs->statistic.reg266_perf_cnt0;

            AVS2D_HAL_TRACE("hal frame %d ref miss %x hard_err %d hw_usage %x", p_hal->frame_no,
                            task->dec.flags.ref_miss, param.hard_err, ref_hw_usage);
        }

        AVS2D_HAL_TRACE("hal frame %d hard_err= %d", p_hal->frame_no, param.hard_err);

        mpp_callback(p_hal->cfg->dec_cb, &param);
    }

    memset(&p_regs->irq_status.reg224, 0, sizeof(RK_U32));

    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

__RETURN:
    AVS2D_HAL_TRACE("Out. ret %d", ret);
    return ret;
}

const MppHalApi hal_avs2d_rkvdpu = {
    .name     = "avs2d_rkvdpu",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVS2,
    .ctx_size = sizeof(Avs2dHalCtx_t),
    .flag     = 0,
    .init     = hal_avs2d_rkv_init,
    .deinit   = hal_avs2d_vdpu_deinit,
    .reg_gen  = hal_avs2d_rkv_gen_regs,
    .start    = hal_avs2d_rkv_start,
    .wait     = hal_avs2d_rkv_wait,
    .reset    = NULL,
    .flush    = NULL,
    .control  = NULL,
    .client   = VPU_CLIENT_RKVDEC,
    .soc_type = {
        ROCKCHIP_SOC_RK3588,
        ROCKCHIP_SOC_BUTT
    },
};

MPP_DEC_HAL_API_REGISTER(hal_avs2d_rkvdpu)
