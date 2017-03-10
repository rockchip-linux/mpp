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

#include <string.h>

#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "jpege_api.h"
#include "jpege_syntax.h"

#define JPEGE_DBG_FUNCTION          (0x00000001)
#define JPEGE_DBG_INPUT             (0x00000010)
#define JPEGE_DBG_OUTPUT            (0x00000020)

RK_U32 jpege_debug = 0;

#define jpege_dbg(flag, fmt, ...)   _mpp_dbg(jpege_debug, flag, fmt, ## __VA_ARGS__)
#define jpege_dbg_f(flag, fmt, ...) _mpp_dbg_f(jpege_debug, flag, fmt, ## __VA_ARGS__)

#define jpege_dbg_func(fmt, ...)    jpege_dbg_f(JPEGE_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define jpege_dbg_input(fmt, ...)   jpege_dbg(JPEGE_DBG_INPUT, fmt, ## __VA_ARGS__)
#define jpege_dbg_output(fmt, ...)  jpege_dbg(JPEGE_DBG_OUTPUT, fmt, ## __VA_ARGS__)

typedef struct {
    /* input from hal */
    JpegeFeedback   feedback;
    MppEncCfgSet    *cfg;
    MppEncCfgSet    *set;
} JpegeCtx;

static MPP_RET jpege_callback(void *ctx, void *feedback)
{
    JpegeCtx *p = (JpegeCtx *)ctx;
    JpegeFeedback *result = &p->feedback;

    jpege_dbg_func("enter ctx %p\n", ctx);

    memcpy(result, feedback, sizeof(*result));
    if (result->hw_status)
        mpp_err("jpege: hardware return error status %x\n", result->hw_status);

    jpege_dbg_output("jpege: stream length %d\n", result->stream_length);

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_init(void *ctx, ControllerCfg *cfg)
{
    JpegeCtx *p = (JpegeCtx *)ctx;

    mpp_env_get_u32("jpege_debug", &jpege_debug, 0);
    jpege_dbg_func("enter ctx %p\n", ctx);

    p->cfg = cfg->cfg;
    p->set = cfg->set;

    mpp_assert(cfg->coding = MPP_VIDEO_CodingMJPEG);
    cfg->task_count = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_deinit(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_encode(void *ctx, HalEncTask *task)
{
    jpege_dbg_func("enter ctx %p\n", ctx);

    task->valid = 1;
    task->is_intra = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_reset(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_flush(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_config(void *ctx, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;

    jpege_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);
    jpege_dbg_func("leave ret %d\n", ret);
    return ret;
}

const ControlApi api_jpege_controller = {
    .name = "jpege_control",
    .coding = MPP_VIDEO_CodingMJPEG,
    .ctx_size = sizeof(JpegeCtx),
    .flag = 0,
    .init = jpege_init,
    .deinit = jpege_deinit,
    .encode = jpege_encode,
    .reset = jpege_reset,
    .flush = jpege_flush,
    .config = jpege_config,
    .callback = jpege_callback,
};
