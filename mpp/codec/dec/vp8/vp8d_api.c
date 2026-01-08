/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vp8d_api"

#include <string.h>

#include "mpp_parser.h"
#include "vp8d_parser.h"
#include "vp8d_codec.h"

const ParserApi mpp_vp8d = {
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

MPP_PARSER_API_REGISTER(mpp_vp8d);
