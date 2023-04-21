/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "h2645d_sei"

#include <stdint.h>
#include <string.h>

#include "mpp_mem.h"
#include "h2645d_sei.h"

static RK_U8 const deny_uuid[2][16] = {{
        0xfa, 0x5e, 0xfe, 0x40, 0xa6, 0xaf, 0x11, 0xdd,
        0xa6, 0x59, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b
    },
    {
        0x68, 0x55, 0x98, 0x4e, 0x49, 0x9c, 0x45, 0xc5,
        0x8e, 0x5b, 0xf2, 0x7b, 0xd1, 0xd4, 0xac, 0xe6
    }
};

static RK_U8 const deny_identity[3][4] = {
    {0x44, 0x69, 0x76, 0x58},
    {0x50, 0x6C, 0x75, 0x73},
    {0x48, 0x45, 0x56, 0x43}
};

MPP_RET check_encoder_sei_info(BitReadCtx_t *gb, RK_S32 payload_size, RK_U32 *is_match)
{
    RK_S32 i = 0;

    RK_U8 *payload = NULL;

    if (payload_size < 25 || payload_size >= INT32_MAX - 1)
        return MPP_ERR_STREAM;

    payload = mpp_calloc(RK_U8, payload_size);

    for (i = 0; i < payload_size; i++)
        READ_BITS(gb, 8, &payload[i]);

    if ((!memcmp(payload, deny_uuid[0], 16) ||
         !memcmp(payload, deny_uuid[1], 16)) &&
        (!memcmp(payload + 16, deny_identity[0], 4)) &&
        (!memcmp(payload + 21, deny_identity[1], 4) ||
         !memcmp(payload + 21, deny_identity[2], 4))) {
        *is_match = 1;
    }

    mpp_free(payload);
    return MPP_OK;;
__BITREAD_ERR:
    mpp_free(payload);
    return gb->ret;
}
