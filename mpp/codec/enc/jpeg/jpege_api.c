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
    /* output to hal */
    JpegeSyntax     syntax;

    /* input from hal */
    JpegeFeedback   feedback;
} JpegeCtx;

MPP_RET jpege_callback(void *ctx, void *feedback)
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

MPP_RET jpege_init(void *ctx, ControllerCfg *ctrlCfg)
{
    JpegeCtx *p = (JpegeCtx *)ctx;

    mpp_env_get_u32("jpege_debug", &jpege_debug, 0);
    jpege_dbg_func("enter ctx %p\n", ctx);

    memset(&p->syntax, 0, sizeof(p->syntax));

    mpp_assert(ctrlCfg->coding = MPP_VIDEO_CodingMJPEG);
    ctrlCfg->task_count = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

MPP_RET jpege_deinit(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

MPP_RET jpege_encode(void *ctx, HalEncTask *task)
{
    JpegeCtx *p = (JpegeCtx *)ctx;

    jpege_dbg_func("enter ctx %p\n", ctx);

    task->valid = 1;
    task->syntax.data   = &p->syntax;
    task->syntax.number = 1;
    task->is_intra      = 1;

    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

MPP_RET jpege_reset(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

MPP_RET jpege_flush(void *ctx)
{
    jpege_dbg_func("enter ctx %p\n", ctx);
    jpege_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET jpege_check_cfg(MppEncConfig *cfg)
{
    if (cfg->width < 16 && cfg->width > 8192) {
        mpp_err("jpege: invalid width %d is not in range [16..8192]\n", cfg->width);
        return MPP_NOK;
    }

    if (cfg->height < 16 && cfg->height > 8192) {
        mpp_err("jpege: invalid height %d is not in range [16..8192]\n", cfg->height);
        return MPP_NOK;
    }

    if (cfg->format != MPP_FMT_YUV420SP &&
        cfg->format != MPP_FMT_YUV420P  &&
        cfg->format != MPP_FMT_RGB888) {
        mpp_err("jpege: invalid format %d is not supportted\n", cfg->format);
        return MPP_NOK;
    }

    if (cfg->qp < 0 || cfg->qp > 10) {
        mpp_err("jpege: invalid quality level %d is not in range [0..10] set to default 8\n");
        cfg->qp = 8;
    }

    return MPP_OK;
}

MPP_RET jpege_config(void *ctx, RK_S32 cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    JpegeCtx *p = (JpegeCtx *)ctx;

    jpege_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case CHK_ENC_CFG : {
        /* NOTE */
        return jpege_check_cfg((MppEncConfig *)param);
    } break;
    case SET_ENC_CFG : {
        MppEncConfig *mpp_cfg = (MppEncConfig *)param;

        if (jpege_check_cfg(mpp_cfg))
            break;

        JpegeSyntax *syntax = &p->syntax;
        syntax->width   = mpp_cfg->width;
        syntax->height  = mpp_cfg->height;
        syntax->format  = mpp_cfg->format;
        syntax->quality = mpp_cfg->qp;
    } break;
    case SET_ENC_RC_CFG : {
        mpp_assert(p);
        MppEncConfig *mpp_cfg = (MppEncConfig *)param;
        JpegeSyntax *syntax = &p->syntax;

        mpp_cfg->width  = syntax->width;
        mpp_cfg->height = syntax->height;
        mpp_cfg->format = syntax->format;
        mpp_cfg->qp     = syntax->quality;
    } break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
        break;
    }

    jpege_dbg_func("leave ret %d\n", ret);

    return ret;
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
