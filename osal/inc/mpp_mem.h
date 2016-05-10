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

#ifndef __MPP_MEM_H__
#define __MPP_MEM_H__

#include <stdlib.h>

#include "rk_type.h"
#include "mpp_err.h"

#define mpp_malloc_tagged(type, count, tag)  \
    (type*)mpp_osal_malloc(tag, sizeof(type) * (count))

#define mpp_malloc(type, count)  \
    (type*)mpp_osal_malloc(MODULE_TAG, sizeof(type) * (count))

#define mpp_malloc_size(type, size)  \
    (type*)mpp_osal_malloc(MODULE_TAG, size)

#define mpp_calloc_size(type, size)  \
    (type*)mpp_osal_calloc(MODULE_TAG, size)

#define mpp_calloc(type, count)  \
    (type*)mpp_osal_calloc(MODULE_TAG, sizeof(type) * (count))

#define mpp_realloc(ptr, type, count) \
    (type*)mpp_osal_realloc(MODULE_TAG, ptr, sizeof(type) * (count))

#define mpp_free(ptr) mpp_osal_free(ptr)

#define MPP_FREE(ptr)   do { if(ptr) mpp_free(ptr); ptr = NULL; } while (0)
#define MPP_FCLOSE(fp)  do { if(fp)  fclose(fp);     fp = NULL; } while (0)

#ifdef __cplusplus
extern "C" {
#endif

void *mpp_osal_malloc(const char *tag, size_t size);
void *mpp_osal_calloc(const char *tag, size_t size);
void *mpp_osal_realloc(const char *tag, void *ptr, size_t size);
void mpp_osal_free(void *ptr);

void mpp_show_mem_status();

/*
 * mpp memory usage snapshot tool
 *
 * usage:
 * call mpp_mem_get_snapshot on context init get one snapshot
 * call mpp_mem_get_snapshot on context deinit get another snapshot
 * call mpp_mem_diff_snapshot to show the difference between these two snapshot
 * call mpp_mem_put_snapshot twice to release these two snapshot
 */
typedef void* MppMemSnapshot;

MPP_RET mpp_mem_get_snapshot(MppMemSnapshot *hnd);
MPP_RET mpp_mem_put_snapshot(MppMemSnapshot *hnd);
MPP_RET mpp_mem_squash_snapshot(MppMemSnapshot hnd0, MppMemSnapshot hnd1);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_MEM_H__*/

