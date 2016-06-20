/*
 * *
 * * Copyright 2016 Rockchip Electronics Co. LTD
 * *
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * * you may not use this file except in compliance with the License.
 * * You may obtain a copy of the License at
 * *
 * *      http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing, software
 * * distributed under the License is distributed on an "AS IS" BASIS,
 * * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * * See the License for the specific language governing permissions and
 * * limitations under the License.
 * */

/*
* @file       hal_vpu_mpg4d_reg.c
* @brief
* @author      gzl(lance.gao@rock-chips.com)

* @version     1.0.0
* @history
*   2016.04.11 : Create
*/

#define MODULE_TAG "hal_vpu_mpg4d"

#include <stdio.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_buffer.h"

#include "mpp_dec.h"
#include "mpg4d_syntax.h"
#include "vpu.h"

typedef struct mpeg4d_reg_context {
    RK_S32          vpu_fd;
    MppBufSlots     slots;
    MppBufSlots     packet_slots;
    MppBufferGroup  group;
    MppBuffer       directMV_Addr;
    int             addr_init_flag;
    void*           hw_regs;
    IOInterruptCB   int_cb;
} hal_mpg4_ctx;

MPP_RET hal_vpu_mpg4d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    hal_mpg4_ctx *reg_ctx = (hal_mpg4_ctx *)hal;

    if (NULL == reg_ctx) {
        mpp_err("hal instan no alloc");
        return MPP_ERR_UNKNOW;
    }
    reg_ctx->slots  = cfg->frame_slots;
    reg_ctx->int_cb = cfg->hal_int_cb;

    reg_ctx->packet_slots = cfg->packet_slots;

    return ret;
}

MPP_RET hal_vpu_mpg4d_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    (void) hal;

    return ret;
}

MPP_RET hal_vpu_mpg4d_gen_regs(void *hal,  HalTaskInfo *syn)
{
    MPP_RET ret = MPP_OK;
    (void) hal;
    (void) syn;
    return ret;
}

MPP_RET hal_vpu_mpg4d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    (void)hal;
    (void)task;
    return ret;
}

MPP_RET hal_vpu_mpg4d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    (void)task;
    (void) hal;
    return ret;
}

MPP_RET hal_vpu_mpg4d_reset(void *hal)
{
    MPP_RET ret = MPP_OK;
    (void)hal;
    return ret;
}

MPP_RET hal_vpu_mpg4d_flush(void *hal)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_vpu_mpg4d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    (void)hal;
    (void)cmd_type;
    (void)param;
    return  ret;
}

const MppHalApi hal_api_mpg4d = {
    "mpg4d_vpu",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingMPEG4,
    sizeof(hal_mpg4_ctx),
    0,
    hal_vpu_mpg4d_init,
    hal_vpu_mpg4d_deinit,
    hal_vpu_mpg4d_gen_regs,
    hal_vpu_mpg4d_start,
    hal_vpu_mpg4d_wait,
    hal_vpu_mpg4d_reset,
    hal_vpu_mpg4d_flush,
    hal_vpu_mpg4d_control,
};

