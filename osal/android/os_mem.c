/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
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
