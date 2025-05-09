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
#include "mpp_trie.h"
#include "mpp_dec_cfg.h"

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

class MppDecCfgService
{
private:
    MppDecCfgService();
    ~MppDecCfgService();
    MppDecCfgService(const MppDecCfgService &);
    MppDecCfgService &operator=(const MppDecCfgService &);

    MppCfgInfoHead mHead;
    MppTrie mTrie;
    RK_S32 mCfgSize;

public:
    static MppDecCfgService *get() {
        static Mutex lock;
        static MppDecCfgService instance;

        AutoMutex auto_lock(&lock);
        return &instance;
    }

    MppTrieInfo *get_info(const char *name);
    MppTrieInfo *get_info_first();
    MppTrieInfo *get_info_next(MppTrieInfo *info);

    RK_S32 get_node_count() { return mHead.node_count; };
    RK_S32 get_info_count() { return mHead.info_count; };
    RK_S32 get_info_size() { return mHead.info_size; };
    RK_S32 get_cfg_size() { return mCfgSize; };
};

#define EXPAND_AS_TRIE(base, name, cfg_type, flag, field_change, field_data) \
    do { \
        MppCfgInfo tmp = { \
            CFG_FUNC_TYPE_##cfg_type, \
            (RK_U32)((long)&(((MppDecCfgSet *)0)->field_change.change)), \
            flag, \
            (RK_U32)((long)&(((MppDecCfgSet *)0)->field_change.field_data)), \
            sizeof((((MppDecCfgSet *)0)->field_change.field_data)), \
        }; \
        mpp_trie_add_info(mTrie, #base":"#name, &tmp, sizeof(tmp)); \
    } while (0);

#define ENTRY_TABLE(ENTRY)  \
    /* rc config */ \
    ENTRY(base, type,               U32,    MPP_DEC_CFG_CHANGE_TYPE,                base, type) \
    ENTRY(base, coding,             U32,    MPP_DEC_CFG_CHANGE_CODING,              base, coding) \
    ENTRY(base, hw_type,            U32,    MPP_DEC_CFG_CHANGE_HW_TYPE,             base, hw_type) \
    ENTRY(base, batch_mode,         U32,    MPP_DEC_CFG_CHANGE_BATCH_MODE,          base, batch_mode) \
    ENTRY(base, out_fmt,            U32,    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT,       base, out_fmt) \
    ENTRY(base, fast_out,           U32,    MPP_DEC_CFG_CHANGE_FAST_OUT,            base, fast_out) \
    ENTRY(base, fast_parse,         U32,    MPP_DEC_CFG_CHANGE_FAST_PARSE,          base, fast_parse) \
    ENTRY(base, split_parse,        U32,    MPP_DEC_CFG_CHANGE_SPLIT_PARSE,         base, split_parse) \
    ENTRY(base, internal_pts,       U32,    MPP_DEC_CFG_CHANGE_INTERNAL_PTS,        base, internal_pts) \
    ENTRY(base, sort_pts,           U32,    MPP_DEC_CFG_CHANGE_SORT_PTS,            base, sort_pts) \
    ENTRY(base, disable_error,      U32,    MPP_DEC_CFG_CHANGE_DISABLE_ERROR,       base, disable_error) \
    ENTRY(base, enable_vproc,       U32,    MPP_DEC_CFG_CHANGE_ENABLE_VPROC,        base, enable_vproc) \
    ENTRY(base, enable_fast_play,   U32,    MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY,    base, enable_fast_play) \
    ENTRY(base, enable_hdr_meta,    U32,    MPP_DEC_CFG_CHANGE_ENABLE_HDR_META,     base, enable_hdr_meta) \
    ENTRY(base, enable_thumbnail,   U32,    MPP_DEC_CFG_CHANGE_ENABLE_THUMBNAIL,    base, enable_thumbnail) \
    ENTRY(base, enable_mvc,         U32,    MPP_DEC_CFG_CHANGE_ENABLE_MVC,          base, enable_mvc) \
    ENTRY(base, disable_dpb_chk,    U32,    MPP_DEC_CFG_CHANGE_DISABLE_DPB_CHECK,   base, disable_dpb_chk) \
    ENTRY(base, disable_thread,     U32,    MPP_DEC_CFG_CHANGE_DISABLE_THREAD,      base, disable_thread) \
    ENTRY(base, codec_mode,         U32,    MPP_DEC_CFG_CHANGE_CODEC_MODE,          base, codec_mode) \
    ENTRY(base, dis_err_clr_mark,   U32,    MPP_DEC_CFG_CHANGE_DIS_ERR_CLR_MARK,    base, dis_err_clr_mark) \
    ENTRY(cb, pkt_rdy_cb,           Ptr,    MPP_DEC_CB_CFG_CHANGE_PKT_RDY,          cb, pkt_rdy_cb) \
    ENTRY(cb, pkt_rdy_ctx,          Ptr,    MPP_DEC_CB_CFG_CHANGE_PKT_RDY,          cb, pkt_rdy_ctx) \
    ENTRY(cb, pkt_rdy_cmd,          S32,    MPP_DEC_CB_CFG_CHANGE_PKT_RDY,          cb, pkt_rdy_cmd) \
    ENTRY(cb, frm_rdy_cb,           Ptr,    MPP_DEC_CB_CFG_CHANGE_FRM_RDY,          cb, frm_rdy_cb) \
    ENTRY(cb, frm_rdy_ctx,          Ptr,    MPP_DEC_CB_CFG_CHANGE_FRM_RDY,          cb, frm_rdy_ctx) \
    ENTRY(cb, frm_rdy_cmd,          S32,    MPP_DEC_CB_CFG_CHANGE_FRM_RDY,          cb, frm_rdy_cmd)

MppDecCfgService::MppDecCfgService() :
    mTrie(NULL)
{
    rk_s32 ret = mpp_trie_init(&mTrie, "MppDecCfg");
    if (ret) {
        mpp_err_f("failed to init dec cfg set trie\n");
        return ;
    }

    ENTRY_TABLE(EXPAND_AS_TRIE)

    mpp_trie_add_info(mTrie, NULL, NULL, 0);

    mHead.node_count = mpp_trie_get_node_count(mTrie);
    mHead.info_count = mpp_trie_get_info_count(mTrie);
    mHead.info_size = mpp_trie_get_buf_size(mTrie);

    mpp_dec_cfg_dbg_func("node cnt: %d\n", mHead.node_count);
}

MppDecCfgService::~MppDecCfgService()
{
    if (mTrie) {
        mpp_trie_deinit(mTrie);
        mTrie = NULL;
    }
}

MppTrieInfo *MppDecCfgService::get_info(const char *name)
{
    return mpp_trie_get_info(mTrie, name);
}

MppTrieInfo *MppDecCfgService::get_info_first()
{
    if (NULL == mTrie)
        return NULL;

    return mpp_trie_get_info_first(mTrie);
}

MppTrieInfo *MppDecCfgService::get_info_next(MppTrieInfo *node)
{
    if (NULL == mTrie)
        return NULL;

    return mpp_trie_get_info_next(mTrie, node);
}

void mpp_dec_cfg_set_default(MppDecCfgSet *cfg)
{
    cfg->base.type = MPP_CTX_BUTT;
    cfg->base.coding = MPP_VIDEO_CodingUnused;
    cfg->base.hw_type = -1;
    cfg->base.fast_parse = 1;
#ifdef ENABLE_FASTPLAY_ONCE
    cfg->base.enable_fast_play = MPP_ENABLE_FAST_PLAY_ONCE;
#else
    cfg->base.enable_fast_play = MPP_ENABLE_FAST_PLAY;
#endif
}

MPP_RET mpp_dec_cfg_init(MppDecCfg *cfg)
{
    MppDecCfgSet *p = NULL;

    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_dec_cfg_debug", &mpp_dec_cfg_debug, 0);

    p = mpp_calloc(MppDecCfgSet, 1);
    if (NULL == p) {
        mpp_err_f("create decoder config failed %p\n", p);
        *cfg = NULL;
        return MPP_ERR_NOMEM;
    }

    mpp_dec_cfg_set_default(p);

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

#define DEC_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppTrieInfo *node = MppDecCfgService::get()->get_info(name); \
        MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_dec_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), strof_cfg_type(info->data_type)); \
        MPP_RET ret = MPP_CFG_SET_##cfg_type(info, cfg, val); \
        return ret; \
    }

DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_s32, RK_S32, S32);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_u32, RK_U32, U32);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_s64, RK_S64, S64);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_u64, RK_U64, U64);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_ptr, void *, Ptr);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_st,  void *, St);

#define DEC_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppTrieInfo *node = MppDecCfgService::get()->get_info(name); \
        MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_dec_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), strof_cfg_type(info->data_type)); \
        MPP_RET ret = MPP_CFG_GET_##cfg_type(info, cfg, val); \
        return ret; \
    }

DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_s32, RK_S32, S32);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_u32, RK_U32, U32);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_s64, RK_S64, S64);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_u64, RK_U64, U64);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_ptr, void *, Ptr);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_st,  void  , St);

void mpp_dec_cfg_show(void)
{
    MppDecCfgService *srv = MppDecCfgService::get();
    MppTrieInfo *root = srv->get_info_first();

    mpp_log("dumping valid configure string start\n");

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
        } while ((node = srv->get_info_next(node)));
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("total cfg count %d with %d node size %d\n",
            srv->get_info_count(), srv->get_node_count(), srv->get_info_size());
}