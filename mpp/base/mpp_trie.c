/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_trie"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_trie.h"

#define MPP_TRIE_DBG_PAVE               (0x00000001)
#define MPP_TRIE_DBG_SET                (0x00000002)
#define MPP_TRIE_DBG_GET                (0x00000004)
#define MPP_TRIE_DBG_CNT                (0x00000008)
#define MPP_TRIE_DBG_WALK               (0x00000010)
#define MPP_TRIE_DBG_LAST               (0x00000020)
#define MPP_TRIE_DBG_LAST_STEP          (0x00000040)
#define MPP_TRIE_DBG_LAST_CHECK         (0x00000080)
#define MPP_TRIE_DBG_IMPORT             (0x00000100)

#define trie_dbg(flag, fmt, ...)        _mpp_dbg_f(mpp_trie_debug, flag, fmt, ## __VA_ARGS__)
#define trie_dbg_pave(fmt, ...)         trie_dbg(MPP_TRIE_DBG_PAVE, fmt, ## __VA_ARGS__)
#define trie_dbg_set(fmt, ...)          trie_dbg(MPP_TRIE_DBG_SET, fmt, ## __VA_ARGS__)
#define trie_dbg_get(fmt, ...)          trie_dbg(MPP_TRIE_DBG_GET, fmt, ## __VA_ARGS__)
#define trie_dbg_cnt(fmt, ...)          trie_dbg(MPP_TRIE_DBG_CNT, fmt, ## __VA_ARGS__)
#define trie_dbg_walk(fmt, ...)         trie_dbg(MPP_TRIE_DBG_WALK, fmt, ## __VA_ARGS__)
#define trie_dbg_last(fmt, ...)         trie_dbg(MPP_TRIE_DBG_LAST, fmt, ## __VA_ARGS__)

#define MPP_TRIE_KEY_LEN                (4)
#define MPP_TRIE_KEY_MAX                (1 << (MPP_TRIE_KEY_LEN))
#define MPP_TRIE_INFO_MAX               (1 << 12)
#define MPP_TRIE_NAME_MAX               (1 << 12)

#define DEFAULT_NODE_COUNT              900
#define DEFAULT_INFO_COUNT              80
#define INVALID_NODE_ID                 (-1)
#define MPP_TRIE_TAG_LEN_MAX            ((sizeof(rk_u64) * 8) / MPP_TRIE_KEY_LEN)

/* spatial optimized trie tree */
typedef struct MppTrieNode_t {
    /* next         - next trie node index */
    rk_s16          next[MPP_TRIE_KEY_MAX];
    /* id           - payload data offset of current trie node */
    rk_s32          id;
    /* idx          - trie node index in ascending order */
    rk_s16          idx;
    /* prev         - previous trie node index */
    rk_s16          prev;

    /* tag_val      - prefix tag */
    rk_u64          tag_val;
    /* key          - current key value in previous node as next */
    rk_u16          key;
    /*
     * tag len      - prefix tag length
     * zero         - normal node with 16 next node
     * positive     - tag node with 64bit prefix tag
     */
    rk_s16          tag_len;

    /* next_cnt     - valid next node count */
    rk_u16          next_cnt;
} MppTrieNode;

typedef struct MppTrieImpl_t {
    char            *name;
    rk_s32          name_len;
    rk_s32          buf_size;

    rk_s32          node_count;
    rk_s32          node_used;
    MppTrieNode     *nodes;

    /* info and name record buffer */
    void            *info_buf;
    rk_s32          info_count;
    rk_s32          info_buf_size;
    rk_s32          info_buf_pos;
    rk_s32          info_name_max;
} MppTrieImpl;

/* work structure for trie traversal */
typedef struct MppTrieWalk_t {
    MppTrieNode     *root;
    rk_u64          tag;
    rk_u32          len;
    rk_s32          match;
} MppTrieWalk;

rk_u32 mpp_trie_debug = 0;

static rk_s32 trie_get_node(MppTrieImpl *trie, rk_s32 prev, rk_u64 key)
{
    MppTrieNode *node;
    rk_s32 idx;

    if (trie->node_used >= trie->node_count) {
        rk_s32 old_count = trie->node_count;
        rk_s32 new_count = old_count * 2;
        MppTrieNode *new_nodes = mpp_realloc(trie->nodes, MppTrieNode, new_count);

        if (!new_nodes) {
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

    idx = trie->node_used++;
    node = &trie->nodes[idx];
    node->idx = idx;
    node->prev = (prev > 0) ? prev : 0;
    node->key = key;
    node->id = INVALID_NODE_ID;

    if (prev >= 0)
        trie->nodes[prev].next_cnt++;

    trie_dbg_cnt("get node %d\n", idx);

    return idx;
}

static rk_s32 trie_prepare_buf(MppTrieImpl *p, rk_s32 info_size)
{
    if (p->info_count >= MPP_TRIE_INFO_MAX) {
        mpp_loge_f("invalid trie info count %d larger than max %d\n",
                   p->info_count, MPP_TRIE_INFO_MAX);
        return rk_nok;
    }

    /* check and enlarge info buffer */
    if (p->info_buf_pos + info_size > p->info_buf_size) {
        rk_s32 old_size = p->info_buf_size;
        rk_s32 new_size = old_size * 2;
        void *ptr = mpp_realloc(p->info_buf, void, new_size);

        if (!ptr) {
            mpp_loge_f("failed to realloc new info buffer %d\n", new_size);
            return rk_nok;
        }

        trie_dbg_cnt("trie %p enlarge info_buf %p:%d -> %p:%d\n",
                     p, p->info_buf, old_size, ptr, new_size);

        p->info_buf = ptr;
        p->info_buf_size = new_size;
    }

    return rk_ok;
}

static rk_s32 trie_pave_node(MppTrieImpl *p, const char *name, rk_s32 str_len)
{
    MppTrieNode *node = NULL;
    const char *s = name;
    rk_s32 next = 0;
    rk_s32 idx = 0;
    rk_s32 i;

    trie_dbg_pave("trie %p add info %s len %d\n", p, s, str_len);

    for (i = 0; i < str_len && s[i]; i++) {
        rk_u32 key = s[i];
        rk_s32 key0 = (key >> 4) & 0xf;
        rk_s32 key1 = (key >> 0) & 0xf;

        node = p->nodes + idx;
        next = node->next[key0];

        trie_dbg_pave("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d\n",
                      p, name, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p, idx, key0);
            if (next < 0)
                return next;

            /* realloc may cause memory address change */
            node = p->nodes + idx;
            node->next[key0] = next;

            trie_dbg_pave("trie %p add %s at %2d char %c:%3d node %d -> %d as new key0\n",
                          p, name, i, key, key, node->idx, next);
        }

        idx = next;
        node = p->nodes + idx;
        next = node->next[key1];

        trie_dbg_pave("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d as key0\n",
                      p, s, i, key, key, key0, key1, idx, next);

        if (!next) {
            next = trie_get_node(p, idx, key1);
            if (next < 0)
                return next;

            /* realloc may cause memory address change */
            node = p->nodes + idx;
            node->next[key1] = next;

            trie_dbg_pave("trie %p add %s at %2d char %c:%3d node %d -> %d as new child\n",
                          p, name, i, key, key, node->idx, next);
        }

        idx = next;

        trie_dbg_pave("trie %p add %s at %2d char %c:%3d:%x:%x node %d -> %d as key1\n",
                      p, name, i, key, key, key0, key1, idx, next);
    }

    return idx;
}

rk_s32 mpp_trie_init(MppTrie *trie, const char *name)
{
    MppTrieImpl *p;
    rk_s32 name_len;
    rk_s32 ret = rk_nok;

    if (!trie || !name) {
        mpp_loge_f("invalid trie %p name %p\n", trie, name);
        return ret;
    }

    mpp_env_get_u32("mpp_trie_debug", &mpp_trie_debug, 0);

    name_len = strnlen(name, MPP_TRIE_NAME_MAX) + 1;
    p = mpp_calloc_size(MppTrieImpl, sizeof(*p) + name_len);
    if (!p) {
        mpp_loge_f("create trie impl %s failed\n", name);
        goto done;
    }

    p->name = (char *)(p + 1);
    p->name_len = name_len;
    strncpy(p->name, name, name_len);

    p->node_count = DEFAULT_NODE_COUNT;
    p->nodes = mpp_calloc(MppTrieNode, p->node_count);
    if (!p->nodes) {
        mpp_loge_f("create %d nodes failed\n", p->node_count);
        goto done;
    }

    p->info_buf_size = 4096;
    p->info_buf = mpp_calloc(void, p->info_buf_size);
    if (!p->info_buf) {
        mpp_loge_f("failed to alloc %d info buffer\n", p->info_buf_size);
        goto done;
    }

    /* get node 0 as root node*/
    trie_get_node(p, -1, 0);

    ret = rk_ok;

done:
    if (ret && p) {
        MPP_FREE(p->info_buf);
        MPP_FREE(p->nodes);
        MPP_FREE(p);
    }

    *trie = p;
    return ret;
}

rk_s32 mpp_trie_init_by_root(MppTrie *trie, void *root)
{
    MppTrieImpl *p;
    MppTrieInfo *info;
    rk_s32 i;

    if (!trie || !root) {
        mpp_loge_f("invalid trie %p root %p\n", trie, root);
        return rk_nok;
    }

    *trie = NULL;
    p = mpp_calloc(MppTrieImpl, 1);
    if (!p) {
        mpp_loge_f("create trie impl failed\n");
        return rk_nok;
    }

    info = mpp_trie_get_info_from_root(root, "__name__");
    if (info)
        p->name = mpp_trie_info_ctx(info);

    info = mpp_trie_get_info_from_root(root, "__node__");
    if (info)
        p->node_used = *(rk_u32 *)mpp_trie_info_ctx(info);

    info = mpp_trie_get_info_from_root(root, "__info__");
    if (info)
        p->info_count = *(rk_u32 *)mpp_trie_info_ctx(info);

    /* import and update new buffer */
    p->nodes = (MppTrieNode *)root;

    if (mpp_trie_debug & MPP_TRIE_DBG_IMPORT)
        mpp_trie_dump(p, "root import");

    info = mpp_trie_get_info_first(p);

    for (i = 0; i < p->info_count; i++) {
        const char *name = mpp_trie_info_name(info);
        MppTrieInfo *info_set = info;
        MppTrieInfo *info_ret = mpp_trie_get_info(p, name);

        info = mpp_trie_get_info_next(p, info);

        if (info_ret && info_set == info_ret && info_ret->index == i)
            continue;

        mpp_loge_f("trie check on import found mismatch info %s [%d:%p] - [%d:%p]\n",
                   name, i, info_set, info_ret ? info_ret->index : -1, info_ret);
        return rk_nok;
    }

    *trie = p;

    return rk_ok;
}

rk_s32 mpp_trie_deinit(MppTrie trie)
{
    if (trie) {
        MppTrieImpl *p = (MppTrieImpl *)trie;

        /* NOTE: do not remvoe imported root */
        if (p->node_count)
            mpp_free(p->nodes);
        else
            p->nodes = NULL;

        MPP_FREE(p->info_buf);
        MPP_FREE(p);
        return rk_ok;
    }

    return rk_nok;
}

static rk_s32 mpp_trie_walk(MppTrieWalk *p, rk_s32 idx, rk_u32 key, rk_u32 keyx, rk_u32 end)
{
    MppTrieNode *node = &p->root[idx];
    char log_buf[128];
    rk_u32 log_size = sizeof(log_buf) - 1;
    rk_u32 log_len = 0;
    rk_u32 log_en = (mpp_trie_debug & MPP_TRIE_DBG_WALK) ? 1 : 0;
    rk_s32 next = -1;

    if (log_en)
        log_len += snprintf(log_buf + log_len, log_size - log_len,
                            "node %d:%d key %02x:%x -> ",
                            node->idx, node->id, key, keyx);

    /* check high 4 bits */
    if (!node->tag_len || p->match == node->idx) {
        /* not tag or tag has be matched -> normal next switch node */
        next = node->next[keyx];
        if (log_en)
            log_len += snprintf(log_buf + log_len, log_size - log_len,
                                "tag %s -> ",
                                !node->tag_len ? "n/a" : "match");
        p->tag = 0;
        p->len = 0;
        p->match = -1;
    } else {
        /* with tag check len to fill or to stop */
        rk_u64 val_new = (p->tag << 4) | keyx;
        rk_s32 len_new = p->len + 1;

        if (log_en)
            log_len += snprintf(log_buf + log_len, log_size - log_len,
                                "tag %0*llx:%d - st %0*llx:%d -> ",
                                node->tag_len, node->tag_val, node->tag_len,
                                node->tag_len, val_new, len_new);

        /* clear old matched node */
        p->match = -1;
        if (node->tag_len > len_new && !end) {
            /* more tag to fill keep current node */
            p->tag = val_new;
            p->len = len_new;
            next = node->idx;
            if (log_en)
                log_len += snprintf(log_buf + log_len, log_size - log_len,
                                    "fill -> ");
        } else {
            /* check tag match with new value */
            p->tag = 0;
            p->len = 0;

            if (node->tag_val != val_new) {
                if (log_en)
                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        "mismatch -> ");
                next = INVALID_NODE_ID;
            } else {
                if (log_en)
                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                        "match -> ");
                next = node->idx;
                p->match = node->idx;
            }
        }
    }

    if (log_en) {
        snprintf(log_buf + log_len, log_size - log_len, "%d", next);
        mpp_logi_f("%s\n", log_buf);
    }

    return next;
}

static MppTrieNode *mpp_trie_get_node(MppTrieNode *root, const char *name)
{
    MppTrieNode *ret = NULL;
    const char *s = name;
    MppTrieWalk walk;
    rk_s32 idx = 0;

    if (!root || !name) {
        mpp_err_f("invalid root %p name %p\n", root, name);
        return NULL;
    }

    trie_dbg_get("root %p search %s start\n", root, name);

    walk.root = root;
    walk.tag = 0;
    walk.len = 0;
    walk.match = 0;

    do {
        char key = *s++;
        rk_u32 key0 = (key >> 4) & 0xf;
        rk_u32 key1 = key & 0xf;
        rk_u32 end = s[0] == '\0';

        idx = mpp_trie_walk(&walk, idx, key, key0, 0);
        if (idx < 0)
            break;

        idx = mpp_trie_walk(&walk, idx, key, key1, end);
        if (idx < 0 || end)
            break;
    } while (1);

    ret = (idx >= 0) ? &root[idx] : NULL;

    trie_dbg_get("get %s ret node %d:%d\n", name, idx, ret ? ret->id : INVALID_NODE_ID);

    return ret;
}

static rk_s32 mpp_trie_check(MppTrie trie, const char *log)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    char *base = p->info_buf;
    rk_s32 info_size = 0;
    rk_s32 pos = 0;
    rk_s32 i;

    for (i = 0; i < p->info_count; i++, pos += info_size) {
        MppTrieInfo *info = (MppTrieInfo *)(base + pos);
        const char *name = (const char *)(info + 1);
        MppTrieNode *node = mpp_trie_get_node(p->nodes, name);

        info_size = sizeof(*info) + info->str_len + info->ctx_len;

        if (node && node->id >= 0 && node->id == pos)
            continue;

        mpp_loge("trie check on %s found mismatch info %s %d - %d\n",
                 log, name, i, node ? node->id : -1);
        return rk_nok;
    }

    return rk_ok;
}

rk_s32 mpp_trie_last_info(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *root;
    MppTrieNode *node;
    char *buf;
    rk_s32 node_count;
    rk_s32 node_valid;
    rk_s32 nodes_size;
    rk_s32 len = 0;
    rk_s32 pos = 0;
    rk_s32 i;
    rk_s32 j;

    if (!trie) {
        mpp_loge_f("invalid NULL trie\n");
        return rk_nok;
    }

    /* write trie self entry info */
    pos = p->info_count + 3;
    mpp_trie_add_info(trie, "__name__", p->name, p->name_len);
    mpp_trie_add_info(trie, "__info__", &pos, sizeof(pos));
    /* NOTE: node count need to be update after shrinking */
    mpp_trie_add_info(trie, "__node__", &p->node_used, sizeof(p->node_used));

    root = p->nodes;
    node_count = p->node_used;
    node_valid = node_count;

    trie_dbg_last("shrink trie node start node %d info %d\n", node_count, p->info_count);

    if (mpp_trie_debug & MPP_TRIE_DBG_LAST_STEP)
        mpp_trie_dump_f(trie);

    for (i = node_count - 1; i > 0; i--) {
        MppTrieNode *prev;
        rk_s32 prev_idx;

        node = &root[i];
        prev_idx = node->prev;
        prev = &root[prev_idx];

        if (prev->next_cnt > 1) {
            trie_dbg_last("node %d:%d prev %d next count %d stop shrinking for multi next\n",
                          i, node->id, prev_idx, prev->next_cnt);
            continue;
        }

        if (node->tag_len >= (rk_s16)MPP_TRIE_TAG_LEN_MAX) {
            trie_dbg_last("node %d:%d tag %d - %016llx stop shrinking for max tag len\n",
                          i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        if (prev->id >= 0) {
            trie_dbg_last("node %d:%d tag %d - %016llx stop shrinking for valid info node\n",
                          i, node->id, node->tag_len, node->tag_val);
            continue;
        }

        prev->id = node->id;
        /* NOTE: do NOT increase tag length on root node */
        prev->tag_len = node->tag_len + 1;
        prev->tag_val = ((rk_u64)node->key << (node->tag_len * 4)) | node->tag_val;
        prev->next_cnt = node->next_cnt;
        memcpy(prev->next, node->next, sizeof(node->next));

        trie_dbg_last("node %d:%d shrink prev %d key %x tag %016llx -> %016llx\n",
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

    trie_dbg_last("shrink trie node finish count %d -> %d\n", node_count, node_valid);

    if (mpp_trie_debug & MPP_TRIE_DBG_LAST_STEP)
        mpp_trie_dump_f(trie);

    if (mpp_trie_debug & MPP_TRIE_DBG_LAST_CHECK)
        mpp_trie_check(trie, "shrink merge tag stage");

    trie_dbg_last("move trie node start to reduce memory %d -> %d\n", node_count, node_valid);

    for (i = 1; i < node_valid; i++) {
        node = &root[i];

        /* skip valid node */
        if (node->idx)
            continue;

        for (j = i; j < node_count; j++) {
            MppTrieNode *tmp = &root[j];
            MppTrieNode *prev;
            rk_s32 k;

            /* skip empty node */
            if (!tmp->idx)
                continue;

            trie_dbg_last("move node %d to %d prev %d\n", j, i, tmp->prev);

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

    trie_dbg_last("move trie node finish used %d\n", p->node_used);

    if (mpp_trie_debug & MPP_TRIE_DBG_LAST_STEP)
        mpp_trie_dump_f(trie);

    if (mpp_trie_debug & MPP_TRIE_DBG_LAST_CHECK)
        mpp_trie_check(trie, "shrink move node stage");

    trie_dbg_last("create user buffer start\n");

    nodes_size = sizeof(MppTrieNode) * p->node_used;
    p->buf_size = nodes_size + p->info_buf_pos;
    p->buf_size = MPP_ALIGN(p->buf_size, sizeof(void *));

    buf = mpp_calloc(char, p->buf_size);
    if (!buf) {
        mpp_loge("failed to alloc trie buffer size %d\n", p->buf_size);
        return rk_nok;
    }

    p->nodes = (MppTrieNode *)buf;
    memcpy(p->nodes, root, nodes_size);
    pos = nodes_size;
    len = 0;

    for (i = 0; i < p->info_count; i++) {
        MppTrieInfo *src = (MppTrieInfo *)(p->info_buf + len);
        MppTrieInfo *dst = (MppTrieInfo *)(buf + pos);
        rk_s32 info_size = sizeof(MppTrieInfo) + src->str_len + src->ctx_len;
        const char *name = (char *)(src + 1);

        node = mpp_trie_get_node(p->nodes, name);
        node->id = pos;

        memcpy(dst, src, info_size);
        pos += info_size;
        len += info_size;
    }

    MPP_FREE(root);
    MPP_FREE(p->info_buf);

    /* NOTE: udpate final shrinked node count */
    {
        MppTrieInfo *info = mpp_trie_get_info_from_root(p->nodes, "__node__");

        if (info)
            *(rk_u32 *)mpp_trie_info_ctx(info) = p->node_used;
    }

    return rk_ok;
}

rk_s32 mpp_trie_add_info(MppTrie trie, const char *name, void *ctx, rk_u32 ctx_len)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    rk_s32 info_size;
    rk_u32 ctx_real;
    rk_s32 str_real;
    rk_s32 str_len;
    rk_s32 idx;

    if (!p) {
        mpp_loge_f("invalid trie %p name %s ctx %p\n", p, name, ctx);
        return rk_nok;
    }

    if (!name)
        return mpp_trie_last_info(p);

    str_real = strnlen(name, MPP_TRIE_NAME_MAX) + 1;
    str_len = MPP_ALIGN(str_real, 4);
    /* NOTE: align all ctx_len to four bytes */
    ctx_real = ctx_len;
    ctx_len = MPP_ALIGN(ctx_real, 4);
    info_size = sizeof(MppTrieInfo) + str_len + ctx_len;

    if (str_len >= MPP_TRIE_NAME_MAX) {
        mpp_loge_f("invalid trie name %s len %d larger than max %d\n",
                   name, str_len, MPP_TRIE_NAME_MAX);
        return rk_nok;
    }

    if (trie_prepare_buf(p, info_size))
        return rk_nok;

    idx = trie_pave_node(p, name, str_real);
    if (idx < 0) {
        mpp_loge_f("trie %p pave node %s failed\n", p, name);
        return rk_nok;
    }
    if (p->nodes[idx].id != -1) {
        mpp_loge_f("trie %p add info %s already exist\n", p, name);
        return rk_nok;
    }

    p->nodes[idx].id = p->info_buf_pos;
    if (p->info_name_max < str_real - 1)
        p->info_name_max = str_real - 1;

    {
        MppTrieInfo *info = (MppTrieInfo *)(p->info_buf + p->info_buf_pos);
        char *buf = (char *)(info + 1);

        info->index = p->info_count;
        info->ctx_len = ctx_len;
        info->str_len = str_len;

        memcpy(buf, name, str_real);
        while (str_real < str_len)
            buf[str_real++] = 0;

        buf += str_len;

        memcpy(buf, ctx, ctx_real);
        while (ctx_real < ctx_len)
            buf[ctx_real++] = 0;
    }

    trie_dbg_set("trie %p add info %d - %s at node %d pos %d ctx %p size %d\n",
                 p, p->info_count, name, idx, p->info_buf_pos, ctx, ctx_len);

    p->info_buf_pos += info_size;
    p->info_count++;

    return rk_ok;
}

rk_s32 mpp_trie_get_node_count(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->node_used : 0;
}

rk_s32 mpp_trie_get_info_count(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->info_count : 0;
}

rk_s32 mpp_trie_get_buf_size(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->buf_size : 0;
}

rk_s32 mpp_trie_get_name_max(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->info_name_max : 0;
}

void *mpp_trie_get_node_root(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? p->nodes : NULL;
}

MppTrieInfo *mpp_trie_get_info(MppTrie trie, const char *name)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    MppTrieNode *node;

    if (!trie || !name) {
        mpp_err_f("invalid trie %p name %p\n", trie, name);
        return NULL;
    }

    node = mpp_trie_get_node(p->nodes, name);
    if (!node || node->id < 0)
        return NULL;

    return (MppTrieInfo *)(((char *)p->nodes) + node->id);
}

MppTrieInfo *mpp_trie_get_info_first(MppTrie trie)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    return (p) ? (MppTrieInfo *)(((char *)p->nodes) + p->node_used * sizeof(MppTrieNode)) : NULL;
}

MppTrieInfo *mpp_trie_get_info_next(MppTrie trie, MppTrieInfo *info)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;

    if (!p || !info || info->index >= p->info_count - 1)
        return NULL;

    return (MppTrieInfo *)((char *)(info + 1) + info->str_len + info->ctx_len);
}

MppTrieInfo *mpp_trie_get_info_from_root(void *root, const char *name)
{
    MppTrieNode *node;

    if (!root || !name) {
        mpp_loge_f("invalid root %p name %p\n", root, name);
        return NULL;
    }

    mpp_env_get_u32("mpp_trie_debug", &mpp_trie_debug, 0);

    node = mpp_trie_get_node((MppTrieNode *)root, name);
    if (!node || node->id < 0)
        return NULL;

    return (MppTrieInfo *)(((char *)root) + node->id);
}

void mpp_trie_dump(MppTrie trie, const char *func)
{
    MppTrieImpl *p = (MppTrieImpl *)trie;
    char *base = p->info_buf ? (char *)p->info_buf : (char *)p->nodes;
    rk_s32 i;
    rk_s32 next_cnt[17];
    rk_s32 tag_len[17];

    memset(next_cnt, 0, sizeof(next_cnt));
    memset(tag_len, 0, sizeof(tag_len));

    mpp_logi("%s dumping trie %p\n", func, trie);
    mpp_logi("name %s size %d node %d info %d\n",
             p->name, p->buf_size, p->node_used, p->info_count);

    for (i = 0; i < p->node_used; i++) {
        MppTrieNode *node = &p->nodes[i];
        rk_s32 valid_count = 0;
        rk_s32 j;

        if (i && !node->idx)
            continue;

        if (node->id >= 0) {
            MppTrieInfo *info = (MppTrieInfo *)((char *)base + node->id);

            /* check before and after last info */
            if (node->id < (rk_s32)(p->node_used * sizeof(MppTrieNode)))
                mpp_logi("node %d key %x info %d - %s\n", node->idx, node->key, node->id,
                         mpp_trie_info_name(info));
            else
                mpp_logi("node %d key %x info %d - %s\n", node->idx, node->key, node->id,
                         mpp_trie_info_name(info));
        } else
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

void mpp_trie_timing_test(MppTrie trie)
{
    MppTrieInfo *root = mpp_trie_get_info_first(trie);
    MppTrieInfo *info = root;
    rk_s64 sum = 0;
    rk_s32 loop = 0;

    do {
        rk_s64 start, end;

        start = mpp_time();
        mpp_trie_get_info(trie, mpp_trie_info_name(info));
        end = mpp_time();

        info = mpp_trie_get_info_next(trie, info);
        sum += end - start;
        loop++;
    } while (info);

    mpp_logi("trie access %d info %lld us averga %lld us\n",
             loop, sum, sum / loop);
}
