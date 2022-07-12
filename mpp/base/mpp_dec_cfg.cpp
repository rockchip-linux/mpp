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
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"

#include "mpp_cfg.h"
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

typedef struct MppDecCfgInfo_t {
    MppCfgInfoHead      head;
    MppTrieNode         trie_node[];
    /* MppCfgInfoNode is following trie_node */
} MppDecCfgInfo;

static MppCfgInfoNode *mpp_dec_cfg_find(MppDecCfgInfo *info, const char *name)
{
    MppTrieNode *node;

    if (NULL == info || NULL == name)
        return NULL;

    node = mpp_trie_get_node(info->trie_node, name);
    if (NULL == node)
        return NULL;

    return (MppCfgInfoNode *)(((char *)info->trie_node) + node->id);
}

class MppDecCfgService
{
private:
    MppDecCfgService();
    ~MppDecCfgService();
    MppDecCfgService(const MppDecCfgService &);
    MppDecCfgService &operator=(const MppDecCfgService &);

    MppDecCfgInfo *mInfo;
    RK_S32 mCfgSize;

public:
    static MppDecCfgService *get() {
        static Mutex lock;
        static MppDecCfgService instance;

        AutoMutex auto_lock(&lock);
        return &instance;
    }

    MppCfgInfoNode *get_info(const char *name) { return mpp_dec_cfg_find(mInfo, name); };
    MppCfgInfoNode *get_info_root();

    RK_S32 get_node_count() { return mInfo ? mInfo->head.node_count : 0; };
    RK_S32 get_info_count() { return mInfo ? mInfo->head.info_count : 0; };
    RK_S32 get_info_size() { return mInfo ? mInfo->head.info_size : 0; };
    RK_S32 get_cfg_size() { return mCfgSize; };
};

#define EXPAND_AS_API(base, name, cfg_type, in_type, flag, field_change, field_data) \
    MppCfgApi api_##base##_##name = \
    { \
        #base":"#name, \
        CFG_FUNC_TYPE_##cfg_type, \
        (RK_U32)((long)&(((MppDecCfgSet *)0)->field_change.change)), \
        flag, \
        (RK_U32)((long)&(((MppDecCfgSet *)0)->field_change.field_data)), \
        sizeof((((MppDecCfgSet *)0)->field_change.field_data)), \
    };

#define EXPAND_AS_ARRAY(base, name, cfg_type, in_type, flag, field_change, field_data) \
    &api_##base##_##name,

#define ENTRY_TABLE(ENTRY)  \
    /* rc config */ \
    ENTRY(base, type,           U32, MppCtxType,        MPP_DEC_CFG_CHANGE_TYPE,            base, type) \
    ENTRY(base, coding,         U32, MppCodingType,     MPP_DEC_CFG_CHANGE_CODING,          base, coding) \
    ENTRY(base, hw_type,        U32, MppCodingType,     MPP_DEC_CFG_CHANGE_HW_TYPE,         base, hw_type) \
    ENTRY(base, batch_mode,     U32, RK_U32,            MPP_DEC_CFG_CHANGE_BATCH_MODE,      base, batch_mode) \
    ENTRY(base, out_fmt,        U32, MppFrameFormat,    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT,   base, out_fmt) \
    ENTRY(base, fast_out,       U32, RK_U32,            MPP_DEC_CFG_CHANGE_FAST_OUT,        base, fast_out) \
    ENTRY(base, fast_parse,     U32, RK_U32,            MPP_DEC_CFG_CHANGE_FAST_PARSE,      base, fast_parse) \
    ENTRY(base, split_parse,    U32, RK_U32,            MPP_DEC_CFG_CHANGE_SPLIT_PARSE,     base, split_parse) \
    ENTRY(base, internal_pts,   U32, RK_U32,            MPP_DEC_CFG_CHANGE_INTERNAL_PTS,    base, internal_pts) \
    ENTRY(base, sort_pts,       U32, RK_U32,            MPP_DEC_CFG_CHANGE_SORT_PTS,        base, sort_pts) \
    ENTRY(base, disable_error,  U32, RK_U32,            MPP_DEC_CFG_CHANGE_DISABLE_ERROR,   base, disable_error) \
    ENTRY(base, enable_vproc,   U32, RK_U32,            MPP_DEC_CFG_CHANGE_ENABLE_VPROC,    base, enable_vproc) \
    ENTRY(base, enable_fast_play, U32, RK_U32,          MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY, base, enable_fast_play) \
    ENTRY(cb, pkt_rdy_cb,       Ptr, MppExtCbFunc,      MPP_DEC_CB_CFG_CHANGE_PKT_RDY,      cb, pkt_rdy_cb) \
    ENTRY(cb, pkt_rdy_ctx,      Ptr, MppExtCbCtx,       MPP_DEC_CB_CFG_CHANGE_PKT_RDY,      cb, pkt_rdy_ctx) \
    ENTRY(cb, pkt_rdy_cmd,      S32, RK_S32,            MPP_DEC_CB_CFG_CHANGE_PKT_RDY,      cb, pkt_rdy_cmd) \
    ENTRY(cb, frm_rdy_cb,       Ptr, MppExtCbFunc,      MPP_DEC_CB_CFG_CHANGE_FRM_RDY,      cb, frm_rdy_cb) \
    ENTRY(cb, frm_rdy_ctx,      Ptr, MppExtCbCtx,       MPP_DEC_CB_CFG_CHANGE_FRM_RDY,      cb, frm_rdy_ctx) \
    ENTRY(cb, frm_rdy_cmd,      S32, RK_S32,            MPP_DEC_CB_CFG_CHANGE_FRM_RDY,      cb, frm_rdy_cmd)

static MppDecCfgInfo *mpp_dec_cfg_flaten(MppTrie trie, MppCfgApi **cfgs)
{
    MppDecCfgInfo *info = NULL;
    MppTrieNode *node_root = mpp_trie_node_root(trie);
    RK_S32 node_count = mpp_trie_get_node_count(trie);
    RK_S32 info_count = mpp_trie_get_info_count(trie);
    MppTrieNode *node_trie;
    char *buf = NULL;
    RK_S32 pos = 0;
    RK_S32 len = 0;
    RK_S32 i;

    pos += node_count * sizeof(*node_root);

    mpp_dec_cfg_dbg_info("info node offset %d\n", pos);

    /* update info size and string name size */
    for (i = 0; i < info_count; i++) {
        const char *name = cfgs[i]->name;
        const char **info_trie = mpp_trie_get_info(trie, name);

        mpp_assert(*info_trie == name);
        len = strlen(name);
        pos += sizeof(MppCfgInfoNode) + MPP_ALIGN(len + 1, sizeof(RK_U64));
    }

    len = pos + sizeof(*info);
    mpp_dec_cfg_dbg_info("tire + info size %d total %d\n", pos, len);

    info = mpp_malloc_size(MppDecCfgInfo, len);
    if (NULL == info)
        return NULL;

    memcpy(info->trie_node, node_root, sizeof(*node_root) * node_count);

    node_root = info->trie_node;
    pos = node_count * sizeof(*node_root);
    buf = (char *)node_root + pos;

    for (i = 0; i < info_count; i++) {
        MppCfgInfoNode *node_info = (MppCfgInfoNode *)buf;
        MppCfgApi *api = cfgs[i];
        const char *name = api->name;
        RK_S32 node_size;

        node_trie = mpp_trie_get_node(node_root, name);
        node_trie->id = pos;

        node_info->name_len     = MPP_ALIGN(strlen(name) + 1, sizeof(RK_U64));
        node_info->data_type    = api->data_type;
        node_info->flag_offset  = api->flag_offset;
        node_info->flag_value   = api->flag_value;
        node_info->data_offset  = api->data_offset;
        node_info->data_size    = api->data_size;
        node_info->node_next    = 0;

        node_size = node_info->name_len + sizeof(*node_info);
        node_info->node_size    = node_size;

        mpp_cfg_node_fixup_func(node_info);

        strcpy(node_info->name, name);

        mpp_dec_cfg_dbg_info("cfg %s offset %d size %d update %d flag %x\n",
                             node_info->name,
                             node_info->data_offset, node_info->data_size,
                             node_info->flag_offset, node_info->flag_value);

        pos += node_size;
        buf += node_size;
    }

    mpp_dec_cfg_dbg_info("total size %d +H %d\n", pos, pos + sizeof(info->head));

    info->head.info_size  = pos;
    info->head.info_count = info_count;
    info->head.node_count = node_count;
    info->head.cfg_size   = sizeof(MppDecCfgSet);

    return info;
}

MppDecCfgService::MppDecCfgService() :
    mInfo(NULL),
    mCfgSize(0)
{
    ENTRY_TABLE(EXPAND_AS_API);

    MppCfgApi *cfgs[] = {
        ENTRY_TABLE(EXPAND_AS_ARRAY)
    };

    RK_S32 cfg_cnt = MPP_ARRAY_ELEMS(cfgs);
    MppTrie trie;
    MPP_RET ret;
    RK_S32 i;
    /*
     * NOTE: The dec_node_len is not the real node count should be allocated
     * The max node count should be stream lengthg * 2 if each word is different.
     */
    ret = mpp_trie_init(&trie, 284, cfg_cnt);
    if (ret) {
        mpp_err_f("failed to init dec cfg set trie\n");
        return ;
    }
    for (i = 0; i < cfg_cnt; i++)
        mpp_trie_add_info(trie, &cfgs[i]->name);

    mInfo = mpp_dec_cfg_flaten(trie, cfgs);
    mCfgSize = mInfo->head.cfg_size;

    mpp_trie_deinit(trie);
}

MppDecCfgService::~MppDecCfgService()
{
    MPP_FREE(mInfo);
}

MppCfgInfoNode *MppDecCfgService::get_info_root()
{
    if (NULL == mInfo)
        return NULL;

    return (MppCfgInfoNode *)(mInfo->trie_node + mInfo->head.node_count);
}

void mpp_dec_cfg_set_default(MppDecCfgSet *cfg)
{
    cfg->base.type = MPP_CTX_BUTT;
    cfg->base.coding = MPP_VIDEO_CodingUnused;
    cfg->base.hw_type = -1;
    cfg->base.fast_parse = 1;
}

MPP_RET mpp_dec_cfg_init(MppDecCfg *cfg)
{
    MppDecCfgImpl *p = NULL;
    RK_S32 cfg_size;

    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    cfg_size = MppDecCfgService::get()->get_cfg_size();
    p = mpp_calloc_size(MppDecCfgImpl, cfg_size + sizeof(p->size));
    if (NULL == p) {
        mpp_err_f("create decoder config failed %p\n", p);
        *cfg = NULL;
        return MPP_ERR_NOMEM;
    }

    mpp_assert(cfg_size == sizeof(p->cfg));
    p->size = cfg_size;
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

#define ENC_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppDecCfgImpl *p = (MppDecCfgImpl *)cfg; \
        MppCfgInfoNode *info = MppDecCfgService::get()->get_info(name); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_dec_cfg_dbg_set("name %s type %s\n", info->name, cfg_type_names[info->data_type]); \
        MPP_RET ret = MPP_CFG_SET_##cfg_type(info, &p->cfg, val); \
        return ret; \
    }

ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_s32, RK_S32, S32);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_u32, RK_U32, U32);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_s64, RK_S64, S64);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_u64, RK_U64, U64);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_ptr, void *, Ptr);
ENC_CFG_SET_ACCESS(mpp_dec_cfg_set_st,  void *, St);

#define ENC_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppDecCfgImpl *p = (MppDecCfgImpl *)cfg; \
        MppCfgInfoNode *info = MppDecCfgService::get()->get_info(name); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_dec_cfg_dbg_set("name %s type %s\n", info->name, cfg_type_names[info->data_type]); \
        MPP_RET ret = MPP_CFG_GET_##cfg_type(info, &p->cfg, val); \
        return ret; \
    }

ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_s32, RK_S32, S32);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_u32, RK_U32, U32);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_s64, RK_S64, S64);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_u64, RK_U64, U64);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_ptr, void *, Ptr);
ENC_CFG_GET_ACCESS(mpp_dec_cfg_get_st,  void  , St);

void mpp_dec_cfg_show(void)
{
    RK_S32 node_count = MppDecCfgService::get()->get_node_count();
    RK_S32 info_count = MppDecCfgService::get()->get_info_count();
    MppCfgInfoNode *info = MppDecCfgService::get()->get_info_root();

    mpp_log("dumping valid configure string start\n");

    if (info) {
        char *p = (char *)info;
        RK_S32 i;

        for (i = 0; i < info_count; i++) {
            info = (MppCfgInfoNode *)p;

            mpp_log("%-25s type %s\n", info->name,
                    cfg_type_names[info->data_type]);

            p += info->node_size;
        }
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("total cfg count %d with %d node size %d\n",
            info_count, node_count,
            MppDecCfgService::get()->get_info_size());
}
