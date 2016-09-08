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


#ifndef __HAL_AVSD_API_H__
#define __HAL_AVSD_API_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_hal.h"



#define AVSD_HAL_DBG_ERROR             (0x00000001)
#define AVSD_HAL_DBG_ASSERT            (0x00000002)
#define AVSD_HAL_DBG_WARNNING          (0x00000004)
#define AVSD_HAL_DBG_TRACE             (0x00000008)



extern RK_U32 avsd_hal_debug;



#define AVSD_HAL_DBG(level, fmt, ...)\
do {\
    if (level & avsd_hal_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define AVSD_HAL_TRACE(fmt, ...)\
do {\
    if (AVSD_HAL_DBG_TRACE & avsd_hal_debug)\
        { mpp_log_f(fmt, ## __VA_ARGS__); }\
} while (0)

#ifdef INP_CHECK
#undef INP_CHECK
#endif

//!< input check
#define INP_CHECK(ret, val, ...)\
do{\
if ((val)) { \
        ret = MPP_ERR_INIT; \
        AVSD_HAL_DBG(AVSD_HAL_DBG_WARNNING, "input empty(%d).\n", __LINE__); \
        goto __RETURN; \
}} while (0)


typedef struct avsd_hal_ctx_t {

    MppBufSlots              frame_slots;
    MppBufSlots              packet_slots;
    IOInterruptCB            init_cb;

} AvsdHalCtx_t;




#ifdef __cplusplus
extern "C" {
#endif

extern const MppHalApi hal_api_avsd;

MPP_RET hal_avsd_init    (void *decoder, MppHalCfg *cfg);
MPP_RET hal_avsd_deinit  (void *decoder);
MPP_RET hal_avsd_gen_regs(void *decoder, HalTaskInfo *task);
MPP_RET hal_avsd_start   (void *decoder, HalTaskInfo *task);
MPP_RET hal_avsd_wait    (void *decoder, HalTaskInfo *task);
MPP_RET hal_avsd_reset   (void *decoder);
MPP_RET hal_avsd_flush   (void *decoder);
MPP_RET hal_avsd_control (void *decoder, RK_S32 cmd_type, void *param);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_AVSD_API_H__*/
