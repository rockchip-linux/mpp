/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#include "mpp_common.h"

static const RK_U8 log2_tab[256] = {
    0, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

RK_S32 mpp_log2(RK_U32 v)
{
    RK_S32 n = 0;

    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += log2_tab[v];

    return n;
}

RK_S32 mpp_log2_16bit(RK_U32 v)
{
    RK_S32 n = 0;

    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += log2_tab[v];

    return n;
}

RK_S32 axb_div_c(RK_S32 a, RK_S32 b, RK_S32 c)
{
    RK_U32 left = 32;
    RK_U32 right = 0;
    RK_U32 shift;
    RK_S32 sign = 1;
    RK_S32 tmp;

    if (a == 0 || b == 0)
        return 0;
    else if ((a * b / b) == a && c != 0)
        return (a * b / c);

    if (a < 0) {
        sign = -1;
        a = -a;
    }
    if (b < 0) {
        sign *= -1;
        b = -b;
    }
    if (c < 0) {
        sign *= -1;
        c = -c;
    }

    if (c == 0)
        return 0x7FFFFFFF * sign;

    if (b > a) {
        tmp = b;
        b = a;
        a = tmp;
    }

    for (--left; (((RK_U32)a << left) >> left) != (RK_U32)a; --left)
        ;

    left--;

    while (((RK_U32)b >> right) > (RK_U32)c)
        right++;

    if (right > left) {
        return 0x7FFFFFFF * sign;
    } else {
        shift = left - right;
        return (RK_S32)((((RK_U32)a << shift) /
                         (RK_U32)c * (RK_U32)b) >> shift) * sign;
    }
}

RK_U32 mpp_align_16(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

RK_U32 mpp_align_64(RK_U32 val)
{
    return MPP_ALIGN(val, 64);
}

RK_U32 mpp_align_128(RK_U32 val)
{
    return MPP_ALIGN(val, 128);
}

RK_U32 mpp_align_256_odd(RK_U32 val)
{
    return MPP_ALIGN(val, 256) | 256;
}

RK_U32 mpp_align_128_odd_plus_64(RK_U32 val)
{
    val = MPP_ALIGN(val, 64);
    if (((val - 64) % 256 == 128))
        return val;
    else
        return ((MPP_ALIGN(val, 128) | 128) + 64);
}
