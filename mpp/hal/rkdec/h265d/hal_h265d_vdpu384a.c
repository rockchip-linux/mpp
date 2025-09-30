/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h265d_vdpu384a"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "h265d_syntax.h"
#include "hal_h265d_debug.h"
#include "hal_h265d_ctx.h"
#include "hal_h265d_com.h"
#include "hal_h265d_vdpu384a.h"
#include "vdpu384a_h265d.h"
#include "vdpu384a_com.h"

#define PPS_SIZE                (112 * 64)//(96x64)

#define FMT 4
#define CTU 3

typedef struct {
    RK_U32 a;
    RK_U32 b;
} FilterdColBufRatio;

#define SPSPPS_ALIGNED_SIZE             (MPP_ALIGN(2181 + 64, 128) / 8) // byte, 2181 bit + Reserve 64
#define SCALIST_ALIGNED_SIZE            (MPP_ALIGN(81 * 1360, SZ_4K))
#define INFO_BUFFER_SIZE                (SPSPPS_ALIGNED_SIZE + SCALIST_ALIGNED_SIZE)
#define ALL_BUFFER_SIZE(cnt)            (INFO_BUFFER_SIZE *cnt)

#define SPSPPS_OFFSET(pos)              (INFO_BUFFER_SIZE * pos)
#define SCALIST_OFFSET(pos)             (SPSPPS_OFFSET(pos) + SPSPPS_ALIGNED_SIZE)

#define pocdistance(a, b)               (((a) > (b)) ? ((a) - (b)) : ((b) - (a)))

static RK_U32 rkv_len_align_422(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 16));
}

static RK_U32 rkv_len_align_444(RK_U32 val)
{
    return (3 * MPP_ALIGN(val, 16));
}

static MPP_RET vdpu384a_setup_scale_origin_bufs(HalH265dCtx *ctx, MppFrame mframe)
{
    /* for 8K FrameBuf scale mode */
    size_t origin_buf_size = 0;

    origin_buf_size = mpp_frame_get_buf_size(mframe);

    if (!origin_buf_size) {
        mpp_err_f("origin_bufs get buf size failed\n");
        return MPP_NOK;
    }

    if (ctx->origin_bufs) {
        hal_bufs_deinit(ctx->origin_bufs);
        ctx->origin_bufs = NULL;
    }
    hal_bufs_init(&ctx->origin_bufs);
    if (!ctx->origin_bufs) {
        mpp_err_f("origin_bufs init fail\n");
        return MPP_ERR_NOMEM;
    }

    hal_bufs_setup(ctx->origin_bufs, 16, 1, &origin_buf_size);

    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu384a_init(void *hal, MppHalCfg *cfg)
{
    RK_S32 ret = 0;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;

    mpp_slots_set_prop(reg_ctx->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(reg_ctx->slots, SLOTS_VER_ALIGN, hevc_ver_align);

    reg_ctx->scaling_qm = mpp_calloc(DXVA_Qmatrix_HEVC, 1);
    if (reg_ctx->scaling_qm == NULL) {
        mpp_err("scaling_org alloc fail");
        return MPP_ERR_MALLOC;
    }

    reg_ctx->scaling_rk = mpp_calloc(scalingFactor_t, 1);
    reg_ctx->pps_buf = mpp_calloc(RK_U8, SPSPPS_ALIGNED_SIZE);

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
        RK_U32 i = 0;
        RK_U32 max_cnt = reg_ctx->fast_mode ? MAX_GEN_REG : 1;

        //!< malloc buffers
        ret = mpp_buffer_get(reg_ctx->group, &reg_ctx->bufs, ALL_BUFFER_SIZE(max_cnt));
        if (ret) {
            mpp_err("h265d mpp_buffer_get failed\n");
            return ret;
        }

        reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
        for (i = 0; i < max_cnt; i++) {
            reg_ctx->g_buf[i].hw_regs = mpp_calloc_size(void, sizeof(Vdpu384aH265dRegSet));
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
        cfg->hal_fbc_adj_cfg->func = vdpu384a_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

    (void) cfg;
    return MPP_OK;
}

static MPP_RET hal_h265d_vdpu384a_deinit(void *hal)
{
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_U32 loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->g_buf) : 1;
    RK_U32 i;

    if (reg_ctx->bufs) {
        mpp_buffer_put(reg_ctx->bufs);
        reg_ctx->bufs = NULL;
    }

    loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->rcb_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }
    }

    if (reg_ctx->group) {
        mpp_buffer_group_put(reg_ctx->group);
        reg_ctx->group = NULL;
    }

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_ctx->g_buf[i].hw_regs);

    MPP_FREE(reg_ctx->scaling_qm);
    MPP_FREE(reg_ctx->scaling_rk);
    MPP_FREE(reg_ctx->pps_buf);

    if (reg_ctx->cmv_bufs) {
        hal_bufs_deinit(reg_ctx->cmv_bufs);
        reg_ctx->cmv_bufs = NULL;
    }

    if (reg_ctx->origin_bufs) {
        hal_bufs_deinit(reg_ctx->origin_bufs);
        reg_ctx->origin_bufs = NULL;
    }

    return MPP_OK;
}

#define SCALING_LIST_NUM 6

void hal_vdpu384a_record_scaling_list(scalingFactor_t *pScalingFactor_out, scalingList_t *pScalingList)
{
    RK_S32 i;
    RK_U32 listId;
    BitputCtx_t bp;

    mpp_set_bitput_ctx(&bp, (RK_U64 *)pScalingFactor_out, 170); // 170*64bits

    //-------- following make it by hardware needed --------
    //sizeId == 0, block4x4
    for (listId = 0; listId < SCALING_LIST_NUM; listId++) {
        RK_U8 *p_data = pScalingList->sl[0][listId];
        /* dump by block4x4, vectial direction */
        for (i = 0; i < 4; i++) {
            mpp_put_bits(&bp, p_data[i + 0], 8);
            mpp_put_bits(&bp, p_data[i + 4], 8);
            mpp_put_bits(&bp, p_data[i + 8], 8);
            mpp_put_bits(&bp, p_data[i + 12], 8);
        }
    }
    //sizeId == 1, block8x8
    for (listId = 0; listId < SCALING_LIST_NUM; listId++) {
        RK_S32 blk4_x = 0, blk4_y = 0;
        RK_U8 *p_data = pScalingList->sl[1][listId];

        /* dump by block4x4, vectial direction */
        for (blk4_x = 0; blk4_x < 8; blk4_x += 4) {
            for (blk4_y = 0; blk4_y < 8; blk4_y += 4) {
                RK_S32 pos = blk4_y * 8 + blk4_x;

                for (i = 0; i < 4; i++) {
                    mpp_put_bits(&bp, p_data[pos + i + 0], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 8], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 16], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 24], 8);
                }
            }
        }
    }
    //sizeId == 2, block16x16
    for (listId = 0; listId < SCALING_LIST_NUM; listId++) {
        RK_S32 blk4_x = 0, blk4_y = 0;
        RK_U8 *p_data = pScalingList->sl[2][listId];

        /* dump by block4x4, vectial direction */
        for (blk4_x = 0; blk4_x < 8; blk4_x += 4) {
            for (blk4_y = 0; blk4_y < 8; blk4_y += 4) {
                RK_S32 pos = blk4_y * 8 + blk4_x;

                for (i = 0; i < 4; i++) {
                    mpp_put_bits(&bp, p_data[pos + i + 0], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 8], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 16], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 24], 8);
                }
            }
        }
    }
    //sizeId == 3, blcok32x32
    for (listId = 0; listId < 6; listId++) {
        RK_S32 blk4_x = 0, blk4_y = 0;
        RK_U8 *p_data = pScalingList->sl[3][listId];

        /* dump by block4x4, vectial direction */
        for (blk4_x = 0; blk4_x < 8; blk4_x += 4) {
            for (blk4_y = 0; blk4_y < 8; blk4_y += 4) {
                RK_S32 pos = blk4_y * 8 + blk4_x;

                for (i = 0; i < 4; i++) {
                    mpp_put_bits(&bp, p_data[pos + i + 0], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 8], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 16], 8);
                    mpp_put_bits(&bp, p_data[pos + i + 24], 8);
                }
            }
        }
    }
    //sizeId == 0, block4x4, horiztion direction */
    for (listId = 0; listId < SCALING_LIST_NUM; listId++) {
        RK_U8 *p_data = pScalingList->sl[0][listId];

        for (i = 0; i < 16; i++)
            mpp_put_bits(&bp, p_data[i], 8);
    }

    // dump dc value
    for (i = 0; i < SCALING_LIST_NUM; i++)//sizeId = 2, 16x16
        mpp_put_bits(&bp, pScalingList->sl_dc[0][i], 8);
    for (i = 0; i < SCALING_LIST_NUM; i++) //sizeId = 3, 32x32
        mpp_put_bits(&bp, pScalingList->sl_dc[1][i], 8);

    mpp_put_align(&bp, 128, 0);
}

static MPP_RET hal_h265d_vdpu384a_scalinglist_packet(void *hal, void *ptr, void *dxva)
{
    scalingList_t sl;
    RK_U32 i, j, pos;
    h265d_dxva2_picture_context_t *dxva_ctx = (h265d_dxva2_picture_context_t*)dxva;
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;

    if (!dxva_ctx->pp.scaling_list_enabled_flag) {
        return MPP_OK;
    }

    if (memcmp((void*)&dxva_ctx->qm, reg_ctx->scaling_qm, sizeof(DXVA_Qmatrix_HEVC))) {
        memset(&sl, 0, sizeof(scalingList_t));

        for (i = 0; i < 6; i++) {
            for (j = 0; j < 16; j++) {
                pos = 4 * hal_hevc_diag_scan4x4_y[j] + hal_hevc_diag_scan4x4_x[j];
                sl.sl[0][i][pos] = dxva_ctx->qm.ucScalingLists0[i][j];
            }

            for (j = 0; j < 64; j++) {
                pos = 8 * hal_hevc_diag_scan8x8_y[j] + hal_hevc_diag_scan8x8_x[j];
                sl.sl[1][i][pos] =  dxva_ctx->qm.ucScalingLists1[i][j];
                sl.sl[2][i][pos] =  dxva_ctx->qm.ucScalingLists2[i][j];

                if (i == 0)
                    sl.sl[3][i][pos] =  dxva_ctx->qm.ucScalingLists3[0][j];
                else if (i == 3)
                    sl.sl[3][i][pos] =  dxva_ctx->qm.ucScalingLists3[1][j];
                else
                    sl.sl[3][i][pos] =  dxva_ctx->qm.ucScalingLists2[i][j];
            }

            sl.sl_dc[0][i] =  dxva_ctx->qm.ucScalingListDCCoefSizeID2[i];
            if (i == 0)
                sl.sl_dc[1][i] =  dxva_ctx->qm.ucScalingListDCCoefSizeID3[0];
            else if (i == 3)
                sl.sl_dc[1][i] =  dxva_ctx->qm.ucScalingListDCCoefSizeID3[1];
            else
                sl.sl_dc[1][i] =  dxva_ctx->qm.ucScalingListDCCoefSizeID2[i];
        }
        hal_vdpu384a_record_scaling_list((scalingFactor_t *)reg_ctx->scaling_rk, &sl);
    }

    memcpy(ptr, reg_ctx->scaling_rk, sizeof(scalingFactor_t));

    return MPP_OK;
}

static RK_S32 hal_h265d_v345_output_pps_packet(void *hal, void *dxva)
{
    RK_S32 i;
    RK_U32 log2_min_cb_size;
    RK_S32 width, height;
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    Vdpu384aH265dRegSet *hw_reg = (Vdpu384aH265dRegSet*)(reg_ctx->hw_regs);
    h265d_dxva2_picture_context_t *dxva_ctx = (h265d_dxva2_picture_context_t*)dxva;
    BitputCtx_t bp;

    if (NULL == reg_ctx || dxva_ctx == NULL) {
        mpp_err("%s:%s:%d reg_ctx or dxva_ctx is NULL",
                __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

    // SPS
    {
        void *pps_ptr = mpp_buffer_get_ptr(reg_ctx->bufs) + reg_ctx->spspps_offset;
        RK_U64 *pps_packet = reg_ctx->pps_buf;

        if (NULL == pps_ptr) {
            mpp_err("pps_data get ptr error");
            return MPP_ERR_NOMEM;
        }

        log2_min_cb_size = dxva_ctx->pp.log2_min_luma_coding_block_size_minus3 + 3;
        width = (dxva_ctx->pp.PicWidthInMinCbsY << log2_min_cb_size);
        height = (dxva_ctx->pp.PicHeightInMinCbsY << log2_min_cb_size);

        mpp_set_bitput_ctx(&bp, pps_packet, SPSPPS_ALIGNED_SIZE / 8);

        if (dxva_ctx->pp.ps_update_flag) {
            mpp_put_bits(&bp, dxva_ctx->pp.vps_id, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.sps_id, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.chroma_format_idc, 2);

            mpp_put_bits(&bp, width, 16);
            mpp_put_bits(&bp, height, 16);
            mpp_put_bits(&bp, dxva_ctx->pp.bit_depth_luma_minus8, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.bit_depth_chroma_minus8, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_max_pic_order_cnt_lsb_minus4 + 4, 5);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size, 2);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_min_luma_coding_block_size_minus3 + 3, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_min_transform_block_size_minus2 + 2, 3);

            mpp_put_bits(&bp, dxva_ctx->pp.log2_diff_max_min_transform_block_size, 2);
            mpp_put_bits(&bp, dxva_ctx->pp.max_transform_hierarchy_depth_inter, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.max_transform_hierarchy_depth_intra, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.scaling_list_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.amp_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.sample_adaptive_offset_enabled_flag, 1);
            ///<-zrh comment ^  68 bit above
            mpp_put_bits(&bp, dxva_ctx->pp.pcm_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.pcm_enabled_flag ? (dxva_ctx->pp.pcm_sample_bit_depth_luma_minus1 + 1) : 0, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.pcm_enabled_flag ? (dxva_ctx->pp.pcm_sample_bit_depth_chroma_minus1 + 1) : 0, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.pcm_loop_filter_disabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_diff_max_min_pcm_luma_coding_block_size, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.pcm_enabled_flag ? (dxva_ctx->pp.log2_min_pcm_luma_coding_block_size_minus3 + 3) : 0, 3);

            mpp_put_bits(&bp, dxva_ctx->pp.num_short_term_ref_pic_sets, 7);
            mpp_put_bits(&bp, dxva_ctx->pp.long_term_ref_pics_present_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.num_long_term_ref_pics_sps, 6);
            mpp_put_bits(&bp, dxva_ctx->pp.sps_temporal_mvp_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.strong_intra_smoothing_enabled_flag, 1);
            // SPS extenstion
            mpp_put_bits(&bp, dxva_ctx->pp.transform_skip_rotation_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.transform_skip_context_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.strong_intra_smoothing_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.implicit_rdpcm_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.explicit_rdpcm_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.extended_precision_processing_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.intra_smoothing_disabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.sps_max_dec_pic_buffering_minus1, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.separate_colour_plane_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.high_precision_offsets_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.persistent_rice_adaptation_enabled_flag, 1);

            /* PPS */
            mpp_put_bits(&bp, dxva_ctx->pp.pps_id, 6);
            mpp_put_bits(&bp, dxva_ctx->pp.sps_id, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.dependent_slice_segments_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.output_flag_present_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.num_extra_slice_header_bits, 13);

            mpp_put_bits(&bp, dxva_ctx->pp.sign_data_hiding_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.cabac_init_present_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.num_ref_idx_l0_default_active_minus1 + 1, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.num_ref_idx_l1_default_active_minus1 + 1, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.init_qp_minus26, 7);
            mpp_put_bits(&bp, dxva_ctx->pp.constrained_intra_pred_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.transform_skip_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.cu_qp_delta_enabled_flag, 1);
            mpp_put_bits(&bp, log2_min_cb_size + dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size - dxva_ctx->pp.diff_cu_qp_delta_depth, 3);

            mpp_put_bits(&bp, dxva_ctx->pp.pps_cb_qp_offset, 5);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_cr_qp_offset, 5);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_slice_chroma_qp_offsets_present_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.weighted_pred_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.weighted_bipred_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.transquant_bypass_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.tiles_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.entropy_coding_sync_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_loop_filter_across_slices_enabled_flag, 1);

            mpp_put_bits(&bp, dxva_ctx->pp.loop_filter_across_tiles_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.deblocking_filter_override_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_deblocking_filter_disabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_beta_offset_div2, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.pps_tc_offset_div2, 4);
            mpp_put_bits(&bp, dxva_ctx->pp.lists_modification_present_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.log2_parallel_merge_level_minus2 + 2, 3);
            mpp_put_bits(&bp, dxva_ctx->pp.slice_segment_header_extension_present_flag, 1);
            mpp_put_bits(&bp, 0, 3);

            // PPS externsion
            if (dxva_ctx->pp.log2_max_transform_skip_block_size > 2) {
                mpp_put_bits(&bp, dxva_ctx->pp.log2_max_transform_skip_block_size - 2, 2);
            } else {
                mpp_put_bits(&bp, 0, 2);
            }
            mpp_put_bits(&bp, dxva_ctx->pp.cross_component_prediction_enabled_flag, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.chroma_qp_offset_list_enabled_flag, 1);

            RK_S32 log2_min_cu_chroma_qp_delta_size = log2_min_cb_size +
                                                      dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size -
                                                      dxva_ctx->pp.diff_cu_chroma_qp_offset_depth;
            mpp_put_bits(&bp, log2_min_cu_chroma_qp_delta_size, 3);
            for (i = 0; i < 6; i++)
                mpp_put_bits(&bp, dxva_ctx->pp.cb_qp_offset_list[i], 5);
            for (i = 0; i < 6; i++)
                mpp_put_bits(&bp, dxva_ctx->pp.cr_qp_offset_list[i], 5);
            mpp_put_bits(&bp, dxva_ctx->pp.chroma_qp_offset_list_len_minus1, 3);

            /* mvc0 && mvc1 */
            mpp_put_bits(&bp, 0xffff, 16);
            mpp_put_bits(&bp, 0, 1);
            mpp_put_bits(&bp, 0, 6);
            mpp_put_bits(&bp, 0, 1);
            mpp_put_bits(&bp, 0, 1);
        } else {
            bp.index = 4;
            bp.bitpos = 41;
            bp.bvalue = bp.pbuf[bp.index] & MPP_GENMASK(bp.bitpos - 1, 0);
        }
        /* poc info */
        {
            RK_S32 dpb_valid[15] = {0}, refpic_poc[15] = {0};

            for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dxva_ctx->pp.RefPicList); i++) {
                if (dxva_ctx->pp.RefPicList[i].bPicEntry != 0xff &&
                    dxva_ctx->pp.RefPicList[i].bPicEntry != 0x7f) {
                    dpb_valid[i] = 1;
                    refpic_poc[i] = dxva_ctx->pp.PicOrderCntValList[i];
                }
            }

            mpp_put_bits(&bp, 0, 1);
            mpp_put_bits(&bp, 0, 1);
            mpp_put_bits(&bp, 0, 1);
            mpp_put_bits(&bp, dxva_ctx->pp.current_poc, 32);

            for (i = 0; i < 15; i++)
                mpp_put_bits(&bp, refpic_poc[i], 32);
            mpp_put_bits(&bp, 0, 32);
            for (i = 0; i < 15; i++)
                mpp_put_bits(&bp, dpb_valid[i], 1);
            mpp_put_bits(&bp, 0, 1);
        }

        /* tile info */
        mpp_put_bits(&bp, dxva_ctx->pp.tiles_enabled_flag ? (dxva_ctx->pp.num_tile_columns_minus1 + 1) : 1, 5);
        mpp_put_bits(&bp, dxva_ctx->pp.tiles_enabled_flag ? (dxva_ctx->pp.num_tile_rows_minus1 + 1) : 1, 5);
        {
            /// tiles info begin
            RK_U16 column_width[20];
            RK_U16 row_height[22];

            memset(column_width, 0, sizeof(column_width));
            memset(row_height, 0, sizeof(row_height));

            if (dxva_ctx->pp.tiles_enabled_flag) {
                if (dxva_ctx->pp.uniform_spacing_flag == 0) {
                    RK_S32 maxcuwidth = dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size;
                    RK_S32 ctu_width_in_pic = (width +
                                               (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                    RK_S32 ctu_height_in_pic = (height +
                                                (1 << maxcuwidth) - 1) / (1 << maxcuwidth) ;
                    RK_S32 sum = 0;
                    for (i = 0; i < dxva_ctx->pp.num_tile_columns_minus1; i++) {
                        column_width[i] = dxva_ctx->pp.column_width_minus1[i] + 1;
                        sum += column_width[i]  ;
                    }
                    column_width[i] = ctu_width_in_pic - sum;

                    sum = 0;
                    for (i = 0; i < dxva_ctx->pp.num_tile_rows_minus1; i++) {
                        row_height[i] = dxva_ctx->pp.row_height_minus1[i] + 1;
                        sum += row_height[i];
                    }
                    row_height[i] = ctu_height_in_pic - sum;
                }  else {
                    RK_S32    pic_in_cts_width = (width +
                                                  (1 << (log2_min_cb_size +
                                                         dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                                 / (1 << (log2_min_cb_size +
                                                          dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size));
                    RK_S32 pic_in_cts_height = (height +
                                                (1 << (log2_min_cb_size +
                                                       dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size)) - 1)
                                               / (1 << (log2_min_cb_size +
                                                        dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size));

                    for (i = 0; i < dxva_ctx->pp.num_tile_columns_minus1 + 1; i++)
                        column_width[i] = ((i + 1) * pic_in_cts_width) / (dxva_ctx->pp.num_tile_columns_minus1 + 1) -
                                          (i * pic_in_cts_width) / (dxva_ctx->pp.num_tile_columns_minus1 + 1);

                    for (i = 0; i < dxva_ctx->pp.num_tile_rows_minus1 + 1; i++)
                        row_height[i] = ((i + 1) * pic_in_cts_height) / (dxva_ctx->pp.num_tile_rows_minus1 + 1) -
                                        (i * pic_in_cts_height) / (dxva_ctx->pp.num_tile_rows_minus1 + 1);
                }
            } else {
                RK_S32 MaxCUWidth = (1 << (dxva_ctx->pp.log2_diff_max_min_luma_coding_block_size + log2_min_cb_size));
                column_width[0] = (width  + MaxCUWidth - 1) / MaxCUWidth;
                row_height[0]   = (height + MaxCUWidth - 1) / MaxCUWidth;
            }

            for (i = 0; i < 20; i++)
                mpp_put_bits(&bp, column_width[i], 12);

            for (i = 0; i < 22; i++)
                mpp_put_bits(&bp, row_height[i], 12);
        }

        /* update rps */
        if (dxva_ctx->pp.rps_update_flag) {
            Short_SPS_RPS_HEVC *cur_st_rps_ptr = &dxva_ctx->pp.cur_st_rps;

            for (i = 0; i < 32; i ++) {
                mpp_put_bits(&bp, dxva_ctx->pp.sps_lt_rps[i].lt_ref_pic_poc_lsb, 16);
                mpp_put_bits(&bp, dxva_ctx->pp.sps_lt_rps[i].used_by_curr_pic_lt_flag, 1);
            }

            mpp_put_bits(&bp, cur_st_rps_ptr->num_negative_pics, 4);
            mpp_put_bits(&bp, cur_st_rps_ptr->num_positive_pics, 4);

            for (i = 0; i <  cur_st_rps_ptr->num_negative_pics; i++) {
                mpp_put_bits(&bp, cur_st_rps_ptr->delta_poc_s0[i], 16);
                mpp_put_bits(&bp, cur_st_rps_ptr->s0_used_flag[i], 1);
            }

            for (i = 0; i <  cur_st_rps_ptr->num_positive_pics; i++) {
                mpp_put_bits(&bp, cur_st_rps_ptr->delta_poc_s1[i], 16);
                mpp_put_bits(&bp, cur_st_rps_ptr->s1_used_flag[i], 1);
            }

            for ( i = cur_st_rps_ptr->num_negative_pics + cur_st_rps_ptr->num_positive_pics; i < 15; i++) {
                mpp_put_bits(&bp, 0, 16);
                mpp_put_bits(&bp, 0, 1);
            }
            mpp_put_align(&bp, 64, 0);//128
        }
        memcpy(pps_ptr, reg_ctx->pps_buf, SPSPPS_ALIGNED_SIZE);
    } /* --- end spspps data ------*/

    if (dxva_ctx->pp.scaling_list_enabled_flag) {
        RK_U32 addr;
        RK_U8 *ptr_scaling = (RK_U8 *)mpp_buffer_get_ptr(reg_ctx->bufs) + reg_ctx->sclst_offset;

        if (dxva_ctx->pp.scaling_list_data_present_flag) {
            addr = (dxva_ctx->pp.pps_id + 16) * 1360;
        } else if (dxva_ctx->pp.scaling_list_enabled_flag) {
            addr = dxva_ctx->pp.sps_id * 1360;
        } else {
            addr = 80 * 1360;
        }

        hal_h265d_vdpu384a_scalinglist_packet(hal, ptr_scaling + addr, dxva);

        hw_reg->common_addr.reg132_scanlist_addr = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(reg_ctx->dev, 132, addr + reg_ctx->sclst_offset);
    }

#ifdef dump
    fwrite(pps_ptr, 1, 80 * 64, fp);
    RK_U32 *tmp = (RK_U32 *)pps_ptr;
    for (i = 0; i < 112 / 4; i++) {
        mpp_log("pps[%3d] = 0x%08x\n", i, tmp[i]);
    }
#endif
#ifdef DUMP_VDPU384A_DATAS
    {
        char *cur_fname = "global_cfg.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)bp.pbuf, 18*128, 128, 0);
    }
#endif

    return 0;
}

static void h265d_refine_rcb_size(Vdpu384aRcbInfo *rcb_info,
                                  RK_S32 width, RK_S32 height, void *dxva)
{
    RK_U32 rcb_bits = 0;
    h265d_dxva2_picture_context_t *dxva_ctx = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_ctx->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 4202 ,422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 tile_row_cut_num = pp->num_tile_rows_minus1;
    RK_U32 tile_col_cut_num = pp->num_tile_columns_minus1;
    RK_U32 ext_row_align_size = tile_row_cut_num * 64 * 8;
    RK_U32 ext_col_align_size = tile_col_cut_num * 64 * 8;
    RK_U32 filterd_row_append = 8192;
    RK_U32 row_uv_para = 0;
    RK_U32 col_uv_para = 0;

    if (chroma_fmt_idc == 1) {
        row_uv_para = 1;
        col_uv_para = 1;
    } else if (chroma_fmt_idc == 2) {
        row_uv_para = 1;
        col_uv_para = 3;
    } else if (chroma_fmt_idc == 3) {
        row_uv_para = 3;
        col_uv_para = 3;
    }

    width = MPP_ALIGN(width, ctu_size);
    height = MPP_ALIGN(height, ctu_size);
    /* RCB_STRMD_ROW && RCB_STRMD_TILE_ROW*/
    rcb_info[RCB_STRMD_ROW].size = 0;
    rcb_info[RCB_STRMD_TILE_ROW].size = 0;

    /* RCB_INTER_ROW && RCB_INTER_TILE_ROW*/
    rcb_bits = ((width + 7) / 8) * 174;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_cut_num)
        rcb_info[RCB_INTER_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTER_TILE_ROW].size = 0;

    /* RCB_INTRA_ROW && RCB_INTRA_TILE_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 4; //TODO:
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_cut_num)
        rcb_info[RCB_INTRA_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTRA_TILE_ROW].size = 0;

    /* RCB_FILTERD_ROW && RCB_FILTERD_TILE_ROW*/
    rcb_bits = (MPP_ALIGN(width, 64) * (1.2 * bit_depth + 0.5) * (8 + 5 * row_uv_para));
    // save space mode : half for RCB_FILTERD_ROW, half for RCB_FILTERD_PROTECT_ROW
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_info[RCB_FILTERD_ROW].size = MPP_RCB_BYTES(rcb_bits / 2) + filterd_row_append;
    rcb_info[RCB_FILTERD_PROTECT_ROW].size = MPP_RCB_BYTES(rcb_bits / 2) + filterd_row_append;
    rcb_bits += ext_row_align_size;
    if (tile_row_cut_num)
        rcb_info[RCB_FILTERD_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_FILTERD_TILE_ROW].size = 0;

    /* RCB_FILTERD_TILE_COL */
    if (tile_col_cut_num) {
        rcb_bits = (MPP_ALIGN(height, 64) * (1.6 * bit_depth + 0.5) * (16.5 + 5.5 * col_uv_para)) + ext_col_align_size;
        rcb_info[RCB_FILTERD_TILE_COL].size = MPP_RCB_BYTES(rcb_bits);
    } else {
        rcb_info[RCB_FILTERD_TILE_COL].size = 0;
    }

}

static void hal_h265d_rcb_info_update(void *hal,  void *dxva,
                                      Vdpu384aH265dRegSet *hw_regs,
                                      RK_S32 width, RK_S32 height)
{
    HalH265dCtx *reg_ctx = ( HalH265dCtx *)hal;
    h265d_dxva2_picture_context_t *dxva_ctx = (h265d_dxva2_picture_context_t*)dxva;
    DXVA_PicParams_HEVC *pp = &dxva_ctx->pp;
    RK_U32 chroma_fmt_idc = pp->chroma_format_idc;//0 400,1 4202 ,422,3 444
    RK_U8 bit_depth = MPP_MAX(pp->bit_depth_luma_minus8, pp->bit_depth_chroma_minus8) + 8;
    RK_U8 ctu_size = 1 << (pp->log2_diff_max_min_luma_coding_block_size + pp->log2_min_luma_coding_block_size_minus3 + 3);
    RK_U32 num_tiles = pp->num_tile_rows_minus1 + 1;
    (void)hw_regs;

    if (reg_ctx->num_row_tiles != num_tiles ||
        reg_ctx->bit_depth != bit_depth ||
        reg_ctx->chroma_fmt_idc != chroma_fmt_idc ||
        reg_ctx->ctu_size !=  ctu_size ||
        reg_ctx->width != width ||
        reg_ctx->height != height) {
        RK_U32 i = 0;
        RK_U32 loop = reg_ctx->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->g_buf) : 1;

        reg_ctx->rcb_buf_size = vdpu384a_get_rcb_buf_size((Vdpu384aRcbInfo *)reg_ctx->rcb_info, width, height);
        h265d_refine_rcb_size((Vdpu384aRcbInfo *)reg_ctx->rcb_info, width, height, dxva_ctx);
        /* vdpu384a_check_rcb_buf_size((Vdpu384aRcbInfo *)reg_ctx->rcb_info, width, height); */

        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf;

            if (reg_ctx->rcb_buf[i]) {
                mpp_buffer_put(reg_ctx->rcb_buf[i]);
                reg_ctx->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(reg_ctx->group, &rcb_buf, reg_ctx->rcb_buf_size);
            reg_ctx->rcb_buf[i] = rcb_buf;
        }

        reg_ctx->num_row_tiles  = num_tiles;
        reg_ctx->bit_depth      = bit_depth;
        reg_ctx->chroma_fmt_idc = chroma_fmt_idc;
        reg_ctx->ctu_size       = ctu_size;
        reg_ctx->width          = width;
        reg_ctx->height         = height;
    }
}

static RK_S32 calc_mv_size(RK_S32 pic_w, RK_S32 pic_h, RK_S32 ctu_w)
{
    RK_S32 seg_w = 64 * 16 * 16 / ctu_w; // colmv_block_size = 16, colmv_per_bytes = 16
    RK_S32 seg_cnt_w = MPP_ALIGN(pic_w, seg_w) / seg_w;
    RK_S32 seg_cnt_h = MPP_ALIGN(pic_h, ctu_w) / ctu_w;
    RK_S32 mv_size   = seg_cnt_w * seg_cnt_h * 64 * 16;

    return mv_size;
}

static MPP_RET hal_h265d_vdpu384a_gen_regs(void *hal,  HalTaskInfo *syn)
{
    RK_S32 i = 0;
    RK_S32 log2_min_cb_size;
    RK_S32 width, height;
    RK_S32 stride_y, stride_uv, virstrid_y;
    Vdpu384aH265dRegSet *hw_regs;
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

    if (reg_ctx ->fast_mode) {
        for (i = 0; i < MAX_GEN_REG; i++) {
            if (!reg_ctx->g_buf[i].use_flag) {
                syn->dec.reg_index = i;

                reg_ctx->spspps_offset = reg_ctx->offset_spspps[i];
                reg_ctx->sclst_offset = reg_ctx->offset_sclst[i];

                reg_ctx->hw_regs = reg_ctx->g_buf[i].hw_regs;
                reg_ctx->g_buf[i].use_flag = 1;
                break;
            }
        }
        if (i == MAX_GEN_REG) {
            mpp_err("hevc rps buf all used");
            return MPP_ERR_NOMEM;
        }
    }

    if (syn->dec.syntax.data == NULL) {
        mpp_err("%s:%s:%d dxva is NULL", __FILE__, __FUNCTION__, __LINE__);
        return MPP_ERR_NULL_PTR;
    }

#ifdef DUMP_VDPU384A_DATAS
    {
        memset(dump_cur_dir, 0, sizeof(dump_cur_dir));
        sprintf(dump_cur_dir, "/data/hevc/Frame%04d", dump_cur_frame);
        if (access(dump_cur_dir, 0)) {
            if (mkdir(dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", dump_cur_dir);
        }
        dump_cur_frame++;
    }
#endif

    /* output pps */
    hw_regs = (Vdpu384aH265dRegSet*)reg_ctx->hw_regs;
    memset(hw_regs, 0, sizeof(Vdpu384aH265dRegSet));

    if (NULL == reg_ctx->hw_regs) {
        return MPP_ERR_NULL_PTR;
    }


    log2_min_cb_size = dxva_ctx->pp.log2_min_luma_coding_block_size_minus3 + 3;
    width = (dxva_ctx->pp.PicWidthInMinCbsY << log2_min_cb_size);
    height = (dxva_ctx->pp.PicHeightInMinCbsY << log2_min_cb_size);
    mv_size = calc_mv_size(width, height, 1 << log2_min_cb_size) * 2;

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
        RK_U32 ver_virstride;
        RK_U32 virstrid_uv;
        MppFrameFormat fmt;
        RK_U32 chroma_fmt_idc = dxva_ctx->pp.chroma_format_idc;

        mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        /* for 8K downscale mode*/
        if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
            reg_ctx->origin_bufs == NULL) {
            vdpu384a_setup_scale_origin_bufs(reg_ctx, mframe);
        }

        fmt = mpp_frame_get_fmt(mframe);

        stride_y = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        stride_uv = stride_y;
        virstrid_y = ver_virstride * stride_y;
        if (chroma_fmt_idc == 3)
            stride_uv *= 2;
        if (chroma_fmt_idc == 3 || chroma_fmt_idc == 2) {
            virstrid_uv = stride_uv * ver_virstride;
        } else {
            virstrid_uv = stride_uv * ver_virstride / 2;
        }
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset;

            hw_regs->ctrl_regs.reg9.dpb_data_sel = 0;
            hw_regs->ctrl_regs.reg9.dpb_output_dis = 0;
            hw_regs->ctrl_regs.reg9.pp_m_output_mode = 0;

            hw_regs->h265d_paras.reg68_dpb_hor_virstride = fbc_hdr_stride / 64;
            fbd_offset = fbc_hdr_stride * MPP_ALIGN(ver_virstride, 64) / 16;
            hw_regs->h265d_addrs.reg193_dpb_fbc64x4_payload_offset = fbd_offset;
            hw_regs->h265d_paras.reg80_error_ref_hor_virstride = hw_regs->h265d_paras.reg68_dpb_hor_virstride;
        } else if (MPP_FRAME_FMT_IS_TILE(fmt)) {
            hw_regs->ctrl_regs.reg9.dpb_data_sel = 1;
            hw_regs->ctrl_regs.reg9.dpb_output_dis = 1;
            hw_regs->ctrl_regs.reg9.pp_m_output_mode = 2;

            if (chroma_fmt_idc == 0) { //yuv400
                hw_regs->h265d_paras.reg77_pp_m_hor_stride = stride_y * 4 / 16;
            } else if (chroma_fmt_idc == 2) { //yuv422
                hw_regs->h265d_paras.reg77_pp_m_hor_stride = stride_y * 8 / 16;
            } else if (chroma_fmt_idc == 3) { //yuv444
                hw_regs->h265d_paras.reg77_pp_m_hor_stride = stride_y * 12 / 16;
            } else { //yuv420
                hw_regs->h265d_paras.reg77_pp_m_hor_stride = stride_y * 6 / 16;
            }
            hw_regs->h265d_paras.reg79_pp_m_y_virstride = (virstrid_y + virstrid_uv) / 16;
            hw_regs->h265d_paras.reg80_error_ref_hor_virstride = hw_regs->h265d_paras.reg77_pp_m_hor_stride;
        } else {
            hw_regs->ctrl_regs.reg9.dpb_data_sel = 1;
            hw_regs->ctrl_regs.reg9.dpb_output_dis = 1;
            hw_regs->ctrl_regs.reg9.pp_m_output_mode = 1;

            hw_regs->h265d_paras.reg77_pp_m_hor_stride = stride_y >> 4;
            hw_regs->h265d_paras.reg78_pp_m_uv_hor_stride = stride_uv >> 4;
            hw_regs->h265d_paras.reg79_pp_m_y_virstride = virstrid_y >> 4;
            hw_regs->h265d_paras.reg80_error_ref_hor_virstride = hw_regs->h265d_paras.reg77_pp_m_hor_stride;
        }
        hw_regs->h265d_paras.reg81_error_ref_raster_uv_hor_virstride = hw_regs->h265d_paras.reg78_pp_m_uv_hor_stride;
        hw_regs->h265d_paras.reg82_error_ref_virstride = hw_regs->h265d_paras.reg79_pp_m_y_virstride;
    }
    mpp_buf_slot_get_prop(reg_ctx->slots, dxva_ctx->pp.CurrPic.Index7Bits,
                          SLOT_BUFFER, &framebuf);

    if (reg_ctx->origin_bufs) {
        origin_buf = hal_bufs_get_buf(reg_ctx->origin_bufs,
                                      dxva_ctx->pp.CurrPic.Index7Bits);
        framebuf = origin_buf->buf[0];
    }

    /* output rkfbc64 */
    // hw_regs->h265d_addrs.reg168_dpb_decout_base = mpp_buffer_get_fd(framebuf); //just index need map
    /* output raster/tile4x4 */
    hw_regs->common_addr.reg135_pp_m_decout_base = mpp_buffer_get_fd(framebuf); //just index need map
    hw_regs->h265d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(framebuf);
    /*if out_base is equal to zero it means this frame may error
    we return directly add by csy*/

    /* output rkfbc64 */
    // if (!hw_regs->h265d_addrs.reg168_dpb_decout_base)
    //     return 0;
    /* output raster/tile4x4 */
    if (!hw_regs->common_addr.reg135_pp_m_decout_base)
        return 0;

    fd =  mpp_buffer_get_fd(framebuf);
    /* output rkfbc64 */
    // hw_regs->h265d_addrs.reg168_dpb_decout_base = fd;
    /* output raster/tile4x4 */
    hw_regs->common_addr.reg135_pp_m_decout_base = fd;
    hw_regs->h265d_addrs.reg192_dpb_payload64x4_st_cur_base = fd;
    mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_ctx->pp.CurrPic.Index7Bits);

    hw_regs->h265d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_VDPU384A_DATAS
    {
        char *cur_fname = "colmv_cur_frame.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                          mpp_buffer_get_size(mv_buf->buf[0]), 64, 0);
    }
#endif

    mpp_buf_slot_get_prop(reg_ctx->packet_slots, syn->dec.input, SLOT_BUFFER,
                          &streambuf);
    if ( dxva_ctx->bitstream == NULL) {
        dxva_ctx->bitstream = mpp_buffer_get_ptr(streambuf);
    }

#ifdef DUMP_VDPU384A_DATAS
    {
        char *cur_fname = "stream_in_128bit.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(streambuf),
                          mpp_buffer_get_size(streambuf), 128, 0);
    }
#endif

    hw_regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(streambuf);
    hw_regs->h265d_paras.reg66_stream_len = ((dxva_ctx->bitstream_size + 15) & (~15)) + 64;
    hw_regs->common_addr.reg129_stream_buf_st_base = mpp_buffer_get_fd(streambuf);
    hw_regs->common_addr.reg130_stream_buf_end_base = mpp_buffer_get_fd(streambuf);
    mpp_dev_set_reg_offset(reg_ctx->dev, 130, mpp_buffer_get_size(streambuf));
    aglin_offset =  hw_regs->h265d_paras.reg66_stream_len - dxva_ctx->bitstream_size;
    if (aglin_offset > 0)
        memset((void *)(dxva_ctx->bitstream + dxva_ctx->bitstream_size), 0, aglin_offset);

    /* common setting */
    hw_regs->ctrl_regs.reg8_dec_mode = 0; // hevc
    hw_regs->ctrl_regs.reg9.low_latency_en = 0;

    hw_regs->ctrl_regs.reg10.strmd_auto_gating_e      = 1;
    hw_regs->ctrl_regs.reg10.inter_auto_gating_e      = 1;
    hw_regs->ctrl_regs.reg10.intra_auto_gating_e      = 1;
    hw_regs->ctrl_regs.reg10.transd_auto_gating_e     = 1;
    hw_regs->ctrl_regs.reg10.recon_auto_gating_e      = 1;
    hw_regs->ctrl_regs.reg10.filterd_auto_gating_e    = 1;
    hw_regs->ctrl_regs.reg10.bus_auto_gating_e        = 1;
    hw_regs->ctrl_regs.reg10.ctrl_auto_gating_e       = 1;
    hw_regs->ctrl_regs.reg10.rcb_auto_gating_e        = 1;
    hw_regs->ctrl_regs.reg10.err_prc_auto_gating_e    = 1;

    hw_regs->ctrl_regs.reg11.rd_outstanding = 32;
    hw_regs->ctrl_regs.reg11.wr_outstanding = 250;
    // hw_regs->ctrl_regs.reg11.dec_timeout_dis = 1;

    hw_regs->ctrl_regs.reg16.error_proc_disable = 1;
    hw_regs->ctrl_regs.reg16.error_spread_disable = 0;
    hw_regs->ctrl_regs.reg16.roi_error_ctu_cal_en = 0;

    hw_regs->ctrl_regs.reg20_cabac_error_en_lowbits = 0xffffffff;
    hw_regs->ctrl_regs.reg21_cabac_error_en_highbits = 0x3ff3f9ff;

    hw_regs->ctrl_regs.reg13_core_timeout_threshold = 0xffff;


    /* output rkfbc64 */
    // valid_ref = hw_regs->h265d_addrs.reg168_dpb_decout_base;
    /* output raster/tile4x4 */
    valid_ref = hw_regs->common_addr.reg135_pp_m_decout_base;
    reg_ctx->error_index[syn->dec.reg_index] = dxva_ctx->pp.CurrPic.Index7Bits;

    hw_regs->h265d_addrs.reg169_error_ref_base = valid_ref;
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
                hw_regs->h265d_addrs.reg170_185_ref_base[i] = mpp_buffer_get_fd(framebuf);
                hw_regs->h265d_addrs.reg195_210_payload_st_ref_base[i] = mpp_buffer_get_fd(framebuf);
                valid_ref = hw_regs->h265d_addrs.reg170_185_ref_base[i];
                if ((pocdistance(dxva_ctx->pp.PicOrderCntValList[i], dxva_ctx->pp.current_poc) < distance)
                    && (!mpp_frame_get_errinfo(mframe))) {

                    distance = pocdistance(dxva_ctx->pp.PicOrderCntValList[i], dxva_ctx->pp.current_poc);
                    hw_regs->h265d_addrs.reg169_error_ref_base = hw_regs->h265d_addrs.reg170_185_ref_base[i];
                    reg_ctx->error_index[syn->dec.reg_index] = dxva_ctx->pp.RefPicList[i].Index7Bits;
                    hw_regs->ctrl_regs.reg16.error_proc_disable = 1;
                }
            } else {
                hw_regs->h265d_addrs.reg170_185_ref_base[i] = valid_ref;
                hw_regs->h265d_addrs.reg195_210_payload_st_ref_base[i] = valid_ref;
            }

            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, dxva_ctx->pp.RefPicList[i].Index7Bits);
            hw_regs->h265d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }
    }

    if ((reg_ctx->error_index[syn->dec.reg_index] == dxva_ctx->pp.CurrPic.Index7Bits) &&
        !dxva_ctx->pp.IntraPicFlag) {
        h265h_dbg(H265H_DBG_TASK_ERR, "current frm may be err, should skip process");
        syn->dec.flags.ref_err = 1;
        return MPP_OK;
    }

    /* pps */
    hw_regs->common_addr.reg131_gbl_base = reg_ctx->bufs_fd;
    hw_regs->h265d_paras.reg67_global_len = SPSPPS_ALIGNED_SIZE / 16;

    mpp_dev_set_reg_offset(reg_ctx->dev, 131, reg_ctx->spspps_offset);

    hal_h265d_v345_output_pps_packet(hal, syn->dec.syntax.data);

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
                hw_regs->h265d_addrs.reg170_185_ref_base[i] = hw_regs->h265d_addrs.reg169_error_ref_base;
                hw_regs->h265d_addrs.reg195_210_payload_st_ref_base[i] = hw_regs->h265d_addrs.reg169_error_ref_base;
                hw_regs->h265d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
            }
        } else {
            mv_buf = hal_bufs_get_buf(reg_ctx->cmv_bufs, reg_ctx->error_index[syn->dec.reg_index]);
            hw_regs->h265d_addrs.reg170_185_ref_base[i] = hw_regs->h265d_addrs.reg169_error_ref_base;
            hw_regs->h265d_addrs.reg195_210_payload_st_ref_base[i] = hw_regs->h265d_addrs.reg169_error_ref_base;
            hw_regs->h265d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }
    }

    hal_h265d_rcb_info_update(hal, dxva_ctx, hw_regs, width, height);
    vdpu384a_setup_rcb(&hw_regs->common_addr, reg_ctx->dev, reg_ctx->fast_mode ?
                       reg_ctx->rcb_buf[syn->dec.reg_index] : reg_ctx->rcb_buf[0],
                       (Vdpu384aRcbInfo *)reg_ctx->rcb_info);
    vdpu384a_setup_statistic(&hw_regs->ctrl_regs);
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
            hw_regs->common_addr.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            origin_buf = hal_bufs_get_buf(reg_ctx->origin_bufs, dxva_ctx->pp.CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            /* output rkfbc64 */
            // hw_regs->h265d_addrs.reg168_dpb_decout_base = fd;
            /* output raster/tile4x4 */
            hw_regs->common_addr.reg135_pp_m_decout_base = fd;
            hw_regs->h265d_addrs.reg192_dpb_payload64x4_st_cur_base = fd;
            hw_regs->h265d_addrs.reg169_error_ref_base = fd;
            vdpu384a_setup_down_scale(mframe, reg_ctx->dev, &hw_regs->ctrl_regs, (void*)&hw_regs->h265d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            hw_regs->common_addr.reg133_scale_down_base = mpp_buffer_get_fd(mbuffer);
            vdpu384a_setup_down_scale(mframe, reg_ctx->dev, &hw_regs->ctrl_regs, (void*)&hw_regs->h265d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            hw_regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

    return ret;
}

static MPP_RET hal_h265d_vdpu384a_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8* p = NULL;
    Vdpu384aH265dRegSet *hw_regs = NULL;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_S32 index =  task->dec.reg_index;

    RK_U32 i;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !reg_ctx->cfg->base.disable_error)) {
        h265h_dbg(H265H_DBG_TASK_ERR, "%s found task error\n", __FUNCTION__);
        return MPP_OK;
    }

    if (reg_ctx->fast_mode) {
        p = (RK_U8*)reg_ctx->g_buf[index].hw_regs;
        hw_regs = ( Vdpu384aH265dRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        p = (RK_U8*)reg_ctx->hw_regs;
        hw_regs = ( Vdpu384aH265dRegSet *)reg_ctx->hw_regs;
    }

    if (hw_regs == NULL) {
        mpp_err("hal_h265d_start hw_regs is NULL");
        return MPP_ERR_NULL_PTR;
    }
    for (i = 0; i < 68; i++) {
        h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                  i, *((RK_U32*)p));
        //mpp_log("RK_HEVC_DEC: regs[%02d]=%08X\n", i, *((RK_U32*)p));
        p += 4;
    }

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &hw_regs->ctrl_regs;
        wr_cfg.size = sizeof(hw_regs->ctrl_regs);
        wr_cfg.offset = OFFSET_CTRL_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->common_addr;
        wr_cfg.size = sizeof(hw_regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_paras;
        wr_cfg.size = sizeof(hw_regs->h265d_paras);
        wr_cfg.offset = OFFSET_CODEC_PARAS_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &hw_regs->h265d_addrs;
        wr_cfg.size = sizeof(hw_regs->h265d_addrs);
        wr_cfg.offset = OFFSET_CODEC_ADDR_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &hw_regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(hw_regs->ctrl_regs.reg15);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu384a_set_rcbinfo(reg_ctx->dev, (Vdpu384aRcbInfo*)reg_ctx->rcb_info);

        ret = mpp_dev_ioctl(reg_ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    return ret;
}


static MPP_RET hal_h265d_vdpu384a_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    RK_S32 index =  task->dec.reg_index;
    HalH265dCtx *reg_ctx = (HalH265dCtx *)hal;
    RK_U8* p = NULL;
    Vdpu384aH265dRegSet *hw_regs = NULL;
    RK_S32 i;

    if (reg_ctx->fast_mode) {
        hw_regs = ( Vdpu384aH265dRegSet *)reg_ctx->g_buf[index].hw_regs;
    } else {
        hw_regs = ( Vdpu384aH265dRegSet *)reg_ctx->hw_regs;
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

    for (i = 0; i < 68; i++) {
        if (i == 1) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }

        if (i == 45) {
            h265h_dbg(H265H_DBG_REG, "RK_HEVC_DEC: regs[%02d]=%08X\n",
                      i, *((RK_U32*)p));
        }
        p += 4;
    }

    if (reg_ctx->fast_mode) {
        reg_ctx->g_buf[index].use_flag = 0;
    }

    return ret;
}

static MPP_RET hal_h265d_vdpu384a_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;
    p_hal->fast_mode_err_found = 0;
    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu384a_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

static MPP_RET hal_h265d_vdpu384a_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    HalH265dCtx *p_hal = (HalH265dCtx *)hal;

    (void)hal;
    (void)param;
    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        MppFrame frame = (MppFrame)param;
        MppFrameFormat fmt = mpp_frame_get_fmt(frame);
        RK_U32 imgwidth = mpp_frame_get_width((MppFrame)param);
        RK_U32 imgheight = mpp_frame_get_height((MppFrame)param);

        if (fmt == MPP_FMT_YUV422SP) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
        } else if (fmt == MPP_FMT_YUV444SP) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align_444);
        }
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu384a_afbc_align_calc(p_hal->slots, frame, 16);
        } else if (imgwidth > 1920 || imgheight > 1088) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
        }
        break;
    }
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO: {
        vdpu384a_update_thumbnail_frame_info((MppFrame)param);
    } break;
    case MPP_DEC_SET_OUTPUT_FORMAT: {
    } break;
    default: {
    } break;
    }
    return  ret;
}

const MppHalApi hal_h265d_vdpu384a = {
    .name = "h265d_vdpu384a",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(HalH265dCtx),
    .flag = 0,
    .init = hal_h265d_vdpu384a_init,
    .deinit = hal_h265d_vdpu384a_deinit,
    .reg_gen = hal_h265d_vdpu384a_gen_regs,
    .start = hal_h265d_vdpu384a_start,
    .wait = hal_h265d_vdpu384a_wait,
    .reset = hal_h265d_vdpu384a_reset,
    .flush = hal_h265d_vdpu384a_flush,
    .control = hal_h265d_vdpu384a_control,
};
