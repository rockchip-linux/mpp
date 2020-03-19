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

#ifndef __MPP_BUFFER_IMPL_H__
#define __MPP_BUFFER_IMPL_H__

#include "mpp_list.h"
#include "mpp_common.h"
#include "mpp_allocator.h"

#define MPP_BUF_DBG_FUNCTION            (0x00000001)
#define MPP_BUF_DBG_OPS_RUNTIME         (0x00000002)
#define MPP_BUF_DBG_OPS_HISTORY         (0x00000004)
#define MPP_BUF_DBG_CLR_ON_EXIT         (0x00000010)
#define MPP_BUF_DBG_DUMP_ON_EXIT        (0x00000020)
#define MPP_BUF_DBG_CHECK_SIZE          (0x00000100)

#define mpp_buf_dbg(flag, fmt, ...)     _mpp_dbg(mpp_buffer_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_buf_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_buffer_debug, flag, fmt, ## __VA_ARGS__)

#define MPP_BUF_FUNCTION_ENTER()        mpp_buf_dbg_f(MPP_BUF_DBG_FUNCTION, "enter\n")
#define MPP_BUF_FUNCTION_LEAVE()        mpp_buf_dbg_f(MPP_BUF_DBG_FUNCTION, "leave\n")
#define MPP_BUF_FUNCTION_LEAVE_OK()     mpp_buf_dbg_f(MPP_BUF_DBG_FUNCTION, "success\n")
#define MPP_BUF_FUNCTION_LEAVE_FAIL()   mpp_buf_dbg_f(MPP_BUF_DBG_FUNCTION, "failed\n")

typedef struct MppBufferImpl_t          MppBufferImpl;
typedef struct MppBufferGroupImpl_t     MppBufferGroupImpl;
typedef void (*MppBufCallback)(void *, void *);

// use index instead of pointer to avoid invalid pointer
struct MppBufferImpl_t {
    char                tag[MPP_TAG_SIZE];
    const char          *caller;
    RK_U32              group_id;
    RK_S32              buffer_id;
    MppBufferMode       mode;

    MppBufferInfo       info;
    size_t              offset;

    /* used for buf on group reset mode
       set disard value to 1 when frame refcount no zero ,
       we will delay relesase buffer after refcount to zero,
       not put this buf to unused list
     */
    RK_S32              discard;
    // used flag is for used/unused list detection
    RK_U32              used;
    RK_U32              internal;
    RK_S32              ref_count;
    struct list_head    list_status;
};

struct MppBufferGroupImpl_t {
    char                tag[MPP_TAG_SIZE];
    const char          *caller;
    RK_U32              group_id;
    MppBufferMode       mode;
    MppBufferType       type;
    // used in limit mode only
    size_t              limit_size;
    RK_S32              limit_count;
    // status record
    size_t              limit;
    size_t              usage;
    RK_S32              buffer_id;
    RK_S32              buffer_count;
    RK_S32              count_used;
    RK_S32              count_unused;

    MppAllocator        allocator;
    MppAllocatorApi     *alloc_api;

    // thread that will be signal on buffer return
    MppBufCallback      callback;
    void                *arg;

    // buffer force clear mode flag
    RK_U32              clear_on_exit;
    // is_orphan: 0 - normal group 1 - orphan group
    RK_U32              is_orphan;

    // buffer log function
    RK_U32              log_runtime_en;
    RK_U32              log_history_en;
    RK_U32              log_count;
    struct list_head    list_logs;

    // link to the other MppBufferGroupImpl
    struct list_head    list_group;

    // link to list_status in MppBufferImpl
    struct list_head    list_used;
    struct list_head    list_unused;
};

#ifdef __cplusplus
extern "C" {
#endif

extern RK_U32 mpp_buffer_debug;

/*
 *  mpp_buffer_create       : create a unused buffer with parameter tag/size/data
 *                            if input buffer is NULL then buffer will be register to unused list
 *                            otherwise the buffer will be register to used list and set to paramter buffer
 *
 *  mpp_buffer_mmap         : The created mpp_buffer can not be accessed directly.
 *                            It required map to access. This is an optimization
 *                            for reducing virtual memory usage.
 *
 *  mpp_buffer_get_unused   : get unused buffer with size. it will first search
 *                            the unused list. if failed it will create on from
 *                            group allocator.
 *
 *  mpp_buffer_ref_inc      : increase buffer's reference counter. if it is unused
 *                            then it will be moved to used list.
 *
 *  mpp_buffer_ref_dec      : decrease buffer's reference counter. if the reference
 *                            reduce to zero buffer will be moved to unused list.
 *
 * normal call flow will be like this:
 *
 * mpp_buffer_create        - create a unused buffer
 * mpp_buffer_get_unused    - get the unused buffer
 * mpp_buffer_ref_inc/dec   - use the buffer
 * mpp_buffer_destory       - destroy the buffer
 */
MPP_RET mpp_buffer_create(const char *tag, const char *caller, MppBufferGroupImpl *group, MppBufferInfo *info, MppBufferImpl **buffer);
MPP_RET mpp_buffer_mmap(MppBufferImpl *buffer, const char* caller);
MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer, const char* caller);
MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer, const char* caller);
MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size);
RK_U32  mpp_buffer_to_addr(MppBuffer buffer, size_t offset);

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, const char *caller, MppBufferMode mode, MppBufferType type);
MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p);
MPP_RET mpp_buffer_group_reset(MppBufferGroupImpl *p);
MPP_RET mpp_buffer_group_set_callback(MppBufferGroupImpl *p,
                                      MppBufCallback callback, void *arg);
// mpp_buffer_group helper function
void mpp_buffer_group_dump(MppBufferGroupImpl *p);
void mpp_buffer_service_dump();
MppBufferGroupImpl *mpp_buffer_get_misc_group(MppBufferMode mode, MppBufferType type);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUFFER_IMPL_H__*/
