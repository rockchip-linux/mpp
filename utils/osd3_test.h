/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef __OSD3_TEST_H__
#define __OSD3_TEST_H__

#include "rk_venc_cmd.h"

#define SMPTE_BAR_CNT       (8)
#define OSD_PATTERN_WIDTH   (128)
#define OSD_PATTERN_HEIGHT  (128)

typedef enum RangeTransMode_e {
    FULL_TO_LIMIT = 0,
    LIMIT_TO_FULL = 1,
} RangeTransMode;

typedef enum OsdQPAdjustMode_e {
    QP_RELATIVE = 0,
    QP_ABSOLUTE,
} OsdQPAdjustMode;

typedef enum OsdAlphaSource_e {
    FROM_DDR = 0,
    FROM_LUT,
    FROM_REG
} OsdAlphaSource;

/**
 * @brief Format translation mode select
 * 0 -- average a 2x2 block
 * 1 -- drop,
 */
typedef enum DownscaleMode_t {
    AVERAGE,
    DROP
} DownsampleMode;
typedef enum OsdTestCase_e {
    OSD_CASE_FMT_ARGB8888 = 0,
    OSD_CASE_FMT_RGBA8888,
    OSD_CASE_FMT_BGRA8888,
    OSD_CASE_FMT_ABGR8888,
    OSD_CASE_FMT_ARGB1555,
    OSD_CASE_FMT_ABGR1555,
    OSD_CASE_FMT_RGBA5551,
    OSD_CASE_FMT_BGRA5551,
    OSD_CASE_FMT_ARGB4444,
    OSD_CASE_FMT_ABGR4444,
    OSD_CASE_FMT_RGBA4444,
    OSD_CASE_FMT_BGRA4444,
    OSD_CASE_FMT_AYUV2BPP,
    OSD_CASE_FMT_AYUV1BPP,
    OSD_CASE_FG_ALPAH_SEL,
    OSD_CASE_CH_DS_MODE,
    OSD_CASE_RANGE_TRNS_SEL,
    OSD_CASE_MAX_REGINON_NUM,
    OSD_CASE_BUTT,
} OsdTestCase;

typedef enum OsdFmt_e {
    OSD_FMT_ARGB8888 = 0,
    OSD_FMT_RGBA8888,
    OSD_FMT_BGRA8888,
    OSD_FMT_ABGR8888,
    OSD_FMT_ARGB1555,
    OSD_FMT_ABGR1555,
    OSD_FMT_RGBA5551,
    OSD_FMT_BGRA5551,
    OSD_FMT_ARGB4444,
    OSD_FMT_ABGR4444,
    OSD_FMT_RGBA4444,
    OSD_FMT_BGRA4444,
    OSD_FMT_AYUV2BPP,
    OSD_FMT_AYUV1BPP,
    OSD_FMT_BUTT,
} OsdFmt;

typedef struct OsdCaseCfg_t {
    OsdTestCase type;
    OsdFmt fmt;
    char name[100];
    MPP_RET (*func)(MppEncOSDData3 *osd_data);
} OsdCaseCfg;

MPP_RET osd3_gen_smpte_bar_argb(RK_U8 **dst);
MPP_RET osd3_get_test_case(MppEncOSDData3 *osd_data, RK_U8 *base_pattern,
                           RK_U32 case_idx, KmppBuffer *osd_buffer);
#endif /* __OSD3_TEST_H__ */
