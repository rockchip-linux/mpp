/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp2"

#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>

#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_service.h"
#include "mpp_platform.h"

#include "vdpp2.h"

#define VDPP2_VEP_MAX_WIDTH   (1920)
#define VDPP2_VEP_MAX_HEIGHT  (2048)
#define VDPP2_HIST_MAX_WIDTH  (4096)
#define VDPP2_HIST_MAX_HEIGHT (4096)
#define VDPP2_MODE_MIN_WIDTH  (128)
#define VDPP2_MODE_MIN_HEIGH  (128)

#define VDPP2_HIST_HSD_DISABLE_LIMIT (1920)
#define VDPP2_HIST_VSD_DISABLE_LIMIT (1080)
#define VDPP2_HIST_VSD_MODE_1_LIMIT  (2160)

RK_U32 vdpp2_debug = 0;
/* default es parameters */
static RK_S32 diff2conf_lut_x_tmp[9]    = {
    0, 1024, 2048, 3072, 4096, 6144, 8192, 12288, 65535,
};
static RK_S32 diff2conf_lut_y_tmp[9]    = {
    0, 84, 141, 179, 204, 233, 246, 253, 255,
};
/* default sharp parameters */
static RK_S32 coring_zero_tmp[7]        = {
    5, 5, 8, 5, 8, 5, 5,
};
static RK_S32 coring_thr_tmp[7]         = {
    40, 40, 40, 24, 26, 30, 26,
};
static RK_S32 coring_ratio_tmp[7]       = {
    1479, 1188, 1024, 1422, 1024, 1024, 1024,
};
static RK_S32 gain_pos_tmp[7]           = {
    128, 256, 512, 256, 512, 256, 256,
};
static RK_S32 gain_neg_tmp[7]           = {
    128, 256, 512, 256, 512, 256, 256,
};
static RK_S32 limit_ctrl_pos0_tmp[7]    = {
    64, 64, 64, 64, 64, 64, 64,
};
static RK_S32 limit_ctrl_pos1_tmp[7]    = {
    120, 120, 120, 120, 120, 120, 120,
};
static RK_S32 limit_ctrl_neg0_tmp[7]    = {
    64, 64, 64, 64, 64, 64, 64,
};
static RK_S32 limit_ctrl_neg1_tmp[7]    = {
    120, 120, 120, 120, 120, 120, 120,
};
static RK_S32 limit_ctrl_ratio_tmp[7]   = {
    128, 128, 128, 128, 128, 128, 128,
};
static RK_S32 limit_ctrl_bnd_pos_tmp[7] = {
    81, 131, 63, 81, 63, 63, 63,
};
static RK_S32 limit_ctrl_bnd_neg_tmp[7] = {
    81, 131, 63, 81, 63, 63, 63,
};
static RK_S32 lum_grd_tmp[6]            = {
    0, 200, 300, 860, 960, 1023
};
static RK_S32 lum_val_tmp[6]            = {
    64, 64, 64, 64, 64, 64,
};
static RK_S32 adp_grd_tmp[6]            = {
    0, 4, 60, 180, 300, 1023,
};
static RK_S32 adp_val_tmp[6]            = {
    64, 64, 64, 64, 64, 64,
};
static RK_S32 var_grd_tmp[6]            = {
    0, 39, 102, 209, 500, 1023,
};
static RK_S32 var_val_tmp[6]            = {
    64, 64, 64, 64, 64, 64,
};
static RK_S32 diag_adj_gain_tab_tmp[8]  = {
    6, 7, 8, 9, 10, 11, 12, 13,
};
static RK_S32 roll_tab_pattern0[16]     = {
    0, 0, 0, 1, 2, 3, 4, 6, 8, 10, 11, 12, 13, 14, 15, 15,
};
static RK_S32 roll_tab_pattern1[16]     = {
    31, 31, 30, 29, 28, 27, 25, 23, 21, 19, 18, 17, 16, 16, 15, 15,
};
static RK_S32 roll_tab_pattern2[16]     = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
};
static RK_S32 tex_grd_tmp[6]            = {
    0, 128, 256, 400, 600, 1023,
};
static RK_S32 tex_val_tmp[6]            = {
    40, 60, 80, 100, 127, 127,
};

typedef enum VdppDciFmt_e {
    VDPP_DCI_FMT_RGB888,        // 0
    VDPP_DCI_FMT_ARGB8888,      // 1
    VDPP_DCI_FMT_Y_8bit_SP = 4, // 4
    VDPP_DCI_FMT_Y_10bit_SP,    // 5
    VDPP_DCI_FMT_BUTT,
} VdppDciFmt;

typedef struct VdppSrcFmtCfg_t {
    VdppDciFmt      format;
    RK_S32          alpha_swap;
    RK_S32          rbuv_swap;
} VdppSrcFmtCfg;

static VdppSrcFmtCfg vdpp_src_yuv_cfg[MPP_FMT_YUV_BUTT] = {
    {   /* MPP_FMT_YUV420SP */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV420SP_10BIT */
        .format     = VDPP_DCI_FMT_Y_10bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV422SP */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV422SP_10BIT */
        .format     = VDPP_DCI_FMT_Y_10bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV420P */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV420SP_VU   */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_YUV422P */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV422SP_VU */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_YUV422_YUYV */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV422_YVYU */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_YUV422_UYVY */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV422_VYUY */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_YUV400 */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV440SP */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV411SP */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV444SP */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_YUV444P */
        .format     = VDPP_DCI_FMT_Y_8bit_SP,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
};

static VdppSrcFmtCfg vdpp_src_rgb_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    {   /* MPP_FMT_RGB565 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_BGR565 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_RGB555 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_BGR555 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_RGB444 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_BGR444 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_RGB888 */
        .format     = VDPP_DCI_FMT_RGB888,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_BGR888 */
        .format     = VDPP_DCI_FMT_RGB888,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_RGB101010 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_BGR101010 */
        .format     = VDPP_DCI_FMT_BUTT,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_ARGB8888 */
        .format     = VDPP_DCI_FMT_ARGB8888,
        .alpha_swap = 1,
        .rbuv_swap  = 1,
    },
    {   /* MPP_FMT_ABGR8888 */
        .format     = VDPP_DCI_FMT_ARGB8888,
        .alpha_swap = 1,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_BGRA8888 */
        .format     = VDPP_DCI_FMT_ARGB8888,
        .alpha_swap = 0,
        .rbuv_swap  = 0,
    },
    {   /* MPP_FMT_RGBA8888 */
        .format     = VDPP_DCI_FMT_ARGB8888,
        .alpha_swap = 0,
        .rbuv_swap  = 1,
    },
};

static void update_dci_ctl(struct vdpp2_params* src_params)
{
    RK_S32 dci_format, dci_alpha_swap, dci_rbuv_swap;

    if (MPP_FRAME_FMT_IS_YUV(src_params->src_fmt)) {
        dci_format = vdpp_src_yuv_cfg[src_params->src_fmt - MPP_FRAME_FMT_YUV].format;
        dci_alpha_swap = vdpp_src_yuv_cfg[src_params->src_fmt - MPP_FRAME_FMT_YUV].alpha_swap;
        dci_rbuv_swap = vdpp_src_yuv_cfg[src_params->src_fmt - MPP_FRAME_FMT_YUV].rbuv_swap;
    } else if (MPP_FRAME_FMT_IS_RGB(src_params->src_fmt)) {
        dci_format = vdpp_src_rgb_cfg[src_params->src_fmt - MPP_FRAME_FMT_RGB].format;
        dci_alpha_swap = vdpp_src_rgb_cfg[src_params->src_fmt - MPP_FRAME_FMT_RGB].alpha_swap;
        dci_rbuv_swap = vdpp_src_rgb_cfg[src_params->src_fmt - MPP_FRAME_FMT_RGB].rbuv_swap;
    } else {
        mpp_err("warning: invalid input format %d", src_params->src_fmt);
        return;
    }

    src_params->dci_format = dci_format;
    src_params->dci_alpha_swap = dci_alpha_swap;
    src_params->dci_rbuv_swap = dci_rbuv_swap;

    VDPP2_DBG(VDPP2_DBG_TRACE, "input format %d  dci_format %d", src_params->src_fmt, dci_format);
}

static void set_hist_to_vdpp2_reg(struct vdpp2_params* src_params, struct vdpp2_reg* dst_reg)
{
    RK_S32 pic_vir_src_ystride;
    RK_S32 hsd_sample_num, vsd_sample_num, sw_dci_blk_hsize, sw_dci_blk_vsize;
    RK_U32 dci_hsd_mode, dci_vsd_mode;

    update_dci_ctl(src_params);

    pic_vir_src_ystride = (src_params->src_width + 3) / 4;

    if (src_params->dci_format == VDPP_DCI_FMT_RGB888) {
        pic_vir_src_ystride = src_params->src_width_vir * 3 / 4;
    } else if (src_params->dci_format == VDPP_DCI_FMT_ARGB8888) {
        pic_vir_src_ystride = src_params->src_width_vir * 4 / 4;
    } else if (src_params->dci_format == VDPP_DCI_FMT_Y_10bit_SP) { // Y 10bit SP
        pic_vir_src_ystride = src_params->src_width_vir * 10 / 8 / 4;
    } else if (src_params->dci_format == VDPP_DCI_FMT_Y_8bit_SP) { // Y 8bit SP
        pic_vir_src_ystride = src_params->src_width_vir / 4;
    } else {
        mpp_err("warning: unsupported dci format %d", src_params->dci_format);
        return;
    }

    dci_hsd_mode = (src_params->dci_hsd_mode & 1);
    dci_vsd_mode = (src_params->dci_vsd_mode & 3);

    /* check vdpp scale down mode for performance optimization */
    if (!(dci_hsd_mode & VDPP_DCI_HSD_MODE_1) &&
        (src_params->src_width_vir > VDPP2_HIST_HSD_DISABLE_LIMIT)) {
        VDPP2_DBG(VDPP2_DBG_INT, "w_vir %d hsd mode %d is optimized as mode 1\n",
                  src_params->src_width_vir, dci_hsd_mode);
        dci_hsd_mode = VDPP_DCI_HSD_MODE_1;
    }

    if (!(dci_vsd_mode & VDPP_DCI_VSD_MODE_1) &&
        (src_params->src_height_vir > VDPP2_HIST_VSD_DISABLE_LIMIT)) {
        VDPP2_DBG(VDPP2_DBG_INT, "h_vir %d vsd mode %d is optimized as mode 1\n",
                  src_params->src_height_vir, dci_vsd_mode);
        dci_vsd_mode = VDPP_DCI_VSD_MODE_1;
    }

    if (!(dci_vsd_mode & VDPP_DCI_VSD_MODE_2) &&
        (src_params->src_height_vir > VDPP2_HIST_VSD_MODE_1_LIMIT)) {
        VDPP2_DBG(VDPP2_DBG_INT, "h_vir %d vsd mode %d is optimized as mode 2\n",
                  src_params->src_height_vir, dci_vsd_mode);
        dci_vsd_mode = VDPP_DCI_VSD_MODE_2;
    }

    switch (dci_hsd_mode) {
    case VDPP_DCI_HSD_DISABLE:
        hsd_sample_num = 2;
        break;
    case VDPP_DCI_HSD_MODE_1:
        hsd_sample_num = 4;
        break;
    default:
        hsd_sample_num = 2;
        break;
    }

    switch (dci_vsd_mode) {
    case VDPP_DCI_VSD_DISABLE:
        vsd_sample_num = 1;
        break;
    case VDPP_DCI_VSD_MODE_1:
        vsd_sample_num = 2;
        break;
    case VDPP_DCI_VSD_MODE_2:
        vsd_sample_num = 4;
        break;
    default:
        vsd_sample_num = 1;
        break;
    }

    if (src_params->src_height < 1080)
        sw_dci_blk_vsize = src_params->src_height / (16 * vsd_sample_num);
    else
        sw_dci_blk_vsize = (src_params->src_height + (16 * vsd_sample_num - 1)) / (16 * vsd_sample_num);

    if (src_params->src_width < 1080)
        sw_dci_blk_hsize = src_params->src_width / (16 * hsd_sample_num);
    else
        sw_dci_blk_hsize = (src_params->src_width + (16 * hsd_sample_num - 1)) / (16 * hsd_sample_num);

    dst_reg->common.reg1.sw_dci_en = src_params->hist_cnt_en;

    dst_reg->dci.reg0.sw_dci_yrgb_addr = src_params->src.y;
    dst_reg->dci.reg1.sw_dci_yrgb_vir_stride = pic_vir_src_ystride;
    dst_reg->dci.reg1.sw_dci_yrgb_gather_num = src_params->dci_yrgb_gather_num;
    dst_reg->dci.reg1.sw_dci_yrgb_gather_en = src_params->dci_yrgb_gather_en;
    dst_reg->dci.reg2.sw_vdpp_src_pic_width = MPP_ALIGN_DOWN(src_params->src_width, hsd_sample_num) - 1;
    dst_reg->dci.reg2.sw_vdpp_src_pic_height = MPP_ALIGN_DOWN(src_params->src_height, vsd_sample_num) - 1;
    dst_reg->dci.reg3.sw_dci_data_format = src_params->dci_format;
    dst_reg->dci.reg3.sw_dci_csc_range = src_params->dci_csc_range;
    dst_reg->dci.reg3.sw_dci_vsd_mode = (src_params->working_mode == VDPP_WORK_MODE_VEP)
                                        ? 0
                                        : dci_vsd_mode;
    dst_reg->dci.reg3.sw_dci_hsd_mode = (src_params->working_mode == VDPP_WORK_MODE_VEP)
                                        ? 0
                                        : dci_hsd_mode;
    dst_reg->dci.reg3.sw_dci_alpha_swap = src_params->dci_alpha_swap;
    dst_reg->dci.reg3.sw_dci_rb_swap = src_params->dci_rbuv_swap;
    dst_reg->dci.reg3.sw_dci_blk_hsize = sw_dci_blk_hsize;
    dst_reg->dci.reg3.sw_dci_blk_vsize = sw_dci_blk_vsize;
    dst_reg->dci.reg4.sw_dci_hist_addr = src_params->hist;
}

static void update_es_tan(EsParams* p_es_param)
{
    RK_U32 angle_lo_th, angle_hi_th;

    angle_lo_th = 23 - p_es_param->es_iAngleDelta - p_es_param->es_iAngleDeltaExtra;
    angle_hi_th = 23 + p_es_param->es_iAngleDelta;

    p_es_param->es_tan_lo_th = mpp_clip(tan(angle_lo_th * acos(-1) / 180.0) * 512, 0, 511);
    p_es_param->es_tan_hi_th = mpp_clip(tan(angle_hi_th * acos(-1) / 180.0) * 512, 0, 511);
}


static void set_es_to_vdpp2_reg(struct vdpp2_params* src_params, struct vdpp2_reg* dst_reg)
{
    EsParams *p_es_param = &src_params->es_params;
    RK_U8 diff2conf_lut_k[8] = { 0 };
    RK_U8 conf_local_mean_th = mpp_clip((256 * (float)p_es_param->es_iWgtLocalTh)
                                        / p_es_param->es_iWgtGain, 0, 255);
    RK_S32 i;

    update_es_tan(p_es_param);

    dst_reg->es.reg0.es_enable = p_es_param->es_bEnabledES;
    dst_reg->es.reg1.dir_th    = p_es_param->es_iGradNoDirTh;
    dst_reg->es.reg1.flat_th   = p_es_param->es_iGradFlatTh;

    dst_reg->es.reg2.tan_lo_th = p_es_param->es_tan_lo_th;
    dst_reg->es.reg2.tan_hi_th = p_es_param->es_tan_hi_th;

    dst_reg->es.reg3.ep_chk_en = p_es_param->es_bEndpointCheckEnable;
    //dst_reg->es.sw_dir_chk.MEM_GAT_EN

    dst_reg->es.reg4.diff_gain0 = p_es_param->es_iK1;
    dst_reg->es.reg4.diff_limit = p_es_param->es_iDeltaLimit;

    dst_reg->es.reg5.diff_gain1 = p_es_param->es_iK2;
    dst_reg->es.reg5.lut_x0     = p_es_param->es_iDiff2conf_lut_x[0];

    dst_reg->es.reg6.lut_x1    = p_es_param->es_iDiff2conf_lut_x[1];
    dst_reg->es.reg6.lut_x2    = p_es_param->es_iDiff2conf_lut_x[2];
    dst_reg->es.reg7.lut_x3    = p_es_param->es_iDiff2conf_lut_x[3];
    dst_reg->es.reg7.lut_x4    = p_es_param->es_iDiff2conf_lut_x[4];
    dst_reg->es.reg8.lut_x5    = p_es_param->es_iDiff2conf_lut_x[5];
    dst_reg->es.reg8.lut_x6    = p_es_param->es_iDiff2conf_lut_x[6];
    dst_reg->es.reg9.lut_x7    = p_es_param->es_iDiff2conf_lut_x[7];
    dst_reg->es.reg9.lut_x8    = p_es_param->es_iDiff2conf_lut_x[8];

    dst_reg->es.reg10.lut_y0    = p_es_param->es_iDiff2conf_lut_y[0];
    dst_reg->es.reg10.lut_y1    = p_es_param->es_iDiff2conf_lut_y[1];
    dst_reg->es.reg10.lut_y2    = p_es_param->es_iDiff2conf_lut_y[2];
    dst_reg->es.reg10.lut_y3    = p_es_param->es_iDiff2conf_lut_y[3];
    dst_reg->es.reg11.lut_y4    = p_es_param->es_iDiff2conf_lut_y[4];
    dst_reg->es.reg11.lut_y5    = p_es_param->es_iDiff2conf_lut_y[5];
    dst_reg->es.reg11.lut_y6    = p_es_param->es_iDiff2conf_lut_y[6];
    dst_reg->es.reg11.lut_y7    = p_es_param->es_iDiff2conf_lut_y[7];
    dst_reg->es.reg12.lut_y8    = p_es_param->es_iDiff2conf_lut_y[8];

    for (i = 0; i < 8; i++) {
        double x0 = p_es_param->es_iDiff2conf_lut_x[i];
        double x1 = p_es_param->es_iDiff2conf_lut_x[i + 1];
        double y0 = p_es_param->es_iDiff2conf_lut_y[i];
        double y1 = p_es_param->es_iDiff2conf_lut_y[i + 1];
        diff2conf_lut_k[i] = mpp_clip(FLOOR(256 * (y1 - y0) / (x1 - x0)), 0, 255);
    }
    dst_reg->es.reg13.lut_k0 = diff2conf_lut_k[0];
    dst_reg->es.reg13.lut_k1 = diff2conf_lut_k[1];
    dst_reg->es.reg13.lut_k2 = diff2conf_lut_k[2];
    dst_reg->es.reg13.lut_k3 = diff2conf_lut_k[3];
    dst_reg->es.reg14.lut_k4 = diff2conf_lut_k[4];
    dst_reg->es.reg14.lut_k5 = diff2conf_lut_k[5];
    dst_reg->es.reg14.lut_k6 = diff2conf_lut_k[6];
    dst_reg->es.reg14.lut_k7 = diff2conf_lut_k[7];

    dst_reg->es.reg15.wgt_decay = p_es_param->es_iWgtDecay;
    dst_reg->es.reg15.wgt_gain  = p_es_param->es_iWgtGain;

    dst_reg->es.reg16.conf_mean_th    = conf_local_mean_th;
    dst_reg->es.reg16.conf_cnt_th     = p_es_param->es_iConfCntTh;
    dst_reg->es.reg16.low_conf_th     = p_es_param->es_iLowConfTh;
    dst_reg->es.reg16.low_conf_ratio  = p_es_param->es_iLowConfRatio;
}

static void set_shp_to_vdpp2_reg(struct vdpp2_params* src_params, struct vdpp2_reg* dst_reg)
{
    ShpParams *p_shp_param = &src_params->shp_params;
    /* Peaking Ctrl sw-process */
    RK_S32 peaking_ctrl_idx_P0[7] = { 0 };
    RK_S32 peaking_ctrl_idx_N0[7] = { 0 };
    RK_S32 peaking_ctrl_idx_P1[7] = { 0 };
    RK_S32 peaking_ctrl_idx_N1[7] = { 0 };
    RK_S32 peaking_ctrl_idx_P2[7] = { 0 };
    RK_S32 peaking_ctrl_idx_N2[7] = { 0 };
    RK_S32 peaking_ctrl_idx_P3[7] = { 0 };
    RK_S32 peaking_ctrl_idx_N3[7] = { 0 };

    RK_S32 peaking_ctrl_value_N1[7] = { 0 };
    RK_S32 peaking_ctrl_value_N2[7] = { 0 };
    RK_S32 peaking_ctrl_value_N3[7] = { 0 };
    RK_S32 peaking_ctrl_value_P1[7] = { 0 };
    RK_S32 peaking_ctrl_value_P2[7] = { 0 };
    RK_S32 peaking_ctrl_value_P3[7] = { 0 };

    RK_S32 peaking_ctrl_ratio_P01[7] = { 0 };
    RK_S32 peaking_ctrl_ratio_P12[7] = { 0 };
    RK_S32 peaking_ctrl_ratio_P23[7] = { 0 };
    RK_S32 peaking_ctrl_ratio_N01[7] = { 0 };
    RK_S32 peaking_ctrl_ratio_N12[7] = { 0 };
    RK_S32 peaking_ctrl_ratio_N23[7] = { 0 };

    RK_S32 global_gain_slp_temp[5] = { 0 };
    RK_S32 ii;
    dst_reg->sharp.reg0.sw_sharp_enable            = p_shp_param->sharp_enable;
    dst_reg->sharp.reg0.sw_lti_enable              = p_shp_param->lti_h_enable || p_shp_param->lti_v_enable;
    dst_reg->sharp.reg0.sw_cti_enable              = p_shp_param->cti_h_enable;
    dst_reg->sharp.reg0.sw_peaking_enable          = p_shp_param->peaking_enable;
    dst_reg->sharp.reg0.sw_peaking_ctrl_enable     = p_shp_param->peaking_coring_enable || p_shp_param->peaking_gain_enable || p_shp_param->peaking_limit_ctrl_enable;
    dst_reg->sharp.reg0.sw_edge_proc_enable        = p_shp_param->peaking_edge_ctrl_enable;
    dst_reg->sharp.reg0.sw_shoot_ctrl_enable       = p_shp_param->shootctrl_enable;
    dst_reg->sharp.reg0.sw_gain_ctrl_enable        = p_shp_param->global_gain_enable;
    dst_reg->sharp.reg0.sw_color_adj_enable        = p_shp_param->color_ctrl_enable;
    dst_reg->sharp.reg0.sw_texture_adj_enable      = p_shp_param->tex_adj_enable;
    dst_reg->sharp.reg0.sw_coloradj_bypass_en      = p_shp_param->sharp_coloradj_bypass_en;
    dst_reg->sharp.reg0.sw_ink_enable              = 0;
    dst_reg->sharp.reg0.sw_sharp_redundent_bypass  = 0;

    if ((dst_reg->sharp.reg0.sw_lti_enable == 1) && (p_shp_param->lti_h_enable == 0))
        p_shp_param->lti_h_gain = 0;

    if ((dst_reg->sharp.reg0.sw_lti_enable == 1) && (p_shp_param->lti_v_enable == 0))
        p_shp_param->lti_v_gain = 0;

    dst_reg->sharp.reg162.sw_ltih_radius        = p_shp_param->lti_h_radius;
    dst_reg->sharp.reg162.sw_ltih_slp1          = p_shp_param->lti_h_slope;
    dst_reg->sharp.reg162.sw_ltih_thr1          = p_shp_param->lti_h_thresold;
    dst_reg->sharp.reg163.sw_ltih_noisethrneg   = p_shp_param->lti_h_noise_thr_neg;
    dst_reg->sharp.reg163.sw_ltih_noisethrpos   = p_shp_param->lti_h_noise_thr_pos;
    dst_reg->sharp.reg163.sw_ltih_tigain        = p_shp_param->lti_h_gain;

    dst_reg->sharp.reg164.sw_ltiv_radius        = p_shp_param->lti_v_radius;
    dst_reg->sharp.reg164.sw_ltiv_slp1          = p_shp_param->lti_v_slope;
    dst_reg->sharp.reg164.sw_ltiv_thr1          = p_shp_param->lti_v_thresold;
    dst_reg->sharp.reg165.sw_ltiv_noisethrneg   = p_shp_param->lti_v_noise_thr_neg;
    dst_reg->sharp.reg165.sw_ltiv_noisethrpos   = p_shp_param->lti_v_noise_thr_pos;
    dst_reg->sharp.reg165.sw_ltiv_tigain        = p_shp_param->lti_v_gain;

    dst_reg->sharp.reg166.sw_ctih_radius        = p_shp_param->cti_h_radius;
    dst_reg->sharp.reg166.sw_ctih_slp1          = p_shp_param->cti_h_slope;
    dst_reg->sharp.reg166.sw_ctih_thr1          = p_shp_param->cti_h_thresold;
    dst_reg->sharp.reg167.sw_ctih_noisethrneg   = p_shp_param->cti_h_noise_thr_neg;
    dst_reg->sharp.reg167.sw_ctih_noisethrpos   = p_shp_param->cti_h_noise_thr_pos;
    dst_reg->sharp.reg167.sw_ctih_tigain        = p_shp_param->cti_h_gain;

    for (ii = 0; ii < 7; ii++) {
        RK_S32 coring_ratio, coring_zero, coring_thr, gain_ctrl_pos,
               gain_ctrl_neg, limit_ctrl_p0, limit_ctrl_p1, limit_ctrl_n0,
               limit_ctrl_n1, limit_ctrl_ratio;
        RK_S32 ratio_pos_tmp, ratio_neg_tmp, peaking_ctrl_add_tmp;

        if (p_shp_param->peaking_coring_enable == 1) {
            coring_ratio    = p_shp_param->peaking_coring_ratio[ii];
            coring_zero     = p_shp_param->peaking_coring_zero[ii] >> 2;
            coring_thr      = p_shp_param->peaking_coring_thr[ii] >> 2;
        } else {
            coring_ratio    = 1024;
            coring_zero     = 0;
            coring_thr      = 0;
        }
        if (p_shp_param->peaking_gain_enable == 1) {
            gain_ctrl_pos   = p_shp_param->peaking_gain_pos[ii];
            gain_ctrl_neg   = p_shp_param->peaking_gain_neg[ii];
        } else {
            gain_ctrl_pos   = 1024;
            gain_ctrl_neg   = 1024;
        }
        if (p_shp_param->peaking_limit_ctrl_enable == 1) {
            limit_ctrl_p0       = p_shp_param->peaking_limit_ctrl_pos0[ii] >> 2;
            limit_ctrl_p1       = p_shp_param->peaking_limit_ctrl_pos1[ii] >> 2;
            limit_ctrl_n0       = p_shp_param->peaking_limit_ctrl_neg0[ii] >> 2;
            limit_ctrl_n1       = p_shp_param->peaking_limit_ctrl_neg1[ii] >> 2;
            limit_ctrl_ratio    = p_shp_param->peaking_limit_ctrl_ratio[ii];
        } else {
            limit_ctrl_p0       = 255 >> 2;
            limit_ctrl_p1       = 255 >> 2;
            limit_ctrl_n0       = 255 >> 2;
            limit_ctrl_n1       = 255 >> 2;
            limit_ctrl_ratio    = 1024;
        }

        peaking_ctrl_idx_P0[ii] =  coring_zero;
        peaking_ctrl_idx_N0[ii] = -coring_zero;
        peaking_ctrl_idx_P1[ii] =  coring_thr;
        peaking_ctrl_idx_N1[ii] = -coring_thr;
        peaking_ctrl_idx_P2[ii] =  limit_ctrl_p0;
        peaking_ctrl_idx_N2[ii] = -limit_ctrl_n0;
        peaking_ctrl_idx_P3[ii] =  limit_ctrl_p1;
        peaking_ctrl_idx_N3[ii] = -limit_ctrl_n1;

        ratio_pos_tmp = (coring_ratio * gain_ctrl_pos + 512) >> 10;
        ratio_neg_tmp = (coring_ratio * gain_ctrl_neg + 512) >> 10;
        peaking_ctrl_value_P1[ii] =  ((ratio_pos_tmp * (coring_thr - coring_zero) + 512) >> 10);
        peaking_ctrl_value_N1[ii] = -((ratio_neg_tmp * (coring_thr - coring_zero) + 512) >> 10);
        peaking_ctrl_ratio_P01[ii] = ratio_pos_tmp;
        peaking_ctrl_ratio_N01[ii] = ratio_neg_tmp;

        peaking_ctrl_ratio_P12[ii] = gain_ctrl_pos;
        peaking_ctrl_ratio_N12[ii] = gain_ctrl_neg;

        peaking_ctrl_add_tmp = (gain_ctrl_pos * (limit_ctrl_p0 - coring_thr) + 512) >> 10;
        peaking_ctrl_value_P2[ii] = peaking_ctrl_value_P1[ii] + peaking_ctrl_add_tmp;
        peaking_ctrl_add_tmp = (gain_ctrl_neg * (limit_ctrl_n0 - coring_thr) + 512) >> 10;
        peaking_ctrl_value_N2[ii] = peaking_ctrl_value_N1[ii] - peaking_ctrl_add_tmp;

        ratio_pos_tmp = (limit_ctrl_ratio * gain_ctrl_pos + 512) >> 10;
        ratio_neg_tmp = (limit_ctrl_ratio * gain_ctrl_neg + 512) >> 10;

        peaking_ctrl_add_tmp = (ratio_pos_tmp * (limit_ctrl_p1 - limit_ctrl_p0) + 512) >> 10;
        peaking_ctrl_value_P3[ii] = peaking_ctrl_value_P2[ii] + peaking_ctrl_add_tmp;
        peaking_ctrl_add_tmp = (ratio_neg_tmp * (limit_ctrl_n1 - limit_ctrl_n0) + 512) >> 10;
        peaking_ctrl_value_N3[ii] = peaking_ctrl_value_N2[ii] - peaking_ctrl_add_tmp;
        peaking_ctrl_ratio_P23[ii] = ratio_pos_tmp;
        peaking_ctrl_ratio_N23[ii] = ratio_neg_tmp;

        peaking_ctrl_idx_N0[ii] = mpp_clip(peaking_ctrl_idx_N0[ii], -256, 255);
        peaking_ctrl_idx_N1[ii] = mpp_clip(peaking_ctrl_idx_N1[ii], -256, 255);
        peaking_ctrl_idx_N2[ii] = mpp_clip(peaking_ctrl_idx_N2[ii], -256, 255);
        peaking_ctrl_idx_N3[ii] = mpp_clip(peaking_ctrl_idx_N3[ii], -256, 255);
        peaking_ctrl_idx_P0[ii] = mpp_clip(peaking_ctrl_idx_P0[ii], -256, 255);
        peaking_ctrl_idx_P1[ii] = mpp_clip(peaking_ctrl_idx_P1[ii], -256, 255);
        peaking_ctrl_idx_P2[ii] = mpp_clip(peaking_ctrl_idx_P2[ii], -256, 255);
        peaking_ctrl_idx_P3[ii] = mpp_clip(peaking_ctrl_idx_P3[ii], -256, 255);

        peaking_ctrl_value_N1[ii] = mpp_clip(peaking_ctrl_value_N1[ii], -256, 255);
        peaking_ctrl_value_N2[ii] = mpp_clip(peaking_ctrl_value_N2[ii], -256, 255);
        peaking_ctrl_value_N3[ii] = mpp_clip(peaking_ctrl_value_N3[ii], -256, 255);
        peaking_ctrl_value_P1[ii] = mpp_clip(peaking_ctrl_value_P1[ii], -256, 255);
        peaking_ctrl_value_P2[ii] = mpp_clip(peaking_ctrl_value_P2[ii], -256, 255);
        peaking_ctrl_value_P3[ii] = mpp_clip(peaking_ctrl_value_P3[ii], -256, 255);

        peaking_ctrl_ratio_P01[ii] = mpp_clip(peaking_ctrl_ratio_P01[ii], 0, 4095);
        peaking_ctrl_ratio_P12[ii] = mpp_clip(peaking_ctrl_ratio_P12[ii], 0, 4095);
        peaking_ctrl_ratio_P23[ii] = mpp_clip(peaking_ctrl_ratio_P23[ii], 0, 4095);
        peaking_ctrl_ratio_N01[ii] = mpp_clip(peaking_ctrl_ratio_N01[ii], 0, 4095);
        peaking_ctrl_ratio_N12[ii] = mpp_clip(peaking_ctrl_ratio_N12[ii], 0, 4095);
        peaking_ctrl_ratio_N23[ii] = mpp_clip(peaking_ctrl_ratio_N23[ii], 0, 4095);
    }

    dst_reg->sharp.reg12.sw_peaking0_idx_n0       = peaking_ctrl_idx_N0[0];
    dst_reg->sharp.reg12.sw_peaking0_idx_n1       = peaking_ctrl_idx_N1[0];
    dst_reg->sharp.reg13.sw_peaking0_idx_n2       = peaking_ctrl_idx_N2[0];
    dst_reg->sharp.reg13.sw_peaking0_idx_n3       = peaking_ctrl_idx_N3[0];
    dst_reg->sharp.reg14.sw_peaking0_idx_p0       = peaking_ctrl_idx_P0[0];
    dst_reg->sharp.reg14.sw_peaking0_idx_p1       = peaking_ctrl_idx_P1[0];
    dst_reg->sharp.reg15.sw_peaking0_idx_p2       = peaking_ctrl_idx_P2[0];
    dst_reg->sharp.reg15.sw_peaking0_idx_p3       = peaking_ctrl_idx_P3[0];
    dst_reg->sharp.reg16.sw_peaking0_value_n1     = peaking_ctrl_value_N1[0];
    dst_reg->sharp.reg16.sw_peaking0_value_n2     = peaking_ctrl_value_N2[0];
    dst_reg->sharp.reg17.sw_peaking0_value_n3     = peaking_ctrl_value_N3[0];
    dst_reg->sharp.reg17.sw_peaking0_value_p1     = peaking_ctrl_value_P1[0];
    dst_reg->sharp.reg18.sw_peaking0_value_p2     = peaking_ctrl_value_P2[0];
    dst_reg->sharp.reg18.sw_peaking0_value_p3     = peaking_ctrl_value_P3[0];
    dst_reg->sharp.reg19.sw_peaking0_ratio_n01    = peaking_ctrl_ratio_N01[0];
    dst_reg->sharp.reg19.sw_peaking0_ratio_n12    = peaking_ctrl_ratio_N12[0];
    dst_reg->sharp.reg20.sw_peaking0_ratio_n23    = peaking_ctrl_ratio_N23[0];
    dst_reg->sharp.reg20.sw_peaking0_ratio_p01    = peaking_ctrl_ratio_P01[0];
    dst_reg->sharp.reg21.sw_peaking0_ratio_p12    = peaking_ctrl_ratio_P12[0];
    dst_reg->sharp.reg21.sw_peaking0_ratio_p23    = peaking_ctrl_ratio_P23[0];

    dst_reg->sharp.reg23.sw_peaking1_idx_n0       = peaking_ctrl_idx_N0[1];
    dst_reg->sharp.reg23.sw_peaking1_idx_n1       = peaking_ctrl_idx_N1[1];
    dst_reg->sharp.reg24.sw_peaking1_idx_n2       = peaking_ctrl_idx_N2[1];
    dst_reg->sharp.reg24.sw_peaking1_idx_n3       = peaking_ctrl_idx_N3[1];
    dst_reg->sharp.reg25.sw_peaking1_idx_p0       = peaking_ctrl_idx_P0[1];
    dst_reg->sharp.reg25.sw_peaking1_idx_p1       = peaking_ctrl_idx_P1[1];
    dst_reg->sharp.reg26.sw_peaking1_idx_p2       = peaking_ctrl_idx_P2[1];
    dst_reg->sharp.reg26.sw_peaking1_idx_p3       = peaking_ctrl_idx_P3[1];
    dst_reg->sharp.reg27.sw_peaking1_value_n1     = peaking_ctrl_value_N1[1];
    dst_reg->sharp.reg27.sw_peaking1_value_n2     = peaking_ctrl_value_N2[1];
    dst_reg->sharp.reg28.sw_peaking1_value_n3     = peaking_ctrl_value_N3[1];
    dst_reg->sharp.reg28.sw_peaking1_value_p1     = peaking_ctrl_value_P1[1];
    dst_reg->sharp.reg29.sw_peaking1_value_p2     = peaking_ctrl_value_P2[1];
    dst_reg->sharp.reg29.sw_peaking1_value_p3     = peaking_ctrl_value_P3[1];
    dst_reg->sharp.reg30.sw_peaking1_ratio_n01    = peaking_ctrl_ratio_N01[1];
    dst_reg->sharp.reg30.sw_peaking1_ratio_n12    = peaking_ctrl_ratio_N12[1];
    dst_reg->sharp.reg31.sw_peaking1_ratio_n23    = peaking_ctrl_ratio_N23[1];
    dst_reg->sharp.reg31.sw_peaking1_ratio_p01    = peaking_ctrl_ratio_P01[1];
    dst_reg->sharp.reg32.sw_peaking1_ratio_p12    = peaking_ctrl_ratio_P12[1];
    dst_reg->sharp.reg32.sw_peaking1_ratio_p23    = peaking_ctrl_ratio_P23[1];

    dst_reg->sharp.reg34.sw_peaking2_idx_n0       = peaking_ctrl_idx_N0[2];
    dst_reg->sharp.reg34.sw_peaking2_idx_n1       = peaking_ctrl_idx_N1[2];
    dst_reg->sharp.reg35.sw_peaking2_idx_n2       = peaking_ctrl_idx_N2[2];
    dst_reg->sharp.reg35.sw_peaking2_idx_n3       = peaking_ctrl_idx_N3[2];
    dst_reg->sharp.reg36.sw_peaking2_idx_p0       = peaking_ctrl_idx_P0[2];
    dst_reg->sharp.reg36.sw_peaking2_idx_p1       = peaking_ctrl_idx_P1[2];
    dst_reg->sharp.reg37.sw_peaking2_idx_p2       = peaking_ctrl_idx_P2[2];
    dst_reg->sharp.reg37.sw_peaking2_idx_p3       = peaking_ctrl_idx_P3[2];
    dst_reg->sharp.reg38.sw_peaking2_value_n1     = peaking_ctrl_value_N1[2];
    dst_reg->sharp.reg38.sw_peaking2_value_n2     = peaking_ctrl_value_N2[2];
    dst_reg->sharp.reg39.sw_peaking2_value_n3     = peaking_ctrl_value_N3[2];
    dst_reg->sharp.reg39.sw_peaking2_value_p1     = peaking_ctrl_value_P1[2];
    dst_reg->sharp.reg40.sw_peaking2_value_p2     = peaking_ctrl_value_P2[2];
    dst_reg->sharp.reg40.sw_peaking2_value_p3     = peaking_ctrl_value_P3[2];
    dst_reg->sharp.reg41.sw_peaking2_ratio_n01    = peaking_ctrl_ratio_N01[2];
    dst_reg->sharp.reg41.sw_peaking2_ratio_n12    = peaking_ctrl_ratio_N12[2];
    dst_reg->sharp.reg42.sw_peaking2_ratio_n23    = peaking_ctrl_ratio_N23[2];
    dst_reg->sharp.reg42.sw_peaking2_ratio_p01    = peaking_ctrl_ratio_P01[2];
    dst_reg->sharp.reg43.sw_peaking2_ratio_p12    = peaking_ctrl_ratio_P12[2];
    dst_reg->sharp.reg43.sw_peaking2_ratio_p23    = peaking_ctrl_ratio_P23[2];

    dst_reg->sharp.reg45.sw_peaking3_idx_n0       = peaking_ctrl_idx_N0[3];
    dst_reg->sharp.reg45.sw_peaking3_idx_n1       = peaking_ctrl_idx_N1[3];
    dst_reg->sharp.reg46.sw_peaking3_idx_n2       = peaking_ctrl_idx_N2[3];
    dst_reg->sharp.reg46.sw_peaking3_idx_n3       = peaking_ctrl_idx_N3[3];
    dst_reg->sharp.reg47.sw_peaking3_idx_p0       = peaking_ctrl_idx_P0[3];
    dst_reg->sharp.reg47.sw_peaking3_idx_p1       = peaking_ctrl_idx_P1[3];
    dst_reg->sharp.reg48.sw_peaking3_idx_p2       = peaking_ctrl_idx_P2[3];
    dst_reg->sharp.reg48.sw_peaking3_idx_p3       = peaking_ctrl_idx_P3[3];
    dst_reg->sharp.reg49.sw_peaking3_value_n1     = peaking_ctrl_value_N1[3];
    dst_reg->sharp.reg49.sw_peaking3_value_n2     = peaking_ctrl_value_N2[3];
    dst_reg->sharp.reg50.sw_peaking3_value_n3     = peaking_ctrl_value_N3[3];
    dst_reg->sharp.reg50.sw_peaking3_value_p1     = peaking_ctrl_value_P1[3];
    dst_reg->sharp.reg51.sw_peaking3_value_p2     = peaking_ctrl_value_P2[3];
    dst_reg->sharp.reg51.sw_peaking3_value_p3     = peaking_ctrl_value_P3[3];
    dst_reg->sharp.reg52.sw_peaking3_ratio_n01    = peaking_ctrl_ratio_N01[3];
    dst_reg->sharp.reg52.sw_peaking3_ratio_n12    = peaking_ctrl_ratio_N12[3];
    dst_reg->sharp.reg53.sw_peaking3_ratio_n23    = peaking_ctrl_ratio_N23[3];
    dst_reg->sharp.reg53.sw_peaking3_ratio_p01    = peaking_ctrl_ratio_P01[3];
    dst_reg->sharp.reg54.sw_peaking3_ratio_p12    = peaking_ctrl_ratio_P12[3];
    dst_reg->sharp.reg54.sw_peaking3_ratio_p23    = peaking_ctrl_ratio_P23[3];

    dst_reg->sharp.reg56.sw_peaking4_idx_n0       = peaking_ctrl_idx_N0[4];
    dst_reg->sharp.reg56.sw_peaking4_idx_n1       = peaking_ctrl_idx_N1[4];
    dst_reg->sharp.reg57.sw_peaking4_idx_n2       = peaking_ctrl_idx_N2[4];
    dst_reg->sharp.reg57.sw_peaking4_idx_n3       = peaking_ctrl_idx_N3[4];
    dst_reg->sharp.reg58.sw_peaking4_idx_p0       = peaking_ctrl_idx_P0[4];
    dst_reg->sharp.reg58.sw_peaking4_idx_p1       = peaking_ctrl_idx_P1[4];
    dst_reg->sharp.reg59.sw_peaking4_idx_p2       = peaking_ctrl_idx_P2[4];
    dst_reg->sharp.reg59.sw_peaking4_idx_p3       = peaking_ctrl_idx_P3[4];
    dst_reg->sharp.reg60.sw_peaking4_value_n1     = peaking_ctrl_value_N1[4];
    dst_reg->sharp.reg60.sw_peaking4_value_n2     = peaking_ctrl_value_N2[4];
    dst_reg->sharp.reg61.sw_peaking4_value_n3     = peaking_ctrl_value_N3[4];
    dst_reg->sharp.reg61.sw_peaking4_value_p1     = peaking_ctrl_value_P1[4];
    dst_reg->sharp.reg62.sw_peaking4_value_p2     = peaking_ctrl_value_P2[4];
    dst_reg->sharp.reg62.sw_peaking4_value_p3     = peaking_ctrl_value_P3[4];
    dst_reg->sharp.reg63.sw_peaking4_ratio_n01    = peaking_ctrl_ratio_N01[4];
    dst_reg->sharp.reg63.sw_peaking4_ratio_n12    = peaking_ctrl_ratio_N12[4];
    dst_reg->sharp.reg64.sw_peaking4_ratio_n23    = peaking_ctrl_ratio_N23[4];
    dst_reg->sharp.reg64.sw_peaking4_ratio_p01    = peaking_ctrl_ratio_P01[4];
    dst_reg->sharp.reg65.sw_peaking4_ratio_p12    = peaking_ctrl_ratio_P12[4];
    dst_reg->sharp.reg65.sw_peaking4_ratio_p23    = peaking_ctrl_ratio_P23[4];

    dst_reg->sharp.reg67.sw_peaking5_idx_n0       = peaking_ctrl_idx_N0[5];
    dst_reg->sharp.reg67.sw_peaking5_idx_n1       = peaking_ctrl_idx_N1[5];
    dst_reg->sharp.reg68.sw_peaking5_idx_n2       = peaking_ctrl_idx_N2[5];
    dst_reg->sharp.reg68.sw_peaking5_idx_n3       = peaking_ctrl_idx_N3[5];
    dst_reg->sharp.reg69.sw_peaking5_idx_p0       = peaking_ctrl_idx_P0[5];
    dst_reg->sharp.reg69.sw_peaking5_idx_p1       = peaking_ctrl_idx_P1[5];
    dst_reg->sharp.reg70.sw_peaking5_idx_p2       = peaking_ctrl_idx_P2[5];
    dst_reg->sharp.reg70.sw_peaking5_idx_p3       = peaking_ctrl_idx_P3[5];
    dst_reg->sharp.reg71.sw_peaking5_value_n1     = peaking_ctrl_value_N1[5];
    dst_reg->sharp.reg71.sw_peaking5_value_n2     = peaking_ctrl_value_N2[5];
    dst_reg->sharp.reg72.sw_peaking5_value_n3     = peaking_ctrl_value_N3[5];
    dst_reg->sharp.reg72.sw_peaking5_value_p1     = peaking_ctrl_value_P1[5];
    dst_reg->sharp.reg73.sw_peaking5_value_p2     = peaking_ctrl_value_P2[5];
    dst_reg->sharp.reg73.sw_peaking5_value_p3     = peaking_ctrl_value_P3[5];
    dst_reg->sharp.reg74.sw_peaking5_ratio_n01    = peaking_ctrl_ratio_N01[5];
    dst_reg->sharp.reg74.sw_peaking5_ratio_n12    = peaking_ctrl_ratio_N12[5];
    dst_reg->sharp.reg75.sw_peaking5_ratio_n23    = peaking_ctrl_ratio_N23[5];
    dst_reg->sharp.reg75.sw_peaking5_ratio_p01    = peaking_ctrl_ratio_P01[5];
    dst_reg->sharp.reg76.sw_peaking5_ratio_p12    = peaking_ctrl_ratio_P12[5];
    dst_reg->sharp.reg76.sw_peaking5_ratio_p23    = peaking_ctrl_ratio_P23[5];

    dst_reg->sharp.reg78.sw_peaking6_idx_n0       = peaking_ctrl_idx_N0[6];
    dst_reg->sharp.reg78.sw_peaking6_idx_n1       = peaking_ctrl_idx_N1[6];
    dst_reg->sharp.reg79.sw_peaking6_idx_n2       = peaking_ctrl_idx_N2[6];
    dst_reg->sharp.reg79.sw_peaking6_idx_n3       = peaking_ctrl_idx_N3[6];
    dst_reg->sharp.reg80.sw_peaking6_idx_p0       = peaking_ctrl_idx_P0[6];
    dst_reg->sharp.reg80.sw_peaking6_idx_p1       = peaking_ctrl_idx_P1[6];
    dst_reg->sharp.reg81.sw_peaking6_idx_p2       = peaking_ctrl_idx_P2[6];
    dst_reg->sharp.reg81.sw_peaking6_idx_p3       = peaking_ctrl_idx_P3[6];
    dst_reg->sharp.reg82.sw_peaking6_value_n1     = peaking_ctrl_value_N1[6];
    dst_reg->sharp.reg82.sw_peaking6_value_n2     = peaking_ctrl_value_N2[6];
    dst_reg->sharp.reg83.sw_peaking6_value_n3     = peaking_ctrl_value_N3[6];
    dst_reg->sharp.reg83.sw_peaking6_value_p1     = peaking_ctrl_value_P1[6];
    dst_reg->sharp.reg84.sw_peaking6_value_p2     = peaking_ctrl_value_P2[6];
    dst_reg->sharp.reg84.sw_peaking6_value_p3     = peaking_ctrl_value_P3[6];
    dst_reg->sharp.reg85.sw_peaking6_ratio_n01    = peaking_ctrl_ratio_N01[6];
    dst_reg->sharp.reg85.sw_peaking6_ratio_n12    = peaking_ctrl_ratio_N12[6];
    dst_reg->sharp.reg86.sw_peaking6_ratio_n23    = peaking_ctrl_ratio_N23[6];
    dst_reg->sharp.reg86.sw_peaking6_ratio_p01    = peaking_ctrl_ratio_P01[6];
    dst_reg->sharp.reg87.sw_peaking6_ratio_p12    = peaking_ctrl_ratio_P12[6];
    dst_reg->sharp.reg87.sw_peaking6_ratio_p23    = peaking_ctrl_ratio_P23[6];

    dst_reg->sharp.reg2.sw_peaking_v00  = p_shp_param->peaking_filt_core_V0[0];
    dst_reg->sharp.reg2.sw_peaking_v01  = p_shp_param->peaking_filt_core_V0[1];
    dst_reg->sharp.reg2.sw_peaking_v02  = p_shp_param->peaking_filt_core_V0[2];
    dst_reg->sharp.reg2.sw_peaking_v10  = p_shp_param->peaking_filt_core_V1[0];
    dst_reg->sharp.reg2.sw_peaking_v11  = p_shp_param->peaking_filt_core_V1[1];
    dst_reg->sharp.reg2.sw_peaking_v12  = p_shp_param->peaking_filt_core_V1[2];
    dst_reg->sharp.reg3.sw_peaking_v20  = p_shp_param->peaking_filt_core_V2[0];
    dst_reg->sharp.reg3.sw_peaking_v21  = p_shp_param->peaking_filt_core_V2[1];
    dst_reg->sharp.reg3.sw_peaking_v22  = p_shp_param->peaking_filt_core_V2[2];
    dst_reg->sharp.reg3.sw_peaking_usm0 = p_shp_param->peaking_filt_core_USM[0];
    dst_reg->sharp.reg3.sw_peaking_usm1 = p_shp_param->peaking_filt_core_USM[1];
    dst_reg->sharp.reg3.sw_peaking_usm2 = p_shp_param->peaking_filt_core_USM[2];
    dst_reg->sharp.reg3.sw_diag_coef    = p_shp_param->peaking_filter_cfg_diag_enh_coef;
    dst_reg->sharp.reg4.sw_peaking_h00  = p_shp_param->peaking_filt_core_H0[0];
    dst_reg->sharp.reg4.sw_peaking_h01  = p_shp_param->peaking_filt_core_H0[1];
    dst_reg->sharp.reg4.sw_peaking_h02  = p_shp_param->peaking_filt_core_H0[2];
    dst_reg->sharp.reg6.sw_peaking_h10  = p_shp_param->peaking_filt_core_H1[0];
    dst_reg->sharp.reg6.sw_peaking_h11  = p_shp_param->peaking_filt_core_H1[1];
    dst_reg->sharp.reg6.sw_peaking_h12  = p_shp_param->peaking_filt_core_H1[2];
    dst_reg->sharp.reg8.sw_peaking_h20  = p_shp_param->peaking_filt_core_H2[0];
    dst_reg->sharp.reg8.sw_peaking_h21  = p_shp_param->peaking_filt_core_H2[1];
    dst_reg->sharp.reg8.sw_peaking_h22  = p_shp_param->peaking_filt_core_H2[2];

    dst_reg->sharp.reg100.sw_peaking_gain       = p_shp_param->peaking_gain;
    dst_reg->sharp.reg100.sw_nondir_thr         = p_shp_param->peaking_edge_ctrl_non_dir_thr;
    dst_reg->sharp.reg100.sw_dir_cmp_ratio      = p_shp_param->peaking_edge_ctrl_dir_cmp_ratio;
    dst_reg->sharp.reg100.sw_nondir_wgt_ratio   = p_shp_param->peaking_edge_ctrl_non_dir_wgt_ratio;
    dst_reg->sharp.reg101.sw_nondir_wgt_offset  = p_shp_param->peaking_edge_ctrl_non_dir_wgt_offset;
    dst_reg->sharp.reg101.sw_dir_cnt_thr        = p_shp_param->peaking_edge_ctrl_dir_cnt_thr;
    dst_reg->sharp.reg101.sw_dir_cnt_avg        = p_shp_param->peaking_edge_ctrl_dir_cnt_avg;
    dst_reg->sharp.reg101.sw_dir_cnt_offset     = p_shp_param->peaking_edge_ctrl_dir_cnt_offset;
    dst_reg->sharp.reg101.sw_diag_dir_thr       = p_shp_param->peaking_edge_ctrl_diag_dir_thr;
    dst_reg->sharp.reg102.sw_diag_adjgain_tab0  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[0];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab1  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[1];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab2  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[2];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab3  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[3];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab4  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[4];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab5  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[5];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab6  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[6];
    dst_reg->sharp.reg102.sw_diag_adjgain_tab7  = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[7];

    dst_reg->sharp.reg103.sw_edge_alpha_over_non            = p_shp_param->peaking_estc_alpha_over_non;
    dst_reg->sharp.reg103.sw_edge_alpha_under_non           = p_shp_param->peaking_estc_alpha_under_non;
    dst_reg->sharp.reg103.sw_edge_alpha_over_unlimit_non    = p_shp_param->peaking_estc_alpha_over_unlimit_non;
    dst_reg->sharp.reg103.sw_edge_alpha_under_unlimit_non   = p_shp_param->peaking_estc_alpha_under_unlimit_non;

    dst_reg->sharp.reg104.sw_edge_alpha_over_v              = p_shp_param->peaking_estc_alpha_over_v;
    dst_reg->sharp.reg104.sw_edge_alpha_under_v             = p_shp_param->peaking_estc_alpha_under_v;
    dst_reg->sharp.reg104.sw_edge_alpha_over_unlimit_v      = p_shp_param->peaking_estc_alpha_over_unlimit_v;
    dst_reg->sharp.reg104.sw_edge_alpha_under_unlimit_v     = p_shp_param->peaking_estc_alpha_under_unlimit_v;

    dst_reg->sharp.reg105.sw_edge_alpha_over_h              = p_shp_param->peaking_estc_alpha_over_h;
    dst_reg->sharp.reg105.sw_edge_alpha_under_h             = p_shp_param->peaking_estc_alpha_under_h;
    dst_reg->sharp.reg105.sw_edge_alpha_over_unlimit_h      = p_shp_param->peaking_estc_alpha_over_unlimit_h;
    dst_reg->sharp.reg105.sw_edge_alpha_under_unlimit_h     = p_shp_param->peaking_estc_alpha_under_unlimit_h;

    dst_reg->sharp.reg106.sw_edge_alpha_over_d0             = p_shp_param->peaking_estc_alpha_over_d0;
    dst_reg->sharp.reg106.sw_edge_alpha_under_d0            = p_shp_param->peaking_estc_alpha_under_d0;
    dst_reg->sharp.reg106.sw_edge_alpha_over_unlimit_d0     = p_shp_param->peaking_estc_alpha_over_unlimit_d0;
    dst_reg->sharp.reg106.sw_edge_alpha_under_unlimit_d0    = p_shp_param->peaking_estc_alpha_under_unlimit_d0;

    dst_reg->sharp.reg107.sw_edge_alpha_over_d1             = p_shp_param->peaking_estc_alpha_over_d1;
    dst_reg->sharp.reg107.sw_edge_alpha_under_d1            = p_shp_param->peaking_estc_alpha_under_d1;
    dst_reg->sharp.reg107.sw_edge_alpha_over_unlimit_d1     = p_shp_param->peaking_estc_alpha_over_unlimit_d1;
    dst_reg->sharp.reg107.sw_edge_alpha_under_unlimit_d1    = p_shp_param->peaking_estc_alpha_under_unlimit_d1;

    dst_reg->sharp.reg108.sw_edge_delta_offset_non          = p_shp_param->peaking_estc_delta_offset_non;
    dst_reg->sharp.reg108.sw_edge_delta_offset_v            = p_shp_param->peaking_estc_delta_offset_v;
    dst_reg->sharp.reg108.sw_edge_delta_offset_h            = p_shp_param->peaking_estc_delta_offset_h;
    dst_reg->sharp.reg109.sw_edge_delta_offset_d0           = p_shp_param->peaking_estc_delta_offset_d0;
    dst_reg->sharp.reg109.sw_edge_delta_offset_d1           = p_shp_param->peaking_estc_delta_offset_d1;

    dst_reg->sharp.reg110.sw_shoot_filt_radius            = p_shp_param->shootctrl_filter_radius;
    dst_reg->sharp.reg110.sw_shoot_delta_offset           = p_shp_param->shootctrl_delta_offset;
    dst_reg->sharp.reg110.sw_shoot_alpha_over             = p_shp_param->shootctrl_alpha_over;
    dst_reg->sharp.reg110.sw_shoot_alpha_under            = p_shp_param->shootctrl_alpha_under;
    dst_reg->sharp.reg111.sw_shoot_alpha_over_unlimit     = p_shp_param->shootctrl_alpha_over_unlimit;
    dst_reg->sharp.reg111.sw_shoot_alpha_under_unlimit    = p_shp_param->shootctrl_alpha_under_unlimit;


    //dst_reg->sharp.reg112.sw_adp_idx0      = p_shp_param->global_gain_adp_grd
    dst_reg->sharp.reg112.sw_adp_idx0       = p_shp_param->global_gain_adp_grd[1] >> 2;
    dst_reg->sharp.reg112.sw_adp_idx1       = p_shp_param->global_gain_adp_grd[2] >> 2;
    dst_reg->sharp.reg112.sw_adp_idx2       = p_shp_param->global_gain_adp_grd[3] >> 2;
    dst_reg->sharp.reg113.sw_adp_idx3       = p_shp_param->global_gain_adp_grd[4] >> 2;
    dst_reg->sharp.reg113.sw_adp_gain0  = p_shp_param->global_gain_adp_val[0];
    dst_reg->sharp.reg113.sw_adp_gain1  = p_shp_param->global_gain_adp_val[1];
    dst_reg->sharp.reg114.sw_adp_gain2  = p_shp_param->global_gain_adp_val[2];
    dst_reg->sharp.reg114.sw_adp_gain3  = p_shp_param->global_gain_adp_val[3];
    dst_reg->sharp.reg114.sw_adp_gain4  = p_shp_param->global_gain_adp_val[4];
    global_gain_slp_temp[0]  = ROUND(128 * (float)(dst_reg->sharp.reg113.sw_adp_gain1 - dst_reg->sharp.reg113.sw_adp_gain0)
                                     / MPP_MAX((float)(dst_reg->sharp.reg112.sw_adp_idx0 - 0), 1));
    global_gain_slp_temp[1]  = ROUND(128 * (float)(dst_reg->sharp.reg114.sw_adp_gain2 - dst_reg->sharp.reg113.sw_adp_gain1)
                                     / MPP_MAX((float)(dst_reg->sharp.reg112.sw_adp_idx1 - dst_reg->sharp.reg112.sw_adp_idx0), 1));
    global_gain_slp_temp[2]  = ROUND(128 * (float)(dst_reg->sharp.reg114.sw_adp_gain3 - dst_reg->sharp.reg114.sw_adp_gain2)
                                     / MPP_MAX((float)(dst_reg->sharp.reg112.sw_adp_idx2 - dst_reg->sharp.reg112.sw_adp_idx1), 1));
    global_gain_slp_temp[3]  = ROUND(128 * (float)(dst_reg->sharp.reg114.sw_adp_gain4 - dst_reg->sharp.reg114.sw_adp_gain3)
                                     / MPP_MAX((float)(dst_reg->sharp.reg113.sw_adp_idx3 - dst_reg->sharp.reg112.sw_adp_idx2), 1));
    global_gain_slp_temp[4]  = ROUND(128 * (float)(p_shp_param->global_gain_adp_val[5] - dst_reg->sharp.reg114.sw_adp_gain4)
                                     / MPP_MAX((float)(255 - dst_reg->sharp.reg113.sw_adp_idx3), 1));
    dst_reg->sharp.reg115.sw_adp_slp01     = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    dst_reg->sharp.reg115.sw_adp_slp12     = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    dst_reg->sharp.reg128.sw_adp_slp23     = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    dst_reg->sharp.reg128.sw_adp_slp34     = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    dst_reg->sharp.reg129.sw_adp_slp45     = mpp_clip(global_gain_slp_temp[4], -1024, 1023);

    dst_reg->sharp.reg129.sw_var_idx0       = p_shp_param->global_gain_var_grd[1] >> 2;
    dst_reg->sharp.reg129.sw_var_idx1       = p_shp_param->global_gain_var_grd[2] >> 2;
    dst_reg->sharp.reg130.sw_var_idx2       = p_shp_param->global_gain_var_grd[3] >> 2;
    dst_reg->sharp.reg130.sw_var_idx3       = p_shp_param->global_gain_var_grd[4] >> 2;
    dst_reg->sharp.reg130.sw_var_gain0  = p_shp_param->global_gain_var_val[0];
    dst_reg->sharp.reg131.sw_var_gain1  = p_shp_param->global_gain_var_val[1];
    dst_reg->sharp.reg131.sw_var_gain2  = p_shp_param->global_gain_var_val[2];
    dst_reg->sharp.reg131.sw_var_gain3  = p_shp_param->global_gain_var_val[3];
    dst_reg->sharp.reg131.sw_var_gain4  = p_shp_param->global_gain_var_val[4];
    global_gain_slp_temp[0]  = ROUND(128 * (float)(dst_reg->sharp.reg131.sw_var_gain1 - dst_reg->sharp.reg130.sw_var_gain0)
                                     / MPP_MAX((float)(dst_reg->sharp.reg129.sw_var_idx0 - 0), 1));
    global_gain_slp_temp[1]  = ROUND(128 * (float)(dst_reg->sharp.reg131.sw_var_gain2 - dst_reg->sharp.reg131.sw_var_gain1)
                                     / MPP_MAX((float)(dst_reg->sharp.reg129.sw_var_idx1 - dst_reg->sharp.reg129.sw_var_idx0), 1));
    global_gain_slp_temp[2]  = ROUND(128 * (float)(dst_reg->sharp.reg131.sw_var_gain3 - dst_reg->sharp.reg131.sw_var_gain2)
                                     / MPP_MAX((float)(dst_reg->sharp.reg130.sw_var_idx2 - dst_reg->sharp.reg129.sw_var_idx1), 1));
    global_gain_slp_temp[3]  = ROUND(128 * (float)(dst_reg->sharp.reg131.sw_var_gain4 - dst_reg->sharp.reg131.sw_var_gain3)
                                     / MPP_MAX((float)(dst_reg->sharp.reg130.sw_var_idx3 - dst_reg->sharp.reg130.sw_var_idx2), 1));
    global_gain_slp_temp[4]  = ROUND(128 * (float)(p_shp_param->global_gain_var_val[5] - dst_reg->sharp.reg131.sw_var_gain4)
                                     / MPP_MAX((float)(255 - dst_reg->sharp.reg130.sw_var_idx3), 1));
    dst_reg->sharp.reg132.sw_var_slp01     = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    dst_reg->sharp.reg132.sw_var_slp12     = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    dst_reg->sharp.reg133.sw_var_slp23     = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    dst_reg->sharp.reg133.sw_var_slp34     = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    dst_reg->sharp.reg134.sw_var_slp45     = mpp_clip(global_gain_slp_temp[4], -1024, 1023);

    dst_reg->sharp.reg134.sw_lum_select     = p_shp_param->global_gain_lum_mode;
    dst_reg->sharp.reg134.sw_lum_idx0     = p_shp_param->global_gain_lum_grd[1] >> 2;
    dst_reg->sharp.reg135.sw_lum_idx1     = p_shp_param->global_gain_lum_grd[2] >> 2;
    dst_reg->sharp.reg135.sw_lum_idx2     = p_shp_param->global_gain_lum_grd[3] >> 2;
    dst_reg->sharp.reg135.sw_lum_idx3     = p_shp_param->global_gain_lum_grd[4] >> 2;
    dst_reg->sharp.reg136.sw_lum_gain0  = p_shp_param->global_gain_lum_val[0];
    dst_reg->sharp.reg136.sw_lum_gain1  = p_shp_param->global_gain_lum_val[1];
    dst_reg->sharp.reg136.sw_lum_gain2  = p_shp_param->global_gain_lum_val[2];
    dst_reg->sharp.reg136.sw_lum_gain3  = p_shp_param->global_gain_lum_val[3];
    dst_reg->sharp.reg137.sw_lum_gain4  = p_shp_param->global_gain_lum_val[4];
    global_gain_slp_temp[0]  = ROUND(128 * (float)(dst_reg->sharp.reg136.sw_lum_gain1 - dst_reg->sharp.reg136.sw_lum_gain0)
                                     / MPP_MAX((float)(dst_reg->sharp.reg134.sw_lum_idx0 - 0), 1));
    global_gain_slp_temp[1]  = ROUND(128 * (float)(dst_reg->sharp.reg136.sw_lum_gain2 - dst_reg->sharp.reg136.sw_lum_gain1)
                                     / MPP_MAX((float)(dst_reg->sharp.reg135.sw_lum_idx1 - dst_reg->sharp.reg134.sw_lum_idx0), 1));
    global_gain_slp_temp[2]  = ROUND(128 * (float)(dst_reg->sharp.reg136.sw_lum_gain3 - dst_reg->sharp.reg136.sw_lum_gain2)
                                     / MPP_MAX((float)(dst_reg->sharp.reg135.sw_lum_idx2 - dst_reg->sharp.reg135.sw_lum_idx1), 1));
    global_gain_slp_temp[3]  = ROUND(128 * (float)(dst_reg->sharp.reg137.sw_lum_gain4 - dst_reg->sharp.reg136.sw_lum_gain3)
                                     / MPP_MAX((float)(dst_reg->sharp.reg135.sw_lum_idx3 - dst_reg->sharp.reg135.sw_lum_idx2), 1));
    global_gain_slp_temp[4]  = ROUND(128 * (float)(p_shp_param->global_gain_lum_val[5] - dst_reg->sharp.reg137.sw_lum_gain4)
                                     / MPP_MAX((float)(255 - dst_reg->sharp.reg135.sw_lum_idx3), 1));
    dst_reg->sharp.reg137.sw_lum_slp01     = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    dst_reg->sharp.reg137.sw_lum_slp12     = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    dst_reg->sharp.reg138.sw_lum_slp23     = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    dst_reg->sharp.reg138.sw_lum_slp34     = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    dst_reg->sharp.reg139.sw_lum_slp45     = mpp_clip(global_gain_slp_temp[4], -1024, 1023);


    dst_reg->sharp.reg140.sw_adj_point_x0      = p_shp_param->color_ctrl_p0_point_u;
    dst_reg->sharp.reg140.sw_adj_point_y0      = p_shp_param->color_ctrl_p0_point_v;
    dst_reg->sharp.reg140.sw_adj_scaling_coef0 = p_shp_param->color_ctrl_p0_scaling_coef;
    dst_reg->sharp.reg141.sw_coloradj_tab0_0   = p_shp_param->color_ctrl_p0_roll_tab[0];
    dst_reg->sharp.reg141.sw_coloradj_tab0_1   = p_shp_param->color_ctrl_p0_roll_tab[1];
    dst_reg->sharp.reg141.sw_coloradj_tab0_2   = p_shp_param->color_ctrl_p0_roll_tab[2];
    dst_reg->sharp.reg141.sw_coloradj_tab0_3   = p_shp_param->color_ctrl_p0_roll_tab[3];
    dst_reg->sharp.reg141.sw_coloradj_tab0_4   = p_shp_param->color_ctrl_p0_roll_tab[4];
    dst_reg->sharp.reg141.sw_coloradj_tab0_5   = p_shp_param->color_ctrl_p0_roll_tab[5];
    dst_reg->sharp.reg142.sw_coloradj_tab0_6   = p_shp_param->color_ctrl_p0_roll_tab[6];
    dst_reg->sharp.reg142.sw_coloradj_tab0_7   = p_shp_param->color_ctrl_p0_roll_tab[7];
    dst_reg->sharp.reg142.sw_coloradj_tab0_8   = p_shp_param->color_ctrl_p0_roll_tab[8];
    dst_reg->sharp.reg142.sw_coloradj_tab0_9   = p_shp_param->color_ctrl_p0_roll_tab[9];
    dst_reg->sharp.reg142.sw_coloradj_tab0_10  = p_shp_param->color_ctrl_p0_roll_tab[10];
    dst_reg->sharp.reg142.sw_coloradj_tab0_11  = p_shp_param->color_ctrl_p0_roll_tab[11];
    dst_reg->sharp.reg143.sw_coloradj_tab0_12  = p_shp_param->color_ctrl_p0_roll_tab[12];
    dst_reg->sharp.reg143.sw_coloradj_tab0_13  = p_shp_param->color_ctrl_p0_roll_tab[13];
    dst_reg->sharp.reg143.sw_coloradj_tab0_14  = p_shp_param->color_ctrl_p0_roll_tab[14];
    dst_reg->sharp.reg143.sw_coloradj_tab0_15  = p_shp_param->color_ctrl_p0_roll_tab[15];

    dst_reg->sharp.reg144.sw_adj_point_x1      = p_shp_param->color_ctrl_p1_point_u;
    dst_reg->sharp.reg144.sw_adj_point_y1      = p_shp_param->color_ctrl_p1_point_v;
    dst_reg->sharp.reg144.sw_adj_scaling_coef1 = p_shp_param->color_ctrl_p1_scaling_coef;
    dst_reg->sharp.reg145.sw_coloradj_tab1_0   = p_shp_param->color_ctrl_p1_roll_tab[0];
    dst_reg->sharp.reg145.sw_coloradj_tab1_1   = p_shp_param->color_ctrl_p1_roll_tab[1];
    dst_reg->sharp.reg145.sw_coloradj_tab1_2   = p_shp_param->color_ctrl_p1_roll_tab[2];
    dst_reg->sharp.reg145.sw_coloradj_tab1_3   = p_shp_param->color_ctrl_p1_roll_tab[3];
    dst_reg->sharp.reg145.sw_coloradj_tab1_4   = p_shp_param->color_ctrl_p1_roll_tab[4];
    dst_reg->sharp.reg145.sw_coloradj_tab1_5   = p_shp_param->color_ctrl_p1_roll_tab[5];
    dst_reg->sharp.reg146.sw_coloradj_tab1_6   = p_shp_param->color_ctrl_p1_roll_tab[6];
    dst_reg->sharp.reg146.sw_coloradj_tab1_7   = p_shp_param->color_ctrl_p1_roll_tab[7];
    dst_reg->sharp.reg146.sw_coloradj_tab1_8   = p_shp_param->color_ctrl_p1_roll_tab[8];
    dst_reg->sharp.reg146.sw_coloradj_tab1_9   = p_shp_param->color_ctrl_p1_roll_tab[9];
    dst_reg->sharp.reg146.sw_coloradj_tab1_10  = p_shp_param->color_ctrl_p1_roll_tab[10];
    dst_reg->sharp.reg146.sw_coloradj_tab1_11  = p_shp_param->color_ctrl_p1_roll_tab[11];
    dst_reg->sharp.reg147.sw_coloradj_tab1_12  = p_shp_param->color_ctrl_p1_roll_tab[12];
    dst_reg->sharp.reg147.sw_coloradj_tab1_13  = p_shp_param->color_ctrl_p1_roll_tab[13];
    dst_reg->sharp.reg147.sw_coloradj_tab1_14  = p_shp_param->color_ctrl_p1_roll_tab[14];
    dst_reg->sharp.reg147.sw_coloradj_tab1_15  = p_shp_param->color_ctrl_p1_roll_tab[15];

    dst_reg->sharp.reg148.sw_adj_point_x2       = p_shp_param->color_ctrl_p2_point_u;
    dst_reg->sharp.reg148.sw_adj_point_y2       = p_shp_param->color_ctrl_p2_point_v;
    dst_reg->sharp.reg148.sw_adj_scaling_coef2  = p_shp_param->color_ctrl_p2_scaling_coef;
    dst_reg->sharp.reg149.sw_coloradj_tab2_0    = p_shp_param->color_ctrl_p2_roll_tab[0];
    dst_reg->sharp.reg149.sw_coloradj_tab2_1    = p_shp_param->color_ctrl_p2_roll_tab[1];
    dst_reg->sharp.reg149.sw_coloradj_tab2_2    = p_shp_param->color_ctrl_p2_roll_tab[2];
    dst_reg->sharp.reg149.sw_coloradj_tab2_3    = p_shp_param->color_ctrl_p2_roll_tab[3];
    dst_reg->sharp.reg149.sw_coloradj_tab2_4    = p_shp_param->color_ctrl_p2_roll_tab[4];
    dst_reg->sharp.reg149.sw_coloradj_tab2_5    = p_shp_param->color_ctrl_p2_roll_tab[5];
    dst_reg->sharp.reg150.sw_coloradj_tab2_6   = p_shp_param->color_ctrl_p2_roll_tab[6];
    dst_reg->sharp.reg150.sw_coloradj_tab2_7   = p_shp_param->color_ctrl_p2_roll_tab[7];
    dst_reg->sharp.reg150.sw_coloradj_tab2_8   = p_shp_param->color_ctrl_p2_roll_tab[8];
    dst_reg->sharp.reg150.sw_coloradj_tab2_9   = p_shp_param->color_ctrl_p2_roll_tab[9];
    dst_reg->sharp.reg150.sw_coloradj_tab2_10  = p_shp_param->color_ctrl_p2_roll_tab[10];
    dst_reg->sharp.reg150.sw_coloradj_tab2_11  = p_shp_param->color_ctrl_p2_roll_tab[11];
    dst_reg->sharp.reg151.sw_coloradj_tab2_12  = p_shp_param->color_ctrl_p2_roll_tab[12];
    dst_reg->sharp.reg151.sw_coloradj_tab2_13  = p_shp_param->color_ctrl_p2_roll_tab[13];
    dst_reg->sharp.reg151.sw_coloradj_tab2_14  = p_shp_param->color_ctrl_p2_roll_tab[14];
    dst_reg->sharp.reg151.sw_coloradj_tab2_15  = p_shp_param->color_ctrl_p2_roll_tab[15];

    dst_reg->sharp.reg152.sw_adj_point_x3      = p_shp_param->color_ctrl_p3_point_u;
    dst_reg->sharp.reg152.sw_adj_point_y3      = p_shp_param->color_ctrl_p3_point_v;
    dst_reg->sharp.reg152.sw_adj_scaling_coef3 = p_shp_param->color_ctrl_p3_scaling_coef;
    dst_reg->sharp.reg153.sw_coloradj_tab3_0   = p_shp_param->color_ctrl_p3_roll_tab[0];
    dst_reg->sharp.reg153.sw_coloradj_tab3_1   = p_shp_param->color_ctrl_p3_roll_tab[1];
    dst_reg->sharp.reg153.sw_coloradj_tab3_2   = p_shp_param->color_ctrl_p3_roll_tab[2];
    dst_reg->sharp.reg153.sw_coloradj_tab3_3   = p_shp_param->color_ctrl_p3_roll_tab[3];
    dst_reg->sharp.reg153.sw_coloradj_tab3_4   = p_shp_param->color_ctrl_p3_roll_tab[4];
    dst_reg->sharp.reg153.sw_coloradj_tab3_5   = p_shp_param->color_ctrl_p3_roll_tab[5];
    dst_reg->sharp.reg154.sw_coloradj_tab3_6   = p_shp_param->color_ctrl_p3_roll_tab[6];
    dst_reg->sharp.reg154.sw_coloradj_tab3_7   = p_shp_param->color_ctrl_p3_roll_tab[7];
    dst_reg->sharp.reg154.sw_coloradj_tab3_8   = p_shp_param->color_ctrl_p3_roll_tab[8];
    dst_reg->sharp.reg154.sw_coloradj_tab3_9   = p_shp_param->color_ctrl_p3_roll_tab[9];
    dst_reg->sharp.reg154.sw_coloradj_tab3_10  = p_shp_param->color_ctrl_p3_roll_tab[10];
    dst_reg->sharp.reg154.sw_coloradj_tab3_11  = p_shp_param->color_ctrl_p3_roll_tab[11];
    dst_reg->sharp.reg155.sw_coloradj_tab3_12  = p_shp_param->color_ctrl_p3_roll_tab[12];
    dst_reg->sharp.reg155.sw_coloradj_tab3_13  = p_shp_param->color_ctrl_p3_roll_tab[13];
    dst_reg->sharp.reg155.sw_coloradj_tab3_14  = p_shp_param->color_ctrl_p3_roll_tab[14];
    dst_reg->sharp.reg155.sw_coloradj_tab3_15  = p_shp_param->color_ctrl_p3_roll_tab[15];


    dst_reg->sharp.reg156.sw_idxmode_select = p_shp_param->tex_adj_mode_select;
    dst_reg->sharp.reg156.sw_ymode_select   = p_shp_param->tex_adj_y_mode_select;
    dst_reg->sharp.reg156.sw_tex_idx0        = p_shp_param->tex_adj_grd[1] >> 2;
    dst_reg->sharp.reg156.sw_tex_idx1       = p_shp_param->tex_adj_grd[2] >> 2;
    dst_reg->sharp.reg157.sw_tex_idx2       = p_shp_param->tex_adj_grd[3] >> 2;
    dst_reg->sharp.reg157.sw_tex_idx3       = p_shp_param->tex_adj_grd[4] >> 2;
    dst_reg->sharp.reg157.sw_tex_gain0      = p_shp_param->tex_adj_val[0];
    dst_reg->sharp.reg158.sw_tex_gain1      = p_shp_param->tex_adj_val[1];
    dst_reg->sharp.reg158.sw_tex_gain2      = p_shp_param->tex_adj_val[2];
    dst_reg->sharp.reg158.sw_tex_gain3      = p_shp_param->tex_adj_val[3];
    dst_reg->sharp.reg158.sw_tex_gain4      = p_shp_param->tex_adj_val[4];
    global_gain_slp_temp[0]      = ROUND(128 * (float)(dst_reg->sharp.reg158.sw_tex_gain1 - dst_reg->sharp.reg157.sw_tex_gain0)
                                         / MPP_MAX((float)(dst_reg->sharp.reg156.sw_tex_idx0 - 0), 1));
    global_gain_slp_temp[1]      = ROUND(128 * (float)(dst_reg->sharp.reg158.sw_tex_gain2 - dst_reg->sharp.reg158.sw_tex_gain1)
                                         / MPP_MAX((float)(dst_reg->sharp.reg156.sw_tex_idx1 - dst_reg->sharp.reg156.sw_tex_idx0), 1));
    global_gain_slp_temp[2]      = ROUND(128 * (float)(dst_reg->sharp.reg158.sw_tex_gain3 - dst_reg->sharp.reg158.sw_tex_gain2)
                                         / MPP_MAX((float)(dst_reg->sharp.reg157.sw_tex_idx2 - dst_reg->sharp.reg156.sw_tex_idx1), 1));
    global_gain_slp_temp[3]      = ROUND(128 * (float)(dst_reg->sharp.reg158.sw_tex_gain4 - dst_reg->sharp.reg158.sw_tex_gain3)
                                         / MPP_MAX((float)(dst_reg->sharp.reg157.sw_tex_idx3 - dst_reg->sharp.reg157.sw_tex_idx2), 1));
    global_gain_slp_temp[4]      = ROUND(128 * (float)(p_shp_param->tex_adj_val[5] - dst_reg->sharp.reg158.sw_tex_gain4)
                                         / MPP_MAX((float)(255 - dst_reg->sharp.reg157.sw_tex_idx3), 1));
    dst_reg->sharp.reg159.sw_tex_slp01      = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    dst_reg->sharp.reg159.sw_tex_slp12      = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    dst_reg->sharp.reg160.sw_tex_slp23      = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    dst_reg->sharp.reg160.sw_tex_slp34      = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    dst_reg->sharp.reg161.sw_tex_slp45      = mpp_clip(global_gain_slp_temp[4], -1024, 1023);

}

static MPP_RET vdpp2_params_to_reg(struct vdpp2_params* src_params, struct vdpp2_api_ctx *ctx)
{
    struct vdpp2_reg *dst_reg = &ctx->reg;
    struct zme_params *zme_params = &src_params->zme_params;

    memset(dst_reg, 0, sizeof(*dst_reg));

    dst_reg->common.reg0.sw_vdpp_frm_en = 1;

    /* 0x0004(reg1), TODO: add debug function */
    dst_reg->common.reg1.sw_vdpp_src_fmt = VDPP_FMT_YUV420;
    dst_reg->common.reg1.sw_vdpp_src_yuv_swap = src_params->src_yuv_swap;

    if (MPP_FMT_YUV420SP_VU == src_params->src_fmt)
        dst_reg->common.reg1.sw_vdpp_src_yuv_swap = 1;

    dst_reg->common.reg1.sw_vdpp_dst_fmt = src_params->dst_fmt;
    dst_reg->common.reg1.sw_vdpp_dst_yuv_swap = src_params->dst_yuv_swap;
    dst_reg->common.reg1.sw_vdpp_dbmsr_en = (src_params->working_mode == VDPP_WORK_MODE_DCI)
                                            ? 0
                                            : src_params->dmsr_params.dmsr_enable;

    /* 0x0008(reg2) */
    dst_reg->common.reg2.sw_vdpp_working_mode = src_params->working_mode;
    VDPP2_DBG(VDPP2_DBG_TRACE, "working_mode %d", src_params->working_mode);

    /* 0x000C ~ 0x001C(reg3 ~ reg7), skip */
    dst_reg->common.reg4.sw_vdpp_clk_on = 1;
    dst_reg->common.reg4.sw_md_clk_on = 1;
    dst_reg->common.reg4.sw_dect_clk_on = 1;
    dst_reg->common.reg4.sw_me_clk_on = 1;
    dst_reg->common.reg4.sw_mc_clk_on = 1;
    dst_reg->common.reg4.sw_eedi_clk_on = 1;
    dst_reg->common.reg4.sw_ble_clk_on = 1;
    dst_reg->common.reg4.sw_out_clk_on = 1;
    dst_reg->common.reg4.sw_ctrl_clk_on = 1;
    dst_reg->common.reg4.sw_ram_clk_on = 1;
    dst_reg->common.reg4.sw_dma_clk_on = 1;
    dst_reg->common.reg4.sw_reg_clk_on = 1;

    /* 0x0020(reg8) */
    dst_reg->common.reg8.sw_vdpp_frm_done_en = 1;
    dst_reg->common.reg8.sw_vdpp_osd_max_en = 1;
    dst_reg->common.reg8.sw_vdpp_bus_error_en = 1;
    dst_reg->common.reg8.sw_vdpp_timeout_int_en = 1;
    dst_reg->common.reg8.sw_vdpp_config_error_en = 1;
    /* 0x0024 ~ 0x002C(reg9 ~ reg11), skip */
    {
        RK_U32 src_right_redundant = src_params->src_width % 16 == 0 ? 0 : 16 - src_params->src_width % 16;
        RK_U32 src_down_redundant  = src_params->src_height % 8 == 0 ? 0 : 8 - src_params->src_height % 8;
        RK_U32 dst_right_redundant = src_params->dst_width % 16 == 0 ? 0 : 16 - src_params->dst_width % 16;
        /* 0x0030(reg12) */
        dst_reg->common.reg12.sw_vdpp_src_vir_y_stride = src_params->src_width_vir / 4;

        /* 0x0034(reg13) */
        dst_reg->common.reg13.sw_vdpp_dst_vir_y_stride = src_params->dst_width_vir / 4;

        /* 0x0038(reg14) */
        dst_reg->common.reg14.sw_vdpp_src_pic_width = src_params->src_width + src_right_redundant - 1;
        dst_reg->common.reg14.sw_vdpp_src_right_redundant = src_right_redundant;
        dst_reg->common.reg14.sw_vdpp_src_pic_height = src_params->src_height + src_down_redundant - 1;
        dst_reg->common.reg14.sw_vdpp_src_down_redundant = src_down_redundant;

        /* 0x003C(reg15) */
        dst_reg->common.reg15.sw_vdpp_dst_pic_width = src_params->dst_width + dst_right_redundant - 1;
        dst_reg->common.reg15.sw_vdpp_dst_right_redundant = dst_right_redundant;
        dst_reg->common.reg15.sw_vdpp_dst_pic_height = src_params->dst_height - 1;
    }
    /* 0x0040 ~ 0x005C(reg16 ~ reg23), skip */
    dst_reg->common.reg20.sw_vdpp_timeout_en = 1;
    dst_reg->common.reg20.sw_vdpp_timeout_cnt = 0x8FFFFFF;

    /* 0x0060(reg24) */
    dst_reg->common.reg24.sw_vdpp_src_addr_y = src_params->src.y;

    /* 0x0064(reg25) */
    dst_reg->common.reg25.sw_vdpp_src_addr_uv = src_params->src.cbcr;

    /* 0x0068(reg26) */
    dst_reg->common.reg26.sw_vdpp_dst_addr_y = src_params->dst.y;

    /* 0x006C(reg27) */
    dst_reg->common.reg27.sw_vdpp_dst_addr_uv = src_params->dst.cbcr;

    if (src_params->yuv_out_diff) {
        RK_U32 dst_right_redundant_c = src_params->dst_c_width % 16 == 0 ? 0 : 16 - src_params->dst_c_width % 16;

        dst_reg->common.reg1.sw_vdpp_yuvout_diff_en = src_params->yuv_out_diff;
        dst_reg->common.reg13.sw_vdpp_dst_vir_c_stride = src_params->dst_c_width_vir / 4;
        /* 0x0040(reg16) */
        dst_reg->common.reg16.sw_vdpp_dst_pic_width_c = src_params->dst_c_width + dst_right_redundant_c - 1;
        dst_reg->common.reg16.sw_vdpp_dst_right_redundant_c = dst_right_redundant_c;
        dst_reg->common.reg16.sw_vdpp_dst_pic_height_c = src_params->dst_c_height - 1;

        dst_reg->common.reg27.sw_vdpp_dst_addr_uv = src_params->dst_c.cbcr;
    }

    set_dmsr_to_vdpp_reg(&src_params->dmsr_params, &ctx->dmsr);
    set_hist_to_vdpp2_reg(src_params, dst_reg);
    set_es_to_vdpp2_reg(src_params, dst_reg);
    set_shp_to_vdpp2_reg(src_params, dst_reg);

    zme_params->src_width = src_params->src_width;
    zme_params->src_height = src_params->src_height;
    zme_params->dst_width = src_params->dst_width;
    zme_params->dst_height = src_params->dst_height;
    zme_params->dst_fmt = src_params->dst_fmt;
    zme_params->yuv_out_diff = src_params->yuv_out_diff;
    zme_params->dst_c_width = src_params->dst_c_width;
    zme_params->dst_c_height = src_params->dst_c_height;
    set_zme_to_vdpp_reg(zme_params, &ctx->zme);

    return MPP_OK;
}

static void vdpp2_set_default_dmsr_param(struct dmsr_params* p_dmsr_param)
{
    p_dmsr_param->dmsr_enable = 1;
    p_dmsr_param->dmsr_str_pri_y = 10;
    p_dmsr_param->dmsr_str_sec_y = 4;
    p_dmsr_param->dmsr_dumping_y = 6;
    p_dmsr_param->dmsr_wgt_pri_gain_even_1 = 12;
    p_dmsr_param->dmsr_wgt_pri_gain_even_2 = 12;
    p_dmsr_param->dmsr_wgt_pri_gain_odd_1 = 8;
    p_dmsr_param->dmsr_wgt_pri_gain_odd_2 = 16;
    p_dmsr_param->dmsr_wgt_sec_gain = 5;
    p_dmsr_param->dmsr_blk_flat_th = 20;
    p_dmsr_param->dmsr_contrast_to_conf_map_x0 = 1680;
    p_dmsr_param->dmsr_contrast_to_conf_map_x1 = 6720;
    p_dmsr_param->dmsr_contrast_to_conf_map_y0 = 0;
    p_dmsr_param->dmsr_contrast_to_conf_map_y1 = 65535;
    p_dmsr_param->dmsr_diff_core_th0 = 1;
    p_dmsr_param->dmsr_diff_core_th1 = 5;
    p_dmsr_param->dmsr_diff_core_wgt0 = 16;
    p_dmsr_param->dmsr_diff_core_wgt1 = 16;
    p_dmsr_param->dmsr_diff_core_wgt2 = 16;
    p_dmsr_param->dmsr_edge_th_low_arr[0] = 30;
    p_dmsr_param->dmsr_edge_th_low_arr[1] = 10;
    p_dmsr_param->dmsr_edge_th_low_arr[2] = 0;
    p_dmsr_param->dmsr_edge_th_low_arr[3] = 0;
    p_dmsr_param->dmsr_edge_th_low_arr[4] = 0;
    p_dmsr_param->dmsr_edge_th_low_arr[5] = 0;
    p_dmsr_param->dmsr_edge_th_low_arr[6] = 0;
    p_dmsr_param->dmsr_edge_th_high_arr[0] = 60;
    p_dmsr_param->dmsr_edge_th_high_arr[1] = 40;
    p_dmsr_param->dmsr_edge_th_high_arr[2] = 20;
    p_dmsr_param->dmsr_edge_th_high_arr[3] = 10;
    p_dmsr_param->dmsr_edge_th_high_arr[4] = 10;
    p_dmsr_param->dmsr_edge_th_high_arr[5] = 10;
    p_dmsr_param->dmsr_edge_th_high_arr[6] = 10;
}

static void vdpp_set_default_es_param(EsParams* p_es_param)
{
    p_es_param->es_bEnabledES          = 0;
    p_es_param->es_iAngleDelta         = 17;
    p_es_param->es_iAngleDeltaExtra    = 5;
    p_es_param->es_iGradNoDirTh        = 37;
    p_es_param->es_iGradFlatTh         = 75;
    p_es_param->es_iWgtGain            = 128;
    p_es_param->es_iWgtDecay           = 128;
    p_es_param->es_iLowConfTh          = 96;
    p_es_param->es_iLowConfRatio       = 32;
    p_es_param->es_iConfCntTh          = 4;
    p_es_param->es_iWgtLocalTh         = 64;
    p_es_param->es_iK1                 = 4096;
    p_es_param->es_iK2                 = 7168;
    p_es_param->es_iDeltaLimit         = 65280;
    memcpy(&p_es_param->es_iDiff2conf_lut_x[0], &diff2conf_lut_x_tmp[0], sizeof(diff2conf_lut_x_tmp));
    memcpy(&p_es_param->es_iDiff2conf_lut_y[0], &diff2conf_lut_y_tmp[0], sizeof(diff2conf_lut_y_tmp));
    p_es_param->es_bEndpointCheckEnable = 1;
}

static void vdpp_set_default_shp_param(ShpParams* p_shp_param)
{

    p_shp_param->sharp_enable               = 1;
    p_shp_param->sharp_coloradj_bypass_en   = 1;

    p_shp_param->lti_h_enable           = 0;
    p_shp_param->lti_h_radius           = 1;
    p_shp_param->lti_h_slope            = 100;
    p_shp_param->lti_h_thresold         = 21;
    p_shp_param->lti_h_gain             = 8;
    p_shp_param->lti_h_noise_thr_pos    = 1023;
    p_shp_param->lti_h_noise_thr_neg    = 1023;

    p_shp_param->lti_v_enable           = 0;
    p_shp_param->lti_v_radius           = 1;
    p_shp_param->lti_v_slope            = 100;
    p_shp_param->lti_v_thresold         = 21;
    p_shp_param->lti_v_gain             = 8;
    p_shp_param->lti_v_noise_thr_pos    = 1023;
    p_shp_param->lti_v_noise_thr_neg    = 1023;

    p_shp_param->cti_h_enable           = 0;
    p_shp_param->cti_h_radius           = 1;
    p_shp_param->cti_h_slope            = 100;
    p_shp_param->cti_h_thresold         = 21;
    p_shp_param->cti_h_gain             = 8;
    p_shp_param->cti_h_noise_thr_pos    = 1023;
    p_shp_param->cti_h_noise_thr_neg    = 1023;

    p_shp_param->peaking_enable         = 1;
    p_shp_param->peaking_gain           = 196;

    p_shp_param->peaking_coring_enable      = 1;
    p_shp_param->peaking_limit_ctrl_enable  = 1;
    p_shp_param->peaking_gain_enable        = 1;

    memcpy(p_shp_param->peaking_coring_zero,    coring_zero_tmp,    sizeof(coring_zero_tmp));
    memcpy(p_shp_param->peaking_coring_thr,     coring_thr_tmp,     sizeof(coring_thr_tmp));
    memcpy(p_shp_param->peaking_coring_ratio,   coring_ratio_tmp,   sizeof(coring_ratio_tmp));
    memcpy(p_shp_param->peaking_gain_pos,   gain_pos_tmp,   sizeof(gain_pos_tmp));
    memcpy(p_shp_param->peaking_gain_neg,   gain_neg_tmp,   sizeof(gain_neg_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_pos0,    limit_ctrl_pos0_tmp,    sizeof(limit_ctrl_pos0_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_pos1,    limit_ctrl_pos1_tmp,    sizeof(limit_ctrl_pos1_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_neg0,    limit_ctrl_neg0_tmp,    sizeof(limit_ctrl_neg0_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_neg1,    limit_ctrl_neg1_tmp,    sizeof(limit_ctrl_neg1_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_ratio,   limit_ctrl_ratio_tmp,   sizeof(limit_ctrl_ratio_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_bnd_pos, limit_ctrl_bnd_pos_tmp, sizeof(limit_ctrl_bnd_pos_tmp));
    memcpy(p_shp_param->peaking_limit_ctrl_bnd_neg, limit_ctrl_bnd_neg_tmp, sizeof(limit_ctrl_bnd_neg_tmp));

    p_shp_param->peaking_edge_ctrl_enable               = 1;
    p_shp_param->peaking_edge_ctrl_non_dir_thr          = 16;
    p_shp_param->peaking_edge_ctrl_dir_cmp_ratio        = 4;
    p_shp_param->peaking_edge_ctrl_non_dir_wgt_offset   = 64;
    p_shp_param->peaking_edge_ctrl_non_dir_wgt_ratio    = 16;
    p_shp_param->peaking_edge_ctrl_dir_cnt_thr          = 2;
    p_shp_param->peaking_edge_ctrl_dir_cnt_avg          = 3;
    p_shp_param->peaking_edge_ctrl_dir_cnt_offset       = 2;
    p_shp_param->peaking_edge_ctrl_diag_dir_thr         = 16;

    memcpy(p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab, diag_adj_gain_tab_tmp, sizeof(diag_adj_gain_tab_tmp));

    p_shp_param->peaking_estc_enable                    = 1;
    p_shp_param->peaking_estc_delta_offset_h            = 4;
    p_shp_param->peaking_estc_alpha_over_h              = 8;
    p_shp_param->peaking_estc_alpha_under_h             = 16;
    p_shp_param->peaking_estc_alpha_over_unlimit_h      = 64;
    p_shp_param->peaking_estc_alpha_under_unlimit_h     = 112;
    p_shp_param->peaking_estc_delta_offset_v            = 4;
    p_shp_param->peaking_estc_alpha_over_v              = 8;
    p_shp_param->peaking_estc_alpha_under_v             = 16;
    p_shp_param->peaking_estc_alpha_over_unlimit_v      = 64;
    p_shp_param->peaking_estc_alpha_under_unlimit_v     = 112;
    p_shp_param->peaking_estc_delta_offset_d0           = 4;
    p_shp_param->peaking_estc_alpha_over_d0             = 16;
    p_shp_param->peaking_estc_alpha_under_d0            = 16;
    p_shp_param->peaking_estc_alpha_over_unlimit_d0     = 96;
    p_shp_param->peaking_estc_alpha_under_unlimit_d0    = 96;
    p_shp_param->peaking_estc_delta_offset_d1           = 4;
    p_shp_param->peaking_estc_alpha_over_d1             = 16;
    p_shp_param->peaking_estc_alpha_under_d1            = 16;
    p_shp_param->peaking_estc_alpha_over_unlimit_d1     = 96;
    p_shp_param->peaking_estc_alpha_under_unlimit_d1    = 96;
    p_shp_param->peaking_estc_delta_offset_non          = 4;
    p_shp_param->peaking_estc_alpha_over_non            = 8;
    p_shp_param->peaking_estc_alpha_under_non           = 8;
    p_shp_param->peaking_estc_alpha_over_unlimit_non    = 112;
    p_shp_param->peaking_estc_alpha_under_unlimit_non   = 112;
    p_shp_param->peaking_filter_cfg_diag_enh_coef       = 6;

    p_shp_param->peaking_filt_core_H0[0]                = 4;
    p_shp_param->peaking_filt_core_H0[1]                = 16;
    p_shp_param->peaking_filt_core_H0[2]                = 24;
    p_shp_param->peaking_filt_core_H1[0]                = -16;
    p_shp_param->peaking_filt_core_H1[1]                = 0;
    p_shp_param->peaking_filt_core_H1[2]                = 32;
    p_shp_param->peaking_filt_core_H2[0]                = 0;
    p_shp_param->peaking_filt_core_H2[1]                = -16;
    p_shp_param->peaking_filt_core_H2[2]                = 32;
    p_shp_param->peaking_filt_core_V0[0]                = 1;
    p_shp_param->peaking_filt_core_V0[1]                = 4;
    p_shp_param->peaking_filt_core_V0[2]                = 6;
    p_shp_param->peaking_filt_core_V1[0]                = -4;
    p_shp_param->peaking_filt_core_V1[1]                = 0;
    p_shp_param->peaking_filt_core_V1[2]                = 8;
    p_shp_param->peaking_filt_core_V2[0]                = 0;
    p_shp_param->peaking_filt_core_V2[1]                = -4;
    p_shp_param->peaking_filt_core_V2[2]                = 8;
    p_shp_param->peaking_filt_core_USM[0]               = 1;
    p_shp_param->peaking_filt_core_USM[1]               = 4;
    p_shp_param->peaking_filt_core_USM[2]               = 6;

    p_shp_param->shootctrl_enable               = 1;
    p_shp_param->shootctrl_filter_radius        = 1;
    p_shp_param->shootctrl_delta_offset         = 16;
    p_shp_param->shootctrl_alpha_over           = 8;
    p_shp_param->shootctrl_alpha_under          = 8;
    p_shp_param->shootctrl_alpha_over_unlimit   = 112;
    p_shp_param->shootctrl_alpha_under_unlimit  = 112;

    p_shp_param->global_gain_enable             = 0;
    p_shp_param->global_gain_lum_mode           = 0;

    memcpy(p_shp_param->global_gain_lum_grd, lum_grd_tmp, sizeof(lum_grd_tmp));
    memcpy(p_shp_param->global_gain_lum_val, lum_val_tmp, sizeof(lum_val_tmp));
    memcpy(p_shp_param->global_gain_adp_grd, adp_grd_tmp, sizeof(adp_grd_tmp));
    memcpy(p_shp_param->global_gain_adp_val, adp_val_tmp, sizeof(adp_val_tmp));
    memcpy(p_shp_param->global_gain_var_grd, var_grd_tmp, sizeof(var_grd_tmp));
    memcpy(p_shp_param->global_gain_var_val, var_val_tmp, sizeof(var_val_tmp));

    p_shp_param->color_ctrl_enable              = 0;

    p_shp_param->color_ctrl_p0_scaling_coef     = 1;
    p_shp_param->color_ctrl_p0_point_u          = 115;
    p_shp_param->color_ctrl_p0_point_v          = 155;
    memcpy(p_shp_param->color_ctrl_p0_roll_tab, roll_tab_pattern0, sizeof(roll_tab_pattern0));
    p_shp_param->color_ctrl_p1_scaling_coef     = 1;
    p_shp_param->color_ctrl_p1_point_u          = 90;
    p_shp_param->color_ctrl_p1_point_v          = 120;
    memcpy(p_shp_param->color_ctrl_p1_roll_tab, roll_tab_pattern1, sizeof(roll_tab_pattern1));
    p_shp_param->color_ctrl_p2_scaling_coef     = 1;
    p_shp_param->color_ctrl_p2_point_u          = 128;
    p_shp_param->color_ctrl_p2_point_v          = 128;
    memcpy(p_shp_param->color_ctrl_p2_roll_tab, roll_tab_pattern2, sizeof(roll_tab_pattern2));
    p_shp_param->color_ctrl_p3_scaling_coef     = 1;
    p_shp_param->color_ctrl_p3_point_u          = 128;
    p_shp_param->color_ctrl_p3_point_v          = 128;
    memcpy(p_shp_param->color_ctrl_p3_roll_tab, roll_tab_pattern2, sizeof(roll_tab_pattern2));

    p_shp_param->tex_adj_enable                 = 0;
    p_shp_param->tex_adj_y_mode_select          = 3;
    p_shp_param->tex_adj_mode_select            = 0;

    memcpy(p_shp_param->tex_adj_grd, tex_grd_tmp, sizeof(tex_grd_tmp));
    memcpy(p_shp_param->tex_adj_val, tex_val_tmp, sizeof(tex_val_tmp));
}

static MPP_RET vdpp2_set_default_param(struct vdpp2_params *param)
{
    param->src_fmt = VDPP_FMT_YUV420;
    param->src_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->dst_fmt = VDPP_FMT_YUV444;
    param->dst_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->src_width = 1920;
    param->src_height = 1080;
    param->dst_width = 1920;
    param->dst_height = 1080;
    param->yuv_out_diff = 0;
    param->working_mode = VDPP_WORK_MODE_VEP;

    vdpp2_set_default_dmsr_param(&param->dmsr_params);
    vdpp_set_default_es_param(&param->es_params);
    vdpp_set_default_shp_param(&param->shp_params);

    param->hist_cnt_en   = 1;
    param->dci_hsd_mode  = VDPP_DCI_HSD_DISABLE;
    param->dci_vsd_mode  = VDPP_DCI_VSD_DISABLE;
    param->dci_yrgb_gather_num  = 0;
    param->dci_yrgb_gather_en  = 0;
    param->dci_csc_range = VDPP_COLOR_SPACE_LIMIT_RANGE;
    update_dci_ctl(param);

    vdpp_set_default_zme_param(&param->zme_params);

    return MPP_OK;
}

MPP_RET vdpp2_init(VdppCtx *ictx)
{
    MPP_RET ret;
    MppReqV1 mpp_req;
    RK_U32 client_data = VDPP_CLIENT_TYPE;
    struct vdpp2_api_ctx *ctx = NULL;

    if (NULL == *ictx) {
        mpp_err_f("found NULL input vdpp2 ctx %p\n", *ictx);
        return MPP_ERR_NULL_PTR;
    }

    ctx = *ictx;

    mpp_env_get_u32("vdpp2_debug", &vdpp2_debug, 0);

    ctx->fd = open("/dev/mpp_service", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        mpp_err("can NOT find device /dev/vdpp\n");
        return MPP_NOK;
    }

    mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
    mpp_req.flag = 0;
    mpp_req.size = sizeof(client_data);
    mpp_req.data_ptr = REQ_DATA_PTR(&client_data);

    memset(&ctx->params, 0, sizeof(struct vdpp2_params));
    /* set default parameters */
    vdpp2_set_default_param(&ctx->params);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        mpp_err("ioctl set_client failed\n");
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET vdpp2_deinit(VdppCtx ictx)
{
    struct vdpp2_api_ctx *ctx = NULL;

    if (NULL == ictx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", ictx);
        return MPP_ERR_NULL_PTR;
    }

    ctx = ictx;

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    return MPP_OK;
}

static MPP_RET vdpp2_set_param(struct vdpp2_api_ctx *ctx,
                               union vdpp_api_content *param,
                               enum VDPP_PARAM_TYPE type)
{
    RK_U16 mask;
    RK_U16 cfg_set;

    switch (type) {
    case VDPP_PARAM_TYPE_COM: // deprecated config
        ctx->params.hist_cnt_en = 0; // default disable

        ctx->params.src_fmt = VDPP_FMT_YUV420; // default 420
        ctx->params.src_yuv_swap = param->com.sswap;
        ctx->params.dst_fmt = param->com.dfmt;
        ctx->params.dst_yuv_swap = param->com.dswap;
        ctx->params.src_width = param->com.src_width;
        ctx->params.src_height = param->com.src_height;
        ctx->params.dst_width = param->com.dst_width;
        ctx->params.dst_height = param->com.dst_height;

        ctx->params.dmsr_params.dmsr_enable = 1;
        ctx->params.es_params.es_bEnabledES = 1;
        ctx->params.shp_params.sharp_enable = 1;
        /* set default vir stride */
        if (!ctx->params.src_width_vir)
            ctx->params.src_width_vir = MPP_ALIGN(ctx->params.src_width, 16);
        if (!ctx->params.src_height_vir)
            ctx->params.src_height_vir = MPP_ALIGN(ctx->params.src_height, 8);
        if (!ctx->params.dst_width)
            ctx->params.dst_width = ctx->params.src_width;
        if (!ctx->params.dst_height)
            ctx->params.dst_height = ctx->params.src_height;
        if (!ctx->params.dst_width_vir)
            ctx->params.dst_width_vir = MPP_ALIGN(ctx->params.dst_width, 16);
        if (!ctx->params.dst_height_vir)
            ctx->params.dst_height_vir = MPP_ALIGN(ctx->params.dst_height, 2);
        /* set default chrome stride */
        if (!ctx->params.dst_c_width)
            ctx->params.dst_c_width = ctx->params.dst_width;
        if (!ctx->params.dst_c_width_vir)
            ctx->params.dst_c_width_vir = MPP_ALIGN(ctx->params.dst_c_width, 16);
        if (!ctx->params.dst_c_height)
            ctx->params.dst_c_height = ctx->params.dst_height;
        if (!ctx->params.dst_c_height_vir)
            ctx->params.dst_c_height_vir = MPP_ALIGN(ctx->params.dst_c_height, 2);
        update_dci_ctl(&ctx->params);
        break;
    case VDPP_PARAM_TYPE_DMSR:
        memcpy(&ctx->params.dmsr_params, &param->dmsr, sizeof(struct dmsr_params));
        break;
    case VDPP_PARAM_TYPE_ZME_COM:
        ctx->params.zme_params.zme_bypass_en = param->zme.bypass_enable;
        ctx->params.zme_params.zme_dering_enable = param->zme.dering_enable;
        ctx->params.zme_params.zme_dering_sen_0 = param->zme.dering_sen_0;
        ctx->params.zme_params.zme_dering_sen_1 = param->zme.dering_sen_1;
        ctx->params.zme_params.zme_dering_blend_alpha = param->zme.dering_blend_alpha;
        ctx->params.zme_params.zme_dering_blend_beta = param->zme.dering_blend_beta;
        break;
    case VDPP_PARAM_TYPE_ZME_COEFF:
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
        break;
    case VDPP_PARAM_TYPE_COM2:
        mask = (param->com2.cfg_set >> 16) & 0x7;
        cfg_set = (param->com2.cfg_set >> 0) & mask;

        ctx->params.src_yuv_swap = param->com2.sswap;
        ctx->params.src_fmt = param->com2.sfmt;
        ctx->params.dst_fmt = param->com2.dfmt;
        ctx->params.dst_yuv_swap = param->com2.dswap;
        ctx->params.src_width = param->com2.src_width;
        ctx->params.src_height = param->com2.src_height;
        ctx->params.src_width_vir = param->com2.src_width_vir;
        ctx->params.src_height_vir = param->com2.src_height_vir;
        ctx->params.dst_width = param->com2.dst_width;
        ctx->params.dst_height = param->com2.dst_height;
        ctx->params.dst_width_vir = param->com2.dst_width_vir;
        ctx->params.dst_height_vir = param->com2.dst_height_vir;
        ctx->params.yuv_out_diff = param->com2.yuv_out_diff;
        ctx->params.dst_c_width = param->com2.dst_c_width;
        ctx->params.dst_c_height = param->com2.dst_c_height;
        ctx->params.dst_c_width_vir = param->com2.dst_c_width_vir;
        ctx->params.dst_c_height_vir = param->com2.dst_c_height_vir;
        ctx->params.working_mode = param->com2.hist_mode_en ?
                                   VDPP_WORK_MODE_DCI : VDPP_WORK_MODE_VEP;
        if (mask & VDPP_DMSR_EN)
            ctx->params.dmsr_params.dmsr_enable = (cfg_set & VDPP_DMSR_EN) ? 1 : 0;
        if (mask & VDPP_ES_EN)
            ctx->params.es_params.es_bEnabledES = (cfg_set & VDPP_ES_EN) ? 1 : 0;
        if (mask & VDPP_SHARP_EN)
            ctx->params.shp_params.sharp_enable = (cfg_set & VDPP_SHARP_EN) ? 1 : 0;
        update_dci_ctl(&ctx->params);
        break;
    case VDPP_PARAM_TYPE_ES:
        memcpy(&ctx->params.es_params, &param->es, sizeof(EsParams));
        update_es_tan(&ctx->params.es_params);
        break;
    case VDPP_PARAM_TYPE_HIST:
        ctx->params.hist_cnt_en = param->hist.hist_cnt_en;
        ctx->params.dci_hsd_mode = param->hist.dci_hsd_mode;
        ctx->params.dci_vsd_mode = param->hist.dci_vsd_mode;
        ctx->params.dci_yrgb_gather_num = param->hist.dci_yrgb_gather_num;
        ctx->params.dci_yrgb_gather_en = param->hist.dci_yrgb_gather_en;
        ctx->params.dci_csc_range = param->hist.dci_csc_range;
        update_dci_ctl(&ctx->params);
        break;
    case VDPP_PARAM_TYPE_SHARP:
        memcpy(&ctx->params.shp_params, &param->sharp, sizeof(ShpParams));
        break;
    default:
        break;
    }

    return MPP_OK;
}

static RK_S32 check_cap(struct vdpp2_params *params)
{
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_U32 vep_mode_check = 0;
    RK_U32 hist_mode_check = 0;

    if (NULL == params) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "found null pointer params\n");
        return VDPP_CAP_UNSUPPORTED;
    }

    if (params->src_height_vir < params->src_height ||
        params->src_width_vir < params->src_width) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "invalid src img_w %d img_h %d img_w_vir %d img_h_vir %d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        return VDPP_CAP_UNSUPPORTED;
    }

    if ((params->src_fmt != MPP_FMT_YUV420SP) &&
        (params->src_fmt != MPP_FMT_YUV420SP_VU)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support nv12 or nv21\n");
        vep_mode_check++;
    }

    if ((params->src_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP2_VEP_MAX_WIDTH) ||
        (params->src_height < VDPP2_MODE_MIN_HEIGH) ||
        (params->src_width < VDPP2_MODE_MIN_WIDTH) ||
        (params->src_height_vir < VDPP2_MODE_MIN_HEIGH) ||
        (params->src_width_vir < VDPP2_MODE_MIN_WIDTH)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported src img_w %d img_h %d img_w_vir %d img_h_vir %d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        vep_mode_check++;
    }

    if ((params->dst_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
        (params->dst_width_vir > VDPP2_VEP_MAX_WIDTH) ||
        (params->dst_height < VDPP2_MODE_MIN_HEIGH) ||
        (params->dst_width < VDPP2_MODE_MIN_WIDTH) ||
        (params->dst_height_vir < VDPP2_MODE_MIN_HEIGH) ||
        (params->dst_width_vir < VDPP2_MODE_MIN_WIDTH) ||
        (params->dst_height_vir < params->dst_height) ||
        (params->dst_width_vir < params->dst_width)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported dst img_w %d img_h %d img_w_vir %d img_h_vir %d\n",
                  params->dst_width, params->dst_height,
                  params->dst_width_vir, params->dst_height_vir);
        vep_mode_check++;
    }

    if ((params->src_width_vir & 0xf) || (params->dst_width_vir & 0xf)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_w_i/o_vir 16Byte align\n");
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported src img_w_vir %d dst img_w_vir %d\n",
                  params->src_width_vir, params->dst_width_vir);
        vep_mode_check++;
    }

    if (params->src_height_vir & 0x7) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_h_in_vir 8pix align\n");
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported src img_h_vir %d\n", params->src_height_vir);
        vep_mode_check++;
    }

    if (params->dst_height_vir & 0x1) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_h_out_vir 2pix align\n");
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported dst img_h_vir %d\n", params->dst_height_vir);
        vep_mode_check++;
    }

    if ((params->src_width & 1) || (params->src_height & 1) ||
        (params->dst_width & 1) || (params->dst_height & 1)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_w/h_vld 2pix align\n");
        VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported img_w_i %d img_h_i %d img_w_o %d img_h_o %d\n",
                  params->src_width, params->src_height, params->dst_width, params->dst_height);
        vep_mode_check++;
    }

    if (params->yuv_out_diff) {
        if ((params->dst_c_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
            (params->dst_c_width_vir > VDPP2_VEP_MAX_WIDTH) ||
            (params->dst_c_height < VDPP2_MODE_MIN_HEIGH) ||
            (params->dst_c_width < VDPP2_MODE_MIN_WIDTH) ||
            (params->dst_c_height_vir < VDPP2_MODE_MIN_HEIGH) ||
            (params->dst_c_width_vir < VDPP2_MODE_MIN_WIDTH) ||
            (params->dst_c_height_vir < params->dst_c_height) ||
            (params->dst_c_width_vir < params->dst_c_width)) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported dst_c img_w %d img_h %d img_w_vir %d img_h_vir %d\n",
                      params->dst_c_width, params->dst_c_height,
                      params->dst_c_width_vir, params->dst_c_height_vir);
            vep_mode_check++;
        }

        if (params->dst_c_width_vir & 0xf) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_w_c_out_vir 16Byte align\n");
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported dst img_w_c_vir %d\n", params->dst_c_width_vir);
            vep_mode_check++;
        }

        if (params->dst_c_height_vir & 0x1) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_h_c_out_vir 2pix align\n");
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported dst img_h_vir %d\n", params->dst_c_height_vir);
            vep_mode_check++;
        }

        if ((params->dst_c_width & 1) || (params->dst_c_height & 1)) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep only support img_c_w/h_vld 2pix align\n");
            VDPP2_DBG(VDPP2_DBG_CHECK, "vep unsupported img_w_o %d img_h_o %d\n",
                      params->dst_c_width, params->dst_c_height);
            vep_mode_check++;
        }
    }

    if ((params->src_height_vir > VDPP2_HIST_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP2_HIST_MAX_WIDTH) ||
        (params->src_height < VDPP2_MODE_MIN_HEIGH) ||
        (params->src_width < VDPP2_MODE_MIN_WIDTH)) {
        VDPP2_DBG(VDPP2_DBG_CHECK, "dci unsupported src img_w %d img_h %d img_w_vir %d img_h_vir %d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        hist_mode_check++;
    }

    /* 10 bit input */
    if ((params->src_fmt == MPP_FMT_YUV420SP_10BIT) ||
        (params->src_fmt == MPP_FMT_YUV422SP_10BIT) ||
        (params->src_fmt == MPP_FMT_YUV444SP_10BIT)) {
        if (params->src_width_vir & 0xf) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "dci Y-10bit input only support img_w_in_vir 16Byte align\n");
            hist_mode_check++;
        }
    } else {
        if (params->src_width_vir & 0x3) {
            VDPP2_DBG(VDPP2_DBG_CHECK, "dci only support img_w_in_vir 4Byte align\n");
            hist_mode_check++;
        }
    }

    VDPP2_DBG(VDPP2_DBG_INT, "vdpp2 src img resolution: w-%d-%d, h-%d-%d\n",
              params->src_width, params->src_width_vir,
              params->src_height, params->src_height_vir);
    VDPP2_DBG(VDPP2_DBG_INT, "vdpp2 dst img resolution: w-%d-%d, h-%d-%d\n",
              params->dst_width, params->dst_width_vir,
              params->dst_height, params->dst_height_vir);

    if (!vep_mode_check) {
        ret_cap |= VDPP_CAP_VEP;
        VDPP2_DBG(VDPP2_DBG_INT, "vdpp2 support mode: VDPP_CAP_VEP\n");
    }

    if (!hist_mode_check) {
        VDPP2_DBG(VDPP2_DBG_INT, "vdpp2 support mode: VDPP_CAP_HIST\n");
        ret_cap |= VDPP_CAP_HIST;
    }

    return ret_cap;
}

MPP_RET vdpp2_start(struct vdpp2_api_ctx *ctx)
{
    MPP_RET ret;
    RegOffsetInfo reg_off[2];
    MppReqV1 mpp_req[12];
    RK_U32 req_cnt = 0;
    struct vdpp2_reg *reg = NULL;
    struct zme_reg *zme = NULL;
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_S32 work_mode;

    if (NULL == ctx) {
        mpp_err_f("found NULL input vdpp2 ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    work_mode = ctx->params.working_mode;
    ret_cap = check_cap(&ctx->params);
    if ((!(ret_cap & VDPP_CAP_VEP) && (VDPP_WORK_MODE_VEP == work_mode)) ||
        (!(ret_cap & VDPP_CAP_HIST) && (VDPP_WORK_MODE_DCI == work_mode))) {
        mpp_err_f("found incompat work mode %d %s, cap %d\n", work_mode,
                  working_mode_name[work_mode & 3], ret_cap);
        return MPP_NOK;
    }

    reg = &ctx->reg;
    zme = &ctx->zme;

    memset(reg_off, 0, sizeof(reg_off));
    memset(mpp_req, 0, sizeof(mpp_req));
    memset(reg, 0, sizeof(*reg));

    vdpp2_params_to_reg(&ctx->params, ctx);

    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(zme->yrgb_hor_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_YRGB_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&zme->yrgb_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(zme->yrgb_ver_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_YRGB_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&zme->yrgb_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(zme->cbcr_hor_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_CBCR_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&zme->cbcr_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(zme->cbcr_ver_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_CBCR_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&zme->cbcr_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(zme->common);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_ZME_COMMON;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&zme->common);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(ctx->dmsr);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_DMSR;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&ctx->dmsr);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(reg->dci);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_DCI;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->dci);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(reg->es);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_ES;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->es);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(reg->sharp);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_SHARP;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->sharp);

    /* set reg offset */
    reg_off[0].reg_idx = 25;
    reg_off[0].offset = ctx->params.src.cbcr_offset;
    reg_off[1].reg_idx = 27;

    if (!ctx->params.yuv_out_diff)
        reg_off[1].offset = ctx->params.dst.cbcr_offset;
    else
        reg_off[1].offset = ctx->params.dst_c.cbcr_offset;
    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_REG_OFFSET_ALONE;
    mpp_req[req_cnt].size = sizeof(reg_off);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(reg_off);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size =  sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    mpp_req[req_cnt].size =  sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req[0]);

    if (ret) {
        mpp_err_f("ioctl SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    return ret;
}

static MPP_RET vdpp2_wait(struct vdpp2_api_ctx *ctx)
{
    MPP_RET ret;
    MppReqV1 mpp_req;

    if (NULL == ctx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    memset(&mpp_req, 0, sizeof(mpp_req));
    mpp_req.cmd = MPP_CMD_POLL_HW_FINISH;
    mpp_req.flag |= MPP_FLAGS_LAST_MSG;

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        mpp_err_f("ioctl POLL_HW_FINISH failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
    }

    return ret;
}

static MPP_RET vdpp2_done(struct vdpp2_api_ctx *ctx)
{
    struct vdpp2_reg *reg = &ctx->reg;

    if (NULL == ctx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    reg = &ctx->reg;

    VDPP2_DBG(VDPP2_DBG_INT, "ro_frm_done_sts=%d\n", reg->common.reg10.ro_frm_done_sts);
    VDPP2_DBG(VDPP2_DBG_INT, "ro_osd_max_sts=%d\n", reg->common.reg10.ro_osd_max_sts);
    VDPP2_DBG(VDPP2_DBG_INT, "ro_bus_error_sts=%d\n", reg->common.reg10.ro_bus_error_sts);
    VDPP2_DBG(VDPP2_DBG_INT, "ro_timeout_sts=%d\n", reg->common.reg10.ro_timeout_sts);
    VDPP2_DBG(VDPP2_DBG_INT, "ro_config_error_sts=%d\n", reg->common.reg10.ro_timeout_sts);

    if (reg->common.reg8.sw_vdpp_frm_done_en &&
        !reg->common.reg10.ro_frm_done_sts) {
        mpp_err_f("run vdpp failed\n");
        return MPP_NOK;
    }

    VDPP2_DBG(VDPP2_DBG_INT, "run vdpp success\n");

    return MPP_OK;
}

static inline MPP_RET set_addr(struct vdpp_addr *addr, VdppImg *img)
{
    addr->y = img->mem_addr;
    addr->cbcr = img->uv_addr;
    addr->cbcr_offset = img->uv_off;

    return MPP_OK;
}

MPP_RET vdpp2_control(VdppCtx ictx, VdppCmd cmd, void *iparam)
{
    struct vdpp2_api_ctx *ctx = ictx;
    MPP_RET ret = MPP_OK;

    if ((NULL == iparam && VDPP_CMD_RUN_SYNC != cmd) ||
        (NULL == ictx)) {
        mpp_err_f("found NULL iparam %p cmd %d ctx %p\n", iparam, cmd, ictx);
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case VDPP_CMD_SET_COM_CFG:
    case VDPP_CMD_SET_DMSR_CFG:
    case VDPP_CMD_SET_ZME_COM_CFG:
    case VDPP_CMD_SET_ZME_COEFF_CFG:
    case VDPP_CMD_SET_COM2_CFG:
    case VDPP_CMD_SET_ES:
    case VDPP_CMD_SET_DCI_HIST:
    case VDPP_CMD_SET_SHARP: {
        struct vdpp_api_params *param = (struct vdpp_api_params *)iparam;

        ret = vdpp2_set_param(ctx, &param->param, param->ptype);
        if (ret) {
            mpp_err_f("set vdpp parameter failed, type %d\n", param->ptype);
        }
        break;
    }
    case VDPP_CMD_SET_SRC:
        set_addr(&ctx->params.src, (VdppImg *)iparam);
        break;
    case VDPP_CMD_SET_DST:
        set_addr(&ctx->params.dst, (VdppImg *)iparam);
        break;
    case VDPP_CMD_SET_DST_C:
        set_addr(&ctx->params.dst_c, (VdppImg *)iparam);
        break;
    case VDPP_CMD_SET_HIST_FD:
        ctx->params.hist = *(RK_S32 *)iparam;
        break;
    case VDPP_CMD_RUN_SYNC:
        ret = vdpp2_start(ctx);
        if (ret) {
            mpp_err_f("run vdpp failed\n");
            return MPP_NOK;
        }

        vdpp2_wait(ctx);
        vdpp2_done(ctx);
        break;
    default:
        ;
    }

    return ret;
}

RK_S32 vdpp2_check_cap(VdppCtx ictx)
{
    struct vdpp2_api_ctx *ctx = ictx;

    if (NULL == ictx) {
        mpp_err_f("found NULL ctx %p\n", ictx);
        return VDPP_CAP_UNSUPPORTED;
    }

    return check_cap(&ctx->params);
}
