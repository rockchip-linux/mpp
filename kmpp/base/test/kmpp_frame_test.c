/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame_test"

#include "mpp_debug.h"
#include "kmpp_frame.h"

#define KMPP_FRAME_TEST_SIZE    1024

#define TEST_CHECK(ret, func, ...) \
    do { \
        ret = func(__VA_ARGS__); \
        if (ret) { \
            mpp_err(MODULE_TAG " %s failed ret %d\n", #func, ret); \
            goto failed; \
        } \
    } while (0)

int main()
{
    KmppFrame frame = NULL;
    rk_u32 width = 1920;
    rk_u32 height = 1080;
    rk_u32 val = 0;
    rk_s32 ret = rk_ok;

    mpp_log(MODULE_TAG " start\n");

    TEST_CHECK(ret, kmpp_frame_get, &frame);
    TEST_CHECK(ret, kmpp_frame_set_width, frame, width);
    TEST_CHECK(ret, kmpp_frame_set_height, frame, height);

    kmpp_frame_dump(frame, "test");

    TEST_CHECK(ret, kmpp_frame_get_width, frame, &val);
    mpp_assert(val == width);
    TEST_CHECK(ret, kmpp_frame_get_height, frame, &val);
    mpp_assert(val == height);

failed:
    kmpp_frame_put(frame);

    mpp_log(MODULE_TAG " %s\n", ret ? "failed" : "success");
    return ret;
}
