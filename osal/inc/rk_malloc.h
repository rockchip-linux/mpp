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

#ifndef __RK_MALLOC_H__
#define __RK_MALLOC_H__

#include <stdlib.h>
#include "rk_type.h"

#define rk_malloc_tagged(type, count, tag)  \
    (type*)rk_mpp_malloc(tag, sizeof(type) * (count))

#define rk_malloc(type, count)  \
    (type*)rk_mpp_malloc(MODULE_TAG, sizeof(type) * (count))

#define rk_free(ptr) rk_mpp_free(ptr)

#ifdef __cplusplus
extern "C" {
#endif

void rk_mpp_show_mem_status();
void *rk_mpp_malloc(char *tag, size_t size);
void rk_mpp_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /*__RK_MPP_MALLOC_H__*/

