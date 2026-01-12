/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef HWPQ_VDPP_PROC_API_H
#define HWPQ_VDPP_PROC_API_H

/* version definition */
#define HWPQ_VDPP_PROC_VERSION_HEX  (0x01030000)    /* [31:0] = [major:8-minor:8-patch:16] */
#define HWPQ_VDPP_PROC_VERSION_STR  "v1.3.0"

/* hwpq vdpp color format definition */
#define VDPP_FRAME_FMT_COLOR_MASK   (0x000f0000)    /* DEPRECATED */
#define VDPP_FRAME_FMT_YUV          (0x00000000)    /* DEPRECATED */
#define VDPP_FRAME_FMT_RGB          (0x00010000)    /* DEPRECATED */

#define VDPP_HIST_LENGTH            (10240)
#define HWPQ_VDPP_SOC_NAME_LENGTH   (128)

typedef enum vdpp_frame_format {
    // YUV
    VDPP_FMT_YUV_MIN = 0,   /* the min YUV format value, please DO NOT use this item! */
    VDPP_FMT_NV24 = 0,      /* YUV444SP, 24bpp, YYYY...YYYY.../UVUVUVUV...UVUVUVUV... */
    VDPP_FMT_NV16,          /* YUV422SP, 16bpp, YYYY...YYYY.../UVUV...UVUV... */ /* RESERVED */
    VDPP_FMT_NV12,          /* YUV420SP, 12bpp, YYYY...YYYY.../UVUV... */
    VDPP_FMT_NV15,          /* RESERVED */
    VDPP_FMT_NV20,          /* RESERVED */
    VDPP_FMT_NV30,          /* RESERVED */
    VDPP_FMT_P010,          /* RESERVED */
    VDPP_FMT_P210,          /* RESERVED */
    VDPP_FMT_Q410,          /* RESERVED */
    VDPP_FMT_Y_ONLY_8BIT,   /* Only 8bit-Y Plane, For VDPP y-uv diff mode output & DCI_HIST input */
    VDPP_FMT_UV_ONLY_8BIT,  /* Only 8bit-UV Plane, For VDPP y-uv diff mode output */
    VDPP_FMT_NV42,          /* YUV444SP, 24bpp, YYYY...YYYY.../VUVUVUVU...VUVUVUVU... */
    VDPP_FMT_NV61,          /* YUV422SP, 16bpp, YYYY...YYYY.../VUVU...VUVU... */ /* RESERVED */
    VDPP_FMT_NV21,          /* YUV420SP, 12bpp, YYYY...YYYY.../VUVU... */
    VDPP_FMT_YUV_MAX,       /* the max YUV format value, please DO NOT use this item! */

    // YUV format alias
    VDPP_FMT_NV24_VU = VDPP_FMT_NV42,
    VDPP_FMT_NV16_VU = VDPP_FMT_NV61,
    VDPP_FMT_NV12_VU = VDPP_FMT_NV21,

    // RGB
    VDPP_FMT_RGB_MIN = 1000,/* the min RGB format value, please DO NOT use this item! */
    VDPP_FMT_RGBA = 1000,   /* RGBA8888, 32bpp */
    VDPP_FMT_RG24,          /* RGB888, 24bpp */
    VDPP_FMT_BG24,          /* BGR888, 24bpp */
    VDPP_FMT_AB30,          /* ABGR2101010, RESERVED */
    VDPP_FMT_RGB_MAX,       /* the max RGB format value, please DO NOT use this item! */
} HwpqVdppFormat;

typedef enum VdppRangeInfo {
    VDPP_LIMIT_RANGE,
    VDPP_FULL_RANGE,
} HwpqVdppRange;

typedef enum VdppRunMode {
    VDPP_RUN_MODE_UNSUPPORTED = -1,
    VDPP_RUN_MODE_VEP         = 0,
    VDPP_RUN_MODE_HIST        = 1,
} HwpqVdppRunMode;

typedef struct vdpp_plane_info {
    int     fd;
    void   *addr;
    int     offset;

    int     w_vld;  // image width,    unit: pixel
    int     h_vld;  // image height,   unit: pixel
    int     w_vir;  // aligned width,  unit: pixel
    int     h_vir;  // aligned height, unit: pixel
} HwpqVdppPlaneInfo;

typedef struct vdpp_img_info {
    HwpqVdppPlaneInfo   img_yrgb;
    HwpqVdppPlaneInfo   img_cbcr;
    HwpqVdppFormat      img_fmt;
} HwpqVdppImgInfo;

/* vdpp module config */
typedef struct vdpp_params {
    /* vdpp 1 features */
    // dmsr config
    unsigned int dmsr_en;               // default: 0
    unsigned int str_pri_y;             // default: 10
    unsigned int str_sec_y;             // default: 4
    unsigned int dumping_y;             // default: 6
    unsigned int reserve_dmsr[4];

    // es config (vdpp2 feature)
    unsigned int es_en;                 // default: 0
    unsigned int es_iWgtGain;           // default: 128
    unsigned int reserve_es[4];

    // zme config
    unsigned int zme_dering_en;         // default: 1
    unsigned int reserve_zme[4];

    /* vdpp 2 features */
    // hist_cnt config
    unsigned int hist_cnt_en;           // default: 1
    unsigned int hist_csc_range;        // default: VDPP_LIMIT_RANGE. see HwpqVdppRange
    unsigned int hist_vsd_mode;         // (RESERVED) range: [0, 2], default: -1, means auto calculate
    unsigned int hist_hsd_mode;         // (RESERVED) range: [0, 1], default: -1, means auto calculate
    unsigned int reserve_hist_cnt[2];

    // sharp config
    unsigned int shp_en;                // default: 0
    unsigned int peaking_gain;          // default: 196
    unsigned int shp_shoot_ctrl_en;     // default: 1
    unsigned int shp_shoot_ctrl_over;   // default: 8
    unsigned int shp_shoot_ctrl_under;  // default: 8
    unsigned int reserve_shp[4];

    /* new features add here */
} HwpqVdppConfig;

typedef struct hwpq_vdpp_info_t {
    void           *p_hist_addr;
    unsigned int    hist_length;
    unsigned short  vdpp_img_w_in;
    unsigned short  vdpp_img_h_in;
    unsigned short  vdpp_img_w_out;
    unsigned short  vdpp_img_h_out;
    unsigned short  vdpp_blk_size_h;
    unsigned short  vdpp_blk_size_v;
} HwpqVdppDciInfo;

typedef union hwpq_vdpp_output_t {
    /* mean luma result */
    struct {
        int             luma_avg;       // U10 range: [0, 1023]. <0 means invalid value.
        HwpqVdppDciInfo dci_vdpp_info;  // dci info for VOP
    } hist;

    /* bbd result, unit: pixel */
    // struct HwpqVdppBbdSize {
    //     int bbd_size_top;       // [0, H], <0 means invalid value.
    //     int bbd_size_bottom;    // [0, H], <0 means invalid value.
    //     int bbd_size_left;      // [0, W], <0 means invalid value.
    //     int bbd_size_right;     // [0, W], <0 means invalid value.
    // } bbd;
} HwpqVdppOutput;

typedef struct rk_vdpp_proc_params {
    unsigned int    frame_idx;
    unsigned int    yuv_diff_flag;  // 0:single buffer out, 1:two buffers(luma/chroma) out
    unsigned int    hist_mode_en;   // DCI_HIST_ONLY mode. 0:disable, 1:enable(see vdpp_run_mode)

    /* buffer info */
    HwpqVdppImgInfo src_img_info;   // input image info
    HwpqVdppImgInfo dst_img_info;   // output image info
    unsigned int    hist_buf_fd;    // fd of hist buffer, NOTE the buffer size should be >= VDPP_HIST_LENGTH !
    void           *p_hist_buf;     // virtual address of hist buffer, DONOT set to NULL !

    unsigned int    vdpp_config_update_flag;    // set this to 1 if needs to update 'vdpp_config'
    HwpqVdppConfig  vdpp_config;

    /* DEPRECATED, use 'hwpq_vdpp_run_cmd()' with corresponding argument instead! */
    HwpqVdppDciInfo dci_vdpp_info;  /* DEPRECATED, arg: HWPQ_VDPP_CMD_GET_HIST_RESULT */
    HwpqVdppOutput  output;         /* DEPRECATED, arg: HWPQ_VDPP_CMD_GET_BBD_RESULT  */
} HwpqVdppParams;

/* hwpq vdpp commands */
typedef enum {
    HWPQ_VDPP_CMD_INIT,                     /* RESERVED */

    /* Get useful info from a context, use 'HwpqVdppQueryInfo' as data in 'hwpq_vdpp_run_cmd()' */
    HWPQ_VDPP_CMD_GET_VERSION,              /* get vdpp version info */
    HWPQ_VDPP_CMD_GET_SOC_NAME,             /* get platform soc name */
    // HWPQ_VDPP_CMD_GET_PYR_MIN_SIZE,         /* get minimal virtual size for each layer of PYR */

    /* Get result from a context, use 'HwpqVdppOutput' as data in 'hwpq_vdpp_run_cmd()' */
    HWPQ_VDPP_CMD_GET_HIST_RESULT = 0x0100, /* get DCI info  */
    // HWPQ_VDPP_CMD_GET_BBD_RESULT,           /* get BBD result */

    /* Set module config to default values */
    HWPQ_VDPP_CMD_SET_DEF_CFG = 0x1000,     /* set 'HwpqVdppConfig' to default values */
} HwpqVdppCmd;

/* hwpq vdpp query infos */
typedef union vdpp_query_info {
    /* common query info */
    struct {
        char soc_name[HWPQ_VDPP_SOC_NAME_LENGTH];
    } platform;

    struct {
        int  vdpp_ver;      // {0x100, 0x200, 0x300}
    } version;

    /* query pyr minimal virtual size info, unit: pixel */
    // struct {
    //     int dst_width;      // [I] set the dst width  after ZME, unit: pixel
    //     int dst_height;     // [I] set the dst height after ZME, unit: pixel
    //     int nb_layers;      // [O] get the number of layers exclude the original layer, always be 3 for now!
    //     int vir_widths[8];  // [O] output virtual widths  of each layer, unit: pixel
    //     int vir_heights[8]; // [O] output virtual heights of each layer, unit: pixel
    // } pyr;
} HwpqVdppQueryInfo;


#ifdef __cplusplus
extern "C"
{
#endif

typedef void* rk_vdpp_context;  /* DEPRECATED, use HwpqVdppContext instead */
typedef rk_vdpp_context HwpqVdppContext;

int hwpq_vdpp_init(HwpqVdppContext *p_ctx_ptr);
int hwpq_vdpp_check_work_mode(HwpqVdppContext ctx, const HwpqVdppParams *p_proc_param);
int hwpq_vdpp_proc(HwpqVdppContext ctx, HwpqVdppParams *p_proc_param);
int hwpq_vdpp_deinit(HwpqVdppContext ctx);

/**
 * | cmd                            | data type         | data_size                 |
 * | ------------------------------ | ----------------- | ------------------------- |
 * | HWPQ_VDPP_CMD_GET_VERSION      | HwpqVdppQueryInfo | sizeof(HwpqVdppQueryInfo) |
 * | HWPQ_VDPP_CMD_GET_SOC_NAME     | HwpqVdppQueryInfo | sizeof(HwpqVdppQueryInfo) |
 * | HWPQ_VDPP_CMD_GET_PYR_MIN_SIZE | HwpqVdppQueryInfo | sizeof(HwpqVdppQueryInfo) |
 * | HWPQ_VDPP_CMD_GET_HIST_RESULT  | HwpqVdppOutput    | sizeof(HwpqVdppOutput)    |
 * | HWPQ_VDPP_CMD_GET_BBD_RESULT   | HwpqVdppOutput    | sizeof(HwpqVdppOutput)    |
 * | HWPQ_VDPP_CMD_SET_DEF_CFG      | HwpqVdppConfig    | sizeof(HwpqVdppConfig)    |
 */
int hwpq_vdpp_run_cmd(HwpqVdppContext ctx, HwpqVdppCmd cmd, void *data, int data_size, void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* HWPQ_VDPP_PROC_API_H */