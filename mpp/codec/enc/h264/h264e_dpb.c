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

void h264e_dpb_dump_usage(H264eDpb *dpb, const char *fmt)
{
    RK_S32 i = 0;
    char buf[256];
    RK_S32 pos = 0;

    pos += snprintf(buf, sizeof(buf) - 1, "total %2d ", dpb->total_cnt);

    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];

        pos += snprintf(buf + pos, sizeof(buf) - 1 - pos, "%04x ", frm->on_used);
    }
    mpp_log("%s %s", fmt, buf);
}

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller, RK_S32 line)
{
    RK_S32 i = 0;

    mpp_log_f("dump dpb frame info in %s line %d\n", caller, line);

    mpp_log_f("dpb %p total count %d size %d\n", dpb, dpb->total_cnt, dpb->dpb_size);
    mpp_log_f("dpb slot use seq type tid ref idx mode arg\n");

    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];
        EncFrmStatus *status = &frm->status;

        mpp_log_f("frm  %2d   %d  %-3d %s    %-3d %-3s %-3d %-4x %-3d\n",
                  i, frm->on_used, status->seq_idx,
                  (status->is_intra) ? (status->is_idr ? "I" : "i" ) :
                      status->is_non_ref ? "p" : "P",
                      status->temporal_id,
                      status->is_non_ref ? "non" : status->is_lt_ref ? "lt" : "st",
                      status->lt_idx,
                      status->ref_mode,
                      status->ref_arg);
    }
}

void h264e_dpb_dump_listX(H264eDpbFrm **list, RK_S32 count)
{
    RK_S32 i;

    for (i = 0; i < count; i++) {
        H264eDpbFrm *frm = list[i];
        EncFrmStatus *status = &frm->status;

        mpp_log_f("frm  %2d   %d  %-3d %s    %-3d %-3s %-3d %-4x %-3d\n",
                  i, frm->on_used, status->seq_idx,
                  (status->is_intra) ? (status->is_idr ? "I" : "i" ) :
                      status->is_non_ref ? "p" : "P",
                      status->temporal_id,
                      status->is_non_ref ? "non" : status->is_lt_ref ? "lt" : "st",
                      status->lt_idx,
                      status->ref_mode,
                      status->ref_arg);
    }
}

void h264e_dpb_dump_list(H264eDpb *dpb)
{
    mpp_log_f("dump dpb list info\n");

    mpp_log_f("dpb  size %d used %d st %d lt %d\n",
              dpb->dpb_size, dpb->used_size, dpb->st_size, dpb->lt_size);

    if (dpb->st_size + dpb->lt_size) {
        mpp_log_f("list slot use seq type tid ref idx mode arg\n", dpb, dpb->total_cnt);

        h264e_dpb_dump_listX(dpb->list, dpb->st_size + dpb->lt_size);
    }
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

MPP_RET h264e_dpb_setup(H264eDpb *dpb, MppEncCfgSet* cfg, H264eSps *sps)
{
    MPP_RET ret = MPP_OK;
    MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(cfg->ref_cfg);

    h264e_dbg_dpb("enter %p\n", dpb);

    RK_S32 ref_frm_num = sps->num_ref_frames;
    RK_S32 log2_max_frm_num = sps->log2_max_frame_num_minus4 + 4;
    RK_S32 log2_max_poc_lsb = sps->log2_max_poc_lsb_minus4 + 4;

    /* NOTE: new configure needs to clear dpb first */
    h264e_dpb_init(dpb, dpb->reorder, dpb->marking);

    memcpy(&dpb->info, info, sizeof(dpb->info));
    dpb->dpb_size = info->dpb_size;
    dpb->total_cnt = info->dpb_size + 1;
    dpb->max_frm_num = 1 << log2_max_frm_num;
    dpb->max_poc_lsb = (1 << log2_max_poc_lsb);
    dpb->poc_type = sps->pic_order_cnt_type;

    if (cfg->hw.extra_buf)
        dpb->total_cnt++;

    h264e_dbg_dpb("max  ref frm num %d total slot %d\n",
                  ref_frm_num, dpb->total_cnt);
    h264e_dbg_dpb("log2 max frm num %d -> %d\n",
                  log2_max_frm_num, dpb->max_frm_num);
    h264e_dbg_dpb("log2 max poc lsb %d -> %d\n",
                  log2_max_poc_lsb, dpb->max_poc_lsb);

    h264e_dbg_dpb("leave %p\n", dpb);

    return ret;
}

H264eDpbFrm *find_cpb_frame(H264eDpb *dpb, EncFrmStatus *frm)
{
    H264eDpbFrm *frms = dpb->frames;
    RK_S32 seq_idx = frm->seq_idx;
    RK_S32 cnt = dpb->total_cnt;
    RK_S32 i;

    if (!frm->valid)
        return NULL;

    h264e_dbg_dpb("frm %d start finding slot\n", frm->seq_idx);

    for (i = 0; i < cnt; i++) {
        EncFrmStatus *p = &frms[i].status;

        if (p->valid && p->seq_idx == seq_idx) {
            h264e_dbg_dpb("frm %d match slot %d valid %d\n",
                          p->seq_idx, i, p->valid);

            mpp_assert(p->is_non_ref == frm->is_non_ref);
            mpp_assert(p->is_lt_ref == frm->is_lt_ref);
            mpp_assert(p->lt_idx == frm->lt_idx);
            mpp_assert(p->temporal_id == frm->temporal_id);
            return &frms[i];
        }
    }

    mpp_err_f("can not find match frm %d\n", seq_idx);
    h264e_dpb_dump_frms(dpb);
    abort();

    return NULL;
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
void h264e_dpb_build_list(H264eDpb *dpb, EncCpbStatus *cpb)
{
    RK_S32 i, j;
    RK_S32 st_size = 0;
    RK_S32 lt_size = 0;

    h264e_dbg_dpb("enter %p\n", dpb);

    /* clear list */
    memset(dpb->list, 0, sizeof(dpb->list));

    if (cpb->curr.is_intra) {
        h264e_dbg_dpb("leave %p\n", dpb);
        return ;
    }

    // 2.1 init list
    // 2.1.1 found all short term and long term ref
    h264e_dbg_list("cpb init scaning start\n");

    for (i = 0; i < MAX_CPB_REFS; i++) {
        EncFrmStatus *frm = &cpb->init[i];

        if (!frm->valid)
            continue;

        mpp_assert(!frm->is_non_ref);

        h264e_dbg_list("idx %d frm %d valid %d is_non_ref %d lt_ref %d\n",
                       i, frm->seq_idx, frm->valid, frm->is_non_ref, frm->is_lt_ref);

        H264eDpbFrm *p = find_cpb_frame(dpb, frm);
        if (!frm->is_lt_ref) {
            dpb->stref[st_size++] = p;
            p->status.val = frm->val;
            h264e_dbg_list("found st %d st_size %d %p\n", i, st_size, frm);
        } else {
            dpb->ltref[lt_size++] = p;
            p->status.val = frm->val;
            h264e_dbg_list("found lt %d lt_size %d %p\n", i, lt_size, frm);
        }
    }

    h264e_dbg_list("cpb init scaning done\n");
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

    mpp_assert(dpb->dpb_size >= st_size + lt_size);

    if (h264e_debug & H264E_DBG_LIST)
        h264e_dpb_dump_list(dpb);

    /* generate syntax */
    h264e_reorder_wr_rewind(dpb->reorder);

    if (dpb->st_size + dpb->lt_size) {
        H264eDpbFrm *curr = dpb->curr;
        H264eDpbFrm *refr = dpb->refr;
        H264eDpbFrm *def_ref = dpb->list[0];

        RK_S32 curr_frm_cnt = curr->status.seq_idx;
        RK_S32 set_ref_frm_cnt = refr->status.seq_idx;
        RK_S32 def_ref_frm_cnt = def_ref->status.seq_idx;

        h264e_dbg_list("refer curr %d def %d set %d reorder %d\n",
                       curr_frm_cnt, def_ref_frm_cnt, set_ref_frm_cnt,
                       (def_ref_frm_cnt != set_ref_frm_cnt));

        if (def_ref_frm_cnt != set_ref_frm_cnt) {
            H264eRplmo op;

            h264e_dbg_list("reorder to frm %d\n", refr->status.seq_idx);

            mpp_assert(!refr->status.is_non_ref);

            op.modification_of_pic_nums_idc = (refr->status.is_lt_ref) ? (2) : (0);
            if (refr->status.is_lt_ref) {
                op.modification_of_pic_nums_idc = 2;
                op.long_term_pic_idx = refr->status.lt_idx;

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
    H264eDpbFrm *frm = dpb->curr;
    H264eMarkingInfo *marking = dpb->marking;

    h264e_dbg_dpb("enter %p\n", dpb);

    h264e_marking_wr_rewind(marking);

    // refernce frame can not mark itself as unreferenced.
    if (frm->status.is_idr) {
        marking->idr_flag = 1;
        marking->no_output_of_prior_pics = 0;
        marking->long_term_reference_flag = frm->status.is_lt_ref;
        goto DONE;
    }

    marking->idr_flag = 0;
    marking->long_term_reference_flag = 0;
    marking->adaptive_ref_pic_buffering = 0;

    h264e_dbg_dpb("frm %d ref %d lt %d T%d\n",
                  frm->status.seq_idx, !frm->status.is_non_ref,
                  frm->status.is_lt_ref, frm->status.temporal_id);

    if (frm->status.is_non_ref)
        goto DONE;

    // When current frame is lt_ref update max_lt_idx
    if (frm->status.is_lt_ref) {
        H264eMmco op;

        dpb->next_max_lt_idx = dpb->info.max_lt_idx;

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

    // TODO: check and mark used frame
DONE:
    h264e_dbg_dpb("dpb size %d used %d\n", dpb->dpb_size, dpb->used_size);
    h264e_dbg_dpb("leave %p\n", dpb);
}

MPP_RET h264e_dpb_proc(H264eDpb *dpb, EncCpbStatus *cpb)
{
    EncFrmStatus *curr = &cpb->curr;
    EncFrmStatus *refr = &cpb->refr;
    EncFrmStatus *init = cpb->init;
    H264eDpbFrm *frames = dpb->frames;
    H264eDpbRt *rt = &dpb->rt;
    RK_S32 used_size = 0;
    RK_S32 seq_idx = curr->seq_idx;
    RK_S32 i;

    h264e_dbg_dpb("enter %p total %d\n", dpb, dpb->total_cnt);

    if (curr->is_idr) {
        for (i = 0; i < H264E_MAX_REFS_CNT + 1; i++) {
            frames[i].dpb_used = 0;
            frames[i].status.valid = 0;
        }
        dpb->used_size = 0;
        dpb->curr_max_lt_idx = 0;
        dpb->next_max_lt_idx = 0;
    } else {
        if (curr->seq_idx == rt->last_seq_idx) {
            h264e_dbg_dpb("NOTE: reenc found at %d\n", curr->seq_idx);
            memcpy(rt, &dpb->rt_bak, sizeof(*rt));
            memcpy(frames, dpb->frm_bak, sizeof(dpb->frm_bak));
        }
    }

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);
    /*
     * 1. scan and map cpb to internal cpb h264edpb is bridge from
     *    user defined cpb to hal slot index
     */
    for (i = 0; i < MAX_CPB_REFS; i++) {
        H264eDpbFrm *frm = find_cpb_frame(dpb, &init[i]);
        dpb->map[i] = frm;
        if (frm) {
            if (!frm->on_used)
                mpp_err_f("warning frm %d is used by cpb but on not used status\n",
                          frm->seq_idx);
            frm->dpb_used = 1;
            used_size++;
        }
    }

    mpp_assert(dpb->used_size == used_size);
    h264e_dbg_dpb("frm %d init cpb used size %d vs %d\n", seq_idx,
                  used_size, dpb->used_size);

    /* backup current runtime status */
    memcpy(&dpb->rt_bak, rt, sizeof(dpb->rt_bak));
    memcpy(dpb->frm_bak, frames, sizeof(dpb->frm_bak));

    /* mark current cpb */
    RK_S32 found_curr = 0;

    /* 2. scan all slot for used map and find a slot for current */
    dpb->curr = NULL;

    h264e_dbg_dpb("frm %d start finding slot for storage\n", seq_idx);
    for (i = 0; i < H264E_MAX_REFS_CNT + 1; i++) {
        H264eDpbFrm *p = &frames[i];
        RK_S32 curr_frm_num = rt->last_frm_num + rt->last_is_ref;
        RK_S32 curr_poc_lsb = rt->last_poc_lsb;
        RK_S32 curr_poc_msb = rt->last_poc_msb;
        RK_S32 valid = 0;
        RK_S32 j;

        for (j = 0; j < MAX_CPB_REFS; j++) {
            if (p == dpb->map[j]) {
                valid = 1;
                break;
            }
        }

        h264e_dbg_dpb("slot %2d check in cpb init valid %d used %04x\n",
                      i, valid, p->on_used);

        if (valid) {
            if (!p->on_used || !p->status.valid) {
                mpp_assert(p->on_used);
                mpp_assert(p->status.valid);
                h264e_dpb_dump_frms(dpb);
            }
            continue;
        }

        h264e_dbg_dpb("slot %2d used %04x checking\n", i, p->on_used);

        if (p->hal_used)
            continue;

        if (found_curr) {
            p->dpb_used = 0;
            p->status.valid = 0;
            continue;
        }

        h264e_dbg_dpb("slot %2d used %04x mark as current\n", i, p->on_used);

        dpb->curr = p;
        p->status.val = curr->val;
        p->seq_idx = curr->seq_idx;
        if (curr->is_idr) {
            p->frame_num = 0;
            p->poc = 0;
            curr_frm_num = 0;
            curr_poc_lsb = 0;
            curr_poc_msb = 0;
        } else {
            if (curr_frm_num >= dpb->max_frm_num)
                curr_frm_num = 0;

            p->frame_num = curr_frm_num;

            if (dpb->poc_type == 0)
                curr_poc_lsb += 2;
            else if (dpb->poc_type == 2)
                curr_poc_lsb += 1 + rt->last_is_ref;
            else
                mpp_err_f("do not support poc type 1\n");

            if (curr_poc_lsb >= dpb->max_poc_lsb) {
                curr_poc_lsb = 0;
                curr_poc_msb++;
            }

            p->poc = curr_poc_lsb + curr_poc_msb * dpb->max_poc_lsb;

        }
        p->lt_idx = curr->lt_idx;
        p->dpb_used = 1;

        rt->last_seq_idx = curr->seq_idx;
        rt->last_is_ref  = !curr->is_non_ref;
        rt->last_frm_num = curr_frm_num;
        rt->last_poc_lsb = curr_poc_lsb;
        rt->last_poc_msb = curr_poc_msb;

        found_curr = 1;
        h264e_dbg_dpb("frm %d update slot %d with frame_num %d poc %d\n",
                      seq_idx, i, p->frame_num, p->poc);
    }

    mpp_assert(dpb->curr);

    if (NULL == dpb->curr)
        mpp_err_f("frm %d failed to find a slot for curr %d\n", seq_idx);

    h264e_dbg_dpb("frm %d start finding slot for refr %d\n", seq_idx, refr->seq_idx);
    dpb->refr = find_cpb_frame(dpb, refr);
    if (NULL == dpb->refr)
        dpb->refr = dpb->curr;

    h264e_dbg_dpb("frm %d curr %d refr %d start building list\n",
                  seq_idx, dpb->curr->slot_idx, dpb->refr->slot_idx);

    h264e_dpb_build_list(dpb, cpb);

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    h264e_dpb_build_marking(dpb);

    if (h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    h264e_dbg_dpb("leave %p\n", dpb);

    return MPP_OK;
}

void h264e_dpb_check(H264eDpb *dpb, EncCpbStatus *cpb)
{
    H264eDpbFrm *curr = dpb->curr;
    H264eDpbFrm *refr = dpb->refr;
    RK_S32 i;

    h264e_dbg_dpb("enter %p\n", dpb);

    h264e_dbg_dpb("frm %d refr -> frm %d ready\n",
                  curr->seq_idx, refr->seq_idx);

    if (curr->status.is_non_ref) {
        curr->dpb_used = 0;
        curr->status.valid = 0;
    } else {
        /* sliding window process */
        RK_S32 used_size = dpb->used_size;
        RK_S32 dpb_size = dpb->dpb_size;

        h264e_dbg_dpb("frm %d %s insert dpb used %d max %d\n",
                      curr->seq_idx, curr->status.is_lt_ref ? "lt" : "st",
                      used_size, dpb_size);

        used_size++;

        /* replace lt ref with same lt_idx */
        if (curr->status.is_lt_ref) {
            RK_S32 lt_idx = curr->lt_idx;

            dpb->lt_size++;

            for (i = 0; i < dpb->total_cnt; i++) {
                H264eDpbFrm *tmp = &dpb->frames[i];

                if (tmp == curr)
                    continue;

                if (!tmp->dpb_used)
                    continue;

                if (!tmp->status.valid)
                    continue;

                mpp_assert(!tmp->status.is_non_ref);

                if (!tmp->status.is_lt_ref)
                    continue;

                if (tmp->lt_idx == lt_idx) {
                    tmp->dpb_used = 0;
                    tmp->status.valid = 0;
                    h264e_dbg_dpb("frm %d lt_idx %d replace %d\n",
                                  curr->seq_idx, curr->lt_idx, tmp->slot_idx);
                    dpb->lt_size--;
                    used_size--;
                }
            }
        } else {
            dpb->st_size++;
        }

        if (used_size > dpb_size) {
            mpp_assert(dpb->lt_size <= dpb_size);

            while (used_size > dpb_size) {
                RK_S32 small_poc = 0x7fffffff;
                H264eDpbFrm *unref = NULL;

                h264e_dbg_dpb("sliding window cpb proc: st %d lt %d max %d\n",
                              dpb->st_size, dpb->lt_size, dpb_size);

                for (i = 0; i < dpb->total_cnt; i++) {
                    H264eDpbFrm *tmp = &dpb->frames[i];

                    h264e_dbg_dpb("frm %d num %d poc %d\n",
                                  tmp->seq_idx, tmp->frame_num, tmp->poc);

                    if (!tmp->on_used)
                        continue;

                    if (!tmp->status.valid)
                        continue;

                    mpp_assert(!tmp->status.is_non_ref);

                    if (tmp->status.is_lt_ref)
                        continue;

                    if (tmp->poc < small_poc) {
                        unref = tmp;
                        small_poc = tmp->poc;

                        h264e_dbg_dpb("frm %d update smallest poc to %d\n",
                                      tmp->seq_idx, tmp->poc);
                    }
                }

                if (unref) {
                    h264e_dbg_dpb("removing frm %d poc %d\n",
                                  unref->seq_idx, unref->poc);

                    unref->dpb_used = 0;
                    dpb->st_size--;
                    mpp_assert(dpb->st_size >= 0);
                    used_size--;
                } else
                    break;
            }
        }

        dpb->used_size = used_size;
        if (used_size > dpb_size) {
            mpp_err_f("error found used_size %d > dpb_size %d\n", used_size, dpb_size);
            mpp_assert(used_size <= dpb_size);
        }
    }

    h264e_dbg_dpb("curr %d done used_size %d\n", curr->seq_idx, dpb->used_size);

    RK_S32 used_size = 0;
    for (i = 0; i < MAX_CPB_REFS; i++) {
        dpb->map[i] = find_cpb_frame(dpb, &cpb->final[i]);
        if (dpb->map[i])
            used_size++;
    }

    h264e_dbg_dpb("curr %d cpb final used_size %d vs %d\n",
                  curr->seq_idx, used_size, dpb->used_size);
    mpp_assert(dpb->used_size == used_size);

    h264e_dbg_dpb("leave %p\n", dpb);
}

MPP_RET h264e_dpb_hal_start(H264eDpb *dpb, RK_S32 slot_idx)
{
    H264eDpbFrm *frm = &dpb->frames[slot_idx];

    frm->hal_used++;
    //h264e_dpb_dump_usage(dpb, __FUNCTION__);
    return MPP_OK;
}

MPP_RET h264e_dpb_hal_end(H264eDpb *dpb, RK_S32 slot_idx)
{
    H264eDpbFrm *frm = &dpb->frames[slot_idx];

    frm->hal_used--;
    //h264e_dpb_dump_usage(dpb, __FUNCTION__);
    return MPP_OK;
}
