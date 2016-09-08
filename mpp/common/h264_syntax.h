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

//!< AVC Profile IDC definitions
typedef enum h264e_profile_t {
    H264_PROFILE_FREXT_CAVLC444     = 44,       //!< YUV 4:4:4/14 "CAVLC 4:4:4"
    H264_PROFILE_BASELINE           = 66,       //!< YUV 4:2:0/8  "Baseline"
    H264_PROFILE_MAIN               = 77,       //!< YUV 4:2:0/8  "Main"
    H264_PROFILE_EXTENDED           = 88,       //!< YUV 4:2:0/8  "Extended"
    H264_PROFILE_HIGH               = 100,      //!< YUV 4:2:0/8  "High"
    H264_PROFILE_HIGH10             = 110,      //!< YUV 4:2:0/10 "High 10"
    H264_PROFILE_HIGH422            = 122,      //!< YUV 4:2:2/10 "High 4:2:2"
    H264_PROFILE_HIGH444            = 244,      //!< YUV 4:4:4/14 "High 4:4:4"
    H264_PROFILE_MVC_HIGH           = 118,      //!< YUV 4:2:0/8  "Multiview High"
    H264_PROFILE_STEREO_HIGH        = 128       //!< YUV 4:2:0/8  "Stereo High"
} H264Profile;

//!< AVC Level IDC definitions
typedef enum {
    H264_LEVEL_1_0                  = 10,
    H264_LEVEL_1_b                  = 99,
    H264_LEVEL_1_1                  = 11,
    H264_LEVEL_1_2                  = 12,
    H264_LEVEL_1_3                  = 13,
    H264_LEVEL_2_0                  = 20,
    H264_LEVEL_2_1                  = 21,
    H264_LEVEL_2_2                  = 22,
    H264_LEVEL_3_0                  = 30,
    H264_LEVEL_3_1                  = 31,
    H264_LEVEL_3_2                  = 32,
    H264_LEVEL_4_0                  = 40,
    H264_LEVEL_4_1                  = 41,
    H264_LEVEL_4_2                  = 42,
    H264_LEVEL_5_0                  = 50,
    H264_LEVEL_5_1                  = 51,
} H264Level;

//!< values for nalu_type
typedef enum {
    NALU_TYPE_NULL      = 0,
    NALU_TYPE_SLICE     = 1,
    NALU_TYPE_DPA       = 2,
    NALU_TYPE_DPB       = 3,
    NALU_TYPE_DPC       = 4,
    NALU_TYPE_IDR       = 5,
    NALU_TYPE_SEI       = 6,
    NALU_TYPE_SPS       = 7,
    NALU_TYPE_PPS       = 8,
    NALU_TYPE_AUD       = 9,   // Access Unit Delimiter
    NALU_TYPE_EOSEQ     = 10,  // end of sequence
    NALU_TYPE_EOSTREAM  = 11,  // end of stream
    NALU_TYPE_FILL      = 12,
    NALU_TYPE_SPSEXT    = 13,
    NALU_TYPE_PREFIX    = 14,  // prefix
    NALU_TYPE_SUB_SPS   = 15,
    NALU_TYPE_SLICE_AUX = 19,
    NALU_TYPE_SLC_EXT   = 20,  // slice extensive
    NALU_TYPE_VDRD      = 24   // View and Dependency Representation Delimiter NAL Unit
} Nalu_type;

//!< values for nal_ref_idc
typedef enum {
    NALU_PRIORITY_HIGHEST    = 3,
    NALU_PRIORITY_HIGH       = 2,
    NALU_PRIORITY_LOW        = 1,
    NALU_PRIORITY_DISPOSABLE = 0
} NalRefIdc_type;

#endif /*__H264_SYNTAX_H__*/

