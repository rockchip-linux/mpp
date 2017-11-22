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

#ifndef __HAL_H264E_RKV_UTILS_H__
#define __HAL_H264E_RKV_UTILS_H__

#include "hal_h264e_com.h"

// TODO: add ROI function
typedef struct h264e_hal_rkv_roi_cfg_t {
    RK_U8 qp_y          : 6;
    RK_U8 set_qp_y_en   : 1;
    RK_U8 forbid_inter  : 1;
} h264e_hal_rkv_roi_cfg;

MPP_RET h264e_rkv_set_osd_plt(H264eHalContext *ctx, void *param);
MPP_RET h264e_rkv_set_osd_data(H264eHalContext *ctx, void *param);


#endif /* __HAL_H264E_RKV_UTILS_H__ */
