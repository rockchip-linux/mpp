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

#include "rk_venc_cfg.h"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"

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

#define EXPAND_AS_ENUM(name, func, type) \
    SET_##name, \
    GET_##name,

/*  Expand to enum
    typedef enum CfgType_e {
        SET_S32,
        GET_S32,
        SET_U32,
        GET_U32,
        SET_S64,
        GET_S64,
        SET_U64,
        GET_U64,
        SET_PTR,
        GET_PTR,
        CFG_FUNC_TYPE_BUTT,
    } CfgType;
 */

#define EXPAND_AS_TYPE(name, func, type) \
    typedef MPP_RET (*CfgSet##func)(void *ctx, type val); \
    typedef MPP_RET (*CfgGet##func)(void *ctx, type *val);

/*  Expand to function type
    typedef MPP_RET (*CfgSetS32)(void *ctx, RK_S32 val);
    typedef MPP_RET (*CfgGetS32)(void *ctx, RK_S32 *val);
    typedef MPP_RET (*CfgSetU32)(void *ctx, RK_U32 val);
    typedef MPP_RET (*CfgGetU32)(void *ctx, RK_U32 *val);
    typedef MPP_RET (*CfgSetS64)(void *ctx, RK_S64 val);
    typedef MPP_RET (*CfgGetS64)(void *ctx, RK_S64 *val);
    typedef MPP_RET (*CfgSetU64)(void *ctx, RK_U64 val);
    typedef MPP_RET (*CfgGetU64)(void *ctx, RK_U64 *val);
    typedef MPP_RET (*CfgSetPtr)(void *ctx, void *val);
    typedef MPP_RET (*CfgGetPtr)(void *ctx, void **val);
 */

#define EXPAND_AS_NAME(name, func, type) \
    #type, \
    #type"*",

/*  Expand to name
    static const char *cfg_func_names[] = {
        "RK_S32",
        "RK_S32*",
        "RK_U32",
        "RK_U32*",
        "RK_S64",
        "RK_S64*",
        "RK_U64",
        "RK_U64*",
        "void *",
        "void **",
    };
 */

#define TYPE_ENTRY_TABLE(ENTRY)  \
    ENTRY(S32, S32, RK_S32) \
    ENTRY(U32, U32, RK_U32) \
    ENTRY(S64, S64, RK_S64) \
    ENTRY(U64, U64, RK_U64) \
    ENTRY(PTR, Ptr, void *)

typedef enum CfgType_e {
    TYPE_ENTRY_TABLE(EXPAND_AS_ENUM)
    CFG_FUNC_TYPE_BUTT,
} CfgType;

TYPE_ENTRY_TABLE(EXPAND_AS_TYPE)

static const char *cfg_func_names[] = {
    TYPE_ENTRY_TABLE(EXPAND_AS_NAME)
};

#define EXPAND_AS_FUNC(base, name, func_type, in_type, flag, field0, field1) \
    MPP_RET set_##base##_##name(MppEncCfgSet *cfg, in_type name) \
    { \
        mpp_enc_cfg_dbg_func("enter\n"); \
        if (cfg->field0.field1 != (in_type)name || SET_##func_type == SET_PTR) { \
            cfg->field0.field1 = (in_type)name; \
            cfg->field0.change |= flag; \
        } \
        return MPP_OK; \
    } \
    MPP_RET get_##base##_##name(MppEncCfgSet *cfg, in_type *name) \
    { \
        mpp_enc_cfg_dbg_func("enter\n"); \
        *name = (in_type)cfg->field0.field1; \
        return MPP_OK; \
    }

#define EXPAND_AS_API(base, name, func_type, in_type, flag, field0, field1) \
    static MppEncCfgApi api_##base##_##name = \
    { \
        #base":"#name, \
        SET_##func_type, \
        GET_##func_type, \
        (void *)set_##base##_##name, \
        (void *)get_##base##_##name, \
    };

#define EXPAND_AS_ARRAY(base, name, func_type, in_type, flag, field0, field1) \
    &api_##base##_##name,

#define EXPAND_AS_STRLEN(base, name, func_type, in_type, flag, field0, field1) \
    const_strlen( #base":"#name ) +

#define ENTRY_TABLE(ENTRY)  \
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
    /* prep config */ \
    ENTRY(prep, width,          S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, width) \
    ENTRY(prep, height,         S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, height) \
    ENTRY(prep, hor_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, hor_stride) \
    ENTRY(prep, ver_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, ver_stride) \
    ENTRY(prep, format,         S32, MppFrameFormat,    MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, format) \
    ENTRY(prep, color,          S32, MppFrameColorSpace,MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, color) \
    ENTRY(prep, range,          S32, MppFrameColorRange,MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, range) \
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
    ENTRY(h264, qp_init,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_init) \
    ENTRY(h264, qp_max,         S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_max) \
    ENTRY(h264, qp_min,         S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_min) \
    ENTRY(h264, qp_max_i,       S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_max) \
    ENTRY(h264, qp_min_i,       S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_min) \
    ENTRY(h264, qp_step,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_QP_LIMIT,       codec.h264, qp_max_step) \
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
    ENTRY(h265, qp_init,        S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, qp_init) \
    ENTRY(h265, qp_max,         S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, max_qp) \
    ENTRY(h265, qp_min,         S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, min_qp) \
    ENTRY(h265, qp_max_i,       S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, max_i_qp) \
    ENTRY(h265, qp_min_i,       S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, min_i_qp) \
    ENTRY(h265, qp_step,        S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, qp_max_step) \
    ENTRY(h265, qp_delta_ip,    S32, RK_S32,            MPP_ENC_H265_CFG_RC_QP_CHANGE,                  codec.h265, ip_qp_delta) \
    /* jpeg config */ \
    ENTRY(jpeg, quant,          S32, RK_S32,            MPP_ENC_JPEG_CFG_CHANGE_QP,             codec.jpeg, quant) \
    ENTRY(jpeg, qtable_y,       PTR, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_y) \
    ENTRY(jpeg, qtable_u,       PTR, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_u) \
    ENTRY(jpeg, qtable_v,       PTR, RK_U8*,            MPP_ENC_JPEG_CFG_CHANGE_QTABLE,         codec.jpeg, qtable_v) \
    ENTRY(jpeg, q_factor,       U32, RK_U32,            MPP_ENC_JPEG_CFG_CHANGE_QFACTOR,        codec.jpeg, q_factor) \
    /* split config */ \
    ENTRY(split, mode,          U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_MODE,          split, split_mode) \
    ENTRY(split, arg,           U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_ARG,           split, split_arg)

ENTRY_TABLE(EXPAND_AS_FUNC)
ENTRY_TABLE(EXPAND_AS_API)

static MppEncCfgApi *cfg_apis[] = {
    ENTRY_TABLE(EXPAND_AS_ARRAY)
};

RK_S32 const_strlen(const char* str)
{
    return *str ? 1 + const_strlen(str + 1) : 0;
}

static RK_S32 node_len = ENTRY_TABLE(EXPAND_AS_STRLEN) - 21;

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

#define ENC_CFG_SET_ACCESS(func_name, in_type, func_enum, func_type) \
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
        MppEncCfgApi *api = (MppEncCfgApi *)info; \
        if (api->type_set != func_enum) { \
            mpp_err_f("%s expect %s input NOT %s\n", api->name, \
                      cfg_func_names[api->type_set], \
                      cfg_func_names[func_enum]); \
        } \
        mpp_enc_cfg_dbg_set("name %s type %s\n", api->name, cfg_func_names[api->type_set]); \
        MPP_RET ret = ((func_type)api->api_set)(&p->cfg, val); \
        return ret; \
    }

ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s32, RK_S32, SET_S32, CfgSetS32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u32, RK_U32, SET_U32, CfgSetU32);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_s64, RK_S64, SET_S64, CfgSetS64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_u64, RK_U64, SET_U64, CfgSetU64);
ENC_CFG_SET_ACCESS(mpp_enc_cfg_set_ptr, void *, SET_PTR, CfgSetPtr);

#define ENC_CFG_GET_ACCESS(func_name, in_type, func_enum, func_type) \
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
        MppEncCfgApi *api = (MppEncCfgApi *)info; \
        if (api->type_get != func_enum) { \
            mpp_err_f("%s expect %s input not %s\n", api->name, \
                      cfg_func_names[api->type_get], \
                      cfg_func_names[func_enum]); \
        } \
        mpp_enc_cfg_dbg_get("name %s type %s\n", api->name, cfg_func_names[api->type_get]); \
        MPP_RET ret = ((func_type)api->api_get)(&p->cfg, val); \
        return ret; \
    }

ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s32, RK_S32, GET_S32, CfgGetS32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u32, RK_U32, GET_U32, CfgGetU32);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_s64, RK_S64, GET_S64, CfgGetS64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_u64, RK_U64, GET_U64, CfgGetU64);
ENC_CFG_GET_ACCESS(mpp_enc_cfg_get_ptr, void *, GET_PTR, CfgGetPtr);

void mpp_enc_cfg_show(void)
{
    RK_U32 i;

    mpp_log_f("dumping valid configure string start\n");

    for (i = 0; i < MPP_ARRAY_ELEMS(cfg_apis); i++)
        mpp_log_f("name %s type %s %s\n", cfg_apis[i]->name,
                  cfg_func_names[cfg_apis[i]->type_set],
                  cfg_func_names[cfg_apis[i]->type_get]);

    mpp_log_f("dumping valid configure string done\n");

    mpp_log_f("total api count %d with node %d -> %d info\n",
              MPP_ARRAY_ELEMS(cfg_apis), node_len,
              mpp_trie_get_node_count(MppEncCfgService::get()->get_api()));
}
