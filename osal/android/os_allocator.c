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

#if defined(__ANDROID__)
#include "allocator_drm.h"
#include "allocator_ext_dma.h"
#include "allocator_ion.h"
#include "allocator_std.h"
#include "mpp_runtime.h"

MPP_RET os_allocator_get(os_allocator *api, MppBufferType type)
{
    MPP_RET ret = MPP_OK;

    switch (type) {
    case MPP_BUFFER_TYPE_NORMAL :
        *api = allocator_std;
    case MPP_BUFFER_TYPE_ION : {
        *api = (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION)) ? allocator_ion :
#if HAVE_DRM
               (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM)) ? allocator_drm :
#endif
               allocator_std;
    } break;
    case MPP_BUFFER_TYPE_EXT_DMA: {
        *api = allocator_ext_dma;
    } break;
    case MPP_BUFFER_TYPE_DRM : {
#if HAVE_DRM
        *api = (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM)) ? allocator_drm :
#else
        * api =
#endif
               (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION)) ? allocator_ion :
               allocator_std;
    } break;
    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}
#endif
