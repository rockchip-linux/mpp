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

#ifndef __VP8E_DEBUG_H__
#define __VP8E_DEBUG_H__

#include "rk_type.h"

#include "mpp_log.h"

#define VP8E_DBG_RC_FUNCTION            (0x00010000)
#define VP8E_DBG_RC_BPS                 (0x00020000)
#define VP8E_DBG_RC                     (0x00040000)
#define VP8E_DBG_RC_CFG                 (0x00080000)

#define VP8E_DBG(flag, fmt, ...)    _mpp_dbg(vp8e_rc_debug, flag, fmt, ## __VA_ARGS__)
#define VP8E_DBG_F(flag, fmt, ...)  _mpp_dbg_f(vp8e_rc_debug, flag, fmt, ## __VA_ARGS__)


#define vp8e_rc_dbg_func(fmt, ...)    VP8E_DBG_F(VP8E_DBG_RC_FUNCTION, fmt, ## __VA_ARGS__)
#define vp8e_rc_dbg_bps(fmt, ...)     VP8E_DBG(VP8E_DBG_RC_BPS, fmt, ## __VA_ARGS__)
#define vp8e_rc_dbg_rc(fmt, ...)      VP8E_DBG(VP8E_DBG_RC, fmt, ## __VA_ARGS__)
#define vp8e_rc_dbg_cfg(fmt, ...)     VP8E_DBG(VP8E_DBG_RC_CFG, fmt, ## __VA_ARGS__)

extern RK_U32 vp8e_rc_debug;

#endif //__VP8E_DEBUG_H__
