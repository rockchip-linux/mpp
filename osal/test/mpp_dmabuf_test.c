/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_dmabuf_test"

#include <string.h>
#include <errno.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_dmabuf.h"

int main()
{
    MppBufferGroup grp = NULL;
    MppBuffer buf = NULL;
    RK_S32 fd = -1;
    RK_U8 *ptr = NULL;
    MPP_RET ret = MPP_NOK;

    mpp_logi("mpp dmabuf test start\n");

    do {
        ret = mpp_buffer_group_get_internal(&grp, MPP_BUFFER_TYPE_DMA_HEAP | MPP_BUFFER_FLAGS_CACHABLE);
        if (ret) {
            mpp_loge("get dmaheap buffer group failed ret %d\n", ret);
            break;
        }

        ret = mpp_buffer_get(grp, &buf, SZ_1M);
        if (ret) {
            mpp_loge("get 1M dmaheap buffer failed ret %d\n", ret);
            break;
        }

        fd = mpp_buffer_get_fd(buf);
        ptr = mpp_buffer_get_ptr(buf);

        ret = mpp_dmabuf_sync_begin(fd, 0, MODULE_TAG);
        if (ret) {
            mpp_loge("get dmaheap buffer sync begin failed ret %d\n", ret);
            break;
        }

        ptr[0] = 'a';
        ptr[1] = 'b';
        ptr[2] = 'c';
        ptr[3] = 'd';

        mpp_logi("ioctl begin cpu access         success\n");

        ret = mpp_dmabuf_sync_end(fd, 0, MODULE_TAG);
        if (ret) {
            mpp_loge("get dmaheap buffer sync begin failed ret %d\n", ret);
            break;
        }

        mpp_logi("ioctl end cpu access           success\n");

        ret = mpp_dmabuf_sync_partial_begin(fd, 0, 0, SZ_512K, MODULE_TAG);
        mpp_logi("ioctl begin cpu access partial %s\n", ret ? "failed" : "success");

        ret = mpp_dmabuf_sync_partial_end(fd, 0, 0, SZ_512K, MODULE_TAG);
        mpp_logi("ioctl end cpu access partial   %s\n", ret ? "failed" : "success");

        ret = mpp_dmabuf_set_name(fd, "dmabuf_test", MODULE_TAG);
        mpp_logi("ioctl set name                 %s\n", ret ? "failed" : "success");

        ret = mpp_dmabuf_sync_partial_support();
        mpp_logi("ioctl sync partial             %s\n", ret ? "YES" : "NO");

        ret = MPP_OK;
    } while (0);

    if (buf) {
        mpp_buffer_put(buf);
        buf = NULL;
    }

    if (grp) {
        mpp_buffer_group_put(grp);
        grp = NULL;
    }

    mpp_logi("mpp dmabuf test done %s\n", ret ? "failed" : "success");

    return 0;
}
