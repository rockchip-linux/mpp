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
#define MODULE_TAG "h264d_api"


#include "h264d_api.h"


MPP_RET  h264d_init(void *decoder, MppParserInitCfg *init)
{
    (void)decoder;
    (void)init;
    return MPP_OK;
}

MPP_RET  h264d_deinit(void *decoder)
{
    (void)decoder;
    return MPP_OK;
}

MPP_RET  h264d_reset(void *decoder)
{
    (void)decoder;
    return MPP_OK;
}


MPP_RET  h264d_flush(void *decoder)
{
    (void)decoder;
    return MPP_OK;
}


MPP_RET  h264d_control(void *decoder, RK_S32 cmd_type, void *param)
{
    (void)decoder;
    (void)cmd_type;
    (void)param;
    return MPP_OK;
}


MPP_RET h264d_parser(void *decoder, MppPacket pkt, HalDecTask *task)
{
    (void)decoder;
    (void)pkt;
    (void)task;
    return MPP_OK;
}

const MppDecParser api_h264d_parser = {
    "h264d_parser",
    MPP_VIDEO_CodingAVC,
    0,
    0,
    h264d_init,
    h264d_deinit,
    h264d_parser,
    h264d_reset,
    h264d_flush,
    h264d_control,
};
