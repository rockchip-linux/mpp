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


#ifndef __H265_SYNTAX_H__
#define __H265_SYNTAX_H__

#define PIXW_1080P      (1920)
#define PIXH_1080P      (1088)
#define PIXW_4Kx2K      (4096)
#define PIXH_4Kx2K      (2304)
#define PIXW_8Kx4K      (8192)
#define PIXH_8Kx4K      (4320)

#if 0

#define REF_PIC_LIST_NUM_IDX 32
#define H265E_UUID_LENGTH           16

#define MAX_CPB_CNT                 32 ///< Upper bound of (cpb_cnt_minus1 + 1)
#define MAX_NUM_LAYER_IDS           64
#define MAX_NUM_VPS                 16
#define MAX_NUM_SPS                 16
#define MAX_NUM_PPS                 64
#define MAX_TLAYER                  7 ///< max number of temporal layer
#define MAX_VPS_NUM_HRD_PARAMETERS  1
#define MAX_VPS_OP_SETS_PLUS1       1024
#define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1
#define MAX_NUM_REF_PICS            16          ///< max. number of pictures used for reference
#define MAX_NUM_REF                 16          ///< max. number of entries in picture reference list
#define MAX_CU_DEPTH                6                           // log2(LCUSize)

#define REF_PIC_LIST_0 0
#define REF_PIC_LIST_1 1
#define REF_PIC_LIST_X 100
#define REF_BY_RECN(idx)            (0x00000001 << idx)
#define REF_BY_REFR(idx)            (0x00000001 << idx)
#define MAX_NUM_REF_PICS 16

#endif

#define MAX_DPB_SIZE 17 // A.4.1
#define MAX_REFS 16

/**
 * 7.4.2.1
 */
#define MAX_SUB_LAYERS 7
#define MAX_VPS_COUNT 16
#define MAX_SPS_COUNT 16
#define MAX_PPS_COUNT 64
#define MAX_SHORT_TERM_RPS_COUNT 64
#define MAX_CU_SIZE 128

//TODO: check if this is really the maximum
#define MAX_TRANSFORM_DEPTH 5

#define MAX_TB_SIZE 32
#define MAX_PB_SIZE 64
#define MAX_LOG2_CTB_SIZE 6
#define MAX_QP 51
#define DEFAULT_INTRA_TC_OFFSET 2

#define HEVC_CONTEXTS 183

#define MRG_MAX_NUM_CANDS     5

#define L0 0
#define L1 1

#define REF_PIC_LIST_NUM_IDX 32
#define MAX_CPB_CNT          32 //< Upper bound of (cpb_cnt_minus1 + 1)
#define MAX_CU_DEPTH         6  // log2(LCUSize)
#define MAX_VPS_NUM_HRD_PARAMETERS  1
#define MAX_VPS_OP_SETS_PLUS1       1024
#define MAX_VPS_NUH_RESERVED_ZERO_LAYER_ID_PLUS1  1
#define MAX_NUM_LONG_TERM_REF_PIC_POC 20
#define REF_BY_RECN(idx)            (0x00000001 << idx)
#define REF_BY_REFR(idx)            (0x00000001 << idx)




#define EPEL_EXTRA_BEFORE 1
#define EPEL_EXTRA_AFTER  2
#define EPEL_EXTRA        3
#define QPEL_EXTRA_BEFORE 3
#define QPEL_EXTRA_AFTER  4
#define QPEL_EXTRA        7

#define EDGE_EMU_BUFFER_STRIDE 80

#define MPP_INPUT_BUFFER_PADDING_SIZE 8

#define MPP_PROFILE_HEVC_MAIN                        1
#define MPP_PROFILE_HEVC_MAIN_10                     2
#define MPP_PROFILE_HEVC_MAIN_STILL_PICTURE          3

#define LOG2_MAX_CTB_SIZE   6
#define LOG2_MIN_CTB_SIZE   4
#define LOG2_MAX_PU_SIZE    6
#define LOG2_MIN_PU_SIZE    2
#define LOG2_MAX_TU_SIZE    5
#define LOG2_MIN_TU_SIZE    2
#define LOG2_MAX_CU_SIZE    6
#define LOG2_MIN_CU_SIZE    3

/**
 * Value of the luma sample at position (x, y) in the 2D array tab.
 */
#define IS_IDR(s) (s->nal_unit_type == NAL_IDR_W_RADL || s->nal_unit_type == NAL_IDR_N_LP)
#define IS_BLA(s) (s->nal_unit_type == NAL_BLA_W_RADL || s->nal_unit_type == NAL_BLA_W_LP || \
                   s->nal_unit_type == NAL_BLA_N_LP)
#define IS_IRAP(s) (s->nal_unit_type >= 16 && s->nal_unit_type <= 23)

/**
 * Table 7-3: NAL unit type codes
 */
enum NALUnitType {
    NAL_INIT_VALUE = -1,
    NAL_TRAIL_N    = 0,
    NAL_TRAIL_R    = 1,
    NAL_TSA_N      = 2,
    NAL_TSA_R      = 3,
    NAL_STSA_N     = 4,
    NAL_STSA_R     = 5,
    NAL_RADL_N     = 6,
    NAL_RADL_R     = 7,
    NAL_RASL_N     = 8,
    NAL_RASL_R     = 9,
    NAL_BLA_W_LP   = 16,
    NAL_BLA_W_RADL = 17,
    NAL_BLA_N_LP   = 18,
    NAL_IDR_W_RADL = 19,
    NAL_IDR_N_LP   = 20,
    NAL_CRA_NUT    = 21,
    NAL_VPS        = 32,
    NAL_SPS        = 33,
    NAL_PPS        = 34,
    NAL_AUD        = 35,
    NAL_EOS_NUT    = 36,
    NAL_EOB_NUT    = 37,
    NAL_FD_NUT     = 38,
    NAL_SEI_PREFIX = 39,
    NAL_SEI_SUFFIX = 40,
};

typedef enum H265NalPriority_t {
    H265_NAL_PRIORITY_DISPOSABLE = 0,
    H265_NAL_PRIORITY_LOW        = 1,
    H265_NAL_PRIORITY_HIGH       = 2,
    H265_NAL_PRIORITY_HIGHEST    = 3,
} H265NalPriority;


enum RPSType {
    ST_CURR_BEF = 0,
    ST_CURR_AFT,
    ST_FOLL,
    LT_CURR,
    LT_FOLL,
    NB_RPS_TYPE,
};

typedef enum SliceType_t {
    B_SLICE = 0,
    P_SLICE = 1,
    I_SLICE = 2,
} SliceType;

typedef enum {
    H265_LEVEL_NONE = 0,
    H265_LEVEL1 = 30,
    H265_LEVEL2 = 60,
    H265_LEVEL2_1 = 63,
    H265_LEVEL3 = 90,
    H265_LEVEL3_1 = 93,
    H265_LEVEL4 = 120,
    H265_LEVEL4_1 = 123,
    H265_LEVEL5 = 150,
    H265_LEVEL5_1 = 153,
    H265_LEVEL5_2 = 156,
    H265_LEVEL6 = 180,
    H265_LEVEL6_1 = 183,
    H265_LEVEL6_2 = 186,
    H265_LEVEL8_5 = 255,
} H265Level;

#endif
