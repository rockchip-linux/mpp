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
        mpp_free(frame->rpl_buf);
        mpp_free(frame->rpl_tab_buf);
        frame->rpl_tab    = NULL;
        frame->refPicList = NULL;
        frame->collocated_ref = NULL;
        if (frame->slot_index <= 0x7f) {
            h265d_dbg(H265D_DBG_REF, "poc %d clr ref index %d", frame->poc, frame->slot_index);
            mpp_buf_slot_clr_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
        }
        h265d_dbg(H265D_DBG_REF, "unref_frame poc %d frame->slot_index %d \n", frame->poc, frame->slot_index);
        frame->poc = INT_MAX;
        frame->slot_index = 0xff;
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

    RK_S32  j;
    RK_U32  i;
    MPP_RET ret = MPP_OK;
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *frame = &s->DPB[i];
        if (frame->slot_index != 0xff) {
            continue;
        }

        if (ret != MPP_OK) {
            goto fail;
        }
        h265d_dbg(H265D_DBG_GLOBAL, "width = %d height = %d", s->h265dctx->width, s->h265dctx->height);
        mpp_frame_set_width(frame->frame, s->h265dctx->width);
        mpp_frame_set_height(frame->frame, s->h265dctx->height);

        mpp_frame_set_hor_stride(frame->frame, (s->h265dctx->coded_width * s->h265dctx->nBitDepth) >> 3);
        mpp_frame_set_ver_stride(frame->frame, s->h265dctx->coded_height);
        mpp_frame_set_fmt(frame->frame, s->h265dctx->pix_fmt);

        h265d_dbg(H265D_DBG_GLOBAL, "w_stride %d h_stride %d\n", s->h265dctx->coded_width, s->h265dctx->coded_height);


        // frame->frame->color_type              = s->h265dctx->pix_fmt;
        //  if (!frame->frame->sample_aspect_ratio.num)
        //     frame->frame->sample_aspect_ratio = s->h265dctx->sample_aspect_ratio;
        mpp_frame_set_pts(frame->frame, s->pts);

        frame->rpl_buf = mpp_calloc(RK_U8, s->nb_nals * sizeof(RefPicListTab));
        if (!frame->rpl_buf)
            goto fail;

        frame->ctb_count = s->sps->ctb_width * s->sps->ctb_height;
        frame->rpl_tab_buf = mpp_calloc(RK_U8, frame->ctb_count * sizeof(RefPicListTab));

        if (!frame->rpl_tab_buf)
            goto fail;

        frame->rpl_tab  = (RefPicListTab **)frame->rpl_tab_buf;

        for (j = 0; j < frame->ctb_count; j++)
            frame->rpl_tab[j] = (RefPicListTab *)frame->rpl_buf;

//       frame->frame->top_field_first = s->picture_struct;
        ret = mpp_buf_slot_get_unused(s->slots, &frame->slot_index);
        return frame;
    fail:
        mpp_hevc_unref_frame(s, frame, ~0);
        return NULL;
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
    if (!ref)
        return MPP_ERR_NOMEM;

    *mframe = ref->frame;
    s->ref = ref;

    ref->poc      = poc;
    h265d_dbg(H265D_DBG_REF, "alloc frame poc %d slot_index %d", poc, ref->slot_index);
    ref->flags    = HEVC_FRAME_FLAG_OUTPUT | HEVC_FRAME_FLAG_SHORT_REF;

    mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_HAL_OUTPUT);
    mpp_buf_slot_set_prop(s->slots, ref->slot_index, SLOT_FRAME, ref->frame);
    s->task->output = ref->slot_index;

    ref->sequence = s->seq_decode;
    ref->window   = s->sps->output_window;
    return 0;
}

static int init_slice_rpl(HEVCContext *s)
{
    HEVCFrame *frame = s->ref;
    RK_S32 ctb_count    = frame->ctb_count;
    RK_S32 ctb_addr_ts  = s->pps->ctb_addr_rs_to_ts[s->sh.slice_segment_addr];
    RK_S32 i;

    if (s->slice_idx >= s->nb_nals)
        return  MPP_ERR_STREAM;

    for (i = ctb_addr_ts; i < ctb_count; i++)
        frame->rpl_tab[i] = (RefPicListTab *)frame->rpl_buf + s->slice_idx;

    frame->refPicList = (RefPicList *)frame->rpl_tab[ctb_addr_ts];

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

    mpp_err(
        "Could not find ref with POC %d\n", poc);
    return NULL;
}

int mpp_hevc_slice_rpl(HEVCContext *s)
{
    RK_U8 list_idx;
    RK_U32 i;
    RK_S32 j, ret;
    RefPicList *rpl = NULL;
    RK_S32 cand_lists[3];

    SliceHeader *sh = &s->sh;
    RK_U8 nb_list = sh->slice_type == B_SLICE ? 2 : 1;

    ret = init_slice_rpl(s);
    if (ret < 0)
        return ret;

    if (!(s->rps[ST_CURR_BEF].nb_refs + s->rps[ST_CURR_AFT].nb_refs +
          s->rps[LT_CURR].nb_refs)) {
        mpp_err( "Zero refs in the frame RPS.\n");
        return  MPP_ERR_STREAM;
    }

    for (list_idx = 0; list_idx < nb_list; list_idx++) {
        RefPicList rpl_tmp;
        memset(&rpl_tmp, 0, sizeof(rpl_tmp));
        rpl = &s->ref->refPicList[list_idx];

        /* The order of the elements is
         * ST_CURR_BEF - ST_CURR_AFT - LT_CURR for the L0 and
         * ST_CURR_AFT - ST_CURR_BEF - LT_CURR for the L1 */
        cand_lists[0] = list_idx ? ST_CURR_AFT : ST_CURR_BEF;
        cand_lists[1] = list_idx ? ST_CURR_BEF : ST_CURR_AFT;
        cand_lists[2] = LT_CURR;
        /* concatenate the candidate lists for the current frame */
        while ((RK_U32)rpl_tmp.nb_refs < sh->nb_refs[list_idx]) {
            for (i = 0; i < MPP_ARRAY_ELEMS(cand_lists); i++) {
                RefPicList *rps = &s->rps[cand_lists[i]];
                for (j = 0; j < rps->nb_refs && rpl_tmp.nb_refs < MAX_REFS; j++) {
                    rpl_tmp.list[rpl_tmp.nb_refs]       = rps->list[j];
                    rpl_tmp.ref[rpl_tmp.nb_refs]        = rps->ref[j];
                    rpl_tmp.isLongTerm[rpl_tmp.nb_refs] = i == 2;
                    rpl_tmp.nb_refs++;
                }
            }
        }

        /* reorder the references if necessary */
        if (sh->rpl_modification_flag[list_idx]) {
            for (i = 0; i < sh->nb_refs[list_idx]; i++) {
                int idx = sh->list_entry_lx[list_idx][i];

                if (!s->decoder_id && idx >= rpl_tmp.nb_refs) {
                    mpp_err( "Invalid reference index.\n");
                    return  MPP_ERR_STREAM;
                }

                rpl->list[i]       = rpl_tmp.list[idx];
                rpl->ref[i]        = rpl_tmp.ref[idx];
                rpl->isLongTerm[i] = rpl_tmp.isLongTerm[idx];
                rpl->nb_refs++;
            }
        } else {
            memcpy(rpl, &rpl_tmp, sizeof(*rpl));
            rpl->nb_refs = MPP_MIN((RK_U32)rpl->nb_refs, sh->nb_refs[list_idx]);
        }

        if (sh->collocated_list == list_idx &&
            sh->collocated_ref_idx < (RK_U32)rpl->nb_refs)
            s->ref->collocated_ref = rpl->ref[sh->collocated_ref_idx];
    }
    return 0;
}

static void mark_ref(HEVCFrame *frame, int flag)
{
    frame->flags &= ~(HEVC_FRAME_FLAG_LONG_REF | HEVC_FRAME_FLAG_SHORT_REF);
    frame->flags |= flag;
}

static HEVCFrame *generate_missing_ref(HEVCContext *s, int poc)
{
    HEVCFrame *frame;
    mpp_log("generate_missing_ref in \n");
    frame = alloc_frame(s);
    if (!frame)
        return NULL;
#if 0
    if (!s->sps->pixel_shift) {
        for (i = 0; frame->frame->buf[i]; i++)
            memset(frame->frame->buf[i]->data, 1 << (s->sps->bit_depth - 1),
                   frame->frame->buf[i]->size);
    } else {
        for (i = 0; frame->frame->data[i]; i++)
            for (y = 0; y < (s->sps->height >> s->sps->vshift[i]); y++)
                for (x = 0; x < (s->sps->width >> s->sps->hshift[i]); x++) {
                    AV_WN16(frame->frame->data[i] + y * frame->frame->linesize[i] + 2 * x,
                            1 << (s->sps->bit_depth - 1));
                }
    }
#endif
    frame->poc      = poc;

    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_READY);
    mpp_buf_slot_set_flag(s->slots, frame->slot_index, SLOT_CODEC_USE);
    mpp_log("generate_missing_ref frame poc %d slot_index %d", poc, frame->slot_index);
    frame->sequence = s->seq_decode;
    frame->flags    = 0;

    return frame;
}

/* add a reference with the given poc to the list and mark it as used in DPB */
static int add_candidate_ref(HEVCContext *s, RefPicList *list,
                             int poc, int ref_flag)
{
    HEVCFrame *ref = find_ref_idx(s, poc);

    if (ref == s->ref)
        return  MPP_ERR_STREAM;

    if (!ref) {
        ref = generate_missing_ref(s, poc);
        if (!ref)
            return MPP_ERR_NOMEM;
    }

    list->list[list->nb_refs] = ref->poc;
    list->ref[list->nb_refs]  = ref;
    list->nb_refs++;
    if (ref_flag) {
        h265d_dbg(H265D_DBG_REF, "set ref poc = %d ref->slot_index %d", ref->poc, ref->slot_index);
        mpp_buf_slot_set_flag(s->slots, ref->slot_index, SLOT_CODEC_USE);
    }
    mark_ref(ref, ref_flag);

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
        if (!long_rps)
            return 0;
    }

    /* clear the reference flags on all frames except the current one */
    for (i = 0; i < MPP_ARRAY_ELEMS(s->DPB); i++) {
        HEVCFrame *frame = &s->DPB[i];
        if (frame == s->ref)
            continue;
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

