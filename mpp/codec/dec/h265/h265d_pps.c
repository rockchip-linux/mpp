/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "h265d"

#include "mpp_mem.h"

#include "h265d_debug.h"
#include "h265d_ctx.h"

H265dPps *h265d_nal_pps(BitReadCtx_t *bit, const H265dSps *sps_list[])
{
    H265dSps *sps = NULL;
    H265dPps *pps = NULL;
    RK_U32 pps_id = 0;
    RK_S32 ret = 0;
    RK_S32 i;

    h265d_dbg_func("Decoding PPS\n");

    pps = (H265dPps *)mpp_calloc(H265dPps, 1);
    if (!pps)
        return NULL;

    READ_UE(bit, &pps_id);
    if (pps_id >= MAX_PPS_COUNT) {
        mpp_loge_f("pps: pps_id %d over MAX_PPS_COUNT\n", pps_id);
        goto err;
    }

    pps->pps_id = pps_id;
    h265d_dbg_pps("pps: pps_id %d", pps->pps_id);
    pps->loop_filter_across_tiles_enabled_flag = 1;
    pps->num_tile_columns = 1;
    pps->num_tile_rows = 1;
    pps->uniform_spacing_flag = 1;

    READ_UE(bit, &pps->sps_id);
    if (pps->sps_id >= MAX_SPS_COUNT) {
        mpp_loge_f("pps: sps_id %d over MAX_SPS_COUNT\n", pps->sps_id);
        goto err;
    }
    h265d_dbg_pps("pps: sps_id %d", pps->sps_id);
    if (!sps_list[pps->sps_id]) {
        mpp_loge_f("pps: sps %u missing\n", pps->sps_id);
        goto err;
    }
    sps = (H265dSps *)sps_list[pps->sps_id];

    READ_ONEBIT(bit, &pps->dependent_slice_segments_enabled_flag);
    READ_ONEBIT(bit, &pps->output_flag_present_flag );
    READ_BITS(bit, 3, &pps->num_extra_slice_header_bits);

    READ_ONEBIT(bit, &pps->sign_data_hiding_flag);

    READ_ONEBIT(bit, &pps->cabac_init_present_flag);

    READ_UE(bit, &pps->num_ref_idx_l0_default_active);
    pps->num_ref_idx_l0_default_active +=  1;
    READ_UE(bit, &pps->num_ref_idx_l1_default_active);
    pps->num_ref_idx_l1_default_active += 1;
    h265d_dbg_pps("pps: num_ref_idx_l0_default_active %d", pps->num_ref_idx_l0_default_active);
    h265d_dbg_pps("pps: num_ref_idx_l1_default_active %d", pps->num_ref_idx_l1_default_active);

    READ_SE(bit, &pps->pic_init_qp_minus26);
    h265d_dbg_pps("pps: pic_init_qp_minus26 %d", pps->pic_init_qp_minus26);

    READ_ONEBIT(bit, &pps->constrained_intra_pred_flag);
    READ_ONEBIT(bit, &pps->transform_skip_enabled_flag);

    READ_ONEBIT(bit, &pps->cu_qp_delta_enabled_flag);
    pps->diff_cu_qp_delta_depth = 0;
    if (pps->cu_qp_delta_enabled_flag)
        READ_UE(bit, &pps->diff_cu_qp_delta_depth);

    READ_SE(bit, &pps->cb_qp_offset);
    h265d_dbg_pps("pps: cb_qp_offset %d", pps->cb_qp_offset);
    if (pps->cb_qp_offset < -12 || pps->cb_qp_offset > 12) {
        mpp_loge_f("pps: cb_qp_offset invalid %d\n",
                   pps->cb_qp_offset);
        goto err;
    }
    READ_SE(bit, &pps->cr_qp_offset);
    h265d_dbg_pps("pps: cr_qp_offset %d", pps->cr_qp_offset);
    if (pps->cr_qp_offset < -12 || pps->cr_qp_offset > 12) {
        mpp_loge_f("pps: cr_qp_offset invalid %d\n",
                   pps->cr_qp_offset);
        goto err;
    }

    READ_ONEBIT(bit, &pps->pic_slice_level_chroma_qp_offsets_present_flag);
    READ_ONEBIT(bit, &pps->weighted_pred_flag);
    READ_ONEBIT(bit, &pps->weighted_bipred_flag);
    READ_ONEBIT(bit, &pps->transquant_bypass_enable_flag);
    READ_ONEBIT(bit, &pps->tiles_enabled_flag);
    READ_ONEBIT(bit, &pps->entropy_coding_sync_enabled_flag);

    if (pps->tiles_enabled_flag) {
        RK_U32 *column_width;
        RK_U32 *row_height;

        READ_UE(bit, &pps->num_tile_columns);
        pps->num_tile_columns += 1;
        READ_UE(bit, &pps->num_tile_rows);
        pps->num_tile_rows += 1;
        if (pps->num_tile_columns == 0 || pps->num_tile_columns >= sps->width) {
            mpp_loge_f("pps: num_tile_columns %d invalid\n",
                       pps->num_tile_columns);
            goto err;
        }
        if (pps->num_tile_rows == 0 || pps->num_tile_rows >= sps->height) {
            mpp_loge_f("pps: num_tile_rows %d invalid\n",
                       pps->num_tile_rows);
            goto err;
        }
        h265d_dbg_pps("pps: num_tile_columns %d", pps->num_tile_columns);
        h265d_dbg_pps("pps: num_tile_rows %d", pps->num_tile_rows);

        RK_S32 column_buf_size = MPP_ALIGN(pps->num_tile_columns * sizeof(RK_U32), 8);
        RK_S32 row_buf_size = pps->num_tile_rows * sizeof(RK_U32);
        RK_S32 total_size = sizeof(H265dPps) + column_buf_size + row_buf_size;

        pps = (H265dPps *)mpp_realloc_size(pps, RK_U8, total_size);
        if (!pps)
            goto err;

        /* Calculate offsets from struct start to tile data */
        pps->column_width_offset = 0;
        pps->row_height_offset = column_buf_size;
        pps->column_width_size = pps->num_tile_columns;
        pps->row_height_size = pps->num_tile_rows;
        pps->total_size = total_size;

        /* Get pointers using offsets */
        column_width = H265D_PPS_COLUMN_WIDTH(pps);
        row_height = H265D_PPS_ROW_HEIGHT(pps);

        READ_ONEBIT(bit, &pps->uniform_spacing_flag);

        if (!pps->uniform_spacing_flag) {
            RK_S32 sum = 0;

            for (i = 0; i < pps->num_tile_columns - 1; i++) {
                READ_UE(bit, &column_width[i]);
                column_width[i] += 1;
                sum += column_width[i];
            }
            if (sum >= sps->ctb_width) {
                mpp_loge_f("pps: invalid tile widths\n");
                goto err;
            }
            column_width[pps->num_tile_columns - 1] = sps->ctb_width - sum;

            sum = 0;
            for (i = 0; i < pps->num_tile_rows - 1; i++) {
                READ_UE(bit, &row_height[i]);
                row_height[i] += 1;
                sum += row_height[i];
            }
            if (sum >= sps->ctb_height) {
                mpp_loge_f("pps: invalid tile heights\n");
                goto err;
            }
            row_height[pps->num_tile_rows - 1] = sps->ctb_height - sum;
        } else {
            for (i = 0; i < pps->num_tile_columns; i++) {
                column_width[i] = ((i + 1) * sps->ctb_width) / pps->num_tile_columns -
                                  (i * sps->ctb_width) / pps->num_tile_columns;
            }
            for (i = 0; i < pps->num_tile_rows; i++) {
                row_height[i] = ((i + 1) * sps->ctb_height) / pps->num_tile_rows -
                                (i * sps->ctb_height) / pps->num_tile_rows;
            }
        }
        READ_ONEBIT(bit, &pps->loop_filter_across_tiles_enabled_flag);
    } else {
        /* No tiles, set total_size to struct size only */
        pps->total_size = sizeof(H265dPps);
    }

    READ_ONEBIT(bit, &pps->seq_loop_filter_across_slices_enabled_flag);
    READ_ONEBIT(bit, &pps->deblocking_filter_control_present_flag);
    if (pps->deblocking_filter_control_present_flag) {
        READ_ONEBIT(bit, &pps->deblocking_filter_override_enabled_flag);
        READ_ONEBIT(bit, &pps->disable_dbf);
        if (!pps->disable_dbf) {
            READ_SE(bit, &pps->beta_offset);
            pps->beta_offset = pps->beta_offset * 2;
            h265d_dbg_pps("pps: beta_offset %d", pps->beta_offset);
            if (pps->beta_offset < -12 || pps->beta_offset > 12) {
                mpp_loge_f("pps: beta_offset invalid %d\n", pps->beta_offset / 2);
                goto err;
            }
            READ_SE(bit, &pps->tc_offset);
            pps->tc_offset = pps->tc_offset * 2;
            h265d_dbg_pps("pps: tc_offset %d", pps->tc_offset);
            if (pps->tc_offset < -12 || pps->tc_offset > 12) {
                mpp_loge_f("pps: tc_offset invalid %d\n", pps->tc_offset / 2);
                goto err;
            }
        }
    }

    READ_ONEBIT(bit, &pps->scaling_list_data_present_flag);
    if (pps->scaling_list_data_present_flag) {
        h265d_set_default_scaling_list(&pps->scaling_list);
        ret = h265d_scaling_list_data(bit, &pps->scaling_list, sps);
        if (ret < 0)
            goto err;
    }

    READ_ONEBIT(bit, &pps->lists_modification_present_flag);

    READ_UE(bit, &pps->log2_parallel_merge_level);
    pps->log2_parallel_merge_level += 2;
    h265d_dbg_pps("pps: log2_parallel_merge_level %d", pps->log2_parallel_merge_level);
    if (pps->log2_parallel_merge_level > sps->log2_ctb_size) {
        mpp_loge_f("pps: log2_parallel_merge_level %d over limit\n",
                   pps->log2_parallel_merge_level);
        goto err;
    }
    READ_ONEBIT(bit, &pps->slice_header_extension_present_flag);
    READ_ONEBIT(bit, &pps->pps_extension_flag);
    h265d_dbg_pps("pps: pps_extension_flag %d", pps->pps_extension_flag);
    if (pps->pps_extension_flag) {
        READ_ONEBIT(bit, &pps->pps_range_extensions_flag);
        SKIP_BITS(bit, 7);
    }

    return pps;
__BITREAD_ERR:
err:
    MPP_FREE(pps);
    return NULL;
}

RK_S32 h265d_pps_check(const H265dPps *pps, const H265dSps *sps,
                       const MppDecHwCap *hw_info)
{
    if (!sps) {
        mpp_loge("pps: sps %u unavailable\n", pps->sps_id);
        return MPP_ERR_STREAM;
    }

    if (!hw_info)
        return 0;

    if (hw_info->cap_lmt_linebuf) {
        RK_S32 max_supt_width = PIXW_1080P;
        RK_S32 max_supt_height = (pps->tiles_enabled_flag != 0) ? PIXH_1080P : PIXW_1080P;

        if (hw_info->cap_8k) {
            max_supt_width = PIXW_8Kx4K;
            max_supt_height = (pps->tiles_enabled_flag != 0) ? PIXH_8Kx4K : PIXW_8Kx4K;
        } else if (hw_info->cap_4k) {
            max_supt_width = PIXW_4Kx2K;
            max_supt_height = (pps->tiles_enabled_flag != 0) ? PIXH_4Kx2K : PIXW_4Kx2K;
        }

        if (sps->width > max_supt_width ||
            (sps->height > max_supt_height && pps->tiles_enabled_flag) ||
            sps->width * sps->height > max_supt_width * max_supt_width) {
            mpp_loge("pps: unsupported %dx%d max %dx%d\n",
                     sps->width, sps->height, max_supt_width, max_supt_height);
            return MPP_ERR_STREAM;
        }
    } else {
        if (sps->width * sps->height > PIXW_8Kx4K * PIXW_8Kx4K * hw_info->cap_core_num) {
            mpp_loge("pps: unsupported %dx%d max %dx%d\n",
                     sps->width, sps->height, PIXW_8Kx4K, PIXW_8Kx4K);
            return MPP_ERR_STREAM;
        }
    }

    return 0;
}
