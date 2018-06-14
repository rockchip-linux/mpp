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

#ifndef __MPP_DEVICE_H__
#define __MPP_DEVICE_H__

#include "rk_mpi.h"

/* mpp service capability description */
typedef enum MppDevCmd_e {
    MPP_DEV_GET_START = 0,
    MPP_DEV_GET_MAX_WIDTH,
    MPP_DEV_GET_MAX_HEIGHT,
    MPP_DEV_GET_MIN_WIDTH,
    MPP_DEV_GET_MIN_HEIGHT,
    MPP_DEV_GET_MMU_STATUS,

    MPP_DEV_SET_START = 0x01000000,
    MPP_DEV_SET_HARD_PLATFORM, // set paltform by user
    MPP_DEV_ENABLE_POSTPROCCESS,

    MPP_DEV_PROP_BUTT,
} MppDevCmd;

typedef struct MppDevCfg_t {
    // input
    MppCtxType      type;
    MppCodingType   coding;
    RK_U32          platform;
    RK_U32          pp_enable;
} MppDevCfg;

typedef void*   MppDevCtx;

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * hardware device open function
 * coding and type for device name detection
 */
MPP_RET mpp_device_init(MppDevCtx *ctx, MppDevCfg *cfg);
MPP_RET mpp_device_deinit(MppDevCtx ctx);

/*
 * control function for set or get device property
 */
RK_S32 mpp_device_control(MppDevCtx ctx, MppDevCmd cmd, void *param);

/*
 * register access interface
 */
MPP_RET mpp_device_send_reg(MppDevCtx ctx, RK_U32 *regs, RK_U32 nregs);
MPP_RET mpp_device_wait_reg(MppDevCtx ctx, RK_U32 *regs, RK_U32 nregs);
MPP_RET mpp_device_send_reg_with_id(MppDevCtx ctx, RK_S32 id, void *param, RK_S32 size);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEVICE_H__ */
