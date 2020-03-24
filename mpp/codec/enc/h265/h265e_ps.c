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

#define MODULE_TAG "h265e_ps"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "h265e_ps.h"

MPP_RET h265e_set_vps(H265eCtx *ctx, H265eVps *vps)
{
    RK_S32 i;
    MppEncH265Cfg *codec = &ctx->cfg->codec.h265;
    ProfileTierLevel *profileTierLevel = &vps->m_ptl.m_generalPTL;

    vps->m_VPSId = 0;
    vps->m_maxTLayers = 1;
    vps->m_maxLayers = 1;
    vps->m_bTemporalIdNestingFlag = 1;
    vps->m_numHrdParameters = 0;
    vps->m_maxNuhReservedZeroLayerId = 0;
    vps->m_hrdParameters = NULL;
    vps->m_hrdOpSetIdx = NULL;
    vps->m_cprmsPresentFlag = NULL;
    for (i = 0; i < MAX_SUB_LAYERS; i++) {
        vps->m_numReorderPics[i] =  0;
        vps->m_maxDecPicBuffering[i] = MPP_MIN(MAX_REFS, MPP_MAX((vps->m_numReorderPics[i] + 3), codec->num_ref) + vps->m_numReorderPics[i]);
        vps->m_maxLatencyIncrease[i] = 0;
    }
    memset(profileTierLevel->m_profileCompatibilityFlag, 0, sizeof(profileTierLevel->m_profileCompatibilityFlag));
    memset(vps->m_ptl.m_subLayerProfilePresentFlag, 0, sizeof(vps->m_ptl.m_subLayerProfilePresentFlag));
    memset(vps->m_ptl.m_subLayerLevelPresentFlag,   0, sizeof(vps->m_ptl.m_subLayerLevelPresentFlag));
    profileTierLevel->m_profileSpace = 0;
    profileTierLevel->m_levelIdc = codec->level;
    profileTierLevel->m_tierFlag = codec->tier ? 1 : 0;
    profileTierLevel->m_profileIdc = codec->profile;

    profileTierLevel->m_profileCompatibilityFlag[codec->profile] = 1;
    profileTierLevel->m_profileCompatibilityFlag[2] = 1;

    profileTierLevel->m_progressiveSourceFlag = 1;
    profileTierLevel->m_nonPackedConstraintFlag = 0;
    profileTierLevel->m_frameOnlyConstraintFlag = 0;
    return MPP_OK;
}

MPP_RET h265e_set_sps(H265eCtx *ctx, H265eSps *sps, H265eVps *vps)
{
    RK_U32 i, c;
    MppEncH265Cfg *codec = &ctx->cfg->codec.h265;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    MppEncRcCfg *rc = &ctx->cfg->rc;
    MppEncH265VuiCfg *vui = &codec->vui;
    RK_S32 i_timebase_num = 1;
    RK_S32 i_timebase_den = rc->fps_out_num / rc->fps_out_denorm;
    RK_U8  convertToBit[MAX_CU_SIZE + 1];
    RK_U32 maxCUDepth, minCUDepth, addCUDepth;
    RK_S32 pad[2] = {0};
    RK_S32 minCUSize, log2MinCUSize;
    RK_S32 tuQTMinLog2Size = 2, tuQTMaxLog2Size;

    memset(convertToBit, -1, sizeof(convertToBit));
    c = 0;
    for (i = 4; i <= MAX_CU_SIZE; i *= 2) {
        convertToBit[i] = c;
        c++;
    }

    maxCUDepth = (uint32_t)convertToBit[codec->max_cu_size];

    minCUDepth = (codec->max_cu_size >> (maxCUDepth - 1));

    tuQTMaxLog2Size = convertToBit[codec->max_cu_size] + 2 - 1;

    addCUDepth = 0;
    while ((RK_U32)(codec->max_cu_size >> maxCUDepth) > (1u << (tuQTMinLog2Size + addCUDepth))) {
        addCUDepth++;
    }

    maxCUDepth += addCUDepth;
    addCUDepth++;

    if ((prep->width % minCUDepth) != 0) {
        RK_U32 padsize = 0;
        RK_U32 rem = prep->width % minCUDepth;
        padsize = minCUDepth - rem;
        pad[0] = padsize; //pad width

        /* set the confirmation window offsets  */
        sps->m_conformanceWindow.m_enabledFlag = 1;
        sps->m_conformanceWindow.m_winRightOffset = pad[0];
    }

    if ((prep->height % minCUDepth) != 0) {
        RK_U32 padsize = 0;
        RK_U32 rem = prep->height % minCUDepth;
        padsize = minCUDepth - rem;
        pad[1] = padsize; //pad height

        /* set the confirmation window offsets  */
        sps->m_conformanceWindow.m_enabledFlag = 1;
        sps->m_conformanceWindow.m_winBottomOffset = pad[1];
    }
    // pad[0] = p->sourceWidth - p->oldSourceWidth;
    // pad[1] = p->sourceHeight - p->oldSourceHeight;

    sps->m_SPSId = 0;
    sps->m_VPSId = 0;
    sps->m_chromaFormatIdc = 0x1; //RKVE_CSP2_I420;
    sps->m_maxTLayers = 1;
    sps->m_picWidthInLumaSamples = prep->width + pad[0];
    sps->m_picHeightInLumaSamples = prep->height + pad[1];
    sps->m_log2MinCodingBlockSize = 0;
    sps->m_log2DiffMaxMinCodingBlockSize = 0 ;
    sps->m_maxCUSize = codec->max_cu_size;
    sps->m_maxCUDepth = maxCUDepth;
    sps->m_RPSList.m_numberOfReferencePictureSets = 0;
    sps->m_RPSList.m_referencePictureSets = NULL;

    minCUSize = sps->m_maxCUSize >> (sps->m_maxCUDepth - addCUDepth);
    log2MinCUSize = 0;
    while (minCUSize > 1) {
        minCUSize >>= 1;
        log2MinCUSize++;
    }
    sps->m_log2MinCodingBlockSize = log2MinCUSize;
    sps->m_log2DiffMaxMinCodingBlockSize = sps->m_maxCUDepth - addCUDepth;

    sps->m_pcmLog2MaxSize = 5;
    sps->m_usePCM = 0;
    sps->m_pcmLog2MinSize = 3;


    sps->m_bLongTermRefsPresent = 0;
    sps->m_quadtreeTULog2MaxSize =  tuQTMaxLog2Size;
    sps->m_quadtreeTULog2MinSize = tuQTMinLog2Size;
    sps->m_quadtreeTUMaxDepthInter = 1;     //tuQTMaxInterDepth
    sps->m_quadtreeTUMaxDepthIntra = 1;     //tuQTMaxIntraDepth
    sps->m_useLossless = 0;

    for (i = 0; i < (maxCUDepth - addCUDepth); i++) {
        sps->m_iAMPAcc[i] = codec->amp_enable;
    }
    sps->m_useAMP = codec->amp_enable;
    for (i = maxCUDepth - addCUDepth; i < maxCUDepth; i++) {
        sps->m_iAMPAcc[i] = codec->amp_enable;
    }

    sps->m_bitDepthY = 8;
    sps->m_bitDepthC = 8;
    sps->m_qpBDOffsetY = 0;
    sps->m_qpBDOffsetC = 0;

    sps->m_bUseSAO = codec->sao_enable;

    sps->m_maxTLayers = 1;

    sps->m_bTemporalIdNestingFlag = 1;

    for (i = 0; i < sps->m_maxTLayers; i++) {
        sps->m_maxDecPicBuffering[i] = vps->m_maxDecPicBuffering[i];
        sps->m_numReorderPics[i] = vps->m_numReorderPics[i];
    }

    sps->m_pcmBitDepthLuma = 8;
    sps->m_pcmBitDepthChroma = 8;

    sps->m_bPCMFilterDisableFlag = 0;
    sps->m_scalingListEnabledFlag = codec->trans_cfg.defalut_ScalingList_enable == 0 ? 0 : 1;

    sps->m_bitsForPOC = 16;
    sps->m_numLongTermRefPicSPS = 0;
    sps->m_maxTrSize = 32;
    sps->m_bLongTermRefsPresent = 0;
    sps->m_TMVPFlagsPresent = codec->tmvp_enable;
    sps->m_useStrongIntraSmoothing = codec->cu_cfg.strong_intra_smoothing_enabled_flag;
    if (codec->ref_cfg.num_lt_ref_pic > 0 || ctx->dpbcfg.nLongTerm) {
        h265e_dbg_ps("ctx->dpbcfg.nLongTerm = %d", ctx->dpbcfg.nLongTerm);
        if (codec->ref_cfg.num_lt_ref_pic == 0) {
            codec->ref_cfg.num_lt_ref_pic = 1;
        }
        sps->m_numLongTermRefPicSPS = codec->ref_cfg.num_lt_ref_pic;
        sps->m_bLongTermRefsPresent = 1;
        sps->m_TMVPFlagsPresent = 0;
        codec->tmvp_enable = 0;
    }

    sps->m_ptl = &vps->m_ptl;
    sps->m_vuiParametersPresentFlag = 1;
    if (sps->m_vuiParametersPresentFlag) {
        sps->vui.m_aspectRatioInfoPresentFlag = 0;
        sps->vui.m_aspectRatioIdc = 0;
        sps->vui.m_sarWidth = 0;
        sps->vui.m_sarHeight = 0;
        sps->vui.m_overscanInfoPresentFlag = 0;
        sps->vui.m_overscanAppropriateFlag = 0;
        sps->vui.m_videoSignalTypePresentFlag = 0;
        sps->vui.m_videoFormat = 5;
        sps->vui.m_videoFullRangeFlag = 0;
        sps->vui.m_colourDescriptionPresentFlag = 0;
        sps->vui.m_colourPrimaries = 2;
        sps->vui.m_transferCharacteristics = 2;
        sps->vui.m_matrixCoefficients = 2;
        sps->vui.m_chromaLocInfoPresentFlag = 0;
        sps->vui.m_chromaSampleLocTypeTopField = 0;
        sps->vui.m_chromaSampleLocTypeBottomField = 0;
        sps->vui.m_neutralChromaIndicationFlag = 0;
        sps->vui.m_fieldSeqFlag = 0;
        sps->vui.m_frameFieldInfoPresentFlag = 0;
        sps->vui.m_hrdParametersPresentFlag = 0;
        sps->vui.m_bitstreamRestrictionFlag = 0;
        sps->vui.m_tilesFixedStructureFlag = 0;
        sps->vui.m_motionVectorsOverPicBoundariesFlag = 1;
        sps->vui.m_restrictedRefPicListsFlag = 1;
        sps->vui.m_minSpatialSegmentationIdc = 0;
        sps->vui.m_maxBytesPerPicDenom = 2;
        sps->vui.m_maxBitsPerMinCuDenom = 1;
        sps->vui.m_log2MaxMvLengthHorizontal = 15;
        sps->vui.m_log2MaxMvLengthVertical = 15;
        if (vui->vui_aspect_ratio) {
            sps->vui.m_aspectRatioInfoPresentFlag = !!vui->vui_aspect_ratio;
            sps->vui.m_aspectRatioIdc = vui->vui_aspect_ratio;
        }
        if (vui->full_range) {
            sps->vui.m_videoFullRangeFlag = 1;
        }
        sps->vui.m_timingInfo.m_timingInfoPresentFlag = 1;
        sps->vui.m_timingInfo.m_numUnitsInTick = i_timebase_num;
        sps->vui.m_timingInfo.m_timeScale = i_timebase_den;
    }



    for (i = 0; i < MAX_SUB_LAYERS; i++) {
        sps->m_maxLatencyIncrease[i] = 0;
    }
    //  sps->m_scalingList = NULL;
    memset(sps->m_ltRefPicPocLsbSps, 0, sizeof(sps->m_ltRefPicPocLsbSps));
    memset(sps->m_usedByCurrPicLtSPSFlag, 0, sizeof(sps->m_usedByCurrPicLtSPSFlag));
    return 0;
}

MPP_RET h265e_set_pps(H265eCtx  *ctx, H265ePps *pps, H265eSps *sps)
{
    MppEncH265Cfg *codec = &ctx->cfg->codec.h265;
    MppEncRcCfg *rc = &ctx->cfg->rc;
    pps->m_bConstrainedIntraPred = 0;
    pps->m_PPSId = 0;
    pps->m_SPSId = 0;
    pps->m_picInitQPMinus26 = 0;
    pps->m_useDQP = 0;
    if (rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        pps->m_useDQP = 1;
        pps->m_maxCuDQPDepth = 0;
        pps->m_minCuDQPSize = (sps->m_maxCUSize >> pps->m_maxCuDQPDepth);
    }


    pps->m_bSliceChromaQpFlag = (!!codec->trans_cfg.cb_qp_offset) || (!!codec->trans_cfg.cr_qp_offset);

    pps->m_sps = sps;
    if (pps->m_bSliceChromaQpFlag) {
        pps->m_chromaCbQpOffset = codec->trans_cfg.cb_qp_offset;
        pps->m_chromaCrQpOffset = codec->trans_cfg.cr_qp_offset;
    } else {
        pps->m_chromaCbQpOffset = 0;
        pps->m_chromaCrQpOffset = 0;
    }

    pps->m_entropyCodingSyncEnabledFlag = 0;
    pps->m_bUseWeightPred = 0;
    pps->m_useWeightedBiPred = 0;
    pps->m_outputFlagPresentFlag = 0;
    pps->m_signHideFlag = 0;
    pps->m_picInitQPMinus26 = codec->intra_qp - 26;
    pps->m_LFCrossSliceBoundaryFlag = codec->slice_cfg.loop_filter_across_slices_enabled_flag;
    pps->m_deblockingFilterControlPresentFlag = !codec->dblk_cfg.slice_deblocking_filter_disabled_flag;
    if (pps->m_deblockingFilterControlPresentFlag) {
        pps->m_deblockingFilterOverrideEnabledFlag = 0;
        pps->m_picDisableDeblockingFilterFlag = (!!codec->dblk_cfg.slice_beta_offset_div2) ||
                                                (!!codec->dblk_cfg.slice_tc_offset_div2);
        if (!pps->m_picDisableDeblockingFilterFlag) {
            pps->m_deblockingFilterBetaOffsetDiv2 = codec->dblk_cfg.slice_beta_offset_div2;
            pps->m_deblockingFilterTcOffsetDiv2 = codec->dblk_cfg.slice_tc_offset_div2;
        }
    } else {
        pps->m_deblockingFilterOverrideEnabledFlag = 0;
        pps->m_picDisableDeblockingFilterFlag = 0;
        pps->m_deblockingFilterBetaOffsetDiv2 = 0;
        pps->m_deblockingFilterTcOffsetDiv2 = 0;
    }

    if (ctx->dpbcfg.vgop_size > 1) {
        pps->m_listsModificationPresentFlag = 1;
    }

    pps->m_log2ParallelMergeLevelMinus2 = 0;
    pps->m_numRefIdxL0DefaultActive = 1;
    pps->m_numRefIdxL1DefaultActive = 1;
    pps->m_transquantBypassEnableFlag = codec->trans_cfg.transquant_bypass_enabled_flag;
    pps->m_useTransformSkip = codec->trans_cfg.transform_skip_enabled_flag;

    pps->m_entropyCodingSyncEnabledFlag = 0;
    pps->m_loopFilterAcrossTilesEnabledFlag = 1;
    pps->m_signHideFlag = 0;
    pps->m_cabacInitPresentFlag = codec->entropy_cfg.cabac_init_flag;
    pps->m_encCABACTableIdx = I_SLICE;
    pps->m_sliceHeaderExtensionPresentFlag = 0;
    pps->m_numExtraSliceHeaderBits = 0;
    return 0;
}
#if 0

void h265e_sei_pack2str(char *str, H265eCtx *ctx, RcSyntax *rc_syn)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH265Cfg *codec = &cfg->codec.h265;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    RK_U32 prep_change = prep->change & MPP_ENC_PREP_CFG_CHANGE_INPUT;
    RK_U32 codec_change = codec->change;
    RK_U32 rc_change = rc->change;
    RK_S32 len = H265E_SEI_BUF_SIZE - H265E_UUID_LENGTH;

    if (prep_change || codec_change || rc_change)
        H265E_HAL_SPRINT(str, len, "frm %d cfg: ", ctx->frame_cnt);

    /* prep cfg */
    if (prep_change) {
        H265E_HAL_SPRINT(str, len, "[prep] ");
        if (prep_change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            H265E_HAL_SPRINT(str, len, "w=%d ", prep->width);
            H265E_HAL_SPRINT(str, len, "h=%d ", prep->height);
            H265E_HAL_SPRINT(str, len, "fmt=%d ", prep->format);
            H265E_HAL_SPRINT(str, len, "h_strd=%d ", prep->hor_stride);
            H265E_HAL_SPRINT(str, len, "v_strd=%d ", prep->ver_stride);
        }
    }
#if 0
    /* codec cfg */
    if (codec_change) {
        H264E_HAL_SPRINT(str, len, "[codec] ");
        H264E_HAL_SPRINT(str, len, "profile=%d ", codec->profile);
        H264E_HAL_SPRINT(str, len, "level=%d ", codec->level);
        H264E_HAL_SPRINT(str, len, "b_cabac=%d ", codec->entropy_coding_mode);
        H264E_HAL_SPRINT(str, len, "cabac_idc=%d ", codec->cabac_init_idc);
        H264E_HAL_SPRINT(str, len, "t8x8=%d ", codec->transform8x8_mode);
        H264E_HAL_SPRINT(str, len, "constrain_intra=%d ", codec->constrained_intra_pred_mode);
        H264E_HAL_SPRINT(str, len, "dblk=%d:%d:%d ", codec->deblock_disable,
                         codec->deblock_offset_alpha, codec->deblock_offset_beta);
        H264E_HAL_SPRINT(str, len, "cbcr_qp_offset=%d:%d ", codec->chroma_cb_qp_offset,
                         codec->chroma_cr_qp_offset);
        H264E_HAL_SPRINT(str, len, "qp_max=%d ", codec->qp_max);
        H264E_HAL_SPRINT(str, len, "qp_min=%d ", codec->qp_min);
        H264E_HAL_SPRINT(str, len, "qp_max_step=%d ", codec->qp_max_step);
    }
#endif
    /* rc cfg */
    if (rc_change) {
        H265E_HAL_SPRINT(str, len, "[rc] ");
        H265E_HAL_SPRINT(str, len, "fps_in=%d:%d:%d ", rc->fps_in_num, rc->fps_in_denorm, rc->fps_in_flex);
        H265E_HAL_SPRINT(str, len, "fps_out=%d:%d:%d ", rc->fps_out_num, rc->fps_out_denorm, rc->fps_out_flex);
        H265E_HAL_SPRINT(str, len, "gop=%d ", rc->gop);
    }
    /* record detailed RC parameter
     * Start to write parameter when the first frame is encoded,
     * because we can get all parameter only when it's encoded.
     */
    if (rc_syn && rc_syn->rc_head && (ctx->frame_cnt > 0)) {
        RecordNode *pos, *m;
        MppLinReg *lin_reg;

        list_for_each_entry_safe(pos, m, rc_syn->rc_head, RecordNode, list) {
            if (ctx->frame_cnt == pos->frm_cnt) {
                H265E_HAL_SPRINT(str, len, "[frm %d]detailed param ", pos->frm_cnt);
                H265E_HAL_SPRINT(str, len, "tgt_bits=%d:%d:%d:%d ",
                                 pos->tgt_bits, pos->real_bits,
                                 pos->bit_min, pos->bit_max);
                H265E_HAL_SPRINT(str, len, "tgt_qp=%d:%d:%d:%d ",
                                 pos->set_qp, pos->real_qp,
                                 pos->qp_min, pos->qp_max);

                H265E_HAL_SPRINT(str, len, "per_pic=%d intra=%d inter=%d ",
                                 pos->bits_per_pic,
                                 pos->bits_per_intra, pos->bits_per_inter);
                H265E_HAL_SPRINT(str, len, "acc_intra=%d inter=%d last_fps=%d ",
                                 pos->acc_intra_bits_in_fps,
                                 pos->acc_inter_bits_in_fps,
                                 pos->last_fps_bits);
                H265E_HAL_SPRINT(str, len, "qp_sum=%d sse_sum=%lld ",
                                 pos->qp_sum, pos->sse_sum);

                lin_reg = &pos->lin_reg;
                H265E_HAL_SPRINT(str, len, "size=%d n=%d i=%d ",
                                 lin_reg->size, lin_reg->n, lin_reg->i);
                H265E_HAL_SPRINT(str, len, "a=%0.2f b=%0.2f c=%0.2f ",
                                 lin_reg->a, lin_reg->b, lin_reg->c);
                H265E_HAL_SPRINT(str, len, "weight_len=%d wlen=%d ",
                                 lin_reg->weight_mode, pos->wlen);

                /* frame type is intra */
                if (pos->frm_type == INTRA_FRAME)
                    H265E_HAL_SPRINT(str, len, "fps=%d gop=%d I=%0.2f ", pos->fps,
                                     pos->gop, pos->last_intra_percent);

                break;
            }
        }
    }
    /* frame type is intra */
    if (rc_syn && (hw_cfg->frame_type == 2)) {
        H265E_HAL_SPRINT(str, len, "[frm %d] ", ctx->frame_cnt);
        H265E_HAL_SPRINT(str, len, "rc_mode=%d ", rc->rc_mode);
        H265E_HAL_SPRINT(str, len, "quality=%d ", rc->quality);
        H265E_HAL_SPRINT(str, len, "bps=%d:%d:%d ", rc->bps_target, rc->bps_min, rc->bps_max);
    }
}
#endif
