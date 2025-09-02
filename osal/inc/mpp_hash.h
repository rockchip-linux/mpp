/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_HASH_H__
#define __MPP_HASH_H__

#include <stdbool.h>

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GOLDEN_RATIO_32 0x61C88647
#define GOLDEN_RATIO_64 0x61C8864680B583EBull

#if __SIZEOF_POINTER__ == 4
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_32
#define hash_long(val, bits) hash_32(val, bits)
#elif __SIZEOF_POINTER__ == 8
#define GOLDEN_RATIO_PRIME GOLDEN_RATIO_64
#define hash_long(val, bits) hash_64(val, bits)
#else
#error __SIZEOF_POINTER__ not 4 or 8
#endif

struct hlist_node {
    struct hlist_node *next, **pprev;
};

struct hlist_head {
    struct hlist_node *first;
};

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)

#define LIST_POISON1  ((void *) 0x100)
#define LIST_POISON2  ((void *) 0x200)

#define WRITE_ONCE(var, val) \
    (*((volatile typeof(val) *)(&(var))) = (val))

#define READ_ONCE(var) (*((volatile typeof(var) *)(&(var))))

static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
    h->next = NULL;
    h->pprev = NULL;
}

static inline int hlist_unhashed(const struct hlist_node *h)
{
    return !h->pprev;
}

static inline int hlist_empty(const struct hlist_head *h)
{
    return !READ_ONCE(h->first);
}

static inline void __hlist_del(struct hlist_node *n)
{
    struct hlist_node *next = n->next;
    struct hlist_node **pprev = n->pprev;

    WRITE_ONCE(*pprev, next);
    if (next)
        next->pprev = pprev;
}

static inline void hlist_del(struct hlist_node *n)
{
    __hlist_del(n);
    n->next = (struct hlist_node*)LIST_POISON1;
    n->pprev = (struct hlist_node**)LIST_POISON2;
}

static inline void hlist_del_init(struct hlist_node *n)
{
    if (!hlist_unhashed(n)) {
        __hlist_del(n);
        INIT_HLIST_NODE(n);
    }
}

static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
    struct hlist_node *first = h->first;
    n->next = first;
    if (first)
        first->pprev = &n->next;
    WRITE_ONCE(h->first, n);
    n->pprev = &h->first;
}

static inline void hlist_add_before(struct hlist_node *n, struct hlist_node *next)
{
    n->pprev = next->pprev;
    n->next = next;
    next->pprev = &n->next;
    WRITE_ONCE(*(n->pprev), n);
}

static inline void hlist_add_behind(struct hlist_node *n, struct hlist_node *prev)
{
    n->next = prev->next;
    WRITE_ONCE(prev->next, n);
    n->pprev = &prev->next;

    if (n->next)
        n->next->pprev  = &n->next;
}

static inline void hlist_add_fake(struct hlist_node *n)
{
    n->pprev = &n->next;
}

static inline int hlist_fake(struct hlist_node *h)
{
    return h->pprev == &h->next;
}

static inline int
hlist_is_singular_node(struct hlist_node *n, struct hlist_head *h)
{
    return !n->next && n->pprev == &h->first;
}

static inline void hlist_move_list(struct hlist_head *old,
                                   struct hlist_head *_new)
{
    _new->first = old->first;
    if (_new->first)
        _new->first->pprev = &_new->first;
    old->first = NULL;
}

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
    for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_for_each_safe(pos, n, head) \
    for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
         pos = n)

#define hlist_entry_safe(ptr, type, member) \
    ({ typeof(ptr) ____ptr = (ptr); \
       ____ptr ? hlist_entry(____ptr, type, member) : NULL; \
    })

#define hlist_for_each_entry(pos, head, member)             \
    for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);\
         pos;                           \
         pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlist_for_each_entry_continue(pos, member)          \
    for (pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member);\
         pos;                           \
         pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlist_for_each_entry_from(pos, member)              \
    for (; pos;                         \
         pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlist_for_each_entry_safe(pos, n, head, member)         \
    for (pos = hlist_entry_safe((head)->first, typeof(*pos), member);\
         pos && ({ n = pos->member.next; 1; });         \
         pos = hlist_entry_safe(n, typeof(*pos), member))

#define DEFINE_HASHTABLE(name, bits) \
    struct hlist_head name[1 << (bits)] = \
            { [0 ... ((1 << (bits)) - 1)] = HLIST_HEAD_INIT }

#define DECLARE_HASHTABLE(name, bits) \
    struct hlist_head name[1 << (bits)]

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

#define ilog2(n) \
    ( \
        (n) & (1ULL << 63) ? 63 : \
        (n) & (1ULL << 62) ? 62 : \
        (n) & (1ULL << 61) ? 61 : \
        (n) & (1ULL << 60) ? 60 : \
        (n) & (1ULL << 59) ? 59 : \
        (n) & (1ULL << 58) ? 58 : \
        (n) & (1ULL << 57) ? 57 : \
        (n) & (1ULL << 56) ? 56 : \
        (n) & (1ULL << 55) ? 55 : \
        (n) & (1ULL << 54) ? 54 : \
        (n) & (1ULL << 53) ? 53 : \
        (n) & (1ULL << 52) ? 52 : \
        (n) & (1ULL << 51) ? 51 : \
        (n) & (1ULL << 50) ? 50 : \
        (n) & (1ULL << 49) ? 49 : \
        (n) & (1ULL << 48) ? 48 : \
        (n) & (1ULL << 47) ? 47 : \
        (n) & (1ULL << 46) ? 46 : \
        (n) & (1ULL << 45) ? 45 : \
        (n) & (1ULL << 44) ? 44 : \
        (n) & (1ULL << 43) ? 43 : \
        (n) & (1ULL << 42) ? 42 : \
        (n) & (1ULL << 41) ? 41 : \
        (n) & (1ULL << 40) ? 40 : \
        (n) & (1ULL << 39) ? 39 : \
        (n) & (1ULL << 38) ? 38 : \
        (n) & (1ULL << 37) ? 37 : \
        (n) & (1ULL << 36) ? 36 : \
        (n) & (1ULL << 35) ? 35 : \
        (n) & (1ULL << 34) ? 34 : \
        (n) & (1ULL << 33) ? 33 : \
        (n) & (1ULL << 32) ? 32 : \
        (n) & (1ULL << 31) ? 31 : \
        (n) & (1ULL << 30) ? 30 : \
        (n) & (1ULL << 29) ? 29 : \
        (n) & (1ULL << 28) ? 28 : \
        (n) & (1ULL << 27) ? 27 : \
        (n) & (1ULL << 26) ? 26 : \
        (n) & (1ULL << 25) ? 25 : \
        (n) & (1ULL << 24) ? 24 : \
        (n) & (1ULL << 23) ? 23 : \
        (n) & (1ULL << 22) ? 22 : \
        (n) & (1ULL << 21) ? 21 : \
        (n) & (1ULL << 20) ? 20 : \
        (n) & (1ULL << 19) ? 19 : \
        (n) & (1ULL << 18) ? 18 : \
        (n) & (1ULL << 17) ? 17 : \
        (n) & (1ULL << 16) ? 16 : \
        (n) & (1ULL << 15) ? 15 : \
        (n) & (1ULL << 14) ? 14 : \
        (n) & (1ULL << 13) ? 13 : \
        (n) & (1ULL << 12) ? 12 : \
        (n) & (1ULL << 11) ? 11 : \
        (n) & (1ULL << 10) ? 10 : \
        (n) & (1ULL << 9) ? 9 : \
        (n) & (1ULL << 8) ? 8 : \
        (n) & (1ULL << 7) ? 7 : \
        (n) & (1ULL << 6) ? 6 : \
        (n) & (1ULL << 5) ? 5 : \
        (n) & (1ULL << 4) ? 4 : \
        (n) & (1ULL << 3) ? 3 : \
        (n) & (1ULL << 2) ? 2 : \
        (n) & (1ULL << 1) ? 1 : 0 \
    )

#define HASH_SIZE(name) (ARRAY_SIZE(name))
#define HASH_BITS(name) ilog2(HASH_SIZE(name))

/* Use hash_32 when possible to allow for fast 32bit hashing in 64bit kernels. */
#define hash_min(val, bits) \
    (sizeof(val) <= 4 ? hash_32(val, bits) : hash_long(val, bits))

#define hash_add(hashtable, node, key) \
    hlist_add_head(node, &hashtable[hash_min(key, HASH_BITS(hashtable))])

/**
 * hash_empty - check whether a hashtable is empty
 * @hashtable: hashtable to check
 *
 * This has to be a macro since HASH_BITS() will not work on pointers since
 * it calculates the size during preprocessing.
 */
#define hash_empty(hashtable) __hash_empty(hashtable, HASH_SIZE(hashtable))

/**
 * hash_for_each - iterate over a hashtable
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each(name, bkt, obj, member)                           \
    for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name); \
         (bkt)++)                                                       \
    hlist_for_each_entry(obj, &name[bkt], member)

/**
 * hash_for_each_safe - iterate over a hashtable safe against removal of
 * hash entry
 * @name: hashtable to iterate
 * @bkt: integer to use as bucket loop cursor
 * @tmp: a &struct used for temporary storage
 * @obj: the type * to use as a loop cursor for each entry
 * @member: the name of the hlist_node within the struct
 */
#define hash_for_each_safe(name, bkt, tmp, obj, member)                 \
    for ((bkt) = 0, obj = NULL; obj == NULL && (bkt) < HASH_SIZE(name); \
         (bkt)++)                                                       \
    hlist_for_each_entry_safe(obj, tmp, &name[bkt], member)

#define hash_for_each_possible(name, obj, member, key) \
    hlist_for_each_entry(obj, &name[hash_min(key, HASH_BITS(name))], member)

static inline RK_U32 hash_32(RK_U32 val, unsigned int bits)
{
    /* On some cpus multiply is faster, on others gcc will do shifts */
    RK_U32 hash = val * GOLDEN_RATIO_32;

    /* High bits are more random, so use them. */
    return hash >> (32 - bits);
}

static inline RK_U32 __hash_32(RK_U32 val)
{
    return val * GOLDEN_RATIO_32;
}

static inline RK_U32 hash_64(RK_U64 val, unsigned int bits)
{
#if __SIZEOF_POINTER__ == 8
    /* 64x64-bit multiply is efficient on all 64-bit processors */
    return val * GOLDEN_RATIO_64 >> (64 - bits);
#else
    /* Hash 64 bits using only 32x32-bit multiply. */
    return hash_32((RK_U32)val ^ ((val >> 32) * GOLDEN_RATIO_32), bits);
#endif
}

static inline RK_U32 hash_ptr(const void *ptr, unsigned int bits)
{
    return hash_long((unsigned long)ptr, bits);
}

/* This really should be called fold32_ptr; it does no hashing to speak of. */
static inline RK_U32 hash32_ptr(const void *ptr)
{
    unsigned long val = (unsigned long)ptr;

#if __SIZEOF_POINTER__ == 8
    val ^= (val >> 32);
#endif
    return (RK_U32)val;
}

/**
 * hash_hashed - check whether an object is in any hashtable
 * @node: the &struct hlist_node of the object to be checked
 */
static inline bool hash_hashed(struct hlist_node *node)
{
    return !hlist_unhashed(node);
}

static inline bool __hash_empty(struct hlist_head *ht, unsigned int sz)
{
    unsigned int i;

    for (i = 0; i < sz; i++)
        if (!hlist_empty(&ht[i]))
            return false;

    return true;
}

/**
 * hash_del - remove an object from a hashtable
 * @node: &struct hlist_node of the object to remove
 */
static inline void hash_del(struct hlist_node *node)
{
    hlist_del_init(node);
}

#ifdef __cplusplus
}
#endif

#endif /*__MPP_HASH_H__*/
