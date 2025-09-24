/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp"

#include "vdpp_common.h"

const char *working_mode_name[] = {
    NULL,
    NULL,
    "VEP MODE",
    "DCI HIST MODE",
};

MPP_RET set_addr(VdppAddr *addr, VdppImg *img)
{
    if (NULL == addr || NULL == img) {
        vdpp_loge("found NULL vdpp_addr %p img %p\n", addr, img);
        return MPP_ERR_NULL_PTR;
    }

    addr->y = img->mem_addr;
    addr->cbcr = img->uv_addr;
    addr->cbcr_offset = img->uv_off;

    return MPP_OK;
}

RK_S32 get_avg_luma_from_hist(const RK_U32 *pHist, RK_S32 length)
{
    RK_S32 avg_luma = -1;

    if (pHist) {
        RK_S64 luma_sum = 0;
        RK_S32 pixel_count = 0;
        RK_S32 i;

        for (i = 0; i < length; i++) {
            /*
            bin0 ---> 0,1,2,3 ---> avg_luma = 1.5 = 0*4+1.5
            bin1 ---> 4,5,6,7 ---> avg_luma = 5.5 = 1*4+1.5
            ...
            bin255 ---> 1020,1021,1022,1023 ---> avg_luma = 1021.5 = 255*4+1.5
            */
            RK_U32 luma_val = i * 8 + 3;

            luma_sum += pHist[i] * luma_val;
            pixel_count += pHist[i];
        }

        if (pixel_count > 0) {
            avg_luma = luma_sum / pixel_count / 2;
            vdpp_logd("get luma_avg from hist: %d (in U10)\n", avg_luma);
        } else {
            vdpp_logw("pixel_count is zero! the hist data is invalid!\n");
            avg_luma = -1;
        }
    } else {
        vdpp_loge("the hist pointer is null!\n");
    }

    return avg_luma;
}