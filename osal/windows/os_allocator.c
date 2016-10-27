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

#if defined(_WIN32)
#include "os_allocator.h"
#include "allocator_std.h"

/*
 * window only support MPP_BUFFER_TYPE_NORMAL
 */

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type)
{
    MPP_RET ret = MPP_OK;
    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
    case MPP_BUFFER_TYPE_ION :
    case MPP_BUFFER_TYPE_V4L2 : {
        *api = allocator_std;
    } break;
    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

#endif
