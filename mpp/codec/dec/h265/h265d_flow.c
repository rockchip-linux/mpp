/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "h265d"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_bit.h"
#include "mpp_packet_impl.h"
#include "rk_hdr_meta_com.h"
#include "mpp_ref_pool.h"

#include "mpp_parser.h"
#include "h265d_debug.h"
#include "h265d_ctx.h"
#include "h265d_syntax.h"
#include "h2645d_sei.h"
#include "h265d_sei.h"
#include "h265d_dpb.h"

#define START_CODE 0x000001

static RK_S32 pred_weight_table(BitReadCtx_t *gb, RK_S32 chroma_format_idc,
                                const RK_U32 nb_refs[2], SliceType slice_type)
{
    RK_U32 luma_log2_weight_denom;
    RK_U8  luma_weight_l0_flag[16];
    RK_U8  chroma_weight_l0_flag[16];
    RK_U8  luma_weight_l1_flag[16];
    RK_U8  chroma_weight_l1_flag[16];
    RK_U32 i = 0;
    RK_U32 j = 0;

    READ_UE(gb, &luma_log2_weight_denom);
    if (chroma_format_idc != 0) {
        RK_S32 delta = 0;
        READ_SE(gb, &delta);
        (void)delta;

    }

    for (i = 0; i < nb_refs[L0]; i++) {
        READ_ONEBIT(gb, &luma_weight_l0_flag[i]);
    }

    if (chroma_format_idc != 0) {
        for (i = 0; i < nb_refs[L0]; i++) {
            READ_ONEBIT(gb, &chroma_weight_l0_flag[i]);
        }
    } else {
        for (i = 0; i < nb_refs[L0]; i++)
            chroma_weight_l0_flag[i] = 0;
    }

    for (i = 0; i < nb_refs[L0]; i++) {
        if (luma_weight_l0_flag[i]) {
            RK_S32 delta_luma_weight_l0 = 0;
            RK_S32 luma_offset_l0 = 0;
            READ_SE(gb, &delta_luma_weight_l0);
            (void)delta_luma_weight_l0;
            READ_SE(gb, &luma_offset_l0);
            (void)luma_offset_l0;

        }
        if (chroma_weight_l0_flag[i]) {
            for (j = 0; j < 2; j++) {
                RK_S32 delta_chroma_weight_l0 = 0;
                RK_S32 delta_chroma_offset_l0 = 0;
                READ_SE(gb, &delta_chroma_weight_l0);
                READ_SE(gb, &delta_chroma_offset_l0);
                (void)delta_chroma_weight_l0;
                (void)delta_chroma_offset_l0;
            }

        }
    }

    if (slice_type == B_SLICE) {
        for (i = 0; i < nb_refs[L1]; i++) {
            READ_ONEBIT(gb, &luma_weight_l1_flag[i]);
        }
        if (chroma_format_idc != 0) {
            for (i = 0; i < nb_refs[L1]; i++)
                READ_ONEBIT(gb, &chroma_weight_l1_flag[i]);
        } else {
            for (i = 0; i < nb_refs[L1]; i++)
                chroma_weight_l1_flag[i] = 0;
        }
        for (i = 0; i < nb_refs[L1]; i++) {
            if (luma_weight_l1_flag[i]) {
                RK_S32 delta_luma_weight_l1 = 0;
                RK_S32 luma_offset_l1 = 0;
                READ_UE(gb, &delta_luma_weight_l1);
                (void)delta_luma_weight_l1;
                READ_SE(gb, &luma_offset_l1);
                (void)luma_offset_l1;

            }
            if (chroma_weight_l1_flag[i]) {
                for (j = 0; j < 2; j++) {
                    RK_S32 delta_chroma_weight_l1 = 0;
                    RK_S32 delta_chroma_offset_l1 = 0;
                    READ_SE(gb, &delta_chroma_weight_l1);
                    READ_SE(gb, &delta_chroma_offset_l1);
                    (void)delta_chroma_weight_l1;
                    (void)delta_chroma_offset_l1;
                }

            }
        }
    }
    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 h265d_lt_rps(H265dPrs *p, H265dLtRps *rps, BitReadCtx_t *gb)
{
    const H265dSps *sps = p->sps;
    RK_S32 poc_lsb_max = 1 << sps->log2_max_poc_lsb;
    RK_S32 msb_delta_prev = 0;
    RK_U32 num_sps = 0, num_slc;
    RK_U32 has_msb;
    RK_U32 used_tmp = 0;
    RK_U32 i;

    rps->nb_refs = 0;
    rps->used = 0;
    if (!sps->lt_rps_sps.valid)
        return 0;

    if (sps->lt_rps_sps.num_refs > 0)
        READ_UE(gb, &num_sps);

    READ_UE(gb, &num_slc);

    if (num_slc + num_sps > MPP_ARRAY_ELEMS(rps->poc))
        return MPP_ERR_STREAM;

    rps->nb_refs = num_slc + num_sps;

    for (i = 0; i < (RK_U32)rps->nb_refs; i++) {
        if ((RK_U32)i < num_sps) {
            RK_U32 sps_idx = 0;

            if (sps->lt_rps_sps.num_refs > 1)
                READ_BITS(gb, mpp_ceil_log2(sps->lt_rps_sps.num_refs), &sps_idx);

            rps->poc[i] = sps->lt_rps_sps.poc_lsb[sps_idx];
            MPP_MOD_BIT(rps->used, i, MPP_GET_BIT(sps->lt_rps_sps.used_flag, sps_idx));
        } else {
            READ_BITS(gb, sps->log2_max_poc_lsb, &rps->poc[i]);
            READ_ONEBIT(gb, &used_tmp);
            MPP_MOD_BIT(rps->used, i, used_tmp);
        }

        READ_ONEBIT(gb, &has_msb);
        if (has_msb) {
            RK_S32 msb_delta = 0;

            READ_UE(gb, &msb_delta);

            if (i && i != num_sps)
                msb_delta += msb_delta_prev;

            rps->poc[i] += p->poc - (msb_delta * poc_lsb_max) - p->slice.pic_order_cnt_lsb;
            msb_delta_prev = msb_delta;
        }
    }

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 set_sps(H265dPrs *p, const H265dSps *sps)
{
    MppFrameFormat fmt = p->ctx->cfg->base.out_fmt & (~MPP_FRAME_FMT_MASK);

    p->ctx->coded_width         = sps->width;
    p->ctx->coded_height        = sps->height;
    p->ctx->width               = sps->output_width;
    p->ctx->height              = sps->output_height;
    p->ctx->pix_fmt             = fmt | sps->pix_fmt;
    p->ctx->bit_depth           = sps->bit_depth;

    if (sps->vui.video_signal_type_present_flag)
        p->ctx->color_range = sps->vui.video_full_range_flag ?
                              MPP_FRAME_RANGE_JPEG : MPP_FRAME_RANGE_MPEG;
    else
        p->ctx->color_range = MPP_FRAME_RANGE_MPEG;

    if (sps->vui.colour_description_present_flag) {
        p->ctx->colorspace      = sps->vui.matrix_coeffs;
    } else {
        p->ctx->colorspace      = MPP_FRAME_SPC_UNSPECIFIED;
    }

    p->sps = sps;
    p->vps = p->vps_list[p->sps->vps_id];

    return 0;
}

static RK_S32 h265d_slice_head(H265dPrs *p)
{
    BitReadCtx_t *gb = &p->bit;
    H265dSlice *slice = &p->slice;
    RK_S32 type = p->nal_unit_type;
    RK_U32 pps_id;
    RK_S32 bit_begin;
    RK_U32 val_u32;
    RK_S32 val_s32;
    RK_S32 ret;
    RK_S32 i;

    READ_ONEBIT(gb, &slice->first_slice_in_pic_flag);
    if ((IS_IDR(type) || IS_BLA(type)) && slice->first_slice_in_pic_flag) {
        h265d_dbg_ref("seq_decode %d -> %d (type %d IDR/BLA)\n",
                      p->seq_decode, (p->seq_decode + 1) & 0xff, type);
        p->seq_decode = (p->seq_decode + 1) & 0xff;
        p->max_ra     = INT_MAX;
        if (IS_IDR(type))
            h265d_frame_unref_all(p);
    }
    if (IS_IRAP(type)) {
        /* no_output_of_prior_pics_flag */
        READ_ONEBIT(gb, &val_u32);
    }

    if (IS_IRAP(type) && p->miss_ref_flag && slice->first_slice_in_pic_flag) {
        p->max_ra     = INT_MAX;
        p->miss_ref_flag = 0;
    }
    READ_UE(gb, &pps_id);

    if (pps_id >= MAX_PPS_COUNT || !p->pps_list[pps_id]) {
        mpp_loge("pps: invalid pps id %d\n", pps_id);
        return MPP_ERR_STREAM;
    } else {
        slice->pps_id = pps_id;
        if (pps_id != p->pre_pps_id) {
            p->ps_need_upate = 1;
            p->pre_pps_id = pps_id;
        }
    }

    if (!slice->first_slice_in_pic_flag &&
        p->pps != (H265dPps*)p->pps_list[slice->pps_id]) {
        mpp_loge("pps: changed between slices\n");
        return MPP_ERR_STREAM;
    }
    p->pps = (H265dPps*)p->pps_list[slice->pps_id];

    /* Check if SPS changed using bitmap - bit set means SPS was (re)parsed */
    RK_U32 sps_id = p->pps->sps_id;
    if (MPP_GET_BIT(p->sps_update_mask, sps_id)) {
        h265d_dbg_ref("seq_decode %d -> %d (SPS updated sps_id %u)\n",
                      p->seq_decode, (p->seq_decode + 1) & 0xff, sps_id);
        p->sps = p->sps_list[sps_id];
        h265d_frame_unref_all(p);
        p->ps_need_upate = 1;
        ret = set_sps(p, p->sps);
        if (ret < 0)
            return ret;
        p->seq_decode = (p->seq_decode + 1) & 0xff;
        p->max_ra     = INT_MAX;
        MPP_CLR_BIT(p->sps_update_mask, sps_id);  /* Clear bit after processing */
    }

    slice->dependent_slice_segment_flag = 0;
    if (!slice->first_slice_in_pic_flag) {
        RK_S32 slice_address_length;

        if (p->pps->dependent_slice_segments_enabled_flag)
            READ_ONEBIT(gb, &slice->dependent_slice_segment_flag);

        slice_address_length = mpp_ceil_log2(p->sps->ctb_width *
                                             p->sps->ctb_height);

        READ_BITS(gb, slice_address_length, &slice->slice_segment_addr);

        if (slice->slice_segment_addr >= (RK_U32)(p->sps->ctb_width * p->sps->ctb_height)) {
            mpp_loge(
                "slice: address over ctb count %u\n",
                slice->slice_segment_addr);
            return MPP_ERR_STREAM;
        }

        if (!slice->dependent_slice_segment_flag) {
            p->slice_idx++;
        }
    } else {
        slice->slice_segment_addr = 0;
        p->slice_idx           = 0;
        p->slice_initialized   = 0;
    }

    if (!slice->dependent_slice_segment_flag) {
        RK_U32 slice_sample_adaptive_offset_flag[3] = {0};
        RK_U32 disable_deblocking_filter_flag = 0;
        RK_U32 collocated_list = 0;
        RK_U32 slice_temporal_mvp_enabled_flag = 0;
        RK_U32 nb_refs[2] = {0};

        p->slice_initialized = 0;

        for (i = 0; i < p->pps->num_extra_slice_header_bits; i++)
            SKIP_BITS(gb, 1);

        READ_UE(gb, &slice->slice_type);
        if (!(slice->slice_type == I_SLICE ||
              slice->slice_type == P_SLICE ||
              slice->slice_type == B_SLICE)) {
            mpp_loge("slice: unknown type %d\n",
                     slice->slice_type);
            return MPP_ERR_STREAM;
        }
        if (IS_IRAP(type) && slice->slice_type != I_SLICE) {
            mpp_loge("slice: irap frame has non-i slice\n");
            return MPP_ERR_STREAM;
        }

        if (p->pps->output_flag_present_flag) {
            /* pic_output_flag */
            READ_ONEBIT(gb, &val_u32);
        }

        if (p->sps->separate_colour_plane_flag) {
            /* colour_plane_id */
            READ_BITS(gb, 2, &val_u32);
        }

        if (!IS_IDR(type)) {
            RK_S32 poc;
            RK_S32 max_poc_lsb;
            RK_S32 prev_poc_lsb;
            RK_S32 prev_poc_msb;
            RK_S32 poc_msb;

            /* pic_order_cnt_lsb */
            READ_BITS(gb, p->sps->log2_max_poc_lsb, &slice->pic_order_cnt_lsb);

            max_poc_lsb = 1 << p->sps->log2_max_poc_lsb;
            prev_poc_lsb = p->pocTid0 % max_poc_lsb;
            prev_poc_msb = p->pocTid0 - prev_poc_lsb;

            if (slice->pic_order_cnt_lsb < prev_poc_lsb && prev_poc_lsb - slice->pic_order_cnt_lsb >= max_poc_lsb / 2)
                poc_msb = prev_poc_msb + max_poc_lsb;
            else if (slice->pic_order_cnt_lsb > prev_poc_lsb && slice->pic_order_cnt_lsb - prev_poc_lsb > max_poc_lsb / 2)
                poc_msb = prev_poc_msb - max_poc_lsb;
            else
                poc_msb = prev_poc_msb;

            if (IS_BLA(type))
                poc_msb = 0;

            poc = poc_msb + slice->pic_order_cnt_lsb;

            if (!slice->first_slice_in_pic_flag && poc != p->poc) {
                mpp_log("Ignoring POC change between slices: %d -> %d\n", p->poc, poc);
                poc = p->poc;
            }
            p->poc = poc;

            /* short_term_ref_pic_set_sps_flag */
            READ_ONEBIT(gb, &slice->short_term_ref_pic_set_sps_flag);

            bit_begin = gb->used_bits;

            if (!slice->short_term_ref_pic_set_sps_flag) {
                ret = h265d_st_rps(gb, &slice->st_rps_slice, p->sps, 1, &p->rps_need_upate);
                if (ret < 0)
                    return ret;

                slice->st_rps_using = &slice->st_rps_slice;
            } else {
                RK_S32 numbits, rps_idx;

                if (!p->sps->nb_st_rps) {
                    mpp_loge("sps: missing reference lists\n");
                    return MPP_ERR_STREAM;
                }

                numbits = mpp_ceil_log2(p->sps->nb_st_rps);
                rps_idx = 0;
                if (numbits > 0)
                    /* short_term_ref_pic_set_idx */
                    READ_BITS(gb, numbits, &rps_idx);

                if (slice->st_rps_using != &p->sps->st_rps_sps[rps_idx])
                    p->rps_need_upate = 1;
                slice->st_rps_using = &p->sps->st_rps_sps[rps_idx];
            }

            slice->short_term_ref_pic_set_size = gb->used_bits - bit_begin;

            ret = h265d_lt_rps(p, &slice->lt_rps, gb);
            if (ret < 0) {
                mpp_log("Invalid long term RPS.\n");
            }

            if (p->sps->sps_temporal_mvp_enabled_flag)
                /* slice_temporal_mvp_enabled_flag */
                READ_ONEBIT(gb, &slice_temporal_mvp_enabled_flag);
            else
                slice_temporal_mvp_enabled_flag = 0;
        } else {
            slice->st_rps_using = NULL;
            p->poc = 0;
        }

        /* Calculate number of reference frames */
        if (slice->slice_type == I_SLICE) {
            p->nb_refs = 0;
        } else {
            RK_S32 nb_refs = 0;
            const H265dStRps *st_rps = slice->st_rps_using;
            const H265dLtRps *lt_rps = &slice->lt_rps;
            RK_U32 i;

            if (st_rps) {
                for (i = 0; i < (RK_U32)st_rps->num_neg; i++)
                    nb_refs += MPP_GET_BIT(st_rps->flags, i);
                for (i = (RK_U32)st_rps->num_neg; i < (RK_U32)st_rps->num_deltas; i++)
                    nb_refs += MPP_GET_BIT(st_rps->flags, i);
            }
            if (lt_rps) {
                for (i = 0; i < (RK_U32)lt_rps->nb_refs; i++)
                    nb_refs += MPP_GET_BIT(lt_rps->used, i);
            }
            p->nb_refs = nb_refs;
        }

        if (p->temporal_id == 0 &&
            type != NAL_TRAIL_N && type != NAL_TSA_N   &&
            type != NAL_STSA_N  && type != NAL_RADL_N  &&
            type != NAL_RADL_R  && type != NAL_RASL_N  &&
            type != NAL_RASL_R)
            p->pocTid0 = p->poc;

        if (p->sps->sao_enabled) {
            /* slice_sao_luma_flag */
            READ_ONEBIT(gb, &slice_sample_adaptive_offset_flag[0]);
            if (p->sps->chroma_format_idc) {
                /* slice_sao_chroma_flag */
                READ_ONEBIT(gb, &slice_sample_adaptive_offset_flag[1]);
                slice_sample_adaptive_offset_flag[2] =
                    slice_sample_adaptive_offset_flag[1];
            } else {
                slice_sample_adaptive_offset_flag[1] = 0;
                slice_sample_adaptive_offset_flag[2] = 0;
            }
        } else {
            slice_sample_adaptive_offset_flag[0] = 0;
            slice_sample_adaptive_offset_flag[1] = 0;
            slice_sample_adaptive_offset_flag[2] = 0;
        }

        nb_refs[L0] = nb_refs[L1] = 0;
        if (slice->slice_type == P_SLICE || slice->slice_type == B_SLICE) {
            RK_S32 total_nb_refs;

            nb_refs[L0] = p->pps->num_ref_idx_l0_default_active;
            if (slice->slice_type == B_SLICE)
                nb_refs[L1] = p->pps->num_ref_idx_l1_default_active;

            /* num_ref_idx_active_override_flag */
            READ_ONEBIT(gb, &val_u32);

            if (val_u32) {
                /* num_ref_idx_l0_active_minus1 */
                READ_UE(gb, &val_u32);
                nb_refs[L0] = val_u32 + 1;
                if (slice->slice_type == B_SLICE) {
                    /* num_ref_idx_l1_active_minus1 */
                    READ_UE(gb, &val_u32);
                    nb_refs[L1] = val_u32 + 1;
                }
            }
            if (nb_refs[L0] > MAX_REFS || nb_refs[L1] > MAX_REFS) {
                mpp_loge("slice: reference count over limit %d %d\n",
                         nb_refs[L0], nb_refs[L1]);
                return MPP_ERR_STREAM;
            }

            total_nb_refs = p->nb_refs;
            if (!total_nb_refs) {
                mpp_loge("slice: p/b slice has no references\n");
                return MPP_ERR_STREAM;
            }

            if (p->pps->lists_modification_present_flag && total_nb_refs > 1) {
                RK_U32 rpl_modification_flag;
                /* ref_pic_list_modification_flag_l0 */
                READ_ONEBIT(gb, &rpl_modification_flag);
                if (rpl_modification_flag) {
                    for (i = 0; (RK_U32)i < nb_refs[L0]; i++) {
                        /* list_entry_l0 */
                        READ_BITS(gb, mpp_ceil_log2(total_nb_refs), &val_u32);
                    }
                }

                if (slice->slice_type == B_SLICE) {
                    /* ref_pic_list_modification_flag_l1 */
                    READ_ONEBIT(gb, &rpl_modification_flag);
                    if (rpl_modification_flag)
                        for (i = 0; (RK_U32)i < nb_refs[L1]; i++) {
                            /* list_entry_l1 */
                            READ_BITS(gb, mpp_ceil_log2(total_nb_refs), &val_u32);
                        }
                }
            }

            if (slice->slice_type == B_SLICE) {
                /* mvd_l1_zero_flag */
                READ_ONEBIT(gb, &val_u32);
            }

            if (p->pps->cabac_init_present_flag) {
                /* cabac_init_flag */
                READ_ONEBIT(gb, &val_u32);
            }

            if (slice_temporal_mvp_enabled_flag) {
                RK_U32 collocated_ref_idx = 0;
                collocated_list = L0;
                if (slice->slice_type == B_SLICE) {
                    /* collocated_from_l0_flag */
                    READ_ONEBIT(gb, &val_u32);
                    collocated_list = !val_u32;
                }

                if (nb_refs[collocated_list] > 1) {
                    /* collocated_ref_idx */
                    READ_UE(gb, &collocated_ref_idx);
                    if (collocated_ref_idx >= nb_refs[collocated_list]) {
                        mpp_loge("slice: collocated_ref_idx %d invalid\n",
                                 collocated_ref_idx);
                        return MPP_ERR_STREAM;
                    }
                }
            }

            if ((p->pps->weighted_pred_flag   && slice->slice_type == P_SLICE) ||
                (p->pps->weighted_bipred_flag && slice->slice_type == B_SLICE)) {
                pred_weight_table(gb, p->sps->chroma_format_idc, nb_refs, slice->slice_type);
            }

            /* five_minus_max_num_merge_cand */
            READ_UE(gb, &val_u32);
            if (val_u32 > 4) {
                mpp_loge("slice: five_minus_max_num_merge_cand %u invalid\n", val_u32);
                return MPP_ERR_STREAM;
            }
        }
        /* slice_qp_delta */
        READ_SE(gb, &val_s32);

        slice->slice_qp = 26U + p->pps->pic_init_qp_minus26 + val_s32;
        if (slice->slice_qp > 51 ||
            slice->slice_qp < -p->sps->qp_bd_offset) {
            mpp_loge("The slice_qp %d is outside the valid range [%d, 51].\n",
                     slice->slice_qp, -p->sps->qp_bd_offset);
            return MPP_ERR_STREAM;
        }

        if (p->pps->pic_slice_level_chroma_qp_offsets_present_flag) {
            /* slice_cb_qp_offset */
            READ_SE(gb, &val_s32);
            /* slice_cr_qp_offset */
            READ_SE(gb, &val_s32);
        }

        if (p->pps->deblocking_filter_control_present_flag) {
            RK_S32 deblocking_filter_override_flag = 0;

            if (p->pps->deblocking_filter_override_enabled_flag)
                /* deblocking_filter_override_flag */
                READ_ONEBIT(gb, &deblocking_filter_override_flag);

            if (deblocking_filter_override_flag) {
                /* slice_deblocking_filter_disabled_flag */
                READ_ONEBIT(gb, &disable_deblocking_filter_flag);
                if (!disable_deblocking_filter_flag) {
                    /* slice_beta_offset_div2 */
                    READ_SE(gb, &val_s32);
                    /* slice_tc_offset_div2 */
                    READ_SE(gb, &val_s32);
                }
            } else {
                disable_deblocking_filter_flag = p->pps->disable_dbf;
            }
        } else {
            disable_deblocking_filter_flag = 0;
        }

        if (p->pps->seq_loop_filter_across_slices_enabled_flag &&
            (slice_sample_adaptive_offset_flag[0] ||
             slice_sample_adaptive_offset_flag[1] ||
             !disable_deblocking_filter_flag)) {
            /* slice_loop_filter_across_slices_enabled_flag */
            READ_ONEBIT(gb, &val_u32);
        }
    } else if (!p->slice_initialized) {
        mpp_loge("slice: dependent slice without independent\n");
        return MPP_ERR_STREAM;
    }

    if (p->pps->tiles_enabled_flag || p->pps->entropy_coding_sync_enabled_flag) {
        RK_S32 num_entry_point_offsets = 0;
        /* num_entry_point_offsets */
        READ_UE(gb, &num_entry_point_offsets);

        if (p->pps->entropy_coding_sync_enabled_flag) {
            if (num_entry_point_offsets > p->sps->ctb_height || num_entry_point_offsets < 0) {
                mpp_loge("slice: entry count %d over ctb rows %d\n",
                         num_entry_point_offsets,
                         p->sps->ctb_height);
                return MPP_ERR_STREAM;
            }
        } else {
            if (num_entry_point_offsets > p->sps->ctb_height * p->sps->ctb_width || num_entry_point_offsets < 0) {
                mpp_loge("slice: entry count %d over ctb count %d\n",
                         num_entry_point_offsets,
                         p->sps->ctb_height * p->sps->ctb_width);
                return MPP_ERR_STREAM;
            }
        }
        if (num_entry_point_offsets) {
            RK_U32 offset_len_minus1 = 0;

            /* offset_len_minus1 */
            READ_UE(gb, &offset_len_minus1);
            for (i = 0; i < num_entry_point_offsets; i++)
                SKIP_BITS(gb, offset_len_minus1 + 1);
        }
    }
    if (p->pps->slice_header_extension_present_flag) {
        RK_U32 length = 0;

        p->start_bit = gb->used_bits;
        /* slice_header_extension_length */
        READ_UE(gb, &length);
        for (i = 0; (RK_U32)i < length; i++) {
            SKIP_BITS(gb, 8);
        }
        p->end_bit = gb->used_bits;
    }

    if (!slice->slice_segment_addr && slice->dependent_slice_segment_flag) {
        mpp_loge("slice: invalid segment\n");
        return MPP_ERR_STREAM;
    }

    p->slice_initialized = 1;

    return 0;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

RK_S32 H265d_nal_head(H265dPrs *p)
{
    BitReadCtx_t *gb = &p->bit;
    RK_S32 value = 0;

    READ_ONEBIT(gb, &value);

    READ_BITS(gb, 6, &p->nal_unit_type);
    READ_BITS(gb, 6, &p->nuh_layer_id);
    READ_BITS(gb, 3, &p->temporal_id);

    p->temporal_id = p->temporal_id - 1;

    h265d_dbg_global("nal_unit_type: %d, nuh_layer_id: %d temporal_id: %d\n",
                     p->nal_unit_type, p->nuh_layer_id, p->temporal_id);

    if (p->temporal_id < 0)
        return MPP_ERR_STREAM;

    return (p->nuh_layer_id);
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static RK_S32 h265d_out_dec_order(void *ctx)
{
    H265dCtx *h265dctx = (H265dCtx *)ctx;
    H265dPrs *p = (H265dPrs *)h265dctx->parser;

    if (p->ref && p->ref->ref_status.output) {
        p->ref->ref_status.output = 0;
        mpp_buf_slot_set_flag(p->slots, p->ref->slot_index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(p->slots, p->ref->slot_index, QUEUE_DISPLAY);
    }

    return 0;
}

RK_S32 h265d_dpb_output(void *ctx, RK_S32 flush)
{
    H265dCtx *h265dctx = (H265dCtx *)ctx;
    H265dPrs *p = (H265dPrs *)h265dctx->parser;
    MppDecCfgSet *cfg = h265dctx->cfg;
    RK_S32 type = p->nal_unit_type;
    RK_U32 find_next_ready = 0;

    if (cfg->base.fast_out)
        return h265d_out_dec_order(ctx);

    do {
        RK_S32 nb_output = 0;
        RK_S32 min_poc   = INT_MAX;
        RK_S32 min_idx = 0;
        RK_U32 i;

        for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
            H265dFrame *frame = &p->dpb[i];

            if (frame->ref_status.output &&
                frame->sequence == p->seq_output) {
                nb_output++;
                if (frame->poc < min_poc) {
                    min_poc = frame->poc;
                    min_idx = i;
                }
            }
        }

        if (!flush && p->seq_output == p->seq_decode && p->sps &&
            nb_output <= (RK_S32)p->sps->layers[p->sps->max_sub_layers - 1].num_reorder_pics) {
            if (cfg->base.enable_fast_play && (IS_IDR(type) ||
                                               (IS_BLA(type) && !p->first_i_fast_play))) {
                p->first_i_fast_play = 1;
            } else {
                return 0;
            }
        }

        if (nb_output) {
            H265dFrame *frame = &p->dpb[min_idx];

            frame->ref_status.output = 0;

            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(p->slots, frame->slot_index, QUEUE_DISPLAY);

            h265d_dbg_ref("output frame poc %d slot_index %d\n", frame->poc, frame->slot_index);

            do {
                find_next_ready = 0;
                for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
                    H265dFrame *next = &p->dpb[i];

                    if (next->ref_status.output &&
                        next->sequence == p->seq_output) {
                        if (next->poc == frame->poc + 1) {
                            find_next_ready = 1;
                            next->ref_status.output = 0;
                            frame = next;
                            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_QUEUE_USE);
                            mpp_buf_slot_enqueue(p->slots, frame->slot_index, QUEUE_DISPLAY);
                            h265d_dbg_ref("output frame poc %d slot_index %d\n",
                                          next->poc, next->slot_index);
                            for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
                                H265dFrmRefStatus mask = {0};
                                h265d_frame_unref(p, i, mask);
                            }
                        }
                    }
                }
            } while (find_next_ready);

            return 1;
        }

        if (p->seq_output != p->seq_decode)
            p->seq_output = (p->seq_output + 1) & 0xff;
        else
            break;
    } while (1);

    return 0;
}

static RK_S32 h265d_frame_new(H265dPrs *p)
{
    RK_S32 type = p->nal_unit_type;
    RK_S32 ret;

    if (p->ref) {
        mpp_log_f("found two frame in one packet do nothing!\n");
        return 0;
    }

    p->is_decoded = 0;
    p->first_nal_type = type;
    p->miss_ref_flag = 0;

    ret = h265d_build_rps(p);
    if (ret < 0) {
        mpp_loge("rps: failed to build\n");
        goto fail;
    }

    ret = h265d_build_refs(p, &p->frame, p->poc);
    if (ret < 0)
        goto fail;

    if (!p->ctx->cfg->base.disable_error && p->recovery.valid_flag &&
        p->recovery.first_frm_valid && p->recovery.first_frm_ref_missing &&
        p->poc < p->recovery.recovery_pic_id && p->poc >= p->recovery.first_frm_id) {
        mpp_frame_set_discard(p->frame, 1);
        h265d_dbg_ref("mark recovery frame discard poc %d\n", mpp_frame_get_poc(p->frame));
    }

    if (!p->ctx->cfg->base.disable_error && p->miss_ref_flag) {
        if (!IS_IRAP(type)) {
            if (p->recovery.valid_flag && p->recovery.first_frm_valid && p->recovery.first_frm_id == p->poc) {
                p->recovery.first_frm_ref_missing = 1;
                mpp_frame_set_discard(p->frame, 1);
                h265d_dbg_ref("recovery frame missing ref mark discard poc %d\n",
                              mpp_frame_get_poc(p->frame));
            } else {
                mpp_frame_set_errinfo(p->frame, MPP_FRAME_ERR_UNKNOW);
                p->ref->err_status.has_err = 1;
                h265d_dbg_ref("missing ref mark error poc %d\n", mpp_frame_get_poc(p->frame));
            }
        } else {
            H265dFrame *frame = NULL;
            RK_U32 i = 0;
            for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
                frame = &p->dpb[i];
                if (frame->poc == p->poc ) {
                    frame->ref_status.output = 0;
                    break;
                } else {
                    frame = NULL;
                }
            }
            do {
                ret = h265d_dpb_output(p->ctx, 1);
            } while (ret);
            if (frame) {
                frame->ref_status.output = 1;
            }
        }
    }

    mpp_buf_slot_set_prop(p->slots, p->ref->slot_index, SLOT_FRAME, p->ref->frame);

    return 0;

fail:
    p->ref = NULL;
    return ret;
}

RK_S32 h265d_nal_unit(H265dPrs *p, const RK_U8 *nal, RK_S32 length)
{
    BitReadCtx_t *gb = &p->bit;
    RK_S32 type;
    RK_S32 ret;

    mpp_set_bitread_ctx(gb, (RK_U8*)nal, length);
    mpp_set_bitread_pseudo_code_type(gb, PSEUDO_CODE_H264_H265);

    ret = H265d_nal_head(p);
    type = p->nal_unit_type;
    if (ret < 0) {
        mpp_loge("nal: unit type %d invalid skip\n", type);
        goto fail;
    } else if (ret && type != NAL_VPS)
        return 0;

    if (p->temporal_id > p->temporal_layer_id)
        return 0;

    p->nuh_layer_id = ret;

    h265d_dbg_global("nal_unit_type %d len %d\n", type, length);

    if (p->deny_flag && (type != NAL_VPS && type != NAL_SPS)) {
        ret = MPP_ERR_STREAM;
        goto fail;
    }

    switch (type) {
    case NAL_VPS: {
        MppMemPool *pool = p->vps_pool;
        H265dVps *vps = (H265dVps *)mpp_mem_pool_get_f(pool);

        if (!vps) {
            ret = MPP_ERR_NOMEM;
            goto fail;
        }
        ret = h265d_nal_vps(&p->bit, vps);
        if (ret < 0) {
            mpp_mem_pool_put_f(pool, vps);
            if (!p->is_decoded) {
                mpp_loge("h265d_nal_vps ret %d", ret);
                goto fail;
            }
        } else {
            RK_U32 vps_id = vps->vps_id;
            H265dVps *vps_old = p->vps_list[vps_id];

            if (vps_old && !memcmp(vps_old, vps, sizeof(H265dVps))) {
                mpp_mem_pool_put_f(pool, vps);
            } else {
                if (vps_old != NULL) {
                    mpp_mem_pool_put_f(pool, vps_old);
                }
                p->vps_list[vps_id] = vps;
                p->ps_need_upate = 1;
            }
        }
    } break;
    case NAL_SPS: {
        H265dSps *sps = (H265dSps *)mpp_mem_pool_get_f(p->sps_pool);
        H265dSps *old_sps;
        RK_S32 i;
        RK_S32 sps_changed = 0;

        if (!sps) {
            ret = MPP_ERR_NOMEM;
            goto fail;
        }

        ret = h265d_nal_sps(&p->bit, sps, (const H265dVps **)p->vps_list);
        if (ret < 0) {
            mpp_mem_pool_put_f(p->sps_pool, sps);
            if (!p->is_decoded) {
                mpp_loge("h265d_nal_sps ret %d", ret);
                goto fail;
            }
        } else {
            RK_U32 sps_id = sps->sps_id;

            h265d_dbg_split("h265d_nal_unit: SPS %u parsed successfully, size %dx%d\n",
                            sps_id, sps->output_width, sps->output_height);

            old_sps = p->sps_list[sps_id];

            /* Compare SPS content to detect actual changes */
            if (old_sps) {
                sps_changed = (memcmp(old_sps, sps, sizeof(H265dSps)) != 0);
            } else {
                sps_changed = 1;  /* New SPS */
            }

            for (i = 0; i < MAX_PPS_COUNT; i++) {
                if (p->pps_list[i] && p->pps_list[i]->sps_id == sps_id) {
                    MPP_FREE(p->pps_list[i]);
                    p->pps_list[i] = NULL;
                }
            }

            if (old_sps)
                mpp_mem_pool_put_f(p->sps_pool, old_sps);

            p->sps_list[sps_id] = sps;

            /* Only set update bit if SPS actually changed */
            if (sps_changed)
                MPP_SET_BIT(p->sps_update_mask, sps_id);

            p->ctx->width = sps->output_width;
            p->ctx->height = sps->output_height;

            if (sps->vui.transfer_characteristic == MPP_FRAME_TRC_SMPTEST2084 ||
                sps->vui.transfer_characteristic == MPP_FRAME_TRC_ARIB_STD_B67)
                p->is_hdr = 1;

            switch (sps->chroma_format_idc) {
            case H265_CHROMA_422 : {
                mpp_slots_set_prop(p->slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);
            } break;
            case H265_CHROMA_444 : {
                mpp_slots_set_prop(p->slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv444);
            } break;
            default : {
            } break;
            }
        }

        p->deny_flag = 0;
    } break;
    case NAL_PPS : {
        H265dPps *pps = h265d_nal_pps(&p->bit, (const H265dSps **)p->sps_list);

        if (!pps) {
            if (!p->is_decoded) {
                mpp_loge("h265d_nal_pps failed\n");
                goto fail;
            }
        } else {
            RK_U32 pps_id = pps->pps_id;
            H265dPps *old_pps = p->pps_list[pps_id];
            H265dSps *sps = (H265dSps *)p->sps_list[pps->sps_id];
            RK_S32 pps_changed = 0;

            ret = h265d_pps_check(pps, sps, p->hw_info);
            if (ret < 0) {
                MPP_FREE(pps);
                if (!p->is_decoded) {
                    mpp_loge("h265d_pps_check ret %d", ret);
                    goto fail;
                }
            } else {
                /* Compare PPS content to detect actual changes */
                if (old_pps) {
                    /* Use total_size for direct memcmp comparison */
                    if (old_pps->total_size == pps->total_size) {
                        pps_changed = (memcmp(old_pps, pps, pps->total_size) != 0);
                    } else {
                        pps_changed = 1;  /* Size changed */
                    }
                } else {
                    pps_changed = 1;  /* New PPS */
                }

                MPP_FREE(old_pps);
                p->pps_list[pps_id] = pps;

                /* Only set update bit if PPS actually changed */
                if (pps_changed)
                    MPP_SET_BIT64(p->pps_update_mask, pps_id);
            }
        }
    } break;
    case NAL_SEI_PREFIX :
    case NAL_SEI_SUFFIX : {
        ret = h265d_nal_sei(p);
        if (ret < 0) {
            mpp_loge("h265d_decode_nal_sei ret %d", ret);
        }
    } break;
    case NAL_TRAIL_R :
    case NAL_TRAIL_N :
    case NAL_TSA_N :
    case NAL_TSA_R :
    case NAL_STSA_N :
    case NAL_STSA_R :
    case NAL_BLA_W_LP :
    case NAL_BLA_W_RADL :
    case NAL_BLA_N_LP :
    case NAL_IDR_W_RADL :
    case NAL_IDR_N_LP :
    case NAL_CRA_NUT :
    case NAL_RADL_N :
    case NAL_RADL_R :
    case NAL_RASL_N :
    case NAL_RASL_R : {
        if (p->task == NULL) {
            p->extra_has_frame = 1;
            break;
        }

        ret = h265d_slice_head(p);
        if (ret < 0) {
            h265d_dbg_slice("h265d_slice_head ret %d", ret);
            if ((p->first_nal_type != type) && (p->first_nal_type != NAL_INIT_VALUE))
                return 0;

            return ret;
        }

        if (p->recovery.valid_flag) {
            if (!p->recovery.first_frm_valid) {
                p->recovery.first_frm_id = p->poc;
                p->recovery.first_frm_valid = 1;
                p->recovery.recovery_pic_id = p->recovery.first_frm_id + p->recovery.recovery_frame_cnt;
                h265d_dbg_sei( "First recovery frame found, poc %d", p->recovery.first_frm_id);
            } else {
                if (p->recovery.recovery_pic_id < p->poc)
                    memset(&p->recovery, 0, sizeof(RecoveryPoint));
            }
        }

        if (p->max_ra == INT_MAX) {
            if (type == NAL_CRA_NUT || IS_BLA(type) ||
                (p->recovery.valid_flag && p->recovery.first_frm_valid &&
                 p->recovery.first_frm_id == p->poc)) {
                p->max_ra = p->poc;
            } else {
                if (IS_IDR(type))
                    p->max_ra = INT_MIN;
            }
        }

        if ((type == NAL_RASL_R || type == NAL_RASL_N) &&
            p->poc <= p->max_ra) {
            p->is_decoded = 0;
            break;
        } else if (!p->ctx->cfg->base.disable_error &&
                   (p->poc < p->max_ra) && !IS_IRAP(type)) {
            p->is_decoded = 0;
            break;
        } else {
            if (type == NAL_RASL_R && p->poc > p->max_ra)
                p->max_ra = INT_MIN;
        }

        if (p->slice.first_slice_in_pic_flag) {
            ret = h265d_frame_new(p);
            if (ret < 0) {
                mpp_loge("h265d_frame_new %d", ret);
                return ret;
            }
        } else if (!p->ref) {
            mpp_loge("slice: missing first slice\n");
            goto fail;
        }

        if (type != p->first_nal_type) {
            mpp_loge("nal: type mismatch %d %d\n",
                     p->first_nal_type, type);
            goto fail;
        }

        if (!p->slice.dependent_slice_segment_flag &&
            p->slice.slice_type != I_SLICE) {
            if (ret < 0) {
                mpp_loge("ref lists: failed to build\n");
                goto fail;
            }
        }

        p->is_decoded = 1;

        if (p->pps && p->pps->slice_header_extension_present_flag) {
            h265d_dxva2_picture_context_t *syn = (h265d_dxva2_picture_context_t *)p->hal_pic_private;
            DXVA_Slice_HEVC_Cut_Param *param = &syn->slice_cut_param[p->slice_cnt];

            param->start_bit = p->start_bit;
            param->end_bit = p->end_bit;
            param->is_enable = 1;
        }
        p->slice_cnt++;
    } break;
    case NAL_EOS_NUT :
    case NAL_EOB_NUT : {
        h265d_dbg_ref("seq_decode %d -> %d (EOS/EOB type %d)\n",
                      p->seq_decode, (p->seq_decode + 1) & 0xff, type);
        p->seq_decode = (p->seq_decode + 1) & 0xff;
        p->max_ra     = INT_MAX;
    } break;
    case NAL_AUD :
    case NAL_FD_NUT : {
    } break;
    case NAL_UNSPEC62: {
        if (length > 2) {
            h265d_fill_dynamic_meta(p, nal + 2, length - 2, DLBY);
        }
    } break;
    default : {
        mpp_log("Skipping NAL unit %d\n", type);
    } break;
    }

    return 0;
fail:

    return ret;
}


typedef union {
    RK_U32  u32;
    RK_U16  u16[2];
    RK_U8   u8 [4];
    float   f32;
} mpp_alias32;

#define MPP_FAST_UNALIGNED 1


#ifndef MPP_RN32A
#define MPP_RN32A(p) (((const mpp_alias32*)(p))->u32)
#endif

void h265d_fill_dynamic_meta(H265dPrs *p, const RK_U8 *data, RK_U32 size, RK_U32 hdr_fmt)
{
    MppRefPool *pool = &p->hdr_meta_pool;
    RK_S32 idx;
    MppFrameHdrDynamicMeta *meta;

    /* Cleanup unused slots in pool */
    mpp_ref_pool_cleanup(pool);

    /* Allocate a new slot from pool (include struct header size) */
    idx = mpp_ref_pool_get(pool, sizeof(MppFrameHdrDynamicMeta) + size);
    if (idx < 0) {
        mpp_loge_f("failed to allocate hdr meta slot\n");
        return;
    }

    meta = (MppFrameHdrDynamicMeta *)mpp_ref_pool_ptr(pool, idx);
    if (size && data) {
        switch (hdr_fmt) {
        case DLBY: {
            RK_U8 start_code[4] = {0, 0, 0, 1};

            memcpy((RK_U8*)meta->data, start_code, 4);
            memcpy((RK_U8*)meta->data + 4, (RK_U8*)data, size - 4);
        } break;
        case HDRVIVID:
        case HDR10PLUS: {
            memcpy((RK_U8*)meta->data, (RK_U8*)data, size);
        } break;
        default:
            break;
        }
        meta->size = size;
        meta->hdr_fmt = hdr_fmt;
    }
    /* Update current active meta index */
    p->hdr_meta_current = idx;
    p->hdr_dynamic = 1;
    p->is_hdr = 1;

}

RK_S32 h265d_nal_units(H265dPrs *p)
{
    RK_S32 ret = 0;
    RK_S32 i;

    p->slice_cnt = 0;

    for (i = 0; i < p->nb_nals; i++) {
        const RK_U8 *nal_data = p->bitstream + p->nals[i].bitstream_offset + 3;
        RK_S32 nal_size = p->nals[i].size;

        ret = h265d_nal_unit(p, nal_data, nal_size);
        if (ret < 0) {
            mpp_loge("nal: parse unit #%d failed ret %d\n", i, ret);
            break;
        }
    }

    return ret;
}

static RK_S32 find_embedded_start_code(const RK_U8 *buf, RK_S32 length)
{
    RK_S32 i;
    RK_S32 actual_length = length;

#define STARTCODE_TEST \
    if (i + 2 < actual_length && buf[i + 1] == 0 && buf[i + 2] == 1) { \
        actual_length = i; \
        break; \
    }

#if MPP_FAST_UNALIGNED
#define FIND_FIRST_ZERO \
    if (i > 0 && !buf[i]) \
        i--; \
    while (buf[i]) \
        i++

    for (i = 0; i + 1 < (RK_S32)actual_length; i += 5) {
        if (!((~MPP_RN32A(buf + i) &
               (MPP_RN32A(buf + i) - 0x01000101U)) &
              0x80008080U))
            continue;

        FIND_FIRST_ZERO;
        STARTCODE_TEST;
        i -= 3;
    }
#else
    for (i = 0; i + 1 < (RK_S32)actual_length; i += 2) {
        if (buf[i])
            continue;
        if (i > 0 && buf[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
#endif

    return actual_length;
}

static RK_S32 h265d_locate_start_code(const RK_U8 *buf, RK_U32 length)
{
    RK_U32 i;
    RK_U32 state = (RK_U32) - 1;

    for (i = 0; i < length; i++) {
        state = (state << 8) | buf[i];
        if (((state >> 8) & 0xFFFFFF) == START_CODE) {
            return (i >= 3) ? (i - 3) : 0;
        }
    }

    return -1;
}

static RK_S32 process_one_nal(H265dPrs *p, const RK_U8 *nal_data, RK_S32 nal_size)
{
    H265dNal *nal = &p->nals[p->nb_nals];
    static const RK_U8 start_code[] = { 0, 0, 1 };
    RK_S32 actual_size;

    actual_size = find_embedded_start_code(nal_data, nal_size);
    nal->size = actual_size;

    if (p->bitstream_pos + 3 > p->bitstream_size) {
        mpp_loge("bitstream overflow\n");
        return -1;
    }
    memcpy(p->bitstream + p->bitstream_pos, start_code, 3);
    nal->bitstream_offset = p->bitstream_pos;
    p->bitstream_pos += 3;

    if (p->bitstream_pos + actual_size > p->bitstream_size) {
        mpp_loge("bitstream overflow\n");
        return -1;
    }
    memcpy(p->bitstream + p->bitstream_pos, nal_data, actual_size);
    p->bitstream_pos += actual_size;

    return actual_size;
}

RK_S32 h265d_split_nal(H265dPrs *p, RK_U8 *buf, RK_S32 length)
{
    RK_S32 max_bitstream_size = length + (length / 100) * 3 + 1024;
    RK_U32 total_consumed = 0;
    RK_S32 consumed;
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    if (p->bitstream_size < max_bitstream_size) {
        mpp_free(p->bitstream);
        p->bitstream = mpp_malloc(RK_U8, max_bitstream_size);
        if (!p->bitstream) {
            ret = MPP_ERR_NOMEM;
            goto fail;
        }
        p->bitstream_size = max_bitstream_size;
    }
    p->bitstream_pos = 0;
    p->nb_nals = 0;

    while (length >= 4) {
        H265dNal *nal;
        RK_S32 extract_length = 0;
        const RK_U8 *nal_data;

        if (p->is_nalff) {
            for (i = 0; i < p->nal_length_size; i++)
                extract_length = (extract_length << 8) | buf[i];

            buf    += p->nal_length_size;
            length -= p->nal_length_size;
            total_consumed += p->nal_length_size;

            nal_data = buf;

            /*
             * Use unsigned comparison to correctly handle corrupted data.
             * When stream data is corrupted (e.g., during seeking), extract_length
             * may become a large negative value like -1347309641 (0xAFAFFB11).
             * Signed comparison would incorrectly treat this as less than length,
             * bypassing the check and causing memcpy to crash.
             */
            if ((RK_U32)extract_length > (RK_U32)length) {
                mpp_loge("nal: unit size invalid extract_length %d length %d\n",
                         extract_length, length);
                ret = MPP_ERR_STREAM;
                goto fail;
            }
        } else {
            if (buf[2] == 0) {
                length--;
                buf++;
                continue;
            }
            if (buf[0] != 0 || buf[1] != 0 || buf[2] != 1) {
                h265d_dbg_split("buf[0-2] not start code, searching... buf[0-7] %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
                RK_S32 offset = h265d_locate_start_code(buf, length);
                h265d_dbg_split("h265d_locate_start_code returned %d\n", offset);
                if (offset >= 0) {
                    length -= offset;
                    buf += offset;
                    h265d_dbg_split("Found start code at offset %d, buf[0-2] %02x %02x %02x\n",
                                    offset, buf[0], buf[1], buf[2]);
                    continue;
                }

                if (p->nb_nals) {
                    h265d_dbg_split("No more start code found, returning OK with %d NALs\n", p->nb_nals);
                    return MPP_OK;
                } else {
                    ret = MPP_ERR_STREAM;
                    goto fail;
                }
            }

            buf    += 3;
            length -= 3;
            extract_length = length;
            nal_data = buf;
            h265d_dbg_split("Found start code, extract_length %d\n", extract_length);
        }

        if (!extract_length) {
            return MPP_OK;
        }
        if (p->nals_allocated < p->nb_nals + 1) {
            RK_S32 old_size = p->nals_allocated;
            RK_S32 new_size = old_size + 10;

            if (new_size < 10)
                new_size = 10;

            p->nals = mpp_realloc(p->nals, H265dNal, new_size);
            if (!p->nals) {
                mpp_loge("realloc nals failed new_size %d", new_size);
                ret = MPP_ERR_NOMEM;
                goto fail;
            }
            memset(p->nals + old_size, 0, (new_size - old_size) * sizeof(H265dNal));
            p->nals_allocated = new_size;
        }

        consumed = process_one_nal(p, nal_data, extract_length);
        if (consumed < 0) {
            ret = MPP_ERR_NOMEM;
            goto fail;
        }

        p->nb_nals++;
        nal = &p->nals[p->nb_nals - 1];
        mpp_set_bitread_ctx(&p->bit, p->bitstream + nal->bitstream_offset + 3, nal->size);
        mpp_set_bitread_pseudo_code_type(&p->bit, PSEUDO_CODE_H264_H265);
        if (H265d_nal_head(p) < 0) {
            p->nb_nals--;
        }

        buf    += consumed;
        length -= consumed;
        total_consumed += consumed;

        if (p->is_nalff && p->nb_nals > 0 && length >= p->nal_length_size + 3) {
            RK_S32 next_nal_length = 0;

            for (i = 0; i < p->nal_length_size; i++)
                next_nal_length = (next_nal_length << 8) | buf[i];

            if (next_nal_length >= 3 && p->nal_length_size + next_nal_length <= (RK_S32)length) {
                const RK_U8 *next_nal_header = buf + p->nal_length_size;
                RK_U8 next_nal_unit_type = (next_nal_header[0] >> 1) & 0x3F;
                RK_U8 next_first_slice_flag = next_nal_header[2] >> 7;

                if (next_nal_unit_type <= NAL_RASL_R ||
                    (next_nal_unit_type >= NAL_BLA_W_LP && next_nal_unit_type <= NAL_CRA_NUT)) {
                    if (next_first_slice_flag) {
                        h265d_dbg_func(
                            "Detected NEXT frame: NAL type %d, first_slice=1, "
                            "stopping at NAL #%d\n",
                            next_nal_unit_type, p->nb_nals);
                        break;
                    }
                }
            }
        }
    }

    p->consumed_bytes = total_consumed;

fail:

    return (p->nb_nals) ? MPP_OK : ret;
}

static RK_U16 U16_AT(const RK_U8 *ptr)
{
    return ptr[0] << 8 | ptr[1];
}

RK_S32 h265d_extradata(H265dPrs *p)
{
    H265dCtx *h265dctx = p->ctx;
    RK_S32 ret = MPP_SUCCESS;

    p->slice_cnt = 0;

    if (h265dctx->extradata_size > 3 &&
        (h265dctx->extradata[0] || h265dctx->extradata[1] ||
         h265dctx->extradata[2] > 1)) {
        const RK_U8 *ptr = (const RK_U8 *)h265dctx->extradata;
        RK_U32 size = h265dctx->extradata_size;
        RK_U32 numofArrays = 0, numofNals = 0;
        RK_U32 j = 0, i = 0;

        if (size < 7) {
            return MPP_NOK;
        }

        mpp_log("extradata is encoded as hvcC format");
        p->is_nalff = 1;
        p->nal_length_size = 1 + (ptr[14 + 7] & 3);
        ptr += 22;
        size -= 22;
        numofArrays = (char)ptr[0];
        ptr += 1;
        size -= 1;
        for (i = 0; i < numofArrays; i++) {
            ptr += 1;
            size -= 1;
            numofNals = U16_AT(ptr);
            ptr += 2;
            size -= 2;

            for (j = 0; j < numofNals; j++) {
                RK_U32 length = 0;
                if (size < 2) {
                    return MPP_NOK;
                }

                length = U16_AT(ptr);

                ptr += 2;
                size -= 2;
                if (size < length) {
                    return MPP_NOK;
                }
                h265d_nal_unit(p, ptr, length);
                ptr += length;
                size -= length;
            }
        }
    } else {
        p->is_nalff = 0;
        ret = h265d_split_nal(p, h265dctx->extradata, h265dctx->extradata_size);
        if (ret < 0)
            return ret;
        ret = h265d_nal_units(p);
        if (ret < 0)
            return ret;
    }
    return ret;
}

RK_S32 h265d_parser_init(H265dParser *s, H265dCtx* ctx)
{
    H265dPrs *p = NULL;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    RK_S32 ret = MPP_OK;
    RK_U32 i;

    if (NULL == s || NULL == ctx) {
        mpp_loge_f("invalid parser %p ctx %p\n", s, ctx);
        return MPP_ERR_NULL_PTR;
    }

    *s = NULL;

    p = mpp_calloc(H265dPrs, 1);
    if (NULL == p) {
        mpp_loge("malloc H265dPrs failed\n");
        return MPP_ERR_MALLOC;
    }

    p->ctx = ctx;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrame *frm = &p->dpb[i];

        frm->slot_index = 0xff;
        frm->poc = INT_MAX;
        frm->err_status.has_err = 0;

        mpp_frame_init(&frm->frame);

        if (!frm->frame)
            return MPP_ERR_NOMEM;
    }

    p->max_ra = INT_MAX;
    p->temporal_layer_id   = 8;
    p->first_nal_type = NAL_INIT_VALUE;

    p->hal_pic_private = mpp_calloc_size(void, sizeof(h265d_dxva2_picture_context_t));
    if (p->hal_pic_private) {
        h265d_dxva2_picture_context_t *syn = (h265d_dxva2_picture_context_t *)p->hal_pic_private;

        syn->slice_short = (DXVA_Slice_HEVC_Short *)mpp_malloc(DXVA_Slice_HEVC_Short, MAX_SLICES);
        if (!syn->slice_short)
            return MPP_ERR_NOMEM;

        syn->slice_cut_param = (DXVA_Slice_HEVC_Cut_Param *)mpp_malloc(DXVA_Slice_HEVC_Cut_Param, MAX_SLICES);
        if (!syn->slice_cut_param)
            return MPP_ERR_NOMEM;
        syn->max_slice_num = MAX_SLICES;
    } else {
        return MPP_ERR_NOMEM;
    }

    p->slots = ctx->parser_cfg->frame_slots;
    p->packet_slots = ctx->parser_cfg->packet_slots;
    p->hw_info = ctx->parser_cfg->hw_info;

    buf = mpp_malloc(RK_U8, size);
    if (buf == NULL)
        return MPP_ERR_NOMEM;

    if (MPP_OK != mpp_packet_init(&p->input_packet, (void*)buf, size))
        return MPP_ERR_NOMEM;

    p->pre_pps_id = -1;

    /* Initialize update bitmasks */
    p->sps_update_mask = 0;
    p->pps_update_mask = 0;

    p->vps_pool = mpp_mem_pool_init_f("h265d_vps", sizeof(H265dVps));
    p->sps_pool = mpp_mem_pool_init_f("h265d_sps", sizeof(H265dSps));

    /* Initialize HDR metadata pool */
    ret = mpp_ref_pool_init(&p->hdr_meta_pool, 4);
    if (ret != MPP_OK) {
        mpp_loge_f("failed to init hdr meta pool\n");
        return ret;
    }
    p->hdr_meta_current = -1;

    *s = p;

    return MPP_OK;
}

RK_S32 h265d_parser_deinit(H265dParser ctx)
{
    H265dPrs *p = (H265dPrs *)ctx;
    RK_U8 *buf = NULL;
    RK_S32 i;

    if (NULL == p)
        return MPP_OK;

    for (i = 0; i < MAX_DPB_SIZE; i++) {
        H265dFrmRefStatus mask = {.flags = 0x07};
        h265d_frame_unref(p, i, mask);
        mpp_frame_deinit(&p->dpb[i].frame);
    }

    for (i = 0; i < MAX_VPS_COUNT; i++) {
        if (p->vps_list[i])
            mpp_mem_pool_put_f(p->vps_pool, p->vps_list[i]);
    }
    for (i = 0; i < MAX_SPS_COUNT; i++) {
        if (p->sps_list[i])
            mpp_mem_pool_put_f(p->sps_pool, p->sps_list[i]);
    }
    for (i = 0; i < MAX_PPS_COUNT; i++)
        MPP_FREE(p->pps_list[i]);

    MPP_FREE(p->nals);
    MPP_FREE(p->bitstream);

    p->nals_allocated = 0;
    p->bitstream_size = 0;
    p->bitstream_pos = 0;

    if (p->hal_pic_private) {
        h265d_dxva2_picture_context_t *ctx_pic = (h265d_dxva2_picture_context_t *)p->hal_pic_private;

        MPP_FREE(ctx_pic->slice_short);
        MPP_FREE(ctx_pic->slice_cut_param);
        mpp_free(p->hal_pic_private);
    }

    if (p->input_packet) {
        buf = mpp_packet_get_data(p->input_packet);
        mpp_free(buf);
        mpp_packet_deinit(&p->input_packet);
    }

    if (p->vps_pool)
        mpp_mem_pool_deinit_f(p->vps_pool);
    if (p->sps_pool)
        mpp_mem_pool_deinit_f(p->sps_pool);

    /* Deinitialize HDR metadata pool */
    mpp_ref_pool_deinit(&p->hdr_meta_pool);

    mpp_free(p);

    return MPP_OK;
}
