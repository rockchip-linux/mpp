/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hwpq_vdpp"

#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <errno.h>

#include "mpp_mem.h"
#include "mpp_buffer.h"

#include "hwpq_debug.h"
#include "hwpq_vdpp_proc_api.h"

typedef struct {
    char src_file_name[128];
    char dst_file_name_y[128];
    char dst_file_name_uv[128];
    char dst_file_name_hist[128];
    unsigned int img_w_i;
    unsigned int img_h_i;
    unsigned int img_w_i_vir;
    unsigned int img_h_i_vir;
    unsigned int img_w_o;
    unsigned int img_h_o;
    unsigned int img_w_o_vir;
    unsigned int img_h_o_vir;
    unsigned int uv_diff_flag;
    unsigned int img_w_o_c;
    unsigned int img_h_o_c;
    unsigned int img_w_o_c_vir;
    unsigned int img_h_o_c_vir;

    unsigned int work_mode;

    int32_t      nthreads;
    int32_t      frame_num;
} VdppCmdCfg;

typedef struct {
    int             chn;

    FILE           *fp_i;
    FILE           *fp_o_y;
    FILE           *fp_o_uv;
    FILE           *fp_o_h;

    unsigned int    frm_eos;
    unsigned int    loop_times;
} VdppTestMultiCtx;

typedef struct {
    VdppCmdCfg           *cmd;
    int                   chn;

    pthread_t             thd;     // thread for for each instance
    VdppTestMultiCtx      ctx;     // context of vdpp
} VdppTestMultiCtxInfo;

static void parse_cmd(char** argv, int argc, VdppCmdCfg* p_cmd_cfg);

extern char *optarg;
extern int   opterr;

static void *multi_vdpp(void *cmd_ctx)
{
    VdppTestMultiCtxInfo *info = (VdppTestMultiCtxInfo *)cmd_ctx;
    VdppTestMultiCtx *ctx = &info->ctx;
    VdppCmdCfg *cfg = info->cmd;

    HwpqVdppContext vdpp_ctx = NULL; // api ctx
    HwpqVdppParams vdpp_proc_cfg = {0};

    RK_U32 yuv_out_diff = cfg->uv_diff_flag;
    size_t srcfrmsize = cfg->img_w_i_vir * cfg->img_h_i_vir * 3 / 2;
    size_t dstfrmsize = cfg->img_w_o_vir * cfg->img_h_o_vir * 3;
    size_t dstfrmsize_c = cfg->img_w_o_c_vir * cfg->img_h_o_c_vir * 2;

    // malloc buffers
    MppBuffer srcbuf;
    MppBuffer dstbuf;
    MppBuffer dstbuf_c;
    MppBuffer histbuf;
    void *psrc = NULL;
    void *pdst = NULL;
    void *phist = NULL;
    RK_S32 fdsrc = -1;
    RK_S32 fddst = -1;
    RK_S32 fdhist = -1;
    int frame_idx = 0;
    MppBufferGroup memGroup;
    MPP_RET ret = 0;

    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        hwpq_loge("memGroup mpp_buffer_group_get failed!\n");
        return NULL;
    }

    mpp_buffer_get(memGroup, &srcbuf, srcfrmsize);
    mpp_buffer_get(memGroup, &dstbuf, dstfrmsize);
    mpp_buffer_get(memGroup, &dstbuf_c, dstfrmsize_c);
    mpp_buffer_get(memGroup, &histbuf, VDPP_HIST_LENGTH);
    psrc    = mpp_buffer_get_ptr(srcbuf);
    pdst    = mpp_buffer_get_ptr(dstbuf);
    phist   = mpp_buffer_get_ptr(histbuf);

    fdsrc   = mpp_buffer_get_fd(srcbuf);
    fddst   = mpp_buffer_get_fd(dstbuf);
    fdhist  = mpp_buffer_get_fd(histbuf);

    hwpq_logi("src w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_i,
              cfg->img_h_i, cfg->img_w_i_vir, cfg->img_h_i_vir, srcfrmsize, fdsrc);
    hwpq_logi("dst w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_o,
              cfg->img_h_o, cfg->img_w_o_vir, cfg->img_h_o_vir, dstfrmsize, fddst);
    if (yuv_out_diff) {
        hwpq_logi("dst_c w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_o_c,
                  cfg->img_h_o_c, cfg->img_w_o_c_vir, cfg->img_h_o_c_vir, dstfrmsize_c, fddst);
    }
    hwpq_vdpp_init(&vdpp_ctx);

    ctx->chn = info->chn;

    ctx->fp_i = fopen(cfg->src_file_name, "rb");
    if (!ctx->fp_i) {
        hwpq_loge("failed to open file %s ! %s", cfg->src_file_name, strerror(errno));
        goto __RET;
    }

    ctx->fp_o_y = fopen(cfg->dst_file_name_y, "wb");
    ctx->fp_o_uv = fopen(cfg->dst_file_name_uv, "wb");
    ctx->fp_o_h = fopen(cfg->dst_file_name_hist, "wb");

    while (1) {
        vdpp_proc_cfg.frame_idx = frame_idx++;

        if ((srcfrmsize > fread(psrc, 1, srcfrmsize, ctx->fp_i)) || feof(ctx->fp_i)) {
            ctx->frm_eos = 1;

            if (cfg->frame_num < 0 || frame_idx < cfg->frame_num) {
                clearerr(ctx->fp_i);
                rewind(ctx->fp_i);
                ctx->frm_eos = 0;
                hwpq_logi("chn %d loop times %d\n", ctx->chn, ++ctx->loop_times);
                continue;
            }
            hwpq_logi("chn %d found last frame. feof %d\n", ctx->chn, feof(ctx->fp_i));
        } else if (ret == MPP_ERR_VALUE)
            break;

        vdpp_proc_cfg.src_img_info.img_fmt = VDPP_FMT_NV12;
        vdpp_proc_cfg.src_img_info.img_yrgb.fd = fdsrc;
        vdpp_proc_cfg.src_img_info.img_yrgb.addr = psrc;
        vdpp_proc_cfg.src_img_info.img_yrgb.offset = 0;
        vdpp_proc_cfg.src_img_info.img_yrgb.w_vld = cfg->img_w_i;
        vdpp_proc_cfg.src_img_info.img_yrgb.h_vld = cfg->img_h_i;
        vdpp_proc_cfg.src_img_info.img_yrgb.w_vir = cfg->img_w_i_vir;
        vdpp_proc_cfg.src_img_info.img_yrgb.h_vir = cfg->img_h_i_vir;

        vdpp_proc_cfg.src_img_info.img_cbcr.fd = fdsrc;
        vdpp_proc_cfg.src_img_info.img_cbcr.addr = psrc;
        vdpp_proc_cfg.src_img_info.img_cbcr.offset = cfg->img_w_i_vir * cfg->img_h_i_vir;
        vdpp_proc_cfg.src_img_info.img_cbcr.w_vld = cfg->img_w_i / 2;
        vdpp_proc_cfg.src_img_info.img_cbcr.h_vld = cfg->img_h_i / 2;
        vdpp_proc_cfg.src_img_info.img_cbcr.w_vir = cfg->img_w_i_vir / 2;
        vdpp_proc_cfg.src_img_info.img_cbcr.h_vir = cfg->img_h_i_vir / 2;

        vdpp_proc_cfg.dst_img_info.img_fmt = VDPP_FMT_NV24;
        vdpp_proc_cfg.dst_img_info.img_yrgb.fd = fddst;
        vdpp_proc_cfg.dst_img_info.img_yrgb.addr = pdst;
        vdpp_proc_cfg.dst_img_info.img_yrgb.offset = 0;
        vdpp_proc_cfg.dst_img_info.img_yrgb.w_vld = cfg->img_w_o;
        vdpp_proc_cfg.dst_img_info.img_yrgb.h_vld = cfg->img_h_o;
        vdpp_proc_cfg.dst_img_info.img_yrgb.w_vir = cfg->img_w_o_vir;
        vdpp_proc_cfg.dst_img_info.img_yrgb.h_vir = cfg->img_h_o_vir;

        vdpp_proc_cfg.dst_img_info.img_cbcr.fd = fddst;
        vdpp_proc_cfg.dst_img_info.img_cbcr.addr = pdst;
        vdpp_proc_cfg.dst_img_info.img_cbcr.offset = cfg->img_w_o_vir * cfg->img_h_o_vir;
        vdpp_proc_cfg.dst_img_info.img_cbcr.w_vld = cfg->img_w_o_c;
        vdpp_proc_cfg.dst_img_info.img_cbcr.h_vld = cfg->img_h_o_c;
        vdpp_proc_cfg.dst_img_info.img_cbcr.w_vir = cfg->img_w_o_c_vir;
        vdpp_proc_cfg.dst_img_info.img_cbcr.h_vir = cfg->img_h_o_c_vir;

        {
            int work_mode_ref = hwpq_vdpp_check_work_mode(vdpp_ctx, &vdpp_proc_cfg);

            vdpp_proc_cfg.hist_mode_en = (VDPP_RUN_MODE_HIST == cfg->work_mode) ||
                                         (VDPP_RUN_MODE_HIST == work_mode_ref);
        }

        vdpp_proc_cfg.vdpp_config.hist_cnt_en = 1;
        vdpp_proc_cfg.hist_buf_fd = fdhist;
        vdpp_proc_cfg.p_hist_buf = phist;

        vdpp_proc_cfg.yuv_diff_flag = yuv_out_diff;
        vdpp_proc_cfg.vdpp_config_update_flag = (0 == vdpp_proc_cfg.frame_idx);

        ret = hwpq_vdpp_proc(vdpp_ctx, &vdpp_proc_cfg);
        if (ret)
            break;

        /* wirte output data */
        if (ctx->fp_o_y && yuv_out_diff) {
            fwrite(vdpp_proc_cfg.dst_img_info.img_yrgb.addr, 1, cfg->img_w_o_vir * cfg->img_h_o_vir, ctx->fp_o_y);
            fwrite(vdpp_proc_cfg.dst_img_info.img_cbcr.addr + vdpp_proc_cfg.dst_img_info.img_cbcr.offset, 1,
                   dstfrmsize_c, ctx->fp_o_y); // write chroma data after dst luma
            if (ctx->fp_o_uv) {
                fwrite(vdpp_proc_cfg.dst_img_info.img_cbcr.addr + vdpp_proc_cfg.dst_img_info.img_cbcr.offset, 1,
                       dstfrmsize_c, ctx->fp_o_uv);
            }
        } else if (ctx->fp_o_y) {
            fwrite(vdpp_proc_cfg.dst_img_info.img_yrgb.addr, 1, dstfrmsize, ctx->fp_o_y);
        }

        if (ctx->fp_o_h) {
            fwrite(vdpp_proc_cfg.p_hist_buf, 1, VDPP_HIST_LENGTH, ctx->fp_o_h);
            hwpq_logi("packed hist dump to: %s\n", cfg->dst_file_name_hist);
            hwpq_logi("get mean luma result: %d (in U10)\n", vdpp_proc_cfg.output.luma_avg);
        }

        if (cfg->frame_num > 0 && frame_idx >= cfg->frame_num) {
            ctx->frm_eos = 1;
            break;
        }

        if (ctx->frm_eos)
            break;
    }

__RET:
    if (ctx->fp_i) {
        fclose(ctx->fp_i);
        ctx->fp_i = NULL;
    }
    if (ctx->fp_o_y) {
        fclose(ctx->fp_o_y);
        ctx->fp_o_y = NULL;
    }
    if (ctx->fp_o_uv) {
        fclose(ctx->fp_o_uv);
        ctx->fp_o_uv = NULL;
    }
    if (ctx->fp_o_h) {
        fclose(ctx->fp_o_h);
        ctx->fp_o_h = NULL;
    }

    mpp_buffer_put(srcbuf);
    mpp_buffer_put(dstbuf);
    mpp_buffer_put(histbuf);
    mpp_buffer_put(dstbuf_c);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    hwpq_vdpp_deinit(vdpp_ctx);

    return NULL;
}

int32_t main(int argc, char **argv)
{
    VdppCmdCfg vdpp_cmd_cfg;
    VdppCmdCfg *p_cmd_cfg = &vdpp_cmd_cfg;
    VdppTestMultiCtxInfo *ctxs = NULL;
    int i = 0;
    int ret = 0;

    hwpq_logi("\n");
    hwpq_logi("=========== hwpq test start ==============\n");
    parse_cmd(argv, argc, p_cmd_cfg);

    ctxs = mpp_calloc(VdppTestMultiCtxInfo, p_cmd_cfg->nthreads);
    if (NULL == ctxs) {
        hwpq_loge("failed to alloc context for instances!\n");
        ret = MPP_ERR_MALLOC;
        goto __RET;
    }

    for (i = 0; i < p_cmd_cfg->nthreads; i++) {
        ctxs[i].cmd = p_cmd_cfg;
        ctxs[i].chn = i;

        ret = pthread_create(&ctxs[i].thd, NULL, multi_vdpp, &ctxs[i]);
        if (ret) {
            hwpq_loge("failed to create thread %d\n", i);
            ret = MPP_NOK;
            goto __RET;
        }
    }

    for (i = 0; i < p_cmd_cfg->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

__RET:
    MPP_FREE(ctxs);
    ctxs = NULL;

    hwpq_logi("=========== hwpq test end ==============\n\n");
    return ret;
}

static void parse_cmd(char** argv, int argc, VdppCmdCfg* cfg)
{
    int32_t ch;
    int32_t option_index = 0;
    static struct option long_options[] = {
        {"ip",         required_argument, 0,  0 },
        {"oy",         required_argument, 0,  0 },
        {"oc",         required_argument, 0,  0 },
        {"oh",         required_argument, 0,  0 },
        {"wi_vld",     required_argument, 0,  0 },
        {"hi_vld",     required_argument, 0,  0 },
        {"wi_vir",     required_argument, 0,  0 },
        {"hi_vir",     required_argument, 0,  0 },
        {"wo_vld",     required_argument, 0,  0 },
        {"ho_vld",     required_argument, 0,  0 },
        {"wo_vir",     required_argument, 0,  0 },
        {"ho_vir",     required_argument, 0,  0 },
        {"uv_diff",    required_argument, 0,  0 },
        {"wo_uv",      required_argument, 0,  0 },
        {"ho_uv",      required_argument, 0,  0 },
        {"wo_uv_vir",  required_argument, 0,  0 },
        {"ho_uv_vir",  required_argument, 0,  0 },
        {"work_mode",  required_argument, 0,  0 },
        {"nthread",    required_argument, 0,  0 },
        {"frame_num",  required_argument, 0,  0 },
        { 0,           0,                 0,  0 },
    };

    opterr = 0;

    /* set default value */
    memset(cfg, 0, sizeof(VdppCmdCfg));
    cfg->work_mode = 0;
    cfg->nthreads = 1;
    cfg->frame_num = 1;

    /* parse arguments */
    while ((ch = getopt_long_only(argc, argv, "", long_options, &option_index)) != -1) {
        switch (ch) {
        case 0: {
            switch (option_index) {
            case 0 : {
                strncpy(cfg->src_file_name, optarg, sizeof(cfg->src_file_name) - 1);
                hwpq_logi("ssrc file name: %s\n", cfg->src_file_name);
            } break;
            case 1 : {
                strncpy(cfg->dst_file_name_y, optarg, sizeof(cfg->dst_file_name_y) - 1);
                hwpq_logi("ddst-Y file name: %s\n", cfg->dst_file_name_y);
            } break;
            case 2 : {
                strncpy(cfg->dst_file_name_uv, optarg, sizeof(cfg->dst_file_name_uv) - 1);
                hwpq_logi("ddst-UV file name: %s\n", cfg->dst_file_name_uv);
            } break;
            case 3 : {
                strncpy(cfg->dst_file_name_hist, optarg, sizeof(cfg->dst_file_name_hist) - 1);
                hwpq_logi("ddst-Hist file name: %s\n", cfg->dst_file_name_hist);
            } break;
            case 4 : {
                cfg->img_w_i = atoi(optarg);
            } break;
            case 5 : {
                cfg->img_h_i = atoi(optarg);
            } break;
            case 6 : {
                cfg->img_w_i_vir = atoi(optarg);
            } break;
            case 7 : {
                cfg->img_h_i_vir = atoi(optarg);
            } break;
            case 8 : {
                cfg->img_w_o = atoi(optarg);
            } break;
            case 9 : {
                cfg->img_h_o = atoi(optarg);
            } break;
            case 10 : {
                cfg->img_w_o_vir = atoi(optarg);
            } break;
            case 11 : {
                cfg->img_h_o_vir = atoi(optarg);
            } break;
            case 12 : {
                cfg->uv_diff_flag = atoi(optarg);
            } break;
            case 13 : {
                cfg->img_w_o_c = atoi(optarg);
            } break;
            case 14 : {
                cfg->img_h_o_c = atoi(optarg);
            } break;
            case 15 : {
                cfg->img_w_o_c_vir = atoi(optarg);
            } break;
            case 16 : {
                cfg->img_h_o_c_vir = atoi(optarg);
            } break;
            case 17 : {
                cfg->work_mode = atoi(optarg);
            } break;
            case 18 : {
                cfg->nthreads = atoi(optarg);
                if (cfg->nthreads < 1)
                    cfg->nthreads = 1;
            } break;
            case 19: {
                cfg->frame_num = atoi(optarg);
            } break;
            default : {
            } break;
            }
            hwpq_logi("%s: %s", long_options[option_index].name, optarg);

        } break;
        default: {
        } break;
        }
    }

    /* check & update */
    if (cfg->uv_diff_flag == 0) {
        cfg->img_w_o_c = cfg->img_w_o;
        cfg->img_h_o_c = cfg->img_h_o;
        cfg->img_w_o_c_vir = cfg->img_w_o_vir;
        cfg->img_h_o_c_vir = cfg->img_h_o_vir;
    }
}
