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

#ifndef __RGA_API_H__
#define __RGA_API_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_frame.h"

/*
 * NOTE: Normal rga usage should consider address align issue.
 * But memory in mpp is always aligned. So we do not tak align issue into
 * consideration.
 */
typedef enum RgaCmd_e {
    RGA_CMD_INIT,                           // reset msg to all zero
    RGA_CMD_SET_SRC,                        // config source image info
    RGA_CMD_SET_DST,                        // config destination image info

    RGA_CMD_SET_SCALE_CFG       = 0x0100,   // config copy parameter

    RGA_CMD_SET_COLOR_CONVERT   = 0x0200,   // config color convert parameter

    RGA_CMD_SET_ROTATION        = 0x0300,   // config rotation parameter

    // hardware trigger command
    RGA_CMD_RUN_SYNC            = 0x1000,   // start sync mode process
    RGA_CMD_RUN_ASYNC,                      // start async mode process
} RgaCmd;

typedef void* RgaCtx;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET rga_init(RgaCtx *ctx);
MPP_RET rga_deinit(RgaCtx ctx);

MPP_RET rga_control(RgaCtx ctx, RgaCmd cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif /* __RGA_API_H__ */
