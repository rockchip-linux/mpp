/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_mem_test"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_mem.h"

// TODO: need to add pressure test case and parameter scan case

int main(void)
{
    void *tmp = NULL;

    tmp = mpp_calloc(int, 100);
    if (tmp) {
        mpp_log("calloc  success ptr 0x%p\n", tmp);
    } else {
        mpp_log("calloc  failed\n");
    }
    if (tmp) {
        tmp = mpp_realloc(tmp, int, 200);
        if (tmp) {
            mpp_log("realloc success ptr 0x%p\n", tmp);
        } else {
            mpp_log("realloc failed\n");
        }
    }
    mpp_free(tmp);
    mpp_log("mpp_mem_test done\n");

    return 0;
}
