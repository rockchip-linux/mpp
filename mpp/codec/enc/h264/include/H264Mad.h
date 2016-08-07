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

#ifndef H264_MAD_H
#define H264_MAD_H

#include "enccommon.h"

#define DSCY                      32 /* n * 32 */
#define I32_MAX           2147483647 /* 2 ^ 31 - 1 */
#define DIV(a, b)       (((a) + (SIGN(a) * (b)) / 2) / (b))
#define MAD_TABLE_LEN              5

typedef struct {
    RK_S32  a1;               /* model parameter, y = a1*x + a2 */
    RK_S32  a2;               /* model parameter */
    RK_S32  th[MAD_TABLE_LEN];     /* mad threshold */
    RK_S32  count[MAD_TABLE_LEN];  /* number of macroblocks under threshold */
    RK_S32  pos;              /* current position */
    RK_S32  len;              /* current lenght */
    RK_S32  threshold;        /* current frame threshold */
    RK_S32  mbPerFrame;       /* number of macroblocks per frame */
} madTable_s;

/*------------------------------------------------------------------------------
    Function prototypes
------------------------------------------------------------------------------*/

void H264MadInit(madTable_s *mad, RK_U32 mbPerFrame);

void H264MadThreshold(madTable_s *madTable, RK_U32 madCount);

#endif

