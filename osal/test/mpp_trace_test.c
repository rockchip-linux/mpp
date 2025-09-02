/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_trace_test"

#include "mpp_log.h"

#include "mpp_trace.h"

int main(void)
{
    mpp_log("mpp trace test start\n");

    mpp_trace_begin("mpp_trace_test");
    mpp_trace_end("mpp_trace_test");

    mpp_trace_async_begin("mpp_trace_test async", 10);
    mpp_trace_async_end("mpp_trace_test async", 10);

    mpp_trace_int32("mpp_trace_test int32", 256);
    mpp_trace_int64("mpp_trace_test int64", 100000000);

    mpp_log("mpp trace test done\n");

    return 0;
}
