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


#define MODULE_TAG "hal_vp9d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"

#include "hal_vp9d_api.h"



/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
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
MPP_RET hal_vp9d_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

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
MPP_RET hal_vp9d_gen_regs(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_start(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_wait(void *hal, HalTaskInfo *task)
{
    (void)hal;
    (void)task;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_reset(void *hal)
{
    (void)hal;

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_flush(void *hal)
{
    (void)hal;


    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_vp9d_control(void *hal, RK_S32 cmd_type, void *param)
{
    (void)hal;
    (void)cmd_type;
    (void)param;

    return MPP_OK;
}


const MppHalApi hal_api_vp9d = {
    "vp9d_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingVP9,
    0,//sizeof(H264dHalCtx_t),
    0,
    hal_vp9d_init,
    hal_vp9d_deinit,
    hal_vp9d_gen_regs,
    hal_vp9d_start,
    hal_vp9d_wait,
    hal_vp9d_reset,
    hal_vp9d_flush,
    hal_vp9d_control,
};

