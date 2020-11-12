/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __HAL_VP9D_DEBUG_H__
#define __HAL_VP9D_DEBUG_H__

#include "mpp_log.h"

#define HAL_VP9D_DBG_FUNC               (0x00000001)
#define HAL_VP9D_DBG_PAR                (0x00000002)
#define HAL_VP9D_DBG_REG                (0x00000004)

#define hal_vp9d_dbg(flag, fmt, ...)    _mpp_dbg(hal_vp9d_debug, flag, fmt, ## __VA_ARGS__)
#define hal_vp9d_dbg_f(flag, fmt, ...)  _mpp_dbg_f(hal_vp9d_debug, flag, fmt, ## __VA_ARGS__)

#define hal_vp9d_dbg_func(fmt, ...)     hal_vp9d_dbg_f(HAL_VP9D_DBG_FUNC, fmt, ## __VA_ARGS__)
#define hal_vp9d_dbg_par(fmt, ...)      hal_vp9d_dbg(HAL_VP9D_DBG_PAR, fmt, ## __VA_ARGS__)
#define hal_vp9d_dbg_reg(fmt, ...)      hal_vp9d_dbg(HAL_VP9D_DBG_REG, fmt, ## __VA_ARGS__)

#define hal_vp9d_enter()                hal_vp9d_dbg_func("(%d) enter\n", __LINE__);
#define hal_vp9d_leave()                hal_vp9d_dbg_func("(%d) leave\n", __LINE__);

extern RK_U32 hal_vp9d_debug;

#endif /* __HAL_VP9D_DEBUG_H__ */
