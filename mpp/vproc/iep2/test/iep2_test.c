/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "iep2_test"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_buffer.h"

#include "iep2_api.h"
#include "utils.h"
//#include "iniparser.h"

#define MAX_URL_LEN                 (100)

typedef struct iep2_test_cfg_t {
    RK_S32      w;
    RK_S32      h;
    RK_S32      src_fmt;
    RK_S32      src_swa;

    RK_S32      dst_fmt;
    RK_S32      dst_swa;

    char        src_url[MAX_URL_LEN];
    char        dst_url[MAX_URL_LEN];
    char        slt_url[MAX_URL_LEN];

    FILE        *fp_src;
    FILE        *fp_dst;
    FILE        *fp_slt;
    RK_U32      field_order;
} iep2_test_cfg;

static OptionInfo iep2_test_cmd[] = {
    {"w",   "width",            "input image width"},
    {"h",   "height",           "input image height"},
    {"c",   "format",           "input image format in ASCII string"},
    {"i",   "src_file",         "input  image file name"},
    {"C",   "dst_format",       "output image format in ASCII string"},
    {"o",   "dst_file",         "output image file name"},
    {"v",   "slt_file",         "slt verify data file"}
};

static void iep2_test_help()
{
    mpp_log("usage: iep2_test [options]\n");
    mpp_log("*******************************\n");
    show_options(iep2_test_cmd);
    mpp_log("*******************************\n");
    mpp_log("supported ASCII format strings:\n");
    mpp_log("1 - yuv422sp\n");
    mpp_log("2 - yuv422p [just for source format]\n");
    mpp_log("3 - yuv420sp\n");
    mpp_log("4 - yuv420p [just for source format]\n");
    mpp_log("5 - yvu422sp\n");
    mpp_log("6 - yvu420sp\n");
    mpp_log("************ sample ***********\n");
    mpp_log("iep2_test -w 720 -h 480 -c yuv420p -i input.yuv -C yuv420p -o output.yuv\n");
}

static RK_S32 str_to_iep2_fmt(const char *str)
{
    RK_S32 fmt = -1;

    mpp_log("format %s\n", str);

    if (!strcmp(str, "yuv422sp") ||
        !strcmp(str, "yuv422p") ||
        !strcmp(str, "yvu422sp"))
        fmt = IEP2_FMT_YUV422;
    else if (!strcmp(str, "yuv420p") ||
             !strcmp(str, "yuv420sp") ||
             !strcmp(str, "yvu420sp"))
        fmt = IEP2_FMT_YUV420;
    else
        mpp_err("invalid format %s\n", str);

    return fmt;
}

static RK_S32 str_to_iep2_swa(const char *str)
{
    RK_S32 swa = -1;

    if (!strcmp(str, "yuv422p") ||
        !strcmp(str, "yuv420p"))
        swa = IEP2_YUV_SWAP_P;
    else if (!strcmp(str, "yvu422sp") ||
             !strcmp(str, "yvu420sp"))
        swa = IEP2_YUV_SWAP_SP_VU;
    else if (!strcmp(str, "yuv422sp") ||
             !strcmp(str, "yuv420sp"))
        swa = IEP2_YUV_SWAP_SP_UV;
    else
        mpp_err("invalid format %s\n", str);

    return swa;
}

static MPP_RET check_input_cmd(iep2_test_cfg *cfg)
{
    MPP_RET ret = MPP_OK;

    if (cfg->w < 16 || cfg->w > 1920) {
        mpp_err("invalid width %d\n", cfg->w);
        ret = MPP_NOK;
    }

    if (cfg->h < 4 || cfg->h > 1088) {
        mpp_err("invalid height %d\n", cfg->h);
        ret = MPP_NOK;
    }

    mpp_log("src fmt %d, %d dst fmt %d, %d\n",
            cfg->src_fmt, cfg->src_swa,
            cfg->dst_fmt, cfg->dst_swa);

    if (cfg->src_fmt < IEP2_FMT_YUV422 ||
        cfg->src_fmt > IEP2_FMT_YUV420 ||
        cfg->src_swa < IEP2_YUV_SWAP_SP_UV ||
        cfg->src_swa > IEP2_YUV_SWAP_P) {
        mpp_err("invalid input format\n");
        ret = MPP_NOK;
    }

    if (cfg->fp_src == NULL) {
        mpp_err("failed to open input file %s\n", cfg->src_url);
        ret = MPP_NOK;
    }

    if (cfg->dst_fmt < IEP2_FMT_YUV422 ||
        cfg->dst_fmt > IEP2_FMT_YUV420 ||
        cfg->dst_swa < IEP2_YUV_SWAP_SP_UV ||
        cfg->dst_swa > IEP2_YUV_SWAP_SP_VU) {
        mpp_err("invalid output format\n");
        ret = MPP_NOK;
    }

    return ret;
}

static inline size_t get_frm_size(RK_S32 fmt, int w, int h)
{
    switch (fmt) {
    case IEP2_FMT_YUV422:
        return w * h * 2;
    case IEP2_FMT_YUV420:
        return w * h * 3 / 2;
    default:
        abort();
        return 0;
    }
}

static void iep2_test_set_img(iep_com_ctx *ctx, int w, int h,
                              IepImg *img, RK_S32 fd, IepCmd cmd)
{
    RK_S32 y_size = w * h;
    img->mem_addr = fd;
    img->uv_addr = fd + (y_size << 10);
    switch (img->format) {
    case IEP2_FMT_YUV422:
        img->v_addr = fd + ((y_size + y_size / 2) << 10);
        break;
    case IEP2_FMT_YUV420:
        img->v_addr = fd + ((y_size + y_size / 4) << 10);
        break;
    default:
        break;
    }

    MPP_RET ret = ctx->ops->control(ctx->priv, cmd, img);
    if (ret)
        mpp_log_f("control %08x failed %d\n", cmd, ret);
}

void iep2_test(iep2_test_cfg *cfg)
{
    iep_com_ctx* iep2 = rockchip_iep2_api_alloc_ctx();
    size_t srcfrmsize = get_frm_size(cfg->src_fmt, cfg->w, cfg->h);
    size_t dstfrmsize = get_frm_size(cfg->dst_fmt, cfg->w, cfg->h);
    MppBuffer srcbuf[3];
    MppBuffer dstbuf[2];
    RK_U8 *psrc[3];
    RK_U8 *pdst[2];
    RK_S32 fdsrc[3];
    RK_S32 fddst[2];
    int prev, curr, next;
    struct iep2_api_params params;
    RK_U32 i;
    RK_S32 field_order = IEP2_FIELD_ORDER_TFF;
    RK_S32 out_order = field_order == IEP2_FIELD_ORDER_TFF ? 0 : 1;
    DataCrc checkcrc;

    // NOTISE, used IepImg structure for version compatibility consideration,
    // only addresses in this structure are useful in iep2
    IepImg imgsrc[3];
    IepImg imgdst[2];

    int idx = 0;

    mpp_assert(iep2);

    memset(&checkcrc, 0, sizeof(checkcrc));
    checkcrc.sum = mpp_malloc(RK_ULONG, 512);

    mpp_buffer_get(NULL, &srcbuf[0], srcfrmsize);
    mpp_buffer_get(NULL, &srcbuf[1], srcfrmsize);
    mpp_buffer_get(NULL, &srcbuf[2], srcfrmsize);
    mpp_buffer_get(NULL, &dstbuf[0], dstfrmsize);
    mpp_buffer_get(NULL, &dstbuf[1], dstfrmsize);
    mpp_assert(srcbuf[0] && srcbuf[1] && srcbuf[2] && dstbuf[0] && dstbuf[1]);

    psrc[0] = mpp_buffer_get_ptr(srcbuf[0]);
    psrc[1] = mpp_buffer_get_ptr(srcbuf[1]);
    psrc[2] = mpp_buffer_get_ptr(srcbuf[2]);

    pdst[0] = mpp_buffer_get_ptr(dstbuf[0]);
    pdst[1] = mpp_buffer_get_ptr(dstbuf[1]);

    fdsrc[0] = mpp_buffer_get_fd(srcbuf[0]);
    fdsrc[1] = mpp_buffer_get_fd(srcbuf[1]);
    fdsrc[2] = mpp_buffer_get_fd(srcbuf[2]);

    fddst[0] = mpp_buffer_get_fd(dstbuf[0]);
    fddst[1] = mpp_buffer_get_fd(dstbuf[1]);

    iep2->ops->init(&iep2->priv);

    if (srcfrmsize > fread(psrc[0], 1, srcfrmsize, cfg->fp_src)) {
        mpp_log("source exhaused\n");
        goto ret;
    }

    if (srcfrmsize > fread(psrc[1], 1, srcfrmsize, cfg->fp_src)) {
        mpp_log("source exhaused\n");
        goto ret;
    }

    curr = 1;

    // set running mode.
    /* if deinterlacing mode is one in the following list:
        IEP2_DIL_MODE_I5O2,
        IEP2_DIL_MODE_I5O1T,
        IEP2_DIL_MODE_I5O1B,
        IEP2_DIL_MODE_DECT
       field order will auto detect by iep2 library.
       so don't try to set field order during the playback.
    */
    params.ptype = IEP2_PARAM_TYPE_MODE;
    params.param.mode.dil_mode = IEP2_DIL_MODE_I5O2;
    params.param.mode.out_mode = IEP2_OUT_MODE_LINE;
    params.param.mode.dil_order = cfg->field_order;

    iep2->ops->control(iep2->priv, IEP_CMD_SET_DEI_CFG, &params);

    // set the image format.
    params.ptype = IEP2_PARAM_TYPE_COM;
    params.param.com.sfmt = cfg->src_fmt;
    params.param.com.dfmt = cfg->dst_fmt;
    params.param.com.sswap = cfg->src_swa;
    params.param.com.dswap = cfg->dst_swa;
    params.param.com.width = cfg->w;
    params.param.com.height = cfg->h;
    iep2->ops->control(iep2->priv, IEP_CMD_SET_DEI_CFG, &params);

    for (i = 0; i < MPP_ARRAY_ELEMS(imgsrc); i++)
        imgsrc[i].format = cfg->src_fmt;

    for (i = 0; i < MPP_ARRAY_ELEMS(imgdst); i++)
        imgdst[i].format = cfg->dst_fmt;

    while (1) {
        prev = (curr - 1) % 3;
        curr = curr % 3;
        next = (curr + 1) % 3;

        if (srcfrmsize > fread(psrc[next], 1, srcfrmsize, cfg->fp_src)) {
            mpp_log("source exhaused\n");
            break;
        }

        // notice the order of the input frames.
        iep2_test_set_img(iep2, cfg->w, cfg->h,
                          &imgsrc[curr], fdsrc[curr], IEP_CMD_SET_SRC);
        iep2_test_set_img(iep2, cfg->w, cfg->h,
                          &imgsrc[next], fdsrc[next], IEP_CMD_SET_DEI_SRC1);
        iep2_test_set_img(iep2, cfg->w, cfg->h,
                          &imgsrc[prev], fdsrc[prev], IEP_CMD_SET_DEI_SRC2);
        iep2_test_set_img(iep2, cfg->w, cfg->h,
                          &imgdst[0], fddst[0], IEP_CMD_SET_DST);
        iep2_test_set_img(iep2, cfg->w, cfg->h,
                          &imgdst[1], fddst[1], IEP_CMD_SET_DEI_DST1);

        memset(pdst[0], 0, dstfrmsize);
        memset(pdst[1], 0, dstfrmsize);
        iep2->ops->control(iep2->priv, IEP_CMD_RUN_SYNC, NULL);

        if (cfg->fp_slt) {
            calc_data_crc(pdst[out_order], dstfrmsize, &checkcrc);
            write_data_crc(cfg->fp_slt, &checkcrc);
            calc_data_crc(pdst[1 - out_order], dstfrmsize, &checkcrc);
            write_data_crc(cfg->fp_slt, &checkcrc);
        }

        if (dstfrmsize > fwrite(pdst[out_order], 1, dstfrmsize, cfg->fp_dst)) {
            mpp_err("destination dump failed\n");
            break;
        }

        if (dstfrmsize > fwrite(pdst[1 - out_order], 1, dstfrmsize, cfg->fp_dst)) {
            mpp_err("destination dump failed\n");
            break;
        }

        curr++;
        idx++;
    }

ret:
    mpp_buffer_put(srcbuf[0]);
    mpp_buffer_put(srcbuf[1]);
    mpp_buffer_put(srcbuf[2]);

    mpp_buffer_put(dstbuf[0]);
    mpp_buffer_put(dstbuf[1]);

    MPP_FREE(checkcrc.sum);

    iep2->ops->deinit(iep2->priv);

    rockchip_iep2_api_release_ctx(iep2);
}

int main(int argc, char **argv)
{
    iep2_test_cfg cfg;
    int ch;

    if (argc < 2) {
        iep2_test_help();
        return 0;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.src_fmt = IEP2_FMT_YUV420;
    cfg.src_swa = IEP2_YUV_SWAP_SP_UV;
    cfg.dst_fmt = IEP2_FMT_YUV420;
    cfg.dst_swa = IEP2_YUV_SWAP_SP_UV;

    /// get options
    opterr = 0;
    while ((ch = getopt(argc, argv, "i:w:h:c:o:C:v:f:")) != -1) {
        switch (ch) {
        case 'w': {
            cfg.w = atoi(optarg);
        } break;
        case 'h': {
            cfg.h = atoi(optarg);
        } break;
        case 'c': {
            cfg.src_fmt = str_to_iep2_fmt(optarg);
            cfg.src_swa = str_to_iep2_swa(optarg);
        } break;
        case 'C': {
            cfg.dst_fmt = str_to_iep2_fmt(optarg);
            cfg.dst_swa = str_to_iep2_swa(optarg);
        } break;
        case 'i': {
            mpp_log("input filename: %s\n", optarg);
            strncpy(cfg.src_url, optarg, sizeof(cfg.src_url) - 1);
            cfg.fp_src = fopen(cfg.src_url, "rb");
        } break;
        case 'o': {
            mpp_log("output filename: %s\n", optarg);
            strncpy(cfg.dst_url, optarg, sizeof(cfg.dst_url) - 1);
            cfg.fp_dst = fopen(cfg.dst_url, "w+b");
        } break;
        case 'v': {
            mpp_log("verify file: %s\n", optarg);
            strncpy(cfg.slt_url, optarg, sizeof(cfg.slt_url) - 1);
            cfg.fp_slt = fopen(cfg.slt_url, "w+b");
        } break;
        case 'f': {
            if (!strcmp(optarg, "TFF"))
                cfg.field_order = IEP2_FIELD_ORDER_TFF;
            else
                cfg.field_order = IEP2_FIELD_ORDER_BFF;
        } break;
        default: {
        } break;
        }
    }

    if (check_input_cmd(&cfg)) {
        mpp_err("failed to pass cmd line check\n");
        iep2_test_help();
        return -1;
    }

    iep2_test(&cfg);

    if (cfg.fp_src) {
        fclose(cfg.fp_src);
        cfg.fp_src = NULL;
    }

    if (cfg.fp_dst) {
        fclose(cfg.fp_dst);
        cfg.fp_dst = NULL;
    }

    if (cfg.fp_slt) {
        fclose(cfg.fp_slt);
        cfg.fp_slt = NULL;
    }

    return 0;
}
