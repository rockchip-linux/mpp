/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
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

#include "vdpp3.h"

#define VDPP3_VEP_MAX_WIDTH   (1920)
#define VDPP3_VEP_MAX_HEIGHT  (2048)
#define VDPP3_HIST_MAX_WIDTH  (4096)
#define VDPP3_HIST_MAX_HEIGHT (4096)
#define VDPP3_MODE_MIN_WIDTH  (128)
#define VDPP3_MODE_MIN_HEIGHT (128)

#define VDPP3_HIST_HSD_DISABLE_LIMIT (1920)
#define VDPP3_HIST_VSD_DISABLE_LIMIT (1080)
#define VDPP3_HIST_VSD_MODE_1_LIMIT  (2160)

// default log level: warning
extern RK_U32 vdpp_debug;

void set_pyramid_to_vdpp3_reg(const PyrParams *pyr_params, RegPyr* pyr_reg)
{
    pyr_reg->reg0.sw_pyramid_en = pyr_params->pyr_en;
    pyr_reg->reg1.sw_pyramid_layer1_dst_addr = pyr_params->layer_imgs[0].mem_addr;
    pyr_reg->reg3.sw_pyramid_layer2_dst_addr = pyr_params->layer_imgs[1].mem_addr;
    pyr_reg->reg5.sw_pyramid_layer3_dst_addr = pyr_params->layer_imgs[2].mem_addr;
    pyr_reg->reg2.sw_pyramid_layer1_dst_vir_wid = pyr_params->layer_imgs[0].wid_vir / 4;
    pyr_reg->reg4.sw_pyramid_layer2_dst_vir_wid = pyr_params->layer_imgs[1].wid_vir / 4;
    pyr_reg->reg6.sw_pyramid_layer3_dst_vir_wid = pyr_params->layer_imgs[2].wid_vir / 4;

    vdpp_logv("set reg: pyr_en=%d, fd=%d/%d/%d, vir_wid=%d/%d/%d\n", pyr_reg->reg0.sw_pyramid_en,
              pyr_reg->reg1.sw_pyramid_layer1_dst_addr, pyr_reg->reg3.sw_pyramid_layer2_dst_addr,
              pyr_reg->reg5.sw_pyramid_layer3_dst_addr, pyr_reg->reg2.sw_pyramid_layer1_dst_vir_wid,
              pyr_reg->reg4.sw_pyramid_layer2_dst_vir_wid, pyr_reg->reg6.sw_pyramid_layer3_dst_vir_wid);

    if ((pyr_reg->reg2.sw_pyramid_layer1_dst_vir_wid & 0x3) ||
        (pyr_reg->reg4.sw_pyramid_layer2_dst_vir_wid & 0x3) ||
        (pyr_reg->reg6.sw_pyramid_layer3_dst_vir_wid & 0x3)) {
        vdpp_logw("the pyr virtual width is not x4 aligned, pyr result might not be correct!!\n");
    }
}

void set_bbd_to_vdpp3_reg(const BbdParams* bbd_params, RegBbd* bbd_reg)
{
    bbd_reg->reg0.sw_det_en = bbd_params->bbd_en;
    bbd_reg->reg0.sw_det_blcklmt = bbd_params->bbd_det_blcklmt;
    vdpp_logv("set reg: bbd_en=%d, lmt=%d\n", bbd_reg->reg0.sw_det_en, bbd_reg->reg0.sw_det_blcklmt);
}

void set_zme_to_vdpp3_reg(const ZmeParams *zme_params, RegZme3 *zme_reg)
{
    VdppZmeFmt zme_format_in = FMT_YCbCr420_888; /* only support yuv420 as input */
    VdppZmeScaleInfo yrgb_scl_info = {0};
    VdppZmeScaleInfo cbcr_scl_info = {0};

    yrgb_scl_info.act_width = zme_params->src_width;
    yrgb_scl_info.act_height = zme_params->src_height;
    yrgb_scl_info.dsp_width = zme_params->dst_width;
    yrgb_scl_info.dsp_height = zme_params->dst_height;
    yrgb_scl_info.xscl_mode = SCL_MPH;
    yrgb_scl_info.yscl_mode = SCL_MPH;
    yrgb_scl_info.dering_en = zme_params->zme_dering_enable;
    calc_scale_factor(zme_params, &yrgb_scl_info, zme_params->zme_bypass_en);

    cbcr_scl_info.act_width = zme_params->src_width / 2;
    cbcr_scl_info.act_height = zme_params->src_height / 2;
    if (zme_params->dst_fmt == VDPP_FMT_YUV444) {
        if (!zme_params->yuv_out_diff) {
            cbcr_scl_info.dsp_width = zme_params->dst_width;
            cbcr_scl_info.dsp_height = zme_params->dst_height;
        } else {
            cbcr_scl_info.dsp_width = zme_params->dst_c_width;
            cbcr_scl_info.dsp_height = zme_params->dst_c_height;
        }
    } else if (zme_params->dst_fmt == VDPP_FMT_YUV420) {
        if (!zme_params->yuv_out_diff) {
            cbcr_scl_info.dsp_width = zme_params->dst_width / 2;
            cbcr_scl_info.dsp_height = zme_params->dst_height / 2;
        } else {
            /** @note: half the chroma size by user! */
            cbcr_scl_info.dsp_width = zme_params->dst_c_width;
            cbcr_scl_info.dsp_height = zme_params->dst_c_height;
        }
    } else {
        /* not supported */
        vdpp_loge("invalid output vdpp format %d!\n", zme_params->dst_fmt);
    }
    cbcr_scl_info.xscl_mode = SCL_MPH;
    cbcr_scl_info.yscl_mode = SCL_MPH;
    cbcr_scl_info.dering_en = zme_params->zme_dering_enable;
    calc_scale_factor(zme_params, &cbcr_scl_info, zme_params->zme_bypass_en);

    /* 0x0800(reg0) */
    zme_reg->reg0.bypass_en = 0;
    zme_reg->reg0.align_en = 0;
    zme_reg->reg0.format_in = zme_format_in;
    if (zme_params->dst_fmt == VDPP_FMT_YUV444)
        zme_reg->reg0.format_out = FMT_YCbCr444_888;
    else
        zme_reg->reg0.format_out = FMT_YCbCr420_888;
    zme_reg->reg0.auto_gating_en = 1;

    /* 0x0804 ~ 0x080C(reg1 ~ reg3), skip */

    /* 0x0810(reg4) */
    zme_reg->reg4.yrgb_xsd_en = yrgb_scl_info.xsd_en;
    zme_reg->reg4.yrgb_xsu_en = yrgb_scl_info.xsu_en;
    zme_reg->reg4.yrgb_scl_mode = yrgb_scl_info.xscl_mode;
    zme_reg->reg4.yrgb_ysd_en = yrgb_scl_info.ysd_en;
    zme_reg->reg4.yrgb_ysu_en = yrgb_scl_info.ysu_en;
    zme_reg->reg4.yrgb_yscl_mode = yrgb_scl_info.yscl_mode;
    zme_reg->reg4.yrgb_dering_en = yrgb_scl_info.dering_en;
    zme_reg->reg4.yrgb_gt_en = yrgb_scl_info.ygt_en;
    zme_reg->reg4.yrgb_gt_mode = yrgb_scl_info.ygt_mode;
    zme_reg->reg4.yrgb_xsd_bypass = yrgb_scl_info.xsd_bypass;
    zme_reg->reg4.yrgb_ys_bypass = yrgb_scl_info.ys_bypass;
    zme_reg->reg4.yrgb_xsu_bypass = yrgb_scl_info.xsu_bypass;

    /* new vars in vdpp3 */
    zme_reg->reg4.yrgb_xscl_coe_sel = yrgb_scl_info.xscl_coe_idx;
    zme_reg->reg4.yrgb_yscl_coe_sel = yrgb_scl_info.yscl_coe_idx;

    /* 0x0814 ~ 0x0818(reg5 ~ reg6), skip */

    /* 0x081C(reg7) */
    zme_reg->reg7.yrgb_dering_sen0 = zme_params->zme_dering_sen_0;
    zme_reg->reg7.yrgb_dering_sen1 = zme_params->zme_dering_sen_1;
    zme_reg->reg7.yrgb_dering_alpha = zme_params->zme_dering_blend_alpha;
    zme_reg->reg7.yrgb_dering_delta = zme_params->zme_dering_blend_beta;

    /* 0x0820(reg8) */
    zme_reg->reg8.yrgb_xscl_factor = yrgb_scl_info.xscl_factor;
    zme_reg->reg8.yrgb_xscl_offset = yrgb_scl_info.xscl_offset;

    /* 0x0824(reg9) */
    zme_reg->reg9.yrgb_yscl_factor = yrgb_scl_info.yscl_factor;
    zme_reg->reg9.yrgb_yscl_offset = yrgb_scl_info.yscl_offset;

    /* 0x0828 ~ 0x082C(reg10 ~ reg11), skip */

    /* 0x0830(reg12) */
    zme_reg->reg12.cbcr_xsd_en = cbcr_scl_info.xsd_en;
    zme_reg->reg12.cbcr_xsu_en = cbcr_scl_info.xsu_en;
    zme_reg->reg12.cbcr_xscl_mode = cbcr_scl_info.xscl_mode;
    zme_reg->reg12.cbcr_ysd_en = cbcr_scl_info.ysd_en;
    zme_reg->reg12.cbcr_ysu_en = cbcr_scl_info.ysu_en;
    zme_reg->reg12.cbcr_yscl_mode = cbcr_scl_info.yscl_mode;
    zme_reg->reg12.cbcr_dering_en = cbcr_scl_info.dering_en;
    zme_reg->reg12.cbcr_gt_en = cbcr_scl_info.ygt_en;
    zme_reg->reg12.cbcr_gt_mode = cbcr_scl_info.ygt_mode;
    zme_reg->reg12.cbcr_xsd_bypass = cbcr_scl_info.xsd_bypass;
    zme_reg->reg12.cbcr_ys_bypass = cbcr_scl_info.ys_bypass;
    zme_reg->reg12.cbcr_xsu_bypass = cbcr_scl_info.xsu_bypass;

    /* new vars in vdpp3 */
    zme_reg->reg12.cbcr_xscl_coe_sel = cbcr_scl_info.xscl_coe_idx;
    zme_reg->reg12.cbcr_yscl_coe_sel = cbcr_scl_info.yscl_coe_idx;

    /* 0x0834 ~ 0x0838(reg13 ~ reg14), skip */

    /* 0x083C(reg15) */
    zme_reg->reg15.cbcr_dering_sen0 = zme_params->zme_dering_sen_0;
    zme_reg->reg15.cbcr_dering_sen1 = zme_params->zme_dering_sen_1;
    zme_reg->reg15.cbcr_dering_alpha = zme_params->zme_dering_blend_alpha;
    zme_reg->reg15.cbcr_dering_delta = zme_params->zme_dering_blend_beta;

    /* 0x0840(reg16) */
    zme_reg->reg16.cbcr_xscl_factor = cbcr_scl_info.xscl_factor;
    zme_reg->reg16.cbcr_xscl_offset = cbcr_scl_info.xscl_offset;

    /* 0x0844(reg17) */
    zme_reg->reg17.cbcr_yscl_factor = cbcr_scl_info.yscl_factor;
    zme_reg->reg17.cbcr_yscl_offset = cbcr_scl_info.yscl_offset;
}

static MPP_RET vdpp3_params_to_reg(Vdpp3Params* src_params, Vdpp3ApiCtx *ctx)
{
    Vdpp3Regs *dst_reg = &ctx->reg;
    DmsrParams *dmsr_params = &src_params->dmsr_params;
    ZmeParams *zme_params = &src_params->zme_params;
    EsParams *es_params = &src_params->es_params;
    ShpParams *shp_params = &src_params->shp_params;
    HistParams *hist_params = &src_params->hist_params;
    PyrParams *pyr_params = &src_params->pyr_params;
    BbdParams *bbd_params = &src_params->bbd_params;

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

        vdpp_logv("set reg: vep_dst_virc_strd=%d [dword], vep_dst_right_redundant_c=%d [pixel]\n",
                  dst_reg->common.reg13.sw_vep_dst_vir_c_stride,
                  dst_reg->common.reg16.sw_vep_dst_right_redundant_c);
        vdpp_logv("set reg: vep_dst_pic_wid_c=%d [pixel], vep_dst_pic_hgt_c=%d [pixel]\n",
                  dst_reg->common.reg16.sw_vep_dst_pic_width_c,
                  dst_reg->common.reg16.sw_vep_dst_pic_height_c);
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
    set_zme_to_vdpp3_reg(zme_params, &dst_reg->zme);

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

    /* vdpp3 */
    set_pyramid_to_vdpp3_reg(pyr_params, &dst_reg->pyr);
    set_bbd_to_vdpp3_reg(bbd_params, &dst_reg->bbd);

    return MPP_OK;
}

void vdpp3_set_default_pyr_param(PyrParams *pyr_params)
{
    memset(pyr_params, 0, sizeof(PyrParams));
    pyr_params->pyr_en = 0;
    pyr_params->nb_layers = 3;
}

void vdpp3_set_default_bbd_param(BbdParams *bbd_params)
{
    bbd_params->bbd_en = 1;
    bbd_params->bbd_det_blcklmt = 20; // 20 in 255
    bbd_params->bbd_size_top = 0;
    bbd_params->bbd_size_bottom = 0;
    bbd_params->bbd_size_left = 0;
    bbd_params->bbd_size_right = 0;
}

MPP_RET vdpp3_set_default_param(Vdpp3Params *param)
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

    /* vdpp3 */
    vdpp3_set_default_pyr_param(&param->pyr_params);
    vdpp3_set_default_bbd_param(&param->bbd_params);

    return MPP_OK;
}

MPP_RET vdpp3_init(VdppCtx ictx)
{
    MppReqV1 mpp_req;
    RK_U32 client_data = VDPP_CLIENT_TYPE;
    Vdpp3ApiCtx *ctx = ictx;
    MPP_RET ret = MPP_OK;

    if (NULL == ictx) {
        vdpp_loge("found NULL input vdpp3 ctx %p\n", ictx);
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
    memset(&ctx->params, 0, sizeof(Vdpp3Params));
    vdpp3_set_default_param(&ctx->params);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        vdpp_loge("ioctl set_client failed!\n");
        return MPP_NOK;
    }

    vdpp_leave();
    return ret;
}

MPP_RET vdpp3_deinit(VdppCtx ictx)
{
    Vdpp3ApiCtx *ctx = NULL;

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

static MPP_RET vdpp3_set_param(Vdpp3ApiCtx *ctx, VdppApiContent *param, VdppParamType type)
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
    } break;
    case VDPP_PARAM_TYPE_ZME_COEFF: {
        /** @note: coef fixed in vdpp3, check if need to disable this case. */
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
    case VDPP_PARAM_TYPE_PYR: {
        mpp_assert(sizeof(ctx->params.pyr_params) == sizeof(param->pyr));
        memcpy(&ctx->params.pyr_params, &param->pyr, sizeof(PyrParams));
    } break;
    case VDPP_PARAM_TYPE_BBD: {
        ctx->params.bbd_params.bbd_en = param->bbd.bbd_en;
        // check if need to convert threshold from full to limited
        if (ctx->params.working_mode == VDPP_WORK_MODE_VEP &&
            ctx->params.src_range == VDPP_COLOR_SPACE_LIMIT_RANGE) {
            ctx->params.bbd_params.bbd_det_blcklmt = DIV_255_FAST(param->bbd.bbd_det_blcklmt * 219) + 16;
            vdpp_logd("set bbd_det_blcklmt from %d to %d since src_range(%d) is limited(0) and working mode(%d) is VEP(2)!\n",
                      param->bbd.bbd_det_blcklmt, ctx->params.bbd_params.bbd_det_blcklmt,
                      ctx->params.src_range, ctx->params.working_mode);
        } else {
            ctx->params.bbd_params.bbd_det_blcklmt = param->bbd.bbd_det_blcklmt;
            vdpp_logd("set bbd_det_blcklmt to %d since src_range(%d) is full(1) or working mode(%d) is DCI_HIST(3)!\n",
                      param->bbd.bbd_det_blcklmt, ctx->params.src_range, ctx->params.working_mode);
        }
    } break;
    default: {
        vdpp_loge("unknown VDPP_PARAM_TYPE %#x !\n", type);
    } break;
    }

    return MPP_OK;
}

static RK_S32 check_cap(Vdpp3Params *params)
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

    if ((params->src_height_vir > VDPP3_VEP_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP3_VEP_MAX_WIDTH) ||
        (params->src_height < VDPP3_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP3_MODE_MIN_WIDTH) ||
        (params->src_height_vir < VDPP3_MODE_MIN_HEIGHT) ||
        (params->src_width_vir < VDPP3_MODE_MIN_WIDTH)) {
        vdpp_logd("vep unsupported src img_w=%d img_h=%d img_w_vir=%d img_h_vir=%d\n",
                  params->src_width, params->src_height,
                  params->src_width_vir, params->src_height_vir);
        vep_mode_check++;
    }

    if ((params->dst_height_vir > VDPP3_VEP_MAX_HEIGHT) ||
        (params->dst_width_vir > VDPP3_VEP_MAX_WIDTH) ||
        (params->dst_height < VDPP3_MODE_MIN_HEIGHT) ||
        (params->dst_width < VDPP3_MODE_MIN_WIDTH) ||
        (params->dst_height_vir < VDPP3_MODE_MIN_HEIGHT) ||
        (params->dst_width_vir < VDPP3_MODE_MIN_WIDTH)) {
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
        if ((params->dst_c_height_vir > VDPP3_VEP_MAX_HEIGHT) ||
            (params->dst_c_width_vir > VDPP3_VEP_MAX_WIDTH) ||
            (params->dst_c_height < VDPP3_MODE_MIN_HEIGHT) ||
            (params->dst_c_width < VDPP3_MODE_MIN_WIDTH) ||
            (params->dst_c_height_vir < VDPP3_MODE_MIN_HEIGHT) ||
            (params->dst_c_width_vir < VDPP3_MODE_MIN_WIDTH) ||
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

    if ((params->src_height_vir > VDPP3_HIST_MAX_HEIGHT) ||
        (params->src_width_vir > VDPP3_HIST_MAX_WIDTH) ||
        (params->src_height < VDPP3_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP3_MODE_MIN_WIDTH)) {
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

    // vdpp_logd("vdpp3 src img resolution: w-%d-%d, h-%d-%d\n",
    //           params->src_width, params->src_width_vir,
    //           params->src_height, params->src_height_vir);
    // vdpp_logd("vdpp3 dst img resolution: w-%d-%d, h-%d-%d\n",
    //           params->dst_width, params->dst_width_vir,
    //           params->dst_height, params->dst_height_vir);

    if (!vep_mode_check) {
        ret_cap |= VDPP_CAP_VEP;
        vdpp_logd("vdpp3 support mode: VDPP_CAP_VEP\n");
    }

    if (!hist_mode_check) {
        vdpp_logd("vdpp3 support mode: VDPP_CAP_HIST\n");
        ret_cap |= VDPP_CAP_HIST;
    }

    return ret_cap;
}

MPP_RET vdpp3_start(Vdpp3ApiCtx *ctx)
{
    VdppRegOffsetInfo reg_off[2];
    /* Note: req_cnt must be 12 here! Make sure req_cnt<=11! */
    MppReqV1 mpp_req[12];
    RK_U32 req_cnt = 0;
    Vdpp3Regs *reg = NULL;
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_S32 work_mode;
    MPP_RET ret = MPP_OK;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp3 ctx %p\n", ctx);
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

    vdpp3_params_to_reg(&ctx->params, ctx);

    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_ZME;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->dmsr);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_DMSR;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->dmsr);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->hist);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_DCI_HIST;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->hist);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->es);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_ES;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->es);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->pyr);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_PYRAMID;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->pyr);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->bbd.reg0);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_BBD;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->bbd);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->sharp);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_SHARP;
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
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    /* read output register for BBD (need to know vsd/hsd mode in hist) */
    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->bbd);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_BBD;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->bbd);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    mpp_req[req_cnt].size = sizeof(reg->hist);
    mpp_req[req_cnt].offset = VDPP3_REG_OFF_DCI_HIST;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->hist);
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

static MPP_RET vdpp3_wait(Vdpp3ApiCtx *ctx)
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

static MPP_RET vdpp3_done(Vdpp3ApiCtx *ctx)
{
    Vdpp3Regs *reg = NULL;
    Vdpp3Params *params = NULL;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    vdpp_enter();

    reg = &ctx->reg;
    params = &ctx->params;

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

    vdpp_logv("read reg: bbd.sw_det_en=%d\n", reg->bbd.reg0.sw_det_en);
    if (reg->bbd.reg0.sw_det_en) {
        params->bbd_params.bbd_size_top = reg->bbd.reg1.blckbar_size_top;
        params->bbd_params.bbd_size_bottom = reg->bbd.reg1.blckbar_size_bottom;
        params->bbd_params.bbd_size_left = reg->bbd.reg2.blckbar_size_left;
        params->bbd_params.bbd_size_right = reg->bbd.reg2.blckbar_size_right;
        vdpp_logv("read reg: bbd.sw_det_blcklmt=%d\n", reg->bbd.reg0.sw_det_blcklmt);
        vdpp_logv("read reg: bbd.blckbar_size_top=%d\n", reg->bbd.reg1.blckbar_size_top);
        vdpp_logv("read reg: bbd.blckbar_size_bottom=%d\n", reg->bbd.reg1.blckbar_size_bottom);
        vdpp_logv("read reg: bbd.blckbar_size_left=%d\n", reg->bbd.reg2.blckbar_size_left);
        vdpp_logv("read reg: bbd.blckbar_size_right=%d\n", reg->bbd.reg2.blckbar_size_right);

        /* rescale bbd result if working_mode is 'VDPP_WORK_MODE_DCI' */
        if (VDPP_WORK_MODE_DCI == reg->common.reg2.sw_vdpp_working_mode) {
            const RK_U32 bit_v = reg->hist.reg3.sw_dci_vsd_mode;
            const RK_U32 bit_h = reg->hist.reg3.sw_dci_hsd_mode + 1;

            params->bbd_params.bbd_size_top <<= bit_v;
            params->bbd_params.bbd_size_bottom <<= bit_v;
            params->bbd_params.bbd_size_left <<= bit_h;
            params->bbd_params.bbd_size_right <<= bit_h;
            vdpp_logd("The working_mode is DCI,  BBD result rescaled below:\n");
            vdpp_logd("rescaled: bbd.blckbar_size_top=%d\n", params->bbd_params.bbd_size_top);
            vdpp_logd("rescaled: bbd.blckbar_size_bottom=%d\n", params->bbd_params.bbd_size_bottom);
            vdpp_logd("rescaled: bbd.blckbar_size_left=%d\n", params->bbd_params.bbd_size_left);
            vdpp_logd("rescaled: bbd.blckbar_size_right=%d\n", params->bbd_params.bbd_size_right);
            vdpp_logd("NOTE this rescaled result might be a little bit different than the original result!\n");
        }
    }

    vdpp_logi("run vdpp success! %d\n", reg->common.reg10);
    vdpp_leave();
    return MPP_OK;
}

MPP_RET vdpp3_control(VdppCtx ictx, VdppCmd cmd, void *iparam)
{
    Vdpp3ApiCtx *ctx = ictx;
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
    case VDPP_CMD_SET_SHARP:
    case VDPP_CMD_SET_PYR:
    case VDPP_CMD_SET_BBD: {
        VdppApiParams *api_param = (VdppApiParams *)iparam;

        ret = vdpp3_set_param(ctx, &api_param->param, api_param->ptype);
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
        ret = vdpp3_start(ctx);
        if (ret) {
            vdpp_loge("run vdpp failed!!\n");
            return MPP_NOK;
        }

        ret |= vdpp3_wait(ctx);
        ret |= vdpp3_done(ctx);
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
    case VDPP_CMD_GET_BBD: {
        if (!ctx->params.bbd_params.bbd_en) {
            vdpp_loge("run cmd 'VDPP_CMD_GET_BBD' failed since the bbd is disabled!\n");
            return MPP_NOK;
        }
        VdppResults *result = (VdppResults *)iparam;

        memset(result, 0, sizeof(VdppResults));
        result->bbd.bbd_size_top = ctx->params.bbd_params.bbd_size_top;
        result->bbd.bbd_size_bottom = ctx->params.bbd_params.bbd_size_bottom;
        result->bbd.bbd_size_left = ctx->params.bbd_params.bbd_size_left;
        result->bbd.bbd_size_right = ctx->params.bbd_params.bbd_size_right;
    } break;
    default: {
        vdpp_loge("unsupported control cmd %#x for vdpp3!\n", cmd);
        ret = MPP_ERR_VALUE;
    } break;
    }

    return ret;
}

RK_S32 vdpp3_check_cap(VdppCtx ictx)
{
    Vdpp3ApiCtx *ctx = ictx;

    if (NULL == ictx) {
        vdpp_loge("found NULL ctx %p!\n", ictx);
        return VDPP_CAP_UNSUPPORTED;
    }

    return check_cap(&ctx->params);
}
