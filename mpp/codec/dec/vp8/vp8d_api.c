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

#define MODULE_TAG "vp8d_api"

#include <string.h>
#include "vpu_api.h"
#include "mpp_env.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "vp8d_api.h"
#include "vp8d_parser.h"
#include "vp8d_codec.h"

const ParserApi api_vp8d_parser = {
    "vp8d_parse",
    MPP_VIDEO_CodingVP8,
    sizeof(VP8DContext),
    0,
    vp8d_parser_init,
    vp8d_parser_deinit,
    vp8d_parser_prepare,
    vp8d_parser_parse,
    vp8d_parser_reset,
    vp8d_parser_flush,
    vp8d_parser_control,
    vp8d_parser_callback,
};
