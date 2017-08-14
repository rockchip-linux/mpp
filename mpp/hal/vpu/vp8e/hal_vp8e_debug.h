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

#ifndef __HAL_VP8E_DEBUG_H__
#define __HAL_VP8E_DEBUG_H__

#include "mpp_log.h"

#define VP8E_DBG_HAL_FUNCTION           (0x00000001)
#define VP8E_DBG_HAL_REG                (0x00000002)
#define VP8E_DBG_HAL_DUMP_REG           (0x00000004)
#define VP8E_DBG_HAL_IRQ                (0x00000008)
#define VP8E_DBG_HAL_DUMP_IVF           (0x00000010)

#define VP8E_DBG(flag, fmt, ...)    _mpp_dbg(vp8e_hal_debug, flag, fmt, ## __VA_ARGS__)
#define VP8E_DBG_F(flag, fmt, ...)  _mpp_dbg_f(vp8e_hal_debug, flag, fmt, ## __VA_ARGS__)

#define vp8e_hal_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

#define vp8e_hal_dbg(type, fmt, ...) \
    do {\
        if (vp8e_hal_debug & type)\
            mpp_log(fmt, ## __VA_ARGS__);\
    } while (0)

#define vp8e_hal_enter() \
    do {\
        if (vp8e_hal_debug & VP8E_DBG_HAL_FUNCTION)\
            mpp_log("line(%d), func(%s), enter", __LINE__, __FUNCTION__);\
    } while (0)

#define vp8e_hal_leave() \
    do {\
        if (vp8e_hal_debug & VP8E_DBG_HAL_FUNCTION)\
            mpp_log("line(%d), func(%s), leave", __LINE__, __FUNCTION__);\
    } while (0)

extern RK_U32 vp8e_hal_debug;

#endif /*__HAL_VP8E_DEBUG_H__*/
