/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_DEVICE_MSG_H__
#define __MPP_DEVICE_MSG_H__

#include "rk_type.h"

typedef struct MppDevReqV1_t {
    RK_U32 cmd;
    RK_U32 flag;
    RK_U32 size;
    RK_U32 offset;
    void   *data;
} MppDevReqV1;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_device_add_request(MppDevCtx ctx, MppDevReqV1 *req);
MPP_RET mpp_device_send_request(MppDevCtx ctx);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEVICE_MSG_H__ */
