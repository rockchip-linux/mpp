/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_PARSER_H
#define MPP_PARSER_H

#include "mpp_singleton.h"

#include "parser_api.h"

typedef void* Parser;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_parser_init(Parser *prs, ParserCfg *cfg);
MPP_RET mpp_parser_deinit(Parser prs);

MPP_RET mpp_parser_prepare(Parser prs, MppPacket pkt, HalDecTask *task);
MPP_RET mpp_parser_parse(Parser prs, HalDecTask *task);

MPP_RET mpp_parser_reset(Parser prs);
MPP_RET mpp_parser_flush(Parser prs);
MPP_RET mpp_parser_control(Parser prs, MpiCmd cmd, void *para);
MPP_RET mpp_parser_callback(void* prs, void *err_info);

MPP_RET mpp_parser_api_register(const ParserApi *api);

#define MPP_PARSER_API_REGISTER(api) \
    static void mpp_parser_api_register_##api##_wrapper(void) \
    { \
        mpp_parser_api_register(&api); \
    } \
    MPP_MODULE_ADD(api, mpp_parser_api_register_##api##_wrapper, NULL);

#ifdef __cplusplus
}
#endif

#endif /* MPP_PARSER_H */
