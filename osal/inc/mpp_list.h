/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_LIST_H__
#define __MPP_LIST_H__

#include "rk_type.h"
#include "mpp_err.h"

#include "mpp_thread.h"

/*
 * two list structures are defined here:
 * 1. MppList           : list with key
 * 2. struct list_head  : typical list head
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MppListNode_t MppListNode;
// desctructor of list node
typedef void *(*node_destructor)(void *);

struct MppListNode_t {
    MppListNode     *prev;
    MppListNode     *next;
    rk_u32          key;
    rk_s32          size;
};

typedef struct MppList_t {
    MppListNode     *head;
    int             count;
    node_destructor destroy;
    MppMutexCond    cond_lock;
    rk_u32          keys;
} MppList;

int mpp_list_add_at_head(MppList *list, void *data, int size);
int mpp_list_add_at_tail(MppList *list, void *data, int size);
int mpp_list_del_at_head(MppList *list, void *data, int size);
int mpp_list_del_at_tail(MppList *list, void *data, int size);

rk_s32 mpp_list_fifo_wr(MppList *list, void *data, rk_s32 size);
rk_s32 mpp_list_fifo_rd(MppList *list, void *data, rk_s32 *size);

int mpp_list_is_empty(MppList *list);
int mpp_list_size(MppList *list);

rk_s32 mpp_list_add_by_key(MppList *list, void *data, rk_s32 size, rk_u32 *key);
rk_s32 mpp_list_del_by_key(MppList *list, void *data, rk_s32 size, rk_u32 key);
rk_s32 mpp_list_show_by_key(MppList *list, void *data, rk_u32 key);

void mpp_list_flush(MppList *list);

MPP_RET mpp_list_wait(MppList* list);
MPP_RET mpp_list_wait_timed(MppList *list, rk_s64 timeout);
MPP_RET mpp_list_wait_lt(MppList *list, rk_s64 timeout, rk_s32 val);
MPP_RET mpp_list_wait_le(MppList *list, rk_s64 timeout, rk_s32 val);
MPP_RET mpp_list_wait_gt(MppList *list, rk_s64 timeout, rk_s32 val);
MPP_RET mpp_list_wait_ge(MppList *list, rk_s64 timeout, rk_s32 val);

void mpp_list_signal(MppList *list);
rk_u32 mpp_list_get_key(MppList *list);

MppList *mpp_list_create(node_destructor func);
void mpp_list_destroy(MppList *list);


struct list_head {
    struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
        (ptr)->next = (ptr); (ptr)->prev = (ptr); \
    } while (0)

#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

#define list_entry(ptr, type, member) \
    ((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

#define list_first_entry(ptr, type, member) \
        list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
        list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member) ({ \
        struct list_head *head__ = (ptr); \
        struct list_head *pos__ = head__->next; \
        pos__ != head__ ? list_entry(pos__, type, member) : NULL; \
})

#define list_next_entry(pos, type, member) \
        list_entry((pos)->member.next, type, member)

#define list_prev_entry(pos, type, member) \
        list_entry((pos)->member.prev, type, member)

#define list_for_each_entry(pos, head, type, member) \
    for (pos = list_entry((head)->next, type, member); \
         &pos->member != (head); \
         pos = list_next_entry(pos, type, member))

#define list_for_each_entry_safe(pos, n, head, type, member) \
    for (pos = list_first_entry(head, type, member),  \
         n = list_next_entry(pos, type, member); \
         &pos->member != (head);                    \
         pos = n, n = list_next_entry(n, type, member))

#define list_for_each_entry_reverse(pos, head, type, member) \
    for (pos = list_last_entry(head, type, member); \
         &pos->member != (head); \
         pos = list_prev_entry(pos, type, member))

#define list_for_each_entry_safe_reverse(pos, n, head, type, member) \
    for (pos = list_last_entry(head, type, member),  \
         n = list_prev_entry(pos, type, member); \
         &pos->member != (head);                    \
         pos = n, n = list_prev_entry(n, type, member))

static __inline void __list_add(struct list_head * _new,
                                struct list_head * prev,
                                struct list_head * next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static __inline void list_add(struct list_head *_new, struct list_head *head)
{
    __list_add(_new, head, head->next);
}

static __inline void list_add_tail(struct list_head *_new, struct list_head *head)
{
    __list_add(_new, head->prev, head);
}

static __inline void __list_del(struct list_head * prev, struct list_head * next)
{
    next->prev = prev;
    prev->next = next;
}

static __inline void list_del_init(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);

    INIT_LIST_HEAD(entry);
}

static __inline void list_move(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add(list, head);
}

static __inline void list_move_tail(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add_tail(list, head);
}

static __inline int list_is_last(const struct list_head *list, const struct list_head *head)
{
    return list->next == head;
}

static __inline int list_empty(struct list_head *head)
{
    return head->next == head;
}

typedef rk_s32 (*ListCmpFunc)(void *, const struct list_head *, const struct list_head *);

void list_sort(void *priv, struct list_head *head, ListCmpFunc cmp);

#ifdef __cplusplus
}
#endif


#endif /*__MPP_LIST_H__*/
