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

#define MODULE_TAG "mpp_mem_test"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_mem.h"

// TODO: need to add pressure test case and parameter scan case

int main()
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
