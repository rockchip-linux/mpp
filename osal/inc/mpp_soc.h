/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2020 Rockchip Electronics Co., Ltd.
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
    ROCKCHIP_SOC_RK3567,
    ROCKCHIP_SOC_RK3568,
    ROCKCHIP_SOC_RK3588,
    ROCKCHIP_SOC_RK3528,
    ROCKCHIP_SOC_RK3562,
    ROCKCHIP_SOC_RK3576,
    ROCKCHIP_SOC_RV1126B,
    ROCKCHIP_SOC_BUTT,
} RockchipSocType;

typedef struct MppDecHwCap_t {
    rk_u32          cap_coding;

    MppClientType   type                : 8;

    rk_u32          cap_fbc             : 4;
    rk_u32          cap_4k              : 1;
    rk_u32          cap_8k              : 1;
    rk_u32          cap_colmv_compress  : 1;
    rk_u32          cap_hw_h265_rps     : 1;
    rk_u32          cap_hw_vp9_prob     : 1;
    rk_u32          cap_jpg_pp_out      : 1;
    rk_u32          cap_10bit           : 1;
    rk_u32          cap_down_scale      : 1;
    rk_u32          cap_lmt_linebuf     : 1;
    rk_u32          cap_core_num        : 3;
    rk_u32          cap_hw_jpg_fix      : 1;
    rk_u32          reserved            : 8;
} MppDecHwCap;

typedef struct MppEncHwCap_t {
    rk_u32          cap_coding;

    MppClientType   type            : 8;

    rk_u32          cap_fbc         : 4;
    rk_u32          cap_4k          : 1;
    rk_u32          cap_8k          : 1;
    rk_u32          cap_hw_osd      : 1;
    rk_u32          cap_hw_roi      : 1;
    rk_u32          reserved        : 16;
} MppEncHwCap;

typedef struct {
    const char              *compatible;
    const RockchipSocType   soc_type;
    const rk_u32            vcodec_type;

    /* Max 6 decoder cap */
    const MppDecHwCap       *dec_caps[6];
    /* Max 4 encoder cap */
    const MppEncHwCap       *enc_caps[4];
} MppSocInfo;

#ifdef __cplusplus
extern "C" {
#endif

const char *mpp_get_soc_name(void);
RockchipSocType mpp_get_soc_type(void);

const MppSocInfo *mpp_get_soc_info(void);
rk_u32 mpp_check_soc_cap(MppCtxType type, MppCodingType coding);
const MppDecHwCap* mpp_get_dec_hw_info_by_client_type(MppClientType client_type);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_SOC_H__*/
