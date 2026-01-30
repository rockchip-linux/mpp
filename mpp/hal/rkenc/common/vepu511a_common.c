/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "vepu511a_common"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_packet.h"
#include "mpp_frame_impl.h"

#include "kmpp_obj.h"
#include "kmpp_buffer.h"

#include "hal_enc_task.h"
#include "vepu5xx_common.h"
#include "vepu511a_common.h"

MPP_RET vepu511a_set_roi(Vepu511aRoiCfg *roi_reg_base, MppEncROICfg * roi, RK_S32 w, RK_S32 h)
{
    MppEncROIRegion *region = NULL;
    Vepu511aRoiCfg  *roi_cfg = NULL;
    Vepu511aRoiRegion *reg_regions = NULL;
    MPP_RET ret = MPP_NOK;
    RK_U32 i = 0;

    if (NULL == roi_reg_base || NULL == roi) {
        mpp_loge_f("invalid buf %p roi %p\n", roi_reg_base, roi);
        goto DONE;
    }

    roi_cfg = (Vepu511aRoiCfg *)roi_reg_base;
    reg_regions = &roi_cfg->regions[0];
    region = roi->regions;

    memset(reg_regions, 0, sizeof(Vepu511aRoiRegion) * 8);

    if (roi->number > VEPU511A_MAX_ROI_NUM) {
        mpp_loge_f("invalid region number %d\n", roi->number);
        goto DONE;
    }

    /* check region config */
    ret = MPP_OK;
    for (i = 0; i < roi->number; i++, region++) {
        if (region->x + region->w > w || region->y + region->h > h)
            ret = MPP_NOK;

        if (region->intra > 1 || region->qp_area_idx >= VEPU511A_MAX_ROI_NUM ||
            region->area_map_en > 1 || region->abs_qp_en > 1)
            ret = MPP_NOK;

        if ((region->abs_qp_en && region->quality > 51) ||
            (!region->abs_qp_en && (region->quality > 51 || region->quality < -51)))
            ret = MPP_NOK;

        if (ret) {
            mpp_loge_f("region %d invalid param:\n", i);
            mpp_loge_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
                       region->x, region->y, region->w, region->h, w, h);
            mpp_loge_f("force intra %d qp area index %d\n",
                       region->intra, region->qp_area_idx);
            mpp_loge_f("abs qp mode %d value %d\n",
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
            reg_regions->reg1063.roi0_mdc0_hevc.mdc_intra16 = 1;
            reg_regions->roi_mdc_hevc.mdc_intra32 = 1;
        }
        reg_regions++;
    }

DONE:
    return ret;
}
