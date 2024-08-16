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

#ifndef __MPP_TRIE_H__
#define __MPP_TRIE_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppTrie;

#define MPP_TRIE_KEY_LEN                (4)
#define MPP_TRIE_KEY_MAX                (MPP_TRIE_KEY_LEN << 4)

/*
 * MppTire node buffer layout
 * +---------------+
 * |  MppTrieImpl  |
 * +---------------+
 * |  MppTireNodes |
 * +---------------+
 * |  MppTrieInfos |
 * +---------------+
 *
 * MppTrieInfo element layout
 * +---------------+
 * |  User context |
 * +---------------+
 * |  MppTrieInfo  |
 * +---------------+
 * |  name string  |
 * +---------------+
 */
typedef struct MppTrieInfo_t {
    /* original name string address, maybe invalid stack address */
    const char  *name;
    /* original context address, maybe invalid stack address */
    void        *ctx;
    /* always valid data */
    RK_S16      index;
    RK_S16      str_len;
} MppTrieInfo;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_trie_init(MppTrie *trie, RK_S32 node_count, RK_S32 info_count);
MPP_RET mpp_trie_deinit(MppTrie trie);

MPP_RET mpp_trie_add_info(MppTrie trie, const char *name, void *ctx);
MPP_RET mpp_trie_shrink(MppTrie trie, RK_S32 info_size);

RK_S32 mpp_trie_get_node_count(MppTrie trie);
RK_S32 mpp_trie_get_info_count(MppTrie trie);
RK_S32 mpp_trie_get_buf_size(MppTrie trie);

/* trie lookup function */
MppTrieInfo *mpp_trie_get_info(MppTrie trie, const char *name);
/* trie lookup slot function for context filling */
void *mpp_trie_get_slot(MppTrie trie, const char *name);
void *mpp_trie_get_slot_first(MppTrie trie);
void *mpp_trie_get_slot_next(MppTrie trie, void *slot);

void mpp_trie_dump(MppTrie trie, const char *func);
#define mpp_trie_dump_f(tire)   mpp_trie_dump(tire, __FUNCTION__)

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TRIE_H__*/
