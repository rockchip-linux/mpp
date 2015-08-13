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

#ifndef __MPP_MALLOC_H__
#define __MPP_MALLOC_H__

#include <stdlib.h>

#include "rk_type.h"

#define mpp_malloc_tagged(type, count, tag)  \
    (type*)mpp_osal_malloc(tag, sizeof(type) * (count))

#define mpp_malloc(type, count)  \
    (type*)mpp_osal_malloc(MODULE_TAG, sizeof(type) * (count))

#define mpp_realloc(ptr, type, count) \
    (type*)mpp_osal_realloc(MODULE_TAG, ptr, sizeof(type) * (count))

#define mpp_free(ptr) mpp_osal_free(ptr)

#ifdef __cplusplus
extern "C" {
#endif

void mpp_show_mem_status();
void *mpp_osal_malloc(char *tag, size_t size);
void *mpp_osal_realloc(char *tag, void *ptr, size_t size);
void mpp_osal_free(void *ptr);

#ifdef __cplusplus
}
#endif

// buffer manipulation function
#if defined(_WIN32)
#define snprintf            _snprintf
#endif

#endif /*__MPP_MALLOC_H__*/

