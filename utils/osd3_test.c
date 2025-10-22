/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "OSD3_TEST"

#include "osd3_test.h"
#include "mpp_common.h"
#include "kmpp_buffer.h"

/**
 * @brief Generate horizontal SMPTE test pattern in ARGB inteleaved format.
 * Test pattern bar color:
 *  {255, 255, 255},    // White
 *  {254, 255, 0},      // Yelow
 *  {0, 254, 255},      // Cyan
 *  {255, 0, 0},        // Red
 *  {0, 255, 0},        // Green
 *  {0, 0, 255},        // Blue
 *  {191, 191, 191},    // 75% White
 *  {0, 0, 0,}          // Black
 *  TODO Add RGB to YUV translation
 *
 * @param dst buffer should be at the size of (bar_width * bar_height * 8 * 4)
 * @param bar_width each bar width
 * @param bar_heigt each bar height
 */
MPP_RET osd3_gen_smpte_bar_argb(RK_U8 **dst)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, j, k = 0;
    FILE *fp = NULL;
    RK_U32 bar_width = OSD_PATTERN_WIDTH;
    RK_U32 bar_height = OSD_PATTERN_HEIGHT / SMPTE_BAR_CNT;
    RK_U8 smpte_bar[8][3] = {
        {255, 255, 255},    // White
        {254, 255, 0},      // Yelow
        {0, 254, 255},      // Cyan
        {255, 0, 0},        // Red
        {0, 255, 0},        // Green
        {0, 0, 255},        // Blue
        {191, 191, 191},    // 75% White
        {0, 0, 0,}          // Black
    };
    RK_U8 *base = malloc(bar_width * bar_height * SMPTE_BAR_CNT * 4);

    *dst = base;
    fp = fopen("/userdata/wind_ABGR8888.ABGR8888", "rb");
    if (!fp) {
        for (k = 0; k < SMPTE_BAR_CNT; k++) {
            for (j = 0; j < bar_height; j++) {
                for (i = 0; i < bar_width; i++) {
                    base[i * 4] = 0xff;
                    base[i * 4 + 1] = smpte_bar[k][0];
                    base[i * 4 + 2] = smpte_bar[k][1];
                    base[i * 4 + 3] = smpte_bar[k][2];
                }
                base += bar_width * 4;
            }
        }
    } else {
        size_t read_size = OSD_PATTERN_WIDTH * OSD_PATTERN_HEIGHT * 4;
        size_t fp_size;

        fp_size = fread(base, 1, read_size, fp);
        if (fp_size != read_size) {
            mpp_err("fread failed, read_size: %zu, fp_size: %zu", read_size, fp_size);
            free(base);
            *dst = NULL;
            ret = MPP_NOK;
        }
        fclose(fp);
    }

    return ret;
}

static MPP_RET translate_argb(RK_U8 *src, RK_U8 *dst, RK_U32 width, RK_U32 height,
                              RK_U32 fmt_idx, MppEncOSDRegion3 *cfg)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i = 0;
    RK_U32 j = 0;
    RK_U16 *tmp_u16 = NULL;

    RK_U8 tmp_r = 0;
    RK_U8 tmp_g = 0;
    RK_U8 tmp_b = 0;

    if (fmt_idx == OSD_FMT_ARGB8888) {
        cfg->fmt = MPP_FMT_ARGB8888;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 4;
        memcpy(dst, src, width * height * 4);
    } else if (fmt_idx == OSD_FMT_RGBA8888) {
        cfg->fmt = MPP_FMT_RGBA8888;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 4;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                dst[j * width * 4 + i * 4 + 0] = src[j * width * 4 + i * 4 + 0];
                dst[j * width * 4 + i * 4 + 1] = src[j * width * 4 + i * 4 + 3];
                dst[j * width * 4 + i * 4 + 2] = src[j * width * 4 + i * 4 + 2];
                dst[j * width * 4 + i * 4 + 3] = src[j * width * 4 + i * 4 + 1];
            }
        }
    } else if (fmt_idx == OSD_FMT_BGRA8888) {
        cfg->fmt = MPP_FMT_ARGB8888;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 4;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                dst[j * width * 4 + i * 4 + 0] = src[j * width * 4 + i * 4 + 3];
                dst[j * width * 4 + i * 4 + 1] = src[j * width * 4 + i * 4 + 2];
                dst[j * width * 4 + i * 4 + 2] = src[j * width * 4 + i * 4 + 1];
                dst[j * width * 4 + i * 4 + 3] = src[j * width * 4 + i * 4 + 0];
            }
        }
    } else if (fmt_idx == OSD_FMT_ABGR8888) {
        cfg->fmt = MPP_FMT_ABGR8888;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 4;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                dst[j * width * 4 + i * 4 + 0] = src[j * width * 4 + i * 4 + 1];
                dst[j * width * 4 + i * 4 + 1] = src[j * width * 4 + i * 4 + 2];
                dst[j * width * 4 + i * 4 + 2] = src[j * width * 4 + i * 4 + 3];
                dst[j * width * 4 + i * 4 + 3] = src[j * width * 4 + i * 4 + 0];
            }
        }
    } else if (fmt_idx == OSD_FMT_BGRA5551) {
        cfg->fmt = MPP_FMT_ARGB1555;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_r >> 3) << 10) +
                                         ((tmp_g >> 3) << 5) +
                                         ((tmp_b >> 3) | 0x8000);
            }
        }
    } else if (fmt_idx == OSD_FMT_RGBA5551) {
        cfg->fmt = MPP_FMT_ARGB1555;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_b >> 3) << 10) +
                                         ((tmp_g >> 3) << 5) +
                                         ((tmp_r >> 3) | 0x8000);
            }
        }
    } else if (fmt_idx == OSD_FMT_ABGR1555) {
        cfg->fmt = MPP_FMT_ARGB1555;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_r >> 3) << 11) +
                                         ((tmp_g >> 3) << 6) +
                                         ((tmp_b >> 3) << 1) + 1;
            }
        }
    } else if (fmt_idx == OSD_FMT_ARGB1555) {
        cfg->fmt = MPP_FMT_ARGB1555;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_b >> 3) << 11) +
                                         ((tmp_g >> 3) << 6) +
                                         ((tmp_r >> 3) << 1) + 1;
            }
        }
    } else if (fmt_idx == OSD_FMT_BGRA4444) {
        cfg->fmt = MPP_FMT_ARGB4444;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_r >> 4) << 8) +
                                         ((tmp_g >> 4) << 4) +
                                         ((tmp_b >> 4) | 0xf000);
            }
        }
    } else if (fmt_idx == OSD_FMT_RGBA4444) {
        cfg->fmt = MPP_FMT_ARGB4444;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_b >> 4) << 8) +
                                         ((tmp_g >> 4) << 4) +
                                         ((tmp_r >> 4) | 0xf000);
            }
        }
    } else if (fmt_idx == OSD_FMT_ABGR4444) {
        cfg->fmt = MPP_FMT_ARGB4444;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 0;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_r >> 4) << 12) +
                                         ((tmp_g >> 4) << 8) +
                                         ((tmp_b >> 4) << 4) + 0x0f;
            }
        }
    } else if (fmt_idx == OSD_FMT_ARGB4444) {
        cfg->fmt = MPP_FMT_ARGB4444;
        cfg->alpha_cfg.alpha_swap = 1;
        cfg->rbuv_swap = 1;
        cfg->stride = width * 2;
        tmp_u16 = (RK_U16 *)dst;
        for (j = 0; j < height; j++) {
            for (i = 0; i < width; i++) {
                tmp_r = src[j * width * 4 + i * 4 + 1];
                tmp_g = src[j * width * 4 + i * 4 + 2];
                tmp_b = src[j * width * 4 + i * 4 + 3];
                tmp_u16[j * width + i] = ((tmp_b >> 4) << 12) +
                                         ((tmp_g >> 4) << 8) +
                                         ((tmp_r >> 4) << 4) + 0x0f;
            }
        }
    } else if (fmt_idx == OSD_FMT_AYUV2BPP) {
        cfg->fmt = MPP_FMT_AYUV2BPP;
        cfg->rbuv_swap = 0;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->stride = width / 4;
        for (j = 0; j < height; j++) {
            RK_U32 is_black;

            is_black = (j / SMPTE_BAR_CNT) % 2 ? 1 : 0;
            for (i = 0; i < width; i++) {
                if (is_black)
                    dst[j * width / 4 + i / 4] = (dst[j * width / 4 + i / 4] << 2) | 0;
                else
                    dst[j * width / 4 + i / 4] = (dst[j * width / 4 + i / 4] << 2) | (1 << 1 | 1);
            }
        }
    } else if (fmt_idx == OSD_FMT_AYUV1BPP) {
        cfg->fmt = MPP_FMT_AYUV1BPP;
        cfg->rbuv_swap = 0;
        cfg->alpha_cfg.alpha_swap = 0;
        cfg->stride = width / 8;
        for (j = 0; j < height; j++) {
            RK_U32 is_black;

            is_black = (j / SMPTE_BAR_CNT) % 2 ? 1 : 0;
            for (i = 0; i < width; i++) {
                if (is_black)
                    dst[j * width / 8 + i / 8] = (dst[j * width / 8 + i / 8] << 1) | 0;
                else
                    dst[j * width / 8 + i / 8] = (dst[j * width / 8 + i / 8] << 1) | 1;
            }
        }
    }

    return ret;
}

static RK_U32 get_frame_size_by_format(OsdFmt fmt, RK_U32 width, RK_U32 height)
{
    switch (fmt) {
    case OSD_FMT_ARGB8888:
    case OSD_FMT_RGBA8888:
    case OSD_FMT_BGRA8888:
    case OSD_FMT_ABGR8888:
        return width * height * 4;
    case OSD_FMT_ARGB1555:
    case OSD_FMT_ABGR1555:
    case OSD_FMT_RGBA5551:
    case OSD_FMT_BGRA5551:
    case OSD_FMT_ARGB4444:
    case OSD_FMT_ABGR4444:
    case OSD_FMT_RGBA4444:
    case OSD_FMT_BGRA4444:
        return width * height * 2;
    case OSD_FMT_AYUV2BPP:
        return width * height / 4;
    case OSD_FMT_AYUV1BPP:
        return width * height / 8;
    default:
        return 0;
    }
}

static MPP_RET osd3_test_case_fmt(MppEncOSDData3 *osd_data)
{
    MppEncOSDRegion3 *region = &osd_data->region[0];

    if (region->fmt == MPP_FMT_AYUV2BPP || region->fmt == MPP_FMT_AYUV1BPP)
        region->alpha_cfg.fg_alpha_sel = FROM_LUT;
    else
        region->alpha_cfg.fg_alpha_sel = FROM_DDR;

    return MPP_OK;
}

static MPP_RET osd3_test_case_fg_alpha_sel(MppEncOSDData3 *osd_data)
{
    MppEncOSDRegion3 *region = &osd_data->region[0];
    RK_U32 region_width = OSD_PATTERN_WIDTH;

    region->alpha_cfg.fg_alpha_sel = FROM_DDR;

    region = &osd_data->region[1];
    memcpy(region, &osd_data->region[0], sizeof(MppEncOSDRegion3));
    region->lt_x = (region_width + 16);
    region->rb_x = region->lt_x + region_width - 1;
    region->alpha_cfg.fg_alpha_sel = FROM_LUT;


    region = &osd_data->region[2];
    memcpy(region, &osd_data->region[0], sizeof(MppEncOSDRegion3));
    region->lt_x = (region_width + 16) * 2;
    region->rb_x = region->lt_x + region_width - 1;
    region->alpha_cfg.fg_alpha_sel = FROM_REG;
    region->alpha_cfg.fg_alpha = 0x80;

    osd_data->num_region = 3;

    return MPP_OK;
}

static MPP_RET osd3_test_case_ch_ds_mode(MppEncOSDData3 *osd_data)
{
    MppEncOSDRegion3 *region = &osd_data->region[0];
    RK_U32 region_width = OSD_PATTERN_WIDTH;

    region->ch_ds_mode = AVERAGE;

    region = &osd_data->region[1];
    memcpy(region, &osd_data->region[0], sizeof(MppEncOSDRegion3));
    region->lt_x = (region_width + 16);
    region->rb_x = region->lt_x + region_width - 1;
    region->ch_ds_mode = DROP;

    osd_data->num_region = 2;

    return MPP_OK;
}

static MPP_RET osd3_test_case_range_trns_sel(MppEncOSDData3 *osd_data)
{
    MppEncOSDRegion3 *region = &osd_data->region[0];
    RK_U32 region_width = OSD_PATTERN_WIDTH;

    region->range_trns_sel = FULL_TO_LIMIT;
    region->range_trns_en = 1;

    region = &osd_data->region[1];
    memcpy(region, &osd_data->region[0], sizeof(MppEncOSDRegion3));
    region->lt_x = (region_width + 16);
    region->rb_x = region->lt_x + region_width - 1;
    region->range_trns_sel = LIMIT_TO_FULL;

    osd_data->num_region = 2;

    return MPP_OK;
}

static MPP_RET osd3_test_case_max_reginon_num(MppEncOSDData3 *osd_data)
{
    MppEncOSDRegion3 *region = &osd_data->region[0];
    RK_U32 region_width = OSD_PATTERN_WIDTH;
    RK_U32 i;

    for (i = 1; i < 8; i++) {
        region = &osd_data->region[i];
        memcpy(region, &osd_data->region[0], sizeof(MppEncOSDRegion3));
        region->lt_x = (region_width + 16) * i;
        region->rb_x = region->lt_x + region_width - 1;
    }

    osd_data->num_region = 8;

    return MPP_OK;
}

static OsdCaseCfg osd3_test_cases[OSD_CASE_BUTT] = {
    {
        OSD_CASE_FMT_ARGB8888,
        OSD_FMT_ARGB8888,
        "osd argb8888",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_RGBA8888,
        OSD_FMT_RGBA8888,
        "osd rgba8888",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_BGRA8888,
        OSD_FMT_BGRA8888,
        "osd bgra8888",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_ABGR8888,
        OSD_FMT_ABGR8888,
        "osd abgr8888",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_ARGB1555,
        OSD_FMT_ARGB1555,
        "osd argb1555",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_ABGR1555,
        OSD_FMT_ABGR1555,
        "osd abgr1555",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_RGBA5551,
        OSD_FMT_RGBA5551,
        "osd rgba5551",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_BGRA5551,
        OSD_FMT_BGRA5551,
        "osd bgra5551",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_ARGB4444,
        OSD_FMT_ARGB4444,
        "osd argb4444",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_ABGR4444,
        OSD_FMT_ABGR4444,
        "osd abgr4444",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_RGBA4444,
        OSD_FMT_RGBA4444,
        "osd rgba4444",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_BGRA4444,
        OSD_FMT_BGRA4444,
        "osd bgra4444",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_AYUV2BPP,
        OSD_FMT_AYUV2BPP,
        "osd ayuv2bpp",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FMT_AYUV1BPP,
        OSD_FMT_AYUV1BPP,
        "osd ayuv1bpp",
        osd3_test_case_fmt,
    },
    {
        OSD_CASE_FG_ALPAH_SEL,
        OSD_FMT_ARGB8888,
        "osd fg alpha sel",
        osd3_test_case_fg_alpha_sel,
    },
    {
        OSD_CASE_CH_DS_MODE,
        OSD_FMT_ARGB8888,
        "osd ch ds mode",
        osd3_test_case_ch_ds_mode,
    },
    {
        OSD_CASE_RANGE_TRNS_SEL,
        OSD_FMT_ARGB8888,
        "osd range trns sel",
        osd3_test_case_range_trns_sel,
    },
    {
        OSD_CASE_MAX_REGINON_NUM,
        OSD_FMT_ARGB8888,
        "osd max reginon num",
        osd3_test_case_max_reginon_num,
    }
};

MPP_RET osd3_get_test_case(MppEncOSDData3 *osd_data, RK_U8 *base_pattern, RK_U32 case_idx, KmppBuffer *osd_buffer)
{
    KmppBufCfg osd_buf_cfg = NULL;
    MPP_RET ret = MPP_OK;
    RK_U32 buffer_size = 0;
    KmppShmPtr osd_sptr;
    RK_U8 *dst_ptr =  NULL;
    MppEncOSDRegion3 *region = NULL;
    RK_U32 region_width = OSD_PATTERN_WIDTH;
    RK_U32 region_height = OSD_PATTERN_HEIGHT;

    if (!osd_data)
        return MPP_ERR_NOMEM;

    region = &osd_data->region[0];
    memset(region, 0, sizeof(MppEncOSDRegion3));

    if (*osd_buffer) {
        kmpp_buffer_put(*osd_buffer);
        *osd_buffer = NULL;
    }

    buffer_size = get_frame_size_by_format(osd3_test_cases[case_idx].fmt, region_width, region_height);

    kmpp_buffer_get(osd_buffer);
    if (!*osd_buffer)
        return MPP_ERR_NOMEM;

    osd_buf_cfg = kmpp_buffer_to_cfg(*osd_buffer);
    if (!osd_buf_cfg)
        return MPP_ERR_NOMEM;

    kmpp_buf_cfg_set_size(osd_buf_cfg, MPP_ALIGN(buffer_size, 16));
    kmpp_buffer_setup(*osd_buffer);
    kmpp_buf_cfg_get_sptr(osd_buf_cfg, &osd_sptr);
    dst_ptr = (RK_U8 *)(uintptr_t)osd_sptr.uaddr;

    ret = translate_argb(base_pattern, dst_ptr, region_width, region_height, osd3_test_cases[case_idx].fmt, region);

    region->osd_buf = *kmpp_obj_to_shm(*osd_buffer);
    region->lt_x = 0;
    region->lt_y = 0;
    region->rb_x = region->lt_x + region_width - 1;
    region->rb_y = region->lt_y + region_height - 1;
    region->enable = 1;

    osd_data->num_region = 1;

    /* black: y=0, u=0x80, v=0x80, white: y=0xff, u=0x80 v=0x80 */
    region->lut[0] = 0x80;  // v0
    region->lut[1] = 0x80;  // u0
    region->lut[2] = 0;     // y0
    region->lut[3] = 0x80;  // v1
    region->lut[4] = 0x80;  // u1
    region->lut[5] = 0xff;  // y1
    region->lut[6] = 0xff;  // a0
    region->lut[7] = 0xff;  // a1

    region->qp_cfg.qp_adj_en = 1;
    region->qp_cfg.qp_adj_sel = QP_RELATIVE;
    region->qp_cfg.qp_min = 10;
    region->qp_cfg.qp_max = 51;
    region->qp_cfg.qp = -1;

    ret = osd3_test_cases[case_idx].func(osd_data);

    mpp_log("osd case %d : %s done\n", case_idx, osd3_test_cases[case_idx].name);

    return ret;
}

