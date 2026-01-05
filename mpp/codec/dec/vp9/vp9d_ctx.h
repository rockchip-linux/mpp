/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef VP9D_CODEC_H
#define VP9D_CODEC_H

#include "mpp_frame.h"
#include "hal_dec_task.h"

#include "vp9d_syntax.h"

typedef enum VP9Profile_e {
    VP9_PROFILE_0 = 0,
    VP9_PROFILE_1 = 1,
    VP9_PROFILE_2 = 2,
    VP9_PROFILE_3 = 3,
    VP9_MAX_PROFILES
} VP9Profile;

typedef enum VP9TxfmMode_e {
    VP9_TX_4X4          = 0,
    VP9_TX_8X8          = 1,
    VP9_TX_16X16        = 2,
    VP9_TX_32X32        = 3,
    VP9_TX_SWITCHABLE   = 4,
    VP9_N_TXFM_MODES
} VP9TxfmMode;

typedef enum VP9TxfmType_e {
    VP9_DCT_DCT         = 0,
    VP9_DCT_ADST        = 1,
    VP9_ADST_DCT        = 2,
    VP9_ADST_ADST       = 3,
    VP9_N_TXFM_TYPES
} VP9TxfmType;

typedef enum VP9IntraPredMode_e {
    VP9_VERT_PRED       = 0,
    VP9_HOR_PRED        = 1,
    VP9_DC_PRED         = 2,
    VP9_DIAG_DOWN_LEFT_PRED = 3,
    VP9_DIAG_DOWN_RIGHT_PRED = 4,
    VP9_VERT_RIGHT_PRED = 5,
    VP9_HOR_DOWN_PRED   = 6,
    VP9_VERT_LEFT_PRED  = 7,
    VP9_HOR_UP_PRED     = 8,
    VP9_TM_VP8_PRED     = 9,
    VP9_LEFT_DC_PRED    = 10,
    VP9_TOP_DC_PRED     = 11,
    VP9_DC_128_PRED     = 12,
    VP9_DC_127_PRED     = 13,
    VP9_DC_129_PRED     = 14,
    VP9_N_INTRA_PRED_MODES
} VP9IntraPredMode;

typedef enum VP9FilterMode_e {
    VP9_FILTER_8TAP_SMOOTH  = 0,
    VP9_FILTER_8TAP_REGULAR = 1,
    VP9_FILTER_8TAP_SHARP   = 2,
    VP9_FILTER_BILINEAR     = 3,
    VP9_FILTER_SWITCHABLE   = 4,
} VP9FilterMode;


typedef enum VP9CompPredMode_e {
    VP9_PRED_SINGLEREF = 0,
    VP9_PRED_COMPREF    = 1,
    VP9_PRED_SWITCHABLE = 2,
} VP9CompPredMode;

typedef enum VP9BlockLevel_e {
    VP9_BL_64X64       = 0,
    VP9_BL_32X32       = 1,
    VP9_BL_16X16       = 2,
    VP9_BL_8X8         = 3,
} VP9BlockLevel;

typedef enum VP9BlockSize_e {
    VP9_BS_64x64       = 0,
    VP9_BS_64x32       = 1,
    VP9_BS_32x64       = 2,
    VP9_BS_32x32       = 3,
    VP9_BS_32x16       = 4,
    VP9_BS_16x32       = 5,
    VP9_BS_16x16       = 6,
    VP9_BS_16x8        = 7,
    VP9_BS_8x16        = 8,
    VP9_BS_8x8         = 9,
    VP9_BS_8x4         = 10,
    VP9_BS_4x8         = 11,
    VP9_BS_4x4         = 12,
    N_BS_SIZES         = 13,
} VP9BlockSize;

typedef struct Vp9dReader_t {
    RK_S32 range;
    RK_S32 bits_left;
    const RK_U8 *buffer;
    const RK_U8 *buffer_end;
    RK_U32 value;
} Vp9dReader;

typedef struct Vp9ProbCtx_t {
    RK_U8 y_mode[4][9];
    RK_U8 uv_mode[10][9];
    RK_U8 filter[4][2];
    RK_U8 mv_mode[7][3];
    RK_U8 intra[4];
    RK_U8 comp[5];
    RK_U8 single_ref[5][2];
    RK_U8 comp_ref[5];
    RK_U8 tx32p[2][3];
    RK_U8 tx16p[2][2];
    RK_U8 tx8p[2];
    RK_U8 skip[3];
    RK_U8 mv_joint[3];
    struct {
        RK_U8 sign;
        RK_U8 classes[10];
        RK_U8 class0;
        RK_U8 bits[10];
        RK_U8 class0_fp[2][3];
        RK_U8 fp[3];
        RK_U8 class0_hp;
        RK_U8 hp;
    } mv_comp[2];
    RK_U8 partition[4][4][3];
} Vp9ProbCtx;

// extern const variables
extern const RK_S16 vp9_dc_qlookup[3][256];
extern const RK_S16 vp9_ac_qlookup[3][256];
extern const Vp9ProbCtx vp9_default_probs;
extern const RK_U8 vp9_default_coef_probs[4][2][2][6][6][3];

#ifdef  __cplusplus
extern "C" {
#endif

// vp9 specific bitread range decoder
RK_S32 vp9d_reader_init(Vp9dReader *r, const uint8_t *buf, RK_S32 buf_size);
RK_S32 vp9d_read(Vp9dReader *r, RK_S32 prob);
RK_S32 vp9d_read_bit(Vp9dReader *r);
RK_S32 vp9d_read_bits(Vp9dReader *r, RK_S32 bits);
RK_S32 vp9d_diff_update_prob(Vp9dReader *r, RK_S32 prob, RK_U8 *delta);
RK_S32 vp9d_merge_prob(RK_U8 *prob, RK_U32 cout0, RK_U32 cout1, RK_S32 max_count, RK_S32 factor);

#ifdef  __cplusplus
}
#endif

#endif /* VP9D_CODEC_H */
