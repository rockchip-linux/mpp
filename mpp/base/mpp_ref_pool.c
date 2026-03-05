/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_ref_pool"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_ref_pool.h"

typedef struct MppRefSlot_t {
    void   *ptr;
    RK_U32 size;
    RK_U32 ref_count;
} MppRefSlot;

RK_S32 mpp_ref_pool_init(MppRefPool *pool, RK_U32 capacity)
{
    if (!pool || !capacity)
        return MPP_NOK;

    pool->slots = mpp_calloc(MppRefSlot, capacity);
    if (!pool->slots) {
        mpp_loge_f("failed to alloc pool slots for %u\n", capacity);
        return MPP_NOK;
    }

    pool->count = 0;
    pool->capacity = capacity;

    return MPP_OK;
}

void mpp_ref_pool_deinit(MppRefPool *pool)
{
    MppRefSlot *slots;
    RK_S32 i;

    if (!pool)
        return;

    slots = (MppRefSlot *)pool->slots;
    if (slots) {
        for (i = 0; i < pool->count; i++)
            MPP_FREE(slots[i].ptr);

        MPP_FREE(pool->slots);
    }
}

void mpp_ref_pool_cleanup(MppRefPool *pool)
{
    MppRefSlot *slots;
    RK_S32 i;

    if (!pool || !pool->slots)
        return;

    slots = (MppRefSlot *)pool->slots;
    for (i = 0; i < pool->count; i++) {
        MppRefSlot *slot = &slots[i];

        if (slot->ref_count == 0) {
            MPP_FREE(slot->ptr);
            slot->size = 0;
        }
    }
}

RK_S32 mpp_ref_pool_get(MppRefPool *pool, RK_U32 size)
{
    MppRefSlot *slots;
    RK_S32 i;

    if (!pool)
        return -1;

    slots = (MppRefSlot *)pool->slots;
    for (i = 0; i < pool->count; i++) {
        MppRefSlot *slot = &slots[i];

        if (slot->ref_count == 0) {
            if (slot->ptr && slot->size >= size) {
                slot->ref_count = 1;
                return i;
            }
            if (slot->ptr) {
                MPP_FREE(slot->ptr);
                slot->ptr = NULL;
                slot->size = 0;
            }
            break;
        }
    }

    if (i >= pool->count) {
        if (pool->count >= pool->capacity) {
            RK_U32 new_capacity = pool->capacity * 2;
            MppRefSlot *new_slots;

            new_slots = mpp_realloc(slots, MppRefSlot, new_capacity);
            if (!new_slots) {
                mpp_loge_f("failed to expand pool from %u to %u\n",
                           pool->capacity, new_capacity);
                return -1;
            }

            pool->slots = new_slots;
            slots = new_slots;
            pool->capacity = new_capacity;
        }
        pool->count++;
        i = pool->count - 1;
    }

    slots[i].ptr = mpp_malloc_size(void, size);
    if (!slots[i].ptr) {
        mpp_loge_f("failed to allocate buffer size %u\n", size);
        return -1;
    }

    slots[i].ref_count = 1;
    slots[i].size = size;

    return (RK_S32)i;
}

void mpp_ref_pool_ref(MppRefPool *pool, RK_S32 idx)
{
    MppRefSlot *slots;

    if (!pool || idx < 0 || idx >= pool->count)
        return;

    slots = (MppRefSlot *)pool->slots;
    slots[idx].ref_count++;
}

void mpp_ref_pool_put(MppRefPool *pool, RK_S32 idx)
{
    MppRefSlot *slots;

    if (!pool || idx < 0 || idx >= pool->count)
        return;

    slots = (MppRefSlot *)pool->slots;
    if (slots[idx].ref_count > 0) {
        slots[idx].ref_count--;

        if (slots[idx].ref_count == 0 && pool->count >= pool->capacity) {
            MPP_FREE(slots[idx].ptr);
            slots[idx].size = 0;
        }
    }
}

void *mpp_ref_pool_ptr(MppRefPool *pool, RK_S32 idx)
{
    MppRefSlot *slots;

    if (!pool || idx < 0 || idx >= pool->count)
        return NULL;

    slots = (MppRefSlot *)pool->slots;
    return slots[idx].ptr;
}

RK_U32 mpp_ref_pool_size(MppRefPool *pool, RK_S32 idx)
{
    MppRefSlot *slots;

    if (!pool || idx < 0 || idx >= pool->count)
        return 0;

    slots = (MppRefSlot *)pool->slots;
    return slots[idx].size;
}
