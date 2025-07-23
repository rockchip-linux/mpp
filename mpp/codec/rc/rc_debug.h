/*
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

#ifndef __RC_DEBUG_H__
#define __RC_DEBUG_H__

#include "mpp_debug.h"

#define RC_DBG_FUNCTION             (0x00000001)
#define RC_DBG_API_IMPL             (0x00000002)
#define RC_DBG_FPS                  (0x00000010)
#define RC_DBG_BPS                  (0x00000020)
#define RC_DBG_RC                   (0x00000040)
#define RC_DBG_QP                   (0x00000080)
#define RC_DBG_CFG                  (0x00000100)
#define RC_DBG_DROP                 (0x00000200)
#define RC_DBG_RECORD               (0x00001000)
#define RC_DBG_VBV                  (0x00002000)

#define rc_dbg(flag, fmt, ...)      _mpp_dbg(rc_debug, flag, fmt, ## __VA_ARGS__)
#define rc_dbg_f(flag, fmt, ...)    _mpp_dbg_f(rc_debug, flag, fmt, ## __VA_ARGS__)

#define rc_dbg_func(fmt, ...)       rc_dbg_f(RC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define rc_dbg_impl(fmt, ...)       rc_dbg(RC_DBG_API_IMPL, fmt, ## __VA_ARGS__)
#define rc_dbg_fps(fmt, ...)        rc_dbg_f(RC_DBG_FPS, fmt, ## __VA_ARGS__)
#define rc_dbg_bps(fmt, ...)        rc_dbg_f(RC_DBG_BPS, fmt, ## __VA_ARGS__)
#define rc_dbg_rc(fmt, ...)         rc_dbg_f(RC_DBG_RC, fmt, ## __VA_ARGS__)
#define rc_dbg_qp(fmt, ...)         rc_dbg_f(RC_DBG_QP, fmt, ## __VA_ARGS__)

#define rc_dbg_cfg(fmt, ...)        rc_dbg(RC_DBG_CFG, fmt, ## __VA_ARGS__)
#define rc_dbg_drop(fmt, ...)       rc_dbg(RC_DBG_DROP, fmt, ## __VA_ARGS__)
#define rc_dbg_vbv(fmt, ...)        rc_dbg(RC_DBG_VBV, fmt, ## __VA_ARGS__)

extern RK_U32 rc_debug;

#endif /* __RC_DEBUG_H__ */
