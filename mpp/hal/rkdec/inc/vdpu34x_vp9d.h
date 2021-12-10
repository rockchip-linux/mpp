/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __HAL_VDPU34X_VP9D_H__
#define __HAL_VDPU34X_VP9D_H__

#include "rk_type.h"
#include "vdpu34x_com.h"


typedef struct Vdpu34xRegVp9dParam_t {
    struct SWREG64_VP9_SET {
        RK_U32      cprheader_offset    : 16;
        RK_U32      reserve             : 16;
    } reg64;

    struct SWREG65_CUR_POC {
        RK_U32      cur_poc : 32;
    } reg65;

    RK_U32 reg66;

    struct SWREG67_74_VP9_SEGID_GRP {
        RK_U32      segid_abs_delta                 : 1;
        RK_U32      segid_frame_qp_delta_en         : 1;
        RK_U32      segid_frame_qp_delta            : 9;
        RK_U32      segid_frame_loopfitler_value_en : 1;
        RK_U32      segid_frame_loopfilter_value    : 7;
        RK_U32      segid_referinfo_en              : 1;
        RK_U32      segid_referinfo                 : 2;
        RK_U32      segid_frame_skip_en             : 1;
        RK_U32      reserve                         : 9;
    } reg67_74[8];

    struct SWREG75_VP9_INFO_LASTFRAME {
        RK_U32      mode_deltas_lastframe           : 14;
        RK_U32      vp9_segment_id_clear            : 1;
        RK_U32      vp9_segment_id_update           : 1;
        RK_U32      segmentation_enable_lstframe    : 1;
        RK_U32      last_show_frame                 : 1;
        RK_U32      last_intra_only                 : 1;
        RK_U32      last_widthheight_eqcur          : 1;
        RK_U32      color_space_lastkeyframe        : 3;
        RK_U32      reserve1                        : 9;
    } reg75;

    struct SWREG76_VP9_CPRHEADER_CONFIG {
        RK_U32      tx_mode                     : 3;
        RK_U32      frame_reference_mode        : 2;
        RK_U32      reserve                     : 27;
    } reg76;

    struct SWREG77_VP9_INTERCMD_NUM {
        RK_U32      intercmd_num        : 24;
        RK_U32      reserve             : 8;
    } reg77;

    struct SWREG78_VP9_LASTTILE_SIZE {
        RK_U32      lasttile_size       : 24;
        RK_U32      reserve             : 8;
    } reg78;

    struct SWREG79_VP9_LASTF_Y_HOR_VIRSTRIDE {
        RK_U32      lastfy_hor_virstride        : 16;
        RK_U32      reserve                     : 16;
    } reg79;

    struct SWREG80_VP9_LASTF_UV_HOR_VIRSTRIDE {
        RK_U32      lastfuv_hor_virstride       : 16;
        RK_U32      reserve                     : 16;
    } reg80;

    struct SWREG81_VP9_GOLDENF_Y_HOR_VIRSTRIDE {
        RK_U32      goldenfy_hor_virstride      : 16;
        RK_U32      reserve                     : 16;
    } reg81;

    struct SWREG82_VP9_GOLDENF_UV_HOR_VIRSTRIDE {
        RK_U32      goldenfuv_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } reg82;

    struct SWREG83_VP9_ALTREFF_Y_HOR_VIRSTRIDE {
        RK_U32      altreffy_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } reg83;

    struct SWREG84_VP9_ALTREFF_UV_HOR_VIRSTRIDE {
        RK_U32      altreffuv_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } reg84;

    struct SWREG85_VP9_LASTF_Y_VIRSTRIDE {
        RK_U32      lastfy_virstride         : 28;
        RK_U32      reserve                  : 4;
    } reg85;

    struct SWREG86_VP9_GOLDEN_Y_VIRSTRIDE {
        RK_U32      goldeny_virstride       : 28;
        RK_U32      reserve                 : 4;
    } reg86;

    struct SWREG87_VP9_ALTREF_Y_VIRSTRIDE {
        RK_U32      altrefy_virstride       : 28;
        RK_U32      reserve                 : 4;
    } reg87;

    struct SWREG88_VP9_LREF_HOR_SCALE {
        RK_U32      lref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg88;

    struct SWREG89_VP9_LREF_VER_SCALE {
        RK_U32      lref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg89;

    struct SWREG90_VP9_GREF_HOR_SCALE {
        RK_U32      gref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg90;

    struct SWREG91_VP9_GREF_VER_SCALE {
        RK_U32      gref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg91;

    struct SWREG92_VP9_AREF_HOR_SCALE {
        RK_U32      aref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg92;

    struct SWREG93_VP9_AREF_VER_SCALE {
        RK_U32      aref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } reg93;

    struct SWREG94_VP9_REF_DELTAS_LASTFRAME {
        RK_U32      ref_deltas_lastframe    : 28;
        RK_U32      reserve                 : 4;
    } reg94;

    struct SWREG95_LAST_POC {
        RK_U32      last_poc : 32;
    } reg95;

    struct SWREG96_GOLDEN_POC {
        RK_U32      golden_poc : 32;
    } reg96;

    struct SWREG97_ALTREF_POC {
        RK_U32      altref_poc : 32;
    } reg97;

    struct SWREG98_COF_REF_POC {
        RK_U32      col_ref_poc : 32;
    } reg98;

    struct SWREG99_PROB_REF_POC {
        RK_U32      prob_ref_poc : 32;
    } reg99;

    struct SWREG100_SEGID_REF_POC {
        RK_U32      segid_ref_poc : 32;
    } reg100;

    RK_U32  reg101_102_no_use[2];

    struct SWREG103_VP9_PROB_EN {
        RK_U32      reserve                 : 20;
        RK_U32      prob_update_en          : 1;
        RK_U32      refresh_en              : 1;
        RK_U32      prob_save_en            : 1;
        RK_U32      intra_only_flag         : 1;

        RK_U32      txfmmode_rfsh_en        : 1;
        RK_U32      ref_mode_rfsh_en        : 1;
        RK_U32      single_ref_rfsh_en      : 1;
        RK_U32      comp_ref_rfsh_en        : 1;

        RK_U32      interp_filter_switch_en : 1;
        RK_U32      allow_high_precision_mv : 1;
        RK_U32      last_key_frame_flag     : 1;
        RK_U32      inter_coef_rfsh_flag    : 1;
    } reg103;

    RK_U32  reg104_no_use;

    struct SWREG105_VP9CNT_UPD_EN_AVS2_HEADLEN {
        RK_U32      avs2_head_len       : 4;
        RK_U32      count_update_en     : 1;
        RK_U32      reserve             : 27;
    } reg105;

    struct SWREG106_VP9_FRAME_WIDTH_LAST {
        RK_U32      framewidth_last     : 16;
        RK_U32      reserve             : 16;
    } reg106;

    struct SWREG107_VP9_FRAME_HEIGHT_LAST {
        RK_U32      frameheight_last    : 16;
        RK_U32      reserve             : 16;
    } reg107;

    struct SWREG108_VP9_FRAME_WIDTH_GOLDEN {
        RK_U32      framewidth_golden   : 16;
        RK_U32      reserve             : 16;
    } reg108;

    struct SWREG109_VP9_FRAME_HEIGHT_GOLDEN {
        RK_U32      frameheight_golden  : 16;
        RK_U32      reserve             : 16;
    } reg109;

    struct SWREG110_VP9_FRAME_WIDTH_ALTREF {
        RK_U32      framewidth_alfter   : 16;
        RK_U32      reserve             : 16;
    } reg110;

    struct SWREG111_VP9_FRAME_HEIGHT_ALTREF {
        RK_U32      frameheight_alfter  : 16;
        RK_U32      reserve             : 16;
    } reg111;

    struct SWREG112_ERROR_REF_INFO {
        RK_U32      ref_error_field         : 1;
        RK_U32      ref_error_topfield      : 1;
        RK_U32      ref_error_topfield_used : 1;
        RK_U32      ref_error_botfield_used : 1;
        RK_U32      reserve                 : 28;
    } reg112;

} Vdpu34xRegVp9dParam;

typedef struct Vdpu34xRegVp9dAddr_t {

    RK_U32  reg160_delta_prob_base;

    RK_U32  reg161_pps_base;

    RK_U32  reg162_last_prob_base;

    RK_U32  reg163_rps_base;

    RK_U32  reg164_ref_last_base;

    RK_U32  reg165_ref_golden_base;

    RK_U32  reg166_ref_alfter_base;

    RK_U32  reg167_count_prob_base;

    RK_U32  reg168_segidlast_base;

    RK_U32  reg169_segidcur_base;

    RK_U32  reg170_ref_colmv_base;

    RK_U32  reg171_intercmd_base;

    RK_U32  reg172_update_prob_wr_base;

    RK_U32  reg173_179_no_use[7];

    RK_U32  reg180_scanlist_base;

    RK_U32  reg181_196_ref_colmv_base[16];

    RK_U32  reg197_cabactbl_base;
} Vdpu34xRegVp9dAddr;

typedef struct Vdpu34xVp9dRegSet_t {
    Vdpu34xRegCommon        common;
    Vdpu34xRegVp9dParam     vp9d_param;
    Vdpu34xRegCommonAddr    common_addr;
    Vdpu34xRegVp9dAddr      vp9d_addr;
    Vdpu34xRegIrqStatus     irq_status;
    Vdpu34xRegStatistic     statistic;
} Vdpu34xVp9dRegSet;

#endif /* __HAL_VDPU34X_VP9D_H__ */