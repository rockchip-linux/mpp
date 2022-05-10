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

#define MODULE_TAG "h264e_sei"

#include <string.h>

#include "mpp_common.h"

#include "mpp_bitwrite.h"
#include "mpp_packet_impl.h"
#include "h264e_debug.h"

#include "h264_syntax.h"
#include "h264e_sei.h"

MPP_RET h264e_sei_to_packet(MppPacket packet, RK_S32 *len, RK_S32 type,
                            RK_U8 uuid[16], const void *data, RK_S32 size)
{
    void *pos = mpp_packet_get_pos(packet);
    void *pkt_base = mpp_packet_get_data(packet);
    size_t pkt_size = mpp_packet_get_size(packet);
    size_t length = mpp_packet_get_length(packet);
    const RK_U8 *src = (RK_U8 *)data;
    void *dst = pos + length;
    RK_S32 buf_size = (pkt_base + pkt_size) - (pos + length);
    MppWriteCtx bit_ctx;
    MppWriteCtx *bit = &bit_ctx;
    RK_S32 uuid_size = 16;
    RK_S32 payload_size = size + uuid_size;
    RK_S32 sei_size = 0;
    RK_S32 i;

    h264e_dbg_sei("write sei to pkt [%p:%u] [%p:%u]\n", pkt_base, pkt_size,
                  pos, length);

    mpp_writer_init(bit, dst, buf_size);

    /* start_code_prefix 00 00 00 01 */
    mpp_writer_put_raw_bits(bit, 0, 24);
    mpp_writer_put_raw_bits(bit, 1, 8);
    /* forbidden_zero_bit */
    mpp_writer_put_raw_bits(bit, 0, 1);
    /* nal_ref_idc */
    mpp_writer_put_raw_bits(bit, H264_NALU_PRIORITY_DISPOSABLE, 2);
    /* nal_unit_type */
    mpp_writer_put_raw_bits(bit, H264_NALU_TYPE_SEI, 5);

    /* sei_payload_type_ff_byte */
    for (i = 0; i <= type - 255; i += 255)
        mpp_writer_put_bits(bit, 0xff, 8);

    /* sei_last_payload_type_byte */
    mpp_writer_put_bits(bit, type - i, 8);

    /* sei_payload_size_ff_byte */
    for (i = 0; i <= payload_size - 255; i += 255)
        mpp_writer_put_bits(bit, 0xff, 8);

    /* sei_last_payload_size_byte */
    mpp_writer_put_bits(bit, payload_size - i, 8);

    /* uuid_iso_iec_11578 */
    for (i = 0; i < uuid_size; i++)
        mpp_writer_put_bits(bit, uuid[i], 8);

    /* sei_payload_data */
    for (i = 0; i < size; i++)
        mpp_writer_put_bits(bit, src[i], 8);

    mpp_writer_trailing(bit);

    sei_size = mpp_writer_bytes(bit);
    if (len)
        *len = sei_size;

    mpp_packet_set_length(packet, length + sei_size);
    mpp_packet_add_segment_info(packet, H264_NALU_TYPE_SEI, length, sei_size);

    h264e_dbg_sei("sei data length %d pkt len %d -> %d\n", sei_size,
                  length, length + sei_size);

    return MPP_OK;
}
