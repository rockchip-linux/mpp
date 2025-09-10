/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_enc_cfg"

#include <string.h>

#include "rk_venc_cfg.h"
#include "rk_venc_kcfg.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "mpp_cfg.h"
#include "mpp_trie.h"
#include "mpp_enc_cfg_impl.h"

#define ENC_CFG_DBG_FUNC            (0x00000001)
#define ENC_CFG_DBG_INFO            (0x00000002)
#define ENC_CFG_DBG_SET             (0x00000004)
#define ENC_CFG_DBG_GET             (0x00000008)

#define enc_cfg_dbg(flag, fmt, ...) _mpp_dbg_f(mpp_enc_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define enc_cfg_dbg_func(fmt, ...)  enc_cfg_dbg(ENC_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define enc_cfg_dbg_info(fmt, ...)  enc_cfg_dbg(ENC_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define enc_cfg_dbg_set(fmt, ...)   enc_cfg_dbg(ENC_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define enc_cfg_dbg_get(fmt, ...)   enc_cfg_dbg(ENC_CFG_DBG_GET, fmt, ## __VA_ARGS__)

#define MPP_ENC_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    CFG_DEF_START() \
    /*    prefix ElemType base type trie name               update flag type                    element address */ \
    STRUCT_START(codec); \
    ENTRY(prefix, s32,  rk_s32,     type,                   FLAG_BASE(0),                       base, coding); \
    STRUCT_END(codec); \
    STRUCT_START(base) \
    ENTRY(prefix, s32,  rk_s32,     low_delay,              FLAG_INCR,                          base, low_delay) \
    ENTRY(prefix, s32,  rk_s32,     smt1_en,                FLAG_INCR,                          base, smt1_en) \
    ENTRY(prefix, s32,  rk_s32,     smt3_en,                FLAG_INCR,                          base, smt3_en) \
    STRUCT_END(base) \
    STRUCT_START(rc) \
    ENTRY(prefix, s32,  rk_s32,     mode,                   FLAG_BASE(0),                       rc, rc_mode) \
    ENTRY(prefix, s32,  rk_s32,     quality,                FLAG_INCR,                          rc, quality); \
    ENTRY(prefix, s32,  rk_s32,     bps_target,             FLAG_INCR,                          rc, bps_target) \
    ENTRY(prefix, s32,  rk_s32,     bps_max,                FLAG_PREV,                          rc, bps_max) \
    ENTRY(prefix, s32,  rk_s32,     bps_min,                FLAG_PREV,                          rc, bps_min) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_flex,            FLAG_INCR,                          rc, fps_in_flex) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_num,             FLAG_PREV,                          rc, fps_in_num) \
    ENTRY(prefix, s32,  rk_s32,     fps_in_denorm,          FLAG_PREV,                          rc, fps_in_denom) \
    ALIAS(prefix, s32,  rk_s32,     fps_in_denom,           FLAG_PREV,                          rc, fps_in_denom) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_flex,           FLAG_INCR,                          rc, fps_out_flex) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_num,            FLAG_PREV,                          rc, fps_out_num) \
    ENTRY(prefix, s32,  rk_s32,     fps_out_denorm,         FLAG_PREV,                          rc, fps_out_denom) \
    ALIAS(prefix, s32,  rk_s32,     fps_out_denom,          FLAG_PREV,                          rc, fps_out_denom) \
    ENTRY(prefix, s32,  rk_s32,     fps_chg_no_idr,         FLAG_PREV,                          rc, fps_chg_no_idr) \
    ENTRY(prefix, s32,  rk_s32,     gop,                    FLAG_INCR,                          rc, gop) \
    ENTRY(prefix, kptr, void *,     ref_cfg,                FLAG_INCR,                          rc, ref_cfg) \
    ENTRY(prefix, u32,  rk_u32,     max_reenc_times,        FLAG_INCR,                          rc, max_reenc_times) \
    ENTRY(prefix, u32,  rk_u32,     priority,               FLAG_INCR,                          rc, rc_priority) \
    ENTRY(prefix, u32,  rk_u32,     drop_mode,              FLAG_INCR,                          rc, drop_mode) \
    ENTRY(prefix, u32,  rk_u32,     drop_thd,               FLAG_PREV,                          rc, drop_threshold) \
    ENTRY(prefix, u32,  rk_u32,     drop_gap,               FLAG_PREV,                          rc, drop_gap) \
    ENTRY(prefix, s32,  rk_s32,     max_i_prop,             FLAG_INCR,                          rc, max_i_prop) \
    ENTRY(prefix, s32,  rk_s32,     min_i_prop,             FLAG_INCR,                          rc, min_i_prop) \
    ENTRY(prefix, s32,  rk_s32,     init_ip_ratio,          FLAG_INCR,                          rc, init_ip_ratio) \
    ENTRY(prefix, u32,  rk_u32,     super_mode,             FLAG_INCR,                          rc, super_mode) \
    ENTRY(prefix, u32,  rk_u32,     super_i_thd,            FLAG_PREV,                          rc, super_i_thd) \
    ENTRY(prefix, u32,  rk_u32,     super_p_thd,            FLAG_PREV,                          rc, super_p_thd) \
    ENTRY(prefix, u32,  rk_u32,     debreath_en,            FLAG_INCR,                          rc, debreath_en) \
    ENTRY(prefix, u32,  rk_u32,     debreath_strength,      FLAG_PREV,                          rc, debre_strength) \
    ENTRY(prefix, s32,  rk_s32,     qp_init,                FLAG_REC_INC(0),                    rc, qp_init) \
    ENTRY(prefix, s32,  rk_s32,     qp_min,                 FLAG_REC_INC(1),                    rc, qp_min) \
    ENTRY(prefix, s32,  rk_s32,     qp_max,                 FLAG_REPLAY(1),                     rc, qp_max) \
    ENTRY(prefix, s32,  rk_s32,     qp_min_i,               FLAG_REC_INC(2),                    rc, qp_min_i) \
    ENTRY(prefix, s32,  rk_s32,     qp_max_i,               FLAG_REPLAY(2),                     rc, qp_max_i) \
    ENTRY(prefix, s32,  rk_s32,     qp_step,                FLAG_REC_INC(3),                    rc, qp_max_step) \
    ENTRY(prefix, s32,  rk_s32,     qp_ip,                  FLAG_REC_INC(4),                    rc, qp_delta_ip) \
    ENTRY(prefix, s32,  rk_s32,     qp_vi,                  FLAG_INCR,                          rc, qp_delta_vi) \
    ENTRY(prefix, s32,  rk_s32,     hier_qp_en,             FLAG_INCR,                          rc, hier_qp_en) \
    STRCT(prefix, st,   void *,     hier_qp_delta,          FLAG_PREV,                          rc, hier_qp_delta); \
    STRCT(prefix, st,   void *,     hier_frame_num,         FLAG_PREV,                          rc, hier_frame_num); \
    ENTRY(prefix, s32,  rk_s32,     stats_time,             FLAG_INCR,                          rc, stats_time) \
    ENTRY(prefix, u32,  rk_u32,     refresh_en,             FLAG_INCR,                          rc, refresh_en) \
    ENTRY(prefix, u32,  rk_u32,     refresh_mode,           FLAG_PREV,                          rc, refresh_mode) \
    ENTRY(prefix, u32,  rk_u32,     refresh_num,            FLAG_PREV,                          rc, refresh_num) \
    ENTRY(prefix, s32,  rk_s32,     fqp_min_i,              FLAG_INCR,                          rc, fqp_min_i) \
    ENTRY(prefix, s32,  rk_s32,     fqp_min_p,              FLAG_PREV,                          rc, fqp_min_p) \
    ENTRY(prefix, s32,  rk_s32,     fqp_max_i,              FLAG_PREV,                          rc, fqp_max_i) \
    ENTRY(prefix, s32,  rk_s32,     fqp_max_p,              FLAG_PREV,                          rc, fqp_max_p) \
    ENTRY(prefix, s32,  rk_s32,     mt_st_swth_frm_qp,      FLAG_PREV,                          rc, mt_st_swth_frm_qp) \
    ENTRY(prefix, s32,  rk_s32,     inst_br_lvl,            FLAG_INCR,                          rc, inst_br_lvl) \
    STRUCT_END(rc) \
    STRUCT_START(prep); \
    ENTRY(prefix, s32,  rk_s32,     width,                  FLAG_BASE(0),                       prep, width_set); \
    ENTRY(prefix, s32,  rk_s32,     height,                 FLAG_PREV,                          prep, height_set); \
    ENTRY(prefix, s32,  rk_s32,     max_width,              FLAG_PREV,                          prep, max_width); \
    ENTRY(prefix, s32,  rk_s32,     max_height,             FLAG_PREV,                          prep, max_height); \
    ENTRY(prefix, s32,  rk_s32,     hor_stride,             FLAG_PREV,                          prep, hor_stride); \
    ENTRY(prefix, s32,  rk_s32,     ver_stride,             FLAG_PREV,                          prep, ver_stride); \
    ENTRY(prefix, s32,  rk_s32,     format,                 FLAG_INCR,                          prep, format); \
    ENTRY(prefix, s32,  rk_s32,     format_out,             FLAG_PREV,                          prep, format_out); \
    ENTRY(prefix, s32,  rk_s32,     chroma_ds_mode,         FLAG_PREV,                          prep, chroma_ds_mode); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_en,          FLAG_PREV,                          prep, fix_chroma_en); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_u,           FLAG_PREV,                          prep, fix_chroma_u); \
    ENTRY(prefix, s32,  rk_s32,     fix_chroma_v,           FLAG_PREV,                          prep, fix_chroma_v); \
    ENTRY(prefix, s32,  rk_s32,     colorspace,             FLAG_INCR,                          prep, color); \
    ENTRY(prefix, s32,  rk_s32,     colorprim,              FLAG_INCR,                          prep, colorprim); \
    ENTRY(prefix, s32,  rk_s32,     colortrc,               FLAG_INCR,                          prep, colortrc); \
    ENTRY(prefix, s32,  rk_s32,     colorrange,             FLAG_INCR,                          prep, range); \
    ALIAS(prefix, s32,  rk_s32,     range,                  FLAG_PREV,                          prep, range); \
    ENTRY(prefix, s32,  rk_s32,     range_out,              FLAG_PREV,                          prep, range_out); \
    ENTRY(prefix, s32,  rk_s32,     rotation,               FLAG_INCR,                          prep, rotation_ext); \
    ENTRY(prefix, s32,  rk_s32,     mirroring,              FLAG_INCR,                          prep, mirroring_ext); \
    ENTRY(prefix, s32,  rk_s32,     flip,                   FLAG_INCR,                          prep, flip); \
    STRUCT_END(prep); \
    STRUCT_START(h264); \
    /* h264 config */ \
    ENTRY(prefix, s32,  rk_s32,     stream_type,            FLAG_BASE(0),                       h264, stream_type); \
    ENTRY(prefix, s32,  rk_s32,     profile,                FLAG_INCR,                          h264, profile); \
    ENTRY(prefix, s32,  rk_s32,     level,                  FLAG_PREV,                          h264, level); \
    ENTRY(prefix, u32,  rk_u32,     poc_type,               FLAG_INCR,                          h264, poc_type); \
    ENTRY(prefix, u32,  rk_u32,     log2_max_poc_lsb,       FLAG_INCR,                          h264, log2_max_poc_lsb); \
    ENTRY(prefix, u32,  rk_u32,     log2_max_frm_num,       FLAG_INCR,                          h264, log2_max_frame_num); \
    ENTRY(prefix, u32,  rk_u32,     gaps_not_allowed,       FLAG_INCR,                          h264, gaps_not_allowed); \
    ENTRY(prefix, s32,  rk_s32,     cabac_en,               FLAG_INCR,                          h264, entropy_coding_mode_ex); \
    ENTRY(prefix, s32,  rk_s32,     cabac_idc,              FLAG_PREV,                          h264, cabac_init_idc_ex); \
    ENTRY(prefix, s32,  rk_s32,     trans8x8,               FLAG_INCR,                          h264, transform8x8_mode_ex); \
    ENTRY(prefix, s32,  rk_s32,     const_intra,            FLAG_INCR,                          h264, constrained_intra_pred_mode); \
    ENTRY(prefix, s32,  rk_s32,     scaling_list,           FLAG_INCR,                          h264, scaling_list_mode); \
    ENTRY(prefix, s32,  rk_s32,     cb_qp_offset,           FLAG_INCR,                          h264, chroma_cb_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     cr_qp_offset,           FLAG_PREV,                          h264, chroma_cr_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     dblk_disable,           FLAG_INCR,                          h264, deblock_disable); \
    ENTRY(prefix, s32,  rk_s32,     dblk_alpha,             FLAG_PREV,                          h264, deblock_offset_alpha); \
    ENTRY(prefix, s32,  rk_s32,     dblk_beta,              FLAG_PREV,                          h264, deblock_offset_beta); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                FLAG_REPLAY(0),                     rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 FLAG_REPLAY(1),                     rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 FLAG_REPLAY(1),                     rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               FLAG_REPLAY(2),                     rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               FLAG_REPLAY(2),                     rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                FLAG_REPLAY(3),                     rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            FLAG_REPLAY(4),                     rc, qp_delta_ip); \
    ENTRY(prefix, s32,  rk_s32,     max_tid,                FLAG_INCR,                          h264, max_tid); \
    ENTRY(prefix, s32,  rk_s32,     max_ltr,                FLAG_INCR,                          h264, max_ltr_frames); \
    ENTRY(prefix, s32,  rk_s32,     prefix_mode,            FLAG_INCR,                          h264, prefix_mode); \
    ENTRY(prefix, s32,  rk_s32,     base_layer_pid,         FLAG_INCR,                          h264, base_layer_pid); \
    ENTRY(prefix, u32,  rk_u32,     constraint_set,         FLAG_INCR,                          h264, constraint_set); \
    ENTRY(prefix, u32,  rk_u32,     vui_en,                 FLAG_INCR,                          h264, vui, vui_en); \
    STRUCT_END(h264); \
    /* h265 config*/ \
    STRUCT_START(h265); \
    ENTRY(prefix, s32,  rk_s32,     profile,                FLAG_BASE(0),                       h265, profile); \
    ENTRY(prefix, s32,  rk_s32,     tier,                   FLAG_PREV,                          h265, tier); \
    ENTRY(prefix, s32,  rk_s32,     level,                  FLAG_PREV,                          h265, level); \
    ENTRY(prefix, u32,  rk_u32,     scaling_list,           FLAG_INCR,                          h265, trans_cfg, scaling_list_mode); \
    ENTRY(prefix, s32,  rk_s32,     cb_qp_offset,           FLAG_PREV,                          h265, trans_cfg, cb_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     cr_qp_offset,           FLAG_PREV,                          h265, trans_cfg, cr_qp_offset); \
    ENTRY(prefix, s32,  rk_s32,     diff_cu_qp_delta_depth, FLAG_PREV,                          h265, trans_cfg, diff_cu_qp_delta_depth); \
    ENTRY(prefix, u32,  rk_u32,     dblk_disable,           FLAG_INCR,                          h265, dblk_cfg, slice_deblocking_filter_disabled_flag); \
    ENTRY(prefix, s32,  rk_s32,     dblk_alpha,             FLAG_PREV,                          h265, dblk_cfg, slice_beta_offset_div2); \
    ENTRY(prefix, s32,  rk_s32,     dblk_beta,              FLAG_PREV,                          h265, dblk_cfg, slice_tc_offset_div2); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                FLAG_REPLAY(0),                     rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 FLAG_REPLAY(1),                     rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 FLAG_REPLAY(1),                     rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               FLAG_REPLAY(2),                     rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               FLAG_REPLAY(2),                     rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                FLAG_REPLAY(3),                     rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            FLAG_REPLAY(4),                     rc, qp_delta_ip); \
    ENTRY(prefix, s32,  rk_s32,     sao_luma_disable,       FLAG_INCR,                          h265, sao_cfg, slice_sao_luma_disable); \
    ENTRY(prefix, s32,  rk_s32,     sao_chroma_disable,     FLAG_PREV,                          h265, sao_cfg, slice_sao_chroma_disable); \
    ENTRY(prefix, s32,  rk_s32,     sao_bit_ratio,          FLAG_PREV,                          h265, sao_cfg, sao_bit_ratio); \
    ENTRY(prefix, u32,  rk_u32,     lpf_acs_sli_en,         FLAG_INCR,                          h265, lpf_acs_sli_en); \
    ENTRY(prefix, u32,  rk_u32,     lpf_acs_tile_disable,   FLAG_INCR,                          h265, lpf_acs_tile_disable); \
    ENTRY(prefix, s32,  rk_s32,     auto_tile,              FLAG_INCR,                          h265, auto_tile); \
    ENTRY(prefix, s32,  rk_s32,     max_tid,                FLAG_INCR,                          h265, max_tid); \
    ENTRY(prefix, s32,  rk_s32,     max_ltr,                FLAG_INCR,                          h265, max_ltr_frames); \
    ENTRY(prefix, s32,  rk_s32,     base_layer_pid,         FLAG_INCR,                          h265, base_layer_pid); \
    ENTRY(prefix, s32,  rk_s32,     const_intra,            FLAG_INCR,                          h265, const_intra_pred); \
    ENTRY(prefix, s32,  rk_s32,     lcu_size,               FLAG_INCR,                          h265, max_cu_size); \
    ENTRY(prefix, s32,  rk_s32,     vui_en,                 FLAG_INCR,                          h265, vui, vui_en); \
    STRUCT_END(h265); \
    /* vp8 config */ \
    STRUCT_START(vp8); \
    ENTRY(prefix, s32,  rk_s32,     disable_ivf,            FLAG_BASE(0),                       vp8, disable_ivf); \
    ALIAS(prefix, s32,  rk_s32,     qp_init,                FLAG_REPLAY(0),                     rc, qp_init); \
    ALIAS(prefix, s32,  rk_s32,     qp_min,                 FLAG_REPLAY(1),                     rc, qp_min); \
    ALIAS(prefix, s32,  rk_s32,     qp_max,                 FLAG_REPLAY(1),                     rc, qp_max); \
    ALIAS(prefix, s32,  rk_s32,     qp_min_i,               FLAG_REPLAY(2),                     rc, qp_min_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_max_i,               FLAG_REPLAY(2),                     rc, qp_max_i); \
    ALIAS(prefix, s32,  rk_s32,     qp_step,                FLAG_REPLAY(3),                     rc, qp_max_step); \
    ALIAS(prefix, s32,  rk_s32,     qp_delta_ip,            FLAG_REPLAY(4),                     rc, qp_delta_ip); \
    STRUCT_END(vp8); \
    /* jpeg config */ \
    STRUCT_START(jpeg); \
    ENTRY(prefix, st,   rk_s32,     q_mode,                 FLAG_BASE(0),                       jpeg, q_mode); \
    ENTRY(prefix, s32,  rk_s32,     quant,                  FLAG_INCR,                          jpeg, quant_ext); \
    ENTRY(prefix, s32,  rk_s32,     q_factor,               FLAG_INCR,                          jpeg, q_factor_ext); \
    ENTRY(prefix, s32,  rk_s32,     qf_max,                 FLAG_PREV,                          jpeg, qf_max_ext); \
    ENTRY(prefix, s32,  rk_s32,     qf_min,                 FLAG_PREV,                          jpeg, qf_min_ext); \
    ENTRY(prefix, st,   void *,     qtable_y,               FLAG_INCR,                          jpeg, qtable_y); \
    ENTRY(prefix, st,   void *,     qtable_u,               FLAG_PREV,                          jpeg, qtable_u); \
    ENTRY(prefix, st,   void *,     qtable_v,               FLAG_PREV,                          jpeg, qtable_v); \
    STRUCT_END(jpeg); \
    /* split config */ \
    STRUCT_START(split); \
    ENTRY(prefix, u32,  rk_u32,     mode,                   FLAG_BASE(0),                       split, split_mode) \
    ENTRY(prefix, u32,  rk_u32,     arg,                    FLAG_INCR,                          split, split_arg) \
    ENTRY(prefix, u32,  rk_u32,     out,                    FLAG_INCR,                          split, split_out) \
    STRUCT_END(split); \
    /* hardware detail config */ \
    STRUCT_START(hw); \
    ENTRY(prefix, s32,  rk_s32,     qp_row,                 FLAG_BASE(0),                       hw, qp_delta_row) \
    ENTRY(prefix, s32,  rk_s32,     qp_row_i,               FLAG_INCR,                          hw, qp_delta_row_i) \
    STRCT(prefix, st,   void *,     aq_thrd_i,              FLAG_INCR,                          hw, aq_thrd_i) \
    STRCT(prefix, st,   void *,     aq_thrd_p,              FLAG_INCR,                          hw, aq_thrd_p) \
    STRCT(prefix, st,   void *,     aq_step_i,              FLAG_INCR,                          hw, aq_step_i) \
    STRCT(prefix, st,   void *,     aq_step_p,              FLAG_INCR,                          hw, aq_step_p) \
    ENTRY(prefix, s32,  rk_s32,     mb_rc_disable,          FLAG_INCR,                          hw, mb_rc_disable) \
    STRCT(prefix, st,   void *,     aq_rnge_arr,            FLAG_INCR,                          hw, aq_rnge_arr) \
    STRCT(prefix, st,   void *,     mode_bias,              FLAG_INCR,                          hw, mode_bias) \
    ENTRY(prefix, s32,  rk_s32,     skip_bias_en,           FLAG_INCR,                          hw, skip_bias_en) \
    ENTRY(prefix, s32,  rk_s32,     skip_sad,               FLAG_PREV,                          hw, skip_sad) \
    ENTRY(prefix, s32,  rk_s32,     skip_bias,              FLAG_PREV,                          hw, skip_bias) \
    ENTRY(prefix, s32,  rk_s32,     qbias_i,                FLAG_INCR,                          hw, qbias_i) \
    ENTRY(prefix, s32,  rk_s32,     qbias_p,                FLAG_INCR,                          hw, qbias_p) \
    ENTRY(prefix, s32,  rk_s32,     qbias_en,               FLAG_INCR,                          hw, qbias_en) \
    STRCT(prefix, st,   void *,     qbias_arr,              FLAG_INCR,                          hw, qbias_arr) \
    ENTRY(prefix, s32,  rk_s32,     flt_str_i,              FLAG_INCR,                          hw, flt_str_i) \
    ENTRY(prefix, s32,  rk_s32,     flt_str_p,              FLAG_INCR,                          hw, flt_str_p) \
    STRUCT_END(hw); \
    /* quality fine tuning config */ \
    STRUCT_START(tune); \
    ENTRY(prefix, s32,  rk_s32,     scene_mode,             FLAG_BASE(0),                       tune, scene_mode); \
    ENTRY(prefix, s32,  rk_s32,     se_mode,                FLAG_INCR,                          tune, se_mode); \
    ENTRY(prefix, s32,  rk_s32,     deblur_en,              FLAG_INCR,                          tune, deblur_en); \
    ENTRY(prefix, s32,  rk_s32,     deblur_str,             FLAG_INCR,                          tune, deblur_str); \
    ENTRY(prefix, s32,  rk_s32,     anti_flicker_str,       FLAG_INCR,                          tune, anti_flicker_str); \
    ENTRY(prefix, s32,  rk_s32,     lambda_idx_i,           FLAG_INCR,                          tune, lambda_idx_i); \
    ENTRY(prefix, s32,  rk_s32,     lambda_idx_p,           FLAG_INCR,                          tune, lambda_idx_p); \
    ENTRY(prefix, s32,  rk_s32,     atr_str_i,              FLAG_INCR,                          tune, atr_str_i); \
    ENTRY(prefix, s32,  rk_s32,     atr_str_p,              FLAG_INCR,                          tune, atr_str_p); \
    ENTRY(prefix, s32,  rk_s32,     atl_str,                FLAG_INCR,                          tune, atl_str); \
    ENTRY(prefix, s32,  rk_s32,     sao_str_i,              FLAG_INCR,                          tune, sao_str_i); \
    ENTRY(prefix, s32,  rk_s32,     sao_str_p,              FLAG_INCR,                          tune, sao_str_p); \
    ENTRY(prefix, s32,  rk_s32,     rc_container,           FLAG_INCR,                          tune, rc_container); \
    ENTRY(prefix, s32,  rk_s32,     vmaf_opt,               FLAG_INCR,                          tune, vmaf_opt); \
    ENTRY(prefix, s32,  rk_s32,     motion_static_switch_enable, FLAG_INCR,                     tune, motion_static_switch_enable); \
    ENTRY(prefix, s32,  rk_s32,     atf_str,                FLAG_INCR,                          tune, atf_str); \
    ENTRY(prefix, s32,  rk_s32,     lgt_chg_lvl,            FLAG_INCR,                          tune, lgt_chg_lvl); \
    ENTRY(prefix, s32,  rk_s32,     static_frm_num,         FLAG_INCR,                          tune, static_frm_num); \
    ENTRY(prefix, s32,  rk_s32,     madp16_th,              FLAG_INCR,                          tune, madp16_th); \
    ENTRY(prefix, s32,  rk_s32,     skip16_wgt,             FLAG_INCR,                          tune, skip16_wgt); \
    ENTRY(prefix, s32,  rk_s32,     skip32_wgt,             FLAG_INCR,                          tune, skip32_wgt); \
    ENTRY(prefix, s32,  rk_s32,     speed,                  FLAG_INCR,                          tune, speed); \
    ENTRY(prefix, s32,  rk_s32,     bg_delta_qp_i,          FLAG_INCR,                          tune, bg_delta_qp_i); \
    ENTRY(prefix, s32,  rk_s32,     bg_delta_qp_p,          FLAG_PREV,                          tune, bg_delta_qp_p); \
    ENTRY(prefix, s32,  rk_s32,     fg_delta_qp_i,          FLAG_PREV,                          tune, fg_delta_qp_i); \
    ENTRY(prefix, s32,  rk_s32,     fg_delta_qp_p,          FLAG_PREV,                          tune, fg_delta_qp_p); \
    ENTRY(prefix, s32,  rk_s32,     bmap_qpmin_i,           FLAG_PREV,                          tune, bmap_qpmin_i); \
    ENTRY(prefix, s32,  rk_s32,     bmap_qpmin_p,           FLAG_PREV,                          tune, bmap_qpmin_p); \
    ENTRY(prefix, s32,  rk_s32,     bmap_qpmax_i,           FLAG_PREV,                          tune, bmap_qpmax_i); \
    ENTRY(prefix, s32,  rk_s32,     bmap_qpmax_p,           FLAG_PREV,                          tune, bmap_qpmax_p); \
    ENTRY(prefix, s32,  rk_s32,     min_bg_fqp,             FLAG_PREV,                          tune, min_bg_fqp); \
    ENTRY(prefix, s32,  rk_s32,     max_bg_fqp,             FLAG_PREV,                          tune, max_bg_fqp); \
    ENTRY(prefix, s32,  rk_s32,     min_fg_fqp,             FLAG_PREV,                          tune, min_fg_fqp); \
    ENTRY(prefix, s32,  rk_s32,     max_fg_fqp,             FLAG_PREV,                          tune, max_fg_fqp); \
    ENTRY(prefix, s32,  rk_s32,     fg_area,                FLAG_PREV,                          tune, fg_area); \
    ENTRY(prefix, s32,  rk_s32,     qpmap_en,               FLAG_INCR,                          tune, qpmap_en); \
    STRUCT_END(tune); \
    CFG_DEF_END()

static rk_s32 mpp_enc_cfg_impl_init(void *entry, KmppObj obj, const char *caller)
{
    MppEncCfgSet *cfg = (MppEncCfgSet *)entry;
    rk_u32 i;

    cfg->rc.max_reenc_times = 1;

    cfg->prep.color = MPP_FRAME_SPC_UNSPECIFIED;
    cfg->prep.colorprim = MPP_FRAME_PRI_UNSPECIFIED;
    cfg->prep.colortrc = MPP_FRAME_TRC_UNSPECIFIED;
    cfg->prep.format_out = MPP_CHROMA_UNSPECIFIED;
    cfg->prep.chroma_ds_mode = MPP_FRAME_CHROMA_DOWN_SAMPLE_MODE_NONE;
    cfg->prep.fix_chroma_en = 0;
    cfg->prep.range_out = MPP_FRAME_RANGE_UNSPECIFIED;

    for (i = 0; i < MPP_ARRAY_ELEMS(cfg->hw.mode_bias); i++)
        cfg->hw.mode_bias[i] = 8;

    cfg->hw.skip_sad  = 8;
    cfg->hw.skip_bias = 8;

    (void) obj;
    (void) caller;

    return rk_ok;
}

static rk_s32 mpp_enc_cfg_impl_dump(void *entry)
{
    MppEncCfgSet *cfg = (MppEncCfgSet *)entry;
    rk_u32 *flag;

    if (!cfg)
        return rk_nok;

    flag = mpp_enc_cfg_prep_change(cfg);

    mpp_logi("cfg %p prep flag %p\n", cfg, flag);
    mpp_logi("prep change       %x\n", *flag);
    mpp_logi("width             %4d -> %4d\n", cfg->prep.width, cfg->prep.width_set);
    mpp_logi("height            %4d -> %4d\n", cfg->prep.height, cfg->prep.height_set);
    mpp_logi("hor_stride        %d\n", cfg->prep.hor_stride);
    mpp_logi("ver_stride        %d\n", cfg->prep.ver_stride);
    mpp_logi("format_in         %d\n", cfg->prep.format);
    mpp_logi("format_out        %d\n", cfg->prep.format_out);
    mpp_logi("rotation          %d -> %d\n", cfg->prep.rotation, cfg->prep.rotation_ext);
    mpp_logi("mirroring         %d -> %d\n", cfg->prep.mirroring, cfg->prep.mirroring_ext);

    return rk_ok;
}

#define KMPP_OBJ_NAME               mpp_enc_cfg
#define KMPP_OBJ_INTF_TYPE          MppEncCfg
#define KMPP_OBJ_IMPL_TYPE          MppEncCfgSet
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_ENC_CFG
#define KMPP_OBJ_FUNC_INIT          mpp_enc_cfg_impl_init
#define KMPP_OBJ_FUNC_DUMP          mpp_enc_cfg_impl_dump
#define KMPP_OBJ_ENTRY_TABLE        MPP_ENC_CFG_ENTRY_TABLE
#define KMPP_OBJ_ACCESS_DISABLE
#define KMPP_OBJ_HIERARCHY_ENABLE
#include "kmpp_obj_helper.h"

#define TO_CHANGE_POS(name, ...) \
    static rk_s32 to_change_pos_##name(void) \
    { \
        static rk_s32 CONCAT_US(name, flag, pos) = -1; \
        if (CONCAT_US(name, flag, pos) < 0) { \
            KmppEntry *tbl = NULL; \
            kmpp_objdef_get_entry(mpp_enc_cfg_def, CONCAT_STR(name, __VA_ARGS__), &tbl); \
            CONCAT_US(name, flag, pos) = tbl ? (tbl->tbl.flag_offset / 8) : 0; \
        } \
        return CONCAT_US(name, flag, pos); \
    }

/* vcodec type -> base.change */
TO_CHANGE_POS(rc, mode)
TO_CHANGE_POS(prep, width)
TO_CHANGE_POS(h264, stream_type)
TO_CHANGE_POS(h265, profile)
TO_CHANGE_POS(vp8, disable_ivf)
TO_CHANGE_POS(jpeg, q_mode)
TO_CHANGE_POS(hw, qp_row)
TO_CHANGE_POS(tune, scene_mode)

MPP_RET mpp_enc_cfg_init(MppEncCfg *cfg)
{
    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, 0);

    return mpp_enc_cfg_get(cfg);
}

RK_S32 mpp_enc_cfg_init_k(MppEncCfg *cfg)
{
    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, 0);

    return mpp_venc_kcfg_init(cfg, MPP_VENC_KCFG_TYPE_ST_CFG);
}

RK_S32 mpp_enc_cfg_deinit(MppEncCfg cfg)
{
    return kmpp_obj_put_f(cfg);
}

#define kmpp_obj_set_S32(obj, name, val) \
    kmpp_obj_set_s32(obj, name, val)
#define kmpp_obj_set_U32(obj, name, val) \
    kmpp_obj_set_u32(obj, name, val)
#define kmpp_obj_set_S64(obj, name, val) \
    kmpp_obj_set_s64(obj, name, val)
#define kmpp_obj_set_U64(obj, name, val) \
    kmpp_obj_set_u64(obj, name, val)
#define kmpp_obj_set_Ptr(obj, name, val) \
    kmpp_obj_set_ptr(obj, name, val)
#define kmpp_obj_set_St(obj, name, val) \
    kmpp_obj_set_st(obj, name, val)

#define ENC_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppEncCfg cfg, const char *name, in_type val) \
    { \
        return kmpp_obj_set_##cfg_type((KmppObj)cfg, name, val); \
    }

ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s32, RK_S32, S32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u32, RK_U32, U32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s64, RK_S64, S64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u64, RK_U64, U64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_ptr, void *, Ptr);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_st,  void *, St);

#define kmpp_obj_get_S32(obj, name, val) \
    kmpp_obj_get_s32(obj, name, val)
#define kmpp_obj_get_U32(obj, name, val) \
    kmpp_obj_get_u32(obj, name, val)
#define kmpp_obj_get_S64(obj, name, val) \
    kmpp_obj_get_s64(obj, name, val)
#define kmpp_obj_get_U64(obj, name, val) \
    kmpp_obj_get_u64(obj, name, val)
#define kmpp_obj_get_Ptr(obj, name, val) \
    kmpp_obj_get_ptr(obj, name, val)
#define kmpp_obj_get_St(obj, name, val) \
    kmpp_obj_get_st(obj, name, val)

#define ENC_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppEncCfg cfg, const char *name, in_type *val) \
    { \
        return kmpp_obj_get_##cfg_type((KmppObj)cfg, name, val); \
    }

ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s32, RK_S32, S32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u32, RK_U32, U32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s64, RK_S64, S64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u64, RK_U64, U64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_ptr, void *, Ptr);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_st,  void  , St);

void mpp_enc_cfg_show(void)
{
    MppTrie trie = kmpp_objdef_get_trie(mpp_enc_cfg_def);
    MppTrieInfo *root;

    if (!trie)
        return;

    mpp_log("dumping valid configure string start\n");

    root = mpp_trie_get_info_first(trie);
    if (root) {
        MppTrieInfo *node = root;
        rk_s32 len = mpp_trie_get_name_max(trie);

        do {
            if (mpp_trie_info_is_self(node))
                continue;

            if (node->ctx_len == sizeof(KmppEntry)) {
                KmppEntry *entry = (KmppEntry *)mpp_trie_info_ctx(node);

                mpp_log("%-*s %-6s | %-6d | %-4d | %-4x\n", len, mpp_trie_info_name(node),
                        strof_elem_type(entry->tbl.elem_type), entry->tbl.elem_offset,
                        entry->tbl.elem_size, entry->tbl.flag_offset);
            } else {
                mpp_log("%-*s size - %d\n", len, mpp_trie_info_name(node), node->ctx_len);
            }
        } while ((node = mpp_trie_get_info_next(trie, node)));
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("enc cfg size %d count %d with trie node %d size %d\n",
            sizeof(MppEncCfgSet), mpp_trie_get_info_count(trie),
            mpp_trie_get_node_count(trie), mpp_trie_get_buf_size(trie));
}

#define GET_ENC_CFG_CHANGE(name) \
    rk_u32 *mpp_enc_cfg_##name##_change(MppEncCfgSet *cfg) \
    { \
        if (cfg) \
            return (rk_u32 *)(POS_TO_FLAG(cfg, to_change_pos_##name())); \
        return NULL; \
    }

GET_ENC_CFG_CHANGE(prep)
GET_ENC_CFG_CHANGE(rc)
GET_ENC_CFG_CHANGE(hw)
GET_ENC_CFG_CHANGE(tune)
GET_ENC_CFG_CHANGE(h264)
GET_ENC_CFG_CHANGE(h265)
GET_ENC_CFG_CHANGE(jpeg)
GET_ENC_CFG_CHANGE(vp8)
