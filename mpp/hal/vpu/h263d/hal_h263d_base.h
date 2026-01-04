/*
 * Copyright 2016 Rockchip Electronics Co. LTD
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

#ifndef HAL_H263D_BASE_H
#define HAL_H263D_BASE_H

#include "mpp_hal.h"

#define H263D_HAL_DBG_REG_PUT       (0x00000001)
#define H263D_HAL_DBG_REG_GET       (0x00000002)

extern RK_U32 hal_h263d_debug;

typedef struct h263d_reg_context {
    MppHalCfg            *cfg;

    // save fd for curr/ref0/ref1 for reg_gen
    RK_S32              vpu_fd;
    RK_S32              fd_curr;
    RK_S32              fd_ref0;

    void                *regs;
} hal_h263_ctx;

#endif
