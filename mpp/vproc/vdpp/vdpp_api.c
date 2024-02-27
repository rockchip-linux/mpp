/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "mpp_soc.h"
#include "mpp_mem.h"

#include "vdpp_api.h"
#include "vdpp.h"
#include "vdpp2.h"

vdpp_com_ctx *rockchip_vdpp_api_alloc_ctx(void)
{
    vdpp_com_ctx *com_ctx = mpp_calloc(vdpp_com_ctx, 1);
    vdpp_com_ops *ops = mpp_calloc(vdpp_com_ops, 1);
    VdppCtx ctx = NULL;

    if (NULL == com_ctx || NULL == ops) {
        mpp_err_f("failed to calloc com_ctx %p ops %p\n", com_ctx, ops);
        goto __ERR;
    }

    if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576) {
        ops->init = vdpp2_init;
        ops->deinit = vdpp2_deinit;
        ops->control = vdpp2_control;

        ctx = mpp_calloc(struct vdpp2_api_ctx, 1);
    } else {
        ops->init = vdpp_init;
        ops->deinit = vdpp_deinit;
        ops->control = vdpp_control;

        ctx = mpp_calloc(struct vdpp_api_ctx, 1);
    }

    if (NULL == ctx) {
        mpp_err_f("failed to calloc vdpp_api_ctx %p\n", ctx);
        goto __ERR;
    }

    com_ctx->ops = ops;
    com_ctx->priv = ctx;

    return com_ctx;

__ERR:
    MPP_FREE(com_ctx);
    MPP_FREE(ops);
    MPP_FREE(ctx);
    return NULL;
}

void rockchip_vdpp_api_release_ctx(vdpp_com_ctx *com_ctx)
{
    if (NULL == com_ctx)
        return;

    MPP_FREE(com_ctx->ops);
    MPP_FREE(com_ctx->priv);
    MPP_FREE(com_ctx);
}

MPP_RET dci_hist_info_parser(RK_U8* p_pack_hist_addr, RK_U32* p_hist_local, RK_U32* p_hist_global)
{
    RK_U32 hw_hist_idx = 0;
    RK_U32 idx;

    if (NULL == p_pack_hist_addr || NULL == p_hist_local || NULL == p_hist_global) {
        mpp_err_f("found NULL ptr, pack_hist %p hist_local %p hist_global %p\n",  p_pack_hist_addr, p_hist_local, p_hist_global);
        return MPP_ERR_NULL_PTR;
    }

    /* Hist packed (10240 byte) -> unpacked (local: 16 * 16 * 16 * U32 + global: 256 * U32) */
    for (idx = 0; idx < RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI * RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI * RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS; idx = idx + 4) {
        RK_U32 tmp0_u18, tmp1_u18, tmp2_u18, tmp3_u18;
        RK_U32 tmp0_u8, tmp1_u8, tmp2_u8, tmp3_u8, tmp4_u8, tmp5_u8, tmp6_u8, tmp7_u8, tmp8_u8;

        tmp0_u8 = *(p_pack_hist_addr + hw_hist_idx + 0);
        tmp1_u8 = *(p_pack_hist_addr + hw_hist_idx + 1);
        tmp2_u8 = *(p_pack_hist_addr + hw_hist_idx + 2);
        tmp3_u8 = *(p_pack_hist_addr + hw_hist_idx + 3);
        tmp4_u8 = *(p_pack_hist_addr + hw_hist_idx + 4);
        tmp5_u8 = *(p_pack_hist_addr + hw_hist_idx + 5);
        tmp6_u8 = *(p_pack_hist_addr + hw_hist_idx + 6);
        tmp7_u8 = *(p_pack_hist_addr + hw_hist_idx + 7);
        tmp8_u8 = *(p_pack_hist_addr + hw_hist_idx + 8);

        tmp0_u18 = ((tmp2_u8 & ((1 << 2) - 1)) << 16) + (tmp1_u8 << 8) + tmp0_u8;
        tmp1_u18 = ((tmp4_u8 & ((1 << 4) - 1)) << 14) + (tmp3_u8 << 6) + (tmp2_u8 >> 2);
        tmp2_u18 = ((tmp6_u8 & ((1 << 6) - 1)) << 12) + (tmp5_u8 << 4) + (tmp4_u8 >> 4);
        tmp3_u18 = (tmp8_u8 << 10) + (tmp7_u8 << 2) + (tmp6_u8 >> 6);

        *(p_hist_local + idx + 0) = tmp0_u18;
        *(p_hist_local + idx + 1) = tmp1_u18;
        *(p_hist_local + idx + 2) = tmp2_u18;
        *(p_hist_local + idx + 3) = tmp3_u18;
        hw_hist_idx += 9;
    }

    for (idx = 0; idx < RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_NUMS; idx++) {
        RK_U32 tmp0_u8, tmp1_u8, tmp2_u8, tmp3_u8;
        RK_U32 tmp_u32;

        tmp0_u8 = *(p_pack_hist_addr + hw_hist_idx + 0);
        tmp1_u8 = *(p_pack_hist_addr + hw_hist_idx + 1);
        tmp2_u8 = *(p_pack_hist_addr + hw_hist_idx + 2);
        tmp3_u8 = *(p_pack_hist_addr + hw_hist_idx + 3);

        tmp_u32 = (tmp3_u8 << 24) + (tmp2_u8 << 16) + (tmp1_u8 << 8) + tmp0_u8;
        *(p_hist_global + idx + 0) = tmp_u32;
        hw_hist_idx += 4;
    }

    return MPP_OK;
}
