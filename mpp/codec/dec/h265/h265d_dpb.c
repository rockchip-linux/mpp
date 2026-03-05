/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "h265d"

#include "mpp_log.h"
#include "mpp_bit.h"
#include "mpp_compat_impl.h"
#include "mpp_ref_pool.h"

#include "h265d_debug.h"
#include "h265d_dpb.h"

#define HEVC_ALIGN(value, x)   ((value + (x-1)) & (~(x-1)))

void h265d_frame_unref(H265dPrs *p, RK_U32 dpb_idx, H265dFrmRefStatus clear_mask)
{
    H265dFrame *frame = &p->dpb[dpb_idx];

    if (!frame->frame || (frame->slot_index == 0xff))
        return;

    /* Clear reference status */
    if (clear_mask.st_ref)
        frame->ref_status.st_ref = 0;
    if (clear_mask.lt_ref)
        frame->ref_status.lt_ref = 0;
    if (clear_mask.output)
        frame->ref_status.output = 0;

    /* When frame is fully unreferenced, clear error status */
    if (!frame->ref_status.flags) {
        RK_S32 hdr_meta_index = frame->hdr_meta_index;

        if (frame->slot_index <= 0x7f) {
            h265d_dbg_ref("poc %d clr ref index %d", frame->poc, frame->slot_index);
            mpp_buf_slot_clr_flag(p->slots, frame->slot_index, SLOT_CODEC_USE);
        }

        /* Release HDR metadata reference */
        if (hdr_meta_index >= 0) {
            mpp_ref_pool_put(&p->hdr_meta_pool, hdr_meta_index);
            frame->hdr_meta_index = -1;
        }

        h265d_dbg_ref("unref_frame poc %d slot_index %d\n", frame->poc, frame->slot_index);
        frame->poc = INT_MAX;
        frame->slot_index = 0xff;
        /* Clear error status when frame is fully unreferenced */
        frame->err_status.has_err = 0;
    }
}

void h265d_frame_unref_all(H265dPrs *p)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrmRefStatus mask = {.st_ref = 1, .lt_ref = 1};
        h265d_frame_unref(p, i, mask);
    }
}

void h265d_dpb_flush(H265dPrs *p)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrmRefStatus mask = {.flags = 0x07};
        h265d_frame_unref(p, i, mask);
    }
}

static H265dFrame *h265d_frame_create(H265dPrs *p, RK_S32 poc, RK_U32 ref_only)
{
    H265dCtx *ctx = p->ctx;
    MppFrameFormat fmt = ctx->cfg->base.out_fmt & (~MPP_FRAME_FMT_MASK);
    MPP_RET ret = MPP_OK;
    H265dFrame *frame = NULL;
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        frame = &p->dpb[i];

        if (frame->slot_index != 0xff)
            continue;

        mpp_frame_set_width(frame->frame, ctx->width);
        mpp_frame_set_height(frame->frame, ctx->height);
        mpp_frame_set_hor_stride(frame->frame,
                                 (MPP_ALIGN(ctx->coded_width, 64) * ctx->bit_depth) >> 3);
        mpp_frame_set_ver_stride(frame->frame, ctx->coded_height);
        ctx->pix_fmt &= MPP_FRAME_FMT_MASK;
        if (p->is_hdr) {
            p->ctx->pix_fmt |= MPP_FRAME_HDR;
        }
        ctx->pix_fmt |= fmt;
        mpp_frame_set_fmt(frame->frame, ctx->pix_fmt);

        if (MPP_FRAME_FMT_IS_FBC(ctx->pix_fmt)) {
            RK_U32 fbc_hdr_stride = MPP_ALIGN(ctx->width, 64);

            mpp_slots_set_prop(p->slots, SLOTS_HOR_ALIGN, mpp_align_64);
            mpp_frame_set_offset_x(frame->frame, 0);
            mpp_frame_set_offset_y(frame->frame, 4);

            if (*compat_ext_fbc_buf_size)
                mpp_frame_set_ver_stride(frame->frame, ctx->coded_height + 16);

            if (*compat_ext_fbc_hdr_256_odd)
                fbc_hdr_stride = MPP_ALIGN(ctx->width, 256) | 256;

            mpp_frame_set_fbc_hdr_stride(frame->frame, fbc_hdr_stride);
        } else if (MPP_FRAME_FMT_IS_TILE(p->ctx->pix_fmt)) {
        } else {
            if ((ctx->cfg->base.enable_vproc & MPP_VPROC_MODE_DETECTION) &&
                ctx->width <= 1920 &&  ctx->height <= 1088)
                mpp_frame_set_mode(frame->frame, MPP_FRAME_FLAG_DEINTERLACED);
        }

        if (ctx->cfg->base.enable_thumbnail && p->hw_info->cap_down_scale)
            mpp_frame_set_thumbnail_en(frame->frame, p->ctx->cfg->base.enable_thumbnail);
        else
            mpp_frame_set_thumbnail_en(frame->frame, 0);

        mpp_frame_set_errinfo(frame->frame, ref_only ? 1 : 0);
        mpp_frame_set_discard(frame->frame, 0);
        mpp_frame_set_pts(frame->frame, p->pts);
        mpp_frame_set_dts(frame->frame, p->dts);
        mpp_frame_set_poc(frame->frame, poc);
        mpp_frame_set_color_range(frame->frame, ctx->color_range);
        mpp_frame_set_color_primaries(frame->frame, p->sps->vui.colour_primaries);
        if (p->alternative_transfer.present)
            mpp_frame_set_color_trc(frame->frame,
                                    p->alternative_transfer.preferred_transfer_characteristics);
        else
            mpp_frame_set_color_trc(frame->frame, p->sps->vui.transfer_characteristic);
        mpp_frame_set_colorspace(frame->frame, ctx->colorspace);
        mpp_frame_set_mastering_display(frame->frame, p->mastering_display);
        mpp_frame_set_content_light(frame->frame, p->content_light);

        h265d_dbg_global("width:height [%d:%d] [%d:%d] poc %d\n",
                         ctx->width, ctx->height, ctx->coded_width, ctx->coded_height, poc);

        ret = mpp_buf_slot_get_unused(p->slots, &frame->slot_index);
        mpp_assert(ret == MPP_OK);

        frame->poc = poc;
        frame->sequence = p->seq_decode;

        /* Initialize HDR metadata index */
        frame->hdr_meta_index = -1;

        if (ref_only) {
            mpp_buf_slot_set_prop(p->slots, frame->slot_index, SLOT_FRAME, frame->frame);
            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_CODEC_READY);
            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_CODEC_USE);
            h265d_dbg_ref("h265d_frame_create ref_only poc %d slot_index %d", poc, frame->slot_index);
            frame->ref_status.output = 0;
            frame->ref_status.st_ref = 0;
            frame->ref_status.lt_ref = 0;
        } else {
            /* Use index-based HDR metadata reference */
            RK_S32 hdr_meta_index = p->hdr_meta_current;
            MppRefPool *hdr_meta_pool = &p->hdr_meta_pool;

            if (hdr_meta_index >= 0 && p->hdr_dynamic) {
                frame->hdr_meta_index = hdr_meta_index;
                mpp_ref_pool_ref(hdr_meta_pool, hdr_meta_index);
                mpp_frame_set_hdr_dynamic_meta(frame->frame,
                                               (MppFrameHdrDynamicMeta*)mpp_ref_pool_ptr(hdr_meta_pool, hdr_meta_index));
                h265d_dbg_ref("h265d_frame_create poc %d slot %d hdr_meta_index %d",
                              poc, frame->slot_index, hdr_meta_index);
            }

            h265d_dbg_ref("h265d_frame_create poc %d slot_index %d", poc, frame->slot_index);
            frame->ref_status.output = 1;
            frame->ref_status.st_ref = 1;
            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_CODEC_USE);
            mpp_buf_slot_set_flag(p->slots, frame->slot_index, SLOT_HAL_OUTPUT);
            p->task->output = frame->slot_index;
            p->ref = frame;
        }

        return frame;
    }
    mpp_loge("refs: dpb full, cannot allocate frame\n");
    return NULL;
}

RK_S32 h265d_build_refs(H265dPrs *p, MppFrame *mframe, RK_S32 poc)
{
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrame *frame = &p->dpb[i];

        if ((frame->slot_index != 0xff) && frame->sequence == p->seq_decode &&
            frame->poc == poc && !p->nuh_layer_id) {
            mpp_loge("refs: duplicate poc %d\n", poc);
            return MPP_ERR_STREAM;
        }
    }

    H265dFrame *ref = h265d_frame_create(p, poc, 0);
    if (!ref) {
        mpp_loge("refs: frame create failed\n");
        return MPP_ERR_NOMEM;
    }

    *mframe = ref->frame;
    return 0;
}

static H265dFrame *h265d_frame_find_by_poc(H265dPrs *p, RK_S32 poc)
{
    RK_S32 lt_mask = (1 << p->sps->log2_max_poc_lsb) - 1;
    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrame *ref = &p->dpb[i];

        if ((ref->slot_index != 0xff) && (ref->sequence == p->seq_decode)) {
            if ((ref->poc & lt_mask) == poc)
                return ref;
        }
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrame *ref = &p->dpb[i];

        if ((ref->slot_index != 0xff) && ref->sequence == p->seq_decode) {
            if (ref->poc == poc || (ref->poc & lt_mask) == poc)
                return ref;
        }
    }

    mpp_loge("refs: cur_frm %d missing ref poc %d\n", p->poc, poc);
    return NULL;
}

static void h265d_set_frame_ref_type(H265dFrame *frame, RK_S32 ref_type)
{
    frame->ref_status.lt_ref = (ref_type == H265D_FRM_FLAG_LONG_REF) ? 1 : 0;
    frame->ref_status.st_ref = (ref_type == H265D_FRM_FLAG_SHORT_REF) ? 1 : 0;
}

static RK_S32 h265d_ref_list_add(H265dPrs *p, H265dRefList *list,
                                 RK_S32 poc, RK_S32 ref_flag, RK_U32 cur_used)
{
    H265dFrame *ref = h265d_frame_find_by_poc(p, poc);

    if (!ref) {
        ref = h265d_frame_create(p, poc, 1);
        if (!ref)
            return MPP_ERR_NOMEM;

        ref->err_status.has_err = 1;
    }

    list->list[list->nb_refs] = ref->poc;
    list->ref[list->nb_refs]  = ref;
    list->nb_refs++;
    if (ref_flag) {
        h265d_dbg_ref("set ref poc %d slot_index %d", ref->poc, ref->slot_index);
        mpp_buf_slot_set_flag(p->slots, ref->slot_index, SLOT_CODEC_USE);
    }
    h265d_set_frame_ref_type(ref, ref_flag);
    if (ref->err_status.has_err && cur_used) {
        p->miss_ref_flag = 1;
    }
    return 0;
}

RK_S32 h265d_build_rps(H265dPrs *p)
{
    const H265dStRps *st_rps = p->slice.st_rps_using;
    const H265dLtRps *lt_prs = &p->slice.lt_rps;
    H265dRefList *rps = p->rps;
    RK_S32  type;
    RK_S32  ret;
    RK_U32  i;

    type = p->nal_unit_type;

    if (!st_rps) {
        rps[0].nb_refs = rps[1].nb_refs = 0;
        return 0;
    }

    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrame *frame = &p->dpb[i];

        h265d_set_frame_ref_type(frame, 0);
    }

    for (i = 0; i < NB_RPS_TYPE; i++) {
        rps[i].nb_refs = 0;
        memset(rps[i].ref, 0, sizeof(rps[i].ref));
        memset(rps[i].list, 0, sizeof(rps[i].list));
    }

    for (i = 0; st_rps && (RK_S32)i < st_rps->num_deltas; i++) {
        RK_S32 poc = p->poc + st_rps->poc_delta[i];
        RK_S32 list;

        if (!MPP_GET_BIT(st_rps->flags, i))
            list = ST_FOLL;
        else if (i < st_rps->num_neg)
            list = ST_CURR_BEF;
        else
            list = ST_CURR_AFT;

        ret = h265d_ref_list_add(p, &rps[list], poc, H265D_FRM_FLAG_SHORT_REF, ST_FOLL != list);
        if (ret < 0)
            return ret;
    }

    for (i = 0; lt_prs && i < lt_prs->nb_refs; i++) {
        RK_S32 poc  = lt_prs->poc[i];
        RK_S32 list = MPP_GET_BIT(lt_prs->used, i) ? LT_CURR : LT_FOLL;

        ret = h265d_ref_list_add(p, &rps[list], poc, H265D_FRM_FLAG_LONG_REF, LT_FOLL != list);
        if (ret < 0)
            return ret;
    }
    for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
        H265dFrmRefStatus mask = {0};
        h265d_frame_unref(p, i, mask);
    }
    if (IS_CRA(type)) {
        for (i = 0; i < MPP_ARRAY_ELEMS(p->dpb); i++) {
            H265dFrame *ref = &p->dpb[i];

            if ((ref->slot_index != 0xff) && (ref->poc > p->poc)) {
                h265d_flush(p->ctx);
                break;
            }
        }
    }

    /* Calculate and store number of reference frames */
    if (p->slice.slice_type == I_SLICE) {
        p->nb_refs = 0;
    } else {
        RK_S32 nb_refs = 0;
        if (st_rps) {
            for (i = 0; i < st_rps->num_neg; i++)
                nb_refs += MPP_GET_BIT(st_rps->flags, i);
            for (i = st_rps->num_neg; (RK_S32)i < st_rps->num_deltas; i++)
                nb_refs += MPP_GET_BIT(st_rps->flags, i);
        }
        if (lt_prs) {
            for (i = 0; i < lt_prs->nb_refs; i++)
                nb_refs += MPP_GET_BIT(lt_prs->used, i);
        }
        p->nb_refs = nb_refs;
    }

    return 0;
}
