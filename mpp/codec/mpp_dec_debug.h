/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __MPP_DEC_DEBUG_H__
#define __MPP_DEC_DEBUG_H__

#include "mpp_debug.h"

#define MPP_DEC_DBG_FUNCTION            (0x00000001)
#define MPP_DEC_DBG_TIMING              (0x00000002)
#define MPP_DEC_DBG_STATUS              (0x00000010)
#define MPP_DEC_DBG_DETAIL              (0x00000020)
#define MPP_DEC_DBG_RESET               (0x00000040)
#define MPP_DEC_DBG_NOTIFY              (0x00000080)

#define mpp_dec_dbg(flag, fmt, ...)     _mpp_dbg(mpp_dec_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_dec_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_dec_debug, flag, fmt, ## __VA_ARGS__)

#define dec_dbg_func(fmt, ...)          mpp_dec_dbg_f(MPP_DEC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define dec_dbg_status(fmt, ...)        mpp_dec_dbg_f(MPP_DEC_DBG_STATUS, fmt, ## __VA_ARGS__)
#define dec_dbg_detail(fmt, ...)        mpp_dec_dbg(MPP_DEC_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define dec_dbg_reset(fmt, ...)         mpp_dec_dbg(MPP_DEC_DBG_RESET, fmt, ## __VA_ARGS__)
#define dec_dbg_notify(fmt, ...)        mpp_dec_dbg_f(MPP_DEC_DBG_NOTIFY, fmt, ## __VA_ARGS__)

extern RK_U32 mpp_dec_debug;

#endif /* __MPP_DEC_DEBUG_H__ */
