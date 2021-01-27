/*
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

#define MODULE_TAG "mpp_enc_cfg"

#include <string.h>

#include "rk_venc_cfg.h"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"

#include "mpp_cfg.h"
#include "mpp_enc_cfg_impl.h"

#define MPP_ENC_CFG_DBG_FUNC            (0x00000001)
#define MPP_ENC_CFG_DBG_INFO            (0x00000002)
#define MPP_ENC_CFG_DBG_SET             (0x00000004)
#define MPP_ENC_CFG_DBG_GET             (0x00000008)

#define mpp_enc_cfg_dbg(flag, fmt, ...) _mpp_dbg_f(mpp_enc_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_enc_cfg_dbg_func(fmt, ...)  mpp_enc_cfg_dbg(MPP_ENC_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_enc_cfg_dbg_info(fmt, ...)  mpp_enc_cfg_dbg(MPP_ENC_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define mpp_enc_cfg_dbg_set(fmt, ...)   mpp_enc_cfg_dbg(MPP_ENC_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define mpp_enc_cfg_dbg_get(fmt, ...)   mpp_enc_cfg_dbg(MPP_ENC_CFG_DBG_GET, fmt, ## __VA_ARGS__)

RK_U32 mpp_enc_cfg_debug = 0;

#define EXPAND_AS_FUNC(base, name, cfg_type, in_type, flag, field_change, field_data) \
    MPP_RET set_##base##_##name(void *cfg, MppCfgApi *api, in_type name) \
    { \
        char *base = (char *)cfg; \
        CfgDataInfo *info = &api->info; \
        CfgDataInfo *update = &api->update; \
        \
        mpp_enc_cfg_dbg_func("enter\n"); \
        switch (CFG_FUNC_TYPE_##cfg_type) { \
        case CFG_FUNC_TYPE_S32 : \
        case CFG_FUNC_TYPE_U32 : \
        case CFG_FUNC_TYPE_S64 : \
        case CFG_FUNC_TYPE_U64 : { \
            if (memcmp(base + info->offset, &name, info->size)) { \
                memcpy(base + info->offset, &name, info->size); \
                *((RK_U32 *)(base + update->offset)) |= flag; \
            } \
        } break; \
        case CFG_FUNC_TYPE_Ptr : { \
            memcpy(base + info->offset, &name, info->size); \
            *((RK_U32 *)(base + update->offset)) |= flag; \
        } break; \
        case CFG_FUNC_TYPE_St  : { \
            if (memcmp(base + info->offset, (void *)((long)name), info->size)) { \
                memcpy(base + info->offset, (void *)((long)name), info->size); \
                *((RK_U32 *)(base + update->offset)) |= flag; \
            } \
        } break; \
        default : { \
        } break; \
        } \
        return MPP_OK; \
    } \
    MPP_RET get_##base##_##name(void *cfg, MppCfgApi *api, in_type *name) \
    { \
        char *base = (char *)cfg; \
        CfgDataInfo *info = &api->info; \
        \
        mpp_enc_cfg_dbg_func("enter\n"); \
        memcpy(name, base + info->offset, info->size); \
        return MPP_OK; \
    }

#define EXPAND_AS_API(base, name, cfg_type, in_type, flag, field_change, field_data) \
    static MppCfgApi api_##base##_##name = \
    { \
        #base":"#name, \
        { \
            CFG_FUNC_TYPE_##cfg_type, \
            sizeof((((MppEncCfgSet *)0)->field_change.field_data)), \
            (RK_U32)((long)&(((MppEncCfgSet *)0)->field_change.field_data)), \
        }, \
        { \
            CFG_FUNC_TYPE_U32, \
            sizeof((((MppEncCfgSet *)0)->field_change.change)), \
            (RK_U32)((long)&(((MppEncCfgSet *)0)->field_change.change)), \
        }, \
        (void *)set_##base##_##name, \
        (void *)get_##base##_##name, \
    };

#define EXPAND_AS_ARRAY(base, name, cfg_type, in_type, flag, field_change, field_data) \
    &api_##base##_##name,

#define EXPAND_AS_STRLEN(base, name, cfg_type, in_type, flag, field_change, field_data) \
    const_strlen( #base":"#name ) +

#define ENTRY_TABLE(ENTRY)  \
    /* base config */ \
    ENTRY(base, low_delay,      S32, RK_S32,            MPP_ENC_BASE_CFG_CHANGE_LOW_DELAY,      base, low_delay) \
    /* rc config */ \
    ENTRY(rc,   mode,           S32, MppEncRcMode,      MPP_ENC_RC_CFG_CHANGE_RC_MODE,          rc, rc_mode) \
    ENTRY(rc,   bps_target,     S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_target) \
    ENTRY(rc,   bps_max,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_max) \
    ENTRY(rc,   bps_min,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_BPS,              rc, bps_min) \
    ENTRY(rc,   fps_in_flex,    S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_flex) \
    ENTRY(rc,   fps_in_num,     S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_num) \
    ENTRY(rc,   fps_in_denorm,  S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_IN,           rc, fps_in_denorm) \
    ENTRY(rc,   fps_out_flex,   S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_flex) \
    ENTRY(rc,   fps_out_num,    S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_num) \
    ENTRY(rc,   fps_out_denorm, S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_FPS_OUT,          rc, fps_out_denorm) \
    ENTRY(rc,   gop,            S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_GOP,              rc, gop) \
    ENTRY(rc,   max_reenc_times,U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_MAX_REENC,        rc, max_reenc_times) \
    ENTRY(rc,   priority,       U32, MppEncRcPriority,  MPP_ENC_RC_CFG_CHANGE_PRIORITY,         rc, rc_priority) \
    ENTRY(rc,   drop_mode,      U32, MppEncRcDropFrmMode, MPP_ENC_RC_CFG_CHANGE_DROP_FRM,       rc, drop_mode) \
    ENTRY(rc,   drop_thd,       U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_DROP_FRM,         rc, drop_threshold) \
    ENTRY(rc,   drop_gap,       U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_DROP_FRM,         rc, drop_gap) \
    ENTRY(rc,   max_i_prop,     S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_MAX_I_PROP,       rc, max_i_prop) \
    ENTRY(rc,   min_i_prop,     S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_MIN_I_PROP,       rc, min_i_prop) \
    ENTRY(rc,   init_ip_ratio,  S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_INIT_IP_RATIO,    rc, init_ip_ratio) \
    ENTRY(rc,   super_mode,     U32, MppEncRcSuperFrameMode, MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,   rc, super_mode) \
    ENTRY(rc,   super_i_thd,    U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,        rc, super_i_thd) \
    ENTRY(rc,   super_p_thd,    U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_SUPER_FRM,        rc, super_p_thd) \
    ENTRY(rc,   debreath_en,    U32, RK_U32,            MPP_ENC_RC_CFG_CHANGE_DEBREATH,         rc, debreath_en) \
    ENTRY(rc,   debreath_strength,  U32, RK_U32,        MPP_ENC_RC_CFG_CHANGE_DEBREATH,         rc, debre_strength) \
    ENTRY(rc,   qp_init,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(rc,   qp_min,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(rc,   qp_max,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(rc,   qp_min_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(rc,   qp_max_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(rc,   qp_step,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(rc,   qp_ip,          S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(rc,   qp_vi,          S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_VI,            rc, qp_delta_vi) \
    /* prep config */ \
    ENTRY(prep, width,          S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, width) \
    ENTRY(prep, height,         S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, height) \
    ENTRY(prep, hor_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, hor_stride) \
    ENTRY(prep, ver_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, ver_stride) \
    ENTRY(prep, format,         S32, MppFrameFormat,    MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, format) \
    ENTRY(prep, colorspace,     S32, MppFrameColorSpace,MPP_ENC_PREP_CFG_CHANGE_COLOR_SPACE,    prep, color) \
    ENTRY(prep, colorprim,      S32, MppFrameColorPrimaries, MPP_ENC_PREP_CFG_CHANGE_COLOR_PRIME, prep, colorprim) \
    ENTRY(prep, colortrc,       S32, MppFrameColorTransferCharacteristic, MPP_ENC_PREP_CFG_CHANGE_COLOR_TRC, prep, colortrc) \
    ENTRY(prep, colorrange,     S32, MppFrameColorRange,MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE,    prep, range) \
    ENTRY(prep, range,          S32, MppFrameColorRange,MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE,    prep, range) \
    ENTRY(prep, rotation,       S32, MppEncRotationCfg, MPP_ENC_PREP_CFG_CHANGE_ROTATION,       prep, rotation) \
    ENTRY(prep, mirroring,      S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_MIRRORING,      prep, mirroring) \
    /* codec coding config */ \
    ENTRY(codec, type,          S32, MppCodingType,     0,                                      codec, coding) \
    /* h264 config */ \
    ENTRY(h264, stream_type,    S32, RK_S32,            MPP_ENC_H264_CFG_STREAM_TYPE,           codec.h264, stream_type) \
    ENTRY(h264, profile,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, profile) \
    ENTRY(h264, level,          S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, level) \
    ENTRY(h264, poc_type,       U32, RK_U32,            MPP_ENC_H264_CFG_CHANGE_POC_TYPE,       codec.h264, poc_type) \
    ENTRY(h264, log2_max_poc_lsb,   U32, RK_U32,        MPP_ENC_H264_CFG_CHANGE_MAX_POC_LSB,    codec.h264, log2_max_poc_lsb) \
    ENTRY(h264, log2_max_frm_num,   U32, RK_U32,        MPP_ENC_H264_CFG_CHANGE_MAX_FRM_NUM,    codec.h264, log2_max_frame_num) \
    ENTRY(h264, gaps_not_allowed,  U32, RK_U32,         MPP_ENC_H264_CFG_CHANGE_GAPS_IN_FRM_NUM, codec.h264, gaps_not_allowed) \
    ENTRY(h264, cabac_en,       S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_ENTROPY,        codec.h264, entropy_coding_mode) \
    ENTRY(h264, cabac_idc,      S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_ENTROPY,        codec.h264, cabac_init_idc) \
    ENTRY(h264, trans8x8,       S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_TRANS_8x8,      codec.h264, transform8x8_mode) \
    ENTRY(h264, const_intra,    S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_CONST_INTRA,    codec.h264, constrained_intra_pred_mode) \
    ENTRY(h264, scaling_list,   S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_SCALING_LIST,   codec.h264, scaling_list_mode) \
    ENTRY(h264, cb_qp_offset,   S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_CHROMA_QP,      codec.h264, chroma_cb_qp_offset) \
    ENTRY(h264, cr_qp_offset,   S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_CHROMA_QP,      codec.h264, chroma_cb_qp_offset) \
    ENTRY(h264, dblk_disable,   S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_disable) \
    ENTRY(h264, dblk_alpha,     S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_offset_alpha) \
    ENTRY(h264, dblk_beta,      S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_DEBLOCKING,     codec.h264, deblock_offset_beta) \
    ENTRY(h264, qp_init,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(h264, qp_min,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(h264, qp_max,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(h264, qp_min_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(h264, qp_max_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(h264, qp_step,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(h264, qp_delta_ip,    S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(h264, max_tid,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_MAX_TID,        codec.h264, max_tid) \
    ENTRY(h264, max_ltr,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_MAX_LTR,        codec.h264, max_ltr_frames) \
    ENTRY(h264, prefix_mode,    S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_ADD_PREFIX,     codec.h264, prefix_mode) \
    ENTRY(h264, base_layer_pid, S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_BASE_LAYER_PID, codec.h264, base_layer_pid) \
    /* h265 config*/ \
    ENTRY(h265, profile,        S32, RK_S32,            MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE,    codec.h265, profile) \
    ENTRY(h265, level,          S32, RK_S32,            MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE,    codec.h265, level) \
    ENTRY(h265, scaling_list,   U32, RK_U32,            MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.defalut_ScalingList_enable) \
    ENTRY(h265, cb_qp_offset,   S32, RK_S32,            MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.cb_qp_offset) \
    ENTRY(h265, cr_qp_offset,   S32, RK_S32,            MPP_ENC_H265_CFG_TRANS_CHANGE,                  codec.h265, trans_cfg.cr_qp_offset) \
    ENTRY(h265, dblk_disable,   U32, RK_U32,            MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_deblocking_filter_disabled_flag) \
    ENTRY(h265, dblk_alpha,     S32, RK_S32,            MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_beta_offset_div2) \
    ENTRY(h265, dblk_beta,      S32, RK_S32,            MPP_ENC_H265_CFG_DBLK_CHANGE,                   codec.h265, dblk_cfg.slice_tc_offset_div2) \
    ENTRY(h265, qp_init,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(h265, qp_min,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(h265, qp_max,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(h265, qp_min_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(h265, qp_max_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(h265, qp_step,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(h265, qp_delta_ip,    S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    /* vp8 config */ \
    ENTRY(vp8,  qp_init,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_INIT,          rc, qp_init) \
    ENTRY(vp8,  qp_min,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_min) \
    ENTRY(vp8,  qp_max,         S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE,         rc, qp_max) \
    ENTRY(vp8,  qp_min_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_min_i) \
    ENTRY(vp8,  qp_max_i,       S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_RANGE_I,       rc, qp_max_i) \
    ENTRY(vp8,  qp_step,        S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_MAX_STEP,      rc, qp_max_step) \
    ENTRY(vp8,  qp_delta_ip,    S32, RK_S32,            MPP_ENC_RC_CFG_CHANGE_QP_IP,            rc, qp_delta_ip) \
    ENTRY(vp8,  disable_ivf,    S32, RK_S32,            MPP_ENC_VP8_CFG_CHANGE_DIS_IVF,         codec.vp8, disable_ivf) \
    /* jpeg config */ \
    ENTRY(jpeg, quant,          S32, RK_S32,            MPP_ENC_JPEG_CFG_CHANGE_QP,             codec.jpeg, quant) \
    ENTRY(jpeg, qtable_y,       Ptr, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_y) \
    ENTRY(jpeg, qtable_u,       Ptr, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_u) \
    ENTRY(jpeg, qtable_v,       Ptr, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_v) \
    ENTRY(jpeg, q_factor,       S32, RK_S32,            MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, q_factor) \
    ENTRY(jpeg, qf_max,         S32, RK_S32,            MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, qf_max) \
    ENTRY(jpeg, qf_min,         S32, RK_S32,            MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, qf_min) \
    /* split config */ \
    ENTRY(split, mode,          U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_MODE,          split, split_mode) \
    ENTRY(split, arg,           U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_ARG,           split, split_arg) \
    /* hardware detail config */ \
    ENTRY(hw,   qp_row,         S32, RK_S32,            MPP_ENC_HW_CFG_CHANGE_QP_ROW,           hw, qp_delta_row) \
    ENTRY(hw,   qp_row_i,       S32, RK_S32,            MPP_ENC_HW_CFG_CHANGE_QP_ROW_I,         hw, qp_delta_row_i) \
    ENTRY(hw,   aq_thrd_i,      St,  RK_S32 *,          MPP_ENC_HW_CFG_CHANGE_AQ_THRD_I,        hw, aq_thrd_i) \
    ENTRY(hw,   aq_thrd_p,      St,  RK_S32 *,          MPP_ENC_HW_CFG_CHANGE_AQ_THRD_P,        hw, aq_thrd_p) \
    ENTRY(hw,   aq_step_i,      St,  RK_S32 *,          MPP_ENC_HW_CFG_CHANGE_AQ_STEP_I,        hw, aq_step_i) \
    ENTRY(hw,   aq_step_p,      St,  RK_S32 *,          MPP_ENC_HW_CFG_CHANGE_AQ_STEP_P,        hw, aq_step_p)

ENTRY_TABLE(EXPAND_AS_FUNC)
ENTRY_TABLE(EXPAND_AS_API)

static MppCfgApi *cfg_apis[] = {
    ENTRY_TABLE(EXPAND_AS_ARRAY)
};

RK_S32 const_strlen(const char* str)
{
    return *str ? 1 + const_strlen(str + 1) : 0;
}

static RK_S32 node_len = ENTRY_TABLE(EXPAND_AS_STRLEN) - 77;

class MppEncCfgService
{
private:
    MppEncCfgService();
    ~MppEncCfgService();
    MppEncCfgService(const MppEncCfgService &);
    MppEncCfgService &operator=(const MppEncCfgService &);

    MppTrie mCfgApi;

public:
    static MppEncCfgService *get() {
        static Mutex lock;
        static MppEncCfgService instance;

        AutoMutex auto_lock(&lock);
        return &instance;
    }

    MppTrie get_api() { return mCfgApi; };
};

MppEncCfgService::MppEncCfgService() :
    mCfgApi(NULL)
{
    MPP_RET ret;
    RK_S32 i;
    RK_S32 api_cnt = MPP_ARRAY_ELEMS(cfg_apis);

    /*
     * NOTE: The node_len is not the real node count should be allocated
     * The max node count should be stream lengthg * 2 if each word is different.
     */
    ret = mpp_trie_init(&mCfgApi, node_len, api_cnt);
    if (ret) {
        mpp_err_f("failed to init enc cfg set trie\n");
    } else {
        for (i = 0; i < api_cnt; i++)
            mpp_trie_add_info(mCfgApi, &cfg_apis[i]->name);
    }

    if (node_len < mpp_trie_get_node_count(mCfgApi))
        mpp_err_f("create info %d with not enough node %d -> %d info\n",
                  api_cnt, node_len, mpp_trie_get_node_count(mCfgApi));
}

MppEncCfgService::~MppEncCfgService()
{
    if (mCfgApi) {
        mpp_trie_deinit(mCfgApi);
        mCfgApi = NULL;
    }
}

static void mpp_enc_cfg_set_default(MppEncCfgSet *cfg)
{
    cfg->prep.color = MPP_FRAME_SPC_UNSPECIFIED;
    cfg->prep.colorprim = MPP_FRAME_PRI_UNSPECIFIED;
    cfg->prep.colortrc = MPP_FRAME_TRC_UNSPECIFIED;
}

MPP_RET mpp_enc_cfg_init(MppEncCfg *cfg)
{
    MppEncCfgImpl *p = NULL;

    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MppEncCfgImpl, 1);
    if (NULL == p) {
        mpp_err_f("create encoder config failed %p\n", p);
        *cfg = NULL;
        return MPP_ERR_NOMEM;
    }

    p->size = sizeof(*p);
    p->api = MppEncCfgService::get()->get_api();
    mpp_enc_cfg_set_default(&p->cfg);

    mpp_env_get_u32("mpp_enc_cfg_debug", &mpp_enc_cfg_debug, 0);

    *cfg = p;

    return MPP_OK;
}

MPP_RET mpp_enc_cfg_deinit(MppEncCfg cfg)
{
    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_FREE(cfg);

    return MPP_OK;
}

#define ENC_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppEncCfg cfg, const char *name, in_type val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppEncCfgImpl *p = (MppEncCfgImpl *)cfg; \
        const char **info = mpp_trie_get_info(p->api, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppCfgApi *api = (MppCfgApi *)info; \
        if (check_cfg_api_info(api, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_enc_cfg_dbg_set("name %s type %s\n", api->name, cfg_type_names[api->info.type]); \
        MPP_RET ret = ((CfgSet##cfg_type)api->api_set)(&p->cfg, api, val); \
        return ret; \
    }

ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s32, RK_S32, S32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u32, RK_U32, U32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s64, RK_S64, S64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u64, RK_U64, U64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_ptr, void *, Ptr);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_st,  void *, St);

#define ENC_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppEncCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppEncCfgImpl *p = (MppEncCfgImpl *)cfg; \
        const char **info = mpp_trie_get_info(p->api, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppCfgApi *api = (MppCfgApi *)info; \
        if (check_cfg_api_info(api, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_enc_cfg_dbg_get("name %s type %s\n", api->name, cfg_type_names[api->info.type]); \
        MPP_RET ret = ((CfgGet##cfg_type)api->api_get)(&p->cfg, api, val); \
        return ret; \
    }

ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s32, RK_S32, S32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u32, RK_U32, U32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s64, RK_S64, S64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u64, RK_U64, U64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_ptr, void *, Ptr);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_st,  void  , St);

void mpp_enc_cfg_show(void)
{
    RK_U32 i;

    mpp_log("dumping valid configure string start\n");

    for (i = 0; i < MPP_ARRAY_ELEMS(cfg_apis); i++)
        mpp_log("%-25s type %s\n", cfg_apis[i]->name,
                cfg_type_names[cfg_apis[i]->info.type]);

    mpp_log("dumping valid configure string done\n");

    mpp_log("total api count %d with node %d -> %d info\n",
            MPP_ARRAY_ELEMS(cfg_apis), node_len,
            mpp_trie_get_node_count(MppEncCfgService::get()->get_api()));
}
