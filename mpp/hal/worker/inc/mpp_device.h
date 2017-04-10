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

#ifdef __cplusplus
extern "C"
{
#endif

#include "rk_mpi.h"

/* mpp service capability description */
typedef enum MppDevProp_e {
    MPP_DEV_MAX_WIDTH,
    MPP_DEV_MAX_HEIGHT,
    MPP_DEV_MIN_WIDTH,
    MPP_DEV_MIN_HEIGHT,
    MPP_DEV_MMU_ENABLE,
    MPP_DEV_PROP_BUTT,
} MppDevProp;

/*
 * hardware device open function
 * coding and type for device name detection
 *
 * flag for postprocess enable
 * MPP_DEVICE_POSTPROCCESS_ENABLE   (0x00000001)
 */
RK_S32 mpp_device_init(MppCtxType coding, MppCodingType type, RK_U32 flag);
MPP_RET mpp_device_deinit(RK_S32 dev);

/*
 * Query device capability with property ID.
 */
RK_S32 mpp_srv_query(MppDevProp id);

/*
 * register access interface
 */
MPP_RET mpp_device_send_reg(RK_S32 dev, RK_U32 *regs, RK_U32 nregs);
MPP_RET mpp_device_wait_reg(RK_S32 dev, RK_U32 *regs, RK_U32 nregs);
MPP_RET mpp_device_send_reg_with_id(RK_S32 dev, RK_S32 id, void *param, RK_S32 size);

/*
 * New interface
 */


#ifdef __cplusplus
}
#endif

#endif /* __MPP_DEVICE_H__ */

