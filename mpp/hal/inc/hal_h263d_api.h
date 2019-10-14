/*
 *
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H263D_API_H__
#define __HAL_H263D_API_H__

#include "mpp_hal.h"

#define H263D_HAL_DBG_REG_PUT       (0x00000001)
#define H263D_HAL_DBG_REG_GET       (0x00000002)

#ifdef __cplusplus
extern "C" {
#endif

extern RK_U32 h263d_hal_debug;

extern const MppHalApi hal_api_h263d;

#ifdef __cplusplus
}
#endif

#endif /*__HAL_H263D_API_H__*/

