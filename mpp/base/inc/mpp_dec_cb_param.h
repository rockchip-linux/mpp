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

#ifndef __MPP_CALLBACK_OPS_H__
#define __MPP_CALLBACK_OPS_H__

#include "rk_type.h"

typedef enum MppCallBackCmd_e {
    MPP_CALLBACK_NONE       = 0,

    DEC_CALLBACK_BASE       = 0x100,
    DEC_PARSER_CALLBACK     = (DEC_CALLBACK_BASE + 1),

    ENC_CALLBACK_BASE       = 0x200,
    MPP_CALLBACK_CMD_BUTT,
} MppCbCmd;

/* DEC_PARSER_CALLBACK */
typedef struct DecCallBackParam_t {
    void    *task;
    RK_U32  *regs;
    RK_U32  hard_err;
} DecCbHalDone;

#endif /* __MPP_CALLBACK_OPS_H__ */
