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

#define MODULE_TAG  "h264e_dpb"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "h264e_debug.h"
#include "h264e_dpb.h"
#include "h264e_slice.h"

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller)
{
    RK_S32 i = 0;

    mpp_log_f("dump dpb frame info in %s\n", caller);

    mpp_log_f("dpb %p total count %d size %d\n", dpb, dpb->total_cnt, dpb->dpb_size);
    mpp_log_f("dpb status - use seq gop idx type idr ref lt idx sta cnt\n");

    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];

        mpp_log_f("frm slot %2d  %-3d %-3d %-3d %-3d %-4d %-3d %-3d %-2d %-3d %-3x %-3d\n",
                  i,
                  frm->on_used,
                  frm->seq_idx,
                  frm->gop_cnt,
                  frm->gop_idx,
                  frm->frame_type,
                  frm->status.is_idr,
                  !frm->status.is_non_ref,
                  frm->status.is_lt_ref,
                  frm->lt_idx,
                  frm->ref_status,
                  frm->ref_count);
    }
}

void h264e_dpb_dump_listX(H264eDpbFrm **list, RK_S32 count)
{
    RK_S32 i;

    for (i = 0; i < count; i++) {
        H264eDpbFrm *frm = list[i];

        mpp_log_f("list slot %d   %-3d %-3d %-3d %-4d %-3d %-3d %-2d %-3d %-3x %-3d\n",
                  i,
                  frm->on_used,
                  frm->seq_idx,
                  frm->gop_idx,
                  frm->frame_type,
                  frm->status.is_idr,
                  !frm->status.is_non_ref,
                  frm->status.is_lt_ref,
                  frm->lt_idx,
                  frm->ref_status,
                  frm->ref_count);
    }
}

void h264e_dpb_dump_list(H264eDpb *dpb)
{
    mpp_log_f("dump dpb list info\n");

    mpp_log_f("dpb  size %d st size %d lt size %d\n",
              dpb->dpb_size, dpb->st_size, dpb->lt_size);

    if (dpb->dpb_size) {
        mpp_log_f("list status - use seq gop type idr ref lt idx sta cnt\n", dpb, dpb->total_cnt);

        h264e_dpb_dump_listX(dpb->list, dpb->dpb_size);
    }
}

static void h264e_dpb_mark_one_nonref(H264eDpb *dpb, H264eDpbFrm *ref)
{
    H264eDpbFrm *cur = dpb->curr;
    RK_S32 cur_frm_num = cur->frame_num;
    RK_S32 unref_frm_num = ref->frame_num;
    H264eMmco op;

    h264e_dbg_dpb("cur %d T%d mark ref %d gop [%d:%d] to be unreference dpb size %d\n",
                  cur->seq_idx, cur->status.temporal_id,
                  ref->seq_idx, ref->gop_cnt, ref->gop_idx, dpb->dpb_size);

    memset(&op, 0, sizeof(op));

    if (!ref->status.is_lt_ref) {
        RK_S32 difference_of_pic_nums_minus1 = MPP_ABS(unref_frm_num - cur_frm_num) - 1;

        h264e_dbg_mmco("cur_frm_num %d unref_frm_num %d\n", cur_frm_num, unref_frm_num);
        h264e_dbg_mmco("add mmco st 1 %d\n", difference_of_pic_nums_minus1);

        op.mmco = 1;
        op.difference_of_pic_nums_minus1 = difference_of_pic_nums_minus1;
    } else {
        h264e_dbg_mmco("add mmco lt 2 %d\n", ref->lt_idx);

        op.mmco = 2;
        op.long_term_frame_idx = ref->lt_idx;
    }

    h264e_marking_wr_op(dpb->marking, &op);
}

MPP_RET h264e_dpb_init(H264eDpb *dpb, H264eReorderInfo *reorder, H264eMarkingInfo *marking)
{
    RK_S32 i;

    h264e_dbg_dpb("enter %p\n", dpb);

    memset(dpb, 0, sizeof(*dpb));

    dpb->reorder    = reorder;
    dpb->marking    = marking;
    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(dpb->frames); i++)
        dpb->frames[i].slot_idx = i;

    h264e_dbg_dpb("leave %p\n", dpb);

    return MPP_OK;
}

MPP_RET h264e_dpb_copy(H264eDpb *dst, H264eDpb *src)
{
    h264e_dbg_dpb("enter dst %p src %p\n", dst, src);

    memcpy(dst, src, sizeof(*dst));

    h264e_dbg_dpb("leave\n");

    return MPP_OK;
}

MPP_RET h264e_dpb_set_cfg(H264eDpb *dpb, MppEncCfgSet* cfg, SynH264eSps *sps)
{
    MPP_RET ret = MPP_OK;
    RK_S32 i;

    h264e_dbg_dpb("enter %p\n", dpb);

    /* 1. setup reference frame number */
    RK_S32 ref_frm_num = sps->num_ref_frames;
    RK_S32 log2_max_frm_num = sps->log2_max_frame_num_minus4 + 4;
    RK_S32 log2_max_poc_lsb = sps->log2_max_poc_lsb_minus4 + 4;

    h264e_dbg_dpb("max  ref frm num %d\n", ref_frm_num);
    h264e_dbg_dpb("log2 max frm num %d\n", log2_max_frm_num);
    h264e_dbg_dpb("log2 max poc lsb %d\n", log2_max_poc_lsb);

    if (ref_frm_num < 0 || ref_frm_num > H264E_MAX_REFS_CNT ||
        log2_max_frm_num < 0 || log2_max_frm_num > 16 ||
        log2_max_poc_lsb < 0 || log2_max_poc_lsb > 16) {
        mpp_err_f("invalid config value %d %d %d\n", ref_frm_num,
                  log2_max_frm_num, log2_max_poc_lsb);

        ref_frm_num = 1;
        log2_max_frm_num = 16;
        log2_max_poc_lsb = 16;

        mpp_err_f("set to default value %d %d %d\n", ref_frm_num,
                  log2_max_frm_num, log2_max_poc_lsb);
    }

    mpp_assert(ref_frm_num <= H264E_MAX_REFS_CNT);

    dpb->max_frm_num    = (1 << log2_max_frm_num) - 1;
    dpb->max_poc_lsb    = (1 << log2_max_poc_lsb) - 1;
    dpb->total_cnt      = ref_frm_num + 1;

    /* 2. setup IDR gop (igop) */
    dpb->idr_gop_cnt = 0;
    dpb->idr_gop_idx = 0;
    dpb->idr_gop_len = cfg->rc.gop;

    /* 3. setup gop ref hierarchy (vgop) */
    MppEncGopRef *ref = &cfg->gop_ref;

    dpb->mode = 0;
    if (!ref->gop_cfg_enable) {
        /* set default dpb info */
        EncFrmStatus *info = &dpb->ref_inf[0];

        info[0].is_intra = 1;
        info[0].is_idr = 1;
        info[0].is_non_ref = 0;
        info[0].is_lt_ref = 0;
        info[0].lt_idx = -1;
        info[0].temporal_id = 0;
        info[0].ref_dist = -1;

        info[1].is_intra = 0;
        info[1].is_idr = 0;
        info[1].is_non_ref = 0;
        info[1].is_lt_ref = 0;
        info[1].lt_idx = -1;
        info[1].temporal_id = 0;
        info[1].ref_dist = -1;

        dpb->ref_cnt[0] = 2;
        dpb->ref_cnt[1] = 2;

        dpb->ref_dist[0] = -1;
        dpb->ref_dist[1] = -1;

        dpb->st_gop_len = 1;
        dpb->lt_gop_len = 0;

        goto GOP_CFG_DONE;
    }

    memset(dpb->ref_inf, 0, sizeof(dpb->ref_inf));
    memset(dpb->ref_sta, 0, sizeof(dpb->ref_sta));
    memset(dpb->ref_cnt, 0, sizeof(dpb->ref_cnt));
    memset(dpb->ref_dist, 0, sizeof(dpb->ref_dist));

    RK_S32 st_gop_len   = ref->ref_gop_len;
    RK_S32 lt_gop_len   = ref->lt_ref_interval;
    RK_S32 max_layer_id = 0;

    dpb->st_gop_len = st_gop_len;
    dpb->lt_gop_len = lt_gop_len;
    if (ref->max_lt_ref_cnt)
        dpb->max_lt_idx = ref->max_lt_ref_cnt - 1;
    else
        dpb->max_lt_idx = 0;

    h264e_dbg_dpb("st_gop_len %d lt_gop_len %d max_lt_idx_plus_1 %d\n",
                  dpb->st_gop_len, dpb->lt_gop_len,
                  dpb->max_lt_idx);

    if (st_gop_len)
        dpb->mode |= H264E_ST_GOP_FLAG;

    if (lt_gop_len) {
        dpb->mode |= H264E_LT_GOP_FLAG;
        mpp_assert(ref->max_lt_ref_cnt > 0);
    }

    RK_S32 max_lt_ref_idx = -1;

    for (i = 0; i < st_gop_len + 1; i++) {
        MppGopRefInfo *info = &ref->gop_info[i];
        RK_S32 is_non_ref   = info->is_non_ref;
        RK_S32 is_lt_ref    = info->is_lt_ref;
        RK_S32 temporal_id  = info->temporal_id;
        RK_S32 lt_idx       = info->lt_idx;
        RK_S32 ref_idx      = info->ref_idx;

        dpb->ref_inf[i].is_intra = (i == 0) ? (1) : (0);
        dpb->ref_inf[i].is_idr = dpb->ref_inf[i].is_intra;
        dpb->ref_inf[i].is_non_ref = is_non_ref;
        dpb->ref_inf[i].is_lt_ref = is_lt_ref;
        dpb->ref_inf[i].lt_idx = lt_idx;
        dpb->ref_inf[i].temporal_id = temporal_id;
        dpb->ref_inf[i].ref_dist = ref_idx - i;
        dpb->ref_dist[i] = ref_idx - i;

        if (!is_non_ref) {
            dpb->ref_sta[i] |= REF_BY_RECN(i);
            dpb->ref_cnt[i]++;
        }

        if (max_layer_id < temporal_id)
            max_layer_id = temporal_id;

        if (is_lt_ref) {
            if (lt_idx > max_lt_ref_idx) {
                max_lt_ref_idx = lt_idx;
                h264e_dbg_dpb("curr %d update lt_idx to %d\n",
                              i, max_lt_ref_idx);

                if (max_lt_ref_idx > dpb->max_lt_idx) {
                    mpp_err("mismatch max_lt_ref_idx_p1 %d vs %d\n",
                            max_lt_ref_idx, dpb->max_lt_idx);
                }
            }

            dpb->mode |= H264E_ST_GOP_WITH_LT_REF;

            if (lt_gop_len)
                mpp_err_f("Can NOT use both lt_ref_interval and gop_info lt_ref at the same time!\n ");
        }

        // update the reference frame status and counter only once
        if (ref_idx == i)
            continue;

        mpp_assert(!dpb->ref_inf[ref_idx].is_non_ref);
        dpb->ref_sta[ref_idx] |= REF_BY_REFR(i);
        dpb->ref_cnt[ref_idx]++;

        h264e_dbg_dpb("refr %d ref_status 0x%03x count %d\n",
                      ref_idx, dpb->ref_sta[ref_idx], dpb->ref_cnt[ref_idx]);
    }

GOP_CFG_DONE:
    h264e_dbg_dpb("leave %p\n", dpb);

    return ret;
}

MPP_RET h264e_dpb_set_curr(H264eDpb *dpb, H264eDpbFrmCfg *cfg)
{
    RK_S32 i;
    // current st gop info for update
    // st_gop_idx_wrap is for reference relationship index
    RK_S32 seq_idx = dpb->seq_idx++;
    RK_S32 idr_gop_cnt = dpb->idr_gop_cnt;
    RK_S32 idr_gop_idx = dpb->idr_gop_idx;
    RK_S32 st_gop_cnt = dpb->st_gop_cnt;
    RK_S32 st_gop_idx = dpb->st_gop_idx;
    RK_S32 st_gop_idx_wrap;
    RK_S32 lt_gop_cnt = dpb->lt_gop_cnt;
    RK_S32 lt_gop_idx = dpb->lt_gop_idx;
    RK_S32 ref_dist;
    RK_S32 lt_req;
    RK_S32 poc_lsb;
    RK_S32 idr_req;
    H264eDpbFrm *frm = NULL;
    H264eDpbFrm *ref = NULL;

    h264e_dbg_dpb("enter %p\n", dpb);
    /*
     * Step 1: Generate flag
     *
     * St gop structure should be reset by idr qop or lt gop.
     * It means when user request IDR frame or T0 long-term reference frame
     * the st gop struture should be reset to index 0.
     */
    if (cfg->force_idr) {
        idr_req = 1;
    } else {
        if (dpb->idr_gop_len == 0) {
            idr_req = (seq_idx) ? (0) : (1);
        } else if (dpb->idr_gop_len == 1) {
            idr_req = 1;
        } else {
            idr_req = (idr_gop_idx) ? (0) : (1);
        }
    }

    if ((cfg->force_lt_idx >= 0) ||
        (dpb->lt_gop_len && !dpb->lt_gop_idx))
        lt_req = 1;
    else
        lt_req = 0;

    h264e_dbg_dpb("prev %5d - gop i [%d:%d] l [%d:%d] s [%d:%d] -> idr  %d lt %d\n",
                  seq_idx, idr_gop_cnt, idr_gop_idx, lt_gop_cnt, lt_gop_idx,
                  st_gop_cnt, st_gop_idx, idr_req, lt_req);

    /*
     * step 2: Update short-term gop counter and idr gop counter
     */
    if (idr_req) {
        // update current and next st gop status
        // NOTE: st_gop_idx here is for next st_gop_idx and idr_req will reset st_gop_cnt
        dpb->st_gop_idx = 1;
        dpb->st_gop_cnt = 0;

        st_gop_cnt = 0;
        st_gop_idx = 0;

        // update idr gop status
        if (seq_idx)
            dpb->idr_gop_cnt++;

        idr_gop_idx = 0;
        dpb->idr_gop_idx = 1;

        dpb->poc_lsb = 2;
        poc_lsb = 0;
    } else {
        if (lt_req) {
            // NOTE: lt_req will not reset st_gop_cnt
            dpb->st_gop_idx = 1;
            dpb->st_gop_cnt++;

            st_gop_idx = 0;
            st_gop_cnt = dpb->st_gop_cnt;
        } else {
            st_gop_cnt = dpb->st_gop_cnt;
            st_gop_idx = dpb->st_gop_idx;

            dpb->st_gop_idx++;

            if (dpb->st_gop_idx >= dpb->st_gop_len) {
                dpb->st_gop_idx = 0;
                dpb->st_gop_cnt++;
            }
        }

        dpb->idr_gop_idx++;
        if (dpb->idr_gop_idx >= dpb->idr_gop_len)
            dpb->idr_gop_idx = 0;

        poc_lsb = dpb->poc_lsb;
        dpb->poc_lsb += 2;
        if (dpb->poc_lsb >= dpb->max_poc_lsb)
            dpb->poc_lsb = 0;
    }

    /*
     * step 3: Update long-term gop counter and idr gop counter
     */
    if (lt_req) {
        lt_gop_idx = 0;
        dpb->lt_gop_idx = 1;
        dpb->lt_gop_cnt++;
    } else {
        if (dpb->lt_gop_len) {
            dpb->lt_gop_idx++;
            if (dpb->lt_gop_idx >= dpb->lt_gop_len) {
                dpb->lt_gop_idx = 0;
                dpb->lt_gop_cnt++;
            }

            lt_gop_idx = dpb->lt_gop_idx;
        }
    }

    /*
     * step 4: Get wrapped short-term index for getting frame info in gop
     */
    if (!st_gop_idx && st_gop_cnt && !lt_req)
        st_gop_idx_wrap = dpb->st_gop_len;
    else
        st_gop_idx_wrap = st_gop_idx;

    h264e_dbg_dpb("curr %5d - gop i [%d:%d] l [%d:%d] s [%d:%d] -> warp %d poc %d\n",
                  seq_idx, idr_gop_cnt, idr_gop_idx, lt_gop_cnt, lt_gop_idx,
                  st_gop_cnt, st_gop_idx, st_gop_idx_wrap, poc_lsb);

    h264e_dbg_dpb("next %5d - gop i [%d:%d] l [%d:%d] s [%d:%d]\n",
                  dpb->seq_idx, dpb->idr_gop_cnt, dpb->idr_gop_idx,
                  dpb->lt_gop_cnt, dpb->lt_gop_idx, dpb->st_gop_cnt,
                  dpb->st_gop_idx);

    /*
     * step 5: Get ref_dist from short-term gop info and find reference frame
     */
    ref_dist = dpb->ref_dist[st_gop_idx_wrap];

    if (cfg->force_ref_lt_idx < 0) {
        if (!idr_req) {
            // normal lt ref loop case
            RK_S32 ref_seq_idx = seq_idx + ref_dist;

            for (i = 0; i < dpb->total_cnt; i++) {
                H264eDpbFrm *tmp = &dpb->frames[i];

                if (!tmp->on_used)
                    continue;

                if (tmp->status.is_non_ref)
                    continue;

                if (tmp->seq_idx == ref_seq_idx) {
                    ref = tmp;
                    h264e_dbg_dpb("frm: %5d found dist %d refer frm %d of %p\n",
                                  seq_idx, ref_dist, ref_seq_idx, ref);
                    break;
                }
            }

            if (NULL == ref) {
                mpp_err_f("failed to find refernce frame %d for current frame %d dist %d\n",
                          ref_seq_idx, seq_idx, ref_dist);
                h264e_dpb_dump_frms(dpb);
            }
        }
    } else {
        // force long-term reference frame as reference frame case
        /* step 5.1: find the long-term reference frame */
        // init list
        // 5.1 found all short term and long term ref
        RK_S32 lt_ref_idx = cfg->force_ref_lt_idx;

        for (i = 0; i < dpb->total_cnt; i++) {
            H264eDpbFrm *tmp = &dpb->frames[i];

            if (!tmp->on_used)
                continue;

            if (tmp->status.is_non_ref)
                continue;

            if (tmp->status.is_lt_ref && lt_ref_idx == tmp->status.lt_idx) {
                h264e_dbg_dpb("found lt_ref_idx %d frm_cnt %d\n",
                              lt_ref_idx, tmp->seq_idx);
                ref = tmp;
                break;
            }
        }

        if (ref == NULL)
            mpp_log_f("failed to find reference frame lt_idx %d\n", lt_ref_idx);

        /* step 5.2: mark all short-term reference frame as non-referenced */
        for (i = 0; i < dpb->total_cnt; i++) {
            H264eDpbFrm *tmp = &dpb->frames[i];

            if (!tmp->on_used)
                continue;

            if (tmp->status.is_non_ref)
                continue;

            // remove short-term reference frame in last lt_gop
            if (!tmp->status.is_lt_ref) {
                tmp->ref_status = 0;
                tmp->ref_count = 0;
            }
        }

        /* step 5.3: set current frame as short-term gop start */
        dpb->st_gop_idx = 1;
        dpb->st_gop_cnt++;

        st_gop_idx = 0;
        st_gop_cnt = dpb->st_gop_cnt;
    }

    dpb->refr = ref;

    /*
     * step 6: Clear dpb slot for next frame
     *
     * When IDR frame clear all dpb slots.
     * When force idr clear previous short-term reference frame
     */
    if (idr_req) {
        // init to 0
        dpb->last_frm_num = 0;

        // mmco 5 mark all reference frame to be non-referenced
        for (i = 0; i < dpb->total_cnt; i++) {
            H264eDpbFrm *tmp = &dpb->frames[i];

            tmp->on_used = 0;
            tmp->status.is_intra = 0;
            tmp->status.is_idr = 0;
            tmp->status.is_non_ref = 1;
            tmp->status.is_lt_ref = 0;
            tmp->status.lt_idx = 0;
            tmp->status.temporal_id = 0;
            tmp->status.ref_dist = 0;

            tmp->ref_status = 0;
            tmp->ref_count = 0;
        }

        dpb->dpb_size = 0;
        dpb->lt_size = 0;
        dpb->st_size = 0;
    }

    /*
     * step 7: Find a slot for current frame
     */
    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *tmp = &dpb->frames[i];

        if (!tmp->on_used) {
            frm = tmp;
            dpb->curr = tmp;

            // found empty slot and clear status
            frm->on_used = 1;
            frm->ref_status = 0;
            frm->ref_count  = 0;
            memset(&frm->status, 0, sizeof(frm->status));
            break;
        }
    }

    /*
     * step 8: Update known info to current frame
     */
    frm->seq_idx    = seq_idx;
    frm->gop_cnt    = st_gop_cnt;
    frm->gop_idx    = st_gop_idx;
    // NOTE: info update 1 - update from default ref info
    frm->status       = dpb->ref_inf[st_gop_idx_wrap];
    frm->ref_status = dpb->ref_sta[st_gop_idx];
    frm->ref_count  = dpb->ref_cnt[st_gop_idx];
    frm->ref_dist   = dpb->ref_dist[st_gop_idx_wrap];
    frm->poc        = poc_lsb;

    // NOTE: current frame info should be updated by previous flag
    if (idr_req) {
        // NOTE: info update 2 - update by idr frame
        frm->status.is_idr = 1;
        frm->status.is_intra = 1;
        frm->status.is_non_ref = 0;
    }

    if (lt_req) {
        // NOTE: info update 3 - update by current frame LT request
        frm->status.is_non_ref = 0;
        frm->status.is_lt_ref = 1;

        if (cfg->force_ref_lt_idx >= 0) {
            // force LTR
            frm->status.lt_idx = cfg->force_ref_lt_idx;
            frm->status.temporal_id = 0;
        } else if (dpb->lt_gop_len && !dpb->lt_gop_idx) {
            // auto LTR loop
            frm->status.lt_idx = dpb->lt_ref_idx;
            frm->status.temporal_id = 0;

            dpb->lt_ref_idx++;
            if (dpb->lt_ref_idx > dpb->max_lt_idx)
                dpb->lt_ref_idx = 0;
        }
    }

    // update ref_dist for reorder
    if (cfg->force_ref_lt_idx >= 0) {
        // NOTE: info update 4 - update by force refer to LTR
        frm->ref_dist = ref->seq_idx - seq_idx;
    }

    // update ref_dist in current frame info
    frm->status.ref_dist = frm->ref_dist;
    // NOTE: info update all done here

    frm->lt_idx = (frm->status.is_lt_ref) ? (frm->status.lt_idx) : (-1);
    frm->frame_num = dpb->last_frm_num;
    frm->frame_type = (idr_req) ? (H264_I_SLICE) : (H264_P_SLICE);

    if (!frm->status.is_non_ref)
        dpb->last_frm_num++;

    h264e_dbg_dpb("frm: %5d gop [%d:%d] frm_num [%d:%d] poc %d R%dL%d ref %d\n",
                  frm->seq_idx, frm->gop_cnt, frm->gop_idx,
                  frm->frame_num, dpb->last_frm_num, frm->poc,
                  !frm->status.is_non_ref, frm->status.is_lt_ref, frm->ref_dist);

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    h264e_dbg_dpb("leave %p\n", dpb);

    return MPP_OK;
}

static int cmp_st_list(const void *p0, const void *p1)
{
    H264eDpbFrm *frm0 = *(H264eDpbFrm **)p0;
    H264eDpbFrm *frm1 = *(H264eDpbFrm **)p1;

    if (frm0->frame_num == frm1->frame_num)
        return 0;
    if (frm0->frame_num < frm1->frame_num)
        return 1;
    else
        return -1;
}

static int cmp_lt_list(const void *p0, const void *p1)
{
    H264eDpbFrm *frm0 = *(H264eDpbFrm **)p0;
    H264eDpbFrm *frm1 = *(H264eDpbFrm **)p1;

    if (frm0->lt_idx == frm1->lt_idx)
        return 0;
    if (frm0->lt_idx > frm1->lt_idx)
        return 1;
    else
        return -1;
}

/*
 * Build list function
 *
 * This function should build the default list for current frame.
 * Then check the reference frame is the default one or not.
 * Reorder command is need if the reference frame is not match.
 */
void h264e_dpb_build_list(H264eDpb *dpb)
{
    RK_S32 i, j;
    RK_S32 st_size = 0;
    RK_S32 lt_size = 0;

    if (dpb->curr->status.is_intra)
        return ;

    h264e_dbg_dpb("enter %p\n", dpb);

    // init list
    // 1. found all short term and long term ref
    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];

        if (!frm->on_used)
            continue;

        if (frm == dpb->curr)
            continue;

        if (frm->status.is_non_ref)
            continue;

        h264e_dbg_list("idx %d on_used %x ref_status %03x is_non_ref %d lt_ref %d\n",
                       i, frm->on_used, frm->ref_status,
                       frm->status.is_non_ref, frm->status.is_lt_ref);

        if (!frm->status.is_non_ref) {
            if (!frm->status.is_lt_ref) {
                dpb->stref[st_size] = frm;
                st_size++;
                h264e_dbg_list("found st %d st_size %d %p\n", i, st_size, frm);
            } else {
                dpb->ltref[lt_size] = frm;
                lt_size++;
                h264e_dbg_list("found lt %d lt_size %d %p\n", i, lt_size, frm);
            }
        }
    }

    h264e_dbg_dpb("dpb_size %d st_size %d lt_size %d\n", dpb->dpb_size, st_size, lt_size);

    // sort st list
    if (st_size > 1) {
        if (h264e_debug & H264E_DBG_LIST) {
            mpp_log_f("dpb st list before sort\n");
            h264e_dpb_dump_listX(dpb->stref, st_size);
        }

        qsort(dpb->stref, st_size, sizeof(dpb->stref[0]), cmp_st_list);

        if (h264e_debug & H264E_DBG_LIST) {
            mpp_log_f("dpb st list after  sort\n");
            h264e_dpb_dump_listX(dpb->stref, st_size);
        }
    }

    if (lt_size > 1) {
        if (h264e_debug & H264E_DBG_LIST) {
            mpp_log_f("dpb lt list before sort\n");
            h264e_dpb_dump_listX(dpb->ltref, lt_size);
        }

        qsort(dpb->ltref, lt_size, sizeof(dpb->ltref[0]), cmp_lt_list);

        if (h264e_debug & H264E_DBG_LIST) {
            mpp_log_f("dpb lt list after  sort\n");
            h264e_dpb_dump_listX(dpb->ltref, lt_size);
        }
    }

    // generate list before reorder
    memset(dpb->list, 0, sizeof(dpb->list));
    j = 0;
    for (i = 0; i < st_size; i++)
        dpb->list[j++] = dpb->stref[i];

    for (i = 0; i < lt_size; i++)
        dpb->list[j++] = dpb->ltref[i];

    dpb->st_size = st_size;
    dpb->lt_size = lt_size;

    mpp_assert(dpb->dpb_size == st_size + lt_size);

    if (h264e_debug & H264E_DBG_LIST)
        h264e_dpb_dump_list(dpb);

    if (dpb->st_size + dpb->lt_size) {
        H264eDpbFrm *curr = dpb->curr;
        H264eDpbFrm *refr = dpb->refr;
        H264eDpbFrm *def_ref = dpb->list[0];

        RK_S32 curr_frm_cnt = curr->seq_idx;
        RK_S32 def_ref_frm_cnt = def_ref->seq_idx;
        RK_S32 set_ref_frm_cnt = curr->seq_idx + curr->ref_dist;

        h264e_dbg_list("refer curr %d def %d set %d reorder %d\n",
                       curr_frm_cnt, def_ref_frm_cnt, set_ref_frm_cnt,
                       (def_ref_frm_cnt != set_ref_frm_cnt));

        if (def_ref_frm_cnt != set_ref_frm_cnt) {
            H264eRplmo op;

            h264e_dbg_list("reorder to frm %d gop %d idx %d\n",
                           refr->seq_idx, refr->gop_cnt, refr->gop_idx);

            mpp_assert(!refr->status.is_non_ref);

            op.modification_of_pic_nums_idc = (refr->status.is_lt_ref) ? (2) : (0);
            if (refr->status.is_lt_ref) {
                op.modification_of_pic_nums_idc = 2;
                op.long_term_pic_idx = refr->lt_idx;

                h264e_dbg_list("reorder lt idx %d \n", op.long_term_pic_idx);
            } else {
                /* Only support refr pic num less than current pic num case */
                op.modification_of_pic_nums_idc = 0;
                op.abs_diff_pic_num_minus1 = MPP_ABS(curr->frame_num - refr->frame_num) - 1;

                h264e_dbg_list("reorder st cur %d refr %d diff - 1 %d\n",
                               curr->frame_num, refr->frame_num,
                               op.abs_diff_pic_num_minus1);
            }

            h264e_reorder_wr_op(dpb->reorder, &op);
        }
    } else {
        h264e_dbg_list("refer NULL\n");
    }

    h264e_dbg_dpb("leave %p\n", dpb);
}

void h264e_dpb_build_marking(H264eDpb *dpb)
{
    RK_S32 i;
    H264eDpbFrm *frm = dpb->curr;
    H264eDpbFrm *ref = dpb->refr;
    H264eMarkingInfo *marking = dpb->marking;

    h264e_dbg_dpb("enter %p\n", dpb);

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    // refernce frame can not mark itself as unreferenced.
    // So do NOT clear RECN flag here
    // clear the ref status and count once
    if (ref && frm != ref) {
        ref->ref_status &= ~(REF_BY_REFR(frm->gop_idx));

        if (ref->ref_count > 0)
            ref->ref_count--;

        h264e_dbg_dpb("refr %d ref_status %03x count %d\n",
                      ref->gop_idx, ref->ref_status, ref->ref_count);
    }

    if (!frm->status.is_non_ref && frm->status.is_lt_ref) {
        h264e_dbg_dpb("found lt idx %d curr max %d\n", frm->lt_idx, dpb->curr_max_lt_idx);
        if (frm->lt_idx > dpb->curr_max_lt_idx) {
            dpb->next_max_lt_idx = frm->lt_idx;
        }
    }

    if (frm->status.is_idr) {
        marking->idr_flag = 1;
        marking->no_output_of_prior_pics = 1;
        marking->long_term_reference_flag = frm->status.is_lt_ref;
        goto DONE;
    }

    marking->idr_flag = 0;
    marking->adaptive_ref_pic_buffering = 0;

    h264e_dbg_dpb("curr -- frm %d gop s [%d:%d] ref %d lt %d T%d\n",
                  frm->seq_idx, frm->gop_cnt, frm->gop_idx,
                  !frm->status.is_non_ref, frm->status.is_lt_ref,
                  frm->status.temporal_id);

    if (frm->status.is_non_ref)
        goto DONE;

    // When current frame is lt_ref update max_lt_idx
    if (frm->status.is_lt_ref) {
        H264eMmco op;

        if (dpb->next_max_lt_idx != dpb->curr_max_lt_idx) {
            RK_S32 max_lt_idx_p1 = dpb->next_max_lt_idx + 1;

            op.mmco = 4;
            op.max_long_term_frame_idx_plus1 = max_lt_idx_p1;

            h264e_marking_wr_op(marking, &op);
            h264e_dbg_mmco("add mmco 4 %d\n", max_lt_idx_p1);

            dpb->curr_max_lt_idx = dpb->next_max_lt_idx;
        }

        op.mmco = 6;
        op.long_term_frame_idx = frm->lt_idx;

        h264e_marking_wr_op(marking, &op);
        h264e_dbg_mmco("add mmco 6 %d\n", frm->lt_idx);
    }

    // if we are reference frame scan the previous frame to mark
    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *tmp = &dpb->frames[i];

        if (!tmp->on_used)
            continue;

        if (tmp->status.is_non_ref)
            continue;

        /*
         * NOTE: In ffmpeg we can not mark reference frame to be
         * non-reference frame. But in JM it is valid for mmco execution
         * is after frame decoding. So for compatibility consideration
         * we do NOT remove current reference frame from dpb.
         * That is the reason for frm->gop_index != ref_index
         *
         * There is a case for the first frame of a new gop which is not
         * first frame of the whole sequeuce. On this case we need to
         * remove frame in previous gop
         *
         * Tsvc temporal hierarchy dependency require the reference
         * must be marked non-reference frame by the frame with same
         * temporal layer id.
         */
        if ((tmp == ref) || (tmp == frm))
            continue;

        h264e_dbg_dpb("slot %2d frm %3d gop %3d idx %2d T%d lt %d cnt %d\n",
                      i, tmp->seq_idx, tmp->gop_cnt, tmp->gop_idx,
                      tmp->status.temporal_id, tmp->status.is_lt_ref, tmp->ref_count);

        if (tmp->status.is_lt_ref) {
            if (frm->status.is_lt_ref && frm->lt_idx == tmp->lt_idx) {
                h264e_dbg_dpb("frm %3d lt idx %d replace frm %d dpb size %d\n",
                              frm->seq_idx, tmp->lt_idx, tmp->seq_idx,
                              dpb->dpb_size);
            }

            continue;
        }

        if (!tmp->ref_count && tmp->status.temporal_id == frm->status.temporal_id) {
            h264e_dpb_mark_one_nonref(dpb, tmp);

            /* update frame status here */
            tmp->status.is_non_ref = 1;
            tmp->on_used = 0;

            h264e_dbg_mmco("set frm %d gop %d idx %d to not used\n",
                           tmp->seq_idx, tmp->gop_cnt, tmp->gop_idx);

            // decrease dpb size here for sometime frame will be remove later
            dpb->dpb_size--;
        }
    }

DONE:
    h264e_dbg_dpb("dpb size %d\n", dpb->dpb_size);

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    h264e_dbg_dpb("leave %p\n", dpb);
}

void h264e_dpb_curr_ready(H264eDpb *dpb)
{
    H264eDpbFrm *frm = dpb->curr;
    H264eDpbFrm *ref = dpb->refr;

    h264e_dbg_dpb("enter %p\n", dpb);

    if (ref)
        h264e_dbg_dpb("curr %d gop %d idx %d refr -> frm %d gop %d idx %d ready\n",
                      frm->seq_idx, frm->gop_cnt, frm->gop_idx,
                      ref->seq_idx, ref->gop_cnt, ref->gop_idx,
                      frm->status.is_non_ref);
    else
        h264e_dbg_dpb("curr %d gop %d idx %d refr -> non ready\n",
                      frm->seq_idx, frm->gop_cnt, frm->gop_idx);

    if (!frm->status.is_non_ref) {
        frm->ref_status &= ~(REF_BY_RECN(frm->gop_idx));
        frm->ref_count--;
        dpb->dpb_size++;

        // FIXME: sliding window marking should be done here
    } else
        frm->on_used = 0;

    // on swap mode just mark reference frame as not used
    if (dpb->mode == 0 && ref) {
        ref->on_used = 0;
        dpb->dpb_size--;
    }

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    dpb->curr = NULL;
    dpb->refr = NULL;

    h264e_dbg_dpb("leave %p\n", dpb);
}

