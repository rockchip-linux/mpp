/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h264e_amend"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_dmabuf.h"
#include "mpp_packet_impl.h"

#include "mpp_enc_cfg.h"
#include "mpp_enc_refs.h"

#include "hal_h264e_debug.h"
#include "hal_h264e_stream_amend.h"

#include "h264e_sps.h"
#include "h264e_pps.h"
#include "h264e_slice.h"

#define START_CODE 0x000001 ///< start_code_prefix_one_3bytes

static RK_S32 get_next_nal(RK_U8 *buf, RK_S32 *length)
{
    RK_S32 i, consumed = 0;
    RK_S32 len = *length;
    RK_U8 *tmp_buf = buf;

    /* search start code */
    while (len >= 4) {
        if (tmp_buf[2] == 0) {
            len--;
            tmp_buf++;
            continue;
        }

        if (tmp_buf[0] != 0 || tmp_buf[1] != 0 || tmp_buf[2] != 1) {
            RK_U32 state = (RK_U32) - 1;
            RK_S32 has_nal = 0;

            for (i = 0; i < (RK_S32)len; i++) {
                state = (state << 8) | tmp_buf[i];
                if (((state >> 8) & 0xFFFFFF) == START_CODE) {
                    has_nal = 1;
                    i = i - 3;
                    break;
                }
            }

            if (has_nal) {
                len -= i;
                tmp_buf += i;
                consumed = *length - len - 1;
                break;
            }

            consumed = *length;
            break;
        }
        tmp_buf   += 3;
        len       -= 3;
    }

    *length = *length - consumed;
    return consumed;
}

MPP_RET h264e_vepu_stream_amend_init(HalH264eVepuStreamAmend *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->buf_size = SZ_128K;
    return MPP_OK;
}

MPP_RET h264e_vepu_stream_amend_deinit(HalH264eVepuStreamAmend *ctx)
{
    MPP_FREE(ctx->src_buf);
    MPP_FREE(ctx->dst_buf);
    return MPP_OK;
}

MPP_RET h264e_vepu_stream_amend_config(HalH264eVepuStreamAmend *ctx,
                                       MppPacket packet, MppEncCfgSet *cfg,
                                       H264eSlice *slice, H264ePrefixNal *prefix)
{
    MppEncRefCfgImpl *ref = (MppEncRefCfgImpl *)cfg->ref_cfg;
    MppEncH264Cfg    *h264 = &cfg->h264;
    MppEncH264HwCfg  *hw_cfg = &h264->hw_cfg;

    if (ref->lt_cfg_cnt || ref->st_cfg_cnt > 1 ||
        hw_cfg->hw_poc_type != h264->poc_type ||
        hw_cfg->hw_log2_max_frame_num_minus4 != h264->log2_max_frame_num) {
        ctx->enable = 1;
        ctx->slice_enabled = 0;

        if (NULL == ctx->dst_buf)
            ctx->dst_buf = mpp_calloc(RK_U8, ctx->buf_size);
        if (NULL == ctx->src_buf)
            ctx->src_buf = mpp_calloc(RK_U8, ctx->buf_size);
    } else {
        MPP_FREE(ctx->dst_buf);
        MPP_FREE(ctx->src_buf);
        h264e_vepu_stream_amend_init(ctx);
    }

    slice->pic_order_cnt_type = cfg->h264.poc_type;

    ctx->slice = slice;
    if (ref->lt_cfg_cnt || ref->st_cfg_cnt > 1)
        ctx->prefix = prefix;

    ctx->packet = packet;
    ctx->buf_base = mpp_packet_get_length(packet);
    ctx->old_length = 0;
    ctx->new_length = 0;

    return MPP_OK;
}

MPP_RET h264e_vepu_stream_amend_proc(HalH264eVepuStreamAmend *ctx, MppEncH264HwCfg *hw_cfg)
{
    H264ePrefixNal *prefix = ctx->prefix;
    H264eSlice *slice = ctx->slice;
    MppPacket pkt = ctx->packet;
    RK_U8 *p = mpp_packet_get_pos(pkt);
    RK_S32 base = ctx->buf_base;
    RK_S32 len = ctx->old_length;
    RK_S32 hw_len_bit = 0;
    RK_S32 sw_len_bit = 0;
    RK_S32 hw_len_byte = 0;
    RK_S32 sw_len_byte = 0;
    RK_S32 diff_size = 0;
    RK_S32 tail_0bit = 0;
    RK_U8  tail_byte = 0;
    RK_U8  tail_tmp = 0;
    RK_U8 *dst_buf = NULL;
    RK_S32 buf_size;
    RK_S32 final_len = 0;
    RK_S32 last_slice = 0;
    const MppPktSeg *seg = mpp_packet_get_segment_info(pkt);
    MppPacket pkt_tmp;
    RK_S32 offset = 0;
    RK_U32 is_cabac = ctx->slice->entropy_coding_mode;

    mpp_packet_new(&pkt_tmp);

    if (seg) {
        while (seg && seg->type != 1 && seg->type != 5) {
            mpp_packet_add_segment_info(pkt_tmp, seg->type, offset, seg->len);
            offset += seg->len;

            seg = seg->next;
        }
    }

    {
        MppBuffer buf = mpp_packet_get_buffer(pkt);
        RK_S32 fd = mpp_buffer_get_fd(buf);

        mpp_dmabuf_sync_partial_begin(fd, 1, base, len, __FUNCTION__);
    }

    {
        RK_S32 more_buf = 0;
        while (len > ctx->buf_size - 16) {
            ctx->buf_size *= 2;
            more_buf = 1;
        }

        if (more_buf) {
            MPP_FREE(ctx->src_buf);
            MPP_FREE(ctx->dst_buf);
            ctx->src_buf = mpp_malloc(RK_U8, ctx->buf_size);
            ctx->dst_buf = mpp_malloc(RK_U8, ctx->buf_size);
        }
    }

    memset(ctx->dst_buf, 0, ctx->buf_size);
    memset(ctx->src_buf, 0, ctx->buf_size);
    dst_buf = ctx->dst_buf;
    buf_size = ctx->buf_size;
    p += base;

    do {
        RK_U32 nal_len = 0;

        tail_0bit = 0;
        // copy hw stream to stream buffer first
        if (slice->is_multi_slice) {
            if ((!seg) || !hw_cfg->hw_split_out || !is_cabac) {
                nal_len = get_next_nal(p, &len);
                last_slice = (len == 0);
            } else {
                nal_len = seg->len;
                len -= seg->len;
                seg = seg->next;
                if (!seg || !len)
                    last_slice = 1;
            }
            memcpy(ctx->src_buf, p, nal_len);
            p += nal_len;
        } else {
            memcpy(ctx->src_buf, p, len);
            nal_len = len;
            last_slice = 1;
        }

        hal_h264e_dbg_amend("nal_len %d multi %d last %d prefix %p\n",
                            nal_len, slice->is_multi_slice, last_slice, prefix);

        if (prefix) {
            /* add prefix for each slice */
            RK_S32 prefix_bit = h264e_slice_write_prefix_nal_unit_svc(prefix, dst_buf, buf_size);

            prefix_bit = (prefix_bit + 7) / 8;

            dst_buf += prefix_bit;
            buf_size -= prefix_bit;
            final_len += prefix_bit;

            mpp_packet_add_segment_info(pkt_tmp, H264_NALU_TYPE_PREFIX, offset, prefix_bit);
            offset += prefix_bit;
        }

        H264eSlice slice_rd;

        memcpy(&slice_rd, slice, sizeof(slice_rd));

        /* update slice by hw_cfg */
        slice_rd.pic_order_cnt_type = hw_cfg->hw_poc_type;
        slice_rd.log2_max_frame_num = hw_cfg->hw_log2_max_frame_num_minus4 + 4;

        if (ctx->reorder) {
            slice_rd.reorder = ctx->reorder;
            h264e_reorder_init(slice_rd.reorder);
        }
        if (ctx->marking) {
            slice_rd.marking = ctx->marking;
            h264e_marking_init(slice_rd.marking);
        }

        hw_len_bit = h264e_slice_read(&slice_rd, ctx->src_buf, nal_len);

        // write new header to header buffer
        slice->qp_delta = slice_rd.qp_delta;
        slice->first_mb_in_slice = slice_rd.first_mb_in_slice;

        if (ctx->reorder)
            slice->reorder = slice_rd.reorder;
        if (ctx->marking)
            slice->marking = slice_rd.marking;

        sw_len_bit = h264e_slice_write(slice, dst_buf, buf_size);

        hw_len_byte = (hw_len_bit + 7) / 8;
        sw_len_byte = (sw_len_bit + 7) / 8;

        tail_byte = ctx->src_buf[nal_len - 1];
        tail_tmp = tail_byte;

        while (!(tail_tmp & 1) && tail_0bit < 8) {
            tail_tmp >>= 1;
            tail_0bit++;
        }

        mpp_assert(tail_0bit < 8);

        // move the reset slice data from src buffer to dst buffer
        diff_size = h264e_slice_move(dst_buf, ctx->src_buf,
                                     sw_len_bit, hw_len_bit, nal_len);

        hal_h264e_dbg_amend("tail 0x%02x %d hw_hdr %d sw_hdr %d len %d hw_byte %d sw_byte %d diff %d\n",
                            tail_byte, tail_0bit, hw_len_bit, sw_len_bit, nal_len, hw_len_byte, sw_len_byte, diff_size);

        if (slice->entropy_coding_mode) {
            memcpy(dst_buf + sw_len_byte, ctx->src_buf + hw_len_byte,
                   nal_len - hw_len_byte);
            final_len += nal_len - hw_len_byte + sw_len_byte;
            nal_len = nal_len - hw_len_byte + sw_len_byte;
        } else {
            RK_S32 hdr_diff_bit = sw_len_bit - hw_len_bit;
            RK_S32 bit_len = nal_len * 8 - tail_0bit + hdr_diff_bit;
            RK_S32 new_len = (bit_len + diff_size * 8 + 7) / 8;

            hal_h264e_dbg_amend("frm %4d %c len %d bit hw %d sw %d byte hw %d sw %d diff %d -> %d\n",
                                slice->frame_num, (slice->idr_flag ? 'I' : 'P'),
                                nal_len, hw_len_bit, sw_len_bit,
                                hw_len_byte, sw_len_byte, diff_size, new_len);

            hal_h264e_dbg_amend("%02x %02x %02x %02x -> %02x %02x %02x %02x\n",
                                ctx->src_buf[nal_len - 4], ctx->src_buf[nal_len - 3],
                                ctx->src_buf[nal_len - 2], ctx->src_buf[nal_len - 1],
                                dst_buf[new_len - 4], dst_buf[new_len - 3],
                                dst_buf[new_len - 2], dst_buf[new_len - 1]);
            nal_len = new_len;
            final_len += new_len;
        }

        {
            H264NaluType type = slice->idr_flag ?  H264_NALU_TYPE_IDR : H264_NALU_TYPE_SLICE;

            mpp_packet_add_segment_info(pkt_tmp, type, offset, nal_len);
            offset += nal_len;
        }

        if (last_slice) {
            p = mpp_packet_get_pos(pkt);
            p += base;
            memcpy(p, ctx->dst_buf, final_len);

            if (slice->entropy_coding_mode) {
                if (final_len < ctx->old_length)
                    memset(p + final_len, 0,  ctx->old_length - final_len);
            } else
                p[final_len] = 0;

            break;
        }

        dst_buf += nal_len;
        buf_size -= nal_len;
    } while (1);

    ctx->new_length = final_len;

    /* update segment */
    mpp_packet_copy_segment_info(pkt, pkt_tmp);

    mpp_packet_deinit(&pkt_tmp);

    return MPP_OK;
}

MPP_RET h264e_vepu_stream_amend_sync_ref_idc(HalH264eVepuStreamAmend *ctx)
{
    H264eSlice *slice = ctx->slice;
    MppPacket pkt = ctx->packet;
    RK_S32 base = ctx->buf_base;
    RK_S32 len = ctx->old_length;
    RK_U8 *p = mpp_packet_get_pos(pkt) + base;
    RK_U8 val = 0;
    RK_S32 hw_nal_ref_idc = 0;
    RK_S32 sw_nal_ref_idc = 0;

    {
        MppBuffer buf = mpp_packet_get_buffer(pkt);
        RK_S32 fd = mpp_buffer_get_fd(buf);

        mpp_dmabuf_sync_partial_begin(fd, 1, base, len, __FUNCTION__);
    }

    val = p[4];
    hw_nal_ref_idc = (val >> 5) & 0x3;
    sw_nal_ref_idc = slice->nal_reference_idc;

    if (hw_nal_ref_idc == sw_nal_ref_idc)
        return MPP_OK;

    /* fix nal_ref_idc in all slice */
    if (!slice->is_multi_slice) {
        /* single slice do NOT scan */
        val = val & (~0x60);
        val |= (sw_nal_ref_idc << 5) & 0x60;
        p[4] = val;
        return MPP_OK;
    }

    /* multi-slice fix each nal_ref_idc */
    do {
        RK_U32 nal_len = get_next_nal(p, &len);

        val = p[4];
        val = val & (~0x60);
        val |= (sw_nal_ref_idc << 5) & 0x60;
        p[4] = val;

        if (len == 0)
            break;

        p += nal_len;
    } while (1);

    return MPP_OK;
}
