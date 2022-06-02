/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#ifndef __MPP_ENC_CB_PARAM_H__
#define __MPP_ENC_CB_PARAM_H__

#include "rk_type.h"

typedef enum MppEncCbCmd_e {
    ENC_CALLBACK_NONE       = 0,

    ENC_CALLBACK_BASE       = 0x200,
    ENC_OUTPUT_FINISH       = (ENC_CALLBACK_BASE + 1),
    ENC_OUTPUT_SLICE        = (ENC_CALLBACK_BASE + 2),
    ENC_CALLBACK_CMD_BUTT,
} MppEncCbCmd;

/* DEC_PARSER_CALLBACK */
typedef struct EncOutParam_t {
    void    *task;
    void    *base;
    RK_S32  start;
    RK_S32  length;
} EncOutParam;

#endif /* __MPP_ENC_CB_PARAM_H__ */
