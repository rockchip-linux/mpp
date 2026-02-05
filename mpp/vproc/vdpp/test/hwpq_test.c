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
#include "mpp_common.h"
#include "mpp_info.h"
#include "mpp_log.h"

#include "hwpq_vdpp_proc_api.h"

#define MAX_URL_LEN         (256)
#define VDPP_WORK_MODE_VEP  (2)
#define VDPP_WORK_MODE_DCI  (3) /* hist only mode */

typedef struct {
    char src_file_name[MAX_URL_LEN];
    char dst_file_name_y[MAX_URL_LEN];
    char dst_file_name_uv[MAX_URL_LEN];
    char dst_dir_name[MAX_URL_LEN];

    RK_S32 img_fmt_i;  // vdpp_frame_format
    RK_S32 img_swap_i; // uv swap flag
    RK_S32 img_range;  // 0-limited, 1-full
    RK_S32 img_w_i;
    RK_S32 img_h_i;
    RK_S32 img_w_i_vir;
    RK_S32 img_h_i_vir;

    RK_S32 img_fmt_o;  // vdpp_frame_format
    RK_S32 img_swap_o; // uv swap flag
    RK_S32 img_w_o;
    RK_S32 img_h_o;
    RK_S32 img_w_o_vir;
    RK_S32 img_h_o_vir;

    RK_S32 uv_diff_flag;  // set to 1 if need to separate the output luma & chroma data
    RK_S32 img_w_o_c;     // same to luma, no need to half for YUV420!
    RK_S32 img_h_o_c;     // same to luma, no need to half for YUV420!
    RK_S32 img_w_o_c_vir; // same to luma, no need to half for YUV420!
    RK_S32 img_h_o_c_vir; // same to luma, no need to half for YUV420!

    RK_S32 work_mode; // 2-vep, 3-dci_hist, otherwise-unsupported
    RK_S32 nthreads;
    RK_S32 frame_num; // default: 1. <0 means infinite loop

    /* high 16 bit: mask; low 3 bit (msb->lsb): dmsr|es|sharp */
    RK_S32 cfg_set; // used in com2, reset enable flag of dmsr/es/sharp
    RK_S32 en_dmsr;
    RK_S32 en_es;
    RK_S32 en_shp;
    RK_S32 en_hist;
    RK_S32 en_pyr;
    RK_S32 en_bbd;
    RK_S32 bbd_th; // default: 20 in U8 range
} VdppCmdCfg;

typedef struct {
    RK_S32 chn; // thread id, [0, nthreads-1]

    FILE *fp_i;
    FILE *fp_o_y;
    FILE *fp_o_uv;
    FILE *fp_o_hist;
    FILE *fp_o_pyrs[3];
    FILE *fp_o_res;

    RK_U32 frm_eos; // frame end-of-stream flag
    RK_U32 loop_times;
} VdppTestMultiCtx;

typedef struct {
    VdppCmdCfg *cmd;        // point to the same cmd config
    RK_S32 chn;             // thread id, [0, nthreads-1]

    pthread_t thd;          // thread for for each instance
    VdppTestMultiCtx ctx;   // context of vdpp
} VdppTestMultiCtxInfo;

static RK_S32 parse_cmd(char **argv, RK_S32 argc, VdppCmdCfg *cfg);
static void print_help_info();

// static RK_S32 get_vdpp_format_uv_sample_shift(RK_S32 fmt, RK_S32 *p_ret_ratiox2);
static size_t get_frame_size(RK_S32 fmt, RK_U32 w, RK_U32 h);

extern char *optarg;
extern int opterr;

static void *multi_vdpp(void *cmd_ctx)
{
    VdppTestMultiCtxInfo *info = (VdppTestMultiCtxInfo *)cmd_ctx;
    VdppTestMultiCtx *mul_ctx = &info->ctx;
    VdppCmdCfg *cfg = info->cmd;

    HwpqVdppContext vdpp_ctx = NULL; // api ctx
    HwpqVdppParams vdpp_params = {0};
    HwpqVdppQueryInfo vdpp_info = {0};
    HwpqVdppOutput vdpp_output = {0};
    RK_U32 vdpp_ver = 1;
    RK_U32 yuv_out_diff = cfg->uv_diff_flag;
    size_t srcfrmsize = 0;
    size_t dstfrmsize = 0;
    size_t dstfrmsize_c = 0;
    RK_U32 pyr_vir_wids[3] = {0};
    RK_U32 pyr_vir_hgts[3] = {0};
    RK_U32 pyr_buf_sizes[3]  = {0};
    RK_S32 pyr_fds[3] = { -1, -1, -1};
    void *pyr_addrs[3] = {0};

    MppBufferGroup memGroup = NULL;
    MppBuffer srcbuf = NULL;
    MppBuffer dstbuf = NULL;
    MppBuffer histbuf = NULL;
    MppBuffer pyrbuf_l1 = NULL;
    MppBuffer pyrbuf_l2 = NULL;
    MppBuffer pyrbuf_l3 = NULL;
    void *psrc = NULL;
    void *pdst = NULL;
    void *phist = NULL;
    RK_S32 fdsrc = -1;
    RK_S32 fddst = -1;
    RK_S32 fdhist = -1;
    RK_S32 frame_idx = 0;
    RK_S32 i = 0;
    RK_S32 ret = 0;

    /* init vdpp context */
    ret = hwpq_vdpp_init(&vdpp_ctx);
    if (ret) {
        mpp_loge("failed to init hwpq_vdpp context! %d", ret);
        goto __RET;
    }

    /* run cmds, query vdpp info */
    ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_GET_VERSION, &vdpp_info,
                            sizeof(vdpp_info), NULL);
    vdpp_ver = vdpp_info.version.vdpp_ver >> 8; // {0x100, 0x200, 0x300} => {1, 2, 3}

    ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_GET_SOC_NAME, &vdpp_info,
                            sizeof(vdpp_info), NULL);
    mpp_logi("get vdpp_info: vdpp_ver: %d, soc: %s\n", vdpp_ver, vdpp_info.platform.soc_name);

    memset(&vdpp_info, 0, sizeof(vdpp_info));
    vdpp_info.pyr.dst_width = cfg->img_w_o;
    vdpp_info.pyr.dst_height = cfg->img_h_o;
    ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_GET_PYR_MIN_SIZE,
                            &vdpp_info, sizeof(vdpp_info), NULL);
    if (0 == ret) {
        pyr_vir_wids[0] = vdpp_info.pyr.vir_widths[0];
        pyr_vir_wids[1] = vdpp_info.pyr.vir_widths[1];
        pyr_vir_wids[2] = vdpp_info.pyr.vir_widths[2];
        pyr_vir_hgts[0] = vdpp_info.pyr.vir_heights[0];
        pyr_vir_hgts[1] = vdpp_info.pyr.vir_heights[1];
        pyr_vir_hgts[2] = vdpp_info.pyr.vir_heights[2];
    } else {
        mpp_logw("failed to run cmd: HWPQ_VDPP_CMD_GET_PYR_MIN_SIZE  %d", ret);
        pyr_vir_wids[0] = MPP_ALIGN(cfg->img_w_o / 2, 16);
        pyr_vir_wids[1] = MPP_ALIGN(cfg->img_w_o / 4, 16);
        pyr_vir_wids[2] = MPP_ALIGN(cfg->img_w_o / 8, 16);
        pyr_vir_hgts[0] = (cfg->img_h_o + 1) / 2;
        pyr_vir_hgts[1] = (cfg->img_h_o + 3) / 4;
        pyr_vir_hgts[2] = (cfg->img_h_o + 7) / 8;
    }
    pyr_buf_sizes[0] = pyr_vir_wids[0] * pyr_vir_hgts[0];
    pyr_buf_sizes[1] = pyr_vir_wids[1] * pyr_vir_hgts[1];
    pyr_buf_sizes[2] = pyr_vir_wids[2] * pyr_vir_hgts[2];

    ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_SET_DEF_CFG, &vdpp_params.vdpp_config,
                            sizeof(HwpqVdppConfig), NULL);
    if (ret) {
        mpp_logw("failed to run cmd: HWPQ_VDPP_CMD_SET_DEF_CFG  %d", ret);
    }

    /* update config with known info */
    yuv_out_diff = yuv_out_diff && (vdpp_ver >= 2);
    cfg->en_dmsr &= (vdpp_ver >= 1) && (cfg->work_mode != VDPP_WORK_MODE_DCI);
    cfg->en_es   &= (vdpp_ver >= 2) && (cfg->work_mode != VDPP_WORK_MODE_DCI);
    cfg->en_shp  &= (vdpp_ver >= 2) && (cfg->work_mode != VDPP_WORK_MODE_DCI);
    cfg->en_hist &= (vdpp_ver >= 2);
    cfg->en_pyr  &= (vdpp_ver >= 3) && (cfg->work_mode != VDPP_WORK_MODE_DCI);
    cfg->en_bbd  &= (vdpp_ver >= 3);
    mpp_logi("update enbale flags: dmsr=%d, es=%d, shp=%d, hist=%d, pyr=%d, bbd=%d\n",
             cfg->en_dmsr, cfg->en_es, cfg->en_shp, cfg->en_hist, cfg->en_pyr, cfg->en_bbd);
    mpp_logi("is_yuv_out_diff: %d, work_mode: %d\n", yuv_out_diff, cfg->work_mode);

    srcfrmsize = get_frame_size(cfg->img_fmt_i, cfg->img_w_i_vir, cfg->img_h_i_vir);
    dstfrmsize = get_frame_size(cfg->img_fmt_o, cfg->img_w_o_vir, cfg->img_h_o_vir);
    dstfrmsize_c = get_frame_size(VDPP_FMT_UV_ONLY_8BIT, cfg->img_w_o_c_vir, cfg->img_h_o_c_vir);

    /* malloc buffers */
    ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_DRM);
    if (MPP_OK != ret) {
        mpp_loge("memGroup mpp_buffer_group_get failed!\n");
        return NULL;
    }

    mpp_buffer_get(memGroup, &srcbuf, srcfrmsize);
    mpp_buffer_get(memGroup, &dstbuf, dstfrmsize);
    mpp_buffer_get(memGroup, &histbuf, VDPP_HIST_LENGTH);
    mpp_buffer_get(memGroup, &pyrbuf_l1, pyr_buf_sizes[0]);
    mpp_buffer_get(memGroup, &pyrbuf_l2, pyr_buf_sizes[1]);
    mpp_buffer_get(memGroup, &pyrbuf_l3, pyr_buf_sizes[2]);

    psrc = mpp_buffer_get_ptr(srcbuf);
    pdst = mpp_buffer_get_ptr(dstbuf);
    phist = mpp_buffer_get_ptr(histbuf);
    pyr_addrs[0] = mpp_buffer_get_ptr(pyrbuf_l1);
    pyr_addrs[1] = mpp_buffer_get_ptr(pyrbuf_l2);
    pyr_addrs[2] = mpp_buffer_get_ptr(pyrbuf_l3);

    fdsrc = mpp_buffer_get_fd(srcbuf);
    fddst = mpp_buffer_get_fd(dstbuf);
    fdhist = mpp_buffer_get_fd(histbuf);
    pyr_fds[0] = mpp_buffer_get_fd(pyrbuf_l1);
    pyr_fds[1] = mpp_buffer_get_fd(pyrbuf_l2);
    pyr_fds[2] = mpp_buffer_get_fd(pyrbuf_l3);

    mpp_logi("src w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_i,
             cfg->img_h_i, cfg->img_w_i_vir, cfg->img_h_i_vir, srcfrmsize, fdsrc);
    mpp_logi("dst w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_o,
             cfg->img_h_o, cfg->img_w_o_vir, cfg->img_h_o_vir, dstfrmsize, fddst);
    if (yuv_out_diff) {
        mpp_logi("dst_c w:h [%d:%d] stride [%d:%d] require buf %d bytes. fd %d", cfg->img_w_o_c,
                 cfg->img_h_o_c, cfg->img_w_o_c_vir, cfg->img_h_o_c_vir, dstfrmsize_c, fddst);
    }
    for (i = 0; i < 3; i++) {
        mpp_logi("pyr[%d] info: size=%ux%u, require buf %u bytes. fd %d\n",
                 i + 1, pyr_vir_wids[i], pyr_vir_hgts[i], pyr_buf_sizes[i], pyr_fds[i]);
    }

    /* open files */
    mul_ctx->chn = info->chn;
    mul_ctx->fp_i = fopen(cfg->src_file_name, "rb");
    if (!mul_ctx->fp_i) {
        mpp_loge("failed to open input file %s! %s", cfg->src_file_name, strerror(errno));
        goto __RET;
    }

    if (cfg->dst_file_name_y[0]) {
        mul_ctx->fp_o_y = fopen(cfg->dst_file_name_y, "wb");
        if (mul_ctx->fp_o_y == NULL) {
            mpp_logw("failed to open output file %s! %s\n", cfg->dst_file_name_y, strerror(errno));
        }
    }

    if (cfg->dst_file_name_uv[0]) {
        mul_ctx->fp_o_uv = fopen(cfg->dst_file_name_uv, "wb");
        if (mul_ctx->fp_o_uv == NULL) {
            mpp_logw("failed to open output chroma file %s! %s\n",
                     cfg->dst_file_name_uv, strerror(errno));
        }
    }

    if (cfg->dst_dir_name[0]) {
        char filename[MAX_URL_LEN] = {0};

        snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_com.txt", cfg->dst_dir_name);
        mul_ctx->fp_o_res = fopen(filename, "wt");
        if (mul_ctx->fp_o_res == NULL) {
            mpp_logw("failed to open output com file %s! %s\n", filename, strerror(errno));
        }

        if (cfg->en_hist) {
            snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_hist_packed.bin", cfg->dst_dir_name);
            mul_ctx->fp_o_hist = fopen(filename, "wb");
        }
        if (cfg->en_pyr) {
            for (i = 0; i < 3; i++) {
                snprintf(filename, MAX_URL_LEN - 1, "%s/vdpp_res_pyr_l%d.yuv",
                         cfg->dst_dir_name, i + 1);
                mul_ctx->fp_o_pyrs[i] = fopen(filename, "wb");
            }
        }
    }

    /* set parameters: switch module flag */
    {
        vdpp_params.vdpp_config.dmsr_en = cfg->en_dmsr;
        vdpp_params.vdpp_config.es_en = cfg->en_es;
        vdpp_params.vdpp_config.shp_en = cfg->en_shp;
        vdpp_params.vdpp_config.hist_cnt_en = cfg->en_hist;
        vdpp_params.vdpp_config.hist_csc_range = cfg->img_range;
        vdpp_params.vdpp_config.pyr_en = cfg->en_pyr;
        vdpp_params.vdpp_config.bbd_en = cfg->en_bbd;
        mpp_logi("modules to run: dmsr=%d, es=%d, shp=%d, hist=%d, pyr=%d, bbd=%d\n",
                 cfg->en_dmsr, cfg->en_es, cfg->en_shp, cfg->en_hist, cfg->en_pyr, cfg->en_bbd);
    }

    /* set parameters: set buffer info */
    {
        vdpp_params.src_img_info.img_fmt = cfg->img_fmt_i;
        vdpp_params.src_img_info.img_yrgb.fd = fdsrc;
        vdpp_params.src_img_info.img_yrgb.addr = psrc;
        vdpp_params.src_img_info.img_yrgb.offset = 0;
        vdpp_params.src_img_info.img_yrgb.w_vld = cfg->img_w_i;
        vdpp_params.src_img_info.img_yrgb.h_vld = cfg->img_h_i;
        vdpp_params.src_img_info.img_yrgb.w_vir = cfg->img_w_i_vir;
        vdpp_params.src_img_info.img_yrgb.h_vir = cfg->img_h_i_vir;

        vdpp_params.src_img_info.img_cbcr.fd = fdsrc;
        vdpp_params.src_img_info.img_cbcr.addr = psrc;
        vdpp_params.src_img_info.img_cbcr.offset = cfg->img_w_i_vir * cfg->img_h_i_vir;
        /* no need to set chroma size for input buffer, info not used */
        // vdpp_params.src_img_info.img_cbcr.w_vld = cfg->img_w_i / 2; // NV12
        // vdpp_params.src_img_info.img_cbcr.h_vld = cfg->img_h_i / 2;
        // vdpp_params.src_img_info.img_cbcr.w_vir = cfg->img_w_i_vir / 2;
        // vdpp_params.src_img_info.img_cbcr.h_vir = cfg->img_h_i_vir / 2;

        vdpp_params.dst_img_info.img_fmt = cfg->img_fmt_o;
        vdpp_params.dst_img_info.img_yrgb.fd = fddst;
        vdpp_params.dst_img_info.img_yrgb.addr = pdst;
        vdpp_params.dst_img_info.img_yrgb.offset = 0;
        vdpp_params.dst_img_info.img_yrgb.w_vld = cfg->img_w_o;
        vdpp_params.dst_img_info.img_yrgb.h_vld = cfg->img_h_o;
        vdpp_params.dst_img_info.img_yrgb.w_vir = cfg->img_w_o_vir;
        vdpp_params.dst_img_info.img_yrgb.h_vir = cfg->img_h_o_vir;

        vdpp_params.dst_img_info.img_cbcr.fd = fddst;
        vdpp_params.dst_img_info.img_cbcr.addr = pdst;
        vdpp_params.dst_img_info.img_cbcr.offset = cfg->img_w_o_vir * cfg->img_h_o_vir;
        vdpp_params.dst_img_info.img_cbcr.w_vld = cfg->img_w_o_c;
        vdpp_params.dst_img_info.img_cbcr.h_vld = cfg->img_h_o_c;
        vdpp_params.dst_img_info.img_cbcr.w_vir = cfg->img_w_o_c_vir;
        vdpp_params.dst_img_info.img_cbcr.h_vir = cfg->img_h_o_c_vir;

        vdpp_params.hist_buf_fd = fdhist;
        vdpp_params.p_hist_buf = phist;

        for (i = 0; i < 3; ++i) {
            vdpp_params.vdpp_config.pyr_layers[i].img_yrgb.fd = pyr_fds[i];
            vdpp_params.vdpp_config.pyr_layers[i].img_yrgb.addr = pyr_addrs[i];
            vdpp_params.vdpp_config.pyr_layers[i].img_yrgb.w_vir = pyr_vir_wids[i];
            vdpp_params.vdpp_config.pyr_layers[i].img_yrgb.h_vir = pyr_vir_hgts[i];
        }
    }

    /* set parameters: remaining config */
    {
        // check if HIST_ONLY_MODE (DCI_HIST path)
        RK_S32 work_mode_ref = hwpq_vdpp_check_work_mode(vdpp_ctx, &vdpp_params);

        vdpp_params.hist_mode_en = (VDPP_WORK_MODE_DCI == cfg->work_mode) ||
                                   (VDPP_RUN_MODE_HIST == work_mode_ref);
        mpp_logi("check_work_mode: work_mode_ref=%d (0-vep, 1-dci), hist_only_mode=%d, en_hist=%d\n",
                 work_mode_ref, vdpp_params.hist_mode_en, cfg->en_hist);

        if ((VDPP_RUN_MODE_HIST == work_mode_ref) && (VDPP_WORK_MODE_DCI != cfg->work_mode)) {
            mpp_loge("invalid work_mode(%d) and cap(%d)!\n", cfg->work_mode, work_mode_ref);
            goto __RET;
        }

        vdpp_params.yuv_diff_flag = yuv_out_diff;
        vdpp_params.vdpp_config_update_flag = 1;
    }

    /* run vdpp */
    while (1) {
        /* read input frame data */
        vdpp_params.frame_idx = frame_idx++;
        vdpp_params.vdpp_config_update_flag = (0 == vdpp_params.frame_idx);

        if ((srcfrmsize > fread(psrc, 1, srcfrmsize, mul_ctx->fp_i)) || feof(mul_ctx->fp_i)) {
            mul_ctx->frm_eos = 1;

            if (cfg->frame_num < 0 || frame_idx < cfg->frame_num) {
                clearerr(mul_ctx->fp_i);
                rewind(mul_ctx->fp_i);
                mul_ctx->frm_eos = 0;
                mpp_logi("chn %d loop times %d\n", mul_ctx->chn, ++mul_ctx->loop_times);
                continue;
            }
            mpp_logi("chn %d found last frame. feof %d\n", mul_ctx->chn, feof(mul_ctx->fp_i));
        } else if (ret == MPP_ERR_VALUE)
            break;

        /* run vdpp */
        ret = hwpq_vdpp_proc(vdpp_ctx, &vdpp_params);
        if (ret)
            break;

        /* wirte output data */
        if (mul_ctx->fp_o_y) {
            if (yuv_out_diff && vdpp_params.dst_img_info.img_cbcr.addr) {
                fwrite(vdpp_params.dst_img_info.img_yrgb.addr, 1,
                       cfg->img_w_o_vir * cfg->img_h_o_vir, mul_ctx->fp_o_y);
                fwrite(vdpp_params.dst_img_info.img_cbcr.addr + vdpp_params.dst_img_info.img_cbcr.offset,
                       1, dstfrmsize_c, mul_ctx->fp_o_y); // write chroma data after dst luma
            } else {
                fwrite(vdpp_params.dst_img_info.img_yrgb.addr, 1, dstfrmsize, mul_ctx->fp_o_y);
            }
            mpp_logi("dst data dump to: %s\n", cfg->dst_file_name_y);
        }

        if (yuv_out_diff && mul_ctx->fp_o_uv && vdpp_params.dst_img_info.img_cbcr.addr) {
            fwrite(vdpp_params.dst_img_info.img_cbcr.addr + vdpp_params.dst_img_info.img_cbcr.offset,
                   1, dstfrmsize_c, mul_ctx->fp_o_uv);
            mpp_logi("dst chroma data dump to: %s\n", cfg->dst_file_name_uv);
        }

        /* get & write hist result */
        if (cfg->en_hist && mul_ctx->fp_o_hist) {
            ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_GET_HIST_RESULT, &vdpp_output,
                                    sizeof(vdpp_output), NULL);
            if (ret) {
                mpp_logw("failed to run cmd: HWPQ_VDPP_CMD_GET_HIST_RESULT  %d", ret);
            }

            fwrite(vdpp_output.hist.dci_vdpp_info.p_hist_addr, 1, VDPP_HIST_LENGTH,
                   mul_ctx->fp_o_hist);
            mpp_logi("get mean luma result: %d (in U10)\n", vdpp_output.hist.luma_avg);
            if (mul_ctx->fp_o_res) {
                fprintf(mul_ctx->fp_o_res, "mean luma result: %d\n", vdpp_output.hist.luma_avg);
                fflush(mul_ctx->fp_o_res);
            }
        }

        /* write pyr result */
        if (cfg->en_pyr && !vdpp_params.hist_mode_en) {
            for (i = 0; i < 3; ++i) {
                if (mul_ctx->fp_o_pyrs[i]) {
                    fwrite(vdpp_params.vdpp_config.pyr_layers[i].img_yrgb.addr,
                           1, pyr_buf_sizes[i], mul_ctx->fp_o_pyrs[i]);
                }
            }
        }

        /* get bbd result */
        if (cfg->en_bbd) {
            ret = hwpq_vdpp_run_cmd(vdpp_ctx, HWPQ_VDPP_CMD_GET_BBD_RESULT, &vdpp_output,
                                    sizeof(vdpp_output), NULL);
            if (ret) {
                mpp_logw("failed to run cmd: HWPQ_VDPP_CMD_GET_BBD_RESULT  %d", ret);
            }

            mpp_logi("get bbd result: top/bottom/left/right = [%d, %d, %d, %d]\n",
                     vdpp_output.bbd.bbd_size_top, vdpp_output.bbd.bbd_size_bottom,
                     vdpp_output.bbd.bbd_size_left, vdpp_output.bbd.bbd_size_right);
            if (mul_ctx->fp_o_res) {
                fprintf(mul_ctx->fp_o_res, "bbd result: [top=%d, btm=%d, left=%d, right=%d]\n",
                        vdpp_output.bbd.bbd_size_top, vdpp_output.bbd.bbd_size_bottom,
                        vdpp_output.bbd.bbd_size_left, vdpp_output.bbd.bbd_size_right);
                fflush(mul_ctx->fp_o_res);
            }
        }

        if (cfg->frame_num > 0 && frame_idx >= cfg->frame_num) {
            mul_ctx->frm_eos = 1;
            break;
        }

        if (mul_ctx->frm_eos)
            break;
    }

__RET:
    if (mul_ctx->fp_i) {
        fclose(mul_ctx->fp_i);
        mul_ctx->fp_i = NULL;
    }
    if (mul_ctx->fp_o_y) {
        fclose(mul_ctx->fp_o_y);
        mul_ctx->fp_o_y = NULL;
    }
    if (mul_ctx->fp_o_uv) {
        fclose(mul_ctx->fp_o_uv);
        mul_ctx->fp_o_uv = NULL;
    }
    if (mul_ctx->fp_o_hist) {
        fclose(mul_ctx->fp_o_hist);
        mul_ctx->fp_o_hist = NULL;
    }
    for (i = 0; i < 3; ++i) {
        if (mul_ctx->fp_o_pyrs[i]) {
            fclose(mul_ctx->fp_o_pyrs[i]);
            mul_ctx->fp_o_pyrs[i] = NULL;
        }
    }

    mpp_buffer_put(srcbuf);
    mpp_buffer_put(dstbuf);
    mpp_buffer_put(histbuf);
    mpp_buffer_put(pyrbuf_l1);
    mpp_buffer_put(pyrbuf_l2);
    mpp_buffer_put(pyrbuf_l3);

    if (memGroup) {
        mpp_buffer_group_put(memGroup);
        memGroup = NULL;
    }

    hwpq_vdpp_deinit(vdpp_ctx);

    return NULL;
}

RK_S32 main(RK_S32 argc, char **argv)
{
    VdppCmdCfg vdpp_cmd_cfg;
    VdppCmdCfg *cfg = &vdpp_cmd_cfg;
    VdppTestMultiCtxInfo *ctxs = NULL;
    RK_S32 i = 0;
    RK_S32 ret = 0;

    mpp_logi("\n");
    mpp_logi("=========== hwpq test start ==============\n");
    ret = parse_cmd(argv, argc, cfg);
    if (ret) {
        return ret;
    }

    ctxs = mpp_calloc(VdppTestMultiCtxInfo, cfg->nthreads);
    if (NULL == ctxs) {
        mpp_loge("failed to alloc context for instances!\n");
        ret = MPP_ERR_MALLOC;
        goto __RET;
    }

    for (i = 0; i < cfg->nthreads; i++) {
        ctxs[i].cmd = cfg;
        ctxs[i].chn = i;

        ret = pthread_create(&ctxs[i].thd, NULL, multi_vdpp, &ctxs[i]);
        if (ret) {
            mpp_loge("failed to create thread %d\n", i);
            ret = MPP_NOK;
            goto __RET;
        }
    }

    for (i = 0; i < cfg->nthreads; i++)
        pthread_join(ctxs[i].thd, NULL);

__RET:
    MPP_FREE(ctxs);
    ctxs = NULL;

    mpp_logi("=========== hwpq test end ==============\n\n");
    return ret;
}

static RK_S32 parse_cmd(char **argv, RK_S32 argc, VdppCmdCfg *cfg)
{
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
        {"outc",    required_argument, 0, 'O'},
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
        {"nthread", required_argument, 0, 't'},
        {0,         0,                 0, 0  },
    };

    /* set default value */
    memset(cfg, 0, sizeof(VdppCmdCfg));
    cfg->img_w_i = 1920;
    cfg->img_h_i = 1080;
    cfg->img_fmt_o = -1; // init to -1 for later checking
    cfg->work_mode = VDPP_WORK_MODE_VEP;
    cfg->nthreads = 1;
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
                cfg->img_w_i_vir = strtol(optarg, NULL, 0);
            } break;
            case 1: {
                cfg->img_h_i_vir = strtol(optarg, NULL, 0);
            } break;
            case 2: {
                cfg->img_w_o_vir = strtol(optarg, NULL, 0);
            } break;
            case 3: {
                cfg->img_h_o_vir = strtol(optarg, NULL, 0);
            } break;
            case 4: {
                cfg->img_w_o_c = strtol(optarg, NULL, 0);
            } break;
            case 5: {
                cfg->img_h_o_c = strtol(optarg, NULL, 0);
            } break;
            case 6: {
                cfg->img_w_o_c_vir = strtol(optarg, NULL, 0);
            } break;
            case 7: {
                cfg->img_h_o_c_vir = strtol(optarg, NULL, 0);
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
            print_help_info();
        } return 1;
        case 'v': {
            mpp_logi("hwpq_vdpp_proc sdk version: %s\n", HWPQ_VDPP_PROC_VERSION_STR);
            show_mpp_version();
        } return 2;
        case 'i': {
            strncpy(cfg->src_file_name, optarg, sizeof(cfg->src_file_name) - 1);
            mpp_logi(" - set input filename: %s\n", cfg->src_file_name);
        } break;
        case 'o': {
            strncpy(cfg->dst_file_name_y, optarg, sizeof(cfg->dst_file_name_y) - 1);
            mpp_logi(" - set output [luma] filename: %s\n", cfg->dst_file_name_y);
        } break;
        case 'O': {
            strncpy(cfg->dst_file_name_uv, optarg, sizeof(cfg->dst_file_name_uv) - 1);
            cfg->uv_diff_flag = 1;
            mpp_logi(" - set yuv_out_diff=1 since output chroma filename set: %s\n",
                     cfg->dst_file_name_uv);
        } break;
        case 'd': {
            strncpy(cfg->dst_dir_name, optarg, sizeof(cfg->dst_dir_name) - 1);
            mpp_logi(" - set output directory: %s\n", optarg);
        } break;
        case 'w': {
            cfg->img_w_i = strtol(optarg, NULL, 0);
        } break;
        case 'g': {
            cfg->img_h_i = strtol(optarg, NULL, 0);
        } break;
        case 'f': {
            cfg->img_fmt_i = strtol(optarg, NULL, 0);
        } break;
        case 'r': {
            cfg->img_range = strtol(optarg, NULL, 0);
            mpp_logi(" - set input range: %d (0-Limit, 1-Full)\n", cfg->img_range);
        } break;
        case 's': {
            cfg->img_swap_i = 1;
        } break;
        case 'W': {
            cfg->img_w_o = strtol(optarg, NULL, 0);
        } break;
        case 'G': {
            cfg->img_h_o = strtol(optarg, NULL, 0);
        } break;
        case 'F': {
            cfg->img_fmt_o = strtol(optarg, NULL, 0);
        } break;
        case 'S': {
            cfg->img_swap_o = 1;
        } break;
        case 'm': {
            cfg->work_mode = MPP_MAX(VDPP_WORK_MODE_VEP, atoi(optarg));
            mpp_logi(" - set work mode: %d (2-vep, 3-dci_hist)\n", cfg->work_mode);
        } break;
        case 'n': {
            cfg->frame_num = strtol(optarg, NULL, 0);
        } break;
        case 't': {
            cfg->nthreads = strtol(optarg, NULL, 0);
            if (cfg->nthreads < 1)
                cfg->nthreads = 1;
        } break;
        default: break;
        }
    }

    /* check & adjust arguments */
    if (!cfg->src_file_name[0]) {
        mpp_loge("input filename not specified!\n");
        return -1;
    }
    if (cfg->img_fmt_o < 0)
        cfg->img_fmt_o = cfg->img_fmt_i;
    if (cfg->work_mode != VDPP_WORK_MODE_DCI) {
        if (cfg->img_fmt_o != VDPP_FMT_NV12 && cfg->img_fmt_o != VDPP_FMT_NV21 &&
            cfg->img_fmt_o != VDPP_FMT_NV24 && cfg->img_fmt_o != VDPP_FMT_NV42) {
            mpp_loge("output format %d not supported!\n", cfg->img_fmt_o);
            return -1;
        }
    }
    mpp_logi(" - set input image size: %dx%d, hwpq_vdpp_format: %d\n",
             cfg->img_w_i, cfg->img_h_i, cfg->img_fmt_i);

    if (!cfg->img_w_i_vir)
        cfg->img_w_i_vir = cfg->img_w_i;
    if (!cfg->img_h_i_vir)
        cfg->img_h_i_vir = cfg->img_h_i;
    if (!cfg->img_w_o)
        cfg->img_w_o = cfg->img_w_i;
    if (!cfg->img_h_o)
        cfg->img_h_o = cfg->img_h_i;
    if (!cfg->img_w_o_vir)
        cfg->img_w_o_vir = cfg->img_w_o;
    if (!cfg->img_h_o_vir)
        cfg->img_h_o_vir = cfg->img_h_o;
    if (cfg->uv_diff_flag) {
        RK_S32 shift_bit = (cfg->img_fmt_o == VDPP_FMT_NV12) || (cfg->img_fmt_o == VDPP_FMT_NV21);

        if (!cfg->img_w_o_c)
            cfg->img_w_o_c = cfg->img_w_o >> shift_bit;
        if (!cfg->img_h_o_c)
            cfg->img_h_o_c = cfg->img_h_i >> shift_bit;
        if (!cfg->img_w_o_c_vir)
            cfg->img_w_o_c_vir = cfg->img_w_o_c;
        if (!cfg->img_h_o_c_vir)
            cfg->img_h_o_c_vir = cfg->img_h_o_c;
        mpp_logi(" - set output chroma image size: %dx%d, virtual size: %dx%d\n",
                 cfg->img_w_o_c, cfg->img_h_o_c, cfg->img_w_o_c_vir, cfg->img_h_o_c_vir);
    }
    mpp_logi(" - set output image size: %dx%d, virtual size: %dx%d, hwpq_vdpp_format: %d\n",
             cfg->img_w_o, cfg->img_h_o, cfg->img_w_o_vir, cfg->img_h_o_vir, cfg->img_fmt_o);
    mpp_logi(" - set enbale flags: dmsr=%d, es=%d, shp=%d, hist=%d, pyr=%d, bbd=%d\n",
             cfg->en_dmsr, cfg->en_es, cfg->en_shp, cfg->en_hist, cfg->en_pyr, cfg->en_bbd);

    return 0;
}

static void print_help_info()
{
    mpp_logi("Usage: hwpq_test [options]\n");
    mpp_logi("Options:\n");
    mpp_logi("  -h, --help            : show this message\n");
    mpp_logi("  -v, --version         : show version info\n");
    mpp_logi("  -i, --input     <file>: input filename\n");
    mpp_logi("  -o, --output    [file]: output filename [of luma data] \n");
    mpp_logi("  -O, --outc      [file]: output filename of chroma data \n");
    mpp_logi("  -d  --outdir     [dir]: output directory for other results(hist/pyr/slt/bbd...)\n");
    mpp_logi("  -w, --swid       [int]: input  valid   width,  unit: pixel, default: 1920\n");
    mpp_logi("  -g, --shgt       [int]: input  valid   height, unit: pixel, default: 1080\n");
    mpp_logi("      --sw_vir     [int]: input  aligned width,  unit: pixel\n");
    mpp_logi("      --sh_vir     [int]: input  aligned height, unit: pixel\n");
    mpp_logi("  -f, --sfmt       [int]: input  image format in HwpqVdppFormat\n");
    mpp_logi("  -s, --sswap           : input  image chroma_uv is swapped\n");
    mpp_logi("  -W, --dwid       [int]: output valid   width,  unit: pixel, default: same to src\n");
    mpp_logi("  -G, --dhgt       [int]: output valid   height, unit: pixel, default: same to src\n");
    mpp_logi("      --dw_vir     [int]: output aligned width,  unit: pixel\n");
    mpp_logi("      --dh_vir     [int]: output aligned height, unit: pixel\n");
    mpp_logi("      --dwc        [int]: output chroma valid   width,  unit: pixel\n");
    mpp_logi("      --dhc        [int]: output chroma valid   height, unit: pixel\n");
    mpp_logi("      --dwc_vir    [int]: output chroma aligned width,  unit: pixel\n");
    mpp_logi("      --dhc_vir    [int]: output chroma aligned height, unit: pixel\n");
    mpp_logi("  -F, --dfmt       [int]: output image format in HwpqVdppFormat\n");
    mpp_logi("  -S, --dswap           : output image chroma_uv is swapped\n");
    mpp_logi("  -m, --mode       [int]: vdpp work mode: {2-vep, 3-dci_hist}, default: 2\n");
    mpp_logi("  -n, --nframe     [int]: frame number, default: 1\n");
    mpp_logi("  -t, --nthread    [int]: thread number, default: 1\n");
    mpp_logi("      --cfg_set    [int]: high 16 bit: mask; low 3 bit (msb->lsb): dmsr|es|sharp. default: 0x0\n");
    mpp_logi("      --en_dmsr    [int]: en_dmsr flag, default: 0\n");
    mpp_logi("      --en_es      [int]: en_es   flag, default: 0\n");
    mpp_logi("      --en_shp     [int]: en_shp  flag, default: 0\n");
    mpp_logi("      --en_hist    [int]: en_hist flag, default: 1\n");
    mpp_logi("      --en_pyr     [int]: en_pyr  flag, default: 1\n");
    mpp_logi("      --en_bbd     [int]: en_bbd  flag, default: 1\n");
    mpp_logi("      --bbd_th     [int]: bbd threshold, default: 20 in U8 input range\n");
}

#if 0
static RK_S32 get_vdpp_format_uv_sample_shift(RK_S32 fmt, RK_S32 *p_ret_shiftx2)
{
    // Wc = Wy >> shifts[0], Hc = Hy >> shifts[1]
    RK_S32 shifts[2] = {0};

    switch (fmt) {
    case VDPP_FMT_NV24:
    case VDPP_FMT_NV24_VU:
    case VDPP_FMT_NV30:
    case VDPP_FMT_Q410:
        shifts[0] = 0;
        shifts[1] = 0;
        break;
    case VDPP_FMT_NV16:
    case VDPP_FMT_NV16_VU:
    case VDPP_FMT_NV20:
    case VDPP_FMT_P210:
        shifts[0] = 1;
        shifts[1] = 0;
        break;
    case VDPP_FMT_NV12:
    case VDPP_FMT_NV12_VU:
    case VDPP_FMT_NV15:
    case VDPP_FMT_P010:
        shifts[0] = 1;
        shifts[1] = 1;
        break;
    case VDPP_FMT_Y_ONLY_8BIT:
    case VDPP_FMT_UV_ONLY_8BIT:
        break;
    default: return -1;
    }

    if (p_ret_shiftx2) {
        p_ret_shiftx2[0] = shifts[0];
        p_ret_shiftx2[1] = shifts[1];
    }
    return 0;
}
#endif

static inline size_t get_frame_size(RK_S32 fmt, RK_U32 w, RK_U32 h)
{
    size_t frame_size = 0;

    switch (fmt) {
    case VDPP_FMT_Y_ONLY_8BIT: {
        frame_size = w * h;
    } break;
    case VDPP_FMT_NV12:
    case VDPP_FMT_NV21: {
        frame_size = w * h * 3 / 2;
    } break;
    case VDPP_FMT_NV16:
    case VDPP_FMT_NV61:
    case VDPP_FMT_UV_ONLY_8BIT: {
        frame_size = w * h * 2;
    } break;
    case VDPP_FMT_NV24:
    case VDPP_FMT_NV42:
    case VDPP_FMT_RG24:
    case VDPP_FMT_BG24: {
        frame_size = w * h * 3;
    } break;
    case VDPP_FMT_RGBA: {
        frame_size = w * h * 4;
    } break;
    case VDPP_FMT_NV15: {
        frame_size = w * h * 3 * 5 / 2 / 4;
    } break;
    case VDPP_FMT_NV20: {
        frame_size = w * h * 2 * 5 / 4;
    } break;
    case VDPP_FMT_NV30: {
        frame_size = w * h * 3 * 5 / 4;
    } break;
    default: {
        frame_size = w * h * 4;
    } break;
    }

    return frame_size;
}
