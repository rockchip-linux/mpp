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

#ifndef __MPP_DEVICE_DEBUG_H__
#define __MPP_DEVICE_DEBUG_H__

#include "mpp_log.h"

#define MPP_DEVICE_DBG_FUNC                 (0x00000001)
#define MPP_DEVICE_DBG_PROBE                (0x00000002)
#define MPP_DEVICE_DBG_DETAIL               (0x00000004)
#define MPP_DEVICE_DBG_REG                  (0x00000010)
#define MPP_DEVICE_DBG_TIME                 (0x00000020)

#define mpp_dev_dbg(flag, fmt, ...)         _mpp_dbg(mpp_device_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_f(flag, fmt, ...)       _mpp_dbg_f(mpp_device_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_dev_dbg_func(fmt, ...)          mpp_dev_dbg_f(MPP_DEVICE_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_probe(fmt, ...)         mpp_dev_dbg_f(MPP_DEVICE_DBG_PROBE, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_detail(fmt, ...)        mpp_dev_dbg(MPP_DEVICE_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_reg(fmt, ...)           mpp_dev_dbg(MPP_DEVICE_DBG_REG, fmt, ## __VA_ARGS__)
#define mpp_dev_dbg_time(fmt, ...)          mpp_dev_dbg(MPP_DEVICE_DBG_TIME, fmt, ## __VA_ARGS__)

extern RK_U32 mpp_device_debug;

#endif /* __MPP_DEVICE_DEBUG_H__ */
