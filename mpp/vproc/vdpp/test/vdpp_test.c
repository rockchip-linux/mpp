/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp_test"

#include <string.h>

#include "mpp_common.h"
#include "mpp_buffer.h"

#include "utils.h"
#include "vdpp_api.h"

#define MAX_URL_LEN                 (256)

typedef struct VdppTestCfg_t {
    RK_S32      src_width;
    RK_S32      src_height;
    RK_S32      src_width_vir;  // 16 align
    RK_S32      src_height_vir; // 8 align
    RK_S32      src_swa;

    RK_S32      dst_width;
    RK_S32      dst_height;
    RK_S32      dst_width_vir;  // 16 align
    RK_S32      dst_height_vir; // 2 align
    RK_S32      dst_fmt;
    RK_S32      dst_swa;

    char        src_url[MAX_URL_LEN];
    char        dst_url[MAX_URL_LEN];

    FILE        *fp_src;
    FILE        *fp_dst;

    RK_U32      frame_num;
} VdppTestCfg;

static OptionInfo vdpp_test_cmd[] = {
    {"w",   "src_width",        "input image width"},
    {"h",   "src_height",       "input image height"},
    {"s",   "src_swap",         "input image UV swap"},
    {"i",   "src_file",         "input image file name"},
    {"W",   "dst_width",        "output image width"},
    {"H",   "dst_height",       "output image height"},
    {"F",   "dst_format",       "output image format in ASCII string"},
    {"S",   "dst_swap",         "output image UV swap"},
    {"o",   "dst_file",         "output image file name"},
    {"n",   "frame_num",        "frame number"}
};

static void vdpp_test_help()
{
    mpp_log("usage: vdpp_test [options]\n");
    mpp_log("*******************************\n");
    show_options(vdpp_test_cmd);
    mpp_log("*******************************\n");
    mpp_log("supported ASCII format strings:\n");
    mpp_log("1 - yuv444\n");
    mpp_log("2 - yuv420 \n");
    mpp_log("************ sample ***********\n");
    mpp_log("vdpp_test -w 720 -h 480 -s 0 -i input.yuv -W 1920 -H 1080 -F yuv444 -S 0 -o output.yuv\n");
}

static RK_S32 str_to_vdpp_fmt(const char *str)
{
    RK_S32 fmt = -1;

    mpp_log("format %s\n", str);

    if (!strcmp(str, "yuv420"))
        fmt = VDPP_FMT_YUV420;
    else if (!strcmp(str, "yuv444"))
        fmt = VDPP_FMT_YUV444;
    else
        mpp_err("invalid format %s\n", str);

    return fmt;
}

static MPP_RET check_input_cmd(VdppTestCfg *cfg)
{
    MPP_RET ret = MPP_OK;

    if (cfg->fp_src == NULL) {
        mpp_err("failed to open input file %s\n", cfg->src_url);
        ret = MPP_NOK;
    }

    return ret;
}

static inline size_t get_frm_size(RK_S32 fmt, RK_U32 w, RK_U32 h)
{
    switch (fmt) {
    case VDPP_FMT_YUV444:
        return w * h * 3;
    case VDPP_FMT_YUV420:
        return w * h * 3 / 2;
    default:
        mpp_err("warning: unsupported input format %d", fmt);
        return 0;
    }
}

static void vdpp_test_set_img(vdpp_com_ctx *ctx, RK_U32 w, RK_U32 h,
                              VdppImg *img, RK_S32 fd, VdppCmd cmd)
{
    RK_S32 y_size = w * h;

    img->mem_addr = fd;
    img->uv_addr = fd;
    img->uv_off = y_size;

    MPP_RET ret = ctx->ops->control(ctx->priv, cmd, img);
    if (ret)
        mpp_log_f("control %08x failed %d\n", cmd, ret);
}

void vdpp_test(VdppTestCfg *cfg)
{
    vdpp_com_ctx* vdpp = rockchip_vdpp_api_alloc_ctx();
    size_t srcfrmsize = get_frm_size(VDPP_FMT_YUV420, cfg->src_width_vir, cfg->src_height_vir);
    size_t dstfrmsize = get_frm_size(cfg->dst_fmt, cfg->dst_width_vir, cfg->dst_height_vir);
    MppBuffer srcbuf;
    MppBuffer dstbuf;
    RK_U8 *psrc;
    RK_U8 *pdst;
    RK_S32 fdsrc;
    RK_S32 fddst;
    struct vdpp_api_params params;

    VdppImg imgsrc;
    VdppImg imgdst;

    mpp_assert(vdpp);
    MppBufferGroup memGroup;
    MPP_RET ret = MPP_NOK;
    RK_U32 cnt = 0;

    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        mpp_err("memGroup mpp_buffer_group_get failed\n");
        mpp_assert(0);
    }

    mpp_buffer_get(memGroup, &srcbuf, srcfrmsize);
    mpp_buffer_get(memGroup, &dstbuf, dstfrmsize);
    mpp_assert(srcbuf && dstbuf);

    psrc = mpp_buffer_get_ptr(srcbuf);
    pdst = mpp_buffer_get_ptr(dstbuf);
    fdsrc = mpp_buffer_get_fd(srcbuf);
    fddst = mpp_buffer_get_fd(dstbuf);
    vdpp->ops->init(&vdpp->priv);

    /* use default dmsr and zme params */
    /* set common params */
    params.ptype = VDPP_PARAM_TYPE_COM;
    params.param.com.src_width = cfg->src_width;
    params.param.com.src_height = cfg->src_height;
    params.param.com.sswap = cfg->src_swa;
    params.param.com.dfmt = cfg->dst_fmt;
    params.param.com.dst_width = cfg->dst_width;
    params.param.com.dst_height = cfg->dst_height;
    params.param.com.dswap = cfg->dst_swa;
    vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_COM_CFG, &params);

    while (1) {
        if (srcfrmsize > fread(psrc, 1, srcfrmsize, cfg->fp_src)) {
            mpp_log("source exhaused\n");
            break;
        }
        if (cnt >= cfg->frame_num)
            break;

        /* notice the order of the input frames */
        vdpp_test_set_img(vdpp, cfg->src_width_vir, cfg->src_height_vir,
                          &imgsrc, fdsrc, VDPP_CMD_SET_SRC);
        vdpp_test_set_img(vdpp, cfg->dst_width_vir, cfg->dst_height_vir,
                          &imgdst, fddst, VDPP_CMD_SET_DST);

        memset(pdst, 0, dstfrmsize);
        vdpp->ops->control(vdpp->priv, VDPP_CMD_RUN_SYNC, NULL);
        cnt ++;

        if (dstfrmsize > fwrite(pdst, 1, dstfrmsize, cfg->fp_dst)) {
            mpp_err("destination dump failed\n");
            break;
        }

    }

    mpp_buffer_put(srcbuf);
    mpp_buffer_put(dstbuf);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    vdpp->ops->deinit(vdpp->priv);

    rockchip_vdpp_api_release_ctx(vdpp);
}

int32_t main(int32_t argc, char **argv)
{
    VdppTestCfg cfg;
    int32_t ch;

    if (argc < 2) {
        vdpp_test_help();
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.src_swa = VDPP_YUV_SWAP_SP_UV;
    cfg.dst_fmt = VDPP_FMT_YUV444;
    cfg.dst_swa = VDPP_YUV_SWAP_SP_UV;

    /* get options */
    opterr = 0;
    while ((ch = getopt(argc, argv, "w:h:s:i:W:H:F:S:o:n:")) != -1) {
        switch (ch) {
        case 'w': {
            cfg.src_width = atoi(optarg);
        } break;
        case 'h': {
            cfg.src_height = atoi(optarg);
        } break;
        case 's': {
            cfg.src_swa = atoi(optarg);
        } break;
        case 'i': {
            mpp_log("input filename: %s\n", optarg);
            strncpy(cfg.src_url, optarg, sizeof(cfg.src_url));
            cfg.fp_src = fopen(cfg.src_url, "rb");
        } break;
        case 'W': {
            cfg.dst_width = atoi(optarg);
        } break;
        case 'H': {
            cfg.dst_height = atoi(optarg);
        } break;
        case 'F': {
            cfg.dst_fmt = str_to_vdpp_fmt(optarg);
        } break;
        case 'S': {
            cfg.dst_swa = atoi(optarg);
        } break;
        case 'o': {
            mpp_log("output filename: %s\n", optarg);
            strncpy(cfg.dst_url, optarg, sizeof(cfg.dst_url));
            cfg.fp_dst = fopen(cfg.dst_url, "w+b");
        } break;
        case 'n': {
            cfg.frame_num = atoi(optarg);
        } break;
        default: {
        } break;
        }
    }
    cfg.src_width_vir = MPP_ALIGN(cfg.src_width, 16);
    cfg.src_height_vir = MPP_ALIGN(cfg.src_height, 8);
    cfg.dst_width_vir = MPP_ALIGN(cfg.dst_width, 16);
    cfg.dst_height_vir = MPP_ALIGN(cfg.dst_height, 2);

    if (check_input_cmd(&cfg)) {
        mpp_err("failed to pass cmd line check\n");
        vdpp_test_help();
        return -1;
    }

    vdpp_test(&cfg);

    if (cfg.fp_src) {
        fclose(cfg.fp_src);
        cfg.fp_src = NULL;
    }

    if (cfg.fp_dst) {
        fclose(cfg.fp_dst);
        cfg.fp_dst = NULL;
    }

    return 0;
}
