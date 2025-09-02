/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_dmabuf.h"
#include "mpp_singleton.h"
#include "linux/dma-buf.h"

#define MPP_NO_PARTIAL_SUPPORT  25  /* ENOTTY */
#define CACHE_LINE_SIZE  64

static RK_U32 has_partial_ops = 0;

static void mpp_dmabuf_init(void)
{
    /*
     * update has_partial_ops by env
     * NOTE: When dmaheap is enabled the dmaheap fd partial ops is fine.
     * But the drm fd partial ops may have error when kernel version above 4.19
     * So we provide the mpp_dmabuf_has_partial_ops env to disable partial ops.
     */
    mpp_env_get_u32("mpp_dmabuf_has_partial_ops", &has_partial_ops, has_partial_ops);
}

MPP_SINGLETON(MPP_SGLN_DMABUF, mpp_dmabuf, mpp_dmabuf_init, NULL);

MPP_RET mpp_dmabuf_sync_begin(RK_S32 fd, RK_S32 ro, const char *caller)
{
    struct dma_buf_sync sync;
    RK_S32 ret;

    sync.flags = DMA_BUF_SYNC_START | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);

    ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret) {
        mpp_err_f("ioctl failed for %s from %s\n", strerror(errno), caller);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_dmabuf_sync_end(RK_S32 fd, RK_S32 ro, const char *caller)
{
    struct dma_buf_sync sync;
    RK_S32 ret;

    sync.flags = DMA_BUF_SYNC_END | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);

    ret = ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
    if (ret) {
        mpp_err_f("ioctl failed for %s from %s\n", strerror(errno), caller);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_dmabuf_sync_partial_begin(RK_S32 fd, RK_S32 ro, RK_U32 offset, RK_U32 length, const char *caller)
{
    if (has_partial_ops) {
        struct dma_buf_sync_partial sync;
        RK_S32 ret;

        if (!length)
            return MPP_OK;

        sync.flags = DMA_BUF_SYNC_START | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);
        sync.offset = MPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
        sync.len = MPP_ALIGN(length + offset - sync.offset, CACHE_LINE_SIZE);

        ret = ioctl(fd, DMA_BUF_IOCTL_SYNC_PARTIAL, &sync);
        if (ret) {
            if (errno == MPP_NO_PARTIAL_SUPPORT) {
                has_partial_ops = 0;
                goto NOT_SUPPORT;
            }

            mpp_err_f("ioctl failed for %s from %s\n", strerror(errno), caller);
            return MPP_NOK;
        }

        return MPP_OK;
    }

NOT_SUPPORT:
    return mpp_dmabuf_sync_begin(fd, ro, caller);
}

MPP_RET mpp_dmabuf_sync_partial_end(RK_S32 fd, RK_S32 ro, RK_U32 offset, RK_U32 length, const char *caller)
{
    if (has_partial_ops) {
        struct dma_buf_sync_partial sync;
        RK_S32 ret;

        if (!length)
            return MPP_OK;

        sync.flags = DMA_BUF_SYNC_END | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);
        sync.offset = MPP_ALIGN_DOWN(offset, CACHE_LINE_SIZE);
        sync.len = MPP_ALIGN(length + offset - sync.offset, CACHE_LINE_SIZE);

        ret = ioctl(fd, DMA_BUF_IOCTL_SYNC_PARTIAL, &sync);
        if (ret) {
            if (errno == MPP_NO_PARTIAL_SUPPORT) {
                has_partial_ops = 0;
                goto NOT_SUPPORT;
            }

            mpp_err_f("ioctl failed for %s from %s\n", strerror(errno), caller);
            return MPP_NOK;
        }

        return MPP_OK;
    }

NOT_SUPPORT:
    return mpp_dmabuf_sync_end(fd, ro, caller);
}

MPP_RET mpp_dmabuf_set_name(RK_S32 fd, const char *name, const char *caller)
{
    RK_S32 ret = ioctl(fd, DMA_BUF_SET_NAME, name);
    if (ret) {
        mpp_err_f("ioctl failed for %s from %s\n", strerror(errno), caller);
        return MPP_NOK;
    }

    return MPP_OK;
}

RK_U32 mpp_dmabuf_sync_partial_support(void)
{
    return has_partial_ops;
}
