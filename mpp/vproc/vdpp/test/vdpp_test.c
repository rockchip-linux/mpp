/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp_test"

#include <getopt.h>
#include <string.h>
#include <errno.h>

#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buffer.h"
#include "mpp_soc.h"

#include "utils.h"
#include "vdpp_api.h"

#define MAX_URL_LEN        (256)

#define SHP_CFG_STRIDE     (684)
#define DM_ZME_CFG_STRIDE  (260)
#define ES_CFG_STRIDE      (68)
#define YUV_MAX_SIZE       (12582912) // 4096 * 2048 * 3 / 2
#define DCI_HIST_SIZE      (10240)    // (16*16*16*18/8/4 + 256) * 4

typedef struct VdppTestCfg_t {
    MppFrameFormat src_format;
    RK_U32      src_width;
    RK_U32      src_height;
    RK_U32      src_width_vir;  // 16 align
    RK_U32      src_height_vir; // 8 align
    RK_U32      src_swa;

    RK_U32      dst_width;
    RK_U32      dst_height;
    RK_U32      dst_width_vir;
    RK_U32      dst_height_vir;
    RK_U32      dst_fmt;
    RK_U32      dst_swa;
    RK_U32      yuv_out_diff;
    RK_U32      dst_c_width;
    RK_U32      dst_c_height;
    RK_U32      dst_c_width_vir;
    RK_U32      dst_c_height_vir;
    /* high 16 bit: mask; low 3 bit: dmsr|es|sharp */
    RK_U32      cfg_set;

    char        src_url[MAX_URL_LEN];
    char        dst_url[MAX_URL_LEN];
    char        dst_c_url[MAX_URL_LEN];
    char        hist_url[MAX_URL_LEN];
    char        hist_l_url[MAX_URL_LEN];
    char        hist_g_url[MAX_URL_LEN];
    char        slt_url[MAX_URL_LEN];

    FILE        *fp_src;
    FILE        *fp_dst;
    FILE        *fp_dst_c;
    FILE        *fp_hist;
    FILE        *fp_hist_l;
    FILE        *fp_hist_g;
    FILE        *fp_slt;

    RK_U32      frame_num;

    RK_U32      hist_mode_en;
} VdppTestCfg;

typedef struct {
    MppFrameFormat  format;
    const char      *name;
} MppFrameFormatInfo;

extern void mpp_show_color_format();

static OptionInfo vdpp_test_cmd[] = {
    {"w",         "src_width",        "input image width"},
    {"h",         "src_height",       "input image height"},
    {"s",         "src_swap",         "input image UV swap"},
    {"f",         "src_format",       "input image format in MppFrameFormat"},
    {"i",         "src_file",         "input image file name"},
    {"W",         "dst_width",        "output image width"},
    {"H",         "dst_height",       "output image height"},
    {"F",         "dst_format",       "output image format in ASCII string"},
    {"S",         "dst_swap",         "output image UV swap"},
    {"o",         "dst_file",         "output image file name"},
    {"n",         "frame_num",        "frame number"},
    {"m",         "work_mode",        "work_mode : 0 - vdpp, 1 - dci hist"},
    {"Y",         "packed_hist",      "output packed hist file name"},
    {"O",         "dst_chroma_file",  "output chroma file name"},
    {"wi_vir",    "src_width_vir",    "input virtual width"},
    {"hi_vir",    "src_height_vir",   "input virtual height"},
    {"wo_vir",    "dst_width_vir",    "output virtual width"},
    {"ho_vir",    "dst_height_vir",   "output virtual height"},
    {"wo_uv",     "dst_c_width",      "output chroma width"},
    {"ho_uv",     "dst_c_height",     "output chroma height"},
    {"wo_uv_vir", "dst_c_width_vir",  "output virtual chroma width"},
    {"ho_uv_vir", "dst_c_height_vir", "output virtual chroma height"},
    {"diff",      "yuv_out_diff",     "luma and chroma diff resolution flag"},
    {"cfg_set",   "cfg_set",          "high 16 bit: mask; low 3 bit: dmsr|es|sharp"},
    {"hist_l",    "local_hist",       "output local hist file name"},
    {"hist_g",    "global_hist",      "output global hist file name"},
    {"slt",       "slt_file",         "slt verify data file"},
};

static void vdpp_test_help()
{
    mpp_log("usage: vdpp_test [options]\n");
    mpp_log("*******************************\n");
    show_options(vdpp_test_cmd);
    mpp_show_color_format();
    mpp_log("*******************************\n");
    mpp_log("supported ASCII format strings:\n");
    mpp_log("1 - yuv444\n");
    mpp_log("2 - yuv420 \n");
    mpp_log("************ sample ***********\n");
    mpp_log("vdpp_test -w 720 -h 480 -s 0 -i input.yuv -W 1920 -H 1080 -F yuv444 -S 0 -o output.yuv -n 1\n");
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

static inline size_t get_src_frm_size(RK_S32 fmt, RK_U32 w, RK_U32 h)
{
    RK_S32 frame_size;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P: {
        frame_size = w * h * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY :
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP :
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 : {
        frame_size = w * h * 2;
    } break;
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 : {
        frame_size = w * h * 3;
    } break;
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        frame_size = w * h * 4;
    } break;
    default: {
        frame_size = w * h * 4;
    } break;
    }

    return frame_size;
}

static inline size_t get_dst_frm_size(RK_S32 fmt, RK_U32 w, RK_U32 h)
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
    size_t srcfrmsize = get_src_frm_size(cfg->src_format, cfg->src_width_vir, cfg->src_height_vir);
    size_t dstfrmsize = get_dst_frm_size(cfg->dst_fmt, cfg->dst_width_vir, cfg->dst_height_vir);
    size_t dstfrmsize_c = cfg->dst_c_width_vir * cfg->dst_c_height_vir * 3;
    MppBuffer srcbuf;
    MppBuffer dstbuf;
    MppBuffer dstbuf_c;
    MppBuffer histbuf;
    MppBuffer histbuf_l;
    MppBuffer histbuf_g;
    RK_U8 *psrc = NULL;
    RK_U8 *pdst = NULL;
    RK_U8 *pdst_c = NULL;
    RK_U8 *phist = NULL;
    RK_U8 *phist_l = NULL;
    RK_U8 *phist_g = NULL;
    RK_S32 fdsrc = -1;
    RK_S32 fddst = -1;
    RK_S32 fddst_c = -1;
    RK_S32 fdhist = -1;
    struct vdpp_api_params params;
    RK_U32 is_vdpp2 = (mpp_get_soc_type() == ROCKCHIP_SOC_RK3576);
    RK_U32 yuv_out_diff = (cfg->yuv_out_diff && is_vdpp2);

    VdppImg imgsrc;
    VdppImg imgdst;
    VdppImg imgdst_c;

    RK_U32 local_hist_size  = RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI * RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI *
                              RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS * sizeof(RK_U32);
    RK_U32 global_hist_size = RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_NUMS * sizeof(RK_U32);

    mpp_assert(vdpp);
    MppBufferGroup memGroup;
    MPP_RET ret = MPP_NOK;
    RK_U32 cnt = 0;
    DataCrc checkcrc;

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);

    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        mpp_err("memGroup mpp_buffer_group_get failed\n");
        mpp_assert(0);
    }

    mpp_log("src w:h [%d:%d] stride [%d:%d] require buf %d",
            cfg->src_width, cfg->src_height, cfg->src_width_vir, cfg->src_height_vir, srcfrmsize);
    mpp_log("dst w:h [%d:%d] stride [%d:%d] require buf %d",
            cfg->dst_width, cfg->dst_height, cfg->dst_width_vir, cfg->dst_height_vir, dstfrmsize);
    mpp_buffer_get(memGroup, &srcbuf, srcfrmsize);
    mpp_buffer_get(memGroup, &dstbuf, dstfrmsize);
    mpp_buffer_get(memGroup, &histbuf, DCI_HIST_SIZE);
    mpp_assert(srcbuf && dstbuf && histbuf);

    mpp_buffer_get(memGroup, &histbuf_l, local_hist_size);
    mpp_buffer_get(memGroup, &histbuf_g, global_hist_size);
    mpp_assert(histbuf_l && histbuf_g);

    psrc = mpp_buffer_get_ptr(srcbuf);
    pdst = mpp_buffer_get_ptr(dstbuf);
    phist = mpp_buffer_get_ptr(histbuf);
    phist_l = mpp_buffer_get_ptr(histbuf_l);
    phist_g = mpp_buffer_get_ptr(histbuf_g);

    fdsrc = mpp_buffer_get_fd(srcbuf);
    fddst = mpp_buffer_get_fd(dstbuf);
    fdhist = mpp_buffer_get_fd(histbuf);

    if (yuv_out_diff) {
        mpp_log("dst chroma w:h [%d:%d] stride [%d:%d] require buf %d",
                cfg->dst_c_width, cfg->dst_c_height, cfg->dst_c_width_vir, cfg->dst_c_height_vir, dstfrmsize_c);

        mpp_buffer_get(memGroup, &dstbuf_c, dstfrmsize_c);
        mpp_assert(dstbuf_c);

        pdst_c = mpp_buffer_get_ptr(dstbuf_c);
        fddst_c = mpp_buffer_get_fd(dstbuf_c);
    }

    vdpp->ops->init(&vdpp->priv);

    /* use default dmsr and zme params */
    /* set common params */
    if (is_vdpp2) {
        params.ptype = VDPP_PARAM_TYPE_COM2;
        memset(&params.param, 0, sizeof(union vdpp_api_content));
        params.param.com2.sfmt = cfg->src_format;
        params.param.com2.src_width = cfg->src_width;
        params.param.com2.src_height = cfg->src_height;
        params.param.com2.src_width_vir = cfg->src_width_vir;
        params.param.com2.src_height_vir = cfg->src_height_vir;
        params.param.com2.sswap = cfg->src_swa;
        params.param.com2.dfmt = cfg->dst_fmt;
        params.param.com2.dst_width = cfg->dst_width;
        params.param.com2.dst_height = cfg->dst_height;
        params.param.com2.dst_width_vir = cfg->dst_width_vir;
        params.param.com2.dst_height_vir = cfg->dst_height_vir;
        if (yuv_out_diff) {
            params.param.com2.yuv_out_diff = yuv_out_diff;
            params.param.com2.dst_c_width = cfg->dst_c_width;
            params.param.com2.dst_c_height = cfg->dst_c_height;
            params.param.com2.dst_c_width_vir = cfg->dst_c_width_vir;
            params.param.com2.dst_c_height_vir = cfg->dst_c_height_vir;
        }
        params.param.com2.dswap = cfg->dst_swa;
        params.param.com2.hist_mode_en = cfg->hist_mode_en;
        params.param.com2.cfg_set = cfg->cfg_set;
        vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_COM2_CFG, &params);
    } else {
        params.ptype = VDPP_PARAM_TYPE_COM;
        memset(&params.param, 0, sizeof(union vdpp_api_content));
        params.param.com.src_width = cfg->src_width;
        params.param.com.src_height = cfg->src_height;
        params.param.com.sswap = cfg->src_swa;
        params.param.com.dfmt = cfg->dst_fmt;
        params.param.com.dst_width = cfg->dst_width;
        params.param.com.dst_height = cfg->dst_height;
        params.param.com.dswap = cfg->dst_swa;
        vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_COM_CFG, &params);
    }

    {
        RK_S32 cap = vdpp->ops->check_cap(vdpp->priv);

        mpp_log("vdpp cap %d\n", cap);
    }

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
        if (yuv_out_diff)
            vdpp_test_set_img(vdpp, cfg->dst_c_width_vir, cfg->dst_c_height_vir,
                              &imgdst_c, fddst_c, VDPP_CMD_SET_DST_C);

        if (is_vdpp2)
            vdpp->ops->control(vdpp->priv, VDPP_CMD_SET_HIST_FD, &fdhist);

        memset(pdst, 0, dstfrmsize);
        memset(phist, 0, DCI_HIST_SIZE);

        if (vdpp->ops->control(vdpp->priv, VDPP_CMD_RUN_SYNC, NULL)) {
            mpp_err("found vdpp run error, exit!\n");
            break;
        }

        cnt++;

        if (cfg->fp_dst) {
            if (dstfrmsize > fwrite(pdst, 1, dstfrmsize, cfg->fp_dst)) {
                mpp_err("destination dump failed, errno %d %s\n", errno, strerror(errno));
                break;
            }
        }

        if (yuv_out_diff && cfg->fp_dst_c) {
            if (dstfrmsize_c > fwrite(pdst_c, 1, dstfrmsize_c, cfg->fp_dst_c)) {
                mpp_err("chroma dump failed, errno %d %s\n", errno, strerror(errno));
                break;
            }
        }

        if (cfg->fp_hist) {
            if (DCI_HIST_SIZE > fwrite(phist, 1, DCI_HIST_SIZE, cfg->fp_hist)) {
                mpp_err("packed hist dump err\n");
                break;
            }
        }

        memset(phist_l, 0, local_hist_size);
        memset(phist_g, 0, global_hist_size);

        dci_hist_info_parser(phist, (RK_U32 *)phist_l, (RK_U32 *)phist_g);

        if (cfg->fp_hist_l) {
            if (local_hist_size > fwrite(phist_l, 1, local_hist_size, cfg->fp_hist_l)) {
                mpp_err("local hist dump err\n");
                break;
            }
        }
        if (cfg->fp_hist_g) {
            if (global_hist_size > fwrite(phist_g, 1, global_hist_size, cfg->fp_hist_g)) {
                mpp_err("global hist dump err\n");
                break;
            }
        }

        if (cfg->fp_slt) {
            if (pdst && !cfg->hist_mode_en) {
                calc_data_crc(pdst, dstfrmsize, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);

            }

            if (pdst_c && yuv_out_diff && !cfg->hist_mode_en) {
                calc_data_crc(pdst_c, dstfrmsize_c, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);

            }

            if (phist) {
                calc_data_crc(phist, DCI_HIST_SIZE, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);
            }
        }

    }

    mpp_buffer_put(srcbuf);
    mpp_buffer_put(dstbuf);
    mpp_buffer_put(histbuf);
    if (histbuf_l)
        mpp_buffer_put(histbuf_l);
    if (histbuf_g)
        mpp_buffer_put(histbuf_g);
    if (yuv_out_diff)
        mpp_buffer_put(dstbuf_c);

    MPP_FREE(checkcrc.sum);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    vdpp->ops->deinit(vdpp->priv);

    rockchip_vdpp_api_release_ctx(vdpp);
}

extern char *optarg;
extern int   opterr;

int32_t main(int32_t argc, char **argv)
{
    VdppTestCfg cfg;
    int32_t ch;
    int32_t option_index = 0;

    if (argc < 2) {
        vdpp_test_help();
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.src_swa = VDPP_YUV_SWAP_SP_UV;
    cfg.dst_fmt = VDPP_FMT_YUV444;
    cfg.dst_swa = VDPP_YUV_SWAP_SP_UV;
    cfg.hist_mode_en = 0;

    /* get options */
    opterr = 0;
    /* not recommended if not familiar with HW */
    static struct option long_options[] = {
        {"wi_vir",     required_argument, 0,  0 },
        {"hi_vir",     required_argument, 0,  0 },
        {"wo_vir",     required_argument, 0,  0 },
        {"ho_vir",     required_argument, 0,  0 },
        {"wo_uv",      required_argument, 0,  0 },
        {"ho_uv",      required_argument, 0,  0 },
        {"wo_uv_vir",  required_argument, 0,  0 },
        {"ho_uv_vir",  required_argument, 0,  0 },
        {"diff",       required_argument, 0,  0 },
        {"cfg_set",    required_argument, 0,  0 },
        {"hist_l",     required_argument, 0,  0 },
        {"hist_g",     required_argument, 0,  0 },
        {"slt",        required_argument, 0,  0 },
        { 0,           0,                 0,  0 },
    };

    while ((ch = getopt_long_only(argc, argv, "w:h:f:s:i:W:H:F:S:o:O:n:m:Y:", long_options, &option_index)) != -1) {
        switch (ch) {
        case 0: {
            switch (option_index) {
            case 0 : {
                cfg.src_width_vir = atoi(optarg);
            } break;
            case 1 : {
                cfg.src_height_vir = atoi(optarg);
            } break;
            case 2 : {
                cfg.dst_width_vir = atoi(optarg);
            } break;
            case 3 : {
                cfg.dst_height_vir = atoi(optarg);
            } break;
            case 4 : {
                cfg.dst_c_width = atoi(optarg);
            } break;
            case 5 : {
                cfg.dst_c_height = atoi(optarg);
            } break;
            case 6 : {
                cfg.dst_c_width_vir = atoi(optarg);
            } break;
            case 7 : {
                cfg.dst_c_height_vir = atoi(optarg);
            } break;
            case 8 : {
                cfg.yuv_out_diff = atoi(optarg);
            } break;
            case 9 : {
                cfg.cfg_set = strtol(optarg, NULL, 0);
            } break;
            case 10 : {
                mpp_log("local hist name: %s\n", optarg);
                strncpy(cfg.hist_l_url, optarg, sizeof(cfg.hist_l_url) - 1);
                cfg.fp_hist_l = fopen(cfg.hist_l_url, "w+b");
            } break;
            case 11 : {
                mpp_log("global hist name: %s\n", optarg);
                strncpy(cfg.hist_g_url, optarg, sizeof(cfg.hist_g_url) - 1);
                cfg.fp_hist_g = fopen(cfg.hist_g_url, "w+b");
            } break;
            case 12 : {
                mpp_log("verify file: %s\n", optarg);
                strncpy(cfg.slt_url, optarg, sizeof(cfg.slt_url) - 1);
                cfg.fp_slt = fopen(cfg.slt_url, "w+b");
            } break;
            default : {
            } break;
            }
            mpp_log("%s: %s", long_options[option_index].name, optarg);

        } break;
        case 'w': {
            cfg.src_width = atoi(optarg);
        } break;
        case 'h': {
            cfg.src_height = atoi(optarg);
        } break;
        case 'f': {
            cfg.src_format = atoi(optarg);
        } break;
        case 's': {
            cfg.src_swa = atoi(optarg);
        } break;
        case 'i': {
            mpp_log("input filename: %s\n", optarg);
            strncpy(cfg.src_url, optarg, sizeof(cfg.src_url) - 1);
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
            strncpy(cfg.dst_url, optarg, sizeof(cfg.dst_url) - 1);
            cfg.fp_dst = fopen(cfg.dst_url, "w+b");
        } break;
        case 'O': {
            mpp_log("output chroma filename: %s\n", optarg);
            strncpy(cfg.dst_c_url, optarg, sizeof(cfg.dst_c_url) - 1);
            cfg.fp_dst_c = fopen(cfg.dst_c_url, "w+b");
        } break;
        case 'n': {
            cfg.frame_num = atoi(optarg);
        } break;
        case 'm': {
            cfg.hist_mode_en = atoi(optarg);
        } break;
        case 'Y': {
            mpp_log("output hist: %s\n", optarg);
            strncpy(cfg.hist_url, optarg, sizeof(cfg.hist_url) - 1);
            cfg.fp_hist = fopen(cfg.hist_url, "w+b");
        } break;
        default: {
        } break;
        }
    }

    /* set default vir stride */
    if (!cfg.src_width_vir)
        cfg.src_width_vir = MPP_ALIGN(cfg.src_width, 16);
    if (!cfg.src_height_vir)
        cfg.src_height_vir = MPP_ALIGN(cfg.src_height, 8);
    if (!cfg.dst_width)
        cfg.dst_width = cfg.src_width;
    if (!cfg.dst_height)
        cfg.dst_height = cfg.src_height;
    if (!cfg.dst_width_vir)
        cfg.dst_width_vir = MPP_ALIGN(cfg.dst_width, 16);
    if (!cfg.dst_height_vir)
        cfg.dst_height_vir = MPP_ALIGN(cfg.dst_height, 2);
    if (cfg.yuv_out_diff) {
        if (!cfg.dst_c_width)
            cfg.dst_c_width = cfg.dst_width;
        if (!cfg.dst_c_width_vir)
            cfg.dst_c_width_vir = MPP_ALIGN(cfg.dst_c_width, 16);
        if (!cfg.dst_c_height)
            cfg.dst_c_height = cfg.dst_height;
        if (!cfg.dst_c_height_vir)
            cfg.dst_c_height_vir = MPP_ALIGN(cfg.dst_c_height, 2);
    }
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

    if (cfg.fp_dst_c) {
        fclose(cfg.fp_dst_c);
        cfg.fp_dst_c = NULL;
    }

    if (cfg.fp_hist) {
        fclose(cfg.fp_hist);
        cfg.fp_hist = NULL;
    }

    if (cfg.fp_hist_l) {
        fclose(cfg.fp_hist_l);
        cfg.fp_hist_l = NULL;
    }

    if (cfg.fp_hist_g) {
        fclose(cfg.fp_hist_g);
        cfg.fp_hist_g = NULL;
    }

    if (cfg.fp_slt) {
        fclose(cfg.fp_slt);
        cfg.fp_slt = NULL;
    }

    return 0;
}
