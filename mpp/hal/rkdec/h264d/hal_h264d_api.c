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


#define MODULE_TAG "hal_h264d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"
#include "h264d_log.h"

#include "hal_h264d_global.h"
#include "hal_h264d_api.h"

#include "hal_h264d_rkv_reg.h"
#include "hal_h264d_vdpu_reg.h"
/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    MppHalApi *p_api = NULL;

    INP_CHECK(ret, ctx, NULL == p_hal);
    memset(p_hal, 0, sizeof(H264dHalCtx_t));

    p_api = &p_hal->hal_api;
    //!< choose hard mode
    switch (cfg->device_id) {
    case HAL_RKVDEC:
        p_api->init    = rkv_h264d_init;
        p_api->deinit  = rkv_h264d_deinit;
        p_api->reg_gen = rkv_h264d_gen_regs;
        p_api->start   = rkv_h264d_start;
        p_api->wait    = rkv_h264d_wait;
        p_api->reset   = rkv_h264d_reset;
        p_api->flush   = rkv_h264d_flush;
        p_api->control = rkv_h264d_control;
        break;
    case HAL_VDPU:
        p_api->init    = vdpu_h264d_init;
        p_api->deinit  = vdpu_h264d_deinit;
        p_api->reg_gen = vdpu_h264d_gen_regs;
        p_api->start   = vdpu_h264d_start;
        p_api->wait    = vdpu_h264d_wait;
        p_api->reset   = vdpu_h264d_reset;
        p_api->flush   = vdpu_h264d_flush;
        p_api->control = vdpu_h264d_control;






    default:
        break;
    }

    //!< run init funtion
    FUN_CHECK(ret = p_api->init(hal, cfg));

__RETURN:
    return MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_deinit(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.deinit(hal);
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.reg_gen(hal, task);
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_start(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.start(hal, task);
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_wait(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.wait(hal, task);
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_reset(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.reset(hal);
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_flush(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.flush(hal);
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.control(hal, cmd_type, param);
}


const MppHalApi hal_api_h264d = {
    "h264d_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingAVC,
    sizeof(H264dHalCtx_t),
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

