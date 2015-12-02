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

#define MODULE_TAG "vp9d_api"

#include <string.h>


#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"

#include "vp9d_api.h"




/*!
***********************************************************************
* \brief
*   alloc all buffer
***********************************************************************
*/

MPP_RET vp9d_init(void *decoder, ParserCfg *init)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    (void)decoder;

    init->task_count = 2;

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*   free all buffer
***********************************************************************
*/
MPP_RET vp9d_deinit(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
    return ret = MPP_OK;

}
/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET  vp9d_reset(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET  vp9d_flush(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET  vp9d_control(void *decoder, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
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
MPP_RET vp9d_prepare(void *decoder, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
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
MPP_RET vp9d_parse(void *decoder, HalDecTask *in_task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
    (void)in_task;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   callback
***********************************************************************
*/
MPP_RET vp9d_callback(void *decoder, void *err_info)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    (void)decoder;
    (void)err_info;
    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/

const ParserApi api_vp9d_parser = {
    "vp9d_parse",
    MPP_VIDEO_CodingVP9,
    0,//sizeof(H264_DecCtx_t),
    0,
    vp9d_init,
    vp9d_deinit,
    vp9d_prepare,
    vp9d_parse,
    vp9d_reset,
    vp9d_flush,
    vp9d_control,
    vp9d_callback,
};

