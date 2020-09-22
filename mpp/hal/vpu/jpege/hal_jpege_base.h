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

#ifndef __HAL_JPEGE_BASE_H__
#define __HAL_JPEGE_BASE_H__

#include "mpp_device.h"
#include "mpp_hal.h"
#include "jpege_syntax.h"

typedef struct JpegeIocRegInfo_t {
    RK_U32              *regs;
    RegExtraInfo        extra_info;
} JpegeIocRegInfo;

typedef struct HalJpegeRc_t {
    /* For quantization table */
    RK_S32              q_factor;
    RK_U8               *qtable_y;
    RK_U8               *qtable_c;
    RK_S32              last_quality;
} HalJpegeRc;

typedef struct hal_jpege_ctx_s {
    IOInterruptCB       int_cb;
    MppDevCtx           dev_ctx;
    JpegeBits           bits;
    JpegeIocRegInfo     ioctl_info;

    MppEncCfgSet        *cfg;
    MppEncCfgSet        *set;
    JpegeSyntax         syntax;
    JpegeFeedback       feedback;

    HalJpegeRc          hal_rc;
    RK_S32              hal_start_pos;
} HalJpegeCtx;

#endif /* __HAL_JPEGE_BASE_H__ */
