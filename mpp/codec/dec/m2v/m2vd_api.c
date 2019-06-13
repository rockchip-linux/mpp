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

#define MODULE_TAG "m2vd_api"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_packet_impl.h"

#include "m2vd_api.h"
#include "m2vd_parser.h"
#include "m2vd_codec.h"

const ParserApi api_m2vd_parser = {
    .name = "m2vd_parse",
    .coding = MPP_VIDEO_CodingMPEG2,
    .ctx_size = sizeof(M2VDContext),
    .flag = 0,
    .init = m2vd_parser_init,
    .deinit = m2vd_parser_deinit,
    .prepare = m2vd_parser_prepare,
    .parse = m2vd_parser_parse,
    .reset = m2vd_parser_reset,
    .flush = m2vd_parser_flush,
    .control = m2vd_parser_control,
    .callback = m2vd_parser_callback,
};
