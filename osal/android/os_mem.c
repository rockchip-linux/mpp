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

#if defined(__ANDROID__)
#include <stdlib.h>
#include "os_mem.h"

int os_malloc(void **memptr, size_t alignment, size_t size)
{
    (void)alignment;
    int ret = 0;
    void *ptr = malloc(size);

    if (ptr) {
        *memptr = ptr;
    } else {
        *memptr = NULL;
        ret = -1;
    }
    return ret;

    //return posix_memalign(memptr, alignment, size);
}

int os_realloc(void *src, void **dst, size_t alignment, size_t size)
{
    (void)alignment;
    *dst = realloc(src, size);
    return (*dst) ? (0) : (-1);
}

void os_free(void *ptr)
{
    free(ptr);
}
#endif
