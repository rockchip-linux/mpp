/*
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

#define MODULE_TAG "h265e_header_gen"

#include <string.h>

#include "mpp_common.h"
#include "mpp_mem.h"
#include "h265e_ps.h"
#include "h265e_header_gen.h"

static void h265e_nals_init(H265eExtraInfo *out)
{
    out->nal_buf = mpp_calloc(RK_U8, H265E_EXTRA_INFO_BUF_SIZE);
    out->nal_num = 0;
}

static void h265e_nals_deinit(H265eExtraInfo *out)
{
    MPP_FREE(out->nal_buf);

    out->nal_num = 0;
}

static RK_U8 *h265e_nal_escape_c(RK_U8 *dst, RK_U8 *src, RK_U8 *end)
{
    if (src < end) *dst++ = *src++;
    if (src < end) *dst++ = *src++;
    while (src < end) {
        // if (src[0] <= 0x03 && !dst[-2] && !dst[-1])
        // *dst++ = 0x03;
        *dst++ = *src++;
    }
    return dst;
}

static void h265e_nal_encode(RK_U8 *dst, H265eNal *nal)
{
    RK_S32 b_annexb = 1;
    RK_S32 size = 0;
    RK_U8 *src = nal->p_payload;
    RK_U8 *end = nal->p_payload + nal->i_payload;
    RK_U8 *orig_dst = dst;
    MppWriteCtx s;

    if (b_annexb) {
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    } else /* save room for size later */
        dst += 4;

    /* nal header */
    mpp_writer_init(&s, dst, 10);
    mpp_writer_put_bits(&s, 0, 1); //forbidden_zero_bit
    mpp_writer_put_bits(&s, nal->i_type, 6);//nal_unit_type
    mpp_writer_put_bits(&s, 0, 6); //nuh_reserved_zero_6bits
    mpp_writer_put_bits(&s, 1, 3); //nuh_temporal_id_plus1
    dst += 2;
    dst = h265e_nal_escape_c(dst, src, end);
    size = (RK_S32)((dst - orig_dst) - 4);

    /* Write the size header for mp4/etc */
    if (!b_annexb) {
        /* Size doesn't include the size of the header we're writing now. */
        orig_dst[0] = size >> 24;
        orig_dst[1] = size >> 16;
        orig_dst[2] = size >> 8;
        orig_dst[3] = size >> 0;
    }

    nal->i_payload = size + 4;
    nal->p_payload = orig_dst;
}

static MPP_RET h265e_encapsulate_nals(H265eExtraInfo *out)
{
    RK_S32 i = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 nal_size = 0;
    RK_S32 necessary_size = 0;
    RK_U8 *nal_buffer = out->nal_buf;
    RK_S32 nal_num = out->nal_num;
    H265eNal *nal = out->nal;

    h265e_dbg_func("enter\n");
    for (i = 0; i < nal_num; i++)
        nal_size += nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    necessary_size = nal_size * 3 / 2 + nal_num * 4 + 4 + 64;
    for (i = 0; i < nal_num; i++)
        necessary_size += nal[i].i_padding;

    for (i = 0; i < nal_num; i++) {
        nal[i].b_long_startcode = !i ||
                                  nal[i].i_type == NAL_VPS ||
                                  nal[i].i_type == NAL_SPS ||
                                  nal[i].i_type == NAL_PPS ||
                                  i_avcintra_class;
        h265e_nal_encode(nal_buffer, &nal[i]);
        nal_buffer += nal[i].i_payload;
    }

    h265e_dbg(H265E_DBG_HEADER, "nals total size: %d bytes",
              nal_buffer - out->nal_buf);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_sei_write(H265eStream *s, RK_U8 uuid[16], const RK_U8 *payload,
                               RK_S32 payload_size, RK_S32 payload_type)
{
    RK_S32 i = 0;
    RK_S32 uuid_len = H265E_UUID_LENGTH;
    RK_S32 data_len = payload_size;

    h265e_dbg_func("enter\n");

    h265e_stream_realign(s);

    payload_size += uuid_len;

    for (i = 0; i <= payload_type - 255; i += 255)
        h265e_stream_write_with_log(s, 0xff, 8,
                                    "sei_payload_type_ff_byte");

    h265e_stream_write_with_log(s, payload_type - i, 8,
                                "sei_last_payload_type_byte");

    for (i = 0; i <= payload_size - 255; i += 255)
        h265e_stream_write_with_log(s, 0xff, 8,
                                    "sei_payload_size_ff_byte");

    h265e_stream_write_with_log(s,  payload_size - i, 8,
                                "sei_last_payload_size_byte");

    for (i = 0; i < uuid_len; i++) {
        h265e_stream_write_with_log(s, uuid[i], 8,
                                    "sei_uuid_byte");
    }

    for (i = 0; i < data_len; i++)
        h265e_stream_write_with_log(s, (RK_U32)payload[i], 8,
                                    "sei_payload_data");

    h265e_stream_rbsp_trailing(s);

    h265e_dbg_func("leave\n");

    return MPP_OK;
}

void codeProfileTier(H265eStream *s, ProfileTierLevel* ptl)
{
    RK_S32 j;
    h265e_stream_write_with_log(s, ptl->m_profileSpace, 2, "profile_space[]");
    h265e_stream_write1_with_log(s, ptl->m_tierFlag,        "tier_flag[]");
    h265e_stream_write_with_log(s, ptl->m_profileIdc, 5,   "profile_idc[]");
    for (j = 0; j < 32; j++) {
        h265e_stream_write1_with_log(s, ptl->m_profileCompatibilityFlag[j], "profile_compatibility_flag[][j]");
    }

    h265e_stream_write1_with_log(s, ptl->m_progressiveSourceFlag,   "general_progressive_source_flag");
    h265e_stream_write1_with_log(s, ptl->m_interlacedSourceFlag,    "general_interlaced_source_flag");
    h265e_stream_write1_with_log(s, ptl->m_nonPackedConstraintFlag, "general_non_packed_constraint_flag");
    h265e_stream_write1_with_log(s, ptl->m_frameOnlyConstraintFlag, "general_frame_only_constraint_flag");

    h265e_stream_write_with_log(s, 0, 16, "reserved_zero_44bits[0..15]");
    h265e_stream_write_with_log(s, 0, 16, "reserved_zero_44bits[16..31]");
    h265e_stream_write_with_log(s, 0, 12, "eserved_zero_44bits[32..43]");
}

void codePTL(H265eStream *s, H265ePTL* ptl, RK_U32 profilePresentFlag, int maxNumSubLayersMinus1)
{
    RK_S32 i;
    if (profilePresentFlag) {
        codeProfileTier(s, &ptl->m_generalPTL);
    }
    h265e_stream_write_with_log(s, ptl->m_generalPTL.m_levelIdc, 8, "general_level_idc");

    for (i = 0; i < maxNumSubLayersMinus1; i++) {
        if (profilePresentFlag) {
            h265e_stream_write1_with_log(s, ptl->m_subLayerProfilePresentFlag[i], "sub_layer_profile_present_flag[i]");
        }

        h265e_stream_write1_with_log(s, ptl->m_subLayerLevelPresentFlag[i], "sub_layer_level_present_flag[i]");
    }

    if (maxNumSubLayersMinus1 > 0) {
        for (i = maxNumSubLayersMinus1; i < 8; i++) {
            h265e_stream_write_with_log(s, 0, 2, "reserved_zero_2bits");
        }
    }

    for (i = 0; i < maxNumSubLayersMinus1; i++) {
        if (profilePresentFlag && ptl->m_subLayerProfilePresentFlag[i]) {
            codeProfileTier(s, &ptl->m_subLayerPTL[i]); // sub_layer_...
        }
        if (ptl->m_subLayerLevelPresentFlag[i]) {
            h265e_stream_write_with_log(s, ptl->m_subLayerPTL[i].m_levelIdc, 8, "sub_layer_level_idc[i]");
        }
    }
}

static MPP_RET h265e_vps_write(H265eVps *vps, H265eStream *s)
{
    RK_S32 vps_byte_start = 0;
    RK_U32 i, opsIdx;

    h265e_dbg_func("enter\n");
    h265e_stream_realign(s);
    vps_byte_start = s->enc_stream.byte_cnt;
    h265e_stream_write_with_log(s, vps->m_VPSId, 4, "vps_video_parameter_set_id");
    h265e_stream_write_with_log(s, 3, 2, "vps_reserved_three_2bits");
    h265e_stream_write_with_log(s, 0, 6, "vps_reserved_zero_6bits");
    h265e_stream_write_with_log(s, vps->m_maxTLayers - 1, 3, "vps_max_sub_layers_minus1");
    h265e_stream_write1_with_log(s, vps->m_bTemporalIdNestingFlag, "vps_temporal_id_nesting_flag");

    h265e_stream_write_with_log(s, 0xffff, 16, "vps_reserved_ffff_16bits");

    codePTL(s, &vps->m_ptl, 1, vps->m_maxTLayers - 1);

    h265e_stream_write1_with_log(s, 1, "vps_sub_layer_ordering_info_present_flag");
    for (i = 0; i <= vps->m_maxTLayers - 1; i++) {
        h265e_stream_write_ue_with_log(s, vps->m_maxDecPicBuffering[i] - 1, "vps_max_dec_pic_buffering_minus1[i]");
        h265e_stream_write_ue_with_log(s, vps->m_numReorderPics[i],  "vps_num_reorder_pics[i]");
        h265e_stream_write_ue_with_log(s, vps->m_maxLatencyIncrease[i], "vps_max_latency_increase_plus1[i]");
    }

    mpp_assert(vps->m_numHrdParameters <= MAX_VPS_NUM_HRD_PARAMETERS);
    mpp_assert(vps->m_maxNuhReservedZeroLayerId < MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1);
    h265e_stream_write_with_log(s, vps->m_maxNuhReservedZeroLayerId, 6, "vps_max_nuh_reserved_zero_layer_id");
    vps->m_numOpSets = 1;
    h265e_stream_write_ue_with_log(s, vps->m_numOpSets - 1, "vps_max_op_sets_minus1");
    for (opsIdx = 1; opsIdx <= (vps->m_numOpSets - 1); opsIdx++) {
        // Operation point set
        for (i = 0; i <= vps->m_maxNuhReservedZeroLayerId; i++) {
            // Only applicable for version 1
            vps->m_layerIdIncludedFlag[opsIdx][i] = 1;
            h265e_stream_write1_with_log(s, vps->m_layerIdIncludedFlag[opsIdx][i] ? 1 : 0, "layer_id_included_flag[opsIdx][i]");
        }
    }

    TimingInfo *timingInfo = &vps->m_timingInfo;
    h265e_stream_write1_with_log(s, timingInfo->m_timingInfoPresentFlag, "vps_timing_info_present_flag");
    if (timingInfo->m_timingInfoPresentFlag) {
        h265e_stream_write_with_log(s, timingInfo->m_numUnitsInTick, 32,           "vps_num_units_in_tick");
        h265e_stream_write_with_log(s, timingInfo->m_timeScale,      32,           "vps_time_scale");
        h265e_stream_write1_with_log(s, timingInfo->m_pocProportionalToTimingFlag,  "vps_poc_proportional_to_timing_flag");
        if (timingInfo->m_pocProportionalToTimingFlag) {
            h265e_stream_write_ue_with_log(s, timingInfo->m_numTicksPocDiffOneMinus1,   "vps_num_ticks_poc_diff_one_minus1");
        }
        vps->m_numHrdParameters = 0;
        h265e_stream_write_ue_with_log(s, vps->m_numHrdParameters,                 "vps_num_hrd_parameters");
#if 0
        if (vps->m_numHrdParameters > 0) {
            vps->createHrdParamBuffer();
        }
        for (uint32_t i = 0; i < vps->getNumHrdParameters(); i++) {
            // Only applicable for version 1
            vps->setHrdOpSetIdx(0, i);
            h265e_stream_write_ue_with_log(s, vps->getHrdOpSetIdx(i),                "hrd_op_set_idx");
            if (i > 0) {
                h265e_stream_write1_with_log(s, vps->getCprmsPresentFlag(i) ? 1 : 0, "cprms_present_flag[i]");
            }
            codeHrdParameters(vps->getHrdParameters(i), vps->getCprmsPresentFlag(i), vps->getMaxTLayers() - 1);
        }
#endif
    }
    h265e_stream_write1_with_log(s, 0,                     "vps_extension_flag");
    h265e_stream_rbsp_trailing(s);
    h265e_stream_flush(s);
    h265e_dbg(H265E_DBG_HEADER, "write pure vps head size: %d bits", (s->enc_stream.byte_cnt - vps_byte_start) * 8);
    //future extensions here..
    h265e_dbg_func("leave\n");
    return MPP_OK;
}

void codeVUI(H265eStream *s, H265eVuiInfo *vui)
{
    h265e_stream_write1_with_log(s, vui->m_aspectRatioInfoPresentFlag,  "aspect_ratio_info_present_flag");
    if (vui->m_aspectRatioInfoPresentFlag) {
        h265e_stream_write_with_log(s, vui->m_aspectRatioIdc, 8,       "aspect_ratio_idc");
        if (vui->m_aspectRatioIdc == 255) {
            h265e_stream_write_with_log(s, vui->m_sarWidth, 16,        "sar_width");
            h265e_stream_write_with_log(s, vui->m_sarHeight, 16,       "sar_height");
        }
    }
    h265e_stream_write1_with_log(s, vui->m_overscanInfoPresentFlag,     "overscan_info_present_flag");
    if (vui->m_overscanInfoPresentFlag) {
        h265e_stream_write1_with_log(s, vui->m_overscanAppropriateFlag, "overscan_appropriate_flag");
    }
    h265e_stream_write1_with_log(s, vui->m_videoSignalTypePresentFlag,  "video_signal_type_present_flag");
    if (vui->m_videoSignalTypePresentFlag) {
        h265e_stream_write_with_log(s, vui->m_videoFormat, 3,          "video_format");
        h265e_stream_write1_with_log(s, vui->m_videoFullRangeFlag,      "video_full_range_flag");
        h265e_stream_write1_with_log(s, vui->m_colourDescriptionPresentFlag, "colour_description_present_flag");
        if (vui->m_colourDescriptionPresentFlag) {
            h265e_stream_write_with_log(s, vui->m_colourPrimaries, 8,         "colour_primaries");
            h265e_stream_write_with_log(s, vui->m_transferCharacteristics, 8, "transfer_characteristics");
            h265e_stream_write_with_log(s, vui->m_matrixCoefficients, 8,      "matrix_coefficients");
        }
    }

    h265e_stream_write1_with_log(s, vui->m_chromaLocInfoPresentFlag,           "chroma_loc_info_present_flag");
    if (vui->m_chromaLocInfoPresentFlag) {
        h265e_stream_write_ue_with_log(s, vui->m_chromaSampleLocTypeTopField,    "chroma_sample_loc_type_top_field");
        h265e_stream_write_ue_with_log(s, vui->m_chromaSampleLocTypeBottomField, "chroma_sample_loc_type_bottom_field");
    }

    h265e_stream_write1_with_log(s, vui->m_neutralChromaIndicationFlag,        "neutral_chroma_indication_flag");
    h265e_stream_write1_with_log(s, vui->m_fieldSeqFlag,                       "field_seq_flag");
    h265e_stream_write1_with_log(s, vui->m_frameFieldInfoPresentFlag,          "frame_field_info_present_flag");

    H265eCropInfo defaultDisplayWindow = vui->m_defaultDisplayWindow;
    h265e_stream_write1_with_log(s, defaultDisplayWindow.m_enabledFlag,           "default_display_window_flag");
    if (defaultDisplayWindow.m_enabledFlag) {
        h265e_stream_write_ue_with_log(s, defaultDisplayWindow.m_winLeftOffset,     "def_disp_win_left_offset");
        h265e_stream_write_ue_with_log(s, defaultDisplayWindow.m_winRightOffset,    "def_disp_win_right_offset");
        h265e_stream_write_ue_with_log(s, defaultDisplayWindow.m_winTopOffset,      "def_disp_win_top_offset");
        h265e_stream_write_ue_with_log(s, defaultDisplayWindow.m_winBottomOffset,   "def_disp_win_bottom_offset");
    }
    TimingInfo *timingInfo = &vui->m_timingInfo;
    h265e_stream_write1_with_log(s, timingInfo->m_timingInfoPresentFlag,       "vui_timing_info_present_flag");
    if (timingInfo->m_timingInfoPresentFlag) {
        h265e_stream_write32(s, timingInfo->m_numUnitsInTick, "vui_num_units_in_tick");
        h265e_stream_write32(s, timingInfo->m_timeScale,      "vui_time_scale");
        h265e_stream_write1_with_log(s, timingInfo->m_pocProportionalToTimingFlag,  "vui_poc_proportional_to_timing_flag");
        if (timingInfo->m_pocProportionalToTimingFlag) {
            h265e_stream_write_ue_with_log(s, timingInfo->m_numTicksPocDiffOneMinus1,   "vui_num_ticks_poc_diff_one_minus1");
        }
        h265e_stream_write1_with_log(s, vui->m_hrdParametersPresentFlag,              "hrd_parameters_present_flag");
        if (vui->m_hrdParametersPresentFlag) {
            // codeHrdParameters(vui->getHrdParameters(), 1, sps->getMaxTLayers() - 1); //todo
        }
    }
    h265e_stream_write1_with_log(s, vui->m_bitstreamRestrictionFlag,                "bitstream_restriction_flag");
    if (vui->m_bitstreamRestrictionFlag) {
        h265e_stream_write1_with_log(s, vui->m_tilesFixedStructureFlag,             "tiles_fixed_structure_flag");
        h265e_stream_write1_with_log(s, vui->m_motionVectorsOverPicBoundariesFlag,  "motion_vectors_over_pic_boundaries_flag");
        h265e_stream_write1_with_log(s, vui->m_restrictedRefPicListsFlag,           "restricted_ref_pic_lists_flag");
        h265e_stream_write_ue_with_log(s, vui->m_minSpatialSegmentationIdc,           "min_spatial_segmentation_idc");
        h265e_stream_write_ue_with_log(s, vui->m_maxBytesPerPicDenom,                 "max_bytes_per_pic_denom");
        h265e_stream_write_ue_with_log(s, vui->m_maxBitsPerMinCuDenom,                "max_bits_per_mincu_denom");
        h265e_stream_write_ue_with_log(s, vui->m_log2MaxMvLengthHorizontal,           "log2_max_mv_length_horizontal");
        h265e_stream_write_ue_with_log(s, vui->m_log2MaxMvLengthHorizontal,             "log2_max_mv_length_vertical");
    }
}

static MPP_RET h265e_sps_write(H265eSps *sps, H265eStream *s)
{
    RK_S32 sps_byte_start = 0;
    RK_U32 i, k;
    const RK_S32 winUnitX[] = { 1, 2, 2, 1 };
    const RK_S32 winUnitY[] = { 1, 2, 1, 1 };
    H265eCropInfo *conf = &sps->m_conformanceWindow;

    h265e_dbg_func("enter\n");
    h265e_stream_realign(s);
    sps_byte_start = s->enc_stream.byte_cnt;

    h265e_stream_write_with_log(s, sps->m_VPSId,          4,       "sps_video_parameter_set_id");
    h265e_stream_write_with_log(s, sps->m_maxTLayers - 1,  3,       "sps_max_sub_layers_minus1");
    h265e_stream_write1_with_log(s, sps->m_bTemporalIdNestingFlag ? 1 : 0, "sps_temporal_id_nesting_flag");
    codePTL(s, sps->m_ptl, 1, sps->m_maxTLayers - 1);
    h265e_stream_write_ue_with_log(s, sps->m_SPSId,                   "sps_seq_parameter_set_id");
    h265e_stream_write_ue_with_log(s, sps->m_chromaFormatIdc,         "chroma_format_idc");

    if (sps->m_chromaFormatIdc == 4) {
        h265e_stream_write1_with_log(s, 0,                             "separate_colour_plane_flag");
    }

    h265e_stream_write_ue_with_log(s, sps->m_picWidthInLumaSamples,   "pic_width_in_luma_samples");
    h265e_stream_write_ue_with_log(s, sps->m_picHeightInLumaSamples,  "pic_height_in_luma_samples");

    h265e_stream_write1_with_log(s, conf->m_enabledFlag,          "conformance_window_flag");
    if (conf->m_enabledFlag) {
        h265e_stream_write_ue_with_log(s, conf->m_winLeftOffset   / winUnitX[sps->m_chromaFormatIdc], "conf_win_left_offset");
        h265e_stream_write_ue_with_log(s, conf->m_winRightOffset  / winUnitX[sps->m_chromaFormatIdc], "conf_win_right_offset");
        h265e_stream_write_ue_with_log(s, conf->m_winTopOffset    / winUnitY[sps->m_chromaFormatIdc], "conf_win_top_offset");
        h265e_stream_write_ue_with_log(s, conf->m_winBottomOffset / winUnitY[sps->m_chromaFormatIdc], "conf_win_bottom_offset");
    }

    h265e_stream_write_ue_with_log(s, sps->m_bitDepthY - 8,             "bit_depth_luma_minus8");
    h265e_stream_write_ue_with_log(s, sps->m_bitDepthC - 8,             "bit_depth_chroma_minus8");

    h265e_stream_write_ue_with_log(s, sps->m_bitsForPOC - 4,            "log2_max_pic_order_cnt_lsb_minus4");

    h265e_stream_write1_with_log(s, 1,     "sps_sub_layer_ordering_info_present_flag");
    for (i = 0; i <= sps->m_maxTLayers - 1; i++) {
        h265e_stream_write_ue_with_log(s, sps->m_maxDecPicBuffering[i] - 1, "sps_max_dec_pic_buffering_minus1[i]");
        h265e_stream_write_ue_with_log(s, sps->m_numReorderPics[i],       "sps_num_reorder_pics[i]");
        h265e_stream_write_ue_with_log(s, sps->m_maxLatencyIncrease[i],   "sps_max_latency_increase_plus1[i]");
    }

    h265e_stream_write_ue_with_log(s, sps->m_log2MinCodingBlockSize - 3,    "log2_min_coding_block_size_minus3");
    h265e_stream_write_ue_with_log(s, sps->m_log2DiffMaxMinCodingBlockSize, "log2_diff_max_min_coding_block_size");
    h265e_stream_write_ue_with_log(s, sps->m_quadtreeTULog2MinSize - 2,     "log2_min_transform_block_size_minus2");
    h265e_stream_write_ue_with_log(s, sps->m_quadtreeTULog2MaxSize - sps->m_quadtreeTULog2MinSize, "log2_diff_max_min_transform_block_size");
    h265e_stream_write_ue_with_log(s, sps->m_quadtreeTUMaxDepthInter - 1,   "max_transform_hierarchy_depth_inter");
    h265e_stream_write_ue_with_log(s, sps->m_quadtreeTUMaxDepthIntra - 1,   "max_transform_hierarchy_depth_intra");
    h265e_stream_write1_with_log(s, sps->m_scalingListEnabledFlag ? 1 : 0,       "scaling_list_enabled_flag");
    if (sps->m_scalingListEnabledFlag) {
        h265e_stream_write1_with_log(s, sps->m_scalingListPresentFlag ? 1 : 0, "sps_scaling_list_data_present_flag");
        if (sps->m_scalingListPresentFlag) {
            mpp_log("to do m_scalingListPresentFlag");
            ;//codeScalingList(m_slice->getScalingList()); //todo only support default
        }
    }
    h265e_stream_write1_with_log(s, sps->m_useAMP ? 1 : 0, "amp_enabled_flag");
    h265e_stream_write1_with_log(s, sps->m_bUseSAO ? 1 : 0, "sample_adaptive_offset_enabled_flag");

    h265e_stream_write1_with_log(s, sps->m_usePCM ? 1 : 0, "pcm_enabled_flag");
    if (sps->m_usePCM) {
        h265e_stream_write_with_log(s, sps->m_pcmBitDepthLuma - 1, 4,                     "pcm_sample_bit_depth_luma_minus1");
        h265e_stream_write_with_log(s, sps->m_pcmBitDepthChroma - 1, 4,                   "pcm_sample_bit_depth_chroma_minus1");
        h265e_stream_write_ue_with_log(s, sps->m_pcmLog2MinSize - 3,                         "log2_min_pcm_luma_coding_block_size_minus3");
        h265e_stream_write_ue_with_log(s, sps->m_pcmLog2MaxSize - sps->m_pcmLog2MinSize,  "log2_diff_max_min_pcm_luma_coding_block_size");
        h265e_stream_write1_with_log(s, sps->m_bPCMFilterDisableFlag ? 1 : 0,               "pcm_loop_filter_disable_flag");
    }

    mpp_assert(sps->m_maxTLayers > 0);

    H265eRPSList* rpsList = &sps->m_RPSList;
    //  H265eReferencePictureSet* rps;

    h265e_stream_write_ue_with_log(s, rpsList->m_numberOfReferencePictureSets, "num_short_term_ref_pic_sets");
    for ( i = 0; i < (RK_U32)rpsList->m_numberOfReferencePictureSets; i++) {
        mpp_log("todo m_numberOfReferencePictureSets");
        //rps = &rpsList->m_referencePictureSets[i];
        // codeShortTermRefPicSet(rps, false, i); //todo no support
    }
    h265e_stream_write1_with_log(s, sps->m_bLongTermRefsPresent ? 1 : 0,      "long_term_ref_pics_present_flag");
    if (sps->m_bLongTermRefsPresent) {
        h265e_stream_write_ue_with_log(s, sps->m_numLongTermRefPicSPS, "num_long_term_ref_pic_sps");
        for (k = 0; k < sps->m_numLongTermRefPicSPS; k++) {
            h265e_stream_write_with_log(s, sps->m_ltRefPicPocLsbSps[k], sps->m_bitsForPOC, "lt_ref_pic_poc_lsb_sps");
            h265e_stream_write1_with_log(s, sps->m_usedByCurrPicLtSPSFlag[k], "used_by_curr_pic_lt_sps_flag");
        }
    }
    h265e_stream_write1_with_log(s, sps->m_TMVPFlagsPresent ? 1 : 0,        "sps_temporal_mvp_enable_flag");

    h265e_stream_write1_with_log(s, sps->m_useStrongIntraSmoothing,          "sps_strong_intra_smoothing_enable_flag");

    h265e_stream_write1_with_log(s, sps->m_vuiParametersPresentFlag,         "vui_parameters_present_flag");
    if (sps->m_vuiParametersPresentFlag) {
        codeVUI(s, &sps->vui);
    }
    h265e_stream_write1_with_log(s, 0, "sps_extension_flag");
    h265e_stream_rbsp_trailing(s);
    h265e_stream_flush(s);
    h265e_dbg(H265E_DBG_HEADER, "write pure sps head size: %d bits", (s->enc_stream.byte_cnt - sps_byte_start) * 8);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

#if 0
static void h265e_rkv_scaling_list_write( H265eRkvStream *s,
                                          H265ePps *pps, RK_S32 idx )
{

}
#endif

static MPP_RET h265e_pps_write(H265ePps *pps, H265eSps *sps, H265eStream *s)
{
    (void) sps;
    RK_S32 pps_byte_start = 0;
    RK_S32 i;
    h265e_stream_realign(s);
    pps_byte_start = s->enc_stream.byte_cnt;

    h265e_dbg_func("enter\n");
    h265e_stream_write_ue_with_log(s, pps->m_PPSId,                            "pps_pic_parameter_set_id");
    h265e_stream_write_ue_with_log(s, pps->m_SPSId,                            "pps_seq_parameter_set_id");
    h265e_stream_write1_with_log(s, 0,                                          "dependent_slice_segments_enabled_flag");
    h265e_stream_write1_with_log(s, pps->m_outputFlagPresentFlag ? 1 : 0,    "output_flag_present_flag");
    h265e_stream_write_with_log(s, pps->m_numExtraSliceHeaderBits, 3,       "num_extra_slice_header_bits");
    h265e_stream_write1_with_log(s, pps->m_signHideFlag,                     "sign_data_hiding_flag");
    h265e_stream_write1_with_log(s, pps->m_cabacInitPresentFlag ? 1 : 0,     "cabac_init_present_flag");
    h265e_stream_write_ue_with_log(s, pps->m_numRefIdxL0DefaultActive - 1,     "num_ref_idx_l0_default_active_minus1");
    h265e_stream_write_ue_with_log(s, pps->m_numRefIdxL1DefaultActive - 1,     "num_ref_idx_l1_default_active_minus1");

    h265e_stream_write_se_with_log(s, pps->m_picInitQPMinus26,                 "init_qp_minus26");
    h265e_stream_write1_with_log(s, pps->m_bConstrainedIntraPred ? 1 : 0,     "constrained_intra_pred_flag");
    h265e_stream_write1_with_log(s, pps->m_useTransformSkip ? 1 : 0,         "transform_skip_enabled_flag");
    h265e_stream_write1_with_log(s, pps->m_useDQP ? 1 : 0,                   "cu_qp_delta_enabled_flag");
    if (pps->m_useDQP) {
        h265e_stream_write_ue_with_log(s, pps->m_maxCuDQPDepth,                "diff_cu_qp_delta_depth");
    }
    h265e_stream_write_se_with_log(s, pps->m_chromaCbQpOffset,                 "pps_cb_qp_offset");
    h265e_stream_write_se_with_log(s, pps->m_chromaCrQpOffset,                 "pps_cr_qp_offset");
    h265e_stream_write1_with_log(s, pps->m_bSliceChromaQpFlag ? 1 : 0,        "pps_slice_chroma_qp_offsets_present_flag");

    h265e_stream_write1_with_log(s, pps->m_bUseWeightPred ? 1 : 0,                    "weighted_pred_flag");  // Use of Weighting Prediction (P_SLICE)
    h265e_stream_write1_with_log(s, pps->m_useWeightedBiPred ? 1 : 0,                 "weighted_bipred_flag"); // Use of Weighting Bi-Prediction (B_SLICE)
    h265e_stream_write1_with_log(s, pps->m_transquantBypassEnableFlag ? 1 : 0, "transquant_bypass_enable_flag");
    h265e_stream_write1_with_log(s, pps->m_tiles_enabled_flag,                  "tiles_enabled_flag");
    h265e_stream_write1_with_log(s, pps->m_entropyCodingSyncEnabledFlag ? 1 : 0, "entropy_coding_sync_enabled_flag");
    if (pps->m_tiles_enabled_flag) {
        h265e_stream_write_ue_with_log(s, pps->m_nNumTileColumnsMinus1, "num_tile_columns_minus1");
        h265e_stream_write_ue_with_log(s, pps->m_nNumTileRowsMinus1, "num_tile_rows_minus1");
        h265e_stream_write1_with_log(s, pps->m_bTileUniformSpacing, "uniform_spacing_flag");
        if (!pps->m_bTileUniformSpacing) {
            for ( i = 0; i < pps->m_nNumTileColumnsMinus1; i++) {
                h265e_stream_write_ue_with_log(s, pps->m_nTileColumnWidthArray[i + 1] - pps->m_nTileColumnWidthArray[i] - 1, "column_width_minus1");
            }
            for (i = 0; i < pps->m_nNumTileRowsMinus1; i++) {
                h265e_stream_write_ue_with_log(s, pps->m_nTileRowHeightArray[i + 1] - pps->m_nTileRowHeightArray[i - 1], "row_height_minus1");
            }
        }
        mpp_assert((pps->m_nNumTileColumnsMinus1 + pps->m_nNumTileRowsMinus1) != 0);
        h265e_stream_write1_with_log(s, pps->m_loopFilterAcrossTilesEnabledFlag ? 1 : 0, "loop_filter_across_tiles_enabled_flag");
    }
    h265e_stream_write1_with_log(s, pps->m_LFCrossSliceBoundaryFlag ? 1 : 0, "loop_filter_across_slices_enabled_flag");

    // TODO: Here have some time sequence problem, we set below field in initEncSlice(), but use them in getStreamHeaders() early
    h265e_stream_write1_with_log(s, pps->m_deblockingFilterControlPresentFlag ? 1 : 0, "deblocking_filter_control_present_flag");
    if (pps->m_deblockingFilterControlPresentFlag) {
        h265e_stream_write1_with_log(s, pps->m_deblockingFilterOverrideEnabledFlag ? 1 : 0,  "deblocking_filter_override_enabled_flag");
        h265e_stream_write1_with_log(s, pps->m_picDisableDeblockingFilterFlag ? 1 : 0,       "pps_disable_deblocking_filter_flag");
        if (!pps->m_picDisableDeblockingFilterFlag) {
            h265e_stream_write_se_with_log(s, pps->m_deblockingFilterBetaOffsetDiv2, "pps_beta_offset_div2");
            h265e_stream_write_se_with_log(s, pps->m_deblockingFilterTcOffsetDiv2,   "pps_tc_offset_div2");
        }
    }
    h265e_stream_write1_with_log(s, pps->m_scalingListPresentFlag ? 1 : 0,         "pps_scaling_list_data_present_flag");
    if (pps->m_scalingListPresentFlag) {
        ;//codeScalingList(m_slice->getScalingList()); //todo
    }
    h265e_stream_write1_with_log(s, pps->m_listsModificationPresentFlag, "lists_modification_present_flag");
    h265e_stream_write_ue_with_log(s, pps->m_log2ParallelMergeLevelMinus2, "log2_parallel_merge_level_minus2");
    h265e_stream_write1_with_log(s, pps->m_sliceHeaderExtensionPresentFlag ? 1 : 0, "slice_segment_header_extension_present_flag");
    h265e_stream_write1_with_log(s, 0, "pps_extension_flag");

    h265e_stream_rbsp_trailing( s );
    h265e_stream_flush( s );

    h265e_dbg(H265E_DBG_HEADER, "write pure pps size: %d bits", (s->enc_stream.byte_cnt - pps_byte_start) * 8);
    h265e_dbg_func("leave\n");

    return MPP_OK;
}

void h265e_nal_start(H265eExtraInfo *out, RK_S32 i_type,
                     RK_S32 i_ref_idc)
{
    H265eStream *s = &out->stream;
    H265eNal *nal = &out->nal[out->nal_num];

    nal->i_ref_idc = i_ref_idc;
    nal->i_type = i_type;
    nal->b_long_startcode = 1;

    nal->i_payload = 0;
    /* NOTE: consistent with stream_init */
    nal->p_payload = &s->buf[s->enc_stream.byte_cnt];
    nal->i_padding = 0;
}

void h265e_nal_end(H265eExtraInfo *out)
{
    H265eNal *nal = &(out->nal[out->nal_num]);
    H265eStream *s = &out->stream;
    /* NOTE: consistent with stream_init */
    RK_U8 *end = &s->buf[s->enc_stream.byte_cnt];
    nal->i_payload = (RK_S32)(end - nal->p_payload);
    /*
     * Assembly implementation of nal_escape reads past the end of the input.
     * While undefined padding wouldn't actually affect the output,
     * it makes valgrind unhappy.
     */
    memset(end, 0xff, 64);
    out->nal_num++;
}

MPP_RET h265e_init_extra_info(void *extra_info)
{

    H265eExtraInfo *info = (H265eExtraInfo *)extra_info;
    // random ID number generated according to ISO-11578
    // NOTE: any element of h264e_sei_uuid should NOT be 0x00,
    // otherwise the string length of sei_buf will always be the distance between the
    // element 0x00 address and the sei_buf start address.
    static const RK_U8 h265e_sei_uuid[H265E_UUID_LENGTH] = {
        0x67, 0xfc, 0x6a, 0x3c, 0xd8, 0x5c, 0x44, 0x1e,
        0x87, 0xfb, 0x3f, 0xab, 0xec, 0xb3, 0x36, 0x77
    };

    h265e_nals_init(info);
    h265e_stream_init(&info->stream);

    info->sei_buf = mpp_calloc_size(RK_U8, H265E_SEI_BUF_SIZE);
    memcpy(info->sei_buf, h265e_sei_uuid, H265E_UUID_LENGTH);

    return MPP_OK;
}

MPP_RET h265e_deinit_extra_info(void *extra_info)
{
    H265eExtraInfo *info = (H265eExtraInfo *)extra_info;
    h265e_stream_deinit(&info->stream);
    h265e_nals_deinit(info);

    MPP_FREE(info->sei_buf);

    return MPP_OK;
}

MPP_RET h265e_set_extra_info(H265eCtx *ctx)
{
    H265eExtraInfo *info = (H265eExtraInfo *)ctx->extra_info;
    H265eSps *sps = &ctx->sps;
    H265ePps *pps = &ctx->pps;
    H265eVps *vps = &ctx->vps;

    h265e_dbg_func("enter\n");
    info->nal_num = 0;
    h265e_stream_reset(&info->stream);


    h265e_nal_start(info, NAL_VPS, H265_NAL_PRIORITY_HIGHEST);
    h265e_set_vps(ctx, vps);
    h265e_vps_write(vps, &info->stream);
    h265e_nal_end(info);

    h265e_nal_start(info, NAL_SPS, H265_NAL_PRIORITY_HIGHEST);
    h265e_set_sps(ctx, sps, vps);
    h265e_sps_write(sps, &info->stream);
    h265e_nal_end(info);

    h265e_nal_start(info, NAL_PPS, H265_NAL_PRIORITY_HIGHEST);
    h265e_set_pps(ctx, pps, sps);
    h265e_pps_write(pps, sps, &info->stream);
    h265e_nal_end(info);

    h265e_encapsulate_nals(info);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

RK_U32 h265e_data_to_sei(void *dst, RK_U8 uuid[16], const void *payload, RK_S32 size)
{
    H265eNal sei_nal;
    H265eStream stream;

    h265e_dbg_func("enter\n");

    h265e_stream_init(&stream);
    memset(&sei_nal, 0 , sizeof(H265eNal));

    sei_nal.i_type = NAL_SEI_PREFIX;
    sei_nal.p_payload = &stream.buf[stream.enc_stream.byte_cnt];

    h265e_sei_write(&stream, uuid, payload, size, H265_SEI_USER_DATA_UNREGISTERED);

    RK_U8 *end = &stream.buf[stream.enc_stream.byte_cnt];
    sei_nal.i_payload = (RK_S32)(end - sei_nal.p_payload);

    h265e_nal_encode(dst, &sei_nal);

    h265e_stream_deinit(&stream);

    h265e_dbg_func("leave\n");
    return sei_nal.i_payload;
}

MPP_RET h265e_get_extra_info(H265eCtx *ctx, MppPacket pkt_out)
{
    RK_S32 k = 0;
    size_t offset = 0;

    if (pkt_out == NULL)
        return MPP_NOK;

    h265e_dbg_func("enter\n");

    H265eExtraInfo *src = (H265eExtraInfo *)ctx->extra_info;

    for (k = 0; k < src->nal_num; k++) {
        h265e_dbg(H265E_DBG_HEADER, "get extra info nal type %d, size %d bytes",
                  src->nal[k].i_type, src->nal[k].i_payload);
        mpp_packet_write(pkt_out, offset, src->nal[k].p_payload, src->nal[k].i_payload);
        offset += src->nal[k].i_payload;
    }
    mpp_packet_set_length(pkt_out, offset);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}
