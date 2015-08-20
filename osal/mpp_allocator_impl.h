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

#ifndef __MPP_ALLOCATOR_IMPL_H__
#define __MPP_ALLOCATOR_IMPL_H__

#include "mpp_thread.h"
#include "mpp_buffer.h"

typedef struct {
    pthread_mutex_t lock;
    MppBufferType   type;
    size_t          alignment;
    void            *allocator;
    void            *api;
} MppAllocatorImpl;

#endif /*__MPP_ALLOCATOR_IMPL_H__*/

