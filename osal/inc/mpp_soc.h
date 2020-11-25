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

#ifndef __MPP_SOC_H__
#define __MPP_SOC_H__

#include "mpp_dev_defs.h"

/* Do NOT use this outside MPP it may be changed in new version */
typedef enum RockchipSocType_e {
    ROCKCHIP_SOC_AUTO,
    ROCKCHIP_SOC_RK3036,
    ROCKCHIP_SOC_RK3066,
    ROCKCHIP_SOC_RK3188,
    ROCKCHIP_SOC_RK3288,
    ROCKCHIP_SOC_RK312X,
    ROCKCHIP_SOC_RK3368,
    ROCKCHIP_SOC_RK3399,
    ROCKCHIP_SOC_RK3228H,
    ROCKCHIP_SOC_RK3328,
    ROCKCHIP_SOC_RK3228,
    ROCKCHIP_SOC_RK3229,
    ROCKCHIP_SOC_RV1108,
    ROCKCHIP_SOC_RV1109,
    ROCKCHIP_SOC_RV1126,
    ROCKCHIP_SOC_RK3326,
    ROCKCHIP_SOC_RK3128H,
    ROCKCHIP_SOC_PX30,
    ROCKCHIP_SOC_RK1808,
    ROCKCHIP_SOC_RK3566,
    ROCKCHIP_SOC_RK3568,
    ROCKCHIP_SOC_BUTT,
} RockchipSocType;

typedef struct MppDecHwCap_t {
    RK_U32          cap_coding;

    MppClientType   type            : 8;

    RK_U32          cap_fbc         : 4;
    RK_U32          cap_4k          : 1;
    RK_U32          cap_8k          : 1;
    RK_U32          cap_colmv_buf   : 1;
    RK_U32          cap_hw_h265_rps : 1;
    RK_U32          cap_hw_vp9_prob : 1;
    RK_U32          cap_jpg_pp_out  : 1;
    RK_U32          cap_10bit       : 1;
    RK_U32          reserved        : 13;
} MppDecHwCap;

typedef struct MppEncHwCap_t {
    RK_U32          cap_coding;

    MppClientType   type            : 8;

    RK_U32          cap_fbc         : 4;
    RK_U32          cap_4k          : 1;
    RK_U32          cap_8k          : 1;
    RK_U32          cap_hw_osd      : 1;
    RK_U32          cap_hw_roi      : 1;
    RK_U32          reserved        : 16;
} MppEncHwCap;

typedef struct {
    const char              *compatible;
    const RockchipSocType   soc_type;
    const RK_U32            vcodec_type;

    /* Max 4 decoder cap */
    const MppDecHwCap       *dec_caps[4];
    /* Max 4 encoder cap */
    const MppEncHwCap       *enc_caps[4];
} MppSocInfo;

#ifdef __cplusplus
extern "C" {
#endif

const char *mpp_get_soc_name(void);
RockchipSocType mpp_get_soc_type(void);
RK_U32 mpp_get_vcodec_type(void);

const MppSocInfo *mpp_get_soc_info(void);
RK_U32 mpp_check_soc_cap(MppCtxType type, MppCodingType coding);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_SOC_H__*/
