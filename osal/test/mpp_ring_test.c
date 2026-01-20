/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_ring_test"

#include <string.h>

#include "mpp_log.h"
#include "mpp_ring.h"

static rk_s32 test_ring_ptr(rk_u8 *ptr, rk_s32 size)
{
    mpp_logi("ptr: [0] %02x [%d-1] %02x [%d] %02x [2*%d-1] %02x\n",
             ptr[0], size, ptr[size - 1], size, ptr[size], size, ptr[2 * size - 1]);

    if (ptr[0] != ptr[size] || ptr[size - 1] != ptr[2 * size - 1]) {
        mpp_loge("failed to get real %d ring buffer\n", size);
        return rk_nok;
    }

    return rk_ok;
}

int main(void)
{
    MppRing ring = NULL;
    rk_u8 *ptr;
    rk_s32 size = 4096;
    rk_s32 ret;

    mpp_logi("start\n");

    do {
        ret = mpp_ring_get(&ring, size, MODULE_TAG);
        if (ret != rk_ok || ring == NULL) {
            mpp_loge("failed to get ring buffer size %d\n", size);
            break;
        }

        ret = rk_nok;

        ptr = (rk_u8 *)mpp_ring_get_ptr(ring);
        if (ptr == NULL) {
            mpp_loge("failed to get ring buffer ptr\n");
            break;
        }

        mpp_logi("ring buffer %p get size %d ptr %p\n", ring, size, ptr);

        mpp_logi("memset ptr 0 ~ size to 1\n");
        memset(ptr, 1, size);

        if (test_ring_ptr(ptr, size) != rk_ok) {
            mpp_loge("failed to get real %d ring buffer\n", size);
            break;
        }

        mpp_logi("memset ptr %d ~ 2*%d to 2\n", size, size);
        memset(ptr + size, 2, size);

        if (test_ring_ptr(ptr, size) != rk_ok) {
            mpp_loge("failed to get real %d ring buffer\n", size);
            break;
        }

        mpp_logi("resize ring buffer size %d -> %d\n", size, size * 2);

        size *= 2;
        ptr = mpp_ring_resize(ring, size);

        mpp_logi("resize ring buffer size %d ptr %p\n", size, ptr);

        mpp_logi("memset ptr 0 ~ %d-1 to 3\n", size);
        memset(ptr, 3, size);

        if (test_ring_ptr(ptr, size) != rk_ok) {
            mpp_loge("failed to get real %d ring buffer\n", size);
            break;
        }

        mpp_logi("memset ptr %d ~ 2*%d to 4\n", size, size);
        memset(ptr + size, 4, size);

        if (test_ring_ptr(ptr, size) != rk_ok) {
            mpp_loge("failed to get real %d ring buffer\n", size);
            break;
        }

        ret = mpp_ring_put(ring);
        ring = NULL;
    } while (0);

    mpp_logi("done %s\n", ret == 0 ? "ok" : "failed");

    if (ring) {
        mpp_ring_put(ring);
        ring = NULL;
    }

    return ret;
}
