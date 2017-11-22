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
        ctx->osd_plt_type = H264E_OSD_PLT_TYPE_USERDEF;
#ifdef RKPLATFORM
        if (MPP_OK != mpp_device_send_reg_with_id(ctx->vpu_fd,
                                                  H264E_IOC_SET_OSD_PLT, param,
                                                  sizeof(MppEncOSDPlt))) {
            h264e_hal_err("set osd plt error");
            return MPP_NOK;
        }
#endif
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
