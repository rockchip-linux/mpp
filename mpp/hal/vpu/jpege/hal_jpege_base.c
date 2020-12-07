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

/*
 *  from RFC435 spec.
 */
const RK_U8 jpege_luma_quantizer[QUANTIZE_TABLE_SIZE] = {
    16, 11, 10, 16, 24, 40, 51, 61,
    12, 12, 14, 19, 26, 58, 60, 55,
    14, 13, 16, 24, 40, 57, 69, 56,
    14, 17, 22, 29, 51, 87, 80, 62,
    18, 22, 37, 56, 68, 109, 103, 77,
    24, 35, 55, 64, 81, 104, 113, 92,
    49, 64, 78, 87, 103, 121, 120, 101,
    72, 92, 95, 98, 112, 100, 103, 99
};

const RK_U8 jpege_chroma_quantizer[QUANTIZE_TABLE_SIZE] = {
    17, 18, 24, 47, 99, 99, 99, 99,
    18, 21, 26, 66, 99, 99, 99, 99,
    24, 26, 56, 99, 99, 99, 99, 99,
    47, 66, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99
};

const RK_U16 jpege_restart_marker[8] = {
    0xFFD0,  0xFFD1,  0xFFD2, 0xFFD3,
    0xFFD4,  0xFFD5,  0xFFD6, 0xFFD7,
};

MPP_RET hal_jpege_vepu_rc(HalJpegeCtx *ctx, HalEncTask *task)
{
    HalJpegeRc *hal_rc = &ctx->hal_rc;
    EncRcTaskInfo *rc_info = (EncRcTaskInfo *)&task->rc_task->info;

    if (rc_info->quality_target != hal_rc->last_quality) {
        RK_U32 i = 0;
        RK_S32 q = 0;

        hal_rc->q_factor = 100 - rc_info->quality_target;
        hal_jpege_dbg_input("use qfactor=%d, rc_info->quality_target=%d\n", hal_rc->q_factor, rc_info->quality_target);

        if (!hal_rc->qtable_y)
            hal_rc->qtable_y = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);
        if (!hal_rc->qtable_c)
            hal_rc->qtable_c = mpp_malloc(RK_U8, QUANTIZE_TABLE_SIZE);
        if (NULL == hal_rc->qtable_y || NULL == hal_rc->qtable_c) {
            mpp_err_f("qtable is null, malloc err\n");
            return MPP_ERR_MALLOC;
        }

        q = hal_rc->q_factor;
        if (q < 50)
            q = 5000 / q;
        else
            q = 200 - (q << 1);

        for (i = 0; i < QUANTIZE_TABLE_SIZE; i++) {
            RK_S16 lq = (jpege_luma_quantizer[i] * q + 50) / 100;
            RK_S16 cq = (jpege_chroma_quantizer[i] * q + 50) / 100;

            /* Limit the quantizers to 1 <= q <= 255 */
            hal_rc->qtable_y[i] = MPP_CLIP3(1, 255, lq);
            hal_rc->qtable_c[i] = MPP_CLIP3(1, 255, cq);
        }
    }

    return MPP_OK;
}

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
