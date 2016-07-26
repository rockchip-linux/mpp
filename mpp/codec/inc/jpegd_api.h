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

#ifndef __JPEGD_API_H__
#define __JPEGD_API_H__
#include "parser_api.h"

#ifdef  __cplusplus
extern "C" {
#endif

extern const ParserApi api_jpegd_parser;

extern RK_U32 jpegd_log;
#define JPEGD_VBE_LOG (0x01)
#define JPEGD_DBG_LOG (0x02)
#define JPEGD_INF_LOG (0x04)
#define JPEGD_ERR_LOG (0x08)
#define JPEGD_DBG_ASSERT (1)

#define FUN_TEST(tag) \
    do {\
        if (JPEGD_VBE_LOG & jpegd_log)\
            { mpp_log("[Verbose] %s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)


#define JPEGD_ASSERT(val)\
		do {\
			if (JPEGD_DBG_ASSERT)\
				{ mpp_assert(val); }\
		} while (0)


//check function return
#define CHECK_FUN(val)                                            \
			do{ 													  \
				if((val) < 0) { 									  \
					ret = (val);									  \
					mpp_log("func return error(Line %d), ret:%d\n", __LINE__, ret); \
					goto __FAILED;									  \
				}													  \
			  } while (0)

//memory malloc check
#define CHECK_MEM(val, ...)\
					do{ if(!(val)) {\
					ret = MPP_ERR_MALLOC;\
					mpp_log("malloc buffer error(Line %d), pointer:%p\n", __LINE__, val);\
					goto __FAILED;\
					} } while (0)

#define JPEGD_VERBOSE_LOG(fmt, ...) \
					do {\
						if (JPEGD_VBE_LOG & jpegd_log)\
							{ mpp_log("[Verbose] func(%s), line(%d), "fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__); }\
					} while (0)

#define JPEGD_DEBUG_LOG(fmt, ...) \
					do {\
						if (JPEGD_DBG_LOG & jpegd_log)\
							{ mpp_log("[Debug] func(%s), line(%d), "fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__); }\
					} while (0)

#define JPEGD_INFO_LOG(fmt, ...) \
					do {\
						if (JPEGD_INF_LOG & jpegd_log)\
							{ mpp_log("[Info] func(%s), line(%d), "fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__); }\
					} while (0)					
					
#define JPEGD_ERROR_LOG(fmt, ...) \
					do {\
						if (JPEGD_ERR_LOG & jpegd_log)\
							{ mpp_log("[Error] func(%s), line(%d), "fmt"", __FUNCTION__, __LINE__, ##__VA_ARGS__); }\
					} while (0)


MPP_RET jpegd_prepare(void *ctx, MppPacket pkt, HalDecTask *task);
MPP_RET jpegd_init(void *ctx, ParserCfg *parser_cfg);
MPP_RET jpegd_parse(void *ctx, HalDecTask *task);
MPP_RET jpegd_deinit(void *ctx);
MPP_RET jpegd_flush(void *ctx);
MPP_RET jpegd_reset(void *ctx);
MPP_RET jpegd_control(void *ctx, RK_S32 cmd, void *param);
MPP_RET jpegd_callback(void *ctx, void *err_info);

#ifdef  __cplusplus
}
#endif

#endif /*__JPEGD_API_H__*/
