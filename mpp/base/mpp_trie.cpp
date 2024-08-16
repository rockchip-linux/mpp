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
#define MPP_TRIE_DBG_WALK               (0x00000010)
#define MPP_TRIE_DBG_SHRINK             (0x00000020)
#define MPP_TRIE_DBG_SHRINK_STEP        (0x00000040)
#define MPP_TRIE_DBG_SHRINK_CHECK       (0x00000080)

#define trie_dbg(flag, fmt, ...)        _mpp_dbg_f(mpp_trie_debug, flag, fmt, ## __VA_ARGS__)
#define trie_dbg_func(fmt, ...)         trie_dbg(MPP_TRIE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define trie_dbg_set(fmt, ...)          trie_dbg(MPP_TRIE_DBG_SET, fmt, ## __VA_ARGS__)
#define trie_dbg_get(fmt, ...)          trie_dbg(MPP_TRIE_DBG_GET, fmt, ## __VA_ARGS__)
#define trie_dbg_cnt(fmt, ...)          trie_dbg(MPP_TRIE_DBG_CNT, fmt, ## __VA_ARGS__)
#define trie_dbg_walk(fmt, ...)         trie_dbg(MPP_TRIE_DBG_WALK, fmt, ## __VA_ARGS__)
#define trie_dbg_shrink(fmt, ...)       trie_dbg(MPP_TRIE_DBG_SHRINK, fmt, ## __VA_ARGS__)

#define DEFAULT_NODE_COUNT              900
#define DEFAULT_INFO_COUNT              80
#define INVALID_NODE_ID                 (-1)
#define MPP_TRIE_TAG_LEN_MAX            ((sizeof(RK_U64) * 8) / MPP_TRIE_KEY_LEN)

/* spatial optimized tire tree */
typedef struct MppTrieNode_t {
    /* next     - next tire node index */
    RK_S16      next[MPP_TRIE_KEY_MAX];
    /* id       - payload data offset of current tire node */
    RK_S32      id;
    /* idx      - tire node index in ascending order */
    RK_S16      idx;
    /* prev     - previous tire node index */
    RK_S16      prev;

    /* tag_val  - prefix tag */
    RK_U64      tag_val;
    /* key      - current key value in previous node as next */
    RK_U16      key;
    /*
     * tag len  - prefix tag length
     * zero     - normal node with 16 next node
     * positive - tag node with 64bit prefix tag
     */
    RK_S16      tag_len;

    /* next_cnt - valid next node count */
    RK_U16      next_cnt;
} MppTrieNode;

typedef struct MppAcImpl_t {
    RK_S32          info_count;
    RK_S32          info_used;
    MppTrieInfo     *info;
    RK_S32          node_count;
    RK_S32          node_used;
    MppTrieNode     *nodes;
    RK_S32          ctx_size;
    RK_S32          buf_size;
} MppTrieImpl;

RK_U32 mpp_trie_debug = 0;

static RK_S32 trie_get_node(MppTrieImpl *trie, RK_S32 prev, RK_U64 key)
{
    if (trie->node_used >= trie->node_count) {
        RK_S32 old_count = trie->node_count;
        RK_S32 new_count = old_count * 2;
        MppTrieNode *new_nodes = mpp_realloc(trie->nodes, MppTrieNode, new_count);

        if (NULL == new_nodes) {
            mpp_err_f("failed to realloc new nodes %d\n", new_count);
            return -1;
        }

        /* NOTE: new memory should be memset to zero */
        memset(new_nodes + old_count, 0, sizeof(*new_nodes) * old_count);

        trie_dbg_cnt("trie %p enlarge node %p:%d -> %p:%d\n",
                     trie, trie->nodes, trie->node_count, new_nodes, new_count);

        trie->nodes = new_nodes;
        trie->node_count = new_count;
    }

    RK_S32 idx = trie->node_used++;
    MppTrieNode *n = &trie->nodes[idx];

    n->idx = idx;
    n->prev = (prev > 0) ? prev : 0;
    n->key = key;
    n->id = INVALID_NODE_ID;

    if (prev >= 0)
        trie->nodes[prev].next_cnt++;

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
    p->info = mpp_calloc(MppTrieInfo, p->info_count);
    if (NULL == p->info) {
        mpp_err_f("failed to alloc %d storage\n", p->info_count);
        goto DONE;
    }

    /* get node 0 as root node*/
    trie_get_node(p, -1, 0);
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

MPP_RET mpp_trie_add_info(MppTrie trie, const char *name, void *ctx)
{
    if (!trie || !name || !ctx) {
        mpp_err_f("invalid trie %p name %s ctx %p\n", trie, name, ctx);
        return MPP_ERR_NULL_PTR;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;

    /* create */
    if (p->info_used >= p->info_count) {
        RK_S32 new_count = p->info_count * 2;
        MppTrieInfo *ptr = mpp_realloc(p->info, MppTrieInfo, new_count);

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
    const char *s = name;
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
            next = trie_get_node(p, idx, key0);
            /* realloc may cause memory address change */
            node = p->nodes + idx;
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
            next = trie_get_node(p, idx, key1);
            /* realloc may cause memory address change */
            node = p->nodes + idx;
            node->next[key1] = next;

            trie_dbg_set("trie %p add %s at %2d char %c:%3d node %d -> %d as new child\n",
                         trie, s, i, key, key, node->idx, next);
        }

        idx = next;

        trie_dbg_set("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d as key1\n",
                     trie, s, i, key, key, key0, key1, idx, next);
    }

    RK_S32 act_id = p->info_used++;

    p->nodes[idx].id = act_id;

    MppTrieInfo *info = &p->info[act_id];

    info->name = name;
    info->ctx = ctx;
    info->index = act_id;
    info->str_len = MPP_ALIGN(len + 1, sizeof(RK_U64));

    trie_dbg_set("trie %p add %d info %s at node %d pos %d action %p done\n",
                 trie, i, s, idx, act_id, ctx);

    return MPP_OK;
}

static RK_S32 mpp_trie_walk(MppTrieNode *node, RK_U64 *tag_val, RK_S32 *tag_len, RK_U32 key)
{
    RK_U64 val = *tag_val;
    RK_S32 len = *tag_len;

    if (node->tag_len > len) {
        *tag_val = (val << 4) | key;
        *tag_len = len + 1;

        trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> key %x -> tag fill\n",
                      node->idx, node->id, node->tag_len, *tag_len, node->tag_val, *tag_val, key);

        return node->idx;
    }

    /* normal next switch node */
    if (!node->tag_len) {
        trie_dbg_walk("node %d:%d -> key %x -> next %d\n",
                      node->idx, node->id, key, node->next[key]);

        return node->next[key];
    }

    *tag_val = 0;
    *tag_len = 0;

    if (node->tag_val != val) {
        trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> tag mismatch\n",
                      node->idx, node->id, node->tag_len, len, node->tag_val, val);
        return INVALID_NODE_ID;
    }

    trie_dbg_walk("node %d:%d tag len %d - %d val %016llx - %016llx -> tag match -> key %d next %d\n",
                  node->idx, node->id, node->tag_len, len, node->tag_val, val, key, node->next[key]);

    return node->next[key];
}

static MppTrieNode *mpp_trie_get_node(MppTrieNode *root, const char *name)
{
    MppTrieNode *ret = NULL;
    const char *s = name;
    RK_U64 tag_val = 0;
    RK_S32 tag_len = 0;
    RK_S32 idx = 0;

    if (NULL == root || NULL == name) {
        mpp_err_f("invalid root %p name %p\n", root, name);
        return NULL;
    }

    trie_dbg_get("root %p search %s start\n", root, name);

    do {
        RK_U8 key = *s++;
        RK_U32 key0 = (key >> 4) & 0xf;
        RK_U32 key1 = key & 0xf;
        RK_S32 end = (s[0] == '\0');

        idx = mpp_trie_walk(&root[idx], &tag_val, &tag_len, key0);
        if (idx < 0)
            break;

        idx = mpp_trie_walk(&root[idx], &tag_val, &tag_len, key1);
        if (idx < 0 || end)
            break;
    } while (1);

    ret = (idx >= 0) ? &root[idx] : NULL;

    trie_dbg_get("get node %d:%d\n", idx, ret ? ret->id : INVALID_NODE_ID);

    return ret;
}

static RK_S32 mpp_trie_check(MppTrie trie, const char *log)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    RK_S32 i;

    for (i = 0; i < p->info_used; i++) {
        const char *name = (const char *)p->info[i].name;
        MppTrieNode *node = mpp_trie_get_node(p->nodes, name);

        if (node && node->id >= 0 && node->id == i)
            continue;

        mpp_loge("trie check on %s found mismatch info %s %d - %d\n",
                 log, name, i, node ? node->id : -1);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_trie_shrink(MppTrie trie, RK_S32 ctx_size)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *root;
    MppTrieNode *node;
    char *buf;
    RK_S32 node_count;
    RK_S32 node_valid;
    RK_S32 nodes_size;
    RK_S32 len = 0;
    RK_S32 pos = 0;
    RK_S32 i;
    RK_S32 j;

    if (!trie) {
        mpp_err_f("invalid trie %p info size %d\n", trie, ctx_size);
        return MPP_ERR_NULL_PTR;
    }

    root = p->nodes;
    node_count = p->node_used;
    node_valid = node_count;

    trie_dbg_shrink("shrink trie node start node %d info %d\n", node_count, p->info_used);

    if (mpp_trie_debug & MPP_TRIE_DBG_SHRINK_STEP)
        mpp_trie_dump_f(trie);

    for (i = node_count - 1; i > 0; i--) {
        MppTrieNode *prev;
        RK_S32 prev_idx;

        node = &root[i];
        prev_idx = node->prev;
        prev = &root[prev_idx];

        if (prev->next_cnt > 1) {
            trie_dbg_shrink("node %d:%d prev %d next count %d stop shrinking for multi next\n",
                            i, node->id, prev_idx, prev->next_cnt);
            continue;
        }

        if (node->tag_len >= (RK_S16)MPP_TRIE_TAG_LEN_MAX) {
            trie_dbg_shrink("node %d:%d tag %d - %016llx stop shrinking for max tag len\n",
                            i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        if (prev->id >= 0) {
            trie_dbg_shrink("node %d:%d tag %d - %016llx stop shrinking for valid info node\n",
                            i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        prev->id = node->id;
        /* NOTE: do NOT increase tag length on root node */
        prev->tag_len = node->tag_len + 1;
        prev->tag_val = ((RK_U64)node->key << (node->tag_len * 4)) | node->tag_val;
        prev->next_cnt = node->next_cnt;
        memcpy(prev->next, node->next, sizeof(node->next));

        trie_dbg_shrink("node %d:%d shrink prev %d key %x tag %016llx -> %016llx\n",
                        i, node->id, prev->idx, prev->key, node->tag_val, prev->tag_val);

        for (j = 0; j < MPP_TRIE_KEY_MAX; j++) {
            if (!prev->next[j])
                continue;

            root[prev->next[j]].prev = prev_idx;
        }

        memset(node, 0, sizeof(*node));
        node->id = INVALID_NODE_ID;
        node_valid--;
    }

    trie_dbg_shrink("shrink trie node finish count %d -> %d\n", node_count, node_valid);

    if (mpp_trie_debug & MPP_TRIE_DBG_SHRINK_STEP)
        mpp_trie_dump_f(trie);

    if (mpp_trie_debug & MPP_TRIE_DBG_SHRINK_CHECK)
        mpp_trie_check(trie, "shrink merge tag stage");

    trie_dbg_shrink("move trie node start to reduce memory %d -> %d\n", node_count, node_valid);

    for (i = 1; i < node_valid; i++) {
        node = &root[i];

        /* skip valid node */
        if (node->idx)
            continue;

        for (j = i; j < node_count; j++) {
            MppTrieNode *tmp = &root[j];
            MppTrieNode *prev;
            RK_S32 k;

            /* skip empty node */
            if (!tmp->idx)
                continue;

            trie_dbg_shrink("move node %d to %d prev %d\n", j, i, tmp->prev);

            prev = &root[tmp->prev];

            /* relink previous node */
            for (k = 0; k < MPP_TRIE_KEY_MAX; k++) {
                if (prev->next[k] != tmp->idx)
                    continue;

                prev->next[k] = i;
                break;
            }

            memcpy(node, tmp, sizeof(*node));
            node->idx = i;
            memset(tmp, 0, sizeof(*tmp));

            /* relink next node */
            for (k = 0; k < MPP_TRIE_KEY_MAX; k++) {
                if (!node->next[k])
                    continue;

                root[node->next[k]].prev = i;
            }

            break;
        }
    }

    p->node_used = node_valid;

    trie_dbg_shrink("move trie node finish used %d\n", p->node_used);

    if (mpp_trie_debug & MPP_TRIE_DBG_SHRINK_STEP)
        mpp_trie_dump_f(trie);

    if (mpp_trie_debug & MPP_TRIE_DBG_SHRINK_CHECK)
        mpp_trie_check(trie, "shrink move node stage");

    trie_dbg_shrink("create user buffer start\n");

    p->ctx_size = ctx_size;
    nodes_size = sizeof(MppTrieNode) * p->node_used;

    pos += nodes_size;
    /* update info size and string name size */
    for (i = 0; i < p->info_used; i++) {
        len = p->info[i].str_len;
        pos += ctx_size + sizeof(MppTrieInfo) + len;
    }

    len = pos;
    buf = mpp_calloc_size(char, len);
    if (!buf) {
        mpp_loge("failed to alloc trie buffer size %d\n", len);
        return MPP_NOK;
    }

    p->nodes = (MppTrieNode *)buf;
    p->buf_size = len;
    memcpy(p->nodes, root, nodes_size);
    pos = nodes_size;

    for (i = 0; i < p->info_used; i++) {
        MppTrieInfo *info;
        const char *name = p->info[i].name;

        node = mpp_trie_get_node(p->nodes, name);
        node->id = pos;

        /* reserve user context space */
        pos += ctx_size;

        /* reserve node info */
        info = (MppTrieInfo *)(buf + pos);
        memcpy(info, &p->info[i], sizeof(MppTrieInfo));
        pos += sizeof(MppTrieInfo);

        /* copy info name */
        strncpy(buf + pos, name, info->str_len);
        pos += info->str_len;
    }

    MPP_FREE(root);

    return MPP_OK;
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

RK_S32 mpp_trie_get_buf_size(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->buf_size : 0;
}

MppTrieNode *mpp_trie_node_root(MppTrie trie)
{
    if (NULL == trie) {
        mpp_err_f("invalid NULL trie\n");
        return NULL;
    }

    MppTrieImpl *p = (MppTrieImpl *)trie;
    return p->nodes;
}

MppTrieInfo *mpp_trie_get_info(MppTrie trie, const char *name)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *node;

    if (NULL == trie || NULL == name) {
        mpp_err_f("invalid trie %p name %p\n", trie, name);
        return NULL;
    }

    node = mpp_trie_get_node(p->nodes, name);
    if (!node || node->id < 0)
        return NULL;

    return (MppTrieInfo *)(((RK_U8 *)p->nodes) + node->id + p->ctx_size);
}

void *mpp_trie_get_slot(MppTrie trie, const char *name)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *node;

    if (NULL == trie || NULL == name) {
        mpp_err_f("invalid trie %p name %p\n", trie, name);
        return NULL;
    }

    if (!p->buf_size) {
        mpp_err_f("trie %p buffer is not shrinked and not ready for usage\n", trie);
        return NULL;
    }

    node = mpp_trie_get_node(p->nodes, name);
    if (!node || node->id < 0)
        return NULL;

    return (void *)(((RK_U8 *)p->nodes) + node->id);
}

void *mpp_trie_get_slot_first(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? ((RK_U8 *)p->nodes) + p->node_used * sizeof(MppTrieNode) : NULL;
}

void *mpp_trie_get_slot_next(MppTrie trie, void *slot)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieInfo *info;

    if (!p || !slot)
        return NULL;

    info = (MppTrieInfo *)((RK_U8 *)slot + p->ctx_size);
    if (info->index < 0 || info->index >= p->info_used - 1)
        return NULL;

    return (void *)((RK_U8 *)slot + p->ctx_size + sizeof(MppTrieInfo) + info->str_len);
}

void mpp_trie_dump(MppTrie trie, const char *func)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    RK_S32 i;
    RK_S32 next_cnt[17];
    RK_S32 tag_len[17];

    memset(next_cnt, 0, sizeof(next_cnt));
    memset(tag_len, 0, sizeof(tag_len));

    mpp_logi("%s dumping node count %d used %d\n", func, p->node_count, p->node_used);

    for (i = 0; i < p->node_used; i++) {
        MppTrieNode *node = &p->nodes[i];
        RK_S32 valid_count = 0;
        RK_S32 j;

        if (i && !node->idx)
            continue;

        if (node->id >= 0)
            mpp_logi("node %d key %x info %d - %s\n", node->idx, node->key, node->id, p->info[node->id].name);
        else
            mpp_logi("node %d key %x\n", node->idx, node->key);

        if (node->tag_len)
            mpp_logi("    prev %d count %d tag %d - %llx\n", node->prev, node->next_cnt, node->tag_len, node->tag_val);
        else
            mpp_logi("    prev %d count %d\n", node->prev, node->next_cnt);

        for (j = 0; j < MPP_TRIE_KEY_MAX; j++) {
            if (node->next[j] > 0) {
                mpp_logi("    next %d:%d -> %d\n", valid_count, j, node->next[j]);
                valid_count++;
            }
        }

        next_cnt[valid_count]++;
        tag_len[node->tag_len]++;
    }

    mpp_logi("node | next |  tag | used %d\n", p->node_used);

    for (i = 0; i < 17; i++) {
        if (next_cnt[i] || tag_len[i])
            mpp_logi("%2d   | %4d | %4d |\n", i, next_cnt[i], tag_len[i]);
    }

    mpp_logi("%s dumping node end\n", func, p->node_used);
}