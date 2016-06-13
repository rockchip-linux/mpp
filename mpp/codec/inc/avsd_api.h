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

#ifndef __AVSD_API_H__
#define __AVSD_API_H__

#include "parser_api.h"


#define AVSD_DBG_ERROR             (0x00000001)
#define AVSD_DBG_ASSERT            (0x00000002)
#define AVSD_DBG_WARNNING          (0x00000004)
#define AVSD_DBG_TRACE             (0x00000008)


#define AVSD_DBG_INPUT             (0x00000010)   //!< input packet


#define AVSD_DBG_TIME              (0x00000020)   //!< input packet

#define AVSD_DBG_CALLBACK          (0x00008000)




extern RK_U32 avsd_parse_debug;

#define AVSD_PARSE_TRACE(fmt, ...)\
do {\
    if (AVSD_DBG_TRACE & avsd_parse_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)


#define AVSD_DBG(level, fmt, ...)\
do {\
    if (level & avsd_parse_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)

#ifdef INP_CHECK
#undef INP_CHECK
#endif
//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
    if ((val)) {\
        ret = MPP_ERR_INIT; \
        AVSD_DBG(AVSD_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)
#ifdef MEM_CHECK
#undef MEM_CHECK
#endif
//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if(!(val)) {\
        ret = MPP_ERR_MALLOC;\
        AVSD_DBG(AVSD_DBG_ERROR, "malloc buffer error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)
#ifdef FUN_CHECK
#undef FUN_CHECK
#endif
//!< function return check
#define FUN_CHECK(val)\
do{\
if ((val) < 0) {\
        AVSD_DBG(AVSD_DBG_WARNNING, "Function error(%d).\n", __LINE__); \
        goto __FAILED; \
}} while (0)




#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_avsd_parser;

MPP_RET  avsd_init   (void *decoder, ParserCfg *cfg);
MPP_RET  avsd_deinit (void *decoder);
MPP_RET  avsd_reset  (void *decoder);
MPP_RET  avsd_flush  (void *decoder);
MPP_RET  avsd_control(void *decoder, RK_S32 cmd_type, void *param);
MPP_RET  avsd_prepare(void *decoder, MppPacket pkt, HalDecTask *task);
MPP_RET  avsd_parse  (void *decoder, HalDecTask *task);
MPP_RET  avsd_callback(void *decoder, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_API_H__*/
