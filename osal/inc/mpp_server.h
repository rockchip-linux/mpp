/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#ifndef __MPP_SERVER_H__
#define __MPP_SERVER_H__

#include "mpp_device.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET mpp_server_attach(MppDev ctx);
MPP_RET mpp_server_detach(MppDev ctx);

MPP_RET mpp_server_send_task(MppDev ctx);
MPP_RET mpp_server_wait_task(MppDev ctx, RK_S64 timeout);

#ifdef  __cplusplus
}
#endif

#endif /* __MPP_SERVER_H__ */
