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

#if defined(_WIN32)
#include <malloc.h>
#include "os_mem.h"

int os_malloc(void **memptr, size_t alignment, size_t size)
{
    *memptr = _aligned_malloc(size, alignment);
    return (*memptr) ? (0) : (-1);
}

int os_realloc(void *src, void **dst, size_t alignment, size_t size)
{
    *dst = _aligned_realloc(src, size, alignment);
    return (*dst) ? (0) : (-1);
}

void os_free(void *ptr)
{
    _aligned_free(ptr);
}

#endif
