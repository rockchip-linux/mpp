/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#include "hal_enc_task.h"
#include "hal_h265e_vepu511a_reg.h"

typedef struct HalH265eVepu511aTune_t {
    H265eV511AHalContext *ctx;

    MppBuffer md_info; /* internal md info buffer */
    RK_U8 *qm_mv_buf; /* qpmap move flag buffer */
    RK_U32 qm_mv_buf_size;
} HalH265eVepu511aTune;

static HalH265eVepu511aTune *vepu511a_h265e_tune_init(H265eV511AHalContext *ctx)
{
    HalH265eVepu511aTune *tune = mpp_calloc(HalH265eVepu511aTune, 1);

    hal_h265e_dbg_func("enter\n");

    if (NULL == tune)
        return tune;

    tune->ctx = ctx;

    hal_h265e_dbg_func("leave\n");

    return tune;
}

static void vepu511a_h265e_tune_deinit(void *tune)
{
    HalH265eVepu511aTune *t = (HalH265eVepu511aTune *)tune;

    hal_h265e_dbg_func("enter\n");

    if (t->md_info) {
        mpp_buffer_put(t->md_info);
        t->md_info = NULL;
    }

    MPP_FREE(t->qm_mv_buf);
    MPP_FREE(tune);
    hal_h265e_dbg_func("leave\n");
}

static MPP_RET vepu511a_h265e_tune_qpmap_init(HalH265eVepu511aTune *tune, MppBuffer md_info)
{
    H265eV511AHalContext *ctx = tune->ctx;
    Vepu511aH265eFrmCfg *frm = ctx->frm;
    H265eV511ARegSet *regs = frm->regs_set;
    H265eVepu511aFrame *reg_frm = &regs->reg_frm;
    RK_S32 w32 = MPP_ALIGN(ctx->cfg->prep.width, 32);
    RK_S32 h32 = MPP_ALIGN(ctx->cfg->prep.height, 32);
    RK_S32 roir_buf_fd = -1;

    hal_h265e_dbg_func("enter\n");

    if (frm->roi_data) {
        //TODO: external qpmap buffer
    } else {
        if (NULL == frm->roir_buf) {
            if (NULL == ctx->roi_grp)
                mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);

            //TODO: bmap_mdc_dpth = 1 ???
            frm->roir_buf_size = w32 * h32 / 256 * 4;
            mpp_buffer_get(ctx->roi_grp, &frm->roir_buf, frm->roir_buf_size);
        }

        roir_buf_fd = mpp_buffer_get_fd(frm->roir_buf);
    }

    if (NULL == frm->roir_buf) {
        mpp_err("failed to get roir_buf\n");
        return MPP_ERR_MALLOC;
    }
    reg_frm->common.adr_roir = roir_buf_fd;

    if (NULL == tune->qm_mv_buf) {
        tune->qm_mv_buf_size = w32 * h32 / 256;
        tune->qm_mv_buf = mpp_calloc(RK_U8, tune->qm_mv_buf_size);
        if (NULL == tune->qm_mv_buf) {
            mpp_err("failed to get qm_mv_buf\n");
            return MPP_ERR_MALLOC;
        }
    }

    if (NULL == md_info) {
        RK_S32 meir_size = w32 * h32 / 256 * 4; /* max 4 bytes for each CU16 */
        if (NULL == tune->md_info) {
            mpp_buffer_get(NULL, &tune->md_info, meir_size);
        }

        if (tune->md_info) {
            reg_frm->common.enc_pic.mei_stor = 1;
            reg_frm->common.meiw_addr = mpp_buffer_get_fd(tune->md_info);
        } else {
            reg_frm->common.enc_pic.mei_stor = 0;
            reg_frm->common.meiw_addr = 0;
            return MPP_ERR_MALLOC;
        }
        hal_h265e_dbg_ctl("md_info_internal %p, size %d\n", tune->md_info, meir_size);
    }

    hal_h265e_dbg_ctl("roir_buf_fd %d, size %d qm_mv_buf %p size %d\n",
                      roir_buf_fd, frm->roir_buf_size, tune->qm_mv_buf,
                      tune->qm_mv_buf_size);
    hal_h265e_dbg_func("leave\n");

    return MPP_OK;
}

static void vepu511a_h265e_tune_qpmap(void *p, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH265eVepu511aTune *tune = (HalH265eVepu511aTune *)p;

    (void)task;
    hal_h265e_dbg_func("enter\n");

    ret = vepu511a_h265e_tune_qpmap_init(tune, task->md_info);
    if (ret != MPP_OK) {
        mpp_err("failed to init qpmap\n");
        return;
    }

    hal_h265e_dbg_func("leave\n");
}

static void vepu511a_h265e_tune_reg_patch(void *p, HalEncTask *task)
{
    HalH265eVepu511aTune *tune = (HalH265eVepu511aTune *)p;

    hal_h265e_dbg_func("enter\n");

    if (NULL == tune)
        return;
    H265eV511AHalContext *ctx = tune->ctx;

    if (ctx->qpmap_en) {
        vepu511a_h265e_tune_qpmap(tune, task);
    }

    hal_h265e_dbg_func("leave\n");
}