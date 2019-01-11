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

/*
 * @file       h265d_parser.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG "H265D_PARSER"

#include "mpp_bitread.h"
#include "h265d_parser.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "h265d_syntax.h"
#include "mpp_packet_impl.h"
#include "h265d_api.h"

#define START_CODE 0x000001 ///< start_code_prefix_one_3bytes

RK_U32 h265d_debug;
#ifdef dump
FILE *fp = NULL;
#endif
//static RK_U32 start_write = 0, value = 0;

/**
 * Find the end of the current frame in the bitstream.
 * @return the position of the first byte of the next frame, or END_NOT_FOUND
 */
static RK_S32 hevc_find_frame_end(SplitContext_t *sc, const RK_U8 *buf,
                                  int buf_size)
{
    RK_S32 i;

    for (i = 0; i < buf_size; i++) {
        int nut, layer_id;

        sc->state64 = (sc->state64 << 8) | buf[i];

        if (((sc->state64 >> 3 * 8) & 0xFFFFFF) != START_CODE)
            continue;
        nut = (sc->state64 >> (2 * 8 + 1)) & 0x3F;
        layer_id  =  (((sc->state64 >> 2 * 8) & 0x01) << 5) + (((sc->state64 >> 1 * 8) & 0xF8) >> 3);
        //mpp_log("nut = %d layer_id = %d\n",nut,layer_id);
        // Beginning of access unit
        if ((nut >= NAL_VPS && nut <= NAL_AUD) || nut == NAL_SEI_PREFIX ||
            (nut >= 41 && nut <= 44) || (nut >= 48 && nut <= 55)) {
            if (sc->frame_start_found && !layer_id) {
                sc->frame_start_found = 0;
                return i - 5;
            }
        } else if (nut <= NAL_RASL_R ||
                   (nut >= NAL_BLA_W_LP && nut <= NAL_CRA_NUT)) {
            int first_slice_segment_in_pic_flag = buf[i] >> 7;
            //mpp_log("nut = %d first_slice_segment_in_pic_flag %d layer_id = %d \n",nut,
            //    first_slice_segment_in_pic_flag,
            //     layer_id);
            if (first_slice_segment_in_pic_flag && !layer_id) {
                if (!sc->frame_start_found) {
                    sc->frame_start_found = 1;
                } else { // First slice of next frame found
                    sc->frame_start_found = 0;
                    return i - 5;
                }
            }
        }
    }
    return END_NOT_FOUND;
}

static RK_S32 mpp_combine_frame(SplitContext_t *sc, RK_S32 next, const RK_U8 **buf, RK_S32 *buf_size)
{
    if (sc->overread) {
        mpp_log("overread %d, state:%X next:%d index:%d o_index:%d\n",
                sc->overread, sc->state, next, sc->index, sc->overread_index);
        mpp_log("%X %X %X %X\n", (*buf)[0], (*buf)[1], (*buf)[2], (*buf)[3]);
    }

    /* Copy overread bytes from last frame into buffer. */
    for (; sc->overread > 0; sc->overread--) {
        sc->buffer[sc->index++] = sc->buffer[sc->overread_index++];
    }

    /* flush remaining if EOF */
    if (!*buf_size && next == END_NOT_FOUND) {
        next = 0;
    }

    sc->last_index = sc->index;

    /* copy into buffer end return */
    if (next == END_NOT_FOUND) {
        RK_U32 min_size = (*buf_size) + sc->index + MPP_INPUT_BUFFER_PADDING_SIZE;
        void* new_buffer;
        if (min_size > sc->buffer_size) {
            min_size = MPP_MAX(17 * min_size / 16 + 32, min_size);
            new_buffer = mpp_realloc(sc->buffer, RK_U8, min_size);
            if (!new_buffer) {
                sc->buffer_size = 0;
                return MPP_ERR_NOMEM;
            }
            sc->buffer_size = min_size;
            sc->buffer = new_buffer;
        }

        memcpy(&sc->buffer[sc->index], *buf, *buf_size);
        sc->index += *buf_size;

        return -1;
    }

    *buf_size =
        sc->overread_index = sc->index + next;

    /* append to buffer */
    if (sc->index) {
        RK_U32 min_size = next + sc->index + MPP_INPUT_BUFFER_PADDING_SIZE;
        void* new_buffer;
        if (min_size > sc->buffer_size) {
            min_size = MPP_MAX(17 * min_size / 16 + 32, min_size);
            new_buffer = mpp_realloc(sc->buffer, RK_U8, min_size);
            if (!new_buffer) {
                sc->buffer_size = 0;
                return MPP_ERR_NOMEM;
            }
            sc->buffer_size = min_size;
            sc->buffer = new_buffer;
        }

        if (next > -MPP_INPUT_BUFFER_PADDING_SIZE)
            memcpy(&sc->buffer[sc->index], *buf,
                   next + MPP_INPUT_BUFFER_PADDING_SIZE);
        sc->index = 0;
        *buf = sc->buffer;
    }

    /* store overread bytes */
    for (; next < 0; next++) {
        sc->state = (sc->state << 8) | sc->buffer[sc->last_index + next];
        sc->state64 = (sc->state64 << 8) | sc->buffer[sc->last_index + next];
        sc->overread++;
    }

    if (sc->overread) {
        mpp_log("overread %d, state:%X next:%d index:%d o_index:%d\n",
                sc->overread, sc->state, next, sc->index, sc->overread_index);
        mpp_log("%X %X %X %X\n", (*buf)[0], (*buf)[1], (*buf)[2], (*buf)[3]);
    }

    return 0;
}

static RK_S32 h265d_split_init(void **sc)
{
    SplitContext_t *s = NULL;
    if (s == NULL) {
        s = mpp_calloc(SplitContext_t, 1);
        if (s != NULL) {
            *sc = s;
        } else {
            mpp_err("split alloc context fail");
            return MPP_ERR_NOMEM;
        }
    }
    s->buffer = mpp_malloc(RK_U8, MAX_FRAME_SIZE);
    s->buffer_size = MAX_FRAME_SIZE;
    s->fetch_timestamp = 1;
    return MPP_OK;
}

static void mpp_fetch_timestamp(SplitContext_t *s, RK_S32 off)
{
    RK_S32 i;

    s->dts = s->pts = -1;
    s->offset = 0;
    for (i = 0; i < MPP_PARSER_PTS_NB; i++) {
        h265d_dbg(H265D_DBG_TIME, "s->cur_offset %lld s->cur_frame_offset[%d] %lld s->frame_offset %lld s->next_frame_offset %lld",
                  s->cur_offset, i, s->cur_frame_offset[i], s->frame_offset, s->next_frame_offset);
        if ( s->cur_offset + off >= s->cur_frame_offset[i]
             && (s->frame_offset < s->cur_frame_offset[i] ||
                 (!s->frame_offset && !s->next_frame_offset)) // first field/frame
             // check disabled since MPEG-TS does not send complete PES packets
             && /*s->next_frame_offset + off <*/  s->cur_frame_end[i]) {
            s->dts = s->cur_frame_dts[i];
            s->pts = s->cur_frame_pts[i];
            s->offset = s->next_frame_offset - s->cur_frame_offset[i];
            if (s->cur_offset + off < s->cur_frame_end[i])
                break;
        }
    }
}
static RK_S32 h265d_split_frame(void *sc,
                                const RK_U8 **poutbuf, RK_S32 *poutbuf_size,
                                const RK_U8 *buf, RK_S32 buf_size, RK_S64 pts,
                                RK_S64 dts)
{
    RK_S32 next, i;

    SplitContext_t *s = (SplitContext_t*)sc;

    if (s->cur_offset + buf_size !=
        s->cur_frame_end[s->cur_frame_start_index]) { /* skip remainder packets */
        /* add a new packet descriptor */
        i = (s->cur_frame_start_index + 1) & (MPP_PARSER_PTS_NB - 1);
        s->cur_frame_start_index = i;
        s->cur_frame_offset[i] = s->cur_offset;
        s->cur_frame_end[i] = s->cur_offset + buf_size;
        s->cur_frame_pts[i] = pts;
        s->cur_frame_dts[i] = dts;
        h265d_dbg(H265D_DBG_TIME, "s->cur_frame_start_index = %d,cur_frame_offset = %lld,s->cur_frame_end = %lld pts = %lld",
                  s->cur_frame_start_index, s->cur_frame_offset[i], s->cur_frame_end[i], pts);
    }

    if (s->fetch_timestamp) {
        s->fetch_timestamp = 0;
        s->last_pts = s->pts;
        s->last_dts = s->dts;
        mpp_fetch_timestamp(s, 0);
    }

    if (s->eos && !buf_size) {
        *poutbuf      = s->buffer;
        *poutbuf_size = s->index;
        return 0;
    }

    next = hevc_find_frame_end(s, buf, buf_size);
    if (s->eos && buf_size && next == END_NOT_FOUND) {
        next = buf_size;
    }

    if (mpp_combine_frame(s, next, &buf, &buf_size) < 0) {
        *poutbuf      = NULL;
        *poutbuf_size = 0;
        s->cur_offset += buf_size;
        return buf_size;
    }

    *poutbuf      = buf;
    *poutbuf_size = buf_size;

    if (next < 0)
        next = 0;

    if (*poutbuf_size) {
        /* fill the data for the current frame */
        s->frame_offset = s->next_frame_offset;

        /* offset of the next frame */
        s->next_frame_offset = s->cur_offset + next;
        s->fetch_timestamp = 1;
    }

    s->cur_offset += next;
    return next;
}

static RK_S32 h265d_split_reset(void *sc)
{
    RK_U8 *buf = NULL;
    RK_U32 size = 0;
    SplitContext_t *s = (SplitContext_t*)sc;
    if (sc == NULL) {
        return MPP_OK;
    }
    buf = s->buffer;
    size = s->buffer_size;
    memset(s, 0, sizeof(SplitContext_t));
    s->fetch_timestamp = 1;
    s->buffer = buf;
    s->buffer_size = size;
    s->eos = 0;
    return MPP_OK;
}


static RK_S32 h265d_split_deinit(void *sc)
{
    SplitContext_t *s = (SplitContext_t *)sc;
    if (s->buffer) {
        mpp_free(s->buffer);
        s->buffer = NULL;
    }
    if (s) {
        mpp_free(s);
        s = NULL;
    }
    return MPP_OK;
}

static RK_S32 pred_weight_table(HEVCContext *s, BitReadCtx_t *gb)
{
    RK_U32 i = 0;
    RK_U32 j = 0;
    RK_U8  luma_weight_l0_flag[16];
    RK_U8  chroma_weight_l0_flag[16];
    RK_U8  luma_weight_l1_flag[16];
    RK_U8  chroma_weight_l1_flag[16];

    READ_UE(gb, &s->sh.luma_log2_weight_denom);
    if (s->sps->chroma_format_idc != 0) {
        RK_S32 delta = 0;
        READ_SE(gb, &delta);
        s->sh.chroma_log2_weight_denom = mpp_clip(s->sh.luma_log2_weight_denom + delta, 0, 7);
    }

    for (i = 0; i < s->sh.nb_refs[L0]; i++) {
        READ_ONEBIT(gb, &luma_weight_l0_flag[i]);
        if (!luma_weight_l0_flag[i]) {
            s->sh.luma_weight_l0[i] = 1 << s->sh.luma_log2_weight_denom;
            s->sh.luma_offset_l0[i] = 0;
        }
    }

    if (s->sps->chroma_format_idc != 0) { // FIXME: invert "if" and "for"
        for (i = 0; i < s->sh.nb_refs[L0]; i++) {
            READ_ONEBIT(gb, &chroma_weight_l0_flag[i]);
        }
    } else {
        for (i = 0; i < s->sh.nb_refs[L0]; i++)
            chroma_weight_l0_flag[i] = 0;
    }

    for (i = 0; i < s->sh.nb_refs[L0]; i++) {
        if (luma_weight_l0_flag[i]) {
            RK_S32 delta_luma_weight_l0 = 0;
            READ_SE(gb, &delta_luma_weight_l0);
            s->sh.luma_weight_l0[i] = (1 << s->sh.luma_log2_weight_denom) + delta_luma_weight_l0;
            READ_SE(gb, &s->sh.luma_offset_l0[i]);
        }
        if (chroma_weight_l0_flag[i]) {
            for (j = 0; j < 2; j++) {
                RK_S32 delta_chroma_weight_l0 = 0;
                RK_S32 delta_chroma_offset_l0 = 0;
                READ_SE(gb, &delta_chroma_weight_l0);
                READ_SE(gb, &delta_chroma_offset_l0);
                s->sh.chroma_weight_l0[i][j] = (1 << s->sh.chroma_log2_weight_denom) + delta_chroma_weight_l0;
                s->sh.chroma_offset_l0[i][j] = mpp_clip((delta_chroma_offset_l0 - ((128 * s->sh.chroma_weight_l0[i][j])
                                                                                   >> s->sh.chroma_log2_weight_denom) + 128), -128, 127);
            }
        } else {
            s->sh.chroma_weight_l0[i][0] = 1 << s->sh.chroma_log2_weight_denom;
            s->sh.chroma_offset_l0[i][0] = 0;
            s->sh.chroma_weight_l0[i][1] = 1 << s->sh.chroma_log2_weight_denom;
            s->sh.chroma_offset_l0[i][1] = 0;
        }
    }

    if (s->sh.slice_type == B_SLICE) {
        for (i = 0; i < s->sh.nb_refs[L1]; i++) {
            READ_ONEBIT(gb, &luma_weight_l1_flag[i]);
            if (!luma_weight_l1_flag[i]) {
                s->sh.luma_weight_l1[i] = 1 << s->sh.luma_log2_weight_denom;
                s->sh.luma_offset_l1[i] = 0;
            }
        }
        if (s->sps->chroma_format_idc != 0) {
            for (i = 0; i < s->sh.nb_refs[L1]; i++)
                READ_ONEBIT(gb, &chroma_weight_l1_flag[i]);
        } else {
            for (i = 0; i < s->sh.nb_refs[L1]; i++)
                chroma_weight_l1_flag[i] = 0;
        }
        for (i = 0; i < s->sh.nb_refs[L1]; i++) {
            if (luma_weight_l1_flag[i]) {
                RK_S32 delta_luma_weight_l1 = 0;
                READ_UE(gb, &delta_luma_weight_l1);
                s->sh.luma_weight_l1[i] = (1 << s->sh.luma_log2_weight_denom) + delta_luma_weight_l1;
                READ_SE(gb, &s->sh.luma_offset_l1[i]);
            }
            if (chroma_weight_l1_flag[i]) {
                for (j = 0; j < 2; j++) {
                    RK_S32 delta_chroma_weight_l1 = 0;
                    RK_S32 delta_chroma_offset_l1 = 0;
                    READ_SE(gb, &delta_chroma_weight_l1);
                    READ_SE(gb, &delta_chroma_offset_l1);
                    s->sh.chroma_weight_l1[i][j] = (1 << s->sh.chroma_log2_weight_denom) + delta_chroma_weight_l1;
                    s->sh.chroma_offset_l1[i][j] = mpp_clip((delta_chroma_offset_l1 - ((128 * s->sh.chroma_weight_l1[i][j])
                                                                                       >> s->sh.chroma_log2_weight_denom) + 128), -128, 127);
                }
            } else {
                s->sh.chroma_weight_l1[i][0] = 1 << s->sh.chroma_log2_weight_denom;
                s->sh.chroma_offset_l1[i][0] = 0;
                s->sh.chroma_weight_l1[i][1] = 1 << s->sh.chroma_log2_weight_denom;
                s->sh.chroma_offset_l1[i][1] = 0;
            }
        }
    }
    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 decode_lt_rps(HEVCContext *s, LongTermRPS *rps, BitReadCtx_t *gb)
{
    const HEVCSPS *sps = s->sps;
    RK_S32 max_poc_lsb    = 1 << sps->log2_max_poc_lsb;
    RK_S32 prev_delta_msb = 0;
    RK_U32 nb_sps = 0, nb_sh;
    RK_S32 i;

    RK_S32 bit_begin = gb->used_bits;
    s->rps_bit_offset[s->slice_idx] =
        s->rps_bit_offset_st[s->slice_idx];

    rps->nb_refs = 0;
    if (!sps->long_term_ref_pics_present_flag)
        return 0;

    if (sps->num_long_term_ref_pics_sps > 0)
        READ_UE(gb, &nb_sps);

    READ_UE(gb, &nb_sh);

    if (nb_sh + nb_sps > MPP_ARRAY_ELEMS(rps->poc))
        return  MPP_ERR_STREAM;

    rps->nb_refs = nb_sh + nb_sps;

    for (i = 0; i < rps->nb_refs; i++) {
        RK_U8 delta_poc_msb_present;

        if ((RK_U32)i < nb_sps) {
            RK_U8 lt_idx_sps = 0;

            if (sps->num_long_term_ref_pics_sps > 1)
                READ_BITS(gb, mpp_ceil_log2(sps->num_long_term_ref_pics_sps), &lt_idx_sps);

            rps->poc[i]  = sps->lt_ref_pic_poc_lsb_sps[lt_idx_sps];
            rps->used[i] = sps->used_by_curr_pic_lt_sps_flag[lt_idx_sps];
        } else {
            READ_BITS(gb, sps->log2_max_poc_lsb, &rps->poc[i]);
            READ_ONEBIT(gb, &rps->used[i]);
        }

        READ_ONEBIT(gb, &delta_poc_msb_present);
        if (delta_poc_msb_present) {
            RK_S32 delta = 0;

            READ_UE(gb, &delta);

            if (i && (RK_U32)i != nb_sps)
                delta += prev_delta_msb;

            rps->poc[i] += s->poc - delta * max_poc_lsb - s->sh.pic_order_cnt_lsb;
            prev_delta_msb = delta;
        }
    }

    s->rps_bit_offset[s->slice_idx]
    += (gb->used_bits - bit_begin);

    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 set_sps(HEVCContext *s, const HEVCSPS *sps)
{
    RK_U32 num = 0, den = 0;

    s->h265dctx->coded_width         = sps->width;
    s->h265dctx->coded_height        = sps->height;
    s->h265dctx->width               = sps->output_width;
    s->h265dctx->height              = sps->output_height;
    s->h265dctx->pix_fmt             = sps->pix_fmt;
    s->h265dctx->nBitDepth           = sps->bit_depth;
    s->h265dctx->sample_aspect_ratio = sps->vui.sar;

    if (sps->vui.video_signal_type_present_flag)
        s->h265dctx->color_range = sps->vui.video_full_range_flag ? MPPCOL_RANGE_JPEG
                                   : MPPCOL_RANGE_MPEG;
    else
        s->h265dctx->color_range = MPPCOL_RANGE_MPEG;

    if (sps->vui.colour_description_present_flag) {
        s->h265dctx->colorspace      = sps->vui.matrix_coeffs;
    } else {
        s->h265dctx->colorspace      = MPPCOL_SPC_UNSPECIFIED;
    }

    s->sps = sps;
    s->vps = (HEVCVPS*) s->vps_list[s->sps->vps_id];

    if (s->vps->vps_timing_info_present_flag) {
        num = s->vps->vps_num_units_in_tick;
        den = s->vps->vps_time_scale;
    } else if (sps->vui.vui_timing_info_present_flag) {
        num = sps->vui.vui_num_units_in_tick;
        den = sps->vui.vui_time_scale;
    }

    if (num != 0 && den != 0) {
        // s->h265dctx->time_base.num = num;
        // s->h265dctx->time_base.den = den;
        // av_reduce(&s->h265dctx->time_base.num, &s->h265dctx->time_base.den,
        //        num, den, 1 << 30);
    }

    return 0;

}
static RK_S32 compare_sliceheader(SliceHeader *openhevc_sh, SliceHeader *sh)
{

    if (openhevc_sh->pps_id != sh->pps_id) {
        mpp_log(" pps_id diff \n");
        return -1;
    }

    if (openhevc_sh->slice_type != sh->slice_type) {
        mpp_log(" slice_type diff \n");
        return -1;
    }

    if (openhevc_sh->pic_order_cnt_lsb != sh->pic_order_cnt_lsb) {
        mpp_log(" pic_order_cnt_lsb diff \n");
        return -1;
    }

    if (openhevc_sh->first_slice_in_pic_flag != sh->first_slice_in_pic_flag) {
        mpp_log(" first_slice_in_pic_flag diff \n");
        return -1;
    }

    if (openhevc_sh->dependent_slice_segment_flag != sh->dependent_slice_segment_flag) {
        mpp_log(" dependent_slice_segment_flag diff \n");
        return -1;
    }

    if (openhevc_sh->pic_output_flag != sh->pic_output_flag) {
        mpp_log(" pic_output_flag diff \n");
        return -1;
    }

    if (openhevc_sh->colour_plane_id != sh->colour_plane_id) {
        mpp_log(" colour_plane_id diff \n");
        return -1;
    }

    if (openhevc_sh->rpl_modification_flag[0] != sh->rpl_modification_flag[0]) {
        mpp_log(" rpl_modification_flag[0] diff \n");
        return -1;
    }

    if (openhevc_sh->rpl_modification_flag[1] != sh->rpl_modification_flag[1]) {
        mpp_log(" rpl_modification_flag[1] diff \n");
        return -1;
    }

    if (openhevc_sh->no_output_of_prior_pics_flag != sh->no_output_of_prior_pics_flag) {
        mpp_log(" no_output_of_prior_pics_flag diff \n");
        return -1;
    }

    if (openhevc_sh->slice_temporal_mvp_enabled_flag != sh->slice_temporal_mvp_enabled_flag) {
        mpp_log(" slice_temporal_mvp_enabled_flag diff \n");
        return -1;
    }

    if (openhevc_sh->nb_refs[0] != sh->nb_refs[0]) {
        mpp_log(" nb_refs[0] diff \n");
        return -1;
    }

    if (openhevc_sh->nb_refs[1] != sh->nb_refs[1]) {
        mpp_log(" nb_refs[1] diff \n");
        return -1;
    }

    if (openhevc_sh->slice_sample_adaptive_offset_flag[0] !=
        sh->slice_sample_adaptive_offset_flag[0]) {
        mpp_log(" slice_sample_adaptive_offset_flag[0] diff \n");
        return -1;
    }

    if (openhevc_sh->slice_sample_adaptive_offset_flag[1] !=
        sh->slice_sample_adaptive_offset_flag[1]) {
        mpp_log(" slice_sample_adaptive_offset_flag[1] diff \n");
        return -1;
    }

    if (openhevc_sh->slice_sample_adaptive_offset_flag[2] !=
        sh->slice_sample_adaptive_offset_flag[2]) {
        mpp_log(" slice_sample_adaptive_offset_flag[2] diff \n");
        return -1;
    }

    if (openhevc_sh->mvd_l1_zero_flag != sh->mvd_l1_zero_flag) {
        mpp_log(" mvd_l1_zero_flag diff \n");
        return -1;
    }
    if (openhevc_sh->cabac_init_flag != sh->cabac_init_flag) {
        mpp_log(" cabac_init_flag diff \n");
        return -1;
    }

    if (openhevc_sh->disable_deblocking_filter_flag !=
        sh->disable_deblocking_filter_flag) {
        mpp_log(" disable_deblocking_filter_flag diff \n");
        return -1;
    }

    if (openhevc_sh->slice_loop_filter_across_slices_enabled_flag !=
        sh->slice_loop_filter_across_slices_enabled_flag) {
        mpp_log(" slice_loop_filter_across_slices_enable diff \n");
        return -1;
    }

    if (openhevc_sh->collocated_list != sh->collocated_list) {
        mpp_log(" collocated_list diff \n");
        return -1;
    }

    if (openhevc_sh->collocated_ref_idx != sh->collocated_ref_idx) {
        mpp_log(" collocated_ref_idx diff \n");
        return -1;
    }

    if (openhevc_sh->slice_qp_delta != sh->slice_qp_delta) {
        mpp_log(" slice_qp_delta diff \n");
        return -1;
    }

    if (openhevc_sh->slice_cb_qp_offset != sh->slice_cb_qp_offset) {
        mpp_log(" slice_cb_qp_offset diff \n");
        return -1;
    }

    if (openhevc_sh->slice_cr_qp_offset != sh->slice_cr_qp_offset) {
        mpp_log(" slice_cr_qp_offset diff \n");
        return -1;
    }

    if (openhevc_sh->beta_offset != sh->beta_offset) {
        mpp_log(" beta_offset diff \n");
        return -1;
    }

    if (openhevc_sh->tc_offset != sh->tc_offset) {
        mpp_log(" tc_offset diff \n");
        return -1;
    }

    if (openhevc_sh->max_num_merge_cand != sh->max_num_merge_cand) {
        mpp_log(" max_num_merge_cand diff \n");
        return -1;
    }

    if (openhevc_sh->num_entry_point_offsets != sh->num_entry_point_offsets) {
        mpp_log(" num_entry_point_offsets diff \n");
        return -1;
    }

    if (openhevc_sh->slice_qp != sh->slice_qp) {
        mpp_log(" slice_qp diff \n");
        return -1;
    }

    if (openhevc_sh->luma_log2_weight_denom != sh->luma_log2_weight_denom) {
        mpp_log(" luma_log2_weight_denom diff \n");
        return -1;
    }

    if (openhevc_sh->chroma_log2_weight_denom != sh->chroma_log2_weight_denom) {
        mpp_log(" chroma_log2_weight_denom diff \n");
        return -1;
    }

    /* if (openhevc_sh->slice_ctb_addr_rs != sh->slice_ctb_addr_rs) {
         mpp_log(" slice_ctb_addr_rs diff \n");
         return -1;
     }*/
    return 0;
}

static RK_S32 hls_slice_header(HEVCContext *s)
{

    BitReadCtx_t *gb = &s->HEVClc->gb;
    SliceHeader *sh   = &s->sh;
    RK_S32 i, ret;
    RK_S32 value, pps_id;
    RK_S32 bit_begin;

#ifdef JCTVC_M0458_INTERLAYER_RPS_SIG
    int NumILRRefIdx;
#endif

    // Coded parameters

    READ_ONEBIT(gb, &sh->first_slice_in_pic_flag);
    if ((IS_IDR(s) || IS_BLA(s)) && sh->first_slice_in_pic_flag) {
        s->seq_decode = (s->seq_decode + 1) & 0xff;
        s->max_ra     = INT_MAX;
        if (IS_IDR(s))
            mpp_hevc_clear_refs(s);
    }
    if (s->nal_unit_type >= 16 && s->nal_unit_type <= 23)
        READ_ONEBIT(gb, &sh->no_output_of_prior_pics_flag);

    if (IS_IRAP(s) && s->miss_ref_flag && sh->first_slice_in_pic_flag) {
        //     mpp_err("s->nal_unit_type = %d s->poc %d",s->nal_unit_type,s->poc);
        s->max_ra     = INT_MAX;
        s->miss_ref_flag = 0;
    }
    READ_UE(gb, &pps_id);

    if (pps_id >= MAX_PPS_COUNT || !s->pps_list[pps_id]) {
        mpp_err( "PPS id out of range: %d\n", pps_id);
        return  MPP_ERR_STREAM;
    } else {
        sh->pps_id = pps_id;
    }

    if (!sh->first_slice_in_pic_flag &&
        s->pps != (HEVCPPS*)s->pps_list[sh->pps_id]) {
        mpp_err( "PPS changed between slices.\n");
        return  MPP_ERR_STREAM;
    }
    s->pps = (HEVCPPS*)s->pps_list[sh->pps_id];

    if (s->sps != (HEVCSPS*)s->sps_list[s->pps->sps_id]) {
        s->sps = (HEVCSPS*)s->sps_list[s->pps->sps_id];
        mpp_hevc_clear_refs(s);
        ret = set_sps(s, s->sps);
        if (ret < 0)
            return ret;

        s->seq_decode = (s->seq_decode + 1) & 0xff;
        s->max_ra     = INT_MAX;
    }

    // s->h265dctx->profile = s->sps->ptl.general_ptl.profile_idc;
    // s->h265dctx->level   = s->sps->ptl.general_ptl.level_idc;

    sh->dependent_slice_segment_flag = 0;
    if (!sh->first_slice_in_pic_flag) {
        RK_S32 slice_address_length;

        if (s->pps->dependent_slice_segments_enabled_flag)
            READ_ONEBIT(gb, &sh->dependent_slice_segment_flag);

        slice_address_length = mpp_ceil_log2(s->sps->ctb_width *
                                             s->sps->ctb_height);

        READ_BITS(gb, slice_address_length, &sh->slice_segment_addr);

        if (sh->slice_segment_addr >= (RK_U32)(s->sps->ctb_width * s->sps->ctb_height)) {
            mpp_err(
                "Invalid slice segment address: %u.\n",
                sh->slice_segment_addr);
            return  MPP_ERR_STREAM;
        }

        if (!sh->dependent_slice_segment_flag) {
            sh->slice_addr = sh->slice_segment_addr;
            s->slice_idx++;
        }
    } else {
        sh->slice_segment_addr = sh->slice_addr = 0;
        s->slice_idx           = 0;
        s->slice_initialized   = 0;
    }

    if (!sh->dependent_slice_segment_flag) {
        s->slice_initialized = 0;

        for (i = 0; i < s->pps->num_extra_slice_header_bits; i++)
            SKIP_BITS(gb, 1);  // slice_reserved_undetermined_flag[]

        READ_UE(gb, &sh->slice_type);
        if (!(sh->slice_type == I_SLICE ||
              sh->slice_type == P_SLICE ||
              sh->slice_type == B_SLICE)) {
            mpp_err( "Unknown slice type: %d.\n",
                     sh->slice_type);
            return  MPP_ERR_STREAM;
        }
        if (!s->decoder_id && IS_IRAP(s) && sh->slice_type != I_SLICE) {
            mpp_err( "Inter slices in an IRAP frame.\n");
            return  MPP_ERR_STREAM;
        }

        if (s->pps->output_flag_present_flag)
            READ_ONEBIT(gb, &sh->pic_output_flag);

        if (s->sps->separate_colour_plane_flag)
            READ_BITS(gb, 2, &sh->colour_plane_id );

        if (!IS_IDR(s)) {
            int  poc;

            READ_BITS(gb, s->sps->log2_max_poc_lsb, &sh->pic_order_cnt_lsb);
            poc = mpp_hevc_compute_poc(s, sh->pic_order_cnt_lsb);
            if (!sh->first_slice_in_pic_flag && poc != s->poc) {
                mpp_log("Ignoring POC change between slices: %d -> %d\n", s->poc, poc);
#if 0
                if (s->h265dctx->err_recognition & AV_EF_EXPLODE)
                    return  MPP_ERR_STREAM;
#endif
                poc = s->poc;
            }
            s->poc = poc;

            READ_ONEBIT(gb, &sh->short_term_ref_pic_set_sps_flag);

            bit_begin = gb->used_bits;

            if (!sh->short_term_ref_pic_set_sps_flag) {

                ret = mpp_hevc_decode_short_term_rps(s, &sh->slice_rps, s->sps, 1);
                if (ret < 0)
                    return ret;

                sh->short_term_rps = &sh->slice_rps;
            } else {
                RK_S32 numbits, rps_idx;

                if (!s->sps->nb_st_rps) {
                    mpp_err( "No ref lists in the SPS.\n");
                    return  MPP_ERR_STREAM;
                }

                numbits = mpp_ceil_log2(s->sps->nb_st_rps);
                rps_idx = 0;
                if (numbits > 0)
                    READ_BITS(gb, numbits, &rps_idx);

                sh->short_term_rps = &s->sps->st_rps[rps_idx];
            }

            s->rps_bit_offset_st[s->slice_idx] = gb->used_bits - bit_begin;

            sh->short_term_ref_pic_set_size = s->rps_bit_offset_st[s->slice_idx];

            ret = decode_lt_rps(s, &sh->long_term_rps, gb);
            if (ret < 0) {
                mpp_log("Invalid long term RPS.\n");
                // if (s->h265dctx->err_recognition & AV_EF_EXPLODE)
                //   return  MPP_ERR_STREAM;
            }

            if (s->sps->sps_temporal_mvp_enabled_flag)
                READ_ONEBIT(gb, &sh->slice_temporal_mvp_enabled_flag);
            else
                sh->slice_temporal_mvp_enabled_flag = 0;
        } else {
            s->sh.short_term_rps = NULL;
            s->poc               = 0;
        }

        /* 8.3.1 */
        if (s->temporal_id == 0 &&
            s->nal_unit_type != NAL_TRAIL_N &&
            s->nal_unit_type != NAL_TSA_N   &&
            s->nal_unit_type != NAL_STSA_N  &&
            s->nal_unit_type != NAL_RADL_N  &&
            s->nal_unit_type != NAL_RADL_R  &&
            s->nal_unit_type != NAL_RASL_N  &&
            s->nal_unit_type != NAL_RASL_R)
            s->pocTid0 = s->poc;

        if (s->sps->sao_enabled) {
            READ_ONEBIT(gb, &sh->slice_sample_adaptive_offset_flag[0]);
            READ_ONEBIT(gb, &sh->slice_sample_adaptive_offset_flag[1]);
            sh->slice_sample_adaptive_offset_flag[2] =
                sh->slice_sample_adaptive_offset_flag[1];
        } else {
            sh->slice_sample_adaptive_offset_flag[0] = 0;
            sh->slice_sample_adaptive_offset_flag[1] = 0;
            sh->slice_sample_adaptive_offset_flag[2] = 0;
        }

        sh->nb_refs[L0] = sh->nb_refs[L1] = 0;
        if (sh->slice_type == P_SLICE || sh->slice_type == B_SLICE) {
            int nb_refs;

            sh->nb_refs[L0] = s->pps->num_ref_idx_l0_default_active;
            if (sh->slice_type == B_SLICE)
                sh->nb_refs[L1] = s->pps->num_ref_idx_l1_default_active;

            READ_ONEBIT(gb, &value);

            if (value) { // num_ref_idx_active_override_flag
                READ_UE(gb, &sh->nb_refs[L0]);
                sh->nb_refs[L0] += 1;
                if (sh->slice_type == B_SLICE) {
                    READ_UE(gb, &sh->nb_refs[L1]);
                    sh->nb_refs[L1] += 1;
                }
            }
            if (sh->nb_refs[L0] > MAX_REFS || sh->nb_refs[L1] > MAX_REFS) {
                mpp_err( "Too many refs: %d/%d.\n",
                         sh->nb_refs[L0], sh->nb_refs[L1]);
                return  MPP_ERR_STREAM;
            }

            sh->rpl_modification_flag[0] = 0;
            sh->rpl_modification_flag[1] = 0;
            nb_refs = mpp_hevc_frame_nb_refs(s);
            if (!nb_refs) {
                mpp_err( "Zero refs for a frame with P or B slices.\n");
                return  MPP_ERR_STREAM;
            }

            if (s->pps->lists_modification_present_flag && nb_refs > 1) {
                READ_ONEBIT(gb, &sh->rpl_modification_flag[0]);
                if (sh->rpl_modification_flag[0]) {
                    for (i = 0; (RK_U32)i < sh->nb_refs[L0]; i++)
                        READ_BITS(gb, mpp_ceil_log2(nb_refs), &sh->list_entry_lx[0][i]);
                }

                if (sh->slice_type == B_SLICE) {
                    READ_ONEBIT(gb, &sh->rpl_modification_flag[1]);
                    if (sh->rpl_modification_flag[1] == 1)
                        for (i = 0; (RK_U32)i < sh->nb_refs[L1]; i++)
                            READ_BITS(gb, mpp_ceil_log2(nb_refs), &sh->list_entry_lx[1][i]);
                }
            }

            if (sh->slice_type == B_SLICE)
                READ_ONEBIT(gb, &sh->mvd_l1_zero_flag);

            if (s->pps->cabac_init_present_flag)
                READ_ONEBIT(gb, &sh->cabac_init_flag);
            else
                sh->cabac_init_flag = 0;

            sh->collocated_ref_idx = 0;
            if (sh->slice_temporal_mvp_enabled_flag) {
                sh->collocated_list = L0;
                if (sh->slice_type == B_SLICE) {
                    READ_ONEBIT(gb, &value);
                    sh->collocated_list = !value;
                }

                if (sh->nb_refs[sh->collocated_list] > 1) {
                    READ_UE(gb, &sh->collocated_ref_idx);
                    if (sh->collocated_ref_idx >= sh->nb_refs[sh->collocated_list]) {
                        mpp_err(
                            "Invalid collocated_ref_idx: %d.\n",
                            sh->collocated_ref_idx);
                        return  MPP_ERR_STREAM;
                    }
                }
            }

            if ((s->pps->weighted_pred_flag   && sh->slice_type == P_SLICE) ||
                (s->pps->weighted_bipred_flag && sh->slice_type == B_SLICE)) {
                pred_weight_table(s, gb);
            }

            READ_UE(gb, &value);
            sh->max_num_merge_cand = 5 - value;
            if (sh->max_num_merge_cand < 1 || sh->max_num_merge_cand > 5) {
                mpp_err(
                    "Invalid number of merging MVP candidates: %d.\n",
                    sh->max_num_merge_cand);
                return  MPP_ERR_STREAM;
            }
        }
        READ_SE(gb, &sh->slice_qp_delta );
        if (s->pps->pic_slice_level_chroma_qp_offsets_present_flag) {
            READ_SE(gb, &sh->slice_cb_qp_offset);
            READ_SE(gb, &sh->slice_cr_qp_offset);
        } else {
            sh->slice_cb_qp_offset = 0;
            sh->slice_cr_qp_offset = 0;
        }

        if (s->pps->deblocking_filter_control_present_flag) {
            int deblocking_filter_override_flag = 0;

            if (s->pps->deblocking_filter_override_enabled_flag)
                READ_ONEBIT(gb, & deblocking_filter_override_flag);

            if (deblocking_filter_override_flag) {
                READ_ONEBIT(gb, &sh->disable_deblocking_filter_flag);
                if (!sh->disable_deblocking_filter_flag) {
                    READ_SE(gb, &sh->beta_offset);
                    sh->beta_offset = sh->beta_offset * 2;
                    READ_SE(gb, &sh->tc_offset);
                    sh->tc_offset = sh->tc_offset * 2;
                }
            } else {
                sh->disable_deblocking_filter_flag = s->pps->disable_dbf;
                sh->beta_offset                    = s->pps->beta_offset;
                sh->tc_offset                      = s->pps->tc_offset;
            }
        } else {
            sh->disable_deblocking_filter_flag = 0;
            sh->beta_offset                    = 0;
            sh->tc_offset                      = 0;
        }

        if (s->pps->seq_loop_filter_across_slices_enabled_flag &&
            (sh->slice_sample_adaptive_offset_flag[0] ||
             sh->slice_sample_adaptive_offset_flag[1] ||
             !sh->disable_deblocking_filter_flag)) {
            READ_ONEBIT(gb, &sh->slice_loop_filter_across_slices_enabled_flag);
        } else {
            sh->slice_loop_filter_across_slices_enabled_flag = s->pps->seq_loop_filter_across_slices_enabled_flag;
        }
    } else if (!s->slice_initialized) {
        mpp_err( "Independent slice segment missing.\n");
        return  MPP_ERR_STREAM;
    }

    sh->num_entry_point_offsets = 0;
    if (s->pps->tiles_enabled_flag || s->pps->entropy_coding_sync_enabled_flag) {
        READ_UE(gb, &sh->num_entry_point_offsets);
        if (s->pps->entropy_coding_sync_enabled_flag) {
            if (sh->num_entry_point_offsets > s->sps->ctb_height || sh->num_entry_point_offsets < 0) {
                mpp_err("The number of entries %d is higher than the number of CTB rows %d \n",
                        sh->num_entry_point_offsets,
                        s->sps->ctb_height);
                return  MPP_ERR_STREAM;
            }
        } else {
            if (sh->num_entry_point_offsets > s->sps->ctb_height * s->sps->ctb_width || sh->num_entry_point_offsets < 0) {
                mpp_err("The number of entries %d is higher than the number of CTBs %d \n",
                        sh->num_entry_point_offsets,
                        s->sps->ctb_height * s->sps->ctb_width);
                return  MPP_ERR_STREAM;
            }
        }
    }
#if 0
    if (s->pps->slice_header_extension_present_flag) {
        RK_U32 length = 0;
        READ_UE(gb, &length);
        for (i = 0; (RK_U32)i < length; i++)
            SKIP_BITS(gb, 8);  // slice_header_extension_data_byte
    }
#endif
    // Inferred parameters
    sh->slice_qp = 26U + s->pps->pic_init_qp_minus26 + sh->slice_qp_delta;
    if (sh->slice_qp > 51 ||
        sh->slice_qp < -s->sps->qp_bd_offset) {
        mpp_err("The slice_qp %d is outside the valid range "
                "[%d, 51].\n",
                sh->slice_qp,
                -s->sps->qp_bd_offset);
        return  MPP_ERR_STREAM;
    }
    if (s->h265dctx->compare_info != NULL && sh->first_slice_in_pic_flag) {
        CurrentFameInf_t *info = (CurrentFameInf_t *)s->h265dctx->compare_info;
        SliceHeader *openhevc_sh = (SliceHeader *)&info->sh;
        h265d_dbg(H265D_DBG_FUNCTION, "compare_sliceheader in");
        if (compare_sliceheader(openhevc_sh, &s->sh) < 0) {
            mpp_log("compare sliceHeader with openhevc diff\n");
            mpp_assert(0);
        }
        h265d_dbg(H265D_DBG_FUNCTION, "compare_sliceheader ok");
    }

    sh->slice_ctb_addr_rs = sh->slice_segment_addr;

    if (!s->sh.slice_ctb_addr_rs && s->sh.dependent_slice_segment_flag) {
        mpp_err("Impossible slice segment.\n");
        return  MPP_ERR_STREAM;
    }

    s->slice_initialized = 1;

    return 0;
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

/**
 * @return AV MPP_ERR_STREAM if the packet is not a valid NAL unit,
 * 0 if the unit should be skipped, 1 otherwise
 */
static RK_S32 hls_nal_unit(HEVCContext *s)
{
    BitReadCtx_t*gb = &s->HEVClc->gb;
    RK_S32 value = 0;

    READ_ONEBIT(gb, &value); /*this bit should be zero*/

    READ_BITS(gb, 6, &s->nal_unit_type);

    READ_BITS(gb, 6, &s->nuh_layer_id);

    READ_BITS(gb, 3, &s->temporal_id);

    s->temporal_id = s->temporal_id - 1;

    if (s->temporal_id < 0)
        return  MPP_ERR_STREAM;

    h265d_dbg(H265D_DBG_GLOBAL,
              "nal_unit_type: %d, nuh_layer_id: %d temporal_id: %d\n",
              s->nal_unit_type, s->nuh_layer_id, s->temporal_id);

    return (s->nuh_layer_id);
__BITREAD_ERR:
    return  MPP_ERR_STREAM;
}

static RK_S32 mpp_hevc_output_frame(void *ctx, int flush)
{

    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = (HEVCContext *)h265dctx->priv_data;

    do {
        RK_S32 nb_output = 0;
        RK_S32 min_poc   = INT_MAX;
        RK_S32 min_idx = 0;
        RK_U32 i;

        for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
            HEVCFrame *frame = &s->DPB[i];
            if ((frame->flags & HEVC_FRAME_FLAG_OUTPUT) &&
                frame->sequence == s->seq_output) {
                nb_output++;
                if (frame->poc < min_poc) {
                    min_poc = frame->poc;
                    min_idx = i;
                }
            }
        }

        /* wait for more frames before output */
        if (!flush && s->seq_output == s->seq_decode && s->sps &&
            nb_output <= s->sps->temporal_layer[s->sps->max_sub_layers - 1].num_reorder_pics)
            return 0;

        if (nb_output) {
            HEVCFrame *frame = &s->DPB[min_idx];

            frame->flags &= ~(HEVC_FRAME_FLAG_OUTPUT);
            s->output_frame_idx = min_idx;

            mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(s->slots, frame->slot_index, QUEUE_DISPLAY);

            h265d_dbg(H265D_DBG_REF,
                      "Output frame with POC %d frame->slot_index = %d\n", frame->poc, frame->slot_index);


            return 1;
        }

        if (s->seq_output != s->seq_decode)
            s->seq_output = (s->seq_output + 1) & 0xff;
        else
            break;
    } while (1);

    return 0;
}

static RK_S32 hevc_frame_start(HEVCContext *s)
{
    int ret;

    if (s->ref) {
        mpp_log_f("found two frame in one packet do nothing!\n");
        return 0;
    }

    s->is_decoded        = 0;
    s->first_nal_type    = s->nal_unit_type;
    s->miss_ref_flag = 0;

    ret = mpp_hevc_frame_rps(s);
    if (ret < 0) {
        mpp_err("Error constructing the frame RPS.\n");
        goto fail;
    }

    ret = mpp_hevc_set_new_ref(s, &s->frame, s->poc);
    if (ret < 0)
        goto fail;

    if (!s->h265dctx->disable_error && s->miss_ref_flag) {
        if (!IS_IRAP(s)) {
            mpp_frame_set_errinfo(s->frame, MPP_FRAME_ERR_UNKNOW);
            s->ref->error_flag = 1;
        } else {
            /*when found current I frame have miss refer
              may be stream have error so first set current frame
              no output and flush other frame output from dpb
              then set current frame can as output
            */
            HEVCFrame *frame = NULL;
            RK_U32 i = 0;
            for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
                frame = &s->DPB[i];
                if (frame->poc == s->poc ) {
                    frame->flags &= ~(HEVC_FRAME_FLAG_OUTPUT);
                    break;
                } else {
                    frame = NULL;
                }
            }
            do {
                ret = mpp_hevc_output_frame(s->h265dctx, 1);
            } while (ret);
            if (frame) {
                frame->flags |= HEVC_FRAME_FLAG_OUTPUT;
            }
        }
    }

    mpp_buf_slot_set_prop(s->slots, s->ref->slot_index, SLOT_FRAME, s->ref->frame);

    return 0;

fail:
    s->ref = NULL;
    return ret;
}

static RK_S32 parser_nal_unit(HEVCContext *s, const RK_U8 *nal, int length)
{

    HEVCLocalContext *lc = s->HEVClc;
    BitReadCtx_t *gb    = &lc->gb;
    RK_S32 ret;
    mpp_set_bitread_ctx(gb, (RK_U8*)nal, length);
    mpp_set_pre_detection(gb);
    ret = hls_nal_unit(s);
    if (ret < 0) {
        mpp_err("Invalid NAL unit %d, skipping.\n",
                s->nal_unit_type);
        goto fail;
    } else if (ret != (s->decoder_id) && s->nal_unit_type != NAL_VPS)
        return 0;

    if (s->temporal_id > s->temporal_layer_id)
        return 0;

    s->nuh_layer_id = ret;
    h265d_dbg(H265D_DBG_GLOBAL, "s->nal_unit_type = %d,len = %d \n", s->nal_unit_type, length);

    switch (s->nal_unit_type) {
    case NAL_VPS:
        ret = mpp_hevc_decode_nal_vps(s);
        if (ret < 0) {
            mpp_err("mpp_hevc_decode_nal_vps error ret = %d", ret);
            goto fail;
        }
        break;
    case NAL_SPS:
        ret = mpp_hevc_decode_nal_sps(s);
        if (ret < 0) {
            mpp_err("mpp_hevc_decode_nal_sps error ret = %d", ret);
            goto fail;
        }
        break;
    case NAL_PPS:
        ret = mpp_hevc_decode_nal_pps(s);
        if (ret < 0) {
            mpp_err("mpp_hevc_decode_nal_pps error ret = %d", ret);
            goto fail;
        }
        break;
    case NAL_SEI_PREFIX:
    case NAL_SEI_SUFFIX:
        ret = mpp_hevc_decode_nal_sei(s);
        if (ret < 0) {
            mpp_err("mpp_hevc_decode_nal_sei error ret = %d", ret);
            //goto fail;
        }
        break;
    case NAL_TRAIL_R:
    case NAL_TRAIL_N:
    case NAL_TSA_N:
    case NAL_TSA_R:
    case NAL_STSA_N:
    case NAL_STSA_R:
    case NAL_BLA_W_LP:
    case NAL_BLA_W_RADL:
    case NAL_BLA_N_LP:
    case NAL_IDR_W_RADL:
    case NAL_IDR_N_LP:
    case NAL_CRA_NUT:
    case NAL_RADL_N:
    case NAL_RADL_R:
    case NAL_RASL_N:
    case NAL_RASL_R:
        if (s->task == NULL) {
            s->extra_has_frame = 1;
            break;
        }
        h265d_dbg(H265D_DBG_FUNCTION, "hls_slice_header in");
        ret = hls_slice_header(s);
        h265d_dbg(H265D_DBG_FUNCTION, "hls_slice_header out");
        if (ret < 0) {
            mpp_err("hls_slice_header error ret = %d", ret);
            return ret;
        }

        if (s->max_ra == INT_MAX) {
            if (s->nal_unit_type == NAL_CRA_NUT || IS_BLA(s)) {
                s->max_ra = s->poc;
            } else {
                if (IS_IDR(s))
                    s->max_ra = INT_MIN;
            }
        }

        if ((s->nal_unit_type == NAL_RASL_R || s->nal_unit_type == NAL_RASL_N) &&
            s->poc <= s->max_ra) {
            s->is_decoded = 0;
            break;
        } else if (!s->h265dctx->disable_error &&
                   (s->poc < s->max_ra) && !IS_IRAP(s)) { //when seek to I slice skip the stream small then I slic poc
            s->is_decoded = 0;
            break;
        } else {
            if (s->nal_unit_type == NAL_RASL_R && s->poc > s->max_ra)
                s->max_ra = INT_MIN;
        }

        if (s->sh.first_slice_in_pic_flag) {
            ret = hevc_frame_start(s);
            if (ret < 0) {
                mpp_err("hevc_frame_start = %d", ret);
                return ret;
            }
        } else if (!s->ref) {
            mpp_err("First slice in a frame missing.\n");
            goto fail;
        }

        if (s->nal_unit_type != s->first_nal_type) {
            mpp_err("Non-matching NAL types of the VCL NALUs: %d %d\n",
                    s->first_nal_type, s->nal_unit_type);
            goto fail;
        }

        if (!s->sh.dependent_slice_segment_flag &&
            s->sh.slice_type != I_SLICE) {
            // ret = mpp_hevc_slice_rpl(s);
            if (ret < 0) {
                mpp_err("Error constructing the reference lists for the current slice.\n");
                goto fail;
            }
            //    rk_get_ref_info(s);
        }


        s->is_decoded = 1;

        break;
    case NAL_EOS_NUT:
    case NAL_EOB_NUT:
        s->seq_decode = (s->seq_decode + 1) & 0xff;
        s->max_ra     = INT_MAX;
        break;
    case NAL_AUD:
    case NAL_FD_NUT:
        break;
    default:
        mpp_log("Skipping NAL unit %d\n", s->nal_unit_type);
    }

    return 0;
fail:

    return ret;
}


typedef union {
    RK_U32  u32;
    RK_U16  u16[2];
    RK_U8   u8 [4];
    float   f32;
} mpp_alias32;

#define MPP_FAST_UNALIGNED 1


#ifndef MPP_RN32A
#define MPP_RN32A(p) (((const mpp_alias32*)(p))->u32)
#endif
RK_S32 mpp_hevc_extract_rbsp(HEVCContext *s, const RK_U8 *src, int length,
                             HEVCNAL *nal)
{
    RK_S32 i;

    s->skipped_bytes = 0;

#define STARTCODE_TEST                                              \
    if (i + 2 < length && src[i + 1] == 0 && src[i + 2] < 3) {      \
            /* startcode, so we must be past the end */             \
        length = i;                                                 \
        break;                                                      \
    }

#if MPP_FAST_UNALIGNED
#define FIND_FIRST_ZERO                                             \
    if (i > 0 && !src[i])                                           \
        i--;                                                        \
    while (src[i])                                                  \
        i++

    for (i = 0; i + 1 < length; i += 5) {
        if (!((~MPP_RN32A(src + i) &
               (MPP_RN32A(src + i) - 0x01000101U)) &
              0x80008080U))
            continue;

        FIND_FIRST_ZERO;

        STARTCODE_TEST;
        i -= 3;
    }
#else
    for (i = 0; i + 1 < length; i += 2) {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }
#endif

    if (length + MPP_INPUT_BUFFER_PADDING_SIZE > nal->rbsp_buffer_size) {
        RK_S32 min_size = length + MPP_INPUT_BUFFER_PADDING_SIZE;
        mpp_free(nal->rbsp_buffer);
        nal->rbsp_buffer = NULL;
        min_size = MPP_MAX(17 * min_size / 16 + 32, min_size);
        nal->rbsp_buffer = mpp_malloc(RK_U8, min_size);
        if (nal->rbsp_buffer == NULL) {
            min_size = 0;
        }
        nal->rbsp_buffer_size = min_size;
    }

    memcpy(nal->rbsp_buffer, src, length);
    nal->data = nal->rbsp_buffer;
    nal->size = length;

    memset(nal->rbsp_buffer + length, 0, MPP_INPUT_BUFFER_PADDING_SIZE);
    return length;
}

static RK_S32 split_nal_units(HEVCContext *s, RK_U8 *buf, RK_U32 length)
{
    RK_S32 i, consumed;
    MPP_RET ret = MPP_OK;
    s->nb_nals = 0;
    while (length >= 4) {
        HEVCNAL *nal;
        RK_S32 extract_length = 0;

        if (s->is_nalff) {
            for (i = 0; i < s->nal_length_size; i++)
                extract_length = (extract_length << 8) | buf[i];
            buf    += s->nal_length_size;
            length -= s->nal_length_size;

            if ((RK_U32)extract_length > length) {
                mpp_err( "Invalid NAL unit size.\n");
                ret =  MPP_ERR_STREAM;
                goto fail;
            }
        } else {
            /* search start code */
            if (buf[2] == 0) {
                length--;
                buf++;
                continue;
            }
            if (buf[0] != 0 || buf[1] != 0 || buf[2] != 1) {
                RK_U32 state = (RK_U32) - 1;
                int has_nal = 0;
                for (i = 0; i < (RK_S32)length; i++) {
                    state = (state << 8) | buf[i];
                    if (((state >> 8) & 0xFFFFFF) == START_CODE) {
                        has_nal = 1;
                        i = i - 3;
                        break;
                    }
                }

                if (has_nal) {
                    length -= i;
                    buf += i;
                    continue;
                }
                mpp_err( "No start code is found.\n");
                ret =  MPP_ERR_STREAM;
                goto fail;
            }

            buf           += 3;
            length        -= 3;
        }

        if (!s->is_nalff)
            extract_length = length;

        if (!extract_length) {
            return MPP_OK;
        }
        if (s->nals_allocated < 1) {
            RK_S32 new_size = s->nals_allocated + 10;
            HEVCNAL *tmp = mpp_malloc(HEVCNAL, new_size);
            memset((void*)tmp, 0, new_size * sizeof(HEVCNAL));
            s->nals_allocated = new_size;
            s->nals = tmp;
        }
        if (s->nals_allocated < s->nb_nals + 1) {
            int new_size = s->nals_allocated + 10;
            HEVCNAL *tmp = mpp_malloc(HEVCNAL, new_size);
            memset((void*)tmp, 0, new_size * sizeof(HEVCNAL));
            if (!tmp) {
                mpp_err("return enomm new_size %d", new_size);
                ret = MPP_ERR_NOMEM;
                goto fail;
            }
            memcpy((void*)tmp, (void*)s->nals, (new_size - 10)*sizeof(HEVCNAL));
            mpp_free(s->nals);
            s->nals = NULL;
            s->nals = tmp;
            memset(s->nals + s->nals_allocated, 0,
                   (new_size - s->nals_allocated) * sizeof(*tmp));
            s->nals_allocated = new_size;
        }
        nal = &s->nals[s->nb_nals];

        consumed = mpp_hevc_extract_rbsp(s, buf, extract_length, nal);

        s->nb_nals++;

        if (consumed <= 0) {
            ret = MPP_ERR_STREAM;
            goto fail;
        }

        mpp_set_bitread_ctx(&s->HEVClc->gb, (RK_U8 *)nal->data, nal->size);
        mpp_set_pre_detection(&s->HEVClc->gb);
        hls_nal_unit(s);

        if (s->nal_unit_type < NAL_VPS) {

            if (nal->size != consumed)
                h265d_dbg(H265D_DBG_GLOBAL, "tag_stream: nal.size=%d, consumed=%d\n", nal->size, consumed);

        }

        /* if (s->nal_unit_type == NAL_EOB_NUT ||
             s->nal_unit_type == NAL_EOS_NUT)
             s->eos = 1;*/

        buf    += consumed;
        length -= consumed;
    }
fail:
    return ret;

}
static RK_S32 parser_nal_units(HEVCContext *s)
{
    /* parse the NAL units */
    RK_S32 i, ret = 0;
    for (i = 0; i < s->nb_nals; i++) {
        ret = parser_nal_unit(s, s->nals[i].data, s->nals[i].size);
        if (ret < 0) {
            mpp_err("Error parsing NAL unit #%d,error ret = 0xd.\n", i, ret);
            goto fail;
        }
    }
fail:
    return ret;
}

static RK_U16 U16_AT(const RK_U8 *ptr)
{
    return ptr[0] << 8 | ptr[1];
}

static RK_S32 hevc_parser_extradata(HEVCContext *s)
{
    H265dContext_t *h265dctx = s->h265dctx;
    RK_S32 ret = MPP_SUCCESS;
    if (h265dctx->extradata_size > 3 &&
        (h265dctx->extradata[0] || h265dctx->extradata[1] ||
         h265dctx->extradata[2] > 1)) {
        /* It seems the extradata is encoded as hvcC format.
         * Temporarily, we support configurationVersion==0 until 14496-15 3rd
         * is finalized. When finalized, configurationVersion will be 1 and we
         * can recognize hvcC by checking if h265dctx->extradata[0]==1 or not. */
        const RK_U8 *ptr = (const RK_U8 *)h265dctx->extradata;
        RK_U32 size = h265dctx->extradata_size;
        RK_U32 numofArrays = 0, numofNals = 0;
        RK_U32 j = 0, i = 0;
        if (size < 7) {
            return MPP_NOK;
        }

        mpp_log("extradata is encoded as hvcC format");
        s->is_nalff = 1;
        s->nal_length_size = 1 + (ptr[14 + 7] & 3);
        ptr += 22;
        size -= 22;
        numofArrays = (char)ptr[0];
        ptr += 1;
        size -= 1;
        for (i = 0; i < numofArrays; i++) {
            ptr += 1;
            size -= 1;
            // Num of nals
            numofNals = U16_AT(ptr);
            ptr += 2;
            size -= 2;

            for (j = 0; j < numofNals; j++) {
                RK_U32 length = 0;
                if (size < 2) {
                    return MPP_NOK;
                }

                length = U16_AT(ptr);

                ptr += 2;
                size -= 2;
                if (size < length) {
                    return MPP_NOK;
                }
                parser_nal_unit(s, ptr, length);
                ptr += length;
                size -= length;
            }
        }
    } else {
        s->is_nalff = 0;
        ret = split_nal_units(s, h265dctx->extradata, h265dctx->extradata_size);
        if (ret < 0)
            return ret;
        ret = parser_nal_units(s);
        if (ret < 0)
            return ret;
    }
    return ret;
}

MPP_RET h265d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{

    MPP_RET ret = MPP_OK;
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = (HEVCContext *)h265dctx->priv_data;
    SplitContext_t *sc = (SplitContext_t*)h265dctx->split_cxt;
    RK_S64 pts = -1, dts = -1;
    RK_U8 *buf = NULL;
    void *pos = NULL;
    RK_S32 length = 0;

    task->valid = 0;
    s->eos = mpp_packet_get_eos(pkt);
    if (sc != NULL) {
        sc->eos = s->eos;
    }
    buf = (RK_U8 *)mpp_packet_get_pos(pkt);
    pts = mpp_packet_get_pts(pkt);
    dts = mpp_packet_get_dts(pkt);
    h265d_dbg(H265D_DBG_TIME, "prepare get pts %lld", pts);
    length = (RK_S32)mpp_packet_get_length(pkt);

    if (mpp_packet_get_flag(pkt)& MPP_PACKET_FLAG_EXTRA_DATA) {

        h265dctx->extradata_size = length;
        h265dctx->extradata = buf;
        s->extra_has_frame = 0;
        s->task = NULL;
        hevc_parser_extradata(s);
        if (!s->extra_has_frame) {
            pos = buf + length;
            mpp_packet_set_pos(pkt, pos);
            return MPP_OK;
        }
    }

    if (h265dctx->need_split && !s->is_nalff) {

        RK_S32 consume = 0;
        RK_U8 *split_out_buf = NULL;
        RK_S32 split_size = 0;

        consume = h265d_split_frame(h265dctx->split_cxt, (const RK_U8**)&split_out_buf, &split_size,
                                    (const RK_U8*)buf, length, pts, dts);
        pos = buf + consume;
        mpp_packet_set_pos(pkt, pos);
        if (split_size) {
            buf = split_out_buf;
            length = split_size;
            s->checksum_buf = buf;  //check with openhevc
            s->checksum_buf_size = split_size;
            h265d_dbg(H265D_DBG_TIME, "split frame get pts %lld", sc->pts);
            s->pts = sc->pts;
            s->eos = (s->eos && (mpp_packet_get_length(pkt) < 4)) ? 1 : 0;
        } else {
            return MPP_FAIL_SPLIT_FRAME;
        }
    } else {
        pos = buf + length;
        s->pts = pts;
        mpp_packet_set_pos(pkt, pos);
        if (s->eos && !length) {
            task->valid = 0;
            task->flags.eos = 1;
            h265d_flush(ctx);
            return ret;
        }
    }
#ifdef dump
    if (s->nb_frame < 10 && fp != NULL) {
        fwrite(buf, 1, length, fp);
    }
#endif
    ret = (MPP_RET)split_nal_units(s, buf, length);

    if (MPP_OK == ret) {
        if (MPP_OK == h265d_syntax_fill_slice(s->h265dctx, task->input)) {
            task->valid = 1;
            task->input_packet = s->input_packet;
        }
    }
    return ret;

}

MPP_RET h265d_get_stream(void *ctx, RK_U8 **buf, RK_S32 *size)
{
    MPP_RET ret = MPP_OK;
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = h265dctx->priv_data;
    *buf = s->checksum_buf;
    *size = s->checksum_buf_size;
    return ret;
}

MPP_RET h265d_set_compare_info(void *ctx, void *info)
{
    MPP_RET ret = MPP_OK;
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    h265dctx->compare_info = info;
    return ret;
}


MPP_RET h265d_parse(void *ctx, HalDecTask *task)
{
    MPP_RET ret;
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = h265dctx->priv_data;

    task->valid = 0;
    s->got_frame = 0;
    s->task = task;
    s->ref = NULL;
    ret    = parser_nal_units(s);
    if (ret < 0) {
        if (ret ==  MPP_ERR_STREAM) {
            mpp_log("current stream is no right skip it %p\n", s->ref);
            ret = 0;
        }
        // return ret;
        task->flags.parse_err = 1;
    }
    h265d_dbg(H265D_DBG_GLOBAL, "decode poc = %d", s->poc);
    if (s->ref) {
        h265d_parser2_syntax(h265dctx);
        s->task->syntax.data = s->hal_pic_private;
        s->task->syntax.number = 1;
        s->task->valid = 1;
    }
    if (s->eos) {
        h265d_flush(ctx);
        s->task->flags.eos = 1;
    }
    s->nb_frame++;
    if (s->is_decoded) {
        h265d_dbg(H265D_DBG_GLOBAL, "Decoded frame with POC %d.\n", s->poc);
        s->is_decoded = 0;
    }
    mpp_hevc_output_frame(ctx, 0);
    return MPP_OK;
}

MPP_RET h265d_deinit(void *ctx)
{
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext       *s = h265dctx->priv_data;
    SplitContext_t *sc = h265dctx->split_cxt;
    RK_U8 *buf = NULL;
    int i;

    for (i = 0; i < MAX_DPB_SIZE; i++) {
        mpp_hevc_unref_frame(s, &s->DPB[i], ~0);
        mpp_frame_deinit(&s->DPB[i].frame);
    }

    for (i = 0; i < MAX_VPS_COUNT; i++)
        mpp_free(s->vps_list[i]);
    for (i = 0; i < MAX_SPS_COUNT; i++)
        mpp_free(s->sps_list[i]);
    for (i = 0; i < MAX_PPS_COUNT; i++)
        mpp_hevc_pps_free(s->pps_list[i]);

    mpp_free(s->HEVClc);

    s->HEVClc = NULL;

    for (i = 0; i < s->nals_allocated; i++)
        mpp_free(s->nals[i].rbsp_buffer);

    if (s->nals) {
        mpp_free(s->nals);
    }

    s->nals_allocated = 0;

    if (s->hal_pic_private) {
        mpp_free(s->hal_pic_private);
    }
    if (s->input_packet) {
        buf = mpp_packet_get_data(s->input_packet);
        mpp_free(buf);
        mpp_packet_deinit(&s->input_packet);
    }

    if (s) {
        mpp_free(s);
    }

    if (sc) {
        h265d_split_deinit(sc);
    }
    return 0;
}

static RK_S32 hevc_init_context(H265dContext_t *h265dctx)
{
    HEVCContext *s = h265dctx->priv_data;
    RK_U32 i;

    s->h265dctx = h265dctx;

    s->HEVClc = (HEVCLocalContext*)mpp_calloc(HEVCLocalContext, 1);
    if (!s->HEVClc)
        goto fail;

    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        s->DPB[i].slot_index = 0xff;
        s->DPB[i].poc = INT_MAX;
        s->DPB[i].error_flag = 0;
        mpp_frame_init(&s->DPB[i].frame);
        if (!s->DPB[i].frame)
            goto fail;
    }

    s->max_ra = INT_MAX;


    s->temporal_layer_id   = 8;
    s->context_initialized = 1;

    return 0;

fail:
    h265d_deinit(h265dctx);
    return MPP_ERR_NOMEM;
}


MPP_RET h265d_init(void *ctx, ParserCfg *parser_cfg)
{

    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = (HEVCContext *)h265dctx->priv_data;
    SplitContext_t *sc = (SplitContext_t*)h265dctx->split_cxt;
    RK_S32 ret;
    RK_U8 *buf = NULL;
    RK_S32 size = SZ_512K;
    if (s == NULL) {
        s = (HEVCContext*)mpp_calloc(HEVCContext, 1);
        if (s == NULL) {
            mpp_err("hevc contxt malloc fail");
            return MPP_ERR_NOMEM;
        }
        h265dctx->priv_data = s;
    }

    h265dctx->need_split = parser_cfg->need_split;

    if (sc == NULL && h265dctx->need_split) {
        h265d_split_init((void**)&sc);
        if (sc == NULL) {
            mpp_err("split contxt malloc fail");
            return MPP_ERR_NOMEM;
        }
        h265dctx->split_cxt = sc;
    }

    //  mpp_env_set_u32("h265d_debug", H265D_DBG_REF);
    mpp_env_get_u32("h265d_debug", &h265d_debug, 0);

    ret = hevc_init_context(h265dctx);

    s->hal_pic_private = mpp_calloc_size(void, sizeof(h265d_dxva2_picture_context_t));

    if (ret < 0)
        return ret;

    s->picture_struct = 0;

    s->slots = parser_cfg->frame_slots;

    s->packet_slots = parser_cfg->packet_slots;

    if (h265dctx->extradata_size > 0 && h265dctx->extradata) {
        ret = hevc_parser_extradata(s);
        if (ret < 0) {
            h265d_deinit(h265dctx);
            return ret;
        }
    }

    buf = mpp_malloc(RK_U8, size);

    if (buf == NULL) {
        return MPP_ERR_NOMEM;
    }

    if (MPP_OK != mpp_packet_init(&s->input_packet, (void*)buf, size)) {
        return MPP_ERR_NOMEM;
    }
    mpp_buf_slot_setup(s->slots, 25);
#ifdef dump
    fp = fopen("/data/dump1.bin", "wb+");
#endif
    return 0;
}

MPP_RET h265d_flush(void *ctx)
{
    RK_S32 ret = 0;
    do {
        ret = mpp_hevc_output_frame(ctx, 1);
    } while (ret);
    return MPP_OK;
}

MPP_RET h265d_reset(void *ctx)
{
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HEVCContext *s = (HEVCContext *)h265dctx->priv_data;
    RK_S32 ret = 0;
    do {
        ret = mpp_hevc_output_frame(ctx, 1);
    } while (ret);
    mpp_hevc_flush_dpb(s);
    h265d_split_reset(h265dctx->split_cxt);
    s->max_ra = INT_MAX;
    s->eos = 0;
    return MPP_OK;
}

MPP_RET h265d_control(void *ctx, RK_S32 cmd, void *param)
{
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;

    switch (cmd) {
    case MPP_DEC_SET_DISABLE_ERROR: {
        h265dctx->disable_error = *((RK_U32 *)param);
    }
    default : {
    } break;
    }

    return MPP_OK;
}

MPP_RET h265d_callback(void *ctx, void *err_info)
{
    H265dContext_t *h265dctx = (H265dContext_t *)ctx;
    HalDecTask *task_dec = (HalDecTask *)err_info;

    if (!h265dctx->disable_error) {
        HEVCContext *s = (HEVCContext *)h265dctx->priv_data;
        MppFrame frame = NULL;
        RK_U32 i = 0;

        if (s->first_nal_type >= 16 && s->first_nal_type <= 23) {
            mpp_log("IS_IRAP frame found error");
            s->max_ra = INT_MAX;
        }
        // s->miss_ref_flag = 1;
        mpp_buf_slot_get_prop(s->slots, task_dec->output, SLOT_FRAME_PTR, &frame);
        mpp_frame_set_errinfo(frame, MPP_FRAME_ERR_UNKNOW);
        for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
            if (s->DPB[i].slot_index == task_dec->output) {
                s->DPB[i].error_flag = 1;
            }
        }
    }
    (void) err_info;

    return MPP_OK;
}



const ParserApi api_h265d_parser = {
    .name = "h265d_parse",
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(H265dContext_t),
    .flag = 0,
    .init = h265d_init,
    .deinit = h265d_deinit,
    .prepare = h265d_prepare,
    .parse = h265d_parse,
    .reset = h265d_reset,
    .flush = h265d_flush,
    .control = h265d_control,
    .callback = h265d_callback,
};


