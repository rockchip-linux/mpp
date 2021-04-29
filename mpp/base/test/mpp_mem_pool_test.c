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

#define MODULE_TAG "mpp_mem_pool_test"

#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_mem_pool.h"

#define MPP_MEM_POOL_TEST_SIZE      1024
#define MPP_MEM_POOL_TEST_COUNT     20

int main()
{
    MppMemPool pool = NULL;
    size_t size = MPP_MEM_POOL_TEST_SIZE;
    void *p [MPP_MEM_POOL_TEST_COUNT];
    RK_U32 i;

    mpp_log("mpp_mem_pool_test start\n");

    pool = mpp_mem_pool_init(size);
    if (NULL == pool) {
        mpp_err("mpp_mem_pool_test mpp_mem_pool_init failed\n");
        goto mpp_mem_pool_test_failed;
    }

    for (i = 0; i < MPP_MEM_POOL_TEST_COUNT; i++) {
        p[i] = mpp_mem_pool_get(pool);
        if (!p[i]) {
            mpp_err("mpp_mem_pool_test mpp_mem_pool_get failed\n");
            goto mpp_mem_pool_test_failed;
        }
    }

    for (i = 0; i < MPP_MEM_POOL_TEST_COUNT / 2; i++) {
        if (p[i]) {
            mpp_mem_pool_put(pool, p[i]);
            p[i] = NULL;
        }
    }

    for (i = 0; i < MPP_MEM_POOL_TEST_COUNT / 4; i++) {
        p[i] = mpp_mem_pool_get(pool);
        if (!p[i]) {
            mpp_err("mpp_mem_pool_test mpp_mem_pool_get failed\n");
            goto mpp_mem_pool_test_failed;
        }
    }

    for (i = 0; i < MPP_MEM_POOL_TEST_COUNT; i++) {
        if (p[i]) {
            mpp_mem_pool_put(pool, p[i]);
            p[i] = NULL;
        }
    }

    mpp_log("mpp_mem_pool_test success\n");
    return MPP_OK;

mpp_mem_pool_test_failed:
    mpp_log("mpp_mem_pool_test failed\n");
    return MPP_NOK;
}

