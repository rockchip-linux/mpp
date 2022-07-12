/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "avs2d_parse"

#include <string.h>
#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "avs2d_api.h"
#include "avs2d_dpb.h"
#include "avs2d_parse.h"
#include "avs2d_ps.h"
#include "avs2d_global.h"

#define AVS2_ISPIC(x)  ((x) == 0xB3 || (x) == 0xB6)
#define AVS2_ISUNIT(x) ((x) == 0xB0 || (x) == 0xB1 || (x) == 0xB2 || AVS2_ISPIC(x))
#define AVS2_IS_START_CODE(x) (((x) & 0xffffff00) == 0x100)

#define AVS2_IS_SLICE_START_CODE(x) ((x) >= AVS2_SLICE_MIN_START_CODE && (x) <= AVS2_SLICE_MAX_START_CODE)
#define AVS2_IS_HEADER(x) ((x) >= AVS2_VIDEO_SEQUENCE_START_CODE && (x) <= AVS2_VIDEO_EDIT_CODE)
#define AVS2_IS_PIC_START_CODE(x) ((x) == AVS2_I_PICUTRE_START_CODE || (x) == AVS2_PB_PICTURE_START_CODE)

/**
 * @brief Find start code 00 00 01 xx
 *
 * Search 01 first and check the previous 2 bytes and the following 1 bytes.
 * If it is start code, return the value of start code at U32 as 0x000001xx.
 *
 * @param buf_start the start of input buffer
 * @param buf_end the end of input buffer
 * @param pos output value, the position of start code
 * @return RK_U32
 */
static RK_U32 avs2_find_start_code(RK_U8 *buf_start, RK_U8* buf_end, RK_U8 **pos)
{
    RK_U8 *buf_ptr = NULL;
    RK_U8 *ptr = buf_start;
    RK_U32 remain_size = buf_end - buf_start + 1;

    while (ptr < buf_end) {
        buf_ptr = memchr(ptr, 0x01, remain_size);

        if (!buf_ptr) {
            return 0;
        }

        //check the position of "01"
        if ((buf_ptr < buf_end) && (buf_ptr - buf_start > 1) &&
            (*(buf_ptr - 1) == 0) && (*(buf_ptr - 2) == 0)) {
            //found 00 00 01 xx
            *pos = buf_ptr + 1;
            return (AVS2_START_CODE | *(buf_ptr + 1));
        } else {
            // keep search at remaining buffer
            remain_size = remain_size - (buf_ptr - ptr + 1);
            ptr = buf_ptr + 1;
        }
    }

    return 0;
}

static MPP_RET avs2_add_nalu_header(Avs2dCtx_t *p_dec, RK_U32 header)
{
    MPP_RET ret = MPP_OK;

    Avs2dStreamBuf_t *p_buf = p_dec->p_header;
    RK_U32 add_size = sizeof(Avs2dNalu_t);

    if ((p_buf->len + add_size) > p_buf->size) {
        ret = MPP_NOK;
        mpp_err_f("buffer is larger than %d", p_buf->size);
        //TODO realloc
        goto __FAILED;
    }
    p_dec->nal = (Avs2dNalu_t *)&p_buf->pbuf[p_buf->len];
    p_dec->nal->eof = 0;
    p_dec->nal->header = header;
    p_dec->nal->pdata = NULL;
    p_dec->nal->length = 0;
    p_dec->nal->start_pos = 4;
    p_buf->len += add_size;
    p_dec->nal_cnt++;
    AVS2D_PARSE_TRACE("add header 0x%x, nal_cnt %d", header, p_dec->nal_cnt);

__FAILED:
    return ret;
}

/**
 * @brief store nalu startcode and data
 * TODO If nalu data isn't appear right after startcode, it may go wrong
 *
 * @param p_dec
 * @param p_start
 * @param len
 * @param start_code
 * @return MPP_RET
 */
static MPP_RET store_nalu(Avs2dCtx_t *p_dec, RK_U8 *p_start, RK_U32 len, RK_U32 start_code)
{
    MPP_RET ret = MPP_OK;
    Avs2dNalu_t *p_nalu = p_dec->nal;
    Avs2dStreamBuf_t *p_header = NULL;;
    RK_U32 add_size = SZ_1K;
    RK_U32 new_size = 0;

    if (AVS2_IS_SLICE_START_CODE(start_code)) {
        p_header = p_dec->p_stream;
        add_size = SZ_512K;
    } else if (AVS2_IS_HEADER(start_code)) {
        p_header = p_dec->p_header;
        add_size = SZ_1K;
    }

    if ((p_header->len + len) > p_header->size) {
        new_size = p_header->size + add_size;
        RK_U8 * new_buffer = mpp_realloc(p_header->pbuf, RK_U8, new_size);
        if (!new_buffer) {
            p_header->size = 0;
            return MPP_ERR_NOMEM;
        } else {
            memset(p_header + p_header->size, 0, new_size - p_header->size);
            p_header->size = new_size;
        }
    }

    if (p_nalu->length == 0) {
        p_nalu->pdata = &p_header->pbuf[p_header->len];
    }

    if (len > 0) {
        memcpy(p_nalu->pdata + p_nalu->length, p_start, len);
        // memcpy(p_nalu->pdata, p_start, len);
        p_nalu->length += len;
        p_header->len += len;
    }

    return ret;
}

/**
 * @brief Once got a new frame, reset previous stored nalu data an buffer
 *
 * @param p_dec
 * @return MPP_RET
 */
static MPP_RET reset_nalu_buf(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;

    AVS2D_PARSE_TRACE("In.");
    p_dec->prev_start_code = 0;
    p_dec->new_frame_flag = 0;

    if (p_dec->nal) {
        p_dec->nal->eof = 0;
        p_dec->nal->header = 0;
        p_dec->nal->pdata = NULL;
        p_dec->nal->length = 0;
        p_dec->nal->start_pos = 0;

        p_dec->nal = NULL;
    }

    memset(p_dec->prev_tail_data, 0xff, AVS2D_PACKET_SPLIT_CHECKER_BUFFER_SIZE);

    if (p_dec->p_stream) {
        memset(p_dec->p_stream->pbuf, 0, p_dec->p_stream->size);
        p_dec->p_stream->len = 0;
    }

    if (p_dec->p_header) {
        memset(p_dec->p_header->pbuf, 0, p_dec->p_header->size);
        p_dec->p_header->len = 0;
    }

    p_dec->nal_cnt = 0;

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

static MPP_RET parse_seq_dispay_ext_header(BitReadCtx_t *bitctx, Avs2dSeqExtHeader_t *exh)
{
    MPP_RET ret = MPP_OK;

    READ_BITS(bitctx, 3, &exh->video_format);
    READ_ONEBIT(bitctx, &exh->sample_range);
    READ_ONEBIT(bitctx, &exh->color_description);

    if (exh->color_description) {
        READ_BITS(bitctx, 8, &exh->color_primaries);
        READ_BITS(bitctx, 8, &exh->transfer_characteristics);
        READ_BITS(bitctx, 8, &exh->matrix_coefficients);
    }
    READ_BITS(bitctx, 14, &exh->display_horizontal_size);
    READ_MARKER_BIT(bitctx);
    READ_BITS(bitctx, 14, &exh->display_vertical_size);

    READ_ONEBIT(bitctx, &exh->td_mode_flag);
    if (exh->td_mode_flag) {
        READ_BITS(bitctx, 8, &exh->td_packing_mode);
        READ_ONEBIT(bitctx, &exh->view_reverse_flag);
    }

    return ret;
__BITREAD_ERR:
    return ret = bitctx->ret;
}
static MPP_RET parse_extension_header(Avs2dCtx_t *p_dec, BitReadCtx_t *bitctx, Avs2dSeqExtHeader_t *exh)
{
    MPP_RET ret = MPP_OK;
    RK_U32 val_temp = 0;

    READ_BITS(bitctx, 4, &val_temp); //!< extension_start_code
    switch (val_temp) {
    case AVS2_SEQUENCE_DISPLAY_EXTENTION:
        FUN_CHECK(ret = parse_seq_dispay_ext_header(bitctx, exh));
        p_dec->got_exh = 1;
        break;
    default:
        break;
    }

    return ret;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

MPP_RET avs2d_reset_parser(Avs2dCtx_t *p_dec)
{
    AVS2D_PARSE_TRACE("In.");
    p_dec->got_vsh      = 0;
    p_dec->got_exh      = 0;
    p_dec->got_keyframe = 0;
    p_dec->vec_flag     = 0;
    p_dec->enable_wq    = 0;
    p_dec->new_frame_flag = 0;
    p_dec->new_seq_flag = 0;
    reset_nalu_buf(p_dec);
    AVS2D_PARSE_TRACE("Out.");

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*    commit buffer to hal
***********************************************************************
*/
MPP_RET avs2d_commit_syntaxs(Avs2dSyntax_t *syntax, HalDecTask *task)
{
    task->syntax.number = 1;
    task->syntax.data = syntax;

    return MPP_OK;
}

MPP_RET avs2d_fill_parameters(Avs2dCtx_t *p_dec, Avs2dSyntax_t *syntax)
{
    RK_S32 i = 0;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;
    Avs2dFrameMgr_t *mgr  = &p_dec->frm_mgr;
    PicParams_Avs2d *pp   = &syntax->pp;
    RefParams_Avs2d *refp = &syntax->refp;
    AlfParams_Avs2d *alfp = &syntax->alfp;
    WqmParams_Avs2d *wqmp = &syntax->wqmp;

    //!< sequence header
    pp->chroma_format_idc          = vsh->chroma_format;
    pp->pic_width_in_luma_samples  = MPP_ALIGN(vsh->horizontal_size, 8);
    pp->pic_height_in_luma_samples = MPP_ALIGN(vsh->vertical_size, 8);
    pp->bit_depth_luma_minus8      = vsh->bit_depth - 8;
    pp->bit_depth_chroma_minus8    = vsh->bit_depth - 8;
    pp->lcu_size                   = vsh->lcu_size;
    pp->progressive_sequence       = vsh->progressive_sequence;
    pp->field_coded_sequence       = vsh->field_coded_sequence;
    pp->multi_hypothesis_skip_enable_flag        = vsh->enable_mhp_skip;
    pp->dual_hypothesis_prediction_enable_flag   = vsh->enable_dhp;
    pp->weighted_skip_enable_flag                = vsh->enable_wsm;
    pp->asymmetrc_motion_partitions_enable_flag  = vsh->enable_amp;
    pp->nonsquare_quadtree_transform_enable_flag = vsh->enable_nsqt;
    pp->nonsquare_intra_prediction_enable_flag   = vsh->enable_nsip;
    pp->secondary_transform_enable_flag          = vsh->enable_2nd_transform;
    pp->sample_adaptive_offset_enable_flag       = vsh->enable_sao;
    pp->adaptive_loop_filter_enable_flag         = vsh->enable_alf;
    pp->pmvr_enable_flag                         = vsh->enable_pmvr;
    pp->cross_slice_loopfilter_enable_flag       = vsh->enable_clf;

    //!< picture header
    pp->picture_type                = ph->picture_type;
    pp->scene_reference_enable_flag = ph->background_reference_flag;
    pp->bottom_field_picture_flag   = (vsh->field_coded_sequence) && (!ph->is_top_field);
    // pp->bottom_field_picture_flag   = !ph->is_top_field;
    pp->fixed_picture_qp            = ph->fixed_picture_qp;
    pp->picture_qp                  = ph->picture_qp;
    pp->loop_filter_disable_flag    = !ph->enable_loop_filter;
    pp->alpha_c_offset              = ph->alpha_c_offset;
    pp->beta_offset                 = ph->beta_offset;

    //!< picture reference params
    refp->ref_pic_num = mgr->num_of_ref;
    memset(refp->ref_poc_list, -1, sizeof(refp->ref_poc_list));
    for (i = 0; i < mgr->num_of_ref; i++) {
        refp->ref_poc_list[i] = mgr->refs[i] ? mgr->refs[i]->poi : -1;
    }

    //!< picture alf params
    alfp->enable_pic_alf_y  = ph->enable_pic_alf_y;
    alfp->enable_pic_alf_cb = ph->enable_pic_alf_cb;
    alfp->enable_pic_alf_cr = ph->enable_pic_alf_cr;
    alfp->alf_filter_num_minus1 = ph->alf_filter_num - 1;
    memcpy(alfp->alf_coeff_idx_tab, ph->alf_coeff_idx_tab, sizeof(ph->alf_coeff_idx_tab));
    memcpy(alfp->alf_coeff_y, ph->alf_coeff_y, sizeof(ph->alf_coeff_y));
    memcpy(alfp->alf_coeff_cb, ph->alf_coeff_cb, sizeof(ph->alf_coeff_cb));
    memcpy(alfp->alf_coeff_cr, ph->alf_coeff_cr, sizeof(ph->alf_coeff_cr));

    //!< picture wqm params
    wqmp->pic_weight_quant_enable_flag = p_dec->enable_wq;
    wqmp->chroma_quant_param_delta_cb = ph->chroma_quant_param_delta_cb;
    wqmp->chroma_quant_param_delta_cr = ph->chroma_quant_param_delta_cr;
    memcpy(wqmp->wq_matrix, p_dec->cur_wq_matrix, sizeof(p_dec->cur_wq_matrix));

    return MPP_OK;
}

MPP_RET avs2_split_nalu(Avs2dCtx_t *p_dec, RK_U8 *buf_start, RK_U32 buf_length, RK_U32 over_read, RK_U32 *remain)
{
    MPP_RET ret = MPP_OK;

    RK_U8 *start_code_ptr = NULL;
    RK_U32 start_code = 0;
    RK_U32 nalu_len = 0;

    RK_U8 *buf_end;

    buf_end = buf_start + buf_length - 1;

    start_code = avs2_find_start_code(buf_start, buf_end, &start_code_ptr);

    if (start_code_ptr) {
        AVS2D_PARSE_TRACE("Found start_code 0x%08x at offset 0x%08x, prev_starcode 0x%08x\n",
                          start_code, start_code_ptr - buf_start, p_dec->prev_start_code);
        if (!p_dec->new_seq_flag) {
            if (start_code == AVS2_VIDEO_SEQUENCE_START_CODE) {
                AVS2D_PARSE_TRACE("Found the first video_sequence_start_code");
                p_dec->nal_cnt = 0;
                avs2_add_nalu_header(p_dec, AVS2_VIDEO_SEQUENCE_START_CODE);
                p_dec->new_seq_flag = 1;
                p_dec->prev_start_code = AVS2_VIDEO_SEQUENCE_START_CODE;
            } else {
                AVS2D_PARSE_TRACE("Skip start code before first video_sequence_start_code");
            }

            *remain = buf_end - start_code_ptr;
        } else {
            if (start_code == AVS2_VIDEO_SEQUENCE_START_CODE) {
                AVS2D_PARSE_TRACE("Found repeated video_sequence_start_code");
            }

            if (AVS2_IS_START_CODE(p_dec->prev_start_code)) {
                nalu_len = start_code_ptr - buf_start - 3;
                if (nalu_len > over_read) {
                    store_nalu(p_dec, buf_start + over_read, nalu_len - over_read, p_dec->prev_start_code);
                }
            }

            if (AVS2_IS_SLICE_START_CODE(p_dec->prev_start_code) && !AVS2_IS_SLICE_START_CODE(start_code)) {
                p_dec->new_frame_flag = 1;
                p_dec->nal->eof = 1;
                *remain = buf_end - start_code_ptr + 4;
            } else {
                avs2_add_nalu_header(p_dec, start_code);

                // need to put slice start code to stream buffer
                if (AVS2_IS_SLICE_START_CODE(start_code)) {
                    store_nalu(p_dec, start_code_ptr - 3, 4, start_code);
                } else if (start_code == AVS2_VIDEO_SEQUENCE_END_CODE) {
                    p_dec->nal->eof = 1;
                }

                *remain = buf_end - start_code_ptr;
            }

            p_dec->prev_start_code = start_code;
        }
    } else {
        if (!p_dec->new_seq_flag) {
            AVS2D_PARSE_TRACE("Skip data code before first video_sequence_start_code");
        } else {
            if (AVS2_IS_START_CODE(p_dec->prev_start_code)) {
                nalu_len = buf_length;
                if (nalu_len > over_read) {
                    store_nalu(p_dec, buf_start + over_read, nalu_len - over_read, p_dec->prev_start_code);
                }
            }
        }

        *remain = 0;
    }

    return ret;
}

MPP_RET avs2d_parse_prepare_split(Avs2dCtx_t *p_dec, MppPacket *pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *p_curdata = NULL;
    RK_U8 *p_start = NULL;
    RK_U8 *p_end = NULL;
    RK_U32 pkt_length = 0;
    RK_U32 pkt_eos = 0;
    RK_U32 first_read_length = 0;

    AVS2D_PARSE_TRACE("In.");

    pkt_eos = mpp_packet_get_eos(pkt);
    pkt_length = (RK_U32) mpp_packet_get_length(pkt);

    // check eos
    if (pkt_eos && !pkt_length) {
        // EOS packet without data, end of stream
        task->valid = 0;
        task->flags.eos = 1;
        return ret;
    }

    if (!pkt_length) {
        AVS2D_PARSE_TRACE("Input have no stream.");
    }

    p_curdata = p_start = (RK_U8 *) mpp_packet_get_pos(pkt);
    p_end = p_start + pkt_length - 1;

    // Combine last packet data
    first_read_length = (pkt_length >= 4) ? 4 : pkt_length;
    memcpy(p_dec->prev_tail_data + 3, p_curdata, first_read_length);

    RK_U32 remain = 0;

    AVS2D_PARSE_TRACE("previous data[0~3]=%02x %02x %02x, first_read_length %d\n",
                      p_dec->prev_tail_data[0], p_dec->prev_tail_data[1],
                      p_dec->prev_tail_data[2], first_read_length);
    ret = avs2_split_nalu(p_dec, p_dec->prev_tail_data,
                          AVS2D_PACKET_SPLIT_CHECKER_BUFFER_SIZE,
                          AVS2D_PACKET_SPLIT_LAST_KEPT_LENGTH,
                          &remain);
    p_curdata = p_start + first_read_length - remain;
    AVS2D_PARSE_TRACE("remian length %d\n", remain);

    remain = 0;

    while (p_curdata < p_end) {
        ret = avs2_split_nalu(p_dec, p_curdata, p_end - p_curdata + 1, 0, &remain);

        if (ret) {
            break;
        } else {
            p_curdata = p_end - remain + 1;
        }

        if (p_dec->new_frame_flag || (p_dec->nal && p_dec->nal->eof == 1)) {
            task->valid = 1;
            break;
        }
    }

    mpp_packet_set_pos(pkt, p_curdata);

    if (remain == 0) {
        memset(p_dec->prev_tail_data, 0xff, 3);

        if (pkt_length >= 3) {
            p_dec->prev_tail_data[0] = p_end[0];
            p_dec->prev_tail_data[1] = p_end[-1];
            p_dec->prev_tail_data[2] = p_end[-2];
        }
    }

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

/**
 * @brief Every packet is a complete frame.
 * So we don't have to do combine data from different packet.
 *
 * @param p_dec
 * @param pkt
 * @param task
 * @return MPP_RET
 */
MPP_RET avs2d_parse_prepare_fast(Avs2dCtx_t *p_dec, MppPacket *pkt, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *p_curdata = NULL;
    RK_U8 *p_start = NULL;
    RK_U8 *p_end = NULL;
    RK_U32 pkt_length = 0;
    RK_U32 pkt_eos = 0;
    RK_U32 remain = 0;

    AVS2D_PARSE_TRACE("In.");

    pkt_eos = mpp_packet_get_eos(pkt);
    pkt_length = (RK_U32) mpp_packet_get_length(pkt);

    // check eos
    if (pkt_eos && !pkt_length) {
        // EOS packet without data, end of stream
        task->valid = 0;
        task->flags.eos = 1;
        return ret;
    }

    if (!pkt_length) {
        AVS2D_PARSE_TRACE("Input have no stream.");
    }

    p_curdata = p_start = (RK_U8 *) mpp_packet_get_pos(pkt);
    p_end = p_start + pkt_length - 1;

    while (p_curdata < p_end) {
        ret = avs2_split_nalu(p_dec, p_curdata, p_end - p_curdata + 1, 0, &remain);

        if (ret) {
            break;
        } else {
            p_curdata = p_end - remain + 1;
        }

        if (p_dec->new_frame_flag || (p_dec->nal && p_dec->nal->eof == 1)) {
            task->valid = 1;
            break;
        }
    }

    mpp_packet_set_pos(pkt, p_curdata);

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_parse_stream(Avs2dCtx_t *p_dec, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    Avs2dNalu_t *p_nalu = (Avs2dNalu_t *)p_dec->p_header->pbuf;
    AVS2D_PARSE_TRACE("In.");

    while (p_dec->nal_cnt > 0) {
        RK_U32 startcode = p_nalu->header;

        AVS2D_PARSE_TRACE("start code 0x%08x\n", startcode);
        if (!AVS2_IS_SLICE_START_CODE(startcode)) {
            memset(&p_dec->bitctx, 0, sizeof(BitReadCtx_t));
            AVS2D_PARSE_TRACE("bitread ctx, pos %d, length %d\n", p_nalu->start_pos, p_nalu->length);
            mpp_set_bitread_ctx(&p_dec->bitctx, p_nalu->pdata, p_nalu->length);
            mpp_set_bitread_pseudo_code_type(&p_dec->bitctx, PSEUDO_CODE_AVS2);
        }

        switch (startcode) {
        case AVS2_VIDEO_SEQUENCE_START_CODE:
            ret = avs2d_parse_sequence_header(p_dec);
            if (ret == MPP_OK) {
                p_dec->got_vsh = 1;
            }

            if (p_dec->new_seq_flag && !p_dec->frm_mgr.initial_flag) {
                avs2d_dpb_create(p_dec);
            }
            p_dec->got_exh = 0;
            break;
        case AVS2_I_PICTURE_START_CODE:
        case AVS2_PB_PICTURE_START_CODE:
            ret = avs2d_parse_picture_header(p_dec, startcode);
            break;
        case AVS2_EXTENSION_START_CODE:
            ret = parse_extension_header(p_dec, &p_dec->bitctx, &p_dec->exh);
            break;
        case AVS2_USER_DATA_START_CODE:
            break;
        case AVS2_VIDEO_SEQUENCE_END_CODE:
            p_dec->new_seq_flag = 0;
            avs2d_dpb_flush(p_dec);
            break;
        case AVS2_VIDEO_EDIT_CODE:
            p_dec->vec_flag = 0;
            break;
        default:
            if (AVS2_IS_SLICE_START_CODE(startcode)) {
                task->valid = 1;
            }

            break;
        }

        if (p_dec->nal_cnt > 1) {
            if (!AVS2_IS_SLICE_START_CODE(startcode) && p_nalu->length) {
                p_nalu = (Avs2dNalu_t *)(p_nalu->pdata + p_nalu->length);
                AVS2D_PARSE_TRACE("next startcode 0x%08x, eof %d", p_nalu->header, p_nalu->eof);
            } else {
                p_nalu++;
            }
        }

        p_dec->nal_cnt--;
    };

    reset_nalu_buf(p_dec);

    AVS2D_PARSE_TRACE("Out. task->valid = %d", task->valid);
    return ret;
}
