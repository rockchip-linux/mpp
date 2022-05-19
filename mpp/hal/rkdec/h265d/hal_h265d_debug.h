/*
*
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

#ifndef __HAL_H265D_DEBUG_H__
#define __HAL_H265D_DEBUG_H__

#include "mpp_debug.h"

#define H265H_DBG_FUNCTION          (0x00000001)
#define H265H_DBG_RPS               (0x00000002)
#define H265H_DBG_PPS               (0x00000004)
#define H265H_DBG_REG               (0x00000008)
#define H265H_DBG_FAST_ERR          (0x00000010)
#define H265H_DBG_TASK_ERR          (0x00000020)

#define h265h_dbg(flag, fmt, ...) _mpp_dbg(hal_h265d_debug, flag, fmt, ## __VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

extern RK_U32 hal_h265d_debug;

#ifdef __cplusplus
}
#endif

#endif /*__HAL_H265D_DEBUG_H__*/
