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


#ifndef __H264_SYNTAX_H__
#define __H264_SYNTAX_H__

/* H.264/AVC-specific definition */

//!< define
#define MAXSPS                      32
#define MAXPPS                      256

//!< aspect ratio explicitly specified as width:height
#define H264_EXTENDED_SAR           255

//!< values for nal_ref_idc
typedef enum H264NalRefIdcType_e {
    H264_NALU_PRIORITY_DISPOSABLE   = 0,
    H264_NALU_PRIORITY_LOW          = 1,
    H264_NALU_PRIORITY_HIGH         = 2,
    H264_NALU_PRIORITY_HIGHEST      = 3
} H264NalRefIdcType;

//!< AVC Profile IDC definitions
typedef enum h264e_profile_t {
    H264_PROFILE_FREXT_CAVLC444     = 44,   //!< YUV 4:4:4/14 "CAVLC 4:4:4"
    H264_PROFILE_BASELINE           = 66,   //!< YUV 4:2:0/8  "Baseline"
    H264_PROFILE_MAIN               = 77,   //!< YUV 4:2:0/8  "Main"
    H264_PROFILE_EXTENDED           = 88,   //!< YUV 4:2:0/8  "Extended"
    H264_PROFILE_HIGH               = 100,  //!< YUV 4:2:0/8  "High"
    H264_PROFILE_HIGH10             = 110,  //!< YUV 4:2:0/10 "High 10"
    H264_PROFILE_HIGH422            = 122,  //!< YUV 4:2:2/10 "High 4:2:2"
    H264_PROFILE_HIGH444            = 244,  //!< YUV 4:4:4/14 "High 4:4:4"
    H264_PROFILE_MVC_HIGH           = 118,  //!< YUV 4:2:0/8  "Multiview High"
    H264_PROFILE_STEREO_HIGH        = 128   //!< YUV 4:2:0/8  "Stereo High"
} H264Profile;

//!< AVC Level IDC definitions
typedef enum {
    H264_LEVEL_1_0                  = 10,   //!< qcif@15fps
    H264_LEVEL_1_b                  = 99,   //!< qcif@15fps
    H264_LEVEL_1_1                  = 11,   //!< cif@7.5fps
    H264_LEVEL_1_2                  = 12,   //!< cif@15fps
    H264_LEVEL_1_3                  = 13,   //!< cif@30fps
    H264_LEVEL_2_0                  = 20,   //!< cif@30fps
    H264_LEVEL_2_1                  = 21,   //!< half-D1@@25fps
    H264_LEVEL_2_2                  = 22,   //!< D1@12.5fps
    H264_LEVEL_3_0                  = 30,   //!< D1@25fps
    H264_LEVEL_3_1                  = 31,   //!< 720p@30fps
    H264_LEVEL_3_2                  = 32,   //!< 720p@60fps
    H264_LEVEL_4_0                  = 40,   //!< 1080p@30fps
    H264_LEVEL_4_1                  = 41,   //!< 1080p@30fps
    H264_LEVEL_4_2                  = 42,   //!< 1080p@60fps
    H264_LEVEL_5_0                  = 50,   //!< 3K@30fps
    H264_LEVEL_5_1                  = 51,   //!< 4K@30fps
    H264_LEVEL_5_2                  = 52,   //!< 4K@60fps
    H264_LEVEL_6_0                  = 60,   //!< 8K@30fps
    H264_LEVEL_6_1                  = 61,   //!< 8K@60fps
    H264_LEVEL_6_2                  = 62,   //!< 8K@120fps
} H264Level;

//!< values for nalu_type
typedef enum H264NaluType_e {
    H264_NALU_TYPE_NULL             = 0,
    H264_NALU_TYPE_SLICE            = 1,
    H264_NALU_TYPE_DPA              = 2,
    H264_NALU_TYPE_DPB              = 3,
    H264_NALU_TYPE_DPC              = 4,
    H264_NALU_TYPE_IDR              = 5,
    H264_NALU_TYPE_SEI              = 6,
    H264_NALU_TYPE_SPS              = 7,
    H264_NALU_TYPE_PPS              = 8,
    H264_NALU_TYPE_AUD              = 9,    // Access Unit Delimiter
    H264_NALU_TYPE_EOSEQ            = 10,   // end of sequence
    H264_NALU_TYPE_EOSTREAM         = 11,   // end of stream
    H264_NALU_TYPE_FILL             = 12,
    H264_NALU_TYPE_SPSEXT           = 13,
    H264_NALU_TYPE_PREFIX           = 14,   // prefix
    H264_NALU_TYPE_SUB_SPS          = 15,
    H264_NALU_TYPE_SLICE_AUX        = 19,
    H264_NALU_TYPE_SLC_EXT          = 20,   // slice extensive
    H264_NALU_TYPE_VDRD             = 24    // View and Dependency Representation Delimiter NAL Unit
} H264NaluType;

typedef enum H264ChromaFmt_e {
    H264_CHROMA_400                 = 0,    //!< Monochrome
    H264_CHROMA_420                 = 1,    //!< 4:2:0
    H264_CHROMA_422                 = 2,    //!< 4:2:2
    H264_CHROMA_444                 = 3     //!< 4:4:4
} H264ChromaFmt;

typedef enum H264SliceType_e {
    H264_P_SLICE                    = 0,
    H264_B_SLICE                    = 1,
    H264_I_SLICE                    = 2,
    H264_SP_SLICE                   = 3,
    H264_SI_SLICE                   = 4,
    H264_NUM_SLICE_TYPES            = 5
} H264SliceType;

//!< SEI
typedef enum H264SeiType_e {
    H264_SEI_BUFFERING_PERIOD       = 0,
    H264_SEI_PIC_TIMING,
    H264_SEI_PAN_SCAN_RECT,
    H264_SEI_FILLER_PAYLOAD,
    H264_SEI_USER_DATA_REGISTERED_ITU_T_T35,
    H264_SEI_USER_DATA_UNREGISTERED,
    H264_SEI_RECOVERY_POINT,
    H264_SEI_DEC_REF_PIC_MARKING_REPETITION,
    H264_SEI_SPARE_PIC,
    H264_SEI_SCENE_INFO,
    H264_SEI_SUB_SEQ_INFO,
    H264_SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
    H264_SEI_SUB_SEQ_CHARACTERISTICS,
    H264_SEI_FULL_FRAME_FREEZE,
    H264_SEI_FULL_FRAME_FREEZE_RELEASE,
    H264_SEI_FULL_FRAME_SNAPSHOT,
    H264_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
    H264_SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
    H264_SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
    H264_SEI_FILM_GRAIN_CHARACTERISTICS,
    H264_SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
    H264_SEI_STEREO_VIDEO_INFO,
    H264_SEI_POST_FILTER_HINTS,
    H264_SEI_TONE_MAPPING,
    H264_SEI_SCALABILITY_INFO,
    H264_SEI_SUB_PIC_SCALABLE_LAYER,
    H264_SEI_NON_REQUIRED_LAYER_REP,
    H264_SEI_PRIORITY_LAYER_INFO,
    H264_SEI_LAYERS_NOT_PRESENT,
    H264_SEI_LAYER_DEPENDENCY_CHANGE,
    H264_SEI_SCALABLE_NESTING,
    H264_SEI_BASE_LAYER_TEMPORAL_HRD,
    H264_SEI_QUALITY_LAYER_INTEGRITY_CHECK,
    H264_SEI_REDUNDANT_PIC_PROPERTY,
    H264_SEI_TL0_DEP_REP_INDEX,
    H264_SEI_TL_SWITCHING_POINT,
    H264_SEI_PARALLEL_DECODING_INFO,
    H264_SEI_MVC_SCALABLE_NESTING,
    H264_SEI_VIEW_SCALABILITY_INFO,
    H264_SEI_MULTIVIEW_SCENE_INFO,
    H264_SEI_MULTIVIEW_ACQUISITION_INFO,
    H264_SEI_NON_REQUIRED_VIEW_COMPONENT,
    H264_SEI_VIEW_DEPENDENCY_CHANGE,
    H264_SEI_OPERATION_POINTS_NOT_PRESENT,
    H264_SEI_BASE_VIEW_TEMPORAL_HRD,
    H264_SEI_FRAME_PACKING_ARRANGEMENT,

    H264_SEI_MAX_ELEMENTS  //!< number of maximum syntax elements
} H264SeiType;

typedef enum H264ScalingListType_e {
    H264_INTRA_4x4_Y,
    H264_INTRA_4x4_U,
    H264_INTRA_4x4_V,
    H264_INTER_4x4_Y,
    H264_INTER_4x4_U,
    H264_INTER_4x4_V,
    H264_INTRA_8x8_Y,
    H264_INTER_8x8_Y,
    H264_SCALING_MATRIX_TYPE_BUTT,
} H264ScalingMatrixType;

#define H264E_MAX_REFS_CNT          16

#endif /*__H264_SYNTAX_H__*/
