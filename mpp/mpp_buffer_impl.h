/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
#include "mpp_buffer.h"

#define MPP_BUF_DBG_FUNCTION            (0x00000001)

#define mpp_buf_dbg(flag, fmt, ...)     mpp_dbg(mpp_buffer_debug, flag, fmt, ## __VA_ARGS__)

#define MPP_BUF_FUNCTION_ENTER()        mpp_buf_dbg(MPP_BUF_DBG_FUNCTION, "%s enter\n", __FUNCTION__)
#define MPP_BUF_FUNCTION_LEAVE()        mpp_buf_dbg(MPP_BUF_DBG_FUNCTION, "%s leave\n", __FUNCTION__)
#define MPP_BUF_FUNCTION_LEAVE_OK()     mpp_buf_dbg(MPP_BUF_DBG_FUNCTION, "%s success\n", __FUNCTION__)
#define MPP_BUF_FUNCTION_LEAVE_FAIL()   mpp_buf_dbg(MPP_BUF_DBG_FUNCTION, "%s failed\n", __FUNCTION__)

typedef struct MppBufferImpl_t          MppBufferImpl;
typedef struct MppBufferGroupImpl_t     MppBufferGroupImpl;

// use index instead of pointer to avoid invalid pointer
struct MppBufferImpl_t {
    char                tag[MPP_TAG_SIZE];
    RK_U32              group_id;
    size_t              size;
    MppBufferData       data;

    // used flag is for used/unused list detection
    RK_U32              used;
    RK_S32              ref_count;
    struct list_head    list_status;
};

struct MppBufferGroupImpl_t {
    char                tag[MPP_TAG_SIZE];
    RK_U32              group_id;
    MppBufferType       type;
    size_t              limit;
    size_t              usage;

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

MPP_RET mpp_buffer_init(MppBufferImpl **buffer, const char *tag, RK_U32 group_id, size_t size, MppBufferData *data);
MPP_RET mpp_buffer_deinit(MppBufferImpl *buffer);
MPP_RET mpp_buffer_ref_inc(MppBufferImpl *buffer);
MPP_RET mpp_buffer_ref_dec(MppBufferImpl *buffer);
// TODO: get unused is a block or wait function

MppBufferImpl *mpp_buffer_get_unused(MppBufferGroupImpl *p, size_t size);

MPP_RET mpp_buffer_group_init(MppBufferGroupImpl **group, const char *tag, MppBufferType type);
MPP_RET mpp_buffer_group_deinit(MppBufferGroupImpl *p);
MPP_RET mpp_buffer_group_limit_reset(MppBufferGroupImpl *p, size_t limit);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUFFER_IMPL_H__*/
