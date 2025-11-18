/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_MEM_H
#define MPP_MEM_H

#include <stdlib.h>

#include "rk_type.h"
#include "mpp_err.h"

#define mpp_malloc_with_caller(type, count, caller)  \
    (type*)mpp_osal_malloc(caller, sizeof(type) * (count))

#define mpp_malloc(type, count)  \
    (type*)mpp_osal_malloc(__FUNCTION__, sizeof(type) * (count))

#define mpp_malloc_size(type, size)  \
    (type*)mpp_osal_malloc(__FUNCTION__, size)

#define mpp_calloc(type, count)  \
    (type*)mpp_osal_calloc(__FUNCTION__, sizeof(type) * (count))

#define mpp_calloc_size(type, size)  \
    (type*)mpp_osal_calloc(__FUNCTION__, size)

#define mpp_realloc(ptr, type, count) \
    (type*)mpp_osal_realloc(__FUNCTION__, ptr, sizeof(type) * (count))

#define mpp_realloc_size(ptr, type, size) \
    (type*)mpp_osal_realloc(__FUNCTION__, ptr, size)

#define mpp_free(ptr) \
    mpp_osal_free(__FUNCTION__, ptr)

#define MPP_FREE(ptr)   do { if(ptr) mpp_free(ptr); ptr = NULL; } while (0)
#define MPP_FCLOSE(fp)  do { if(fp)  fclose(fp);     fp = NULL; } while (0)

#ifdef __cplusplus
extern "C" {
#endif

void *mpp_osal_malloc(const char *caller, size_t size);
void *mpp_osal_calloc(const char *caller, size_t size);
void *mpp_osal_realloc(const char *caller, void *ptr, size_t size);
void mpp_osal_free(const char *caller, void *ptr);

void mpp_show_mem_status(void);
rk_u32 mpp_mem_total_now(void);
rk_u32 mpp_mem_total_max(void);

#ifdef __cplusplus
}
#endif

#endif /* MPP_MEM_H */

