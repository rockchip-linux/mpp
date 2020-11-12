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
    } vp9_set;

    struct SWREG65_CUR_POC {
        RK_U32      cur_top_poc : 32;
    } cur_poc;

    struct SWREG66_H264_CUR_POC1 {
        RK_U32      cur_bot_poc : 32;
    } no_use;

    struct SWREG67_74_VP9_SEGID_GRP {
        RK_U32      segid_abs_delta                 : 1;
        RK_U32      segid6_frame_qp_delta_en        : 1;
        RK_U32      segid6_frame_qp_delta           : 9;
        RK_U32      segid6_frame_loopfitler_value_en : 1;
        RK_U32      segid6_frame_loopfilter_value   : 7;
        RK_U32      segid6_referinfo_en             : 1;
        RK_U32      segid6_referinfo                : 2;
        RK_U32      segid6_frame_skip_en            : 1;
        RK_U32      reserve                         : 9;
    } vp9_segid_grp0_7[8];

    struct SWREG75_VP9_INFO_LASTFRAME {
        RK_U32      mode_deltas_lastframe           : 14;
        RK_U32      reserve0                        : 2;
        RK_U32      segmentation_enable_lstframe    : 1;
        RK_U32      last_show_frame                 : 1;
        RK_U32      last_intra_only                 : 1;
        RK_U32      last_widthheight_eqcur          : 1;
        RK_U32      color_space_lastkeyframe        : 3;
        RK_U32      reserve1                        : 9;
    } vp9_info_lastframe;

    struct SWREG76_VP9_CPRHEADER_CONFIG {
        RK_U32      tx_mode                     : 3;
        RK_U32      frame_reference_mode        : 2;
        RK_U32      reserve                     : 27;
    } vp9_cprheader_cfg;

    struct SWREG77_VP9_INTERCMD_NUM {
        RK_U32      intercmd_num        : 24;
        RK_U32      reserve             : 8;
    } vp9_intercmd_num;

    struct SWREG78_VP9_LASTTILE_SIZE {
        RK_U32      lasttile_size       : 24;
        RK_U32      reserve             : 8;
    } vp9_lasttile_size;

    struct SWREG79_VP9_LASTF_Y_HOR_VIRSTRIDE {
        RK_U32      lastfy_hor_virstride        : 16;
        RK_U32      reserve                     : 16;
    } vp9_lastf_hor_virstride;

    struct SWREG80_VP9_LASTF_UV_HOR_VIRSTRIDE {
        RK_U32      lastfuv_hor_virstride       : 16;
        RK_U32      reserve                     : 16;
    } vp9_lastf_uv_hor_virstride;

    struct SWREG81_VP9_GOLDENF_Y_HOR_VIRSTRIDE {
        RK_U32      goldenfy_hor_virstride      : 16;
        RK_U32      reserve                     : 16;
    } vp9_goldenf_y_hor_virstride;

    struct SWREG82_VP9_GOLDENF_UV_HOR_VIRSTRIDE {
        RK_U32      goldenfuv_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } vp9_goldenf_uv_hor_virstride;

    struct SWREG83_VP9_ALTREFF_Y_HOR_VIRSTRIDE {
        RK_U32      altreffy_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } vp9_altreff_y_hor_virstride;

    struct SWREG84_VP9_ALTREFF_UV_HOR_VIRSTRIDE {
        RK_U32      altreffuv_hor_virstride     : 16;
        RK_U32      reserve                     : 16;
    } vp9_altreff_uv_hor_virstride;

    struct SWREG85_VP9_LASTF_Y_VIRSTRIDE {
        RK_U32      lastfy_virstride         : 28;
        RK_U32      reserve                  : 4;
    } vp9_lastf_y_virstride;

    struct SWREG86_VP9_GOLDEN_Y_VIRSTRIDE {
        RK_U32      goldeny_virstride       : 28;
        RK_U32      reserve                 : 4;
    } vp9_golden_y_virstride;

    struct SWREG87_VP9_ALTREF_Y_VIRSTRIDE {
        RK_U32      altrefy_virstride       : 28;
        RK_U32      reserve                 : 4;
    } vp9_altref_y_virstride;

    struct SWREG88_VP9_LREF_HOR_SCALE {
        RK_U32      lref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_lref_hor_scale;

    struct SWREG89_VP9_LREF_VER_SCALE {
        RK_U32      lref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_lref_ver_scale;

    struct SWREG90_VP9_GREF_HOR_SCALE {
        RK_U32      gref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_gref_hor_scale;

    struct SWREG91_VP9_GREF_VER_SCALE {
        RK_U32      gref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_gref_ver_scale;

    struct SWREG92_VP9_AREF_HOR_SCALE {
        RK_U32      aref_hor_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_aref_hor_scale;

    struct SWREG93_VP9_AREF_VER_SCALE {
        RK_U32      aref_ver_scale          : 16;
        RK_U32      reserve                 : 16;
    } vp9_aref_ver_scale;

    struct SWREG94_VP9_REF_DELTAS_LASTFRAME {
        RK_U32      ref_deltas_lastframe    : 28;
        RK_U32      reserve                 : 4;
    } vp9_ref_deltas_lastframe;

    RK_U32  reg95_102_no_use[8];

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
    } vp9_prob_en;

    RK_U32  reg104_no_use;

    struct SWREG105_VP9CNT_UPD_EN_AVS2_HEADLEN {
        RK_U32      avs2_head_len       : 4;
        RK_U32      count_update_en     : 1;
        RK_U32      reserve             : 27;
    } vp9cnt_upd_en_avs2_head_len;

    struct SWREG106_VP9_FRAME_WIDTH_LAST {
        RK_U32      framewidth_last     : 16;
        RK_U32      reserve             : 16;
    } vp9_framewidth_last;

    struct SWREG107_VP9_FRAME_HEIGHT_LAST {
        RK_U32      frameheight_last    : 16;
        RK_U32      reserve             : 16;
    } vp9_frameheight_last;

    struct SWREG108_VP9_FRAME_WIDTH_GOLDEN {
        RK_U32      framewidth_golden   : 16;
        RK_U32      reserve             : 16;
    } vp9_framewidth_golden;

    struct SWREG109_VP9_FRAME_HEIGHT_GOLDEN {
        RK_U32      frameheight_golden  : 16;
        RK_U32      reserve             : 16;
    } vp9_frameheight_golden;

    struct SWREG110_VP9_FRAME_WIDTH_ALTREF {
        RK_U32      framewidth_alfter   : 16;
        RK_U32      reserve             : 16;
    } vp9_framewidth_alfter;

    struct SWREG111_VP9_FRAME_HEIGHT_ALTREF {
        RK_U32      frameheight_alfter  : 16;
        RK_U32      reserve             : 16;
    } vp9_frameheight_alfter;

    struct SWREG112_ERROR_REF_INFO {
        RK_U32      ref_error_field         : 1;
        RK_U32      ref_error_topfield      : 1;
        RK_U32      ref_error_topfield_used : 1;
        RK_U32      ref_error_botfield_used : 1;
        RK_U32      reserve                 : 28;
    } err_ref_info;

} Vdpu34xRegVp9dParam;

typedef struct Vdpu34xRegVp9dAddr_t {
    struct SWREG160_VP9_DELTA_PROB_BASE {
        RK_U32 vp9_delta_prob_base  : 32;
    } reg160_no_use;

    struct SWREG161_PPS_BASE {
        RK_U32      pps_base    : 32;
    } pps_base;

    struct SWREG162_VP9_LAST_PROB_BASE {
        RK_U32      last_porb_base  : 32;
    } vp9_last_porb_base;

    struct SWREG163_RPS_BASE {
        RK_U32      rps_base    : 32;
    } rps_base;

    struct SWREG164_VP9_REF_LAST_BASE {
        RK_U32      last_base       : 32;
    } vp9_ref_last_base;

    struct SWREG165_VP9_REF_GOLDEN_BASE {
        RK_U32      golden_base     : 32;
    } vp9_ref_golden_base;

    struct SWREG166_VP9_REF_ALFTER_BASE {
        RK_U32      alfter_base         : 32;
    } vp9_ref_alfter_base;

    struct SWREG167_VP9_COUNT_BASE {
        RK_U32      count_prob_base     : 32;
    } vp9_count_prob_base;

    struct SWREG168_VP9_SEGIDLAST_BASE {
        RK_U32      segidlast_base      : 32;
    } vp9_segidlast_base;

    struct SWREG169_VP9_SEGIDCUR_BASE {
        RK_U32      segidcur_base       : 32;
    } vp9_segidcur_base;

    struct SWREG170_VP9_REF_COLMV_BASE {
        RK_U32      refcolmv_base       : 32;
    } vp9_ref_colmv_base;

    struct SWREG171_VP9_INTERCMD_BASE {
        RK_U32      intercmd_base       : 32;
    } vp9_intercmd_base;

    struct SWREG172_VP9_UPDATE_PROB_WR_BASE {
        RK_U32      update_prob_wr_base : 32;
    } vp9_update_prob_wr_base;


    RK_U32  reg173_179_no_use[7];

    struct SWREG180_H26x_SCANLIST_BASE {
        RK_U32      scanlist_addr   : 32;
    } h26x_scanlist_base;

    struct SWREG181_196_H26x_COLMV_REF_BASE {
        RK_U32      colmv_base  : 32;
    } ref0_15_colmv_base[16];

    struct SWREG197_CABACTBL_BASE {
        RK_U32      cabactbl_base   : 32;
    } cabactbl_base;
} Vdpu34xRegVp9dAddr;

typedef struct Vdpu34xVp9dRegSet_t {
    Vdpu34xRegCommon        common;
    Vdpu34xRegVp9dParam     vp9d_param;
    Vdpu34xRegCommonAddr    common_addr;
    Vdpu34xRegVp9dAddr      vp9d_addr;
    Vdpu34xRegIrqStatus     irq_status;
} Vdpu34xVp9dRegSet;

#endif /* __HAL_VDPU34X_VP9D_H__ */