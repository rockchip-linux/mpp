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
#define MEM_ALIGNED(x)          (((x) + MEM_ALIGN) & (~MEM_ALIGN_MASK))
#define MEM_HEAD_ROOM(debug)    ((debug & MEM_EXT_ROOM) ? (MEM_ALIGN) : (0))
#define MEM_NODE_MAX            (1024)
#define MEM_FREE_MAX            (512)
#define MEM_LOG_MAX             (1024)
#define MEM_CHECK_MARK          (0xdd)
#define MEM_HEAD_MASK           (0xab)
#define MEM_TAIL_MASK           (0xcd)

#define MPP_MEM_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            mpp_err("found mpp_mem assert failed, start dumping:\n"); \
            service.dump(__FUNCTION__); \
            mpp_assert(cond); \
        } \
    } while (0)

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
typedef struct MppMemNode_s {
    RK_S32      index;
    size_t      size;
    void        *ptr;
    const char  *caller;
} MppMemNode;

typedef struct MppMemLog_s {
    RK_U32      index;
    MppMemOps   ops;
    size_t      size_0;         // size at input
    size_t      size_1;         // size at output
    void        *ptr;           // ptr  at input
    void        *ret;           // ptr  at output
    MppMemNode  *node;          // node for operation
    const char  *caller;
} MppMemLog;

class MppMemService
{
public:
    // avoid any unwanted function
    MppMemService();
    ~MppMemService();

    void    add_node(const char *caller, void *ptr, size_t size);
    /*
     * try delete node will return index in nodes
     * return 1 for need os_free call to real free
     * return 0 for reserve memory for check
     */
    RK_S32  find_node(const char *caller, void *ptr, size_t *size, RK_S32 *idx);
    /*
     */
    void    del_node(const char *caller, void *ptr, size_t *size);
    void*   delay_del_node(const char *caller, void *ptr, size_t *size);
    void    reset_node(const char *caller, void *ptr, void *ret, size_t size);

    void    chk_node(const char *caller, MppMemNode *node);
    void    chk_mem(const char *caller, void *ptr, size_t size);
    RK_S32  chk_poison(MppMemNode *node);

    void    add_log(MppMemOps ops, const char *caller, void *ptr, void *ret,
                    size_t size_0, size_t size_1);

    void    dump(const char *caller);

    Mutex       lock;
    RK_U32      debug;

private:
    // data for node record and delay free check
    RK_S32      nodes_max;
    RK_S32      nodes_idx;
    RK_S32      nodes_cnt;
    RK_S32      frees_max;
    RK_S32      frees_idx;
    RK_S32      frees_cnt;

    MppMemNode  *nodes;
    MppMemNode  *frees;

    // data for log record
    RK_U32      log_index;
    RK_S32      log_max;
    RK_S32      log_idx;
    RK_S32      log_cnt;

    MppMemLog   *logs;
    RK_U32      total_size;

    MppMemService(const MppMemService &);
    MppMemService &operator=(const MppMemService &);
};

static MppMemService service;

static const char *ops2str[MEM_OPS_BUTT] = {
    "malloc",
    "realloc",
    "free",
    "delayed",
};

static void show_mem(RK_U32 *buf, RK_S32 size)
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
    memset((RK_U8 *)p - MEM_ALIGN, MEM_HEAD_MASK, MEM_ALIGN);
    memset((RK_U8 *)p + size,      MEM_TAIL_MASK, MEM_ALIGN);
}

MppMemService::MppMemService()
    : debug(0),
      nodes_max(MEM_NODE_MAX),
      nodes_idx(0),
      nodes_cnt(0),
      frees_max(MEM_FREE_MAX),
      frees_idx(0),
      frees_cnt(0),
      nodes(NULL),
      frees(NULL),
      log_index(0),
      log_max(MEM_LOG_MAX),
      log_idx(0),
      log_cnt(0),
      logs(NULL),
      total_size(0)
{
    mpp_env_get_u32("mpp_mem_debug", &debug, 0);

    // add more flag if debug enabled
    if (debug)
        debug |= MEM_DEBUG_EN;

    if (debug & MEM_DEBUG_EN) {
        mpp_env_get_u32("mpp_mem_node_max", (RK_U32 *)&nodes_max, MEM_NODE_MAX);

        mpp_log_f("mpp_mem_debug enabled %x max node %d\n",
                  debug, nodes_max);

        size_t size = nodes_max * sizeof(MppMemNode);
        os_malloc((void **)&nodes, MEM_ALIGN, size);
        mpp_assert(nodes);
        memset(nodes, 0xff, size);
        add_node(__FUNCTION__, nodes, size);

        size = frees_max * sizeof(MppMemNode);
        os_malloc((void **)&frees, MEM_ALIGN, size);
        mpp_assert(frees);
        memset(frees, 0xff, size);
        add_node(__FUNCTION__, frees, size);

        size = log_max * sizeof(MppMemLog);
        os_malloc((void **)&logs, MEM_ALIGN, size);
        mpp_assert(logs);
        add_node(__FUNCTION__, logs, size);

        add_node(__FUNCTION__, this, sizeof(MppMemService));
    }
}

MppMemService::~MppMemService()
{
    if (debug & MEM_DEBUG_EN) {
        AutoMutex auto_lock(&lock);

        RK_S32 i = 0;
        MppMemNode *node = nodes;

        // delete self node first
        size_t size = 0;

        del_node(__FUNCTION__, this,  &size);
        del_node(__FUNCTION__, nodes, &size);
        del_node(__FUNCTION__, frees, &size);
        del_node(__FUNCTION__, logs,  &size);

        // then check leak memory
        if (nodes_cnt) {
            for (i = 0; i < nodes_max; i++, node++) {
                if (node->index >= 0) {
                    mpp_log("found idx %8d mem %10p size %d leaked\n",
                            node->index, node->ptr, node->size);
                    nodes_cnt--;
                    add_log(MEM_FREE, __FUNCTION__, node->ptr, NULL,
                            node->size, 0);
                }
            }

            mpp_assert(nodes_cnt == 0);
        }

        // finally release all delay free memory
        if (frees_cnt) {
            node = frees;

            for (i = 0; i < frees_max; i++, node++) {
                if (node->index >= 0) {
                    os_free((RK_U8 *)node->ptr - MEM_HEAD_ROOM(debug));
                    node->index = ~node->index;
                    frees_cnt--;
                    add_log(MEM_FREE_DELAY, __FUNCTION__, node->ptr, NULL,
                            node->size, 0);
                }
            }

            mpp_assert(frees_cnt == 0);
        }

        os_free(nodes);
        os_free(frees);
        os_free(logs);
    }
}

void MppMemService::add_node(const char *caller, void *ptr, size_t size)
{
    RK_S32 i = 0;

    if (debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d inc size %8d at %s\n",
                nodes_cnt, total_size, size, caller);

    if (nodes_cnt >= nodes_max) {
        mpp_err("******************************************************\n");
        mpp_err("* Reach max limit of mpp_mem counter %5d           *\n", nodes_max);
        mpp_err("* Increase limit by setup env mpp_mem_node_max or    *\n");
        mpp_err("* recompile mpp with larger macro MEM_NODE_MAX value *\n");
        mpp_err("******************************************************\n");
        mpp_abort();
    }

    MppMemNode *node = nodes;
    for (i = 0; i < nodes_max; i++, node++) {
        if (node->index < 0) {
            node->index = nodes_idx++;
            node->size  = size;
            node->ptr   = ptr;
            node->caller = caller;

            // NOTE: reset node index on revert
            if (nodes_idx < 0)
                nodes_idx = 0;

            nodes_cnt++;
            total_size += size;
            break;
        }
    }
}

RK_S32 MppMemService::find_node(const char *caller, void *ptr, size_t *size, RK_S32 *idx)
{
    RK_S32 i = 0;
    MppMemNode *node = nodes;

    MPP_MEM_ASSERT(nodes_cnt <= nodes_max);
    for (i = 0; i < nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            *size = node->size;
            *idx  = i;
            return 1;
        }
    }

    mpp_err("%s can NOT found node with ptr %p\n", caller, ptr);
    mpp_abort();
    return 0;
}

void MppMemService::del_node(const char *caller, void *ptr, size_t *size)
{
    RK_S32 i = 0;
    MppMemNode *node = nodes;

    MPP_MEM_ASSERT(nodes_cnt <= nodes_max);

    for (i = 0; i < nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            *size = node->size;
            node->index = ~node->index;
            nodes_cnt--;
            total_size -= node->size;

            if (debug & MEM_NODE_LOG)
                mpp_log("mem cnt: %5d total %8d dec size %8d at %s\n",
                        nodes_cnt, total_size, node->size, caller);
            return ;
        }
    }

    mpp_err("%s fail to find node with ptr %p\n", caller, ptr);
    mpp_abort();
    return ;
}

void *MppMemService::delay_del_node(const char *caller, void *ptr, size_t *size)
{
    RK_S32 i = 0;
    MppMemNode *node = nodes;

    // clear output first
    void *ret = NULL;
    *size = 0;

    // find the node to save
    MPP_MEM_ASSERT(nodes_cnt <= nodes_max);
    for (i = 0; i < nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            chk_node(caller, node);
            break;
        }
    }

    MPP_MEM_ASSERT(i < nodes_max);
    if (debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d dec size %8d at %s\n",
                nodes_cnt, total_size, node->size, caller);

    MppMemNode *free_node = NULL;

    MPP_MEM_ASSERT(frees_cnt <= frees_max);

    if (frees_cnt) {
        MppMemNode *tmp = frees;

        // NODE: check all data here
        for (i = 0; i < frees_max; i++, tmp++) {
            if (tmp->index >= 0) {
                chk_node(caller, tmp);
                chk_poison(tmp);
            }
        }
    }

    if (frees_cnt >= frees_max) {
        // free list full start del
        RK_S32 frees_last = frees_idx - frees_cnt;
        if (frees_last < 0)
            frees_last += frees_max;

        free_node = &frees[frees_last];

        if (free_node->index >= 0) {
            chk_node(caller, free_node);
            chk_poison(free_node);
            ret = free_node->ptr;
            *size = free_node->size;
            free_node->index = ~free_node->index;
            frees_cnt--;
        }
    }

    MPP_MEM_ASSERT(frees_cnt <= frees_max);

    // free list is NOT full just store
    free_node = &frees[frees_idx];
    frees_idx++;
    if (frees_idx >= frees_max)
        frees_idx = 0;
    if (frees_cnt < frees_max)
        frees_cnt++;

    MPP_MEM_ASSERT(frees_cnt <= frees_max);

    memcpy(&frees[frees_idx], node, sizeof(*node));

    if ((debug & MEM_POISON) && (node->size < 1024))
        memset(node->ptr, MEM_CHECK_MARK, node->size);

    node->index = ~node->index;
    total_size -= node->size;
    nodes_cnt--;

    return ret;
}

void MppMemService::chk_node(const char *caller, MppMemNode *node)
{
    if ((debug & MEM_EXT_ROOM) == 0)
        return ;

    chk_mem(caller, node->ptr, node->size);
}

void MppMemService::chk_mem(const char *caller, void *ptr, size_t size)
{
    if ((debug & MEM_EXT_ROOM) == 0)
        return ;

    RK_S32 i = 0;
    RK_U8 *p = (RK_U8 *)ptr - MEM_ALIGN;

    for (i = 0; i < MEM_ALIGN; i++) {
        if (p[i] != MEM_HEAD_MASK) {
            mpp_err("%s checking ptr %p head room found error!\n", caller, ptr);
            dump(caller);
            show_mem((RK_U32 *)p, MEM_ALIGN);
            mpp_abort();
        }
    }

    p = (RK_U8 *)ptr + size;
    for (i = 0; i < MEM_ALIGN; i++) {
        if (p[i] != MEM_TAIL_MASK) {
            mpp_err("%s checking ptr %p tail room found error!\n", caller, ptr);
            dump(caller);
            show_mem((RK_U32 *)p, MEM_ALIGN);
            mpp_abort();
        }
    }
}

RK_S32 MppMemService::chk_poison(MppMemNode *node)
{
    if ((debug & MEM_POISON) == 0)
        return 0;

    // check oldest memory and free it
    RK_U8 *node_ptr = (RK_U8 *)node->ptr;
    RK_S32 size = node->size;
    RK_S32 i = 0;
    RK_S32 start = -1;
    RK_S32 end = -1;

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
        dump(node->caller);
    }

    return end - start;
}

void MppMemService::reset_node(const char *caller, void *ptr, void *ret, size_t size)
{
    RK_S32 i = 0;
    MppMemNode *node = nodes;

    if (debug & MEM_NODE_LOG)
        mpp_log("mem cnt: %5d total %8d equ size %8d at %s\n",
                nodes_cnt, total_size, size, __FUNCTION__);

    MPP_MEM_ASSERT(nodes_cnt <= nodes_max);

    for (i = 0; i < nodes_max; i++, node++) {
        if (node->index >= 0 && node->ptr == ptr) {
            total_size  += size;
            total_size  -= node->size;

            node->ptr   = ret;
            node->size  = size;
            node->caller = caller;

            if (debug & MEM_EXT_ROOM)
                set_mem_ext_room(ret, size);
            break;
        }
    }
}

void MppMemService::add_log(MppMemOps ops, const char *caller,
                            void *ptr, void *ret, size_t size_0, size_t size_1)
{
    MppMemLog *log = &logs[log_idx];

    if (service.debug & MEM_RUNTIME_LOG)
        mpp_log("%-7s ptr %010p %010p size %8u %8u at %s\n",
                ops2str[ops], ptr, ret, size_0, size_1, caller);

    log->index  = log_index++;
    log->ops    = ops;
    log->size_0 = size_0;
    log->size_1 = size_1;
    log->ptr    = ptr;
    log->ret    = ret;
    log->node   = NULL;
    log->caller = caller;

    log_idx++;
    if (log_idx >= log_max)
        log_idx = 0;

    if (log_cnt < log_max)
        log_cnt++;
}

void MppMemService::dump(const char *caller)
{
    RK_S32 i;
    MppMemNode *node = nodes;

    mpp_log("mpp_mem enter status dumping from %s:\n", caller);

    mpp_log("mpp_mem node count %d:\n", nodes_cnt);
    if (nodes_cnt) {
        for (i = 0; i < nodes_max; i++, node++) {
            if (node->index < 0)
                continue;

            mpp_log("mpp_memory index %d caller %-32s size %-8u ptr %p\n",
                    node->index, node->caller, node->size, node->ptr);
        }
    }

    node = frees;
    mpp_log("mpp_mem free count %d:\n", frees_cnt);
    if (frees_cnt) {
        for (i = 0; i < frees_max; i++, node++) {
            if (node->index < 0)
                continue;

            mpp_log("mpp_freed  index %d caller %-32s size %-8u ptr %p\n",
                    node->index, node->caller, node->size, node->ptr);
        }
    }

    RK_S32 start = log_idx - log_cnt;
    RK_S32 tmp_cnt = log_cnt;

    if (start < 0)
        start += log_max;

    mpp_log("mpp_mem enter log dumping:\n");

    while (tmp_cnt) {
        MppMemLog *log = &logs[start];

        mpp_log("idx %-8d op: %-7s from %-32s ptr %10p %10p size %7d %7d\n",
                log->index, ops2str[log->ops], log->caller,
                log->ptr, log->ret, log->size_0, log->size_1);

        start++;
        if (start >= log_max)
            start = 0;

        tmp_cnt--;
    }
}

void *mpp_osal_malloc(const char *caller, size_t size)
{
    AutoMutex auto_lock(&service.lock);
    RK_U32 debug = service.debug;
    size_t size_align = MEM_ALIGNED(size);
    size_t size_real = (debug & MEM_EXT_ROOM) ? (size_align + 2 * MEM_ALIGN) :
                       (size_align);
    void *ptr;

    os_malloc(&ptr, MEM_ALIGN, size_real);

    if (debug) {
        service.add_log(MEM_MALLOC, caller, NULL, ptr, size, size_real);

        if (ptr) {
            if (debug & MEM_EXT_ROOM) {
                ptr = (RK_U8 *)ptr + MEM_ALIGN;
                set_mem_ext_room(ptr, size);
            }

            service.add_node(caller, ptr, size);
        }
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
    AutoMutex auto_lock(&service.lock);
    RK_U32 debug = service.debug;
    void *ret;

    if (NULL == ptr)
        return mpp_osal_malloc(caller, size);

    if (0 == size) {
        mpp_err("warning: realloc %p to zero size\n", ptr);
        return NULL;
    }

    size_t size_align = MEM_ALIGNED(size);
    size_t size_real = (debug & MEM_EXT_ROOM) ? (size_align + 2 * MEM_ALIGN) :
                       (size_align);
    void *ptr_real = (RK_U8 *)ptr - MEM_HEAD_ROOM(debug);

    os_realloc(ptr_real, &ret, MEM_ALIGN, size_align);

    if (NULL == ret) {
        // if realloc fail the original buffer will be kept the same.
        mpp_err("mpp_realloc ptr %p to size %d failed\n", ptr, size);
    } else {
        // if realloc success reset the node and record
        if (debug) {
            void *ret_ptr = (debug & MEM_EXT_ROOM) ?
                            ((RK_U8 *)ret + MEM_ALIGN) : (ret);

            service.reset_node(caller, ptr, ret_ptr, size);
            service.add_log(MEM_REALLOC, caller, ptr, ret_ptr, size, size_real);
            ret = ret_ptr;
        }
    }

    return ret;
}

void mpp_osal_free(const char *caller, void *ptr)
{
    AutoMutex auto_lock(&service.lock);
    RK_U32 debug = service.debug;
    if (NULL == ptr)
        return;

    if (!debug) {
        os_free(ptr);
        return ;
    }

    size_t size = 0;

    if (debug & MEM_POISON) {
        // NODE: keep this node and  delete delay node
        void *ret = service.delay_del_node(caller, ptr, &size);
        if (ret)
            os_free((RK_U8 *)ret - MEM_ALIGN);

        service.add_log(MEM_FREE_DELAY, caller, ptr, ret, size, 0);
    } else {
        void *ptr_real = (RK_U8 *)ptr - MEM_HEAD_ROOM(debug);
        // NODE: delete node and return size here
        service.del_node(caller, ptr, &size);
        service.chk_mem(caller, ptr, size);
        os_free(ptr_real);
        service.add_log(MEM_FREE, caller, ptr, ptr_real, size, 0);
    }
}

/* dump memory status */
void mpp_show_mem_status()
{
    AutoMutex auto_lock(&service.lock);
    if (service.debug & MEM_DEBUG_EN)
        service.dump(__FUNCTION__);
}
