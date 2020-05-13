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
#define MPP_ENC_CFG_DBG_SET             (0x00000002)
#define MPP_ENC_CFG_DBG_GET             (0x00000004)

#define mpp_enc_cfg_dbg(flag, fmt, ...) _mpp_dbg_f(mpp_enc_cfg_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_enc_cfg_dbg_func(fmt, ...)  mpp_enc_cfg_dbg(MPP_ENC_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
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
    static MppEncCfgApi api_set_##base##_##name = \
    { \
        #base":"#name, \
        SET_##func_type, \
        (void *)set_##base##_##name, \
    }; \
    static MppEncCfgApi api_get_##base##_##name = \
    { \
        #base":"#name, \
        GET_##func_type, \
        (void *)get_##base##_##name, \
    };

#define EXPAND_AS_SET_ARRAY(base, name, func_type, in_type, flag, field0, field1) \
    &api_set_##base##_##name,

#define EXPAND_AS_GET_ARRAY(base, name, func_type, in_type, flag, field0, field1) \
    &api_get_##base##_##name,

#define ENTRY_TABLE(ENTRY)  \
    /* rc config */ \
    ENTRY(rc,   rc_mode,        S32, MppEncRcMode,      MPP_ENC_RC_CFG_CHANGE_RC_MODE,          rc, rc_mode) \
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
    /* prep config */ \
    ENTRY(prep, width,          S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, width) \
    ENTRY(prep, height,         S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, height) \
    ENTRY(prep, hor_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, hor_stride) \
    ENTRY(prep, ver_stride,     S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_INPUT,          prep, ver_stride) \
    ENTRY(prep, format,         S32, MppFrameFormat,    MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, format) \
    ENTRY(prep, color,          S32, MppFrameColorSpace,MPP_ENC_PREP_CFG_CHANGE_FORMAT,         prep, color) \
    ENTRY(prep, rotation,       S32, MppEncRotationCfg, MPP_ENC_PREP_CFG_CHANGE_ROTATION,       prep, rotation) \
    ENTRY(prep, mirroring,      S32, RK_S32,            MPP_ENC_PREP_CFG_CHANGE_MIRRORING,      prep, mirroring) \
    /* codec coding config */ \
    ENTRY(codec, type,          S32, MppCodingType,     0,                                      codec, coding) \
    /* h264 config */ \
    ENTRY(h264, stream_type,    S32, RK_S32,            MPP_ENC_H264_CFG_STREAM_TYPE,           codec.h264, stream_type) \
    ENTRY(h264, profile,        S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, profile) \
    ENTRY(h264, level,          S32, RK_S32,            MPP_ENC_H264_CFG_CHANGE_PROFILE,        codec.h264, level) \
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
    /* split config */ \
    ENTRY(split, split_mode,    U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_MODE,          split, split_mode) \
    ENTRY(split, split_arg,     U32, RK_U32,            MPP_ENC_SPLIT_CFG_CHANGE_ARG,           split, split_arg)

ENTRY_TABLE(EXPAND_AS_FUNC)
ENTRY_TABLE(EXPAND_AS_API)

MppEncCfgApi *set_cfg_apis[] = {
    ENTRY_TABLE(EXPAND_AS_SET_ARRAY)
};

MppEncCfgApi *get_cfg_apis[] = {
    ENTRY_TABLE(EXPAND_AS_GET_ARRAY)
};

class MppEncCfgService
{
private:
    MppEncCfgService();
    ~MppEncCfgService();
    MppEncCfgService(const MppEncCfgService &);
    MppEncCfgService &operator=(const MppEncCfgService &);

    MppTrie mSetCfg;
    MppTrie mGetCfg;

public:
    static MppEncCfgService *get() {
        static Mutex lock;
        static MppEncCfgService instance;

        AutoMutex auto_lock(&lock);
        return &instance;
    }

    MppTrie get_api_set() { return mSetCfg; };
    MppTrie get_api_get() { return mGetCfg; };
};

MppEncCfgService::MppEncCfgService() :
    mSetCfg(NULL),
    mGetCfg(NULL)
{
    MPP_RET ret;
    RK_U32 i;

    ret = mpp_trie_init(&mSetCfg, 600);
    if (ret) {
        mpp_err_f("failed to init enc cfg set trie\n");
    } else {
        for (i = 0; i < MPP_ARRAY_ELEMS(set_cfg_apis); i++)
            mpp_trie_add_info(mSetCfg, &set_cfg_apis[i]->name);
    }

    ret = mpp_trie_init(&mGetCfg, 600);
    if (ret) {
        mpp_err_f("failed to init enc cfg get trie\n");
    } else {
        for (i = 0; i < MPP_ARRAY_ELEMS(get_cfg_apis); i++)
            mpp_trie_add_info(mGetCfg, &get_cfg_apis[i]->name);
    }

    mpp_log_f("create config %d node %d info\n",
              mpp_trie_get_node_count(mSetCfg),
              mpp_trie_get_info_count(mSetCfg));
}

MppEncCfgService::~MppEncCfgService()
{
    if (mSetCfg) {
        mpp_trie_deinit(mSetCfg);
        mSetCfg = NULL;
    }
    if (mGetCfg) {
        mpp_trie_deinit(mGetCfg);
        mGetCfg = NULL;
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
    p->set = MppEncCfgService::get()->get_api_set();
    p->get = MppEncCfgService::get()->get_api_get();

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
        const char **info = mpp_trie_get_info(p->set, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppEncCfgApi *api = (MppEncCfgApi *)info; \
        mpp_assert(api->type == func_enum); \
        mpp_enc_cfg_dbg_set("name %s type %s\n", api->name, cfg_func_names[api->type]); \
        MPP_RET ret = ((func_type)api->api)(&p->cfg, val); \
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
        const char **info = mpp_trie_get_info(p->get, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppEncCfgApi *api = (MppEncCfgApi *)info; \
        mpp_assert(api->type == func_enum); \
        mpp_enc_cfg_dbg_get("name %s type %s\n", api->name, cfg_func_names[api->type]); \
        MPP_RET ret = ((func_type)api->api)(&p->cfg, val); \
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

    mpp_log_f("dumping valid configure string start:\n");

    for (i = 0; i < MPP_ARRAY_ELEMS(set_cfg_apis); i++)
        mpp_log_f("name %s type %s\n", set_cfg_apis[i]->name,
                  cfg_func_names[set_cfg_apis[i]->type]);

    mpp_log_f("dumping valid configure string done:\n");
}
