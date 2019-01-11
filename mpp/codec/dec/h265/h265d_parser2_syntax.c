/*
 *
 * Copyright 2015 Rockchip Electronics Co. LTD
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

/*
 * @file       h265d_parser2_syntax.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG "H265SYNATX"

#include "h265d_parser.h"
#include "h265d_syntax.h"


static void fill_picture_entry(DXVA_PicEntry_HEVC *pic,
                               unsigned index, unsigned flag)
{
    mpp_assert((index & 0x7f) == index && (flag & 0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}

static RK_S32 get_refpic_index(const DXVA_PicParams_HEVC *pp, int surface_index)
{
    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefPicList); i++) {
        if ((pp->RefPicList[i].bPicEntry & 0x7f) == surface_index) {
            //mpp_err("retun %d slot_index = %d",i,surface_index);
            return i;
        }
    }
    return 0xff;
}

static void fill_picture_parameters(const HEVCContext *h,
                                    DXVA_PicParams_HEVC *pp)
{
    const HEVCFrame *current_picture = h->ref;
    const HEVCPPS *pps = (HEVCPPS *)h->pps_list[h->sh.pps_id];
    const HEVCSPS *sps = (HEVCSPS *)h->sps_list[pps->sps_id];

    RK_U32 i, j;
    RK_U32 rps_used[16];
    RK_U32 nb_rps_used;

    memset(pp, 0, sizeof(*pp));

    pp->PicWidthInMinCbsY  = sps->min_cb_width;
    pp->PicHeightInMinCbsY = sps->min_cb_height;
    pp->pps_id = h->sh.pps_id;
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

    pp->sps_max_dec_pic_buffering_minus1         = sps->temporal_layer[sps->max_sub_layers - 1].max_dec_pic_buffering - 1;
    pp->log2_min_luma_coding_block_size_minus3   = sps->log2_min_cb_size - 3;
    pp->log2_diff_max_min_luma_coding_block_size = sps->log2_diff_max_min_coding_block_size;
    pp->log2_min_transform_block_size_minus2     = sps->log2_min_tb_size - 2;
    pp->log2_diff_max_min_transform_block_size   = sps->log2_max_trafo_size  - sps->log2_min_tb_size;
    pp->max_transform_hierarchy_depth_inter      = sps->max_transform_hierarchy_depth_inter;
    pp->max_transform_hierarchy_depth_intra      = sps->max_transform_hierarchy_depth_intra;
    pp->num_short_term_ref_pic_sets              = sps->nb_st_rps;
    pp->num_long_term_ref_pics_sps               = sps->num_long_term_ref_pics_sps;

    pp->num_ref_idx_l0_default_active_minus1     = pps->num_ref_idx_l0_default_active - 1;
    pp->num_ref_idx_l1_default_active_minus1     = pps->num_ref_idx_l1_default_active - 1;
    pp->init_qp_minus26                          = pps->pic_init_qp_minus26;

    if (h->sh.short_term_ref_pic_set_sps_flag == 0 && h->sh.short_term_rps) {
        pp->ucNumDeltaPocsOfRefRpsIdx            = h->sh.short_term_rps->rps_idx_num_delta_pocs;
        pp->wNumBitsForShortTermRPSInSlice       = h->sh.short_term_ref_pic_set_size;
    }

    pp->dwCodingParamToolFlags = (sps->scaling_list_enable_flag                  <<  0) |
                                 (sps->amp_enabled_flag                          <<  1) |
                                 (sps->sao_enabled                               <<  2) |
                                 (sps->pcm_enabled_flag                          <<  3) |
                                 ((sps->pcm_enabled_flag ? (sps->pcm.bit_depth - 1) : 0)            <<  4) |
                                 ((sps->pcm_enabled_flag ? (sps->pcm.bit_depth_chroma - 1) : 0)     <<  8) |
                                 ((sps->pcm_enabled_flag ? (sps->pcm.log2_min_pcm_cb_size - 3) : 0) << 12) |
                                 ((sps->pcm_enabled_flag ? (sps->pcm.log2_max_pcm_cb_size - sps->pcm.log2_min_pcm_cb_size) : 0) << 14) |
                                 (sps->pcm.loop_filter_disable_flag              << 16) |
                                 (sps->long_term_ref_pics_present_flag           << 17) |
                                 (sps->sps_temporal_mvp_enabled_flag             << 18) |
                                 (sps->sps_strong_intra_smoothing_enable_flag    << 19) |
                                 (pps->dependent_slice_segments_enabled_flag     << 20) |
                                 (pps->output_flag_present_flag                  << 21) |
                                 (pps->num_extra_slice_header_bits               << 22) |
                                 (pps->sign_data_hiding_flag                     << 25) |
                                 (pps->cabac_init_present_flag                   << 26) |
                                 (0                                              << 27);

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
                                              ((pps->tiles_enabled_flag ? pps->loop_filter_across_tiles_enabled_flag : 0) << 10) |
                                              (pps->seq_loop_filter_across_slices_enabled_flag    << 11) |
                                              (pps->deblocking_filter_override_enabled_flag       << 12) |
                                              (pps->disable_dbf                                   << 13) |
                                              (pps->lists_modification_present_flag               << 14) |
                                              (pps->slice_header_extension_present_flag           << 15) |
                                              (IS_IRAP(h)                                         << 16) |
                                              (IS_IDR(h)                                          << 17) |
                                              /* IntraPicFlag */
                                              (IS_IRAP(h)                                         << 18) |
                                              (0                                                  << 19);
    pp->pps_cb_qp_offset            = pps->cb_qp_offset;
    pp->pps_cr_qp_offset            = pps->cr_qp_offset;
    if (pps->tiles_enabled_flag) {
        pp->num_tile_columns_minus1 = pps->num_tile_columns - 1;
        pp->num_tile_rows_minus1    = pps->num_tile_rows - 1;

        if (!pps->uniform_spacing_flag) {
            for (i = 0; i < (RK_U32)pps->num_tile_columns; i++)
                pp->column_width_minus1[i] = pps->column_width[i] - 1;

            for (i = 0; i < (RK_U32)pps->num_tile_rows; i++)
                pp->row_height_minus1[i] = pps->row_height[i] - 1;
        }
    }

    pp->diff_cu_qp_delta_depth           = pps->diff_cu_qp_delta_depth;
    pp->pps_beta_offset_div2             = pps->beta_offset / 2;
    pp->pps_tc_offset_div2               = pps->tc_offset / 2;
    pp->log2_parallel_merge_level_minus2 = pps->log2_parallel_merge_level - 2;
    pp->CurrPicOrderCntVal               = h->poc;

    nb_rps_used = 0;
    for (i = 0; i < NB_RPS_TYPE; i++) {
        for (j = 0; j < (RK_U32)h->rps[i].nb_refs; j++) {
            if ((i == ST_FOLL) || (i == LT_FOLL)) {
                ;
            } else {
                rps_used[nb_rps_used++] = h->rps[i].list[j];
            }
        }
    }
    // mpp_err("fill RefPicList from the DPB");
    // fill RefPicList from the DPB
    for (i = 0, j = 0; i < MPP_ARRAY_ELEMS(pp->RefPicList); i++) {
        const HEVCFrame *frame = NULL;
        while (!frame && j < MPP_ARRAY_ELEMS(h->DPB)) {
            if (&h->DPB[j] != current_picture &&
                (h->DPB[j].flags & (HEVC_FRAME_FLAG_LONG_REF | HEVC_FRAME_FLAG_SHORT_REF))) {
                RK_U32 k = 0;
                for (k = 0; k < nb_rps_used; k++) {  /*skip fill RefPicList no used in rps*/
                    if (rps_used[k] == (RK_U32)h->DPB[j].poc) {
                        frame = &h->DPB[j];
                    }
                }
            }
            j++;
        }

        if (frame && (frame->slot_index != 0xff)) {
            fill_picture_entry(&pp->RefPicList[i], frame->slot_index, !!(frame->flags & HEVC_FRAME_FLAG_LONG_REF));
            pp->PicOrderCntValList[i] = frame->poc;
            mpp_buf_slot_set_flag(h->slots, frame->slot_index, SLOT_HAL_INPUT);
            h->task->refer[i] = frame->slot_index;
            //mpp_err("ref[%d] = %d",i,frame->slot_index);
        } else {
            pp->RefPicList[i].bPicEntry = 0xff;
            pp->PicOrderCntValList[i]   = 0;
            h->task->refer[i] = -1;
        }
    }

#define DO_REF_LIST(ref_idx, ref_list) { \
        const RefPicList *rpl = &h->rps[ref_idx]; \
        for (i = 0, j = 0; i < MPP_ARRAY_ELEMS(pp->ref_list); i++) { \
            const HEVCFrame *frame = NULL; \
            while (!frame && j < (RK_U32)rpl->nb_refs) \
                frame = rpl->ref[j++]; \
            if (frame) \
                pp->ref_list[i] = get_refpic_index(pp, frame->slot_index); \
            else \
                pp->ref_list[i] = 0xff; \
        } \
    }

    // Fill short term and long term lists
    DO_REF_LIST(ST_CURR_BEF, RefPicSetStCurrBefore);
    DO_REF_LIST(ST_CURR_AFT, RefPicSetStCurrAfter);
    DO_REF_LIST(LT_CURR, RefPicSetLtCurr);

}
extern RK_U8 mpp_hevc_diag_scan4x4_x[16];
extern RK_U8 mpp_hevc_diag_scan4x4_y[16];
extern RK_U8 mpp_hevc_diag_scan8x8_x[64];
extern RK_U8 mpp_hevc_diag_scan8x8_y[64];

static void fill_scaling_lists(const HEVCContext *h, DXVA_Qmatrix_HEVC *qm)
{
    RK_U32 i, j, pos;
    const HEVCPPS *pps = (HEVCPPS *)h->pps_list[h->sh.pps_id];
    const HEVCSPS *sps = (HEVCSPS *)h->sps_list[pps->sps_id];
    const ScalingList *sl = pps->scaling_list_data_present_flag ?
                            &pps->scaling_list : &sps->scaling_list;

    memset(qm, 0, sizeof(DXVA_Qmatrix_HEVC));
    for (i = 0; i < 6; i++) {
        for (j = 0; j < 16; j++) {
            pos = 4 * mpp_hevc_diag_scan4x4_y[j] + mpp_hevc_diag_scan4x4_x[j];
            qm->ucScalingLists0[i][j] = sl->sl[0][i][pos];
        }

        for (j = 0; j < 64; j++) {
            pos = 8 * mpp_hevc_diag_scan8x8_y[j] + mpp_hevc_diag_scan8x8_x[j];
            qm->ucScalingLists1[i][j] = sl->sl[1][i][pos];
            qm->ucScalingLists2[i][j] = sl->sl[2][i][pos];

            if (i < 2)
                qm->ucScalingLists3[i][j] = sl->sl[3][i * 3][pos];
        }

        qm->ucScalingListDCCoefSizeID2[i] = sl->sl_dc[0][i];

        if (i < 2)
            qm->ucScalingListDCCoefSizeID3[i] = sl->sl_dc[1][i * 3];
    }
}

static void fill_slice_short(DXVA_Slice_HEVC_Short *slice,
                             unsigned position, unsigned size)
{
    memset(slice, 0, sizeof(*slice));
    slice->BSNALunitDataLocation = position;
    slice->SliceBytesInBuffer    = size;
    slice->wBadSliceChopping     = 0;
}

RK_S32 h265d_parser2_syntax(void *ctx)
{

    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    const HEVCContext *h = (const HEVCContext *)h265dctx->priv_data;

    h265d_dxva2_picture_context_t *ctx_pic = (h265d_dxva2_picture_context_t *)h->hal_pic_private;

    /* Fill up DXVA_PicParams_HEVC */
    fill_picture_parameters(h, &ctx_pic->pp);

    /* Fill up DXVA_Qmatrix_HEVC */
    fill_scaling_lists(h, &ctx_pic->qm);

    return 0;
}

RK_S32 h265d_syntax_fill_slice(void *ctx, RK_S32 input_index)
{
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    const HEVCContext *h = (const HEVCContext *)h265dctx->priv_data;
    h265d_dxva2_picture_context_t *ctx_pic = (h265d_dxva2_picture_context_t *)h->hal_pic_private;
    MppBuffer streambuf = NULL;
    RK_S32 i, count = 0;
    RK_U32 position = 0;
    RK_U8 *ptr = NULL;
    RK_U8 *current = NULL;
    RK_U32 size = 0, length = 0;
    // mpp_err("input_index = %d",input_index);
    if (-1 != input_index) {
        mpp_buf_slot_get_prop(h->packet_slots, input_index, SLOT_BUFFER, &streambuf);
        current = ptr = (RK_U8 *)mpp_buffer_get_ptr(streambuf);
        if (current == NULL) {
            return MPP_ERR_NULL_PTR;
        }
    } else {
        RK_S32 buff_size = 0;
        current = (RK_U8 *)mpp_packet_get_data(h->input_packet);
        size = (RK_U32)mpp_packet_get_size(h->input_packet);
        for (i = 0; i < h->nb_nals; i++) {
            length += h->nals[i].size;
        }
        length = MPP_ALIGN(length, 16) + 64;
        if (length > size) {
            mpp_free(current);
            buff_size = MPP_ALIGN(length + 10 * 1024, 1024);
            current = mpp_malloc(RK_U8, buff_size);
            mpp_packet_set_data(h->input_packet, (void*)current);
            mpp_packet_set_size(h->input_packet, buff_size);
        }
    }
    for (i = 0; i < h->nb_nals; i++) {
        static const RK_U8 start_code[] = {0, 0, 1 };
        static const RK_U32 start_code_size = sizeof(start_code);
        BitReadCtx_t gb_cxt, *gb;
        RK_S32 value;
        RK_U32 nal_type;

        mpp_set_bitread_ctx(&gb_cxt, (RK_U8 *)h->nals[i].data,
                            h->nals[i].size);
        mpp_set_pre_detection(&gb_cxt);

        gb = &gb_cxt;

        READ_ONEBIT(gb, &value); /*this bit should be zero*/

        READ_BITS(gb, 6, &nal_type);

        if (nal_type >= 32) {
            continue;
        }
        memcpy(current, start_code, start_code_size);
        current += start_code_size;
        position += start_code_size;
        memcpy(current, h->nals[i].data, h->nals[i].size);
        // mpp_log("h->nals[%d].size = %d", i, h->nals[i].size);
        fill_slice_short(&ctx_pic->slice_short[count], position, h->nals[i].size);
        current += h->nals[i].size;
        position += h->nals[i].size;
        count++;
    }
    ctx_pic->slice_count    = count;
    ctx_pic->bitstream_size = position;
    if (-1 != input_index) {
        ctx_pic->bitstream      = (RK_U8*)ptr;

        mpp_buf_slot_set_flag(h->packet_slots, input_index, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(h->packet_slots, input_index, SLOT_HAL_INPUT);
    } else {
        ctx_pic->bitstream = NULL;
        mpp_packet_set_length(h->input_packet, position);
    }
    return MPP_OK;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}
