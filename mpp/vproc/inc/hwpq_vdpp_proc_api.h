/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __HWPQ_VDPP_PROC_API_H__
#define __HWPQ_VDPP_PROC_API_H__

typedef void* rk_vdpp_context;

/* hwpq vdpp color format definition */
#define VDPP_FRAME_FMT_COLOR_MASK    (0x000f0000)
#define VDPP_FRAME_FMT_YUV           (0x00000000)
#define VDPP_FRAME_FMT_RGB           (0x00010000)

typedef enum {
    // YUV
    VDPP_FMT_YUV_MIN = 0,   /* the min YUV format value, please DO NOT use this item! */
    VDPP_FMT_NV24 = 0,      /* YUV444SP, 2 plane YCbCr, 24bpp/8 bpc, non-subsampled Cr:Cb plane */
    VDPP_FMT_NV16,          /* YUV422SP, 2 plane YCbCr, 16bpp/8 bpc, 2x1 subsampled Cr:Cb plane */
    VDPP_FMT_NV12,          /* YUV420SP, 2 plane YCbCr, 12bpp/8 bpc, 2x2 subsampled Cr:Cb plane */
    VDPP_FMT_NV15,          /* YUV420SP, 2 plane YCbCr, 15bpp/10bpc, 10bit packed data */
    VDPP_FMT_NV20,          /* YUV422SP, 2 plane YCbCr, 20bpp/10bpc, 10bit packed data, output supported only */ /* reserved */
    VDPP_FMT_NV30,          /* YUV444SP, 2 plane YCbCr, 30bpp/10bpc, 10bit packed data, output supported only */
    VDPP_FMT_P010,          /* YUV420SP, 2 plane YCbCr, 24bpp/16bpc, 10bit unpacked data with MSB aligned, output supported only */
    VDPP_FMT_P210,          /* YUV422SP, 2 plane YCbCr, 32bpp/16bpc, 10bit unpacked data with MSB aligned, output supported only */ /* reserved */
    VDPP_FMT_Q410,          /* YUV444P , 3 plane YCbCr, 48bpp/16bpc, 10bit unpacked data with MSB aligned, output supported only */
    VDPP_FMT_Y_ONLY_8BIT,   /* Only 8bit-Y Plane, For VDPP y-uv diff mode */
    VDPP_FMT_UV_ONLY_8BIT,  /* Only 8bit-UV Plane, For VDPP y-uv diff mode */
    VDPP_FMT_NV24_VU,
    VDPP_FMT_NV16_VU,
    VDPP_FMT_NV12_VU,
    VDPP_FMT_YUV_MAX,       /* the max YUV format value, please DO NOT use this item! */

    // RGB
    VDPP_FMT_RGB_MIN = 1000,/* the min RGB format value, please DO NOT use this item! */
    VDPP_FMT_RGBA = 1000,   /* RGBA8888, 32bpp */
    VDPP_FMT_RG24,          /* RGB888, 24bpp */
    VDPP_FMT_BG24,          /* BGR888, 24bpp */
    VDPP_FMT_AB30,          /* ABGR2101010, reserved */
    VDPP_FMT_RGB_MAX,       /* the max RGB format value, please DO NOT use this item! */
} vdpp_frame_format;

typedef enum {
    VDPP_RUN_MODE_UNSUPPORTED = -1,
    VDPP_RUN_MODE_VEP         = 0,
    VDPP_RUN_MODE_HIST        = 1,
} VdppRunMode;

#define VDPP_HIST_LENGTH        (10240)

typedef struct {
    int     fd;
    void*   addr;
    int     offset;

    int     w_vld;
    int     h_vld;
    int     w_vir;
    int     h_vir;
} vdpp_plane_info;

typedef struct {
    vdpp_plane_info     img_yrgb;
    vdpp_plane_info     img_cbcr;

    vdpp_frame_format   img_fmt;
} vdpp_img_info;

/* vdpp module config */
typedef struct {
    // dmsr config
    unsigned int dmsr_en;
    unsigned int str_pri_y;
    unsigned int str_sec_y;
    unsigned int dumping_y;
    unsigned int reserve_dmsr[4];

    // es config
    unsigned int es_en;
    unsigned int es_iWgtGain;
    unsigned int reserve_es[4];

    // zme config
    unsigned int zme_dering_en;
    unsigned int reserve_zme[4];

    // hist_cnt config
    unsigned int hist_cnt_en;
    unsigned int hist_csc_range;
    unsigned int reserve_hist_cnt[4];

    // sharp config
    unsigned int shp_en;
    unsigned int peaking_gain;
    unsigned int shp_shoot_ctrl_en;
    unsigned int shp_shoot_ctrl_over;
    unsigned int shp_shoot_ctrl_under;
    unsigned int reserve_shp[4];
} vdpp_params;

typedef struct {
    void*               p_hist_addr;
    unsigned int        hist_length;
    unsigned short      vdpp_img_w_in;
    unsigned short      vdpp_img_h_in;
    unsigned short      vdpp_img_w_out;
    unsigned short      vdpp_img_h_out;
    unsigned short      vdpp_blk_size_h;
    unsigned short      vdpp_blk_size_v;
} hwpq_vdpp_info_t;

typedef struct {
    unsigned int    frame_idx;
    unsigned int    yuv_diff_flag;
    unsigned int    hist_mode_en;

    vdpp_img_info   src_img_info;
    vdpp_img_info   dst_img_info;
    unsigned int    hist_buf_fd;
    void*           p_hist_buf;

    unsigned int    vdpp_config_update_flag;
    vdpp_params     vdpp_config;

    hwpq_vdpp_info_t dci_vdpp_info;

} rk_vdpp_proc_params;

#ifdef __cplusplus
extern "C"
{
#endif

int hwpq_vdpp_init(rk_vdpp_context *p_ctx_ptr);
int hwpq_vdpp_check_work_mode(rk_vdpp_context ctx, rk_vdpp_proc_params *p_proc_param);
int hwpq_vdpp_proc(rk_vdpp_context ctx, rk_vdpp_proc_params *p_proc_param);
int hwpq_vdpp_deinit(rk_vdpp_context ctx);

#ifdef __cplusplus
}
#endif

#endif // __HWPQ_VDPP_PROC_API_H__