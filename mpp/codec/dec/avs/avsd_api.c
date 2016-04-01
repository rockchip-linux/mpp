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

#define MODULE_TAG "avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_env.h"

#include "avsd_api.h"


RK_U32 avsd_parse_debug = 0;

/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET avsd_init(void *ctx, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    mpp_env_get_u32("avsd_debug", &avsd_parse_debug, 0);

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    (void)init;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET avsd_deinit(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET avsd_reset(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET avsd_flush(void *ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET avsd_control(void *ctx, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    (void)cmd_type;
    (void)param;

    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
MPP_RET avsd_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");
    (void)ctx;
    (void)pkt;
    (void)task;
    return ret = MPP_OK;
}


/*!
***********************************************************************
* \brief
*   parser
***********************************************************************
*/
MPP_RET avsd_parse(void *ctx, HalDecTask *in_task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)ctx;
    (void)in_task;

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET avsd_callback(void *decoder, void *info)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_PARSE_TRACE("In.");

    (void)decoder;
    (void)info;

    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const ParserApi api_avsd_parser = {
    "avsd_parse",
    MPP_VIDEO_CodingAVS,
    200,//sizeof(AvsCodecContext),
    0,
    avsd_init,
    avsd_deinit,
    avsd_prepare,
    avsd_parse,
    avsd_reset,
    avsd_flush,
    avsd_control,
    avsd_callback,
};

