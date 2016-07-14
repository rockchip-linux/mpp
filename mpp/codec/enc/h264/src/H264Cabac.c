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

#include "H264Cabac.h"
#include "H264CabacContext.h"
#include "enccommon.h"
#include "enccfg.h"

#define CLIP3(x,y,z)    (((z) < (x)) ? (x) : (((z) > (y)) ? (y) : (z)))

static void SwapEndianess(u32 * buf, u32 sizeBytes)
{
#if (ENC6820_OUTPUT_SWAP_8 == 1)
    u32 i = 0;
    i32 words = sizeBytes / 4;

    ASSERT((sizeBytes % 8) == 0);

    while (words > 0) {
        u32 val = buf[i];
        u32 tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;

#if(ENC6820_OUTPUT_SWAP_32 == 1)    /* need this for 64-bit HW */
        {
            u32 val2 = buf[i + 1];
            u32 tmp2 = 0;

            tmp2 |= (val2 & 0xFF) << 24;
            tmp2 |= (val2 & 0xFF00) << 8;
            tmp2 |= (val2 & 0xFF0000) >> 8;
            tmp2 |= (val2 & 0xFF000000) >> 24;

            buf[i] = tmp2;
            words--;
            i++;
        }
#endif
        buf[i] = tmp;
        words--;
        i++;
    }
#endif

}

void H264CabacInit(u32 * contextTable, u32 cabac_init_idc)
{
    const i32(*ctx)[460][2];
    int i, j, qp;
    u8 *table = (u8 *) contextTable;

    for (qp = 0; qp < 52; qp++) { /* All QP values */
        for (j = 0; j < 2; j++) { /* Intra/Inter */
            if (j == 0)
                ctx = /*lint -e(545) */ &h264ContextInitIntra;
            else
                ctx = /*lint -e(545) */ &h264ContextInit[cabac_init_idc];

            for (i = 0; i < 460; i++) {
                i32 m = (i32) (*ctx)[i][0];
                i32 n = (i32) (*ctx)[i][1];

                i32 preCtxState = CLIP3(1, 126, ((m * (i32) qp) >> 4) + n);

                if (preCtxState <= 63) {
                    table[qp * 464 * 2 + j * 464 + i] =
                        (u8) ((63 - preCtxState) << 1);
                } else {
                    table[qp * 464 * 2 + j * 464 + i] =
                        (u8) (((preCtxState - 64) << 1) | 1);
                }
            }
        }
    }

    SwapEndianess(contextTable, 52 * 2 * 464);
}
