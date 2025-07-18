/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_ENC_CFG_H__
#define __MPP_ENC_CFG_H__

#include "rk_venc_cmd.h"
#include "rk_venc_ref.h"
#include "rc_data.h"

/*
 * MppEncCfgSet shows the relationship between different configuration
 * Due to the huge amount of configurable parameters we need to setup
 * only minimum amount of necessary parameters.
 *
 * For normal user rc and prep config are enough.
 */
typedef struct MppEncCfgSet_t {
    RK_S32              size;
    MppEncBaseCfg       base;

    // esential config
    MppEncPrepCfg       prep;
    MppEncRcCfg         rc;

    // hardware related config
    MppEncHwCfg         hw;

    // codec detail config
    MppEncCodecCfg      codec;

    MppEncSliceSplit    split;
    MppEncRefCfg        ref_cfg;
    union {
        MppEncROICfg    roi;
        /* for kmpp venc roi */
        MppEncROICfgLegacy roi_legacy;
    };
    /* for kmpp venc osd */
    MppEncOSDData3      osd;
    MppEncOSDPltCfg     plt_cfg;
    MppEncOSDPlt        plt_data;
    /* for kmpp venc ref */
    MppEncRefParam      ref_param;

    // quality fine tuning config
    MppEncFineTuneCfg   tune;
} MppEncCfgSet;

#include "kmpp_obj.h"

typedef struct MppEncCfgImpl_t {
    RK_U32              is_kobj;
    union {
        MppEncCfgSet    *cfg;
        KmppObj         obj;
    };
} MppEncCfgImpl;

#endif /*__MPP_ENC_CFG_H__*/
