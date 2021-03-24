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

#ifndef __AV1D_COMMON_H__
#define __AV1D_COMMON_H__

#include "mpp_common.h"
// #include "hal_av1d_common.h"
#define AV1_REF_SCALE_SHIFT 14

#define NUM_REF_FRAMES 8
#define NUM_REF_FRAMES_LG2 3

// Max tiles for AV1 (custom size) for Level <= 6.x
#define AV1_MAX_TILES 128
#define AV1_MAX_TILE_COL 64
#define AV1_MAX_TILE_ROW 64

#define AV1_MIN_COMP_BASIS 8
#define AV1_MAX_CODED_FRAME_SIZE \
  (8192 * 4352 * 10 * 6 / 32 / AV1_MIN_COMP_BASIS) /* approx 8 MB */

#define ALLOWED_REFS_PER_FRAME_EX 7

#define NUM_FRAME_CONTEXTS_LG2_EX 3
#define NUM_FRAME_CONTEXTS_EX (1 << NUM_FRAME_CONTEXTS_LG2_EX)

#define MIN_TILE_WIDTH 256
#define MAX_TILE_WIDTH 4096
#define MIN_TILE_WIDTH_SBS (MIN_TILE_WIDTH >> 6)
#define MAX_TILE_WIDTH_SBS (MAX_TILE_WIDTH >> 6)
#define FRAME_OFFSET_BITS 5
#define MAX_TILE_AREA (4096 * 2304)
// #define AV1_MAX_TILE_COLS 64
// #define AV1_MAX_TILE_ROWS 64

#define ALLOWED_REFS_PER_FRAME 3

#define NUM_FRAME_CONTEXTS_LG2 2
#define NUM_FRAME_CONTEXTS (1 << NUM_FRAME_CONTEXTS_LG2)

#define DCPREDSIMTHRESH 0
#define DCPREDCNTTHRESH 3

#define PREDICTION_PROBS 3

#define DEFAULT_PRED_PROB_0 120
#define DEFAULT_PRED_PROB_1 80
#define DEFAULT_PRED_PROB_2 40

#define AV1_DEF_UPDATE_PROB 252

#define MBSKIP_CONTEXTS 3

#define MAX_MB_SEGMENTS 8
#define MB_SEG_TREE_PROBS (MAX_MB_SEGMENTS - 1)

#define MAX_REF_LF_DELTAS_EX 8

#define MAX_REF_LF_DELTAS 4
#define MAX_MODE_LF_DELTAS 2

/* Segment Feature Masks */
#define SEGMENT_DELTADATA 0
#define SEGMENT_ABSDATA 1
#define MAX_MV_REFS 9

#define AV1_SWITCHABLE_FILTERS 3 /* number of switchable filters */
#define SWITCHABLE_FILTER_CONTEXTS ((AV1_SWITCHABLE_FILTERS + 1) * 4)
#ifdef DUAL_FILTER
#define AV1_SWITCHABLE_EXT_FILTERS 4 /* number of switchable filters */
#endif

#define COMP_PRED_CONTEXTS 2

#define COEF_UPDATE_PROB 252
#define AV1_PROB_HALF 128
#define AV1_NMV_UPDATE_PROB 252
#define AV1_MV_UPDATE_PRECISION 7
#define MV_JOINTS 4
#define MV_FP_SIZE 4
#define MV_CLASSES 11
#define CLASS0_BITS 1
#define CLASS0_SIZE (1 << CLASS0_BITS)
#define MV_OFFSET_BITS (MV_CLASSES + CLASS0_BITS - 2)

#define MV_MAX_BITS (MV_CLASSES + CLASS0_BITS + 2)
#define MV_MAX ((1 << MV_MAX_BITS) - 1)
#define MV_VALS ((MV_MAX << 1) + 1)

#define MAX_ENTROPY_TOKENS 12
#define ENTROPY_NODES 11

/* The first nodes of the entropy probs are unconstrained, the rest are
 * modeled with statistic distribution. */
#define UNCONSTRAINED_NODES 3
#define MODEL_NODES (ENTROPY_NODES - UNCONSTRAINED_NODES)
#define PIVOT_NODE 2  // which node is pivot
#define COEFPROB_MODELS 128

/* Entropy nodes above is divided in two parts, first three probs in part1
 * and the modeled probs in part2. Part1 is padded so that tables align with
 *  32 byte addresses, so there is four bytes for each table. */
#define ENTROPY_NODES_PART1 4
#define ENTROPY_NODES_PART2 8
#define INTER_MODE_CONTEXTS 7
#define AV1_INTER_MODE_CONTEXTS 15

#define CFL_JOINT_SIGNS 8
#define CFL_ALPHA_CONTEXTS 6
#define CFL_ALPHABET_SIZE 16

#define NEWMV_MODE_CONTEXTS 6
#define ZEROMV_MODE_CONTEXTS 2
#define GLOBALMV_MODE_CONTEXTS 2
#define REFMV_MODE_CONTEXTS 9
#define DRL_MODE_CONTEXTS 3
#define NMV_CONTEXTS 3

#define INTRA_INTER_CONTEXTS 4
#define COMP_INTER_CONTEXTS 5
#define REF_CONTEXTS 5
#define AV1_REF_CONTEXTS 3
#define FWD_REFS 4
#define BWD_REFS 3
#define SINGLE_REFS 7

#define BLOCK_TYPES 2
#define REF_TYPES 2  // intra=0, inter=1
#define COEF_BANDS 6
#define PREV_COEF_CONTEXTS 6

#define MODULUS_PARAM 13 /* Modulus parameter */

#define ACTIVE_HT 110  // quantization stepsize threshold

#define MAX_MV_REF_CANDIDATES 2

/* Coefficient token alphabet */

#define ZERO_TOKEN 0         /* 0         Extra Bits 0+0 */
#define ONE_TOKEN 1          /* 1         Extra Bits 0+1 */
#define TWO_TOKEN 2          /* 2         Extra Bits 0+1 */
#define THREE_TOKEN 3        /* 3         Extra Bits 0+1 */
#define FOUR_TOKEN 4         /* 4         Extra Bits 0+1 */
#define DCT_VAL_CATEGORY1 5  /* 5-6       Extra Bits 1+1 */
#define DCT_VAL_CATEGORY2 6  /* 7-10      Extra Bits 2+1 */
#define DCT_VAL_CATEGORY3 7  /* 11-18     Extra Bits 3+1 */
#define DCT_VAL_CATEGORY4 8  /* 19-34     Extra Bits 4+1 */
#define DCT_VAL_CATEGORY5 9  /* 35-66     Extra Bits 5+1 */
#define DCT_VAL_CATEGORY6 10 /* 67+       Extra Bits 13+1 */
#define DCT_EOB_TOKEN 11     /* EOB       Extra Bits 0+0 */
#define MAX_ENTROPY_TOKENS 12

#define INTERINTRA_MODES 4
#define INTER_COMPOUND_MODES 8
#define COMPOUND_TYPES 3
#define HEAD_TOKENS 5
#define TAIL_TOKENS 9
#define ONE_TOKEN_EOB 1
#define ONE_TOKEN_NEOB 2

#define MULTICORE_LEFT_TILE 1
#define MULTICORE_INNER_TILE 2
#define MULTICORE_RIGHT_TILE 3

#define DCT_EOB_MODEL_TOKEN 3 /* EOB       Extra Bits 0+0 */

typedef RK_U32 av1_coeff_count[REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
[UNCONSTRAINED_NODES + 1];
typedef RK_U8 av1_coeff_probs[REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
[UNCONSTRAINED_NODES];

#define BLOCK_SIZE_GROUPS 4

// AV1 extended transforms (ext_tx)
#define EXT_TX_SETS_INTER 4  // Sets of transform selections for INTER
#define EXT_TX_SETS_INTRA 3  // Sets of transform selections for INTRA
#define EXTTX_SIZES 4        // ext_tx experiment tx sizes
#define EXT_TX_TYPES 16

#define EXT_TX_SIZES 3

#define TX_TYPES 4

#define ROUND_POWER_OF_TWO(value, n) (((value) + (1 << ((n)-1))) >> (n))

/* Shift down with rounding for use when n >= 0, value >= 0 for (64 bit) */
#define ROUND_POWER_OF_TWO_64(value, n) \
  (((value) + ((((int64)1 << (n)) >> 1))) >> (n))

/* Shift down with rounding for signed integers, for use when n >= 0 (64 bit) */
#define ROUND_POWER_OF_TWO_SIGNED_64(value, n)           \
  (((value) < 0) ? -ROUND_POWER_OF_TWO_64(-(value), (n)) \
                 : ROUND_POWER_OF_TWO_64((value), (n)))

/* Shift down with rounding for signed integers, for use when n >= 0 */
#define ROUND_POWER_OF_TWO_SIGNED(value, n)           \
  (((value) < 0) ? -ROUND_POWER_OF_TWO(-(value), (n)) \
                 : ROUND_POWER_OF_TWO((value), (n)))

typedef RK_U16 av1_cdf;

#define MAX_MB_SEGMENTS 8

enum Av1SegLevelFeatures {
    SEG_AV1_LVL_ALT_Q,       // Use alternate Quantizer ....
    SEG_AV1_LVL_ALT_LF_Y_V,  // Use alternate loop filter value on y plane
    // vertical
    SEG_AV1_LVL_ALT_LF_Y_H,  // Use alternate loop filter value on y plane
    // horizontal
    SEG_AV1_LVL_ALT_LF_U,    // Use alternate loop filter value on u plane
    SEG_AV1_LVL_ALT_LF_V,    // Use alternate loop filter value on v plane
    SEG_AV1_LVL_REF_FRAME,   // Optional Segment reference frame
    SEG_AV1_LVL_SKIP,        // Optional Segment (0,0) + skip mode
    SEG_AV1_LVL_GLOBALMV,
    SEG_AV1_LVL_MAX
};

#define AV1_ACTIVE_REFS 3
#define AV1_ACTIVE_REFS_EX 7
#define AV1_REF_LIST_SIZE 8
#define AV1_REF_SCALE_SHIFT 14

enum MvReferenceFrame {
    NONE              = -1,
    INTRA_FRAME       = 0,
    LAST_FRAME        = 1,
    LAST2_FRAME_EX    = 2,
    LAST3_FRAME_EX    = 3,
    GOLDEN_FRAME_EX   = 4,
    BWDREF_FRAME_EX   = 5,
    ALTREF2_FRAME_EX  = 6,
    ALTREF_FRAME_EX   = 7,
    MAX_REF_FRAMES_EX = 8,
    GOLDEN_FRAME      = 2,
    ALTREF_FRAME      = 3,

    MAX_REF_FRAMES    = 4
};

enum BlockSizeType {
    BLOCK_SIZE_AB4X4,
    BLOCK_SIZE_SB4X8,
    BLOCK_SIZE_SB8X4,
    BLOCK_SIZE_SB8X8,
    BLOCK_SIZE_SB8X16,
    BLOCK_SIZE_SB16X8,
    BLOCK_SIZE_MB16X16,
    BLOCK_SIZE_SB16X32,
    BLOCK_SIZE_SB32X16,
    BLOCK_SIZE_SB32X32,
    BLOCK_SIZE_SB32X64,
    BLOCK_SIZE_SB64X32,
    BLOCK_SIZE_SB64X64,
    BLOCK_SIZE_SB64X128,
    BLOCK_SIZE_SB128X64,
    BLOCK_SIZE_SB128X128,
    BLOCK_SIZE_SB4X16,
    BLOCK_SIZE_SB16X4,
    BLOCK_SIZE_SB8X32,
    BLOCK_SIZE_SB32X8,
    BLOCK_SIZE_SB16X64,
    BLOCK_SIZE_SB64X16,
    BLOCK_SIZE_TYPES,
    BLOCK_SIZES_ALL = BLOCK_SIZE_TYPES

};

enum PartitionType {
    PARTITION_NONE,
    PARTITION_HORZ,
    PARTITION_VERT,
    PARTITION_SPLIT,
    /*
    PARTITION_HORZ_A,
    PARTITION_HORZ_B,
    PARTITION_VERT_A,
    PARTITION_VERT_B,
    PARTITION_HORZ_4,
    PARTITION_VERT_4,
    */
    PARTITION_TYPES
};

#define PARTITION_PLOFFSET 4  // number of probability models per block size
#define NUM_PARTITION_CONTEXTS (4 * PARTITION_PLOFFSET)

enum FrameType {
    KEY_FRAME = 0,
    INTER_FRAME = 1,
    NUM_FRAME_TYPES,
};

enum MbPredictionMode {
    DC_PRED,  /* average of above and left pixels */
    V_PRED,   /* vertical prediction */
    H_PRED,   /* horizontal prediction */
    D45_PRED, /* Directional 45 deg prediction  [anti-clockwise from 0 deg hor] */
    D135_PRED, /* Directional 135 deg prediction [anti-clockwise from 0 deg hor]
              */
    D117_PRED, /* Directional 112 deg prediction [anti-clockwise from 0 deg hor]
              */
    D153_PRED, /* Directional 157 deg prediction [anti-clockwise from 0 deg hor]
              */
    D27_PRED, /* Directional 22 deg prediction  [anti-clockwise from 0 deg hor] */
    D63_PRED, /* Directional 67 deg prediction  [anti-clockwise from 0 deg hor] */
    SMOOTH_PRED,
    TM_PRED_AV1 = SMOOTH_PRED,
    SMOOTH_V_PRED,  // Vertical interpolation
    SMOOTH_H_PRED,  // Horizontal interpolation
    TM_PRED,        /* Truemotion prediction */
    PAETH_PRED = TM_PRED,
    NEARESTMV,
    NEARMV,
    ZEROMV,
    NEWMV,
    NEAREST_NEARESTMV,
    NEAR_NEARMV,
    NEAREST_NEWMV,
    NEW_NEARESTMV,
    NEAR_NEWMV,
    NEW_NEARMV,
    ZERO_ZEROMV,
    NEW_NEWMV,
    SPLITMV,
    MB_MODE_COUNT
};

// Must match hardware/src/include/common_defs.h
#define AV1_INTRA_MODES 13

#define MAX_INTRA_MODES AV1_INTRA_MODES

#define MAX_INTRA_MODES_DRAM_ALIGNED ((MAX_INTRA_MODES + 15) & (~15))

#define AV1_INTER_MODES (1 + NEWMV - NEARESTMV)

#define MOTION_MODE_CONTEXTS 10

#define DIRECTIONAL_MODES 8
#define MAX_ANGLE_DELTA 3

enum FilterIntraModeType {
    FILTER_DC_PRED,
    FILTER_V_PRED,
    FILTER_H_PRED,
    FILTER_D153_PRED,
    FILTER_PAETH_PRED,
    FILTER_INTRA_MODES,
    FILTER_INTRA_UNUSED = 7
};

#define FILTER_INTRA_SIZES 19

enum { SIMPLE_TRANSLATION, OBMC_CAUSAL, MOTION_MODE_COUNT };

#define SUBMVREF_COUNT 5

/* Integer pel reference mv threshold for use of high-precision 1/8 mv */
#define COMPANDED_MVREF_THRESH 8

#define TX_SIZE_CONTEXTS 2
#define AV1_TX_SIZE_CONTEXTS 3
#define VARTX_PART_CONTEXTS 22
#define TXFM_PARTITION_CONTEXTS 22

enum InterpolationFilterType {
    EIGHTTAP_SMOOTH,
    EIGHTTAP,
    EIGHTTAP_SHARP,
#ifdef DUAL_FILTER
    EIGHTTAP_SMOOTH2,
    BILINEAR,
    SWITCHABLE, /* should be the last one */
#else
    BILINEAR,
    SWITCHABLE, /* should be the last one */
#endif
    MULTITAP_SHARP = EIGHTTAP_SHARP
};

static const int av1_literal_to_filter[4] = {EIGHTTAP_SMOOTH, EIGHTTAP,
                                             EIGHTTAP_SHARP, BILINEAR
                                            };

extern const enum InterpolationFilterType
av1hwd_switchable_interp[AV1_SWITCHABLE_FILTERS];

enum CompPredModeType {
    SINGLE_PREDICTION_ONLY = 0,
    COMP_PREDICTION_ONLY = 1,
    HYBRID_PREDICTION = 2,
    NB_PREDICTION_TYPES = 3,
};

enum TxfmMode {
    ONLY_4X4 = 0,
    ALLOW_8X8 = 1,
    ALLOW_16X16 = 2,
    ALLOW_32X32 = 3,
    TX_MODE_LARGEST = ALLOW_32X32,  // AV1
    TX_MODE_SELECT = 4,
    NB_TXFM_MODES = 5,
};

enum SegLevelFeatures {
    SEG_LVL_ALT_Q = 0,
    SEG_LVL_ALT_LF = 1,
    SEG_LVL_REF_FRAME = 2,
    SEG_LVL_SKIP = 3,
    SEG_LVL_MAX = 4
};

enum { AV1_SEG_FEATURE_DELTA, AV1_SEG_FEATURE_ABS };

static const int av1_seg_feature_data_signed[SEG_AV1_LVL_MAX] = {1, 1, 1, 1,
                                                                 1, 0, 0
                                                                };
static const int av1_seg_feature_data_max[SEG_AV1_LVL_MAX] = {255, 63, 63, 63,
                                                              63,  7,  0
                                                             };
static const int av1_seg_feature_data_bits[SEG_AV1_LVL_MAX] = {8, 6, 6, 6,
                                                               6, 3, 0
                                                              };

enum TxSize {
    TX_4X4 = 0,
    TX_8X8 = 1,
    TX_16X16 = 2,
    TX_32X32 = 3,
    TX_SIZE_MAX_SB,
};
#define MAX_TX_DEPTH 2

enum TxType { DCT_DCT = 0, ADST_DCT = 1, DCT_ADST = 2, ADST_ADST = 3 };

enum SplitMvPartitioningType {
    PARTITIONING_16X8 = 0,
    PARTITIONING_8X16,
    PARTITIONING_8X8,
    PARTITIONING_4X4,
    NB_PARTITIONINGS,
};

enum PredId {
    PRED_SEG_ID = 0,
    PRED_MBSKIP = 1,
    PRED_SWITCHABLE_INTERP = 2,
    PRED_INTRA_INTER = 3,
    PRED_COMP_INTER_INTER = 4,
    PRED_SINGLE_REF_P1 = 5,
    PRED_SINGLE_REF_P2 = 6,
    PRED_COMP_REF_P = 7,
    PRED_TX_SIZE = 8
};

/* Symbols for coding which components are zero jointly */
enum MvJointType {
    MV_JOINT_ZERO = 0,   /* Zero vector */
    MV_JOINT_HNZVZ = 1,  /* Vert zero, hor nonzero */
    MV_JOINT_HZVNZ = 2,  /* Hor zero, vert nonzero */
    MV_JOINT_HNZVNZ = 3, /* Both components nonzero */
};

/* Symbols for coding magnitude class of nonzero components */
enum MvClassType {
    MV_CLASS_0 = 0,   /* (0, 2]     integer pel */
    MV_CLASS_1 = 1,   /* (2, 4]     integer pel */
    MV_CLASS_2 = 2,   /* (4, 8]     integer pel */
    MV_CLASS_3 = 3,   /* (8, 16]    integer pel */
    MV_CLASS_4 = 4,   /* (16, 32]   integer pel */
    MV_CLASS_5 = 5,   /* (32, 64]   integer pel */
    MV_CLASS_6 = 6,   /* (64, 128]  integer pel */
    MV_CLASS_7 = 7,   /* (128, 256] integer pel */
    MV_CLASS_8 = 8,   /* (256, 512] integer pel */
    MV_CLASS_9 = 9,   /* (512, 1024] integer pel */
    MV_CLASS_10 = 10, /* (1024,2048] integer pel */
};

enum RefreshFrameContextModeAv1 {
    /**
     * AV1 Only, no refresh
     */
    AV1_REFRESH_FRAME_CONTEXT_NONE,
    /**
     * Update frame context to values resulting from backward probability
     * updates based on entropy/counts in the decoded frame
     */
    AV1_REFRESH_FRAME_CONTEXT_BACKWARD
};

// 75B
struct NmvContext {
    // Start at +27B offset
    RK_U8 joints[MV_JOINTS - 1];  // 3B
    RK_U8 sign[2];                // 2B

    // A+1
    RK_U8 class0[2][CLASS0_SIZE - 1];  // 2B
    RK_U8 fp[2][MV_FP_SIZE - 1];       // 6B
    RK_U8 class0_hp[2];                // 2B
    RK_U8 hp[2];                       // 2B
    RK_U8 classes[2][MV_CLASSES - 1];  // 20B

    // A+2
    RK_U8 class0_fp[2][CLASS0_SIZE][MV_FP_SIZE - 1];  // 12B
    RK_U8 bits[2][MV_OFFSET_BITS];                    // 20B
};

struct NmvContextCounts {
    // 8dw (u32) / DRAM word (u256)
    RK_U32 joints[MV_JOINTS];
    RK_U32 sign[2][2];
    RK_U32 classes[2][MV_CLASSES];
    RK_U32 class0[2][CLASS0_SIZE];
    RK_U32 bits[2][MV_OFFSET_BITS][2];
    RK_U32 class0_fp[2][CLASS0_SIZE][4];
    RK_U32 fp[2][4];
    RK_U32 class0_hp[2][2];
    RK_U32 hp[2][2];
};

typedef RK_U8 av1_prob;

#define ICDF(x) (32768U - (x))
#define CDF_SIZE(x) ((x)-1)

#define AV1HWPAD(x, y) RK_U8 x[y]

struct NmvJointSign {
    RK_U8 joints[MV_JOINTS - 1];  // 3B
    RK_U8 sign[2];                // 2B
};
struct NmvMagnitude {
    RK_U8 class0[2][CLASS0_SIZE - 1];
    RK_U8 fp[2][MV_FP_SIZE - 1];
    RK_U8 class0_hp[2];
    RK_U8 hp[2];
    RK_U8 classes[2][MV_CLASSES - 1];
    RK_U8 class0_fp[2][CLASS0_SIZE][MV_FP_SIZE - 1];
    RK_U8 bits[2][MV_OFFSET_BITS];
};

struct RefMvNmvContext {
    // Starts at +4B offset (for mbskip)
    struct NmvJointSign joints_sign[NMV_CONTEXTS];  // 15B
    AV1HWPAD(pad1, 13);

    // A+1
    struct NmvMagnitude magnitude[NMV_CONTEXTS];
};

/* Adaptive entropy contexts, padding elements are added to have
 * 256 bit aligned tables for HW access.
 * Compile with TRACE_PROB_TABLES to print bases for each table. */
struct Av1AdaptiveEntropyProbs {
    // address A (56)

    // Address A+0
    RK_U8 inter_mode_prob[INTER_MODE_CONTEXTS][4];  // 7*4 = 28B
    RK_U8 intra_inter_prob[INTRA_INTER_CONTEXTS];   // 4B

    // Address A+1
    RK_U8 uv_mode_prob[MAX_INTRA_MODES]
    [MAX_INTRA_MODES_DRAM_ALIGNED];  // 10*16/32 = 5 addrs

#if ((MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32)
    AV1HWPAD(pad1,
             ((MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32 == 0)
             ? 0
             : 32 - (MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32);
#endif

    // Address A+6
    RK_U8 tx8x8_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 3];    // 2*(4-3) = 2B
    RK_U8 tx16x16_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 2];  // 2*(4-2) = 4B
    RK_U8 tx32x32_prob[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 1];  // 2*(4-1) = 6B

    RK_U8 switchable_interp_prob[AV1_SWITCHABLE_FILTERS + 1]
    [AV1_SWITCHABLE_FILTERS - 1];  // 8B
    RK_U8 comp_inter_prob[COMP_INTER_CONTEXTS];                // 5B

    AV1HWPAD(pad6, 7);

    // Address A+7
    RK_U8 sb_ymode_prob[BLOCK_SIZE_GROUPS]
    [MAX_INTRA_MODES_DRAM_ALIGNED];  // 4*16/32 = 2 addrs

    // Address A+9
    RK_U8 partition_prob[NUM_FRAME_TYPES][NUM_PARTITION_CONTEXTS]
    [PARTITION_TYPES];  // 2*16*4 = 4 addrs

    // Address A+13
    AV1HWPAD(pad13, 24);
    RK_U8 mbskip_probs[MBSKIP_CONTEXTS];  // 3B
    struct NmvContext nmvc;

    // Address A+16
    RK_U8 single_ref_prob[REF_CONTEXTS][2];          // 10B
    RK_U8 comp_ref_prob[REF_CONTEXTS];               // 5B
    RK_U8 mb_segment_tree_probs[MB_SEG_TREE_PROBS];  // 7B
    RK_U8 segment_pred_probs[PREDICTION_PROBS];      // 3B
    AV1HWPAD(pad16, 7);

    // Address A+17
    RK_U8 prob_coeffs[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [ENTROPY_NODES_PART1];  // 18 addrs
    RK_U8 prob_coeffs8x8[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [ENTROPY_NODES_PART1];
    RK_U8 prob_coeffs16x16[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [ENTROPY_NODES_PART1];
    RK_U8 prob_coeffs32x32[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [ENTROPY_NODES_PART1];
};

/* Entropy contexts */
struct Av1EntropyProbs {
    /* Default keyframe probs */
    /* Table formatted for 256b memory, probs 0to7 for all tables followed by
     * probs 8toN for all tables.
     * Compile with TRACE_PROB_TABLES to print bases for each table. */

    // In AOM code, this table is [M][M][M-1]; we pad to 16B so each entry is 1/2
    // DRAM word.
    RK_U8 kf_bmode_prob[MAX_INTRA_MODES][MAX_INTRA_MODES]
    [MAX_INTRA_MODES_DRAM_ALIGNED];

#if ((MAX_INTRA_MODES * MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32)
    AV1HWPAD(pad0, (((MAX_INTRA_MODES * MAX_INTRA_MODES *
                      MAX_INTRA_MODES_DRAM_ALIGNED) %
                     32) == 0)
             ? 0
             : 32 - ((MAX_INTRA_MODES * MAX_INTRA_MODES *
                      MAX_INTRA_MODES_DRAM_ALIGNED) %
                     32));
#endif

    // Address 50
    AV1HWPAD(unused_bytes, 4);  // 4B of padding to maintain the old alignments.
    RK_U8 ref_pred_probs[PREDICTION_PROBS];   // 3B
    RK_U8 ref_scores[MAX_REF_FRAMES];         // 4B
    RK_U8 prob_comppred[COMP_PRED_CONTEXTS];  // 2B

    AV1HWPAD(pad1, 19);

    // Address 51
    RK_U8 kf_uv_mode_prob[MAX_INTRA_MODES][MAX_INTRA_MODES_DRAM_ALIGNED];

#if ((MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32)
    AV1HWPAD(pad51,
             ((MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32 == 0)
             ? 0
             : 32 - (MAX_INTRA_MODES * MAX_INTRA_MODES_DRAM_ALIGNED) % 32);
#endif

    // Address 56
    struct Av1AdaptiveEntropyProbs a;  // Probs with backward adaptation
};

/* Counters for adaptive entropy contexts */
struct Av1EntropyCounts {
    RK_U32 inter_mode_counts[INTER_MODE_CONTEXTS][AV1_INTER_MODES - 1][2];
    RK_U32 sb_ymode_counts[BLOCK_SIZE_GROUPS][MAX_INTRA_MODES];
    RK_U32 uv_mode_counts[MAX_INTRA_MODES][MAX_INTRA_MODES];
    RK_U32 partition_counts[NUM_PARTITION_CONTEXTS][PARTITION_TYPES];
    RK_U32 switchable_interp_counts[AV1_SWITCHABLE_FILTERS + 1]
    [AV1_SWITCHABLE_FILTERS];
    RK_U32 intra_inter_count[INTRA_INTER_CONTEXTS][2];
    RK_U32 comp_inter_count[COMP_INTER_CONTEXTS][2];
    RK_U32 single_ref_count[REF_CONTEXTS][2][2];
    RK_U32 comp_ref_count[REF_CONTEXTS][2];
    RK_U32 tx32x32_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB];
    RK_U32 tx16x16_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 1];
    RK_U32 tx8x8_count[TX_SIZE_CONTEXTS][TX_SIZE_MAX_SB - 2];
    RK_U32 mbskip_count[MBSKIP_CONTEXTS][2];

    struct NmvContextCounts nmvcount;

    RK_U32 count_coeffs[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [UNCONSTRAINED_NODES + 1];
    RK_U32 count_coeffs8x8[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [UNCONSTRAINED_NODES + 1];
    RK_U32 count_coeffs16x16[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [UNCONSTRAINED_NODES + 1];
    RK_U32 count_coeffs32x32[BLOCK_TYPES][REF_TYPES][COEF_BANDS][PREV_COEF_CONTEXTS]
    [UNCONSTRAINED_NODES + 1];

    RK_U32 count_eobs[TX_SIZE_MAX_SB][BLOCK_TYPES][REF_TYPES][COEF_BANDS]
    [PREV_COEF_CONTEXTS];
};

struct CoeffHeadCDFModel {
    RK_U16 band0[3][5];
    RK_U16 bands[5][6][4];
};

struct CoeffTailCDFModel {
    RK_U16 band0[3][9];
    RK_U16 bands[5][6][9];
};

// 135
typedef struct CoeffHeadCDFModel coeff_head_cdf_model[BLOCK_TYPES][REF_TYPES];
// 297
typedef struct CoeffTailCDFModel coeff_tail_cdf_model[BLOCK_TYPES][REF_TYPES];

//#define PALETTE_BLOCK_SIZES (BLOCK_SIZE_SB64X64 - BLOCK_SIZE_SB8X8 + 1)
#define PALETTE_BLOCK_SIZES 7
#define PALETTE_SIZES 7
#define PALETTE_Y_MODE_CONTEXTS 3
#define PALETTE_UV_MODE_CONTEXTS 2
#define PALETTE_COLOR_INDEX_CONTEXTS 5
#define PALETTE_IDX_CONTEXTS 18
#define PALETTE_COLORS 8
#define KF_MODE_CONTEXTS 5

#define PLANE_TYPES 2
#define TX_SIZES 5
#define TXB_SKIP_CONTEXTS 13
#define DC_SIGN_CONTEXTS 3
#define SIG_COEF_CONTEXTS_EOB 4
#define SIG_COEF_CONTEXTS 42
#define COEFF_BASE_CONTEXTS 42
#define EOB_COEF_CONTEXTS 9
#define LEVEL_CONTEXTS 21
#define NUM_BASE_LEVELS 2
#define BR_CDF_SIZE 4
#define MOTION_MODES 3
#define DELTA_Q_PROBS 3
#define COMP_REF_TYPE_CONTEXTS 5
#define UNI_COMP_REF_CONTEXTS 3
#define UNIDIR_COMP_REFS 4
//#define FILTER_INTRA_MODES 5
#define SKIP_MODE_CONTEXTS 3
#define SKIP_CONTEXTS 3
#define COMP_INDEX_CONTEXTS 6
#define COMP_GROUP_IDX_CONTEXTS 7
#define MAX_TX_CATS 4
#define CFL_ALLOWED_TYPES 2
#define UV_INTRA_MODES 14
#define EXT_PARTITION_TYPES 10
#define AV1_PARTITION_CONTEXTS (5 * PARTITION_PLOFFSET)

#define RESTORE_SWITCHABLE_TYPES 3
#define DELTA_LF_PROBS 3
#define FRAME_LF_COUNT 4
#define MAX_SEGMENTS 8
#define TOKEN_CDF_Q_CTXS 4
#define SEG_TEMPORAL_PRED_CTXS 3
#define SPATIAL_PREDICTION_PROBS 3

typedef RK_U16 aom_cdf_prob;

typedef struct {
    RK_U16 joint_cdf[3];
    RK_U16 sign_cdf[2];
    RK_U16 clsss_cdf[2][10];
    RK_U16 clsss0_fp_cdf[2][2][3];
    RK_U16 fp_cdf[2][3];
    RK_U16 class0_hp_cdf[2];
    RK_U16 hp_cdf[2];
    RK_U16 class0_cdf[2];
    RK_U16 bits_cdf[2][10];
} MvCDFs;

typedef struct {
    RK_U16 partition_cdf[13][16];
    // 64
    RK_U16 kf_ymode_cdf[KF_MODE_CONTEXTS][KF_MODE_CONTEXTS][AV1_INTRA_MODES - 1];
    RK_U16 segment_pred_cdf[PREDICTION_PROBS];
    RK_U16 spatial_pred_seg_tree_cdf[SPATIAL_PREDICTION_PROBS][MAX_MB_SEGMENTS - 1];
    RK_U16 mbskip_cdf[MBSKIP_CONTEXTS];
    RK_U16 delta_q_cdf[DELTA_Q_PROBS];
    RK_U16 delta_lf_multi_cdf[FRAME_LF_COUNT][DELTA_LF_PROBS];
    RK_U16 delta_lf_cdf[DELTA_LF_PROBS];
    RK_U16 skip_mode_cdf[SKIP_MODE_CONTEXTS];
    RK_U16 vartx_part_cdf[VARTX_PART_CONTEXTS][1];
    RK_U16 tx_size_cdf[MAX_TX_CATS][AV1_TX_SIZE_CONTEXTS][MAX_TX_DEPTH];
    RK_U16 if_ymode_cdf[BLOCK_SIZE_GROUPS][AV1_INTRA_MODES - 1];
    RK_U16 uv_mode_cdf[2][AV1_INTRA_MODES][AV1_INTRA_MODES - 1 + 1];
    RK_U16 intra_inter_cdf[INTRA_INTER_CONTEXTS];
    RK_U16 comp_inter_cdf[COMP_INTER_CONTEXTS];
    RK_U16 single_ref_cdf[AV1_REF_CONTEXTS][SINGLE_REFS - 1];
    RK_U16 comp_ref_type_cdf[COMP_REF_TYPE_CONTEXTS][1];
    RK_U16 uni_comp_ref_cdf[UNI_COMP_REF_CONTEXTS][UNIDIR_COMP_REFS - 1][1];
    RK_U16 comp_ref_cdf[AV1_REF_CONTEXTS][FWD_REFS - 1];
    RK_U16 comp_bwdref_cdf[AV1_REF_CONTEXTS][BWD_REFS - 1];
    RK_U16 newmv_cdf[NEWMV_MODE_CONTEXTS];
    RK_U16 zeromv_cdf[ZEROMV_MODE_CONTEXTS];
    RK_U16 refmv_cdf[REFMV_MODE_CONTEXTS];
    RK_U16 drl_cdf[DRL_MODE_CONTEXTS];
    RK_U16 interp_filter_cdf[SWITCHABLE_FILTER_CONTEXTS][AV1_SWITCHABLE_FILTERS - 1];

    MvCDFs mv_cdf;

    RK_U16 obmc_cdf[BLOCK_SIZE_TYPES];
    RK_U16 motion_mode_cdf[BLOCK_SIZE_TYPES][2];

    RK_U16 inter_compound_mode_cdf[AV1_INTER_MODE_CONTEXTS][INTER_COMPOUND_MODES - 1];
    RK_U16 compound_type_cdf[BLOCK_SIZE_TYPES][CDF_SIZE(COMPOUND_TYPES - 1)];
    RK_U16 interintra_cdf[BLOCK_SIZE_GROUPS];
    RK_U16 interintra_mode_cdf[BLOCK_SIZE_GROUPS][INTERINTRA_MODES - 1];
    RK_U16 wedge_interintra_cdf[BLOCK_SIZE_TYPES];
    RK_U16 wedge_idx_cdf[BLOCK_SIZE_TYPES][CDF_SIZE(16)];

    RK_U16 palette_y_mode_cdf[PALETTE_BLOCK_SIZES][PALETTE_Y_MODE_CONTEXTS][1];
    RK_U16 palette_uv_mode_cdf[PALETTE_UV_MODE_CONTEXTS][1];
    RK_U16 palette_y_size_cdf[PALETTE_BLOCK_SIZES][PALETTE_SIZES - 1];
    RK_U16 palette_uv_size_cdf[PALETTE_BLOCK_SIZES][PALETTE_SIZES - 1];

    RK_U16 cfl_sign_cdf[CFL_JOINT_SIGNS - 1];
    RK_U16 cfl_alpha_cdf[CFL_ALPHA_CONTEXTS][CFL_ALPHABET_SIZE - 1];

    RK_U16 intrabc_cdf[1];
    RK_U16 angle_delta_cdf[DIRECTIONAL_MODES][6];

    RK_U16 filter_intra_mode_cdf[FILTER_INTRA_MODES - 1];
    RK_U16 filter_intra_cdf[BLOCK_SIZES_ALL];
    RK_U16 comp_group_idx_cdf[COMP_GROUP_IDX_CONTEXTS][CDF_SIZE(2)];
    RK_U16 compound_idx_cdf[COMP_INDEX_CONTEXTS][CDF_SIZE(2)];

    RK_U16 dummy0[14];

    // Palette index contexts; sizes 1/7, 2/6, 3/5 packed together
    RK_U16 palette_y_color_index_cdf[PALETTE_IDX_CONTEXTS][8];
    RK_U16 palette_uv_color_index_cdf[PALETTE_IDX_CONTEXTS][8];
    // RK_U16 dummy1[0];

    // Note: cdf space can be optimized (most sets have fewer than EXT_TX_TYPES
    // symbols)
    RK_U16 tx_type_intra0_cdf[EXTTX_SIZES][AV1_INTRA_MODES][8];
    RK_U16 tx_type_intra1_cdf[EXTTX_SIZES][AV1_INTRA_MODES][4];
    RK_U16 tx_type_inter_cdf[2][EXTTX_SIZES][EXT_TX_TYPES];

    aom_cdf_prob txb_skip_cdf[TX_SIZES][TXB_SKIP_CONTEXTS][CDF_SIZE(2)];
    aom_cdf_prob eob_extra_cdf[TX_SIZES][PLANE_TYPES][EOB_COEF_CONTEXTS][CDF_SIZE(2)];
    RK_U16 dummy_[5];

    aom_cdf_prob eob_flag_cdf16[PLANE_TYPES][2][4];
    aom_cdf_prob eob_flag_cdf32[PLANE_TYPES][2][8];
    aom_cdf_prob eob_flag_cdf64[PLANE_TYPES][2][8];
    aom_cdf_prob eob_flag_cdf128[PLANE_TYPES][2][8];
    aom_cdf_prob eob_flag_cdf256[PLANE_TYPES][2][8];
    aom_cdf_prob eob_flag_cdf512[PLANE_TYPES][2][16];
    aom_cdf_prob eob_flag_cdf1024[PLANE_TYPES][2][16];
    aom_cdf_prob coeff_base_eob_cdf[TX_SIZES][PLANE_TYPES][SIG_COEF_CONTEXTS_EOB][CDF_SIZE(3)];
    aom_cdf_prob coeff_base_cdf[TX_SIZES][PLANE_TYPES][SIG_COEF_CONTEXTS][CDF_SIZE(4) + 1];
    aom_cdf_prob dc_sign_cdf[PLANE_TYPES][DC_SIGN_CONTEXTS][CDF_SIZE(2)];
    RK_U16 dummy_2[2];
    aom_cdf_prob coeff_br_cdf[TX_SIZES][PLANE_TYPES][LEVEL_CONTEXTS][CDF_SIZE(BR_CDF_SIZE) + 1];
    RK_U16 dummy2[16];
} AV1CDFs;

typedef struct {
    RK_U8 scaling_lut_y[256];
    RK_U8 scaling_lut_cb[256];
    RK_U8 scaling_lut_cr[256];
    RK_S16 cropped_luma_grain_block[4096];
    RK_S16 cropped_chroma_grain_block[1024 * 2];
} AV1FilmGrainMemory;

#endif  // __AV1COMMONDEC_H__
