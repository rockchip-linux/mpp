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
 * @file       h265d_ps.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG "H265PARSER_PS"

#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "h265d_parser.h"

static const RK_U8 default_scaling_list_intra[] = {
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29, 36, 47, 65, 88, 115
};

static const RK_U8 default_scaling_list_inter[] = {
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91
};

static const MppRational_t vui_sar[] = {
    {  0,   1 },
    {  1,   1 },
    { 12,  11 },
    { 10,  11 },
    { 16,  11 },
    { 40,  33 },
    { 24,  11 },
    { 20,  11 },
    { 32,  11 },
    { 80,  33 },
    { 18,  11 },
    { 15,  11 },
    { 64,  33 },
    { 160, 99 },
    {  4,   3 },
    {  3,   2 },
    {  2,   1 },
};

const RK_U8 mpp_hevc_diag_scan4x4_x[16] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 1, 2,
    3, 2, 3, 3,
};

const RK_U8 mpp_hevc_diag_scan4x4_y[16] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 3, 2,
    1, 3, 2, 3,
};

const RK_U8 mpp_hevc_diag_scan8x8_x[64] = {
    0, 0, 1, 0,
    1, 2, 0, 1,
    2, 3, 0, 1,
    2, 3, 4, 0,
    1, 2, 3, 4,
    5, 0, 1, 2,
    3, 4, 5, 6,
    0, 1, 2, 3,
    4, 5, 6, 7,
    1, 2, 3, 4,
    5, 6, 7, 2,
    3, 4, 5, 6,
    7, 3, 4, 5,
    6, 7, 4, 5,
    6, 7, 5, 6,
    7, 6, 7, 7,
};

const RK_U8 mpp_hevc_diag_scan8x8_y[64] = {
    0, 1, 0, 2,
    1, 0, 3, 2,
    1, 0, 4, 3,
    2, 1, 0, 5,
    4, 3, 2, 1,
    0, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 0,
    7, 6, 5, 4,
    3, 2, 1, 7,
    6, 5, 4, 3,
    2, 7, 6, 5,
    4, 3, 7, 6,
    5, 4, 7, 6,
    5, 7, 6, 7,
};

int mpp_hevc_decode_short_term_rps(HEVCContext *s, ShortTermRPS *rps,
                                   const HEVCSPS *sps, RK_S32 is_slice_header)
{
    HEVCLocalContext *lc = s->HEVClc;
    RK_U8 rps_predict = 0;
    RK_S32 delta_poc;
    RK_S32 k0 = 0;
    RK_S32 k1 = 0;
    RK_S32 k  = 0;
    RK_S32 i;

    BitReadCtx_t *gb = &lc->gb;

    if (rps != sps->st_rps && sps->nb_st_rps)
        READ_ONEBIT(gb, &rps_predict);

    if (rps_predict) {
        const ShortTermRPS *rps_ridx;
        RK_S32 delta_rps, abs_delta_rps;
        RK_U8 use_delta_flag = 0;
        RK_U8 delta_rps_sign;

        if (is_slice_header) {
            RK_U32 delta_idx = 0;
            READ_UE(gb, &delta_idx);
            delta_idx = delta_idx + 1;
            if (delta_idx > sps->nb_st_rps) {
                mpp_err(
                    "Invalid value of delta_idx in slice header RPS: %d > %d.\n",
                    delta_idx, sps->nb_st_rps);
                return  MPP_ERR_STREAM;
            }
            rps_ridx = &sps->st_rps[sps->nb_st_rps - delta_idx];
        } else
            rps_ridx = &sps->st_rps[rps - sps->st_rps - 1];

        READ_BITS(gb, 1, &delta_rps_sign);

        READ_UE(gb, &abs_delta_rps);

        abs_delta_rps = abs_delta_rps + 1;

        delta_rps      = (1 - (delta_rps_sign << 1)) * abs_delta_rps;
        for (i = 0; i <= rps_ridx->num_delta_pocs; i++) {
            RK_S32 used = 0;
            READ_ONEBIT(gb, &used);

            rps->used[k] = used;

            if (!used)
                READ_ONEBIT(gb, &use_delta_flag);

            if (used || use_delta_flag) {
                if (i < rps_ridx->num_delta_pocs)
                    delta_poc = delta_rps + rps_ridx->delta_poc[i];
                else
                    delta_poc = delta_rps;
                rps->delta_poc[k] = delta_poc;
                if (delta_poc < 0)
                    k0++;
                else
                    k1++;
                k++;
            }
        }

        rps->num_delta_pocs    = k;
        rps->num_negative_pics = k0;
        // sort in increasing order (smallest first)
        if (rps->num_delta_pocs != 0) {
            RK_S32 used, tmp;
            for (i = 1; i < rps->num_delta_pocs; i++) {
                delta_poc = rps->delta_poc[i];
                used      = rps->used[i];
                for (k = i - 1; k >= 0; k--) {
                    tmp = rps->delta_poc[k];
                    if (delta_poc < tmp) {
                        rps->delta_poc[k + 1] = tmp;
                        rps->used[k + 1]      = rps->used[k];
                        rps->delta_poc[k]     = delta_poc;
                        rps->used[k]          = used;
                    }
                }
            }
        }
        if ((rps->num_negative_pics >> 1) != 0) {
            RK_S32 used;
            k = rps->num_negative_pics - 1;
            // flip the negative values to largest first
            for (i = 0; (RK_U32)i < rps->num_negative_pics >> 1; i++) {
                delta_poc         = rps->delta_poc[i];
                used              = rps->used[i];
                rps->delta_poc[i] = rps->delta_poc[k];
                rps->used[i]      = rps->used[k];
                rps->delta_poc[k] = delta_poc;
                rps->used[k]      = used;
                k--;
            }
        }
    } else {
        RK_U32 prev, nb_positive_pics;

        READ_UE(gb, &rps->num_negative_pics);

        READ_UE(gb, &nb_positive_pics);

        if (rps->num_negative_pics >= MAX_REFS ||
            nb_positive_pics >= MAX_REFS) {
            mpp_err( "Too many refs in a short term RPS.\n");
            return  MPP_ERR_STREAM;
        }

        rps->num_delta_pocs = rps->num_negative_pics + nb_positive_pics;
        if (rps->num_delta_pocs) {
            prev = 0;
            for (i = 0; (RK_U32)i < rps->num_negative_pics; i++) {
                READ_UE(gb, &delta_poc);
                delta_poc += 1;
                prev -= delta_poc;
                rps->delta_poc[i] = prev;
                READ_ONEBIT(gb, &rps->used[i]);
            }
            prev = 0;
            for (i = 0; (RK_U32)i < nb_positive_pics; i++) {
                READ_UE(gb, &delta_poc);
                delta_poc = delta_poc + 1;
                prev += delta_poc;
                rps->delta_poc[rps->num_negative_pics + i] = prev;
                READ_ONEBIT(gb, &rps->used[rps->num_negative_pics + i]);
            }
        }
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}


static RK_S32 decode_profile_tier_level(HEVCContext *s, PTLCommon *ptl)
{
    int i;
    HEVCLocalContext *lc = s->HEVClc;
    BitReadCtx_t *gb = &lc->gb;

    READ_BITS(gb, 2, &ptl->profile_space);
    READ_ONEBIT(gb, &ptl->tier_flag);
    READ_BITS(gb, 5, &ptl->profile_idc);

    if (ptl->profile_idc == MPP_PROFILE_HEVC_MAIN)
        h265d_dbg(H265D_DBG_GLOBAL, "Main profile bitstream\n");
    else if (ptl->profile_idc == MPP_PROFILE_HEVC_MAIN_10)
        h265d_dbg(H265D_DBG_GLOBAL, "Main 10 profile bitstream\n");
    else if (ptl->profile_idc == MPP_PROFILE_HEVC_MAIN_STILL_PICTURE)
        h265d_dbg(H265D_DBG_GLOBAL, "Main Still Picture profile bitstream\n");
    else
        mpp_log("Unknown HEVC profile: %d\n", ptl->profile_idc);

    for (i = 0; i < 32; i++)
        READ_ONEBIT(gb, & ptl->profile_compatibility_flag[i]);
    READ_ONEBIT(gb, &ptl->progressive_source_flag);
    READ_ONEBIT(gb, &ptl->interlaced_source_flag);
    READ_ONEBIT(gb, &ptl->non_packed_constraint_flag);
    READ_ONEBIT(gb, &ptl->frame_only_constraint_flag);

    SKIP_BITS(gb, 16); // XXX_reserved_zero_44bits[0..15]
    SKIP_BITS(gb, 16); // XXX_reserved_zero_44bits[16..31]
    SKIP_BITS(gb, 12); // XXX_reserved_zero_44bits[32..43]
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 parse_ptl(HEVCContext *s, PTL *ptl, int max_num_sub_layers)
{
    RK_S32 i;
    HEVCLocalContext *lc = s->HEVClc;
    BitReadCtx_t *gb = &lc->gb;
    decode_profile_tier_level(s, &ptl->general_ptl);
    READ_BITS(gb, 8, &ptl->general_ptl.level_idc);

    for (i = 0; i < max_num_sub_layers - 1; i++) {
        READ_ONEBIT(gb, &ptl->sub_layer_profile_present_flag[i]);
        READ_ONEBIT(gb, &ptl->sub_layer_level_present_flag[i]);
    }
    if (max_num_sub_layers - 1 > 0)
        for (i = max_num_sub_layers - 1; i < 8; i++)
            SKIP_BITS(gb, 2); // reserved_zero_2bits[i]
    for (i = 0; i < max_num_sub_layers - 1; i++) {
        if (ptl->sub_layer_profile_present_flag[i])
            decode_profile_tier_level(s, &ptl->sub_layer_ptl[i]);
        if (ptl->sub_layer_level_present_flag[i])
            READ_BITS(gb, 8, &ptl->sub_layer_ptl[i].level_idc);
    }

    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_sublayer_hrd(HEVCContext *s, unsigned int nb_cpb,
                                  int subpic_params_present)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_U32 i, value;

    for (i = 0; i < nb_cpb; i++) {
        READ_UE(gb, &value); // bit_rate_value_minus1
        READ_UE(gb, &value); // cpb_size_value_minus1

        if (subpic_params_present) {
            READ_UE(gb, &value); // cpb_size_du_value_minus1
            READ_UE(gb, &value); // bit_rate_du_value_minus1
        }
        SKIP_BITS(gb, 1); // cbr_flag
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_hrd(HEVCContext *s, int common_inf_present,
                         int max_sublayers)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    int nal_params_present = 0, vcl_params_present = 0;
    int subpic_params_present = 0;
    int i;

    if (common_inf_present) {
        READ_ONEBIT(gb, &nal_params_present);
        READ_ONEBIT(gb, &vcl_params_present);

        if (nal_params_present || vcl_params_present) {
            READ_ONEBIT(gb, &subpic_params_present);

            if (subpic_params_present) {
                SKIP_BITS(gb, 8); // tick_divisor_minus2
                SKIP_BITS(gb, 5); // du_cpb_removal_delay_increment_length_minus1
                SKIP_BITS(gb, 1); // sub_pic_cpb_params_in_pic_timing_sei_flag
                SKIP_BITS(gb, 5); // dpb_output_delay_du_length_minus1
            }

            SKIP_BITS(gb, 4); // bit_rate_scale
            SKIP_BITS(gb, 4); // cpb_size_scale

            if (subpic_params_present)
                SKIP_BITS(gb, 4);  // cpb_size_du_scale

            SKIP_BITS(gb, 5); // initial_cpb_removal_delay_length_minus1
            SKIP_BITS(gb, 5); // au_cpb_removal_delay_length_minus1
            SKIP_BITS(gb, 5); // dpb_output_delay_length_minus1
        }
    }

    for (i = 0; i < max_sublayers; i++) {
        RK_S32 low_delay = 0;
        RK_U32 nb_cpb = 1;
        RK_S32 fixed_rate = 0;
        RK_S32 value = 0;

        READ_ONEBIT(gb, &fixed_rate);

        if (!fixed_rate)
            READ_ONEBIT(gb, &fixed_rate);

        if (fixed_rate)
            READ_UE(gb, &value); // elemental_duration_in_tc_minus1
        else
            READ_ONEBIT(gb, &low_delay);

        if (!low_delay) {
            READ_UE(gb, &nb_cpb);
            nb_cpb = nb_cpb + 1;
        }

        if (nal_params_present)
            decode_sublayer_hrd(s, nb_cpb, subpic_params_present);
        if (vcl_params_present)
            decode_sublayer_hrd(s, nb_cpb, subpic_params_present);
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}


#ifdef VPS_EXTENSION
static RK_S32 parse_vps_extension (HEVCContext *s, HEVCVPS *vps)
{
    RK_S32 i, j;
    RK_S32 value = 0;
    BitReadCtx_t *gb = &s->HEVClc->gb;
#ifdef VPS_EXTN_MASK_AND_DIM_INFO
    RK_S32 numScalabilityTypes = 0;
    READ_ONEBIT(gb, &vps->avc_base_layer_flag);
    READ_ONEBIT(gb, &vps->splitting_flag);

    for (i = 0; i < MAX_VPS_NUM_SCALABILITY_TYPES; i++) {
        READ_ONEBIT(gb, &vps->scalability_mask[i]);
        numScalabilityTypes += vps->scalability_mask[i];
        if ( i != 1 && vps->scalability_mask[i] != 0) {
            mpp_err( "Multiview and reserved masks are not used in this version of software \n");
        }
    }

    vps->m_numScalabilityTypes = numScalabilityTypes;
    for (i = 0; i < numScalabilityTypes; i++) {
        READ_BITS(gb, 3, &vps->dimension_id_len[i]);
        vps->dimension_id_len[i] = vps->dimension_id_len[i] + 1;
    }

    if (vps->splitting_flag) {
        int numBits = 0;
        for (i = 0; i < numScalabilityTypes; i++) {
            numBits += vps->dimension_id_len[i];
        }
        if (numBits > 6)
            mpp_err( "numBits>6 \n");
    }

    READ_ONEBIT(gb, &vps->nuh_layer_id_present_flag);
    vps->layer_id_in_nuh[0] = 0;
    vps->m_layerIdInVps[0] = 0;

    for (i = 1; i < vps->vps_max_layers; i++) {
        if (vps->nuh_layer_id_present_flag ) {
            READ_BITS(gb, 6, vps->layer_id_in_nuh[i]);
        } else {
            vps->layer_id_in_nuh[i] = i;
        }
        vps->m_layerIdInVps[vps->layer_id_in_nuh[i]] = i;
        for (j = 0; j < numScalabilityTypes; j++) {
            READ_BITS(gb, vps->dimension_id_len[j], &vps->dimension_id[i][j]);
        }
    }

#endif
#ifdef VPS_MOVE_DIR_DEPENDENCY_FLAG
#ifdef VPS_EXTN_DIRECT_REF_LAYERS
    vps->m_numDirectRefLayers[0] = 0;
    for ( i = 1; i <= vps->vps_max_layers - 1; i++) {
        RK_S32 numDirectRefLayers = 0;
        for ( j = 0; j < i; j++) {
            READ_ONEBIT(gb, &vps->direct_dependency_flag[i][j]);
            if (vps->direct_dependency_flag[i][j]) {
                vps->m_refLayerId[i][numDirectRefLayers] = j;
                numDirectRefLayers++;
            }
        }

        vps->m_numDirectRefLayers[i] = numDirectRefLayers;
    }
#endif
#endif
#ifdef VPS_EXTN_PROFILE_INFO
#ifdef VPS_PROFILE_OUTPUT_LAYERS
    READ_BITS(gb, 10, value);
    if ( value != (vps->vps_num_layer_sets - 1)) { //vps_number_layer_sets_minus1
        mpp_err( "Erro vps_number_layer_sets_minus1 != vps->vps_num_layer_sets-1 \n");
    }
    READ_BITS(gb, 6, &vps->vps_num_profile_tier_level);
    vps->vps_num_profile_tier_level = vps->vps_num_profile_tier_level + 1;
    vps->PTLExt = mpp_malloc(PTL*, vps->vps_num_profile_tier_level); // TO DO add free

    for (i = 1; i <= vps->vps_num_profile_tier_level - 1; i++)
#else
    vps->PTLExt = mpp_malloc(PTL*, vps->vps_num_layer_sets); // TO DO add free
    for (i = 1; i <= vps->vps_num_layer_sets - 1; i++)
#endif
    {
        READ_ONEBIT(gb, &vps->vps_profile_present_flag[i]);
        if ( !vps->vps_profile_present_flag[i] ) {
#ifdef VPS_PROFILE_OUTPUT_LAYERS
            READ_BITS(gb, 6, &vps->profile_ref[i])

            vps->profile_ref[i] = vps->profile_ref[i] + 1;

#else
            READ_UE(gb, &vps->profile_ref[i]);
#endif
            vps->PTLExt[i] = mpp_malloc(PTL, 1); // TO DO add free
            memcpy(vps->PTLExt[i], vps->PTLExt[vps->profile_ref[i]], sizeof(PTL));
        }
        vps->PTLExt[i] = mpp_malloc(PTL, 1); // TO DO add free
        parse_ptl(s, vps->PTLExt[i], vps->vps_max_sub_layers);
    }
#endif

#ifdef VPS_PROFILE_OUTPUT_LAYERS
    READ_ONEBIT(gb, &vps->more_output_layer_sets_than_default_flag );
    RK_S32 numOutputLayerSets = 0;
    if (! vps->more_output_layer_sets_than_default_flag ) {
        numOutputLayerSets = vps->vps_num_layer_sets;
    } else {
        READ_BITS(gb, 10, &vps->num_add_output_layer_sets);
        numOutputLayerSets = vps->vps_num_layer_sets + vps->num_add_output_layer_sets;
    }
    if (numOutputLayerSets > 1) {
        READ_ONEBIT(gb, &vps->default_one_target_output_layer_flag);
    }
    vps->m_numOutputLayerSets = numOutputLayerSets;
    for (i = 1; i < numOutputLayerSets; i++) {
        if ( i > (vps->vps_num_layer_sets - 1) ) {
            RK_S32 numBits = 1, lsIdx;
            while ((1 << numBits) < (vps->vps_num_layer_sets - 1)) {
                numBits++;
            }
            READ_BITS(gb, numBits, &vps->output_layer_set_idx[i]);
            vps->output_layer_set_idx[i] = vps->output_layer_set_idx[i] + 1;
            lsIdx = vps->output_layer_set_idx[i];
            for (j = 0; j < vps->m_numLayerInIdList[lsIdx] - 1; j++) {
                READ_ONEBIT(gb, &vps->output_layer_flag[i][j]);
            }
        } else {
            int lsIdx = i;
            if ( vps->default_one_target_output_layer_flag ) {
                for (j = 0; j < vps->m_numLayerInIdList[lsIdx]; j++) {
                    vps->output_layer_flag[i][j] = (j == (vps->m_numLayerInIdList[lsIdx] - 1));
                }
            } else {
                for (j = 0; j < vps->m_numLayerInIdList[lsIdx]; j++) {
                    vps->output_layer_flag[i][j] = 1;
                }
            }
        }
        int numBits = 1;
        while ((1 << numBits) < (vps->vps_num_profile_tier_level)) {
            numBits++;
        }
        READ_BITS(gb, numBits, &vps->profile_level_tier_idx[i]);
    }
#endif
#ifdef JCTVC_M0458_INTERLAYER_RPS_SIG
    READ_ONEBIT(gb, &vps->max_one_active_ref_layer_flag);
#endif
}
#endif
static RK_S32 compare_vps(HEVCVPS *openhevc_vps, HEVCVPS *vps)
{

    if (openhevc_vps->vps_temporal_id_nesting_flag !=
        vps->vps_temporal_id_nesting_flag) {
        mpp_log("vps_temporal_id_nesting_flag diff\n");
        return -1;
    }

    if (openhevc_vps->vps_max_layers != vps->vps_max_layers) {
        mpp_log("vps_max_layers diff\n");
        return -1;
    }

    if (openhevc_vps->vps_max_sub_layers != vps->vps_max_sub_layers) {
        mpp_log("vps_max_sub_layers diff\n");
        return -1;
    }
    if (openhevc_vps->vps_sub_layer_ordering_info_present_flag !=
        vps->vps_sub_layer_ordering_info_present_flag) {
        mpp_log("vps_sub_layer_ordering_info_present_flag diff\n");
        return -1;
    }

    if (openhevc_vps->vps_max_layer_id !=  vps->vps_max_layer_id) {

        mpp_log("vps_max_layer_id diff\n");
        return -1;
    }

    if (openhevc_vps->vps_num_layer_sets != vps->vps_num_layer_sets) {

        mpp_log("vps_num_layer_sets diff\n");
        return -1;
    }
    if (openhevc_vps->vps_timing_info_present_flag !=
        vps->vps_timing_info_present_flag) {

        mpp_log("vps_timing_info_present_flag diff\n");
        return -1;
    }
    if (openhevc_vps->vps_num_units_in_tick !=
        vps->vps_num_units_in_tick) {
        mpp_log("vps_num_units_in_tick diff\n");
        return -1;
    }

    if ( openhevc_vps->vps_time_scale !=
         vps->vps_time_scale) {
        mpp_log("vps_time_scale diff\n");
        return -1;
    }
    if (openhevc_vps->vps_poc_proportional_to_timing_flag !=
        vps->vps_poc_proportional_to_timing_flag) {
        mpp_log("vps_poc_proportional_to_timing_flag \n");
        return -1;
    }
    if (openhevc_vps->vps_num_ticks_poc_diff_one !=
        vps->vps_num_ticks_poc_diff_one) {
        mpp_log("vps_poc_proportional_to_timing_flag \n");
        return -1;
    }

    if (openhevc_vps->vps_num_hrd_parameters
        != vps->vps_num_hrd_parameters) {
        mpp_log("vps_num_hrd_parameters \n");
        return -1;
    }
    return 0;
}

static RK_S32 compare_sps(HEVCSPS *openhevc_sps, HEVCSPS *sps)
{
    if (openhevc_sps->vps_id != sps->vps_id) {
        mpp_log("vps_id diff\n");
        return -1;
    }
    if (openhevc_sps->sps_id != sps->sps_id) {
        mpp_log("sps_id diff\n");
        return -1;
    }
    if (openhevc_sps->chroma_format_idc != sps->chroma_format_idc) {
        mpp_log("chroma_format_idc diff\n");
        return -1;
    }
    if (openhevc_sps->separate_colour_plane_flag
        != sps->separate_colour_plane_flag) {
        mpp_log("separate_colour_plane_flag diff\n");
        return -1;
    }

    if (openhevc_sps->output_width != sps->output_width) {
        mpp_log("output_width diff\n");
        return -1;
    }
    if (openhevc_sps->output_height != sps->output_height) {
        mpp_log("output_height diff\n");
        return -1;
    }

    if (openhevc_sps->bit_depth != sps->bit_depth) {
        mpp_log("bit_depth diff\n");
        return -1;
    }
    if (openhevc_sps->bit_depth_chroma != sps->bit_depth_chroma) {
        mpp_log("bit_depth_chroma diff\n");
        return -1;
    }
    if (openhevc_sps->pixel_shift != sps->pixel_shift) {
        mpp_log("pixel_shift diff\n");
        return -1;
    }
    // openhevc_sps->pix_fmt;

    if (openhevc_sps->log2_max_poc_lsb != sps->log2_max_poc_lsb) {
        mpp_log("log2_max_poc_lsb diff\n");
        return -1;
    }
    if (openhevc_sps->pcm_enabled_flag != sps->pcm_enabled_flag) {
        mpp_log("pcm_enabled_flag diff\n");
        return -1;
    }

    if (openhevc_sps->max_sub_layers != sps->max_sub_layers) {
        mpp_log("max_sub_layers diff\n");
        return -1;
    }

    if (openhevc_sps->scaling_list_enable_flag != sps->scaling_list_enable_flag) {
        mpp_log("scaling_list_enable_flag diff\n");
        return -1;
    }

    if (openhevc_sps->nb_st_rps != sps->nb_st_rps) {
        mpp_log("nb_st_rps diff\n");
        return -1;
    }

    if (openhevc_sps->amp_enabled_flag != sps->amp_enabled_flag) {
        mpp_log(" amp_enabled_flag diff openhevc %d != %d\n", openhevc_sps->amp_enabled_flag, sps->amp_enabled_flag);
        return -1;
    }
    if (openhevc_sps->sao_enabled != sps->sao_enabled) {
        mpp_log("sao_enabled diff\n");
        return -1;
    }

    if (openhevc_sps->long_term_ref_pics_present_flag
        != sps->long_term_ref_pics_present_flag) {
        mpp_log("long_term_ref_pics_present_flag diff\n");
        return -1;
    }
    if (openhevc_sps->num_long_term_ref_pics_sps !=
        sps->num_long_term_ref_pics_sps) {
        mpp_log("num_long_term_ref_pics_sps diff\n");
        return -1;
    }

    if (openhevc_sps->sps_temporal_mvp_enabled_flag !=
        sps->sps_temporal_mvp_enabled_flag) {
        mpp_log("sps_temporal_mvp_enabled_flag diff\n");
        return -1;
    }
    if (openhevc_sps->sps_strong_intra_smoothing_enable_flag !=
        sps->sps_strong_intra_smoothing_enable_flag) {
        mpp_log("sps_strong_intra_smoothing_enable_flag diff\n");
        return -1;
    }

    if (openhevc_sps->log2_min_cb_size != sps->log2_min_cb_size) {
        mpp_log("log2_min_cb_size diff\n");
        return -1;
    }
    if (openhevc_sps->log2_diff_max_min_coding_block_size !=
        sps->log2_diff_max_min_coding_block_size) {
        mpp_log("log2_diff_max_min_coding_block_size diff\n");
        return -1;
    }
    if (openhevc_sps->log2_min_tb_size != sps->log2_min_tb_size) {
        mpp_log("log2_min_tb_size diff\n");
        return -1;
    }
    if (openhevc_sps->log2_max_trafo_size != sps->log2_max_trafo_size) {
        mpp_log("log2_max_trafo_size diff\n");
        return -1;
    }
    if (openhevc_sps->log2_ctb_size != sps->log2_ctb_size) {
        mpp_log("log2_ctb_size diff\n");
        return -1;
    }
    if (openhevc_sps->log2_min_pu_size != sps->log2_min_pu_size) {
        mpp_log("log2_min_pu_size diff\n");
        return -1;
    }

    if (openhevc_sps->max_transform_hierarchy_depth_inter !=
        sps->max_transform_hierarchy_depth_inter) {
        mpp_log("max_transform_hierarchy_depth_inter diff\n");
        return -1;
    }
    if (openhevc_sps->max_transform_hierarchy_depth_intra !=
        sps->max_transform_hierarchy_depth_intra) {
        mpp_log("max_transform_hierarchy_depth_intra diff\n");
        return -1;
    }

    if (openhevc_sps->width != sps->width) {
        mpp_log("width diff\n");
        return -1;
    }

    if (openhevc_sps->height != sps->height) {
        mpp_log("height diff\n");
        return -1;
    }

    if (openhevc_sps->ctb_width != sps->ctb_width) {
        mpp_log("ctb_width diff\n");
        return -1;
    }

    if ( openhevc_sps->ctb_height != sps->ctb_height) {
        mpp_log("ctb_height diff\n");
        return -1;
    }

    if (openhevc_sps->ctb_size != sps->ctb_size) {
        mpp_log("ctb_size diff\n");
        return -1;
    }

    if (openhevc_sps->min_cb_width != sps->min_cb_width) {
        mpp_log("min_cb_width diff\n");
        return -1;
    }

    if (openhevc_sps->min_cb_height != sps->min_cb_height) {
        mpp_log("min_cb_height diff\n");
        return -1;
    }

    if (openhevc_sps->min_tb_width != sps->min_tb_width) {
        mpp_log("min_tb_width diff\n");
        return -1;
    }

    if (openhevc_sps->min_tb_height != sps->min_tb_height) {
        mpp_log("min_tb_height diff\n");
        return -1;
    }

    if (openhevc_sps->min_pu_width != sps->min_pu_width) {
        mpp_log("min_pu_width diff\n");
        return -1;
    }

    if (openhevc_sps->min_pu_height != sps->min_pu_height) {
        mpp_log("min_pu_height diff\n");
        return -1;
    }

    if (openhevc_sps->qp_bd_offset != sps->qp_bd_offset) {
        mpp_log("qp_bd_offset diff \n");
        return -1;
    }
    return 0;
}

static RK_S32 compare_pps(HEVCPPS *openhevc_pps, HEVCPPS *pps)
{

    if (openhevc_pps->sps_id != pps->sps_id) {
        mpp_log("sps_id diff \n");
        return -1;
    }

    if (openhevc_pps->pps_id != pps->pps_id) {
        mpp_log("pps_id diff \n");
        return -1;
    }

    if (openhevc_pps->sign_data_hiding_flag != pps->sign_data_hiding_flag) {
        mpp_log("sign_data_hiding_flag diff \n");
        return -1;
    }

    if (openhevc_pps->cabac_init_present_flag != pps->cabac_init_present_flag) {
        mpp_log("cabac_init_present_flag diff \n");
        return -1;
    }

    if (openhevc_pps->num_ref_idx_l0_default_active != pps->num_ref_idx_l0_default_active) {
        mpp_log("num_ref_idx_l0_default_active diff \n");
        return -1;
    }
    if (openhevc_pps->num_ref_idx_l1_default_active != pps->num_ref_idx_l1_default_active) {
        mpp_log("num_ref_idx_l1_default_active diff \n");
        return -1;
    }
    if (openhevc_pps->pic_init_qp_minus26 != pps->pic_init_qp_minus26) {
        mpp_log("pic_init_qp_minus26 diff \n");
        return -1;
    }

    if (openhevc_pps->constrained_intra_pred_flag != pps->constrained_intra_pred_flag) {
        mpp_log("constrained_intra_pred_flag diff \n");
        return -1;
    }
    if (openhevc_pps->transform_skip_enabled_flag != pps->transform_skip_enabled_flag) {
        mpp_log("transform_skip_enabled_flag diff \n");
        return -1;
    }

    if (openhevc_pps->cu_qp_delta_enabled_flag != pps->cu_qp_delta_enabled_flag) {
        mpp_log("cu_qp_delta_enabled_flag diff \n");
        return -1;
    }
    if (openhevc_pps->diff_cu_qp_delta_depth != pps->diff_cu_qp_delta_depth) {
        mpp_log("diff_cu_qp_delta_depth diff \n");
        return -1;
    }

    if (openhevc_pps->cb_qp_offset != pps->cb_qp_offset) {
        mpp_log("cb_qp_offset diff \n");
        return -1;
    }

    if (openhevc_pps->cr_qp_offset != pps->cr_qp_offset) {
        mpp_log("cr_qp_offset diff \n");
        return -1;
    }

    if (openhevc_pps->pic_slice_level_chroma_qp_offsets_present_flag !=
        pps->pic_slice_level_chroma_qp_offsets_present_flag) {
        mpp_log("pic_slice_level_chroma_qp_offsets_present_flag diff \n");
        return -1;
    }
    if (openhevc_pps->weighted_pred_flag != pps->weighted_pred_flag) {
        mpp_log("weighted_pred_flag diff \n");
        return -1;
    }
    if (openhevc_pps->weighted_bipred_flag != pps->weighted_bipred_flag) {
        mpp_log("weighted_bipred_flag diff \n");
        return -1;
    }
    if (openhevc_pps->output_flag_present_flag != pps->output_flag_present_flag) {
        mpp_log("output_flag_present_flag diff \n");
        return -1;
    }
    if (openhevc_pps->transquant_bypass_enable_flag !=
        pps->transquant_bypass_enable_flag) {
        mpp_log("transquant_bypass_enable_flag diff \n");
        return -1;
    }

    if (openhevc_pps->dependent_slice_segments_enabled_flag !=
        pps->dependent_slice_segments_enabled_flag) {
        mpp_log("dependent_slice_segments_enabled_flag diff \n");
        return -1;
    }
    if (openhevc_pps->tiles_enabled_flag != pps->tiles_enabled_flag) {
        mpp_log("tiles_enabled_flag diff \n");
        return -1;
    }
    if (openhevc_pps->entropy_coding_sync_enabled_flag !=
        pps->entropy_coding_sync_enabled_flag) {
        mpp_log("entropy_coding_sync_enabled_flag diff \n");
        return -1;
    }

    if (openhevc_pps->num_tile_columns != pps->num_tile_columns) {
        mpp_log("num_tile_columns diff \n");
        return -1;
    }
    if (openhevc_pps->num_tile_rows != pps->num_tile_rows) {
        mpp_log("num_tile_rows diff \n");
        return -1;
    }
    if (openhevc_pps->uniform_spacing_flag != pps->uniform_spacing_flag) {
        mpp_log("qp_bd_offset diff \n");
        return -1;
    }
    if (openhevc_pps->loop_filter_across_tiles_enabled_flag !=
        pps->loop_filter_across_tiles_enabled_flag) {
        mpp_log("loop_filter_across_tiles_enabled_flag diff \n");
        return -1;
    }

    if (openhevc_pps->seq_loop_filter_across_slices_enabled_flag !=
        pps->seq_loop_filter_across_slices_enabled_flag) {
        mpp_log("seq_loop_filter_across_slices_enabled_flag diff \n");
        return -1;
    }

    if (openhevc_pps->deblocking_filter_control_present_flag !=
        pps->deblocking_filter_control_present_flag) {
        mpp_log("deblocking_filter_control_present_flag diff \n");
        return -1;
    }
    if (openhevc_pps->deblocking_filter_override_enabled_flag !=
        pps->deblocking_filter_override_enabled_flag) {
        mpp_log("deblocking_filter_override_enabled_flag diff \n");
        return -1;
    }
    if (openhevc_pps->disable_dbf != pps->disable_dbf) {
        mpp_log("disable_dbf diff \n");
        return -1;
    }
    if (openhevc_pps->beta_offset != pps->beta_offset) {
        mpp_log("beta_offset diff \n");
        return -1;
    }
    if (openhevc_pps->tc_offset != pps->tc_offset) {
        mpp_log("tc_offset diff \n");
        return -1;
    }

    if (openhevc_pps->scaling_list_data_present_flag !=
        pps->scaling_list_data_present_flag) {
        mpp_log("scaling_list_data_present_flag diff \n");
        return -1;
    }

    if (openhevc_pps->lists_modification_present_flag !=
        pps->lists_modification_present_flag) {
        mpp_log("lists_modification_present_flag diff \n");
        return -1;
    }
    if (openhevc_pps->log2_parallel_merge_level != pps->log2_parallel_merge_level) {
        mpp_log("log2_parallel_merge_level diff \n");
        return -1;
    }
    if (openhevc_pps->num_extra_slice_header_bits !=
        pps->num_extra_slice_header_bits) {
        mpp_log("num_extra_slice_header_bits diff \n");
        return -1;
    }
#if 0
    if (openhevc_pps->slice_header_extension_present_flag !=
        pps->slice_header_extension_present_flag) {
        mpp_log("slice_header_extension_present_flag diff \n");
        return -1;
    }

    if (openhevc_pps->pps_extension_flag != pps->pps_extension_flag) {
        mpp_log("pps_extension_flag diff \n");
        return -1;
    }
    if (openhevc_pps->pps_extension_data_flag != pps->pps_extension_data_flag) {
        mpp_log("pps_extension_data_flag diff \n");
        return -1;
    }
#endif
    return 0;

}



int mpp_hevc_decode_nal_vps(HEVCContext *s)
{
    RK_S32 i, j;
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_S32 vps_id = 0;
    HEVCVPS *vps = NULL;
    RK_U8 *vps_buf = mpp_calloc(RK_U8, sizeof(HEVCVPS));
    RK_S32 value = 0;

    if (!vps_buf)
        return MPP_ERR_NOMEM;
    vps = (HEVCVPS*)vps_buf;

    h265d_dbg(H265D_DBG_FUNCTION, "Decoding VPS\n");

    READ_BITS(gb, 4, &vps_id);

    h265d_dbg(H265D_DBG_VPS, "vps_id = 0x%x", vps_id);

    if (vps_id >= MAX_VPS_COUNT) {
        mpp_err( "VPS id out of range: %d\n", vps_id);
        goto err;
    }
    READ_BITS(gb, 2, &value);
    if (value != 3) { // vps_reserved_three_2bits
        mpp_err( "vps_reserved_three_2bits is not three\n");
        goto err;
    }

    READ_BITS(gb, 6, &vps->vps_max_layers);
    vps->vps_max_layers = vps->vps_max_layers + 1;

    READ_BITS(gb, 3, &vps->vps_max_sub_layers);
    vps->vps_max_sub_layers =  vps->vps_max_sub_layers + 1;

    READ_ONEBIT(gb, & vps->vps_temporal_id_nesting_flag);
    READ_BITS(gb, 16, &value);

    if (value != 0xffff) { // vps_reserved_ffff_16bits
        mpp_err( "vps_reserved_ffff_16bits is not 0xffff\n");
        goto err;
    }

    if (vps->vps_max_sub_layers > MAX_SUB_LAYERS) {
        mpp_err( "vps_max_sub_layers out of range: %d\n",
                 vps->vps_max_sub_layers);
        goto err;
    }

    parse_ptl(s, &vps->ptl, vps->vps_max_sub_layers);

    READ_ONEBIT(gb, &vps->vps_sub_layer_ordering_info_present_flag);

    i = vps->vps_sub_layer_ordering_info_present_flag ? 0 : vps->vps_max_sub_layers - 1;
    for (; i < vps->vps_max_sub_layers; i++) {
        READ_UE(gb, &vps->vps_max_dec_pic_buffering[i]);
        vps->vps_max_dec_pic_buffering[i] = vps->vps_max_dec_pic_buffering[i] + 1;
        READ_UE(gb, &vps->vps_num_reorder_pics[i]);
        READ_UE(gb, &vps->vps_max_latency_increase[i]);
        vps->vps_max_latency_increase[i]  = vps->vps_max_latency_increase[i]  - 1;

        if (vps->vps_max_dec_pic_buffering[i] > MAX_DPB_SIZE) {
            mpp_err( "vps_max_dec_pic_buffering_minus1 out of range: %d\n",
                     vps->vps_max_dec_pic_buffering[i] - 1);
            goto err;
        }
        if (vps->vps_num_reorder_pics[i] > vps->vps_max_dec_pic_buffering[i] - 1) {
            mpp_err( "vps_max_num_reorder_pics out of range: %d\n",
                     vps->vps_num_reorder_pics[i]);
            goto err;
        }
    }

    READ_BITS(gb, 6, &vps->vps_max_layer_id);
    READ_UE(gb, &vps->vps_num_layer_sets);
    vps->vps_num_layer_sets += 1;
    for (i = 1; i < vps->vps_num_layer_sets; i++)
        for (j = 0; j <= vps->vps_max_layer_id; j++)
            SKIP_BITS(gb, 1);  // layer_id_included_flag[i][j]

    READ_ONEBIT(gb, &vps->vps_timing_info_present_flag);
    if (vps->vps_timing_info_present_flag) {
        mpp_read_longbits(gb, 32, &vps->vps_num_units_in_tick);
        mpp_read_longbits(gb, 32, &vps->vps_time_scale);
        READ_ONEBIT(gb, &vps->vps_poc_proportional_to_timing_flag);
        if (vps->vps_poc_proportional_to_timing_flag) {
            READ_UE(gb, &vps->vps_num_ticks_poc_diff_one);
            vps->vps_num_ticks_poc_diff_one +=  1;
        }
        READ_UE(gb, &vps->vps_num_hrd_parameters);
        for (i = 0; i < vps->vps_num_hrd_parameters; i++) {
            RK_S32 common_inf_present = 1;
            RK_S32 hrd_layer_set_idx = 0;

            READ_UE(gb, &hrd_layer_set_idx); // hrd_layer_set_idx
            if (i)
                READ_ONEBIT(gb, &common_inf_present);
            decode_hrd(s, common_inf_present, vps->vps_max_sub_layers);
        }
    }

    //  READ_ONEBIT(gb, &vps->vps_extension_flag);

#ifdef VPS_EXTENSION
    if (vps->vps_extension_flag) { // vps_extension_flag
        parse_vps_extension(s, vps);
    }
#endif
    if (s->h265dctx->compare_info != NULL) {
        CurrentFameInf_t *info = (CurrentFameInf_t *)s->h265dctx->compare_info;
        HEVCVPS *openhevc_vps = (HEVCVPS*)&info->vps[vps_id];
        mpp_log("compare_vps in \n");
        if (compare_vps(openhevc_vps, (HEVCVPS*)vps_buf) < 0) {
            mpp_err("compare_vps return error \n");
            mpp_assert(0);
            return -1;
        }
        mpp_log("compare_vps ok \n");
    }

    if (s->vps_list[vps_id] &&
        !memcmp(s->vps_list[vps_id], vps_buf, sizeof(HEVCVPS))) {
        mpp_free(vps_buf);
    } else {
        if (s->vps_list[vps_id] != NULL) {
            mpp_free(s->vps_list[vps_id]);
        }
        s->vps_list[vps_id] = vps_buf;
    }

    return 0;
__BITREAD_ERR:
err:
    mpp_free(vps_buf);
    return  MPP_ERR_STREAM;
}


static RK_S32 decode_vui(HEVCContext *s, HEVCSPS *sps)
{
    VUI *vui          = &sps->vui;
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_S32 sar_present;

    h265d_dbg(H265D_DBG_FUNCTION, "Decoding VUI\n");

    READ_ONEBIT(gb, &sar_present);
    if (sar_present) {
        RK_U8 sar_idx = 0;
        READ_BITS(gb, 8, &sar_idx);
        if (sar_idx < MPP_ARRAY_ELEMS(vui_sar))
            vui->sar = vui_sar[sar_idx];
        else if (sar_idx == 255) {
            READ_BITS(gb, 16, &vui->sar.num);
            READ_BITS(gb, 16, &vui->sar.den);
        } else
            mpp_log("Unknown SAR index: %u.\n", sar_idx);
    }

    READ_ONEBIT(gb, &vui->overscan_info_present_flag);
    if (vui->overscan_info_present_flag)
        READ_ONEBIT(gb, &vui->overscan_appropriate_flag);

    READ_ONEBIT(gb, &vui->video_signal_type_present_flag);
    if (vui->video_signal_type_present_flag) {
        READ_BITS(gb, 3, & vui->video_format);
        READ_ONEBIT(gb, &vui->video_full_range_flag);
        READ_ONEBIT(gb, &vui->colour_description_present_flag);

        if (vui->colour_description_present_flag) {
            READ_BITS(gb, 8, &vui->colour_primaries);
            READ_BITS(gb, 8, &vui->transfer_characteristic);
            READ_BITS(gb, 8, &vui->matrix_coeffs);

            // Set invalid values to "unspecified"
            //  if (vui->colour_primaries >= RKCOL_PRI_NB)
            //      vui->colour_primaries = RKCOL_PRI_UNSPECIFIED;
            //  if (vui->transfer_characteristic >= RKCOL_TRC_NB)
            //      vui->transfer_characteristic = RKCOL_TRC_UNSPECIFIED;
            if (vui->matrix_coeffs >= MPPCOL_SPC_NB)
                vui->matrix_coeffs = MPPCOL_SPC_UNSPECIFIED;
        }
    }

    READ_ONEBIT(gb, &vui->chroma_loc_info_present_flag );
    if (vui->chroma_loc_info_present_flag) {
        READ_UE(gb, &vui->chroma_sample_loc_type_top_field);
        READ_UE(gb, &vui->chroma_sample_loc_type_bottom_field);
    }

    READ_ONEBIT(gb, &vui->neutra_chroma_indication_flag);
    READ_ONEBIT(gb, &vui->field_seq_flag);
    READ_ONEBIT(gb, &vui->frame_field_info_present_flag);

    READ_ONEBIT(gb, &vui->default_display_window_flag);
    if (vui->default_display_window_flag) {
        //TODO: * 2 is only valid for 420
        READ_UE(gb, &vui->def_disp_win.left_offset);
        vui->def_disp_win.left_offset  =  vui->def_disp_win.left_offset * 2;

        READ_UE(gb, &vui->def_disp_win.right_offset);
        vui->def_disp_win.right_offset  =  vui->def_disp_win.right_offset * 2;

        READ_UE(gb, &vui->def_disp_win.right_offset);
        vui->def_disp_win.top_offset = vui->def_disp_win.top_offset * 2;

        READ_UE(gb, &vui->def_disp_win.right_offset);
        vui->def_disp_win.bottom_offset = vui->def_disp_win.bottom_offset * 2;

#if 0
        if (s->apply_defdispwin &&
            s->h265dctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {
            h265d_dbg(H265D_DBG_SPS,
                      "discarding vui default display window, "
                      "original values are l:%u r:%u t:%u b:%u\n",
                      vui->def_disp_win.left_offset,
                      vui->def_disp_win.right_offset,
                      vui->def_disp_win.top_offset,
                      vui->def_disp_win.bottom_offset);

            vui->def_disp_win.left_offset   =
                vui->def_disp_win.right_offset  =
                    vui->def_disp_win.top_offset    =
                        vui->def_disp_win.bottom_offset = 0;
        }
#endif
    }

    READ_ONEBIT(gb, &vui->vui_timing_info_present_flag);

    if (vui->vui_timing_info_present_flag) {
        mpp_read_longbits(gb, 32, &vui->vui_num_units_in_tick);
        mpp_read_longbits(gb, 32, &vui->vui_time_scale);
        READ_ONEBIT(gb, &vui->vui_poc_proportional_to_timing_flag);
        if (vui->vui_poc_proportional_to_timing_flag)
            READ_UE(gb, &vui->vui_num_ticks_poc_diff_one_minus1);
        READ_ONEBIT(gb, &vui->vui_hrd_parameters_present_flag);
        if (vui->vui_hrd_parameters_present_flag)
            decode_hrd(s, 1, sps->max_sub_layers);
    }

    READ_ONEBIT(gb, &vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        READ_ONEBIT(gb, &vui->tiles_fixed_structure_flag);
        READ_ONEBIT(gb, &vui->motion_vectors_over_pic_boundaries_flag);
        READ_ONEBIT(gb, &vui->restricted_ref_pic_lists_flag);
        READ_UE(gb, &vui->min_spatial_segmentation_idc);
        READ_UE(gb, &vui->max_bytes_per_pic_denom);
        READ_UE(gb, &vui->max_bits_per_min_cu_denom);
        READ_UE(gb, &vui->log2_max_mv_length_horizontal);
        READ_UE(gb, &vui->log2_max_mv_length_vertical);
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static void set_default_scaling_list_data(ScalingList *sl)
{
    int matrixId;

    for (matrixId = 0; matrixId < 6; matrixId++) {
        // 4x4 default is 16
        memset(sl->sl[0][matrixId], 16, 16);
        sl->sl_dc[0][matrixId] = 16; // default for 16x16
        sl->sl_dc[1][matrixId] = 16; // default for 32x32
    }
    memcpy(sl->sl[1][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[1][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[1][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[2][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[2][5], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][0], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][1], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][2], default_scaling_list_intra, 64);
    memcpy(sl->sl[3][3], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][4], default_scaling_list_inter, 64);
    memcpy(sl->sl[3][5], default_scaling_list_inter, 64);
}

static int scaling_list_data(HEVCContext *s, ScalingList *sl, HEVCSPS *sps)
{
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_U8 scaling_list_pred_mode_flag;
    RK_S32 scaling_list_dc_coef[2][6];
    RK_S32 size_id,  i, pos;
    RK_U32 matrix_id;

    for (size_id = 0; size_id < 4; size_id++)
        for (matrix_id = 0; matrix_id < 6; matrix_id += ((size_id == 3) ? 3 : 1)) {
            READ_ONEBIT(gb, &scaling_list_pred_mode_flag);
            if (!scaling_list_pred_mode_flag) {
                RK_U32 delta = 0;
                READ_UE(gb, &delta);
                /* Only need to handle non-zero delta. Zero means default,
                 * which should already be in the arrays. */
                if (delta) {
                    // Copy from previous array.
                    if (matrix_id < delta) {
                        mpp_err(
                            "Invalid delta in scaling list data: %d.\n", delta);
                        return  MPP_ERR_STREAM;
                    }

                    memcpy(sl->sl[size_id][matrix_id],
                           sl->sl[size_id][matrix_id - delta],
                           size_id > 0 ? 64 : 16);
                    if (size_id > 1)
                        sl->sl_dc[size_id - 2][matrix_id] = sl->sl_dc[size_id - 2][matrix_id - delta];
                }
            } else {
                RK_S32 next_coef, coef_num;
                RK_S32 scaling_list_delta_coef;

                next_coef = 8;
                coef_num  = MPP_MIN(64, 1 << (4 + (size_id << 1)));
                if (size_id > 1) {
                    READ_SE(gb, &scaling_list_dc_coef[size_id - 2][matrix_id]);
                    scaling_list_dc_coef[size_id - 2][matrix_id] =  scaling_list_dc_coef[size_id - 2][matrix_id] + 8;
                    next_coef = scaling_list_dc_coef[size_id - 2][matrix_id];
                    sl->sl_dc[size_id - 2][matrix_id] = next_coef;
                }
                for (i = 0; i < coef_num; i++) {
                    if (size_id == 0)
                        pos = 4 * mpp_hevc_diag_scan4x4_y[i] +
                              mpp_hevc_diag_scan4x4_x[i];
                    else
                        pos = 8 * mpp_hevc_diag_scan8x8_y[i] +
                              mpp_hevc_diag_scan8x8_x[i];

                    READ_SE(gb, &scaling_list_delta_coef);
                    next_coef = (next_coef + scaling_list_delta_coef + 256) % 256;
                    sl->sl[size_id][matrix_id][pos] = next_coef;
                }
            }
        }
    if (sps->chroma_format_idc == 3) {
        for (i = 0; i < 64; i++) {
            sl->sl[3][1][i] = sl->sl[2][1][i];
            sl->sl[3][2][i] = sl->sl[2][2][i];
            sl->sl[3][4][i] = sl->sl[2][4][i];
            sl->sl[3][5][i] = sl->sl[2][5][i];
        }
        sl->sl_dc[1][1] = sl->sl_dc[0][1];
        sl->sl_dc[1][2] = sl->sl_dc[0][2];
        sl->sl_dc[1][4] = sl->sl_dc[0][4];
        sl->sl_dc[1][5] = sl->sl_dc[0][5];
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

RK_S32 mpp_hevc_decode_nal_sps(HEVCContext *s)
{
    // const AVPixFmtDescriptor *desc;
    BitReadCtx_t *gb = &s->HEVClc->gb;
    RK_S32 ret    = 0;
    RK_S32 sps_id = 0;
    RK_S32 log2_diff_max_min_transform_block_size;
    RK_S32 bit_depth_chroma, start, vui_present, sublayer_ordering_info;
    RK_S32 i;
    RK_S32 value = 0;

    HEVCSPS *sps;
    RK_U8 *sps_buf = mpp_calloc(RK_U8, sizeof(*sps));

    if (!sps_buf)
        return MPP_ERR_NOMEM;
    sps = (HEVCSPS*)sps_buf;

    h265d_dbg(H265D_DBG_FUNCTION, "Decoding SPS\n");

    // Coded parameters

    READ_BITS(gb, 4, &sps->vps_id);
    if (sps->vps_id >= MAX_VPS_COUNT) {
        mpp_err( "VPS id out of range: %d\n", sps->vps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    if (!s->vps_list[sps->vps_id]) {
        mpp_err( "VPS %d does not exist\n",
                 sps->vps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    READ_BITS(gb, 3, &sps->max_sub_layers);
    sps->max_sub_layers += 1;

    if (sps->max_sub_layers > MAX_SUB_LAYERS) {
        mpp_err( "sps_max_sub_layers out of range: %d\n",
                 sps->max_sub_layers);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    SKIP_BITS(gb, 1); // temporal_id_nesting_flag

    parse_ptl(s, &sps->ptl, sps->max_sub_layers);

    READ_UE(gb, &sps_id);
    sps->sps_id = sps_id;///<- zrh add
    if (sps_id >= MAX_SPS_COUNT) {
        mpp_err( "SPS id out of range: %d\n", sps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    READ_UE(gb, &sps->chroma_format_idc);
    if (sps->chroma_format_idc != 1) {
        mpp_err("chroma_format_idc != 1\n");
        ret =  MPP_ERR_PROTOL;
        goto err;
    }

    if (sps->chroma_format_idc == 3)
        READ_ONEBIT(gb, &sps->separate_colour_plane_flag);

    READ_UE(gb, &sps->width);
    READ_UE(gb, &sps->height);

    READ_ONEBIT(gb, &value);

    if (value) { // pic_conformance_flag
        //TODO: * 2 is only valid for 420
        READ_UE(gb, &sps->pic_conf_win.left_offset);
        sps->pic_conf_win.left_offset   = sps->pic_conf_win.left_offset * 2;
        READ_UE(gb, &sps->pic_conf_win.right_offset);
        sps->pic_conf_win.right_offset  = sps->pic_conf_win.right_offset * 2;
        READ_UE(gb, &sps->pic_conf_win.top_offset);
        sps->pic_conf_win.top_offset    =  sps->pic_conf_win.top_offset * 2;
        READ_UE(gb, &sps->pic_conf_win.bottom_offset);
        sps->pic_conf_win.bottom_offset = sps->pic_conf_win.bottom_offset * 2;
#if 0
        if (s->h265dctx->flags2 & CODEC_FLAG2_IGNORE_CROP) {
            h265d_dbg(H265D_DBG_SPS,
                      "discarding sps conformance window, "
                      "original values are l:%u r:%u t:%u b:%u\n",
                      sps->pic_conf_win.left_offset,
                      sps->pic_conf_win.right_offset,
                      sps->pic_conf_win.top_offset,
                      sps->pic_conf_win.bottom_offset);

            sps->pic_conf_win.left_offset   =
                sps->pic_conf_win.right_offset  =
                    sps->pic_conf_win.top_offset    =
                        sps->pic_conf_win.bottom_offset = 0;
        }

#endif
        sps->output_window = sps->pic_conf_win;
    }

    READ_UE(gb, &sps->bit_depth);

    sps->bit_depth   =  sps->bit_depth + 8;
    READ_UE(gb, &bit_depth_chroma);
    bit_depth_chroma = bit_depth_chroma + 8;
    sps->bit_depth_chroma = bit_depth_chroma;///<- zrh add
    if (bit_depth_chroma != sps->bit_depth) {
        mpp_err(
            "Luma bit depth (%d) is different from chroma bit depth (%d), "
            "this is unsupported.\n",
            sps->bit_depth, bit_depth_chroma);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    if (sps->chroma_format_idc == 1) {
        switch (sps->bit_depth) {
        case 8:  sps->pix_fmt = MPP_FMT_YUV420SP;   break;
        case 10: sps->pix_fmt = MPP_FMT_YUV420SP_10BIT; break;
        default:
            mpp_err( "Unsupported bit depth: %d\n",
                     sps->bit_depth);
            ret =  MPP_ERR_PROTOL;
            goto err;
        }
    } else {
        mpp_err(
            "non-4:2:0 support is currently unspecified.\n");
        return  MPP_ERR_PROTOL;
    }
#if 0
    desc = av_pix_fmt_desc_get(sps->pix_fmt);
    if (!desc) {
        goto err;
    }

    sps->hshift[0] = sps->vshift[0] = 0;
    sps->hshift[2] = sps->hshift[1] = desc->log2_chroma_w;
    sps->vshift[2] = sps->vshift[1] = desc->log2_chroma_h;
#endif
    sps->pixel_shift = sps->bit_depth > 8;

    READ_UE(gb, &sps->log2_max_poc_lsb);
    sps->log2_max_poc_lsb += 4;
    if (sps->log2_max_poc_lsb > 16) {
        mpp_err( "log2_max_pic_order_cnt_lsb_minus4 out range: %d\n",
                 sps->log2_max_poc_lsb - 4);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    READ_ONEBIT(gb, &sublayer_ordering_info);
    h265d_dbg(H265D_DBG_SPS, "read bit left %d", gb->num_remaining_bits_in_curr_byte_ + gb->bytes_left_ * 8);
    start = sublayer_ordering_info ? 0 : sps->max_sub_layers - 1;
    for (i = start; i < sps->max_sub_layers; i++) {
        READ_UE(gb, &sps->temporal_layer[i].max_dec_pic_buffering) ;
        sps->temporal_layer[i].max_dec_pic_buffering += 1;
        READ_UE(gb, &sps->temporal_layer[i].num_reorder_pics);
        READ_UE(gb, &sps->temporal_layer[i].max_latency_increase );
        sps->temporal_layer[i].max_latency_increase  -= 1;
        if (sps->temporal_layer[i].max_dec_pic_buffering > MAX_DPB_SIZE) {
            mpp_err( "sps_max_dec_pic_buffering_minus1 out of range: %d\n",
                     sps->temporal_layer[i].max_dec_pic_buffering - 1);
            ret =  MPP_ERR_STREAM;
            goto err;
        }
        if (sps->temporal_layer[i].num_reorder_pics > sps->temporal_layer[i].max_dec_pic_buffering - 1) {
            mpp_err( "sps_max_num_reorder_pics out of range: %d\n",
                     sps->temporal_layer[i].num_reorder_pics);
            if (sps->temporal_layer[i].num_reorder_pics > MAX_DPB_SIZE - 1) {
                ret =  MPP_ERR_STREAM;
                goto err;
            }
            sps->temporal_layer[i].max_dec_pic_buffering = sps->temporal_layer[i].num_reorder_pics + 1;
        }
    }

    if (!sublayer_ordering_info) {
        for (i = 0; i < start; i++) {
            sps->temporal_layer[i].max_dec_pic_buffering = sps->temporal_layer[start].max_dec_pic_buffering;
            sps->temporal_layer[i].num_reorder_pics      = sps->temporal_layer[start].num_reorder_pics;
            sps->temporal_layer[i].max_latency_increase  = sps->temporal_layer[start].max_latency_increase;
        }
    }

    h265d_dbg(H265D_DBG_SPS, "2 read bit left %d", gb->num_remaining_bits_in_curr_byte_ + gb->bytes_left_ * 8);
    READ_UE(gb, &sps->log2_min_cb_size) ;
    sps->log2_min_cb_size += 3;

    h265d_dbg(H265D_DBG_SPS, "sps->log2_min_cb_size %d", sps->log2_min_cb_size);
    READ_UE(gb, &sps->log2_diff_max_min_coding_block_size);

    h265d_dbg(H265D_DBG_SPS, "sps->log2_diff_max_min_coding_block_size %d", sps->log2_diff_max_min_coding_block_size);
    READ_UE(gb, &sps->log2_min_tb_size);
    sps->log2_min_tb_size += 2;

    h265d_dbg(H265D_DBG_SPS, "sps->log2_min_tb_size %d", sps->log2_min_tb_size);
    READ_UE(gb, &log2_diff_max_min_transform_block_size);

    h265d_dbg(H265D_DBG_SPS, "sps->log2_diff_max_min_transform_block_size %d", log2_diff_max_min_transform_block_size);
    sps->log2_max_trafo_size                 = log2_diff_max_min_transform_block_size +
                                               sps->log2_min_tb_size;

    h265d_dbg(H265D_DBG_SPS, "sps->log2_max_trafo_size %d", sps->log2_max_trafo_size);

    if (sps->log2_min_tb_size >= sps->log2_min_cb_size) {
        mpp_err( "Invalid value for log2_min_tb_size");
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    READ_UE(gb, &sps->max_transform_hierarchy_depth_inter);
    READ_UE(gb, &sps->max_transform_hierarchy_depth_intra);


    READ_ONEBIT(gb, &sps->scaling_list_enable_flag );

    if (sps->scaling_list_enable_flag) {
        value = 0;
        set_default_scaling_list_data(&sps->scaling_list);
        READ_ONEBIT(gb, &value);
        if (value) {
            ret = scaling_list_data(s, &sps->scaling_list, sps);
            if (ret < 0)
                goto err;
        }

        s->scaling_list_listen[sps_id] = 1;
    }

    READ_ONEBIT(gb, &sps->amp_enabled_flag);
    READ_ONEBIT(gb, &sps->sao_enabled);
    READ_ONEBIT(gb, &sps->pcm_enabled_flag);

    h265d_dbg(H265D_DBG_SPS, "sps->amp_enabled_flag = %d", sps->amp_enabled_flag);
    h265d_dbg(H265D_DBG_SPS, "sps->sao_enabled = %d", sps->sao_enabled);
    h265d_dbg(H265D_DBG_SPS, "sps->pcm_enabled_flag = %d", sps->pcm_enabled_flag);

    if (sps->pcm_enabled_flag) {
        READ_BITS(gb, 4, &sps->pcm.bit_depth);
        sps->pcm.bit_depth +=  1;
        READ_BITS(gb, 4, &sps->pcm.bit_depth_chroma);
        sps->pcm.bit_depth_chroma += 1;
        READ_UE(gb, &sps->pcm.log2_min_pcm_cb_size );
        sps->pcm.log2_min_pcm_cb_size += 3;
        READ_UE(gb, &value);
        sps->pcm.log2_max_pcm_cb_size = sps->pcm.log2_min_pcm_cb_size + value;

        if (sps->pcm.bit_depth > sps->bit_depth) {
            mpp_err(
                "PCM bit depth (%d) is greater than normal bit depth (%d)\n",
                sps->pcm.bit_depth, sps->bit_depth);
            ret =  MPP_ERR_STREAM;
            goto err;
        }
        READ_ONEBIT(gb, &sps->pcm.loop_filter_disable_flag);
    }

    READ_UE(gb, &sps->nb_st_rps);
    if (sps->nb_st_rps > MAX_SHORT_TERM_RPS_COUNT) {
        mpp_err( "Too many short term RPS: %d.\n",
                 sps->nb_st_rps);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    for (i = 0; (RK_U32)i < sps->nb_st_rps; i++) {
        if ((ret = mpp_hevc_decode_short_term_rps(s, &sps->st_rps[i],
                                                  sps, 0)) < 0)
            goto err;
    }

    READ_ONEBIT(gb, &sps->long_term_ref_pics_present_flag);
    if (sps->long_term_ref_pics_present_flag) {
        READ_UE(gb, &sps->num_long_term_ref_pics_sps);
        for (i = 0; (RK_U8)i < sps->num_long_term_ref_pics_sps; i++) {
            READ_BITS(gb, sps->log2_max_poc_lsb, &sps->lt_ref_pic_poc_lsb_sps[i]);
            READ_ONEBIT(gb, &sps->used_by_curr_pic_lt_sps_flag[i]);
        }
    }

    READ_ONEBIT(gb, &sps->sps_temporal_mvp_enabled_flag);

#ifdef REF_IDX_MFM
    if (s->nuh_layer_id > 0)
        READ_ONEBIT(gb, &sps->set_mfm_enabled_flag);
#endif
    READ_ONEBIT(gb, &sps->sps_strong_intra_smoothing_enable_flag);

    sps->vui.sar.num = 0;
    sps->vui.sar.den = 1;
    READ_ONEBIT(gb, &vui_present);
    if (vui_present)
        decode_vui(s, sps);
#ifdef SCALED_REF_LAYER_OFFSETS
    if ( s->nuh_layer_id > 0 )   {
        READ_SE(gb, &value);
        sps->scaled_ref_layer_window.left_offset = (value << 1);
        READ_SE(gb, &value);
        sps->scaled_ref_layer_window.top_offset = (value << 1);
        READ_SE(gb, &value);
        sps->scaled_ref_layer_window.right_offset = (value << 1);
        READ_SE(gb, &value);
        sps->scaled_ref_layer_window.bottom_offset = (value << 1);
    }
#endif

    //  SKIP_BITS(gb, 1); // sps_extension_flag

    if (s->apply_defdispwin) {
        sps->output_window.left_offset   += sps->vui.def_disp_win.left_offset;
        sps->output_window.right_offset  += sps->vui.def_disp_win.right_offset;
        sps->output_window.top_offset    += sps->vui.def_disp_win.top_offset;
        sps->output_window.bottom_offset += sps->vui.def_disp_win.bottom_offset;
    }
#if 1
    if (sps->output_window.left_offset & (0x1F >> (sps->pixel_shift))) {
        sps->output_window.left_offset &= ~(0x1F >> (sps->pixel_shift));
        mpp_log("Reducing left output window to %d "
                "chroma samples to preserve alignment.\n",
                sps->output_window.left_offset);
    }
#endif
    sps->output_width  = sps->width -
                         (sps->output_window.left_offset + sps->output_window.right_offset);
    sps->output_height = sps->height -
                         (sps->output_window.top_offset + sps->output_window.bottom_offset);
    if (sps->output_width <= 0 || sps->output_height <= 0) {
        mpp_log("Invalid visible frame dimensions: %dx%d.\n",
                sps->output_width, sps->output_height);
#if 0
        if (s->h265dctx->err_recognition & AV_EF_EXPLODE) {
            ret =  MPP_ERR_STREAM;
            goto err;
        }
#endif
        mpp_log("Displaying the whole video surface.\n");
        sps->pic_conf_win.left_offset   = 0;
        sps->pic_conf_win.right_offset  = 0;
        sps->pic_conf_win.top_offset    = 0;
        sps->pic_conf_win.bottom_offset = 0;
        sps->output_width               = sps->width;
        sps->output_height              = sps->height;
    }

    // NOTE: only do this for the first time of parsing sps
    //       this is for extra data sps/pps parser
    if (s->h265dctx->width == 0 && s->h265dctx->height == 0) {
        s->h265dctx->width = sps->output_width;
        s->h265dctx->height = sps->output_height;
    }

    // Inferred parameters
    sps->log2_ctb_size = sps->log2_min_cb_size + sps->log2_diff_max_min_coding_block_size;

    h265d_dbg(H265D_DBG_SPS, "sps->log2_min_cb_size = %d sps->log2_diff_max_min_coding_block_size = %d", sps->log2_min_cb_size, sps->log2_diff_max_min_coding_block_size);

    h265d_dbg(H265D_DBG_SPS, "plus sps->log2_ctb_size %d", sps->log2_ctb_size);
    sps->log2_min_pu_size = sps->log2_min_cb_size - 1;

    sps->ctb_width  = (sps->width  + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    sps->ctb_height = (sps->height + (1 << sps->log2_ctb_size) - 1) >> sps->log2_ctb_size;
    sps->ctb_size   = sps->ctb_width * sps->ctb_height;

    sps->min_cb_width  = sps->width  >> sps->log2_min_cb_size;
    sps->min_cb_height = sps->height >> sps->log2_min_cb_size;
    sps->min_tb_width  = sps->width  >> sps->log2_min_tb_size;
    sps->min_tb_height = sps->height >> sps->log2_min_tb_size;
    sps->min_pu_width  = sps->width  >> sps->log2_min_pu_size;
    sps->min_pu_height = sps->height >> sps->log2_min_pu_size;

    sps->qp_bd_offset = 6 * (sps->bit_depth - 8);

    if (sps->width  & ((1 << sps->log2_min_cb_size) - 1) ||
        sps->height & ((1 << sps->log2_min_cb_size) - 1)) {
        mpp_err( "Invalid coded frame dimensions.\n");
        goto err;
    }

    if (sps->log2_ctb_size > MAX_LOG2_CTB_SIZE) {
        mpp_err( "CTB size out of range: 2^%d\n", sps->log2_ctb_size);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_inter > (RK_S32)(sps->log2_ctb_size - sps->log2_min_tb_size)) {
        mpp_err( "max_transform_hierarchy_depth_inter out of range: %d\n",
                 sps->max_transform_hierarchy_depth_inter);
        goto err;
    }
    if (sps->max_transform_hierarchy_depth_intra > (RK_S32)(sps->log2_ctb_size - sps->log2_min_tb_size)) {
        mpp_err( "max_transform_hierarchy_depth_intra out of range: %d\n",
                 sps->max_transform_hierarchy_depth_intra);
        goto err;
    }
    h265d_dbg(H265D_DBG_SPS, "sps->log2_ctb_size %d", sps->log2_ctb_size);
    if (sps->log2_max_trafo_size > (RK_U32)MPP_MIN(sps->log2_ctb_size, 5)) {
        mpp_err(
            "max transform block size out of range: %d\n",
            sps->log2_max_trafo_size);
        goto err;
    }
    if (s->h265dctx->compare_info != NULL) {
        CurrentFameInf_t *info = (CurrentFameInf_t *)s->h265dctx->compare_info;
        HEVCSPS *openhevc_sps = (HEVCSPS *)&info->sps[sps_id];
        mpp_log("compare sps in");
        if (compare_sps(openhevc_sps, (HEVCSPS *)sps_buf) < 0) {
            mpp_err("compare sps with openhevc error found");
            mpp_assert(0);
            return -1;
        }
        mpp_log("compare sps ok");
    }
#if 0
    if (s->h265dctx->debug & FF_DEBUG_BITSTREAM) {
        h265d_dbg(H265D_DBG_SPS,
                  "Parsed SPS: id %d; coded wxh: %dx%d; "
                  "cropped wxh: %dx%d; pix_fmt: %s.\n",
                  sps_id, sps->width, sps->height,
                  sps->output_width, sps->output_height,
                  av_get_pix_fmt_name(sps->pix_fmt));
    }
#endif
    /* check if this is a repeat of an already parsed SPS, then keep the
     * original one.
     * otherwise drop all PPSes that depend on it */

    if (s->sps_list[sps_id] &&
        !memcmp(s->sps_list[sps_id], sps_buf, sizeof(HEVCSPS))) {
        mpp_free(sps_buf);
    } else {
        for (i = 0; (RK_U32)i < MPP_ARRAY_ELEMS(s->pps_list); i++) {
            if (s->pps_list[i] && ((HEVCPPS*)s->pps_list[i])->sps_id == sps_id) {
                mpp_hevc_pps_free(s->pps_list[i]);
                s->pps_list[i] = NULL;
            }
        }
        if (s->sps_list[sps_id] != NULL)
            mpp_free(s->sps_list[sps_id]);
        s->sps_list[sps_id] = sps_buf;
    }

    if (s->sps_list[sps_id])
        s->sps_list_of_updated[sps_id] = 1;

    return 0;
__BITREAD_ERR:
err:
    mpp_free(sps_buf);
    return ret;
}

void mpp_hevc_pps_free(RK_U8 *data)
{
    HEVCPPS *pps = (HEVCPPS*)data;
    if (pps != NULL) {
        mpp_free(pps->column_width);
        mpp_free(pps->row_height);
        mpp_free(pps);
    }
}

int mpp_hevc_decode_nal_pps(HEVCContext *s)
{

    BitReadCtx_t *gb = &s->HEVClc->gb;
    HEVCSPS      *sps = NULL;
    RK_S32 i;
    RK_S32 ret    = 0;
    RK_S32 pps_id = 0;

    HEVCPPS *pps = NULL;
    RK_U8 *pps_buf;
    pps_buf = mpp_calloc(RK_U8, sizeof(*pps));

    if (!pps_buf)
        return MPP_ERR_NOMEM;

    pps = ( HEVCPPS *)pps_buf;
    h265d_dbg(H265D_DBG_FUNCTION, "Decoding PPS\n");

    // Default values
    pps->loop_filter_across_tiles_enabled_flag = 1;
    pps->num_tile_columns                      = 1;
    pps->num_tile_rows                         = 1;
    pps->uniform_spacing_flag                  = 1;
    pps->disable_dbf                           = 0;
    pps->beta_offset                           = 0;
    pps->tc_offset                             = 0;

    // Coded parameters
    READ_UE(gb, &pps_id);
    pps->pps_id = pps_id;///<- zrh add
    if (pps_id >= MAX_PPS_COUNT) {
        mpp_err( "PPS id out of range: %d\n", pps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    READ_UE(gb, &pps->sps_id);
    if (pps->sps_id >= MAX_SPS_COUNT) {
        mpp_err( "SPS id out of range: %d\n", pps->sps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    if (!s->sps_list[pps->sps_id]) {
        mpp_err( "SPS %u does not exist.\n", pps->sps_id);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    sps = (HEVCSPS *)s->sps_list[pps->sps_id];

    h265d_dbg(H265D_DBG_FUNCTION, "Decoding PPS 1\n");
    READ_ONEBIT(gb, &pps->dependent_slice_segments_enabled_flag);
    READ_ONEBIT(gb, &pps->output_flag_present_flag );
    READ_BITS(gb, 3, &pps->num_extra_slice_header_bits);

    READ_ONEBIT(gb, &pps->sign_data_hiding_flag);

    READ_ONEBIT(gb, &pps->cabac_init_present_flag);

    READ_UE(gb, &pps->num_ref_idx_l0_default_active);
    pps->num_ref_idx_l0_default_active +=  1;
    READ_UE(gb, &pps->num_ref_idx_l1_default_active);
    pps->num_ref_idx_l1_default_active += 1;

    READ_SE(gb, &pps->pic_init_qp_minus26);

    READ_ONEBIT(gb, & pps->constrained_intra_pred_flag);
    READ_ONEBIT(gb, &pps->transform_skip_enabled_flag);

    READ_ONEBIT(gb, &pps->cu_qp_delta_enabled_flag);
    pps->diff_cu_qp_delta_depth   = 0;
    if (pps->cu_qp_delta_enabled_flag)
        READ_UE(gb, &pps->diff_cu_qp_delta_depth);

    READ_SE(gb, &pps->cb_qp_offset);
    if (pps->cb_qp_offset < -12 || pps->cb_qp_offset > 12) {
        mpp_err( "pps_cb_qp_offset out of range: %d\n",
                 pps->cb_qp_offset);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    READ_SE(gb, &pps->cr_qp_offset);
    if (pps->cr_qp_offset < -12 || pps->cr_qp_offset > 12) {
        mpp_err( "pps_cr_qp_offset out of range: %d\n",
                 pps->cr_qp_offset);
        ret =  MPP_ERR_STREAM;
        goto err;
    }
    READ_ONEBIT(gb, & pps->pic_slice_level_chroma_qp_offsets_present_flag);

    READ_ONEBIT(gb, &pps->weighted_pred_flag);

    READ_ONEBIT(gb, &pps->weighted_bipred_flag);

    READ_ONEBIT(gb, &pps->transquant_bypass_enable_flag);
    READ_ONEBIT(gb, &pps->tiles_enabled_flag);
    READ_ONEBIT(gb, &pps->entropy_coding_sync_enabled_flag);

    // check support solution
    {
        RK_S32 max_supt_width = MAX_WIDTH;
        RK_S32 max_supt_height = pps->tiles_enabled_flag ? MAX_HEIGHT : MAX_WIDTH;

        if (sps->width > max_supt_width || sps->height > max_supt_height) {
            mpp_err("cannot support %dx%d, max solution %dx%d\n",
                    sps->width, sps->height, max_supt_width, max_supt_height);
            goto err;
        }
    }

    if (pps->tiles_enabled_flag) {
        READ_UE(gb, &pps->num_tile_columns);
        pps->num_tile_columns += 1;
        READ_UE(gb, &pps->num_tile_rows);
        pps->num_tile_rows += 1;
        if (pps->num_tile_columns == 0 ||
            pps->num_tile_columns >= sps->width) {
            mpp_err( "num_tile_columns_minus1 out of range: %d\n",
                     pps->num_tile_columns - 1);
            ret =  MPP_ERR_STREAM;
            goto err;
        }
        if (pps->num_tile_rows == 0 ||
            pps->num_tile_rows >= sps->height) {
            mpp_err( "num_tile_rows_minus1 out of range: %d\n",
                     pps->num_tile_rows - 1);
            ret =  MPP_ERR_STREAM;
            goto err;
        }

        pps->column_width = mpp_malloc(RK_U32, pps->num_tile_columns);
        pps->row_height   = mpp_malloc(RK_U32, pps->num_tile_rows);
        if (!pps->column_width || !pps->row_height) {
            ret = MPP_ERR_NOMEM;
            goto err;
        }

        READ_ONEBIT(gb, &pps->uniform_spacing_flag );
        if (!pps->uniform_spacing_flag) {
            RK_S32 sum = 0;
            for (i = 0; i < pps->num_tile_columns - 1; i++) {
                READ_UE(gb, &pps->column_width[i]);
                pps->column_width[i] +=  1;
                sum                  += pps->column_width[i];
            }
            if (sum >= sps->ctb_width) {
                mpp_err( "Invalid tile widths.\n");
                ret =  MPP_ERR_STREAM;
                goto err;
            }
            pps->column_width[pps->num_tile_columns - 1] = sps->ctb_width - sum;

            sum = 0;
            for (i = 0; i < pps->num_tile_rows - 1; i++) {
                READ_UE(gb, &pps->row_height[i]);
                pps->row_height[i] += 1;
                sum                += pps->row_height[i];
            }
            if (sum >= sps->ctb_height) {
                mpp_err( "Invalid tile heights.\n");
                ret =  MPP_ERR_STREAM;
                goto err;
            }
            pps->row_height[pps->num_tile_rows - 1] = sps->ctb_height - sum;
        }
        READ_ONEBIT(gb, &pps->loop_filter_across_tiles_enabled_flag);
    }

    READ_ONEBIT(gb, &pps->seq_loop_filter_across_slices_enabled_flag);
    READ_ONEBIT(gb, &pps->deblocking_filter_control_present_flag);
    if (pps->deblocking_filter_control_present_flag) {
        READ_ONEBIT(gb, &pps->deblocking_filter_override_enabled_flag);
        READ_ONEBIT(gb, & pps->disable_dbf);
        if (!pps->disable_dbf) {
            READ_SE(gb, &pps->beta_offset);
            pps->beta_offset = pps->beta_offset * 2;
            READ_SE(gb, &pps->tc_offset);
            pps->tc_offset = pps->tc_offset * 2;
            if (pps->beta_offset / 2 < -6 || pps->beta_offset / 2 > 6) {
                mpp_err( "pps_beta_offset_div2 out of range: %d\n",
                         pps->beta_offset / 2);
                ret =  MPP_ERR_STREAM;
                goto err;
            }
            if (pps->tc_offset / 2 < -6 || pps->tc_offset / 2 > 6) {
                mpp_err( "pps_tc_offset_div2 out of range: %d\n",
                         pps->tc_offset / 2);
                ret =  MPP_ERR_STREAM;
                goto err;
            }
        }
    }

    READ_ONEBIT(gb, &pps->scaling_list_data_present_flag);
    if (pps->scaling_list_data_present_flag) {
        set_default_scaling_list_data(&pps->scaling_list);
        ret = scaling_list_data(s, &pps->scaling_list, sps);

        if (ret < 0)
            goto err;
        s->scaling_list_listen[pps_id + 16] = 1;
    }

    h265d_dbg(H265D_DBG_PPS, "num bit left %d", gb->num_remaining_bits_in_curr_byte_);
    READ_ONEBIT(gb, & pps->lists_modification_present_flag);


    h265d_dbg(H265D_DBG_PPS, "num bit left %d", gb->num_remaining_bits_in_curr_byte_);
    READ_UE(gb, &pps->log2_parallel_merge_level);

    h265d_dbg(H265D_DBG_PPS, "num bit left %d", gb->num_remaining_bits_in_curr_byte_);
    pps->log2_parallel_merge_level += 2;
    if (pps->log2_parallel_merge_level > sps->log2_ctb_size) {
        mpp_err( "log2_parallel_merge_level_minus2 out of range: %d\n",
                 pps->log2_parallel_merge_level - 2);
        ret =  MPP_ERR_STREAM;
        goto err;
    }

    if (s->h265dctx->compare_info != NULL) {
        CurrentFameInf_t *info = (CurrentFameInf_t *)s->h265dctx->compare_info;
        HEVCPPS *openhevc_pps = (HEVCPPS*)&info->pps[pps_id];
        mpp_log("compare pps in");
        if (compare_pps(openhevc_pps, (HEVCPPS*)pps_buf) < 0) {
            mpp_err("compare pps with openhevc error found");
            mpp_assert(0);
            return -1;
        }
        mpp_log("compare pps ok");
    }

    // Inferred parameters
    if (pps->uniform_spacing_flag) {
        if (!pps->column_width) {
            pps->column_width = mpp_malloc(RK_U32, pps->num_tile_columns );
            pps->row_height   = mpp_malloc(RK_U32, pps->num_tile_rows);
        }
        if (!pps->column_width || !pps->row_height) {
            ret = MPP_ERR_NOMEM;
            goto err;
        }

        for (i = 0; i < pps->num_tile_columns; i++) {
            pps->column_width[i] = ((i + 1) * sps->ctb_width) / pps->num_tile_columns -
                                   (i * sps->ctb_width) / pps->num_tile_columns;
        }

        for (i = 0; i < pps->num_tile_rows; i++) {
            pps->row_height[i] = ((i + 1) * sps->ctb_height) / pps->num_tile_rows -
                                 (i * sps->ctb_height) / pps->num_tile_rows;
        }
    }

    if (s->pps_list[pps_id] != NULL) {
        mpp_hevc_pps_free(s->pps_list[pps_id]);
        s->pps_list[pps_id] = NULL;
    }
    s->pps_list[pps_id] = pps_buf;

    if (s->pps_list[pps_id])
        s->pps_list_of_updated[pps_id] = 1;

    return 0;
__BITREAD_ERR:
err:
    mpp_hevc_pps_free(pps_buf);
    return ret;
}
