/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_mem"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "os_mem.h"

// mpp_mem_debug bit mask
#define MEM_DEBUG_EN            (0x00000001)
// NOTE: runtime log need debug enable
#define MEM_RUNTIME_LOG         (0x00000002)
#define MEM_NODE_LOG            (0x00000004)
#define MEM_EXT_ROOM            (0x00000010)
#define MEM_POISON              (0x00000020)

// default memory align size is set to 32
#define MEM_MAX_INDEX           (0x7fffffff)
#define MEM_ALIGN               32
#define MEM_ALIGN_MASK          (MEM_ALIGN - 1)
#define MEM_ALIGNED(x)          (((x) + MEM_ALIGN_MASK) & (~MEM_ALIGN_MASK))
#define MEM_HEAD_ROOM(debug)    (((debug & MEM_EXT_ROOM) != 0) ? (MEM_ALIGN) : (0))
#define MEM_NODE_MAX            (1024)
#define MEM_FREE_MAX            (512)
#define MEM_LOG_MAX             (1024)
#define MEM_CHECK_MARK          (0xdd)
#define MEM_HEAD_MASK           (0xab)
#define MEM_TAIL_MASK           (0xcd)

#define MPP_MEM_ASSERT(srv, cond) \
    do { \
        if (!(cond)) { \
            mpp_err("found mpp_mem assert failed, start dumping:\n"); \
            mpp_mem_srv_dump(srv, __FUNCTION__); \
            mpp_assert(cond); \
        } \
    } while (0)

#define get_srv_mem(caller) \
    ({ \
        MppMemSrv *__tmp; \
        if (!srv_mem) { \
            mpp_mem_srv_init(); \
        } \
        if (srv_mem) { \
            __tmp = srv_mem; \
        } else { \
            mpp_err("mpp mem srv not init at %s : %s\n", __FUNCTION__, caller); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

#define get_srv_mem_f() \
    ({ \
        MppMemSrv *__tmp; \
        if (!srv_mem) { \
            mpp_mem_srv_init(); \
        } \
        if (srv_mem) { \
            __tmp = srv_mem; \
        } else { \
            mpp_err("mpp mem srv not init at %s\n", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef enum MppMemOps_e {
    MEM_MALLOC,
    MEM_REALLOC,
    MEM_FREE,
    MEM_FREE_DELAY,

    MEM_OPS_BUTT,
} MppMemOps;

/*
 * Here we combined valid flag with index value to keep node structure small
 * If index >= 0 this node is valid otherwise it is invalid
 * When we need to invalid one index use ~ to revert all bit
 * Then max valid index is 0x7fffffff. When index goes beyond it and becomes
 * negative value index will be reset to zero.
 */
typedef struct MppMemNode_t {
    rk_s32          index;
    size_t          size;
    void            *ptr;
    const char      *caller;
} MppMemNode;

typedef struct MppMemLog_t {
    rk_u32          index;
    MppMemOps       ops;
    size_t          size_0;         // size at input
    size_t          size_1;         // size at output
    void            *ptr;           // ptr  at input
    void            *ret;           // ptr  at output
    MppMemNode      *node;          // node for operation
    const char      *caller;
} MppMemLog;

typedef struct MppMemSrv_t {
    // data for node record and delay free check
    rk_s32          nodes_max;
    rk_s32          nodes_idx;
    rk_s32          nodes_cnt;
    rk_s32          frees_max;
    rk_s32          frees_idx;
    rk_s32          frees_cnt;

    MppMemNode      *nodes;
    MppMemNode      *frees;

    // data for log record
    rk_u32          log_index;
    rk_s32          log_max;
    rk_s32          log_idx;
    rk_s32          log_cnt;

    MppMemLog       *logs;
    rk_u32          total_size;
    rk_u32          total_max;
    pthread_mutex_t lock;
    rk_u32          debug;
} MppMemSrv;

static MppMemSrv *srv_mem = NULL;

static const char *ops2str[MEM_OPS_BUTT] = {
    "malloc",
    "realloc",
    "free",
    "delayed",
};

static void show_mem(rk_u32 *buf, rk_s32 size)
{
    mpp_err("dumping buf %p size %d start\n", buf, size);

    while (size > 0) {
        if (size >= 16) {
            mpp_err("%08x %08x %08x %08x\n", buf[0], buf[1], buf[2], buf[3]);
            buf += 4;
            size -= 16;
        } else if (size >= 12) {
            mpp_err("%08x %08x %08x\n", buf[0], buf[1], buf[2]);
            buf += 3;
            size -= 12;
        } else if (size >= 8) {
            mpp_err("%08x %08x\n", buf[0], buf[1]);
            buf += 2;
            size -= 8;
        } else if (size >= 4) {
            mpp_err("%08x\n", buf[0]);
            buf += 1;
            size -= 4;
        } else {
            mpp_log("end with size %d\n", size);
            break;
        }
    }
    mpp_err("dumping buf %p size %d end\n", buf, size);
}

static void set_mem_ext_room(void *p, size_t size)
{
    memset((rk_u8 *)p - MEM_ALIGN, MEM_HEAD_MASK, MEM_ALIGN);
    memset((rk_u8 *)p + size,      MEM_TAIL_MASK, MEM_ALIGN);
}

void mpp_mem_srv_dump(MppMemSrv *srv, const char *caller)
{
    MppMemNode *node = srv->nodes;
    rk_s32 start;
    rk_s32 tmp_cnt;
    rk_s32 i;

    mpp_log("mpp_mem enter status dumping from %s:\n", caller);

    mpp_log("mpp_mem node count %d:\n", srv->nodes_cnt);
    if (srv->nodes_cnt) {
        for (i = 0; i < srv->nodes_max; i++, node++) {
            if (node->index < 0)
                continue;

            mpp_log("mpp_memory index %d caller %-32s size %-8u ptr %p\n",
                    node->index, node->caller, node->size, node->ptr);
        }
    }

    node = srv->frees;
    mpp_log("mpp_mem free count %d:\n", srv->frees_cnt);
    if (srv->frees_cnt) {
        for (i = 0; i < srv->frees_max; i++, node++) {
            if (node->index < 0)
                continue;

            mpp_log("mpp_freed  index %d caller %-32s size %-8u ptr %p\n",
                    node->index, node->caller, node->size, node->ptr);
        }
    }

    start = srv->log_idx - srv->log_cnt;
    tmp_cnt = srv->log_cnt;

    if (start < 0)
        start += srv->log_max;

    mpp_log("mpp_mem enter log dumping:\n");

    while (tmp_cnt) {
        MppMemLog *log = &srv->logs[start];

        mpp_log("idx %-8d op: %-7s from %-32s ptr %10p %10p size %7d %7d\n",
                log->index, ops2str[log->ops], log->caller,
                log->ptr, log->ret, log->size_0, log->size_1);

        start++;
        if (start >= srv->log_max)
            start = 0;

        tmp_cnt--;
    }
}

static void check_mem(MppMemSrv *srv, void *ptr, size_t size, const char *caller)
{
    rk_u8 *p;
    rk_s32 i;

    if ((srv->debug & MEM_EXT_ROOM) == 0)
        return ;

    p = (rk_u8 *)ptr - MEM_ALIGN;

    for (i = 0; i < MEM_ALIGN; i++) {
        if (p[i] != MEM_HEAD_MASK) {
            mpp_err("%s checking ptr %p head room found error!\n", caller, ptr);
            mpp_mem_srv_dump(srv, caller);
            show_mem((rk_u32 *)p, MEM_ALIGN);
            mpp_abort();
        }
    }

    p = (rk_u8 *)ptr + size;
    for (i = 0; i < MEM_ALIGN; i++) {
        if (p[i] != MEM_TAIL_MASK) {
            mpp_err("%s checking ptr %p tail room found error!\n", caller, ptr);
            mpp_mem_srv_dump(srv, caller);
            show_mem((rk_u32 *)p, MEM_ALIGN);
            mpp_abort();
        }
    }
}

static void check_node(MppMemSrv *srv, MppMemNode *node, const char *caller)
{
    if ((srv->debug & MEM_EXT_ROOM) == 0)
        return ;

    check_mem(srv, node->ptr, node->size, caller);
}

static rk_s32 check_poison(MppMemSrv *srv, MppMemNode *node)
{
    if ((srv->debug & MEM_POISON) == 0)
        return 0;

    // check oldest memory and free it
    rk_u8 *node_ptr = (rk_u8 *)node->ptr;
    rk_s32 size = node->size;
    rk_s32 i = 0;
    rk_s32 start = -1;
    rk_s32 end = -1;

    if (size >= 1024)
        return 0;

    for (; i < size; i++) {
        if (node_ptr[i] != MEM_CHECK_MARK) {
            if (start < 0) {
                start = i;
            }
            end = i + 1;
        }
    }

    if (start >= 0 || end >= 0) {
        mpp_err_f("found memory %p size %d caller %s overwrite from %d to %d\n",
                  node_ptr, size, node->caller, start, end);
        mpp_mem_srv_dump(srv, node->caller);
    }

    return end - start;
}

static void reset_node(MppMemSrv *srv, void *ptr, void *ret, size_t size, const char *caller)
{
    MppMemNode *node = srv->nodes;
    rk_s32 i = 0;

    if (srv->debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d equ size %8d at %s\n",
                srv->nodes_cnt, srv->total_size, size, __FUNCTION__);

    MPP_MEM_ASSERT(srv, srv->nodes_cnt <= srv->nodes_max);

    for (i = 0; i < srv->nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            srv->total_size += size;
            srv->total_size -= node->size;

            node->ptr   = ret;
            node->size  = size;
            node->caller = caller;

            if (srv->debug & MEM_EXT_ROOM)
                set_mem_ext_room(ret, size);
            break;
        }
    }
}

static void add_log(MppMemSrv *srv, MppMemOps ops, const char *caller, void *ptr,
                    void *ret, size_t size_0, size_t size_1)
{
    MppMemLog *log = &srv->logs[srv->log_idx];

    if (srv->debug & MEM_RUNTIME_LOG)
        mpp_log("%-7s ptr %010p %010p size %8u %8u at %s\n",
                ops2str[ops], ptr, ret, size_0, size_1, caller);

    log->index  = srv->log_index++;
    log->ops    = ops;
    log->size_0 = size_0;
    log->size_1 = size_1;
    log->ptr    = ptr;
    log->ret    = ret;
    log->node   = NULL;
    log->caller = caller;

    srv->log_idx++;
    if (srv->log_idx >= srv->log_max)
        srv->log_idx = 0;

    if (srv->log_cnt < srv->log_max)
        srv->log_cnt++;
}

static void add_node(MppMemSrv *srv, void *ptr, size_t size, const char *caller)
{
    MppMemNode *node;
    rk_s32 i;

    if (srv->debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d inc size %8d at %s\n",
                srv->nodes_cnt, srv->total_size, size, caller);

    if (srv->nodes_cnt >= srv->nodes_max) {
        mpp_err("******************************************************\n");
        mpp_err("* Reach max limit of mpp_mem counter %5d           *\n", srv->nodes_max);
        mpp_err("* Increase limit by setup env mpp_mem_node_max or    *\n");
        mpp_err("* recompile mpp with larger macro MEM_NODE_MAX value *\n");
        mpp_err("******************************************************\n");
        mpp_abort();
    }

    node = srv->nodes;
    for (i = 0; i < srv->nodes_max; i++, node++) {
        if (node->index < 0) {
            node->index = srv->nodes_idx++;
            node->size  = size;
            node->ptr   = ptr;
            node->caller = caller;

            // NOTE: reset node index on revert
            if (srv->nodes_idx < 0)
                srv->nodes_idx = 0;

            srv->nodes_cnt++;
            srv->total_size += size;
            if (srv->total_size > srv->total_max)
                srv->total_max = srv->total_size;
            break;
        }
    }
}

static void del_node(MppMemSrv *srv, void *ptr, size_t *size, const char *caller)
{
    MppMemNode *node = srv->nodes;
    rk_s32 i;

    MPP_MEM_ASSERT(srv, srv->nodes_cnt <= srv->nodes_max);

    for (i = 0; i < srv->nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            *size = node->size;
            node->index = ~node->index;
            srv->nodes_cnt--;
            srv->total_size -= node->size;

            if (srv->debug & MEM_NODE_LOG)
                mpp_log("mem cnt: %5d total %8d dec size %8d at %s\n",
                        srv->nodes_cnt, srv->total_size, node->size, caller);
            return ;
        }
    }

    mpp_err("%s fail to find node with ptr %p\n", caller, ptr);
    mpp_abort();
    return ;
}

static void *delay_del_node(MppMemSrv *srv, void *ptr, size_t *size, const char *caller)
{
    MppMemNode *node = srv->nodes;
    MppMemNode *free_node = NULL;
    void *ret = NULL;
    rk_s32 i = 0;

    // clear output first
    *size = 0;

    // find the node to save
    MPP_MEM_ASSERT(srv, srv->nodes_cnt <= srv->nodes_max);
    for (i = 0; i < srv->nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            check_node(srv, node, caller);
            break;
        }
    }

    MPP_MEM_ASSERT(srv, i < srv->nodes_max);
    if (srv->debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d dec size %8d at %s\n",
                srv->nodes_cnt, srv->total_size, node->size, caller);

    MPP_MEM_ASSERT(srv, srv->frees_cnt <= srv->frees_max);

    if (srv->frees_cnt) {
        MppMemNode *tmp = srv->frees;

        // NODE: check all data here
        for (i = 0; i < srv->frees_max; i++, tmp++) {
            if (tmp->index >= 0) {
                check_node(srv, tmp, caller);
                check_poison(srv, tmp);
            }
        }
    }

    if (srv->frees_cnt >= srv->frees_max) {
        // free list full start del
        rk_s32 frees_last = srv->frees_idx - srv->frees_cnt;

        if (frees_last < 0)
            frees_last += srv->frees_max;

        free_node = &srv->frees[frees_last];

        if (free_node->index >= 0) {
            check_node(srv, free_node, caller);
            check_poison(srv, free_node);
            ret = free_node->ptr;
            *size = free_node->size;
            free_node->index = ~free_node->index;
            srv->frees_cnt--;
        }
    }

    MPP_MEM_ASSERT(srv, srv->frees_cnt <= srv->frees_max);

    // free list is NOT full just store
    free_node = &srv->frees[srv->frees_idx];
    srv->frees_idx++;
    if (srv->frees_idx >= srv->frees_max)
        srv->frees_idx = 0;
    if (srv->frees_cnt < srv->frees_max)
        srv->frees_cnt++;

    MPP_MEM_ASSERT(srv, srv->frees_cnt <= srv->frees_max);

    memcpy(&srv->frees[srv->frees_idx], node, sizeof(*node));

    if ((srv->debug & MEM_POISON) && (node->size < 1024))
        memset(node->ptr, MEM_CHECK_MARK, node->size);

    node->index = ~node->index;
    srv->total_size -= node->size;
    srv->nodes_cnt--;

    return ret;
}

static void mpp_mem_srv_init()
{
    MppMemSrv *srv = srv_mem;

    if (srv)
        return;

    os_malloc((void **)&srv, MEM_ALIGN, sizeof(*srv));
    mpp_assert(srv);
    srv_mem = srv;

    memset(srv, 0, sizeof(*srv));

    mpp_env_get_u32("mpp_mem_debug", &srv->debug, 0);

    srv->nodes_max = MEM_NODE_MAX;
    srv->frees_max = MEM_FREE_MAX;
    srv->log_max = MEM_LOG_MAX;

    {
        // init mutex lock
        pthread_mutexattr_t attr;

        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&srv->lock, &attr);
        pthread_mutexattr_destroy(&attr);
    }

    pthread_mutex_lock(&srv->lock);

    // add more flag if debug enabled
    if (srv->debug)
        srv->debug |= MEM_DEBUG_EN;

    if (srv->debug & MEM_DEBUG_EN) {
        size_t size;

        mpp_env_get_u32("mpp_mem_node_max", (rk_u32 *)&srv->nodes_max, MEM_NODE_MAX);

        mpp_log_f("mpp_mem_debug enabled %x max node %d\n",
                  srv->debug, srv->nodes_max);

        size = srv->nodes_max * sizeof(MppMemNode);
        os_malloc((void **)&srv->nodes, MEM_ALIGN, size);
        mpp_assert(srv->nodes);
        memset(srv->nodes, 0xff, size);
        add_node(srv, srv->nodes, size, __FUNCTION__);

        size = srv->frees_max * sizeof(MppMemNode);
        os_malloc((void **)&srv->frees, MEM_ALIGN, size);
        mpp_assert(srv->frees);
        memset(srv->frees, 0xff, size);
        add_node(srv, srv->frees, size, __FUNCTION__);

        size = srv->log_max * sizeof(MppMemLog);
        os_malloc((void **)&srv->logs, MEM_ALIGN, size);
        mpp_assert(srv->logs);
        add_node(srv, srv->logs, size, __FUNCTION__);

        add_node(srv, srv, sizeof(MppMemSrv), __FUNCTION__);
    }

    pthread_mutex_unlock(&srv->lock);
}

static void mpp_mem_srv_deinit()
{
    MppMemSrv *srv = get_srv_mem_f();

    if (!srv)
        return;

    if (srv->debug & MEM_DEBUG_EN) {
        MppMemNode *node = srv->nodes;
        size_t size = 0;
        rk_s32 i = 0;

        pthread_mutex_lock(&srv->lock);

        // delete self node first
        del_node(srv, srv,  &size, __FUNCTION__);
        del_node(srv, srv->nodes, &size, __FUNCTION__);
        del_node(srv, srv->frees, &size, __FUNCTION__);
        del_node(srv, srv->logs,  &size, __FUNCTION__);

        // then check leak memory
        if (srv->nodes_cnt) {
            for (i = 0; i < srv->nodes_max; i++, node++) {
                if (node->index >= 0) {
                    mpp_log("found idx %8d mem %10p size %d leaked from %s\n",
                            node->index, node->ptr, node->size, node->caller);
                    srv->nodes_cnt--;
                    add_log(srv, MEM_FREE, __FUNCTION__, node->ptr, NULL,
                            node->size, 0);
                }
            }

            mpp_assert(srv->nodes_cnt == 0);
        }

        // finally release all delay free memory
        if (srv->frees_cnt) {
            node = srv->frees;

            for (i = 0; i < srv->frees_max; i++, node++) {
                if (node->index >= 0) {
                    os_free((rk_u8 *)node->ptr - MEM_HEAD_ROOM(srv->debug));
                    node->index = ~node->index;
                    srv->frees_cnt--;
                    add_log(srv, MEM_FREE_DELAY, __FUNCTION__, node->ptr, NULL,
                            node->size, 0);
                }
            }

            mpp_assert(srv->frees_cnt == 0);
        }

        os_free(srv->nodes);
        os_free(srv->frees);
        os_free(srv->logs);

        pthread_mutex_unlock(&srv->lock);
    }

    pthread_mutex_destroy(&srv->lock);

    os_free(srv);
    srv_mem = NULL;
}

MPP_SINGLETON(MPP_SGLN_OS_MEM, mpp_mem, mpp_mem_srv_init, mpp_mem_srv_deinit)

void *mpp_osal_malloc(const char *caller, size_t size)
{
    MppMemSrv *srv = get_srv_mem(caller);
    rk_u32 debug = srv->debug;
    size_t size_align = MEM_ALIGNED(size);
    size_t size_real = ((debug & MEM_EXT_ROOM) != 0) ? (size_align + 2 * MEM_ALIGN) : (size_align);
    void *ptr;

    os_malloc(&ptr, MEM_ALIGN, size_real);

    if (debug) {
        pthread_mutex_lock(&srv->lock);

        add_log(srv, MEM_MALLOC, caller, NULL, ptr, size, size_real);

        if (ptr) {
            if (debug & MEM_EXT_ROOM) {
                ptr = (rk_u8 *)ptr + MEM_ALIGN;
                set_mem_ext_room(ptr, size);
            }

            add_node(srv, ptr, size, caller);
        }

        pthread_mutex_unlock(&srv->lock);
    }

    return ptr;
}

void *mpp_osal_calloc(const char *caller, size_t size)
{
    void *ptr = mpp_osal_malloc(caller, size);

    if (ptr)
        memset(ptr, 0, size);

    return ptr;
}

void *mpp_osal_realloc(const char *caller, void *ptr, size_t size)
{
    MppMemSrv *srv = get_srv_mem(caller);
    rk_u32 debug = srv->debug;
    size_t size_align;
    size_t size_real;
    void *ptr_real;
    void *ret;

    if (!ptr)
        return mpp_osal_malloc(caller, size);

    if (0 == size) {
        mpp_err("warning: realloc %p to zero size\n", ptr);
        return NULL;
    }

    size_align = MEM_ALIGNED(size);
    size_real = ((debug & MEM_EXT_ROOM) != 0) ? (size_align + 2 * MEM_ALIGN) : (size_align);
    ptr_real = (rk_u8 *)ptr - MEM_HEAD_ROOM(debug);

    os_realloc(ptr_real, &ret, MEM_ALIGN, size_align);

    if (!ret) {
        // if realloc fail the original buffer will be kept the same.
        mpp_err("mpp_realloc ptr %p to size %d failed\n", ptr, size);
    } else {
        pthread_mutex_lock(&srv->lock);

        // if realloc success reset the node and record
        if (debug) {
            void *ret_ptr = ((debug & MEM_EXT_ROOM) != 0) ?
                            ((rk_u8 *)ret + MEM_ALIGN) : (ret);

            reset_node(srv, ptr, ret_ptr, size, caller);
            add_log(srv, MEM_REALLOC, caller, ptr, ret_ptr, size, size_real);
            ret = ret_ptr;
        }

        pthread_mutex_unlock(&srv->lock);
    }

    return ret;
}

void mpp_osal_free(const char *caller, void *ptr)
{
    MppMemSrv *srv = get_srv_mem(caller);
    size_t size = 0;

    if (!ptr)
        return;

    if (!srv || !srv->debug) {
        os_free(ptr);
        return ;
    }

    pthread_mutex_lock(&srv->lock);

    if (srv->debug & MEM_POISON) {
        // NODE: keep this node and  delete delay node
        void *ret = delay_del_node(srv, ptr, &size, caller);
        if (ret)
            os_free((rk_u8 *)ret - MEM_ALIGN);

        add_log(srv, MEM_FREE_DELAY, caller, ptr, ret, size, 0);
    } else {
        void *ptr_real = (rk_u8 *)ptr - MEM_HEAD_ROOM(srv->debug);

        // NODE: delete node and return size here
        del_node(srv, ptr, &size, caller);
        check_mem(srv, ptr, size, caller);
        os_free(ptr_real);
        add_log(srv, MEM_FREE, caller, ptr, ptr_real, size, 0);
    }

    pthread_mutex_unlock(&srv->lock);
}

/* dump memory status */
void mpp_show_mem_status()
{
    MppMemSrv *srv = get_srv_mem_f();

    if (srv) {
        pthread_mutex_lock(&srv->lock);
        if (srv->debug & MEM_DEBUG_EN)
            mpp_mem_srv_dump(srv, __FUNCTION__);
        pthread_mutex_unlock(&srv->lock);
    }
}

rk_u32 mpp_mem_total_now()
{
    MppMemSrv *srv = get_srv_mem_f();
    rk_u32 total_now = 0;

    if (srv) {
        pthread_mutex_lock(&srv->lock);
        total_now = srv->total_size;
        pthread_mutex_unlock(&srv->lock);
    }

    return total_now;
}

rk_u32 mpp_mem_total_max()
{
    MppMemSrv *srv = get_srv_mem_f();
    rk_u32 total_max = 0;

    if (srv) {
        pthread_mutex_lock(&srv->lock);
        total_max = srv->total_max;
        pthread_mutex_unlock(&srv->lock);
    }

    return total_max;
}
