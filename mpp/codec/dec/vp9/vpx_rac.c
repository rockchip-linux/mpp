/*
*
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


#include "vpx_rac.h"

#define DEF( name, bytes, read, write) \
    static unsigned int bytestream_get_ ## name(const uint8_t **b) \
    { \
        (*b) += bytes; \
        return read(*b - bytes); \
    }

DEF(be24, 3, MPP_RB24, MPP_WB24)
DEF(be16, 2, MPP_RB16, MPP_WB16)

const uint8_t vpx_norm_shift[256] = {
    8, 7, 6, 6, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void vpx_init_range_decoder(VpxRangeCoder *c, const uint8_t *buf, int buf_size)
{
    c->high = 255;
    c->bits = -16;
    c->buffer = buf;
    c->end = buf + buf_size;
    c->code_word = bytestream_get_be24(&c->buffer);
}

unsigned int vpx_rac_renorm(VpxRangeCoder *c)
{
    int shift = vpx_norm_shift[c->high];
    int bits = c->bits;
    unsigned int code_word = c->code_word;

    c->high   <<= shift;
    code_word <<= shift;
    bits       += shift;
    if (bits >= 0 && c->buffer < c->end) {
        code_word |= bytestream_get_be16(&c->buffer) << bits;
        bits -= 16;
    }
    c->bits = bits;
    return code_word;
}

int vpx_rac_get_prob(VpxRangeCoder *c, uint8_t prob)
{
    unsigned int code_word = vpx_rac_renorm(c);
    unsigned int low = 1 + (((c->high - 1) * prob) >> 8);
    unsigned int low_shift = low << 16;
    int bit = code_word >= low_shift;

    c->high = bit ? c->high - low : low;
    c->code_word = bit ? code_word - low_shift : code_word;

    return bit;
}

// branchy variant, to be used where there's a branch based on the bit decoded
int vpx_rac_get_prob_branchy(VpxRangeCoder *c, int prob)
{
    unsigned long code_word = vpx_rac_renorm(c);
    unsigned low = 1 + (((c->high - 1) * prob) >> 8);
    unsigned low_shift = low << 16;

    if (code_word >= low_shift) {
        c->high     -= low;
        c->code_word = code_word - low_shift;
        return 1;
    }

    c->high = low;
    c->code_word = code_word;
    return 0;
}

// rounding is different than vpx_rac_get, is vpx_rac_get wrong?
int vpx_rac_get(VpxRangeCoder *c)
{
    return vpx_rac_get_prob(c, 128);
}

int vpx_rac_get_uint(VpxRangeCoder *c, int bits)
{
    int value = 0;

    while (bits--) {
        value = (value << 1) | vpx_rac_get(c);
    }

    return value;
}

