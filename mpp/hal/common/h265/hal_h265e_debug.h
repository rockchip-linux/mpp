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

#ifndef __HAL_H265E_DEBUG_H__
#define __HAL_H265E_DEBUG_H__

#include "mpp_log.h"

#define HAL_H265E_DBG_FUNCTION          (0x00000001)
#define HAL_H265E_DBG_SIMPLE            (0x00000002)
#define HAL_H265E_DBG_FLOW              (0x00000004)
#define HAL_H265E_DBG_DETAIL            (0x00000008)

#define HAL_H265E_DBG_REGS              (0x00000010)
#define HAL_H265E_DBG_CTL_REGS          (0x00000020)
#define HAL_H265E_DBG_RCKUT_REGS        (0x00000040)
#define HAL_H265E_DBG_WGT_REGS          (0x00000080)
#define HAL_H265E_DBG_RDO_REGS          (0x000000C0)


#define HAL_H265E_DBG_INPUT             (0x00020000)
#define HAL_H265E_DBG_OUTPUT            (0x00040000)
#define HAL_H265E_DBG_WRITE_IN_STREAM   (0x00080000)
#define HAL_H265E_DBG_WRITE_OUT_STREAM  (0x00100000)

#define hal_h265e_dbg(flag, fmt, ...)   _mpp_dbg(hal_h265e_debug, flag, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_f(flag, fmt, ...) _mpp_dbg_f(hal_h265e_debug, flag, fmt, ## __VA_ARGS__)

#define hal_h265e_dbg_func(fmt, ...)    hal_h265e_dbg_f(HAL_H265E_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_simple(fmt, ...)  hal_h265e_dbg_f(HAL_H265E_DBG_SIMPLE, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_flow(fmt, ...)    hal_h265e_dbg(HAL_H265E_DBG_FLOW, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_detail(fmt, ...)  hal_h265e_dbg(HAL_H265E_DBG_DETAIL, fmt, ## __VA_ARGS__)

#define hal_h265e_dbg_regs(fmt, ...)    hal_h265e_dbg(HAL_H265E_DBG_REGS, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_ctl(fmt, ...)     hal_h265e_dbg(HAL_H265E_DBG_CTL_REGS, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_rckut(fmt, ...)   hal_h265e_dbg(HAL_H265E_DBG_RCKUT_REGS, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_wgt(fmt, ...)     hal_h265e_dbg(HAL_H265E_DBG_WGT_REGS, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_rdo(fmt, ...)     hal_h265e_dbg(HAL_H265E_DBG_RDO_REGS, fmt, ## __VA_ARGS__)

#define hal_h265e_dbg_input(fmt, ...)   hal_h265e_dbg(HAL_H265E_DBG_INPUT, fmt, ## __VA_ARGS__)
#define hal_h265e_dbg_output(fmt, ...)  hal_h265e_dbg(HAL_H265E_DBG_OUTPUT, fmt, ## __VA_ARGS__)

#define hal_h265e_enter()               hal_h265e_dbg_flow("(%d) enter\n", __LINE__);
#define hal_h265e_leave()               hal_h265e_dbg_flow("(%d) leave\n", __LINE__);

extern RK_U32 hal_h265e_debug;

#endif
