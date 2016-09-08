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
#ifndef _VPU_MEM_H_
#define _VPU_MEM_H_

#include "vpu_api.h"

typedef struct vpu_display_mem_pool_impl {
    vpu_display_mem_pool_FIELDS
    MppBufferGroup group;
    RK_S32 size;
} vpu_display_mem_pool_impl;

#endif //_VPU_MEM_H_