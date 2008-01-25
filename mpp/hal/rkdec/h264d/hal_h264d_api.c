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
#define MODULE_TAG "hal_h264d"

#include "hal_h264d_api.h"


MPP_RET hal_h264d_init(void **hal, MppHalCfg *cfg)
{
    return MPP_OK;
}

MPP_RET hal_h264d_deinit(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_gen_regs(void *hal, MppSyntax *syn)
{
    return MPP_OK;
}
MPP_RET hal_h264d_start(void *hal, MppHalDecTask task)
{
    return MPP_OK;
}

MPP_RET hal_h264d_wait(void *hal, MppHalDecTask task)
{
    return MPP_OK;
}

MPP_RET hal_h264d_reset(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_flush(void *hal)
{
    return MPP_OK;
}

MPP_RET hal_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    return MPP_OK;
}



const MppHalApi api_h264d_hal = {
    0,
    hal_h264d_init,
    hal_h264d_deinit,
    hal_h264d_gen_regs,
    hal_h264d_start,
    hal_h264d_wait,
    hal_h264d_reset,
    hal_h264d_flush,
    hal_h264d_control,
};