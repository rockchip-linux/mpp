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

#define MODULE_TAG "h265e_stream"

#include "mpp_common.h"
#include "mpp_mem.h"

#include "h265e_stream.h"
#include "h265e_codec.h"

static const RK_U8 ue_size_tab[256] = {
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

MPP_RET h265e_stream_init(H265eStream *s)
{
    s->buf = mpp_calloc(RK_U8, H265E_EXTRA_INFO_BUF_SIZE);
    s->size = H265E_EXTRA_INFO_BUF_SIZE;
    mpp_writer_init(&s->enc_stream, s->buf, s->size);
    return MPP_OK;
}

MPP_RET h265e_stream_deinit(H265eStream *s)
{
    MPP_FREE(s->buf);
    mpp_writer_reset(&s->enc_stream);
    return MPP_OK;
}

MPP_RET h265e_stream_reset(H265eStream *s)
{
    mpp_writer_reset(&s->enc_stream);
    return MPP_OK;
}

MPP_RET h265e_stream_realign(H265eStream *s)
{
    if (s->enc_stream.buffered_bits)
        mpp_writer_trailing(&s->enc_stream);
    return MPP_OK;
}

MPP_RET h265e_stream_write_with_log(H265eStream *s,
                                    RK_U32 val, RK_S32 i_count, char *name)
{

    h265e_dbg(H265E_DBG_HEADER, "write bits name %s, count %d, val %d",
              name, i_count, val);

    mpp_writer_put_bits(&s->enc_stream, val, i_count);
    return MPP_OK;
}

MPP_RET h265e_stream_write1_with_log(H265eStream *s,
                                     RK_U32 val, char *name)
{
    h265e_dbg(H265E_DBG_HEADER, "write 1 bit name %s, val %d", name, val);

    mpp_writer_put_bits(&s->enc_stream, val, 1);

    return MPP_OK;
}

MPP_RET h265e_stream_write_ue_with_log(H265eStream *s,
                                       RK_U32 val, char *name)
{

    h265e_dbg(H265E_DBG_HEADER,
              "write UE bits name %s, val %d ",
              name, val);
#if 1
    RK_S32 size = 0;
    RK_S32 tmp = ++val;

    h265e_dbg(H265E_DBG_HEADER,
              "write UE bits name %s, val %d ",
              name, val);
    if (tmp >= 0x10000) {
        size = 32;
        tmp >>= 16;
    }
    if (tmp >= 0x100) {
        size += 16;
        tmp >>= 8;
    }
    size += ue_size_tab[tmp];

    h265e_stream_write_with_log(s, 0, size >> 1, name);
    h265e_stream_write_with_log(s, val, (size >> 1) + 1, name);
#else
    mpp_writer_write_ue(&s->enc_stream, val, name);
#endif
    return MPP_OK;
}

MPP_RET h265e_stream_write_se_with_log(H265eStream *s,
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
    size += ue_size_tab[tmp];

    h265e_dbg(H265E_DBG_HEADER,
              "write SE bits name %s, val %d ",
              name, val);

    return h265e_stream_write_with_log(s, val, size, name);;
}

MPP_RET h265e_stream_write32(H265eStream *s, RK_U32 i_bits,
                             char *name)
{
    h265e_stream_write_with_log(s, i_bits >> 16, 16, name);
    h265e_stream_write_with_log(s, i_bits, 16, name);
    return MPP_OK;
}

RK_S32 h265e_stream_size_se( RK_S32 val )
{
    RK_S32 tmp = 1 - val * 2;
    if ( tmp < 0 ) tmp = val * 2;
    if ( tmp < 256 )
        return ue_size_tab[tmp];
    else
        return ue_size_tab[tmp >> 8] + 16;
}

MPP_RET h265e_stream_rbsp_trailing(H265eStream *s)
{
    //align bits, 1+N zeros.
    mpp_writer_trailing(&s->enc_stream);
    return MPP_OK;
}

/*
 * Write the rest of cur_bits to the bitstream;
 * results in a bitstream no longer 32-bit aligned.
 */
MPP_RET h265e_stream_flush(H265eStream *s)
{
    if (s->enc_stream.buffered_bits)
        mpp_writer_trailing(&s->enc_stream);
    return MPP_OK;
}
