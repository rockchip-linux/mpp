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

#define MODULE_TAG "m2vd_parser"

#include <string.h>

#include "mpp_env.h"
#include "mpp_packet_impl.h"

#include "m2vd_parser.h"
#include "m2vd_codec.h"

#define VPU_BITSTREAM_START_CODE (0x42564b52)  /* RKVB, rockchip video bitstream */

RK_U32 m2vd_debug = 0x0;

static RK_U8 scanOrder[2][64] = {
    {   /* zig-zag */
        0, 1, 8, 16, 9, 2, 3, 10,
        17, 24, 32, 25, 18, 11, 4, 5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13, 6, 7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    },

    {   /* Alternate */
        0, 8, 16, 24, 1, 9, 2, 10,
        17, 25, 32, 40, 48, 56, 57, 49,
        41, 33, 26, 18, 3, 11, 4, 12,
        19, 27, 34, 42, 50, 58, 35, 43,
        51, 59, 20, 28, 5, 13, 6, 14,
        21, 29, 36, 44, 52, 60, 37, 45,
        53, 61, 22, 30, 7, 15, 23, 31,
        38, 46, 54, 62, 39, 47, 55, 63
    }
};

static RK_U8 intraDefaultQMatrix[64] = {
    8, 16, 19, 22, 26, 27, 29, 34,
    16, 16, 22, 24, 27, 29, 34, 37,
    19, 22, 26, 27, 29, 34, 34, 38,
    22, 22, 26, 27, 29, 34, 37, 40,
    22, 26, 27, 29, 32, 35, 40, 48,
    26, 27, 29, 32, 35, 40, 48, 58,
    26, 27, 29, 34, 38, 46, 56, 69,
    27, 29, 35, 38, 46, 56, 69, 83
};

static int frame_period_Table[16] = {
    1,
    11142,
    10667,
    10240,
    8542,
    8533,
    5120,
    4271,
    4267,
    1,
    1,
    1,
    1,
    1,
    1,
    1
};

static const MppFrameRational mpeg2_aspect[16] = {
    {0, 1},
    {1, 1},
    {4, 3},
    {16, 9},
    {221, 100},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
    {0, 1},
};

static inline RK_S32 m2vd_get_readbits(BitReadCtx_t *bx)
{
    return bx->used_bits;
}

static inline RK_S32 m2vd_get_leftbits(BitReadCtx_t *bx)
{
    return  (bx->bytes_left_ * 8 + bx->num_remaining_bits_in_curr_byte_);
}

static RK_S32 m2vd_read_bits(BitReadCtx_t *bx, RK_U32 bits)
{
    RK_S32 ret = 0;
    if (bits < 32)
        mpp_read_bits(bx, bits, &ret);
    else
        mpp_read_longbits(bx, bits, (RK_U32 *)&ret);
    return ret;
}

static RK_S32 m2vd_show_bits(BitReadCtx_t *bx, RK_U32 bits)
{
    RK_S32 ret = 0;
    mpp_show_bits(bx, bits, &ret);
    return ret;
}

static MPP_RET m2vd_parser_init_ctx(M2VDParserContext *ctx, ParserCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_S32 i = 0;
    m2vd_dbg_func("FUN_I");

    M2VD_CHK_I(ctx);
    memset(ctx, 0, sizeof(*ctx));

    ctx->dxva_ctx = mpp_calloc(M2VDDxvaParam, 1);
    ctx->bitread_ctx = mpp_calloc(BitReadCtx_t, 1);

    ctx->packet_slots = cfg->packet_slots;
    ctx->frame_slots = cfg->frame_slots;

    mpp_buf_slot_setup(ctx->frame_slots, 16);

    ctx->initFlag = 0;

    /* copy from mpeg2decoder::mpeg2decoder */
    memset(&ctx->Framehead, 0, 3 * sizeof(M2VDFrameHead));

    ctx->frame_ref0 = &ctx->Framehead[0];
    ctx->frame_ref1 = &ctx->Framehead[1];
    ctx->frame_cur = &ctx->Framehead[2];
    for (i = 0; i < 3; i++) {
        mpp_frame_init(&ctx->Framehead[i].f);
        if (!ctx->Framehead[i].f) {
            mpp_err("Failed to allocate frame buffer %d\n", i);
            return MPP_ERR_NOMEM;
        }
        ctx->Framehead[i].picCodingType = 0xffffffff;
        ctx->Framehead[i].slot_index = 0xff;
    }
#if M2VD_SKIP_ERROR_FRAME_EN
    ctx->mHwDecStatus = MPP_OK;
#endif
    ctx->resetFlag = 0;
    ctx->flush_dpb_eos = 0;
    ctx->pic_head.pre_temporal_reference = 0;
    ctx->pic_head.pre_picture_coding_type = 0;
    ctx->pic_head.picture_coding_type = 0;

    /* copy form mpeg2decoder::decoder_init */
    //----------------------------------------------------
    ctx->bitstream_sw_buf = mpp_calloc(RK_U8, M2VD_BUF_SIZE_BITMEM);
    mpp_packet_init(&ctx->input_packet, ctx->bitstream_sw_buf, M2VD_BUF_SIZE_BITMEM);

    ctx->qp_tab_sw_buf = mpp_calloc(RK_U8, M2VD_BUF_SIZE_QPTAB);
    ctx->seq_head.pIntra_table = ctx->qp_tab_sw_buf;
    ctx->seq_head.pInter_table = ctx->seq_head.pIntra_table + 64;

    ctx->MPEG2_Flag = 0;
    ctx->seq_ext_head.progressive_sequence = 1;
    ctx->pic_code_ext_head.progressive_frame = 1;
    ctx->pic_code_ext_head.picture_structure = M2VD_PIC_STRUCT_FRAME;
    ctx->pic_head.pre_picture_coding_type = M2VD_CODING_TYPE_D;
    ctx->top_first_cnt = 0;
    ctx->bottom_first_cnt = 0;
    ctx->pic_code_ext_head.frame_pred_frame_dct = 1;
    ctx->seq_ext_head.chroma_format = 1;
    ctx->seq_disp_ext_head.matrix_coefficients = 5;
    ctx->PreGetFrameTime = 0;
    ctx->maxFrame_inGOP = 0;
    ctx->preframe_period = 0;
    ctx->mHeaderDecFlag = 0;
    ctx->mExtraHeaderDecFlag = 0;
    ctx->max_stream_size = M2VD_BUF_SIZE_BITMEM;
    ctx->ref_frame_cnt = 0;
    ctx->need_split = cfg->need_split;
    ctx->left_length = 0;
    ctx->vop_header_found = 0;

    if (M2VD_DBG_DUMP_REG & m2vd_debug) {
        RK_S32 k = 0;
        for (k = 0; k < M2VD_DBG_FILE_NUM; k++)
            ctx->fp_dbg_file[k] = NULL;

        ctx->fp_dbg_file[0] = fopen("/sdcard/m2vd_dbg_stream.txt", "wb");
        if (!ctx->fp_dbg_file[0])
            mpp_log("open file failed: %s", "/sdcard/m2vd_dbg_stream.txt");

        ctx->fp_dbg_yuv = fopen("/sdcard/m2vd_dbg_yuv_out.txt", "wb");
        if (!ctx->fp_dbg_yuv)
            mpp_log("open file failed: %s", "/sdcard/m2vd_dbg_yuv_out.txt");
    } else {
        RK_S32 k = 0;
        for (k = 0; k < M2VD_DBG_FILE_NUM; k++)
            ctx->fp_dbg_file[k] = NULL;
    }


    m2vd_dbg_func("FUN_O");
__FAILED:

    return ret;
}

MPP_RET m2vd_parser_init(void *ctx, ParserCfg *parser_cfg)
{
    MPP_RET ret = MPP_OK;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    m2vd_dbg_func("FUN_I");
    if (p == NULL) {
        M2VD_CHK_M(p = (M2VDParserContext*)mpp_calloc(M2VDParserContext, 1));
        c->parse_ctx = p;
    }

    M2VD_CHK_F(m2vd_parser_init_ctx(p, parser_cfg));

    mpp_env_get_u32("m2vd_debug", &m2vd_debug, 0);

    m2vd_dbg_func("FUN_O");
__FAILED:
    return ret;
}

MPP_RET m2vd_parser_deinit(void *ctx)
{
    RK_U32 k = 0;
    MPP_RET ret = MPP_OK;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;

    m2vd_dbg_func("FUN_I");

    for (k = 0; k < M2VD_DBG_FILE_NUM; k++) {
        M2VD_FCLOSE(p->fp_dbg_file[k]);
    }
    M2VD_FCLOSE(p->fp_dbg_yuv);

    if (p->bitstream_sw_buf) {
        mpp_free(p->bitstream_sw_buf);
        p->bitstream_sw_buf = NULL;
    }
    if (p->qp_tab_sw_buf) {
        mpp_free(p->qp_tab_sw_buf);
        p->qp_tab_sw_buf = NULL;
    }

    if (p->input_packet) {
        mpp_packet_deinit(&p->input_packet);
    }

    if (p->dxva_ctx) {
        mpp_free(p->dxva_ctx);
        p->dxva_ctx = NULL;
    }
    if (p->bitread_ctx) {
        mpp_free(p->bitread_ctx);
        p->bitread_ctx = NULL;
    }

    for (k = 0; k < 3; k++) {
        mpp_free(p->Framehead[k].f);
    }

    if (p) {
        mpp_free(p);
    }

    m2vd_dbg_func("FUN_O");
    return ret;
}

/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET m2vd_parser_reset(void *ctx)
{
    MPP_RET ret = MPP_OK;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    m2vd_dbg_func("FUN_I");
    if (p->frame_cur->slot_index != 0xff)
        mpp_buf_slot_clr_flag(p->frame_slots, p->frame_cur->slot_index,
                              SLOT_CODEC_USE);

    if (p->frame_ref0->slot_index != 0xff) {
        if (p->frame_ref0->flags) {
            mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref0->slot_index,
                                  SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(p->frame_slots, p->frame_ref0->slot_index,
                                 QUEUE_DISPLAY);
            p->frame_ref0->flags = 0;
        }
        mpp_buf_slot_clr_flag(p->frame_slots, p->frame_ref0->slot_index,
                              SLOT_CODEC_USE);
    }

    if (p->frame_ref1->slot_index != 0xff)
        mpp_buf_slot_clr_flag(p->frame_slots, p->frame_ref1->slot_index,
                              SLOT_CODEC_USE);

    p->frame_cur->slot_index = 0xff;
    p->frame_ref0->slot_index = 0xff;
    p->frame_ref1->slot_index = 0xff;
    p->ref_frame_cnt = 0;
    p->resetFlag = 1;
    p->eos = 0;
    p->left_length = 0;
    p->need_split = 0;
    p->vop_header_found = 0;
    m2vd_dbg_func("FUN_O");
    return ret;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET m2vd_parser_flush(void *ctx)
{
    MPP_RET ret = MPP_OK;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    m2vd_dbg_func("FUN_I");

    if ((p->frame_ref0->slot_index != 0xff) && !p->frame_ref0->flags) {
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref0->slot_index,
                              SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(p->frame_slots, p->frame_ref0->slot_index,
                             QUEUE_DISPLAY);
        p->frame_ref0->flags = 0;
    }

    m2vd_dbg_func("FUN_O");
    return ret;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET m2vd_parser_control(void *ctx, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;
    m2vd_dbg_func("FUN_I");
    (void)ctx;
    (void)cmd_type;
    (void)param;
    m2vd_dbg_func("FUN_O");
    return ret;
}

/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/
MPP_RET mpp_m2vd_parser_split(M2VDParserContext *ctx, MppPacket dst, MppPacket src)
{
    MPP_RET ret = MPP_NOK;
    M2VDParserContext *p = ctx;
    RK_U8 *src_buf = (RK_U8 *)mpp_packet_get_pos(src);
    RK_U32 src_len = (RK_U32)mpp_packet_get_length(src);
    RK_U32 src_eos = mpp_packet_get_eos(src);
    RK_U8 *dst_buf = (RK_U8 *)mpp_packet_get_data(dst);
    RK_U32 dst_len = (RK_U32)mpp_packet_get_length(dst);
    RK_U32 src_pos = 0;

    if (!p->vop_header_found) {
        if ((dst_len < sizeof(p->state)) &&
            ((p->state & 0x00FFFFFF) == 0x000001)) {
            dst_buf[0] = 0;
            dst_buf[1] = 0;
            dst_buf[2] = 1;
            dst_len = 3;
        }

        while (src_pos < src_len) {
            p->state = (p->state << 8) | src_buf[src_pos];
            dst_buf[dst_len++] = src_buf[src_pos++];

            /*
             * 0x1b3 : sequence header
             * 0x100 : frame header
             * we see all 0x1b3 and 0x100 as boundary
             */
            if (p->state == SEQUENCE_HEADER_CODE || p->state == PICTURE_START_CODE) {
                p->pts = mpp_packet_get_pts(src);
                p->vop_header_found = 1;
                break;
            }
        }
    }

    if (p->vop_header_found) {
        while (src_pos < src_len) {
            p->state = (p->state << 8) | src_buf[src_pos];
            dst_buf[dst_len++] = src_buf[src_pos++];

            if (((p->state & 0x00FFFFFF) == 0x000001) &&
                (src_buf[src_pos] == (SEQUENCE_HEADER_CODE & 0xFF) ||
                 src_buf[src_pos] == (PICTURE_START_CODE & 0xFF))) {
                dst_len -= 3;
                p->vop_header_found = 0;
                ret = MPP_OK;
                break;
            }
        }
    }

    if (src_eos && src_pos >= src_len) {
        mpp_packet_set_eos(dst);
        ret = MPP_OK;
    }

    mpp_packet_set_length(dst, dst_len);
    mpp_packet_set_pos(src, src_buf + src_pos);

    return ret;
}

MPP_RET m2vd_parser_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    RK_U8 *pos = NULL;
    size_t length = 0;
    RK_U32 eos = 0;

    if (ctx == NULL || pkt == NULL || task == NULL) {
        mpp_err_f("found NULL input ctx %p pkt %p task %p\n", ctx, pkt, task);
        return MPP_ERR_NULL_PTR;
    }

    pos = mpp_packet_get_pos(pkt);
    length = mpp_packet_get_length(pkt);
    eos = mpp_packet_get_eos(pkt);

    if (eos && !length) {
        task->valid = 0;
        task->flags.eos = 1;
        m2vd_parser_flush(ctx);
        return MPP_OK;
    }

    if (p->bitstream_sw_buf == NULL) {
        mpp_err("failed to malloc task buffer for hardware with size %d\n", length);
        return MPP_ERR_UNKNOW;
    }

    mpp_packet_set_length(p->input_packet, p->left_length);

    size_t total_length = MPP_ALIGN(p->left_length + length, 16) + 64;

    if (total_length > p->max_stream_size) {
        RK_U8 *dst = NULL;

        do {
            p->max_stream_size <<= 1;
        } while (total_length > p->max_stream_size);

        dst = mpp_malloc_size(RK_U8, p->max_stream_size);
        mpp_assert(dst);

        if (p->left_length > 0) {
            memcpy(dst, p->bitstream_sw_buf, p->left_length);
        }
        mpp_free(p->bitstream_sw_buf);
        p->bitstream_sw_buf = dst;

        mpp_packet_set_data(p->input_packet, p->bitstream_sw_buf);
        mpp_packet_set_size(p->input_packet, p->max_stream_size);
    }

    if (!p->need_split) {
        RK_U32 *val = (RK_U32 *)mpp_packet_get_pos(pkt);
        /* if input data is rk format styl skip those 32 byte */
        RK_S32 offset = (VPU_BITSTREAM_START_CODE == val[0]) ? 32 : 0;
        memcpy(p->bitstream_sw_buf, pos + offset, length - offset);

        mpp_packet_set_length(p->input_packet, length - offset);
        mpp_packet_set_data(p->input_packet, p->bitstream_sw_buf);
        mpp_packet_set_size(p->input_packet, p->max_stream_size);

        p->pts = mpp_packet_get_pts(pkt);
        task->valid = 1;
        mpp_packet_set_length(pkt, 0);
    } else {
        if (MPP_OK == mpp_m2vd_parser_split(p, p->input_packet, pkt)) {
            p->left_length = 0;
            task->valid = 1;
        } else {
            task->valid = 0;
            p->left_length = mpp_packet_get_length(p->input_packet);
        }
    }

    if (mpp_packet_get_flag(pkt) & MPP_PACKET_FLAG_EXTRA_DATA) {
        mpp_packet_set_extra_data(p->input_packet);
    }

    p->eos = mpp_packet_get_eos(pkt);
    mpp_packet_set_pts(p->input_packet, p->pts);
    task->input_packet = p->input_packet;
    task->flags.eos = p->eos;

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   parser
***********************************************************************
*/

static RK_U32 m2vd_search_header(BitReadCtx_t *bx)
{
    mpp_align_get_bits(bx);
    while (m2vd_show_bits(bx, 24) != 0x01) {
        mpp_skip_bits(bx, 8);
        if (m2vd_get_leftbits(bx) < 32) {
            if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
                mpp_log("[m2v]: seach_header: str.leftbit()[%d] < 32", m2vd_get_leftbits(bx));
            }
            return NO_MORE_STREAM;
        }
    }
    return m2vd_show_bits(bx, 32);
}

static int m2vd_decode_seq_ext_header(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;
    m2vd_dbg_func("FUN_I");
    ctx->MPEG2_Flag = 1;
    ctx->seq_ext_head.profile_and_level_indication = m2vd_read_bits(bx, 8);
    ctx->seq_ext_head.progressive_sequence         = m2vd_read_bits(bx, 1);
    ctx->seq_ext_head.chroma_format                = m2vd_read_bits(bx, 2);
    if (ctx->seq_ext_head.chroma_format != 1)
        return M2VD_DEC_UNSURPORT;
    ctx->seq_ext_head.horizontal_size_extension    = m2vd_read_bits(bx, 2);
    ctx->seq_ext_head.vertical_size_extension      = m2vd_read_bits(bx, 2);
    ctx->seq_ext_head.bit_rate_extension           = m2vd_read_bits(bx, 12);
    mpp_skip_bits(bx, 1);
    ctx->seq_ext_head.vbv_buffer_size_extension    = m2vd_read_bits(bx, 8);
    ctx->seq_ext_head.low_delay                    = m2vd_read_bits(bx, 1);
    ctx->seq_ext_head.frame_rate_extension_n       = m2vd_read_bits(bx, 2);
    ctx->seq_ext_head.frame_rate_extension_d       = m2vd_read_bits(bx, 5);

    ctx->seq_head.bit_rate_value |= (ctx->seq_ext_head.bit_rate_extension << 18);
    ctx->seq_head.vbv_buffer_size += (ctx->seq_ext_head.vbv_buffer_size_extension << 10);

    m2vd_dbg_func("FUN_O");
    return  M2VD_DEC_OK;
}

static int m2vd_decode_seqdisp_ext_header(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;
    ctx->seq_disp_ext_head.video_format      = m2vd_read_bits(bx, 3);
    ctx->seq_disp_ext_head.color_description = m2vd_read_bits(bx, 1);

    if (ctx->seq_disp_ext_head.color_description) {
        ctx->seq_disp_ext_head.color_primaries          = m2vd_read_bits(bx, 8);
        ctx->seq_disp_ext_head.transfer_characteristics = m2vd_read_bits(bx, 8);
        ctx->seq_disp_ext_head.matrix_coefficients      = m2vd_read_bits(bx, 8);
    }

    m2vd_read_bits(bx, 14);
    mpp_skip_bits(bx, 1);
    m2vd_read_bits(bx, 14);
    return  M2VD_DEC_OK;
}

static int m2vd_decode_matrix_ext_header(M2VDParserContext *ctx)
{
    RK_U32  load_intra_quantizer_matrix;
    RK_U32  load_non_intra_quantizer_matrix;
    RK_U32  load_chroma_intra_quantizer_matrix;
    RK_U32  load_chroma_non_intra_quantizer_matrix;
    RK_U32  i;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    load_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (load_intra_quantizer_matrix) {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pIntra_table[scanOrder[0][i]] = (unsigned char)m2vd_read_bits(bx, 8);

    }
    load_non_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (load_non_intra_quantizer_matrix) {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pInter_table[scanOrder[0][i]] = (unsigned char)m2vd_read_bits(bx, 8);
    }
    load_chroma_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (load_chroma_intra_quantizer_matrix) {
        return M2VD_DEC_UNSURPORT;
    }
    load_chroma_non_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (load_chroma_non_intra_quantizer_matrix) {
        return M2VD_DEC_UNSURPORT;
    }
    return  M2VD_DEC_OK;
}

static int m2vd_decode_scalable_ext_header(M2VDParserContext *ctx)
{
    /* ISO/IEC 13818-2 section 6.2.2.5: sequence_scalable_extension() header */
    int layer_id;
    int lower_layer_prediction_horizontal_size;
    int lower_layer_prediction_vertical_size;
    int horizontal_subsampling_factor_m;
    int horizontal_subsampling_factor_n;
    int vertical_subsampling_factor_m;
    int vertical_subsampling_factor_n;
    int scalable_mode;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    /* scalable_mode */
#define SC_NONE 0
#define SC_DP   1
#define SC_SPAT 2
#define SC_SNR  3
#define SC_TEMP 4

    scalable_mode = m2vd_read_bits(bx, 2) + 1; /* add 1 to make SC_DP != SC_NONE */

    layer_id = m2vd_read_bits(bx, 4);

    if (scalable_mode == SC_SPAT) {
        lower_layer_prediction_horizontal_size = m2vd_read_bits(bx, 14);
        mpp_skip_bits(bx, 1);
        lower_layer_prediction_vertical_size   = m2vd_read_bits(bx, 14);
        horizontal_subsampling_factor_m        = m2vd_read_bits(bx, 5);
        horizontal_subsampling_factor_n        = m2vd_read_bits(bx, 5);
        vertical_subsampling_factor_m          = m2vd_read_bits(bx, 5);
        vertical_subsampling_factor_n          = m2vd_read_bits(bx, 5);
    }

    (void)layer_id;
    (void)lower_layer_prediction_horizontal_size;
    (void)lower_layer_prediction_vertical_size;
    (void)horizontal_subsampling_factor_m;
    (void)horizontal_subsampling_factor_n;
    (void)vertical_subsampling_factor_m;
    (void)vertical_subsampling_factor_n;


    if (scalable_mode == SC_TEMP)
        return M2VD_DEC_UNSURPORT;
    else
        return  M2VD_DEC_OK;
}

static int m2vd_decode_picdisp_ext_header(M2VDParserContext *ctx)
{
    int i;
    int number_of_frame_center_offsets;
    BitReadCtx_t *bx = ctx->bitread_ctx;
    /* based on ISO/IEC 13818-2 section 6.3.12
    (November 1994) Picture display extensions */

    /* derive number_of_frame_center_offsets */
    if (ctx->seq_ext_head.progressive_sequence) {
        if (ctx->pic_code_ext_head.repeat_first_field) {
            if (ctx->pic_code_ext_head.top_field_first)
                number_of_frame_center_offsets = 3;
            else
                number_of_frame_center_offsets = 2;
        } else {
            number_of_frame_center_offsets = 1;
        }
    } else {
        if (ctx->pic_code_ext_head.picture_structure != M2VD_PIC_STRUCT_FRAME) {
            number_of_frame_center_offsets = 1;
        } else {
            if (ctx->pic_code_ext_head.repeat_first_field)
                number_of_frame_center_offsets = 3;
            else
                number_of_frame_center_offsets = 2;
        }
    }

    /* now parse */
    for (i = 0; i < number_of_frame_center_offsets; i++) {
        ctx->pic_disp_ext_head.frame_center_horizontal_offset[i] = m2vd_read_bits(bx, 16);
        mpp_skip_bits(bx, 1);

        ctx->pic_disp_ext_head.frame_center_vertical_offset[i]   = m2vd_read_bits(bx, 16);
        mpp_skip_bits(bx, 1);
    }
    return  M2VD_DEC_OK;
}

static int m2vd_decode_spatial_ext_header(M2VDParserContext *ctx)
{
    /* ISO/IEC 13818-2 section 6.2.3.5: picture_spatial_scalable_extension() header */
    int lower_layer_temporal_reference;
    int lower_layer_horizontal_offset;
    int lower_layer_vertical_offset;
    int spatial_temporal_weight_code_table_index;
    int lower_layer_progressive_frame;
    int lower_layer_deinterlaced_field_select;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    lower_layer_temporal_reference = m2vd_read_bits(bx, 10);
    mpp_skip_bits(bx, 1);
    lower_layer_horizontal_offset = m2vd_read_bits(bx, 15);
    if (lower_layer_horizontal_offset >= 16384)
        lower_layer_horizontal_offset -= 32768;
    mpp_skip_bits(bx, 1);
    lower_layer_vertical_offset = m2vd_read_bits(bx, 15);
    if (lower_layer_vertical_offset >= 16384)
        lower_layer_vertical_offset -= 32768;
    spatial_temporal_weight_code_table_index = m2vd_read_bits(bx, 2);
    lower_layer_progressive_frame = m2vd_read_bits(bx, 1);
    lower_layer_deinterlaced_field_select = m2vd_read_bits(bx, 1);

    (void)lower_layer_temporal_reference;
    (void)lower_layer_vertical_offset;
    (void)spatial_temporal_weight_code_table_index;
    (void)lower_layer_progressive_frame;
    (void)lower_layer_deinterlaced_field_select;

    return  M2VD_DEC_OK;
}

static int m2vd_decode_pic_ext_header(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;
    ctx->pic_code_ext_head.f_code[0][0] = m2vd_read_bits(bx, 4);
    ctx->pic_code_ext_head.f_code[0][1] = m2vd_read_bits(bx, 4);
    ctx->pic_code_ext_head.f_code[1][0] = m2vd_read_bits(bx, 4);
    ctx->pic_code_ext_head.f_code[1][1] = m2vd_read_bits(bx, 4);
    if (ctx->MPEG2_Flag) {
        ctx->pic_head.full_pel_forward_vector = ctx->pic_code_ext_head.f_code[0][0];
        ctx->pic_head.forward_f_code = ctx->pic_code_ext_head.f_code[0][1];
        ctx->pic_head.full_pel_backward_vector = ctx->pic_code_ext_head.f_code[1][0];
        ctx->pic_head.backward_f_code = ctx->pic_code_ext_head.f_code[1][1];
    }

    ctx->pic_code_ext_head.intra_dc_precision         = m2vd_read_bits(bx, 2);
    ctx->pic_code_ext_head.picture_structure          = m2vd_read_bits(bx, 2);
    if (ctx->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_FRAME)
        ctx->pic_code_ext_head.top_field_first            = m2vd_read_bits(bx, 1);
    else {
        m2vd_read_bits(bx, 1);
        if ((ctx->pic_head.pre_picture_coding_type != ctx->pic_head.picture_coding_type) &&
            (ctx->pic_head.pre_picture_coding_type != M2VD_CODING_TYPE_I)) {
            if (ctx->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD)
                ctx->top_first_cnt++;
            else
                ctx->bottom_first_cnt++;
        }
        if (ctx->top_first_cnt >= ctx->bottom_first_cnt)
            ctx->pic_code_ext_head.top_field_first = 1;
        else
            ctx->pic_code_ext_head.top_field_first = 0;
    }
    ctx->pic_code_ext_head.frame_pred_frame_dct       = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.concealment_motion_vectors = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.q_scale_type           = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.intra_vlc_format           = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.alternate_scan         = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.repeat_first_field         = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.chroma_420_type            = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.progressive_frame          = m2vd_read_bits(bx, 1);
    ctx->pic_code_ext_head.composite_display_flag     = m2vd_read_bits(bx, 1);
    if (ctx->pic_code_ext_head.composite_display_flag) {
        ctx->pic_code_ext_head.v_axis            = m2vd_read_bits(bx, 1);
        ctx->pic_code_ext_head.field_sequence    = m2vd_read_bits(bx, 3);
        ctx->pic_code_ext_head.sub_carrier       = m2vd_read_bits(bx, 1);
        ctx->pic_code_ext_head.burst_amplitude   = m2vd_read_bits(bx, 7);
        ctx->pic_code_ext_head.sub_carrier_phase = m2vd_read_bits(bx, 8);
    }
    return  M2VD_DEC_OK;
}

static inline int m2vd_decode_temp_ext_header(void)
{
    return  M2VD_DEC_UNSURPORT;
}

static int m2vd_decode_copyright_ext_header(M2VDParserContext *ctx)
{
    int copyright_flag;
    int copyright_identifier;
    int original_or_copy;
    int copyright_number_1;
    int copyright_number_2;
    int copyright_number_3;
    int reserved_data;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    copyright_flag =       m2vd_read_bits(bx, 1);
    copyright_identifier = m2vd_read_bits(bx, 8);
    original_or_copy =     m2vd_read_bits(bx, 1);

    /* reserved */
    reserved_data = m2vd_read_bits(bx, 7);

    mpp_skip_bits(bx, 1);
    copyright_number_1 =   m2vd_read_bits(bx, 20);
    mpp_skip_bits(bx, 1);
    copyright_number_2 =   m2vd_read_bits(bx, 22);
    mpp_skip_bits(bx, 1);
    copyright_number_3 =   m2vd_read_bits(bx, 22);

    (void)copyright_flag;
    (void)copyright_identifier;
    (void)original_or_copy;
    (void)copyright_number_1;
    (void)copyright_number_2;
    (void)copyright_number_3;
    (void)reserved_data;

    return  M2VD_DEC_OK;
}

static int m2vd_decode_ext_header(M2VDParserContext *ctx)
{
    RK_U32      code;
    RK_U32      ext_ID;
    int         result;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    code = m2vd_search_header(bx);

    if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
        mpp_log("[m2v]: decoder_ext_header : seach_header return code: %#03x", code);
    }
    while (code == EXTENSION_START_CODE || code == USER_DATA_START_CODE) {
        if (code == EXTENSION_START_CODE) {
            mpp_skip_bits(bx, 32);
            ext_ID = m2vd_read_bits(bx, 4);
            if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
                mpp_log("[m2v]: decoder_ext_header : ext_ID: %d", ext_ID);
            }
            switch (ext_ID) {
            case SEQUENCE_EXTENSION_ID:
                result = m2vd_decode_seq_ext_header(ctx);
                break;
            case SEQUENCE_DISPLAY_EXTENSION_ID:
                result = m2vd_decode_seqdisp_ext_header(ctx);
                break;
            case QUANT_MATRIX_EXTENSION_ID:
                result = m2vd_decode_matrix_ext_header(ctx);
                break;
            case SEQUENCE_SCALABLE_EXTENSION_ID:
                result = m2vd_decode_scalable_ext_header(ctx);
                break;
            case PICTURE_DISPLAY_EXTENSION_ID:
                result = m2vd_decode_picdisp_ext_header(ctx);
                break;
            case PICTURE_CODING_EXTENSION_ID:
                result = m2vd_decode_pic_ext_header(ctx);
                break;
            case PICTURE_SPATIAL_SCALABLE_EXTENSION_ID:
                result = m2vd_decode_spatial_ext_header(ctx);
                break;
            case PICTURE_TEMPORAL_SCALABLE_EXTENSION_ID:
                result = m2vd_decode_temp_ext_header();
                break;
            case COPYRIGHT_EXTENSION_ID:
                result = m2vd_decode_copyright_ext_header(ctx);
                break;
            case NO_MORE_STREAM:
                break;
            default:
                break;
            }

            if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
                mpp_log("[m2v]: decoder_ext_header result: %d", result);
            }
            if (result)
                return result;
        } else {
            mpp_skip_bits(bx, 32);
        }
        code = m2vd_search_header(bx);
    }

    return MPP_OK;
}

static int m2vd_decode_gop_header(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;

    ctx->gop_head.drop_flag   = m2vd_read_bits(bx, 1);
    ctx->gop_head.hour        = m2vd_read_bits(bx, 5);
    ctx->gop_head.minute      = m2vd_read_bits(bx, 6);

    mpp_skip_bits(bx, 1);

    ctx->gop_head.sec         = m2vd_read_bits(bx, 6);
    ctx->gop_head.frame       = m2vd_read_bits(bx, 6);
    ctx->gop_head.closed_gop  = m2vd_read_bits(bx, 1);
    ctx->gop_head.broken_link = m2vd_read_bits(bx, 1);

    return m2vd_decode_ext_header(ctx);
}

static int m2vd_decode_seq_header(M2VDParserContext *ctx)
{
    RK_U32  i;
    BitReadCtx_t *bx = ctx->bitread_ctx;
    ctx->seq_head.decode_width = m2vd_read_bits(bx, 12);
    ctx->seq_head.decode_height = m2vd_read_bits(bx, 12);
    ctx->display_width = ctx->seq_head.decode_width;
    ctx->display_height = ctx->seq_head.decode_height;
    ctx->seq_head.aspect_ratio_information = m2vd_read_bits(bx, 4);
    ctx->seq_head.frame_rate_code = m2vd_read_bits(bx, 4);
    if (!ctx->frame_period)
        ctx->frame_period = frame_period_Table[ctx->seq_head.frame_rate_code];
    ctx->seq_head.bit_rate_value = m2vd_read_bits(bx, 18);
    mpp_skip_bits(bx, 1);
    ctx->seq_head.vbv_buffer_size = m2vd_read_bits(bx, 10);
    ctx->seq_head.constrained_parameters_flag = m2vd_read_bits(bx, 1);
    ctx->seq_head.load_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (ctx->seq_head.load_intra_quantizer_matrix) {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pIntra_table[scanOrder[0][i]] = (unsigned char)m2vd_read_bits(bx, 8);
    } else {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pIntra_table[i] = intraDefaultQMatrix[i];
    }

    ctx->seq_head.load_non_intra_quantizer_matrix = m2vd_read_bits(bx, 1);
    if (ctx->seq_head.load_non_intra_quantizer_matrix) {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pInter_table[scanOrder[0][i]] = (unsigned char)m2vd_read_bits(bx, 8);

    } else {
        for (i = 0; i < 64; i++)
            ctx->seq_head.pInter_table[i] = 16;

    }


    return m2vd_decode_ext_header(ctx);
}

static int m2vd_extra_bit_information(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;
    while (m2vd_read_bits(bx, 1)) {
        mpp_skip_bits(bx, 8);
    }
    return M2VD_DEC_OK;
}

static int m2vd_decode_pic_header(M2VDParserContext *ctx)
{
    BitReadCtx_t *bx = ctx->bitread_ctx;
    ctx->pic_head.temporal_reference  = m2vd_read_bits(bx, 10);
    ctx->pic_head.picture_coding_type = m2vd_read_bits(bx, 3);
    ctx->pic_head.vbv_delay           = m2vd_read_bits(bx, 16);
    if (ctx->pic_head.temporal_reference > 50) {
        ctx->pic_head.temporal_reference = ctx->pretemporal_reference;
    }
    //if ((maxFrame_inGOP < temporal_reference)&&(temporal_reference<60))
    //  maxFrame_inGOP = temporal_reference;
    if (((RK_S32)ctx->maxFrame_inGOP < ctx->pic_head.temporal_reference) && (ctx->pic_head.temporal_reference < 60))
        ctx->maxFrame_inGOP = ctx->pic_head.temporal_reference;


    if (ctx->pic_head.picture_coding_type == M2VD_CODING_TYPE_P ||
        ctx->pic_head.picture_coding_type == M2VD_CODING_TYPE_B) {
        ctx->pic_head.full_pel_forward_vector = m2vd_read_bits(bx, 1);
        ctx->pic_head.forward_f_code = m2vd_read_bits(bx, 3);
    }
    if (ctx->pic_head.picture_coding_type == M2VD_CODING_TYPE_B) {
        ctx->pic_head.full_pel_backward_vector = m2vd_read_bits(bx, 1);
        ctx->pic_head.backward_f_code = m2vd_read_bits(bx, 3);
    }

    m2vd_extra_bit_information(ctx);

    return m2vd_decode_ext_header(ctx);
}

static MPP_RET m2vd_decode_head(M2VDParserContext *ctx)
{
    RK_U32      code;
    int         ret = 0;
    BitReadCtx_t *bx = ctx->bitread_ctx;

    m2vd_dbg_func("FUN_I");
    while (1) {
        code = m2vd_search_header(bx);
        if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
            mpp_log("[m2v]: decoder_header : seach_header return code: 0x%3x", code);
        }
        if (code == NO_MORE_STREAM)
            return M2VD_DEC_FILE_END;
        code = m2vd_read_bits(bx, 32);

        switch (code) {
        case SEQUENCE_HEADER_CODE:
            ret = m2vd_decode_seq_header(ctx);
            if (!ret) {
                MppPacket pkt = ctx->input_packet;
                if (mpp_packet_get_flag(pkt) & MPP_PACKET_FLAG_EXTRA_DATA) {
                    ctx->mExtraHeaderDecFlag = 1;
                } else {
                    ctx->mHeaderDecFlag = 1;
                }
            }
            break;
        case GROUP_START_CODE:
            ret = m2vd_decode_gop_header(ctx);
            break;
        case PICTURE_START_CODE:
            ret = m2vd_decode_pic_header(ctx);
            ret = M2VD_DEC_PICHEAD_OK;
            break;
        case SEQUENCE_END_CODE:
            ret = M2VD_DEC_STREAM_END;
            break;
        default:
            if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
                mpp_log("code=%x,leftbit=%d", code, m2vd_get_leftbits(bx));
            }
            break;
        }
        if (ret)
            break;
    }
    m2vd_dbg_func("FUN_O");

    return ret;
}

static MPP_RET m2vd_alloc_frame(M2VDParserContext *ctx)
{
    RK_U64 pts = (RK_U64)(ctx->pts / 1000);
    if (ctx->resetFlag && ctx->pic_head.picture_coding_type != M2VD_CODING_TYPE_I) {
        mpp_log("[m2v]: resetFlag[%d] && picture_coding_type[%d] != I_TYPE", ctx->resetFlag, ctx->pic_head.picture_coding_type);
        return MPP_NOK;
    } else {
        ctx->resetFlag = 0;
    }
    if ((ctx->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_FRAME ) ||
        ((ctx->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) && ctx->pic_code_ext_head.top_field_first) ||
        ((ctx->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_BOTTOM_FIELD) && (!ctx->pic_code_ext_head.top_field_first)) ||
        (ctx->frame_cur->slot_index == 0xff)) {
        RK_S64 Time = 0;
        if (ctx->PreGetFrameTime != pts) {
            RK_U32 tmp_frame_period;
            if (ctx->GroupFrameCnt) {
                ctx->GroupFrameCnt = ctx->GroupFrameCnt + ctx->pic_head.temporal_reference;
            } else if (ctx->pic_head.temporal_reference == (RK_S32)ctx->PreChangeTime_index)
                ctx->GroupFrameCnt = ctx->max_temporal_reference + 1;
            else if (ctx->pic_head.temporal_reference)
                ctx->GroupFrameCnt = ctx->pic_head.temporal_reference - ctx->PreChangeTime_index;
            else
                ctx->GroupFrameCnt = ctx->max_temporal_reference - ctx->PreChangeTime_index + 1;

            tmp_frame_period = pts - ctx->PreGetFrameTime;

            if ((pts > ctx->PreGetFrameTime) && (ctx->pic_head.temporal_reference > (RK_S32)ctx->PreChangeTime_index)) {
                RK_S32 theshold_frame_period = tmp_frame_period * 2 ;
                RK_S32 last_frame_period = ctx->preframe_period ? ctx->preframe_period : ctx->frame_period;
                RK_S32 predict_frame_period = (ctx->pic_head.temporal_reference - ctx->PreChangeTime_index) * last_frame_period / 256;
                if (theshold_frame_period < predict_frame_period) {
                    pts = ctx->PreGetFrameTime + predict_frame_period;
                    tmp_frame_period = predict_frame_period;
                }
            }

            if ((pts > ctx->PreGetFrameTime) && (ctx->GroupFrameCnt > 0)) {
                tmp_frame_period = (tmp_frame_period * 256) / ctx->GroupFrameCnt;
                if ((tmp_frame_period > 4200) && (tmp_frame_period < 11200) && (abs(ctx->frame_period - tmp_frame_period) > 128)) {
                    if (abs(ctx->preframe_period - tmp_frame_period) > 128)
                        ctx->preframe_period = tmp_frame_period;
                    else
                        ctx->frame_period = tmp_frame_period;
                }
            }

            ctx->Group_start_Time = pts - (ctx->pic_head.temporal_reference * ctx->frame_period / 256);
            if (ctx->Group_start_Time < 0)
                ctx->Group_start_Time = 0;
            ctx->PreGetFrameTime = pts;
            ctx->PreChangeTime_index = ctx->pic_head.temporal_reference;
            ctx->GroupFrameCnt = 0;
        } else if ((RK_S32)ctx->pretemporal_reference > ctx->pic_head.temporal_reference + 5) {
            ctx->Group_start_Time += ((ctx->max_temporal_reference + 1) * ctx->frame_period / 256);
            ctx->GroupFrameCnt = ctx->max_temporal_reference - ctx->PreChangeTime_index + 1;
            ctx->max_temporal_reference = 0;
        }
        if ((RK_S32)ctx->pretemporal_reference > ctx->pic_head.temporal_reference + 5)
            ctx->max_temporal_reference = 0;
        if (ctx->pic_head.temporal_reference > (RK_S32)ctx->max_temporal_reference)
            ctx->max_temporal_reference = ctx->pic_head.temporal_reference;
        ctx->pretemporal_reference = ctx->pic_head.temporal_reference;
        /*if(picture_coding_type == I_TYPE)
        {
            if (Video_Bitsream.Slice_Time.low_part >= (RK_U32)(frame_period/128))
                Group_start_Time = Video_Bitsream.Slice_Time.low_part - (RK_U32)(frame_period/128);
            else
                Group_start_Time = Video_Bitsream.Slice_Time.low_part;
        }*/
        Time = ctx->Group_start_Time;
        Time += ((ctx->pic_head.temporal_reference * ctx->frame_period) / 256);
        if (Time < 0) {
            Time = 0;
        }
        //base.Current_frame->ShowTime = Time;//Video_Bitsream.Slice_Time.low_part;
        //!< mark current slot error
        if (ctx->pic_head.picture_coding_type != M2VD_CODING_TYPE_I
            && ctx->pic_head.pre_picture_coding_type == ctx->pic_head.picture_coding_type) {
            if ((ctx->pic_head.temporal_reference - ctx->pic_head.pre_temporal_reference > 3)
                || (ctx->pic_head.temporal_reference - ctx->pic_head.pre_temporal_reference < -3)) {
                mpp_frame_set_errinfo(ctx->frame_cur->f, 1);
            } else {
                mpp_frame_set_errinfo(ctx->frame_cur->f, 0);
            }
        }
        ctx->pic_head.pre_temporal_reference = ctx->pic_head.temporal_reference;
        ctx->pic_head.pre_picture_coding_type = ctx->pic_head.picture_coding_type;
        ctx->frame_cur->picCodingType = ctx->pic_head.picture_coding_type;
        ctx->seq_head.decode_height = (ctx->seq_head.decode_height + 15) & (~15);
        ctx->seq_head.decode_width = (ctx->seq_head.decode_width + 15) & (~15);

        if ((ctx->ref_frame_cnt < 2) && (ctx->frame_cur->picCodingType == M2VD_CODING_TYPE_B)) {
            // not enough refs on B-type need cal pts and parser ctx but not need to decode
            mpp_log("[m2v]: (ref_frame_cnt[%d] < 2) && (frame_cur->picCodingType[%d] == B_TYPE)", ctx->ref_frame_cnt, ctx->frame_cur->picCodingType);
            return MPP_NOK;
        }
        if (ctx->frame_cur->slot_index == 0xff) {
            RK_U32 frametype = 0;
            mpp_frame_set_width(ctx->frame_cur->f, ctx->display_width);
            mpp_frame_set_height(ctx->frame_cur->f, ctx->display_height);
            mpp_frame_set_hor_stride(ctx->frame_cur->f, ctx->display_width);
            mpp_frame_set_ver_stride(ctx->frame_cur->f, ctx->display_height);
            mpp_frame_set_errinfo(ctx->frame_cur->f, 0);
            mpp_frame_set_pts(ctx->frame_cur->f, Time * 1000);
            ctx->frame_cur->flags = M2V_OUT_FLAG;
            if (ctx->seq_ext_head.progressive_sequence) {
                frametype = MPP_FRAME_FLAG_FRAME;
            } else {
                frametype = MPP_FRAME_FLAG_PAIRED_FIELD;
                if (ctx->pic_code_ext_head.top_field_first)
                    frametype |= MPP_FRAME_FLAG_TOP_FIRST;
                else
                    frametype |= MPP_FRAME_FLAG_BOT_FIRST;
            }
            mpp_frame_set_mode(ctx->frame_cur->f, frametype);

            if (ctx->seq_head.aspect_ratio_information >= 0 && ctx->seq_head.aspect_ratio_information < 16)
                mpp_frame_set_sar(ctx->frame_cur->f, mpeg2_aspect[ctx->seq_head.aspect_ratio_information]);

            mpp_buf_slot_get_unused(ctx->frame_slots, &ctx->frame_cur->slot_index);
            mpp_buf_slot_set_prop(ctx->frame_slots, ctx->frame_cur->slot_index, SLOT_FRAME, ctx->frame_cur->f);
            mpp_buf_slot_set_flag(ctx->frame_slots, ctx->frame_cur->slot_index, SLOT_CODEC_USE);
            mpp_buf_slot_set_flag(ctx->frame_slots, ctx->frame_cur->slot_index, SLOT_HAL_OUTPUT);
        }
    }
    //alloc frame space
    if ((ctx->ref_frame_cnt < 2) && (ctx->frame_cur->picCodingType == M2VD_CODING_TYPE_B)) {
        mpp_log("[m2v]: (ref_frame_cnt[%d] < 2) && (frame_cur->picCodingType[%d] == B_TYPE)", ctx->ref_frame_cnt, ctx->frame_cur->picCodingType);
        return MPP_NOK;
    }

    return MPP_OK;
}

static MPP_RET m2v_update_ref_frame(M2VDParserContext *p)
{

    MPP_RET ret = MPP_OK;
    m2vd_dbg_func("FUN_I");
    //push frame
    if ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_FRAME) ||
        ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_BOTTOM_FIELD) && p->pic_code_ext_head.top_field_first ) ||
        ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) && (!p->pic_code_ext_head.top_field_first))) {

        if (p->frame_cur->picCodingType == M2VD_CODING_TYPE_B) {
            mpp_buf_slot_set_flag(p->frame_slots, p->frame_cur->slot_index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(p->frame_slots, p->frame_cur->slot_index, QUEUE_DISPLAY);
            mpp_buf_slot_clr_flag(p->frame_slots, p->frame_cur->slot_index, SLOT_CODEC_USE);
            p->frame_cur->slot_index = 0xFF;
        } else if (p->frame_cur->picCodingType != 0xffffffff) {
            M2VDFrameHead *tmpHD = NULL;
            p->ref_frame_cnt++;
            if (p->frame_ref0->slot_index < 0x7f) {
                mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref0->slot_index, SLOT_QUEUE_USE);
                mpp_buf_slot_enqueue(p->frame_slots, p->frame_ref0->slot_index, QUEUE_DISPLAY);
                p->frame_ref0->flags = 0;
            }
            if (p->frame_ref1->slot_index < 0x7f) {
                mpp_buf_slot_clr_flag(p->frame_slots, p->frame_ref1->slot_index, SLOT_CODEC_USE);
                p->frame_ref1->slot_index = 0xff;
            }
            tmpHD = p->frame_ref1;
            p->frame_ref1 = p->frame_ref0;
            p->frame_ref0 = p->frame_cur;
            p->frame_cur = tmpHD;
        }
    }
    return ret;
}

static MPP_RET m2vd_convert_to_dxva(M2VDParserContext *p)
{
    MPP_RET ret = MPP_OK;
    m2vd_dbg_func("FUN_I");
    M2VDDxvaParam *dst = p->dxva_ctx;
    M2VDFrameHead *pfw = p->frame_ref1;
    M2VDFrameHead *pbw = p->frame_ref0;
    BitReadCtx_t *bx = p->bitread_ctx;
    RK_U32 i = 0;
    RK_U32 error_info = 0;

    RK_S32 readbits = m2vd_get_readbits(bx);

    dst->seq.decode_width                       = p->seq_head.decode_width;
    dst->seq.decode_height                      = p->seq_head.decode_height;
    dst->seq.aspect_ratio_information           = p->seq_head.aspect_ratio_information;
    dst->seq.frame_rate_code                    = p->seq_head.frame_rate_code;
    dst->seq.bit_rate_value                     = p->seq_head.bit_rate_value;
    dst->seq.vbv_buffer_size                    = p->seq_head.vbv_buffer_size;
    dst->seq.constrained_parameters_flag        = p->seq_head.constrained_parameters_flag;
    dst->seq.load_intra_quantizer_matrix        = p->seq_head.load_intra_quantizer_matrix;
    dst->seq.load_non_intra_quantizer_matrix    = p->seq_head.load_non_intra_quantizer_matrix;

    dst->seq_ext.profile_and_level_indication   = p->seq_ext_head.profile_and_level_indication;
    dst->seq_ext.progressive_sequence           = p->seq_ext_head.progressive_sequence;
    dst->seq_ext.chroma_format                  = p->seq_ext_head.chroma_format;
    dst->seq_ext.low_delay                      = p->seq_ext_head.low_delay;
    dst->seq_ext.frame_rate_extension_n         = p->seq_ext_head.frame_rate_extension_n;
    dst->seq_ext.frame_rate_extension_d         = p->seq_ext_head.frame_rate_extension_d;


    dst->gop.drop_flag           = p->gop_head.drop_flag;
    dst->gop.hour                = p->gop_head.hour;
    dst->gop.minute              = p->gop_head.minute;
    dst->gop.sec                 = p->gop_head.sec;
    dst->gop.frame               = p->gop_head.frame;
    dst->gop.closed_gop          = p->gop_head.closed_gop;
    dst->gop.broken_link         = p->gop_head.broken_link;


    dst->pic.temporal_reference             = p->pic_head.temporal_reference;
    dst->pic.picture_coding_type            = p->pic_head.picture_coding_type;
    dst->pic.pre_picture_coding_type        = p->pic_head.pre_picture_coding_type;
    dst->pic.vbv_delay                      = p->pic_head.vbv_delay;
    dst->pic.full_pel_forward_vector        = p->pic_head.full_pel_forward_vector;
    dst->pic.forward_f_code                 = p->pic_head.forward_f_code;
    dst->pic.full_pel_backward_vector       = p->pic_head.full_pel_backward_vector;
    dst->pic.backward_f_code                = p->pic_head.backward_f_code;
    dst->pic.pre_temporal_reference         = p->pic_head.pre_temporal_reference;

    dst->seq_disp_ext.video_format              = p->seq_disp_ext_head.video_format;
    dst->seq_disp_ext.color_description         = p->seq_disp_ext_head.color_description;
    dst->seq_disp_ext.color_primaries           = p->seq_disp_ext_head.color_primaries;
    dst->seq_disp_ext.transfer_characteristics  = p->seq_disp_ext_head.transfer_characteristics;
    dst->seq_disp_ext.matrix_coefficients       = p->seq_disp_ext_head.matrix_coefficients;

    memcpy(dst->pic_code_ext.f_code, p->pic_code_ext_head.f_code, 2 * 2 * sizeof(RK_S32));
    dst->pic_code_ext.intra_dc_precision            = p->pic_code_ext_head.intra_dc_precision;
    dst->pic_code_ext.picture_structure             = p->pic_code_ext_head.picture_structure;
    dst->pic_code_ext.top_field_first               = p->pic_code_ext_head.top_field_first;
    dst->pic_code_ext.frame_pred_frame_dct          = p->pic_code_ext_head.frame_pred_frame_dct;
    dst->pic_code_ext.concealment_motion_vectors    = p->pic_code_ext_head.concealment_motion_vectors;
    dst->pic_code_ext.q_scale_type                  = p->pic_code_ext_head.q_scale_type;
    dst->pic_code_ext.intra_vlc_format              = p->pic_code_ext_head.intra_vlc_format;
    dst->pic_code_ext.alternate_scan                = p->pic_code_ext_head.alternate_scan;
    dst->pic_code_ext.repeat_first_field            = p->pic_code_ext_head.repeat_first_field;
    dst->pic_code_ext.chroma_420_type               = p->pic_code_ext_head.chroma_420_type;
    dst->pic_code_ext.progressive_frame             = p->pic_code_ext_head.progressive_frame;
    dst->pic_code_ext.composite_display_flag        = p->pic_code_ext_head.composite_display_flag;
    dst->pic_code_ext.v_axis                        = p->pic_code_ext_head.v_axis;
    dst->pic_code_ext.field_sequence                = p->pic_code_ext_head.field_sequence;
    dst->pic_code_ext.sub_carrier                   = p->pic_code_ext_head.sub_carrier;
    dst->pic_code_ext.burst_amplitude               = p->pic_code_ext_head.burst_amplitude;
    dst->pic_code_ext.sub_carrier_phase             = p->pic_code_ext_head.sub_carrier_phase;

    memcpy(dst->pic_disp_ext.frame_center_horizontal_offset, p->pic_disp_ext_head.frame_center_horizontal_offset, 3 * sizeof(RK_S32));
    memcpy(dst->pic_disp_ext.frame_center_vertical_offset, p->pic_disp_ext_head.frame_center_vertical_offset, 3 * sizeof(RK_S32));

    dst->bitstream_length = p->frame_size - ((readbits >> 3) & (~7));
    dst->bitstream_offset = ((readbits >> 3) & (~7));
    dst->bitstream_start_bit = readbits & 0x3f;
    dst->qp_tab = p->qp_tab_sw_buf;
    dst->CurrPic.Index7Bits = p->frame_cur->slot_index;
    p->cur_slot_index = p->frame_cur->slot_index;

    if (p->frame_ref0->slot_index == 0xff) {
        pbw = p->frame_cur;
    }
    if (p->frame_ref1->slot_index == 0xff) {
        pfw = pbw;
    }

    for (i = 0; i < 4; i++) {
        dst->frame_refs[i].bPicEntry = 0xff;
    }
    if (p->pic_head.picture_coding_type == M2VD_CODING_TYPE_B) {
        MppFrame frame0 = NULL;
        MppFrame frame1 = NULL;
        dst->frame_refs[0].Index7Bits = pfw->slot_index;
        dst->frame_refs[1].Index7Bits = pfw->slot_index;
        dst->frame_refs[2].Index7Bits = pbw->slot_index;
        dst->frame_refs[3].Index7Bits = pbw->slot_index;
        mpp_buf_slot_get_prop(p->frame_slots, pfw->slot_index, SLOT_FRAME_PTR, &frame0);
        mpp_buf_slot_get_prop(p->frame_slots, pbw->slot_index, SLOT_FRAME_PTR, &frame1);
        error_info = mpp_frame_get_errinfo(frame0) | mpp_frame_get_errinfo(frame1);
    } else {
        MppFrame frame = NULL;
        if ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_FRAME) ||
            ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) && p->pic_code_ext_head.top_field_first) ||
            ((p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_BOTTOM_FIELD) && (!p->pic_code_ext_head.top_field_first))) {
            dst->frame_refs[0].Index7Bits = pbw->slot_index;
            dst->frame_refs[1].Index7Bits = pbw->slot_index;
        } else if (p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_TOP_FIELD) {
            dst->frame_refs[0].Index7Bits = pbw->slot_index;
            dst->frame_refs[1].Index7Bits = p->frame_cur->slot_index;
        } else if (p->pic_code_ext_head.picture_structure == M2VD_PIC_STRUCT_BOTTOM_FIELD) {
            dst->frame_refs[0].Index7Bits = p->frame_cur->slot_index;
            dst->frame_refs[1].Index7Bits = pbw->slot_index;
        }
        dst->frame_refs[2].Index7Bits = p->frame_cur->slot_index;
        dst->frame_refs[3].Index7Bits = p->frame_cur->slot_index;
        mpp_buf_slot_get_prop(p->frame_slots, pbw->slot_index, SLOT_FRAME_PTR, &frame);
        error_info = mpp_frame_get_errinfo(frame);
    }
    if (p->frame_cur->picCodingType == M2VD_CODING_TYPE_I) {
        error_info = 0;
    }
    dst->seq_ext_head_dec_flag = p->MPEG2_Flag;
    {
        MppFrame frame = NULL;
        mpp_buf_slot_get_prop(p->frame_slots, p->cur_slot_index, SLOT_FRAME_PTR, &frame);
        mpp_frame_set_errinfo(frame, error_info);
    }
    m2vd_dbg_func("FUN_O");
    return ret;
}

MPP_RET m2vd_parser_parse(void *ctx, HalDecTask *in_task)
{
    MPP_RET ret = MPP_OK;
    int rev = 0;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    m2vd_dbg_func("FUN_I");

    M2VD_CHK_I(in_task->valid);

    in_task->valid = 0;

    p->flush_dpb_eos = 0;

    p->frame_size = (RK_U32)mpp_packet_get_length(in_task->input_packet);

    mpp_set_bitread_ctx(p->bitread_ctx, p->bitstream_sw_buf, p->frame_size);

    rev = m2vd_decode_head(p);

    if (!p->mHeaderDecFlag) {
        if (p->mExtraHeaderDecFlag &&
            p->pic_head.picture_coding_type == M2VD_CODING_TYPE_I) {
            p->mHeaderDecFlag = 1;
            if (M2VD_DBG_SEC_HEADER & m2vd_debug)
                mpp_log("[m2v]: use extra data sequence header");
        } else {
            if (M2VD_DBG_SEC_HEADER & m2vd_debug)
                mpp_log("[m2v]: !mHeaderDecFlag");
            goto __FAILED;
        }
    }

    p->mb_width = (p->seq_head.decode_width + 15) >> 4;
    p->mb_height = (p->seq_head.decode_height + 15) >> 4;

    if (rev < M2VD_DEC_OK) {
        if (M2VD_DBG_SEC_HEADER & m2vd_debug) {
            mpp_log("decoder_header error rev %d", rev);
        }
        goto __FAILED;
    }

    if (rev == M2VD_DEC_PICHEAD_OK) {
        if (MPP_OK != m2vd_alloc_frame(p)) {
            mpp_err("m2vd_alloc_frame not OK");
            goto __FAILED;
        }
        m2vd_convert_to_dxva(p);
        in_task->syntax.data = (void *)p->dxva_ctx;
        in_task->syntax.number = sizeof(M2VDDxvaParam);
        in_task->output = p->frame_cur->slot_index;
        p->pic_head.pre_picture_coding_type = p->pic_head.picture_coding_type;

        if (p->frame_ref0->slot_index < 0x7f &&
            (p->frame_ref0->slot_index != p->frame_cur->slot_index)) {
            mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref0->slot_index, SLOT_HAL_INPUT);
            in_task->refer[0] = p->frame_ref0->slot_index;
        }

        if (p->frame_ref1->slot_index < 0x7f && (p->frame_ref1->slot_index != p->frame_cur->slot_index)) {
            mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref1->slot_index, SLOT_HAL_INPUT);
            in_task->refer[1] = p->frame_ref1->slot_index;
        }

        MppFrame frame = NULL;
        mpp_buf_slot_get_prop(p->frame_slots, p->cur_slot_index, SLOT_FRAME_PTR, &frame);
        mpp_frame_set_poc(frame, p->pic_head.temporal_reference);

        in_task->valid = 1;
        m2v_update_ref_frame(p);
    }

__FAILED:
    if (p->eos) {
        m2vd_parser_flush(ctx);
    }

    m2vd_dbg_func("FUN_O");

    return ret;
}

MPP_RET m2vd_parser_callback(void *ctx, void *errinfo)
{
    MPP_RET ret = MPP_OK;
    M2VDContext *c = (M2VDContext *)ctx;
    M2VDParserContext *p = (M2VDParserContext *)c->parse_ctx;
    MppFrame frame = NULL;
    (void)errinfo;

    m2vd_dbg_func("FUN_I");
    mpp_buf_slot_get_prop(p->frame_slots, p->cur_slot_index, SLOT_FRAME_PTR, &frame);
    mpp_frame_set_errinfo(frame, 1);
    m2vd_dbg_func("FUN_O");

    return ret;
}
