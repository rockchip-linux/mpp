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

#define MODULE_TAG "hal_jpege_base"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_jpege_debug.h"
#include "hal_jpege_base.h"

const RK_U32 qp_reorder_table[64] = {
    0,  8, 16, 24,  1,  9, 17, 25, 32, 40, 48, 56, 33, 41, 49, 57,
    2, 10, 18, 26,  3, 11, 19, 27, 34, 42, 50, 58, 35, 43, 51, 59,
    4, 12, 20, 28,  5, 13, 21, 29, 36, 44, 52, 60, 37, 45, 53, 61,
    6, 14, 22, 30,  7, 15, 23, 31, 38, 46, 54, 62, 39, 47, 55, 63
};

const RK_U16 jpege_restart_marker[8] = {
    0xFFD0,  0xFFD1,  0xFFD2, 0xFFD3,
    0xFFD4,  0xFFD5,  0xFFD6, 0xFFD7,
};

void get_msb_lsb_at_pos(RK_U32 *msb, RK_U32 *lsb, RK_U8 *buf, RK_U32 bytepos)
{
    RK_U32 val32;
    RK_S32 left_byte = bytepos & 0x7;
    RK_U8 *tmp = buf + (bytepos & (~0x7));

    // clear the rest bytes in 64bit
    if (left_byte) {
        RK_U32 i;

        for (i = left_byte; i < 8; i++)
            tmp[i] = 0;
    }

    val32 = (tmp[0] << 24) |
            (tmp[1] << 16) |
            (tmp[2] <<  8) |
            (tmp[3] <<  0);

    *msb = val32;

    if (left_byte > 4) {
        val32 = (tmp[4] << 24) |
                (tmp[5] << 16) |
                (tmp[6] <<  8);
    } else
        val32 = 0;

    *lsb = val32;
}
