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

#define MODULE_TAG "hal_dummy_enc"

#include "hal_dummy_enc_api.h"


MPP_RET hal_dummy_enc_init(void *hal, MppHalCfg *cfg)
{
    (void)hal;
    (void)cfg;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_deinit(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_gen_regs(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}
MPP_RET hal_dummy_enc_start(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_wait(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_reset(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_flush(void *hal)
{
    (void)hal;
    return MPP_OK;
}

MPP_RET hal_dummy_enc_control(void *hal, RK_S32 cmd_type, void *param)
{
    (void)hal;
    (void)cmd_type;
    (void)param;
    return MPP_OK;
}

const MppHalApi hal_api_dummy_enc = {
    .name = "dummy_hw_enc",
    .type = MPP_CTX_ENC,
    .coding = MPP_VIDEO_CodingUnused,
    .ctx_size = 0,
    .flag = 0,
    .init = hal_dummy_enc_init,
    .deinit = hal_dummy_enc_deinit,
    .reg_gen = hal_dummy_enc_gen_regs,
    .start = hal_dummy_enc_start,
    .wait = hal_dummy_enc_wait,
    .reset = hal_dummy_enc_reset,
    .flush = hal_dummy_enc_flush,
    .control = hal_dummy_enc_control,
};


