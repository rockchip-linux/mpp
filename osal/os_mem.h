/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __OS_MEM_H__
#define __OS_MEM_H__

#ifdef __cplusplus
extern "C" {
#endif

int os_malloc(void **memptr, size_t alignment, size_t size);
int os_realloc(void *src, void **dst, size_t alignment, size_t size);
void os_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /*__OS_MEM_H__*/


