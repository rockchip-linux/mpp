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


#define MODULE_TAG "hal_avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "vpu.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"

#include "hal_avsd_api.h"


RK_U32 avsd_hal_debug = 0;


/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    mpp_env_get_u32("avsd_debug", &avsd_hal_debug, 0);

    AVSD_HAL_TRACE("In.");

    (void)hal;
    (void)cfg;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");
    (void)hal;
    (void)task;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    (void)task;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    (void)task;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)hal;
    (void)cmd_type;
    (void)param;

    return ret = MPP_OK;
}


const MppHalApi hal_api_avsd = {
    "avsd_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingAVS,
    200,//sizeof(AvsdHalCtx_t),
    0,
    hal_avsd_init,
    hal_avsd_deinit,
    hal_avsd_gen_regs,
    hal_avsd_start,
    hal_avsd_wait,
    hal_avsd_reset,
    hal_avsd_flush,
    hal_avsd_control,
};

