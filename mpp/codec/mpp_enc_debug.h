/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __MPP_ENC_DEBUG_H__
#define __MPP_ENC_DEBUG_H__

#include "mpp_debug.h"

#define MPP_ENC_DBG_FUNCTION            (0x00000001)
#define MPP_ENC_DBG_CONTROL             (0x00000002)
#define MPP_ENC_DBG_CFG                 (0x00000004)
#define MPP_ENC_DBG_STATUS              (0x00000010)
#define MPP_ENC_DBG_DETAIL              (0x00000020)
#define MPP_ENC_DBG_RESET               (0x00000040)
#define MPP_ENC_DBG_NOTIFY              (0x00000080)
#define MPP_ENC_DBG_REENC               (0x00000100)
#define MPP_ENC_DBG_SLICE               (0x00000200)

#define MPP_ENC_DBG_FRM_STATUS          (0x00010000)

#define mpp_enc_dbg(flag, fmt, ...)     _mpp_dbg(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_enc_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)

#define enc_dbg_func(fmt, ...)          mpp_enc_dbg_f(MPP_ENC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define enc_dbg_ctrl(fmt, ...)          mpp_enc_dbg_f(MPP_ENC_DBG_CONTROL, fmt, ## __VA_ARGS__)
#define enc_dbg_cfg(fmt, ...)           mpp_enc_dbg(MPP_ENC_DBG_CFG, fmt, ## __VA_ARGS__)
#define enc_dbg_status(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_STATUS, fmt, ## __VA_ARGS__)
#define enc_dbg_detail(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define enc_dbg_notify(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_NOTIFY, fmt, ## __VA_ARGS__)
#define enc_dbg_reenc(fmt, ...)         mpp_enc_dbg_f(MPP_ENC_DBG_REENC, fmt, ## __VA_ARGS__)
#define enc_dbg_slice(fmt, ...)         mpp_enc_dbg(MPP_ENC_DBG_SLICE, fmt, ## __VA_ARGS__)
#define enc_dbg_frm_status(fmt, ...)    mpp_enc_dbg_f(MPP_ENC_DBG_FRM_STATUS, fmt, ## __VA_ARGS__)

extern RK_U32 mpp_enc_debug;

#endif /* __MPP_ENC_DEBUG_H__ */
