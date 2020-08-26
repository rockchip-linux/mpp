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

#ifndef __MPP_EVENTFD_H__
#define __MPP_EVENTFD_H__

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

RK_S32 mpp_eventfd_get(RK_U32 init);
RK_S32 mpp_eventfd_put(RK_S32 fd);

RK_S32 mpp_eventfd_read(RK_S32 fd, RK_U64 *val, RK_S64 timeout);
RK_S32 mpp_eventfd_write(RK_S32 fd, RK_U64 val);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_EVENTFD_H__*/

