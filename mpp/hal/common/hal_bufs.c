/*
 * Copyright 2015 - 2017 Rockchip Electronics Co. LTD
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
#define MODULE_TAG "hal_bufs"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_bufs.h"

#define HAL_BUFS_DBG_FUNCTION           (0x00000001)

#define hal_bufs_dbg(flag, fmt, ...)    _mpp_dbg(hal_bufs_debug, flag, fmt, ## __VA_ARGS__)
#define hal_bufs_dbg_f(flag, fmt, ...)  _mpp_dbg_f(hal_bufs_debug, flag, fmt, ## __VA_ARGS__)

#define hal_bufs_dbg_func(fmt, ...)     hal_bufs_dbg_f(HAL_BUFS_DBG_FUNCTION, fmt, ## __VA_ARGS__)

#define hal_bufs_enter()                hal_bufs_dbg_func("enter\n");
#define hal_bufs_leave()                hal_bufs_dbg_func("leave\n");

#define MAX_HAL_BUFS_CNT                40
#define MAX_HAL_BUFS_SIZE_CNT           8

typedef struct HalBufsImpl_t {
    MppBufferGroup  group;

    RK_S32          max_cnt;
    RK_S32          size_cnt;
    RK_S32          size_sum;
    RK_S32          elem_size;

    RK_U32          valid;
    size_t          sizes[MAX_HAL_BUFS_SIZE_CNT];
    RK_U8           *bufs;
} HalBufsImpl;

static RK_U32 hal_bufs_debug = 0;

static HalBuf *hal_bufs_pos(HalBufsImpl *impl, RK_S32 idx)
{
    RK_S32 elem_size = impl->elem_size;

    return (HalBuf *)(impl->bufs + idx * elem_size);
}

static MPP_RET hal_bufs_clear(HalBufsImpl *impl)
{
    MPP_RET ret = MPP_OK;

    if (impl->valid && impl->size_sum) {
        RK_S32 i;

        for (i = 0; i < impl->max_cnt; i++) {
            RK_U32 mask = 1 << i;

            if (impl->valid & mask) {
                HalBuf *buf = hal_bufs_pos(impl, i);
                RK_S32 j = 0;

                for (j = 0; j < impl->size_cnt; j++) {
                    if (buf->buf[j]) {
                        ret |= mpp_buffer_put(buf->buf[j]);
                        buf->buf[j] = NULL;
                    }
                }
                impl->valid &= ~mask;
            }
        }

        mpp_assert(impl->valid == 0);
    }

    impl->max_cnt = 0;
    impl->size_cnt = 0;
    impl->size_sum = 0;
    impl->valid = 0;
    memset(impl->sizes, 0, sizeof(impl->sizes));
    MPP_FREE(impl->bufs);

    return ret;
}

MPP_RET hal_bufs_init(HalBufs *bufs)
{
    MPP_RET ret = MPP_OK;

    if (NULL == bufs) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("hal_bufs_debug", &hal_bufs_debug, 0);

    hal_bufs_enter();

    HalBufsImpl *impl = mpp_calloc(HalBufsImpl, 1);
    if (impl) {
        ret = mpp_buffer_group_get_internal(&impl->group, MPP_BUFFER_TYPE_ION);
    } else {
        mpp_err_f("failed to malloc HalBufs\n");
        ret = MPP_ERR_MALLOC;
    }

    *bufs = impl;

    hal_bufs_leave();

    return ret;
}

MPP_RET hal_bufs_deinit(HalBufs bufs)
{
    HalBufsImpl *impl = (HalBufsImpl *)bufs;
    MPP_RET ret = MPP_OK;

    if (NULL == bufs) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    hal_bufs_enter();

    ret = hal_bufs_clear(impl);

    if (impl->group) {
        ret |= mpp_buffer_group_put(impl->group);
        impl->group = NULL;
    }

    memset(impl, 0, sizeof(*impl));
    MPP_FREE(impl);

    hal_bufs_leave();

    return ret;
}

MPP_RET hal_bufs_setup(HalBufs bufs, RK_S32 max_cnt, RK_S32 size_cnt, size_t sizes[])
{
    HalBufsImpl *impl = (HalBufsImpl *)bufs;
    MPP_RET ret = MPP_OK;
    RK_S32 elem_size = 0;
    RK_S32 impl_size = 0;

    if (NULL == bufs || NULL == sizes) {
        mpp_err_f("invalid NULL input bufs %p sizes %p\n", bufs, sizes);
        return MPP_ERR_NULL_PTR;
    }

    if (max_cnt <= 0 || max_cnt > MAX_HAL_BUFS_CNT ||
        size_cnt <= 0 || size_cnt > MAX_HAL_BUFS_SIZE_CNT) {
        mpp_err_f("invalid max cnt %d size cnt %d\n", max_cnt, size_cnt);
        return MPP_ERR_VALUE;
    }

    hal_bufs_enter();

    hal_bufs_clear(impl);

    if (impl->group)
        ret = mpp_buffer_group_clear(impl->group);
    else
        ret = mpp_buffer_group_get_internal(&impl->group, MPP_BUFFER_TYPE_ION);

    mpp_assert(impl->group);

    elem_size = sizeof(HalBuf) + sizeof(MppBuffer) * size_cnt;
    impl_size = elem_size * max_cnt;

    impl->elem_size = elem_size;
    impl->bufs = mpp_calloc_size(void, impl_size);
    if (impl->bufs) {
        RK_S32 size_sum = 0;
        RK_S32 i;

        for (i = 0; i < size_cnt; i++) {
            size_sum += sizes[i];
            impl->sizes[i] = sizes[i];
        }

        impl->size_sum = size_sum;

        for (i = 0; i < max_cnt; i++) {
            HalBuf *buf = hal_bufs_pos(impl, i);

            buf->cnt = size_cnt;
            buf->buf = (MppBuffer)(buf + 1);
        }

        impl->max_cnt = max_cnt;
        impl->size_cnt = size_cnt;
    } else {
        mpp_err_f("failed to malloc size %d for impl\n", impl_size);
        ret = MPP_ERR_MALLOC;
    }

    hal_bufs_leave();

    return ret;
}

HalBuf *hal_bufs_get_buf(HalBufs bufs, RK_S32 buf_idx)
{
    HalBufsImpl *impl = (HalBufsImpl *)bufs;

    if (NULL == impl || buf_idx < 0 || buf_idx >= impl->max_cnt) {
        mpp_err_f("invalid input impl %p buf_idx %d\n", impl, buf_idx);
        return NULL;
    }

    hal_bufs_enter();

    HalBuf *hal_buf = hal_bufs_pos(impl, buf_idx);
    RK_U32 mask = 1 << buf_idx;

    if (!(impl->valid & mask)) {
        MppBufferGroup group = impl->group;
        RK_S32 i;

        for (i = 0; i < impl->size_cnt; i++) {
            size_t size = impl->sizes[i];
            MppBuffer buf = hal_buf->buf[i];

            if (size && NULL == buf)
                mpp_buffer_get(group, &buf, size);

            mpp_assert(buf);
            hal_buf->buf[i] = buf;
        }

        impl->valid |= mask;
    }

    hal_bufs_leave();

    return hal_buf;
}
