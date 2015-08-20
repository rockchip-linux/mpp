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

#ifndef __OS_ALLOCATOR_H__
#define __OS_ALLOCATOR_H__

#include "mpp_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

int os_allocator_open(void **ctx, size_t alignment);
int os_allocator_alloc(void *ctx, MppBufferData *data, size_t size);
void os_allocator_free(void *ctx, MppBufferData *data);
void os_allocator_close(void *ctx);

#ifdef __cplusplus
}
#endif

#endif /*__OS_ALLOCATOR_H__*/


