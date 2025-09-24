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
#include "mpp_info.h"
#include "mpp_env.h"

#include "utils.h"
#include "vdpp_api.h"

#define MAX_URL_LEN       (256)

#define SHP_CFG_STRIDE    (684)
#define DM_ZME_CFG_STRIDE (260)
#define ES_CFG_STRIDE     (68)
#define YUV_MAX_SIZE      (12582912) // 4096 * 2048 * 3 / 2
#define DCI_HIST_SIZE     (10240)    // (16*16*16*18/8/4 + 256) * 4

typedef struct VdppTestCfg_t {
    MppFrameFormat src_format; // see MppFrameFormat: 0-nv12, 15-nv24
    RK_U32 src_width;          // unit: pixel
    RK_U32 src_height;         // unit: pixel
    RK_U32 src_width_vir;      // 16 align, unit: pixel
    RK_U32 src_height_vir;     // 8 align, unit: pixel
    RK_U32 src_swap;           // 0-UV, 1-VU
    RK_U32 src_range;          // 0-limited, 1-full

    RK_U32 dst_fmt;            // see VDPP_FMT: 0-yuv444sp, 3-yuv420sp
    RK_U32 dst_width;          // unit: pixel
    RK_U32 dst_height;         // unit: pixel
    RK_U32 dst_width_vir;      // 16 align, unit: pixel
    RK_U32 dst_height_vir;     // 8 align, unit: pixel
    RK_U32 dst_swap;           // 0-UV, 1-VU
    RK_U32 yuv_out_diff;       // diff buffer for chroma data
    RK_U32 dst_c_width;        // unit: pixel
    RK_U32 dst_c_height;       // unit: pixel
    RK_U32 dst_c_width_vir;    // unit: pixel
    RK_U32 dst_c_height_vir;   // unit: pixel

    RK_U32 frame_num;          // default: 1
    RK_U32 work_mode;          // {1-iep(unsupported), 2-vep, 3-dci_hist}

    /* high 16 bit: mask; low 3 bit (msb->lsb): dmsr|es|sharp */
    RK_U32 cfg_set; // used in com2, reset enable flag of dmsr/es/sharp
    RK_U32 en_dmsr;
    RK_U32 en_es;
    RK_U32 en_shp;
    RK_U32 en_hist;
    RK_U32 en_pyr;
    RK_U32 en_bbd;
    RK_U32 bbd_th; // default: 20 in U8 range

    char src_url[MAX_URL_LEN];   // input image filename
    char dst_url[MAX_URL_LEN];   // output image filename
    char dst_c_url[MAX_URL_LEN]; // output image filename for chroma data
    char slt_url[MAX_URL_LEN];   // system level test file
    char out_dir[MAX_URL_LEN];   // output directory for other results

    FILE *fp_src;
    FILE *fp_dst;
    FILE *fp_dst_c;
    FILE *fp_hist;
    FILE *fp_hist_l;
    FILE *fp_hist_g;
    FILE *fp_pyrs[3];
    FILE *fp_slt;
    FILE *fp_result;
} VdppTestCfg;

const RK_U32 local_hist_size = RKVOP_PQ_PREPROCESS_HIST_SIZE_VERI * RKVOP_PQ_PREPROCESS_HIST_SIZE_HORI *
                               RKVOP_PQ_PREPROCESS_LOCAL_HIST_BIN_NUMS * sizeof(RK_U32);
const RK_U32 global_hist_size = RKVOP_PQ_PREPROCESS_GLOBAL_HIST_BIN_NUMS * sizeof(RK_U32);

extern void mpp_show_color_format();

static void print_help_info()
{
    mpp_logi("Usage: vdpq_test [options]\n");
    mpp_logi("Options:\n");
    mpp_logi("  -h, --help            : show this message\n");
    mpp_logi("  -v, --version         : show version info\n");
    mpp_logi("  -i, --input     <file>: input filename\n");
    mpp_logi("  -o, --output    [file]: output filename [of luma data] \n");
    mpp_logi("  -O, --outc      [file]: output filename of chroma data \n");
    mpp_logi("  -d  --outdir     [dir]: output directory for other results (hist/pyr data; avg_luma/bbd result...)\n");
    mpp_logi("  -w, --swid       [int]: input  valid   width,  unit: pixel, default: 1920\n");
    mpp_logi("  -g, --shgt       [int]: input  valid   height, unit: pixel, default: 1080\n");
    mpp_logi("      --sw_vir     [int]: input  aligned width,  unit: pixel\n");
    mpp_logi("      --sh_vir     [int]: input  aligned height, unit: pixel\n");
    mpp_logi("  -f, --sfmt       [int]: input  image format in MppFrameFormat, defualt: 0(nv12)\n");
    mpp_logi("  -s, --sswap           : input  image chroma_uv is swapped\n");
    mpp_logi("  -r, --srange     [int]: input  image range is limited(0) or full(1)\n");
    mpp_logi("  -W, --dwid       [int]: output [luma] valid   width,  unit: pixel, default: same to src\n");
    mpp_logi("  -G, --dhgt       [int]: output [luma] valid   height, unit: pixel, default: same to src\n");
    mpp_logi("      --dw_vir     [int]: output [luma] aligned width,  unit: pixel\n");
    mpp_logi("      --dh_vir     [int]: output [luma] aligned height, unit: pixel\n");
    mpp_logi("      --dwc        [int]: output chroma valid   width,  unit: pixel\n");
    mpp_logi("      --dhc        [int]: output chroma valid   height, unit: pixel\n");
    mpp_logi("      --dwc_vir    [int]: output chroma aligned width,  unit: pixel\n");
    mpp_logi("      --dhc_vir    [int]: output chroma aligned height, unit: pixel\n");
    mpp_logi("  -F, --dfmt       [int]: output image format in ASCII string, defualt: yuv444\n");
    mpp_logi("  -S, --dswap           : output image chroma_uv is swapped\n");
    mpp_logi("  -m, --mode       [int]: vdpp work mode: {2-vep, 3-dci_hist}, default: 2\n");
    mpp_logi("  -n, --nframe     [int]: frame number, default: 1\n");
    mpp_logi("      --slt       <file>: system level test file\n");
    mpp_logi("      --cfg_set    [int]: high 16 bit: mask; low 3 bit (msb->lsb): dmsr|es|sharp. default: 0x0\n");
    mpp_logi("      --en_dmsr    [int]: en_dmsr flag, default: 0\n");
    mpp_logi("      --en_es      [int]: en_es   flag, default: 0\n");
    mpp_logi("      --en_shp     [int]: en_shp  flag, default: 0\n");
    mpp_logi("      --en_hist    [int]: en_hist flag, default: 1\n");
    mpp_logi("      --en_pyr     [int]: en_pyr  flag, default: 1\n");
    mpp_logi("      --en_bbd     [int]: en_bbd  flag, default: 1\n");
    mpp_logi("      --bbd_th     [int]: bbd threshold, default: 20 in U8 input range\n");
}

static void vdpp_test_help()
{
    mpp_logi("usage: vdpp_test [options]\n");
    mpp_logi("*******************************\n");
    print_help_info();
    mpp_show_color_format();
    mpp_logi("*******************************\n");
    mpp_logi("supported output ASCII format strings:\n");
    mpp_logi("0 - yuv444(sp)\n");
    mpp_logi("2 - yuv420(sp) \n");
    mpp_logi("************ sample ***********\n");
    mpp_logi("vdpp_test -w 720 -g 480 -i input_yuv420.yuv -f 0 -W 1920 -G 1080 -F yuv444 -o "
             "output_yuv444.yuv -n 1\n");
}

static RK_S32 str_to_vdpp_fmt(const char *str)
{
    RK_S32 fmt = -1;

    if (0 == strcmp(str, "yuv420") || 0 == strcmp(str, "2"))
        fmt = VDPP_FMT_YUV420;
    else if (0 == strcmp(str, "yuv444") || 0 == strcmp(str, "0"))
        fmt = VDPP_FMT_YUV444;
    else
        mpp_loge("invalid output vdpp format %s!\n", str);

    return fmt;
}

static inline size_t get_src_frm_size(RK_S32 fmt, RK_U32 w, RK_U32 h)
{
    RK_S32 frame_size;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP:
    case MPP_FMT_YUV420P:  {
        frame_size = w * h * 3 / 2;
    } break;

    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_YUV422P:
    case MPP_FMT_YUV422SP:
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:      {
        frame_size = w * h * 2;
    } break;
    case MPP_FMT_YUV444P:
    case MPP_FMT_YUV444SP:
    case MPP_FMT_RGB888:
    case MPP_FMT_BGR888:   {
        frame_size = w * h * 3;
    } break;
    case MPP_FMT_RGB101010:
    case MPP_FMT_BGR101010:
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888:  {
        frame_size = w * h * 4;
    } break;
    case MPP_FMT_YUV420SP_10BIT: {
        frame_size = w * h * 3 * 5 / 2 / 4;
    } break;
    case MPP_FMT_YUV422SP_10BIT: {
        frame_size = w * h * 2 * 5 / 4;
    } break;
    case MPP_FMT_YUV444SP_10BIT: {
        frame_size = w * h * 3 * 5 / 4;
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
    case VDPP_FMT_YUV444 + 10: // chroma of NV24
    case VDPP_FMT_YUV420 + 10: // chroma of NV12
        return w * h * 2;
    default: {
        mpp_loge("warning: unsupported output format %d!\n", fmt);
    } return 0;
    }
}

static int parse_cmd(int argc, char **argv, VdppTestCfg *cfg)
{
    extern char *optarg;
    extern int opterr;
    RK_S32 ch;
    RK_S32 option_index = 0;

    static struct option long_options[] = {
        {"sw_vir",  required_argument, 0, 0  }, // 0
        {"sh_vir",  required_argument, 0, 0  },
        {"dw_vir",  required_argument, 0, 0  },
        {"dh_vir",  required_argument, 0, 0  },
        {"dwc",     required_argument, 0, 0  }, // 4
        {"dhc",     required_argument, 0, 0  },
        {"dwc_vir", required_argument, 0, 0  },
        {"dhc_vir", required_argument, 0, 0  },
        {"cfg_set", required_argument, 0, 0  }, // 8
        {"en_dmsr", required_argument, 0, 0  }, // 9
        {"en_es",   required_argument, 0, 0  }, // 10
        {"en_shp",  required_argument, 0, 0  }, // 11
        {"en_hist", required_argument, 0, 0  }, // 12
        {"en_pyr",  required_argument, 0, 0  }, // 13
        {"en_bbd",  required_argument, 0, 0  }, // 14
        {"bbd_th",  required_argument, 0, 0  }, // 15
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {"input",   required_argument, 0, 'i'},
        {"output",  required_argument, 0, 'o'},
        {"outc",    required_argument, 0, 'O'}, // set 'uv_diff' to 1 if 'outc' valid
        {"outdir",  required_argument, 0, 'd'},
        {"swid",    required_argument, 0, 'w'},
        {"shgt",    required_argument, 0, 'g'},
        {"sfmt",    required_argument, 0, 'f'},
        {"srange",  required_argument, 0, 'r'},
        {"sswap",   no_argument,       0, 's'},
        {"dwid",    required_argument, 0, 'W'},
        {"dhgt",    required_argument, 0, 'G'},
        {"dfmt",    required_argument, 0, 'F'},
        {"dswap",   no_argument,       0, 'S'},
        {"mode",    required_argument, 0, 'm'},
        {"nframe",  required_argument, 0, 'n'},
        {"slt",     required_argument, 0, 't'},
        {0,         0,                 0, 0  },
    };

    /* set default values */
    memset(cfg, 0, sizeof(VdppTestCfg));
    cfg->src_format = MPP_FMT_YUV420SP;
    cfg->src_swap = VDPP_YUV_SWAP_SP_UV;
    cfg->src_range = VDPP_COLOR_SPACE_LIMIT_RANGE;
    cfg->src_width = 1920;
    cfg->src_height = 1080;
    cfg->dst_fmt = VDPP_FMT_YUV420;
    cfg->dst_swap = VDPP_YUV_SWAP_SP_UV;
    cfg->work_mode = VDPP_WORK_MODE_VEP;
    cfg->frame_num = 1;
    cfg->en_dmsr = 0;
    cfg->en_es = 0;
    cfg->en_shp = 0;
    cfg->en_hist = 1;
    cfg->en_pyr = 1;
    cfg->en_bbd = 1;
    cfg->bbd_th = 20;

    /* parse arguments */
    opterr = 0;
    while ((ch = getopt_long_only(argc, argv, "i:o:O:d:w:g:f:r:W:G:F:m:n:t:hvsS", long_options,
                                  &option_index)) != -1) {
        switch (ch) {
        case 0: {
            mpp_logi(" - set %dth option: %s = %s", option_index,
                     long_options[option_index].name, optarg);
            switch (option_index) {
            case 0: {
                cfg->src_width_vir = strtol(optarg, NULL, 0);
            } break;
            case 1: {
                cfg->src_height_vir = strtol(optarg, NULL, 0);
            } break;
            case 2: {
                cfg->dst_width_vir = strtol(optarg, NULL, 0);
            } break;
            case 3: {
                cfg->dst_height_vir = strtol(optarg, NULL, 0);
            } break;
            case 4: {
                cfg->dst_c_width = strtol(optarg, NULL, 0);
            } break;
            case 5: {
                cfg->dst_c_height = strtol(optarg, NULL, 0);
            } break;
            case 6: {
                cfg->dst_c_width_vir = strtol(optarg, NULL, 0);
            } break;
            case 7: {
                cfg->dst_c_height_vir = strtol(optarg, NULL, 0);
            } break;
            case 8: {
                cfg->cfg_set = strtol(optarg, NULL, 0);
            } break;
            case 9: {
                cfg->en_dmsr = strtol(optarg, NULL, 0);
            } break;
            case 10: {
                cfg->en_es = strtol(optarg, NULL, 0);
            } break;
            case 11: {
                cfg->en_shp = strtol(optarg, NULL, 0);
            } break;
            case 12: {
                cfg->en_hist = strtol(optarg, NULL, 0);
            } break;
            case 13: {
                cfg->en_pyr = strtol(optarg, NULL, 0);
            } break;
            case 14: {
                cfg->en_bbd = strtol(optarg, NULL, 0);
            } break;
            case 15: {
                cfg->bbd_th = strtol(optarg, NULL, 0);
            } break;
            default: break;
            }
        } break;
        case 'h': {
            vdpp_test_help();
        } return 1;
        case 'v': {
            show_mpp_version();
        } return 2;
        case 'i': {
            strncpy(cfg->src_url, optarg, sizeof(cfg->src_url) - 1);
            mpp_logi(" - set input filename: %s\n", optarg);
        } break;
        case 'o': {
            strncpy(cfg->dst_url, optarg, sizeof(cfg->dst_url) - 1);
            mpp_logi(" - set output [luma] filename: %s\n", optarg);
        } break;
        case 'O': {
            strncpy(cfg->dst_c_url, optarg, sizeof(cfg->dst_c_url) - 1);
            cfg->yuv_out_diff = 1;
            mpp_logi(" - set yuv_out_diff=1 since output chroma filename set: %s\n", optarg);
        } break;
        case 'd': {
            strncpy(cfg->out_dir, optarg, sizeof(cfg->out_dir) - 1);
            mpp_logi(" - set output directory: %s\n", optarg);
        } break;
        case 'w': {
            cfg->src_width = strtol(optarg, NULL, 0);
        } break;
        case 'g': {
            cfg->src_height = strtol(optarg, NULL, 0);
        } break;
        case 'f': {
            cfg->src_format = strtol(optarg, NULL, 0);
            mpp_logi(" - set input mpp format: %d\n", cfg->src_format);
        } break;
        case 'r': {
            cfg->src_range = strtol(optarg, NULL, 0);
            mpp_logi(" - set input range: %d (0-Limit, 1-Full)\n", cfg->src_range);
        } break;
        case 's': {
            cfg->src_swap = 1;
            mpp_logi(" - set input format UV swaped!\n");
        } break;
        case 'W': {
            cfg->dst_width = strtol(optarg, NULL, 0);
        } break;
        case 'G': {
            cfg->dst_height = strtol(optarg, NULL, 0);
        } break;
        case 'F': {
            cfg->dst_fmt = str_to_vdpp_fmt(optarg);
            mpp_logi(" - set output format: %s -> %d (vdpp format)\n", optarg, cfg->dst_fmt);
        } break;
        case 'S': {
            cfg->dst_swap = 1;
            mpp_logi(" - set output format UV swaped!\n");
        } break;
        case 'm': {
            cfg->work_mode = MPP_MAX(VDPP_WORK_MODE_VEP, atoi(optarg));
            mpp_logi(" - set work mode: %d (2-vep, 3-dci_hist)\n", cfg->work_mode);
        } break;
        case 'n': {
            cfg->frame_num = strtol(optarg, NULL, 0);
        } break;
        case 't': {
            strncpy(cfg->slt_url, optarg, sizeof(cfg->slt_url) - 1);
            mpp_logi(" - set slt filename: %s\n", optarg);
        } break;
        default: break;
        }
    }

    /* check & adjust arguments */
    if (!cfg->src_url[0]) {
        mpp_err("input filename not specified!\n");
        return -1;
    }
    if (cfg->work_mode != VDPP_WORK_MODE_DCI) {
        if (cfg->dst_fmt != VDPP_FMT_YUV420 && cfg->dst_fmt != VDPP_FMT_YUV444) {
            mpp_loge("output format %d not supported!\n", cfg->dst_fmt);
            return -1;
        }
    }
    mpp_logi(" - set input image size: %dx%d, mpp format: %d, fullrange: %d\n", cfg->src_width,
             cfg->src_height, cfg->src_format, cfg->src_range);

    if (!cfg->src_width_vir)
        cfg->src_width_vir = MPP_ALIGN(cfg->src_width, 16);
    if (!cfg->src_height_vir)
        cfg->src_height_vir = MPP_ALIGN(cfg->src_height, 8);
    if (!cfg->dst_width)
        cfg->dst_width = cfg->src_width;
    if (!cfg->dst_height)
        cfg->dst_height = cfg->src_height;
    if (!cfg->dst_width_vir)
        cfg->dst_width_vir = MPP_ALIGN(cfg->dst_width, 16);
    if (!cfg->dst_height_vir)
        cfg->dst_height_vir = MPP_ALIGN(cfg->dst_height, 2);
    if (cfg->yuv_out_diff) {
        int shift_bit = (cfg->dst_fmt == VDPP_FMT_YUV420); // 1 for yuv420

        if (!cfg->dst_c_width)
            cfg->dst_c_width = cfg->dst_width >> shift_bit;
        if (!cfg->dst_c_width_vir)
            cfg->dst_c_width_vir = MPP_ALIGN(cfg->dst_c_width, 16);
        if (!cfg->dst_c_height)
            cfg->dst_c_height = cfg->dst_height >> shift_bit;
        if (!cfg->dst_c_height_vir)
            cfg->dst_c_height_vir = MPP_ALIGN(cfg->dst_c_height, 2);
        mpp_logi(" - set output chroma size: %dx%d, virtual size: %dx%d\n", cfg->dst_c_width,
                 cfg->dst_c_height, cfg->dst_c_width_vir, cfg->dst_c_height_vir);
    }
    mpp_logi(" - set output image size: %dx%d, virtual size: %dx%d, vdpp format: %d\n",
             cfg->dst_width, cfg->dst_height, cfg->dst_width_vir, cfg->dst_height_vir, cfg->dst_fmt);
    mpp_logi(" - set enbale flags: dmsr=%d, es=%d, shp=%d, hist=%d, pyr=%d, bbd=%d\n",
             cfg->en_dmsr, cfg->en_es, cfg->en_shp, cfg->en_hist, cfg->en_pyr, cfg->en_bbd);

    return 0;
}

static void vdpp_test_set_img(VdppComCtx *ctx, RK_S32 fd_y, RK_S32 fd_c,
                              RK_U32 uv_offset, VdppCmd cmd)
{
    VdppImg img = {0};
    MPP_RET ret = MPP_OK;

    img.mem_addr = fd_y;
    img.uv_addr = fd_c;
    img.uv_off = uv_offset;
    mpp_logi("set buf: type=%#x, yrgb_fd=%d, cbcr_fd=%d, chroma_offset=%d\n",
             cmd, fd_y, fd_c, uv_offset);

    ret = ctx->ops->control(ctx->priv, cmd, &img);
    if (ret)
        mpp_loge_f("control %08x failed %d!\n", cmd, ret);
}

int vdpp_test(VdppComCtx *vdppCtx, VdppTestCfg *cfg)
{
    MppBufferGroup memGroup = NULL;
    MppBuffer srcbuf = NULL;
    MppBuffer dstbuf = NULL;
    MppBuffer dstbuf_c = NULL;
    MppBuffer histbuf = NULL;
    MppBuffer histbuf_l = NULL;
    MppBuffer histbuf_g = NULL;
    MppBuffer pyrbuf_l1 = NULL;
    MppBuffer pyrbuf_l2 = NULL;
    MppBuffer pyrbuf_l3 = NULL;
    RK_U8 *psrc = NULL;
    RK_U8 *pdst = NULL;
    RK_U8 *pdst_c = NULL;
    RK_U8 *phist = NULL;
    RK_U8 *phist_l = NULL;
    RK_U8 *phist_g = NULL;
    RK_U8 *pyrAddrs[3] = {0};
    RK_S32 fdsrc = -1;
    RK_S32 fddst = -1;
    RK_S32 fddst_c = -1;
    RK_S32 fdhist = -1;
    RK_S32 pyrFds[3] = {-1, -1, -1};
    RK_U32 pyrWids[3] = {0};
    RK_U32 pyrHgts[3] = {0};
    RK_U32 pyrSizes[3] = {0};

    VdppApiParams params;
    VdppResults results = {0};
    DataCrc checkcrc;

    RK_U32 vdpp_ver = MPP_MAX(vdppCtx->ver >> 8, 1); // {0x100, 0x200, 0x300}
    RK_U32 yuv_out_diff = (cfg->yuv_out_diff && (vdpp_ver >= 2));
    size_t srcfrmsize = 0;
    size_t dstfrmsize = 0;
    size_t dstfrmsize_c = 0;
    size_t readsize = 0;
    RK_U32 frame_cnt = 0;
    RK_S32 i = 0;
    MPP_RET ret = MPP_NOK;

    // calculate aligned width independently, dword 4x align
    pyrWids[0] = MPP_ALIGN(cfg->dst_width / 2, 16);
    pyrWids[1] = MPP_ALIGN(cfg->dst_width / 4, 16);
    pyrWids[2] = MPP_ALIGN(cfg->dst_width / 8, 16);
    pyrHgts[0] = (cfg->dst_height + 1) / 2;
    pyrHgts[1] = (cfg->dst_height + 3) / 4;
    pyrHgts[2] = (cfg->dst_height + 7) / 8;
    pyrSizes[0] = pyrWids[0] * pyrHgts[0];
    pyrSizes[1] = pyrWids[1] * pyrHgts[1];
    pyrSizes[2] = pyrWids[2] * pyrHgts[2];

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);

    srcfrmsize = get_src_frm_size(cfg->src_format, cfg->src_width_vir, cfg->src_height_vir);
    dstfrmsize = get_dst_frm_size(cfg->dst_fmt, cfg->dst_width_vir, cfg->dst_height_vir);
    // (dst_fmt + 10) for chroma plane
    dstfrmsize_c = get_dst_frm_size(cfg->dst_fmt + 10, cfg->dst_c_width_vir, cfg->dst_c_height_vir);

    /* create buffers */
    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        mpp_loge("memGroup mpp_buffer_group_get failed! %d\n", ret);
        return ret;
    }

    mpp_buffer_get(memGroup, &srcbuf, srcfrmsize);
    mpp_buffer_get(memGroup, &dstbuf, dstfrmsize);
    mpp_assert(srcbuf && dstbuf);

    mpp_buffer_get(memGroup, &histbuf, DCI_HIST_SIZE);
    mpp_buffer_get(memGroup, &histbuf_l, local_hist_size);
    mpp_buffer_get(memGroup, &histbuf_g, global_hist_size);
    mpp_assert(histbuf && histbuf_l && histbuf_g);

    mpp_buffer_get(memGroup, &pyrbuf_l1, pyrSizes[0]);
    mpp_buffer_get(memGroup, &pyrbuf_l2, pyrSizes[1]);
    mpp_buffer_get(memGroup, &pyrbuf_l3, pyrSizes[2]);
    mpp_assert(pyrbuf_l1 && pyrbuf_l2 && pyrbuf_l3);

    psrc = mpp_buffer_get_ptr(srcbuf);
    pdst = mpp_buffer_get_ptr(dstbuf);
    phist = mpp_buffer_get_ptr(histbuf);
    phist_l = mpp_buffer_get_ptr(histbuf_l);
    phist_g = mpp_buffer_get_ptr(histbuf_g);
    pyrAddrs[0] = mpp_buffer_get_ptr(pyrbuf_l1);
    pyrAddrs[1] = mpp_buffer_get_ptr(pyrbuf_l2);
    pyrAddrs[2] = mpp_buffer_get_ptr(pyrbuf_l3);

    fdsrc = mpp_buffer_get_fd(srcbuf);
    fddst = mpp_buffer_get_fd(dstbuf);
    fdhist = mpp_buffer_get_fd(histbuf);
    pyrFds[0] = mpp_buffer_get_fd(pyrbuf_l1);
    pyrFds[1] = mpp_buffer_get_fd(pyrbuf_l2);
    pyrFds[2] = mpp_buffer_get_fd(pyrbuf_l3);

    mpp_logi("vdpp_ver: %d, is_yuv_out_diff: %d, work_mode: %d",
             vdpp_ver, yuv_out_diff, cfg->work_mode);
    mpp_logi("src w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d\n", cfg->src_width,
             cfg->src_height, cfg->src_width_vir, cfg->src_height_vir, srcfrmsize, fdsrc);
    mpp_logi("dst w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d\n", cfg->dst_width,
             cfg->dst_height, cfg->dst_width_vir, cfg->dst_height_vir, dstfrmsize, fddst);

    if (yuv_out_diff) {
        mpp_buffer_get(memGroup, &dstbuf_c, dstfrmsize_c);
        mpp_assert(dstbuf_c);

        pdst_c = mpp_buffer_get_ptr(dstbuf_c);
        fddst_c = mpp_buffer_get_fd(dstbuf_c);

        mpp_logi("dst chroma w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d\n",
                 cfg->dst_c_width, cfg->dst_c_height, cfg->dst_c_width_vir,
                 cfg->dst_c_height_vir, dstfrmsize_c, fddst_c);
    }

    for (i = 0; i < 3; i++) {
        mpp_logi("pyr[%d] info: size=%ux%u, require buf %u bytes. fd %d\n",
                 i + 1, pyrWids[i], pyrHgts[i], pyrSizes[i], pyrFds[i]);
    }

    /* vdpp_params are all set to default values in init() */
    ret = vdppCtx->ops->init(vdppCtx->priv);
    if (ret) {
        mpp_err_f("init vdpp context failed! %d\n", ret);
        return ret;
    }

    /* set common api params */
    if (vdpp_ver >= 2) {
        params.ptype = VDPP_PARAM_TYPE_COM2;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.com2.sfmt = cfg->src_format;
        params.param.com2.sswap = cfg->src_swap;
        params.param.com2.src_width = cfg->src_width;
        params.param.com2.src_height = cfg->src_height;
        params.param.com2.src_width_vir = cfg->src_width_vir;
        params.param.com2.src_height_vir = cfg->src_height_vir;
        params.param.com2.src_range = cfg->src_range;
        params.param.com2.dfmt = cfg->dst_fmt;
        params.param.com2.dswap = cfg->dst_swap;
        params.param.com2.dst_width = cfg->dst_width;
        params.param.com2.dst_height = cfg->dst_height;
        params.param.com2.dst_width_vir = cfg->dst_width_vir;
        params.param.com2.dst_height_vir = cfg->dst_height_vir;
        if (yuv_out_diff) {
            params.param.com2.yuv_out_diff = 1;
            params.param.com2.dst_c_width = cfg->dst_c_width;
            params.param.com2.dst_c_height = cfg->dst_c_height;
            params.param.com2.dst_c_width_vir = cfg->dst_c_width_vir;
            params.param.com2.dst_c_height_vir = cfg->dst_c_height_vir;
        }
        params.param.com2.hist_mode_en = cfg->work_mode == VDPP_WORK_MODE_DCI;
        params.param.com2.cfg_set = cfg->cfg_set;
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_COM2_CFG, &params);

        mpp_logi("set api param: sfmt=%d, sswap=%d, src_range=%d, size=%dx%d [%dx%d]\n",
                 params.param.com2.sfmt, params.param.com2.sswap, cfg->src_range,
                 params.param.com2.src_width, params.param.com2.src_height,
                 params.param.com2.src_width_vir, params.param.com2.src_height_vir);
        mpp_logi("set api param: dfmt=%d, dswap=%d, size=%dx%d [%dx%d]\n", params.param.com2.dfmt,
                 params.param.com2.dswap, params.param.com2.dst_width, params.param.com2.dst_height,
                 params.param.com2.dst_width_vir, params.param.com2.dst_height_vir);
        mpp_logi("set api param: yuv_out_diff=%d, dst_c_size=%dx%d [%dx%d]\n",
                 params.param.com2.yuv_out_diff, params.param.com2.dst_c_width,
                 params.param.com2.dst_c_height, params.param.com2.dst_c_width_vir,
                 params.param.com2.dst_c_height_vir);
        mpp_logi("set api param: hist_only_mode=%d, cfg_set=%d\n", params.param.com2.hist_mode_en,
                 params.param.com2.cfg_set);
    } else {
        params.ptype = VDPP_PARAM_TYPE_COM;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.com.src_width = cfg->src_width;
        params.param.com.src_height = cfg->src_height;
        params.param.com.sswap = cfg->src_swap;
        params.param.com.dfmt = cfg->dst_fmt;
        params.param.com.dst_width = cfg->dst_width;
        params.param.com.dst_height = cfg->dst_height;
        params.param.com.dswap = cfg->dst_swap;
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_COM_CFG, &params);
    }

    /* close dmsr/es/sharp after com if needed, since the enable_flags will be reset by com */
    if (!cfg->en_dmsr) {
        params.ptype = VDPP_PARAM_TYPE_DMSR;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.dmsr.enable = 0;
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_DMSR_CFG, &params);
        mpp_logi("set api param: disbale DMSR...\n");
    }

    if (!cfg->en_es) {
        params.ptype = VDPP_PARAM_TYPE_ES;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.es.es_bEnabledES = 0;
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_ES, &params);
        mpp_logi("set api param: disbale ES...\n");
    }

    if (vdpp_ver >= 2) {
        if (!cfg->en_shp) {
            params.ptype = VDPP_PARAM_TYPE_SHARP;
            memset(&params.param, 0, sizeof(VdppApiContent));
            params.param.sharp.sharp_enable = 0;
            params.param.sharp.sharp_coloradj_bypass_en = 1;
            vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_SHARP, &params);
            mpp_logi("set api param: disbale Sharp...\n");
        }

        params.ptype = VDPP_PARAM_TYPE_HIST;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.hist.hist_cnt_en = cfg->en_hist;
        params.param.hist.dci_csc_range = cfg->src_range;
        params.param.hist.hist_addr = fdhist;
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_DCI_HIST, &params);
        mpp_logi("set api param: hist_cnt_en=%d (work_mode=%d), csc_range=%d, hist_fd=%d\n",
                 params.param.hist.hist_cnt_en, cfg->work_mode, params.param.hist.dci_csc_range,
                 params.param.hist.hist_addr);
    }

    /* set pyr & bbd params */
    if (vdpp_ver >= 3) {
        params.ptype = VDPP_PARAM_TYPE_PYR;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.pyr.pyr_en = cfg->en_pyr && (pyrFds[0] > 0) && (pyrFds[1] > 0) &&
                                  (pyrFds[2] > 0) && cfg->work_mode != VDPP_WORK_MODE_DCI;
        mpp_logi("set api param: pyr_en=%d.\n", params.param.pyr.pyr_en);
        if (params.param.pyr.pyr_en) {
            params.param.pyr.nb_layers = 3;
            for (i = 0; i < 3; i++) {
                params.param.pyr.layer_imgs[i].mem_addr = (RK_U32)pyrFds[i]; // fd
                params.param.pyr.layer_imgs[i].wid_vir = pyrWids[i];         // unit: pixel
                params.param.pyr.layer_imgs[i].hgt_vir = pyrHgts[i];         // unit: pixel
                mpp_logi("set api param: pyr[%d] addr=%u, wid_vir=%u (unit: pixel)\n",
                         i + 1, params.param.pyr.layer_imgs[i].mem_addr,
                         params.param.pyr.layer_imgs[i].wid_vir);
            }
        }
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_PYR, &params);

        params.ptype = VDPP_PARAM_TYPE_BBD;
        memset(&params.param, 0, sizeof(VdppApiContent));
        params.param.bbd.bbd_en = cfg->en_bbd;
        params.param.bbd.bbd_det_blcklmt = 20;
        mpp_logi("set api param: bbd_en=%d, bbd_det_blcklmt=%d\n", params.param.bbd.bbd_en,
                 params.param.bbd.bbd_det_blcklmt);
        vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_BBD, &params);
    }

    /* check cap */
    {
        RK_S32 cap = vdppCtx->ops->check_cap(vdppCtx->priv);

        mpp_logi("vdpp cap %#x (bitfield: 0-UNSUPPORTED, 1-VEP, 2-HIST)\n", cap);
    }

    /* run vdpp */
    while (1) {
        readsize = fread(psrc, 1, srcfrmsize, cfg->fp_src);
        if (srcfrmsize > readsize) {
            mpp_logi("source exhaused since readsize=%d < srcfrmsize=%d. total frames processed: %d\n",
                     readsize, srcfrmsize, frame_cnt);
            break;
        }
        if (frame_cnt >= cfg->frame_num)
            break;

        /* notice the order of the input frames */
        vdpp_test_set_img(vdppCtx, fdsrc, fdsrc, cfg->src_width_vir * cfg->src_height_vir,
                          VDPP_CMD_SET_SRC);
        vdpp_test_set_img(vdppCtx, fddst, fddst, cfg->dst_width_vir * cfg->dst_height_vir,
                          VDPP_CMD_SET_DST);
        if (yuv_out_diff)
            vdpp_test_set_img(vdppCtx, fddst, fddst_c, 0, VDPP_CMD_SET_DST_C);

        if (cfg->en_hist)
            vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_SET_HIST_FD, &fdhist);

        memset(pdst, 0, dstfrmsize);
        memset(phist, 0, DCI_HIST_SIZE);

        ret = vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_RUN_SYNC, NULL);
        if (ret) {
            mpp_loge("found vdpp run error, exit! %d\n", ret);
            break;
        }

        frame_cnt++;

        // write dst data
        if (cfg->fp_dst) {
            if (yuv_out_diff && pdst_c) {
                fwrite(pdst, 1, cfg->dst_width_vir * cfg->dst_height_vir, cfg->fp_dst);
                fwrite(pdst_c, 1, dstfrmsize_c, cfg->fp_dst); // write chroma data after dst luma
            } else
                fwrite(pdst, 1, dstfrmsize, cfg->fp_dst);
            mpp_logi("dst data dump to: %s\n", cfg->dst_url);
        }

        if (yuv_out_diff && cfg->fp_dst_c && pdst_c) {
            if (dstfrmsize_c > fwrite(pdst_c, 1, dstfrmsize_c, cfg->fp_dst_c)) {
                mpp_loge("chroma dump failed, errno %d %s\n", errno, strerror(errno));
                break;
            }
            mpp_logi("dst chroma data dump to: %s\n", cfg->dst_c_url);
        }

        // write hist data
        if (cfg->en_hist) {
            memset(phist_l, 0, local_hist_size);
            memset(phist_g, 0, global_hist_size);
            dci_hist_info_parser(phist, (RK_U32 *)phist_l, (RK_U32 *)phist_g);

            if (cfg->fp_hist) {
                if (DCI_HIST_SIZE > fwrite(phist, 1, DCI_HIST_SIZE, cfg->fp_hist)) {
                    mpp_loge("packed hist dump err!\n");
                    break;
                }
            }
            if (cfg->fp_hist_l) {
                if (local_hist_size > fwrite(phist_l, 1, local_hist_size, cfg->fp_hist_l)) {
                    mpp_loge("local hist dump err!\n");
                    break;
                }
            }
            if (cfg->fp_hist_g) {
                if (global_hist_size > fwrite(phist_g, 1, global_hist_size, cfg->fp_hist_g)) {
                    mpp_loge("global hist dump err!\n");
                    break;
                }
            }

            // get mean luma result
            ret = vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_GET_AVG_LUMA, &results);
            if (ret) {
                mpp_loge("get mean luma result error! %d\n", ret);
            } else {
                mpp_logi("get mean luma result: %d (in U10)\n", results.hist.luma_avg);
                if (cfg->fp_result) {
                    fprintf(cfg->fp_result, "mean luma result: %d\n", results.hist.luma_avg);
                }
            }
        }

        // write pyr data
        if (cfg->en_pyr) {
            for (i = 0; i < 3; ++i) {
                if (cfg->fp_pyrs[i]) {
                    if (pyrSizes[i] > fwrite(pyrAddrs[i], 1, pyrSizes[i], cfg->fp_pyrs[i])) {
                        mpp_loge("pryamid layer%d dump err!\n", i + 1);
                        break;
                    }
                }
            }
        }

        // write crc, DONOT modify the code here!
        if (cfg->fp_slt) {
            if (pdst && cfg->work_mode != VDPP_WORK_MODE_DCI) {
                calc_data_crc(pdst, dstfrmsize, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);
            }
            if (pdst_c && yuv_out_diff && cfg->work_mode != VDPP_WORK_MODE_DCI) {
                calc_data_crc(pdst_c, dstfrmsize_c, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);
            }
            if (phist) {
                calc_data_crc(phist, DCI_HIST_SIZE, &checkcrc);
                write_data_crc(cfg->fp_slt, &checkcrc);
            }
        }

        // get bbd result
        if (cfg->en_bbd) {
            ret = vdppCtx->ops->control(vdppCtx->priv, VDPP_CMD_GET_BBD, &results);
            if (ret) {
                mpp_loge("get bbd result error! %d\n", ret);
            } else {
                mpp_logi("get bbd result: top/bottom/left/right = [%d, %d, %d, %d]\n",
                         results.bbd.bbd_size_top, results.bbd.bbd_size_bottom,
                         results.bbd.bbd_size_left, results.bbd.bbd_size_right);
                if (cfg->fp_result) {
                    fprintf(cfg->fp_result, "bbd result: [top=%d, btm=%d, left=%d, right=%d]\n",
                            results.bbd.bbd_size_top, results.bbd.bbd_size_bottom,
                            results.bbd.bbd_size_left, results.bbd.bbd_size_right);
                    fflush(cfg->fp_result);
                }
            }
        }
    }
    mpp_logi("=============run sync done=============cnt[%d]\n", frame_cnt);

    mpp_buffer_put(srcbuf);
    mpp_buffer_put(dstbuf);
    mpp_buffer_put(histbuf);
    if (histbuf_l)
        mpp_buffer_put(histbuf_l);
    if (histbuf_g)
        mpp_buffer_put(histbuf_g);
    if (yuv_out_diff)
        mpp_buffer_put(dstbuf_c);
    if (pyrbuf_l1)
        mpp_buffer_put(pyrbuf_l1);
    if (pyrbuf_l2)
        mpp_buffer_put(pyrbuf_l2);
    if (pyrbuf_l3)
        mpp_buffer_put(pyrbuf_l3);

    MPP_FREE(checkcrc.sum);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    ret = vdppCtx->ops->deinit(vdppCtx->priv);
    if (ret) {
        mpp_logw("vdpp deinit ret %d!\n", ret);
    }

    return ret;
}

RK_S32 main(RK_S32 argc, char **argv)
{
    VdppTestCfg cfg = {0};
    RK_U32 vdpp_ver = 1;
    RK_S32 i = 0;
    RK_S32 ret = 0;

    mpp_logi("\n");
    mpp_logi("=========== vdpp test start ==============\n");
    ret = parse_cmd(argc, argv, &cfg);
    if (ret) {
        return ret;
    }

    VdppComCtx *vdppCtx = rockchip_vdpp_api_alloc_ctx();
    if (!vdppCtx) {
        mpp_loge_f("alloc vdpp context failed! Exit...\n");
        return MPP_ERR_NULL_PTR;
    }

    /* update enbale flags */
    vdpp_ver = MPP_MAX(vdppCtx->ver >> 8, 1); // {0x100, 0x200, 0x300}
    cfg.en_dmsr &= (vdpp_ver >= 1) && (cfg.work_mode != VDPP_WORK_MODE_DCI);
    cfg.en_es   &= (vdpp_ver >= 2) && (cfg.work_mode != VDPP_WORK_MODE_DCI);
    cfg.en_shp  &= (vdpp_ver >= 2) && (cfg.work_mode != VDPP_WORK_MODE_DCI);
    cfg.en_hist &= (vdpp_ver >= 2);
    cfg.en_pyr  &= (vdpp_ver >= 3) && (cfg.work_mode != VDPP_WORK_MODE_DCI);
    cfg.en_bbd  &= (vdpp_ver >= 3);
    mpp_logi(" - update enbale flags: dmsr=%d, es=%d, shp=%d, hist=%d, pyr=%d, bbd=%d\n",
             cfg.en_dmsr, cfg.en_es, cfg.en_shp, cfg.en_hist, cfg.en_pyr, cfg.en_bbd);

    /* open files */
    cfg.fp_src = fopen(cfg.src_url, "rb");
    if (cfg.fp_src == NULL) {
        mpp_loge("failed to open input file %s! %s\n", cfg.src_url, strerror(errno));
        return -1;
    }

    if (cfg.dst_url[0]) {
        cfg.fp_dst = fopen(cfg.dst_url, "wb");
        if (cfg.fp_dst == NULL) {
            mpp_logw("failed to open output file %s! %s\n", cfg.dst_url, strerror(errno));
        }
    }

    if (cfg.dst_c_url[0]) {
        cfg.fp_dst_c = fopen(cfg.dst_c_url, "wb");
        if (cfg.fp_dst_c == NULL) {
            mpp_logw("failed to open output chroma file %s! %s\n", cfg.dst_c_url, strerror(errno));
        }
    }

    if (cfg.slt_url[0]) {
        cfg.fp_slt = fopen(cfg.slt_url, "wt");
        if (cfg.fp_slt == NULL) {
            mpp_logw("failed to open output file %s! %s\n", cfg.slt_url, strerror(errno));
        }
    }

    if (cfg.out_dir[0]) {
        char filename[MAX_URL_LEN] = {0};

        snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_com.txt", cfg.out_dir);
        cfg.fp_result = fopen(filename, "wt");
        if (cfg.fp_result == NULL) {
            mpp_logw("failed to open output com file %s! %s\n", filename, strerror(errno));
        }

        if (cfg.en_hist) {
            snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_hist_packed.bin", cfg.out_dir);
            cfg.fp_hist = fopen(filename, "wb");

            snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_hist_local.bin", cfg.out_dir);
            cfg.fp_hist_l = fopen(filename, "wb");

            snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_hist_global.bin", cfg.out_dir);
            cfg.fp_hist_g = fopen(filename, "wb");
        }

        if (cfg.en_pyr) {
            for (i = 0; i < 3; ++i) {
                snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_pyr_l%d.yuv", cfg.out_dir, i + 1);
                cfg.fp_pyrs[i] = fopen(filename, "wb");
            }
        }
    }

    /* run vdpp test */
    ret = vdpp_test(vdppCtx, &cfg);

    /* close files */
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

    if (cfg.fp_result) {
        fclose(cfg.fp_result);
        cfg.fp_result = NULL;
    }

    for (i = 0; i < 3; ++i) {
        if (cfg.fp_pyrs[i]) {
            fclose(cfg.fp_pyrs[i]);
            cfg.fp_pyrs[i] = NULL;
        }
    }

    rockchip_vdpp_api_release_ctx(vdppCtx);

    mpp_logi("=========== vdpp test end ==============\n\n");
    return ret;
}
