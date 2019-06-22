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
 * @file       h265_refs.c
 * @brief
 * @author      csy(csy@rock-chips.com)

 * @version     1.0.0
 * @history
 *   2015.7.15 : Create
 */

#define MODULE_TAG  "H265_PARSER_REF"

#include "h265d_parser.h"

#define HEVC_ALIGN(value, x)   ((value + (x-1)) & (~(x-1)))

void mpp_hevc_unref_frame(HEVCContext *s, HEVCFrame *frame, int flags)
{
    /* frame->frame can be NULL if context init failed */
    if (!frame->frame || (frame->slot_index == 0xff))
        return;

    frame->flags &= ~flags;
    if (!frame->flags) {
        frame->refPicList = NULL;
        frame->collocated_ref = NULL;
        if (frame->slot_index <= 0x7f) {
            h265d_dbg(H265D_DBG_REF, "poc %d clr ref index %d", frame->poc, frame->slot_index);
            mpp_buf_slot_clr_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
        }
        h265d_dbg(H265D_DBG_REF, "unref_frame poc %d frame->slot_index %d \n", frame->poc, frame->slot_index);
        frame->poc = INT_MAX;
        frame->slot_index = 0xff;
        frame->error_flag = 0;
    }
}


void mpp_hevc_clear_refs(HEVCContext *s)
{
    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        mpp_hevc_unref_frame(s, &s->DPB[i],
                             HEVC_FRAME_FLAG_SHORT_REF |
                             HEVC_FRAME_FLAG_LONG_REF);
    }
}

void mpp_hevc_flush_dpb(HEVCContext *s)
{
    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        mpp_hevc_unref_frame(s, &s->DPB[i], ~0);
    }
}

static HEVCFrame *alloc_frame(HEVCContext *s)
{

    RK_U32  i;
    MPP_RET ret = MPP_OK;
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *frame = &s->DPB[i];
        if (frame->slot_index != 0xff) {
            continue;
        }

        h265d_dbg(H265D_DBG_GLOBAL, "width = %d height = %d", s->h265dctx->width, s->h265dctx->height);
        mpp_frame_set_width(frame->frame, s->h265dctx->width);
        mpp_frame_set_height(frame->frame, s->h265dctx->height);
        mpp_frame_set_hor_stride(frame->frame, (s->h265dctx->coded_width * s->h265dctx->nBitDepth) >> 3);
        mpp_frame_set_ver_stride(frame->frame, s->h265dctx->coded_height);
        mpp_frame_set_fmt(frame->frame, s->h265dctx->pix_fmt);
        mpp_frame_set_errinfo(frame->frame, 0);
        mpp_frame_set_pts(frame->frame, s->pts);
        mpp_frame_set_poc(frame->frame, s->poc);
        mpp_frame_set_color_range(frame->frame, s->h265dctx->color_range);
        mpp_frame_set_color_primaries(frame->frame, s->sps->vui.colour_primaries);
        mpp_frame_set_color_trc(frame->frame, s->sps->vui.transfer_characteristic);
        mpp_frame_set_colorspace(frame->frame, s->h265dctx->colorspace);
        mpp_frame_set_mastering_display(frame->frame, s->mastering_display);
        mpp_frame_set_content_light(frame->frame, s->content_light);
        h265d_dbg(H265D_DBG_GLOBAL, "w_stride %d h_stride %d\n", s->h265dctx->coded_width, s->h265dctx->coded_height);
        ret = mpp_buf_slot_get_unused(s->slots, &frame->slot_index);
        mpp_assert(ret == MPP_OK);
        return frame;
    }
    mpp_err( "Error allocating frame, DPB full.\n");
    return NULL;
}

int mpp_hevc_set_new_ref(HEVCContext *s, MppFrame *mframe, int poc)
{

    HEVCFrame *ref = NULL;
    RK_U32 i;

    /* check that this POC doesn't already exist */
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *frame = &s->DPB[i];

        if ((frame->slot_index != 0xff) && frame->sequence == s->seq_decode &&
            frame->poc == poc && !s->nuh_layer_id) {
            mpp_err( "Duplicate POC in a sequence: %d.\n",
                     poc);
            return  MPP_ERR_STREAM;
        }
    }

    ref = alloc_frame(s);
    if (!ref) {
        mpp_err( "alloc_frame error\n");
        return MPP_ERR_NOMEM;
    }

    *mframe = ref->frame;
    s->ref = ref;
    ref->poc      = poc;
    h265d_dbg(H265D_DBG_REF, "alloc frame poc %d slot_index %d", poc, ref->slot_index);
    ref->flags    = HEVC_FRAME_FLAG_OUTPUT | HEVC_FRAME_FLAG_SHORT_REF;
    mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_HAL_OUTPUT);
    s->task->output = ref->slot_index;

    ref->sequence = s->seq_decode;
    ref->window   = s->sps->output_window;
    return 0;
}

static HEVCFrame *find_ref_idx(HEVCContext *s, int poc)
{
    RK_U32 i;
    RK_S32 LtMask = (1 << s->sps->log2_max_poc_lsb) - 1;

    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *ref = &s->DPB[i];
        if ((ref->slot_index != 0xff) && (ref->sequence == s->seq_decode)) {
            if ((ref->poc & LtMask) == poc)
                return ref;
        }
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *ref = &s->DPB[i];
        if ((ref->slot_index != 0xff) && ref->sequence == s->seq_decode) {
            if (ref->poc == poc || (ref->poc & LtMask) == poc)
                return ref;
        }
    }
    mpp_err("Could not find ref with POC %d\n", poc);
    return NULL;
}



static void mark_ref(HEVCFrame *frame, int flag)
{
    frame->flags &= ~(HEVC_FRAME_FLAG_LONG_REF | HEVC_FRAME_FLAG_SHORT_REF);
    frame->flags |= flag;
}

static HEVCFrame *generate_missing_ref(HEVCContext *s, int poc)
{
    HEVCFrame *frame;
    h265d_dbg(H265D_DBG_REF, "generate_missing_ref in \n");
    frame = alloc_frame(s);
    if (!frame)
        return NULL;
    frame->poc      = poc;
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_READY);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
    h265d_dbg(H265D_DBG_REF, "generate_missing_ref frame poc %d slot_index %d", poc, frame->slot_index);
    frame->sequence = s->seq_decode;
    frame->flags    = 0;
    return frame;
}

/* add a reference with the given poc to the list and mark it as used in DPB */
static int add_candidate_ref(HEVCContext *s, RefPicList *list,
                             int poc, int ref_flag)
{
    HEVCFrame *ref = find_ref_idx(s, poc);

    if (!ref) {
        ref = generate_missing_ref(s, poc);
        if (!ref)
            return MPP_ERR_NOMEM;
        ref->error_flag = 1;
    }

    list->list[list->nb_refs] = ref->poc;
    list->ref[list->nb_refs]  = ref;
    list->nb_refs++;
    if (ref_flag) {
        h265d_dbg(H265D_DBG_REF, "set ref poc = %d ref->slot_index %d", ref->poc, ref->slot_index);
        mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_CODEC_USE);
    }
    mark_ref(ref, ref_flag);
    if (ref->error_flag) {
        s->miss_ref_flag = 1;
    }
    return 0;
}

RK_S32 mpp_hevc_frame_rps(HEVCContext *s)
{

    const ShortTermRPS *short_rps = s->sh.short_term_rps;
    const LongTermRPS  *long_rps  = &s->sh.long_term_rps;
    RefPicList               *rps = s->rps;
    RK_S32  ret;
    RK_U32  i;

    if (!short_rps) {
        rps[0].nb_refs = rps[1].nb_refs = 0;
        return 0;
    }

    /* clear the reference flags on all frames except the current one */
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *frame = &s->DPB[i];
        mark_ref(frame, 0);
    }

    for (i = 0; i < NB_RPS_TYPE; i++)
        rps[i].nb_refs = 0;

    /* add the short refs */
    for (i = 0; short_rps && (RK_S32)i < short_rps->num_delta_pocs; i++) {
        int poc = s->poc + short_rps->delta_poc[i];
        int list;

        if (!short_rps->used[i])
            list = ST_FOLL;
        else if (i < short_rps->num_negative_pics)
            list = ST_CURR_BEF;
        else
            list = ST_CURR_AFT;

        ret = add_candidate_ref(s, &rps[list], poc, HEVC_FRAME_FLAG_SHORT_REF);
        if (ret < 0)
            return ret;
    }

    /* add the long refs */
    for (i = 0; long_rps && i < long_rps->nb_refs; i++) {
        int poc  = long_rps->poc[i];
        int list = long_rps->used[i] ? LT_CURR : LT_FOLL;

        ret = add_candidate_ref(s, &rps[list], poc, HEVC_FRAME_FLAG_LONG_REF);
        if (ret < 0)
            return ret;
    }
    /* release any frames that are now unused */
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        mpp_hevc_unref_frame(s, &s->DPB[i], 0);
    }

    return 0;
}

int mpp_hevc_compute_poc(HEVCContext *s, int poc_lsb)
{
    RK_S32 max_poc_lsb  = 1 << s->sps->log2_max_poc_lsb;
    RK_S32 prev_poc_lsb = s->pocTid0 % max_poc_lsb;
    RK_S32 prev_poc_msb = s->pocTid0 - prev_poc_lsb;
    RK_S32 poc_msb;

    if (poc_lsb < prev_poc_lsb && prev_poc_lsb - poc_lsb >= max_poc_lsb / 2)
        poc_msb = prev_poc_msb + max_poc_lsb;
    else if (poc_lsb > prev_poc_lsb && poc_lsb - prev_poc_lsb > max_poc_lsb / 2)
        poc_msb = prev_poc_msb - max_poc_lsb;
    else
        poc_msb = prev_poc_msb;

    // For BLA picture types, POCmsb is set to 0.
    if (s->nal_unit_type == NAL_BLA_W_LP   ||
        s->nal_unit_type == NAL_BLA_W_RADL ||
        s->nal_unit_type == NAL_BLA_N_LP)
        poc_msb = 0;

    return poc_msb + poc_lsb;
}

int mpp_hevc_frame_nb_refs(HEVCContext *s)
{
    RK_S32 ret = 0;
    RK_S32 i;
    const ShortTermRPS *rps = s->sh.short_term_rps;
    LongTermRPS *long_rps   = &s->sh.long_term_rps;

    if (s->sh.slice_type == I_SLICE) {
        return 0;
    }
    if (rps) {
        for (i = 0; (RK_U32)i < rps->num_negative_pics; i++)
            ret += !!rps->used[i];
        for (; i < rps->num_delta_pocs; i++)
            ret += !!rps->used[i];
    }

    if (long_rps) {
        for (i = 0; i < long_rps->nb_refs; i++)
            ret += !!long_rps->used[i];
    }
    return ret;
}

