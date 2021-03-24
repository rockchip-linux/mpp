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

#define MODULE_TAG "av1d_parser2syntax"

#include <string.h>

#include "av1d_parser.h"
#include "av1d_syntax.h"

static int av1d_fill_picparams(Av1CodecContext *ctx, DXVA_PicParams_AV1 *pp)
{
    int i, j, uses_lr;
    AV1Context *h = ctx->priv_data;
    const AV1RawSequenceHeader *seq = h->sequence_header;
    const AV1RawFrameHeader *frame_header = h->raw_frame_header;
    const AV1RawFilmGrainParams *film_grain = &h->cur_frame.film_grain;

    unsigned char remap_lr_type[4] = { AV1_RESTORE_NONE, AV1_RESTORE_SWITCHABLE, AV1_RESTORE_WIENER, AV1_RESTORE_SGRPROJ };
    // int apply_grain = !(ctx->export_side_data & AV_CODEC_EXPORT_DATA_FILM_GRAIN) && film_grain->apply_grain;
    int apply_grain = film_grain->apply_grain;

    memset(pp, 0, sizeof(*pp));

    pp->width  = ctx->width;
    pp->height = ctx->height;

    pp->max_width  = seq->max_frame_width_minus_1 + 1;
    pp->max_height = seq->max_frame_height_minus_1 + 1;

    pp->CurrPicTextureIndex = h->cur_frame.slot_index;
    pp->superres_denom      = frame_header->use_superres ? frame_header->coded_denom : AV1_SUPERRES_NUM;
    pp->bitdepth            = h->bit_depth;
    pp->seq_profile         = seq->seq_profile;

    /* Tiling info */
    pp->tiles.cols = frame_header->tile_cols;
    pp->tiles.rows = frame_header->tile_rows;
    pp->tiles.context_update_id = frame_header->context_update_tile_id;

    {
        RK_U8 val = 0;
        for (i = 0; i < pp->tiles.cols; i++) {
            pp->tiles.widths[i] = val;
            val += frame_header->width_in_sbs_minus_1[i] + 1;
        }
        pp->tiles.widths[i] = val;

        val = 0;
        for (i = 0; i < pp->tiles.rows; i++) {
            pp->tiles.heights[i] = val;
            val += frame_header->height_in_sbs_minus_1[i] + 1;
        }
        pp->tiles.heights[i] = val;
    }

    for (i = 0; i < AV1_MAX_TILES; i++) {
        pp->tiles.tile_offset_start[i] = h->tile_offset_start[i];
        pp->tiles.tile_offset_end[i] = h->tile_offset_end[i];
    }
    pp->tiles.tile_sz_mag = h->raw_frame_header->tile_size_bytes_minus1;
    /* Coding tools */
    pp->coding.use_128x128_superblock       = seq->use_128x128_superblock;
    pp->coding.intra_edge_filter            = seq->enable_intra_edge_filter;
    pp->coding.interintra_compound          = seq->enable_interintra_compound;
    pp->coding.masked_compound              = seq->enable_masked_compound;
    pp->coding.warped_motion                = frame_header->allow_warped_motion;
    pp->coding.dual_filter                  = seq->enable_dual_filter;
    pp->coding.jnt_comp                     = seq->enable_jnt_comp;
    pp->coding.screen_content_tools         = frame_header->allow_screen_content_tools;
    pp->coding.integer_mv                   = frame_header->force_integer_mv;

    pp->coding.cdef_en                      = seq->enable_cdef;
    pp->coding.restoration                  = seq->enable_restoration;
    pp->coding.film_grain_en                = seq->film_grain_params_present  ;//&& !(avctx->export_side_data & AV_CODEC_EXPORT_DATA_FILM_GRAIN);
    pp->coding.intrabc                      = frame_header->allow_intrabc;
    pp->coding.high_precision_mv            = frame_header->allow_high_precision_mv;
    pp->coding.switchable_motion_mode       = frame_header->is_motion_mode_switchable;
    pp->coding.filter_intra                 = seq->enable_filter_intra;
    pp->coding.disable_frame_end_update_cdf = frame_header->disable_frame_end_update_cdf;
    pp->coding.disable_cdf_update           = frame_header->disable_cdf_update;
    pp->coding.reference_mode               = frame_header->reference_select;
    pp->coding.skip_mode                    = frame_header->skip_mode_present;
    pp->coding.reduced_tx_set               = frame_header->reduced_tx_set;
    pp->coding.superres                     = frame_header->use_superres;
    pp->coding.tx_mode                      = frame_header->tx_mode;
    pp->coding.use_ref_frame_mvs            = frame_header->use_ref_frame_mvs;
    pp->coding.enable_ref_frame_mvs         = seq->enable_ref_frame_mvs;
    pp->coding.reference_frame_update       = 1; // 0 for show_existing_frame with key frames, but those are not passed to the hwaccel
    pp->coding.error_resilient_mode         = frame_header->error_resilient_mode;

    /* Format & Picture Info flags */
    pp->format.frame_type     = frame_header->frame_type;
    pp->format.show_frame     = frame_header->show_frame;
    pp->format.showable_frame = frame_header->showable_frame;
    pp->format.subsampling_x  = seq->color_config.subsampling_x;
    pp->format.subsampling_y  = seq->color_config.subsampling_y;
    pp->format.mono_chrome    = seq->color_config.mono_chrome;
    pp->coded_lossless = h->cur_frame.coded_lossless;
    /* References */
    pp->primary_ref_frame = frame_header->primary_ref_frame;
    pp->order_hint        = frame_header->order_hint;
    pp->order_hint_bits   = seq->enable_order_hint ? seq->order_hint_bits_minus_1 + 1 : 0;

    memset(pp->RefFrameMapTextureIndex, 0xFF, sizeof(pp->RefFrameMapTextureIndex));
    for (i = 0; i < AV1_REFS_PER_FRAME; i++) {
        int8_t ref_idx = frame_header->ref_frame_idx[i];
        AV1Frame *ref_frame = &h->ref[ref_idx];
        RefInfo *ref_i = ref_frame->ref;
        pp->frame_refs[i].width  = mpp_frame_get_width(ref_frame->f);
        pp->frame_refs[i].height = mpp_frame_get_height(ref_frame->f);;
        pp->frame_refs[i].Index  = ref_frame->slot_index;
        pp->frame_refs[i].order_hint = ref_frame->order_hint;
        if (ref_i) {
            pp->frame_refs[i].lst_frame_offset = ref_i->lst_frame_offset;
            pp->frame_refs[i].lst2_frame_offset = ref_i->lst2_frame_offset;
            pp->frame_refs[i].lst3_frame_offset = ref_i->lst3_frame_offset;
            pp->frame_refs[i].gld_frame_offset = ref_i->gld_frame_offset;
            pp->frame_refs[i].bwd_frame_offset = ref_i->bwd_frame_offset;
            pp->frame_refs[i].alt2_frame_offset = ref_i->alt2_frame_offset;
            pp->frame_refs[i].alt_frame_offset = ref_i->alt_frame_offset ;
            pp->frame_refs[i].is_intra_frame = ref_i->is_intra_frame;
            pp->frame_refs[i].intra_only = ref_i->intra_only;
        }
        /* Global Motion */
        pp->frame_refs[i].wminvalid = (h->cur_frame.gm_type[AV1_REF_FRAME_LAST + i] == AV1_WARP_MODEL_IDENTITY);
        pp->frame_refs[i].wmtype    = h->cur_frame.gm_type[AV1_REF_FRAME_LAST + i];
        for (j = 0; j < 6; ++j) {
            pp->frame_refs[i].wmmat[j] = h->cur_frame.gm_params[AV1_REF_FRAME_LAST + i][j];
        }
    }
    for (i = 0; i < AV1_NUM_REF_FRAMES; i++) {
        AV1Frame *ref_frame = &h->ref[i];
        if (ref_frame->slot_index < 0x7f)
            pp->RefFrameMapTextureIndex[i] = ref_frame->slot_index;
        else
            pp->RefFrameMapTextureIndex[i] = 0xff;
    }

    /* Loop filter parameters */
    pp->loop_filter.filter_level[0]        = frame_header->loop_filter_level[0];
    pp->loop_filter.filter_level[1]        = frame_header->loop_filter_level[1];
    pp->loop_filter.filter_level_u         = frame_header->loop_filter_level[2];
    pp->loop_filter.filter_level_v         = frame_header->loop_filter_level[3];
    pp->loop_filter.sharpness_level        = frame_header->loop_filter_sharpness;
    pp->loop_filter.mode_ref_delta_enabled = frame_header->loop_filter_delta_enabled;
    pp->loop_filter.mode_ref_delta_update  = frame_header->loop_filter_delta_update;
    pp->loop_filter.delta_lf_multi         = frame_header->delta_lf_multi;
    pp->loop_filter.delta_lf_present       = frame_header->delta_lf_present;
    pp->loop_filter.delta_lf_res           = frame_header->delta_lf_res;

    for (i = 0; i < AV1_TOTAL_REFS_PER_FRAME; i++) {
        pp->loop_filter.ref_deltas[i] = frame_header->loop_filter_ref_deltas[i];
    }

    pp->loop_filter.mode_deltas[0]                = frame_header->loop_filter_mode_deltas[0];
    pp->loop_filter.mode_deltas[1]                = frame_header->loop_filter_mode_deltas[1];
    pp->loop_filter.frame_restoration_type[0]     = remap_lr_type[frame_header->lr_type[0]];
    pp->loop_filter.frame_restoration_type[1]     = remap_lr_type[frame_header->lr_type[1]];
    pp->loop_filter.frame_restoration_type[2]     = remap_lr_type[frame_header->lr_type[2]];
    uses_lr = frame_header->lr_type[0] || frame_header->lr_type[1] || frame_header->lr_type[2];
    pp->loop_filter.log2_restoration_unit_size[0] = uses_lr ? (1 + frame_header->lr_unit_shift) : 3;
    pp->loop_filter.log2_restoration_unit_size[1] = uses_lr ? (1 + frame_header->lr_unit_shift - frame_header->lr_uv_shift) : 3;
    pp->loop_filter.log2_restoration_unit_size[2] = uses_lr ? (1 + frame_header->lr_unit_shift - frame_header->lr_uv_shift) : 3;

    /* Quantization */
    pp->quantization.delta_q_present = frame_header->delta_q_present;
    pp->quantization.delta_q_res     = frame_header->delta_q_res;
    pp->quantization.base_qindex     = frame_header->base_q_idx;
    pp->quantization.y_dc_delta_q    = frame_header->delta_q_y_dc;
    pp->quantization.u_dc_delta_q    = frame_header->delta_q_u_dc;
    pp->quantization.v_dc_delta_q    = frame_header->delta_q_v_dc;
    pp->quantization.u_ac_delta_q    = frame_header->delta_q_u_ac;
    pp->quantization.v_ac_delta_q    = frame_header->delta_q_v_ac;
    pp->quantization.qm_y            = frame_header->using_qmatrix ? frame_header->qm_y : 0xFF;
    pp->quantization.qm_u            = frame_header->using_qmatrix ? frame_header->qm_u : 0xFF;
    pp->quantization.qm_v            = frame_header->using_qmatrix ? frame_header->qm_v : 0xFF;

    /* Cdef parameters */
    pp->cdef.damping = frame_header->cdef_damping_minus_3;
    pp->cdef.bits    = frame_header->cdef_bits;
    for (i = 0; i < 8; i++) {
        pp->cdef.y_strengths[i].primary    = frame_header->cdef_y_pri_strength[i];
        pp->cdef.y_strengths[i].secondary  = frame_header->cdef_y_sec_strength[i];
        pp->cdef.uv_strengths[i].primary   = frame_header->cdef_uv_pri_strength[i];
        pp->cdef.uv_strengths[i].secondary = frame_header->cdef_uv_sec_strength[i];
    }

    /* Misc flags */
    pp->interp_filter = frame_header->interpolation_filter;

    /* Segmentation */
    pp->segmentation.enabled         = frame_header->segmentation_enabled;
    pp->segmentation.update_map      = frame_header->segmentation_update_map;
    pp->segmentation.update_data     = frame_header->segmentation_update_data;
    pp->segmentation.temporal_update = frame_header->segmentation_temporal_update;
    for (i = 0; i < AV1_MAX_SEGMENTS; i++) {
        for (j = 0; j < AV1_SEG_LVL_MAX; j++) {
            pp->segmentation.feature_mask[i]      |= frame_header->feature_enabled[i][j] << j;
            pp->segmentation.feature_data[i][j]    = frame_header->feature_value[i][j];
        }
    }

    /* Film grain */
    if (apply_grain) {
        pp->film_grain.apply_grain              = 1;
        pp->film_grain.scaling_shift_minus8     = film_grain->grain_scaling_minus_8;
        pp->film_grain.chroma_scaling_from_luma = film_grain->chroma_scaling_from_luma;
        pp->film_grain.ar_coeff_lag             = film_grain->ar_coeff_lag;
        pp->film_grain.ar_coeff_shift_minus6    = film_grain->ar_coeff_shift_minus_6;
        pp->film_grain.grain_scale_shift        = film_grain->grain_scale_shift;
        pp->film_grain.overlap_flag             = film_grain->overlap_flag;
        pp->film_grain.clip_to_restricted_range = film_grain->clip_to_restricted_range;
        pp->film_grain.matrix_coeff_is_identity = (seq->color_config.matrix_coefficients == MPP_FRAME_SPC_RGB);

        pp->film_grain.grain_seed               = film_grain->grain_seed;
        pp->film_grain.num_y_points             = film_grain->num_y_points;
        for (i = 0; i < film_grain->num_y_points; i++) {
            pp->film_grain.scaling_points_y[i][0] = film_grain->point_y_value[i];
            pp->film_grain.scaling_points_y[i][1] = film_grain->point_y_scaling[i];
        }
        pp->film_grain.num_cb_points            = film_grain->num_cb_points;
        for (i = 0; i < film_grain->num_cb_points; i++) {
            pp->film_grain.scaling_points_cb[i][0] = film_grain->point_cb_value[i];
            pp->film_grain.scaling_points_cb[i][1] = film_grain->point_cb_scaling[i];
        }
        pp->film_grain.num_cr_points            = film_grain->num_cr_points;
        for (i = 0; i < film_grain->num_cr_points; i++) {
            pp->film_grain.scaling_points_cr[i][0] = film_grain->point_cr_value[i];
            pp->film_grain.scaling_points_cr[i][1] = film_grain->point_cr_scaling[i];
        }
        for (i = 0; i < 24; i++) {
            pp->film_grain.ar_coeffs_y[i] = film_grain->ar_coeffs_y_plus_128[i];
        }
        for (i = 0; i < 25; i++) {
            pp->film_grain.ar_coeffs_cb[i] = film_grain->ar_coeffs_cb_plus_128[i];
            pp->film_grain.ar_coeffs_cr[i] = film_grain->ar_coeffs_cr_plus_128[i];
        }
        pp->film_grain.cb_mult      = film_grain->cb_mult;
        pp->film_grain.cb_luma_mult = film_grain->cb_luma_mult;
        pp->film_grain.cr_mult      = film_grain->cr_mult;
        pp->film_grain.cr_luma_mult = film_grain->cr_luma_mult;
        pp->film_grain.cb_offset    = film_grain->cb_offset;
        pp->film_grain.cr_offset    = film_grain->cr_offset;
        pp->film_grain.cr_offset    = film_grain->cr_offset;
    }
    pp->upscaled_width = h->upscaled_width;
    pp->frame_to_show_map_idx = frame_header->frame_to_show_map_idx;
    pp->frame_tag_size = h->frame_tag_size;
    pp->skip_ref0      = h->skip_ref0;
    pp->skip_ref1      = h->skip_ref1;
    pp->refresh_frame_flags = frame_header->refresh_frame_flags;

    pp->cdfs = h->cdfs;
    pp->cdfs_ndvc = h->cdfs_ndvc;
    pp->tile_cols_log2 = frame_header->tile_cols_log2;
    // XXX: Setting the StatusReportFeedbackNumber breaks decoding on some drivers (tested on NVIDIA 457.09)
    // Status Reporting is not used by FFmpeg, hence not providing a number does not cause any issues
    //pp->StatusReportFeedbackNumber = 1 + DXVA_CONTEXT_REPORT_ID(avctx, ctx)++;
    return 0;
}

void av1d_fill_counts(Av1CodecContext *ctx)
{
    //AV1Context *s = ctx->priv_data;
    (void) ctx;
    // memcpy(&ctx->pic_params.counts, &s->counts, sizeof(s->counts));
}

RK_S32 av1d_parser2_syntax(Av1CodecContext *ctx)
{
    av1d_fill_picparams(ctx, &ctx->pic_params);
    return 0;
}
