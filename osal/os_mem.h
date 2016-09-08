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


