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

#define MODULE_TAG "mpp_mem"

#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_common.h"

#include "os_mem.h"

// mpp_mem_flag bit mask
#define OSAL_MEM_LIST_EN        (0x00000001)
#define OSAL_MEM_RUNTIME_LOG    (0x00000002)

// default memory align size is set to 32
#define RK_OSAL_MEM_ALIGN       32

static RK_U32 mpp_mem_flag  = 0;
static RK_U64 osal_mem_index = 0;
static struct list_head mem_list;
static pthread_mutex_t mem_list_lock;

struct mem_node {
    struct list_head list;
    void *ptr;
    size_t size;
    RK_U64 index;

    /* memory node extra information */
    char tag[MPP_TAG_SIZE];
};

static void get_osal_mem_flag()
{
    static RK_U32 once = 1;
    if (once) {
        mpp_env_get_u32("mpp_mem_flag", &mpp_mem_flag, 0);

        INIT_LIST_HEAD(&mem_list);

        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&mem_list_lock, &attr);
        pthread_mutexattr_destroy(&attr);
        once = 0;
    }
}

void *mpp_osal_malloc(const char *tag, size_t size)
{
    void *ptr;

    get_osal_mem_flag();

    os_malloc(&ptr, RK_OSAL_MEM_ALIGN, size);

    if (mpp_mem_flag & OSAL_MEM_RUNTIME_LOG)
        mpp_log("mpp_malloc  tag %-16s size %-8u ret %p\n", tag, size, ptr);

    if ((mpp_mem_flag & OSAL_MEM_LIST_EN) && ptr) {
        struct mem_node *node = (struct mem_node *)malloc(sizeof(struct mem_node));
        mpp_assert(node);
        INIT_LIST_HEAD(&node->list);
        node->ptr   = ptr;
        node->size  = size;
        snprintf(node->tag, sizeof(node->tag), "%s", tag);

        pthread_mutex_lock(&mem_list_lock);
        node->index = osal_mem_index++;
        list_add_tail(&node->list, &mem_list);
        pthread_mutex_unlock(&mem_list_lock);
    }

    return ptr;
}

void *mpp_osal_calloc(const char *tag, size_t size)
{
    void *ptr = mpp_osal_malloc(tag, size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void *mpp_osal_realloc(const char *tag, void *ptr, size_t size)
{
    void *ret;

    if (NULL == ptr)
        return mpp_osal_malloc(tag, size);

    if (0 == size)
        return NULL;

    get_osal_mem_flag();

    if (mpp_mem_flag & OSAL_MEM_LIST_EN) {
        struct mem_node *pos, *n;

        ret = NULL;
        pthread_mutex_lock(&mem_list_lock);
        list_for_each_entry_safe(pos, n, &mem_list, struct mem_node, list) {
            if (ptr == pos->ptr) {
                if (MPP_OK == os_realloc(ptr, &pos->ptr, RK_OSAL_MEM_ALIGN, size)) {
                    pos->size   = size;
                    strncpy(pos->tag, tag, sizeof(pos->tag));
                    ret = pos->ptr;
                } else {
                    list_del_init(&pos->list);
                    free(pos);
                }
                break;
            }
        }
        pthread_mutex_unlock(&mem_list_lock);
    } else {
        os_realloc(ptr, &ret, RK_OSAL_MEM_ALIGN, size);
    }

    if (mpp_mem_flag & OSAL_MEM_RUNTIME_LOG)
        mpp_log("mpp_realloc tag %-16s size %-8u ptr %p ret %p\n", tag, size, ptr, ret);

    if (NULL == ret)
        mpp_err("mpp_realloc ptr %p to size %d failed\n", ptr, size);

    return ret;
}

void mpp_osal_free(void *ptr)
{
    if (NULL == ptr)
        return;

    get_osal_mem_flag();

    if (mpp_mem_flag & OSAL_MEM_LIST_EN) {
        struct mem_node *pos, *n;
        RK_U32 found_match = 0;

        pthread_mutex_lock(&mem_list_lock);
        list_for_each_entry_safe(pos, n, &mem_list, struct mem_node, list) {
            if (ptr == pos->ptr) {
                list_del_init(&pos->list);
                free(pos);
                found_match = 1;
                break;
            }
        }
        pthread_mutex_unlock(&mem_list_lock);

        if (!found_match)
            mpp_err_f("can not found match on free %p\n", ptr);
    }

    if (mpp_mem_flag & OSAL_MEM_RUNTIME_LOG)
        mpp_log("mpp_free %p\n", ptr);

    os_free(ptr);
}

/*
 * dump memory status
 * this function need MODULE_TAG statistic information
 */
void mpp_show_mem_status()
{
    struct mem_node *pos, *n;

    pthread_mutex_lock(&mem_list_lock);
    list_for_each_entry_safe(pos, n, &mem_list, struct mem_node, list) {
        mpp_log("unfree memory %p size %d tag %s index %llu",
                pos->ptr, pos->size, pos->tag, pos->index);
    }
    pthread_mutex_unlock(&mem_list_lock);
}

typedef struct MppMemSnapshotImpl {
    struct list_head    list;
    RK_U64              total_size;
    RK_U32              total_count;
} MppMemSnapshotImpl;

MPP_RET mpp_mem_get_snapshot(MppMemSnapshot *hnd)
{
    struct mem_node *pos, *n;
    MppMemSnapshotImpl *p = (MppMemSnapshotImpl *)malloc(sizeof(MppMemSnapshotImpl));
    if (!p) {
        mpp_err_f("failed to alloc");
        *hnd = NULL;
        return MPP_NOK;
    }

    INIT_LIST_HEAD(&p->list);
    p->total_size  = 0;
    p->total_count = 0;

    pthread_mutex_lock(&mem_list_lock);
    list_for_each_entry_safe(pos, n, &mem_list, struct mem_node, list) {
        struct mem_node *node = (struct mem_node *)malloc(sizeof(struct mem_node));
        mpp_assert(node);

        memcpy(node, pos, sizeof(*pos));
        INIT_LIST_HEAD(&node->list);
        list_add_tail(&node->list, &p->list);
        p->total_size += pos->size;
        p->total_count++;
    }
    *hnd = p;
    pthread_mutex_unlock(&mem_list_lock);

    return MPP_OK;
}

MPP_RET mpp_mem_put_snapshot(MppMemSnapshot *hnd)
{
    if (hnd && *hnd) {
        MppMemSnapshotImpl *p = (MppMemSnapshotImpl *)*hnd;
        struct mem_node *pos, *n;

        pthread_mutex_lock(&mem_list_lock);
        list_for_each_entry_safe(pos, n, &p->list, struct mem_node, list) {
            list_del_init(&pos->list);
            free(pos);
        }
        free(p);
        *hnd = NULL;
        pthread_mutex_unlock(&mem_list_lock);
    }

    return MPP_OK;
}

MPP_RET mpp_mem_squash_snapshot(MppMemSnapshot hnd0, MppMemSnapshot hnd1)
{
    if (!hnd0 || !hnd1) {
        mpp_err_f("invalid input %p %p\n", hnd0, hnd1);
        return MPP_NOK;
    }
    MppMemSnapshotImpl *p0 = (MppMemSnapshotImpl *)hnd0;
    MppMemSnapshotImpl *p1 = (MppMemSnapshotImpl *)hnd1;
    struct mem_node *pos0, *n0;
    struct mem_node *pos1, *n1;

    mpp_log_f("snapshot0 total count %6d size %d\n", p0->total_count, p0->total_size);
    mpp_log_f("snapshot1 total count %6d size %d\n", p1->total_count, p1->total_size);

    pthread_mutex_lock(&mem_list_lock);
    /* handle 0 search */
    list_for_each_entry_safe(pos0, n0, &p0->list, struct mem_node, list) {
        RK_U32 found_match = 0;

        list_for_each_entry_safe(pos1, n1, &p1->list, struct mem_node, list) {
            if (pos0->index == pos1->index) {
                list_del_init(&pos0->list);
                list_del_init(&pos1->list);
                free(pos0);
                free(pos1);
                found_match = 1;
                break;
            }
        }

        if (!found_match) {
            mpp_log_f("snapshot0 %p found mismatch memory %p size %d tag %s index %llu",
                      p0, pos0->ptr, pos0->size, pos0->tag, pos0->index);
        }
    }

    /* handle 1 search */
    list_for_each_entry_safe(pos1, n1, &p1->list, struct mem_node, list) {
        mpp_log_f("snapshot1 %p found mismatch memory %p size %d tag %s index %llu",
                  p1, pos1->ptr, pos1->size, pos1->tag, pos1->index);
    }
    pthread_mutex_unlock(&mem_list_lock);

    return MPP_OK;
}

