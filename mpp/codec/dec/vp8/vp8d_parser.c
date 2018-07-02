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
#define MODULE_TAG "vp8d_parser"

#include <string.h>

#include "vp8d_parser.h"
#include "vp8d_codec.h"
#include "mpp_frame.h"
#include "mpp_env.h"

#define FUN_T(tag) \
    do {\
        if (VP8D_DBG_FUNCTION & vp8d_debug)\
            { mpp_log("%s: line(%d), func(%s)", tag, __LINE__, __FUNCTION__); }\
    } while (0)

static RK_U32 vp8d_debug = 0x0;

static void vp8hwdBoolStart(vpBoolCoder_t *bit_ctx, RK_U8 *buffer, RK_U32 len)
{
    FUN_T("FUN_IN");
    bit_ctx->lowvalue = 0;
    bit_ctx->range = 255;
    bit_ctx->count = 8;
    bit_ctx->buffer = buffer;
    bit_ctx->pos = 0;

    bit_ctx->value = (bit_ctx->buffer[0] << 24) + (bit_ctx->buffer[1] << 16)
                     + (bit_ctx->buffer[2] << 8) + (bit_ctx->buffer[3]);

    bit_ctx->pos += 4;

    bit_ctx->streamEndPos = len;
    bit_ctx->strmError = bit_ctx->pos > bit_ctx->streamEndPos;

    FUN_T("FUN_OUT");
}

static RK_U32 vp8hwdDecodeBool(vpBoolCoder_t *bit_ctx, RK_S32 probability)
{
    RK_U32  bit = 0;
    RK_U32  split;
    RK_U32  bigsplit;
    RK_U32  count = bit_ctx->count;
    RK_U32  range = bit_ctx->range;
    RK_U32  value = bit_ctx->value;

    FUN_T("FUN_IN");
    split = 1 + (((range - 1) * probability) >> 8);
    bigsplit = (split << 24);
    range = split;

    if (value >= bigsplit) {
        range = bit_ctx->range - split;
        value = value - bigsplit;
        bit = 1;
    }

    if (range >= 0x80) {
        bit_ctx->value = value;
        bit_ctx->range = range;
        return bit;
    } else {
        do {
            range += range;
            value += value;

            if (!--count) {
                /* no more stream to read? */
                if (bit_ctx->pos >= bit_ctx->streamEndPos) {
                    bit_ctx->strmError = 1;
                    mpp_log("vp8hwdDecodeBool read end");
                    break;
                }
                count = 8;
                value |=  bit_ctx->buffer[bit_ctx->pos];
                bit_ctx->pos++;
            }
        } while (range < 0x80);
    }


    bit_ctx->count = count;
    bit_ctx->value = value;
    bit_ctx->range = range;

    FUN_T("FUN_OUT");
    return bit;
}

static RK_U32 vp8hwdDecodeBool128(vpBoolCoder_t *bit_ctx)
{
    RK_U32 bit = 0;
    RK_U32 split;
    RK_U32 bigsplit;
    RK_U32 count =  bit_ctx->count;
    RK_U32 range = bit_ctx->range;
    RK_U32 value = bit_ctx->value;

    FUN_T("FUN_IN");
    split = (range + 1) >> 1;
    bigsplit = (split << 24);
    range = split;

    if (value >= bigsplit) {
        range = (bit_ctx->range - split);
        value = (value - bigsplit);
        bit = 1;
    }

    if (range >= 0x80) {
        bit_ctx->value = value;
        bit_ctx->range = range;

        FUN_T("FUN_OUT");
        return bit;
    } else {
        range <<= 1;
        value <<= 1;

        if (!--count) {
            /* no more stream to read? */
            if (bit_ctx->pos >= bit_ctx->streamEndPos) {
                bit_ctx->strmError = 1;
                mpp_log("vp8hwdDecodeBool128 read end");
                return 0; /* any value, not valid */
            }
            count = 8;
            value |= bit_ctx->buffer[bit_ctx->pos];
            bit_ctx->pos++;
        }
    }

    bit_ctx->count = count;
    bit_ctx->value = value;
    bit_ctx->range = range;

    FUN_T("FUN_OUT");
    return bit;
}

static RK_U32 vp8hwdReadBits(vpBoolCoder_t *bit_ctx, RK_S32 bits)
{
    RK_U32 z = 0;
    RK_S32 bit;

    FUN_T("FUN_IN");
    for (bit = bits - 1; bit >= 0; bit--) {
        z |= (vp8hwdDecodeBool128(bit_ctx) << bit);
    }

    FUN_T("FUN_OUT");
    return z;
}

static RK_U32 ScaleDimension( RK_U32 orig, RK_U32 scale )
{

    FUN_T("FUN_IN");
    switch (scale) {
    case 0:
        return orig;
        break;
    case 1: /* 5/4 */
        return (5 * orig) / 4;
        break;
    case 2: /* 5/3 */
        return (5 * orig) / 3;
        break;
    case 3: /* 2 */
        return 2 * orig;
        break;
    }

    FUN_T("FUN_OUT");
    return orig;
}

static RK_S32 DecodeQuantizerDelta(vpBoolCoder_t *bit_ctx)
{
    RK_S32  result = 0;

    FUN_T("FUN_IN");
    if (vp8hwdDecodeBool128(bit_ctx)) {
        result = vp8hwdReadBits(bit_ctx, 4);
        if (vp8hwdDecodeBool128(bit_ctx))
            result = -result;
    }

    FUN_T("FUN_OUT");
    return result;
}

MPP_RET vp8d_parser_init(void *ctx, ParserCfg *parser_cfg)
{
    MPP_RET ret = MPP_OK;
    VP8DContext *c = (VP8DContext *)ctx;
    VP8DParserContext_t *p = (VP8DParserContext_t *)c->parse_ctx;

    FUN_T("FUN_IN");
    if (p == NULL) {
        p = (VP8DParserContext_t*)mpp_calloc(VP8DParserContext_t, 1);
        if (NULL == p) {
            mpp_err("vp8d malloc VP8DParserContext_t fail");
            FUN_T("FUN_OUT");
            return MPP_ERR_NOMEM;
        }
        c->parse_ctx = p;
    }
    p->packet_slots = parser_cfg->packet_slots;
    p->frame_slots = parser_cfg->frame_slots;

    mpp_buf_slot_setup(p->frame_slots, 15);

    p->dxva_ctx = mpp_calloc(DXVA_PicParams_VP8, 1);

    if (NULL == p->dxva_ctx) {
        mpp_err("vp8d malloc dxva_ctx fail");
        FUN_T("FUN_OUT");
        return MPP_ERR_NOMEM;
    }
    p->decMode = VP8HWD_VP8;
    p->bitstream_sw_buf = mpp_malloc(RK_U8, VP8D_BUF_SIZE_BITMEM);
    mpp_packet_init(&p->input_packet, p->bitstream_sw_buf,
                    VP8D_BUF_SIZE_BITMEM);
    p->max_stream_size = VP8D_BUF_SIZE_BITMEM;

    FUN_T("FUN_OUT");
    return ret;
}

static void vp8d_unref_frame(VP8DParserContext_t *p, VP8Frame *frame)
{

    FUN_T("FUN_IN");
    if (NULL == frame || frame->ref_count <= 0
        || frame->slot_index >= 0x7f) {
        mpp_err("ref count alreay is zero");
        FUN_T("FUN_OUT");
        return;
    }
    frame->ref_count--;
    if (!frame->ref_count && frame->slot_index < 0x7f) {
        mpp_buf_slot_clr_flag(p->frame_slots, frame->slot_index, SLOT_CODEC_USE);
        frame->slot_index = 0xff;
        mpp_free(frame->f);
        mpp_free(frame);
        frame = NULL;
    }

    FUN_T("FUN_OUT");
    return;
}

static void vp8d_unref_allframe(VP8DParserContext_t *p)
{

    FUN_T("FUN_IN");
    if (NULL != p->frame_out) {
        vp8d_unref_frame(p, p->frame_out);
        p->frame_out = NULL;
    }

    if (NULL != p->frame_ref) {
        vp8d_unref_frame(p, p->frame_ref);
        p->frame_ref = NULL;
    }

    if (NULL != p->frame_golden) {
        vp8d_unref_frame(p, p->frame_golden);
        p->frame_golden = NULL;
    }

    if (NULL != p->frame_alternate) {
        vp8d_unref_frame(p, p->frame_alternate);
        p->frame_alternate = NULL;
    }

    FUN_T("FUN_OUT");
    return;
}

MPP_RET vp8d_parser_deinit(void *ctx)
{
    MPP_RET ret = MPP_OK;
    VP8DContext *c = (VP8DContext *)ctx;
    VP8DParserContext_t *p = (VP8DParserContext_t *)c->parse_ctx;

    FUN_T("FUN_IN");

    if (NULL != p->bitstream_sw_buf) {
        mpp_free(p->bitstream_sw_buf);
        p->bitstream_sw_buf = NULL;
    }

    if (NULL != p->dxva_ctx) {
        mpp_free(p->dxva_ctx);
        p->dxva_ctx = NULL;
    }

    vp8d_unref_allframe(p);

    if (p->input_packet) {
        mpp_packet_deinit(&p->input_packet);
        p->input_packet = NULL;
    }

    if ( NULL != p) {
        mpp_free(p);
    }
    FUN_T("FUN_OUT");
    return ret;
}
/*!
***********************************************************************
* \brief
*   reset
***********************************************************************
*/
MPP_RET vp8d_parser_reset(void *ctx)
{
    MPP_RET ret = MPP_OK;
    VP8DContext *c = (VP8DContext *)ctx;
    VP8DParserContext_t *p = (VP8DParserContext_t *)c->parse_ctx;

    FUN_T("FUN_IN");
    vp8d_unref_allframe(p);
    p->needKeyFrame = 0;
    p->eos = 0;
    FUN_T("FUN_OUT");
    return ret;
}

/*!
***********************************************************************
* \brief
*   flush
***********************************************************************
*/
MPP_RET vp8d_parser_flush(void *ctx)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    (void) ctx;
    FUN_T("FUN_OUT");
    return ret;
}

/*!
***********************************************************************
* \brief
*   control/perform
***********************************************************************
*/
MPP_RET vp8d_parser_control(void *ctx, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    (void)ctx;
    (void)cmd_type;
    (void)param;

    FUN_T("FUN_OUT");
    return ret;
}


/*!
***********************************************************************
* \brief
*   prepare
***********************************************************************
*/

static MPP_RET vp8d_parser_split_frame(RK_U8 *src, RK_U32 src_size,
                                       RK_U8 *dst, RK_U32 *dst_size)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    memcpy(dst, src, src_size);;
    *dst_size = src_size;

    (void)dst;

    FUN_T("FUN_OUT");
    return ret;
}


MPP_RET vp8d_parser_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 out_size = 0, len_in = 0;
    RK_U8 * pos = NULL;
    RK_U8 *buf = NULL;
    VP8DContext *c = (VP8DContext *)ctx;

    VP8DParserContext_t *p = (VP8DParserContext_t *)c->parse_ctx;
    MppPacket input_packet = p->input_packet;

    FUN_T("FUN_IN");
    task->valid = 0;


    buf = pos = mpp_packet_get_pos(pkt);
    p->pts = mpp_packet_get_pts(pkt);

    len_in = (RK_U32)mpp_packet_get_length(pkt),
    p->eos = mpp_packet_get_eos(pkt);
    // mpp_log("len_in = %d",len_in);
    if (len_in > p->max_stream_size) {
        mpp_free(p->bitstream_sw_buf);
        p->bitstream_sw_buf = NULL;
        p->bitstream_sw_buf = mpp_malloc(RK_U8, (len_in + 1024));
        if (NULL == p->bitstream_sw_buf) {
            mpp_err("vp8d_parser realloc fail");
            return MPP_ERR_NOMEM;
        }
        p->max_stream_size = len_in + 1024;
    }

    vp8d_parser_split_frame(buf,
                            len_in,
                            p->bitstream_sw_buf,
                            &out_size);
    pos += out_size;

    mpp_packet_set_pos(pkt, pos);

    if (out_size == 0 && p->eos) {
        task->flags.eos = p->eos;
        return ret;
    }



    // mpp_log("p->bitstream_sw_buf = 0x%x", p->bitstream_sw_buf);
    // mpp_log("out_size = 0x%x", out_size);
    mpp_packet_set_data(input_packet, p->bitstream_sw_buf);
    mpp_packet_set_size(input_packet, p->max_stream_size);
    mpp_packet_set_length(input_packet, out_size);
    p->stream_size = out_size;
    task->input_packet = input_packet;
    task->valid = 1;

    FUN_T("FUN_OUT");
    return ret;
}

static MPP_RET
vp8d_convert_to_syntx( VP8DParserContext_t *p, HalDecTask *in_task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, tmp;
    RK_U32 byteOffset = 0, extraBytesPacked = 0;
    DXVA_PicParams_VP8 *pic_param = p->dxva_ctx;

    FUN_T("FUN_IN");
    tmp = (p->bitstr.pos) * 8 + (8 - p->bitstr.count);

    if (p->frameTagSize == 4)
        tmp += 8;

    if (p->decMode == VP8HWD_VP8 && p->keyFrame)
        extraBytesPacked += 7;

    tmp += extraBytesPacked * 8;
    byteOffset = tmp / 8;
    pic_param->stream_start_bit = (byteOffset & 0x07U) * 8;
    byteOffset &= (~0x07U);  /* align the base */
    pic_param->stream_start_offset = byteOffset;

    pic_param->stream_start_bit += (tmp & 0x7);

    pic_param->frame_type = !p->keyFrame;
    pic_param->stVP8Segments.segmentation_enabled = p->segmentationEnabled;
    pic_param->stVP8Segments.update_mb_segmentation_map =
        p->segmentationMapUpdate;
    pic_param->mode_ref_lf_delta_enabled = p->modeRefLfEnabled;
    pic_param->mb_no_coeff_skip = p->coeffSkipMode;
    pic_param->width               = p->width;
    pic_param->height              = p->height;
    pic_param->decMode             = p->decMode;
    pic_param->filter_type      = p->loopFilterType;
    pic_param->sharpness    = p->loopFilterSharpness;
    pic_param->filter_level     = p->loopFilterLevel;
    pic_param->stVP8Segments.update_mb_segmentation_data =
        p->segmentFeatureMode;
    pic_param->version      = p->vpVersion;
    pic_param->bool_value          = ((p->bitstr.value >> 24) & (0xFFU));
    pic_param->bool_range          = (p->bitstr.range & (0xFFU));
    pic_param->frameTagSize        = p->frameTagSize;
    pic_param->streamEndPos        = p->bitstr.streamEndPos;
    pic_param->log2_nbr_of_dct_partitions = p->nbrDctPartitions;
    pic_param->offsetToDctParts    = p->offsetToDctParts;

    pic_param->y1ac_delta_q = p->qpYAc;
    pic_param->y1dc_delta_q = p->qpYDc;
    pic_param->y2ac_delta_q = p->qpY2Ac;
    pic_param->y2dc_delta_q = p->qpY2Dc;
    pic_param->uvac_delta_q = p->qpChAc;
    pic_param->uvdc_delta_q = p->qpChDc;
    pic_param->probe_skip_false = p->probMbSkipFalse;
    pic_param->prob_intra = p->probIntra;
    pic_param->prob_last = p->probRefLast;
    pic_param->prob_golden = p->probRefGolden;

    memcpy(pic_param->vp8_coef_update_probs, p->entropy.probCoeffs,
           sizeof(pic_param->vp8_coef_update_probs));
    memcpy(pic_param->vp8_mv_update_probs, p->entropy.probMvContext,
           sizeof(pic_param->vp8_mv_update_probs));

    for ( i = 0; i < 3; i++) {
        pic_param->intra_chroma_prob[i] = p->entropy.probChromaPredMode[i];
        pic_param->stVP8Segments.mb_segment_tree_probs[i] = p->probSegment[i];
    }

    pic_param->ref_frame_sign_bias_golden = p->refFrameSignBias[0];
    pic_param->ref_frame_sign_bias_altref = p->refFrameSignBias[1];


    for (i = 0; i < 4; i++) {
        pic_param->stVP8Segments.segment_feature_data[0][i] = p->segmentQp[i];
        pic_param->ref_lf_deltas[i] = p->mbRefLfDelta[i];
        pic_param->mode_lf_deltas[i] = p->mbModeLfDelta[i];
        pic_param->stVP8Segments.segment_feature_data[1][i] =
            p->segmentLoopfilter[i];
        pic_param->intra_16x16_prob[i] = p->entropy.probLuma16x16PredMode[i];
    }

    p->dxva_ctx->CurrPic.Index7Bits = p->frame_out->slot_index;
    memset(in_task->refer, -1, sizeof(in_task->refer));

    if (p->frame_ref != NULL) {
        pic_param->lst_fb_idx.Index7Bits = p->frame_ref->slot_index;
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_ref->slot_index,
                              SLOT_HAL_INPUT);
        in_task->refer[0] = p->frame_ref->slot_index;
    } else {
        pic_param->lst_fb_idx.Index7Bits = 0x7f;
    }

    if (p->frame_golden != NULL) {
        pic_param->gld_fb_idx.Index7Bits = p->frame_golden->slot_index;
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_golden->slot_index,
                              SLOT_HAL_INPUT);
        in_task->refer[1] = p->frame_golden->slot_index;
    } else {
        pic_param->gld_fb_idx.Index7Bits = 0x7f;
    }

    if (p->frame_alternate != NULL) {
        pic_param->alt_fb_idx.Index7Bits = p->frame_alternate->slot_index;
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_alternate->slot_index,
                              SLOT_HAL_INPUT);
        in_task->refer[2] = p->frame_alternate->slot_index;
    } else {
        pic_param->alt_fb_idx.Index7Bits = 0x7f;
    }

    memcpy(pic_param->dctPartitionOffsets, p->dctPartitionOffsets,
           sizeof(p->dctPartitionOffsets));

    FUN_T("FUN_OUT");
    return ret;
}

static MPP_RET vp8d_alloc_frame(VP8DParserContext_t *p)
{
    MPP_RET ret = MPP_OK;

    FUN_T("FUN_IN");
    if (NULL == p->frame_out) {
        p->frame_out = mpp_calloc(VP8Frame, 1);
        if (NULL == p->frame_out) {
            mpp_err("alloc vp8 frame fail");
            return MPP_ERR_NOMEM;
        }

        if (NULL == p->frame_out->f) {
            mpp_frame_init(&p->frame_out->f);
            if (NULL == p->frame_out->f) {
                mpp_err("alloc vp8 mpp frame fail");
                return MPP_ERR_NOMEM;
            }
        }
        p->frame_out->slot_index = 0xff;
    }

    if (p->frame_out->slot_index == 0xff) {
        mpp_frame_set_width(p->frame_out->f, p->width);
        mpp_frame_set_height(p->frame_out->f, p->height);
        mpp_frame_set_hor_stride(p->frame_out->f, p->width);
        mpp_frame_set_ver_stride(p->frame_out->f, p->height);
        mpp_frame_set_errinfo(p->frame_out->f, 0);
        mpp_frame_set_pts(p->frame_out->f, p->pts);
        ret = mpp_buf_slot_get_unused(p->frame_slots,
                                      &p->frame_out->slot_index);
        if (MPP_OK != ret) {
            mpp_err("vp8 buf_slot_get_unused get fail");
            return ret;
        }
        mpp_buf_slot_set_prop(p->frame_slots, p->frame_out->slot_index,
                              SLOT_FRAME, p->frame_out->f);
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_out->slot_index,
                              SLOT_CODEC_USE);
        mpp_buf_slot_set_flag(p->frame_slots, p->frame_out->slot_index,
                              SLOT_HAL_OUTPUT);
        mpp_frame_set_mode(p->frame_out->f, 0);

        if (p->showFrame) {
            mpp_buf_slot_set_flag(p->frame_slots, p->frame_out->slot_index,
                                  SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(p->frame_slots, p->frame_out->slot_index,
                                 QUEUE_DISPLAY);
        }
        p->frame_out->ref_count++;
    }

    FUN_T("FUN_OUT");
    return ret;
}

static void vp8d_ref_frame(VP8Frame *frame)
{

    FUN_T("FUN_IN");
    if ((NULL == frame) || (frame->slot_index >= 0x7f)) {
        mpp_err("frame is null or slot_index is no valid");
        return;
    }
    frame->ref_count++;

    FUN_T("FUN_OUT");
}


static MPP_RET vp8d_ref_update(VP8DParserContext_t *p)
{

    FUN_T("FUN_IN");
    if (p->decMode != VP8HWD_WEBP) {
        if (p->copyBufferToAlternate == 1) {
            if (NULL != p->frame_alternate) {
                vp8d_unref_frame(p, p->frame_alternate);
                p->frame_alternate = NULL;
            }
            p->frame_alternate = p->frame_ref;
            vp8d_ref_frame(p->frame_alternate);
        } else if (p->copyBufferToAlternate == 2) {
            if (NULL != p->frame_alternate) {
                vp8d_unref_frame(p, p->frame_alternate);
                p->frame_alternate = NULL;
            }
            p->frame_alternate = p->frame_golden;
            vp8d_ref_frame(p->frame_alternate);
        }

        if (p->copyBufferToGolden == 1) {
            if (NULL != p->frame_golden) {
                vp8d_unref_frame(p, p->frame_golden);
                p->frame_golden = NULL;
            }
            p->frame_golden = p->frame_ref;
            vp8d_ref_frame(p->frame_golden);
        } else if (p->copyBufferToGolden == 2) {
            if (NULL != p->frame_golden) {
                vp8d_unref_frame(p, p->frame_golden);
                p->frame_golden = NULL;
            }
            p->frame_golden = p->frame_alternate;
            vp8d_ref_frame(p->frame_golden);
        }

        if (p->refreshGolden) {
            if (NULL != p->frame_golden) {
                vp8d_unref_frame(p, p->frame_golden);
                p->frame_golden = NULL;
            }
            p->frame_golden = p->frame_out;
            vp8d_ref_frame(p->frame_golden);
        }

        if (p->refreshAlternate) {
            if (NULL != p->frame_alternate) {
                vp8d_unref_frame(p, p->frame_alternate);
                p->frame_alternate = NULL;
            }
            p->frame_alternate = p->frame_out;
            vp8d_ref_frame(p->frame_out);
        }

        if (p->refreshLast) {
            if (NULL != p->frame_ref) {
                vp8d_unref_frame(p, p->frame_ref);
                p->frame_ref = NULL;
            }
            p->frame_ref = p->frame_out;
            vp8d_ref_frame(p->frame_ref);
        }
        vp8d_unref_frame(p, p->frame_out);
        p->frame_out = NULL;
    }

    FUN_T("FUN_OUT");
    return 0;
}

static void vp8hwdResetProbs(VP8DParserContext_t *p)
{
    RK_U32 i, j, k, l;
    static const RK_U32 Vp7DefaultScan[] = {
        0,  1,  4,  8,  5,  2,  3,  6,
        9, 12, 13, 10,  7, 11, 14, 15,
    };

    FUN_T("FUN_IN");
    for ( i = 0 ; i < 16 ; ++i )
        p->vp7ScanOrder[i] = Vp7DefaultScan[i];


    /* Intra-prediction modes */
    p->entropy.probLuma16x16PredMode[0] = 112;
    p->entropy.probLuma16x16PredMode[1] = 86;
    p->entropy.probLuma16x16PredMode[2] = 140;
    p->entropy.probLuma16x16PredMode[3] = 37;
    p->entropy.probChromaPredMode[0] = 162;
    p->entropy.probChromaPredMode[1] = 101;
    p->entropy.probChromaPredMode[2] = 204;

    for (i = 0; i < MAX_NBR_OF_MB_REF_LF_DELTAS; i++)
        p->mbRefLfDelta[i] = 0;

    for (i = 0; i < MAX_NBR_OF_MB_MODE_LF_DELTAS; i++)
        p->mbModeLfDelta[i] = 0;

    /* MV context */
    k = 0;
    if (p->decMode == VP8HWD_VP8) {
        for ( i = 0 ; i < 2 ; ++i )
            for ( j = 0 ; j < VP8_MV_PROBS_PER_COMPONENT ; ++j, ++k )
                p->entropy.probMvContext[i][j] = Vp8DefaultMvProbs[i][j];
    } else {
        for ( i = 0 ; i < 2 ; ++i )
            for ( j = 0 ; j < VP7_MV_PROBS_PER_COMPONENT ; ++j, ++k )
                p->entropy.probMvContext[i][j] = Vp7DefaultMvProbs[i][j];
    }

    /* Coefficients */
    for ( i = 0 ; i < 4 ; ++i )
        for ( j = 0 ; j < 8 ; ++j )
            for ( k = 0 ; k < 3 ; ++k )
                for ( l = 0 ; l < 11 ; ++l )
                    p->entropy.probCoeffs[i][j][k][l] =
                        DefaultCoeffProbs[i][j][k][l];

    FUN_T("FUN_OUT");
}

static void vp8hwdDecodeCoeffUpdate(VP8DParserContext_t *p)
{
    RK_U32 i, j, k, l;

    FUN_T("FUN_IN");
    for ( i = 0; i < 4; i++ ) {
        for ( j = 0; j < 8; j++ ) {
            for ( k = 0; k < 3; k++ ) {
                for ( l = 0; l < 11; l++ ) {
                    if (vp8hwdDecodeBool(&p->bitstr,
                                         CoeffUpdateProbs[i][j][k][l]))
                        p->entropy.probCoeffs[i][j][k][l] =
                            vp8hwdReadBits(&p->bitstr, 8);
                }
            }
        }
    }
    FUN_T("FUN_OUT");
}

static MPP_RET vp8_header_parser(VP8DParserContext_t *p, RK_U8 *pbase,
                                 RK_U32 size)
{
    RK_U32  tmp;
    int     i, j;
    vpBoolCoder_t *bit_ctx = &p->bitstr;

    FUN_T("FUN_IN");
    if (p->keyFrame) {
        tmp = (pbase[0] << 16) | (pbase[1] << 8) | (pbase[2] << 0);
        if (tmp != VP8_KEY_FRAME_START_CODE)
            return MPP_ERR_PROTOL;
        tmp = (pbase[3] << 0) | (pbase[4] << 8);
        p->width = tmp & 0x3fff;
        p->scaledWidth = ScaleDimension(p->width, tmp >> 14);
        tmp = (pbase[5] << 0) | (pbase[6] << 8);
        p->height = tmp & 0x3fff;
        p->scaledHeight = ScaleDimension(p->height, tmp >> 14);
        pbase += 7;
        size -= 7;
    }
    vp8hwdBoolStart(bit_ctx, pbase, size);
    if (p->keyFrame) {
        p->colorSpace = (vpColorSpace_e)vp8hwdDecodeBool128(bit_ctx);
        p->clamping = vp8hwdDecodeBool128(bit_ctx);
    }
    p->segmentationEnabled = vp8hwdDecodeBool128(bit_ctx);
    p->segmentationMapUpdate = 0;
    if (p->segmentationEnabled) {
        p->segmentationMapUpdate = vp8hwdDecodeBool128(bit_ctx);
        if (vp8hwdDecodeBool128(bit_ctx)) {    /* Segmentation map update */
            p->segmentFeatureMode = vp8hwdDecodeBool128(bit_ctx);
            memset(&p->segmentQp[0], 0, MAX_NBR_OF_SEGMENTS * sizeof(RK_S32));
            memset(&p->segmentLoopfilter[0], 0,
                   MAX_NBR_OF_SEGMENTS * sizeof(RK_S32));
            for (i = 0; i < MAX_NBR_OF_SEGMENTS; i++) {
                if (vp8hwdDecodeBool128(bit_ctx)) {
                    p->segmentQp[i] = vp8hwdReadBits(bit_ctx, 7);
                    if (vp8hwdDecodeBool128(bit_ctx))
                        p->segmentQp[i] = -p->segmentQp[i];
                }
            }
            for (i = 0; i < MAX_NBR_OF_SEGMENTS; i++) {
                if (vp8hwdDecodeBool128(bit_ctx)) {
                    p->segmentLoopfilter[i] = vp8hwdReadBits(bit_ctx, 6);
                    if (vp8hwdDecodeBool128(bit_ctx))
                        p->segmentLoopfilter[i] = -p->segmentLoopfilter[i];
                }
            }
        }
        if (p->segmentationMapUpdate) {
            p->probSegment[0] = 255;
            p->probSegment[1] = 255;
            p->probSegment[2] = 255;
            for (i = 0; i < 3; i++) {
                if (vp8hwdDecodeBool128(bit_ctx)) {
                    p->probSegment[i] = vp8hwdReadBits(bit_ctx, 8);
                }
            }
        }
        if (bit_ctx->strmError) {
            mpp_err_f("paser header stream no enough");
            FUN_T("FUN_OUT");
            return MPP_ERR_STREAM;
        }
    }
    p->loopFilterType = vp8hwdDecodeBool128(bit_ctx);
    p->loopFilterLevel = vp8hwdReadBits(bit_ctx, 6);
    p->loopFilterSharpness = vp8hwdReadBits(bit_ctx, 3);
    p->modeRefLfEnabled = vp8hwdDecodeBool128(bit_ctx);
    if (p->modeRefLfEnabled) {
        if (vp8hwdDecodeBool128(bit_ctx)) {
            for (i = 0; i < MAX_NBR_OF_MB_REF_LF_DELTAS; i++) {
                if (vp8hwdDecodeBool128(bit_ctx)) {
                    p->mbRefLfDelta[i] = vp8hwdReadBits(bit_ctx, 6);
                    if (vp8hwdDecodeBool128(bit_ctx))
                        p->mbRefLfDelta[i] = -p->mbRefLfDelta[i];
                }
            }
            for (i = 0; i < MAX_NBR_OF_MB_MODE_LF_DELTAS; i++) {
                if (vp8hwdDecodeBool128(&p->bitstr)) {
                    p->mbModeLfDelta[i] = vp8hwdReadBits(bit_ctx, 6);
                    if (vp8hwdDecodeBool128(bit_ctx))
                        p->mbModeLfDelta[i] = -p->mbModeLfDelta[i];
                }
            }
        }
    }
    if (bit_ctx->strmError) {
        mpp_err_f("paser header stream no enough");
        FUN_T("FUN_OUT");
        return MPP_ERR_STREAM;
    }
    p->nbrDctPartitions = vp8hwdReadBits(bit_ctx, 2);
    p->qpYAc = vp8hwdReadBits(bit_ctx, 7);
    p->qpYDc = DecodeQuantizerDelta(bit_ctx);
    p->qpY2Dc = DecodeQuantizerDelta(bit_ctx);
    p->qpY2Ac = DecodeQuantizerDelta(bit_ctx);
    p->qpChDc = DecodeQuantizerDelta(bit_ctx);
    p->qpChAc = DecodeQuantizerDelta(bit_ctx);
    if (p->keyFrame) {
        p->refreshGolden          = 1;
        p->refreshAlternate       = 1;
        p->copyBufferToGolden     = 0;
        p->copyBufferToAlternate  = 0;

        /* Refresh entropy probs */
        p->refreshEntropyProbs = vp8hwdDecodeBool128(bit_ctx);

        p->refFrameSignBias[0] = 0;
        p->refFrameSignBias[1] = 0;
        p->refreshLast = 1;
    } else {
        /* Refresh golden */
        p->refreshGolden = vp8hwdDecodeBool128(bit_ctx);
        /* Refresh alternate */
        p->refreshAlternate = vp8hwdDecodeBool128(bit_ctx);
        if ( p->refreshGolden == 0 ) {
            /* Copy to golden */
            p->copyBufferToGolden = vp8hwdReadBits(bit_ctx, 2);
        } else
            p->copyBufferToGolden = 0;

        if ( p->refreshAlternate == 0 ) {
            /* Copy to alternate */
            p->copyBufferToAlternate = vp8hwdReadBits(bit_ctx, 2);
        } else
            p->copyBufferToAlternate = 0;

        /* Sign bias for golden frame */
        p->refFrameSignBias[0] = vp8hwdDecodeBool128(bit_ctx);
        /* Sign bias for alternate frame */
        p->refFrameSignBias[1] = vp8hwdDecodeBool128(bit_ctx);
        /* Refresh entropy probs */
        p->refreshEntropyProbs = vp8hwdDecodeBool128(bit_ctx);
        /* Refresh last */
        p->refreshLast = vp8hwdDecodeBool128(bit_ctx);
    }

    /* Make a "backup" of current entropy probabilities if refresh is not set */
    if (p->refreshEntropyProbs == 0) {
        memcpy((void*)&p->entropyLast, (void*)&p->entropy,
               (unsigned long)sizeof(vp8EntropyProbs_t));
        memcpy( (void*)p->vp7PrevScanOrder, (void*)p->vp7ScanOrder,
                (unsigned long)sizeof(p->vp7ScanOrder));
    }

    vp8hwdDecodeCoeffUpdate(p);
    p->coeffSkipMode =  vp8hwdDecodeBool128(bit_ctx);
    if (!p->keyFrame) {
        RK_U32  mvProbs;
        if (p->coeffSkipMode)
            p->probMbSkipFalse = vp8hwdReadBits(bit_ctx, 8);
        p->probIntra = vp8hwdReadBits(bit_ctx, 8);
        p->probRefLast = vp8hwdReadBits(bit_ctx, 8);
        p->probRefGolden = vp8hwdReadBits(bit_ctx, 8);
        if (vp8hwdDecodeBool128(bit_ctx)) {
            for (i = 0; i < 4; i++)
                p->entropy.probLuma16x16PredMode[i] = vp8hwdReadBits(bit_ctx, 8);
        }
        if (vp8hwdDecodeBool128(bit_ctx)) {
            for (i = 0; i < 3; i++)
                p->entropy.probChromaPredMode[i] = vp8hwdReadBits(bit_ctx, 8);
        }
        mvProbs = VP8_MV_PROBS_PER_COMPONENT;
        for ( i = 0 ; i < 2 ; ++i ) {
            for ( j = 0 ; j < (RK_S32)mvProbs ; ++j ) {
                if (vp8hwdDecodeBool(bit_ctx, MvUpdateProbs[i][j]) == 1) {
                    tmp = vp8hwdReadBits(bit_ctx, 7);
                    if ( tmp )
                        tmp = tmp << 1;
                    else
                        tmp = 1;
                    p->entropy.probMvContext[i][j] = tmp;
                }
            }
        }
    } else {
        if (p->coeffSkipMode)
            p->probMbSkipFalse = vp8hwdReadBits(bit_ctx, 8);
    }
    if (bit_ctx->strmError) {
        mpp_err_f("paser header stream no enough");
        FUN_T("FUN_OUT");
        return MPP_ERR_STREAM;
    }
    FUN_T("FUN_OUT");
    return MPP_OK;
}

static MPP_RET
vp7_header_parser(VP8DParserContext_t *p, RK_U8 *pbase, RK_U32 size)
{
    RK_U32  tmp;
    int     i, j;

    FUN_T("FUN_IN");
    vpBoolCoder_t *bit_ctx = &p->bitstr;
    vp8hwdBoolStart(bit_ctx, pbase, size);

    if (p->keyFrame) {
        p->width = vp8hwdReadBits(bit_ctx, 12);
        p->height = vp8hwdReadBits(bit_ctx, 12);
        tmp = vp8hwdReadBits(bit_ctx, 2);
        p->scaledWidth = ScaleDimension(p->width, tmp);
        tmp = vp8hwdReadBits(bit_ctx, 2);
        p->scaledHeight = ScaleDimension(p->height, tmp);
    }
    {
        const RK_U32 vp70FeatureBits[4] = { 7, 6, 0, 8 };
        const RK_U32 vp71FeatureBits[4] = { 7, 6, 0, 5 };
        const RK_U32 *featureBits;
        if (p->vpVersion == 0)
            featureBits = vp70FeatureBits;
        else
            featureBits = vp71FeatureBits;
        for (i = 0; i < MAX_NBR_OF_VP7_MB_FEATURES; i++) {
            if (vp8hwdDecodeBool128(bit_ctx)) {
                tmp = vp8hwdReadBits(bit_ctx, 8);
                for (j = 0; j < 3; j++) {
                    if (vp8hwdDecodeBool128(bit_ctx))
                        tmp = vp8hwdReadBits(bit_ctx, 8);
                }
                if (featureBits[i]) {
                    for (j = 0; j < 4; j++) {
                        if (vp8hwdDecodeBool128(bit_ctx))
                            tmp = vp8hwdReadBits(bit_ctx, featureBits[i]);
                    }
                }
                FUN_T("FUN_OUT");
                return MPP_ERR_PROTOL;
            }

        }
        p->nbrDctPartitions = 0;
    }
    p->qpYAc = (RK_S32)vp8hwdReadBits(bit_ctx, 7 );
    p->qpYDc  = vp8hwdReadBits(bit_ctx, 1 )
                ? (RK_S32)vp8hwdReadBits(bit_ctx, 7 ) : p->qpYAc;
    p->qpY2Dc = vp8hwdReadBits(bit_ctx, 1 )
                ? (RK_S32)vp8hwdReadBits(bit_ctx, 7 ) : p->qpYAc;
    p->qpY2Ac = vp8hwdReadBits(bit_ctx, 1 )
                ? (RK_S32)vp8hwdReadBits(bit_ctx, 7 ) : p->qpYAc;
    p->qpChDc = vp8hwdReadBits(bit_ctx, 1 )
                ? (RK_S32)vp8hwdReadBits(bit_ctx, 7 ) : p->qpYAc;
    p->qpChAc = vp8hwdReadBits(bit_ctx, 1 )
                ? (RK_S32)vp8hwdReadBits(bit_ctx, 7 ) : p->qpYAc;
    if (!p->keyFrame) {
        p->refreshGolden = vp8hwdDecodeBool128(bit_ctx);
        if (p->vpVersion >= 1) {
            p->refreshEntropyProbs = vp8hwdDecodeBool128(bit_ctx);
            p->refreshLast = vp8hwdDecodeBool128(bit_ctx);
        } else {
            p->refreshEntropyProbs = 1;
            p->refreshLast = 1;
        }
    } else {
        p->refreshGolden = 1;
        p->refreshAlternate = 1;
        p->copyBufferToGolden = 0;
        p->copyBufferToAlternate = 0;
        if (p->vpVersion >= 1)
            p->refreshEntropyProbs = vp8hwdDecodeBool128(bit_ctx);
        else
            p->refreshEntropyProbs = 1;
        p->refFrameSignBias[0] = 0;
        p->refFrameSignBias[1] = 0;
        p->refreshLast = 1;
    }

    if (!p->refreshEntropyProbs) {
        memcpy(&p->entropyLast, &p->entropy,
               (unsigned long)sizeof(vp8EntropyProbs_t));
        memcpy(p->vp7PrevScanOrder, p->vp7ScanOrder,
               (unsigned long)sizeof(p->vp7ScanOrder));
    }
    if (p->refreshLast) {
        if (vp8hwdDecodeBool128(bit_ctx)) {
            tmp = vp8hwdReadBits(bit_ctx, 8);
            tmp = vp8hwdReadBits(bit_ctx, 8);
            FUN_T("FUN_OUT");
            return MPP_ERR_STREAM;
        }
    }
    if (p->vpVersion == 0) {
        p->loopFilterType = vp8hwdDecodeBool128(bit_ctx);
    }
    if (vp8hwdDecodeBool128(bit_ctx)) {
        static const RK_U32 Vp7DefaultScan[] = {
            0,  1,  4,  8,  5,  2,  3,  6,
            9, 12, 13, 10,  7, 11, 14, 15,
        };
        p->vp7ScanOrder[0] = 0;
        for (i = 1; i < 16; i++)
            p->vp7ScanOrder[i] = Vp7DefaultScan[vp8hwdReadBits(bit_ctx, 4)];
    }
    if (p->vpVersion >= 1)
        p->loopFilterType = vp8hwdDecodeBool128(bit_ctx);
    p->loopFilterLevel = vp8hwdReadBits(bit_ctx, 6);
    p->loopFilterSharpness = vp8hwdReadBits(bit_ctx, 3);
    vp8hwdDecodeCoeffUpdate(p);
    if (!p->keyFrame) {
        p->probIntra = vp8hwdReadBits(bit_ctx, 8);
        p->probRefLast = vp8hwdReadBits(bit_ctx, 8);
        if (vp8hwdDecodeBool128(bit_ctx)) {
            for (i = 0; i < 4; i++)
                p->entropy.probLuma16x16PredMode[i] =
                    vp8hwdReadBits(bit_ctx, 8);
        }
        if (vp8hwdDecodeBool128(bit_ctx)) {
            for (i = 0; i < 3; i++)
                p->entropy.probChromaPredMode[i] = vp8hwdReadBits(bit_ctx, 8);
        }
        for ( i = 0 ; i < 2 ; ++i ) {
            for ( j = 0 ; j < VP7_MV_PROBS_PER_COMPONENT ; ++j ) {
                if (vp8hwdDecodeBool(bit_ctx, MvUpdateProbs[i][j])) {
                    tmp = vp8hwdReadBits(bit_ctx, 7);
                    if ( tmp )
                        tmp = tmp << 1;
                    else
                        tmp = 1;
                    p->entropy.probMvContext[i][j] = tmp;
                }
            }
        }
    }
    if (bit_ctx->strmError) {
        FUN_T("FUN_OUT");
        return MPP_ERR_PROTOL;
    }

    FUN_T("FUN_OUT");
    return MPP_OK;
}

static MPP_RET
vp8hwdSetPartitionOffsets(VP8DParserContext_t *p, RK_U8 *stream, RK_U32 len)
{
    RK_U32 i = 0;
    RK_U32 offset = 0;
    RK_U32 baseOffset;
    RK_U32 extraBytesPacked = 0;

    FUN_T("FUN_IN");
    if (p->decMode == VP8HWD_VP8 && p->keyFrame)
        extraBytesPacked += 7;

    stream += p->frameTagSize;

    baseOffset = p->frameTagSize + p->offsetToDctParts
                 + 3 * ( (1 << p->nbrDctPartitions) - 1);

    stream += p->offsetToDctParts + extraBytesPacked;
    for ( i = 0 ; i < (RK_U32)(1 << p->nbrDctPartitions) - 1 ; ++i ) {
        RK_U32  tmp;

        p->dctPartitionOffsets[i] = baseOffset + offset;
        tmp = stream[0] | (stream[1] << 8) | (stream[2] << 16);
        offset += tmp;
        stream += 3;
    }
    p->dctPartitionOffsets[i] = baseOffset + offset;

    return (p->dctPartitionOffsets[i] < len ? MPP_OK : MPP_ERR_STREAM);
}

static MPP_RET
decoder_frame_header(VP8DParserContext_t *p, RK_U8 *pbase, RK_U32 size)
{
    MPP_RET ret;

    FUN_T("FUN_IN");
    p->keyFrame = !(pbase[0] & 1);
    p->vpVersion = (pbase[0] >> 1) & 7;
    p->showFrame = 1;
    if (p->keyFrame && !p->needKeyFrame) {
        p->needKeyFrame = 1;
    } else {
        if (!p->needKeyFrame) {
            mpp_err("no found key frame");
            return MPP_NOK;
        }
    }
    if (p->decMode == VP8HWD_VP7) {
        p->offsetToDctParts = (pbase[0] >> 4) | (pbase[1] << 4) | (pbase[2] << 12);
        p->frameTagSize = p->vpVersion >= 1 ? 3 : 4;
    } else {
        p->offsetToDctParts = (pbase[0] >> 5) | (pbase[1] << 3) | (pbase[2] << 11);
#if 0
        mpp_log("offsetToDctParts %d pbase[0] = 0x%x pbase[1] = 0x%x pbase[2] = 0x%x ",
                p->offsetToDctParts, pbase[0], pbase[1], pbase[2]);
#endif
        p->showFrame = (pbase[0] >> 4) & 1;
        p->frameTagSize = 3;
    }
    pbase += p->frameTagSize;
    size -= p->frameTagSize;
    if (p->keyFrame)
        vp8hwdResetProbs(p);
    //mpp_log_f("p->decMode = %d", p->decMode);
    if (p->decMode == VP8HWD_VP8) {
        ret = vp8_header_parser(p, pbase, size);
    }  else {
        ret = vp7_header_parser(p, pbase, size);
    }
    if (ret != MPP_OK) {
        return ret;
    }
    return MPP_OK;
}

MPP_RET vp8d_parser_parse(void *ctx, HalDecTask *in_task)
{
    MPP_RET ret = MPP_OK;
    VP8DContext *c = (VP8DContext *)ctx;
    VP8DParserContext_t *p = (VP8DParserContext_t *)c->parse_ctx;
    FUN_T("FUN_IN");

    ret = decoder_frame_header(p, p->bitstream_sw_buf, p->stream_size);

    if (MPP_OK != ret) {
        mpp_err("decoder_frame_header err ret %d", ret);
        FUN_T("FUN_OUT");
        return ret;
    }

    vp8hwdSetPartitionOffsets(p, p->bitstream_sw_buf, p->stream_size);

    ret = vp8d_alloc_frame(p);
    if (MPP_OK != ret) {
        mpp_err("vp8d_alloc_frame err ret %d", ret);
        FUN_T("FUN_OUT");
        return ret;
    }

    vp8d_convert_to_syntx(p, in_task);
    /* Rollback entropy probabilities if refresh is not set */
    if (p->refreshEntropyProbs == 0) {
        memcpy((void*)&p->entropy, (void*)&p->entropyLast,
               (unsigned long)sizeof(vp8EntropyProbs_t));
        memcpy((void*)p->vp7ScanOrder, (void*)p->vp7PrevScanOrder,
               (unsigned long)sizeof(p->vp7ScanOrder));
    }
    in_task->syntax.data = (void *)p->dxva_ctx;
    in_task->syntax.number = 1;
    in_task->output = p->frame_out->slot_index;
    in_task->valid = 1;
    if (p->eos) {
        in_task->flags.eos = p->eos;
    }
    vp8d_ref_update(p);

    FUN_T("FUN_OUT");
    return ret;
}

MPP_RET vp8d_parser_callback(void *ctx, void *hal_info)
{
    MPP_RET ret = MPP_OK;
    FUN_T("FUN_IN");
    (void)ctx;
    (void)hal_info;
    FUN_T("FUN_OUT");
    return ret;
}
