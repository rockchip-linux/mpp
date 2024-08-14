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

/* spatial optimized tire tree */
typedef struct MppAcNode_t {
    /* id       - tire node carried payload data */
    RK_S32      id;
    /* idx      - tire node index in ascending order */
    RK_S16      idx;
    /* prev     - tire node index in ascending order */
    RK_S16      prev;
    /* key      - current key value in previous node as next */
    RK_S16      key;

    /* tag len  - common tag length
     * zero     - normal node with 16 next node
     * positive - single path node with 4bit unit tag length */
    RK_S16      tag_len;
    /* id_tag   - last tag index */
    RK_U64      tag_val;

    /* valid next position bitmap */
    RK_U16      next_cnt;
    RK_S16      next[MPP_TRIE_KEY_MAX];
} MppTrieNode;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_trie_init(MppTrie *trie, RK_S32 node_count, RK_S32 info_count);
MPP_RET mpp_trie_deinit(MppTrie trie);

MPP_RET mpp_trie_add_info(MppTrie trie, const char **info);
MPP_RET mpp_trie_shrink(MppTrie trie, RK_S32 info_size);

RK_S32 mpp_trie_get_node_count(MppTrie trie);
RK_S32 mpp_trie_get_info_count(MppTrie trie);

MppTrieNode *mpp_trie_get_node(MppTrieNode *root, const char *name);
const char **mpp_trie_get_info(MppTrie trie, const char *name);
MppTrieNode *mpp_trie_node_root(MppTrie trie);

void mpp_trie_dump(MppTrie trie, const char *func);
#define mpp_trie_dump_f(tire)   mpp_trie_dump(tire, __FUNCTION__)

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TRIE_H__*/
