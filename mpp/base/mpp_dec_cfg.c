/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_dec_cfg"

#include "rk_vdec_cfg.h"
#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_lock.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "mpp_trie.h"
#include "mpp_cfg.h"
#include "mpp_cfg_io.h"
#include "mpp_dec_cfg.h"

#define MPP_DEC_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    CFG_DEF_START() \
    STRUCT_START(base) \
    ENTRY(prefix, u32, rk_u32,     type,                FLAG_BASE(0),      base, type) \
    ENTRY(prefix, u32, rk_u32,     coding,              FLAG_AT(1),     base, coding) \
    ENTRY(prefix, u32, rk_u32,     hw_type,             FLAG_AT(2),     base, hw_type) \
    ENTRY(prefix, u32, rk_u32,     batch_mode,          FLAG_AT(3),     base, batch_mode) \
    ENTRY(prefix, u32, rk_u32,     out_fmt,             FLAG_AT(4),     base, out_fmt) \
    ENTRY(prefix, u32, rk_u32,     fast_out,            FLAG_AT(5),     base, fast_out) \
    ENTRY(prefix, u32, rk_u32,     fast_parse,          FLAG_AT(6),     base, fast_parse) \
    ENTRY(prefix, u32, rk_u32,     split_parse,         FLAG_AT(7),     base, split_parse) \
    ENTRY(prefix, u32, rk_u32,     internal_pts,        FLAG_AT(8),     base, internal_pts) \
    ENTRY(prefix, u32, rk_u32,     sort_pts,            FLAG_AT(9),     base, sort_pts) \
    ENTRY(prefix, u32, rk_u32,     disable_error,       FLAG_AT(10),    base, disable_error) \
    ENTRY(prefix, u32, rk_u32,     enable_vproc,        FLAG_AT(11),    base, enable_vproc) \
    ENTRY(prefix, u32, rk_u32,     enable_fast_play,    FLAG_AT(12),    base, enable_fast_play) \
    ENTRY(prefix, u32, rk_u32,     enable_hdr_meta,     FLAG_AT(13),    base, enable_hdr_meta) \
    ENTRY(prefix, u32, rk_u32,     enable_thumbnail,    FLAG_AT(14),    base, enable_thumbnail) \
    ENTRY(prefix, u32, rk_u32,     enable_mvc,          FLAG_AT(15),    base, enable_mvc) \
    ENTRY(prefix, u32, rk_u32,     disable_dpb_chk,     FLAG_AT(16),    base, disable_dpb_chk) \
    ENTRY(prefix, u32, rk_u32,     disable_thread,      FLAG_AT(17),    base, disable_thread) \
    ENTRY(prefix, u32, rk_u32,     codec_mode,          FLAG_AT(18),    base, codec_mode) \
    ENTRY(prefix, u32, rk_u32,     dis_err_clr_mark,    FLAG_AT(19),    base, dis_err_clr_mark) \
    STRUCT_END(base) \
    STRUCT_START(cb) \
    ENTRY(prefix, ptr, void *,     pkt_rdy_cb,          FLAG_BASE(0),      cb, pkt_rdy_cb) \
    ENTRY(prefix, ptr, void *,     pkt_rdy_ctx,         FLAG_AT(0),     cb, pkt_rdy_ctx) \
    ENTRY(prefix, s32, rk_s32,     pkt_rdy_cmd,         FLAG_AT(0),     cb, pkt_rdy_cmd) \
    ENTRY(prefix, ptr, void *,     frm_rdy_cb,          FLAG_AT(1),     cb, frm_rdy_cb) \
    ENTRY(prefix, ptr, void *,     frm_rdy_ctx,         FLAG_AT(1),     cb, frm_rdy_ctx) \
    ENTRY(prefix, s32, rk_s32,     frm_rdy_cmd,         FLAG_AT(1),     cb, frm_rdy_cmd) \
    STRUCT_END(cb) \
    CFG_DEF_END()

rk_s32 mpp_dec_cfg_set_default(void *entry, KmppObj obj, const char *caller)
{
    MppDecCfgSet *cfg = (MppDecCfgSet *)entry;

    cfg->base.type = MPP_CTX_BUTT;
    cfg->base.coding = MPP_VIDEO_CodingUnused;
    cfg->base.hw_type = -1;
    cfg->base.fast_parse = 1;
#ifdef ENABLE_FASTPLAY_ONCE
    cfg->base.enable_fast_play = MPP_ENABLE_FAST_PLAY_ONCE;
#else
    cfg->base.enable_fast_play = MPP_ENABLE_FAST_PLAY;
#endif
    (void) obj;
    (void) caller;

    return rk_ok;
}

#define KMPP_OBJ_NAME               mpp_dec_cfg
#define KMPP_OBJ_INTF_TYPE          MppDecCfg
#define KMPP_OBJ_IMPL_TYPE          MppDecCfgSet
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_DEC_CFG
#define KMPP_OBJ_FUNC_INIT          mpp_dec_cfg_set_default
#define KMPP_OBJ_ENTRY_TABLE        MPP_DEC_CFG_ENTRY_TABLE
#define KMPP_OBJ_ACCESS_DISABLE
#define KMPP_OBJ_HIERARCHY_ENABLE
#include "kmpp_obj_helper.h"

/* wrapper for old interface */
MPP_RET mpp_dec_cfg_init(MppDecCfg *cfg)
{
    return mpp_dec_cfg_get(cfg);
}

MPP_RET mpp_dec_cfg_deinit(MppDecCfg cfg)
{
    return mpp_dec_cfg_put(cfg);
}

#define DEC_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return rk_nok; \
        } \
        return kmpp_obj_set_##cfg_type(cfg, name, val); \
    }

DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_s32, RK_S32, s32);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_u32, RK_U32, u32);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_s64, RK_S64, s64);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_u64, RK_U64, u64);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_ptr, void *, ptr);
DEC_CFG_SET_ACCESS(mpp_dec_cfg_set_st,  void *, st);

#define DEC_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppDecCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return rk_nok; \
        } \
        return kmpp_obj_get_##cfg_type(cfg, name, val); \
    }

DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_s32, RK_S32, s32);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_u32, RK_U32, u32);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_s64, RK_S64, s64);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_u64, RK_U64, u64);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_ptr, void *, ptr);
DEC_CFG_GET_ACCESS(mpp_dec_cfg_get_st,  void  , st);

void mpp_dec_cfg_show(void)
{
    MppTrie trie = kmpp_objdef_get_trie(mpp_dec_cfg_def);
    MppTrieInfo *root;

    if (!trie)
        return;

    root = mpp_trie_get_info_first(trie);

    mpp_log("dumping valid configure string start\n");

    if (root) {
        MppTrieInfo *node = root;
        rk_s32 len = mpp_trie_get_name_max(trie);

        mpp_log("%-*s %-6s | %6s | %4s | %4s\n", len, "name", "type", "offset", "size", "flag (hex)");

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

    mpp_log("dec cfg size %d count %d with trie node %d size %d\n",
            sizeof(MppDecCfgSet), mpp_trie_get_info_count(trie),
            mpp_trie_get_node_count(trie), mpp_trie_get_buf_size(trie));
}