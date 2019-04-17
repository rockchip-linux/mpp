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

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "hal_h264e_com.h"
#include "h264e_dpb.h"

H264eFrmBuf *h264e_dpb_frm_buf_get(H264eFrmBufGrp *grp)
{
    RK_U32 i;

    for (i = 0; i < H264E_REF_MAX; i++) {
        if (!grp->bufs[i].is_used) {
            grp->bufs[i].is_used = 1;
            return &grp->bufs[i];
        }
    }

    mpp_err_f("failed to get buffer for dpb frame\n");

    return NULL;
}

MPP_RET h264e_dpb_frm_buf_put(H264eFrmBufGrp *grp, H264eFrmBuf *buf)
{
    mpp_assert(buf->is_used);
    buf->is_used = 0;

    (void)grp;

    return MPP_OK;
}

/* get buffer at init */
MPP_RET h264e_dpb_init_curr(H264eDpb *dpb, H264eDpbFrm *frm)
{
    H264eFrmBufGrp *buf_grp = &dpb->buf_grp;

    mpp_assert(!frm->on_used);

    frm->dpb        = dpb;
    frm->ref_status = 0;
    frm->ref_count  = 0;

    memset(&frm->info, 0, sizeof(frm->info));

    if (!frm->buf) {
        RK_U32 i;
        H264eFrmBuf *buf = h264e_dpb_frm_buf_get(buf_grp);

        for (i = 0; i < buf_grp->count; i++) {
            mpp_buffer_get(buf_grp->group, &buf->buf[i], buf_grp->size[i]);
            mpp_assert(buf->buf[i]);
        }

        frm->buf = buf;
    }

    frm->inited = 1;

    return MPP_OK;
}

/* put buffer at deinit */
MPP_RET h264e_dpb_frm_deinit(H264eDpbFrm *frm)
{
    RK_U32 i;
    H264eDpb *dpb = frm->dpb;
    H264eFrmBufGrp *buf_grp = &dpb->buf_grp;
    H264eFrmBuf *buf = frm->buf;

    h264e_dpb_dbg_f("enter %d\n", frm->frm_cnt);

    for (i = 0; i < buf_grp->count; i++) {
        mpp_assert(buf->buf[i]);
        mpp_buffer_put(buf->buf[i]);
        buf->buf[i] = NULL;
    }

    h264e_dpb_frm_buf_put(buf_grp, buf);
    frm->inited = 0;

    return MPP_OK;
}

MppBuffer h264e_dpb_frm_get_buf(H264eDpbFrm *frm, RK_S32 index)
{
    H264eFrmBuf *buf = frm->buf;

    if (NULL == buf)
        return NULL;

    mpp_assert(buf->is_used);
    mpp_assert(index >= 0 && index < H264E_MAX_BUF_CNT);
    mpp_assert(buf->buf[index]);
    return buf->buf[index];
}

static void h264e_dpb_dump_hier(H264eDpb *dpb)
{
    RK_S32 i = 0;

    mpp_log_f("dpb %p gop length %d\n", dpb, dpb->gop_len);

    for (i = 0; i < dpb->gop_len + 1; i++) {
        mpp_log_f("hier %d cnt %2d status 0x%03x dist %d\n", i,
                  dpb->ref_cnt[i], dpb->ref_sta[i], dpb->ref_dist[i]);
    }
}

void h264e_dpb_dump_frm(H264eDpb *dpb, const char *caller)
{
    RK_S32 i = 0;

    mpp_log_f("dump dpb frame info in %s\n", caller);

    mpp_log_f("dpb %p total count %d\n", dpb, dpb->total_cnt);
    mpp_log_f("dpb status - init use cnt gop idx type idr ref lt idx sta cnt\n", dpb, dpb->total_cnt);

    for (i = 0; i < dpb->total_cnt; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];

        if (i == 0) {
            mpp_log_f("short-term reference frame:\n");
        } else if (i == dpb->max_st_cnt) {
            mpp_log_f("long-term reference frame:\n");
        } else if (i == dpb->curr_idx) {
            mpp_log_f("current frame:\n");
        }

        if (!frm->inited)
            continue;

        mpp_log_f("frm slot %2d  %-4d %-3d %-3d %-3d %-3d %-4d %-3d %-3d %-2d %-3d %-3x %-3d\n",
                  i,
                  frm->inited,
                  frm->on_used,
                  frm->frm_cnt,
                  frm->gop_cnt,
                  frm->gop_idx,
                  frm->frame_type,
                  frm->info.is_idr,
                  !frm->info.is_non_ref,
                  frm->info.is_lt_ref,
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

        mpp_log_f("list slot %d   %-4d %-3d %-3d %-3d %-4d %-3d %-3d %-2d %-3d %-3x %-3d\n",
                  i,
                  frm->inited,
                  frm->on_used,
                  frm->frm_cnt,
                  frm->gop_idx,
                  frm->frame_type,
                  frm->info.is_idr,
                  !frm->info.is_non_ref,
                  frm->info.is_lt_ref,
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
        mpp_log_f("list status - init use cnt gop type idr ref lt idx sta cnt\n", dpb, dpb->total_cnt);

        h264e_dpb_dump_listX(dpb->list, dpb->dpb_size);
    }
}

MPP_RET h264e_dpb_init(H264eDpb **dpb, H264eDpbCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H264eDpb *p = NULL;
    if (NULL == dpb || NULL == cfg) {
        mpp_err_f("invalid parameter %p %p\n", dpb, cfg);
        return MPP_ERR_VALUE;
    }

    h264e_dpb_dbg_f("enter\n");
    h264e_dpb_dbg_f("max  ref frm num %d\n", cfg->ref_frm_num);
    h264e_dpb_dbg_f("log2 max frm num %d\n", cfg->log2_max_frm_num);
    h264e_dpb_dbg_f("log2 max poc lsb %d\n", cfg->log2_max_poc_lsb);

    if (cfg->ref_frm_num < 0 || cfg->ref_frm_num > H264E_REF_MAX ||
        cfg->log2_max_frm_num < 0 || cfg->log2_max_frm_num > 16 ||
        cfg->log2_max_poc_lsb < 0 || cfg->log2_max_poc_lsb > 16) {
        mpp_err_f("invalid config value %d %d %d\n", cfg->ref_frm_num,
                  cfg->log2_max_frm_num, cfg->log2_max_poc_lsb);
        return MPP_ERR_VALUE;
    }

    RK_S32 max_st_cnt = cfg->ref_frm_num;
    RK_S32 max_lt_cnt = cfg->ref_frm_num;
    RK_S32 total_cnt  = max_st_cnt + max_lt_cnt + 1;

    p = mpp_calloc_size(H264eDpb, sizeof(H264eDpb) +
                        total_cnt * sizeof(H264eDpbFrm));

    if (NULL == p)
        return MPP_ERR_MALLOC;

    p->cfg = *cfg;

    p->ref_frm_num      = max_st_cnt;
    p->max_frm_num      = (1 << cfg->log2_max_frm_num) - 1;
    p->max_poc_lsb      = (1 << cfg->log2_max_poc_lsb) - 1;

    p->total_cnt        = total_cnt;
    p->max_st_cnt       = max_st_cnt;
    p->max_lt_cnt       = max_lt_cnt;
    p->curr_idx         = total_cnt - 1;

    p->frames = (H264eDpbFrm *)(p + 1);

    ret = mpp_buffer_group_get_internal(&p->buf_grp.group, MPP_BUFFER_TYPE_ION);
    mpp_assert(ret == MPP_OK);
    mpp_assert(p->buf_grp.group);
    mpp_assert(dpb);
    *dpb = p;

    h264e_dpb_dbg_f("leave %p\n", p);

    return ret;
}

MPP_RET h264e_dpb_deinit(H264eDpb *dpb)
{
    RK_S32 i;

    h264e_dpb_dbg_f("enter %p\n", dpb);

    for (i = 0; i < dpb->total_cnt; i++) {
        if (dpb->frames[i].inited)
            h264e_dpb_frm_deinit(&dpb->frames[i]);
    }

    if (dpb->buf_grp.group)
        mpp_buffer_group_put(dpb->buf_grp.group);

    MPP_FREE(dpb);

    h264e_dpb_dbg_f("leave\n");

    return MPP_OK;
}

MPP_RET h264e_dpb_setup_buf_size(H264eDpb *dpb, RK_U32 size[], RK_U32 count)
{
    RK_U32 i;
    H264eFrmBufGrp *buf_grp = &dpb->buf_grp;

    if (count > H264E_MAX_BUF_CNT) {
        mpp_err_f("invalid count %d\n", count);
        return MPP_NOK;
    }

    for (i = 0; i < count; i++) {
        buf_grp->size[i] = size[i];
        h264e_dpb_dbg_f("size[%d] %d\n", i, size[i]);
    }

    buf_grp->count = count;

    return MPP_OK;
}

MPP_RET h264e_dpb_setup_hier(H264eDpb *dpb, MppEncHierCfg *cfg)
{
    // update gop hierarchy reference status and each frame counter
    RK_S32 i;
    MppEncFrmRefInfo *info;

    memset(dpb->ref_inf, 0, sizeof(dpb->ref_inf));
    memset(dpb->ref_sta, 0, sizeof(dpb->ref_sta));
    memset(dpb->ref_cnt, 0, sizeof(dpb->ref_cnt));
    memset(dpb->ref_dist, 0, sizeof(dpb->ref_dist));

    dpb->gop_len = cfg->length;
    dpb->max_lt_idx = 0;

    mpp_enc_get_hier_info(&info, cfg);

    for (i = 0; i < cfg->length + 1; i++, info++) {
        RK_S32 ref_idx = info->ref_gop_idx;

        // setup ref_status and ref_idx for new frame
        dpb->ref_inf[i] = info->status;
        dpb->ref_dist[i] = ref_idx - i;

        if (!info->status.is_non_ref) {
            dpb->ref_sta[i] |= REF_BY_RECN(i);
            dpb->ref_cnt[i]++;
        }

        if (!info->status.is_non_ref && info->status.is_lt_ref) {
            if (info->status.lt_idx > dpb->max_lt_idx) {
                dpb->max_lt_idx = info->status.lt_idx;
                h264e_dpb_dbg_f("curr %d update lt_idx to %d\n",
                                i, dpb->max_lt_idx);
            }
        }

        h264e_dpb_dbg_f("curr %d ref_status 0x%03x count %d -> %d dist %d\n",
                        i, dpb->ref_sta[i], dpb->ref_cnt[i],
                        ref_idx, dpb->ref_dist[i]);

        // update the reference frame status and counter only once
        if (ref_idx == i)
            continue;

        dpb->ref_sta[ref_idx] |= REF_BY_REFR(i);
        dpb->ref_cnt[ref_idx]++;

        h264e_dpb_dbg_f("refr %d ref_status 0x%03x count %d\n",
                        ref_idx, dpb->ref_sta[ref_idx], dpb->ref_cnt[ref_idx]);
    }

    mpp_enc_put_hier_info(info);

    if (hal_h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_hier(dpb);

    return MPP_OK;
}

static void h264e_dpb_frm_swap(H264eDpb *dpb, RK_S32 a, RK_S32 b)
{
    H264eDpbFrm tmp;

    tmp = dpb->frames[a];
    dpb->frames[a] = dpb->frames[b];
    dpb->frames[b] = tmp;
}

H264eDpbFrm *h264e_dpb_get_curr(H264eDpb *dpb, RK_S32 new_seq)
{
    RK_S32 gop_cnt = 0;
    RK_S32 gop_idx = 0;
    RK_S32 gop_idx_wrap = 0;

    H264eDpbFrm *frm = &dpb->frames[dpb->curr_idx];

    h264e_dpb_dbg_f("try get curr %d new_seq %d gop cnt %d idx %d\n",
                    dpb->seq_idx, new_seq, dpb->gop_cnt, dpb->gop_idx);

    if (!frm->inited)
        h264e_dpb_init_curr(dpb, frm);

    mpp_assert(!frm->on_used);
    mpp_assert(!dpb->curr);

    if (new_seq) {
        // NOTE: gop_idx here is for next gop_idx
        dpb->gop_idx = 1;
        dpb->gop_cnt = 0;

        if (dpb->seq_idx)
            dpb->seq_cnt++;

        dpb->seq_idx = 0;

        gop_cnt = 0;
        gop_idx = 0;
    } else {
        gop_cnt = dpb->gop_cnt;
        gop_idx = dpb->gop_idx++;
    }

    h264e_dpb_dbg_f("A frm cnt %d gop %d idx %d dpb gop %d idx %d\n",
                    frm->frm_cnt, gop_cnt, gop_idx,
                    dpb->gop_cnt, dpb->gop_idx);

    if (dpb->gop_idx >= dpb->gop_len) {
        dpb->gop_idx = 0;
        dpb->gop_cnt++;
    }

    frm->frm_cnt = dpb->seq_idx++;
    frm->gop_cnt = gop_cnt;
    frm->gop_idx = gop_idx;

    h264e_dpb_dbg_f("B frm cnt %d gop %d idx %d dpb gop %d idx %d\n",
                    frm->frm_cnt, gop_cnt, gop_idx,
                    dpb->gop_cnt, dpb->gop_idx);

    gop_idx_wrap = gop_idx;

    if (!gop_idx && gop_cnt)
        gop_idx_wrap = dpb->gop_len;

    frm->info           = dpb->ref_inf[gop_idx_wrap];
    frm->ref_status     = dpb->ref_sta[gop_idx];
    frm->ref_count      = dpb->ref_cnt[gop_idx];
    frm->ref_dist       = dpb->ref_dist[gop_idx_wrap];
    frm->poc            = frm->frm_cnt * 2;
    frm->lt_idx         = frm->info.lt_idx;
    frm->on_used        = 1;

    h264e_dpb_dbg_f("setup frm %d gop %d idx %d wrap %d ref dist %d\n",
                    frm->frm_cnt, frm->gop_cnt, frm->gop_idx, gop_idx_wrap, frm->ref_dist);

    dpb->curr = frm;

    h264e_dpb_dbg_f("frm %d old frm num %d next %d\n",
                    frm->frm_cnt, dpb->curr_frm_num, dpb->next_frm_num);

    // new_seq is for dpb reset and IDR frame
    if (new_seq) {
        RK_S32 i;

        frm->info.is_idr = 1;
        frm->info.is_intra = 1;

        // init to 0
        dpb->curr_frm_num = 0;
        dpb->next_frm_num = 1;
        frm->frame_num = 0;
        frm->poc = 0;

        // mmco 5 mark all reference frame to be non-referenced
        for (i = 0; i < dpb->curr_idx; i++) {
            H264eDpbFrm *tmp = &dpb->frames[i];

            tmp->on_used = 0;
            tmp->info.is_non_ref = 1;
            tmp->info.is_lt_ref = 0;
            tmp->ref_status = 0;
            tmp->ref_count = 0;
            tmp->marked_unref = 0;
        }

        dpb->dpb_size = 0;
        dpb->lt_size = 0;
        dpb->st_size = 0;
        dpb->unref_cnt = 0;
    } else
        dpb->curr_frm_num = dpb->next_frm_num;

    frm->frame_num = dpb->curr_frm_num;

    dpb->next_frm_num = dpb->curr_frm_num + !frm->info.is_non_ref;
    h264e_dpb_dbg_f("frm %d new frm num %d next %d\n",
                    frm->frm_cnt, dpb->curr_frm_num, dpb->next_frm_num);


    if (hal_h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    return frm;
}

H264eDpbFrm *h264e_dpb_get_refr(H264eDpbFrm *frm)
{
    RK_S32 i;
    H264eDpbFrm *ref = NULL;
    H264eDpb *dpb = frm->dpb;
    RK_S32 frm_cnt = frm->frm_cnt;
    RK_S32 ref_dist = frm->ref_dist;
    RK_S32 ref_frm_cnt = frm_cnt + ref_dist;

    for (i = 0; i < dpb->curr_idx; i++) {
        H264eDpbFrm *tmp = &dpb->frames[i];

        if (tmp->on_used && !tmp->info.is_non_ref &&
            tmp->frm_cnt == ref_frm_cnt) {
            ref = &dpb->frames[i];
            break;
        }
    }

    h264e_dpb_dbg_f("frm %d found dist %d refer frm %d at %d %p\n",
                    frm_cnt, ref_dist, ref_frm_cnt, i, ref);

    if (NULL == ref)
        ref = &dpb->frames[dpb->curr_idx];

    return ref;
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

    if (dpb->curr->info.is_intra)
        return ;

    // init list
    // 1. found all short term and long term ref
    for (i = 0; i < dpb->curr_idx; i++) {
        H264eDpbFrm *frm = &dpb->frames[i];

        if (!frm->on_used)
            continue;

        if (frm->info.is_non_ref)
            continue;

        h264e_dpb_dbg_f("idx %d on_used %x ref_status %03x is_non_ref %d lt_ref %d\n",
                        i, frm->on_used, frm->ref_status,
                        frm->info.is_non_ref, frm->info.is_lt_ref);

        if (!frm->info.is_non_ref) {
            if (!frm->info.is_lt_ref) {
                dpb->stref[st_size] = frm;
                st_size++;
                h264e_dpb_dbg_f("found st %d st_size %d %p\n", i, st_size, frm);
            } else {
                dpb->ltref[lt_size] = frm;
                lt_size++;
                h264e_dpb_dbg_f("found lt %d lt_size %d %p\n", i, lt_size, frm);
            }
        }
    }

    h264e_dpb_dbg_f("dpb_size %d st_size %d lt_size %d\n", dpb->dpb_size, st_size, lt_size);

    // sort st list
    if (st_size > 1) {
        if (hal_h264e_debug & H264E_DBG_DPB) {
            mpp_log_f("dpb st list before sort\n");
            h264e_dpb_dump_listX(dpb->stref, st_size);
        }

        qsort(dpb->stref, st_size, sizeof(dpb->stref[0]), cmp_st_list);

        if (hal_h264e_debug & H264E_DBG_DPB) {
            mpp_log_f("dpb st list after  sort\n");
            h264e_dpb_dump_listX(dpb->stref, st_size);
        }
    }

    if (lt_size > 1) {
        if (hal_h264e_debug & H264E_DBG_DPB) {
            mpp_log_f("dpb lt list before sort\n");
            h264e_dpb_dump_listX(dpb->ltref, lt_size);
        }

        qsort(dpb->ltref, lt_size, sizeof(dpb->ltref[0]), cmp_lt_list);

        if (hal_h264e_debug & H264E_DBG_DPB) {
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

    if (hal_h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_list(dpb);

    if (dpb->st_size + dpb->lt_size) {
        H264eDpbFrm *curr = dpb->curr;
        H264eDpbFrm *def_ref = dpb->list[0];

        RK_S32 curr_frm_cnt = curr->frm_cnt;
        RK_S32 def_ref_frm_cnt = def_ref->frm_cnt;
        RK_S32 set_ref_frm_cnt = curr->frm_cnt + curr->ref_dist;

        dpb->need_reorder = (def_ref_frm_cnt != set_ref_frm_cnt) ? (1) : (0);
        h264e_dpb_dbg_f("refer curr %d def %d set %d reorder %d\n",
                        curr_frm_cnt, def_ref_frm_cnt, set_ref_frm_cnt,
                        dpb->need_reorder);
    } else {
        h264e_dpb_dbg_f("refer NULL\n");
    }
}

void h264e_dpb_build_marking(H264eDpb *dpb)
{
    RK_S32 i;
    H264eDpbFrm *frm = dpb->curr;
    H264eDpbFrm *ref = h264e_dpb_get_refr(frm);

    h264e_dpb_dbg_f("curr %d gop %d idx %d ref %d ready -> frm %d gop %d idx %d\n",
                    frm->frm_cnt, frm->gop_cnt, frm->gop_idx, !frm->info.is_non_ref,
                    ref->frm_cnt, ref->gop_cnt, ref->gop_cnt);

    // refernce frame can not mark itself as unreferenced.
    // So do NOT clear RECN flag here
    // clear the ref status and count once
    if (frm != ref) {
        ref->ref_status &= ~(REF_BY_REFR(frm->gop_idx));

        if (ref->ref_count > 0)
            ref->ref_count--;

        h264e_dpb_dbg_f("refr %d ref_status %03x count %d\n",
                        ref->gop_idx, ref->ref_status, ref->ref_count);
    }

    if (!frm->info.is_non_ref && frm->info.is_lt_ref) {
        h264e_dpb_dbg_f("found lt idx %d curr max %d\n", frm->lt_idx, dpb->curr_max_lt_idx);
        if (frm->lt_idx > dpb->curr_max_lt_idx) {
            dpb->next_max_lt_idx = frm->lt_idx;
        }
    }

    if (frm->info.is_idr)
        return ;

    if (frm->info.is_non_ref)
        return ;

    h264e_dpb_dbg_f("curr -- frm %d gop %d idx %d T%d\n",
                    frm->frm_cnt, frm->gop_cnt, frm->gop_idx, frm->info.temporal_id);

    // if we are reference frame scan the previous frame to mark
    for (i = 0; i < dpb->curr_idx; i++) {
        H264eDpbFrm *tmp = &dpb->frames[i];

        if (!tmp->on_used)
            continue;

        if (tmp->info.is_non_ref)
            continue;

        h264e_dpb_dbg_f("slot %2d frm %3d gop %3d idx %2d T%d lt %d cnt %d\n",
                        i, tmp->frm_cnt, tmp->gop_cnt, tmp->gop_idx,
                        tmp->info.temporal_id, tmp->info.is_lt_ref, tmp->ref_count);

        if (tmp->info.is_lt_ref) {
            if (frm->info.is_lt_ref && frm->lt_idx == tmp->lt_idx) {
                h264e_dpb_dbg_f("frm %3d lt idx %d replace frm %d dpb size %d\n",
                                frm->frm_cnt, tmp->lt_idx, tmp->frm_cnt,
                                dpb->dpb_size);
            }

            continue;
        }

        if (!tmp->marked_unref && !tmp->ref_count &&
            tmp != frm && tmp != ref &&
            tmp->info.temporal_id == frm->info.temporal_id) {
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

            dpb->unref[dpb->unref_cnt] = tmp;
            dpb->unref_cnt++;

            tmp->marked_unref = 1;
            h264e_dpb_dbg_f("frm %d T%d mark frm %d gop %d idx %d to be unreference dpb size %d\n",
                            frm->frm_cnt, frm->info.temporal_id, tmp->frm_cnt, tmp->gop_cnt,
                            tmp->gop_idx, dpb->dpb_size);
        }
    }

    h264e_dpb_dbg_f("dpb size %d unref_cnt %d\n", dpb->dpb_size, dpb->unref_cnt);
}

void h264e_dpb_curr_ready(H264eDpb *dpb)
{
    H264eDpbFrm *frm = dpb->curr;
    H264eDpbFrm *ref = h264e_dpb_get_refr(frm);

    h264e_dpb_dbg_f("curr %d gop %d idx %d refr -> frm %d gop %d idx %d ready\n",
                    frm->frm_cnt, frm->gop_cnt, frm->gop_idx,
                    ref->frm_cnt, ref->gop_cnt, ref->gop_idx, frm->info.is_non_ref);

    if (!frm->info.is_non_ref) {
        frm->ref_status &= ~(REF_BY_RECN(frm->gop_idx));
        frm->ref_count--;
    }

    if (!dpb->curr->info.is_non_ref) {
        RK_S32 i;
        RK_S32 insert = -1;

        dpb->dpb_size++;

        if (dpb->curr->info.is_lt_ref) {
            // long-term ref swap and replace
            for (i = dpb->max_st_cnt; i < dpb->curr_idx; i++) {
                H264eDpbFrm *tmp = &dpb->frames[i];

                // try found the swap frame with same lt_idx
                if (!tmp->on_used) {
                    insert = i;
                    break;
                } else {
                    if (tmp->lt_idx == frm->lt_idx) {
                        tmp->on_used = 0;
                        dpb->dpb_size--;
                        insert = i;
                        break;
                    }
                }
            }
        } else {
            // normal gop position swap and decrease ref_cnt
            for (i = 0; i < dpb->max_st_cnt; i++) {
                H264eDpbFrm *tmp = &dpb->frames[i];

                if (!tmp->on_used) {
                    insert = i;
                    break;
                }
            }
        }

        mpp_assert(insert >= 0);

        h264e_dpb_dbg_f("frm %d lt %d swap from %d to pos %d\n",
                        frm->frm_cnt, frm->info.is_lt_ref,
                        dpb->curr_idx, insert);

        h264e_dpb_frm_swap(dpb, insert, dpb->curr_idx);
    } else
        dpb->curr->on_used = 0;

    if (hal_h264e_debug & H264E_DBG_DPB)
        h264e_dpb_dump_frms(dpb);

    dpb->curr = NULL;
}

