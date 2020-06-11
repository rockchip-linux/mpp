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

#define MODULE_TAG "mpp_trie"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp_trie.h"

#define MPP_TRIE_DBG_FUNC               (0x00000001)
#define MPP_TRIE_DBG_SET                (0x00000002)
#define MPP_TRIE_DBG_GET                (0x00000004)
#define MPP_TRIE_DBG_CNT                (0x00000008)

#define trie_dbg(flag, fmt, ...)        _mpp_dbg_f(mpp_trie_debug, flag, fmt, ## __VA_ARGS__)
#define trie_dbg_func(fmt, ...)         trie_dbg(MPP_TRIE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define trie_dbg_set(fmt, ...)          trie_dbg(MPP_TRIE_DBG_SET, fmt, ## __VA_ARGS__)
#define trie_dbg_get(fmt, ...)          trie_dbg(MPP_TRIE_DBG_GET, fmt, ## __VA_ARGS__)
#define trie_dbg_cnt(fmt, ...)          trie_dbg(MPP_TRIE_DBG_CNT, fmt, ## __VA_ARGS__)

/* ascii 32 is ' ', char after space is valid data */
#define MAX_NEXT                        (128 - 32)
#define DEFAULT_NODE_COUNT              900
#define DEFAULT_INFO_COUNT              80

/* spatial optimized tire tree */
typedef struct MppAcNode_t {
    RK_S32          idx;
    RK_S32          info_id;
    RK_S16          next[16];
} MppTrieNode;

typedef struct MppAcImpl_t {
    RK_S32          info_count;
    RK_S32          info_used;
    const char      ***info;
    RK_S32          node_count;
    RK_S32          node_used;
    MppTrieNode     *nodes;
} MppTrieImpl;

RK_U32 mpp_trie_debug = 0;

static RK_S32 trie_get_node(MppTrieImpl *trie)
{
    if (trie->node_used >= trie->node_count) {
        RK_S32 new_count = trie->node_count * 2;
        MppTrieNode *new_nodes = mpp_realloc(trie->nodes, MppTrieNode, new_count);
        if (NULL == new_nodes) {
            mpp_err_f("failed to realloc new nodes %d\n", new_count);
            return -1;
        }

        trie_dbg_cnt("trie %p enlarge node %p:%d -> %p:%d\n",
                     trie, trie->nodes, trie->node_count, new_nodes, new_count);

        trie->nodes = new_nodes;
        trie->node_count = new_count;
    }

    RK_S32 idx = trie->node_used++;
    MppTrieNode *n = &trie->nodes[idx];

    n->idx = idx;
    n->info_id = -1;

    trie_dbg_cnt("get node %d\n", idx);

    return idx;
}

MPP_RET mpp_trie_init(MppTrie *trie, RK_S32 node_count, RK_S32 info_count)
{
    if (NULL == trie) {
        mpp_err_f("invalid NULL input trie automation\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_trie_debug", &mpp_trie_debug, 0);

    MPP_RET ret = MPP_ERR_NOMEM;
    MppTrieImpl *p = mpp_calloc(MppTrieImpl, 1);
    if (NULL == p) {
        mpp_err_f("create trie impl failed\n");
        goto DONE;
    }

    p->node_count = node_count ? node_count : DEFAULT_NODE_COUNT;
    if (p->node_count) {
        p->nodes = mpp_calloc(MppTrieNode, p->node_count);
        if (NULL == p->nodes) {
            mpp_err_f("create %d nodes failed\n", p->node_count);
            goto DONE;
        }
    }

    p->info_count = info_count ? info_count : DEFAULT_INFO_COUNT;
    p->info = mpp_calloc(const char **, p->info_count);
    if (NULL == p->info) {
        mpp_err_f("failed to alloc %d storage\n", p->info_count);
        goto DONE;
    }

    /* get node 0 as root node*/
    trie_get_node(p);
    ret = MPP_OK;

DONE:
    if (ret && p) {
        MPP_FREE(p->info);
        MPP_FREE(p->nodes);
        MPP_FREE(p);
    }

    *trie = p;
    return ret;
}

MPP_RET mpp_trie_deinit(MppTrie trie)
{
    if (NULL == trie) {
        mpp_err_f("invalid NULL input trie automation\n");
        return MPP_ERR_NULL_PTR;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;

    MPP_FREE(p->nodes);
    MPP_FREE(p->info);
    MPP_FREE(p);

    return MPP_OK;
}

MPP_RET mpp_trie_add_info(MppTrie trie, const char **info)
{
    if (NULL == trie || NULL == info) {
        mpp_err_f("invalid trie %p info %p\n", trie, info);
        return MPP_ERR_NULL_PTR;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;

    /* create */
    if (p->info_used >= p->info_count) {
        RK_S32 new_count = p->info_count * 2;
        const char ***ptr = mpp_realloc(p->info, const char **, new_count);
        if (NULL == ptr) {
            mpp_err_f("failed to realloc new action %d\n", new_count);
            return MPP_ERR_MALLOC;
        }

        trie_dbg_cnt("trie %p enlarge info %p:%d -> %p:%d\n",
                     trie, p->info, p->info_count, ptr, new_count);

        p->info = ptr;
        p->info_count = new_count;
    }

    MppTrieNode *node = NULL;
    const char *s = *info;
    RK_S32 len = strnlen(s, SZ_1K);
    RK_S32 next = 0;
    RK_S32 idx = 0;
    RK_S32 i;

    trie_dbg_set("trie %p add info %s len %d\n", trie, s, len);

    for (i = 0; i < len && s[i]; i++) {
        RK_U32 key = s[i];
        RK_S32 key0 = (key >> 4) & 0xf;
        RK_S32 key1 = (key >> 0) & 0xf;

        node = p->nodes + idx;
        next = node->next[key0];

        trie_dbg_set("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d\n",
                     trie, s, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p);
            node->next[key0] = next;

            trie_dbg_set("trie %p add %s at %2d char %c:%3d node %d -> %d as new key0\n",
                         trie, s, i, key, key, node->idx, next);
        }

        idx = next;
        node = p->nodes + idx;
        next = node->next[key1];

        trie_dbg_set("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d as key0\n",
                     trie, s, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p);
            node->next[key1] = next;

            trie_dbg_set("trie %p add %s at %2d char %c:%3d node %d -> %d as new child\n",
                         trie, s, i, key, key, node->idx, next);
        }

        idx = next;
        node = p->nodes + idx;

        trie_dbg_set("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d as key1\n",
                     trie, s, i, key, key, key0, key1, idx, next);
    }

    RK_S32 act_id = p->info_used++;
    p->nodes[idx].info_id = act_id;
    p->info[act_id] = info;

    trie_dbg_set("trie %p add %d info %s at node %d pos %d action %p done\n",
                 trie, i, s, idx, act_id, info);

    return MPP_OK;
}

const char **mpp_trie_get_info(MppTrie trie, const char *name)
{
    if (NULL == trie || NULL == name) {
        mpp_err_f("invalid trie %p name %p\n", trie, name);
        return NULL;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *nodes = p->nodes;
    MppTrieNode *node = nodes;
    const char *s = name;
    RK_S32 len = strlen(name);
    RK_S16 idx = 0;
    RK_S32 i;

    trie_dbg_get("trie %p search %s len %2d start\n", trie, name, len);

    for (i = 0; i < len; i++, s++) {
        idx = node->next[(s[0] >> 4) & 0xf];
        if (!idx)
            break;

        node = &nodes[idx];

        idx = node->next[(s[0] >> 0) & 0xf];
        if (!idx)
            break;

        node = &nodes[idx];
    }

    trie_dbg_get("idx %d node %p info_id %d\n", idx, node, node->info_id);

    return (idx && node && node->info_id >= 0) ? p->info[node->info_id] : NULL;
}

RK_S32 mpp_trie_get_node_count(MppTrie trie)
{
    if (NULL == trie) {
        mpp_err_f("invalid NULL trie\n");
        return 0;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;
    return p->node_used;
}

RK_S32 mpp_trie_get_info_count(MppTrie trie)
{
    if (NULL == trie) {
        mpp_err_f("invalid NULL trie\n");
        return 0;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;
    return p->info_used;
}
