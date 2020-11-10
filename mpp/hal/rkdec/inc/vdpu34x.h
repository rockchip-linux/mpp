/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __VDPU34X_H__
#define __VDPU34X_H__

#include "rk_type.h"

#define HWID_VDPU34X                (0x032a3f03)

#define OFFSET_COMMON_REGS          (8 * sizeof(RK_U32))
#define OFFSET_H264D_PARAMS_REGS    (64 * sizeof(RK_U32))
#define OFFSET_H265D_PARAMS_REGS    (64 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS     (128 * sizeof(RK_U32))
#define OFFSET_H264D_ADDR_REGS      (160 * sizeof(RK_U32))
#define OFFSET_H265D_ADDR_REGS      (160 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS       (224 * sizeof(RK_U32))
#define OFFSET_STATISTIC_REGS       (256 * sizeof(RK_U32))

#define RCB_ALLINE_SIZE     (64)
#define RCB_INTRAR_COEF     (6)
#define RCB_TRANSDR_COEF    (1)
#define RCB_TRANSDC_COEF    (1)
#define RCB_STRMDR_COEF     (6)
#define RCB_INTERR_COEF     (6)
#define RCB_INTERC_COEF     (3)
#define RCB_DBLKR_COEF      (21)
#define RCB_SAOR_COEF       (5)
#define RCB_FBCR_COEF       (10)
#define RCB_FILTC_COEF      (67)

#endif /* __VDPU34X_H__ */
