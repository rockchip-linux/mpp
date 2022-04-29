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
#include "mpp_common.h"


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

static mpp_list_node* create_list_with_size(void *data, RK_S32 size, RK_U32 key)
{
    mpp_list_node *node = (mpp_list_node*)malloc(sizeof(mpp_list_node) +
                                                 sizeof(size) + size);
    if (node) {
        RK_S32 *dst = (RK_S32 *)(node + 1);
        list_node_init_with_key_and_size(node, key, size);
        *dst++ = size;
        memcpy(dst, data, size);
    } else {
        LIST_ERROR("failed to allocate list node");
    }
    return node;
}

RK_S32 mpp_list::fifo_wr(void *data, RK_S32 size)
{
    RK_S32 ret = -EINVAL;
    if (head) {
        mpp_list_node *node = create_list_with_size(data, size, 0);
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

static void release_list_with_size(mpp_list_node* node, void *data, RK_S32 *size)
{
    RK_S32 *src = (RK_S32*)(node + 1);
    RK_S32 data_size = *src++;

    *size = data_size;

    if (data)
        memcpy(data, src, data_size);

    free(node);
}

RK_S32 mpp_list::fifo_rd(void *data, RK_S32 *size)
{
    RK_S32 ret = -EINVAL;
    if (head && count) {
        mpp_list_node *node = head->next;

        mpp_list_del_init(node);
        release_list_with_size(node, data, size);
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
    signal();
    return 0;
}

MPP_RET mpp_list::wait_lt(RK_S64 timeout, RK_S32 val)
{
    if (list_size() < val)
        return MPP_OK;

    if (!timeout)
        return MPP_NOK;

    if (timeout < 0)
        wait();
    else
        wait(timeout);

    return list_size() < val ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_list::wait_le(RK_S64 timeout, RK_S32 val)
{
    if (list_size() <= val)
        return MPP_OK;

    if (!timeout)
        return MPP_NOK;

    if (timeout < 0)
        wait();
    else
        wait(timeout);

    return list_size() <= val ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_list::wait_gt(RK_S64 timeout, RK_S32 val)
{
    if (list_size() > val)
        return MPP_OK;

    if (!timeout)
        return MPP_NOK;

    if (timeout < 0)
        wait();
    else
        wait(timeout);

    return list_size() > val ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_list::wait_ge(RK_S64 timeout, RK_S32 val)
{
    if (list_size() >= val)
        return MPP_OK;

    if (!timeout)
        return MPP_NOK;

    if (timeout < 0)
        wait();
    else
        wait(timeout);

    return list_size() >= val ? MPP_OK : MPP_NOK;
}

RK_U32 mpp_list::get_key()
{
    return keys++;
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

/* list sort porting from kernel list_sort.c */

/*
 * Returns a list organized in an intermediate format suited
 * to chaining of merge() calls: null-terminated, no reserved or
 * sentinel head node, "prev" links not maintained.
 */
static struct list_head *merge(void *priv, list_cmp_func_t cmp,
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
static void merge_final(void *priv, list_cmp_func_t cmp, struct list_head *head,
                        struct list_head *a, struct list_head *b)
{
    struct list_head *tail = head;
    RK_U8 count = 0;

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

void list_sort(void *priv, struct list_head *head, list_cmp_func_t cmp)
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

