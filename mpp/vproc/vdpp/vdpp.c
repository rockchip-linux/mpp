/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp"

#include <sys/ioctl.h>
#include <errno.h>

#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_service.h"
#include "mpp_platform.h"

#include "vdpp.h"

#define VDPP_MODE_MIN_WIDTH   (128)
#define VDPP_MODE_MIN_HEIGHT  (128)

RK_U32 vdpp_debug = 0;

static MPP_RET vdpp_params_to_reg(struct vdpp_params* src_params, struct vdpp_api_ctx *ctx)
{
    struct vdpp_reg *dst_reg = &ctx->reg;
    struct zme_params *zme_params = &src_params->zme_params;

    memset(dst_reg, 0, sizeof(*dst_reg));

    /* 1. set reg::common */
    dst_reg->common.reg0.sw_vdpp_frm_en = 1;

    /* 0x0004(reg1), TODO: add debug function */
    dst_reg->common.reg1.sw_vdpp_src_fmt = VDPP_FMT_YUV420;
    dst_reg->common.reg1.sw_vdpp_src_yuv_swap = src_params->src_yuv_swap;
    dst_reg->common.reg1.sw_vdpp_dst_fmt = src_params->dst_fmt;
    dst_reg->common.reg1.sw_vdpp_dst_yuv_swap = src_params->dst_yuv_swap;
    dst_reg->common.reg1.sw_vdpp_dbmsr_en = src_params->dmsr_params.dmsr_enable;

    /* 0x0008(reg2) */
    dst_reg->common.reg2.sw_vdpp_working_mode = VDPP_WORK_MODE_VEP;

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
        dst_reg->common.reg12.sw_vdpp_src_vir_y_stride = (src_params->src_width + src_right_redundant + 3) / 4;

        /* 0x0034(reg13) */
        dst_reg->common.reg13.sw_vdpp_dst_vir_y_stride = (src_params->dst_width + dst_right_redundant + 3) / 4;

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

    set_dmsr_to_vdpp_reg(&src_params->dmsr_params, &ctx->dmsr);

    zme_params->src_width = src_params->src_width;
    zme_params->src_height = src_params->src_height;
    zme_params->dst_width = src_params->dst_width;
    zme_params->dst_height = src_params->dst_height;
    zme_params->dst_fmt = src_params->dst_fmt;
    set_zme_to_vdpp_reg(zme_params, &ctx->zme);

    return MPP_OK;
}

static void vdpp_set_default_dmsr_param(struct dmsr_params* p_dmsr_param)
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
    p_dmsr_param->dmsr_diff_core_th0 = 2;
    p_dmsr_param->dmsr_diff_core_th1 = 5;
    p_dmsr_param->dmsr_diff_core_wgt0 = 16;
    p_dmsr_param->dmsr_diff_core_wgt1 = 12;
    p_dmsr_param->dmsr_diff_core_wgt2 = 8;
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

static MPP_RET vdpp_set_default_param(struct vdpp_params *param)
{
    /* src_fmt only NV12 supported */
    param->src_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->dst_fmt = VDPP_FMT_YUV444;
    param->dst_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->src_width = 1920;
    param->src_height = 1080;
    param->dst_width = 1920;
    param->dst_height = 1080;


    vdpp_set_default_dmsr_param(&param->dmsr_params);
    vdpp_set_default_zme_param(&param->zme_params);

    return MPP_OK;
}

MPP_RET vdpp_init(VdppCtx *ictx)
{
    MPP_RET ret;
    MppReqV1 mpp_req;
    RK_U32 client_data = VDPP_CLIENT_TYPE;
    struct vdpp_api_ctx *ctx = NULL;

    if (NULL == *ictx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", *ictx);
        return MPP_ERR_NULL_PTR;
    }

    ctx = *ictx;

    mpp_env_get_u32("vdpp_debug", &vdpp_debug, 0);

    ctx->fd = open("/dev/mpp_service", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        mpp_err("can NOT find device /dev/vdpp\n");
        return MPP_NOK;
    }

    mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
    mpp_req.flag = 0;
    mpp_req.size = sizeof(client_data);
    mpp_req.data_ptr = REQ_DATA_PTR(&client_data);

    memset(&ctx->params, 0, sizeof(struct vdpp_params));
    /* set default parameters */
    vdpp_set_default_param(&ctx->params);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        mpp_err("ioctl set_client failed\n");
        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET vdpp_deinit(VdppCtx ictx)
{
    struct vdpp_api_ctx *ctx = NULL;

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

static MPP_RET vdpp_set_param(struct vdpp_api_ctx *ctx,
                              union vdpp_api_content *param,
                              enum VDPP_PARAM_TYPE type)
{
    MPP_RET ret = MPP_OK;

    switch (type) {
    case VDPP_PARAM_TYPE_COM :
        ctx->params.src_yuv_swap = param->com.sswap;
        ctx->params.dst_fmt = param->com.dfmt;
        ctx->params.dst_yuv_swap = param->com.dswap;
        ctx->params.src_width = param->com.src_width;
        ctx->params.src_height = param->com.src_height;
        ctx->params.dst_width = param->com.dst_width;
        ctx->params.dst_height = param->com.dst_height;
        break;

    case VDPP_PARAM_TYPE_DMSR :
        memcpy(&ctx->params.dmsr_params, &param->dmsr, sizeof(struct dmsr_params));
        break;

    case VDPP_PARAM_TYPE_ZME_COM :
        ctx->params.zme_params.zme_bypass_en = param->zme.bypass_enable;
        ctx->params.zme_params.zme_dering_enable = param->zme.dering_enable;
        ctx->params.zme_params.zme_dering_sen_0 = param->zme.dering_sen_0;
        ctx->params.zme_params.zme_dering_sen_1 = param->zme.dering_sen_1;
        ctx->params.zme_params.zme_dering_blend_alpha = param->zme.dering_blend_alpha;
        ctx->params.zme_params.zme_dering_blend_beta = param->zme.dering_blend_beta;
        break;

    case VDPP_PARAM_TYPE_ZME_COEFF :
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
        break;

    default:
        break;
    }

    return ret;
}

static RK_S32 check_cap(struct vdpp_params *params)
{
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_U32 vep_mode_check = 0;

    if (NULL == params) {
        VDPP_DBG(VDPP_DBG_CHECK, "found null pointer params\n");
        return VDPP_CAP_UNSUPPORTED;
    }

    if ((params->src_height < VDPP_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP_MODE_MIN_WIDTH)) {
        VDPP_DBG(VDPP_DBG_CHECK, "vep src unsupported img_w %d img_h %d\n",
                 params->src_height, params->src_width);
        vep_mode_check++;
    }

    if ((params->dst_height < VDPP_MODE_MIN_HEIGHT) ||
        (params->dst_width < VDPP_MODE_MIN_WIDTH)) {
        VDPP_DBG(VDPP_DBG_CHECK, "vep dst unsupported img_w %d img_h %d\n",
                 params->dst_height, params->dst_width);
        vep_mode_check++;
    }

    if ((params->src_width & 1) || (params->src_height & 1) ||
        (params->dst_width & 1) || (params->dst_height & 1)) {
        VDPP_DBG(VDPP_DBG_CHECK, "vep only support img_w/h_vld 2pix align\n");
        VDPP_DBG(VDPP_DBG_CHECK, "vep unsupported img_w_i %d img_h_i %d img_w_o %d img_h_o %d\n",
                 params->src_width, params->src_height, params->dst_width, params->dst_height);
        vep_mode_check++;
    }

    if (!vep_mode_check) {
        ret_cap |= VDPP_CAP_VEP;
        VDPP_DBG(VDPP_DBG_INT, "vdpp support mode: VDPP_CAP_VEP\n");
    }

    return ret_cap;
}

static MPP_RET vdpp_start(struct vdpp_api_ctx *ctx)
{
    MPP_RET ret;
    RegOffsetInfo reg_off[2];
    MppReqV1 mpp_req[9];
    RK_U32 req_cnt = 0;
    struct vdpp_reg *reg = NULL;
    struct zme_reg *zme = NULL;
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;

    if (NULL == ctx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    ret_cap = check_cap(&ctx->params);
    if (!(ret_cap & VDPP_CAP_VEP)) {
        mpp_err_f("found incompat work mode %s cap %d\n",
                  working_mode_name[VDPP_WORK_MODE_VEP], ret_cap);
        return MPP_NOK;
    }

    reg = &ctx->reg;
    zme = &ctx->zme;

    memset(reg_off, 0, sizeof(reg_off));
    memset(mpp_req, 0, sizeof(mpp_req));
    memset(reg, 0, sizeof(*reg));

    vdpp_params_to_reg(&ctx->params, ctx);

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

    /* set reg offset */
    reg_off[0].reg_idx = 25;
    reg_off[0].offset = ctx->params.src.cbcr_offset;
    reg_off[1].reg_idx = 27;
    reg_off[1].offset = ctx->params.dst.cbcr_offset;
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

static MPP_RET vdpp_wait(struct vdpp_api_ctx *ctx)
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

static MPP_RET vdpp_done(struct vdpp_api_ctx *ctx)
{
    struct vdpp_reg *reg = NULL;

    if (NULL == ctx) {
        mpp_err_f("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    reg = &ctx->reg;

    VDPP_DBG(VDPP_DBG_INT, "ro_frm_done_sts=%d\n", reg->common.reg10.ro_frm_done_sts);
    VDPP_DBG(VDPP_DBG_INT, "ro_osd_max_sts=%d\n", reg->common.reg10.ro_osd_max_sts);
    VDPP_DBG(VDPP_DBG_INT, "ro_bus_error_sts=%d\n", reg->common.reg10.ro_bus_error_sts);
    VDPP_DBG(VDPP_DBG_INT, "ro_timeout_sts=%d\n", reg->common.reg10.ro_timeout_sts);
    VDPP_DBG(VDPP_DBG_INT, "ro_config_error_sts=%d\n", reg->common.reg10.ro_timeout_sts);

    if (reg->common.reg8.sw_vdpp_frm_done_en &&
        !reg->common.reg10.ro_frm_done_sts) {
        mpp_err_f("run vdpp failed\n");
        return MPP_NOK;
    }

    VDPP_DBG(VDPP_DBG_INT, "run vdpp success\n");

    return MPP_OK;
}

static inline MPP_RET set_addr(struct vdpp_addr *addr, VdppImg *img)
{
    if (NULL == addr || NULL == img) {
        mpp_err_f("found NULL vdpp_addr %p img %p\n", addr, img);
        return MPP_ERR_NULL_PTR;
    }

    addr->y = img->mem_addr;
    addr->cbcr = img->uv_addr;
    addr->cbcr_offset = img->uv_off;

    return MPP_OK;
}

MPP_RET vdpp_control(VdppCtx ictx, VdppCmd cmd, void *iparam)
{
    struct vdpp_api_ctx *ctx = ictx;
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
    case VDPP_CMD_SET_ZME_COEFF_CFG: {
        struct vdpp_api_params *param = (struct vdpp_api_params *)iparam;

        ret = vdpp_set_param(ctx, &param->param, param->ptype);
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
    case VDPP_CMD_RUN_SYNC:
        ret = vdpp_start(ctx);
        if (ret) {
            mpp_err_f("run vdpp failed\n");
            return MPP_NOK;
        }

        vdpp_wait(ctx);
        vdpp_done(ctx);
        break;
    default:
        ;
    }

    return ret;
}

RK_S32 vdpp_check_cap(VdppCtx ictx)
{
    struct vdpp_api_ctx *ctx = ictx;

    if (NULL == ictx) {
        mpp_err_f("found NULL ctx %p\n", ictx);
        return VDPP_CAP_UNSUPPORTED;
    }

    return check_cap(&ctx->params);
}
