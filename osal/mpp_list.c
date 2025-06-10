/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_list"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#define LIST_DEBUG(fmt, ...) mpp_log(fmt, ## __VA_ARGS__)
#define LIST_ERROR(fmt, ...) mpp_err(fmt, ## __VA_ARGS__)

static inline void list_node_init(MppListNode *node)
{
    node->prev = node->next = node;
}

static inline void list_node_init_with_key_and_size(MppListNode *node, rk_u32 key, rk_s32 size)
{
    list_node_init(node);
    node->key   = key;
    node->size  = size;
}

static MppListNode* create_list(void *data, rk_s32 size, rk_u32 key)
{
    MppListNode *node = mpp_malloc_size(MppListNode, sizeof(MppListNode) + size);

    if (node) {
        void *dst = (void*)(node + 1);
        list_node_init_with_key_and_size(node, key, size);
        memcpy(dst, data, size);
    } else {
        LIST_ERROR("failed to allocate list node");
    }
    return node;
}

static inline void _mpp_list_add(MppListNode * _new, MppListNode * prev, MppListNode * next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void mpp_list_add(MppListNode *_new, MppListNode *head)
{
    _mpp_list_add(_new, head, head->next);
}

static inline void mpp_list_add_tail(MppListNode *_new, MppListNode *head)
{
    _mpp_list_add(_new, head->prev, head);
}

int mpp_list_add_at_head(MppList *list, void *data, int size)
{
    rk_s32 ret = -EINVAL;

    if (list->head) {
        MppListNode *node = create_list(data, size, 0);
        if (node) {
            mpp_list_add(node, list->head);
            list->count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

int mpp_list_add_at_tail(MppList *list, void *data, int size)
{
    rk_s32 ret = -EINVAL;

    if (list->head) {
        MppListNode *node = create_list(data, size, 0);

        if (node) {
            mpp_list_add_tail(node, list->head);
            list->count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

static void release_list(MppListNode*node, void *data, rk_s32 size)
{
    void *src = (void*)(node + 1);

    if (node->size == size) {
        if (data)
            memcpy(data, src, size);
    } else {
        LIST_ERROR("node size check failed when release_list");
        size = (size < node->size) ? (size) : (node->size);
        if (data)
            memcpy(data, src, size);
    }
    mpp_free(node);
}

static inline void _mpp_list_del(MppListNode *prev, MppListNode *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void mpp_list_del_init(MppListNode *node)
{
    _mpp_list_del(node->prev, node->next);
    list_node_init(node);
}

static inline void _list_del_node_no_lock(MppListNode *node, void *data, rk_s32 size)
{
    mpp_list_del_init(node);
    release_list(node, data, size);
}

int mpp_list_del_at_head(MppList *list, void *data, int size)
{
    rk_s32 ret = -EINVAL;

    if (list->head && list->count) {
        _list_del_node_no_lock(list->head->next, data, size);
        list->count--;
        ret = 0;
    }
    return ret;
}

int mpp_list_del_at_tail(MppList *list, void *data, int size)
{
    rk_s32 ret = -EINVAL;

    if (list->head && list->count) {
        _list_del_node_no_lock(list->head->prev, data, size);
        list->count--;
        ret = 0;
    }
    return ret;
}
static MppListNode* create_list_with_size(void *data, rk_s32 size, rk_u32 key)
{
    MppListNode *node = mpp_malloc_size(MppListNode, sizeof(MppListNode) + sizeof(size) + size);

    if (node) {
        rk_s32 *dst = (rk_s32 *)(node + 1);
        list_node_init_with_key_and_size(node, key, size);
        *dst++ = size;
        memcpy(dst, data, size);
    } else {
        LIST_ERROR("failed to allocate list node");
    }
    return node;
}

rk_s32 mpp_list_fifo_wr(MppList *list, void *data, rk_s32 size)
{
    rk_s32 ret = -EINVAL;

    if (list && list->head) {
        MppListNode *node = create_list_with_size(data, size, 0);

        if (node) {
            mpp_list_add_tail(node, list->head);
            list->count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

static void release_list_with_size(MppListNode* node, void *data, rk_s32 *size)
{
    rk_s32 *src = (rk_s32*)(node + 1);
    rk_s32 data_size = *src++;

    *size = data_size;

    if (data)
        memcpy(data, src, data_size);

    mpp_free(node);
}

rk_s32 mpp_list_fifo_rd(MppList *list, void *data, rk_s32 *size)
{
    rk_s32 ret = -EINVAL;

    if (list && list->head && list->count) {
        MppListNode *node = list->head->next;

        mpp_list_del_init(node);
        release_list_with_size(node, data, size);
        list->count--;
        ret = 0;
    }
    return ret;
}

int mpp_list_is_empty(MppList *list)
{
    return list->count == 0;
}

int mpp_list_size(MppList *list)
{
    return list->count;
}

rk_s32 mpp_list_add_by_key(MppList *list, void *data, rk_s32 size, rk_u32 *key)
{
    rk_s32 ret = 0;

    if (list->head) {
        MppListNode *node;
        rk_u32 list_key = mpp_list_get_key(list);

        *key = list_key;
        node = create_list(data, size, list_key);
        if (node) {
            mpp_list_add_tail(node, list->head);
            list->count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

rk_s32 mpp_list_del_by_key(MppList *list, void *data, rk_s32 size, rk_u32 key)
{
    rk_s32 ret = 0;

    if (list && list->head && list->count) {
        MppListNode *tmp = list->head->next;

        ret = -EINVAL;
        while (tmp->next != list->head) {
            if (tmp->key == key) {
                _list_del_node_no_lock(tmp, data, size);
                list->count--;
                break;
            }
            tmp = tmp->next;
        }
    }
    return ret;
}


rk_s32 mpp_list_show_by_key(MppList *list, void *data, rk_u32 key)
{
    rk_s32 ret = -EINVAL;

    (void)list;
    (void)data;
    (void)key;
    return ret;
}

void mpp_list_flush(MppList* list)
{
    if (list->head) {
        while (list->count) {
            MppListNode* node = list->head->next;

            mpp_list_del_init(node);

            if (list->destroy) {
                list->destroy((void*)(node + 1));
            }

            mpp_free(node);
            list->count--;
        }
    }

    mpp_list_signal(list);
}

MPP_RET mpp_list_wait(MppList* list)
{
    int ret;

    ret = mpp_mutex_cond_wait(&list->cond_lock);

    if (ret == 0) {
        return MPP_OK;
    } else if (ret == ETIMEDOUT) {
        return MPP_NOK;
    } else {
        return MPP_NOK;
    }
}

MPP_RET mpp_list_wait_timed(MppList *list, rk_s64 timeout)
{
    int ret;

    ret = (MPP_RET)mpp_mutex_cond_timedwait(&list->cond_lock, timeout);

    if (ret == 0) {
        return MPP_OK;
    } else if (ret == ETIMEDOUT) {
        return MPP_NOK;
    } else {
        return MPP_NOK;
    }
}

MPP_RET mpp_list_wait_lt(MppList *list, rk_s64 timeout, rk_s32 val)
{
    if (list->count < val)
        return MPP_OK;

    if (timeout < 0) {
        return mpp_list_wait(list);
    } else {
        return mpp_list_wait_timed(list, timeout);
    }
}

MPP_RET mpp_list_wait_le(MppList *list, rk_s64 timeout, rk_s32 val)
{
    if (list->count <= val)
        return MPP_OK;

    if (timeout < 0) {
        return mpp_list_wait(list);
    } else {
        return mpp_list_wait_timed(list, timeout);
    }
}

MPP_RET mpp_list_wait_gt(MppList *list, rk_s64 timeout, rk_s32 val)
{
    if (list->count > val)
        return MPP_OK;

    if (timeout < 0) {
        return mpp_list_wait(list);
    } else {
        return mpp_list_wait_timed(list, timeout);
    }
}

MPP_RET mpp_list_wait_ge(MppList *list, rk_s64 timeout, rk_s32 val)
{
    if (list->count >= val)
        return MPP_OK;

    if (timeout < 0) {
        return mpp_list_wait(list);
    } else {
        return mpp_list_wait_timed(list, timeout);
    }
}

void mpp_list_signal(MppList *list)
{
    mpp_mutex_cond_signal(&list->cond_lock);
}

rk_u32 mpp_list_get_key(MppList *list)
{
    return list->keys++;
}

MppList *mpp_list_create(node_destructor func)
{
    MppList *list = mpp_malloc(MppList, 1);

    if (list == NULL) {
        LIST_ERROR("Failed to allocate memory for mpp_list.\n");
        return NULL;
    }

    list->destroy = func;
    list->count = 0;

    list->head = mpp_malloc(MppListNode, 1);
    if (list->head == NULL) {
        LIST_ERROR("Failed to allocate memory for list header.\n");
        mpp_free(list);
        return NULL;
    }

    list_node_init_with_key_and_size(list->head, 0, 0);

    mpp_mutex_cond_init(&list->cond_lock);

    return list;
}

void mpp_list_destroy(MppList *list)
{
    MppListNode *node;

    if (!list)
        return;

    mpp_list_flush(list);

    node = list->head->next;
    while (node != list->head) {
        MppListNode *next = node->next;

        mpp_free(node);
        node = next;
    }

    mpp_mutex_cond_destroy(&list->cond_lock);

    mpp_free(list->head);
    list->head = NULL;

    mpp_free(list);
    list = NULL;
}

/* list sort porting from kernel list_sort.c */

/*
 * Returns a list organized in an intermediate format suited
 * to chaining of merge() calls: null-terminated, no reserved or
 * sentinel head node, "prev" links not maintained.
 */
static struct list_head *merge(void *priv, ListCmpFunc cmp,
                               struct list_head *a, struct list_head *b)
{
    struct list_head *head, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

/*
 * Combine final list merge with restoration of standard doubly-linked
 * list structure.  This approach duplicates code from merge(), but
 * runs faster than the tidier alternatives of either a separate final
 * prev-link restoration pass, or maintaining the prev links
 * throughout.
 */
static void merge_final(void *priv, ListCmpFunc cmp, struct list_head *head,
                        struct list_head *a, struct list_head *b)
{
    struct list_head *tail = head;
    rk_u8 count = 0;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        /*
         * If the merge is highly unbalanced (e.g. the input is
         * already sorted), this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        if (!++count)
            cmp(priv, b, b);
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}

void list_sort(void *priv, struct list_head *head, ListCmpFunc cmp)
{
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0;   /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    /*
     * Data structure invariants:
     * - All lists are singly linked and null-terminated; prev
     *   pointers are not maintained.
     * - pending is a prev-linked "list of lists" of sorted
     *   sublists awaiting further merging.
     * - Each of the sorted sublists is power-of-two in size.
     * - Sublists are sorted by size and age, smallest & newest at front.
     * - There are zero to two sublists of each size.
     * - A pair of pending sublists are merged as soon as the number
     *   of following pending elements equals their size (i.e.
     *   each time count reaches an odd multiple of that size).
     *   That ensures each later final merge will be at worst 2:1.
     * - Each round consists of:
     *   - Merging the two sublists selected by the highest bit
     *     which flips when count is incremented, and
     *   - Adding an element from the input as a size-1 sublist.
     */
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (bits) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(priv, cmp, b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(priv, cmp, pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(priv, cmp, head, pending, list);
}
