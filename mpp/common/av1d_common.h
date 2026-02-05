/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef AV1D_COMMON_H
#define AV1D_COMMON_H

#include "mpp_common.h"

// Max tiles for AV1 (custom size) for Level <= 6.x
#define AV1_MAX_TILES           128
#define AV1_MAX_TILE_COLS       64
#define AV1_MAX_TILE_ROWS       64
#define AV1_MAX_TILE_WIDTH      4096
#define AV1_MAX_TILE_AREA       (4096 * 2304)

// Pixels per Mode Info (MI) unit
#define AV1_MI_SIZE_LOG2        2

#define AV1_PREDICTION_PROBS    3
#define AV1_MBSKIP_CONTEXTS     3
#define AV1_MAX_MODE_LF_DELTAS  2

#define AV1_SWITCHABLE_FILTERS  3 /* number of switchable filters */
#define AV1_SWITCHABLE_FILTER_CONTEXTS ((AV1_SWITCHABLE_FILTERS + 1) * 4)

#define AV1_INTER_MODE_CONTEXTS 15

#define AV1_CFL_JOINT_SIGNS     8
#define AV1_CFL_ALPHA_CONTEXTS  6
#define AV1_CFL_ALPHABET_SIZE   16

#define AV1_NEWMV_MODE_CONTEXTS     6
#define AV1_ZEROMV_MODE_CONTEXTS    2
#define AV1_GLOBALMV_MODE_CONTEXTS  2
#define AV1_REFMV_MODE_CONTEXTS     9
#define AV1_DRL_MODE_CONTEXTS       3
#define AV1_INTRA_INTER_CONTEXTS    4
#define AV1_COMP_INTER_CONTEXTS     5
#define AV1_REF_CONTEXTS            3
#define AV1_FWD_REFS                4
#define AV1_BWD_REFS                3
#define AV1_SINGLE_REFS             7

#define AV1_INTERINTRA_MODES    4
#define INTER_COMPOUND_MODES    8
#define AV1_COMPOUND_TYPES      3

#define AV1_BLOCK_SIZE_GROUPS   4

// AV1 extended transforms (ext_tx)
#define AV1_EXTTX_SIZES         4        // ext_tx experiment tx sizes
#define AV1_EXT_TX_TYPES        16

// Frame Restoration types (section 6.10.15)
enum {
    AV1_RESTORE_NONE            = 0,
    AV1_RESTORE_WIENER          = 1,
    AV1_RESTORE_SGRPROJ         = 2,
    AV1_RESTORE_SWITCHABLE      = 3,
};

// Frame types (section 6.8.2).
enum {
    AV1_FRAME_KEY               = 0,
    AV1_FRAME_INTER             = 1,
    AV1_FRAME_INTRA_ONLY        = 2,
    AV1_FRAME_SWITCH            = 3,
};

#define AV1_MAX_SEGMENTS 8
enum Av1SegLevelFeatures {
    AV1_SEG_LVL_ALT_Q           = 0,  // Use alternate Quantizer ....
    AV1_SEG_LVL_ALT_LF_Y_V      = 1,  // Use alternate loop filter value on y plane
    // vertical
    AV1_SEG_LVL_ALT_LF_Y_H      = 2,  // Use alternate loop filter value on y plane
    // horizontal
    AV1_SEG_LVL_ALT_LF_U        = 3,  // Use alternate loop filter value on u plane
    AV1_SEG_LVL_ALT_LF_V        = 4,  // Use alternate loop filter value on v plane
    AV1_SEG_LVL_REF_FRAME       = 5,  // Optional Segment reference frame
    AV1_SEG_LVL_SKIP            = 6,  // Optional Segment (0,0) + skip mode
    AV1_SEG_LVL_GLOBALMV        = 7,
    AV1_SEG_LVL_MAX             = 8
};

#define AV1_ACTIVE_REFS_EX      7
#define AV1_REF_LIST_SIZE       8
#define AV1_REF_SCALE_SHIFT     14

enum Av1MvReferenceFrame {
    AV1_REF_NONE                = -1,
    AV1_REF_FRAME_INTRA         = 0,
    AV1_REF_FRAME_LAST          = 1,
    AV1_REF_FRAME_LAST2         = 2,
    AV1_REF_FRAME_LAST3         = 3,
    AV1_REF_FRAME_GOLDEN        = 4,
    AV1_REF_FRAME_BWDREF        = 5,
    AV1_REF_FRAME_ALTREF2       = 6,
    AV1_REF_FRAME_ALTREF        = 7,
    AV1_NUM_REF_FRAMES          = 8,

    AV1_REFS_PER_FRAME          = 7,
    AV1_TOTAL_REFS_PER_FRAME    = 8,
    AV1_PRIMARY_REF_NONE        = 7,
};

enum Av1BlockSizeType {
    AV1_BLOCK_SIZE_AB4X4        = 0,
    AV1_BLOCK_SIZE_SB4X8        = 1,
    AV1_BLOCK_SIZE_SB8X4        = 2,
    AV1_BLOCK_SIZE_SB8X8        = 3,
    AV1_BLOCK_SIZE_SB8X16       = 4,
    AV1_BLOCK_SIZE_SB16X8       = 5,
    AV1_BLOCK_SIZE_MB16X16      = 6,
    AV1_BLOCK_SIZE_SB16X32      = 7,
    AV1_BLOCK_SIZE_SB32X16      = 8,
    AV1_BLOCK_SIZE_SB32X32      = 9,
    AV1_BLOCK_SIZE_SB32X64      = 10,
    AV1_BLOCK_SIZE_SB64X32      = 11,
    AV1_BLOCK_SIZE_SB64X64      = 12,
    AV1_BLOCK_SIZE_SB64X128     = 13,
    AV1_BLOCK_SIZE_SB128X64     = 14,
    AV1_BLOCK_SIZE_SB128X128    = 15,
    AV1_BLOCK_SIZE_SB4X16       = 16,
    AV1_BLOCK_SIZE_SB16X4       = 17,
    AV1_BLOCK_SIZE_SB8X32       = 18,
    AV1_BLOCK_SIZE_SB32X8       = 19,
    AV1_BLOCK_SIZE_SB16X64      = 20,
    AV1_BLOCK_SIZE_SB64X16      = 21,
    AV1_BLOCK_SIZES_ALL         = 22
};

enum Av1FrameType {
    AV1_KEY_FRAME               = 0,
    AV1_INTER_FRAME             = 1,
    AV1_INTRA_ONLY_FRAME        = 2,  // replaces intra-only
    AV1_S_FRAME                 = 3,
    AV1_NUM_FRAME_TYPES         = 4,
};

#define AV1_INTRA_MODES         13
#define AV1_DIRECTIONAL_MODES   8
#define AV1_MAX_ANGLE_DELTA     3

enum Av1FilterIntraModeType {
    AV1_FILTER_DC_PRED          = 0,
    AV1_FILTER_V_PRED           = 1,
    AV1_FILTER_H_PRED           = 2,
    AV1_FILTER_D153_PRED        = 3,
    AV1_FILTER_PAETH_PRED       = 4,
    AV1_FILTER_INTRA_MODES      = 5,

    AV1_FILTER_INTRA_UNUSED     = 7
};

#define AV1_TX_SIZE_CONTEXTS        3
#define AV1_VARTX_PART_CONTEXTS     22
#define AV1_TXFM_PARTITION_CONTEXTS 22

enum InterpolationFilterType {
    AV1_INTERP_EIGHTTAP_SMOOTH  = 0,
    AV1_INTERP_EIGHTTAP         = 1,
    AV1_INTERP_EIGHTTAP_SHARP   = 2,
    AV1_INTERP_BILINEAR         = 3,
    AV1_INTERP_SWITCHABLE       = 4, // should be the last one
};

enum Av1TxfmMode {
    AV1_ONLY_4X4                = 0,
    AV1_TX_MODE_LARGEST         = 1,
    AV1_TX_MODE_SELECT          = 2,
    AV1_NB_TXFM_MODES           = 3,
};

#define AV1_MAX_TX_DEPTH 2

#define AV1_CDF_SIZE(x) ((x)-1)

#define AV1_PALETTE_BLOCK_SIZES         7
#define AV1_PALETTE_SIZES               7
#define AV1_PALETTE_Y_MODE_CONTEXTS     3
#define AV1_PALETTE_UV_MODE_CONTEXTS    2
#define AV1_PALETTE_IDX_CONTEXTS        18
#define AV1_KF_MODE_CONTEXTS            5

#define AV1_PLANE_TYPES             2
#define AV1_TX_SIZES                5
#define AV1_TXB_SKIP_CONTEXTS       13
#define AV1_DC_SIGN_CONTEXTS        3
#define AV1_SIG_COEF_CONTEXTS_EOB   4
#define AV1_SIG_COEF_CONTEXTS       42

#define AV1_EOB_COEF_CONTEXTS       9
#define AV1_LEVEL_CONTEXTS          21
#define AV1_NUM_BASE_LEVELS         2
#define AV1_BR_CDF_SIZE             4
#define AV1_MOTION_MODES            3
#define AV1_DELTA_Q_PROBS           3
#define AV1_COMP_REF_TYPE_CONTEXTS  5
#define AV1_UNI_COMP_REF_CONTEXTS   3
#define AV1_UNIDIR_COMP_REFS        4

#define AV1_SKIP_MODE_CONTEXTS          3
#define AV1_SKIP_CONTEXTS               3
#define AV1_COMP_INDEX_CONTEXTS         6
#define AV1_COMP_GROUP_IDX_CONTEXTS     7
#define AV1_MAX_TX_CATS                 4
#define AV1_CFL_ALLOWED_TYPES           2
#define AV1_UV_INTRA_MODES              14

#define AV1_DELTA_LF_PROBS      3
#define AV1_FRAME_LF_COUNT      4

#define AV1_TOKEN_CDF_Q_CTXS            4
#define AV1_SEG_TEMPORAL_PRED_CTXS      3
#define AV1_SPATIAL_PREDICTION_PROBS    3

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
} Av1MvCDFs;

typedef struct {
    RK_U16 partition_cdf[13][16];
    // 64
    RK_U16 kf_ymode_cdf[AV1_KF_MODE_CONTEXTS][AV1_KF_MODE_CONTEXTS][AV1_CDF_SIZE(AV1_INTRA_MODES)];
    RK_U16 segment_pred_cdf[AV1_PREDICTION_PROBS];
    RK_U16 spatial_pred_seg_tree_cdf[AV1_SPATIAL_PREDICTION_PROBS][AV1_CDF_SIZE(AV1_MAX_SEGMENTS)];
    RK_U16 mbskip_cdf[AV1_MBSKIP_CONTEXTS];
    RK_U16 delta_q_cdf[AV1_DELTA_Q_PROBS];
    RK_U16 delta_lf_multi_cdf[AV1_FRAME_LF_COUNT][AV1_DELTA_LF_PROBS];
    RK_U16 delta_lf_cdf[AV1_DELTA_LF_PROBS];
    RK_U16 skip_mode_cdf[AV1_SKIP_MODE_CONTEXTS];
    RK_U16 vartx_part_cdf[AV1_VARTX_PART_CONTEXTS][1];
    RK_U16 tx_size_cdf[AV1_MAX_TX_CATS][AV1_TX_SIZE_CONTEXTS][AV1_MAX_TX_DEPTH];
    RK_U16 if_ymode_cdf[AV1_BLOCK_SIZE_GROUPS][AV1_CDF_SIZE(AV1_INTRA_MODES)];
    RK_U16 uv_mode_cdf[2][AV1_INTRA_MODES][AV1_CDF_SIZE(AV1_UV_INTRA_MODES)];
    RK_U16 intra_inter_cdf[AV1_INTRA_INTER_CONTEXTS];
    RK_U16 comp_inter_cdf[AV1_COMP_INTER_CONTEXTS];
    RK_U16 single_ref_cdf[AV1_REF_CONTEXTS][AV1_SINGLE_REFS - 1];
    RK_U16 comp_ref_type_cdf[AV1_COMP_REF_TYPE_CONTEXTS][1];
    RK_U16 uni_comp_ref_cdf[AV1_UNI_COMP_REF_CONTEXTS][AV1_UNIDIR_COMP_REFS - 1][AV1_CDF_SIZE(2)];
    RK_U16 comp_ref_cdf[AV1_REF_CONTEXTS][AV1_FWD_REFS - 1];
    RK_U16 comp_bwdref_cdf[AV1_REF_CONTEXTS][AV1_BWD_REFS - 1];
    RK_U16 newmv_cdf[AV1_NEWMV_MODE_CONTEXTS];
    RK_U16 zeromv_cdf[AV1_ZEROMV_MODE_CONTEXTS];
    RK_U16 refmv_cdf[AV1_REFMV_MODE_CONTEXTS];
    RK_U16 drl_cdf[AV1_DRL_MODE_CONTEXTS];
    RK_U16 interp_filter_cdf[AV1_SWITCHABLE_FILTER_CONTEXTS][AV1_CDF_SIZE(AV1_SWITCHABLE_FILTERS)];

    Av1MvCDFs mv_cdf;

    RK_U16 obmc_cdf[AV1_BLOCK_SIZES_ALL];
    RK_U16 motion_mode_cdf[AV1_BLOCK_SIZES_ALL][2];

    RK_U16 inter_compound_mode_cdf[AV1_INTER_MODE_CONTEXTS][AV1_CDF_SIZE(INTER_COMPOUND_MODES)];
    RK_U16 compound_type_cdf[AV1_BLOCK_SIZES_ALL][AV1_CDF_SIZE(AV1_COMPOUND_TYPES - 1)];
    RK_U16 interintra_cdf[AV1_BLOCK_SIZE_GROUPS];
    RK_U16 interintra_mode_cdf[AV1_BLOCK_SIZE_GROUPS][AV1_CDF_SIZE(AV1_INTERINTRA_MODES)];
    RK_U16 wedge_interintra_cdf[AV1_BLOCK_SIZES_ALL];
    RK_U16 wedge_idx_cdf[AV1_BLOCK_SIZES_ALL][AV1_CDF_SIZE(16)];

    RK_U16 palette_y_mode_cdf[AV1_PALETTE_BLOCK_SIZES][AV1_PALETTE_Y_MODE_CONTEXTS][1];
    RK_U16 palette_uv_mode_cdf[AV1_PALETTE_UV_MODE_CONTEXTS][1];
    RK_U16 palette_y_size_cdf[AV1_PALETTE_BLOCK_SIZES][AV1_CDF_SIZE(AV1_PALETTE_SIZES)];
    RK_U16 palette_uv_size_cdf[AV1_PALETTE_BLOCK_SIZES][AV1_CDF_SIZE(AV1_PALETTE_SIZES)];

    RK_U16 cfl_sign_cdf[AV1_CDF_SIZE(AV1_CFL_JOINT_SIGNS)];
    RK_U16 cfl_alpha_cdf[AV1_CFL_ALPHA_CONTEXTS][AV1_CDF_SIZE(AV1_CFL_ALPHABET_SIZE)];

    RK_U16 intrabc_cdf[1];
    RK_U16 angle_delta_cdf[AV1_DIRECTIONAL_MODES][AV1_CDF_SIZE(2 * AV1_MAX_ANGLE_DELTA + 1)];

    RK_U16 filter_intra_mode_cdf[AV1_CDF_SIZE(AV1_FILTER_INTRA_MODES)];
    RK_U16 filter_intra_cdf[AV1_BLOCK_SIZES_ALL];
    RK_U16 comp_group_idx_cdf[AV1_COMP_GROUP_IDX_CONTEXTS][AV1_CDF_SIZE(2)];
    RK_U16 compound_idx_cdf[AV1_COMP_INDEX_CONTEXTS][AV1_CDF_SIZE(2)];

    RK_U16 dummy0[14];

    // Palette index contexts; sizes 1/7, 2/6, 3/5 packed together
    RK_U16 palette_y_color_index_cdf[AV1_PALETTE_IDX_CONTEXTS][8];
    RK_U16 palette_uv_color_index_cdf[AV1_PALETTE_IDX_CONTEXTS][8];
    // RK_U16 dummy1[0];

    // Note: cdf space can be optimized (most sets have fewer than EXT_TX_TYPES symbols)
    RK_U16 tx_type_intra0_cdf[AV1_EXTTX_SIZES][AV1_INTRA_MODES][8];
    RK_U16 tx_type_intra1_cdf[AV1_EXTTX_SIZES][AV1_INTRA_MODES][4];
    RK_U16 tx_type_inter_cdf[2][AV1_EXTTX_SIZES][AV1_EXT_TX_TYPES];

    RK_U16 txb_skip_cdf[AV1_TX_SIZES][AV1_TXB_SKIP_CONTEXTS][AV1_CDF_SIZE(2)];
    RK_U16 eob_extra_cdf[AV1_TX_SIZES][AV1_PLANE_TYPES][AV1_EOB_COEF_CONTEXTS][AV1_CDF_SIZE(2)];
    RK_U16 dummy_[5];

    RK_U16 eob_flag_cdf16[AV1_PLANE_TYPES][2][4];
    RK_U16 eob_flag_cdf32[AV1_PLANE_TYPES][2][8];
    RK_U16 eob_flag_cdf64[AV1_PLANE_TYPES][2][8];
    RK_U16 eob_flag_cdf128[AV1_PLANE_TYPES][2][8];
    RK_U16 eob_flag_cdf256[AV1_PLANE_TYPES][2][8];
    RK_U16 eob_flag_cdf512[AV1_PLANE_TYPES][2][16];
    RK_U16 eob_flag_cdf1024[AV1_PLANE_TYPES][2][16];
    RK_U16 coeff_base_eob_cdf[AV1_TX_SIZES][AV1_PLANE_TYPES][AV1_SIG_COEF_CONTEXTS_EOB][AV1_CDF_SIZE(3)];
    RK_U16 coeff_base_cdf[AV1_TX_SIZES][AV1_PLANE_TYPES][AV1_SIG_COEF_CONTEXTS][AV1_CDF_SIZE(4) + 1];
    RK_U16 dc_sign_cdf[AV1_PLANE_TYPES][AV1_DC_SIGN_CONTEXTS][AV1_CDF_SIZE(2)];
    RK_U16 dummy_2[2];
    RK_U16 coeff_br_cdf[AV1_TX_SIZES][AV1_PLANE_TYPES][AV1_LEVEL_CONTEXTS][AV1_CDF_SIZE(AV1_BR_CDF_SIZE) + 1];
    RK_U16 dummy2[16];
} AV1CDFs;

typedef struct {
    RK_U8 scaling_lut_y[256];
    RK_U8 scaling_lut_cb[256];
    RK_U8 scaling_lut_cr[256];
    RK_S16 cropped_luma_grain_block[4096];
    RK_S16 cropped_chroma_grain_block[1024 * 2];
} AV1FilmGrainMemory;

#endif // AV1COMMONDEC_H
