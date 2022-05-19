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

#ifndef __HAL_JPEGE_DEBUG_H__
#define __HAL_JPEGE_DEBUG_H__

#include "mpp_debug.h"

#define HAL_JPEGE_DBG_FUNCTION          (0x00000001)
#define HAL_JPEGE_DBG_SIMPLE            (0x00000002)
#define HAL_JPEGE_DBG_DETAIL            (0x00000004)
#define HAL_JPEGE_DBG_INPUT             (0x00000010)
#define HAL_JPEGE_DBG_OUTPUT            (0x00000020)

#define hal_jpege_dbg(flag, fmt, ...)   _mpp_dbg(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_f(flag, fmt, ...) _mpp_dbg_f(hal_jpege_debug, flag, fmt, ## __VA_ARGS__)

#define hal_jpege_dbg_func(fmt, ...)    hal_jpege_dbg_f(HAL_JPEGE_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_simple(fmt, ...)  hal_jpege_dbg(HAL_JPEGE_DBG_SIMPLE, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_detail(fmt, ...)  hal_jpege_dbg(HAL_JPEGE_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_input(fmt, ...)   hal_jpege_dbg(HAL_JPEGE_DBG_INPUT, fmt, ## __VA_ARGS__)
#define hal_jpege_dbg_output(fmt, ...)  hal_jpege_dbg(HAL_JPEGE_DBG_OUTPUT, fmt, ## __VA_ARGS__)

extern RK_U32 hal_jpege_debug;

#endif /* __HAL_JPEGE_DEBUG_H__ */
