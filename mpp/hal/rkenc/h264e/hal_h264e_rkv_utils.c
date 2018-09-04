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

#include <string.h>

#include "mpp_device.h"
#include "hal_h264e_rkv_utils.h"

#define H264E_IOC_CUSTOM_BASE           0x1000
#define H264E_IOC_SET_OSD_PLT           (H264E_IOC_CUSTOM_BASE + 1)

MPP_RET h264e_rkv_set_osd_plt(H264eHalContext *ctx, void *param)
{
    MppEncOSDPlt *plt = (MppEncOSDPlt *)param;
    h264e_hal_enter();

    if (plt->buf) {
        MPP_RET ret = mpp_device_send_reg_with_id(ctx->dev_ctx,
                                                  H264E_IOC_SET_OSD_PLT,
                                                  param, sizeof(MppEncOSDPlt));
        ctx->osd_plt_type = H264E_OSD_PLT_TYPE_USERDEF;
        if (ret) {
            h264e_hal_err("set osd plt error");
            return MPP_NOK;
        }
    } else {
        ctx->osd_plt_type = H264E_OSD_PLT_TYPE_DEFAULT;
    }

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET h264e_rkv_set_osd_data(H264eHalContext *ctx, void *param)
{
    MppEncOSDData *src = (MppEncOSDData *)param;
    MppEncOSDData *dst = &ctx->osd_data;
    RK_U32 num = src->num_region;

    h264e_hal_enter();
    if (ctx->osd_plt_type == H264E_OSD_PLT_TYPE_NONE)
        mpp_err("warning: plt type is invalid\n");

    if (num > 8) {
        h264e_hal_err("number of region %d exceed maxinum 8");
        return MPP_NOK;
    }

    if (num) {
        dst->num_region = num;
        if (src->buf) {
            dst->buf = src->buf;
            memcpy(dst->region, src->region, num * sizeof(MppEncOSDRegion));
        }
    } else {
        memset(dst, 0, sizeof(*dst));
    }

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET h264e_rkv_set_roi_data(H264eHalContext *ctx, void *param)
{
    MppEncROICfg *src = (MppEncROICfg *)param;
    MppEncROICfg *dst = &ctx->roi_data;

    h264e_hal_enter();

    if (src->number && src->regions) {
        dst->number = src->number;
        dst->regions = src->regions;
    } else {
        memset(dst, 0, sizeof(*dst));
    }

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET rkv_config_roi_area(H264eHalContext *ctx, RK_U8 *roi_base)
{
    h264e_hal_enter();
    RK_U32 ret = MPP_OK, idx = 0, num = 0;
    RK_U32 init_pos_x, init_pos_y, roi_width, roi_height, mb_width, pos_y;
    RkvRoiCfg cfg;
    MppEncROIRegion *region;
    RK_U8 *ptr = roi_base;

    if (ctx == NULL || roi_base == NULL) {
        mpp_err("NULL pointer ctx %p roi_base %p\n", ctx, roi_base);
        return MPP_NOK;
    }

    region = ctx->roi_data.regions;
    for (num = 0; num < ctx->roi_data.number; num++) {
        init_pos_x = (region->x + 15) / 16;
        init_pos_y = (region->y + 15) / 16;
        roi_width = (region->w + 15) / 16;
        roi_height = (region->h + 15) / 16;
        mb_width = (ctx->hw_cfg.width + 15) / 16;
        pos_y = init_pos_y;

        for (idx = 0; idx < roi_width * roi_height; idx++) {
            if (idx % roi_width == 0)
                pos_y = init_pos_y + idx / roi_width;
            ptr = roi_base + (pos_y * mb_width + init_pos_x) + (idx % roi_width);

            if (region->quality) {
                cfg.qp_y = region->quality;
                cfg.set_qp_y_en = 1;
                cfg.forbid_inter = region->intra;
            } else {
                cfg.set_qp_y_en = 0;
                cfg.forbid_inter = 0;
            }

            memcpy(ptr, &cfg, sizeof(RkvRoiCfg));
        }

        region++;
    }
    h264e_hal_leave();

    return ret;
}
