/*
 *
 * Copyright 2010 Rockchip Electronics Co. LTD
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

/*
 * @file        api_mpg4d_parser.c
 * @brief
 * @author      gzl(lance.gao@rock-chips.com)
 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_packet.h"

#include "mpg4d_parser.h"

typedef struct {
    MppBufSlots frame_slots;
    RK_S32      output;
    RK_U32      width;
    RK_U32      height;
    RK_S64      pts;
    RK_U32      eos;

    // spliter parameter
    RK_S32      pos_frm_start;      // negtive - not found; non-negtive - position of frame start
    RK_S32      pos_frm_end;        // negtive - not found; non-negtive - position of frame end
} Mpg4dParserImpl;

MPP_RET mpp_mpg4_parser_init(Mpg4dParser *ctx, MppBufSlots frame_slots)
{
    Mpg4dParserImpl *p = mpp_calloc(Mpg4dParserImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc context failed\n");
        return MPP_NOK;
    }
    mpp_buf_slot_setup(frame_slots, 8);
    p->frame_slots   = frame_slots;
    p->pos_frm_start = -1;
    p->pos_frm_end   = -1;
    *ctx = p;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_deinit(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    mpp_free(p);
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_flush(Mpg4dParser ctx)
{
    (void)ctx;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_reset(Mpg4dParser ctx)
{
    (void)ctx;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_split(Mpg4dParser ctx, MppPacket dst, MppPacket src)
{
    MPP_RET ret = MPP_NOK;
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    RK_U8 *dst_buf = mpp_packet_get_data(dst);
    size_t dst_len = mpp_packet_get_length(dst);
    RK_U8 *src_buf = mpp_packet_get_pos(src);
    size_t src_len = mpp_packet_get_length(src);
    RK_S32 pos_frm_start = p->pos_frm_start;
    RK_S32 pos_frm_end   = p->pos_frm_end;
    size_t src_pos = 0;
    RK_U32 state = 0;

    mpp_assert(src_len);

    if (pos_frm_start < 0) {
        // scan for frame start
        for (src_pos = 0; src_pos < src_len; src_pos++) {
            state = (state << 8) | src_buf[src_pos];
            if (state == 0x1B6) {
                src_pos++;
                pos_frm_start = src_pos;
                break;
            }
        }
    }

    if (pos_frm_start) {
        // scan for frame end
        for (; src_pos < src_len; src_pos++) {
            state = (state << 8) | src_buf[src_pos];
            if ((state & 0xFFFFFF00) == 0x100) {
                pos_frm_end = src_pos - 3;
                break;
            }
        }
    }

    if (pos_frm_start < 0 || pos_frm_end < 0) {
        // do not found frame start or do not found frame end, just copy the hold buffer to dst
        memcpy(dst_buf + dst_len, src_buf, src_len);
        // update dst buffer length
        mpp_packet_set_length(dst, dst_len + src_len);
        // set src buffer pos to end to src buffer
        mpp_packet_set_pos(src, src_buf + src_len);
    } else {
        // found both frame start and frame end - only copy frame
        memcpy(dst_buf + dst_len, src_buf, pos_frm_end);
        mpp_packet_set_length(dst, dst_len + pos_frm_end);
        // set src buffer pos to end to src buffer
        mpp_packet_set_pos(src, src_buf + pos_frm_end);
        // return ok indicate the frame is ready and reset frame start/end position
        ret = MPP_OK;
        pos_frm_start = -1;
        pos_frm_end = -1;
    }

    p->pos_frm_start = pos_frm_start;
    p->pos_frm_end   = pos_frm_end;

    return ret;
}

MPP_RET mpp_mpg4_parser_decode(Mpg4dParser ctx, MppPacket pkt)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    RK_U8 *buf = mpp_packet_get_data(pkt);

    p->width    = 1920;
    p->height   = 1080;

    mpp_packet_set_pos(pkt, buf);
    mpp_packet_set_length(pkt, 0);

    p->pts = mpp_packet_get_pts(pkt);
    p->eos = mpp_packet_get_eos(pkt);

    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_syntax(Mpg4dParser ctx, MppSyntax syntax)
{
    (void) ctx;
    (void) syntax;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_output(Mpg4dParser ctx, RK_S32 *output)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    MppFrame frame = NULL;
    RK_S32 index = -1;

    mpp_frame_init(&frame);
    mpp_frame_set_width(frame, p->width);
    mpp_frame_set_height(frame, p->height);
    mpp_frame_set_hor_stride(frame, MPP_ALIGN(p->width, 16));
    mpp_frame_set_ver_stride(frame, MPP_ALIGN(p->height, 16));

    /*
     * set slots information
     * 1. output index MUST be set
     * 2. get unused index for output if needed
     * 3. set output index as hal_input
     * 4. set frame information to output index
     * 5. if one frame can be display, it SHOULD be enqueued to display queue
     */
    mpp_buf_slot_get_unused(slots, &index);
    mpp_buf_slot_set_flag(slots, index, SLOT_HAL_OUTPUT);
    mpp_frame_set_pts(frame, p->pts);
    mpp_frame_set_eos(frame, p->eos);
    mpp_buf_slot_set_prop(slots, index, SLOT_FRAME, frame);
    mpp_frame_deinit(&frame);
    mpp_assert(NULL == frame);
    p->output = index;
    *output = index;
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_refer(Mpg4dParser ctx, RK_S32 *refer, RK_S32 max_ref)
{
    (void) ctx;
    /*
     * setup output task
     * 1. valid flag MUST be set if need hardware to run once
     * 2. set output slot index
     * 3. set reference slot index
     */
    memset(refer, -1, sizeof(max_ref * sizeof(*refer)));
    return MPP_OK;
}

MPP_RET mpp_mpg4_parser_setup_display(Mpg4dParser ctx)
{
    Mpg4dParserImpl *p = (Mpg4dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    RK_S32 output = p->output;

    /*
     * update dpb status assuming that hw has decoded the frame
     */
    mpp_buf_slot_set_flag(slots, output, SLOT_QUEUE_USE);
    mpp_buf_slot_enqueue(slots, output, QUEUE_DISPLAY);

    return MPP_OK;
}


