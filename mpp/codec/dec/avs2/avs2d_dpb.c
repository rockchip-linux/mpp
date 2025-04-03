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

#define MODULE_TAG "avs2d_dpb"

#include <string.h>
#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_compat_impl.h"

#include "hal_task.h"
#include "avs2d_dpb.h"

#ifndef INT_MAX
#define INT_MAX       2147483647      /* maximum (signed) int value */
#endif

/**
 * @brief Calculate dpb size according to specification
 * Some bitstream may require larger dpb than declaration, so the largest dpb
 * size is used for all bitstream. And this function is commented to silence
 * compiler's unused warnning.
 */
static RK_U32 dpb_get_size(Avs2dCtx_t *p_dec)
{
    RK_U32 mini_size = 8;
    RK_U32 mini_cu_width, mini_cu_height;
    RK_U32 pic_size = 0;
    RK_U32 dpb_size = AVS2_MAX_DPB_SIZE;

    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    mini_cu_width = (vsh->horizontal_size + mini_size - 1) / mini_size;
    if (vsh->progressive_sequence == 0 && vsh->field_coded_sequence == 0) {
        mini_cu_height = (vsh->vertical_size + 2 * mini_size - 1) / (2 * mini_size);
    } else {
        mini_cu_height = (vsh->vertical_size + mini_size - 1) / mini_size;
    }
    pic_size = mini_cu_width * mini_size * mini_cu_height * mini_size;
    avs2d_dbg_dpb("level_id %d, pic_size %d", vsh->level_id, pic_size);

    switch (vsh->level_id) {
    case 0x10:
    case 0x12:
    case 0x14:
    case 0x20:
    case 0x22:
        dpb_size = 15;
        break;
    case 0x40:
    case 0x42:
    case 0x44:
    case 0x46:
    case 0x48:
    case 0x4A:
        dpb_size = MPP_MIN(13369344 / pic_size, 16) - 1;
        break;
    case 0x50:
    case 0x52:
    case 0x54:
    case 0x56:
    case 0x58:
    case 0x5A:
        dpb_size = MPP_MIN(56623104 / pic_size, 16) - 1;
        break;
    case 0x60:
    case 0x62:
    case 0x64:
    case 0x66:
    case 0x68:
    case 0x6A:
        dpb_size = MPP_MIN(213909504 / pic_size, 16) - 1;
        break;
    default:
        AVS2D_DBG(AVS2D_DBG_WARNNING, "invalid level id(%d)", vsh->level_id);
        break;
    }

    if (dpb_size < (RK_U32)(vsh->num_of_rps + 1)) {
        dpb_size = (RK_U32)(vsh->num_of_rps + 1);
    }
    dpb_size = MPP_MIN(dpb_size, AVS2_MAX_DPB_SIZE);

    return dpb_size;
}

static Avs2dFrame_t *new_frame()
{
    MPP_RET ret = MPP_OK;

    Avs2dFrame_t *f = mpp_calloc(Avs2dFrame_t, 1);
    MEM_CHECK(ret, f);
    f->slot_idx = NO_VAL;
    f->doi = NO_VAL;
    f->poi = NO_VAL;

    return f;
__FAILED:
    (void)ret;
    return NULL;
}

static void dpb_init_management(Avs2dFrameMgr_t *mgr)
{
    mgr->num_of_ref   = 0;
    mgr->prev_doi     = NO_VAL;
    mgr->output_poi   = NO_VAL;
    mgr->tr_wrap_cnt  = 0;

    mgr->scene_ref = NULL;
    mgr->cur_frm   = NULL;
    mgr->used_size = 0;
    memset(mgr->refs, 0, sizeof(mgr->refs));
    memset(&mgr->cur_rps, 0, sizeof(Avs2dRps_t));
}

MPP_RET avs2d_dpb_create(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");
    mgr->dpb_specific_size = dpb_get_size(p_dec);
    mgr->dpb_size = 15;
    avs2d_dbg_dpb("create dpb, size %d, specific_size %d\n", mgr->dpb_size, mgr->dpb_specific_size);
    mgr->dpb = mpp_calloc(Avs2dFrame_t*, mgr->dpb_size);
    for (i = 0; i < mgr->dpb_size; i++) {
        mgr->dpb[i] = new_frame();
        MEM_CHECK(ret, mgr->dpb[i]);
        mpp_frame_init(&mgr->dpb[i]->frame);
        avs2d_dbg_dpb("DPB[%d], frame %p", i, mgr->dpb[i]->frame);
        MEM_CHECK(ret, mgr->dpb[i]->frame);
    }

    dpb_init_management(mgr);
    mgr->initial_flag = 1;
__FAILED:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_dpb_destroy(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");
    for (i = 0; i < mgr->dpb_size; i++) {
        mpp_frame_deinit(&mgr->dpb[i]->frame);
        MPP_FREE(mgr->dpb[i]);
    }

    MPP_FREE(mgr->dpb);
    mgr->initial_flag = 0;

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

static void recompute_cycle_order_index(Avs2dFrameMgr_t *mgr, Avs2dPicHeader_t *ph)
{
    RK_U32 i;
    RK_S32 wrap_oi;
    Avs2dFrame_t *p;

    if (ph->doi < (INT_MAX - 2 * AVS2_DOI_CYCLE)) {
        return;
    }

    wrap_oi = (mgr->tr_wrap_cnt - 1) * AVS2_DOI_CYCLE;
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];
        if (p->slot_idx == NO_VAL) {
            continue;
        }
        p->doi -= wrap_oi;
        p->poi -= wrap_oi;
    }

    ph->doi -= wrap_oi;
    if (mgr->output_poi != NO_VAL) {
        mgr->output_poi -= wrap_oi;
    }
}

static void compute_frame_order_index(Avs2dCtx_t *p_dec)
{
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;
    Avs2dFrameMgr_t *mgr  = &p_dec->frm_mgr;
    RK_U32 i = 0;
    Avs2dFrame_t *p;

    //!< DOI should be a periodically-repeated value from 0 to 255
    if (mgr->output_poi != -1 &&
        ph->doi != (mgr->prev_doi + 1) % AVS2_DOI_CYCLE) {
        AVS2D_DBG(AVS2D_DBG_WARNNING, "discontinuous DOI (prev: %d --> curr: %d).", mgr->prev_doi, ph->doi);
        for (i = 0; i < mgr->dpb_size; i++) {
            p = mgr->dpb[i];
            if (p->slot_idx == NO_VAL) {
                continue;
            }

            if (MPP_ABS(ph->doi - (p->doi % AVS2_DOI_CYCLE)) > 1) {
                p->refered_by_others = 0;
                p->refered_by_scene = 0;
            }
        }
    }

    //!< update DOI
    if (ph->doi < mgr->prev_doi) {
        mgr->tr_wrap_cnt++;
    }

    mgr->prev_doi = ph->doi;
    ph->doi += mgr->tr_wrap_cnt * AVS2_DOI_CYCLE;
    recompute_cycle_order_index(mgr, ph);

    avs2d_dbg_dpb("low_delay %d, reorder_delay %d\n", vsh->low_delay, vsh->picture_reorder_delay);
    if (vsh->low_delay == 0) {
        ph->poi = ph->doi + ph->picture_output_delay - vsh->picture_reorder_delay;
    } else {
        ph->poi = ph->doi;
    }

    mpp_assert(ph->doi >= 0 && ph->poi >= 0);

    if (mgr->output_poi == NO_VAL && ph->picture_type == I_PICTURE) {
        mgr->output_poi = ph->poi;
    }

    avs2d_dbg_dpb("picuture DOI %d, POI %d, out_delay %d, output_poi %d\n", ph->doi, ph->poi, ph->picture_output_delay, mgr->output_poi);
}

static RK_S32 is_outputable_frame(Avs2dFrame_t *p)
{
    // if (p && p->slot_idx != NO_VAL && !p->invisible && !p->is_output && p->is_decoded) {
    if (p && p->slot_idx != NO_VAL && !p->invisible && !p->is_output) {
        return 1;
    }

    return 0;
}

static RK_S32 get_outputable_smallest_poi(Avs2dFrameMgr_t *mgr, RK_S32 *poi, RK_S32 *pos)
{
    RK_U32 i = 0;
    RK_S32 find_flag = 0;
    RK_S32 min_pos = -1;
    RK_S32 min_poi = INT_MAX;
    Avs2dFrame_t *p;

    *pos = -1;
    *poi = INT_MAX;
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];
        if (!is_outputable_frame(p)) {
            continue;
        }
        if (min_poi > p->poi) {
            min_poi = p->poi;
            min_pos = i;
        }
        if (*poi > p->poi && !p->is_output) {
            *poi = p->poi;
            *pos = i;
            find_flag = 1;
        }
    }
    if (!find_flag) {
        *poi = min_poi;
        *pos = min_pos;
    }

    return find_flag;
}

/**
 * @brief Ouput frame to display queue
 * Mark frame at DPB as output, but still remain at DPB
 *
 * @param p_dec
 * @param p
 * @return MPP_RET
 */
static MPP_RET output_display_frame(Avs2dCtx_t *p_dec, Avs2dFrame_t *p)
{
    MPP_RET ret = MPP_NOK;

    avs2d_dbg_dpb("In.");
    mpp_assert(p);
    // if (p->slot_idx >= 0 && !p->invisible && !p->is_output && p->is_decoded) {
    if (p->slot_idx >= 0 && !p->invisible && !p->is_output) {
        ret = MPP_OK;
        p->is_output = 1;
        p_dec->frm_mgr.output_poi = p->poi;
        mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_QUEUE_USE);
        mpp_buf_slot_enqueue(p_dec->frame_slots, p->slot_idx, QUEUE_DISPLAY);
        avs2d_dbg_dpb("output display frame poi %d slot_idx %d, pts %lld", p->poi, p->slot_idx,
                      mpp_frame_get_pts(p->frame));
    }

    avs2d_dbg_dpb("Out. ret = %d", ret);
    return ret;
}

/**
 * @brief Remove frame from DPB
 *
 * @param p_dec
 * @param p
 * @return MPP_RET
 */
static MPP_RET dpb_remove_frame(Avs2dCtx_t *p_dec, Avs2dFrame_t *p)
{
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;
    MppFrame frame = p->frame;

    avs2d_dbg_dpb("In.");

    if (p->picture_type == GB_PICTURE || p->invisible) {
        MppBuffer buffer = NULL;
        mpp_buf_slot_get_prop(p_dec->frame_slots, p->slot_idx, SLOT_BUFFER, &buffer);
        mpp_buffer_put(buffer);
    }

    mpp_buf_slot_clr_flag(p_dec->frame_slots, p->slot_idx, SLOT_CODEC_USE);
    avs2d_dbg_dpb("dpb remove frame slot_idx %d, doi %d poi %d, dpb used %d",
                  p->slot_idx, p->doi, p->poi, mgr->used_size);

    memset(p, 0, sizeof(Avs2dFrame_t));
    p->frame = frame;
    p->slot_idx = NO_VAL;
    p->doi = NO_VAL;
    p->poi = NO_VAL;
    mgr->used_size--;

    avs2d_dbg_dpb("Out.");
    return MPP_OK;
}

static RK_S32 is_refered(Avs2dFrame_t *p)
{
    mpp_assert(p);
    if (p->refered_by_others || p->refered_by_scene) {
        return 1;
    }

    return 0;
}

static void unmark_refered(Avs2dFrame_t *p)
{
    mpp_assert(p);
    p->refered_by_scene = 0;
    p->refered_by_others = 0;
}

static void unmark_other_refered(Avs2dFrame_t *p)
{
    p->refered_by_others = 0;
}

static MPP_RET dpb_remove_scene_frame(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    Avs2dFrame_t *p = NULL;
    Avs2dPicHeader_t *ph = &p_dec->ph;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    avs2d_dbg_dpb("In.");
    if (ph->picture_type == G_PICTURE || ph->picture_type == GB_PICTURE) {
        if (mgr->scene_ref) {
            p = mgr->scene_ref;
            p->refered_by_scene = 0;
            //!< remove GB frame directly
            if (p->picture_type == GB_PICTURE) {
                FUN_CHECK(ret = dpb_remove_frame(p_dec, p));
            }
            mgr->scene_ref = NULL;
        }
    }

__FAILED:
    avs2d_dbg_dpb("Out.");
    return ret;
}

MPP_RET dpb_remove_unused_frame(Avs2dCtx_t *p_dec)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_OK;
    Avs2dFrame_t *p = NULL;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];
        if (p->slot_idx == NO_VAL) {
            continue;
        }

        if ((MPP_ABS(p->poi - p_dec->ph.poi) >= AVS2_MAX_POC_DISTANCE)) {
            p->refered_by_others = 0;
            p->refered_by_scene = 0;
        }

        if ((p->is_output || p->invisible) && !is_refered(p)) {
            FUN_CHECK(ret = dpb_remove_frame(p_dec, p));
        }
    }

__FAILED:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

static Avs2dFrame_t *dpb_get_one_frame(Avs2dFrameMgr_t *mgr, Avs2dSeqHeader_t *vsh, Avs2dPicHeader_t *ph)
{
    RK_U32 i;
    Avs2dFrame_t *p = NULL;

    //!< get paired field frame
    if (vsh->field_coded_sequence) {
        for (i = 0; i < mgr->dpb_size; i++) {
            p = mgr->dpb[i];
            if (p->slot_idx == NO_VAL) {
                continue;
            }

            if (p->poi == ph->poi) {
                return p;
            }
        }
    }

    //!< get unused frame
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];
        if (p->slot_idx == NO_VAL) {
            break;
        }
    }

    return p;
}

static Avs2dFrame_t *dpb_alloc_frame(Avs2dCtx_t *p_dec, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    MppFrame mframe = NULL;
    Avs2dFrame_t *frm = NULL;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;
    Avs2dSeqExtHeader_t *exh  = &p_dec->exh;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    avs2d_dbg_dpb("In.");
    frm = dpb_get_one_frame(mgr, vsh, ph);
    if (!frm) {
        mpp_err("Failed to get dpb buffer.\n");
        return NULL;
    }

    mframe = frm->frame;
    frm->doi = ph->doi;
    frm->poi = ph->poi;
    frm->out_delay = ph->picture_output_delay;
    frm->picture_type = ph->picture_type;
    frm->invisible = (frm->picture_type == GB_PICTURE);
    frm->scene_frame_flag = (frm->picture_type == G_PICTURE || frm->picture_type == GB_PICTURE);
    frm->intra_frame_flag = (frm->scene_frame_flag || frm->picture_type == I_PICTURE);
    frm->refered_by_scene = frm->scene_frame_flag;
    frm->refered_by_others = (frm->picture_type != GB_PICTURE && mgr->cur_rps.refered_by_others);
    avs2d_dbg_dpb("frame picture type %d, ref by others %d\n", frm->picture_type, frm->refered_by_others);
    if (vsh->chroma_format == CHROMA_420 && vsh->bit_depth == 8) {
        mpp_frame_set_fmt(mframe, MPP_FMT_YUV420SP);
    } else if (vsh->chroma_format == CHROMA_420 && vsh->bit_depth == 10) {
        mpp_frame_set_fmt(mframe, MPP_FMT_YUV420SP_10BIT);
    }

    if (MPP_FRAME_FMT_IS_FBC(p_dec->init.cfg->base.out_fmt)) {
        // fbc header alignment
        RK_U32 fbc_hdr_stride = MPP_ALIGN(vsh->horizontal_size, 64);

        mpp_frame_set_fmt(mframe, mpp_frame_get_fmt(mframe) | (p_dec->init.cfg->base.out_fmt & (MPP_FRAME_FBC_MASK)));

        if (*compat_ext_fbc_hdr_256_odd)
            fbc_hdr_stride = MPP_ALIGN(vsh->horizontal_size, 256) | 256;

        mpp_frame_set_fbc_hdr_stride(mframe, fbc_hdr_stride);
        // fbc output frame update
        mpp_frame_set_offset_y(mframe, 8);
    } else if (MPP_FRAME_FMT_IS_TILE(p_dec->init.cfg->base.out_fmt))
        mpp_frame_set_fmt(mframe, mpp_frame_get_fmt(mframe) | (p_dec->init.cfg->base.out_fmt & (MPP_FRAME_TILE_FLAG)));

    if (p_dec->is_hdr)
        mpp_frame_set_fmt(mframe, mpp_frame_get_fmt(mframe) | MPP_FRAME_HDR);

    if (p_dec->init.cfg->base.enable_thumbnail && p_dec->init.hw_info->cap_down_scale)
        mpp_frame_set_thumbnail_en(mframe, p_dec->init.cfg->base.enable_thumbnail);
    else
        mpp_frame_set_thumbnail_en(mframe, 0);

    mpp_frame_set_width(mframe, vsh->horizontal_size);
    mpp_frame_set_height(mframe, vsh->vertical_size);
    mpp_frame_set_pts(mframe, mpp_packet_get_pts(task->input_packet));
    mpp_frame_set_dts(mframe, mpp_packet_get_dts(task->input_packet));
    mpp_frame_set_errinfo(mframe, 0);
    mpp_frame_set_discard(mframe, 0);
    mpp_frame_set_poc(mframe, frm->poi);
    if (p_dec->got_exh) {
        mpp_frame_set_color_primaries(mframe, exh->color_primaries);
        mpp_frame_set_color_trc(mframe, exh->transfer_characteristics);
        mpp_frame_set_colorspace(mframe, exh->matrix_coefficients);
    }
    mpp_frame_set_content_light(mframe, p_dec->content_light);
    mpp_frame_set_mastering_display(mframe, p_dec->display_meta);
    if (p_dec->hdr_dynamic_meta && p_dec->hdr_dynamic) {
        mpp_frame_set_hdr_dynamic_meta(mframe, p_dec->hdr_dynamic_meta);
        p_dec->hdr_dynamic = 0;
    }
    if (vsh->progressive_sequence) {
        frm->frame_mode = MPP_FRAME_FLAG_FRAME;
        frm->frame_coding_mode = MPP_FRAME_FLAG_FRAME;

        if (p_dec->init.cfg->base.enable_vproc & MPP_VPROC_MODE_DETECTION) {
            frm->frame_mode |= MPP_FRAME_FLAG_DEINTERLACED;
        }
    } else {
        frm->frame_mode = MPP_FRAME_FLAG_PAIRED_FIELD;
        if (vsh->field_coded_sequence) {
            frm->frame_coding_mode = MPP_FRAME_FLAG_PAIRED_FIELD;
            if (ph->top_field_first) {
                frm->frame_mode |= MPP_FRAME_FLAG_TOP_FIRST;
                frm->frame_coding_mode |= MPP_FRAME_FLAG_TOP_FIRST;
            } else {
                frm->frame_mode |= MPP_FRAME_FLAG_BOT_FIRST;
                frm->frame_coding_mode |= MPP_FRAME_FLAG_BOT_FIRST;
            }
        } else {
            frm->frame_coding_mode = MPP_FRAME_FLAG_FRAME;
            if (ph->top_field_first) {
                frm->frame_mode |= MPP_FRAME_FLAG_TOP_FIRST;
            } else {
                frm->frame_mode |= MPP_FRAME_FLAG_BOT_FIRST;
            }
        }
    }

    mpp_frame_set_mode(mframe, frm->frame_mode);
    ret = mpp_buf_slot_get_unused(p_dec->frame_slots, &frm->slot_idx);
    mpp_assert(ret == MPP_OK);
    mgr->used_size++;
    avs2d_dbg_dpb("get unused buf slot %d, DPB used %d \n", frm->slot_idx, mgr->used_size);
    avs2d_dbg_dpb("Out.");
    return frm;
}

static MPP_RET dpb_output_next_frame(Avs2dCtx_t *p_dec, RK_S32 continuous)
{
    MPP_RET ret = MPP_NOK;
    RK_S32 poi, pos;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");
    if (get_outputable_smallest_poi(mgr, &poi, &pos)) {
        avs2d_dbg_dpb("smallest poi %d, pos %d, output_poi %d\n",
                      poi, pos, mgr->output_poi);

        if ((poi - mgr->output_poi <= 1) ||
            (mgr->dpb[pos]->doi + mgr->dpb[pos]->out_delay < mgr->cur_frm->doi) ||
            !continuous) {
            FUN_CHECK(ret = output_display_frame(p_dec, mgr->dpb[pos]));
            if (!is_refered(mgr->dpb[pos])) {
                FUN_CHECK(ret = dpb_remove_frame(p_dec, mgr->dpb[pos]));
            }
        }
    }

    AVS2D_PARSE_TRACE("Out.");
__FAILED:
    return ret;
}

static Avs2dFrame_t *find_ref_frame(Avs2dFrameMgr_t *mgr, RK_S32 doi)
{
    RK_U32 i;
    Avs2dFrame_t *p = NULL;

    avs2d_dbg_dpb("In.");
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];

        if (p->slot_idx == NO_VAL || p->doi == NO_VAL) {
            continue;
        }

        if (p->doi >= 0 && doi == p->doi) {
            if (!is_refered(p)) {
                // DOI matched but not a referenced frame
                p->error_flag = 1;
                AVS2D_DBG(AVS2D_DBG_WARNNING, "invalid reference frame [doi: %d].\n", doi);
            }
            avs2d_dbg_dpb("found ref[%d] at slot_idx %d, doi %d", i, p->slot_idx, p->doi);
            return p;
        }
    }

    AVS2D_DBG(AVS2D_DBG_ERROR, "reference frame [doi: %d] missed.\n", doi);
    avs2d_dbg_dpb("Out.");
    return NULL;
}

MPP_RET dpb_update_refs(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    RK_S32 doi_of_remove;
    Avs2dFrame_t *p;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;
    Avs2dRps_t *p_rps = &mgr->cur_rps;
    avs2d_dbg_dpb("In.");

    if (!mgr->cur_frm) {
        ret = MPP_NOK;
        AVS2D_DBG(AVS2D_DBG_WARNNING, "No current frame to update dpb.\n");
        goto __FAILED;
    }

    //!< update scene reference frame
    if (mgr->cur_frm->refered_by_scene) {
        mgr->scene_ref = mgr->cur_frm;
    }

    //!< update reference frame flag
    for (i = 0; i < p_rps->num_to_remove; i++) {
        doi_of_remove = mgr->cur_frm->doi - p_rps->remove_pic[i];
        avs2d_dbg_dpb("current doi %d, remove_pic[%d]=%d", mgr->cur_frm->doi, i, p_rps->remove_pic[i]);
        p = find_ref_frame(mgr, doi_of_remove);
        if (p) {
            unmark_other_refered(p);
            avs2d_dbg_dpb("unmark picture refered, slot_idx %d, doi %d poi %d",
                          p->slot_idx, p->doi, p->poi);
        }
    }

__FAILED:
    avs2d_dbg_dpb("Out. ret %d", ret);
    return ret;
}

static MPP_RET dpb_set_frame_refs(Avs2dCtx_t *p_dec, Avs2dFrameMgr_t *mgr, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;
    RK_U8 error_flag = 0;
    RK_U8 num_of_ref;
    RK_S32 doi_of_ref;
    Avs2dRps_t *p_rps;
    Avs2dFrame_t *p_cur, *p;
    RK_U8 replace_ref_flag = 0;

    (void) task;

    avs2d_dbg_dpb("In.");
    memset(mgr->refs, 0, sizeof(mgr->refs));

    p_cur = mgr->cur_frm;
    mgr->num_of_ref = 0;
    p_rps = &mgr->cur_rps;
    num_of_ref = p_rps->num_of_ref;
    task->flags.ref_miss = 0;
    for (i = 0; i < num_of_ref; i++) {
        doi_of_ref = p_cur->doi - p_rps->ref_pic[i];
        p = find_ref_frame(mgr, doi_of_ref);
        if (!p) {
            task->flags.ref_miss |= 1 << i;
            avs2d_dbg_dpb("Missing ref doi %d", doi_of_ref);
        } else {
            mgr->refs[mgr->num_of_ref] = p;
            mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_CODEC_USE);
            mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_HAL_INPUT);
        }

        mgr->num_of_ref++;
    }

    if (mgr->num_of_ref < 1 && !p_cur->intra_frame_flag) {
        error_flag = 1;
    } else if (p_cur->picture_type == S_PICTURE) {
        p_cur->refered_bg_frame = 1;
        if (!mgr->scene_ref) {
            error_flag = 1;
        } else if (mgr->scene_ref != mgr->refs[0] || mgr->num_of_ref > 1) {
            AVS2D_DBG(AVS2D_DBG_ERROR, "Error reference frame(doi %ld ~ %ld) for S.\n",
                      mgr->scene_ref->doi, mgr->refs[0] ? mgr->refs[0]->doi : -1);
            replace_ref_flag = 1;
            p_dec->syntax.refp.scene_ref_replace_pos = 0;
        }
    } else if ((p_cur->picture_type == P_PICTURE || p_cur->picture_type == F_PICTURE) &&
               p_dec->ph.background_reference_flag) {
        p_cur->refered_bg_frame = 1;
        if (!mgr->scene_ref) {
            error_flag = 1;
        } else {
            replace_ref_flag = 1;
            p_dec->syntax.refp.scene_ref_replace_pos = mgr->num_of_ref - 1;
        }
    } else if (p_cur->picture_type == B_PICTURE &&
               (mgr->num_of_ref != 2 || (mgr->refs[0] && mgr->refs[0]->poi <= p_cur->poi) ||
                (mgr->refs[1] && mgr->refs[1]->poi >= p_cur->poi))) {
        error_flag = 1;
    }

    if (replace_ref_flag && mgr->scene_ref) {
        p_dec->syntax.refp.scene_ref_enable = 1;
        p_dec->syntax.refp.scene_ref_slot_idx = mgr->scene_ref->slot_idx;
    } else {
        p_dec->syntax.refp.scene_ref_enable = 0;
    }

    if (error_flag) {
        ret = MPP_NOK;
        mpp_frame_set_errinfo(p_cur->frame, MPP_FRAME_ERR_UNKNOW);
        AVS2D_DBG(AVS2D_DBG_ERROR, "Error reference frame for picture(%d).\n", p_cur->picture_type);
        goto __FAILED;
    }

__FAILED:
    avs2d_dbg_dpb("Out. ret %d", ret);
    return ret;
}

MPP_RET avs2d_dpb_insert(Avs2dCtx_t *p_dec, HalDecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    Avs2dFrame_t *p;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");

    compute_frame_order_index(p_dec);

    //!< output frame from dpb
    dpb_output_next_frame(p_dec, 1);

    //!< remove scene(G/GB) frame(scene dbp has only one G/GB)
    dpb_remove_scene_frame(p_dec);

    //!< remove unused dpb frame
    dpb_remove_unused_frame(p_dec);

    //!< set task with new frame
    p = dpb_alloc_frame(p_dec, task);
    p_dec->frame_no++;
    avs2d_dbg_dpb("get unused frame from dpb %d", p->slot_idx);
    if (!p) {
        ret = MPP_ERR_NOMEM;
        mpp_err("Failed to alloc dpb frame.\n");
        goto __FAILED;
    }

    mgr->cur_frm = p;
    mpp_buf_slot_set_prop(p_dec->frame_slots, p->slot_idx, SLOT_FRAME, p->frame);
    mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_CODEC_USE);
    mpp_buf_slot_set_flag(p_dec->frame_slots, p->slot_idx, SLOT_HAL_OUTPUT);

    //!< set task output
    task->output = mgr->cur_frm->slot_idx;
    avs2d_dbg_dpb("curframe slot_idx %d\n", mgr->cur_frm->slot_idx);

    //!< set task refers
    ret = dpb_set_frame_refs(p_dec, mgr, task);
    if (ret)
        task->flags.ref_err = 0;

    for (i = 0; i < mgr->num_of_ref; i++) {
        task->refer[i] = mgr->refs[i] ? mgr->refs[i]->slot_idx : -1;
        if (mgr->refs[i]) {
            task->refer[i] = mgr->refs[i]->slot_idx;
            avs2d_dbg_dpb("task refer[%d] slot_idx %d doi %d poi %d", i, task->refer[i], mgr->refs[i]->doi, mgr->refs[i]->poi);
        } else {
            task->refer[i] = -1;
            avs2d_dbg_dpb("task refer[%d] missing ref\n", i);
        }
    }

    //!< update dpb by rps
    dpb_update_refs(p_dec);

    avs2d_dbg_dpb("--------DPB INFO--------");
    avs2d_dbg_dpb("dpb_idx slt_idx doi poi type out refered\n");
    Avs2dFrame_t *tmp;
    for (i = 0; i < mgr->dpb_size; i++) {
        tmp = mgr->dpb[i];
        avs2d_dbg_dpb("%02d      %02d      %02d  %02d  %c    %d   %d",
                      i, tmp->slot_idx, tmp->doi, tmp->poi, PICTURE_TYPE_TO_CHAR(tmp->picture_type),
                      tmp->is_output, tmp->refered_by_others | tmp->refered_by_scene);
    }
    avs2d_dbg_dpb("------------------------");

__FAILED:
    AVS2D_PARSE_TRACE("Out.");
    return ret;
}

MPP_RET avs2d_dpb_flush(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    Avs2dFrame_t *p;
    Avs2dFrameMgr_t *mgr = &p_dec->frm_mgr;

    AVS2D_PARSE_TRACE("In.");
    //!< unmark all frames refered flag
    for (i = 0; i < mgr->dpb_size; i++) {
        p = mgr->dpb[i];
        if (p->slot_idx != NO_VAL) {
            unmark_refered(p);
        }
    }

    //!< remove all unused frame
    dpb_remove_unused_frame(p_dec);

    //!< output frames in POI order
    while (mgr->used_size) {
        if ((ret = dpb_output_next_frame(p_dec, 0)) != MPP_OK) {
            break;
        }
    }

    //!< reset dpb management
    dpb_init_management(mgr);

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}
