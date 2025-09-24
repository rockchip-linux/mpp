/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp"

#include <sys/ioctl.h>
#include <errno.h>
#include <math.h>
#include <sys/mman.h>
#include <string.h>

#include "mpp_common.h"
#include "mpp_env.h"
#include "mpp_service.h"
#include "mpp_platform.h"

#include "vdpp2.h"

#define VDPP2_VEP_MAX_WIDTH   (1920)
#define VDPP2_VEP_MAX_HEIGHT  (2048)
#define VDPP2_HIST_MAX_WIDTH  (4096)
#define VDPP2_HIST_MAX_HEIGHT (4096)
#define VDPP2_MODE_MIN_WIDTH  (128)
#define VDPP2_MODE_MIN_HEIGHT (128)

#define VDPP2_HIST_HSD_DISABLE_LIMIT (1920)
#define VDPP2_HIST_VSD_DISABLE_LIMIT (1080)
#define VDPP2_HIST_VSD_MODE_1_LIMIT  (2160)

// default log level: warning
extern RK_U32 vdpp_debug;

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


int update_dci_ctl(HistParams *hist_params)
{
    RK_S32 dci_format = VDPP_DCI_FMT_BUTT;
    RK_S32 dci_alpha_swap = 0;
    RK_S32 dci_rbuv_swap = 0;

    /* for YUV formats: only consider luma data  */
    /* for RGB unpacked formats: convert LSB order to MSB order */
    switch (hist_params->src_fmt) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV422SP:
    case MPP_FMT_YUV420P:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV400:
    case MPP_FMT_YUV440SP:
    case MPP_FMT_YUV411SP:
    case MPP_FMT_YUV444SP:
    case MPP_FMT_YUV444P:  {
        dci_format = VDPP_DCI_FMT_Y_8bit_SP;
        dci_rbuv_swap = 0;
    } break;
    case MPP_FMT_YUV420SP_VU:
    case MPP_FMT_YUV422SP_VU: {
        dci_format = VDPP_DCI_FMT_Y_8bit_SP;
        dci_rbuv_swap = 1;
    } break;
    case MPP_FMT_YUV420SP_10BIT:
    case MPP_FMT_YUV422SP_10BIT:
    case MPP_FMT_YUV444SP_10BIT: {
        dci_format = VDPP_DCI_FMT_Y_10bit_SP;
        dci_rbuv_swap = 0;
    } break;
    case MPP_FMT_RGB888: {
        dci_format = VDPP_DCI_FMT_RGB888;
        dci_rbuv_swap = 1;
    } break;
    case MPP_FMT_BGR888: {
        dci_format = VDPP_DCI_FMT_RGB888;
        dci_rbuv_swap = 0;
    } break;
    case MPP_FMT_ARGB8888: {
        dci_format = VDPP_DCI_FMT_ARGB8888;
        dci_alpha_swap = 1;
        dci_rbuv_swap = 1;
    } break;
    case MPP_FMT_ABGR8888: {
        dci_format = VDPP_DCI_FMT_ARGB8888;
        dci_alpha_swap = 1;
        dci_rbuv_swap = 0;
    } break;
    case MPP_FMT_BGRA8888: {
        dci_format = VDPP_DCI_FMT_ARGB8888;
        dci_alpha_swap = 0;
        dci_rbuv_swap = 0;
    } break;
    case MPP_FMT_RGBA8888: {
        dci_format = VDPP_DCI_FMT_ARGB8888;
        dci_alpha_swap = 0;
        dci_rbuv_swap = 1;
    } break;
    default: break;
    }

    if (dci_format == VDPP_DCI_FMT_BUTT) {
        vdpp_loge("error: invalid input mpp format %d !", hist_params->src_fmt);
        return MPP_ERR_VALUE;
    }

    hist_params->dci_format = dci_format;
    hist_params->dci_alpha_swap = dci_alpha_swap;
    hist_params->dci_rbuv_swap = dci_rbuv_swap;

    vdpp_logv("input mpp_format=%d => dci_format=%d (0: rgb24, 1: argb32, 4: y8, 5: y10_packed), "
              "alpha_swap=%d, rbuv_swap=%d",
              hist_params->src_fmt, dci_format, dci_alpha_swap, dci_rbuv_swap);
    return MPP_OK;
}

void set_hist_to_vdpp2_reg(HistParams *hist_params, RegHist *hist, RegCom2 *reg_com)
{
    RK_S32 pic_vir_src_ystride;
    RK_S32 hsd_sample_num, vsd_sample_num, sw_dci_blk_hsize, sw_dci_blk_vsize;
    RK_U32 dci_hsd_mode, dci_vsd_mode;
    MPP_RET ret = MPP_OK;

    // update 'dci_format/dci_alpha_swap/dci_rbuv_swap' according to input mpp format
    ret = update_dci_ctl(hist_params);
    if (ret) {
        reg_com->reg1.sw_dci_en = 0;
        return;
    }

    pic_vir_src_ystride = (hist_params->src_width + 3) / 4;

    if (hist_params->dci_format == VDPP_DCI_FMT_RGB888) {
        pic_vir_src_ystride = hist_params->src_width_vir * 3 / 4;
    } else if (hist_params->dci_format == VDPP_DCI_FMT_ARGB8888) {
        pic_vir_src_ystride = hist_params->src_width_vir * 4 / 4;
    } else if (hist_params->dci_format == VDPP_DCI_FMT_Y_10bit_SP) { // Y 10bit SP
        pic_vir_src_ystride = hist_params->src_width_vir * 10 / 8 / 4;
    } else if (hist_params->dci_format == VDPP_DCI_FMT_Y_8bit_SP) { // Y 8bit SP
        pic_vir_src_ystride = hist_params->src_width_vir / 4;
    } else {
        vdpp_loge("unsupported dci format %d!\n", hist_params->dci_format);
        return;
    }

    dci_hsd_mode = (hist_params->dci_hsd_mode & 1);
    dci_vsd_mode = (hist_params->dci_vsd_mode & 3);

    /* check vdpp scale down mode for performance optimization */
    if (!(dci_hsd_mode & VDPP_DCI_HSD_MODE_1) &&
        (hist_params->src_width_vir > VDPP2_HIST_HSD_DISABLE_LIMIT)) {
        vdpp_logd("w_vir %d hsd mode %d is optimized as mode 1\n",
                  hist_params->src_width_vir, dci_hsd_mode);
        dci_hsd_mode = VDPP_DCI_HSD_MODE_1;
    }

    if (!(dci_vsd_mode & VDPP_DCI_VSD_MODE_1) &&
        (hist_params->src_height_vir > VDPP2_HIST_VSD_DISABLE_LIMIT)) {
        vdpp_logd("h_vir %d vsd mode %d is optimized as mode 1\n",
                  hist_params->src_height_vir, dci_vsd_mode);
        dci_vsd_mode = VDPP_DCI_VSD_MODE_1;
    }

    if (!(dci_vsd_mode & VDPP_DCI_VSD_MODE_2) &&
        (hist_params->src_height_vir > VDPP2_HIST_VSD_MODE_1_LIMIT)) {
        vdpp_logd("h_vir %d vsd mode %d is optimized as mode 2\n",
                  hist_params->src_height_vir, dci_vsd_mode);
        dci_vsd_mode = VDPP_DCI_VSD_MODE_2;
    }

    dci_vsd_mode = (hist_params->working_mode == VDPP_WORK_MODE_VEP) ? 0 : dci_vsd_mode;
    dci_hsd_mode = (hist_params->working_mode == VDPP_WORK_MODE_VEP) ? 0 : dci_hsd_mode;
    vdpp_logv("final vsd/hsd mode: %d/%d, working_mode: %d\n",
              dci_vsd_mode, dci_hsd_mode, hist_params->working_mode);

    switch (dci_hsd_mode) {
    case VDPP_DCI_HSD_MODE_1:
        hsd_sample_num = 4;
        break;
    case VDPP_DCI_HSD_DISABLE:
    default:
        hsd_sample_num = 2;
        break;
    }

    switch (dci_vsd_mode) {
    case VDPP_DCI_VSD_MODE_1:
        vsd_sample_num = 2;
        break;
    case VDPP_DCI_VSD_MODE_2:
        vsd_sample_num = 4;
        break;
    case VDPP_DCI_VSD_DISABLE:
    default:
        vsd_sample_num = 1;
        break;
    }

    if (hist_params->src_height < 1080)
        sw_dci_blk_vsize = hist_params->src_height / (16 * vsd_sample_num);
    else
        sw_dci_blk_vsize = (hist_params->src_height + (16 * vsd_sample_num - 1)) / (16 * vsd_sample_num);

    if (hist_params->src_width < 1080)
        sw_dci_blk_hsize = hist_params->src_width / (16 * hsd_sample_num);
    else
        sw_dci_blk_hsize = (hist_params->src_width + (16 * hsd_sample_num - 1)) / (16 * hsd_sample_num);

    reg_com->reg1.sw_dci_en = hist_params->hist_cnt_en;

    hist->reg0.sw_dci_yrgb_addr = hist_params->src_addr;
    hist->reg1.sw_dci_yrgb_vir_stride = pic_vir_src_ystride;
    hist->reg1.sw_dci_yrgb_gather_num = hist_params->dci_yrgb_gather_num;
    hist->reg1.sw_dci_yrgb_gather_en = hist_params->dci_yrgb_gather_en;
    hist->reg2.sw_vep_src_pic_width = MPP_ALIGN_DOWN(hist_params->src_width, hsd_sample_num) - 1;
    hist->reg2.sw_vep_src_pic_height = MPP_ALIGN_DOWN(hist_params->src_height, vsd_sample_num) - 1;
    hist->reg3.sw_dci_data_format = hist_params->dci_format;
    hist->reg3.sw_dci_csc_range = hist_params->dci_csc_range;
    hist->reg3.sw_dci_vsd_mode = dci_vsd_mode;
    hist->reg3.sw_dci_hsd_mode = dci_hsd_mode;
    hist->reg3.sw_dci_alpha_swap = hist_params->dci_alpha_swap;
    hist->reg3.sw_dci_rb_swap = hist_params->dci_rbuv_swap;
    hist->reg3.sw_dci_blk_hsize = sw_dci_blk_hsize;
    hist->reg3.sw_dci_blk_vsize = sw_dci_blk_vsize;
    hist->reg4.sw_dci_hist_addr = hist_params->hist_addr;

    if (reg_com->reg1.sw_dci_en) {
        vdpp_logv("set reg: dci_yrgb_addr(fd)=%d\n", hist->reg0.sw_dci_yrgb_addr);
        vdpp_logv("set reg: dci_yrgb_vir_stride=%d [dword], gather_num=%d, gather_en=%d\n",
                  hist->reg1.sw_dci_yrgb_vir_stride, hist->reg1.sw_dci_yrgb_gather_num,
                  hist->reg1.sw_dci_yrgb_gather_en);
        vdpp_logv("set reg: vep_src pic_width=%d [pixel], pic_height=%d [pixel]\n",
                  hist->reg2.sw_vep_src_pic_width, hist->reg2.sw_vep_src_pic_height);
        vdpp_logv("set reg: dci_data_format=%d, csc_range=%d, vsd_mode=%d, hsd_mode=%d\n",
                  hist->reg3.sw_dci_data_format, hist->reg3.sw_dci_csc_range,
                  hist->reg3.sw_dci_vsd_mode, hist->reg3.sw_dci_hsd_mode);
        vdpp_logv("set reg: dci_alpha_swap=%d, rb_swap=%d, blk_hsize=%d, blk_vsize=%d\n",
                  hist->reg3.sw_dci_alpha_swap, hist->reg3.sw_dci_rb_swap,
                  hist->reg3.sw_dci_blk_hsize, hist->reg3.sw_dci_blk_vsize);
        vdpp_logv("set reg: dci_hist_addr(fd)=%d\n", hist->reg4.sw_dci_hist_addr);
    } else {
        vdpp_logv("set reg: dissable DCI_HIST!\n");
    }
}

static void update_es_tan(EsParams* p_es_param)
{
    RK_U32 angle_lo_th, angle_hi_th;

    angle_lo_th = 23 - p_es_param->es_iAngleDelta - p_es_param->es_iAngleDeltaExtra;
    angle_hi_th = 23 + p_es_param->es_iAngleDelta;

    p_es_param->es_tan_lo_th = mpp_clip(tan(angle_lo_th * acos(-1) / 180.0) * 512, 0, 511);
    p_es_param->es_tan_hi_th = mpp_clip(tan(angle_hi_th * acos(-1) / 180.0) * 512, 0, 511);
}

void set_es_to_vdpp2_reg(EsParams *p_es_param, RegEs *es)
{
    RK_U8 diff2conf_lut_k[8] = {0};
    RK_U8 conf_local_mean_th = 0;
    RK_S32 i;

    if (p_es_param->es_iWgtGain > 0) {
        conf_local_mean_th = mpp_clip(256 * (float)p_es_param->es_iWgtLocalTh / p_es_param->es_iWgtGain, 0, 255);
    }

    update_es_tan(p_es_param);

    es->reg0.es_enable = p_es_param->es_bEnabledES;
    es->reg1.dir_th    = p_es_param->es_iGradNoDirTh;
    es->reg1.flat_th   = p_es_param->es_iGradFlatTh;

    es->reg2.tan_lo_th = p_es_param->es_tan_lo_th;
    es->reg2.tan_hi_th = p_es_param->es_tan_hi_th;

    es->reg3.ep_chk_en = p_es_param->es_bEndpointCheckEnable;
    es->reg3.mem_gat_en = 1; // enable this to reduce power consumption

    es->reg4.diff_gain0 = p_es_param->es_iK1;
    es->reg4.diff_limit = p_es_param->es_iDeltaLimit;

    es->reg5.diff_gain1 = p_es_param->es_iK2;
    es->reg5.lut_x0     = p_es_param->es_iDiff2conf_lut_x[0];

    es->reg6.lut_x1    = p_es_param->es_iDiff2conf_lut_x[1];
    es->reg6.lut_x2    = p_es_param->es_iDiff2conf_lut_x[2];
    es->reg7.lut_x3    = p_es_param->es_iDiff2conf_lut_x[3];
    es->reg7.lut_x4    = p_es_param->es_iDiff2conf_lut_x[4];
    es->reg8.lut_x5    = p_es_param->es_iDiff2conf_lut_x[5];
    es->reg8.lut_x6    = p_es_param->es_iDiff2conf_lut_x[6];
    es->reg9.lut_x7    = p_es_param->es_iDiff2conf_lut_x[7];
    es->reg9.lut_x8    = p_es_param->es_iDiff2conf_lut_x[8];

    es->reg10.lut_y0    = p_es_param->es_iDiff2conf_lut_y[0];
    es->reg10.lut_y1    = p_es_param->es_iDiff2conf_lut_y[1];
    es->reg10.lut_y2    = p_es_param->es_iDiff2conf_lut_y[2];
    es->reg10.lut_y3    = p_es_param->es_iDiff2conf_lut_y[3];
    es->reg11.lut_y4    = p_es_param->es_iDiff2conf_lut_y[4];
    es->reg11.lut_y5    = p_es_param->es_iDiff2conf_lut_y[5];
    es->reg11.lut_y6    = p_es_param->es_iDiff2conf_lut_y[6];
    es->reg11.lut_y7    = p_es_param->es_iDiff2conf_lut_y[7];
    es->reg12.lut_y8    = p_es_param->es_iDiff2conf_lut_y[8];

    for (i = 0; i < 8; i++) {
        double x0 = p_es_param->es_iDiff2conf_lut_x[i];
        double x1 = p_es_param->es_iDiff2conf_lut_x[i + 1];
        double y0 = p_es_param->es_iDiff2conf_lut_y[i];
        double y1 = p_es_param->es_iDiff2conf_lut_y[i + 1];

        diff2conf_lut_k[i] = mpp_clip(FLOOR(256 * (y1 - y0) / (x1 - x0)), 0, 255);
    }
    es->reg13.lut_k0 = diff2conf_lut_k[0];
    es->reg13.lut_k1 = diff2conf_lut_k[1];
    es->reg13.lut_k2 = diff2conf_lut_k[2];
    es->reg13.lut_k3 = diff2conf_lut_k[3];
    es->reg14.lut_k4 = diff2conf_lut_k[4];
    es->reg14.lut_k5 = diff2conf_lut_k[5];
    es->reg14.lut_k6 = diff2conf_lut_k[6];
    es->reg14.lut_k7 = diff2conf_lut_k[7];

    es->reg15.wgt_decay = p_es_param->es_iWgtDecay;
    es->reg15.wgt_gain  = p_es_param->es_iWgtGain;

    es->reg16.conf_mean_th    = conf_local_mean_th;
    es->reg16.conf_cnt_th     = p_es_param->es_iConfCntTh;
    es->reg16.low_conf_th     = p_es_param->es_iLowConfTh;
    es->reg16.low_conf_ratio  = p_es_param->es_iLowConfRatio;
}

void set_shp_to_vdpp2_reg(ShpParams *p_shp_param, RegShp *shp)
{
    /* Peaking Ctrl sw-process */
    RK_S32 peaking_ctrl_idx_P0[7] = {0};
    RK_S32 peaking_ctrl_idx_N0[7] = {0};
    RK_S32 peaking_ctrl_idx_P1[7] = {0};
    RK_S32 peaking_ctrl_idx_N1[7] = {0};
    RK_S32 peaking_ctrl_idx_P2[7] = {0};
    RK_S32 peaking_ctrl_idx_N2[7] = {0};
    RK_S32 peaking_ctrl_idx_P3[7] = {0};
    RK_S32 peaking_ctrl_idx_N3[7] = {0};

    RK_S32 peaking_ctrl_value_N1[7] = {0};
    RK_S32 peaking_ctrl_value_N2[7] = {0};
    RK_S32 peaking_ctrl_value_N3[7] = {0};
    RK_S32 peaking_ctrl_value_P1[7] = {0};
    RK_S32 peaking_ctrl_value_P2[7] = {0};
    RK_S32 peaking_ctrl_value_P3[7] = {0};

    RK_S32 peaking_ctrl_ratio_P01[7] = {0};
    RK_S32 peaking_ctrl_ratio_P12[7] = {0};
    RK_S32 peaking_ctrl_ratio_P23[7] = {0};
    RK_S32 peaking_ctrl_ratio_N01[7] = {0};
    RK_S32 peaking_ctrl_ratio_N12[7] = {0};
    RK_S32 peaking_ctrl_ratio_N23[7] = {0};

    RK_S32 global_gain_slp_temp[5] = {0};
    RK_S32 ii;

    shp->reg0.sw_sharp_enable = p_shp_param->sharp_enable;
    shp->reg0.sw_lti_enable = p_shp_param->lti_h_enable || p_shp_param->lti_v_enable;
    shp->reg0.sw_cti_enable = p_shp_param->cti_h_enable;
    shp->reg0.sw_peaking_enable = p_shp_param->peaking_enable;
    shp->reg0.sw_peaking_ctrl_enable = p_shp_param->peaking_coring_enable ||
                                       p_shp_param->peaking_gain_enable ||
                                       p_shp_param->peaking_limit_ctrl_enable;
    shp->reg0.sw_edge_proc_enable = p_shp_param->peaking_edge_ctrl_enable;
    shp->reg0.sw_shoot_ctrl_enable = p_shp_param->shootctrl_enable;
    shp->reg0.sw_gain_ctrl_enable = p_shp_param->global_gain_enable;
    shp->reg0.sw_color_adj_enable = p_shp_param->color_ctrl_enable;
    shp->reg0.sw_texture_adj_enable = p_shp_param->tex_adj_enable;
    shp->reg0.sw_coloradj_bypass_en = p_shp_param->sharp_coloradj_bypass_en;
    shp->reg0.sw_ink_enable = 0;
    shp->reg0.sw_sharp_redundent_bypass = 0;

    if ((shp->reg0.sw_lti_enable == 1) && (p_shp_param->lti_h_enable == 0))
        p_shp_param->lti_h_gain = 0;

    if ((shp->reg0.sw_lti_enable == 1) && (p_shp_param->lti_v_enable == 0))
        p_shp_param->lti_v_gain = 0;

    shp->reg162.sw_ltih_radius = p_shp_param->lti_h_radius;
    shp->reg162.sw_ltih_slp1 = p_shp_param->lti_h_slope;
    shp->reg162.sw_ltih_thr1 = p_shp_param->lti_h_thresold;
    shp->reg163.sw_ltih_noisethrneg = p_shp_param->lti_h_noise_thr_neg;
    shp->reg163.sw_ltih_noisethrpos = p_shp_param->lti_h_noise_thr_pos;
    shp->reg163.sw_ltih_tigain = p_shp_param->lti_h_gain;

    shp->reg164.sw_ltiv_radius = p_shp_param->lti_v_radius;
    shp->reg164.sw_ltiv_slp1 = p_shp_param->lti_v_slope;
    shp->reg164.sw_ltiv_thr1 = p_shp_param->lti_v_thresold;
    shp->reg165.sw_ltiv_noisethrneg = p_shp_param->lti_v_noise_thr_neg;
    shp->reg165.sw_ltiv_noisethrpos = p_shp_param->lti_v_noise_thr_pos;
    shp->reg165.sw_ltiv_tigain = p_shp_param->lti_v_gain;

    shp->reg166.sw_ctih_radius = p_shp_param->cti_h_radius;
    shp->reg166.sw_ctih_slp1 = p_shp_param->cti_h_slope;
    shp->reg166.sw_ctih_thr1 = p_shp_param->cti_h_thresold;
    shp->reg167.sw_ctih_noisethrneg = p_shp_param->cti_h_noise_thr_neg;
    shp->reg167.sw_ctih_noisethrpos = p_shp_param->cti_h_noise_thr_pos;
    shp->reg167.sw_ctih_tigain = p_shp_param->cti_h_gain;

    for (ii = 0; ii < 7; ii++) {
        RK_S32 coring_ratio, coring_zero, coring_thr, gain_ctrl_pos, gain_ctrl_neg, limit_ctrl_p0,
               limit_ctrl_p1, limit_ctrl_n0, limit_ctrl_n1, limit_ctrl_ratio;
        RK_S32 ratio_pos_tmp, ratio_neg_tmp, peaking_ctrl_add_tmp;

        if (p_shp_param->peaking_coring_enable == 1) {
            coring_ratio = p_shp_param->peaking_coring_ratio[ii];
            coring_zero = p_shp_param->peaking_coring_zero[ii] >> 2;
            coring_thr = p_shp_param->peaking_coring_thr[ii] >> 2;
        } else {
            coring_ratio = 1024;
            coring_zero = 0;
            coring_thr = 0;
        }
        if (p_shp_param->peaking_gain_enable == 1) {
            gain_ctrl_pos = p_shp_param->peaking_gain_pos[ii];
            gain_ctrl_neg = p_shp_param->peaking_gain_neg[ii];
        } else {
            gain_ctrl_pos = 1024;
            gain_ctrl_neg = 1024;
        }
        if (p_shp_param->peaking_limit_ctrl_enable == 1) {
            limit_ctrl_p0 = p_shp_param->peaking_limit_ctrl_pos0[ii] >> 2;
            limit_ctrl_p1 = p_shp_param->peaking_limit_ctrl_pos1[ii] >> 2;
            limit_ctrl_n0 = p_shp_param->peaking_limit_ctrl_neg0[ii] >> 2;
            limit_ctrl_n1 = p_shp_param->peaking_limit_ctrl_neg1[ii] >> 2;
            limit_ctrl_ratio = p_shp_param->peaking_limit_ctrl_ratio[ii];
        } else {
            limit_ctrl_p0 = 255 >> 2;
            limit_ctrl_p1 = 255 >> 2;
            limit_ctrl_n0 = 255 >> 2;
            limit_ctrl_n1 = 255 >> 2;
            limit_ctrl_ratio = 1024;
        }

        peaking_ctrl_idx_P0[ii] = coring_zero;
        peaking_ctrl_idx_N0[ii] = -coring_zero;
        peaking_ctrl_idx_P1[ii] = coring_thr;
        peaking_ctrl_idx_N1[ii] = -coring_thr;
        peaking_ctrl_idx_P2[ii] = limit_ctrl_p0;
        peaking_ctrl_idx_N2[ii] = -limit_ctrl_n0;
        peaking_ctrl_idx_P3[ii] = limit_ctrl_p1;
        peaking_ctrl_idx_N3[ii] = -limit_ctrl_n1;

        ratio_pos_tmp = (coring_ratio * gain_ctrl_pos + 512) >> 10;
        ratio_neg_tmp = (coring_ratio * gain_ctrl_neg + 512) >> 10;
        peaking_ctrl_value_P1[ii] = ((ratio_pos_tmp * (coring_thr - coring_zero) + 512) >> 10);
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

    shp->reg12.sw_peaking0_idx_n0 = peaking_ctrl_idx_N0[0];
    shp->reg12.sw_peaking0_idx_n1 = peaking_ctrl_idx_N1[0];
    shp->reg13.sw_peaking0_idx_n2 = peaking_ctrl_idx_N2[0];
    shp->reg13.sw_peaking0_idx_n3 = peaking_ctrl_idx_N3[0];
    shp->reg14.sw_peaking0_idx_p0 = peaking_ctrl_idx_P0[0];
    shp->reg14.sw_peaking0_idx_p1 = peaking_ctrl_idx_P1[0];
    shp->reg15.sw_peaking0_idx_p2 = peaking_ctrl_idx_P2[0];
    shp->reg15.sw_peaking0_idx_p3 = peaking_ctrl_idx_P3[0];
    shp->reg16.sw_peaking0_value_n1 = peaking_ctrl_value_N1[0];
    shp->reg16.sw_peaking0_value_n2 = peaking_ctrl_value_N2[0];
    shp->reg17.sw_peaking0_value_n3 = peaking_ctrl_value_N3[0];
    shp->reg17.sw_peaking0_value_p1 = peaking_ctrl_value_P1[0];
    shp->reg18.sw_peaking0_value_p2 = peaking_ctrl_value_P2[0];
    shp->reg18.sw_peaking0_value_p3 = peaking_ctrl_value_P3[0];
    shp->reg19.sw_peaking0_ratio_n01 = peaking_ctrl_ratio_N01[0];
    shp->reg19.sw_peaking0_ratio_n12 = peaking_ctrl_ratio_N12[0];
    shp->reg20.sw_peaking0_ratio_n23 = peaking_ctrl_ratio_N23[0];
    shp->reg20.sw_peaking0_ratio_p01 = peaking_ctrl_ratio_P01[0];
    shp->reg21.sw_peaking0_ratio_p12 = peaking_ctrl_ratio_P12[0];
    shp->reg21.sw_peaking0_ratio_p23 = peaking_ctrl_ratio_P23[0];

    shp->reg23.sw_peaking1_idx_n0 = peaking_ctrl_idx_N0[1];
    shp->reg23.sw_peaking1_idx_n1 = peaking_ctrl_idx_N1[1];
    shp->reg24.sw_peaking1_idx_n2 = peaking_ctrl_idx_N2[1];
    shp->reg24.sw_peaking1_idx_n3 = peaking_ctrl_idx_N3[1];
    shp->reg25.sw_peaking1_idx_p0 = peaking_ctrl_idx_P0[1];
    shp->reg25.sw_peaking1_idx_p1 = peaking_ctrl_idx_P1[1];
    shp->reg26.sw_peaking1_idx_p2 = peaking_ctrl_idx_P2[1];
    shp->reg26.sw_peaking1_idx_p3 = peaking_ctrl_idx_P3[1];
    shp->reg27.sw_peaking1_value_n1 = peaking_ctrl_value_N1[1];
    shp->reg27.sw_peaking1_value_n2 = peaking_ctrl_value_N2[1];
    shp->reg28.sw_peaking1_value_n3 = peaking_ctrl_value_N3[1];
    shp->reg28.sw_peaking1_value_p1 = peaking_ctrl_value_P1[1];
    shp->reg29.sw_peaking1_value_p2 = peaking_ctrl_value_P2[1];
    shp->reg29.sw_peaking1_value_p3 = peaking_ctrl_value_P3[1];
    shp->reg30.sw_peaking1_ratio_n01 = peaking_ctrl_ratio_N01[1];
    shp->reg30.sw_peaking1_ratio_n12 = peaking_ctrl_ratio_N12[1];
    shp->reg31.sw_peaking1_ratio_n23 = peaking_ctrl_ratio_N23[1];
    shp->reg31.sw_peaking1_ratio_p01 = peaking_ctrl_ratio_P01[1];
    shp->reg32.sw_peaking1_ratio_p12 = peaking_ctrl_ratio_P12[1];
    shp->reg32.sw_peaking1_ratio_p23 = peaking_ctrl_ratio_P23[1];

    shp->reg34.sw_peaking2_idx_n0 = peaking_ctrl_idx_N0[2];
    shp->reg34.sw_peaking2_idx_n1 = peaking_ctrl_idx_N1[2];
    shp->reg35.sw_peaking2_idx_n2 = peaking_ctrl_idx_N2[2];
    shp->reg35.sw_peaking2_idx_n3 = peaking_ctrl_idx_N3[2];
    shp->reg36.sw_peaking2_idx_p0 = peaking_ctrl_idx_P0[2];
    shp->reg36.sw_peaking2_idx_p1 = peaking_ctrl_idx_P1[2];
    shp->reg37.sw_peaking2_idx_p2 = peaking_ctrl_idx_P2[2];
    shp->reg37.sw_peaking2_idx_p3 = peaking_ctrl_idx_P3[2];
    shp->reg38.sw_peaking2_value_n1 = peaking_ctrl_value_N1[2];
    shp->reg38.sw_peaking2_value_n2 = peaking_ctrl_value_N2[2];
    shp->reg39.sw_peaking2_value_n3 = peaking_ctrl_value_N3[2];
    shp->reg39.sw_peaking2_value_p1 = peaking_ctrl_value_P1[2];
    shp->reg40.sw_peaking2_value_p2 = peaking_ctrl_value_P2[2];
    shp->reg40.sw_peaking2_value_p3 = peaking_ctrl_value_P3[2];
    shp->reg41.sw_peaking2_ratio_n01 = peaking_ctrl_ratio_N01[2];
    shp->reg41.sw_peaking2_ratio_n12 = peaking_ctrl_ratio_N12[2];
    shp->reg42.sw_peaking2_ratio_n23 = peaking_ctrl_ratio_N23[2];
    shp->reg42.sw_peaking2_ratio_p01 = peaking_ctrl_ratio_P01[2];
    shp->reg43.sw_peaking2_ratio_p12 = peaking_ctrl_ratio_P12[2];
    shp->reg43.sw_peaking2_ratio_p23 = peaking_ctrl_ratio_P23[2];

    shp->reg45.sw_peaking3_idx_n0 = peaking_ctrl_idx_N0[3];
    shp->reg45.sw_peaking3_idx_n1 = peaking_ctrl_idx_N1[3];
    shp->reg46.sw_peaking3_idx_n2 = peaking_ctrl_idx_N2[3];
    shp->reg46.sw_peaking3_idx_n3 = peaking_ctrl_idx_N3[3];
    shp->reg47.sw_peaking3_idx_p0 = peaking_ctrl_idx_P0[3];
    shp->reg47.sw_peaking3_idx_p1 = peaking_ctrl_idx_P1[3];
    shp->reg48.sw_peaking3_idx_p2 = peaking_ctrl_idx_P2[3];
    shp->reg48.sw_peaking3_idx_p3 = peaking_ctrl_idx_P3[3];
    shp->reg49.sw_peaking3_value_n1 = peaking_ctrl_value_N1[3];
    shp->reg49.sw_peaking3_value_n2 = peaking_ctrl_value_N2[3];
    shp->reg50.sw_peaking3_value_n3 = peaking_ctrl_value_N3[3];
    shp->reg50.sw_peaking3_value_p1 = peaking_ctrl_value_P1[3];
    shp->reg51.sw_peaking3_value_p2 = peaking_ctrl_value_P2[3];
    shp->reg51.sw_peaking3_value_p3 = peaking_ctrl_value_P3[3];
    shp->reg52.sw_peaking3_ratio_n01 = peaking_ctrl_ratio_N01[3];
    shp->reg52.sw_peaking3_ratio_n12 = peaking_ctrl_ratio_N12[3];
    shp->reg53.sw_peaking3_ratio_n23 = peaking_ctrl_ratio_N23[3];
    shp->reg53.sw_peaking3_ratio_p01 = peaking_ctrl_ratio_P01[3];
    shp->reg54.sw_peaking3_ratio_p12 = peaking_ctrl_ratio_P12[3];
    shp->reg54.sw_peaking3_ratio_p23 = peaking_ctrl_ratio_P23[3];

    shp->reg56.sw_peaking4_idx_n0 = peaking_ctrl_idx_N0[4];
    shp->reg56.sw_peaking4_idx_n1 = peaking_ctrl_idx_N1[4];
    shp->reg57.sw_peaking4_idx_n2 = peaking_ctrl_idx_N2[4];
    shp->reg57.sw_peaking4_idx_n3 = peaking_ctrl_idx_N3[4];
    shp->reg58.sw_peaking4_idx_p0 = peaking_ctrl_idx_P0[4];
    shp->reg58.sw_peaking4_idx_p1 = peaking_ctrl_idx_P1[4];
    shp->reg59.sw_peaking4_idx_p2 = peaking_ctrl_idx_P2[4];
    shp->reg59.sw_peaking4_idx_p3 = peaking_ctrl_idx_P3[4];
    shp->reg60.sw_peaking4_value_n1 = peaking_ctrl_value_N1[4];
    shp->reg60.sw_peaking4_value_n2 = peaking_ctrl_value_N2[4];
    shp->reg61.sw_peaking4_value_n3 = peaking_ctrl_value_N3[4];
    shp->reg61.sw_peaking4_value_p1 = peaking_ctrl_value_P1[4];
    shp->reg62.sw_peaking4_value_p2 = peaking_ctrl_value_P2[4];
    shp->reg62.sw_peaking4_value_p3 = peaking_ctrl_value_P3[4];
    shp->reg63.sw_peaking4_ratio_n01 = peaking_ctrl_ratio_N01[4];
    shp->reg63.sw_peaking4_ratio_n12 = peaking_ctrl_ratio_N12[4];
    shp->reg64.sw_peaking4_ratio_n23 = peaking_ctrl_ratio_N23[4];
    shp->reg64.sw_peaking4_ratio_p01 = peaking_ctrl_ratio_P01[4];
    shp->reg65.sw_peaking4_ratio_p12 = peaking_ctrl_ratio_P12[4];
    shp->reg65.sw_peaking4_ratio_p23 = peaking_ctrl_ratio_P23[4];

    shp->reg67.sw_peaking5_idx_n0 = peaking_ctrl_idx_N0[5];
    shp->reg67.sw_peaking5_idx_n1 = peaking_ctrl_idx_N1[5];
    shp->reg68.sw_peaking5_idx_n2 = peaking_ctrl_idx_N2[5];
    shp->reg68.sw_peaking5_idx_n3 = peaking_ctrl_idx_N3[5];
    shp->reg69.sw_peaking5_idx_p0 = peaking_ctrl_idx_P0[5];
    shp->reg69.sw_peaking5_idx_p1 = peaking_ctrl_idx_P1[5];
    shp->reg70.sw_peaking5_idx_p2 = peaking_ctrl_idx_P2[5];
    shp->reg70.sw_peaking5_idx_p3 = peaking_ctrl_idx_P3[5];
    shp->reg71.sw_peaking5_value_n1 = peaking_ctrl_value_N1[5];
    shp->reg71.sw_peaking5_value_n2 = peaking_ctrl_value_N2[5];
    shp->reg72.sw_peaking5_value_n3 = peaking_ctrl_value_N3[5];
    shp->reg72.sw_peaking5_value_p1 = peaking_ctrl_value_P1[5];
    shp->reg73.sw_peaking5_value_p2 = peaking_ctrl_value_P2[5];
    shp->reg73.sw_peaking5_value_p3 = peaking_ctrl_value_P3[5];
    shp->reg74.sw_peaking5_ratio_n01 = peaking_ctrl_ratio_N01[5];
    shp->reg74.sw_peaking5_ratio_n12 = peaking_ctrl_ratio_N12[5];
    shp->reg75.sw_peaking5_ratio_n23 = peaking_ctrl_ratio_N23[5];
    shp->reg75.sw_peaking5_ratio_p01 = peaking_ctrl_ratio_P01[5];
    shp->reg76.sw_peaking5_ratio_p12 = peaking_ctrl_ratio_P12[5];
    shp->reg76.sw_peaking5_ratio_p23 = peaking_ctrl_ratio_P23[5];

    shp->reg78.sw_peaking6_idx_n0 = peaking_ctrl_idx_N0[6];
    shp->reg78.sw_peaking6_idx_n1 = peaking_ctrl_idx_N1[6];
    shp->reg79.sw_peaking6_idx_n2 = peaking_ctrl_idx_N2[6];
    shp->reg79.sw_peaking6_idx_n3 = peaking_ctrl_idx_N3[6];
    shp->reg80.sw_peaking6_idx_p0 = peaking_ctrl_idx_P0[6];
    shp->reg80.sw_peaking6_idx_p1 = peaking_ctrl_idx_P1[6];
    shp->reg81.sw_peaking6_idx_p2 = peaking_ctrl_idx_P2[6];
    shp->reg81.sw_peaking6_idx_p3 = peaking_ctrl_idx_P3[6];
    shp->reg82.sw_peaking6_value_n1 = peaking_ctrl_value_N1[6];
    shp->reg82.sw_peaking6_value_n2 = peaking_ctrl_value_N2[6];
    shp->reg83.sw_peaking6_value_n3 = peaking_ctrl_value_N3[6];
    shp->reg83.sw_peaking6_value_p1 = peaking_ctrl_value_P1[6];
    shp->reg84.sw_peaking6_value_p2 = peaking_ctrl_value_P2[6];
    shp->reg84.sw_peaking6_value_p3 = peaking_ctrl_value_P3[6];
    shp->reg85.sw_peaking6_ratio_n01 = peaking_ctrl_ratio_N01[6];
    shp->reg85.sw_peaking6_ratio_n12 = peaking_ctrl_ratio_N12[6];
    shp->reg86.sw_peaking6_ratio_n23 = peaking_ctrl_ratio_N23[6];
    shp->reg86.sw_peaking6_ratio_p01 = peaking_ctrl_ratio_P01[6];
    shp->reg87.sw_peaking6_ratio_p12 = peaking_ctrl_ratio_P12[6];
    shp->reg87.sw_peaking6_ratio_p23 = peaking_ctrl_ratio_P23[6];

    shp->reg2.sw_peaking_v00 = p_shp_param->peaking_filt_core_V0[0];
    shp->reg2.sw_peaking_v01 = p_shp_param->peaking_filt_core_V0[1];
    shp->reg2.sw_peaking_v02 = p_shp_param->peaking_filt_core_V0[2];
    shp->reg2.sw_peaking_v10 = p_shp_param->peaking_filt_core_V1[0];
    shp->reg2.sw_peaking_v11 = p_shp_param->peaking_filt_core_V1[1];
    shp->reg2.sw_peaking_v12 = p_shp_param->peaking_filt_core_V1[2];
    shp->reg3.sw_peaking_v20 = p_shp_param->peaking_filt_core_V2[0];
    shp->reg3.sw_peaking_v21 = p_shp_param->peaking_filt_core_V2[1];
    shp->reg3.sw_peaking_v22 = p_shp_param->peaking_filt_core_V2[2];
    shp->reg3.sw_peaking_usm0 = p_shp_param->peaking_filt_core_USM[0];
    shp->reg3.sw_peaking_usm1 = p_shp_param->peaking_filt_core_USM[1];
    shp->reg3.sw_peaking_usm2 = p_shp_param->peaking_filt_core_USM[2];
    shp->reg3.sw_diag_coef = p_shp_param->peaking_filter_cfg_diag_enh_coef;
    shp->reg4.sw_peaking_h00 = p_shp_param->peaking_filt_core_H0[0];
    shp->reg4.sw_peaking_h01 = p_shp_param->peaking_filt_core_H0[1];
    shp->reg4.sw_peaking_h02 = p_shp_param->peaking_filt_core_H0[2];
    shp->reg6.sw_peaking_h10 = p_shp_param->peaking_filt_core_H1[0];
    shp->reg6.sw_peaking_h11 = p_shp_param->peaking_filt_core_H1[1];
    shp->reg6.sw_peaking_h12 = p_shp_param->peaking_filt_core_H1[2];
    shp->reg8.sw_peaking_h20 = p_shp_param->peaking_filt_core_H2[0];
    shp->reg8.sw_peaking_h21 = p_shp_param->peaking_filt_core_H2[1];
    shp->reg8.sw_peaking_h22 = p_shp_param->peaking_filt_core_H2[2];

    shp->reg100.sw_peaking_gain = p_shp_param->peaking_gain;
    shp->reg100.sw_nondir_thr = p_shp_param->peaking_edge_ctrl_non_dir_thr;
    shp->reg100.sw_dir_cmp_ratio = p_shp_param->peaking_edge_ctrl_dir_cmp_ratio;
    shp->reg100.sw_nondir_wgt_ratio = p_shp_param->peaking_edge_ctrl_non_dir_wgt_ratio;
    shp->reg101.sw_nondir_wgt_offset = p_shp_param->peaking_edge_ctrl_non_dir_wgt_offset;
    shp->reg101.sw_dir_cnt_thr = p_shp_param->peaking_edge_ctrl_dir_cnt_thr;
    shp->reg101.sw_dir_cnt_avg = p_shp_param->peaking_edge_ctrl_dir_cnt_avg;
    shp->reg101.sw_dir_cnt_offset = p_shp_param->peaking_edge_ctrl_dir_cnt_offset;
    shp->reg101.sw_diag_dir_thr = p_shp_param->peaking_edge_ctrl_diag_dir_thr;
    shp->reg102.sw_diag_adjgain_tab0 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[0];
    shp->reg102.sw_diag_adjgain_tab1 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[1];
    shp->reg102.sw_diag_adjgain_tab2 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[2];
    shp->reg102.sw_diag_adjgain_tab3 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[3];
    shp->reg102.sw_diag_adjgain_tab4 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[4];
    shp->reg102.sw_diag_adjgain_tab5 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[5];
    shp->reg102.sw_diag_adjgain_tab6 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[6];
    shp->reg102.sw_diag_adjgain_tab7 = p_shp_param->peaking_edge_ctrl_diag_adj_gain_tab[7];

    shp->reg103.sw_edge_alpha_over_non = p_shp_param->peaking_estc_alpha_over_non;
    shp->reg103.sw_edge_alpha_under_non = p_shp_param->peaking_estc_alpha_under_non;
    shp->reg103.sw_edge_alpha_over_unlimit_non = p_shp_param->peaking_estc_alpha_over_unlimit_non;
    shp->reg103.sw_edge_alpha_under_unlimit_non = p_shp_param->peaking_estc_alpha_under_unlimit_non;

    shp->reg104.sw_edge_alpha_over_v = p_shp_param->peaking_estc_alpha_over_v;
    shp->reg104.sw_edge_alpha_under_v = p_shp_param->peaking_estc_alpha_under_v;
    shp->reg104.sw_edge_alpha_over_unlimit_v = p_shp_param->peaking_estc_alpha_over_unlimit_v;
    shp->reg104.sw_edge_alpha_under_unlimit_v = p_shp_param->peaking_estc_alpha_under_unlimit_v;

    shp->reg105.sw_edge_alpha_over_h = p_shp_param->peaking_estc_alpha_over_h;
    shp->reg105.sw_edge_alpha_under_h = p_shp_param->peaking_estc_alpha_under_h;
    shp->reg105.sw_edge_alpha_over_unlimit_h = p_shp_param->peaking_estc_alpha_over_unlimit_h;
    shp->reg105.sw_edge_alpha_under_unlimit_h = p_shp_param->peaking_estc_alpha_under_unlimit_h;

    shp->reg106.sw_edge_alpha_over_d0 = p_shp_param->peaking_estc_alpha_over_d0;
    shp->reg106.sw_edge_alpha_under_d0 = p_shp_param->peaking_estc_alpha_under_d0;
    shp->reg106.sw_edge_alpha_over_unlimit_d0 = p_shp_param->peaking_estc_alpha_over_unlimit_d0;
    shp->reg106.sw_edge_alpha_under_unlimit_d0 = p_shp_param->peaking_estc_alpha_under_unlimit_d0;

    shp->reg107.sw_edge_alpha_over_d1 = p_shp_param->peaking_estc_alpha_over_d1;
    shp->reg107.sw_edge_alpha_under_d1 = p_shp_param->peaking_estc_alpha_under_d1;
    shp->reg107.sw_edge_alpha_over_unlimit_d1 = p_shp_param->peaking_estc_alpha_over_unlimit_d1;
    shp->reg107.sw_edge_alpha_under_unlimit_d1 = p_shp_param->peaking_estc_alpha_under_unlimit_d1;

    shp->reg108.sw_edge_delta_offset_non = p_shp_param->peaking_estc_delta_offset_non;
    shp->reg108.sw_edge_delta_offset_v = p_shp_param->peaking_estc_delta_offset_v;
    shp->reg108.sw_edge_delta_offset_h = p_shp_param->peaking_estc_delta_offset_h;
    shp->reg109.sw_edge_delta_offset_d0 = p_shp_param->peaking_estc_delta_offset_d0;
    shp->reg109.sw_edge_delta_offset_d1 = p_shp_param->peaking_estc_delta_offset_d1;

    shp->reg110.sw_shoot_filt_radius = p_shp_param->shootctrl_filter_radius;
    shp->reg110.sw_shoot_delta_offset = p_shp_param->shootctrl_delta_offset;
    shp->reg110.sw_shoot_alpha_over = p_shp_param->shootctrl_alpha_over;
    shp->reg110.sw_shoot_alpha_under = p_shp_param->shootctrl_alpha_under;
    shp->reg111.sw_shoot_alpha_over_unlimit = p_shp_param->shootctrl_alpha_over_unlimit;
    shp->reg111.sw_shoot_alpha_under_unlimit = p_shp_param->shootctrl_alpha_under_unlimit;

    //dst_reg->sharp.reg112.sw_adp_idx0 = p_shp_param->global_gain_adp_grd
    shp->reg112.sw_adp_idx0 = p_shp_param->global_gain_adp_grd[1] >> 2;
    shp->reg112.sw_adp_idx1 = p_shp_param->global_gain_adp_grd[2] >> 2;
    shp->reg112.sw_adp_idx2 = p_shp_param->global_gain_adp_grd[3] >> 2;
    shp->reg113.sw_adp_idx3 = p_shp_param->global_gain_adp_grd[4] >> 2;
    shp->reg113.sw_adp_gain0 = p_shp_param->global_gain_adp_val[0];
    shp->reg113.sw_adp_gain1 = p_shp_param->global_gain_adp_val[1];
    shp->reg114.sw_adp_gain2 = p_shp_param->global_gain_adp_val[2];
    shp->reg114.sw_adp_gain3 = p_shp_param->global_gain_adp_val[3];
    shp->reg114.sw_adp_gain4 = p_shp_param->global_gain_adp_val[4];
    global_gain_slp_temp[0] = ROUND(128 * (float)(shp->reg113.sw_adp_gain1 - shp->reg113.sw_adp_gain0) /
                                    MPP_MAX((float)(shp->reg112.sw_adp_idx0 - 0), 1));
    global_gain_slp_temp[1] = ROUND(128 * (float)(shp->reg114.sw_adp_gain2 - shp->reg113.sw_adp_gain1) /
                                    MPP_MAX((float)(shp->reg112.sw_adp_idx1 - shp->reg112.sw_adp_idx0), 1));
    global_gain_slp_temp[2] = ROUND(128 * (float)(shp->reg114.sw_adp_gain3 - shp->reg114.sw_adp_gain2) /
                                    MPP_MAX((float)(shp->reg112.sw_adp_idx2 - shp->reg112.sw_adp_idx1), 1));
    global_gain_slp_temp[3] = ROUND(128 * (float)(shp->reg114.sw_adp_gain4 - shp->reg114.sw_adp_gain3) /
                                    MPP_MAX((float)(shp->reg113.sw_adp_idx3 - shp->reg112.sw_adp_idx2), 1));
    global_gain_slp_temp[4] = ROUND(128 * (float)(p_shp_param->global_gain_adp_val[5] - shp->reg114.sw_adp_gain4) /
                                    MPP_MAX((float)(255 - shp->reg113.sw_adp_idx3), 1));
    shp->reg115.sw_adp_slp01 = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    shp->reg115.sw_adp_slp12 = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    shp->reg128.sw_adp_slp23 = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    shp->reg128.sw_adp_slp34 = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    shp->reg129.sw_adp_slp45 = mpp_clip(global_gain_slp_temp[4], -1024, 1023);

    shp->reg129.sw_var_idx0 = p_shp_param->global_gain_var_grd[1] >> 2;
    shp->reg129.sw_var_idx1 = p_shp_param->global_gain_var_grd[2] >> 2;
    shp->reg130.sw_var_idx2 = p_shp_param->global_gain_var_grd[3] >> 2;
    shp->reg130.sw_var_idx3 = p_shp_param->global_gain_var_grd[4] >> 2;
    shp->reg130.sw_var_gain0 = p_shp_param->global_gain_var_val[0];
    shp->reg131.sw_var_gain1 = p_shp_param->global_gain_var_val[1];
    shp->reg131.sw_var_gain2 = p_shp_param->global_gain_var_val[2];
    shp->reg131.sw_var_gain3 = p_shp_param->global_gain_var_val[3];
    shp->reg131.sw_var_gain4 = p_shp_param->global_gain_var_val[4];
    global_gain_slp_temp[0] = ROUND(128 * (float)(shp->reg131.sw_var_gain1 - shp->reg130.sw_var_gain0) /
                                    MPP_MAX((float)(shp->reg129.sw_var_idx0 - 0), 1));
    global_gain_slp_temp[1] = ROUND(128 * (float)(shp->reg131.sw_var_gain2 - shp->reg131.sw_var_gain1) /
                                    MPP_MAX((float)(shp->reg129.sw_var_idx1 - shp->reg129.sw_var_idx0), 1));
    global_gain_slp_temp[2] = ROUND(128 * (float)(shp->reg131.sw_var_gain3 - shp->reg131.sw_var_gain2) /
                                    MPP_MAX((float)(shp->reg130.sw_var_idx2 - shp->reg129.sw_var_idx1), 1));
    global_gain_slp_temp[3] = ROUND(128 * (float)(shp->reg131.sw_var_gain4 - shp->reg131.sw_var_gain3) /
                                    MPP_MAX((float)(shp->reg130.sw_var_idx3 - shp->reg130.sw_var_idx2), 1));
    global_gain_slp_temp[4] = ROUND(128 * (float)(p_shp_param->global_gain_var_val[5] - shp->reg131.sw_var_gain4) /
                                    MPP_MAX((float)(255 - shp->reg130.sw_var_idx3), 1));
    shp->reg132.sw_var_slp01 = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    shp->reg132.sw_var_slp12 = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    shp->reg133.sw_var_slp23 = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    shp->reg133.sw_var_slp34 = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    shp->reg134.sw_var_slp45 = mpp_clip(global_gain_slp_temp[4], -1024, 1023);

    shp->reg134.sw_lum_select = p_shp_param->global_gain_lum_mode;
    shp->reg134.sw_lum_idx0 = p_shp_param->global_gain_lum_grd[1] >> 2;
    shp->reg135.sw_lum_idx1 = p_shp_param->global_gain_lum_grd[2] >> 2;
    shp->reg135.sw_lum_idx2 = p_shp_param->global_gain_lum_grd[3] >> 2;
    shp->reg135.sw_lum_idx3 = p_shp_param->global_gain_lum_grd[4] >> 2;
    shp->reg136.sw_lum_gain0 = p_shp_param->global_gain_lum_val[0];
    shp->reg136.sw_lum_gain1 = p_shp_param->global_gain_lum_val[1];
    shp->reg136.sw_lum_gain2 = p_shp_param->global_gain_lum_val[2];
    shp->reg136.sw_lum_gain3 = p_shp_param->global_gain_lum_val[3];
    shp->reg137.sw_lum_gain4 = p_shp_param->global_gain_lum_val[4];
    global_gain_slp_temp[0] = ROUND(128 * (float)(shp->reg136.sw_lum_gain1 - shp->reg136.sw_lum_gain0) /
                                    MPP_MAX((float)(shp->reg134.sw_lum_idx0 - 0), 1));
    global_gain_slp_temp[1] = ROUND(128 * (float)(shp->reg136.sw_lum_gain2 - shp->reg136.sw_lum_gain1) /
                                    MPP_MAX((float)(shp->reg135.sw_lum_idx1 - shp->reg134.sw_lum_idx0), 1));
    global_gain_slp_temp[2] = ROUND(128 * (float)(shp->reg136.sw_lum_gain3 - shp->reg136.sw_lum_gain2) /
                                    MPP_MAX((float)(shp->reg135.sw_lum_idx2 - shp->reg135.sw_lum_idx1), 1));
    global_gain_slp_temp[3] = ROUND(128 * (float)(shp->reg137.sw_lum_gain4 - shp->reg136.sw_lum_gain3) /
                                    MPP_MAX((float)(shp->reg135.sw_lum_idx3 - shp->reg135.sw_lum_idx2), 1));
    global_gain_slp_temp[4] = ROUND(128 * (float)(p_shp_param->global_gain_lum_val[5] - shp->reg137.sw_lum_gain4) /
                                    MPP_MAX((float)(255 - shp->reg135.sw_lum_idx3), 1));
    shp->reg137.sw_lum_slp01 = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    shp->reg137.sw_lum_slp12 = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    shp->reg138.sw_lum_slp23 = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    shp->reg138.sw_lum_slp34 = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    shp->reg139.sw_lum_slp45 = mpp_clip(global_gain_slp_temp[4], -1024, 1023);


    shp->reg140.sw_adj_point_x0 = p_shp_param->color_ctrl_p0_point_u;
    shp->reg140.sw_adj_point_y0 = p_shp_param->color_ctrl_p0_point_v;
    shp->reg140.sw_adj_scaling_coef0 = p_shp_param->color_ctrl_p0_scaling_coef;
    shp->reg141.sw_coloradj_tab0_0 = p_shp_param->color_ctrl_p0_roll_tab[0];
    shp->reg141.sw_coloradj_tab0_1 = p_shp_param->color_ctrl_p0_roll_tab[1];
    shp->reg141.sw_coloradj_tab0_2 = p_shp_param->color_ctrl_p0_roll_tab[2];
    shp->reg141.sw_coloradj_tab0_3 = p_shp_param->color_ctrl_p0_roll_tab[3];
    shp->reg141.sw_coloradj_tab0_4 = p_shp_param->color_ctrl_p0_roll_tab[4];
    shp->reg141.sw_coloradj_tab0_5 = p_shp_param->color_ctrl_p0_roll_tab[5];
    shp->reg142.sw_coloradj_tab0_6 = p_shp_param->color_ctrl_p0_roll_tab[6];
    shp->reg142.sw_coloradj_tab0_7 = p_shp_param->color_ctrl_p0_roll_tab[7];
    shp->reg142.sw_coloradj_tab0_8 = p_shp_param->color_ctrl_p0_roll_tab[8];
    shp->reg142.sw_coloradj_tab0_9 = p_shp_param->color_ctrl_p0_roll_tab[9];
    shp->reg142.sw_coloradj_tab0_10 = p_shp_param->color_ctrl_p0_roll_tab[10];
    shp->reg142.sw_coloradj_tab0_11 = p_shp_param->color_ctrl_p0_roll_tab[11];
    shp->reg143.sw_coloradj_tab0_12 = p_shp_param->color_ctrl_p0_roll_tab[12];
    shp->reg143.sw_coloradj_tab0_13 = p_shp_param->color_ctrl_p0_roll_tab[13];
    shp->reg143.sw_coloradj_tab0_14 = p_shp_param->color_ctrl_p0_roll_tab[14];
    shp->reg143.sw_coloradj_tab0_15 = p_shp_param->color_ctrl_p0_roll_tab[15];

    shp->reg144.sw_adj_point_x1 = p_shp_param->color_ctrl_p1_point_u;
    shp->reg144.sw_adj_point_y1 = p_shp_param->color_ctrl_p1_point_v;
    shp->reg144.sw_adj_scaling_coef1 = p_shp_param->color_ctrl_p1_scaling_coef;
    shp->reg145.sw_coloradj_tab1_0 = p_shp_param->color_ctrl_p1_roll_tab[0];
    shp->reg145.sw_coloradj_tab1_1 = p_shp_param->color_ctrl_p1_roll_tab[1];
    shp->reg145.sw_coloradj_tab1_2 = p_shp_param->color_ctrl_p1_roll_tab[2];
    shp->reg145.sw_coloradj_tab1_3 = p_shp_param->color_ctrl_p1_roll_tab[3];
    shp->reg145.sw_coloradj_tab1_4 = p_shp_param->color_ctrl_p1_roll_tab[4];
    shp->reg145.sw_coloradj_tab1_5 = p_shp_param->color_ctrl_p1_roll_tab[5];
    shp->reg146.sw_coloradj_tab1_6 = p_shp_param->color_ctrl_p1_roll_tab[6];
    shp->reg146.sw_coloradj_tab1_7 = p_shp_param->color_ctrl_p1_roll_tab[7];
    shp->reg146.sw_coloradj_tab1_8 = p_shp_param->color_ctrl_p1_roll_tab[8];
    shp->reg146.sw_coloradj_tab1_9 = p_shp_param->color_ctrl_p1_roll_tab[9];
    shp->reg146.sw_coloradj_tab1_10 = p_shp_param->color_ctrl_p1_roll_tab[10];
    shp->reg146.sw_coloradj_tab1_11 = p_shp_param->color_ctrl_p1_roll_tab[11];
    shp->reg147.sw_coloradj_tab1_12 = p_shp_param->color_ctrl_p1_roll_tab[12];
    shp->reg147.sw_coloradj_tab1_13 = p_shp_param->color_ctrl_p1_roll_tab[13];
    shp->reg147.sw_coloradj_tab1_14 = p_shp_param->color_ctrl_p1_roll_tab[14];
    shp->reg147.sw_coloradj_tab1_15 = p_shp_param->color_ctrl_p1_roll_tab[15];

    shp->reg148.sw_adj_point_x2 = p_shp_param->color_ctrl_p2_point_u;
    shp->reg148.sw_adj_point_y2 = p_shp_param->color_ctrl_p2_point_v;
    shp->reg148.sw_adj_scaling_coef2 = p_shp_param->color_ctrl_p2_scaling_coef;
    shp->reg149.sw_coloradj_tab2_0 = p_shp_param->color_ctrl_p2_roll_tab[0];
    shp->reg149.sw_coloradj_tab2_1 = p_shp_param->color_ctrl_p2_roll_tab[1];
    shp->reg149.sw_coloradj_tab2_2 = p_shp_param->color_ctrl_p2_roll_tab[2];
    shp->reg149.sw_coloradj_tab2_3 = p_shp_param->color_ctrl_p2_roll_tab[3];
    shp->reg149.sw_coloradj_tab2_4 = p_shp_param->color_ctrl_p2_roll_tab[4];
    shp->reg149.sw_coloradj_tab2_5 = p_shp_param->color_ctrl_p2_roll_tab[5];
    shp->reg150.sw_coloradj_tab2_6 = p_shp_param->color_ctrl_p2_roll_tab[6];
    shp->reg150.sw_coloradj_tab2_7 = p_shp_param->color_ctrl_p2_roll_tab[7];
    shp->reg150.sw_coloradj_tab2_8 = p_shp_param->color_ctrl_p2_roll_tab[8];
    shp->reg150.sw_coloradj_tab2_9 = p_shp_param->color_ctrl_p2_roll_tab[9];
    shp->reg150.sw_coloradj_tab2_10 = p_shp_param->color_ctrl_p2_roll_tab[10];
    shp->reg150.sw_coloradj_tab2_11 = p_shp_param->color_ctrl_p2_roll_tab[11];
    shp->reg151.sw_coloradj_tab2_12 = p_shp_param->color_ctrl_p2_roll_tab[12];
    shp->reg151.sw_coloradj_tab2_13 = p_shp_param->color_ctrl_p2_roll_tab[13];
    shp->reg151.sw_coloradj_tab2_14 = p_shp_param->color_ctrl_p2_roll_tab[14];
    shp->reg151.sw_coloradj_tab2_15 = p_shp_param->color_ctrl_p2_roll_tab[15];

    shp->reg152.sw_adj_point_x3 = p_shp_param->color_ctrl_p3_point_u;
    shp->reg152.sw_adj_point_y3 = p_shp_param->color_ctrl_p3_point_v;
    shp->reg152.sw_adj_scaling_coef3 = p_shp_param->color_ctrl_p3_scaling_coef;
    shp->reg153.sw_coloradj_tab3_0 = p_shp_param->color_ctrl_p3_roll_tab[0];
    shp->reg153.sw_coloradj_tab3_1 = p_shp_param->color_ctrl_p3_roll_tab[1];
    shp->reg153.sw_coloradj_tab3_2 = p_shp_param->color_ctrl_p3_roll_tab[2];
    shp->reg153.sw_coloradj_tab3_3 = p_shp_param->color_ctrl_p3_roll_tab[3];
    shp->reg153.sw_coloradj_tab3_4 = p_shp_param->color_ctrl_p3_roll_tab[4];
    shp->reg153.sw_coloradj_tab3_5 = p_shp_param->color_ctrl_p3_roll_tab[5];
    shp->reg154.sw_coloradj_tab3_6 = p_shp_param->color_ctrl_p3_roll_tab[6];
    shp->reg154.sw_coloradj_tab3_7 = p_shp_param->color_ctrl_p3_roll_tab[7];
    shp->reg154.sw_coloradj_tab3_8 = p_shp_param->color_ctrl_p3_roll_tab[8];
    shp->reg154.sw_coloradj_tab3_9 = p_shp_param->color_ctrl_p3_roll_tab[9];
    shp->reg154.sw_coloradj_tab3_10 = p_shp_param->color_ctrl_p3_roll_tab[10];
    shp->reg154.sw_coloradj_tab3_11 = p_shp_param->color_ctrl_p3_roll_tab[11];
    shp->reg155.sw_coloradj_tab3_12 = p_shp_param->color_ctrl_p3_roll_tab[12];
    shp->reg155.sw_coloradj_tab3_13 = p_shp_param->color_ctrl_p3_roll_tab[13];
    shp->reg155.sw_coloradj_tab3_14 = p_shp_param->color_ctrl_p3_roll_tab[14];
    shp->reg155.sw_coloradj_tab3_15 = p_shp_param->color_ctrl_p3_roll_tab[15];


    shp->reg156.sw_idxmode_select = p_shp_param->tex_adj_mode_select;
    shp->reg156.sw_ymode_select = p_shp_param->tex_adj_y_mode_select;
    shp->reg156.sw_tex_idx0 = p_shp_param->tex_adj_grd[1] >> 2;
    shp->reg156.sw_tex_idx1 = p_shp_param->tex_adj_grd[2] >> 2;
    shp->reg157.sw_tex_idx2 = p_shp_param->tex_adj_grd[3] >> 2;
    shp->reg157.sw_tex_idx3 = p_shp_param->tex_adj_grd[4] >> 2;
    shp->reg157.sw_tex_gain0 = p_shp_param->tex_adj_val[0];
    shp->reg158.sw_tex_gain1 = p_shp_param->tex_adj_val[1];
    shp->reg158.sw_tex_gain2 = p_shp_param->tex_adj_val[2];
    shp->reg158.sw_tex_gain3 = p_shp_param->tex_adj_val[3];
    shp->reg158.sw_tex_gain4 = p_shp_param->tex_adj_val[4];
    global_gain_slp_temp[0] = ROUND(128 * (float)(shp->reg158.sw_tex_gain1 - shp->reg157.sw_tex_gain0) /
                                    MPP_MAX((float)(shp->reg156.sw_tex_idx0 - 0), 1));
    global_gain_slp_temp[1] = ROUND(128 * (float)(shp->reg158.sw_tex_gain2 - shp->reg158.sw_tex_gain1) /
                                    MPP_MAX((float)(shp->reg156.sw_tex_idx1 - shp->reg156.sw_tex_idx0), 1));
    global_gain_slp_temp[2] = ROUND(128 * (float)(shp->reg158.sw_tex_gain3 - shp->reg158.sw_tex_gain2) /
                                    MPP_MAX((float)(shp->reg157.sw_tex_idx2 - shp->reg156.sw_tex_idx1), 1));
    global_gain_slp_temp[3] = ROUND(128 * (float)(shp->reg158.sw_tex_gain4 - shp->reg158.sw_tex_gain3) /
                                    MPP_MAX((float)(shp->reg157.sw_tex_idx3 - shp->reg157.sw_tex_idx2), 1));
    global_gain_slp_temp[4] = ROUND(128 * (float)(p_shp_param->tex_adj_val[5] - shp->reg158.sw_tex_gain4) /
                                    MPP_MAX((float)(255 - shp->reg157.sw_tex_idx3), 1));
    shp->reg159.sw_tex_slp01 = mpp_clip(global_gain_slp_temp[0], -1024, 1023);
    shp->reg159.sw_tex_slp12 = mpp_clip(global_gain_slp_temp[1], -1024, 1023);
    shp->reg160.sw_tex_slp23 = mpp_clip(global_gain_slp_temp[2], -1024, 1023);
    shp->reg160.sw_tex_slp34 = mpp_clip(global_gain_slp_temp[3], -1024, 1023);
    shp->reg161.sw_tex_slp45 = mpp_clip(global_gain_slp_temp[4], -1024, 1023);
}

static MPP_RET vdpp2_params_to_reg(Vdpp2Params* src_params, Vdpp2ApiCtx *ctx)
{
    Vdpp2Regs *dst_reg = &ctx->reg;
    DmsrParams *dmsr_params = &src_params->dmsr_params;
    ZmeParams *zme_params = &src_params->zme_params;
    EsParams *es_params = &src_params->es_params;
    ShpParams *shp_params = &src_params->shp_params;
    HistParams *hist_params = &src_params->hist_params;

    memset(dst_reg, 0, sizeof(*dst_reg));

    dst_reg->common.reg0.sw_vep_frm_en = 1;

    /* 0x0004(reg1) */
    dst_reg->common.reg1.sw_vep_src_fmt = VDPP_FMT_YUV420; // vep only support yuv420
    dst_reg->common.reg1.sw_vep_src_yuv_swap = src_params->src_yuv_swap;

    if (MPP_FMT_YUV420SP_VU == src_params->src_fmt)
        dst_reg->common.reg1.sw_vep_src_yuv_swap = 1;

    dst_reg->common.reg1.sw_vep_dst_fmt = src_params->dst_fmt;
    dst_reg->common.reg1.sw_vep_dst_yuv_swap = src_params->dst_yuv_swap;
    dst_reg->common.reg1.sw_vep_dbmsr_en = (src_params->working_mode == VDPP_WORK_MODE_DCI)
                                           ? 0
                                           : dmsr_params->dmsr_enable;

    vdpp_logv("set reg: vep_src_fmt=%d(must be 3 here), vep_src_yuv_swap=%d\n",
              dst_reg->common.reg1.sw_vep_src_fmt, dst_reg->common.reg1.sw_vep_src_yuv_swap);
    vdpp_logv("set reg: vep_dst_fmt=%d(vdppFmt: 0-444sp; 3-420sp), vep_dst_yuv_swap=%d\n",
              dst_reg->common.reg1.sw_vep_dst_fmt, dst_reg->common.reg1.sw_vep_dst_yuv_swap);

    /* 0x0008(reg2) */
    dst_reg->common.reg2.sw_vdpp_working_mode = src_params->working_mode;
    vdpp_logv("set reg: vdpp_working_mode=%d (1-iep2, 2-vep, 3-dci)\n", src_params->working_mode);

    /* 0x000C ~ 0x001C(reg3 ~ reg7), skip */
    dst_reg->common.reg4.sw_vep_clk_on = 1;
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
    dst_reg->common.reg8.sw_vep_frm_done_en = 1;
    dst_reg->common.reg8.sw_vep_osd_max_en = 1;
    dst_reg->common.reg8.sw_vep_bus_error_en = 1;
    dst_reg->common.reg8.sw_vep_timeout_int_en = 1;
    dst_reg->common.reg8.sw_vep_config_error_en = 1;
    /* 0x0024 ~ 0x002C(reg9 ~ reg11), skip */
    {
        RK_U32 src_right_redundant = (16 - (src_params->src_width & 0xF)) & 0xF;
        RK_U32 src_down_redundant  = (8 - (src_params->src_height & 0x7)) & 0x7;
        // TODO: check if the dst_right_redundant is aligned to 2 or 4(for yuv420sp)
        RK_U32 dst_right_redundant = (16 - (src_params->dst_width & 0xF)) & 0xF;
        /* 0x0030(reg12) */
        dst_reg->common.reg12.sw_vep_src_vir_y_stride = src_params->src_width_vir / 4;

        /* 0x0034(reg13) */
        dst_reg->common.reg13.sw_vep_dst_vir_y_stride = src_params->dst_width_vir / 4;

        /* 0x0038(reg14) */
        dst_reg->common.reg14.sw_vep_src_pic_width = src_params->src_width + src_right_redundant - 1;
        dst_reg->common.reg14.sw_vep_src_right_redundant = src_right_redundant;
        dst_reg->common.reg14.sw_vep_src_pic_height = src_params->src_height + src_down_redundant - 1;
        dst_reg->common.reg14.sw_vep_src_down_redundant = src_down_redundant;

        /* 0x003C(reg15) */
        dst_reg->common.reg15.sw_vep_dst_pic_width = src_params->dst_width + dst_right_redundant - 1;
        dst_reg->common.reg15.sw_vep_dst_right_redundant = dst_right_redundant;
        dst_reg->common.reg15.sw_vep_dst_pic_height = src_params->dst_height - 1;
    }
    /* 0x0040 ~ 0x005C(reg16 ~ reg23), skip */
    dst_reg->common.reg20.sw_vep_timeout_en = 1;
    dst_reg->common.reg20.sw_vep_timeout_cnt = 0x8FFFFFF;

    /* 0x0060(reg24) */
    dst_reg->common.reg24.sw_vep_src_addr_y = src_params->src.y;

    /* 0x0064(reg25) */
    dst_reg->common.reg25.sw_vep_src_addr_uv = src_params->src.cbcr;

    /* 0x0068(reg26) */
    dst_reg->common.reg26.sw_vep_dst_addr_y = src_params->dst.y;

    /* 0x006C(reg27) */
    dst_reg->common.reg27.sw_vep_dst_addr_uv = src_params->dst.cbcr;

    if (src_params->yuv_out_diff) {
        RK_S32 dst_c_width     = src_params->dst_c_width;
        RK_S32 dst_c_height    = src_params->dst_c_height;
        RK_S32 dst_c_width_vir = src_params->dst_c_width_vir;
        RK_S32 dst_redundant_c = 0;

        /** @note: no need to half the chroma size for YUV420, since vdpp will do it according to the output format! */
        if (dst_reg->common.reg1.sw_vep_dst_fmt == VDPP_FMT_YUV420) {
            dst_c_width     *= 2;
            dst_c_height    *= 2;
            dst_c_width_vir *= 2;
        }

        dst_redundant_c = (16 - (dst_c_width & 0xF)) & 0xF;
        dst_reg->common.reg1.sw_vep_yuvout_diff_en = 1;
        dst_reg->common.reg13.sw_vep_dst_vir_c_stride = dst_c_width_vir / 4; // unit: dword
        /* 0x0040(reg16) */
        dst_reg->common.reg16.sw_vep_dst_pic_width_c = dst_c_width + dst_redundant_c - 1;
        dst_reg->common.reg16.sw_vep_dst_right_redundant_c = dst_redundant_c;
        dst_reg->common.reg16.sw_vep_dst_pic_height_c = dst_c_height - 1;

        dst_reg->common.reg27.sw_vep_dst_addr_uv = src_params->dst_c.cbcr;

        vdpp_logv("set reg: vep_dst_vir_c_stride=%d [dword], vep_dst_right_redundant_c=%d [pixel]\n",
                  dst_reg->common.reg13.sw_vep_dst_vir_c_stride, dst_reg->common.reg16.sw_vep_dst_right_redundant_c);
        vdpp_logv("set reg: vep_dst_pic_width_c=%d [pixel], vep_dst_pic_height_c=%d [pixel]\n",
                  dst_reg->common.reg16.sw_vep_dst_pic_width_c, dst_reg->common.reg16.sw_vep_dst_pic_height_c);
    }
    vdpp_logv("set reg: vep_yuvout_diff_en=%d, vep_dst_addr_uv(fd)=%d\n",
              dst_reg->common.reg1.sw_vep_yuvout_diff_en, dst_reg->common.reg27.sw_vep_dst_addr_uv);

    /* vdpp1 */
    set_dmsr_to_vdpp_reg(dmsr_params, &dst_reg->dmsr);

    zme_params->src_width = src_params->src_width;
    zme_params->src_height = src_params->src_height;
    zme_params->dst_width = src_params->dst_width;
    zme_params->dst_height = src_params->dst_height;
    zme_params->dst_fmt = src_params->dst_fmt;
    zme_params->yuv_out_diff = src_params->yuv_out_diff;
    zme_params->dst_c_width = src_params->dst_c_width;
    zme_params->dst_c_height = src_params->dst_c_height;
    set_zme_to_vdpp_reg(zme_params, &dst_reg->zme);

    /* vdpp2 */
    hist_params->src_fmt = src_params->src_fmt;
    hist_params->src_width = src_params->src_width;
    hist_params->src_height = src_params->src_height;
    hist_params->src_width_vir = src_params->src_width_vir;
    hist_params->src_height_vir = src_params->src_height_vir;
    hist_params->src_addr = src_params->src.y;
    hist_params->working_mode = src_params->working_mode;
    set_hist_to_vdpp2_reg(hist_params, &dst_reg->hist, &dst_reg->common);

    set_es_to_vdpp2_reg(es_params, &dst_reg->es);
    set_shp_to_vdpp2_reg(shp_params, &dst_reg->sharp);

    return MPP_OK;
}

static MPP_RET vdpp2_set_default_param(Vdpp2Params *param)
{
    // base
    param->src_fmt = MPP_FMT_YUV420SP; // default 420sp
    param->src_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->dst_fmt = VDPP_FMT_YUV444;
    param->dst_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->src_width = 1920;
    param->src_height = 1080;
    param->dst_width = 1920;
    param->dst_height = 1080;
    param->yuv_out_diff = 0;
    param->working_mode = VDPP_WORK_MODE_VEP;

    /* vdpp1 features */
    vdpp_set_default_dmsr_param(&param->dmsr_params);
    vdpp_set_default_zme_param(&param->zme_params);

    /* vdpp2 features */
    vdpp2_set_default_es_param(&param->es_params);
    vdpp2_set_default_shp_param(&param->shp_params);
    vdpp2_set_default_hist_param(&param->hist_params);
    param->hist_params.src_fmt = param->src_fmt;
    param->hist_params.src_width = param->src_width;
    param->hist_params.src_height = param->src_height;
    param->hist_params.src_width_vir = param->src_width;
    param->hist_params.src_height_vir = param->src_height;
    param->hist_params.src_addr = 0;
    param->hist_params.working_mode = VDPP_WORK_MODE_VEP;

    return MPP_OK;
}

void vdpp2_set_default_hist_param(HistParams *hist_params)
{
    if (!hist_params) {
        vdpp_loge("NULL argument set! \n");
        return;
    }

    memset(hist_params, 0, sizeof(HistParams));
}

void vdpp2_set_default_es_param(EsParams *param)
{
    param->es_bEnabledES = 0;
    param->es_iAngleDelta = 17;
    param->es_iAngleDeltaExtra = 5;
    param->es_iGradNoDirTh = 37;
    param->es_iGradFlatTh = 75;
    param->es_iWgtGain = 128;
    param->es_iWgtDecay = 128;
    param->es_iLowConfTh = 96;
    param->es_iLowConfRatio = 32;
    param->es_iConfCntTh = 4;
    param->es_iWgtLocalTh = 64;
    param->es_iK1 = 4096;
    param->es_iK2 = 7168;
    param->es_iDeltaLimit = 65280;
    memcpy(&param->es_iDiff2conf_lut_x[0], &diff2conf_lut_x_tmp[0], sizeof(diff2conf_lut_x_tmp));
    memcpy(&param->es_iDiff2conf_lut_y[0], &diff2conf_lut_y_tmp[0], sizeof(diff2conf_lut_y_tmp));
    param->es_bEndpointCheckEnable = 1;
}

void vdpp2_set_default_shp_param(ShpParams *param)
{
    param->sharp_enable = 1;
    param->sharp_coloradj_bypass_en = 1;

    param->lti_h_enable = 0;
    param->lti_h_radius = 1;
    param->lti_h_slope = 100;
    param->lti_h_thresold = 21;
    param->lti_h_gain = 8;
    param->lti_h_noise_thr_pos = 1023;
    param->lti_h_noise_thr_neg = 1023;

    param->lti_v_enable = 0;
    param->lti_v_radius = 1;
    param->lti_v_slope = 100;
    param->lti_v_thresold = 21;
    param->lti_v_gain = 8;
    param->lti_v_noise_thr_pos = 1023;
    param->lti_v_noise_thr_neg = 1023;

    param->cti_h_enable = 0;
    param->cti_h_radius = 1;
    param->cti_h_slope = 100;
    param->cti_h_thresold = 21;
    param->cti_h_gain = 8;
    param->cti_h_noise_thr_pos = 1023;
    param->cti_h_noise_thr_neg = 1023;

    param->peaking_enable = 1;
    param->peaking_gain = 196;

    param->peaking_coring_enable = 1;
    param->peaking_limit_ctrl_enable = 1;
    param->peaking_gain_enable = 1;

    memcpy(param->peaking_coring_zero, coring_zero_tmp, sizeof(coring_zero_tmp));
    memcpy(param->peaking_coring_thr, coring_thr_tmp, sizeof(coring_thr_tmp));
    memcpy(param->peaking_coring_ratio, coring_ratio_tmp, sizeof(coring_ratio_tmp));
    memcpy(param->peaking_gain_pos, gain_pos_tmp, sizeof(gain_pos_tmp));
    memcpy(param->peaking_gain_neg, gain_neg_tmp, sizeof(gain_neg_tmp));
    memcpy(param->peaking_limit_ctrl_pos0, limit_ctrl_pos0_tmp, sizeof(limit_ctrl_pos0_tmp));
    memcpy(param->peaking_limit_ctrl_pos1, limit_ctrl_pos1_tmp, sizeof(limit_ctrl_pos1_tmp));
    memcpy(param->peaking_limit_ctrl_neg0, limit_ctrl_neg0_tmp, sizeof(limit_ctrl_neg0_tmp));
    memcpy(param->peaking_limit_ctrl_neg1, limit_ctrl_neg1_tmp, sizeof(limit_ctrl_neg1_tmp));
    memcpy(param->peaking_limit_ctrl_ratio, limit_ctrl_ratio_tmp, sizeof(limit_ctrl_ratio_tmp));
    memcpy(param->peaking_limit_ctrl_bnd_pos, limit_ctrl_bnd_pos_tmp, sizeof(limit_ctrl_bnd_pos_tmp));
    memcpy(param->peaking_limit_ctrl_bnd_neg, limit_ctrl_bnd_neg_tmp, sizeof(limit_ctrl_bnd_neg_tmp));

    param->peaking_edge_ctrl_enable = 1;
    param->peaking_edge_ctrl_non_dir_thr = 16;
    param->peaking_edge_ctrl_dir_cmp_ratio = 4;
    param->peaking_edge_ctrl_non_dir_wgt_offset = 64;
    param->peaking_edge_ctrl_non_dir_wgt_ratio = 16;
    param->peaking_edge_ctrl_dir_cnt_thr = 2;
    param->peaking_edge_ctrl_dir_cnt_avg = 3;
    param->peaking_edge_ctrl_dir_cnt_offset = 2;
    param->peaking_edge_ctrl_diag_dir_thr = 16;

    memcpy(param->peaking_edge_ctrl_diag_adj_gain_tab, diag_adj_gain_tab_tmp,
           sizeof(diag_adj_gain_tab_tmp));

    param->peaking_estc_enable = 1;
    param->peaking_estc_delta_offset_h = 4;
    param->peaking_estc_alpha_over_h = 8;
    param->peaking_estc_alpha_under_h = 16;
    param->peaking_estc_alpha_over_unlimit_h = 64;
    param->peaking_estc_alpha_under_unlimit_h = 112;
    param->peaking_estc_delta_offset_v = 4;
    param->peaking_estc_alpha_over_v = 8;
    param->peaking_estc_alpha_under_v = 16;
    param->peaking_estc_alpha_over_unlimit_v = 64;
    param->peaking_estc_alpha_under_unlimit_v = 112;
    param->peaking_estc_delta_offset_d0 = 4;
    param->peaking_estc_alpha_over_d0 = 16;
    param->peaking_estc_alpha_under_d0 = 16;
    param->peaking_estc_alpha_over_unlimit_d0 = 96;
    param->peaking_estc_alpha_under_unlimit_d0 = 96;
    param->peaking_estc_delta_offset_d1 = 4;
    param->peaking_estc_alpha_over_d1 = 16;
    param->peaking_estc_alpha_under_d1 = 16;
    param->peaking_estc_alpha_over_unlimit_d1 = 96;
    param->peaking_estc_alpha_under_unlimit_d1 = 96;
    param->peaking_estc_delta_offset_non = 4;
    param->peaking_estc_alpha_over_non = 8;
    param->peaking_estc_alpha_under_non = 8;
    param->peaking_estc_alpha_over_unlimit_non = 112;
    param->peaking_estc_alpha_under_unlimit_non = 112;
    param->peaking_filter_cfg_diag_enh_coef = 6;

    param->peaking_filt_core_H0[0] = 4;
    param->peaking_filt_core_H0[1] = 16;
    param->peaking_filt_core_H0[2] = 24;
    param->peaking_filt_core_H1[0] = -16;
    param->peaking_filt_core_H1[1] = 0;
    param->peaking_filt_core_H1[2] = 32;
    param->peaking_filt_core_H2[0] = 0;
    param->peaking_filt_core_H2[1] = -16;
    param->peaking_filt_core_H2[2] = 32;
    param->peaking_filt_core_V0[0] = 1;
    param->peaking_filt_core_V0[1] = 4;
    param->peaking_filt_core_V0[2] = 6;
    param->peaking_filt_core_V1[0] = -4;
    param->peaking_filt_core_V1[1] = 0;
    param->peaking_filt_core_V1[2] = 8;
    param->peaking_filt_core_V2[0] = 0;
    param->peaking_filt_core_V2[1] = -4;
    param->peaking_filt_core_V2[2] = 8;
    param->peaking_filt_core_USM[0] = 1;
    param->peaking_filt_core_USM[1] = 4;
    param->peaking_filt_core_USM[2] = 6;

    param->shootctrl_enable = 1;
    param->shootctrl_filter_radius = 1;
    param->shootctrl_delta_offset = 16;
    param->shootctrl_alpha_over = 8;
    param->shootctrl_alpha_under = 8;
    param->shootctrl_alpha_over_unlimit = 112;
    param->shootctrl_alpha_under_unlimit = 112;

    param->global_gain_enable = 0;
    param->global_gain_lum_mode = 0;

    memcpy(param->global_gain_lum_grd, lum_grd_tmp, sizeof(lum_grd_tmp));
    memcpy(param->global_gain_lum_val, lum_val_tmp, sizeof(lum_val_tmp));
    memcpy(param->global_gain_adp_grd, adp_grd_tmp, sizeof(adp_grd_tmp));
    memcpy(param->global_gain_adp_val, adp_val_tmp, sizeof(adp_val_tmp));
    memcpy(param->global_gain_var_grd, var_grd_tmp, sizeof(var_grd_tmp));
    memcpy(param->global_gain_var_val, var_val_tmp, sizeof(var_val_tmp));

    param->color_ctrl_enable = 0;

    param->color_ctrl_p0_scaling_coef = 1;
    param->color_ctrl_p0_point_u = 115;
    param->color_ctrl_p0_point_v = 155;
    memcpy(param->color_ctrl_p0_roll_tab, roll_tab_pattern0, sizeof(roll_tab_pattern0));
    param->color_ctrl_p1_scaling_coef = 1;
    param->color_ctrl_p1_point_u = 90;
    param->color_ctrl_p1_point_v = 120;
    memcpy(param->color_ctrl_p1_roll_tab, roll_tab_pattern1, sizeof(roll_tab_pattern1));
    param->color_ctrl_p2_scaling_coef = 1;
    param->color_ctrl_p2_point_u = 128;
    param->color_ctrl_p2_point_v = 128;
    memcpy(param->color_ctrl_p2_roll_tab, roll_tab_pattern2, sizeof(roll_tab_pattern2));
    param->color_ctrl_p3_scaling_coef = 1;
    param->color_ctrl_p3_point_u = 128;
    param->color_ctrl_p3_point_v = 128;
    memcpy(param->color_ctrl_p3_roll_tab, roll_tab_pattern2, sizeof(roll_tab_pattern2));

    param->tex_adj_enable = 0;
    param->tex_adj_y_mode_select = 3;
    param->tex_adj_mode_select = 0;

    memcpy(param->tex_adj_grd, tex_grd_tmp, sizeof(tex_grd_tmp));
    memcpy(param->tex_adj_val, tex_val_tmp, sizeof(tex_val_tmp));
}

MPP_RET vdpp2_init(VdppCtx ictx)
{
    MppReqV1 mpp_req;
    RK_U32 client_data = VDPP_CLIENT_TYPE;
    Vdpp2ApiCtx *ctx = ictx;
    MPP_RET ret = MPP_OK;

    if (NULL == ictx) {
        vdpp_loge("found NULL input vdpp2 ctx %p\n", ictx);
        return MPP_ERR_NULL_PTR;
    }

    // default log level: warning
    mpp_env_get_u32(VDPP_COM_DEBUG_ENV_NAME, &vdpp_debug,
                    (VDPP_LOG_FATAL | VDPP_LOG_ERROR | VDPP_LOG_WARNING));
    vdpp_logi("get env '%s' flag: %#x", VDPP_COM_DEBUG_ENV_NAME, vdpp_debug);

    vdpp_enter();

    ctx->fd = open("/dev/mpp_service", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        vdpp_loge("can NOT find device /dev/vdpp\n");
        return MPP_NOK;
    }

    mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
    mpp_req.flag = 0;
    mpp_req.size = sizeof(client_data);
    mpp_req.data_ptr = REQ_DATA_PTR(&client_data);

    /* set default parameters */
    memset(&ctx->params, 0, sizeof(Vdpp2Params));
    vdpp2_set_default_param(&ctx->params);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        vdpp_loge("ioctl set_client failed!\n");
        return MPP_NOK;
    }

    vdpp_leave();
    return ret;
}

MPP_RET vdpp2_deinit(VdppCtx ictx)
{
    Vdpp2ApiCtx *ctx = NULL;

    if (NULL == ictx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ictx);
        return MPP_ERR_NULL_PTR;
    }

    ctx = ictx;

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    return MPP_OK;
}

static MPP_RET vdpp2_set_param(Vdpp2ApiCtx *ctx, VdppApiContent *param, VdppParamType type)
{
    RK_U16 mask;
    RK_U16 cfg_set;

    switch (type) {
    case VDPP_PARAM_TYPE_COM: { // deprecated config
        ctx->params.src_fmt = MPP_FMT_YUV420SP; // default 420
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
        ctx->params.hist_params.hist_cnt_en = 0; // default disable

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
    } break;
    case VDPP_PARAM_TYPE_DMSR: {
        mpp_assert(sizeof(ctx->params.dmsr_params) == sizeof(param->dmsr));
        memcpy(&ctx->params.dmsr_params, &param->dmsr, sizeof(DmsrParams));
    } break;
    case VDPP_PARAM_TYPE_ZME_COM: {
        ctx->params.zme_params.zme_bypass_en = param->zme.bypass_enable;
        ctx->params.zme_params.zme_dering_enable = param->zme.dering_enable;
        ctx->params.zme_params.zme_dering_sen_0 = param->zme.dering_sen_0;
        ctx->params.zme_params.zme_dering_sen_1 = param->zme.dering_sen_1;
        ctx->params.zme_params.zme_dering_blend_alpha = param->zme.dering_blend_alpha;
        ctx->params.zme_params.zme_dering_blend_beta = param->zme.dering_blend_beta;
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
    } break;
    case VDPP_PARAM_TYPE_ZME_COEFF: {
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
    } break;
    case VDPP_PARAM_TYPE_COM2: {
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
        ctx->params.working_mode = (param->com2.hist_mode_en != 0) ?
                                   VDPP_WORK_MODE_DCI : VDPP_WORK_MODE_VEP;
        if (mask & VDPP_DMSR_EN)
            ctx->params.dmsr_params.dmsr_enable = ((cfg_set & VDPP_DMSR_EN) != 0) ? 1 : 0;
        if (mask & VDPP_ES_EN)
            ctx->params.es_params.es_bEnabledES = ((cfg_set & VDPP_ES_EN) != 0) ? 1 : 0;
        if (mask & VDPP_SHARP_EN)
            ctx->params.shp_params.sharp_enable = ((cfg_set & VDPP_SHARP_EN) != 0) ? 1 : 0;
    } break;
    case VDPP_PARAM_TYPE_ES: {
        mpp_assert(sizeof(ctx->params.es_params) == sizeof(param->es));
        memcpy(&ctx->params.es_params, &param->es, sizeof(EsParams));
    } break;
    case VDPP_PARAM_TYPE_HIST: {
        ctx->params.hist_params.hist_cnt_en = param->hist.hist_cnt_en;
        ctx->params.hist_params.dci_hsd_mode = param->hist.dci_hsd_mode;
        ctx->params.hist_params.dci_vsd_mode = param->hist.dci_vsd_mode;
        ctx->params.hist_params.dci_yrgb_gather_num = param->hist.dci_yrgb_gather_num;
        ctx->params.hist_params.dci_yrgb_gather_en = param->hist.dci_yrgb_gather_en;
        ctx->params.hist_params.dci_csc_range = param->hist.dci_csc_range;
        ctx->params.hist_params.hist_addr = param->hist.hist_addr;
    } break;
    case VDPP_PARAM_TYPE_SHARP: {
        mpp_assert(sizeof(ctx->params.shp_params) == sizeof(param->sharp));
        memcpy(&ctx->params.shp_params, &param->sharp, sizeof(ShpParams));
    } break;
    default: {
        vdpp_loge("unknown VDPP_PARAM_TYPE %#x !\n", type);
    } break;
    }

    return MPP_OK;
}

static RK_S32 check_cap(Vdpp2Params *params)
{
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_U32 vep_mode_check = 0;
    RK_U32 hist_mode_check = 0;

    if (NULL == params) {
        vdpp_loge("found null pointer params\n");
        return VDPP_CAP_UNSUPPORTED;
    }

    if (params->src_height_vir < params->src_height ||
        params->src_width_vir < params->src_width) {
        vdpp_logd("invalid src img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        return VDPP_CAP_UNSUPPORTED;
    }

    if (params->dst_height_vir < params->dst_height ||
        params->dst_width_vir < params->dst_width) {
        vdpp_logd("invalid dst img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->dst_width, params->dst_height,
                  params->dst_width_vir, params->dst_height_vir);
        return VDPP_CAP_UNSUPPORTED;
    }

    if ((params->src_fmt != MPP_FMT_YUV420SP) &&
        (params->src_fmt != MPP_FMT_YUV420SP_VU)) {
        vdpp_logd("vep only support nv12 or nv21\n");
        vep_mode_check++;
    }

    if ((params->src_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP2_VEP_MAX_WIDTH) ||
        (params->src_height < VDPP2_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP2_MODE_MIN_WIDTH) ||
        (params->src_height_vir < VDPP2_MODE_MIN_HEIGHT) ||
        (params->src_width_vir < VDPP2_MODE_MIN_WIDTH)) {
        vdpp_logd("vep unsupported src img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        vep_mode_check++;
    }

    if ((params->dst_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
        (params->dst_width_vir > VDPP2_VEP_MAX_WIDTH) ||
        (params->dst_height < VDPP2_MODE_MIN_HEIGHT) ||
        (params->dst_width < VDPP2_MODE_MIN_WIDTH) ||
        (params->dst_height_vir < VDPP2_MODE_MIN_HEIGHT) ||
        (params->dst_width_vir < VDPP2_MODE_MIN_WIDTH)) {
        vdpp_logd("vep unsupported dst img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->dst_width, params->dst_height,
                  params->dst_width_vir, params->dst_height_vir);
        vep_mode_check++;
    }

    if ((params->src_width_vir & 0xf) || (params->dst_width_vir & 0xf)) {
        vdpp_logd("vep only support img_w_i/o_vir 16Byte align\n");
        vdpp_logd("vep unsupported src img_w_vir=%d dst img_w_vir=%d\n",
                  params->src_width_vir, params->dst_width_vir);
        vep_mode_check++;
    }

    if (params->src_height_vir & 0x7) {
        vdpp_logd("vep only support img_h_in_vir 8pix align\n");
        vdpp_logd("vep unsupported src img_h_vir=%d\n", params->src_height_vir);
        vep_mode_check++;
    }

    if (params->dst_height_vir & 0x1) {
        vdpp_logd("vep only support img_h_out_vir 2pix align\n");
        vdpp_logd("vep unsupported dst img_h_vir=%d\n", params->dst_height_vir);
        vep_mode_check++;
    }

    if ((params->src_width & 1) || (params->src_height & 1) ||
        (params->dst_width & 1) || (params->dst_height & 1)) {
        vdpp_logd("vep only support img_w/h_vld 2pix align\n");
        vdpp_logd("vep unsupported img_w_i=%d img_h_i=%d img_w_o=%d img_h_o=%d\n",
                  params->src_width, params->src_height, params->dst_width, params->dst_height);
        vep_mode_check++;
    }

    if (params->yuv_out_diff) {
        if ((params->dst_c_height_vir > VDPP2_VEP_MAX_HEIGHT) ||
            (params->dst_c_width_vir > VDPP2_VEP_MAX_WIDTH) ||
            (params->dst_c_height < VDPP2_MODE_MIN_HEIGHT) ||
            (params->dst_c_width < VDPP2_MODE_MIN_WIDTH) ||
            (params->dst_c_height_vir < VDPP2_MODE_MIN_HEIGHT) ||
            (params->dst_c_width_vir < VDPP2_MODE_MIN_WIDTH) ||
            (params->dst_c_height_vir < params->dst_c_height) ||
            (params->dst_c_width_vir < params->dst_c_width)) {
            vdpp_logd("vep unsupported dst_c img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                      params->dst_c_width, params->dst_c_height,
                      params->dst_c_width_vir, params->dst_c_height_vir);
            vep_mode_check++;
        }

        if (params->dst_c_width_vir & 0xf) {
            vdpp_logd("vep only support img_w_c_out_vir 16Byte align\n");
            vdpp_logd("vep unsupported dst img_w_c_vir=%d\n", params->dst_c_width_vir);
            vep_mode_check++;
        }

        if (params->dst_c_height_vir & 0x1) {
            vdpp_logd("vep only support img_h_c_out_vir 2pix align\n");
            vdpp_logd("vep unsupported dst img_h_vir=%d\n", params->dst_c_height_vir);
            vep_mode_check++;
        }

        if ((params->dst_c_width & 1) || (params->dst_c_height & 1)) {
            vdpp_logd("vep only support img_c_w/h_vld 2pix align\n");
            vdpp_logd("vep unsupported img_w_o=%d img_h_o=%d\n",
                      params->dst_c_width, params->dst_c_height);
            vep_mode_check++;
        }
    }

    if ((params->src_height_vir > VDPP2_HIST_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP2_HIST_MAX_WIDTH) ||
        (params->src_height < VDPP2_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP2_MODE_MIN_WIDTH)) {
        vdpp_logd("dci unsupported src img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        hist_mode_check++;
    }

    /* 10 bit input */
    if ((params->src_fmt == MPP_FMT_YUV420SP_10BIT) ||
        (params->src_fmt == MPP_FMT_YUV422SP_10BIT) ||
        (params->src_fmt == MPP_FMT_YUV444SP_10BIT)) {
        if (params->src_width_vir & 0xf) {
            vdpp_logd("dci Y-10bit input only support img_w_in_vir 16Byte align\n");
            hist_mode_check++;
        }
    } else {
        if (params->src_width_vir & 0x3) {
            vdpp_logd("dci only support img_w_in_vir 4Byte align\n");
            hist_mode_check++;
        }
    }

    // vdpp_logd("vdpp2 src img resolution: w-%d-%d, h-%d-%d\n",
    //           params->src_width, params->src_width_vir,
    //           params->src_height, params->src_height_vir);
    // vdpp_logd("vdpp2 dst img resolution: w-%d-%d, h-%d-%d\n",
    //           params->dst_width, params->dst_width_vir,
    //           params->dst_height, params->dst_height_vir);

    if (!vep_mode_check) {
        ret_cap |= VDPP_CAP_VEP;
        vdpp_logd("vdpp2 support mode: VDPP_CAP_VEP\n");
    }

    if (!hist_mode_check) {
        vdpp_logd("vdpp2 support mode: VDPP_CAP_HIST\n");
        ret_cap |= VDPP_CAP_HIST;
    }

    return ret_cap;
}

MPP_RET vdpp2_start(Vdpp2ApiCtx *ctx)
{
    VdppRegOffsetInfo reg_off[2];
    /* Note: req_cnt must be 12 here! Make sure req_cnt<=11! */
    MppReqV1 mpp_req[12];
    RK_U32 req_cnt = 0;
    Vdpp2Regs *reg = NULL;
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_S32 work_mode;
    MPP_RET ret = MPP_OK;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp2 ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    vdpp_enter();

    work_mode = ctx->params.working_mode;
    ret_cap = check_cap(&ctx->params);
    if ((!(ret_cap & VDPP_CAP_VEP) && (VDPP_WORK_MODE_VEP == work_mode)) ||
        (!(ret_cap & VDPP_CAP_HIST) && (VDPP_WORK_MODE_DCI == work_mode))) {
        vdpp_loge("found incompat work mode %d %s, cap %d\n", work_mode,
                  working_mode_name[work_mode & 3], ret_cap);
        return MPP_NOK;
    }

    reg = &ctx->reg;

    memset(reg_off, 0, sizeof(reg_off));
    memset(mpp_req, 0, sizeof(mpp_req));
    memset(reg, 0, sizeof(*reg));

    vdpp2_params_to_reg(&ctx->params, ctx);

    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.yrgb_hor_coe);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_YRGB_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.yrgb_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.yrgb_ver_coe);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_YRGB_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.yrgb_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.cbcr_hor_coe);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_CBCR_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.cbcr_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.cbcr_ver_coe);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_CBCR_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.cbcr_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.common);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_ZME_COMMON;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.common);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->dmsr);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_DMSR;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->dmsr);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->hist);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_DCI_HIST;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->hist);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->es);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_ES;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->es);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->sharp);
    mpp_req[req_cnt].offset = VDPP2_REG_OFF_SHARP;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->sharp);

    /* set reg offset */
    reg_off[0].reg_idx = 25;
    reg_off[0].offset = ctx->params.src.cbcr_offset;
    reg_off[1].reg_idx = 27;

    if (!ctx->params.yuv_out_diff)
        reg_off[1].offset = ctx->params.dst.cbcr_offset;
    else
        reg_off[1].offset = ctx->params.dst_c.cbcr_offset;

    vdpp_logv("set reg: cbcr_offset for src/dst=%d/%d\n", reg_off[0].offset, reg_off[1].offset);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_REG_OFFSET_ALONE;
    mpp_req[req_cnt].size = sizeof(reg_off);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(reg_off);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    /* read output register for common */
    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    mpp_req[req_cnt].size = sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);
    mpp_assert(req_cnt <= 11);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req[0]);

    if (ret) {
        vdpp_loge("ioctl SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    vdpp_leave();
    return ret;
}

static MPP_RET vdpp2_wait(Vdpp2ApiCtx *ctx)
{
    MppReqV1 mpp_req;
    MPP_RET ret = MPP_OK;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    memset(&mpp_req, 0, sizeof(mpp_req));
    mpp_req.cmd = MPP_CMD_POLL_HW_FINISH;
    mpp_req.flag |= MPP_FLAGS_LAST_MSG;

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        vdpp_loge("ioctl POLL_HW_FINISH failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
    }

    return ret;
}

static MPP_RET vdpp2_done(Vdpp2ApiCtx *ctx)
{
    Vdpp2Regs *reg = NULL;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    vdpp_enter();

    reg = &ctx->reg;

    vdpp_logd("ro_frm_done_sts=%d\n", reg->common.reg10.ro_frm_done_sts);
    vdpp_logd("ro_osd_max_sts=%d\n", reg->common.reg10.ro_osd_max_sts);
    vdpp_logd("ro_bus_error_sts=%d\n", reg->common.reg10.ro_bus_error_sts);
    vdpp_logd("ro_timeout_sts=%d\n", reg->common.reg10.ro_timeout_sts);
    vdpp_logd("ro_config_error_sts=%d\n", reg->common.reg10.ro_config_error_sts);

    if (reg->common.reg8.sw_vep_frm_done_en &&
        !reg->common.reg10.ro_frm_done_sts) {
        vdpp_loge("run vdpp failed!!\n");
        return MPP_NOK;
    }

    if (reg->common.reg10.ro_timeout_sts) {
        vdpp_loge("run vdpp timeout!!\n");
        return MPP_NOK;
    }

    vdpp_logi("run vdpp success! %d\n", reg->common.reg10);
    vdpp_leave();
    return MPP_OK;
}

MPP_RET vdpp2_control(VdppCtx ictx, VdppCmd cmd, void *iparam)
{
    Vdpp2ApiCtx *ctx = ictx;
    MPP_RET ret = MPP_OK;

    if ((NULL == iparam && VDPP_CMD_RUN_SYNC != cmd) ||
        (NULL == ictx)) {
        vdpp_loge("found NULL iparam %p cmd %d ctx %p\n", iparam, cmd, ictx);
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
        VdppApiParams *api_param = (VdppApiParams *)iparam;

        ret = vdpp2_set_param(ctx, &api_param->param, api_param->ptype);
        if (ret) {
            vdpp_loge("set vdpp parameter failed, type %d\n", api_param->ptype);
        }
    } break;
    case VDPP_CMD_SET_SRC: {
        set_addr(&ctx->params.src, (VdppImg *)iparam);
    } break;
    case VDPP_CMD_SET_DST: {
        set_addr(&ctx->params.dst, (VdppImg *)iparam);
    } break;
    case VDPP_CMD_SET_DST_C: {
        set_addr(&ctx->params.dst_c, (VdppImg *)iparam);
    } break;
    case VDPP_CMD_SET_HIST_FD: {
        ctx->params.hist_params.hist_addr = *(RK_S32 *)iparam;
    } break;
    case VDPP_CMD_RUN_SYNC: {
        ret = vdpp2_start(ctx);
        if (ret) {
            vdpp_loge("run vdpp failed!!\n");
            return MPP_NOK;
        }

        ret |= vdpp2_wait(ctx);
        ret |= vdpp2_done(ctx);
    } break;
    case VDPP_CMD_GET_AVG_LUMA: {
        VdppResults *result = (VdppResults *)iparam;

        memset(result, 0, sizeof(VdppResults));

        if (!ctx->params.hist_params.hist_cnt_en || ctx->params.hist_params.hist_addr <= 0) {
            vdpp_logw("return invalid luma_avg value since the hist_cnt(%d)=0 or fd(%d)<=0!\n",
                      ctx->params.hist_params.hist_cnt_en, ctx->params.hist_params.hist_addr);
            result->hist.luma_avg = -1;
        } else {
            RK_U32 *p_hist_packed = (RK_U32 *)mmap(NULL, VDPP_HIST_PACKED_SIZE, PROT_READ | PROT_WRITE,
                                                   MAP_SHARED, ctx->params.hist_params.hist_addr, 0);

            if (p_hist_packed) {
                result->hist.luma_avg = get_avg_luma_from_hist(p_hist_packed + VDPP_HIST_GLOBAL_OFFSET / 4,
                                                               VDPP_HIST_GLOBAL_LENGTH);
                munmap(p_hist_packed, VDPP_HIST_PACKED_SIZE);
            } else {
                result->hist.luma_avg = -1;
                vdpp_loge("map hist fd(%d) failed! %s\n", ctx->params.hist_params.hist_addr,
                          strerror(errno));
            }
        }
    } break;
    default: {
        vdpp_loge("unsupported control cmd %#x for vdpp2!\n", cmd);
        ret = MPP_ERR_VALUE;
    } break;
    }

    return ret;
}

RK_S32 vdpp2_check_cap(VdppCtx ictx)
{
    Vdpp2ApiCtx *ctx = ictx;

    if (NULL == ictx) {
        vdpp_loge("found NULL ctx %p!\n", ictx);
        return VDPP_CAP_UNSUPPORTED;
    }

    return check_cap(&ctx->params);
}
