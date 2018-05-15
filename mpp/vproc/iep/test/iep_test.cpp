/*
 * Copyright 2018 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define MODULE_TAG "iep_test"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_buffer.h"

#include "iep_api.h"
#include "utils.h"
#include "iniparser.h"

#define MAX_URL_LEN                 (100)
#define ADD_OFFSET(base, offset)    ((RK_U32)base + ((RK_U32)(offset) << 10))

typedef enum {
    TEST_CASE_NONE,
    TEST_CASE_DEINTERLACE,
    TEST_CASE_YUVENHANCE,
    TEST_CASE_RGBENHANCE,
} TEST_CASE;

typedef struct IepTestCfg_t {
    RK_S32      src_w;
    RK_S32      src_h;
    RK_S32      src_fmt;

    RK_S32      dst_w;
    RK_S32      dst_h;
    RK_S32      dst_fmt;

    char        src_url[MAX_URL_LEN];
    char        dst_url[MAX_URL_LEN];
    char        cfg_url[MAX_URL_LEN];

    FILE        *fp_src;
    FILE        *fp_dst;

    TEST_CASE   mode;

    dictionary  *ini;
    IepCmdParamDeiCfg       *dei_cfg;
    IepCmdParamYuvEnhance   *yuv_enh;
    IepCmdParamRgbEnhance   *rgb_enh;
    IepCmdParamColorConvert *colorcvt;
} IepTestCfg;

typedef struct IepTestInfo_t {
    IepTestCfg *cfg;

    size_t src_size;
    size_t dst_size;

    RK_U32 phy_src0;
    RK_U32 phy_src1;
    RK_U32 phy_dst0;
    RK_U32 phy_dst1;

    RK_U8 *vir_src0;
    RK_U8 *vir_src1;
    RK_U8 *vir_dst0;
    RK_U8 *vir_dst1;

    IepImg src;
    IepImg dst;

    IepCtx                  ctx;
} IepTestInfo;

static void parse_cfg(IepTestCfg *cfg)
{
    dictionary *ini = cfg->ini;

    cfg->dei_cfg = mpp_calloc(IepCmdParamDeiCfg, 1);
    if (iniparser_find_entry(ini, "deinterlace")) {
        mpp_assert(cfg->dei_cfg);
        cfg->dei_cfg->dei_high_freq_en =
            iniparser_getint(ini, "deinterlace:high_freq_en", 0);
        cfg->dei_cfg->dei_mode =
            (IepDeiMode)iniparser_getint(ini, "deinterlace:dei_mode", 0);
        cfg->dei_cfg->dei_high_freq_fct =
            iniparser_getint(ini, "deinterlace:dei_high_freq_fct", 0);
        cfg->dei_cfg->dei_ei_mode =
            iniparser_getint(ini, "deinterlace:dei_ei_mode", 0);
        cfg->dei_cfg->dei_ei_smooth =
            iniparser_getint(ini, "deinterlace:dei_ei_smooth", 0);
        cfg->dei_cfg->dei_ei_sel =
            iniparser_getint(ini, "deinterlace:dei_ei_sel", 0);
        cfg->dei_cfg->dei_ei_radius =
            iniparser_getint(ini, "deinterlace:dei_ei_radius", 0);
    } else {
        cfg->dei_cfg->dei_high_freq_en  = 1;
        cfg->dei_cfg->dei_mode          = IEP_DEI_MODE_I4O1;
        cfg->dei_cfg->dei_field_order   = IEP_DEI_FLD_ORDER_BOT_FIRST;
        cfg->dei_cfg->dei_ei_mode       = 0;
        cfg->dei_cfg->dei_ei_smooth     = 0;
        cfg->dei_cfg->dei_ei_sel        = 0;
        cfg->dei_cfg->dei_ei_radius     = 0;
    }

    if (iniparser_find_entry(ini, "yuv enhance")) {
        cfg->yuv_enh = mpp_calloc(IepCmdParamYuvEnhance, 1);
        cfg->yuv_enh->saturation =
            iniparser_getint(ini, "yuv enhance:saturation", 0);
        cfg->yuv_enh->contrast =
            iniparser_getint(ini, "yuv enhance:contrast", 0);
        cfg->yuv_enh->brightness =
            iniparser_getint(ini, "yuv enhance:brightness", 0);
        cfg->yuv_enh->hue_angle =
            iniparser_getint(ini, "yuv enhance:hue_angle", 0);
        cfg->yuv_enh->video_mode =
            (IepVideoMode)
            iniparser_getint(ini, "yuv enhance:video_mode", 0);
        cfg->yuv_enh->color_bar_y =
            iniparser_getint(ini, "yuv enhance:color_bar_y", 0);
        cfg->yuv_enh->color_bar_u =
            iniparser_getint(ini, "yuv enhance:color_bar_u", 0);
        cfg->yuv_enh->color_bar_v =
            iniparser_getint(ini, "yuv enhance:color_bar_v", 0);
    }

    if (iniparser_find_entry(ini, "rgb enhance")) {
        cfg->rgb_enh = mpp_calloc(IepCmdParamRgbEnhance, 1);
        cfg->rgb_enh->coe =
            iniparser_getint(ini, "rgb enhance:coe", 0);
        cfg->rgb_enh->mode =
            (IepRgbEnhanceMode)
            iniparser_getint(ini, "rgb enhance:mode", 0);
        cfg->rgb_enh->cg_en =
            iniparser_getint(ini, "rgb enhance:cg_en", 0);
        cfg->rgb_enh->cg_rr =
            iniparser_getint(ini, "rgb enhance:cg_rr", 0);
        cfg->rgb_enh->cg_rg =
            iniparser_getint(ini, "rgb enhance:cg_rg", 0);
        cfg->rgb_enh->cg_rb =
            iniparser_getint(ini, "rgb enhance:cg_rb", 0);
        cfg->rgb_enh->order =
            (IepRgbEnhanceOrder)
            iniparser_getint(ini, "rgb enhance:order", 0);
        cfg->rgb_enh->threshold =
            iniparser_getint(ini, "rgb enhance:threshold", 0);
        cfg->rgb_enh->alpha_num =
            iniparser_getint(ini, "rgb enhance:alpha_num", 0);
        cfg->rgb_enh->alpha_base =
            iniparser_getint(ini, "rgb enhance:alpha_base", 0);
        cfg->rgb_enh->radius =
            iniparser_getint(ini, "rgb enhance:radius", 0);
    }

    if (iniparser_find_entry(ini, "colorcvt")) {
        cfg->colorcvt = mpp_calloc(IepCmdParamColorConvert, 1);
        cfg->colorcvt->rgb2yuv_mode =
            (IepColorConvertMode)
            iniparser_getint(ini, "colorcvt:rgb2yuv_mode", 0);
        cfg->colorcvt->yuv2rgb_mode =
            (IepColorConvertMode)
            iniparser_getint(ini, "colorcvt:yuv2rgb_mode", 0);
        cfg->colorcvt->rgb2yuv_input_clip   =
            iniparser_getint(ini, "colorcvt:rgb2yuv_input_clip", 0);
        cfg->colorcvt->yuv2rgb_input_clip   =
            iniparser_getint(ini, "colorcvt:yuv2rgb_input_clip", 0);
        cfg->colorcvt->global_alpha_value   =
            iniparser_getint(ini, "colorcvt:global_alpha_value", 0);
        cfg->colorcvt->dither_up_en         =
            iniparser_getint(ini, "colorcvt:dither_up_en", 0);
        cfg->colorcvt->dither_down_en       =
            iniparser_getint(ini, "colorcvt:dither_down_en", 0);
    }
}

static RK_S32 get_image_size(RK_S32 w, RK_S32 h, RK_S32 fmt)
{
    RK_S32 img_size = 0;

    switch (fmt) {
    case IEP_FORMAT_ABGR_8888:
    case IEP_FORMAT_ARGB_8888:
    case IEP_FORMAT_BGRA_8888:
    case IEP_FORMAT_RGBA_8888:
        img_size = w * h * 4;
        break;
    case IEP_FORMAT_BGR_565:
    case IEP_FORMAT_RGB_565:
    case IEP_FORMAT_YCbCr_422_P:
    case IEP_FORMAT_YCbCr_422_SP:
    case IEP_FORMAT_YCrCb_422_P:
    case IEP_FORMAT_YCrCb_422_SP:
        img_size = w * h * 2;
        break;
    case IEP_FORMAT_YCbCr_420_P:
    case IEP_FORMAT_YCbCr_420_SP:
    case IEP_FORMAT_YCrCb_420_P:
    case IEP_FORMAT_YCrCb_420_SP:
        img_size = w * h * 3 / 2;
        break;
    default : {
    } break;
    }
    return img_size;
}

static void config_iep_img(IepImg *img, RK_S32 w, RK_S32 h, RK_S32 fmt, RK_U32 base)
{
    switch (fmt) {
    case IEP_FORMAT_ABGR_8888:
    case IEP_FORMAT_ARGB_8888:
    case IEP_FORMAT_BGRA_8888:
    case IEP_FORMAT_RGBA_8888:
        img->v_addr = 0;
        break;
    case IEP_FORMAT_BGR_565:
    case IEP_FORMAT_RGB_565:
    case IEP_FORMAT_YCbCr_422_P:
    case IEP_FORMAT_YCbCr_422_SP:
    case IEP_FORMAT_YCrCb_422_P:
    case IEP_FORMAT_YCrCb_422_SP:
        img->v_addr = ADD_OFFSET(base, w * h + w * h / 2);
        break;
    case IEP_FORMAT_YCbCr_420_P:
    case IEP_FORMAT_YCbCr_420_SP:
    case IEP_FORMAT_YCrCb_420_P:
    case IEP_FORMAT_YCrCb_420_SP:
        img->v_addr = ADD_OFFSET(base, w * h + w * h / 4);
        break;
    default : {
    } break;
    }

    img->act_w      = w;
    img->act_h      = h;
    img->x_off      = 0;
    img->y_off      = 0;
    img->vir_w      = w;
    img->vir_h      = h;
    img->format     = fmt;
    img->mem_addr   = base;
    img->uv_addr    = ADD_OFFSET(base, w * h);
}

void* iep_process_thread(void *param)
{
    IepTestInfo *info = (IepTestInfo*)param;
    IepTestCfg *cfg = info->cfg;

    config_iep_img(&info->src, cfg->src_w, cfg->src_h, cfg->src_fmt, info->phy_src0);
    config_iep_img(&info->dst, cfg->dst_w, cfg->dst_h, cfg->dst_fmt, info->phy_dst0);

    if (info->src_size > fread(info->vir_src0, 1, info->src_size, cfg->fp_src)) {
        mpp_err("read file failed\n");
        return NULL;
    }

    iep_control(info->ctx, IEP_CMD_INIT, NULL);
    iep_control(info->ctx, IEP_CMD_SET_SRC, &info->src);
    iep_control(info->ctx, IEP_CMD_SET_DST, &info->dst);

    switch (cfg->mode) {
    case TEST_CASE_YUVENHANCE: {
        iep_control(info->ctx, IEP_CMD_SET_YUV_ENHANCE, cfg->yuv_enh);
    } break;
    case TEST_CASE_RGBENHANCE: {
        iep_control(info->ctx, IEP_CMD_SET_RGB_ENHANCE, cfg->rgb_enh);
    } break;
    case TEST_CASE_DEINTERLACE: {
        IepImg src1;
        IepImg dst1;

        if (info->src_size != fread(info->vir_src1, 1, info->src_size, cfg->fp_src))
            mpp_err("failed to read %d from input\n", info->src_size);

        config_iep_img(&src1, cfg->src_w, cfg->src_h, cfg->src_fmt, info->phy_src1);
        config_iep_img(&dst1, cfg->src_w, cfg->src_h, cfg->src_fmt, info->phy_dst1);

        iep_control(info->ctx, IEP_CMD_SET_DEI_SRC1, &src1);
        iep_control(info->ctx, IEP_CMD_SET_DEI_DST1, &dst1);
        iep_control(info->ctx, IEP_CMD_SET_DEI_CFG, cfg->dei_cfg);
    } break;
    default: {
    } break;
    }

    RK_S64 intime = mpp_time();
    if (0 == iep_control(info->ctx, IEP_CMD_RUN_SYNC, NULL))
        mpp_log("%d success\n", getpid());
    else
        mpp_log("%d failure\n", getpid());

    mpp_log("%s consume %lld ms\n", __func__, (mpp_time() - intime) / 1000);

    if (cfg->fp_dst)
        fwrite(info->vir_dst0, 1, info->dst_size, cfg->fp_dst);

    return NULL;
}

static RK_S32 str_to_iep_fmt(const char *str)
{
    RK_S32 fmt = -1;

    if (!strcmp(str, "argb8888"))
        fmt = IEP_FORMAT_ARGB_8888;
    else if (!strcmp(str, "abgr8888"))
        fmt = IEP_FORMAT_ABGR_8888;
    else if (!strcmp(str, "rgba8888"))
        fmt = IEP_FORMAT_BGRA_8888;
    else if (!strcmp(str, "bgra8888"))
        fmt = IEP_FORMAT_BGRA_8888;
    else if (!strcmp(str, "rgb565"))
        fmt = IEP_FORMAT_RGB_565;
    else if (!strcmp(str, "bgr565"))
        fmt = IEP_FORMAT_BGR_565;
    else if (!strcmp(str, "yuv422sp"))
        fmt = IEP_FORMAT_YCbCr_422_SP;
    else if (!strcmp(str, "yuv422p"))
        fmt = IEP_FORMAT_YCbCr_422_P;
    else if (!strcmp(str, "yuv420sp"))
        fmt = IEP_FORMAT_YCbCr_420_SP;
    else if (!strcmp(str, "yuv420p"))
        fmt = IEP_FORMAT_YCbCr_420_P;
    else if (!strcmp(str, "yvu422sp"))
        fmt = IEP_FORMAT_YCrCb_422_SP;
    else if (!strcmp(str, "yvu422p"))
        fmt = IEP_FORMAT_YCrCb_422_P;
    else if (!strcmp(str, "yvu420sp"))
        fmt = IEP_FORMAT_YCrCb_420_SP;
    else if (!strcmp(str, "yvu420p"))
        fmt = IEP_FORMAT_YCrCb_420_P;
    else
        mpp_err("invalid format %s\n", str);

    return fmt;
}

static MPP_RET check_input_cmd(IepTestCfg *cfg)
{
    MPP_RET ret = MPP_OK;

    if (cfg->src_w <= 0) {
        mpp_err("non-positive input width %d\n", cfg->src_w);
        ret = MPP_NOK;
    }
    if (cfg->src_h <= 0) {
        mpp_err("non-positive input height %d\n", cfg->src_h);
        ret = MPP_NOK;
    }
    if (cfg->src_fmt < IEP_FORMAT_ARGB_8888 ||
        cfg->src_fmt > IEP_FORMAT_YCrCb_420_P) {
        mpp_err("invalid input format\n");
        ret = MPP_NOK;
    }
    if (cfg->fp_src == NULL) {
        mpp_err("failed to open input file %s\n", cfg->src_url);
        ret = MPP_NOK;
    }
    if (cfg->dst_w <= 0) {
        mpp_err("non-positive input width %d\n", cfg->dst_w);
        ret = MPP_NOK;
    }
    if (cfg->dst_h <= 0) {
        mpp_err("non-positive input height %d\n", cfg->dst_h);
        ret = MPP_NOK;
    }
    if (cfg->dst_fmt < IEP_FORMAT_ARGB_8888 ||
        cfg->dst_fmt > IEP_FORMAT_YCrCb_420_P) {
        mpp_err("invalid input format\n");
        ret = MPP_NOK;
    }
    if (cfg->mode == TEST_CASE_NONE) {
        mpp_err("invalid work mode\n");
        ret = MPP_NOK;
    }

    return ret;
}

static OptionInfo iep_test_cmd[] = {
    {"w",   "src_width",            "input  image width"},
    {"h",   "src_height",           "input  image height"},
    {"c",   "src_format",           "input  image format in ASCII string"},
    {"f",   "src_file",             "input  image file name"},
    {"W",   "dst_width",            "output image width"},
    {"H",   "dst_height",           "output image height"},
    {"C",   "dst_format",           "output image format in ASCII string"},
    {"F",   "dst_file",             "output image file name"},
    {"m",   "work_mode",            "working mode of iep in ASCII string"},
    {"x",   "configure",            "configure file of working mode"},
};

static void iep_test_help()
{
    mpp_log("usage: iep_test [options]\n");
    mpp_log("*******************************\n");
    show_options(iep_test_cmd);
    mpp_log("*******************************\n");
    mpp_log("supported ASCII format strings:\n");
    mpp_log(" 0 - argb8888\n");
    mpp_log(" 1 - abgr8888\n");
    mpp_log(" 2 - rgba8888\n");
    mpp_log(" 3 - bgra8888\n");
    mpp_log(" 4 - rgb565\n");
    mpp_log(" 5 - bgr565\n");
    mpp_log(" 6 - yuv422sp\n");
    mpp_log(" 7 - yuv422p\n");
    mpp_log(" 8 - yuv420sp\n");
    mpp_log(" 9 - yuv420p\n");
    mpp_log("10 - yvu422sp\n");
    mpp_log("11 - yvu422p\n");
    mpp_log("12 - yvu420sp\n");
    mpp_log("13 - yvu420p\n");
    mpp_log("*******************************\n");
    mpp_log("supported work mode strings:\n");
    mpp_log(" 1 - deinterlace\n");
    mpp_log(" 2 - yuvenhance\n");
    mpp_log(" 3 - rgbenhance\n");
    mpp_log("************ sample ***********\n");
    mpp_log("iep_test -w 720 -h 480 -c yuv420p -f input.yuv -W 720 -H 480 -C yuv420p -F output.yuv -m deinterlace -x dil.cfg\n");
}

int main(int argc, char **argv)
{
    IepTestCfg cfg;
    IepTestInfo info;
    int ch;

    if (argc < 2) {
        iep_test_help();
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    memset(&info, 0, sizeof(info));
    cfg.src_fmt = IEP_FORMAT_YCbCr_420_SP;
    cfg.dst_fmt = IEP_FORMAT_YCbCr_420_SP;

    /// get options
    opterr = 0;
    while ((ch = getopt(argc, argv, "f:w:h:c:F:W:H:C:m:x:")) != -1) {
        switch (ch) {
        case 'w': {
            cfg.src_w = atoi(optarg);
        } break;
        case 'h': {
            cfg.src_h = atoi(optarg);
        } break;
        case 'c': {
            cfg.src_fmt = str_to_iep_fmt(optarg);
        } break;
        case 'W': {
            cfg.dst_w = atoi(optarg);
        } break;
        case 'H': {
            cfg.dst_h = atoi(optarg);
        } break;
        case 'C': {
            cfg.dst_fmt = str_to_iep_fmt(optarg);
        } break;
        case 'f': {
            mpp_log("input filename: %s\n", optarg);
            strncpy(cfg.src_url, optarg, sizeof(cfg.src_url));
            cfg.fp_src = fopen(cfg.src_url, "rb");
        } break;
        case 'F': {
            mpp_log("output filename: %s\n", optarg);
            strncpy(cfg.dst_url, optarg, sizeof(cfg.dst_url));
            cfg.fp_dst = fopen(cfg.dst_url, "w+b");
        } break;
        case 'm': {
            if (!strcmp(optarg, "deinterlace")) {
                cfg.mode = TEST_CASE_DEINTERLACE;
            } else if (!strcmp(optarg, "yuvenhance")) {
                cfg.mode = TEST_CASE_YUVENHANCE;
            } else if (!strcmp(optarg, "rgbenhance")) {
                cfg.mode = TEST_CASE_RGBENHANCE;
            } else {
                mpp_err("invalid work mode %s\n", optarg);
                cfg.mode = TEST_CASE_NONE;
            }
        } break;
        case 'x': {
            mpp_log("configure filename: %s\n", optarg);
            strncpy(cfg.cfg_url, optarg, sizeof(cfg.cfg_url));
            cfg.ini = iniparser_load(cfg.cfg_url);
            if (cfg.ini)
                parse_cfg(&cfg);
            else
                mpp_err("invalid configure file %s\n", optarg);
        } break;
        default: {
        } break;
        }
    }

    if (check_input_cmd(&cfg)) {
        mpp_err("failed to pass cmd line check\n");
        iep_test_help();
        return -1;
    }

    info.cfg = &cfg;
    info.src_size = get_image_size(cfg.src_w, cfg.src_h, cfg.src_fmt);
    info.dst_size = get_image_size(cfg.dst_w, cfg.dst_h, cfg.dst_fmt);

    // allocate 12M in/out memory and initialize memory address
    MppBuffer buffer = NULL;
    size_t src_size = MPP_ALIGN(info.src_size, SZ_4K);
    size_t dst_size = MPP_ALIGN(info.dst_size, SZ_4K);
    // NOTE: deinterlace need 4 fields (2 frames) as input and output 2 frames
    size_t total_size = src_size * 2 + dst_size * 2;

    mpp_buffer_get(NULL, &buffer, total_size);
    mpp_assert(buffer);

    int buf_fd = mpp_buffer_get_fd(buffer);
    RK_U8 *buf_ptr = (RK_U8 *)mpp_buffer_get_ptr(buffer);

    memset(buf_ptr, 0xff, total_size);

    info.phy_src0 = ADD_OFFSET(buf_fd, 0);
    info.phy_src1 = ADD_OFFSET(buf_fd, src_size);
    info.phy_dst0 = ADD_OFFSET(buf_fd, src_size * 2);
    info.phy_dst1 = ADD_OFFSET(buf_fd, src_size * 2 + dst_size);

    info.vir_src0 = buf_ptr;
    info.vir_src1 = buf_ptr + src_size;
    info.vir_dst0 = buf_ptr + src_size * 2;
    info.vir_dst1 = buf_ptr + src_size * 2 + dst_size;

    iep_init(&info.ctx);

    pthread_t td;
    pthread_create(&td, NULL, iep_process_thread, &info);
    pthread_join(td, NULL);

    if (info.ctx) {
        iep_deinit(info.ctx);
        info.ctx = NULL;
    }

    if (cfg.fp_src) {
        fclose(cfg.fp_src);
        cfg.fp_src = NULL;
    }

    if (cfg.fp_dst) {
        fclose(cfg.fp_dst);
        cfg.fp_dst = NULL;
    }

    if (cfg.ini) {
        iniparser_freedict(cfg.ini);
        cfg.ini = NULL;
        if (cfg.yuv_enh)
            mpp_free(cfg.yuv_enh);
        if (cfg.rgb_enh)
            mpp_free(cfg.rgb_enh);
        if (cfg.dei_cfg)
            mpp_free(cfg.dei_cfg);
        if (cfg.colorcvt)
            mpp_free(cfg.colorcvt);
    }

    mpp_buffer_put(buffer);

    return 0;
}
