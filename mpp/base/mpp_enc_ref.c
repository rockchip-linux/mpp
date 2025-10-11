/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_enc_ref"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"

#include "mpp_rc_defs.h"
#include "mpp_enc_ref.h"
#include "mpp_enc_refs.h"

#define setup_mpp_enc_ref_cfg(ref) \
    ((MppEncRefCfgImpl*)ref)->name = MODULE_TAG;

MPP_RET _check_is_mpp_enc_ref_cfg(const char *func, void *ref)
{
    if (NULL == ref) {
        mpp_err("%s input ref check NULL failed\n", func);
        return MPP_NOK;
    }

    if (strcmp(((MppEncRefCfgImpl*)(ref))->name, MODULE_TAG) != 0) {
        mpp_err("%s input ref check %p %p failed\n", func, ((MppEncRefCfgImpl*)(ref))->name);
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_init(MppEncRefCfg *ref)
{
    if (NULL == ref) {
        mpp_err_f("invalid NULL input ref_cfg\n");
        return MPP_ERR_NULL_PTR;
    }

    MppEncRefCfgImpl *p = mpp_calloc(MppEncRefCfgImpl, 1);
    *ref = p;
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("enc_ref_cfg_debug", &p->debug, 0);

    setup_mpp_enc_ref_cfg(p);

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_deinit(MppEncRefCfg *ref)
{
    if (!ref || check_is_mpp_enc_ref_cfg(*ref)) {
        mpp_err_f("input %p check failed\n", ref);
        return MPP_ERR_VALUE;
    }

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)(*ref);
    MPP_FREE(p->lt_cfg);
    MPP_FREE(p->st_cfg);
    MPP_FREE(p);

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_reset(MppEncRefCfg ref)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_VALUE;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;
    MPP_FREE(p->lt_cfg);
    MPP_FREE(p->st_cfg);
    memset(p, 0, sizeof(*p));
    setup_mpp_enc_ref_cfg(p);

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_set_cfg_cnt(MppEncRefCfg ref, RK_S32 lt_cnt, RK_S32 st_cnt)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_NULL_PTR;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;
    MppEncRefLtFrmCfg *lt_cfg = p->lt_cfg;
    MppEncRefStFrmCfg *st_cfg = p->st_cfg;;

    if (lt_cfg || st_cfg) {
        mpp_err_f("do call reset before setup new cnout\n");
        MPP_FREE(lt_cfg);
        MPP_FREE(st_cfg);
    }

    if (lt_cnt) {
        lt_cfg = mpp_calloc(MppEncRefLtFrmCfg, lt_cnt);
        if (NULL == lt_cfg)
            mpp_log_f("failed to create %d lt ref cfg\n", lt_cnt);
    }

    if (st_cnt) {
        st_cfg = mpp_calloc(MppEncRefStFrmCfg, st_cnt);
        if (NULL == st_cfg)
            mpp_log_f("failed to create %d st ref cfg\n", lt_cnt);
    }

    p->max_lt_cfg = lt_cnt;
    p->max_st_cfg = st_cnt;
    p->lt_cfg_cnt = 0;
    p->st_cfg_cnt = 0;
    p->lt_cfg = lt_cfg;
    p->st_cfg = st_cfg;

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_add_lt_cfg(MppEncRefCfg ref, RK_S32 cnt, MppEncRefLtFrmCfg *frm)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_VALUE;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;

    if (p->debug)
        mpp_log("ref %p add lt %d cfg idx %d tid %d gap %d delay %d ref mode %x\n",
                ref, p->lt_cfg_cnt, frm->lt_idx, frm->temporal_id,
                frm->lt_gap, frm->lt_delay, frm->ref_mode);

    memcpy(&p->lt_cfg[p->lt_cfg_cnt], frm, sizeof(*frm) * cnt);
    p->lt_cfg_cnt += cnt;

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_add_st_cfg(MppEncRefCfg ref, RK_S32 cnt, MppEncRefStFrmCfg *frm)
{
    if (check_is_mpp_enc_ref_cfg(ref)) {
        mpp_err_f("input %p check failed\n", ref);
        return MPP_ERR_VALUE;
    }

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;

    if (p->debug)
        mpp_log("ref %p add lt %d cfg non %d tid %d gap repeat %d ref mode %x arg %d\n",
                ref, p->st_cfg_cnt, frm->is_non_ref, frm->temporal_id,
                frm->repeat, frm->ref_mode, frm->ref_arg);

    memcpy(&p->st_cfg[p->st_cfg_cnt], frm, sizeof(*frm) * cnt);
    p->st_cfg_cnt += cnt;

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_check(MppEncRefCfg ref)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_VALUE;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;
    RK_S32 lt_cfg_cnt = p->lt_cfg_cnt;
    RK_S32 st_cfg_cnt = p->st_cfg_cnt;
    RK_S32 max_lt_ref_cnt   = 0;
    RK_S32 max_lt_ref_idx   = 0;
    RK_S32 lt_idx_used_mask = 0;
    RK_S32 lt_dryrun_length = 0;
    RK_S32 max_st_ref_cnt   = 0;
    RK_S32 max_st_tid       = 0;
    RK_S32 st_tid_used_mask = 0;
    RK_S32 st_dryrun_length = 0;
    RK_S32 ready = 1;

    /* parse and check gop config for encoder */
    if (lt_cfg_cnt) {
        RK_S32 pos = 0;
        MppEncRefLtFrmCfg *cfg = p->lt_cfg;

        for (pos = 0; pos < lt_cfg_cnt; pos++, cfg++) {
            MppEncRefMode ref_mode = cfg->ref_mode;
            RK_S32 temporal_id = cfg->temporal_id;
            RK_S32 lt_idx = cfg->lt_idx;
            RK_U32 lt_idx_mask = 1 << lt_idx;

            /* check lt idx */
            if (lt_idx >= MPP_ENC_MAX_LT_REF_NUM) {
                mpp_err_f("ref cfg %p lt cfg %d with invalid lt_idx %d larger than MPP_ENC_MAX_LT_REF_NUM\n",
                          ref, pos, lt_idx);
                ready = 0;
            }

            if (lt_idx_used_mask & lt_idx_mask) {
                mpp_err_f("ref cfg %p lt cfg %d with redefined lt_idx %d config\n",
                          ref, pos, lt_idx);
                ready = 0;
            }

            if (!(lt_idx_used_mask & lt_idx_mask)) {
                lt_idx_used_mask |= lt_idx_mask;
                max_lt_ref_cnt++;
            }

            if (lt_idx > max_lt_ref_idx)
                max_lt_ref_idx = lt_idx;

            /* check temporal id */
            if (temporal_id != 0) {
                mpp_err_f("ref cfg %p lt cfg %d with invalid temporal_id %d is non-zero\n",
                          ref, pos, temporal_id);
                ready = 0;
            }

            /* check gop mode */
            if (!REF_MODE_IS_GLOBAL(ref_mode) && !REF_MODE_IS_LT_MODE(ref_mode)) {
                mpp_err_f("ref cfg %p lt cfg %d with invalid ref mode %x\n",
                          ref, pos, ref_mode);
                ready = 0;
            }

            /* if check failed just quit */
            if (!ready)
                break;

            if (cfg->lt_gap && (cfg->lt_gap + cfg->lt_delay > lt_dryrun_length))
                lt_dryrun_length = cfg->lt_gap + cfg->lt_delay;
        }
    }

    /* check st-ref config */
    if (ready && st_cfg_cnt) {
        RK_S32 pos = 0;
        MppEncRefStFrmCfg *cfg = p->st_cfg;

        for (pos = 0; pos < st_cfg_cnt; pos++, cfg++) {
            MppEncRefMode ref_mode = cfg->ref_mode;
            RK_S32 temporal_id = cfg->temporal_id;
            RK_U32 tid_mask = 1 << temporal_id;

            /* check temporal_id */
            if (temporal_id > MPP_ENC_MAX_TEMPORAL_LAYER_NUM - 1) {
                mpp_err_f("ref cfg %p st cfg %d with invalid temporal_id %d larger than MPP_ENC_MAX_TEMPORAL_LAYER_NUM\n",
                          ref, pos, temporal_id);
                ready = 0;
            }

            /* check gop mode */
            if (!REF_MODE_IS_GLOBAL(ref_mode) && !REF_MODE_IS_ST_MODE(ref_mode)) {
                mpp_err_f("ref cfg %p st cfg %d with invalid ref mode %x\n",
                          ref, pos, ref_mode);
                ready = 0;
            }

            if (cfg->repeat < 0) {
                mpp_err_f("ref cfg %p st cfg %d with negative repeat %d set to zero\n",
                          ref, pos, cfg->repeat);
                cfg->repeat = 0;
            }

            /* constrain on head and tail frame */
            if (pos == 0 || (pos == st_cfg_cnt - 1)) {
                if (cfg->is_non_ref) {
                    mpp_err_f("ref cfg %p st cfg %d with invalid non-ref frame on head/tail frame\n",
                              ref, pos);
                    ready = 0;
                }

                if (temporal_id > 0) {
                    mpp_err_f("ref cfg %p st cfg %d with invalid non-zero temporal id %d on head/tail frame\n",
                              ref, pos, temporal_id);
                    ready = 0;
                }
            }

            if (!ready)
                break;

            if (!cfg->is_non_ref && !(st_tid_used_mask & tid_mask)) {
                max_st_ref_cnt++;
                st_tid_used_mask |= tid_mask;
            }

            if (temporal_id > max_st_tid)
                max_st_tid = temporal_id;

            st_dryrun_length++;
            st_dryrun_length += cfg->repeat;
        }
    }

    if (ready) {
        MppEncCpbInfo *cpb_info = &p->cpb_info;
        MppEncRefs refs = NULL;
        MPP_RET ret = MPP_OK;

        cpb_info->dpb_size = 0;
        cpb_info->max_lt_cnt = max_lt_ref_cnt;
        cpb_info->max_st_cnt = max_st_ref_cnt;
        cpb_info->max_lt_idx = max_lt_ref_idx;
        cpb_info->max_st_tid = max_st_tid;
        cpb_info->lt_gop     = lt_dryrun_length;
        cpb_info->st_gop     = st_dryrun_length - 1;

        ret = mpp_enc_refs_init(&refs);
        ready = (ret) ? 0 : (ready);
        ret = mpp_enc_refs_set_cfg(refs, ref);
        ready = (ret) ? 0 : (ready);
        ret = mpp_enc_refs_dryrun(refs);
        ready = (ret) ? 0 : (ready);

        /* update dpb size */
        ret = mpp_enc_refs_get_cpb_info(refs, cpb_info);
        ready = (ret) ? 0 : (ready);

        ret = mpp_enc_refs_deinit(&refs);
        ready = (ret) ? 0 : (ready);
    } else {
        mpp_err_f("check ref cfg %p failed\n", ref);
    }
    p->ready = ready;

    return ready ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_enc_ref_cfg_set_keep_cpb(MppEncRefCfg ref, RK_S32 keep)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_VALUE;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;
    p->keep_cpb = keep;

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_show(MppEncRefCfg ref)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return MPP_ERR_VALUE;

    return MPP_OK;
}

MPP_RET mpp_enc_ref_cfg_copy(MppEncRefCfg dst, MppEncRefCfg src)
{
    if (check_is_mpp_enc_ref_cfg(dst) || check_is_mpp_enc_ref_cfg(src))
        return MPP_ERR_VALUE;

    MPP_RET ret = MPP_OK;
    MppEncRefCfgImpl *d = (MppEncRefCfgImpl *)dst;
    MppEncRefCfgImpl *s = (MppEncRefCfgImpl *)src;
    RK_S32 max_lt_cfg = s->max_lt_cfg;
    RK_S32 max_st_cfg = s->max_st_cfg;

    /* step 1 - free cfg in dst */
    MPP_FREE(d->lt_cfg);
    MPP_FREE(d->st_cfg);

    /* step 2 - copy contex from src */
    memcpy(d, s, sizeof(*d));

    /* step 3 - create new storage and copy lt/st cfg */
    if (max_lt_cfg) {
        MppEncRefLtFrmCfg *lt_cfg = mpp_calloc(MppEncRefLtFrmCfg, max_lt_cfg);

        if (NULL == lt_cfg) {
            mpp_log_f("failed to create %d lt ref cfg\n", max_lt_cfg);
            ret = MPP_NOK;
        } else
            memcpy(lt_cfg, s->lt_cfg, sizeof(lt_cfg[0]) * s->max_lt_cfg);

        d->lt_cfg = lt_cfg;
    }

    if (max_st_cfg) {
        MppEncRefStFrmCfg *st_cfg = mpp_calloc(MppEncRefStFrmCfg, max_st_cfg);

        if (NULL == st_cfg) {
            mpp_log_f("failed to create %d st ref cfg\n", max_st_cfg);
            ret = MPP_NOK;
        } else
            memcpy(st_cfg, s->st_cfg, sizeof(st_cfg[0]) * s->max_st_cfg);

        d->st_cfg = st_cfg;
    }

    if (ret)
        mpp_enc_ref_cfg_reset(dst);

    return ret;
}

MppEncCpbInfo *mpp_enc_ref_cfg_get_cpb_info(MppEncRefCfg ref)
{
    if (check_is_mpp_enc_ref_cfg(ref))
        return NULL;

    MppEncRefCfgImpl *p = (MppEncRefCfgImpl *)ref;
    return &p->cpb_info;
}

static MppEncRefStFrmCfg default_st_ref_cfg = {
    .is_non_ref         = 0,
    .temporal_id        = 0,
    .ref_mode           = REF_TO_PREV_REF_FRM,
    .ref_arg            = 0,
    .repeat             = 0,
};

static const MppEncRefCfgImpl default_ref_cfg = {
    .name               = MODULE_TAG,
    .ready              = 1,
    .debug              = 0,
    .keep_cpb           = 0,
    .max_lt_cfg         = 0,
    .max_st_cfg         = 1,
    .lt_cfg_cnt         = 0,
    .st_cfg_cnt         = 1,
    .max_tlayers        = 1,
    .lt_cfg             = NULL,
    .st_cfg             = &default_st_ref_cfg,
    .cpb_info           = { 1, 0, 1, 0, 0, 0, 0 },
};

MppEncRefCfg mpp_enc_ref_default(void)
{
    return (MppEncRefCfg)&default_ref_cfg;
}
