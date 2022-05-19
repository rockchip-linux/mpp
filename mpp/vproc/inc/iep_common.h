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

#ifndef __IEP_COMMON_H__
#define __IEP_COMMON_H__

#include "mpp_debug.h"

#define IEP_DBG_FUNCTION            (0x00000001)
#define IEP_DBG_TRACE               (0x00000002)
#define IEP_DBG_IMAGE               (0x00000010)

#define iep_dbg(flag, fmt, ...)     _mpp_dbg(iep_debug, flag, fmt, ## __VA_ARGS__)
#define iep_dbg_f(flag, fmt, ...)   _mpp_dbg_f(iep_debug, flag, fmt, ## __VA_ARGS__)
#define iep_dbg_func(fmt, ...)      iep_dbg(IEP_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define iep_dbg_trace(fmt, ...)     iep_dbg(IEP_DBG_TRACE, fmt, ## __VA_ARGS__)

typedef enum IepCmd_e {
    IEP_CMD_INIT,                           // reset msg to all zero
    IEP_CMD_SET_SRC,                        // config source image info
    IEP_CMD_SET_DST,                        // config destination image info

    // deinterlace command
    IEP_CMD_SET_DEI_CFG         = 0x0100,   // config deinterlace configure
    IEP_CMD_SET_DEI_SRC1,                   // config deinterlace extra source
    IEP_CMD_SET_DEI_DST1,                   // config deinterlace extra destination
    IEP_CMD_SET_DEI_SRC2,                   // in case of iep2 deinterlacing

    // color enhancement command
    IEP_CMD_SET_YUV_ENHANCE     = 0x0200,   // config YUV enhancement parameter
    IEP_CMD_SET_RGB_ENHANCE,                // config RGB enhancement parameter

    // scale algorithm command
    IEP_CMD_SET_SCALE           = 0x0300,   // config scale algorithm

    // color convert command
    IEP_CMD_SET_COLOR_CONVERT   = 0x0400,   // config color convert parameter

    // hardware trigger command
    IEP_CMD_RUN_SYNC            = 0x1000,   // start sync mode process
    IEP_CMD_RUN_ASYNC,                      // start async mode process

    // hardware capability query command
    IEP_CMD_QUERY_CAP           = 0x8000,   // query iep capability
} IepCmd;

typedef enum IepFormat_e {
    IEP_FORMAT_RGB_BASE     = 0x0,
    IEP_FORMAT_ARGB_8888    = IEP_FORMAT_RGB_BASE,
    IEP_FORMAT_ABGR_8888    = 0x1,
    IEP_FORMAT_RGBA_8888    = 0x2,
    IEP_FORMAT_BGRA_8888    = 0x3,
    IEP_FORMAT_RGB_565      = 0x4,
    IEP_FORMAT_BGR_565      = 0x5,
    IEP_FORMAT_RGB_BUTT,

    IEP_FORMAT_YUV_BASE     = 0x10,
    IEP_FORMAT_YCbCr_422_SP = IEP_FORMAT_YUV_BASE,
    IEP_FORMAT_YCbCr_422_P  = 0x11,
    IEP_FORMAT_YCbCr_420_SP = 0x12,
    IEP_FORMAT_YCbCr_420_P  = 0x13,
    IEP_FORMAT_YCrCb_422_SP = 0x14,
    IEP_FORMAT_YCrCb_422_P  = 0x15, // same as IEP_FORMAT_YCbCr_422_P
    IEP_FORMAT_YCrCb_420_SP = 0x16,
    IEP_FORMAT_YCrCb_420_P  = 0x17, // same as IEP_FORMAT_YCbCr_420_P
    IEP_FORMAT_YUV_BUTT,
} IepFormat;

// iep image for external user
typedef struct IegImg_t {
    RK_U16  act_w;          // act_width
    RK_U16  act_h;          // act_height
    RK_S16  x_off;          // x offset for the vir,word unit
    RK_S16  y_off;          // y offset for the vir,word unit

    RK_U16  vir_w;          // unit in byte
    RK_U16  vir_h;          // unit in byte
    RK_U32  format;         // IepFormat
    RK_U32  mem_addr;       // base address fd
    RK_U32  uv_addr;        // chroma address fd + (offset << 10)
    RK_U32  v_addr;
} IepImg;

typedef void* IepCtx;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iep_com_ctx_t iep_com_ctx;

typedef struct iep_com_ops_t {
    MPP_RET (*init)(IepCtx *ctx);
    MPP_RET (*deinit)(IepCtx ctx);
    MPP_RET (*control)(IepCtx ctx, IepCmd cmd, void *param);
    void (*release)(iep_com_ctx *ctx);
} iep_com_ops;

typedef struct iep_com_ctx_t {
    iep_com_ops *ops;
    IepCtx priv;
    int ver;
} iep_com_ctx;

struct dev_compatible {
    const char *compatible;
    iep_com_ctx* (*get)(void);
    void (*put)(iep_com_ctx *ctx);
    int ver;
};

iep_com_ctx* get_iep_ctx();
void put_iep_ctx(iep_com_ctx *ictx);
extern RK_U32 iep_debug;

#ifdef __cplusplus
}
#endif

#endif /* __IEP_API_H__ */
