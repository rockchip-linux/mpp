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

#define MODULE_TAG "hal_dummy_dec"

#include "hal_dummy_dec_api.h"


MPP_RET hal_dummy_dec_init(void *hal, MppHalCfg *cfg)
{
    (void)hal;
    (void)cfg;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_deinit(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_gen_regs(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}
MPP_RET hal_dummy_dec_start(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_wait(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_reset(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_flush(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_dec_control(void *hal, RK_S32 cmd_type, void *param)
{
    (void)hal;
    (void)cmd_type;
    (void)param;
    return MPP_OK;
}

const MppHalApi hal_api_dummy_dec = {
    "dummy_hw_dec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingUnused,
    0,
    0,
    hal_dummy_dec_init,
    hal_dummy_dec_deinit,
    hal_dummy_dec_gen_regs,
    hal_dummy_dec_start,
    hal_dummy_dec_wait,
    hal_dummy_dec_reset,
    hal_dummy_dec_flush,
    hal_dummy_dec_control,
};

