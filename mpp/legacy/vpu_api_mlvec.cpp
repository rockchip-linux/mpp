/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "vpu_api_mlvec"

#include <fcntl.h>
#include "string.h"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "vpu_api_mlvec.h"
#include "vpu_api_legacy.h"

#define VPU_API_DBG_MLVEC_FUNC      (0x00010000)
#define VPU_API_DBG_MLVEC_FLOW      (0x00020000)

#define mlvec_dbg_func(fmt, ...)    vpu_api_dbg_f(VPU_API_DBG_MLVEC_FUNC, fmt, ## __VA_ARGS__)
#define mlvec_dbg_flow(fmt, ...)    vpu_api_dbg_f(VPU_API_DBG_MLVEC_FLOW, fmt, ## __VA_ARGS__)

typedef struct VpuApiMlvecImpl_t {
    MppCtx      mpp;
    MppApi      *mpi;
    MppEncCfg   enc_cfg;

    VpuApiMlvecStaticCfg    st_cfg;
    VpuApiMlvecDynamicCfg   dy_cfg;
} VpuApiMlvecImpl;

MPP_RET vpu_api_mlvec_init(VpuApiMlvec *ctx)
{
    if (NULL == ctx) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mlvec_dbg_func("enter %p\n", ctx);

    VpuApiMlvecImpl *impl = mpp_calloc(VpuApiMlvecImpl, 1);
    if (NULL == impl)
        mpp_err_f("failed to create MLVEC context\n");

    mpp_assert(sizeof(VpuApiMlvecStaticCfg) == sizeof(EncParameter_t));
    /* default disable frame_qp setup */
    impl->dy_cfg.frame_qp = -1;

    *ctx = impl;

    mlvec_dbg_func("leave %p %p\n", ctx, impl);
    return (impl) ? (MPP_OK) : (MPP_NOK);
}

MPP_RET vpu_api_mlvec_deinit(VpuApiMlvec ctx)
{
    mlvec_dbg_func("enter %p\n", ctx);
    MPP_FREE(ctx);
    mlvec_dbg_func("leave %p\n", ctx);
    return MPP_OK;
}

MPP_RET vpu_api_mlvec_setup(VpuApiMlvec ctx, MppCtx mpp, MppApi *mpi, MppEncCfg enc_cfg)
{
    if (NULL == ctx || NULL == mpp || NULL == mpi || NULL == enc_cfg) {
        mpp_err_f("invalid NULL input ctx %p mpp %p mpi %p cfg %p\n",
                  ctx, mpp, mpi, enc_cfg);
        return MPP_ERR_NULL_PTR;
    }

    mlvec_dbg_func("enter %p\n", ctx);

    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    impl->mpp = mpp;
    impl->mpi = mpi;
    impl->enc_cfg = enc_cfg;

    mlvec_dbg_func("leave %p\n", ctx);

    return MPP_OK;
}

MPP_RET vpu_api_mlvec_check_cfg(void *p)
{
    if (NULL == p) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    VpuApiMlvecStaticCfg *cfg = (VpuApiMlvecStaticCfg *)p;
    RK_U32 magic = cfg->magic;
    MPP_RET ret = MPP_OK;

    if ((((magic >> 24) & 0xff) != MLVEC_MAGIC) ||
        (((magic >> 16) & 0xff) != MLVEC_VERSION))
        ret = MPP_NOK;

    mlvec_dbg_flow("check mlvec cfg magic %08x %s\n", magic,
                   (ret == MPP_OK) ? "success" : "failed");

    return ret;
}

MPP_RET vpu_api_mlvec_set_st_cfg(VpuApiMlvec ctx, VpuApiMlvecStaticCfg *cfg)
{
    if (NULL == ctx || NULL == cfg) {
        mpp_err_f("invalid NULL input ctx %p cfg %p\n");
        return MPP_ERR_NULL_PTR;
    }

    mlvec_dbg_func("enter ctx %p cfg %p\n", ctx, cfg);

    /* check mlvec magic word */
    if (vpu_api_mlvec_check_cfg(cfg))
        return MPP_NOK;

    MPP_RET ret = MPP_OK;
    /* update static configure */
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;

    memcpy(&impl->st_cfg, cfg, sizeof(impl->st_cfg));
    cfg = &impl->st_cfg;

    /* get mpp context and check */
    MppCtx mpp_ctx = impl->mpp;
    MppApi *mpi = impl->mpi;
    MppEncCfg enc_cfg = impl->enc_cfg;

    mpp_assert(mpp_ctx);
    mpp_assert(mpi);
    mpp_assert(enc_cfg);

    /* start control mpp */
    mlvec_dbg_flow("hdr_on_idr %d\n", cfg->hdr_on_idr);
    MppEncHeaderMode mode = cfg->hdr_on_idr ?
                            MPP_ENC_HEADER_MODE_EACH_IDR :
                            MPP_ENC_HEADER_MODE_DEFAULT;

    ret = mpi->control(mpp_ctx, MPP_ENC_SET_HEADER_MODE, &mode);
    if (ret)
        mpp_err("setup enc header mode %d failed ret %d\n", mode, ret);

    mlvec_dbg_flow("add_prefix %d\n", cfg->add_prefix);
    mpp_enc_cfg_set_s32(enc_cfg, "h264:prefix_mode", cfg->add_prefix);

    mlvec_dbg_flow("slice_mbs  %d\n", cfg->slice_mbs);
    if (cfg->slice_mbs) {
        mpp_enc_cfg_set_u32(enc_cfg, "split:mode", MPP_ENC_SPLIT_BY_CTU);
        mpp_enc_cfg_set_u32(enc_cfg, "split:arg", cfg->slice_mbs);
    } else
        mpp_enc_cfg_set_u32(enc_cfg, "split:mode", MPP_ENC_SPLIT_NONE);

    /* NOTE: ltr_frames is already configured */
    vpu_api_mlvec_set_dy_max_tid(ctx, cfg->max_tid);

    mlvec_dbg_func("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}

MPP_RET vpu_api_mlvec_set_dy_cfg(VpuApiMlvec ctx, VpuApiMlvecDynamicCfg *cfg, MppMeta meta)
{
    if (NULL == ctx || NULL == cfg || NULL == meta) {
        mpp_err_f("invalid NULL input ctx %p cfg %p meta %p\n",
                  ctx, cfg, meta);
        return MPP_ERR_NULL_PTR;
    }

    mlvec_dbg_func("enter ctx %p cfg %p meta %p\n", ctx, cfg, meta);

    MPP_RET ret = MPP_OK;
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    VpuApiMlvecDynamicCfg *dst = &impl->dy_cfg;

    /* clear non-sticky flag first */
    dst->mark_ltr   = -1;
    dst->use_ltr    = -1;
    /* frame qp and base layer pid is sticky flag */

    /* update flags */
    if (cfg->updated) {
        if (cfg->updated & VPU_API_ENC_MARK_LTR_UPDATED)
            dst->mark_ltr = cfg->mark_ltr;

        if (cfg->updated & VPU_API_ENC_USE_LTR_UPDATED)
            dst->use_ltr = cfg->use_ltr;

        if (cfg->updated & VPU_API_ENC_FRAME_QP_UPDATED)
            dst->frame_qp = cfg->frame_qp;

        if (cfg->updated & VPU_API_ENC_BASE_PID_UPDATED)
            dst->base_layer_pid = cfg->base_layer_pid;

        /* dynamic max temporal layer count updated go through mpp ref cfg */
        cfg->updated = 0;
    }

    mlvec_dbg_flow("ltr mark %2d use %2d frm qp %2d blpid %d\n", dst->mark_ltr,
                   dst->use_ltr, dst->frame_qp, dst->base_layer_pid);

    /* setup next frame configure */
    if (dst->mark_ltr >= 0)
        mpp_meta_set_s32(meta, KEY_ENC_MARK_LTR, dst->mark_ltr);

    if (dst->use_ltr >= 0)
        mpp_meta_set_s32(meta, KEY_ENC_USE_LTR, dst->use_ltr);

    if (dst->frame_qp >= 0)
        mpp_meta_set_s32(meta, KEY_ENC_FRAME_QP, dst->frame_qp);

    if (dst->base_layer_pid >= 0)
        mpp_meta_set_s32(meta, KEY_ENC_BASE_LAYER_PID, dst->base_layer_pid);

    mlvec_dbg_func("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}

MPP_RET vpu_api_mlvec_set_dy_max_tid(VpuApiMlvec ctx, RK_S32 max_tid)
{
    if (NULL == ctx) {
        mpp_err_f("invalid NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    mlvec_dbg_func("enter ctx %p max_tid %d\n", ctx, max_tid);

    MPP_RET ret = MPP_OK;
    VpuApiMlvecImpl *impl = (VpuApiMlvecImpl *)ctx;
    MppCtx mpp_ctx = impl->mpp;
    MppApi *mpi = impl->mpi;
    MppEncCfg enc_cfg = impl->enc_cfg;

    mpp_assert(mpp_ctx);
    mpp_assert(mpi);
    mpp_assert(enc_cfg);

    MppEncRefLtFrmCfg lt_ref[16];
    MppEncRefStFrmCfg st_ref[16];
    RK_S32 lt_cfg_cnt = 0;
    RK_S32 st_cfg_cnt = 0;
    RK_S32 tid0_loop = 0;
    RK_S32 ltr_frames = impl->st_cfg.ltr_frames;

    memset(lt_ref, 0, sizeof(lt_ref));
    memset(st_ref, 0, sizeof(st_ref));

    mlvec_dbg_flow("ltr_frames %d\n", ltr_frames);
    mlvec_dbg_flow("max_tid    %d\n", max_tid);

    switch (max_tid) {
    case 0 : {
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;

        st_cfg_cnt = 1;
        tid0_loop = 1;
        mlvec_dbg_flow("no tsvc\n");
    } break;
    case 1 : {
        /* set tsvc2 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 1 - non-ref */
        st_ref[1].is_non_ref    = 1;
        st_ref[1].temporal_id   = 1;
        st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 0 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 0;
        st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;

        st_cfg_cnt = 3;
        tid0_loop = 2;
        mlvec_dbg_flow("tsvc2\n");
    } break;
    case 2 : {
        /* set tsvc3 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 2 - non-ref */
        st_ref[1].is_non_ref    = 0;
        st_ref[1].temporal_id   = 2;
        st_ref[1].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 1 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 1;
        st_ref[2].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;
        /* st 3 layer 2 - non-ref */
        st_ref[3].is_non_ref    = 0;
        st_ref[3].temporal_id   = 2;
        st_ref[3].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[3].ref_arg       = 1;
        st_ref[3].repeat        = 0;
        /* st 4 layer 0 - ref */
        st_ref[4].is_non_ref    = 0;
        st_ref[4].temporal_id   = 0;
        st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[4].ref_arg       = 0;
        st_ref[4].repeat        = 0;

        st_cfg_cnt = 5;
        tid0_loop = 4;
        mlvec_dbg_flow("tsvc3\n");
    } break;
    case 3 : {
        /* set tsvc3 st-ref struct */
        /* st 0 layer 0 - ref */
        st_ref[0].is_non_ref    = 0;
        st_ref[0].temporal_id   = 0;
        st_ref[0].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[0].ref_arg       = 0;
        st_ref[0].repeat        = 0;
        /* st 1 layer 3 - non-ref */
        st_ref[1].is_non_ref    = 1;
        st_ref[1].temporal_id   = 3;
        st_ref[1].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[1].ref_arg       = 0;
        st_ref[1].repeat        = 0;
        /* st 2 layer 2 - ref */
        st_ref[2].is_non_ref    = 0;
        st_ref[2].temporal_id   = 2;
        st_ref[2].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[2].ref_arg       = 0;
        st_ref[2].repeat        = 0;
        /* st 3 layer 3 - non-ref */
        st_ref[3].is_non_ref    = 1;
        st_ref[3].temporal_id   = 3;
        st_ref[3].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[3].ref_arg       = 0;
        st_ref[3].repeat        = 0;
        /* st 4 layer 1 - ref */
        st_ref[4].is_non_ref    = 0;
        st_ref[4].temporal_id   = 1;
        st_ref[4].ref_mode      = REF_TO_TEMPORAL_LAYER;
        st_ref[4].ref_arg       = 0;
        st_ref[4].repeat        = 0;
        /* st 5 layer 3 - non-ref */
        st_ref[5].is_non_ref    = 1;
        st_ref[5].temporal_id   = 3;
        st_ref[5].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[5].ref_arg       = 0;
        st_ref[5].repeat        = 0;
        /* st 6 layer 2 - ref */
        st_ref[6].is_non_ref    = 0;
        st_ref[6].temporal_id   = 2;
        st_ref[6].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[6].ref_arg       = 0;
        st_ref[6].repeat        = 0;
        /* st 7 layer 3 - non-ref */
        st_ref[7].is_non_ref    = 1;
        st_ref[7].temporal_id   = 3;
        st_ref[7].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[7].ref_arg       = 0;
        st_ref[7].repeat        = 0;
        /* st 8 layer 0 - ref */
        st_ref[8].is_non_ref    = 0;
        st_ref[8].temporal_id   = 0;
        st_ref[8].ref_mode      = REF_TO_PREV_REF_FRM;
        st_ref[8].ref_arg       = 0;
        st_ref[8].repeat        = 0;

        st_cfg_cnt = 9;
        tid0_loop = 8;
        mlvec_dbg_flow("tsvc4\n");
    } break;
    default : {
        mpp_err("invalid max temporal layer id %d\n", max_tid);
    } break;
    }

    if (ltr_frames) {
        RK_S32 i;

        lt_cfg_cnt = ltr_frames;
        mpp_assert(ltr_frames <= MPP_ENC_MAX_LT_REF_NUM);
        for (i = 0; i < ltr_frames; i++) {
            lt_ref[i].lt_idx        = i;
            lt_ref[i].temporal_id   = 0;
            lt_ref[i].ref_mode      = REF_TO_PREV_LT_REF;
            lt_ref[i].lt_gap        = 0;
            lt_ref[i].lt_delay      = tid0_loop * i;
        }
    }

    if (lt_cfg_cnt)
        mpp_assert(st_cfg_cnt);

    mlvec_dbg_flow("lt_cfg_cnt %d st_cfg_cnt %d\n", lt_cfg_cnt, st_cfg_cnt);
    if (lt_cfg_cnt || st_cfg_cnt) {
        MppEncRefCfg ref = NULL;

        mpp_enc_ref_cfg_init(&ref);

        ret = mpp_enc_ref_cfg_set_cfg_cnt(ref, lt_cfg_cnt, st_cfg_cnt);
        ret = mpp_enc_ref_cfg_add_lt_cfg(ref, lt_cfg_cnt, lt_ref);
        ret = mpp_enc_ref_cfg_add_st_cfg(ref, st_cfg_cnt, st_ref);
        ret = mpp_enc_ref_cfg_set_keep_cpb(ref, 1);
        ret = mpp_enc_ref_cfg_check(ref);

        ret = mpi->control(mpp_ctx, MPP_ENC_SET_REF_CFG, ref);
        if (ret)
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);

        mpp_enc_ref_cfg_deinit(&ref);
    } else {
        ret = mpi->control(mpp_ctx, MPP_ENC_SET_REF_CFG, NULL);
        if (ret)
            mpp_err("mpi control enc set ref cfg failed ret %d\n", ret);
    }

    mlvec_dbg_func("leave ctx %p ret %d\n", ctx, ret);

    return ret;
}
