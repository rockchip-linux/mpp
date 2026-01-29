/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "h265d"

#include "mpp_debug.h"
#include "mpp_bit.h"

#include "h265d_ctx.h"
#include "h265d_pps.h"
#include "h265d_syntax.h"

static void fill_picture_entry(DXVA_PicEntry_HEVC *pic, RK_U32 index, RK_U32 flag)
{
    mpp_assert((index & 0x7f) == index && (flag & 0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}

static RK_S32 get_refpic_index(const DXVA_PicParams_HEVC *pp, RK_S32 surface_index)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefPicList); i++) {
        if ((pp->RefPicList[i].bPicEntry & 0x7f) == surface_index) {
            return i;
        }
    }
    return 0xff;
}

static void fill_ref_pic_set(const DXVA_PicParams_HEVC *pp,
                             const H265dPrs *p,
                             RK_U32 rps_idx,
                             RK_U8 *target_array)
{
    const H265dRefList *src = &p->rps[rps_idx];
    RK_U32 write_pos = 0;
    RK_U32 read_pos = 0;

    for (write_pos = 0; write_pos < 15; write_pos++) {
        RK_S32 idx = 0xff;
        while (read_pos < (RK_U32)src->nb_refs && idx == 0xff) {
            H265dFrame *frm = src->ref[read_pos++];
            if (frm && frm->slot_index != 0xff)
                idx = get_refpic_index(pp, frm->slot_index);
        }
        target_array[write_pos] = idx;
    }
}

static void fill_picture_parameters(const H265dPrs *p, DXVA_PicParams_HEVC *pp)
{
    const H265dFrame *current_picture = p->ref;
    const H265dPps *pps = (H265dPps *)p->pps_list[p->slice.pps_id];
    const H265dSps *sps = (H265dSps *)p->sps_list[pps->sps_id];
    const H265dStRps *src_rps = sps->st_rps_sps;
    Short_SPS_RPS_HEVC *dst_rps = pp->sps_st_rps;
    const H265dStRps *cur_src_rps = p->slice.st_rps_using;
    Short_SPS_RPS_HEVC *cur_dst_rps = &pp->cur_st_rps;
    RK_U32 rps_used[16];
    RK_U32 nb_rps_used;
    RK_U32 i, j;

    memset(pp, 0, sizeof(*pp));

    pp->PicWidthInMinCbsY  = sps->min_cb_width;
    pp->PicHeightInMinCbsY = sps->min_cb_height;
    pp->pps_id = p->slice.pps_id;
    pp->sps_id = pps->sps_id;
    pp->vps_id = sps->vps_id;

    pp->wFormatAndSequenceInfoFlags = (sps->chroma_format_idc             <<  0) |
                                      (sps->separate_colour_plane_flag    <<  2) |
                                      ((sps->bit_depth - 8)               <<  3) |
                                      ((sps->bit_depth - 8)               <<  6) |
                                      ((sps->log2_max_poc_lsb - 4)        <<  9) |
                                      (0                                  << 13) |
                                      (0                                  << 14) |
                                      (0                                  << 15);

    fill_picture_entry(&pp->CurrPic, current_picture->slot_index, 0);

    pp->sps_max_dec_pic_buffering_minus1         = sps->layers[sps->max_sub_layers - 1].max_dec_pic_buffering - 1;
    pp->log2_min_luma_coding_block_size_minus3   = sps->log2_min_cb_size - 3;
    pp->log2_diff_max_min_luma_coding_block_size = sps->log2_diff_max_min_coding_block_size;
    pp->log2_min_transform_block_size_minus2     = sps->log2_min_tb_size - 2;
    pp->log2_diff_max_min_transform_block_size   = sps->log2_max_trafo_size  - sps->log2_min_tb_size;
    pp->max_transform_hierarchy_depth_inter      = sps->max_transform_hierarchy_depth_inter;
    pp->max_transform_hierarchy_depth_intra      = sps->max_transform_hierarchy_depth_intra;
    pp->num_short_term_ref_pic_sets              = sps->nb_st_rps;
    pp->num_long_term_ref_pics_sps               = sps->lt_rps_sps.num_refs;

    pp->num_ref_idx_l0_default_active_minus1     = pps->num_ref_idx_l0_default_active - 1;
    pp->num_ref_idx_l1_default_active_minus1     = pps->num_ref_idx_l1_default_active - 1;
    pp->init_qp_minus26                          = pps->pic_init_qp_minus26;

    if (p->slice.short_term_ref_pic_set_sps_flag == 0 && p->slice.st_rps_using) {
        pp->ucNumDeltaPocsOfRefRpsIdx            = p->slice.st_rps_using->ref_delta_cnt;
        pp->wNumBitsForShortTermRPSInSlice       = p->slice.short_term_ref_pic_set_size;
    }

    RK_U32 tool_flags = 0;
    RK_U32 pcm_flags = 0;
    RK_U32 rps_flags = 0;

    // SPS tool flags
    tool_flags |= (sps->scaling_list_enable_flag & 1) << 0;
    tool_flags |= (sps->amp_enabled_flag & 1) << 1;
    tool_flags |= (sps->sao_enabled & 1) << 2;
    tool_flags |= (sps->pcm_enabled_flag & 1) << 3;

    // PCM specific flags
    if (sps->pcm_enabled_flag) {
        pcm_flags |= ((sps->pcm.bit_depth - 1) & 0x0F) << 4;
        pcm_flags |= ((sps->pcm.bit_depth_chroma - 1) & 0x0F) << 8;
        pcm_flags |= ((sps->pcm.log2_min_pcm_cb_size - 3) & 0x03) << 12;
        pcm_flags |= ((sps->pcm.log2_max_pcm_cb_size - sps->pcm.log2_min_pcm_cb_size) & 0x03) << 14;
    }

    // RPS flags
    rps_flags |= (sps->pcm.loop_filter_disable_flag & 1) << 16;
    rps_flags |= (sps->lt_rps_sps.valid & 1) << 17;
    rps_flags |= (sps->sps_temporal_mvp_enabled_flag & 1) << 18;
    rps_flags |= (sps->sps_strong_intra_smoothing_enable_flag & 1) << 19;

    // PPS flags
    tool_flags |= (pps->dependent_slice_segments_enabled_flag & 1) << 20;
    tool_flags |= (pps->output_flag_present_flag & 1) << 21;
    tool_flags |= ((pps->num_extra_slice_header_bits & 0x07) << 22);
    tool_flags |= (pps->sign_data_hiding_flag & 1) << 25;
    tool_flags |= (pps->cabac_init_present_flag & 1) << 26;

    pp->dwCodingParamToolFlags = tool_flags | pcm_flags | rps_flags;

    pp->dwCodingSettingPicturePropertyFlags = (pps->constrained_intra_pred_flag                   <<  0) |
                                              (pps->transform_skip_enabled_flag                   <<  1) |
                                              (pps->cu_qp_delta_enabled_flag                      <<  2) |
                                              (pps->pic_slice_level_chroma_qp_offsets_present_flag <<  3) |
                                              (pps->weighted_pred_flag                            <<  4) |
                                              (pps->weighted_bipred_flag                          <<  5) |
                                              (pps->transquant_bypass_enable_flag                 <<  6) |
                                              (pps->tiles_enabled_flag                            <<  7) |
                                              (pps->entropy_coding_sync_enabled_flag              <<  8) |
                                              (pps->uniform_spacing_flag                          <<  9) |
                                              (pps->loop_filter_across_tiles_enabled_flag         << 10) |
                                              (pps->seq_loop_filter_across_slices_enabled_flag    << 11) |
                                              (pps->deblocking_filter_override_enabled_flag       << 12) |
                                              (pps->disable_dbf                                   << 13) |
                                              (pps->lists_modification_present_flag               << 14) |
                                              (pps->slice_header_extension_present_flag           << 15) |
                                              (0                                                  << 19);

    pp->IdrPicFlag = (p->first_nal_type == 19 || p->first_nal_type == 20);
    pp->IrapPicFlag = (p->first_nal_type >= 16 && p->first_nal_type <= 23);
    pp->IntraPicFlag =  (p->first_nal_type >= 16 && p->first_nal_type <= 23) || p->slice.slice_type == I_SLICE;
    pp->pps_cb_qp_offset            = pps->cb_qp_offset;
    pp->pps_cr_qp_offset            = pps->cr_qp_offset;
    if (pps->tiles_enabled_flag) {
        const RK_U32 *column_width = H265D_PPS_COLUMN_WIDTH(pps);
        const RK_U32 *row_height = H265D_PPS_ROW_HEIGHT(pps);

        pp->num_tile_columns_minus1 = pps->num_tile_columns - 1;
        pp->num_tile_rows_minus1    = pps->num_tile_rows - 1;

        if (!pps->uniform_spacing_flag) {
            for (i = 0; i < (RK_U32)pps->num_tile_columns; i++)
                pp->column_width_minus1[i] = column_width[i] - 1;

            for (i = 0; i < (RK_U32)pps->num_tile_rows; i++)
                pp->row_height_minus1[i] = row_height[i] - 1;
        }
    }

    pp->diff_cu_qp_delta_depth           = pps->diff_cu_qp_delta_depth;
    pp->pps_beta_offset_div2             = pps->beta_offset / 2;
    pp->pps_tc_offset_div2               = pps->tc_offset / 2;
    pp->log2_parallel_merge_level_minus2 = pps->log2_parallel_merge_level - 2;
    pp->slice_segment_header_extension_present_flag = pps->slice_header_extension_present_flag;
    pp->CurrPicOrderCntVal               = p->poc;
    pp->ps_update_flag                   = p->ps_need_upate;
    pp->rps_update_flag                  = p->rps_need_upate || p->ps_need_upate;

    if (pp->rps_update_flag) {
        for (i = 0; i < 32; i++) {
            pp->sps_lt_rps[i].lt_ref_pic_poc_lsb = sps->lt_rps_sps.poc_lsb[i];
            pp->sps_lt_rps[i].used_by_curr_pic_lt_flag = MPP_GET_BIT(sps->lt_rps_sps.used_flag, i);
        }

        if (cur_src_rps) {
            RK_U32 n_pics = p->slice.st_rps_using->num_neg;
            cur_dst_rps->num_negative_pics = n_pics;
            cur_dst_rps->num_positive_pics = cur_src_rps->num_deltas - n_pics;
            for (i = 0; i < cur_dst_rps->num_negative_pics; i++) {
                cur_dst_rps->delta_poc_s0[i] = cur_src_rps->poc_delta[i];
                cur_dst_rps->s0_used_flag[i] = MPP_GET_BIT(cur_src_rps->flags, i);
            }
            for (i = 0; i < cur_dst_rps->num_positive_pics; i++) {
                cur_dst_rps->delta_poc_s1[i] = cur_src_rps->poc_delta[i + n_pics];
                cur_dst_rps->s1_used_flag[i] = MPP_GET_BIT(cur_src_rps->flags, i + n_pics);
            }
        }

        for (i = 0; i < 64; i++) {
            if (i < sps->nb_st_rps) {

                RK_U32 n_pics = src_rps[i].num_neg;
                dst_rps[i].num_negative_pics = n_pics;
                dst_rps[i].num_positive_pics = src_rps[i].num_deltas - n_pics;
                for (j = 0; j < dst_rps[i].num_negative_pics; j++) {
                    dst_rps[i].delta_poc_s0[j] = src_rps[i].poc_delta[j];
                    dst_rps[i].s0_used_flag[j] = MPP_GET_BIT(src_rps[i].flags, j);
                }

                for ( j = 0; j < dst_rps[i].num_positive_pics; j++) {
                    dst_rps[i].delta_poc_s1[j] = src_rps[i].poc_delta[j + n_pics];
                    dst_rps[i].s1_used_flag[j] = MPP_GET_BIT(src_rps[i].flags, j + n_pics);
                }
            }
        }
    }

    nb_rps_used = 0;
    for (i = 0; i < NB_RPS_TYPE; i++) {
        for (j = 0; j < (RK_U32)p->rps[i].nb_refs; j++) {
            if ((i == ST_FOLL) || (i == LT_FOLL)) {
                ;
            } else {
                rps_used[nb_rps_used++] = p->rps[i].list[j];
            }
        }
    }
    pp->current_poc = current_picture->poc;

    // First pass: collect valid reference frames
    RK_U32 ref_idx = 0;
    for (RK_U32 dpb_idx = 0; dpb_idx < MPP_ARRAY_ELEMS(p->dpb) && ref_idx < 15; dpb_idx++) {
        const H265dFrame *frm = &p->dpb[dpb_idx];
        if (frm == current_picture || frm->slot_index == 0xff)
            continue;
        if (!frm->ref_status.st_ref && !frm->ref_status.lt_ref)
            continue;

        // Check if this frame is in rps_used
        RK_BOOL is_used = RK_FALSE;
        for (RK_U32 k = 0; k < nb_rps_used; k++) {
            if (rps_used[k] == (RK_U32)frm->poc) {
                is_used = RK_TRUE;
                break;
            }
        }
        if (!is_used)
            continue;

        RK_U32 long_term = frm->ref_status.lt_ref ? 1 : 0;
        fill_picture_entry(&pp->RefPicList[ref_idx], frm->slot_index, long_term);
        pp->PicOrderCntValList[ref_idx] = frm->poc;
        mpp_buf_slot_set_flag(p->slots, frm->slot_index, SLOT_HAL_INPUT);
        p->task->refer[ref_idx] = frm->slot_index;
        ref_idx++;
    }

    // Fill remaining slots with 0xff
    while (ref_idx < MPP_ARRAY_ELEMS(pp->RefPicList)) {
        pp->RefPicList[ref_idx].bPicEntry = 0xff;
        pp->PicOrderCntValList[ref_idx] = 0;
        p->task->refer[ref_idx] = -1;
        ref_idx++;
    }

    fill_ref_pic_set(pp, p, ST_CURR_BEF, pp->RefPicSetStCurrBefore);
    fill_ref_pic_set(pp, p, ST_CURR_AFT, pp->RefPicSetStCurrAfter);
    fill_ref_pic_set(pp, p, LT_CURR, pp->RefPicSetLtCurr);

    pp->sps_extension_flag                          = sps->sps_extension_flag;;
    pp->sps_range_extension_flag                    = sps->sps_range_extension_flag;;
    pp->transform_skip_rotation_enabled_flag        = sps->transform_skip_rotation_enabled_flag;;
    pp->transform_skip_context_enabled_flag         = sps->transform_skip_context_enabled_flag;;
    pp->implicit_rdpcm_enabled_flag                 = sps->implicit_rdpcm_enabled_flag;;
    pp->explicit_rdpcm_enabled_flag                 = sps->explicit_rdpcm_enabled_flag;;
    pp->extended_precision_processing_flag          = sps->extended_precision_processing_flag;;
    pp->intra_smoothing_disabled_flag               = sps->intra_smoothing_disabled_flag;;
    pp->high_precision_offsets_enabled_flag         = sps->high_precision_offsets_enabled_flag;;
    pp->persistent_rice_adaptation_enabled_flag     = sps->persistent_rice_adaptation_enabled_flag;;
    pp->cabac_bypass_alignment_enabled_flag         = sps->cabac_bypass_alignment_enabled_flag;;

    pp->cross_component_prediction_enabled_flag     = pps->cross_component_prediction_enabled_flag;
    pp->chroma_qp_offset_list_enabled_flag          = pps->chroma_qp_offset_list_enabled_flag;

    pp->log2_max_transform_skip_block_size          = pps->log2_max_transform_skip_block_size;
    pp->diff_cu_chroma_qp_offset_depth              = pps->diff_cu_chroma_qp_offset_depth;
    pp->chroma_qp_offset_list_len_minus1            = pps->chroma_qp_offset_list_len_minus1;
    for (i = 0; i < 6; i++) {
        pp->cb_qp_offset_list[i] = pps->cb_qp_offset_list[i];
        pp->cr_qp_offset_list[i] = pps->cr_qp_offset_list[i];
    }
}
extern RK_U8 h265d_diag_scan4x4_map[16];
extern RK_U8 h265d_diag_scan8x8_map[64];

static void fill_scaling_lists(const H265dPrs *p, DXVA_Qmatrix_HEVC *qm)
{
    const H265dPps *pps = (H265dPps *)p->pps_list[p->slice.pps_id];
    const H265dSps *sps = (H265dSps *)p->sps_list[pps->sps_id];
    const H265dScalingList *sl = (pps->scaling_list_data_present_flag != 0) ?
                                 &pps->scaling_list : &sps->scaling_list;
    RK_U32 i, j, pos;

    if (!sps->scaling_list_enable_flag)
        return;

    memset(qm, 0, sizeof(DXVA_Qmatrix_HEVC));
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 16; j++) {
            pos = h265d_diag_scan4x4_map[j];
            qm->ucScalingLists0[i][j] = sl->scaling_lists[SCALE_4X4_COPY][i][pos];
        }

        for (j = 0; j < 64; j++) {
            pos = h265d_diag_scan8x8_map[j];
            qm->ucScalingLists1[i][j] = sl->scaling_lists[SCALE_4X4][i][pos];
            qm->ucScalingLists2[i][j] = sl->scaling_lists[SCALE_8X8][i][pos];

            if (i < 2)
                qm->ucScalingLists3[i][j] = sl->scaling_lists[SCALE_16X16][i * 3][pos];
        }

        qm->ucScalingListDCCoefSizeID2[i] = sl->scaling_list_dc[0][i];

        if (i < 2)
            qm->ucScalingListDCCoefSizeID3[i] = sl->scaling_list_dc[1][i * 3];
    }
}

static void fill_slice_short(DXVA_Slice_HEVC_Short *slice,
                             RK_U32 position, RK_U32 size)
{
    memset(slice, 0, sizeof(*slice));
    slice->BSNALunitDataLocation = position;
    slice->SliceBytesInBuffer    = size;
    slice->wBadSliceChopping     = 0;
}

static void init_slice_cut_param(DXVA_Slice_HEVC_Cut_Param *slice)
{
    memset(slice, 0, sizeof(*slice));
}

RK_S32 h265d_parser2_syntax(H265dCtx *ctx)
{
    const H265dPrs *p = (const H265dPrs *)ctx->parser;
    h265d_dxva2_picture_context_t *ctx_pic = (h265d_dxva2_picture_context_t *)p->hal_pic_private;

    fill_picture_parameters(p, &ctx_pic->pp);

    fill_scaling_lists(p, &ctx_pic->qm);

    return 0;
}

RK_S32 h265d_syntax_fill_slice(H265dCtx *ctx, RK_S32 input_index)
{
    const H265dPrs *p = (const H265dPrs *)ctx->parser;
    h265d_dxva2_picture_context_t *ctx_pic = (h265d_dxva2_picture_context_t *)p->hal_pic_private;
    MppBuffer streambuf = NULL;
    RK_U8 *ptr = NULL;
    RK_U8 *current = NULL;
    RK_U32 position = 0;
    RK_U32 size = 0;
    RK_U32 length = 0;
    RK_S32 count = 0;
    RK_S32 i;

    if (-1 != input_index) {
        mpp_buf_slot_get_prop(p->packet_slots, input_index, SLOT_BUFFER, &streambuf);
        current = ptr = (RK_U8 *)mpp_buffer_get_ptr(streambuf);
        if (current == NULL)
            return MPP_ERR_NULL_PTR;
    } else {
        RK_S32 buff_size = 0;

        current = (RK_U8 *)mpp_packet_get_data(p->input_packet);
        size = (RK_U32)mpp_packet_get_size(p->input_packet);
        for (i = 0; i < p->nb_nals; i++) {
            length += p->nals[i].size + 4;
        }
        length = MPP_ALIGN(length, 16) + 64;
        if (length > size) {
            mpp_free(current);
            buff_size = MPP_ALIGN(length + 10 * 1024, 1024);
            current = mpp_malloc(RK_U8, buff_size);
            mpp_packet_set_data(p->input_packet, (void*)current);
            mpp_packet_set_size(p->input_packet, buff_size);
        }
    }

    if (ctx_pic->max_slice_num < p->nb_nals) {
        MPP_FREE(ctx_pic->slice_short);

        ctx_pic->slice_short = (DXVA_Slice_HEVC_Short *)mpp_malloc(DXVA_Slice_HEVC_Short, p->nb_nals);
        if (!ctx_pic->slice_short)
            return MPP_ERR_NOMEM;

        MPP_FREE(ctx_pic->slice_cut_param);

        ctx_pic->slice_cut_param = (DXVA_Slice_HEVC_Cut_Param *)mpp_malloc(DXVA_Slice_HEVC_Cut_Param, p->nb_nals);
        if (!ctx_pic->slice_cut_param)
            return MPP_ERR_NOMEM;

        ctx_pic->max_slice_num = p->nb_nals;
    }

    for (i = 0; i < p->nb_nals; i++) {
        static const RK_U8 start_code[] = {0, 0, 1 };
        static const RK_U32 start_code_size = sizeof(start_code);
        BitReadCtx_t gb_cxt, *gb;
        RK_S32 value;
        RK_U32 nal_type;
        RK_U8 *nal_data = p->bitstream + p->nals[i].bitstream_offset + 3;

        mpp_set_bitread_ctx(&gb_cxt, nal_data, p->nals[i].size);
        mpp_set_bitread_pseudo_code_type(&gb_cxt, PSEUDO_CODE_H264_H265);

        gb = &gb_cxt;

        READ_ONEBIT(gb, &value);

        READ_BITS(gb, 6, &nal_type);

        if (nal_type >= 32) {
            continue;
        }

        memcpy(current, start_code, start_code_size);
        current += start_code_size;
        position += start_code_size;

        memcpy(current, nal_data, p->nals[i].size);
        fill_slice_short(&ctx_pic->slice_short[count], position, p->nals[i].size);
        init_slice_cut_param(&ctx_pic->slice_cut_param[count]);
        current += p->nals[i].size;
        position += p->nals[i].size;
        count++;
    }

    ctx_pic->slice_count = count;
    ctx_pic->bitstream_size = position;

    if (-1 != input_index) {
        ctx_pic->bitstream = (RK_U8*)ptr;

        mpp_buf_slot_set_flag(p->packet_slots, input_index, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(p->packet_slots, input_index, SLOT_HAL_INPUT);
    } else {
        ctx_pic->bitstream = NULL;
        mpp_packet_set_length(p->input_packet, position);
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}
