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

#include "mpp_common.h"
#include "mpp_mem.h"
#include "hal_h264e_com.h"
#include "hal_h264e_rkv_stream.h"

static const RK_U8 h264e_ue_size_tab[256] = {
    1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
};

RK_S32 h264e_rkv_stream_get_pos(H264eRkvStream *s)
{
    return (RK_S32)(8 * (s->p - s->p_start) + (4 * 8) - s->i_left);
}

MPP_RET h264e_rkv_stream_init(H264eRkvStream *s)
{
    RK_S32 offset = 0;
    s->buf = mpp_calloc(RK_U8, H264E_EXTRA_INFO_BUF_SIZE);
    s->buf_plus8 = s->buf + 8; //NOTE: prepare for align

    offset = (size_t)(s->buf_plus8) & 3;
    s->p = s->p_start = s->buf_plus8 - offset;
    //s->p_end = (RK_U8*)s->buf + i_data;
    s->i_left = (4 - offset) * 8;
    //s->cur_bits = endian_fix32(M32(s->p));
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                  + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;

    return MPP_OK;
}

MPP_RET h264e_rkv_stream_deinit(H264eRkvStream *s)
{
    MPP_FREE(s->buf);

    s->p = NULL;
    s->p_start = NULL;
    //s->p_end = NULL;

    s->i_left = 0;
    s->cur_bits = 0;
    s->count_bit = 0;

    return MPP_OK;
}

MPP_RET h264e_rkv_stream_reset(H264eRkvStream *s)
{
    RK_S32 offset = 0;
    h264e_hal_enter();

    offset = (size_t)(s->buf_plus8) & 3;
    s->p = s->p_start;
    s->i_left = (4 - offset) * 8;
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                  + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET h264e_rkv_stream_realign(H264eRkvStream *s)
{
    RK_S32 offset = (size_t)(s->p) & 3; //used to judge alignment
    if (offset) {
        s->p = s->p - offset; //move pointer to 32bit aligned pos
        s->i_left = (4 - offset) * 8; //init
        //s->cur_bits = endian_fix32(M32(s->p));
        s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16)
                      + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
        s->cur_bits >>= (4 - offset) * 8; //shift right the invalid bit
    }

    return MPP_OK;
}

MPP_RET h264e_rkv_stream_write_with_log(H264eRkvStream *s,
                                        RK_S32 i_count, RK_U32 val, char *name)
{
    RK_U32 i_bits = val;

    h264e_hal_dbg(H264E_DBG_HEADER, "write bits name %s, count %d, val %d",
                  name, i_count, val);

    s->count_bit += i_count;
    if (i_count < s->i_left) {
        s->cur_bits = (s->cur_bits << i_count) | i_bits;
        s->i_left -= i_count;
    } else {
        i_count -= s->i_left;
        s->cur_bits = (s->cur_bits << s->i_left) | (i_bits >> i_count);
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = 0;
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->cur_bits = i_bits;
        s->i_left = 32 - i_count;
    }
    return MPP_OK;
}

MPP_RET h264e_rkv_stream_write1_with_log(H264eRkvStream *s,
                                         RK_U32 val, char *name)
{
    RK_U32 i_bit = val;

    h264e_hal_dbg(H264E_DBG_HEADER, "write 1 bit name %s, val %d", name, val);

    s->count_bit += 1;
    s->cur_bits <<= 1;
    s->cur_bits |= i_bit;
    s->i_left--;
    if (s->i_left == 4 * 8 - 32) {
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->i_left = 4 * 8;
    }
    return MPP_OK;
}

MPP_RET h264e_rkv_stream_write_ue_with_log(H264eRkvStream *s,
                                           RK_U32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = ++val;

    h264e_hal_dbg(H264E_DBG_HEADER,
                  "write UE bits name %s, val %d (2 steps below are real writting)",
                  name, val);
    if (tmp >= 0x10000) {
        size = 32;
        tmp >>= 16;
    }
    if (tmp >= 0x100) {
        size += 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    h264e_rkv_stream_write_with_log(s, size >> 1, 0, name);
    h264e_rkv_stream_write_with_log(s, (size >> 1) + 1, val, name);

    return MPP_OK;
}

MPP_RET h264e_rkv_stream_write_se_with_log(H264eRkvStream *s,
                                           RK_S32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = 1 - val * 2;
    if (tmp < 0)
        tmp = val * 2;

    val = tmp;
    if (tmp >= 0x100) {
        size = 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    return h264e_rkv_stream_write_with_log(s, size, val, name);
}

MPP_RET
h264e_rkv_stream_write32(H264eRkvStream *s, RK_U32 i_bits, char *name)
{
    h264e_rkv_stream_write_with_log(s, 16, i_bits >> 16, name);
    h264e_rkv_stream_write_with_log(s, 16, i_bits      , name);

    return MPP_OK;
}

RK_S32 h264e_rkv_stream_size_se( RK_S32 val )
{
    RK_S32 tmp = 1 - val * 2;
    if ( tmp < 0 ) tmp = val * 2;
    if ( tmp < 256 )
        return h264e_ue_size_tab[tmp];
    else
        return h264e_ue_size_tab[tmp >> 8] + 16;
}

MPP_RET h264e_rkv_stream_rbsp_trailing(H264eRkvStream *s)
{
    //align bits, 1+N zeros.
    h264e_rkv_stream_write1_with_log(s, 1, "align_1_bit");
    h264e_rkv_stream_write_with_log(s, s->i_left & 7, 0, "align_N_bits");

    return MPP_OK;
}

/*
 * Write the rest of cur_bits to the bitstream;
 * results in a bitstream no longer 32-bit aligned.
 */
MPP_RET h264e_rkv_stream_flush(H264eRkvStream *s)
{
    //do 4 bytes aligned
    //M32(s->p) = endian_fix32(s->cur_bits << (s->i_left & 31));
    RK_U32 tmp_bit = s->cur_bits << (s->i_left & 31);
    *(s->p) = (tmp_bit >> 24) & 0xff;
    *(s->p + 1) = (tmp_bit >> 16) & 0xff;
    *(s->p + 2) = (tmp_bit >> 8) & 0xff;
    *(s->p + 3) = (tmp_bit >> 0) & 0xff;
    /*
     * p point to bit which aligned, rather than
     * the pos next to 4-byte alinged
     */
    s->p += 4 - (s->i_left >> 3);
    s->i_left = 4 * 8;

    return MPP_OK;
}


