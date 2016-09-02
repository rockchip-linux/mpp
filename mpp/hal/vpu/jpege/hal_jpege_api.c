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

#define MODULE_TAG "hal_jpege_api"

#include "mpp_hal.h"

#include "hal_jpege_api.h"

typedef struct hal_jpege_ctx_s {
    MppBufSlots         frm_slots;
    MppBufSlots         pkt_slots;
    IOInterruptCB       int_cb;
} hal_jpege_ctx;

RK_U32 hal_jpege_debug = 0;

MPP_RET hal_jpege_init(void *hal, MppHalCfg *cfg)
{
	(void)hal;
	(void)cfg;

	return MPP_OK;
}

MPP_RET hal_jpege_deinit(void *hal)
{
	(void)hal;

	return MPP_OK;
}

MPP_RET hal_jpege_gen_regs(void *hal, HalTaskInfo *task)
{
	(void)hal;
	(void)task;

	return MPP_OK;
}

MPP_RET hal_jpege_start(void *hal, HalTaskInfo *task)
{
	(void)task;
	(void)hal;

	return MPP_OK;
}

MPP_RET hal_jpege_wait(void *hal, HalTaskInfo *task)
{
	(void)task;
	(void)hal;

	return MPP_OK;
}

MPP_RET hal_jpege_reset(void *hal)
{
	(void)hal;

	return MPP_OK;
}

MPP_RET hal_jpege_flush(void *hal)
{
	(void)hal;

	return MPP_OK;
}

MPP_RET hal_jpege_control(void *hal, RK_S32 cmd_type, void *param)
{
	(void)hal;
	(void)param;
	(void)cmd_type;

	return MPP_OK;
}

const MppHalApi hal_api_jpege = {
    "jpege_vpu",
    MPP_CTX_ENC,
	MPP_VIDEO_CodingMJPEG,
    sizeof(hal_jpege_ctx),
    0,
	hal_jpege_init,
	hal_jpege_deinit,
	hal_jpege_gen_regs,
	hal_jpege_start,
	hal_jpege_wait,
	hal_jpege_reset,
	hal_jpege_flush,
	hal_jpege_control,
};

