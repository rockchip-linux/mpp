/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_enc_cfg"

#include <string.h>

#include "rk_venc_cfg.h"

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

#define get_srv_enc_cfg_f() \
    ({ \
        MppEncCfgSrv *__tmp; \
        if (srv_enc_cfg) { \
            __tmp = srv_enc_cfg; \
        } else { \
            mpp_enc_cfg_srv_init(); \
            __tmp = srv_enc_cfg; \
            if (!__tmp) \
                mpp_err("mpp enc cfg srv not init at %s\n", __FUNCTION__); \
        } \
        __tmp; \
    })

typedef struct MppEncCfgSrv_t {
    MppTrie     trie;
} MppEncCfgSrv;

static MppEncCfgSrv *srv_enc_cfg = NULL;
static RK_U32 mpp_enc_cfg_debug = 0;

#define EXPAND_AS_TRIE(base, name, cfg_type, flag, field_change, field_data) \
    do { \
        MppCfgInfo tmp = { \
            CFG_FUNC_TYPE_##cfg_type, \
            (RK_U32)((long)&(((MppEncCfgSet *)0)->field_change.change)), \
            flag, \
            (RK_U32)((long)&(((MppEncCfgSet *)0)->field_change.field_data)), \
            sizeof((((MppEncCfgSet *)0)->field_change.field_data)), \
        }; \
        mpp_trie_add_info(srv->trie, #base":"#name, &tmp, sizeof(tmp)); \
    } while (0);

#define ENTRY_TABLE(ENTRY)  \
    /* base config */ \
    ENTRY(base, low_delay,      S32,        MPP_ENC_BASE_CFG_CHANGE_LOW_DELAY,      base, low_delay) \
    ENTRY(base, smart_en,       S32,        MPP_ENC_BASE_CFG_CHANGE_SMART_EN,       base, smart_en) \
    /* rc config */ \
    ENTRY(rc,   mode,           S32,        MPP_ENC_RC_CFG_CHANGE_RC_MODE,          rc, rc_mode) \
    ENTRY(rc,   bps_target,     S32,        MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_target) \
    ENTRY(rc,   bps_max,        S32,        MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_max) \
    ENTRY(rc,   bps_min,        S32,        MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_min) \
    ENTRY(rc,   fps_in_flex,    S32,        MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_flex) \
    ENTRY(rc,   fps_in_num,     S32,        MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_num) \
    ENTRY(rc,   fps_in_denom,   S32,        MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_denom) \
    ENTRY(rc,   fps_in_denorm,  S32,        MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_denom) \
    ENTRY(rc,   fps_out_flex,   S32,        MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_flex) \
    ENTRY(rc,   fps_out_num,    S32,        MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_num) \
    ENTRY(rc,   fps_out_denom,  S32,        MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_denom) \
    ENTRY(rc,   fps_out_denorm, S32,        MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_denom) \
    ENTRY(rc,   fps_chg_no_idr, S32,        MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_chg_no_idr) \
    ENTRY(rc,   gop,            S32,        MPP_ENC_RC_CFG_CHANGE_GOP,              rc, gop) \
    ENTRY(rc,   ref_cfg,        Ptr,        MPP_ENC_RC_CFG_CHANGE_GOP_REF_CFG,      rc, ref_cfg) \
    ENTRY(rc,   max_reenc_times,U32,        MPP_ENC_RC_CFG_CHANGE_MAX_REENC,        rc, max_reenc_times) \
    ENTRY(rc,   priority,       U32,        MPP_ENC_RC_CFG_CHANGE_PRIORITY,         rc, rc_priority) \
    ENTRY(rc,   drop_mode,      U32,        MPP_ENC_RC_CFG_CHANGE_DROP_FRM,         rc, drop_mode) \
    ENTRY(rc,   drop_thd,       U32,        MPP_ENC_RC_CFG_CHANGE_DROP_FRM,         rc, drop_threshold) \
    ENTRY(rc,   drop_gap,       U32,        MPP_ENC_RC_CFG_CHANGE_DROP_FRM,         rc, drop_gap) \
    ENTRY(rc,   max_i_prop,     S32,        MPP_ENC_RC_CFG_CHANGE_MAX_I_PROP,       rc, max_i_prop) \
    ENTRY(rc,   min_i_prop,     S32,        MPP_ENC_RC_CFG_CHANGE_MIN_I_PROP,       rc, min_i_prop) \
    ENTRY(rc,   init_ip_ratio,  S32,        MPP_ENC_RC_CFG_CHANGE_INIT_IP_RATIO,    rc, init_ip_ratio) \
    ENTRY(rc,   super_mode,     U32,        MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,        rc, super_mode) \
    ENTRY(rc,   super_i_thd,    U32,        MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,        rc, super_i_thd) \
    ENTRY(rc,   super_p_thd,    U32,        MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,        rc, super_p_thd) \
    ENTRY(rc,   debreath_en,    U32,        MPP_ENC_RC_CFG_CHANGE_DEBREATH,         rc, debreath_en) \
    ENTRY(rc,   debreath_strength,  U32,    MPP_ENC_RC_CFG_CHANGE_DEBREATH,         rc, debre_strength) \
    ENTRY(rc,   qp_init,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(rc,   qp_min,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(rc,   qp_max,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(rc,   qp_min_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(rc,   qp_max_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(rc,   qp_step,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(rc,   qp_ip,          S32,        MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(rc,   qp_vi,          S32,        MPP_ENC_RC_CFG_CHANGE_QP_VI,            rc, qp_delta_vi) \
    ENTRY(rc,   hier_qp_en,     S32,        MPP_ENC_RC_CFG_CHANGE_HIER_QP,          rc, hier_qp_en) \
    ENTRY(rc,   hier_qp_delta,  St,         MPP_ENC_RC_CFG_CHANGE_HIER_QP,          rc, hier_qp_delta) \
    ENTRY(rc,   hier_frame_num, St,         MPP_ENC_RC_CFG_CHANGE_HIER_QP,          rc, hier_frame_num) \
    ENTRY(rc,   stats_time,     S32,        MPP_ENC_RC_CFG_CHANGE_ST_TIME,          rc, stats_time) \
    ENTRY(rc,   refresh_en,     U32,        MPP_ENC_RC_CFG_CHANGE_REFRESH,          rc, refresh_en) \
    ENTRY(rc,   refresh_mode,   U32,        MPP_ENC_RC_CFG_CHANGE_REFRESH,          rc, refresh_mode) \
    ENTRY(rc,   refresh_num,    U32,        MPP_ENC_RC_CFG_CHANGE_REFRESH,          rc, refresh_num) \
    ENTRY(rc,   fqp_min_i,      S32,        MPP_ENC_RC_CFG_CHANGE_FQP,              rc, fqp_min_i) \
    ENTRY(rc,   fqp_min_p,      S32,        MPP_ENC_RC_CFG_CHANGE_FQP,              rc, fqp_min_p) \
    ENTRY(rc,   fqp_max_i,      S32,        MPP_ENC_RC_CFG_CHANGE_FQP,              rc, fqp_max_i) \
    ENTRY(rc,   fqp_max_p,      S32,        MPP_ENC_RC_CFG_CHANGE_FQP,              rc, fqp_max_p) \
    ENTRY(rc,   mt_st_swth_frm_qp, S32,     MPP_ENC_RC_CFG_CHANGE_FQP,              rc, mt_st_swth_frm_qp) \
    /* prep config */ \
    ENTRY(prep, width,          S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, width) \
    ENTRY(prep, height,         S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, height) \
    ENTRY(prep, max_width,      S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, max_width) \
    ENTRY(prep, max_height,     S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, max_height) \
    ENTRY(prep, hor_stride,     S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, hor_stride) \
    ENTRY(prep, ver_stride,     S32,        MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, ver_stride) \
    ENTRY(prep, format,         S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, format) \
    ENTRY(prep, format_out,     S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, format_out) \
    ENTRY(prep, chroma_ds_mode, S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, chroma_ds_mode) \
    ENTRY(prep, fix_chroma_en,  S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, fix_chroma_en) \
    ENTRY(prep, fix_chroma_u,   S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, fix_chroma_u) \
    ENTRY(prep, fix_chroma_v,   S32,        MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, fix_chroma_v) \
    ENTRY(prep, colorspace,     S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_SPACE,    prep, color) \
    ENTRY(prep, colorprim,      S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_PRIME,    prep, colorprim) \
    ENTRY(prep, colortrc,       S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_TRC,      prep, colortrc) \
    ENTRY(prep, colorrange,     S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE,    prep, range) \
    ENTRY(prep, range,          S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE,    prep, range) \
    ENTRY(prep, range_out,      S32,        MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE,    prep, range_out) \
    ENTRY(prep, rotation,       S32,        MPP_ENC_PREP_CFG_CHANGE_ROTATION,       prep, rotation_ext) \
    ENTRY(prep, mirroring,      S32,        MPP_ENC_PREP_CFG_CHANGE_MIRRORING,      prep, mirroring_ext) \
    ENTRY(prep, flip,           S32,        MPP_ENC_PREP_CFG_CHANGE_FLIP,           prep, flip) \
    /* codec coding config */ \
    ENTRY(codec, type,          S32,        0,                                      codec, coding) \
    /* h264 config */ \
    ENTRY(h264, stream_type,    S32,        MPP_ENC_H264_CFG_STREAM_TYPE,           codec.h264, stream_type) \
    ENTRY(h264, profile,        S32,        MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, profile) \
    ENTRY(h264, level,          S32,        MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, level) \
    ENTRY(h264, poc_type,       U32,        MPP_ENC_H264_CFG_CHANGE_POC_TYPE,       codec.h264, poc_type) \
    ENTRY(h264, log2_max_poc_lsb,   U32,    MPP_ENC_H264_CFG_CHANGE_MAX_POC_LSB,    codec.h264, log2_max_poc_lsb) \
    ENTRY(h264, log2_max_frm_num,   U32,    MPP_ENC_H264_CFG_CHANGE_MAX_FRM_NUM,    codec.h264, log2_max_frame_num) \
    ENTRY(h264, gaps_not_allowed,   U32,    MPP_ENC_H264_CFG_CHANGE_GAPS_IN_FRM_NUM, codec.h264, gaps_not_allowed) \
    ENTRY(h264, cabac_en,       S32,        MPP_ENC_H264_CFG_CHANGE_ENTROPY,        codec.h264, entropy_coding_mode_ex) \
    ENTRY(h264, cabac_idc,      S32,        MPP_ENC_H264_CFG_CHANGE_ENTROPY,        codec.h264, cabac_init_idc_ex) \
    ENTRY(h264, trans8x8,       S32,        MPP_ENC_H264_CFG_CHANGE_TRANS_8x8,      codec.h264, transform8x8_mode_ex) \
    ENTRY(h264, const_intra,    S32,        MPP_ENC_H264_CFG_CHANGE_CONST_INTRA,    codec.h264, constrained_intra_pred_mode) \
    ENTRY(h264, scaling_list,   S32,        MPP_ENC_H264_CFG_CHANGE_SCALING_LIST,   codec.h264, scaling_list_mode) \
    ENTRY(h264, cb_qp_offset,   S32,        MPP_ENC_H264_CFG_CHANGE_CHROMA_QP,      codec.h264, chroma_cb_qp_offset) \
    ENTRY(h264, cr_qp_offset,   S32,        MPP_ENC_H264_CFG_CHANGE_CHROMA_QP,      codec.h264, chroma_cr_qp_offset) \
    ENTRY(h264, dblk_disable,   S32,        MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_disable) \
    ENTRY(h264, dblk_alpha,     S32,        MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_offset_alpha) \
    ENTRY(h264, dblk_beta,      S32,        MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_offset_beta) \
    ENTRY(h264, qp_init,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(h264, qp_min,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(h264, qp_max,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(h264, qp_min_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(h264, qp_max_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(h264, qp_step,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(h264, qp_delta_ip,    S32,        MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(h264, max_tid,        S32,        MPP_ENC_H264_CFG_CHANGE_MAX_TID,        codec.h264, max_tid) \
    ENTRY(h264, max_ltr,        S32,        MPP_ENC_H264_CFG_CHANGE_MAX_LTR,        codec.h264, max_ltr_frames) \
    ENTRY(h264, prefix_mode,    S32,        MPP_ENC_H264_CFG_CHANGE_ADD_PREFIX,     codec.h264, prefix_mode) \
    ENTRY(h264, base_layer_pid, S32,        MPP_ENC_H264_CFG_CHANGE_BASE_LAYER_PID, codec.h264, base_layer_pid) \
    ENTRY(h264, constraint_set, U32,        MPP_ENC_H264_CFG_CHANGE_CONSTRAINT_SET, codec.h264, constraint_set) \
    ENTRY(h264, vui_en,         U32,        MPP_ENC_H264_CFG_CHANGE_VUI,            codec.h264, vui.vui_en) \
    /* h265 config*/ \
    ENTRY(h265, profile,        S32,        MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE,    codec.h265, profile) \
    ENTRY(h265, tier   ,        S32,        MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE,    codec.h265, tier) \
    ENTRY(h265, level,          S32,        MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE,    codec.h265, level) \
    ENTRY(h265, scaling_list,   U32,        MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.defalut_ScalingList_enable) \
    ENTRY(h265, cb_qp_offset,   S32,        MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.cb_qp_offset) \
    ENTRY(h265, cr_qp_offset,   S32,        MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.cr_qp_offset) \
    ENTRY(h265, diff_cu_qp_delta_depth, S32, MPP_ENC_H265_CFG_TRANS_CHANGE,                 codec.h265, trans_cfg.diff_cu_qp_delta_depth) \
    ENTRY(h265, dblk_disable,   U32,        MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_deblocking_filter_disabled_flag) \
    ENTRY(h265, dblk_alpha,     S32,        MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_beta_offset_div2) \
    ENTRY(h265, dblk_beta,      S32,        MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_tc_offset_div2) \
    ENTRY(h265, qp_init,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(h265, qp_min,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(h265, qp_max,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(h265, qp_min_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(h265, qp_max_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(h265, qp_step,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(h265, qp_delta_ip,    S32,        MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(h265, sao_luma_disable,   S32,    MPP_ENC_H265_CFG_SAO_CHANGE,            codec.h265, sao_cfg.slice_sao_luma_disable) \
    ENTRY(h265, sao_chroma_disable, S32,    MPP_ENC_H265_CFG_SAO_CHANGE,            codec.h265, sao_cfg.slice_sao_chroma_disable) \
    ENTRY(h265, sao_bit_ratio,  S32,        MPP_ENC_H265_CFG_SAO_CHANGE,            codec.h265, sao_cfg.sao_bit_ratio) \
    ENTRY(h265, lpf_acs_sli_en, U32,        MPP_ENC_H265_CFG_SLICE_LPFACS_CHANGE,   codec.h265, lpf_acs_sli_en) \
    ENTRY(h265, lpf_acs_tile_disable, U32,  MPP_ENC_H265_CFG_TILE_LPFACS_CHANGE,    codec.h265, lpf_acs_tile_disable) \
    ENTRY(h265, auto_tile,      S32,        MPP_ENC_H265_CFG_TILE_CHANGE,           codec.h265, auto_tile) \
    ENTRY(h265, max_tid,        S32,        MPP_ENC_H265_CFG_CHANGE_MAX_TID,        codec.h265, max_tid) \
    ENTRY(h265, max_ltr,        S32,        MPP_ENC_H265_CFG_CHANGE_MAX_LTR,        codec.h265, max_ltr_frames) \
    ENTRY(h265, base_layer_pid, S32,        MPP_ENC_H265_CFG_CHANGE_BASE_LAYER_PID, codec.h265, base_layer_pid) \
    ENTRY(h265, const_intra,    S32,        MPP_ENC_H265_CFG_CHANGE_CONST_INTRA,    codec.h265, const_intra_pred) \
    ENTRY(h265, lcu_size,       S32,        MPP_ENC_H265_CFG_CHANGE_LCU_SIZE,       codec.h265, max_cu_size) \
    ENTRY(h265, vui_en,         U32,        MPP_ENC_H265_CFG_CHANGE_VUI,            codec.h265, vui.vui_en) \
    /* vp8 config */ \
    ENTRY(vp8,  qp_init,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(vp8,  qp_min,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(vp8,  qp_max,         S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(vp8,  qp_min_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(vp8,  qp_max_i,       S32,        MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(vp8,  qp_step,        S32,        MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(vp8,  qp_delta_ip,    S32,        MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(vp8,  disable_ivf,    S32,        MPP_ENC_VP8_CFG_CHANGE_DIS_IVF,         codec.vp8, disable_ivf) \
    /* jpeg config */ \
    ENTRY(jpeg, quant,          S32,        MPP_ENC_JPEG_CFG_CHANGE_QP,             codec.jpeg, quant) \
    ENTRY(jpeg, qtable_y,       Ptr,        MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_y) \
    ENTRY(jpeg, qtable_u,       Ptr,        MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_u) \
    ENTRY(jpeg, qtable_v,       Ptr,        MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_v) \
    ENTRY(jpeg, q_factor,       S32,        MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, q_factor) \
    ENTRY(jpeg, qf_max,         S32,        MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, qf_max) \
    ENTRY(jpeg, qf_min,         S32,        MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, qf_min) \
    /* split config */ \
    ENTRY(split, mode,          U32,        MPP_ENC_SPLIT_CFG_CHANGE_MODE,          split, split_mode) \
    ENTRY(split, arg,           U32,        MPP_ENC_SPLIT_CFG_CHANGE_ARG,           split, split_arg) \
    ENTRY(split, out,           U32,        MPP_ENC_SPLIT_CFG_CHANGE_OUTPUT,        split, split_out) \
    /* hardware detail config */ \
    ENTRY(hw,   qp_row,         S32,        MPP_ENC_HW_CFG_CHANGE_QP_ROW,           hw, qp_delta_row) \
    ENTRY(hw,   qp_row_i,       S32,        MPP_ENC_HW_CFG_CHANGE_QP_ROW_I,         hw, qp_delta_row_i) \
    ENTRY(hw,   aq_thrd_i,      St,         MPP_ENC_HW_CFG_CHANGE_AQ_THRD_I,        hw, aq_thrd_i) \
    ENTRY(hw,   aq_thrd_p,      St,         MPP_ENC_HW_CFG_CHANGE_AQ_THRD_P,        hw, aq_thrd_p) \
    ENTRY(hw,   aq_step_i,      St,         MPP_ENC_HW_CFG_CHANGE_AQ_STEP_I,        hw, aq_step_i) \
    ENTRY(hw,   aq_step_p,      St,         MPP_ENC_HW_CFG_CHANGE_AQ_STEP_P,        hw, aq_step_p) \
    ENTRY(hw,   mb_rc_disable,  S32,        MPP_ENC_HW_CFG_CHANGE_MB_RC,            hw, mb_rc_disable) \
    ENTRY(hw,   aq_rnge_arr,    St,         MPP_ENC_HW_CFG_CHANGE_AQ_RNGE_ARR,      hw, aq_rnge_arr) \
    ENTRY(hw,   mode_bias,      St,         MPP_ENC_HW_CFG_CHANGE_CU_MODE_BIAS,     hw, mode_bias) \
    ENTRY(hw,   skip_bias_en,   S32,        MPP_ENC_HW_CFG_CHANGE_CU_SKIP_BIAS,     hw, skip_bias_en) \
    ENTRY(hw,   skip_sad,       S32,        MPP_ENC_HW_CFG_CHANGE_CU_SKIP_BIAS,     hw, skip_sad) \
    ENTRY(hw,   skip_bias,      S32,        MPP_ENC_HW_CFG_CHANGE_CU_SKIP_BIAS,     hw, skip_bias) \
    ENTRY(hw,   qbias_i,        S32,        MPP_ENC_HW_CFG_CHANGE_QBIAS_I,          hw, qbias_i) \
    ENTRY(hw,   qbias_p,        S32,        MPP_ENC_HW_CFG_CHANGE_QBIAS_P,          hw, qbias_p) \
    ENTRY(hw,   qbias_en,       S32,        MPP_ENC_HW_CFG_CHANGE_QBIAS_EN,         hw, qbias_en) \
    ENTRY(hw,   qbias_arr,      St,         MPP_ENC_HW_CFG_CHANGE_QBIAS_ARR,        hw, qbias_arr) \
    ENTRY(hw,   flt_str_i,      S32,        MPP_ENC_HW_CFG_CHANGE_FLT_STR_I,        hw, flt_str_i) \
    ENTRY(hw,   flt_str_p,      S32,        MPP_ENC_HW_CFG_CHANGE_FLT_STR_P,        hw, flt_str_p) \
    /* quality fine tuning config */ \
    ENTRY(tune, scene_mode,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SCENE_MODE,     tune, scene_mode) \
    ENTRY(tune, se_mode,        S32,        MPP_ENC_TUNE_CFG_CHANGE_SE_MODE,        tune, se_mode) \
    ENTRY(tune, deblur_en,      S32,        MPP_ENC_TUNE_CFG_CHANGE_DEBLUR_EN,      tune, deblur_en) \
    ENTRY(tune, deblur_str,     S32,        MPP_ENC_TUNE_CFG_CHANGE_DEBLUR_STR,     tune, deblur_str) \
    ENTRY(tune, anti_flicker_str,S32,       MPP_ENC_TUNE_CFG_CHANGE_ANTI_FLICKER_STR,tune, anti_flicker_str) \
    ENTRY(tune, lambda_idx_i,   S32,        MPP_ENC_TUNE_CFG_CHANGE_LAMBDA_IDX_I,   tune, lambda_idx_i) \
    ENTRY(tune, lambda_idx_p,   S32,        MPP_ENC_TUNE_CFG_CHANGE_LAMBDA_IDX_P,   tune, lambda_idx_p) \
    ENTRY(tune, atr_str_i,      S32,        MPP_ENC_TUNE_CFG_CHANGE_ATR_STR_I,      tune, atr_str_i) \
    ENTRY(tune, atr_str_p,      S32,        MPP_ENC_TUNE_CFG_CHANGE_ATR_STR_P,      tune, atr_str_p) \
    ENTRY(tune, atl_str,        S32,        MPP_ENC_TUNE_CFG_CHANGE_ATL_STR,        tune, atl_str) \
    ENTRY(tune, sao_str_i,      S32,        MPP_ENC_TUNE_CFG_CHANGE_SAO_STR_I,      tune, sao_str_i) \
    ENTRY(tune, sao_str_p,      S32,        MPP_ENC_TUNE_CFG_CHANGE_SAO_STR_P,      tune, sao_str_p) \
    ENTRY(tune, rc_container,   S32,        MPP_ENC_TUNE_CFG_CHANGE_RC_CONTAINER,   tune, rc_container) \
    ENTRY(tune, vmaf_opt,       S32,        MPP_ENC_TUNE_CFG_CHANGE_VMAF_OPT,       tune, vmaf_opt) \
    ENTRY(tune, motion_static_switch_enable, S32, MPP_ENC_TUNE_CFG_CHANGE_MOTION_STATIC_SWITCH_ENABLE, tune, motion_static_switch_enable) \
    ENTRY(tune, atf_str,        S32,        MPP_ENC_TUNE_CFG_CHANGE_ATF_STR,        tune, atf_str) \
    ENTRY(tune, lgt_chg_lvl,    S32,        MPP_ENC_TUNE_CFG_CHANGE_LGT_CHG_LVL,    tune, lgt_chg_lvl) \
    ENTRY(tune, static_frm_num, S32,        MPP_ENC_TUNE_CFG_CHANGE_STATIC_FRM_NUM, tune, static_frm_num) \
    ENTRY(tune, madp16_th,      S32,        MPP_ENC_TUNE_CFG_CHANGE_MADP16_TH,      tune, madp16_th) \
    ENTRY(tune, skip16_wgt,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SKIP16_WGT,     tune, skip16_wgt) \
    ENTRY(tune, skip32_wgt,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SKIP32_WGT,     tune, skip32_wgt) \
    ENTRY(tune, speed,          S32,        MPP_ENC_TUNE_CFG_CHANGE_SPEED,          tune, speed) \
    ENTRY(tune, bg_delta_qp_i,  S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bg_delta_qp_i) \
    ENTRY(tune, bg_delta_qp_p,  S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bg_delta_qp_p) \
    ENTRY(tune, fg_delta_qp_i,  S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, fg_delta_qp_i) \
    ENTRY(tune, fg_delta_qp_p,  S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, fg_delta_qp_p) \
    ENTRY(tune, bmap_qpmin_i,   S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bmap_qpmin_i) \
    ENTRY(tune, bmap_qpmin_p,   S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bmap_qpmin_p) \
    ENTRY(tune, bmap_qpmax_i,   S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bmap_qpmax_i) \
    ENTRY(tune, bmap_qpmax_p,   S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, bmap_qpmax_p) \
    ENTRY(tune, min_bg_fqp,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, min_bg_fqp) \
    ENTRY(tune, max_bg_fqp,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, max_bg_fqp) \
    ENTRY(tune, min_fg_fqp,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, min_fg_fqp) \
    ENTRY(tune, max_fg_fqp,     S32,        MPP_ENC_TUNE_CFG_CHANGE_SMART_V3_CFG,   tune, max_fg_fqp)

static void mpp_enc_cfg_srv_init()
{
    MppEncCfgSrv *srv = srv_enc_cfg;

    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, mpp_enc_cfg_debug);

    if (srv)
        return ;

    srv = mpp_calloc(MppEncCfgSrv, 1);
    if (!srv) {
        mpp_err_f("failed to allocate enc cfg set service\n");
        return ;
    }

    srv_enc_cfg = srv;

    mpp_trie_init(&srv->trie, "MppEncCfg");
    if (srv->trie) {
        ENTRY_TABLE(EXPAND_AS_TRIE)

        mpp_trie_add_info(srv->trie, NULL, NULL, 0);
    }

    enc_cfg_dbg_func("info cnt %d node cnt %d size %d\n",
                     mpp_trie_get_info_count(srv->trie),
                     mpp_trie_get_node_count(srv->trie),
                     mpp_trie_get_buf_size(srv->trie));
}

static void mpp_enc_cfg_srv_deinit()
{
    MppEncCfgSrv *srv = srv_enc_cfg;

    if (!srv)
        return ;

    if (srv->trie) {
        mpp_trie_deinit(srv->trie);
        srv->trie = NULL;
    }

    MPP_FREE(srv_enc_cfg);
}

MPP_SINGLETON(MPP_SGLN_ENC_CFG, mpp_enc_cfg, mpp_enc_cfg_srv_init, mpp_enc_cfg_srv_deinit)

static MppTrieInfo *service_get_info(const char *name)
{
    MppEncCfgSrv *srv = get_srv_enc_cfg_f();

    if (!srv)
        return NULL;

    return mpp_trie_get_info(srv->trie, name);
}

static void mpp_enc_cfg_set_default(MppEncCfgSet *cfg)
{
    RK_U32 i;

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
}

MPP_RET mpp_enc_cfg_init(MppEncCfg *cfg)
{
    MppEncCfgImpl *impl = NULL;
    MPP_RET ret = MPP_OK;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, 0);

    *cfg = NULL;

    do {
        impl = mpp_calloc(MppEncCfgImpl, 1);
        if (!impl) {
            mpp_err_f("create MppEncCfgImpl failed\n");
            ret = MPP_ERR_NOMEM;
            break;
        }

        impl->cfg = mpp_calloc(MppEncCfgSet, 1);
        if (!impl->cfg) {
            mpp_err_f("create MppEncCfgSet failed\n");
            ret = MPP_ERR_NOMEM;
            break;
        }

        /* NOTE: compatible to old struct size */
        impl->cfg->size = sizeof(*impl->cfg);
        mpp_enc_cfg_set_default(impl->cfg);

        *cfg = impl;
    } while (0);

    if (ret) {
        if (impl) {
            MPP_FREE(impl->cfg);
            MPP_FREE(impl);
        }
    }

    return ret;
}

MPP_RET mpp_enc_cfg_deinit(MppEncCfg cfg)
{
    return mpp_enc_cfg_put(cfg);
}

RK_S32 mpp_enc_cfg_get(MppEncCfg *cfg, const char *name, void *val)
{
    static const char *kcfg_name = "KmppVencStCfg";
    (void)val;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, 0);

    *cfg = NULL;

    if (!name || !strcmp(name, "MppEncCfg")) {
        return mpp_enc_cfg_init(cfg);
    } else if (!strcmp(name, kcfg_name)) {
        MppEncCfgImpl *impl = NULL;

        impl = mpp_calloc(MppEncCfgImpl, 1);
        if (!impl) {
            mpp_err_f("create MppEncCfgImpl failed\n");
            return MPP_ERR_NOMEM;
        }
        impl->is_kobj = 1;
        kmpp_obj_get_by_name_f(&impl->obj, kcfg_name);
        if (!impl->obj) {
            mpp_err_f("failed to get obj by name %s\n", kcfg_name);
            MPP_FREE(impl);
            return MPP_ERR_NOMEM;
        }
        *cfg = impl;
        return MPP_OK;
    }

    mpp_loge_f("invalid cfg %s\n", name);

    return MPP_NOK;
}

RK_S32 mpp_enc_cfg_put(MppEncCfg cfg)
{
    MppEncCfgImpl *impl = (MppEncCfgImpl *)cfg;

    if (!impl) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!impl->is_kobj) {
        MPP_FREE(impl->cfg);
    } else {
        kmpp_obj_put_f(impl->obj);
    }

    MPP_FREE(impl);

    return MPP_OK;
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
        MppEncCfgImpl *impl = (MppEncCfgImpl *)cfg; \
        if (!impl || !impl->cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        if (impl->is_kobj) { \
            return kmpp_obj_set_##cfg_type(impl->obj, name, val); \
        } else { \
            MppEncCfgSet *p = impl->cfg; \
            MppTrieInfo *node = service_get_info(name); \
            MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
            if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
                return MPP_NOK; \
            } \
            enc_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), strof_cfg_type(info->data_type)); \
            return MPP_CFG_SET_##cfg_type(info, p, val); \
        } \
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
        MppEncCfgImpl *impl = (MppEncCfgImpl *)cfg; \
        if (!impl || !impl->cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        if (impl->is_kobj) { \
            return kmpp_obj_get_##cfg_type(impl->obj, name, val); \
        } else { \
            MppEncCfgSet *p = impl->cfg; \
            MppTrieInfo *node = service_get_info(name); \
            MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
            if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
                return MPP_NOK; \
            } \
            enc_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), strof_cfg_type(info->data_type)); \
            return MPP_CFG_GET_##cfg_type(info, p, val); \
        } \
    }

ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s32, RK_S32, S32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u32, RK_U32, U32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s64, RK_S64, S64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u64, RK_U64, U64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_ptr, void *, Ptr);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_st,  void  , St);

void mpp_enc_cfg_show(void)
{
    MppEncCfgSrv *srv = get_srv_enc_cfg_f();
    MppTrieInfo *root;

    if (!srv)
        return;

    mpp_log("dumping valid configure string start\n");

    root = mpp_trie_get_info_first(srv->trie);
    if (root) {
        MppTrieInfo *node = root;

        do {
            if (mpp_trie_info_is_self(node))
                continue;

            if (node->ctx_len == sizeof(MppCfgInfo)) {
                MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node);

                mpp_log("%-25s type %s - %d:%d\n", mpp_trie_info_name(node),
                        strof_cfg_type(info->data_type), info->data_offset, info->data_size);
            } else {
                mpp_log("%-25s size - %d\n", mpp_trie_info_name(node), node->ctx_len);
            }
        } while ((node = mpp_trie_get_info_next(srv->trie, node)));
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("total cfg count %d with %d node size %d\n",
            mpp_trie_get_info_count(srv->trie), mpp_trie_get_node_count(srv->trie),
            mpp_trie_get_buf_size(srv->trie));
}
