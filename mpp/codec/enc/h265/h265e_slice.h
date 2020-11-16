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

#ifndef __H265E_SLICE_H__
#define __H265E_SLICE_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_list.h"
#include "h265e_dpb.h"
#include "h265_syntax.h"
#include "h265e_enctropy.h"
#include "h265e_context_table.h"

#define MIN_PU_SIZE             4
#define MIN_TU_SIZE             4
#define MAX_NUM_SPU_W           (64 / MIN_PU_SIZE) // maximum number of SPU in horizontal line

/*
 * For H.265 encoder slice header process.
 * Remove some syntax that encoder not supported.
 * Field, mbaff, B slice are not supported yet.
 */
typedef struct  H265eDpbFrm_t       H265eDpbFrm;

typedef struct DataCu_t {
    RK_U8   m_cuSize[256];
    RK_U8   m_cuDepth[256];
    RK_U32  pixelX;
    RK_U32  pixelY;
    RK_U32  mb_w;
    RK_U32  mb_h;
    RK_U32  cur_addr;
} DataCu;

typedef struct H265eReferencePictureSet_e {
    RK_S32  m_deltaRIdxMinus1;
    RK_S32  m_deltaRPS;
    RK_S32  m_numRefIdc;
    RK_S32  m_refIdc[MAX_REFS + 1];

    // Parameters for long term references
    RK_U32  check_lt_msb[MAX_REFS];
    RK_S32  m_pocLSBLT[MAX_REFS];
    RK_S32  m_deltaPOCMSBCycleLT[MAX_REFS];
    RK_U32  m_deltaPocMSBPresentFlag[MAX_REFS];

    RK_S32  m_numberOfPictures;
    RK_S32  num_negative_pic;
    RK_S32  num_positive_pic;
    RK_S32  delta_poc[MAX_REFS];
    RK_U32  m_used[MAX_REFS];
    RK_S32  poc[MAX_REFS];
    RK_S32  m_RealPoc[MAX_REFS];

    RK_U32  m_interRPSPrediction;
    RK_S32  num_long_term_pic;          // Zero when disabled
} H265eReferencePictureSet;

typedef struct H265eRPSList_e {
    RK_S32  m_numberOfReferencePictureSets;
    H265eReferencePictureSet *m_referencePictureSets;
} H265eRPSList;

typedef struct H265eRefPicListModification_e {
    RK_U32 m_refPicListModificationFlagL0;
    RK_U32 m_refPicListModificationFlagL1;
    RK_U32 m_RefPicSetIdxL0[REF_PIC_LIST_NUM_IDX];
    RK_U32 m_RefPicSetIdxL1[REF_PIC_LIST_NUM_IDX];
} H265eRefPicListModification;

typedef struct ProfileTierLevel_e {
    RK_S32     m_profileSpace;
    RK_S32     m_tierFlag;
    RK_S32     m_profileIdc;
    RK_S32     m_profileCompatibilityFlag[32];
    RK_S32     m_levelIdc;
    RK_S32     m_progressiveSourceFlag;
    RK_S32     m_interlacedSourceFlag;
    RK_S32     m_nonPackedConstraintFlag;
    RK_S32     m_frameOnlyConstraintFlag;
} ProfileTierLevel;

typedef struct  H265ePTL_e {
    ProfileTierLevel    m_generalPTL;
    ProfileTierLevel    m_subLayerPTL[6];    // max. value of max_sub_layers_minus1 is 6
    RK_S32              m_subLayerProfilePresentFlag[6];
    RK_S32              m_subLayerLevelPresentFlag[6];
} H265ePTL;

typedef struct TimeingInfo_e {
    RK_U32  m_timingInfoPresentFlag;
    RK_U32  m_numUnitsInTick;
    RK_U32  m_timeScale;
    RK_U32  m_pocProportionalToTimingFlag;
    RK_U32  m_numTicksPocDiffOneMinus1;
} TimingInfo;

typedef struct H265HrdSubLayerInfo_e {
    RK_U32  fixedPicRateFlag;
    RK_U32  fixedPicRateWithinCvsFlag;
    RK_U32  picDurationInTcMinus1;
    RK_U32  lowDelayHrdFlag;
    RK_U32  cpbCntMinus1;
    RK_U32  bitRateValueMinus1[MAX_CPB_CNT][2];
    RK_U32  cpbSizeValue[MAX_CPB_CNT][2];
    RK_U32  ducpbSizeValue[MAX_CPB_CNT][2];
    RK_U32  cbrFlag[MAX_CPB_CNT][2];
    RK_U32  duBitRateValue[MAX_CPB_CNT][2];
} H265eHrdSubLayerInfo;

typedef struct H265eHrdParameters_e {
    RK_U32  m_nalHrdParametersPresentFlag;
    RK_U32  m_vclHrdParametersPresentFlag;
    RK_U32  m_subPicHrdParamsPresentFlag;
    RK_U32  m_tickDivisorMinus2;
    RK_U32  m_duCpbRemovalDelayLengthMinus1;
    RK_U32  m_subPicCpbParamsInPicTimingSEIFlag;
    RK_U32  m_dpbOutputDelayDuLengthMinus1;
    RK_U32  m_bitRateScale;
    RK_U32  m_cpbSizeScale;
    RK_U32  m_ducpbSizeScale;
    RK_U32  m_initialCpbRemovalDelayLengthMinus1;
    RK_U32  m_cpbRemovalDelayLengthMinus1;
    RK_U32  m_dpbOutputDelayLengthMinus1;
    H265eHrdSubLayerInfo m_HRD[MAX_SUB_LAYERS];
} H265eHrdParameters;

typedef struct H265eVps_e {
    RK_S32          m_VPSId;
    RK_U32          m_maxTLayers;
    RK_U32          m_maxLayers;
    RK_U32          m_bTemporalIdNestingFlag;

    RK_U32          m_numReorderPics[MAX_SUB_LAYERS];
    RK_U32          m_maxDecPicBuffering[MAX_SUB_LAYERS];
    RK_U32          m_maxLatencyIncrease[MAX_SUB_LAYERS];  // Really max latency increase plus 1 (value 0 expresses no limit)

    RK_U32          m_numHrdParameters;
    RK_U32          m_maxNuhReservedZeroLayerId;
    H265eHrdParameters   *m_hrdParameters;
    RK_U32          *m_hrdOpSetIdx;
    RK_U32          *m_cprmsPresentFlag;
    RK_U32          m_numOpSets;
    RK_U32          m_layerIdIncludedFlag[MAX_VPS_OP_SETS_PLUS1][MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1];

    H265ePTL        m_ptl;
    TimingInfo      m_timingInfo;
} H265eVps;

typedef struct H265eCropInfo_e {
    RK_U32          m_enabledFlag;
    RK_S32           m_winLeftOffset;
    RK_S32           m_winRightOffset;
    RK_S32           m_winTopOffset;
    RK_S32           m_winBottomOffset;
} H265eCropInfo;

typedef struct H265eVuiInfo_e {
    RK_U32  m_aspectRatioInfoPresentFlag;
    RK_S32  m_aspectRatioIdc;
    RK_S32  m_sarWidth;
    RK_S32  m_sarHeight;
    RK_U32  m_overscanInfoPresentFlag;
    RK_U32  m_overscanAppropriateFlag;
    RK_U32  m_videoSignalTypePresentFlag;
    RK_S32  m_videoFormat;
    RK_U32  m_videoFullRangeFlag;
    RK_U32  m_colourDescriptionPresentFlag;
    RK_S32  m_colourPrimaries;
    RK_S32  m_transferCharacteristics;
    RK_S32  m_matrixCoefficients;
    RK_U32  m_chromaLocInfoPresentFlag;
    RK_S32  m_chromaSampleLocTypeTopField;
    RK_S32  m_chromaSampleLocTypeBottomField;
    RK_U32  m_neutralChromaIndicationFlag;
    RK_U32  m_fieldSeqFlag;

    H265eCropInfo m_defaultDisplayWindow;
    RK_U32  m_frameFieldInfoPresentFlag;
    RK_U32  m_hrdParametersPresentFlag;
    RK_U32  m_bitstreamRestrictionFlag;
    RK_U32  m_tilesFixedStructureFlag;
    RK_U32  m_motionVectorsOverPicBoundariesFlag;
    RK_U32  m_restrictedRefPicListsFlag;
    RK_S32  m_minSpatialSegmentationIdc;
    RK_S32  m_maxBytesPerPicDenom;
    RK_S32  m_maxBitsPerMinCuDenom;
    RK_S32  m_log2MaxMvLengthHorizontal;
    RK_S32  m_log2MaxMvLengthVertical;
    H265eHrdParameters m_hrdParameters;
    TimingInfo m_timingInfo;
} H265eVuiInfo;

typedef struct H265eSps_e {

    RK_S32          m_SPSId;
    RK_S32          m_VPSId;
    RK_S32          m_chromaFormatIdc;
    RK_U32          m_colorPlaneFlag;
    RK_U32          m_maxTLayers;         // maximum number of temporal layers

    // Structure
    RK_U32          m_picWidthInLumaSamples;
    RK_U32          m_picHeightInLumaSamples;

    RK_S32          m_log2MinCodingBlockSize;
    RK_S32          m_log2DiffMaxMinCodingBlockSize;
    RK_U32          m_maxCUSize;
    RK_U32          m_maxCUDepth;
    RK_U32          m_addCUDepth;

    H265eCropInfo   m_conformanceWindow;

    H265eRPSList    m_RPSList;
    RK_U32          m_bLongTermRefsPresent;
    RK_U32          m_TMVPFlagsPresent;
    RK_S32          m_numReorderPics[MAX_SUB_LAYERS];

    // Tool list
    RK_U32          m_quadtreeTULog2MaxSize;
    RK_U32          m_quadtreeTULog2MinSize;
    RK_U32          m_quadtreeTUMaxDepthInter;
    RK_U32          m_quadtreeTUMaxDepthIntra;
    RK_U32          m_usePCM;
    RK_U32          m_pcmLog2MaxSize;
    RK_U32          m_pcmLog2MinSize;
    RK_U32          m_useAMP;

    // Parameter
    RK_S32          m_bitDepthY;
    RK_S32          m_bitDepthC;
    RK_S32          m_qpBDOffsetY;
    RK_S32          m_qpBDOffsetC;

    RK_U32          m_useLossless;

    RK_U32          m_pcmBitDepthLuma;
    RK_U32          m_pcmBitDepthChroma;
    RK_U32          m_bPCMFilterDisableFlag;

    RK_U32          m_bitsForPOC;
    RK_U32          m_numLongTermRefPicSPS;
    RK_U32          m_ltRefPicPocLsbSps[33];
    RK_U32          m_usedByCurrPicLtSPSFlag[33];

    // Max physical transform size
    RK_U32          m_maxTrSize;

    RK_S32          m_iAMPAcc[MAX_CU_DEPTH];
    RK_U32          m_bUseSAO;

    RK_U32          m_bTemporalIdNestingFlag; // temporal_id_nesting_flag

    RK_U32          m_scalingListEnabledFlag;
    RK_U32          m_scalingListPresentFlag;
    RK_U32          m_maxDecPicBuffering[MAX_SUB_LAYERS];

    RK_U32          m_maxLatencyIncrease[MAX_SUB_LAYERS]; // Really max latency increase plus 1 (value 0 expresses no limit)

    RK_U32          m_useDF;
    RK_U32          m_useStrongIntraSmoothing;

    RK_S32          m_vuiParametersPresentFlag;
    H265eVuiInfo    vui;
    H265ePTL        *m_ptl;
    RK_U32          zscan2raster[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
    RK_U32          raster2zscan[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
    RK_U32          raster2pelx[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
    RK_U32          raster2pely[MAX_NUM_SPU_W * MAX_NUM_SPU_W];
} H265eSps;

typedef struct H265ePps_e {
    RK_S32      m_PPSId;                  // pic_parameter_set_id
    RK_S32      m_SPSId;                  // seq_parameter_set_id
    RK_S32      m_picInitQPMinus26;
    RK_U32      m_useDQP;
    RK_U32      m_bConstrainedIntraPred;  // constrained_intra_pred_flag
    RK_U32      m_bSliceChromaQpFlag;     // slicelevel_chroma_qp_flag

    // access channel
    H265eSps   *m_sps;
    RK_U32      m_maxCuDQPDepth;
    RK_U32      m_minCuDQPSize;

    RK_S32      m_chromaCbQpOffset;
    RK_S32      m_chromaCrQpOffset;

    RK_U32      m_numRefIdxL0DefaultActive;
    RK_U32      m_numRefIdxL1DefaultActive;

    RK_U32      m_bUseWeightPred;         // Use of Weighting Prediction (P_SLICE)
    RK_U32      m_useWeightedBiPred;      // Use of Weighting Bi-Prediction (B_SLICE)
    RK_U32      m_outputFlagPresentFlag; // Indicates the presence of output_flag in slice header

    RK_U32      m_transquantBypassEnableFlag; // Indicates presence of cu_transquant_bypass_flag in CUs.
    RK_U32      m_useTransformSkip;
    RK_U32      m_entropyCodingSyncEnabledFlag; //!< Indicates the presence of wavefronts


    RK_S32      m_signHideFlag;
    RK_S32      m_tiles_enabled_flag;
    RK_U32      m_bTileUniformSpacing;
    RK_S32      m_nNumTileColumnsMinus1;
    RK_S32      m_nTileColumnWidthArray[33];
    RK_S32      m_nNumTileRowsMinus1;
    RK_S32      m_nTileRowHeightArray[128];
    RK_U32      m_loopFilterAcrossTilesEnabledFlag;

    RK_U32      m_cabacInitPresentFlag;
    RK_U32      m_encCABACTableIdx;         // Used to transmit table selection across slices

    RK_U32      m_sliceHeaderExtensionPresentFlag;
    RK_U32      m_deblockingFilterControlPresentFlag;
    RK_U32      m_LFCrossSliceBoundaryFlag;
    RK_U32      m_deblockingFilterOverrideEnabledFlag;
    RK_U32      m_picDisableDeblockingFilterFlag;
    RK_S32      m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    RK_S32      m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    RK_U32      m_scalingListPresentFlag;

//    TComScalingList* m_scalingList; //!< ScalingList class pointer
    RK_U32      m_listsModificationPresentFlag;
    RK_U32      m_log2ParallelMergeLevelMinus2;
    RK_S32      m_numExtraSliceHeaderBits;
} H265ePps;

typedef struct H265eSlice_e {

    RK_U32         m_saoEnabledFlag;
    RK_U32         m_saoEnabledFlagChroma; ///< SAO Cb&Cr enabled flag
    RK_S32         m_ppsId;                ///< picture parameter set ID
    RK_U32         m_picOutputFlag;        ///< pic_output_flag
    RK_S32         poc;
    RK_S32         gop_idx;
    RK_S32         last_idr;

    H265eReferencePictureSet *m_rps;
    H265eReferencePictureSet m_localRPS;
    RK_S32         m_bdIdx;
    H265eRefPicListModification m_RefPicListModification;
    H265eContextModel_t m_contextModels[MAX_OFF_CTX_MOD];
    H265eCabacCtx     m_cabac;

    enum NALUnitType m_nalUnitType;       ///< Nal unit type for the slice
    SliceType        m_sliceType;
    RK_U32           m_IsGenB;
    RK_S32           m_sliceQp;
    RK_U32           m_dependentSliceSegmentFlag;
    RK_U32           m_deblockingFilterDisable;
    RK_U32           m_deblockingFilterOverrideFlag;    //< offsets for deblocking filter inherit from PPS
    RK_S32           m_deblockingFilterBetaOffsetDiv2;  //< beta offset for deblocking filter
    RK_S32           m_deblockingFilterTcOffsetDiv2;    //< tc offset for deblocking filter
    RK_S32           m_numRefIdx[2];     //  for multiple reference of current slice

    RK_U32           m_bCheckLDC;

    //  Data
    RK_S32           m_sliceQpDelta;
    RK_S32           m_sliceQpDeltaCb;
    RK_S32           m_sliceQpDeltaCr;
    H265eDpbFrm      *m_refPicList[2][MAX_REFS + 1];
    RK_S32           m_refPOCList[2][MAX_REFS + 1];
    RK_U32           m_bIsUsedAsLongTerm[2][MAX_REFS + 1];

    // referenced slice?
    RK_U32           is_referenced;

    // access channel
    H265eSps*       m_sps;
    H265ePps*       m_pps;
    H265eVps*       m_vps;
    RK_U32          m_colFromL0Flag; // collocated picture from List0 flag

    RK_U32      m_colRefIdx;
    RK_U32      m_maxNumMergeCand;

    RK_U32      m_sliceCurEndCUAddr;
    RK_U32      m_nextSlice;
    RK_U32      m_sliceBits;
    RK_U32      m_sliceSegmentBits;
    RK_U32      m_bFinalized;

    RK_U32      m_tileOffstForMultES;

    RK_U32*     m_substreamSizes;

//   TComScalingList* m_scalingList; //!< pointer of quantization matrix
    RK_U32      m_cabacInitFlag;

    RK_U32      m_bLMvdL1Zero;
    RK_S32      m_numEntryPointOffsets;
    RK_U32      m_temporalLayerNonReferenceFlag;
    RK_U32      m_LFCrossSliceBoundaryFlag;
    RK_U32      m_enableTMVPFlag;

    RK_U32      slice_reserved_flag;
    RK_U32      no_output_of_prior_pics_flag;
    RK_U32      slice_header_extension_length;
    RK_U32      ref_pic_list_modification_flag_l0;
    RK_U32      lst_entry_l0;
    RK_U32      tot_poc_num;
    RK_U32      num_long_term_sps;
    RK_U32      num_long_term_pics;
} H265eSlice;

#ifdef  __cplusplus
extern "C" {
#endif

void h265e_slice_set_ref_list(H265eDpbFrm *frame_list, H265eSlice *slice);
void h265e_slice_set_ref_poc_list(H265eSlice *slice);
void h265e_slice_init(void *ctx, EncFrmStatus curr);
RK_S32 h265e_code_slice_skip_frame(void *ctx, H265eSlice *slice, RK_U8 *buf, RK_S32 len);

#ifdef __cplusplus
}
#endif

#endif /* __H265E_SLICE_H__ */
