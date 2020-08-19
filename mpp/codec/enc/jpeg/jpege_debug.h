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

#ifndef __JPEGE_DEBUG_H__
#define __JPEGE_DEBUG_H__

#include "mpp_log.h"

#define JPEGE_DBG_FUNCTION          (0x00000001)
#define JPEGE_DBG_INPUT             (0x00000010)
#define JPEGE_DBG_OUTPUT            (0x00000020)
#define JPEGE_DBG_CTRL              (0x00000040)

#define jpege_dbg(flag, fmt, ...)   _mpp_dbg(jpege_debug, flag, fmt, ## __VA_ARGS__)
#define jpege_dbg_f(flag, fmt, ...) _mpp_dbg_f(jpege_debug, flag, fmt, ## __VA_ARGS__)

#define jpege_dbg_func(fmt, ...)    jpege_dbg_f(JPEGE_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define jpege_dbg_input(fmt, ...)   jpege_dbg(JPEGE_DBG_INPUT, fmt, ## __VA_ARGS__)
#define jpege_dbg_output(fmt, ...)  jpege_dbg(JPEGE_DBG_OUTPUT, fmt, ## __VA_ARGS__)
#define jpege_dbg_ctrl(fmt, ...)    jpege_dbg(JPEGE_DBG_CTRL, fmt, ## __VA_ARGS__)

extern RK_U32 jpege_debug;

#endif /* __JPEGE_DEBUG_H__ */
