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

#include "vp8d_api.h"
#include "vp8d_parser.h"
#include "vp8d_codec.h"

const ParserApi api_vp8d_parser = {
    .name = "vp8d_parse",
    .coding = MPP_VIDEO_CodingVP8,
    .ctx_size = sizeof(VP8DContext),
    .flag = 0,
    .init = vp8d_parser_init,
    .deinit = vp8d_parser_deinit,
    .prepare = vp8d_parser_prepare,
    .parse = vp8d_parser_parse,
    .reset = vp8d_parser_reset,
    .flush = vp8d_parser_flush,
    .control = vp8d_parser_control,
    .callback = vp8d_parser_callback,
};
