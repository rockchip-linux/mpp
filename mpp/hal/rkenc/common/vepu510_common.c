/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "vepu510_common"

#include <string.h>
#include "mpp_log.h"
#include "mpp_common.h"
#include "vepu510_common.h"

MPP_RET vepu510_set_roi(void *roi_reg_base, MppEncROICfg * roi,
                        RK_S32 w, RK_S32 h)
{
    MppEncROIRegion *region = roi->regions;
    Vepu510RoiCfg  *roi_cfg = (Vepu510RoiCfg *)roi_reg_base;
    Vepu510RoiRegion *reg_regions = &roi_cfg->regions[0];
    MPP_RET ret = MPP_NOK;
    RK_S32 i = 0;

    if (NULL == reg_regions) {
        mpp_err_f("invalid reg_regions %p\n", reg_regions);
        goto DONE;
    }
    memset(reg_regions, 0, sizeof(Vepu510RoiRegion) * 8);

    if (NULL == roi_cfg || NULL == roi) {
        mpp_err_f("invalid buf %p roi %p\n", roi_cfg, roi);
        goto DONE;
    }

    if (roi->number > VEPU510_MAX_ROI_NUM) {
        mpp_err_f("invalid region number %d\n", roi->number);
        goto DONE;
    }

    /* check region config */
    ret = MPP_OK;
    for (i = 0; i < (RK_S32) roi->number; i++, region++) {
        if (region->x + region->w > w || region->y + region->h > h)
            ret = MPP_NOK;

        if (region->intra > 1
            || region->qp_area_idx >= VEPU510_MAX_ROI_NUM
            || region->area_map_en > 1 || region->abs_qp_en > 1)
            ret = MPP_NOK;

        if ((region->abs_qp_en && region->quality > 51) ||
            (!region->abs_qp_en
             && (region->quality > 51 || region->quality < -51)))
            ret = MPP_NOK;

        if (ret) {
            mpp_err_f("region %d invalid param:\n", i);
            mpp_err_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
                      region->x, region->y, region->w, region->h, w,
                      h);
            mpp_err_f("force intra %d qp area index %d\n",
                      region->intra, region->qp_area_idx);
            mpp_err_f("abs qp mode %d value %d\n",
                      region->abs_qp_en, region->quality);
            goto DONE;
        }
        reg_regions->roi_pos_lt.roi_lt_x = MPP_ALIGN(region->x, 16) >> 4;
        reg_regions->roi_pos_lt.roi_lt_y = MPP_ALIGN(region->y, 16) >> 4;
        reg_regions->roi_pos_rb.roi_rb_x = MPP_ALIGN(region->x + region->w, 16) >> 4;
        reg_regions->roi_pos_rb.roi_rb_y = MPP_ALIGN(region->y + region->h, 16) >> 4;
        reg_regions->roi_base.roi_qp_value = region->quality;
        reg_regions->roi_base.roi_qp_adj_mode = region->abs_qp_en;
        reg_regions->roi_base.roi_en = 1;
        reg_regions->roi_base.roi_pri = 0x1f;
        if (region->intra) {
            reg_regions->roi_mdc.roi_mdc_intra16 = 1;
            reg_regions->roi_mdc.roi0_mdc_intra32_hevc = 1;
        }
        reg_regions++;
    }

DONE:
    return ret;
}

