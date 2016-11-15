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
#include "avsd_syntax.h"


#define AVSD_HAL_DBG_ERROR             (0x00000001)
#define AVSD_HAL_DBG_ASSERT            (0x00000002)
#define AVSD_HAL_DBG_WARNNING          (0x00000004)
#define AVSD_HAL_DBG_TRACE             (0x00000008)



extern RK_U32 avsd_hal_debug;

typedef struct avsd_hal_picture_t {
    RK_U32 valid;
    RK_U32 pic_type;
    RK_U32 pic_code_type;
    RK_U32 picture_distance;

    RK_S32 slot_idx;
} AvsdHalPic_t;


typedef struct avsd_hal_ctx_t {

    MppBufSlots              frame_slots;
    MppBufSlots              packet_slots;
    MppBufferGroup           buf_group;
    IOInterruptCB            init_cb;

    RK_S32                   vpu_socket;
    AvsdSyntax_t             syn;
    RK_U32                  *p_regs;
    MppBuffer                mv_buf;

    AvsdHalPic_t             pic[3];
    //!< add for control
    RK_U32                   first_field;
    RK_U32                   prev_pic_structure;
    RK_U32                   prev_pic_code_type;
    RK_S32                   future2prev_past_dist;
    RK_S32                   work0;
    RK_S32                   work1;
    RK_S32                   work_out;
    RK_U32                   data_offset;

    RK_U32                   frame_no;
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
