/*
*
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


#ifndef __HAL_H264D_VDPU_REG_H__
#define __HAL_H264D_VDPU_REG_H__

#include "mpp_hal.h"

typedef struct h264d_vdpu_regs_t {
    struct {
        RK_U32    minor_ver : 8;
        RK_U32    level : 1;
        RK_U32    dec_support : 3;
        RK_U32    profile : 1;
        RK_U32    reserve0 : 1;
        RK_U32    codec_flag : 1;
        RK_U32    reserve1 : 1;
        RK_U32    prod_num : 16;
    } swreg0_id;


} H264_VdpuRegs_t;



#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vdpu_h264d_init    (void *hal, MppHalCfg *cfg);
MPP_RET vdpu_h264d_deinit  (void *hal);
MPP_RET vdpu_h264d_gen_regs(void *hal, HalTaskInfo *task);
MPP_RET vdpu_h264d_start   (void *hal, HalTaskInfo *task);
MPP_RET vdpu_h264d_wait    (void *hal, HalTaskInfo *task);
MPP_RET vdpu_h264d_reset   (void *hal);
MPP_RET vdpu_h264d_flush   (void *hal);
MPP_RET vdpu_h264d_control (void *hal, RK_S32 cmd_type, void *param);


#ifdef __cplusplus
}
#endif

#endif /*__HAL_H264D_VDPU_REG_H__*/
