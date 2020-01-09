/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_BUFS_H__
#define __HAL_BUFS_H__

#include "mpp_buffer.h"

/* HalBuf buffer set allocater */
typedef struct HalBuf_t {
    RK_U32      cnt;
    MppBuffer   *buf;
} HalBuf;

typedef void* HalBufs;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET hal_bufs_init(HalBufs *bufs);
MPP_RET hal_bufs_deinit(HalBufs bufs);

MPP_RET hal_bufs_setup(HalBufs bufs, RK_S32 max_cnt, RK_S32 size_cnt, size_t sizes[]);
HalBuf *hal_bufs_get_buf(HalBufs bufs, RK_S32 buf_idx);

#ifdef __cplusplus
}
#endif

#endif
