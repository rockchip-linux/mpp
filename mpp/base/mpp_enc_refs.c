/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_enc_refs"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "mpp_enc_ref.h"
#include "mpp_enc_refs.h"

#define MAX_CPB_ST_FRM          16
#define MAX_CPB_LT_FRM          16
#define MAX_CPB_TID_FRM         16
#define MAX_CPB_LT_IDX          16
#define MAX_CPB_FRM             ((MAX_CPB_ST_FRM) + (MAX_CPB_LT_FRM))

#define MPP_ENC_REFS_DBG_FUNC       (0x00000001)
#define MPP_ENC_REFS_DBG_FLOW       (0x00000002)
#define MPP_ENC_REFS_DBG_FRM        (0x00000004)
#define MPP_ENC_REFS_DBG_SIZE       (0x00000008)

#define enc_refs_dbg_func(fmt, ...) _mpp_dbg_f(enc_refs_debug, MPP_ENC_REFS_DBG_FUNC, fmt, ## __VA_ARGS__)
#define enc_refs_dbg_flow(fmt, ...) _mpp_dbg_f(enc_refs_debug, MPP_ENC_REFS_DBG_FLOW, fmt, ## __VA_ARGS__)
#define enc_refs_dbg_frm(fmt, ...)  _mpp_dbg(enc_refs_debug, MPP_ENC_REFS_DBG_FRM, fmt, ## __VA_ARGS__)
#define enc_refs_dbg_size(fmt, ...) _mpp_dbg(enc_refs_debug, MPP_ENC_REFS_DBG_SIZE, fmt, ## __VA_ARGS__)

#define ENC_REFS_REF_CFG_CHANGED    (0x00000001)
#define ENC_REFS_USR_CFG_CHANGED    (0x00000002)
#define ENC_REFS_IGOP_CHANGED       (0x00000004)

typedef struct RefsCnt_t {
    RK_S32              delay;
    RK_S32              delay_cnt;
    RK_S32              len;
    RK_S32              cnt;
    RK_S32              idx;

    RK_S32              lt_idx;
    RK_S32              tid;
    MppEncRefMode       ref_mode;
    RK_S32              ref_arg;
} RefsCnt;

typedef struct EncVirtualCpb_t {
    MppEncCpbInfo       info;

    /*
     * max 32 reference slot for frame searching
     * (max 16 lt + max 16 st)
     * st_slot  0 ~ 15
     * lt_slot 16 ~ 31
     */
    EncFrmStatus        cpb_refs[MAX_CPB_FRM];

    /* max 32 reference mode */
    EncFrmStatus        mode_refs[MAX_CPB_FRM];

    /*
     * reference mode storage with args
     * st + tid - max 16
     * lt + idx - max 16
     */
    EncFrmStatus        st_tid_refs[MAX_CPB_TID_FRM];
    EncFrmStatus        lt_idx_refs[MAX_CPB_LT_IDX];

    /* long-term counter */
    /* lt ref will have multiple counters */
    RefsCnt             lt_cnter[MAX_CPB_LT_IDX];

    /* max two list and each list as two frames */
    EncFrmStatus        list0[2];
    EncFrmStatus        list1[2];
    /* frame kept in cpb */
    EncFrmStatus        cpb_st[2];

    /* runtime record */
    RK_S32              frm_idx;    /* overall frame index */
    RK_S32              seq_idx;    /* sequence index in one gop */
    RK_S32              seq_cnt;
    RK_S32              st_cfg_pos;
    RK_S32              st_cfg_repeat_pos;
} EncVirtualCpb;

typedef struct MppEncRefsImpl_t {
    RK_U32              changed;
    MppEncRefCfgImpl    *ref_cfg;
    MppEncRefFrmUsrCfg  usr_cfg;
    RK_S32              igop;
    RK_U32              refresh_length;

    RK_S32              hdr_need_update;

    EncVirtualCpb       cpb;
    EncVirtualCpb       cpb_stash;
} MppEncRefsImpl;

RK_U32 enc_refs_debug = 0;

void _dump_frm(EncFrmStatus *frm, const char *func, RK_S32 line)
{
    if (!frm->valid)
        return ;

    if (frm->is_non_ref) {
        mpp_log("%s:%d valid %d frm %d %s tid %d non-ref -> [%x:%d]\n",
                func, line, frm->valid, frm->seq_idx,
                frm->is_intra ? "intra" : "inter",
                frm->temporal_id, frm->ref_mode, frm->ref_arg);
    } else if (frm->is_lt_ref) {
        mpp_log("%s:%d valid %d frm %d %s tid %d lt-ref  -> [%x:%d] lt_idx %d\n",
                func, line, frm->valid, frm->seq_idx,
                frm->is_intra ? "intra" : "inter",
                frm->temporal_id, frm->ref_mode, frm->ref_arg,
                frm->lt_idx);
    } else {
        mpp_log("%s:%d valid %d frm %d %s tid %d st-ref  -> [%x:%d]\n",
                func, line, frm->valid, frm->seq_idx,
                frm->is_intra ? "intra" : "inter",
                frm->temporal_id, frm->ref_mode, frm->ref_arg);
    }
}

#define dump_cpb(cpb)   _dump_cpb(cpb, __FUNCTION__, __LINE__)

void _dump_cpb(EncVirtualCpb *cpb, const char *func, RK_S32 line)
{
    MppEncCpbInfo *info = &cpb->info;
    RK_S32 i;

    mpp_log("%s:%d cpb %p status:\n", func, line, cpb);
    mpp_log("cpb info: dpb_size %d max_lt/st cnt [%d:%d] \n",
            info->dpb_size, info->max_lt_cnt, info->max_st_cnt);
    mpp_log("cpb info: max_lt_idx %d max_st_tid %d\n",
            info->max_lt_idx, info->max_st_tid);
    mpp_log("cpb info: lt_gop %d st_gop %d\n",
            info->lt_gop, info->st_gop);

    mpp_log("cpb cpb_refs:\n");
    for (i = 0; i < MAX_CPB_FRM; i++)
        dump_frm(&cpb->cpb_refs[i]);

    mpp_log("cpb mode_refs:\n");
    for (i = 0; i < MAX_CPB_FRM; i++)
        dump_frm(&cpb->mode_refs[i]);

    mpp_log("cpb st_tid_refs:\n");
    for (i = 0; i < MAX_CPB_TID_FRM; i++)
        dump_frm(&cpb->st_tid_refs[i]);

    mpp_log("cpb lt_idx_refs:\n");
    for (i = 0; i < MAX_CPB_LT_IDX; i++)
        dump_frm(&cpb->lt_idx_refs[i]);

    mpp_log("cpb runtime: frm_idx %d seq_idx %d seq_cnt %d st_cfg [%d:%d]\n",
            cpb->frm_idx, cpb->seq_idx, cpb->seq_cnt,
            cpb->st_cfg_pos, cpb->st_cfg_repeat_pos);
}

MPP_RET mpp_enc_refs_init(MppEncRefs *refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_NULL_PTR;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = mpp_calloc(MppEncRefsImpl, 1);
    *refs = p;
    if (NULL == p) {
        mpp_err_f("create refs_impl failed\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("enc_refs_debug", &enc_refs_debug, 0);

    enc_refs_dbg_func("leave %p\n", p);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_deinit(MppEncRefs *refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)(*refs);
    MPP_FREE(p);

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_set_cfg(MppEncRefs refs, MppEncRefCfg ref_cfg)
{
    if (NULL == refs || (ref_cfg && check_is_mpp_enc_ref_cfg(ref_cfg))) {
        mpp_err_f("invalid input refs %p ref_cfg %p\n", refs, ref_cfg);
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p cfg %p\n", refs, ref_cfg);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    EncVirtualCpb *cpb = &p->cpb;
    MppEncRefCfgImpl *cfg = NULL;

    if (NULL == ref_cfg)
        ref_cfg = mpp_enc_ref_default();

    cfg = (MppEncRefCfgImpl *)ref_cfg;

    p->ref_cfg = cfg;
    p->changed |= ENC_REFS_REF_CFG_CHANGED;
    p->hdr_need_update = 0;

    /* clear cpb on setup new cfg */
    if (!cfg->keep_cpb) {
        memset(cpb, 0, sizeof(*cpb));
        p->hdr_need_update = 1;
    }

    if (cfg->lt_cfg_cnt) {
        RK_S32 i;

        mpp_assert(cfg->lt_cfg_cnt < MAX_CPB_LT_FRM);

        for (i = 0; i < cfg->lt_cfg_cnt; i++) {
            RefsCnt *lt_cnt = &cpb->lt_cnter[i];
            MppEncRefLtFrmCfg *lt_cfg = &cfg->lt_cfg[i];

            lt_cnt->delay       = lt_cfg->lt_delay;
            lt_cnt->delay_cnt   = lt_cfg->lt_delay;
            lt_cnt->len         = lt_cfg->lt_gap;
            lt_cnt->lt_idx      = lt_cfg->lt_idx;
            lt_cnt->tid         = lt_cfg->temporal_id;
            lt_cnt->ref_mode    = lt_cfg->ref_mode;
            lt_cnt->ref_arg     = lt_cfg->ref_arg;
        }
    }

    MppEncCpbInfo *info = &cpb->info;

    if (info->dpb_size && info->dpb_size < cfg->cpb_info.dpb_size)
        p->hdr_need_update = 1;

    memcpy(info, &cfg->cpb_info, sizeof(cfg->cpb_info));

    enc_refs_dbg_flow("ref_cfg cpb size: lt %d st %d max lt_idx %d tid %d\n",
                      info->max_lt_cnt, info->max_st_cnt,
                      info->max_lt_idx, info->max_st_tid);

    enc_refs_dbg_func("leave %p cfg %p\n", refs, ref_cfg);
    return MPP_OK;
}

static void cleanup_cpb_refs(EncVirtualCpb *cpb)
{
    RK_U32 i;

    memset(cpb->cpb_refs, 0, sizeof(cpb->cpb_refs));
    memset(cpb->mode_refs, 0, sizeof(cpb->mode_refs));
    memset(cpb->st_tid_refs, 0, sizeof(cpb->st_tid_refs));
    memset(cpb->lt_idx_refs, 0, sizeof(cpb->lt_idx_refs));
    memset(cpb->list0, 0, sizeof(cpb->list0));
    memset(cpb->list1, 0, sizeof(cpb->list1));
    memset(cpb->cpb_st, 0, sizeof(cpb->cpb_st));

    cpb->seq_idx = 0;
    cpb->seq_cnt++;
    cpb->st_cfg_pos = 0;
    cpb->st_cfg_repeat_pos = 0;

    for (i = 0; i < MPP_ARRAY_ELEMS(cpb->lt_cnter); i++) {
        RefsCnt *lt_cnt = &cpb->lt_cnter[i];

        lt_cnt->delay_cnt = lt_cnt->delay;
        lt_cnt->cnt = 0;
        lt_cnt->idx = 0;
    }
}

static void set_st_cfg_to_frm(EncFrmStatus *frm, RK_S32 seq_idx,
                              MppEncRefStFrmCfg *st_cfg)
{
    memset(frm, 0, sizeof(*frm));

    frm->seq_idx = seq_idx;
    frm->valid = 1;
    frm->is_idr = (seq_idx == 0);
    frm->is_intra = frm->is_idr;
    frm->is_non_ref = st_cfg->is_non_ref;
    frm->is_lt_ref = 0;
    frm->temporal_id = st_cfg->temporal_id;
    frm->ref_mode = st_cfg->ref_mode;
    frm->ref_arg = st_cfg->ref_arg;

    if (enc_refs_debug & MPP_ENC_REFS_DBG_FRM)
        dump_frm(frm);
}

static MPP_RET set_frm_refresh_flag(EncFrmStatus *frm, MppEncRefsImpl *p)
{
    MPP_RET ret = MPP_OK;

    if (!frm || !p)
        return ret = MPP_ERR_NULL_PTR;

    if (p->refresh_length) {
        frm->is_i_refresh = ((frm->seq_idx  % p->igop) < p->refresh_length) && p->cpb.seq_cnt > 1;
        frm->is_i_recovery = !(frm->seq_idx  % p->igop)  && p->cpb.seq_cnt > 1;
    } else
        frm->is_i_refresh = 0;

    return ret;
}

static void set_lt_cfg_to_frm(EncFrmStatus *frm, RefsCnt *lt_cfg)
{
    frm->is_non_ref = 0;
    frm->is_lt_ref = 1;
    frm->temporal_id = lt_cfg->tid;
    frm->lt_idx = lt_cfg->lt_idx;

    if (lt_cfg->ref_mode != REF_TO_ST_REF_SETUP) {
        frm->ref_mode = lt_cfg->ref_mode;
        frm->ref_arg = lt_cfg->ref_arg;
    }

    if (enc_refs_debug & MPP_ENC_REFS_DBG_FRM)
        dump_frm(frm);
}

static EncFrmStatus *get_ref_from_cpb(EncVirtualCpb *cpb, EncFrmStatus *frm)
{
    MppEncRefMode ref_mode = frm->ref_mode;
    RK_S32 ref_arg = frm->ref_arg;

    if (frm->is_idr)
        return NULL;

    EncFrmStatus *ref = NULL;

    /* step 3.1 find seq_idx by mode and arg */
    switch (ref_mode) {
    case REF_TO_PREV_REF_FRM :
    case REF_TO_PREV_ST_REF :
    case REF_TO_PREV_LT_REF :
    case REF_TO_PREV_INTRA : {
        ref = &cpb->mode_refs[ref_mode];
    } break;
    case REF_TO_TEMPORAL_LAYER : {
        ref = &cpb->st_tid_refs[ref_arg];
    } break;
    case REF_TO_LT_REF_IDX : {
        ref = &cpb->lt_idx_refs[ref_arg];
    } break;
    case REF_TO_ST_PREV_N_REF : {
        ref = &cpb->cpb_refs[ref_arg];
    } break;
    case REF_TO_ST_REF_SETUP :
    default : {
        mpp_err_f("frm %d not supported ref mode 0x%x\n", frm->seq_idx, ref_mode);
    } break;
    }

    if (ref) {
        if (ref->valid)
            enc_refs_dbg_flow("frm %d ref mode %d arg %d -> seq %d %s idx %d\n",
                              frm->seq_idx, ref_mode, ref_arg, ref->seq_idx,
                              ref->is_lt_ref ? "lt" : "st",
                              ref->is_lt_ref ? ref->lt_idx : 0);
        else
            mpp_err_f("frm %d found mode %d arg %d -> ref %d but it is invalid\n",
                      frm->seq_idx, ref_mode, ref_arg, ref->seq_idx);
    } else {
        ref = NULL;
    }

    return ref;
}

static RK_S32 check_ref_cpb_pos(EncVirtualCpb *cpb, EncFrmStatus *frm)
{
    RK_S32 seq_idx = frm->seq_idx;
    RK_S32 found = 0;
    RK_S32 pos = -1;

    if (!frm->valid || frm->is_non_ref) {
        enc_refs_dbg_flow("frm %d is not valid ref frm\n", seq_idx);
        return pos;
    }

    if (frm->is_lt_ref) {
        /* find same lt_idx */
        for (pos = 0; pos < MAX_CPB_LT_FRM; pos++) {
            RK_S32 cpb_idx = pos + MAX_CPB_ST_FRM;
            EncFrmStatus *cpb_ref = &cpb->cpb_refs[cpb_idx];

            if (cpb_ref->valid && cpb_ref->lt_idx == frm->lt_idx) {
                pos = cpb_idx;
                enc_refs_dbg_flow("found ltr ref %d at pos %d\n", seq_idx, pos);
                found = 1;
                break;
            }
        }
    } else {
        /* search seq_idx in cpb to check the st cpb size */
        for (pos = 0; pos < MAX_CPB_ST_FRM; pos++) {
            EncFrmStatus *cpb_ref = &cpb->cpb_refs[pos];

            enc_refs_dbg_flow("matching ref %d at pos %d %d\n",
                              seq_idx, pos, cpb_ref->seq_idx);

            if (cpb_ref->valid && cpb_ref->seq_idx == seq_idx) {
                enc_refs_dbg_flow("found ref %d at pos %d\n", seq_idx, pos);
                found = 1;
                break;
            }
        }
    }

    if (!found) {
        mpp_err_f("frm %d can NOT be found in st refs!!\n", seq_idx);
        pos = -1;
        dump_cpb(cpb);
    }

    return pos;
}

static void save_cpb_status(EncVirtualCpb *cpb, EncFrmStatus *refs)
{
    EncFrmStatus *ref = &cpb->cpb_refs[MAX_CPB_ST_FRM];
    MppEncCpbInfo *info = &cpb->info;
    RK_S32 dpb_size = info->dpb_size;
    RK_S32 lt_ref_cnt = 0;
    RK_S32 st_ref_cnt = 0;
    RK_S32 ref_cnt = 0;
    RK_S32 i;

    /* save lt ref */
    for (i = 0; i < info->max_lt_cnt; i++, ref++) {
        if (!ref->valid || ref->is_non_ref || !ref->is_lt_ref)
            continue;

        mpp_assert(!ref->is_non_ref);
        mpp_assert(ref->is_lt_ref);
        mpp_assert(ref->lt_idx >= 0);

        enc_refs_dbg_flow("save lt ref %d to slot %d\n", ref->seq_idx, ref_cnt);
        refs[ref_cnt++].val = ref->val;
        lt_ref_cnt++;
    }

    ref = &cpb->cpb_refs[0];
    /* save st ref */
    if (ref_cnt < dpb_size) {
        RK_S32 max_st_cnt = info->max_st_cnt;

        if (max_st_cnt < dpb_size - ref_cnt)
            max_st_cnt = dpb_size - ref_cnt;

        for (i = 0; i < max_st_cnt; i++, ref++) {
            if (!ref->valid || ref->is_non_ref || ref->is_lt_ref)
                continue;

            mpp_assert(!ref->is_non_ref);
            mpp_assert(!ref->is_lt_ref);
            mpp_assert(ref->temporal_id >= 0);

            enc_refs_dbg_flow("save st ref %d to slot %d\n", ref->seq_idx, ref_cnt);
            refs[ref_cnt++].val = ref->val;
            st_ref_cnt++;
        }
    }

    enc_refs_dbg_flow("save ref total %d lt %d st %d\n", ref_cnt, lt_ref_cnt, st_ref_cnt);
    if (enc_refs_debug & MPP_ENC_REFS_DBG_FLOW)
        for (i = 0; i < ref_cnt; i++)
            dump_frm(&refs[i]);
}

static void store_ref_to_cpb(EncVirtualCpb *cpb, EncFrmStatus *frm)
{
    RK_S32 seq_idx = frm->seq_idx;
    RK_S32 lt_idx = frm->lt_idx;
    RK_S32 tid = frm->temporal_id;
    RK_S32 i;

    mpp_assert(frm->valid);
    mpp_assert(lt_idx < MAX_CPB_LT_IDX);
    mpp_assert(tid < MAX_CPB_LT_FRM);

    /* non-ref do not save to cpb */
    if (frm->is_non_ref)
        return ;

    if (frm->is_intra)
        cpb->mode_refs[REF_TO_PREV_INTRA].val = frm->val;

    if (frm->is_lt_ref) {
        cpb->lt_idx_refs[lt_idx].val = frm->val;
        cpb->st_tid_refs[tid].val = frm->val;
        cpb->mode_refs[REF_TO_PREV_REF_FRM].val = frm->val;
        cpb->mode_refs[REF_TO_PREV_LT_REF].val = frm->val;

        RK_S32 found = 0;
        EncFrmStatus *cpb_ref = NULL;

        /* find same lt_idx and replace */
        for (i = 0; i < MAX_CPB_LT_FRM; i++) {
            RK_S32 cpb_idx = i + MAX_CPB_ST_FRM;
            cpb_ref = &cpb->cpb_refs[cpb_idx];

            if (!cpb_ref->valid) {
                found = 1;
                break;
            } else {
                if (cpb_ref->lt_idx == lt_idx) {
                    found = 2;
                    break;
                }
            }
        }

        if (found) {
            cpb_ref->val = frm->val;
            enc_refs_dbg_flow("frm %d with lt idx %d %s to pos %d\n",
                              seq_idx, lt_idx, (found == 1) ? "add" : "replace", i);
        } else {
            mpp_err_f("frm %d with lt idx %d found no place to add or relace\n",
                      seq_idx, lt_idx);
        }
    } else {
        /* do normal st sliding window */
        cpb->st_tid_refs[tid].val = frm->val;
        cpb->mode_refs[REF_TO_PREV_REF_FRM].val = frm->val;
        cpb->mode_refs[REF_TO_PREV_ST_REF].val = frm->val;

        for (i = MAX_CPB_ST_FRM - 1; i > 0; i--)
            cpb->cpb_refs[i].val = cpb->cpb_refs[i - 1].val;

        cpb->cpb_refs[0].val = frm->val;

        // TODO: Add prev intra valid check?
    }

    enc_refs_dbg_flow("dumping cpb refs status start\n");
    if (enc_refs_debug & MPP_ENC_REFS_DBG_FLOW)
        for (i = 0; i < MAX_CPB_FRM; i++)
            if (cpb->cpb_refs[i].valid)
                dump_frm(&cpb->cpb_refs[i]);

    enc_refs_dbg_flow("dumping cpb refs status done\n");
}

MPP_RET mpp_enc_refs_dryrun(MppEncRefs refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    MppEncRefCfgImpl *cfg = p->ref_cfg;
    MppEncRefStFrmCfg *st_cfg = cfg->st_cfg;
    EncVirtualCpb *cpb = &p->cpb;
    MppEncCpbInfo *info = &cpb->info;
    RK_S32 lt_cfg_cnt = cfg->lt_cfg_cnt;
    RK_S32 st_cfg_cnt = cfg->st_cfg_cnt;
    RK_S32 cpb_st_used_size = 0;
    RK_S32 seq_idx = 0;
    RK_S32 st_idx;
    RK_S32 walk_len = MPP_MAX(lt_cfg_cnt, st_cfg_cnt);

    if (cfg->ready)
        goto DONE;

    cleanup_cpb_refs(cpb);

    cfg->max_tlayers = cpb->info.max_st_tid + 1;
    enc_refs_dbg_flow("dryrun start: lt_cfg %d st_cfg %d\n",
                      lt_cfg_cnt, st_cfg_cnt);

    for (st_idx = 0; st_idx < walk_len; st_idx++) {
        st_cfg = &cfg->st_cfg[st_idx % st_cfg_cnt];
        EncFrmStatus frm;
        RK_S32 repeat = (st_cfg->repeat) ? st_cfg->repeat : 1;

        while (repeat-- > 0) {
            /* step 1. updated by st_cfg */
            set_st_cfg_to_frm(&frm, seq_idx++, st_cfg);
            set_frm_refresh_flag(&frm, p);

            /* step 2. updated by lt_cfg */
            RefsCnt *lt_cfg = &cpb->lt_cnter[0];
            RK_S32 set_to_lt = 0;
            RK_S32 i;

            for (i = 0; i < lt_cfg_cnt; i++, lt_cfg++) {
                if (lt_cfg->delay_cnt) {
                    lt_cfg->delay_cnt--;
                    continue;
                }

                if (!set_to_lt) {
                    if (!lt_cfg->cnt) {
                        set_lt_cfg_to_frm(&frm, lt_cfg);
                        set_to_lt = 1;
                    }
                }

                lt_cfg->cnt++;
                if (lt_cfg->cnt >= lt_cfg->len) {
                    if (lt_cfg->len) {
                        /* when there is loop len loop lt_cfg */
                        lt_cfg->cnt = 0;
                        lt_cfg->idx++;
                    } else {
                        /* else just set lt_cfg once */
                        lt_cfg->cnt = 1;
                        lt_cfg->idx = 1;
                    }
                }
            }

            /* step 3. try find ref by the ref_mode and update used cpb size */
            EncFrmStatus *ref = get_ref_from_cpb(cpb, &frm);

            if (ref) {
                RK_S32 cpb_pos = check_ref_cpb_pos(cpb, ref);

                if (cpb_pos < MAX_CPB_ST_FRM) {
                    if (cpb_st_used_size < cpb_pos + 1) {
                        cpb_st_used_size = cpb_pos + 1;
                        enc_refs_dbg_flow("cpb_st_used_size update to %d\n", cpb_st_used_size);
                    }
                }
            }

            /* step 4. store frame according to status */
            store_ref_to_cpb(cpb, &frm);
        }
    }

    cleanup_cpb_refs(cpb);
    info->max_st_cnt = cpb_st_used_size ? cpb_st_used_size : 1;

DONE:
    info->dpb_size = info->max_lt_cnt + info->max_st_cnt;

    enc_refs_dbg_size("dryrun success: cpb size %d\n", info->dpb_size);

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_set_usr_cfg(MppEncRefs refs, MppEncRefFrmUsrCfg *cfg)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    memcpy(&p->usr_cfg, cfg, sizeof(p->usr_cfg));
    if (cfg->force_flag)
        p->changed |= ENC_REFS_USR_CFG_CHANGED;

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_set_rc_igop(MppEncRefs refs, RK_S32 igop)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;

    if (p->igop != igop) {
        p->igop = igop;
        p->changed |= ENC_REFS_IGOP_CHANGED;
    }

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_set_refresh_length(MppEncRefs refs, RK_S32 len)
{
    MPP_RET ret = MPP_OK;
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;

    if (len < p->igop) {
        p->refresh_length = len;
    } else {
        p->refresh_length = p->igop;
        ret = MPP_ERR_VALUE;
        goto RET;
    }
    enc_refs_dbg_func("leave %p\n", refs);
RET:
    return ret;
}

RK_S32 mpp_enc_refs_update_hdr(MppEncRefs refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return 0;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    RK_S32 hdr_need_update = p->hdr_need_update;

    enc_refs_dbg_func("leave %p\n", refs);
    return hdr_need_update;
}

MPP_RET mpp_enc_refs_get_cpb_info(MppEncRefs refs, MppEncCpbInfo *info)
{
    if (NULL == refs || NULL == info) {
        mpp_err_f("invalid input refs %p info %p\n", refs, info);
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    memcpy(info, &p->cpb.info, sizeof(*info));

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

static RK_S32 get_cpb_st_cfg_pos(EncVirtualCpb *cpb, MppEncRefCfgImpl *cfg)
{
    RK_S32 st_cfg_pos = cpb->st_cfg_pos;
    RK_S32 st_cfg_cnt = cfg->st_cfg_cnt;

    /* NOTE: second loop will start from 1 */
    if (st_cfg_pos >= st_cfg_cnt)
        st_cfg_pos = (st_cfg_cnt > 1) ? (1) : (0);

    return st_cfg_pos;
}

MPP_RET mpp_enc_refs_get_cpb(MppEncRefs refs, EncCpbStatus *status)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    MppEncRefCfgImpl *cfg = p->ref_cfg;
    EncVirtualCpb *cpb = &p->cpb;
    MppEncRefStFrmCfg *st_cfg = NULL;
    MppEncRefFrmUsrCfg *usr_cfg = &p->usr_cfg;
    EncFrmStatus *frm = &status->curr;
    EncFrmStatus *ref = &status->refr;
    RefsCnt *lt_cfg = cpb->lt_cnter;
    RK_S32 set_to_lt = 0;
    RK_S32 cleanup_cpb = 0;
    RK_S32 prev_frm_is_pass1 = frm->save_pass1;
    RK_S32 i;

    /* step 1. check igop from cfg_set and force idr for usr_cfg */
    if (p->changed & ENC_REFS_IGOP_CHANGED)
        cleanup_cpb = 1;

    if (p->igop && (cpb->seq_idx >= p->igop)) {
        if (p->refresh_length) {
            p->cpb.seq_cnt = cpb->seq_idx / p->igop + 1;
        } else
            cleanup_cpb = 1;
    }

    if (usr_cfg->force_flag & ENC_FORCE_IDR) {
        usr_cfg->force_flag &= (~ENC_FORCE_IDR);
        cleanup_cpb = 1;
        p->cpb.seq_cnt = 0;
    }

    if (cleanup_cpb) {
        /* update seq_idx for igop loop and force idr */
        cleanup_cpb_refs(cpb);
    } else if (p->changed & ENC_REFS_REF_CFG_CHANGED) {
        cpb->st_cfg_pos = 0;
        cpb->st_cfg_repeat_pos = 0;
    }

    p->changed = 0;

    cpb->frm_idx++;
    cpb->st_cfg_pos = get_cpb_st_cfg_pos(cpb, cfg);
    st_cfg = &cfg->st_cfg[cpb->st_cfg_pos];
    /* step 2. updated by st_cfg */
    set_st_cfg_to_frm(frm, cpb->seq_idx++, st_cfg);
    set_frm_refresh_flag(frm, p);

    lt_cfg = p->cpb.lt_cnter;

    /* step 3. updated by lt_cfg */
    for (i = 0; i < cfg->lt_cfg_cnt; i++, lt_cfg++) {
        if (lt_cfg->delay_cnt) {
            lt_cfg->delay_cnt--;
            continue;
        }

        if (!set_to_lt) {
            if (!lt_cfg->cnt) {
                set_lt_cfg_to_frm(frm, lt_cfg);
                set_to_lt = 1;
            }
        }

        lt_cfg->cnt++;
        if (lt_cfg->cnt >= lt_cfg->len) {
            if (lt_cfg->len) {
                /* when there is loop len loop lt_cfg */
                lt_cfg->cnt = 0;
                lt_cfg->idx++;
            } else {
                /* else just set lt_cfg once */
                lt_cfg->cnt = 1;
                lt_cfg->idx = 1;
            }
        }
    }

    /* step 4. process force flags and force ref_mode */
    if (usr_cfg->force_flag & ENC_FORCE_LT_REF_IDX) {
        frm->is_non_ref = 0;
        frm->is_lt_ref = 1;
        frm->lt_idx = usr_cfg->force_lt_idx;
        if (frm->is_idr && frm->lt_idx) {
            frm->lt_idx = 0;
            mpp_err_f("can not set IDR to ltr with non-zero index\n");
        }
        /* lt_ref will be forced to tid 0 */
        frm->temporal_id = 0;

        /* reset st_cfg to next loop */
        cpb->st_cfg_repeat_pos = 0;
        cpb->st_cfg_pos = 0;
        usr_cfg->force_flag &= ~ENC_FORCE_LT_REF_IDX;
    }

    if (usr_cfg->force_flag & ENC_FORCE_TEMPORAL_ID) {
        if (usr_cfg->force_temporal_id >= cfg->max_tlayers ||
            frm->is_idr || frm->is_lt_ref)
            mpp_err_f("Invalid temporal_id %d, frm is %s\n", usr_cfg->force_temporal_id,
                      frm->is_idr ? "IDR" : (frm->is_lt_ref ? "LTR" : "st"));
        else
            frm->temporal_id = usr_cfg->force_temporal_id;

        usr_cfg->force_flag &= ~ENC_FORCE_TEMPORAL_ID;
    }

    if (usr_cfg->force_flag & ENC_FORCE_REF_MODE) {
        frm->ref_mode = usr_cfg->force_ref_mode;
        frm->ref_arg = usr_cfg->force_ref_arg;

        usr_cfg->force_flag &= ~ENC_FORCE_REF_MODE;
    }

    if (usr_cfg->force_flag & ENC_FORCE_PSKIP_NON_REF) {
        frm->is_non_ref = 1;

        usr_cfg->force_flag &= ~ENC_FORCE_PSKIP_NON_REF;
    }

    if (usr_cfg->force_flag & ENC_FORCE_PSKIP_IS_REF) {
        frm->force_pskip_is_ref = 1;

        usr_cfg->force_flag &= ~ENC_FORCE_PSKIP_IS_REF;
    }

    frm->non_recn = frm->is_non_ref || (p->igop == 1);

    /* update st_cfg for st_cfg loop */
    cpb->st_cfg_repeat_pos++;
    if (cpb->st_cfg_repeat_pos > st_cfg->repeat) {
        cpb->st_cfg_repeat_pos = 0;
        cpb->st_cfg_pos++;
    }

    /* step 4. try find ref by the ref_mode */
    EncFrmStatus *ref_found = get_ref_from_cpb(&p->cpb, frm);
    if (ref_found) {
        RK_S32 cpb_idx = check_ref_cpb_pos(&p->cpb, ref_found);

        mpp_assert(cpb_idx >= 0);
        cpb->list0[0].val = ref->val;
        ref->val = ref_found->val;
    } else
        ref->val = 0;

    /* step 5. check use previous pass one frame as input */
    if (prev_frm_is_pass1)
        frm->use_pass1 = 1;

    if (enc_refs_debug & MPP_ENC_REFS_DBG_FRM) {
        mpp_log_f("frm status:\n");
        dump_frm(frm);
        mpp_log_f("ref status:\n");
        dump_frm(ref);
    }

    /* step 5. generate cpb init */
    memset(status->init, 0, sizeof(status->init));
    save_cpb_status(&p->cpb, status->init);
    // TODO: cpb_init must be the same to cpb_final

    /* step 6. store frame according to status */
    store_ref_to_cpb(&p->cpb, frm);

    /* step 7. generate cpb final */
    memset(status->final, 0, sizeof(status->final));
    save_cpb_status(&p->cpb, status->final);

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

RK_S32 mpp_enc_refs_next_frm_is_intra(MppEncRefs refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    EncVirtualCpb *cpb = &p->cpb;
    MppEncRefFrmUsrCfg *usr_cfg = &p->usr_cfg;
    RK_S32 is_intra = 0;

    if (p->changed & ENC_REFS_IGOP_CHANGED)
        is_intra = 1;

    if (p->igop && cpb->seq_idx >= p->igop)
        is_intra = 1;

    if (usr_cfg->force_flag & ENC_FORCE_IDR)
        is_intra = 1;

    if (!cpb->frm_idx)
        is_intra = 0;

    enc_refs_dbg_func("leave %p\n", refs);

    return is_intra;
}

MPP_RET mpp_enc_refs_get_cpb_pass1(MppEncRefs refs, EncCpbStatus *status)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    EncVirtualCpb *cpb = &p->cpb;
    EncFrmStatus *frm = &status->curr;
    EncFrmStatus *ref = &status->refr;

    frm->valid = 1;
    frm->save_pass1 = 1;
    frm->is_non_ref = 1;
    frm->is_lt_ref = 0;
    frm->temporal_id = 0;
    frm->ref_mode = REF_TO_PREV_REF_FRM;
    frm->ref_arg = 0;
    frm->non_recn = 0;

    /* step 4. try find ref by the ref_mode */
    EncFrmStatus *ref_found = get_ref_from_cpb(cpb, frm);
    if (ref_found) {
        RK_S32 cpb_idx = check_ref_cpb_pos(cpb, ref_found);

        mpp_assert(cpb_idx >= 0);
        cpb->list0[0].val = ref->val;
        ref->val = ref_found->val;
    } else
        ref->val = 0;

    if (enc_refs_debug & MPP_ENC_REFS_DBG_FRM) {
        mpp_log_f("frm status:\n");
        dump_frm(frm);
        mpp_log_f("ref status:\n");
        dump_frm(ref);
    }

    /* step 5. generate cpb init */
    memset(status->init, 0, sizeof(status->init));
    save_cpb_status(cpb, status->init);
    // TODO: cpb_init must be the same to cpb_final

    /* step 6. store frame according to status */
    store_ref_to_cpb(cpb, frm);

    /* step 7. generate cpb final */
    memset(status->final, 0, sizeof(status->final));
    save_cpb_status(cpb, status->final);

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}
MPP_RET mpp_enc_refs_stash(MppEncRefs refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    memcpy(&p->cpb_stash, &p->cpb, sizeof(p->cpb_stash));

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}

MPP_RET mpp_enc_refs_rollback(MppEncRefs refs)
{
    if (NULL == refs) {
        mpp_err_f("invalid NULL input refs\n");
        return MPP_ERR_VALUE;
    }

    enc_refs_dbg_func("enter %p\n", refs);

    MppEncRefsImpl *p = (MppEncRefsImpl *)refs;
    memcpy(&p->cpb, &p->cpb_stash, sizeof(p->cpb));

    enc_refs_dbg_func("leave %p\n", refs);
    return MPP_OK;
}
