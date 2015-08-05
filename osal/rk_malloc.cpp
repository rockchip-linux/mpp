/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#define LOG_TAG "rk_malloc"
#include "rk_type.h"
#include "rk_log.h"
#include "rk_env.h"
#include "rk_malloc.h"

#include "os_malloc.h"

// default memory align size is set to 64
static size_t rk_mpp_mem_align  = 64;
static RK_S32 mem_protect_flag  = -1;

static inline RK_S32 mpp_get_mem_proctect_flag()
{
    if (mem_protect_flag < 0) {
        rk_get_env_u32("mpp_protect", (RK_U32 *)&mem_protect_flag, 0);
    }
    return mem_protect_flag;
}

void rk_mpp_set_mem_align(size_t size)
{
    /* determine align size is power of 2 */
    if (size && !(size & (size - 1)))
        rk_mpp_mem_align = size;
    else
        rk_log("set memalign to %d failed\n", size);
}

void *rk_mpp_malloc(size_t size)
{
    void *ptr;
    if (0 == os_malloc(&ptr, rk_mpp_mem_align, size))
        return ptr;
    else
        return NULL;
}

void rk_mpp_free(void *ptr)
{
    os_free(ptr);
}

