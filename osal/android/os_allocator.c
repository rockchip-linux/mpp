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

#include <stdlib.h>
#include <ion/ion.h>

#include "os_mem.h"
#include "os_allocator.h"

#include "mpp_mem.h"
#include "mpp_log.h"

typedef struct {
    RK_S32 ion_client;
    RK_U32 align;
} allocator_ion;

int os_allocator_open(void **ctx, size_t alignment)
{
    if (NULL == ctx) {
        mpp_err("os_allocator_open Android do not accept NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    allocator_ion *p = mpp_malloc(allocator_ion, 1);
    if (NULL == p) {
        *ctx = NULL;
        mpp_err("os_allocator_open Android failed to allocate context\n");
        return MPP_ERR_MALLOC;
    }
    p->ion_client =
    *ctx = p;
    return 0;
}

int os_allocator_alloc(void *ctx, MppBufferData *data, size_t alignment, size_t size)
{
    (void) ctx;
    return os_malloc(&data->ptr, alignment, size);
}

void os_allocator_free(void *ctx, MppBufferData *data)
{
    (void) ctx;
    os_free(data->ptr);
}

void os_allocator_close(void *ctx)
{
    (void) ctx;
}

