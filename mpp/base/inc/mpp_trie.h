/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_TRIE_H__
#define __MPP_TRIE_H__

#include <string.h>

#include "mpp_internal.h"

#define MPP_TRIE_KEY_LEN                (4)
#define MPP_TRIE_KEY_MAX                (1 << (MPP_TRIE_KEY_LEN))

/*
 * MppTrie node buffer layout
 * +---------------+
 * |  MppTrieImpl  |
 * +---------------+
 * |  MppTrieNodes |
 * +---------------+
 * |  MppTrieInfos |
 * +---------------+
 *
 * MppTrieInfo element layout
 * +---------------+
 * |  MppTrieInfo  |
 * +---------------+
 * |  name string  |
 * +---------------+
 * |  User context |
 * +---------------+
 */
typedef struct MppTrieInfo_t {
    rk_u32      index       : 12;
    rk_u32      ctx_len     : 12;
    rk_u32      str_len     : 8;
} MppTrieInfo;

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_trie_init(MppTrie *trie, const char *name);
rk_s32 mpp_trie_init_by_root(MppTrie *trie, void *root);
rk_s32 mpp_trie_deinit(MppTrie trie);

/* Add NULL info to mark the last trie entry */
rk_s32 mpp_trie_add_info(MppTrie trie, const char *name, void *ctx, rk_u32 ctx_len);

rk_s32 mpp_trie_get_node_count(MppTrie trie);
rk_s32 mpp_trie_get_info_count(MppTrie trie);
rk_s32 mpp_trie_get_buf_size(MppTrie trie);
rk_s32 mpp_trie_get_name_max(MppTrie trie);
void *mpp_trie_get_node_root(MppTrie trie);

static inline const char *mpp_trie_info_name(MppTrieInfo *info)
{
    return (info) ? (const char *)(info + 1) : NULL;
}

static inline void *mpp_trie_info_ctx(MppTrieInfo *info)
{
    return (info) ? (void *)((char *)(info + 1) + info->str_len) : NULL;
}

static inline rk_s32 mpp_trie_info_is_self(MppTrieInfo *info)
{
    return (info) ? !!strstr((const char *)(info + 1), "__") : 0;
}

static inline rk_s32 mpp_trie_info_name_is_self(const char *name)
{
    return (name) ? !!strstr(name, "__") : 0;
}

/* trie lookup function */
MppTrieInfo *mpp_trie_get_info(MppTrie trie, const char *name);
MppTrieInfo *mpp_trie_get_info_first(MppTrie trie);
MppTrieInfo *mpp_trie_get_info_next(MppTrie trie, MppTrieInfo *info);
/* root base lookup function */
MppTrieInfo *mpp_trie_get_info_from_root(void *root, const char *name);

void mpp_trie_dump(MppTrie trie, const char *func);
#define mpp_trie_dump_f(trie)   mpp_trie_dump(trie, __FUNCTION__)

void mpp_trie_timing_test(MppTrie trie);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TRIE_H__*/
