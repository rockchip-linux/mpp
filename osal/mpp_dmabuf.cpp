/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/dma-buf.h>

#include "mpp_log.h"
#include "mpp_dmabuf.h"

/* SET_NAME and SYNC_PARTIAL are supported after 4.4 kernel */

/* Add dma buffer name uapi */
#ifndef DMA_BUF_SET_NAME
/* 32/64bitness of this uapi was botched in android, there's no difference
 * between them in actual uapi, they're just different numbers.
 */
#define DMA_BUF_SET_NAME                _IOW(DMA_BUF_BASE, 1, const char *)
#define DMA_BUF_SET_NAME_A              _IOW(DMA_BUF_BASE, 1, __u32)
#define DMA_BUF_SET_NAME_B              _IOW(DMA_BUF_BASE, 1, __u64)
#endif

/* Add dma buffer sync partial uapi */
#ifndef DMA_BUF_IOCTL_SYNC_PARTIAL
struct dma_buf_sync_partial {
    __u64 flags;
    __u32 offset;
    __u32 len;
};

#define DMA_BUF_IOCTL_SYNC_PARTIAL      _IOW(DMA_BUF_BASE, 2, struct dma_buf_sync_partial)
#endif

#define MPP_NO_PARTIAL_SUPPORT  25  /* ENOTTY */

static RK_U32 has_partial_ops = 1;

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

        sync.flags = DMA_BUF_SYNC_START | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);
        sync.offset = offset;
        sync.len = length;

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

        sync.flags = DMA_BUF_SYNC_END | (ro ? DMA_BUF_SYNC_READ : DMA_BUF_SYNC_RW);
        sync.offset = offset;
        sync.len = length;

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