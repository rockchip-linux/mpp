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

#define MODULE_TAG "mpp_list"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "mpp_log.h"
#include "mpp_list.h"


#define LIST_DEBUG(fmt, ...) mpp_log(fmt, ## __VA_ARGS__)
#define LIST_ERROR(fmt, ...) mpp_err(fmt, ## __VA_ARGS__)

RK_U32 mpp_list::keys = 0;

typedef struct mpp_list_node {
    mpp_list_node*  prev;
    mpp_list_node*  next;
    RK_U32          key;
    RK_S32          size;
} mpp_list_node;

static inline void list_node_init(mpp_list_node *node)
{
    node->prev = node->next = node;
}

static inline void list_node_init_with_key_and_size(mpp_list_node *node, RK_U32 key, RK_S32 size)
{
    list_node_init(node);
    node->key   = key;
    node->size  = size;
}

static mpp_list_node* create_list(void *data, RK_S32 size, RK_U32 key)
{
    mpp_list_node *node = (mpp_list_node*)malloc(sizeof(mpp_list_node) + size);
    if (node) {
        void *dst = (void*)(node + 1);
        list_node_init_with_key_and_size(node, key, size);
        memcpy(dst, data, size);
    } else {
        LIST_ERROR("failed to allocate list node");
    }
    return node;
}

static inline void _mpp_list_add(mpp_list_node * _new, mpp_list_node * prev, mpp_list_node * next)
{
    next->prev = _new;
    _new->next = next;
    _new->prev = prev;
    prev->next = _new;
}

static inline void mpp_list_add(mpp_list_node *_new, mpp_list_node *head)
{
    _mpp_list_add(_new, head, head->next);
}

static inline void mpp_list_add_tail(mpp_list_node *_new, mpp_list_node *head)
{
    _mpp_list_add(_new, head->prev, head);
}

RK_S32 mpp_list::add_at_head(void *data, RK_S32 size)
{
    RK_S32 ret = -EINVAL;
    if (head) {
        mpp_list_node *node = create_list(data, size, 0);
        if (node) {
            mpp_list_add(node, head);
            count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

RK_S32 mpp_list::add_at_tail(void *data, RK_S32 size)
{
    RK_S32 ret = -EINVAL;
    if (head) {
        mpp_list_node *node = create_list(data, size, 0);
        if (node) {
            mpp_list_add_tail(node, head);
            count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

static void release_list(mpp_list_node*node, void *data, RK_S32 size)
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
    free(node);
}

static inline void _mpp_list_del(mpp_list_node *prev, mpp_list_node *next)
{
    next->prev = prev;
    prev->next = next;
}

static inline void mpp_list_del_init(mpp_list_node *node)
{
    _mpp_list_del(node->prev, node->next);
    list_node_init(node);
}

static inline int list_is_last(const mpp_list_node *list, const mpp_list_node *head)
{
    return list->next == head;
}

static inline void _list_del_node_no_lock(mpp_list_node *node, void *data, RK_S32 size)
{
    mpp_list_del_init(node);
    release_list(node, data, size);
}

RK_S32 mpp_list::del_at_head(void *data, RK_S32 size)
{
    RK_S32 ret = -EINVAL;
    if (head && count) {
        _list_del_node_no_lock(head->next, data, size);
        count--;
        ret = 0;
    }
    return ret;
}

RK_S32 mpp_list::del_at_tail(void *data, RK_S32 size)
{
    RK_S32 ret = -EINVAL;
    if (head && count) {
        _list_del_node_no_lock(head->prev, data, size);
        count--;
        ret = 0;
    }
    return ret;
}

RK_S32 mpp_list::list_is_empty()
{
    RK_S32 ret = (count == 0);
    return ret;
}

RK_S32 mpp_list::list_size()
{
    RK_S32 ret = count;
    return ret;
}

RK_S32 mpp_list::add_by_key(void *data, RK_S32 size, RK_U32 *key)
{
    RK_S32 ret = 0;
    if (head) {
        RK_U32 list_key = get_key();
        *key = list_key;
        mpp_list_node *node = create_list(data, size, list_key);
        if (node) {
            mpp_list_add_tail(node, head);
            count++;
            ret = 0;
        } else {
            ret = -ENOMEM;
        }
    }
    return ret;
}

RK_S32 mpp_list::del_by_key(void *data, RK_S32 size, RK_U32 key)
{
    RK_S32 ret = 0;
    if (head && count) {
        struct mpp_list_node *tmp = head->next;
        ret = -EINVAL;
        while (tmp->next != head) {
            if (tmp->key == key) {
                _list_del_node_no_lock(tmp, data, size);
                count--;
                break;
            }
        }
    }
    return ret;
}


RK_S32 mpp_list::show_by_key(void *data, RK_U32 key)
{
    RK_S32 ret = -EINVAL;
    (void)data;
    (void)key;
    return ret;
}

RK_S32 mpp_list::flush()
{
    if (head) {
        while (count) {
            mpp_list_node* node = head->next;
            mpp_list_del_init(node);
            if (destroy) {
                destroy((void*)(node + 1));
            }
            free(node);
            count--;
        }
    }
    mCondition.signal();
    return 0;
}

void mpp_list::lock()
{
    mMutex.lock();
}

void mpp_list::unlock()
{
    mMutex.unlock();
}

RK_S32 mpp_list::trylock()
{
    return mMutex.trylock();
}

Mutex *mpp_list::mutex()
{
    return &mMutex;
}

RK_U32 mpp_list::get_key()
{
    return keys++;
}

void mpp_list::wait()
{
    mCondition.wait(mMutex);
}

RK_S32 mpp_list::wait(RK_S64 timeout)
{
    return mCondition.timedwait(mMutex, timeout);
}

void mpp_list::signal()
{
    mCondition.signal();
}

mpp_list::mpp_list(node_destructor func)
    : destroy(NULL),
      head(NULL),
      count(0)
{
    destroy = func;
    head = (mpp_list_node*)malloc(sizeof(mpp_list_node));
    if (NULL == head) {
        LIST_ERROR("failed to allocate list header");
    } else {
        list_node_init_with_key_and_size(head, 0, 0);
    }
}

mpp_list::~mpp_list()
{
    flush();
    if (head) free(head);
    head = NULL;
    destroy = NULL;
}

#if BUILD_RK_LIST_TEST
#include "vpu_mem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOOP_RK_LIST    600

#define COUNT_ADD       100
#define COUNT_DEL       99

volatile int err = 0;

static int mpp_list_fifo_test(mpp_list *list_0)
{
    int count;
    VPUMemLinear_t m;
    for (count = 0; count < COUNT_ADD; count++) {
        err |= VPUMallocLinear(&m, 100);
        if (err) {
            printf("VPUMallocLinear in mpp_list_fifo_test\n");
            break;
        }
        err |= list_0->add_at_head(&m, sizeof(m));
        if (err) {
            printf("add_at_head in mpp_list_fifo_test\n");
            break;
        }
    }

    if (!err) {
        for (count = 0; count < COUNT_DEL; count++) {
            err |= list_0->del_at_tail(&m, sizeof(m));
            if (err) {
                printf("del_at_tail in mpp_list_fifo_test\n");
                break;
            }
            err |= VPUFreeLinear(&m);
            if (err) {
                printf("VPUFreeLinear in mpp_list_fifo_test\n");
                break;
            }
        }
    }
    return err;
}

static int mpp_list_filo_test(mpp_list *list_0)
{
    int count;
    VPUMemLinear_t m;
    for (count = 0; count < COUNT_ADD + COUNT_DEL; count++) {
        if (count & 1) {
            err |= list_0->del_at_head(&m, sizeof(m));
            if (err) {
                printf("del_at_head in mpp_list_filo_test\n");
                break;
            }
            err |= VPUFreeLinear(&m);
            if (err) {
                printf("VPUFreeLinear in mpp_list_fifo_test\n");
                break;
            }
        } else {
            err |= VPUMallocLinear(&m, 100);
            if (err) {
                printf("VPUMallocLinear in mpp_list_filo_test\n");
                break;
            }
            err |= list_0->add_at_head(&m, sizeof(m));
            if (err) {
                printf("add_at_head in mpp_list_fifo_test\n");
                break;
            }
        }
    }

    return err;
}


void *mpp_list_test_loop_0(void *pdata)
{
    int i;
    mpp_list *list_0 = (mpp_list *)pdata;

    printf("mpp_list test 0 loop start\n");
    for (i = 0; i < LOOP_RK_LIST; i++) {
        err |= mpp_list_filo_test(list_0);
        if (err) break;
    }

    if (err) {
        printf("thread: found vpu mem operation err %d\n", err);
    } else {
        printf("thread: test done and found no err\n");
    }
    return NULL;
}

int mpp_list_test_0()
{
    int i, err = 0;
    printf("mpp_list test 0 FIFO start\n");

    mpp_list *list_0 = new mpp_list((node_destructor)VPUFreeLinear);

    pthread_t mThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_create(&mThread, &attr, mpp_list_test_loop_0, (void*)list_0);
    pthread_attr_destroy(&attr);

    for (i = 0; i < LOOP_RK_LIST; i++) {
        err |= mpp_list_fifo_test(list_0);
        if (err) break;
    }
    if (err) {
        printf("main  : found mpp_list operation err %d\n", err);
    } else {
        printf("main  : test done and found no err\n");
    }

    void *dummy;
    pthread_join(mThread, &dummy);

    printf("mpp_list test 0 end size %d\n", list_0->list_size());
    delete list_0;
    return err;
}

#define TOTAL_RK_LIST_TEST_COUNT    1

typedef int (*RK_LIST_TEST_FUNC)(void);
RK_LIST_TEST_FUNC test_func[TOTAL_RK_LIST_TEST_COUNT] = {
    mpp_list_test_0,
};

int main(int argc, char *argv[])
{
    int i, start = 0, end = 0;
    if (argc < 2) {
        end = TOTAL_RK_LIST_TEST_COUNT;
    } else if (argc == 2) {
        start = atoi(argv[1]);
        end   = start + 1;
    } else if (argc == 3) {
        start = atoi(argv[1]);
        end   = atoi(argv[2]);
    } else {
        printf("too many argc %d\n", argc);
        return -1;
    }
    if (start < 0 || start > TOTAL_RK_LIST_TEST_COUNT || end < 0 || end > TOTAL_RK_LIST_TEST_COUNT) {
        printf("invalid input: start %d end %d\n", start, end);
        return -1;
    }
    for (i = start; i < end; i++) {
        int err = test_func[i]();
        if (err) {
            printf("test case %d return err %d\n", i, err);
            break;
        }
    }
    return 0;
}
#endif

