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

#define MODULE_TAG "jpege_api"

#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "jpege_api.h"

typedef struct {
    void    *ctx;
} JpegeCtx;

RK_U32 jpege_debug = 0;

MPP_RET jpege_init(void *ctx, ControllerCfg *ctrlCfg)
{
	(void)ctx;
	(void)ctrlCfg;

    return MPP_OK;
}

MPP_RET jpege_deinit(void *ctx)
{
	(void)ctx;

	return MPP_OK;
}

MPP_RET jpege_encode(void *ctx, HalEncTask *task)
{
	(void)ctx;
	(void)task;

    return MPP_OK;
}

MPP_RET jpege_reset(void *ctx)
{
    (void)ctx;

    return MPP_OK;
}

MPP_RET jpege_flush(void *ctx)
{
    (void)ctx;

    return MPP_OK;
}

MPP_RET jpege_config(void *ctx, RK_S32 cmd, void *param)
{
	(void)ctx;
	(void)cmd;
	(void)param;

    return MPP_OK;
}

MPP_RET jpege_callback(void *ctx, void *feedback)
{
	(void)ctx;
	(void)feedback;

    return MPP_OK;
}

const ControlApi api_jpege_controller = {
    "jpege_control",
	MPP_VIDEO_CodingMJPEG,
	sizeof(JpegeCtx),
    0,
    jpege_init,
    jpege_deinit,
	jpege_encode,
	jpege_reset,
	jpege_flush,
	jpege_config,
	jpege_callback,
};
