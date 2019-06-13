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

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_bitread.h"
#include "h263d_parser.h"
#include "h263d_syntax.h"

RK_U32 h263d_debug = 0;

#define h263d_dbg(flag, fmt, ...)   _mpp_dbg(h263d_debug, flag, fmt, ## __VA_ARGS__)
#define h263d_dbg_f(flag, fmt, ...) _mpp_dbg_f(h263d_debug, flag, fmt, ## __VA_ARGS__)

#define h263d_dbg_func(fmt, ...)    h263d_dbg_f(H263D_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define h263d_dbg_bit(fmt, ...)     h263d_dbg(H263D_DBG_BITS, fmt, ## __VA_ARGS__)
#define h263d_dbg_status(fmt, ...)  h263d_dbg(H263D_DBG_STATUS, fmt, ## __VA_ARGS__)

#define H263_STARTCODE                      0x00000080      /* 17 zero and 1 one */
#define H263_STARTCODE_MASK                 0x00FFFF80
#define H263_GOB_ZERO                       0x00000000
#define H263_GOB_ZERO_MASK                  0x0000007C

#define H263_SF_SQCIF                       1      /* 001 */
#define H263_SF_QCIF                        2      /* 010 */
#define H263_SF_CIF                         3      /* 011 */
#define H263_SF_4CIF                        4      /* 100 */
#define H263_SF_16CIF                       5      /* 101 */
#define H263_SF_CUSTOM                      6      /* 110 */
#define H263_EXTENDED_PTYPE                 7      /* 111 */
#define H263_EXTENDED_PAR                   15     /* 1111 */

typedef struct H263Hdr_t {
    H263VOPType  pict_type;
    RK_S32  width;
    RK_S32  height;
    RK_U32  TR;
    RK_U32  quant;

    // frame related parameter
    RK_S64  pts;
    RK_S32  slot_idx;
    RK_U32  enqueued;
    RK_U32  hdr_bits;
} H263Hdr;

typedef struct {
    // global paramter
    MppBufSlots     frame_slots;
    RK_U32          use_internal_pts;
    RK_U32          found_i_vop;

    // frame size parameter
    RK_S32          width;
    RK_S32          height;
    RK_S32          hor_stride;
    RK_S32          ver_stride;
    RK_U32          info_change;
    RK_U32          eos;

    // spliter parameter
    RK_S32          pos_frm_start;      // negtive - not found; non-negtive - position of frame start
    RK_S32          pos_frm_end;        // negtive - not found; non-negtive - position of frame end

    // bit read context
    BitReadCtx_t    *bit_ctx;

    // decoding parameter
    H263Hdr         hdr_curr;
    H263Hdr         hdr_ref0;

    // dpb/output information
    RK_S32          output;
    RK_S64          pts;

    // syntax for hal
    h263d_dxva2_picture_context_t *syntax;
} H263dParserImpl;

static RK_U32 h263d_fmt_to_dimension[8][2] = {
    {    0,    0 },       /* invalid */
    {  128,   96 },       /* SQCIF   */
    {  176,  144 },       /* QCIF    */
    {  352,  288 },       /* CIF     */
    {  704,  576 },       /* 4CIF    */
    { 1408, 1152 },       /* 16CIF   */
    {    0,    0 },       /* custorm */
    {    0,    0 },       /* extend  */
};

static void h263d_fill_picture_parameters(const H263dParserImpl *p,
                                          DXVA_PicParams_H263 *pp)
{
    const H263Hdr *hdr_curr = &p->hdr_curr;
    const H263Hdr *hdr_ref0 = &p->hdr_ref0;

    pp->short_video_header = 1;
    pp->vop_coding_type = hdr_curr->pict_type;
    pp->vop_quant = hdr_curr->quant;
    pp->wDecodedPictureIndex = hdr_curr->slot_idx;
    pp->wForwardRefPictureIndex = hdr_ref0->slot_idx;
    pp->vop_time_increment_resolution = 30000;
    pp->vop_width = hdr_curr->width;
    pp->vop_height = hdr_curr->height;

    // Rockchip special data
    pp->prev_coding_type = hdr_ref0->pict_type;
    pp->header_bits = hdr_curr->hdr_bits;
}

static void h263_syntax_init(h263d_dxva2_picture_context_t *syntax)
{
    DXVA2_DecodeBufferDesc *data = &syntax->desc[0];

    //!< commit picture paramters
    memset(data, 0, sizeof(*data));
    data->CompressedBufferType = DXVA2_PictureParametersBufferType;
    data->pvPVPState = (void *)&syntax->pp;
    data->DataSize = sizeof(syntax->pp);
    syntax->data[0] = data;

    //!< commit bitstream
    data = &syntax->desc[1];
    memset(data, 0, sizeof(*data));
    data->CompressedBufferType = DXVA2_BitStreamDateBufferType;
    syntax->data[1] = data;
}

static MPP_RET h263_parse_picture_header(H263dParserImpl *p, BitReadCtx_t *gb)
{
    RK_U32 val = 0;
    H263Hdr *hdr_curr = &p->hdr_curr;
    H263VOPType pict_type = H263_INVALID_VOP;

    /* start code */
    READ_BITS(gb, 17, &val); /* start code */
    mpp_assert(val == 1);

    /* gob */
    READ_BITS(gb, 5, &val); /* gob */
    mpp_assert(val == 0);

    /* time reference */
    READ_BITS(gb, 8, &hdr_curr->TR);

    /* first 5 bit of PTYPE */
    SKIP_BITS(gb, 5);

    /* source format */
    READ_BITS(gb, 3, &val); /* source format */
    hdr_curr->width  = h263d_fmt_to_dimension[val][0];
    hdr_curr->height = h263d_fmt_to_dimension[val][1];
    if (!hdr_curr->width && !hdr_curr->height) {
        mpp_err_f("unsupport source format %d\n", val);
        return MPP_NOK;
    }

    /* picture coding type: 0 - INTRA, 1 - INTER */
    READ_BITS(gb, 1, &val);
    pict_type = val;

    /* last 4 bit for PTYPE: UMV, AP mode, PB frame */
    READ_BITS(gb, 4, &val);
    if (val) {
        mpp_err_f("unsupport PTYPE mode %x\n", val);
        return MPP_NOK;
    }

    READ_BITS(gb, 5, &val);
    hdr_curr->quant = val;

    SKIP_BITS(gb, 1);

    READ_BITS(gb, 1, &val);
    while (val) {
        SKIP_BITS(gb, 8);
        READ_BITS(gb, 1, &val);
    }

    if (!p->found_i_vop)
        p->found_i_vop = (pict_type == H263_I_VOP);

    if (!p->found_i_vop)
        return MPP_NOK;

    hdr_curr->hdr_bits = gb->used_bits;
    hdr_curr->pict_type = pict_type;

    return MPP_OK;
__BITREAD_ERR:
    mpp_err_f("found error stream\n");
    return MPP_ERR_STREAM;
}

MPP_RET mpp_h263_parser_init(H263dParser *ctx, MppBufSlots frame_slots)
{
    BitReadCtx_t *bit_ctx = mpp_calloc(BitReadCtx_t, 1);
    H263dParserImpl *p = mpp_calloc(H263dParserImpl, 1);
    h263d_dxva2_picture_context_t *syntax = mpp_calloc(h263d_dxva2_picture_context_t, 1);

    if (NULL == p || NULL == bit_ctx || NULL == syntax) {
        mpp_err_f("malloc context failed\n");
        if (p)
            mpp_free(p);
        if (bit_ctx)
            mpp_free(bit_ctx);
        if (syntax)
            mpp_free(syntax);
        return MPP_NOK;
    }

    mpp_buf_slot_setup(frame_slots, 4);
    p->frame_slots      = frame_slots;
    p->pos_frm_start    = -1;
    p->pos_frm_end      = -1;
    p->bit_ctx          = bit_ctx;
    p->hdr_curr.slot_idx = H263_INVALID_VOP;
    p->hdr_ref0.slot_idx = H263_INVALID_VOP;
    h263_syntax_init(syntax);
    p->syntax = syntax;

    mpp_env_get_u32("h263d_debug", &h263d_debug, 0);

    *ctx = p;
    return MPP_OK;
}

MPP_RET mpp_h263_parser_deinit(H263dParser ctx)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    if (p) {
        if (p->bit_ctx) {
            mpp_free(p->bit_ctx);
            p->bit_ctx = NULL;
        }
        if (p->syntax) {
            mpp_free(p->syntax);
            p->syntax = NULL;
        }
        mpp_free(p);
    }
    return MPP_OK;
}

MPP_RET mpp_h263_parser_flush(H263dParser ctx)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    H263Hdr *hdr_curr = &p->hdr_ref0;
    RK_S32 index = hdr_curr->slot_idx;

    h263d_dbg_func("in\n");

    if (!hdr_curr->enqueued && index >= 0) {
        mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
        hdr_curr->enqueued = 1;
    }

    h263d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_h263_parser_reset(H263dParser ctx)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    H263Hdr *hdr_curr = &p->hdr_ref0;
    H263Hdr *hdr_ref0 = &p->hdr_ref0;
    RK_S32 index = hdr_curr->slot_idx;

    h263d_dbg_func("in\n");

    if (index >= 0) {
        mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);
        hdr_curr->slot_idx = -1;
    }

    index = hdr_ref0->slot_idx;
    if (index >= 0) {
        mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);
        hdr_ref0->slot_idx = -1;
    }

    p->found_i_vop = 0;

    h263d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_h263_parser_split(H263dParser ctx, MppPacket dst, MppPacket src)
{
    MPP_RET ret = MPP_NOK;
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    RK_U8 *dst_buf = mpp_packet_get_data(dst);
    size_t dst_len = mpp_packet_get_length(dst);
    RK_U8 *src_buf = mpp_packet_get_pos(src);
    RK_S32 src_len = (RK_S32)mpp_packet_get_length(src);
    RK_S32 pos_frm_start = p->pos_frm_start;
    RK_S32 pos_frm_end   = p->pos_frm_end;
    RK_U32 src_eos = mpp_packet_get_eos(src);
    RK_S32 src_pos = 0;
    RK_U32 state = (RK_U32) - 1;

    h263d_dbg_func("in\n");

    mpp_assert(src_len);

    if (dst_len) {
        mpp_assert(dst_len >= 4);
        state = ((RK_U32)(dst_buf[dst_len - 1]) <<  0) |
                ((RK_U32)(dst_buf[dst_len - 2]) <<  8) |
                ((RK_U32)(dst_buf[dst_len - 3]) << 16) |
                ((RK_U32)(dst_buf[dst_len - 4]) << 24);
    }

    if (pos_frm_start < 0) {
        // scan for frame start
        for (src_pos = 0; src_pos < src_len; src_pos++) {
            state = (state << 8) | src_buf[src_pos];
            if ((state & H263_STARTCODE_MASK) == H263_STARTCODE &&
                (state & H263_GOB_ZERO_MASK)  == H263_GOB_ZERO) {
                pos_frm_start = src_pos - 3;
                src_pos++;
                break;
            }
        }
    }

    if (pos_frm_start >= 0) {
        // scan for frame end
        for (; src_pos < src_len; src_pos++) {
            state = (state << 8) | src_buf[src_pos];

            if ((state & H263_STARTCODE_MASK) == H263_STARTCODE &&
                (state & H263_GOB_ZERO_MASK)  == H263_GOB_ZERO) {
                pos_frm_end = src_pos - 3;
                break;
            }
        }
        if (src_eos && src_pos == src_len) {
            pos_frm_end = src_len;
            mpp_packet_set_eos(dst);
        }
    }

    //mpp_log("pkt pos: start %d end %d len: left %d in %d\n",
    //        pos_frm_start, pos_frm_end, dst_len, src_len);

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
        mpp_assert((RK_S32)mpp_packet_get_length(src) == (src_len - pos_frm_end));
        mpp_packet_set_length(src, src_len - pos_frm_end);

        // return ok indicate the frame is ready and reset frame start/end position
        ret = MPP_OK;
        pos_frm_start = -1;
        pos_frm_end = -1;
    }

    p->pos_frm_start = pos_frm_start;
    p->pos_frm_end   = pos_frm_end;

    h263d_dbg_func("out\n");

    return ret;
}

MPP_RET mpp_h263_parser_decode(H263dParser ctx, MppPacket pkt)
{
    MPP_RET ret = MPP_NOK;
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    BitReadCtx_t *gb = p->bit_ctx;
    RK_U8 *buf = mpp_packet_get_data(pkt);
    RK_S32 len = (RK_S32)mpp_packet_get_length(pkt);
    RK_U32 startcode = 0xff;
    RK_S32 i = 0;

    h263d_dbg_func("in\n");

    while (i < len) {
        startcode = (startcode << 8) | buf[i++];

        if (startcode >> (32 - 22) == 0x20) {
            i -= 4;
            h263d_dbg_bit("found startcode at byte %d\n", i);
            break;
        }
    }

    if (i == len) {
        mpp_err_f("can not found start code in len %d packet\n", len);
        goto __BITREAD_ERR;
    }

    // setup bit read context
    mpp_set_bitread_ctx(gb, buf + i, len - i);

    ret = h263_parse_picture_header(p, gb);
    if (ret)
        goto __BITREAD_ERR;

    p->width  = p->hdr_curr.width;
    p->height = p->hdr_curr.height;
    p->pts  = mpp_packet_get_pts(pkt);
__BITREAD_ERR:
    h263d_dbg_status("found i_frame %d frame_type %d ret %d\n",
                     p->found_i_vop, p->hdr_curr.pict_type, ret);

    mpp_packet_set_pos(pkt, buf);
    mpp_packet_set_length(pkt, 0);
    p->eos = mpp_packet_get_eos(pkt);

    h263d_dbg_func("out\n");

    return ret;
}

MPP_RET mpp_h263_parser_setup_syntax(H263dParser ctx, MppSyntax *syntax)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    h263d_dxva2_picture_context_t *syn = p->syntax;

    h263d_dbg_func("in\n");

    h263d_fill_picture_parameters(p, &syn->pp);

    // fill bit stream parameter
    syn->data[1]->DataSize   = p->bit_ctx->buf_len;
    syn->data[1]->DataOffset = p->hdr_curr.hdr_bits;
    syn->data[1]->pvPVPState = p->bit_ctx->buf;

    syntax->number = 2;
    syntax->data = syn->data;

    h263d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_h263_parser_setup_hal_output(H263dParser ctx, RK_S32 *output)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    RK_S32 index = -1;

    h263d_dbg_func("in\n");

    if (p->found_i_vop) {
        H263Hdr *hdr_curr = &p->hdr_curr;
        MppBufSlots slots = p->frame_slots;
        MppFrame frame = NULL;

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
        mpp_frame_set_mode(frame, MPP_FRAME_FLAG_FRAME);

        mpp_buf_slot_set_prop(slots, index, SLOT_FRAME, frame);
        mpp_frame_deinit(&frame);
        mpp_assert(NULL == frame);

        hdr_curr->slot_idx = index;
    }

    p->output = index;
    *output = index;

    h263d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_h263_parser_setup_refer(H263dParser ctx, RK_S32 *refer, RK_S32 max_ref)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    H263Hdr *hdr_curr = &p->hdr_curr;
    MppBufSlots slots = p->frame_slots;
    RK_S32 index;

    h263d_dbg_func("in\n");

    memset(refer, -1, sizeof(max_ref * sizeof(*refer)));
    if (hdr_curr->pict_type == H263_P_VOP) {
        index = p->hdr_ref0.slot_idx;
        if (index >= 0) {
            mpp_buf_slot_set_flag(slots, index, SLOT_HAL_INPUT);
            refer[0] = index;
        }
    }

    h263d_dbg_func("out\n");

    return MPP_OK;
}

MPP_RET mpp_h263_parser_update_dpb(H263dParser ctx)
{
    H263dParserImpl *p = (H263dParserImpl *)ctx;
    MppBufSlots slots = p->frame_slots;
    H263Hdr *hdr_curr = &p->hdr_curr;
    H263Hdr *hdr_ref0 = &p->hdr_ref0;
    RK_S32 index = hdr_curr->slot_idx;

    h263d_dbg_func("in\n");

    mpp_assert(index >= 0);
    mpp_buf_slot_set_flag(slots, index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
    mpp_buf_slot_enqueue(slots, index, QUEUE_DISPLAY);
    hdr_curr->enqueued = 1;

    index = hdr_ref0->slot_idx;
    if (index >= 0) {
        mpp_buf_slot_clr_flag(slots, index, SLOT_CODEC_USE);
        hdr_ref0->slot_idx = -1;
    }

    // swap current to ref0
    *hdr_ref0 = *hdr_curr;
    hdr_curr->slot_idx  = H263_INVALID_VOP;
    hdr_curr->pts       = 0;
    hdr_curr->enqueued  = 0;

    h263d_dbg_func("out\n");

    return MPP_OK;
}



