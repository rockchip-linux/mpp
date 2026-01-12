/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp"

#include "mpp_soc.h"
#include "mpp_mem.h"

#include "vdpp_api.h"
#include "vdpp.h"
#include "vdpp2.h"

VdppComCtx *rockchip_vdpp_api_alloc_ctx(void)
{
    VdppComCtx *com_ctx = mpp_calloc(VdppComCtx, 1);
    VdppComOps *ops = mpp_calloc(VdppComOps, 1);
    VdppCtx ctx = NULL;
    RockchipSocType soc_type = mpp_get_soc_type();
    RK_U32 vdpp_ver = 0;

    switch (soc_type) {
    case ROCKCHIP_SOC_RK3528: vdpp_ver = 1; break;
    case ROCKCHIP_SOC_RK3576: vdpp_ver = 2; break;
    //case ROCKCHIP_SOC_RK3538:
    //case ROCKCHIP_SOC_RK3539:
    //case ROCKCHIP_SOC_RK3572: vdpp_ver = 3; break;
    default:
        vdpp_logf("unsupported soc_type %d for vdpp!!!\n", soc_type);
        goto __ERR;
    }

    if (NULL == com_ctx || NULL == ops) {
        vdpp_loge("failed to calloc com_ctx %p ops %p\n", com_ctx, ops);
        goto __ERR;
    }

    if (1 == vdpp_ver) {
        ops->init = vdpp_init;
        ops->deinit = vdpp_deinit;
        ops->control = vdpp_control;
        ops->check_cap = vdpp_check_cap;
        com_ctx->ver = 0x100;

        ctx = mpp_calloc(Vdpp1ApiCtx, 1);
    } else {
        ops->init = vdpp2_init;
        ops->deinit = vdpp2_deinit;
        ops->control = vdpp2_control;
        ops->check_cap = vdpp2_check_cap;
        com_ctx->ver = 0x200;

        ctx = mpp_calloc(Vdpp2ApiCtx, 1);
    }

    if (NULL == ctx) {
        vdpp_loge("failed to calloc vdpp_api_ctx %p\n", ctx);
        goto __ERR;
    }

    com_ctx->ops = ops;
    com_ctx->priv = ctx;
    vdpp_logi("init vdp_api_ctx: soc=%d, vdpp_ver=%d\n", soc_type, com_ctx->ver >> 8);

    return com_ctx;

__ERR:
    MPP_FREE(com_ctx);
    MPP_FREE(ops);
    MPP_FREE(ctx);
    return NULL;
}

void rockchip_vdpp_api_release_ctx(VdppComCtx *com_ctx)
{
    if (NULL == com_ctx)
        return;

    MPP_FREE(com_ctx->ops);
    MPP_FREE(com_ctx->priv);
    MPP_FREE(com_ctx);
}

MPP_RET dci_hist_info_parser(const RK_U8 *p_pack_hist_addr, RK_U32 *p_hist_local, RK_U32 *p_hist_global)
{
    const RK_U32 local_hist_length = RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI *
                                     RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI *
                                     RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS;
    RK_U32 hw_hist_idx = 0;
    RK_U32 idx = 0;

    if (NULL == p_pack_hist_addr || NULL == p_hist_local || NULL == p_hist_global) {
        vdpp_loge("found NULL ptr, pack_hist %p hist_local %p hist_global %p\n",
                  p_pack_hist_addr, p_hist_local, p_hist_global);
        return MPP_ERR_NULL_PTR;
    }

    /* Hist packed (10240 byte) -> unpacked (local: 16 * 16 * 16 * U32 + global: 256 * U32) */
    for (idx = 0; idx < local_hist_length; idx = idx + 4) {
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
