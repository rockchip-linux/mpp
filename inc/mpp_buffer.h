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

#ifndef __MPP_BUFFER_H__
#define __MPP_BUFFER_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef void* MppBuffer;
typedef void* MppBufferPool;

typedef struct {
    RK_U32  api_size;
    RK_U32  api_version;

    MPP_RET (*commit)(MppBufferPool pool, MppBuffer buffer);

    MPP_RET (*get_unused)(MppBufferPool pool, MppBuffer *buffer);
    MPP_RET (*ref_used)(MppBufferPool pool, MppBuffer buffer);
    MPP_RET (*unref_used)(MppBufferPool pool, MppBuffer buffer);

    MPP_RET (*reset)(MppBufferPool pool);
    RK_S32  (*get_unsed_num)(MppBufferPool pool);

    RK_U32 reserv[16];
} MppBufferPoolApi;


#ifdef __cplusplus
extern "C" {
#endif

/*
 * MppBufferPool interface
 */
MPP_RET mpp_buffer_init(MppBuffer *buffer, RK_U32 type, RK_U32 size);
MPP_RET mpp_buffer_deinit(MppBuffer *buffer);
MPP_RET mpp_buffer_inc_ref(MppBuffer buffer);
MPP_RET mpp_buffer_dec_ref(MppBuffer buffer);

/*
 * MppBufferPool interface
 */
MPP_RET mpp_buffer_pool_init(MppBufferPool *pool, RK_U32 count, RK_U32 size);
MPP_RET mpp_buffer_pool_get_api(MppBufferPool pool, MppBufferPoolApi *api);
MPP_RET mpp_buffer_pool_deinit(MppBufferPool *pool);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_BUFFER_H__*/
