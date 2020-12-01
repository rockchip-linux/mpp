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

#define MODULE_TAG "mpp_dec_cfg"

#include "rk_vdec_cfg.h"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"

#include "mpp_dec_cfg_impl.h"

#define MPP_DEC_CFG_DBG_FUNC            (0x00000001)
#define MPP_DEC_CFG_DBG_INFO            (0x00000002)
#define MPP_DEC_CFG_DBG_SET             (0x00000004)
#define MPP_DEC_CFG_DBG_GET             (0x00000008)

#define mpp_dec_cfg_dbg(flag, fmt, ...) _mpp_dbg_f(mpp_dec_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_dec_cfg_dbg_func(fmt, ...)  mpp_dec_cfg_dbg(MPP_DEC_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_dec_cfg_dbg_info(fmt, ...)  mpp_dec_cfg_dbg(MPP_DEC_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define mpp_dec_cfg_dbg_set(fmt, ...)   mpp_dec_cfg_dbg(MPP_DEC_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define mpp_dec_cfg_dbg_get(fmt, ...)   mpp_dec_cfg_dbg(MPP_DEC_CFG_DBG_GET, fmt, ## __VA_ARGS__)

RK_U32 mpp_dec_cfg_debug = 0;

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
    static const char *dec_cfg_func_names[] = {
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

static const char *dec_cfg_func_names[] = {
    TYPE_ENTRY_TABLE(EXPAND_AS_NAME)
};

#define EXPAND_AS_FUNC(base, name, func_type, in_type, flag, field0, field1) \
    MPP_RET set_##base##_##name(MppDecCfgSet *cfg, in_type name) \
    { \
        mpp_dec_cfg_dbg_func("enter\n"); \
        if (cfg->field0.field1 != (in_type)name || SET_##func_type == SET_PTR) { \
            cfg->field0.field1 = (in_type)name; \
            cfg->field0.change |= flag; \
        } \
        return MPP_OK; \
    } \
    MPP_RET get_##base##_##name(MppDecCfgSet *cfg, in_type *name) \
    { \
        mpp_dec_cfg_dbg_func("enter\n"); \
        *name = (in_type)cfg->field0.field1; \
        return MPP_OK; \
    }

#define EXPAND_AS_API(base, name, func_type, in_type, flag, field0, field1) \
    static MppDecCfgApi api_##base##_##name = \
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
    dec_const_strlen( #base":"#name ) +

#define ENTRY_TABLE(ENTRY)  \
    /* rc config */ \
    ENTRY(base, out_fmt,        U32, MppFrameFormat,    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT,   base, out_fmt) \
    ENTRY(base, fast_out,       U32, RK_U32,            MPP_DEC_CFG_CHANGE_FAST_OUT,        base, fast_out) \
    ENTRY(base, fast_parse,     U32, RK_U32,            MPP_DEC_CFG_CHANGE_FAST_PARSE,      base, fast_parse) \
    ENTRY(base, split_parse,    U32, RK_U32,            MPP_DEC_CFG_CHANGE_SPLIT_PARSE,     base, split_parse) \
    ENTRY(base, internal_pts,   U32, RK_U32,            MPP_DEC_CFG_CHANGE_INTERNAL_PTS,    base, internal_pts) \
    ENTRY(base, sort_pts,       U32, RK_U32,            MPP_DEC_CFG_CHANGE_SORT_PTS,        base, sort_pts) \
    ENTRY(base, disable_error,  U32, RK_U32,            MPP_DEC_CFG_CHANGE_DISABLE_ERROR,   base, disable_error) \
    ENTRY(base, enable_vproc,   U32, RK_U32,            MPP_DEC_CFG_CHANGE_ENABLE_VPROC,    base, enable_vproc)

ENTRY_TABLE(EXPAND_AS_FUNC)
ENTRY_TABLE(EXPAND_AS_API)

static MppDecCfgApi *dec_cfg_apis[] = {
    ENTRY_TABLE(EXPAND_AS_ARRAY)
};

RK_S32 dec_const_strlen(const char* str)
{
    return *str ? 1 + dec_const_strlen(str + 1) : 0;
}

static RK_S32 dec_node_len = ENTRY_TABLE(EXPAND_AS_STRLEN) + 36;

class MppDecCfgService
{
private:
    MppDecCfgService();
    ~MppDecCfgService();
    MppDecCfgService(const MppDecCfgService &);
    MppDecCfgService &operator=(const MppDecCfgService &);

    MppTrie mCfgApi;

public:
    static MppDecCfgService *get() {
        static Mutex lock;
        static MppDecCfgService instance;

        AutoMutex auto_lock(&lock);
        return &instance;
    }

    MppTrie get_api() { return mCfgApi; };
};

MppDecCfgService::MppDecCfgService() :
    mCfgApi(NULL)
{
    MPP_RET ret;
    RK_S32 i;
    RK_S32 api_cnt = MPP_ARRAY_ELEMS(dec_cfg_apis);

    /*
     * NOTE: The dec_node_len is not the real node count should be allocated
     * The max node count should be stream lengthg * 2 if each word is different.
     */
    ret = mpp_trie_init(&mCfgApi, dec_node_len, api_cnt);
    if (ret) {
        mpp_err_f("failed to init enc cfg set trie\n");
    } else {
        for (i = 0; i < api_cnt; i++)
            mpp_trie_add_info(mCfgApi, &dec_cfg_apis[i]->name);
    }

    if (dec_node_len < mpp_trie_get_node_count(mCfgApi))
        mpp_err_f("create info %d with not enough node %d -> %d info\n",
                  api_cnt, dec_node_len, mpp_trie_get_node_count(mCfgApi));
}

MppDecCfgService::~MppDecCfgService()
{
    if (mCfgApi) {
        mpp_trie_deinit(mCfgApi);
        mCfgApi = NULL;
    }
}

static void mpp_dec_cfg_set_default(MppDecCfgSet *cfg)
{
    (void) cfg;
}

MPP_RET mpp_dec_cfg_init(MppDecCfg *cfg)
{
    MppDecCfgImpl *p = NULL;

    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    p = mpp_calloc(MppDecCfgImpl, 1);
    if (NULL == p) {
        mpp_err_f("create encoder config failed %p\n", p);
        *cfg = NULL;
        return MPP_ERR_NOMEM;
    }

    p->size = sizeof(*p);
    p->api = MppDecCfgService::get()->get_api();
    mpp_dec_cfg_set_default(&p->cfg);

    mpp_env_get_u32("mpp_dec_cfg_debug", &mpp_dec_cfg_debug, 0);

    *cfg = p;

    return MPP_OK;
}

MPP_RET mpp_dec_cfg_deinit(MppDecCfg cfg)
{
    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_FREE(cfg);

    return MPP_OK;
}

#define ENC_CFG_SET_ACCESS(func_name, in_type, func_enum, func_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppDecCfgImpl *p = (MppDecCfgImpl *)cfg; \
        const char **info = mpp_trie_get_info(p->api, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppDecCfgApi *api = (MppDecCfgApi *)info; \
        if (api->type_set != func_enum) { \
            mpp_err_f("%s expect %s input NOT %s\n", api->name, \
                      dec_cfg_func_names[api->type_set], \
                      dec_cfg_func_names[func_enum]); \
        } \
        mpp_dec_cfg_dbg_set("name %s type %s\n", api->name, dec_cfg_func_names[api->type_set]); \
        MPP_RET ret = ((func_type)api->api_set)(&p->cfg, val); \
        return ret; \
    }

ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_s32, RK_S32, SET_S32, CfgSetS32);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_u32, RK_U32, SET_U32, CfgSetU32);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_s64, RK_S64, SET_S64, CfgSetS64);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_u64, RK_U64, SET_U64, CfgSetU64);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_ptr, void *, SET_PTR, CfgSetPtr);

#define ENC_CFG_GET_ACCESS(func_name, in_type, func_enum, func_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppDecCfgImpl *p = (MppDecCfgImpl *)cfg; \
        const char **info = mpp_trie_get_info(p->api, name); \
        if (NULL == info) { \
            mpp_err_f("failed to set %s to %d\n", name, val); \
            return MPP_NOK; \
        } \
        MppDecCfgApi *api = (MppDecCfgApi *)info; \
        if (api->type_get != func_enum) { \
            mpp_err_f("%s expect %s input not %s\n", api->name, \
                      dec_cfg_func_names[api->type_get], \
                      dec_cfg_func_names[func_enum]); \
        } \
        mpp_dec_cfg_dbg_get("name %s type %s\n", api->name, dec_cfg_func_names[api->type_get]); \
        MPP_RET ret = ((func_type)api->api_get)(&p->cfg, val); \
        return ret; \
    }

ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_s32, RK_S32, GET_S32, CfgGetS32);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_u32, RK_U32, GET_U32, CfgGetU32);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_s64, RK_S64, GET_S64, CfgGetS64);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_u64, RK_U64, GET_U64, CfgGetU64);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_ptr, void *, GET_PTR, CfgGetPtr);

void mpp_dec_cfg_show(void)
{
    RK_U32 i;

    mpp_log("dumping valid configure string start\n");

    for (i = 0; i < MPP_ARRAY_ELEMS(dec_cfg_apis); i++)
        mpp_log("%-25s type set:%s get:%s\n", dec_cfg_apis[i]->name,
                dec_cfg_func_names[dec_cfg_apis[i]->type_set],
                dec_cfg_func_names[dec_cfg_apis[i]->type_get]);

    mpp_log("dumping valid configure string done\n");

    mpp_log("total api count %d with node %d -> %d info\n",
            MPP_ARRAY_ELEMS(dec_cfg_apis), dec_node_len,
            mpp_trie_get_node_count(MppDecCfgService::get()->get_api()));
}
