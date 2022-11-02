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

#ifndef __IEP2_API_H__
#define __IEP2_API_H__

#include <stdint.h>
#include <stdbool.h>

#include "iep_common.h"

enum IEP2_FIELD_ORDER {
    IEP2_FIELD_ORDER_TFF,
    IEP2_FIELD_ORDER_BFF
};

enum IEP2_FMT {
    IEP2_FMT_YUV422 = 2,
    IEP2_FMT_YUV420
};

enum IEP2_YUV_SWAP {
    IEP2_YUV_SWAP_SP_UV,
    IEP2_YUV_SWAP_SP_VU,
    IEP2_YUV_SWAP_P0,
    IEP2_YUV_SWAP_P
};

enum IEP2_DIL_MODE {
    IEP2_DIL_MODE_DISABLE,
    IEP2_DIL_MODE_I5O2,
    IEP2_DIL_MODE_I5O1T,
    IEP2_DIL_MODE_I5O1B,
    IEP2_DIL_MODE_I2O2,
    IEP2_DIL_MODE_I1O1T,
    IEP2_DIL_MODE_I1O1B,
    IEP2_DIL_MODE_PD,
    IEP2_DIL_MODE_BYPASS,
    IEP2_DIL_MODE_DECT
};

enum IEP2_OUT_MODE {
    IEP2_OUT_MODE_LINE,
    IEP2_OUT_MODE_TILE
};

enum IEP2_PARAM_TYPE {
    IEP2_PARAM_TYPE_COM,
    IEP2_PARAM_TYPE_MODE,
    IEP2_PARAM_TYPE_MD,
    IEP2_PARAM_TYPE_DECT,
    IEP2_PARAM_TYPE_OSD,
    IEP2_PARAM_TYPE_ME,
    IEP2_PARAM_TYPE_EEDI,
    IEP2_PARAM_TYPE_BLE,
    IEP2_PARAM_TYPE_COMB,
    IEP2_PARAM_TYPE_ROI
};

enum PD_COMP_FLAG {
    PD_COMP_FLAG_CC,
    PD_COMP_FLAG_CN,
    PD_COMP_FLAG_NC,
    PD_COMP_FLAG_NON
};

enum PD_TYPES {
    PD_TYPES_3_2_3_2,
    PD_TYPES_2_3_2_3,
    PD_TYPES_2_3_3_2,
    PD_TYPES_3_2_2_3,
    PD_TYPES_UNKNOWN
};

union iep2_api_content {
    struct {
        enum IEP2_FMT sfmt;
        enum IEP2_YUV_SWAP sswap;
        enum IEP2_FMT dfmt;
        enum IEP2_YUV_SWAP dswap;
        int width;
        int height;
    } com;

    struct {
        enum IEP2_DIL_MODE dil_mode;
        enum IEP2_OUT_MODE out_mode;
        enum IEP2_FIELD_ORDER dil_order;
    } mode;

    struct {
        uint32_t md_theta;
        uint32_t md_r;
        uint32_t md_lambda;
    } md;

    struct {
        uint32_t roi_en;
    } roi;
};

struct iep2_api_params {
    enum IEP2_PARAM_TYPE ptype;
    union iep2_api_content param;
};

struct iep2_api_info {
    enum IEP2_FIELD_ORDER dil_order;
    bool frm_mode;
    enum PD_TYPES pd_types;
    enum PD_COMP_FLAG pd_flag;
};

struct mv_list {
    int mv[8];
    int vld[8];
    int idx;
};

#ifdef __cplusplus
extern "C" {
#endif

iep_com_ctx* rockchip_iep2_api_alloc_ctx(void);
void rockchip_iep2_api_release_ctx(iep_com_ctx *com_ctx);

#ifdef __cplusplus
}
#endif

#endif
