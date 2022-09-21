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
#include "mpp_soc.h"
#include "mpp_common.h"

#include "h265e_ps.h"

#define MAX_UINT        0xFFFFFFFFU

typedef struct H265levelspec_t {
    RK_U32 maxLumaSamples;
    RK_U32 maxLumaSamplesPerSecond;
    RK_U32 maxBitrateMain;
    RK_U32 maxBitrateHigh;
    RK_U32 maxCpbSizeMain;
    RK_U32 maxCpbSizeHigh;
    RK_U32 minCompressionRatio;
    RK_S32 levelEnum;
    const char* name;
    RK_S32 levelIdc;
} H265levelspec;

H265levelspec levels[] = {
    { 36864,    552960,     128,      MAX_UINT, 350,    MAX_UINT, 2, H265_LEVEL1,   "1",   10 },
    { 122880,   3686400,    1500,     MAX_UINT, 1500,   MAX_UINT, 2, H265_LEVEL2,   "2",   20 },
    { 245760,   7372800,    3000,     MAX_UINT, 3000,   MAX_UINT, 2, H265_LEVEL2_1, "2.1", 21 },
    { 552960,   16588800,   6000,     MAX_UINT, 6000,   MAX_UINT, 2, H265_LEVEL3,   "3",   30 },
    { 983040,   33177600,   10000,    MAX_UINT, 10000,  MAX_UINT, 2, H265_LEVEL3_1, "3.1", 31 },
    { 2228224,  66846720,   12000,    30000,    12000,  30000,    4, H265_LEVEL4,   "4",   40 },
    { 2228224,  133693440,  20000,    50000,    20000,  50000,    4, H265_LEVEL4_1, "4.1", 41 },
    { 8912896,  267386880,  25000,    100000,   25000,  100000,   6, H265_LEVEL5,   "5",   50 },
    { 8912896,  534773760,  40000,    160000,   40000,  160000,   8, H265_LEVEL5_1, "5.1", 51 },
    { 8912896,  1069547520, 60000,    240000,   60000,  240000,   8, H265_LEVEL5_2, "5.2", 52 },
    { 35651584, 1069547520, 60000,    240000,   60000,  240000,   8, H265_LEVEL6,   "6",   60 },
    { 35651584, 2139095040, 120000,   480000,   120000, 480000,   8, H265_LEVEL6_1, "6.1", 61 },
    { 35651584, 4278190080U, 240000,  800000,   240000, 800000,   6, H265_LEVEL6_2, "6.2", 62 },
    { MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, MAX_UINT, 1, H265_LEVEL8_5, "8.5", 85 },
};

void init_zscan2raster(RK_S32 maxDepth, RK_S32 depth, RK_U32 startVal, RK_U32** curIdx)
{
    RK_S32 stride = 1 << (maxDepth - 1);
    if (depth == maxDepth) {
        (*curIdx)[0] = startVal;
        (*curIdx)++;
    } else {
        RK_S32 step = stride >> depth;
        init_zscan2raster(maxDepth, depth + 1, startVal,                        curIdx);
        init_zscan2raster(maxDepth, depth + 1, startVal + step,                 curIdx);
        init_zscan2raster(maxDepth, depth + 1, startVal + step * stride,        curIdx);
        init_zscan2raster(maxDepth, depth + 1, startVal + step * stride + step, curIdx);
    }
}

void init_raster2zscan(RK_U32 maxCUSize, RK_U32 maxDepth, RK_U32 *raster2zscan, RK_U32 *zscan2raster)
{
    RK_U32  unitSize = maxCUSize  >> (maxDepth - 1);
    RK_U32  numPartInCUSize  = (RK_U32)maxCUSize / unitSize;
    RK_U32  i;

    for ( i = 0; i < numPartInCUSize * numPartInCUSize; i++) {
        raster2zscan[zscan2raster[i]] = i;
    }
}

void init_raster2pelxy(RK_U32 maxCUSize, RK_U32 maxDepth, RK_U32 *raster2pelx, RK_U32 *raster2pely)
{
    RK_U32 i;

    RK_U32* tempx = &raster2pelx[0];
    RK_U32* tempy = &raster2pely[0];

    RK_U32  unitSize  = maxCUSize >> (maxDepth - 1);

    RK_U32  numPartInCUSize = maxCUSize / unitSize;

    tempx[0] = 0;
    tempx++;
    for (i = 1; i < numPartInCUSize; i++) {
        tempx[0] = tempx[-1] + unitSize;
        tempy++;
    }

    for (i = 1; i < numPartInCUSize; i++) {
        memcpy(tempx, tempx - numPartInCUSize, sizeof(RK_U32) * numPartInCUSize);
        tempx += numPartInCUSize;
    }

    for (i = 1; i < numPartInCUSize * numPartInCUSize; i++) {
        tempy[i] = (i / numPartInCUSize) * unitSize;
    }
}

MPP_RET h265e_set_vps(H265eCtx *ctx, H265eVps *vps)
{
    RK_S32 i;
    MppEncH265Cfg *codec = &ctx->cfg->codec.h265;
    ProfileTierLevel *profileTierLevel = &vps->m_ptl.m_generalPTL;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_U32 maxlumas = prep->width * prep->height;
    RK_S32 level_idc = H265_LEVEL_NONE;

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
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(levels); i++) {
        if (levels[i].maxLumaSamples >= maxlumas) {
            level_idc = levels[i].levelEnum;
            break;
        }
    }

    profileTierLevel->m_profileSpace = 0;
    if (codec->level < level_idc) {
        profileTierLevel->m_levelIdc = level_idc;
    } else {
        profileTierLevel->m_levelIdc = codec->level;
    }
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
    MppEncRefCfg ref_cfg = ctx->cfg->ref_cfg;
    MppEncH265VuiCfg *vui = &codec->vui;
    RK_S32 i_timebase_num = rc->fps_out_denorm;
    RK_S32 i_timebase_den = rc->fps_out_num;
    RK_U8  convertToBit[MAX_CU_SIZE + 1];
    RK_U32 maxCUDepth, minCUDepth, addCUDepth;
    RK_S32 pad[2] = {0};
    RK_S32 minCUSize, log2MinCUSize;
    RK_S32 tuQTMinLog2Size = 2, tuQTMaxLog2Size;
    MppEncCpbInfo *cpb_info = mpp_enc_ref_cfg_get_cpb_info(ref_cfg);
    RK_U32 *tmp = &sps->zscan2raster[0];

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
    init_zscan2raster(maxCUDepth + 1, 1, 0, &tmp );
    init_raster2zscan(codec->max_cu_size, maxCUDepth + 1, &sps->raster2zscan[0], &sps->zscan2raster[0]);
    init_raster2pelxy(codec->max_cu_size, maxCUDepth + 1, &sps->raster2pelx[0], &sps->raster2pely[0]);

    memset(&sps->m_conformanceWindow, 0, sizeof(H265eCropInfo));
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
    sps->m_addCUDepth = addCUDepth;
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
    if (cpb_info->max_lt_cnt) {
        sps->m_numLongTermRefPicSPS = cpb_info->max_lt_cnt;
        sps->m_bLongTermRefsPresent = 1;
        sps->m_TMVPFlagsPresent = 0;
        codec->tmvp_enable = 0;
    } else if (cpb_info->max_st_tid) {
        sps->m_TMVPFlagsPresent = 0;
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
        sps->vui.m_videoFormat = MPP_FRAME_VIDEO_FMT_UNSPECIFIED;
        if (prep->range == MPP_FRAME_RANGE_JPEG) {
            sps->vui.m_videoFullRangeFlag = 1;
            sps->vui.m_videoSignalTypePresentFlag = 1;
        }

        if ((prep->colorprim <= MPP_FRAME_PRI_JEDEC_P22 &&
             prep->colorprim != MPP_FRAME_PRI_UNSPECIFIED) ||
            (prep->colortrc <= MPP_FRAME_TRC_ARIB_STD_B67 &&
             prep->colortrc != MPP_FRAME_TRC_UNSPECIFIED) ||
            (prep->color <= MPP_FRAME_SPC_ICTCP &&
             prep->color != MPP_FRAME_SPC_UNSPECIFIED)) {
            sps->vui.m_videoSignalTypePresentFlag = 1;
            sps->vui.m_colourDescriptionPresentFlag = 1;
            sps->vui.m_colourPrimaries = prep->colorprim;
            sps->vui.m_transferCharacteristics = prep->colortrc;
            sps->vui.m_matrixCoefficients = prep->color;
        }

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

    pps->m_sps = sps;
    pps->m_bSliceChromaQpFlag = 0;
    pps->m_chromaCbQpOffset = codec->trans_cfg.cb_qp_offset;
    /* rkvenc hw only support one color qp offset. Set all offset to cb offset*/
    pps->m_chromaCrQpOffset = codec->trans_cfg.cb_qp_offset;

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
        pps->m_picDisableDeblockingFilterFlag = 0;
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

    pps->m_listsModificationPresentFlag = 1;
    pps->m_log2ParallelMergeLevelMinus2 = 0;
    pps->m_numRefIdxL0DefaultActive = 1;
    pps->m_numRefIdxL1DefaultActive = 1;
    pps->m_transquantBypassEnableFlag = codec->trans_cfg.transquant_bypass_enabled_flag;
    pps->m_useTransformSkip = codec->trans_cfg.transform_skip_enabled_flag;

    pps->m_entropyCodingSyncEnabledFlag = 0;
    pps->m_signHideFlag = 0;
    pps->m_cabacInitPresentFlag = codec->entropy_cfg.cabac_init_flag;
    pps->m_encCABACTableIdx = I_SLICE;
    pps->m_sliceHeaderExtensionPresentFlag = 0;
    pps->m_numExtraSliceHeaderBits = 0;
    pps->m_tiles_enabled_flag = 0;
    pps->m_bTileUniformSpacing = 0;
    pps->m_nNumTileRowsMinus1 = 0;
    pps->m_nNumTileColumnsMinus1 = 0;
    pps->m_loopFilterAcrossTilesEnabledFlag = 1;
    {
        const char *soc_name = mpp_get_soc_name();
        /* check tile support on rk3566 and rk3568 */
        if (strstr(soc_name, "rk3566") || strstr(soc_name, "rk3568")) {
            pps->m_nNumTileColumnsMinus1 = (sps->m_picWidthInLumaSamples - 1) / 1920 ;
        } else if (strstr(soc_name, "rk3588")) {
            if (sps->m_picWidthInLumaSamples > 8192) {
                /* 4 tile for over 8k encoding */
                pps->m_nNumTileColumnsMinus1 = 3;
            } else if (sps->m_picWidthInLumaSamples > 4096) {
                /* 2 tile for 4k ~ 8k encoding */
                pps->m_nNumTileColumnsMinus1 = 1;
            } else {
                /* 1 tile for less 4k encoding and use 2 tile on auto tile enabled */
                pps->m_nNumTileColumnsMinus1 = codec->auto_tile ? 1 : 0;
            }
        }
        if (pps->m_nNumTileColumnsMinus1) {
            pps->m_tiles_enabled_flag = 1;
            pps->m_bTileUniformSpacing = 1;
            pps->m_loopFilterAcrossTilesEnabledFlag = 1;
        }
    }

    return 0;
}
