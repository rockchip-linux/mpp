/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "hal_h265e_v510"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"

#include "rkv_enc_def.h"
#include "h265e_syntax_new.h"
#include "h265e_dpb.h"
#include "hal_bufs.h"
#include "hal_h265e_debug.h"
#include "hal_h265e_vepu510.h"
#include "hal_h265e_vepu510_reg.h"

#include "vepu5xx_common.h"
#include "vepu541_common.h"
#include "vepu510_common.h"

#define MAX_FRAME_TASK_NUM      2

#define hal_h265e_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

typedef struct Vepu510H265Fbk_t {
    RK_U32 hw_status; /* 0:corret, 1:error */
    RK_U32 qp_sum;
    RK_U32 out_strm_size;
    RK_U32 out_hw_strm_size;
    RK_S64 sse_sum;
    RK_U32 st_lvl64_inter_num;
    RK_U32 st_lvl32_inter_num;
    RK_U32 st_lvl16_inter_num;
    RK_U32 st_lvl8_inter_num;
    RK_U32 st_lvl32_intra_num;
    RK_U32 st_lvl16_intra_num;
    RK_U32 st_lvl8_intra_num;
    RK_U32 st_lvl4_intra_num;
    RK_U32 st_cu_num_qp[52];
    RK_U32 st_madp;
    RK_U32 st_madi;
    RK_U32 st_mb_num;
    RK_U32 st_ctu_num;
} Vepu510H265Fbk;

typedef struct Vepu510H265eFrmCfg_t {
    RK_S32              frame_count;
    RK_S32              frame_type;

    /* dchs cfg on frame parallel */
    RK_S32              dchs_curr_idx;
    RK_S32              dchs_prev_idx;

    /* hal dpb management slot idx */
    RK_S32              hal_curr_idx;
    RK_S32              hal_refr_idx;

    /* regs cfg */
    H265eV510RegSet     *regs_set;
    H265eV510StatusElem *regs_ret;

    /* hardware return info collection cfg */
    Vepu510H265Fbk      feedback;

    /* osd cfg */
    Vepu541OsdCfg       osd_cfg;
    void                *roi_data;

    /* gdr roi cfg */
    MppBuffer           roi_base_cfg_buf;
    void                *roi_base_cfg_sw_buf;
    RK_S32              roi_base_buf_size;

    /* variable length cfg */
    MppDevRegOffCfgs    *reg_cfg;
} Vepu510H265eFrmCfg;

typedef struct H265eV510HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;
    void                *reg_out;
    Vepu510H265eFrmCfg  *frms[MAX_FRAME_TASK_NUM];

    /* current used frame config */
    Vepu510H265eFrmCfg  *frm;

    /* slice split poll cfg */
    RK_S32              poll_slice_max;
    RK_S32              poll_cfg_size;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_count;

    /* frame parallel info */
    RK_S32              task_cnt;
    RK_S32              task_idx;

    /* dchs cfg */
    RK_S32              curr_idx;
    RK_S32              prev_idx;

    Vepu510H265Fbk      feedback;
    void                *dump_files;
    RK_U32              frame_cnt_gen_ready;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    void                *roi_data;
    MppEncCfgSet        *cfg;
    MppDevRegOffCfgs    *reg_cfg;
    H265eSyntax_new     *syn;
    H265eDpb            *dpb;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    RK_U32              frame_num;
    HalBufs             dpb_bufs;
    RK_S32              fbc_header_len;
    RK_U32              title_num;

    /* external line buffer over 3K */
    MppBufferGroup          ext_line_buf_grp;
    RK_S32                  ext_line_buf_size;
    MppBuffer               ext_line_buf;
    MppBuffer               buf_pass1;
    MppBuffer               ext_line_bufs[MAX_FRAME_TASK_NUM];

} H265eV510HalContext;

static RK_U32 aq_thd_default[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 25
};

static RK_S32 aq_qp_dealt_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    4,  5,  6,  8,
};

static void setup_ext_line_bufs(H265eV510HalContext *ctx)
{
    RK_S32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i])
            continue;

        mpp_buffer_get(ctx->ext_line_buf_grp, &ctx->ext_line_bufs[i],
                       ctx->ext_line_buf_size);
    }
}

static void clear_ext_line_bufs(H265eV510HalContext *ctx)
{
    RK_S32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i]) {
            mpp_buffer_put(ctx->ext_line_bufs[i]);
            ctx->ext_line_bufs[i] = NULL;
        }
    }
}

static MPP_RET vepu510_h265_setup_hal_bufs(H265eV510HalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    RK_U32 frame_size;
    Vepu541Fmt input_fmt = VEPU541_FMT_YUV420P;
    RK_S32 mb_wd64, mb_h64;
    MppEncRefCfg ref_cfg = ctx->cfg->ref_cfg;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_S32 old_max_cnt = ctx->max_buf_cnt;
    RK_S32 new_max_cnt = 4;
    RK_S32 alignment = 32;
    RK_S32 aligned_w = MPP_ALIGN(prep->width,  alignment);

    hal_h265e_enter();

    mb_wd64 = (prep->width + 63) / 64;
    mb_h64 = (prep->height + 63) / 64 + 1;

    frame_size = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16);
    vepu541_set_fmt(fmt, ctx->cfg->prep.format);
    input_fmt = (Vepu541Fmt)fmt->format;
    switch (input_fmt) {
    case VEPU540_FMT_YUV400:
        break;
    case VEPU541_FMT_YUV420P:
    case VEPU541_FMT_YUV420SP: {
        frame_size = frame_size * 3 / 2;
    } break;
    case VEPU541_FMT_YUV422P:
    case VEPU541_FMT_YUV422SP:
    case VEPU541_FMT_YUYV422:
    case VEPU541_FMT_UYVY422:
    case VEPU541_FMT_BGR565: {
        frame_size *= 2;
    } break;
    case VEPU541_FMT_BGR888:
    case VEPU580_FMT_YUV444SP:
    case VEPU580_FMT_YUV444P: {
        frame_size *= 3;
    } break;
    case VEPU541_FMT_BGRA8888: {
        frame_size *= 4;
    } break;
    default: {
        hal_h265e_err("invalid src color space: %d\n", input_fmt);
        return MPP_NOK;
    }
    }

    if (ref_cfg) {
        MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(ref_cfg);
        new_max_cnt = MPP_MAX(new_max_cnt, info->dpb_size + 1);
    }

    if (aligned_w > SZ_4K) {
        RK_S32 ctu_w = (aligned_w + 31) / 32;
        RK_S32 ext_line_buf_size = ((ctu_w - 113) * 27 + 15) / 16 * 16 * 16;

        if (NULL == ctx->ext_line_buf_grp)
            mpp_buffer_group_get_internal(&ctx->ext_line_buf_grp, MPP_BUFFER_TYPE_ION);
        else if (ext_line_buf_size != ctx->ext_line_buf_size) {
            clear_ext_line_bufs(ctx);
            mpp_buffer_group_clear(ctx->ext_line_buf_grp);
        }

        mpp_assert(ctx->ext_line_buf_grp);
        setup_ext_line_bufs(ctx);
        ctx->ext_line_buf_size = ext_line_buf_size;
    } else {
        clear_ext_line_bufs(ctx);

        if (ctx->ext_line_buf_grp) {
            mpp_buffer_group_clear(ctx->ext_line_buf_grp);
            mpp_buffer_group_put(ctx->ext_line_buf_grp);
            ctx->ext_line_buf_grp = NULL;
        }
        ctx->ext_line_buf_size = 0;
    }

    if (frame_size > ctx->frame_size || new_max_cnt > old_max_cnt) {
        size_t size[3] = {0};

        hal_bufs_deinit(ctx->dpb_bufs);
        hal_bufs_init(&ctx->dpb_bufs);

        ctx->fbc_header_len = MPP_ALIGN(((mb_wd64 * mb_h64) << 6), SZ_8K);
        size[0] = ctx->fbc_header_len + ((mb_wd64 * mb_h64) << 12) * 3 / 2; //fbc_h + fbc_b
        size[1] = (mb_wd64 * mb_h64 << 8);
        size[2] = MPP_ALIGN(mb_wd64 * mb_h64 * 16 * 4, 256) * 16;
        new_max_cnt = MPP_MAX(new_max_cnt, old_max_cnt);

        hal_h265e_dbg_detail("frame size %d -> %d max count %d -> %d\n",
                             ctx->frame_size, frame_size, old_max_cnt, new_max_cnt);

        hal_bufs_setup(ctx->dpb_bufs, new_max_cnt, 3, size);

        ctx->frame_size = frame_size;
        ctx->max_buf_cnt = new_max_cnt;
    }
    hal_h265e_leave();
    return ret;
}

static void vepu510_h265_rdo_cfg (Vepu510Sqi *reg)
{
    rdo_skip_par   *p_rdo_skip   = NULL;
    rdo_noskip_par *p_rdo_noskip = NULL;
    pre_cst_par    *p_pre_cst    = NULL;

    reg->rdo_segment_cfg.rdo_segment_multi          = 28;
    reg->rdo_segment_cfg.rdo_segment_en             = 1;
    reg->rdo_smear_cfg_comb.rdo_smear_en            = 0;
    reg->rdo_smear_cfg_comb.rdo_smear_lvl16_multi   = 9;
    reg->rdo_segment_cfg.rdo_smear_lvl8_multi       = 8;
    reg->rdo_segment_cfg.rdo_smear_lvl4_multi       = 8 ;
    reg->rdo_smear_cfg_comb.rdo_smear_dlt_qp        = 0 ;
    reg->rdo_smear_cfg_comb.rdo_smear_order_state   = 0;
    reg->rdo_smear_cfg_comb.stated_mode             = 0;
    reg->rdo_smear_cfg_comb.online_en               = 0;
    reg->rdo_smear_cfg_comb.smear_stride            = 0;
    reg->rdo_smear_madp_thd0_comb.rdo_smear_madp_cur_thd0    = 0 ;
    reg->rdo_smear_madp_thd0_comb.rdo_smear_madp_cur_thd1    = 24;
    reg->rdo_smear_madp_thd1_comb.rdo_smear_madp_cur_thd2    = 48;
    reg->rdo_smear_madp_thd1_comb.rdo_smear_madp_cur_thd3    = 64;
    reg->rdo_smear_madp_thd2_comb.rdo_smear_madp_around_thd0 = 16;
    reg->rdo_smear_madp_thd2_comb.rdo_smear_madp_around_thd1 = 32;
    reg->rdo_smear_madp_thd3_comb.rdo_smear_madp_around_thd2 = 48;
    reg->rdo_smear_madp_thd3_comb.rdo_smear_madp_around_thd3 = 96;
    reg->rdo_smear_madp_thd4_comb.rdo_smear_madp_around_thd4 = 48;
    reg->rdo_smear_madp_thd4_comb.rdo_smear_madp_around_thd5 = 24;
    reg->rdo_smear_madp_thd5_comb.rdo_smear_madp_ref_thd0    = 96;
    reg->rdo_smear_madp_thd5_comb.rdo_smear_madp_ref_thd1    = 48;
    reg->rdo_smear_cnt_thd0_comb.rdo_smear_cnt_cur_thd0      = 1 ;
    reg->rdo_smear_cnt_thd0_comb.rdo_smear_cnt_cur_thd1      = 3 ;
    reg->rdo_smear_cnt_thd0_comb.rdo_smear_cnt_cur_thd2      = 1 ;
    reg->rdo_smear_cnt_thd0_comb.rdo_smear_cnt_cur_thd3      = 3 ;
    reg->rdo_smear_cnt_thd1_comb.rdo_smear_cnt_around_thd0   = 1 ;
    reg->rdo_smear_cnt_thd1_comb.rdo_smear_cnt_around_thd1   = 4 ;
    reg->rdo_smear_cnt_thd1_comb.rdo_smear_cnt_around_thd2   = 1 ;
    reg->rdo_smear_cnt_thd1_comb.rdo_smear_cnt_around_thd3   = 4 ;
    reg->rdo_smear_cnt_thd2_comb.rdo_smear_cnt_around_thd4   = 0 ;
    reg->rdo_smear_cnt_thd2_comb.rdo_smear_cnt_around_thd5   = 3 ;
    reg->rdo_smear_cnt_thd2_comb.rdo_smear_cnt_around_thd6   = 0 ;
    reg->rdo_smear_cnt_thd2_comb.rdo_smear_cnt_around_thd7   = 3 ;
    reg->rdo_smear_cnt_thd3_comb.rdo_smear_cnt_ref_thd0      = 1 ;
    reg->rdo_smear_cnt_thd3_comb.rdo_smear_cnt_ref_thd1      = 3 ;
    reg->rdo_smear_resi_thd0_comb.rdo_smear_resi_small_cur_th0    = 6;
    reg->rdo_smear_resi_thd0_comb.rdo_smear_resi_big_cur_th0      = 9;
    reg->rdo_smear_resi_thd0_comb.rdo_smear_resi_small_cur_th1    = 6;
    reg->rdo_smear_resi_thd0_comb.rdo_smear_resi_big_cur_th1      = 9;
    reg->rdo_smear_resi_thd1_comb.rdo_smear_resi_small_around_th0 = 6;
    reg->rdo_smear_resi_thd1_comb.rdo_smear_resi_big_around_th0   = 11;
    reg->rdo_smear_resi_thd1_comb.rdo_smear_resi_small_around_th1 = 6;
    reg->rdo_smear_resi_thd1_comb.rdo_smear_resi_big_around_th1   = 8;
    reg->rdo_smear_resi_thd2_comb.rdo_smear_resi_small_around_th2 = 9;
    reg->rdo_smear_resi_thd2_comb.rdo_smear_resi_big_around_th2   = 20;
    reg->rdo_smear_resi_thd2_comb.rdo_smear_resi_small_around_th3 = 6;
    reg->rdo_smear_resi_thd2_comb.rdo_smear_resi_big_around_th3   = 20;
    reg->rdo_smear_resi_thd3_comb.rdo_smear_resi_small_ref_th0    = 7;
    reg->rdo_smear_resi_thd3_comb.rdo_smear_resi_big_ref_th0      = 16;
    reg->rdo_smear_st_thd0_comb.rdo_smear_resi_th0     = 9;
    reg->rdo_smear_st_thd0_comb.rdo_smear_resi_th1     = 6;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th0 = 1;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th1 = 5;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th2 = 1;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th3 = 3;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th4 = 1;
    reg->rdo_smear_st_thd1_comb.rdo_smear_madp_cnt_th5 = 2;

    p_rdo_skip = &reg->rdo_b32_skip;
    p_rdo_skip->atf_thd0.madp_thd0 = 5  ;
    p_rdo_skip->atf_thd0.madp_thd1 = 10 ;
    p_rdo_skip->atf_thd1.madp_thd2 = 15 ;
    p_rdo_skip->atf_thd1.madp_thd3 = 72 ;
    p_rdo_skip->atf_wgt0.wgt0 =      20 ;
    p_rdo_skip->atf_wgt0.wgt1 =      16 ;
    p_rdo_skip->atf_wgt0.wgt2 =      16 ;
    p_rdo_skip->atf_wgt0.wgt3 =      16 ;
    p_rdo_skip->atf_wgt1.wgt4 =      16 ;

    p_rdo_noskip = &reg->rdo_b32_inter;
    p_rdo_noskip->ratf_thd0.madp_thd0 = 20;
    p_rdo_noskip->ratf_thd0.madp_thd1 = 40;
    p_rdo_noskip->ratf_thd1.madp_thd2 = 72;
    p_rdo_noskip->atf_wgt.wgt0 =        16;
    p_rdo_noskip->atf_wgt.wgt1 =        16;
    p_rdo_noskip->atf_wgt.wgt2 =        16;
    p_rdo_noskip->atf_wgt.wgt3 =        16;

    p_rdo_noskip = &reg->rdo_b32_intra;
    p_rdo_noskip->ratf_thd0.madp_thd0 = 20;
    p_rdo_noskip->ratf_thd0.madp_thd1 = 40;
    p_rdo_noskip->ratf_thd1.madp_thd2 = 72;
    p_rdo_noskip->atf_wgt.wgt0 =        27;
    p_rdo_noskip->atf_wgt.wgt1 =        25;
    p_rdo_noskip->atf_wgt.wgt2 =        20;
    p_rdo_noskip->atf_wgt.wgt3 =        19;

    p_rdo_skip = &reg->rdo_b16_skip;
    p_rdo_skip->atf_thd0.madp_thd0 = 1  ;
    p_rdo_skip->atf_thd0.madp_thd1 = 10 ;
    p_rdo_skip->atf_thd1.madp_thd2 = 15 ;
    p_rdo_skip->atf_thd1.madp_thd3 = 25 ;
    p_rdo_skip->atf_wgt0.wgt0 =      20 ;
    p_rdo_skip->atf_wgt0.wgt1 =      16 ;
    p_rdo_skip->atf_wgt0.wgt2 =      16 ;
    p_rdo_skip->atf_wgt0.wgt3 =      16 ;
    p_rdo_skip->atf_wgt1.wgt4 =      16 ;

    p_rdo_noskip = &reg->rdo_b16_inter;
    p_rdo_noskip->ratf_thd0.madp_thd0 = 20;
    p_rdo_noskip->ratf_thd0.madp_thd1 = 40;
    p_rdo_noskip->ratf_thd1.madp_thd2 = 72;
    p_rdo_noskip->atf_wgt.wgt0 =        16;
    p_rdo_noskip->atf_wgt.wgt1 =        16;
    p_rdo_noskip->atf_wgt.wgt2 =        16;
    p_rdo_noskip->atf_wgt.wgt3 =        16;

    p_rdo_noskip = &reg->rdo_b16_intra;
    p_rdo_noskip->ratf_thd0.madp_thd0 = 20;
    p_rdo_noskip->ratf_thd0.madp_thd1 = 40;
    p_rdo_noskip->ratf_thd1.madp_thd2 = 72;
    p_rdo_noskip->atf_wgt.wgt0 =        27;
    p_rdo_noskip->atf_wgt.wgt1 =        25;
    p_rdo_noskip->atf_wgt.wgt2 =        20;
    p_rdo_noskip->atf_wgt.wgt3 =        16;

    reg->rdo_b32_intra_atf_cnt_thd.thd0      = 1 ;
    reg->rdo_b32_intra_atf_cnt_thd.thd1      = 4 ;
    reg->rdo_b32_intra_atf_cnt_thd.thd2      = 1 ;
    reg->rdo_b32_intra_atf_cnt_thd.thd3      = 4 ;

    reg->rdo_b16_intra_atf_cnt_thd_comb.thd0 = 1 ;
    reg->rdo_b16_intra_atf_cnt_thd_comb.thd1 = 4 ;
    reg->rdo_b16_intra_atf_cnt_thd_comb.thd2 = 1 ;
    reg->rdo_b16_intra_atf_cnt_thd_comb.thd3 = 4 ;
    reg->rdo_atf_resi_thd_comb.big_th0     = 16;
    reg->rdo_atf_resi_thd_comb.big_th1     = 16;
    reg->rdo_atf_resi_thd_comb.small_th0   = 8;
    reg->rdo_atf_resi_thd_comb.small_th1   = 8;

    p_pre_cst = &reg->preintra32_cst;
    p_pre_cst->cst_madi_thd0.madi_thd0 = 5  ;
    p_pre_cst->cst_madi_thd0.madi_thd1 = 3  ;
    p_pre_cst->cst_madi_thd0.madi_thd2 = 3  ;
    p_pre_cst->cst_madi_thd0.madi_thd3 = 6  ;
    p_pre_cst->cst_madi_thd1.madi_thd4 = 7  ;
    p_pre_cst->cst_madi_thd1.madi_thd5 = 10 ;
    p_pre_cst->cst_wgt0.wgt0          =  20 ;
    p_pre_cst->cst_wgt0.wgt1          =  18 ;
    p_pre_cst->cst_wgt0.wgt2          =  19 ;
    p_pre_cst->cst_wgt0.wgt3          =  18 ;
    p_pre_cst->cst_wgt1.wgt4          =  6  ;
    p_pre_cst->cst_wgt1.wgt5          =  9  ;
    p_pre_cst->cst_wgt1.wgt6          =  14 ;
    p_pre_cst->cst_wgt1.wgt7          =  18 ;
    p_pre_cst->cst_wgt2.wgt8          =  17 ;
    p_pre_cst->cst_wgt2.wgt9          =  17 ;
    p_pre_cst->cst_wgt2.mode_th       =  5  ;

    p_pre_cst = &reg->preintra16_cst;
    p_pre_cst->cst_madi_thd0.madi_thd0 = 5 ;
    p_pre_cst->cst_madi_thd0.madi_thd1 = 3 ;
    p_pre_cst->cst_madi_thd0.madi_thd2 = 3 ;
    p_pre_cst->cst_madi_thd0.madi_thd3 = 6;
    p_pre_cst->cst_madi_thd1.madi_thd4 = 5;
    p_pre_cst->cst_madi_thd1.madi_thd5 = 7;
    p_pre_cst->cst_wgt0.wgt0          =  20;
    p_pre_cst->cst_wgt0.wgt1          =  18;
    p_pre_cst->cst_wgt0.wgt2          =  19;
    p_pre_cst->cst_wgt0.wgt3          =  18;
    p_pre_cst->cst_wgt1.wgt4          =  6 ;
    p_pre_cst->cst_wgt1.wgt5          =  9 ;
    p_pre_cst->cst_wgt1.wgt6          =  14;
    p_pre_cst->cst_wgt1.wgt7          =  18;
    p_pre_cst->cst_wgt2.wgt8          =  17;
    p_pre_cst->cst_wgt2.wgt9          =  17;
    p_pre_cst->cst_wgt2.mode_th       =  5 ;

    reg->preintra_sqi_cfg.pre_intra_qp_thd          = 28;
    reg->preintra_sqi_cfg.pre_intra4_lambda_mv_bit  = 3;
    reg->preintra_sqi_cfg.pre_intra8_lambda_mv_bit  = 4;
    reg->preintra_sqi_cfg.pre_intra16_lambda_mv_bit = 4;
    reg->preintra_sqi_cfg.pre_intra32_lambda_mv_bit = 5;
    reg->rdo_atr_i_cu32_madi_cfg0.i_cu32_madi_thd0  = 3;
    reg->rdo_atr_i_cu32_madi_cfg0.i_cu32_madi_thd1  = 25;
    reg->rdo_atr_i_cu32_madi_cfg0.i_cu32_madi_thd2  = 25;
    reg->rdo_atr_i_cu32_madi_cfg1.i_cu32_madi_cnt_thd3   = 0;
    reg->rdo_atr_i_cu32_madi_cfg1.i_cu32_madi_thd4       = 20;
    reg->rdo_atr_i_cu32_madi_cfg1.i_cu32_madi_cost_multi = 24;
    reg->rdo_atr_i_cu16_madi_cfg0.i_cu16_madi_thd0       = 4;
    reg->rdo_atr_i_cu16_madi_cfg0.i_cu16_madi_thd1       = 6;
    reg->rdo_atr_i_cu16_madi_cfg0.i_cu16_madi_cost_multi = 24;

    /* 0x00002100 reg2112 */
    reg->cudecis_thd0.base_thre_rough_mad32_intra           = 9;
    reg->cudecis_thd0.delta0_thre_rough_mad32_intra         = 10;
    reg->cudecis_thd0.delta1_thre_rough_mad32_intra         = 55;
    reg->cudecis_thd0.delta2_thre_rough_mad32_intra         = 55;
    reg->cudecis_thd0.delta3_thre_rough_mad32_intra         = 66;
    reg->cudecis_thd0.delta4_thre_rough_mad32_intra_low5    = 2;

    /* 0x00002104 reg2113 */
    reg->cudecis_thd1.delta4_thre_rough_mad32_intra_high2   = 2;
    reg->cudecis_thd1.delta5_thre_rough_mad32_intra         = 74;
    reg->cudecis_thd1.delta6_thre_rough_mad32_intra         = 106;
    reg->cudecis_thd1.base_thre_fine_mad32_intra            = 8;
    reg->cudecis_thd1.delta0_thre_fine_mad32_intra          = 0;
    reg->cudecis_thd1.delta1_thre_fine_mad32_intra          = 13;
    reg->cudecis_thd1.delta2_thre_fine_mad32_intra_low3     = 6;

    /* 0x00002108 reg2114 */
    reg->cudecis_thd2.delta2_thre_fine_mad32_intra_high2    = 1;
    reg->cudecis_thd2.delta3_thre_fine_mad32_intra          = 17;
    reg->cudecis_thd2.delta4_thre_fine_mad32_intra          = 23;
    reg->cudecis_thd2.delta5_thre_fine_mad32_intra          = 50;
    reg->cudecis_thd2.delta6_thre_fine_mad32_intra          = 54;
    reg->cudecis_thd2.base_thre_str_edge_mad32_intra        = 6;
    reg->cudecis_thd2.delta0_thre_str_edge_mad32_intra      = 0;
    reg->cudecis_thd2.delta1_thre_str_edge_mad32_intra      = 0;

    /* 0x0000210c reg2115 */
    reg->cudecis_thd3.delta2_thre_str_edge_mad32_intra      = 3;
    reg->cudecis_thd3.delta3_thre_str_edge_mad32_intra      = 8;
    reg->cudecis_thd3.base_thre_str_edge_bgrad32_intra      = 25;
    reg->cudecis_thd3.delta0_thre_str_edge_bgrad32_intra    = 0;
    reg->cudecis_thd3.delta1_thre_str_edge_bgrad32_intra    = 0;
    reg->cudecis_thd3.delta2_thre_str_edge_bgrad32_intra    = 7;
    reg->cudecis_thd3.delta3_thre_str_edge_bgrad32_intra    = 0;
    reg->cudecis_thd3.base_thre_mad16_intra                 = 6;
    reg->cudecis_thd3.delta0_thre_mad16_intra               = 0;

    /* 0x00002110 reg2116 */
    reg->cudecis_thd4.delta1_thre_mad16_intra          = 3;
    reg->cudecis_thd4.delta2_thre_mad16_intra          = 3;
    reg->cudecis_thd4.delta3_thre_mad16_intra          = 24;
    reg->cudecis_thd4.delta4_thre_mad16_intra          = 28;
    reg->cudecis_thd4.delta5_thre_mad16_intra          = 40;
    reg->cudecis_thd4.delta6_thre_mad16_intra          = 52;
    reg->cudecis_thd4.delta0_thre_mad16_ratio_intra    = 7;

    /* 0x00002114 reg2117 */
    reg->cudecis_thd5.delta1_thre_mad16_ratio_intra           =  7;
    reg->cudecis_thd5.delta2_thre_mad16_ratio_intra           =  2;
    reg->cudecis_thd5.delta3_thre_mad16_ratio_intra           =  2;
    reg->cudecis_thd5.delta4_thre_mad16_ratio_intra           =  0;
    reg->cudecis_thd5.delta5_thre_mad16_ratio_intra           =  0;
    reg->cudecis_thd5.delta6_thre_mad16_ratio_intra           =  0;
    reg->cudecis_thd5.delta7_thre_mad16_ratio_intra           =  4;
    reg->cudecis_thd5.delta0_thre_rough_bgrad32_intra         =  1;
    reg->cudecis_thd5.delta1_thre_rough_bgrad32_intra         =  5;
    reg->cudecis_thd5.delta2_thre_rough_bgrad32_intra_low4    =  8;

    /* 0x00002118 reg2118 */
    reg->cudecis_thd6.delta2_thre_rough_bgrad32_intra_high2    = 2;
    reg->cudecis_thd6.delta3_thre_rough_bgrad32_intra          = 540;
    reg->cudecis_thd6.delta4_thre_rough_bgrad32_intra          = 692;
    reg->cudecis_thd6.delta5_thre_rough_bgrad32_intra_low10    = 866;

    /* 0x0000211c reg2119 */
    reg->cudecis_thd7.delta5_thre_rough_bgrad32_intra_high1   = 1;
    reg->cudecis_thd7.delta6_thre_rough_bgrad32_intra         = 3286;
    reg->cudecis_thd7.delta7_thre_rough_bgrad32_intra         = 6620;
    reg->cudecis_thd7.delta0_thre_bgrad16_ratio_intra         = 8;
    reg->cudecis_thd7.delta1_thre_bgrad16_ratio_intra_low2    = 3;

    /* 0x00002120 reg2120 */
    reg->cudecis_thdt8.delta1_thre_bgrad16_ratio_intra_high2    = 2;
    reg->cudecis_thdt8.delta2_thre_bgrad16_ratio_intra          = 15;
    reg->cudecis_thdt8.delta3_thre_bgrad16_ratio_intra          = 15;
    reg->cudecis_thdt8.delta4_thre_bgrad16_ratio_intra          = 13;
    reg->cudecis_thdt8.delta5_thre_bgrad16_ratio_intra          = 13;
    reg->cudecis_thdt8.delta6_thre_bgrad16_ratio_intra          = 7;
    reg->cudecis_thdt8.delta7_thre_bgrad16_ratio_intra          = 15;
    reg->cudecis_thdt8.delta0_thre_fme_ratio_inter              = 4;
    reg->cudecis_thdt8.delta1_thre_fme_ratio_inter              = 4;

    /* 0x00002124 reg2121 */
    reg->cudecis_thd9.delta2_thre_fme_ratio_inter    = 3;
    reg->cudecis_thd9.delta3_thre_fme_ratio_inter    = 2;
    reg->cudecis_thd9.delta4_thre_fme_ratio_inter    = 0;
    reg->cudecis_thd9.delta5_thre_fme_ratio_inter    = 0;
    reg->cudecis_thd9.delta6_thre_fme_ratio_inter    = 0;
    reg->cudecis_thd9.delta7_thre_fme_ratio_inter    = 0;
    reg->cudecis_thd9.base_thre_fme32_inter          = 4;
    reg->cudecis_thd9.delta0_thre_fme32_inter        = 2;
    reg->cudecis_thd9.delta1_thre_fme32_inter        = 7;
    reg->cudecis_thd9.delta2_thre_fme32_inter        = 12;

    /* 0x00002128 reg2122 */
    reg->cudecis_thd10.delta3_thre_fme32_inter    = 23;
    reg->cudecis_thd10.delta4_thre_fme32_inter    = 41;
    reg->cudecis_thd10.delta5_thre_fme32_inter    = 71;
    reg->cudecis_thd10.delta6_thre_fme32_inter    = 123;
    reg->cudecis_thd10.thre_cme32_inter           = 48;

    /* 0x0000212c reg2123 */
    reg->cudecis_thd11.delta0_thre_mad_fme_ratio_inter    = 0;
    reg->cudecis_thd11.delta1_thre_mad_fme_ratio_inter    = 7;
    reg->cudecis_thd11.delta2_thre_mad_fme_ratio_inter    = 7;
    reg->cudecis_thd11.delta3_thre_mad_fme_ratio_inter    = 6;
    reg->cudecis_thd11.delta4_thre_mad_fme_ratio_inter    = 5;
    reg->cudecis_thd11.delta5_thre_mad_fme_ratio_inter    = 4;
    reg->cudecis_thd11.delta6_thre_mad_fme_ratio_inter    = 4;
    reg->cudecis_thd11.delta7_thre_mad_fme_ratio_inter    = 4;
}

static void vepu510_h265_global_cfg_set(H265eV510HalContext *ctx, H265eV510RegSet *regs)
{
    MppEncHwCfg *hw = &ctx->cfg->hw;
    RK_U32 i;
    H265eVepu510RcRoi *rc_regs = &regs->reg_rc_roi;
    H265eVepu510Param *reg_param = &regs->reg_param;
    Vepu510Sqi  *reg_sqi = &regs->reg_sqi;

    vepu510_h265_rdo_cfg(reg_sqi);
    memcpy(&reg_param->pprd_lamb_satd_0_51[0], lamd_satd_qp_510, sizeof(lamd_satd_qp));

    if (ctx->frame_type == INTRA_FRAME) {
        RK_U8 *thd  = (RK_U8 *)&rc_regs->aq_tthd0;
        RK_S8 *step = (RK_S8 *)&rc_regs->aq_stp0;

        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            thd[i]  = hw->aq_thrd_i[i];
            step[i] = hw->aq_step_i[i] & 0x3f;
        }
        reg_param->iprd_lamb_satd_ofst.lambda_satd_offset = 11;
        memcpy(&reg_param->rdo_wgta_qp_grpa_0_51[0], lamd_moda_qp, sizeof(lamd_moda_qp));
    } else {
        RK_U8 *thd  = (RK_U8 *)&rc_regs->aq_tthd0;
        RK_S8 *step = (RK_S8 *)&rc_regs->aq_stp0;
        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            thd[i]  = hw->aq_thrd_p[i];
            step[i] = hw->aq_step_p[i] & 0x3f;
        }
        reg_param->iprd_lamb_satd_ofst.lambda_satd_offset = 11;
        memcpy(&reg_param->rdo_wgta_qp_grpa_0_51[0], lamd_modb_qp, sizeof(lamd_modb_qp));
    }
    reg_param->reg1484_qnt_bias_comb.qnt_bias_i = 171;
    reg_param->reg1484_qnt_bias_comb.qnt_bias_p = 85;
    if (hw->qbias_en) {
        reg_param->reg1484_qnt_bias_comb.qnt_bias_i = hw->qbias_i;
        reg_param->reg1484_qnt_bias_comb.qnt_bias_p = hw->qbias_p;
    }
    /* CIME */
    {
        /* 0x1760 */
        regs->reg_param.me_sqi_cfg.cime_pmv_num = 1;
        regs->reg_param.me_sqi_cfg.cime_fuse   = 1;
        regs->reg_param.me_sqi_cfg.itp_mode    = 0;
        regs->reg_param.me_sqi_cfg.move_lambda = 2;
        regs->reg_param.me_sqi_cfg.rime_lvl_mrg     = 0;
        regs->reg_param.me_sqi_cfg.rime_prelvl_en   = 3;
        regs->reg_param.me_sqi_cfg.rime_prersu_en   = 3;

        /* 0x1764 */
        regs->reg_param.cime_mvd_th.cime_mvd_th0 = 8;
        regs->reg_param.cime_mvd_th.cime_mvd_th1 = 20;
        regs->reg_param.cime_mvd_th.cime_mvd_th2 = 32;

        /* 0x1768 */
        regs->reg_param.cime_madp_th.cime_madp_th = 16;

        /* 0x176c */
        regs->reg_param.cime_multi.cime_multi0 = 8;
        regs->reg_param.cime_multi.cime_multi1 = 12;
        regs->reg_param.cime_multi.cime_multi2 = 16;
        regs->reg_param.cime_multi.cime_multi3 = 20;
    }

    /* RIME && FME */
    {
        /* 0x1770 */
        regs->reg_param.rime_mvd_th.rime_mvd_th0  = 1;
        regs->reg_param.rime_mvd_th.rime_mvd_th1  = 2;
        regs->reg_param.rime_mvd_th.fme_madp_th   = 0;

        /* 0x1774 */
        regs->reg_param.rime_madp_th.rime_madp_th0 = 8;
        regs->reg_param.rime_madp_th.rime_madp_th1 = 16;

        /* 0x1778 */
        regs->reg_param.rime_multi.rime_multi0 = 4;
        regs->reg_param.rime_multi.rime_multi1 = 8;
        regs->reg_param.rime_multi.rime_multi2 = 12;

        /* 0x177C */
        regs->reg_param.cmv_st_th.cmv_th0 = 64;
        regs->reg_param.cmv_st_th.cmv_th1 = 96;
        regs->reg_param.cmv_st_th.cmv_th2 = 128;
    }
}

MPP_RET hal_h265e_v510_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    RK_S32 i = 0;

    mpp_env_get_u32("hal_h265e_debug", &hal_h265e_debug, 0);
    hal_h265e_enter();

    ctx->task_cnt = cfg->task_cnt;
    mpp_assert(ctx->task_cnt && ctx->task_cnt <= MAX_FRAME_TASK_NUM);
    if (ctx->task_cnt > MAX_FRAME_TASK_NUM)
        ctx->task_cnt = MAX_FRAME_TASK_NUM;

    for (i = 0; i < ctx->task_cnt; i++) {
        Vepu510H265eFrmCfg *frm_cfg = mpp_calloc(Vepu510H265eFrmCfg, 1);

        frm_cfg->regs_set = mpp_calloc(H265eV510RegSet, 1);
        frm_cfg->regs_ret = mpp_calloc(H265eV510StatusElem, 1);
        frm_cfg->frame_type = INTRA_FRAME;
        ctx->frms[i] = frm_cfg;
    }

    ctx->input_fmt      = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg            = cfg->cfg;
    hal_bufs_init(&ctx->dpb_bufs);

    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->enc_mode = 1;
    cfg->cap_recn_out = 1;
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }
    mpp_dev_multi_offset_init(&ctx->reg_cfg, 24);
    ctx->dev = cfg->dev;
    ctx->frame_type = INTRA_FRAME;

    {   /* setup default hardware config */
        MppEncHwCfg *hw = &cfg->cfg->hw;
        RK_U32 j;

        hw->qp_delta_row_i  = 2;
        hw->qp_delta_row    = 2;
        hw->qbias_i         = 171;
        hw->qbias_p         = 85;
        hw->qbias_en        = 0;

        memcpy(hw->aq_thrd_i, aq_thd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, aq_thd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, aq_qp_dealt_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, aq_qp_dealt_default, sizeof(hw->aq_step_p));

        for (j = 0; j < MPP_ARRAY_ELEMS(hw->mode_bias); j++)
            hw->mode_bias[j] = 8;
    }

    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v510_deinit(void *hal)
{
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    RK_S32 i = 0;

    hal_h265e_enter();
    MPP_FREE(ctx->input_fmt);
    hal_bufs_deinit(ctx->dpb_bufs);

    for (i = 0; i < ctx->task_cnt; i++) {
        Vepu510H265eFrmCfg *frm = ctx->frms[i];

        if (!frm)
            continue;

        if (frm->roi_base_cfg_buf) {
            mpp_buffer_put(frm->roi_base_cfg_buf);
            frm->roi_base_cfg_buf = NULL;
            frm->roi_base_buf_size = 0;
        }

        MPP_FREE(frm->roi_base_cfg_sw_buf);

        if (frm->reg_cfg) {
            mpp_dev_multi_offset_deinit(frm->reg_cfg);
            frm->reg_cfg = NULL;
        }

        MPP_FREE(ctx->frms[i]);
    }

    clear_ext_line_bufs(ctx);

    if (ctx->ext_line_buf_grp) {
        mpp_buffer_group_put(ctx->ext_line_buf_grp);
        ctx->ext_line_buf_grp = NULL;
    }

    if (ctx->buf_pass1) {
        mpp_buffer_put(ctx->buf_pass1);
        ctx->buf_pass1 = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    if (ctx->reg_cfg) {
        mpp_dev_multi_offset_deinit(ctx->reg_cfg);
        ctx->reg_cfg = NULL;
    }

    hal_h265e_leave();
    return MPP_OK;
}

static MPP_RET hal_h265e_vepu510_prepare(void *hal)
{
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h265e_dbg_func("enter %p\n", hal);

    if (prep->change & (MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT)) {
        RK_S32 i;

        // pre-alloc required buffers to reduce first frame delay
        vepu510_h265_setup_hal_bufs(ctx);
        for (i = 0; i < ctx->max_buf_cnt; i++)
            hal_bufs_get_buf(ctx->dpb_bufs, i);

        prep->change = 0;
    }

    hal_h265e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET
vepu510_h265_set_patch_info(H265eSyntax_new *syn, Vepu541Fmt input_fmt,  MppDevRegOffCfgs *offsets, HalEncTask *task)
{
    RK_U32 hor_stride = syn->pp.hor_stride;
    RK_U32 ver_stride = syn->pp.ver_stride ? syn->pp.ver_stride : syn->pp.pic_height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
        mpp_err("VEPU_510 unsupports FBC format input.\n");

        ret = MPP_NOK;
    } else {
        switch (input_fmt) {
        case VEPU541_FMT_YUV420P: {
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        } break;
        case VEPU541_FMT_YUV420SP:
        case VEPU541_FMT_YUV422SP: {
            u_offset = frame_size;
            v_offset = frame_size;
        } break;
        case VEPU541_FMT_YUV422P: {
            u_offset = frame_size;
            v_offset = frame_size * 3 / 2;
        } break;
        case VEPU540_FMT_YUV400:
        case VEPU541_FMT_YUYV422:
        case VEPU541_FMT_UYVY422: {
            u_offset = 0;
            v_offset = 0;
        } break;
        case VEPU541_FMT_BGR565:
        case VEPU541_FMT_BGR888:
        case VEPU541_FMT_BGRA8888: {
            u_offset = 0;
            v_offset = 0;
        } break;
        case VEPU580_FMT_YUV444SP : {
            u_offset = hor_stride * ver_stride;
            v_offset = hor_stride * ver_stride;
        } break;
        case VEPU580_FMT_YUV444P : {
            u_offset = hor_stride * ver_stride;
            v_offset = hor_stride * ver_stride * 2;
        } break;
        default: {
            hal_h265e_err("unknown color space: %d\n", input_fmt);
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        }
        }
    }
    mpp_dev_multi_offset_update(offsets, 161, u_offset);
    mpp_dev_multi_offset_update(offsets, 162, v_offset);

    return ret;
}


#if 0
static MPP_RET vepu510_h265_set_roi_regs(H265eV510HalContext *ctx, H265eVepu510Frame *regs)
{
    /* memset register on start so do not clear registers again here */
    if (ctx->roi_data) {
        /* roi setup */
        MppEncROICfg2 *cfg = ( MppEncROICfg2 *)ctx->roi_data;

        regs->reg0192_enc_pic.roi_en = 1;
        regs->reg0178_roi_addr = mpp_dev_get_iova_address(ctx->dev, cfg->base_cfg_buf, 0);
        if (cfg->roi_qp_en) {
            regs->reg0179_roi_qp_addr = mpp_dev_get_iova_address(ctx->dev, cfg->qp_cfg_buf, 0);
            regs->reg0228_roi_en.roi_qp_en = 1;
        }

        if (cfg->roi_amv_en) {
            regs->reg0180_roi_amv_addr = mpp_dev_get_iova_address(ctx->dev, cfg->amv_cfg_buf, 0);
            regs->reg0228_roi_en.roi_amv_en = 1;
        }

        if (cfg->roi_mv_en) {
            regs->reg0181_roi_mv_addr = mpp_dev_get_iova_address(ctx->dev, cfg->mv_cfg_buf, 0);
            regs->reg0228_roi_en.roi_mv_en = 1;
        }
    }

    return MPP_OK;
}
#endif

static MPP_RET vepu510_h265_set_rc_regs(H265eV510HalContext *ctx, H265eV510RegSet *regs, HalEncTask *task)
{
    H265eSyntax_new *syn = ctx->syn;
    EncRcTaskInfo *rc_cfg = &task->rc_task->info;
    H265eVepu510Frame *reg_frm = &regs->reg_frm;
    H265eVepu510RcRoi *reg_rc = &regs->reg_rc_roi;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncHwCfg *hw = &cfg->hw;
    MppEncCodecCfg *codec = &cfg->codec;
    MppEncH265Cfg *h265 = &codec->h265;
    RK_S32 mb_wd32 = (syn->pp.pic_width + 31) / 32;
    RK_S32 mb_h32 = (syn->pp.pic_height + 31) / 32;

    RK_U32 ctu_target_bits_mul_16 = (rc_cfg->bit_target << 4) / (mb_wd32 * mb_h32);
    RK_U32 ctu_target_bits;
    RK_S32 negative_bits_thd, positive_bits_thd;

    if (rc->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        reg_frm->reg0192_enc_pic.pic_qp    = rc_cfg->quality_target;
        reg_frm->reg0240_synt_sli1.sli_qp  = rc_cfg->quality_target;

        reg_frm->reg213_rc_qp.rc_max_qp   = rc_cfg->quality_target;
        reg_frm->reg213_rc_qp.rc_min_qp   = rc_cfg->quality_target;
    } else {
        if (ctu_target_bits_mul_16 >= 0x100000) {
            ctu_target_bits_mul_16 = 0x50000;
        }
        ctu_target_bits = (ctu_target_bits_mul_16 * mb_wd32) >> 4;
        negative_bits_thd = 0 - 5 * ctu_target_bits / 16;
        positive_bits_thd = 5 * ctu_target_bits / 16;

        reg_frm->reg0192_enc_pic.pic_qp    = rc_cfg->quality_target;
        reg_frm->reg0240_synt_sli1.sli_qp  = rc_cfg->quality_target;
        reg_frm->reg212_rc_cfg.rc_en      = 1;
        reg_frm->reg212_rc_cfg.aq_en  = 1;
        reg_frm->reg212_rc_cfg.rc_ctu_num = mb_wd32;
        reg_frm->reg213_rc_qp.rc_qp_range = (ctx->frame_type == INTRA_FRAME) ?
                                            hw->qp_delta_row_i : hw->qp_delta_row;
        reg_frm->reg213_rc_qp.rc_max_qp   = rc_cfg->quality_max;
        reg_frm->reg213_rc_qp.rc_min_qp   = rc_cfg->quality_min;
        reg_frm->reg214_rc_tgt.ctu_ebit  = ctu_target_bits_mul_16;

        reg_rc->rc_dthd_0_8[0] = 2 * negative_bits_thd;
        reg_rc->rc_dthd_0_8[1] = negative_bits_thd;
        reg_rc->rc_dthd_0_8[2] = positive_bits_thd;
        reg_rc->rc_dthd_0_8[3] = 2 * positive_bits_thd;
        reg_rc->rc_dthd_0_8[4] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[5] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[6] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[7] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[8] = 0x7FFFFFFF;

        reg_rc->rc_adj0.qp_adj0  = -2;
        reg_rc->rc_adj0.qp_adj1  = -1;
        reg_rc->rc_adj0.qp_adj2  = 0;
        reg_rc->rc_adj0.qp_adj3  = 1;
        reg_rc->rc_adj0.qp_adj4  = 2;
        reg_rc->rc_adj1.qp_adj5  = 0;
        reg_rc->rc_adj1.qp_adj6  = 0;
        reg_rc->rc_adj1.qp_adj7  = 0;
        reg_rc->rc_adj1.qp_adj8  = 0;

        reg_rc->roi_qthd0.qpmin_area0 = h265->qpmin_map[0] > 0 ? h265->qpmin_map[0] : rc_cfg->quality_min;
        reg_rc->roi_qthd0.qpmax_area0 = h265->qpmax_map[0] > 0 ? h265->qpmax_map[0] : rc_cfg->quality_max;
        reg_rc->roi_qthd0.qpmin_area1 = h265->qpmin_map[1] > 0 ? h265->qpmin_map[1] : rc_cfg->quality_min;
        reg_rc->roi_qthd0.qpmax_area1 = h265->qpmax_map[1] > 0 ? h265->qpmax_map[1] : rc_cfg->quality_max;
        reg_rc->roi_qthd0.qpmin_area2 = h265->qpmin_map[2] > 0 ? h265->qpmin_map[2] : rc_cfg->quality_min;;
        reg_rc->roi_qthd1.qpmax_area2 = h265->qpmax_map[2] > 0 ? h265->qpmax_map[2] : rc_cfg->quality_max;
        reg_rc->roi_qthd1.qpmin_area3 = h265->qpmin_map[3] > 0 ? h265->qpmin_map[3] : rc_cfg->quality_min;;
        reg_rc->roi_qthd1.qpmax_area3 = h265->qpmax_map[3] > 0 ? h265->qpmax_map[3] : rc_cfg->quality_max;
        reg_rc->roi_qthd1.qpmin_area4 = h265->qpmin_map[4] > 0 ? h265->qpmin_map[4] : rc_cfg->quality_min;;
        reg_rc->roi_qthd1.qpmax_area4 = h265->qpmax_map[4] > 0 ? h265->qpmax_map[4] : rc_cfg->quality_max;
        reg_rc->roi_qthd2.qpmin_area5 = h265->qpmin_map[5] > 0 ? h265->qpmin_map[5] : rc_cfg->quality_min;;
        reg_rc->roi_qthd2.qpmax_area5 = h265->qpmax_map[5] > 0 ? h265->qpmax_map[5] : rc_cfg->quality_max;
        reg_rc->roi_qthd2.qpmin_area6 = h265->qpmin_map[6] > 0 ? h265->qpmin_map[6] : rc_cfg->quality_min;;
        reg_rc->roi_qthd2.qpmax_area6 = h265->qpmax_map[6] > 0 ? h265->qpmax_map[6] : rc_cfg->quality_max;
        reg_rc->roi_qthd2.qpmin_area7 = h265->qpmin_map[7] > 0 ? h265->qpmin_map[7] : rc_cfg->quality_min;;
        reg_rc->roi_qthd3.qpmax_area7 = h265->qpmax_map[7] > 0 ? h265->qpmax_map[7] : rc_cfg->quality_max;
        reg_rc->roi_qthd3.qpmap_mode  = h265->qpmap_mode;
    }
    return MPP_OK;
}

static MPP_RET vepu510_h265_set_pp_regs(H265eV510RegSet *regs, VepuFmtCfg *fmt, MppEncPrepCfg *prep_cfg)
{
    H265eVepu510ControlCfg *reg_ctl = &regs->reg_ctl;
    H265eVepu510Frame        *reg_frm = &regs->reg_frm;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    reg_ctl->reg0012_dtrns_map.src_bus_edin = fmt->src_endian;
    reg_frm->reg0198_src_fmt.src_cfmt = fmt->format;
    reg_frm->reg0198_src_fmt.alpha_swap = fmt->alpha_swap;
    reg_frm->reg0198_src_fmt.rbuv_swap = fmt->rbuv_swap;

    // reg_frm->reg0198_src_fmt.src_range = fmt->src_range;
    reg_frm->reg0198_src_fmt.out_fmt = ((prep_cfg->format & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV400) ? 0 : 1;

    reg_frm->reg0203_src_proc.src_mirr = prep_cfg->mirroring > 0;
    reg_frm->reg0203_src_proc.src_rot = prep_cfg->rotation;
    reg_frm->reg0203_src_proc.tile4x4_en = 0;

    if (prep_cfg->hor_stride) {
        if (MPP_FRAME_FMT_IS_TILE(prep_cfg->format)) {
            reg_frm->reg0203_src_proc.tile4x4_en = 1;

            switch (prep_cfg->format & MPP_FRAME_FMT_MASK) {
            case MPP_FMT_YUV400:
                stridey = prep_cfg->hor_stride * 4;
                break;
            case MPP_FMT_YUV420P:
            case MPP_FMT_YUV420SP:
                stridey = prep_cfg->hor_stride * 4 * 3 / 2;
                break;
            case MPP_FMT_YUV422P:
            case MPP_FMT_YUV422SP:
                stridey = prep_cfg->hor_stride * 4 * 2;
                break;
            case MPP_FMT_YUV444P:
            case MPP_FMT_YUV444SP:
                stridey = prep_cfg->hor_stride * 4 * 3;
                break;
            default:
                mpp_err("Unsupported input format 0x%08x, with TILE mask.\n", fmt);
                return MPP_ERR_VALUE;
                break;
            }
        } else {
            stridey = prep_cfg->hor_stride;
        }
    } else {
        if (reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_BGRA8888 )
            stridey = prep_cfg->width * 4;
        else if (reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_BGR888 )
            stridey = prep_cfg->width * 3;
        else if (reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_BGR565 ||
                 reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_YUYV422 ||
                 reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_UYVY422)
            stridey = prep_cfg->width * 2;
    }

    stridec = (reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_YUV420SP ||
               reg_frm->reg0198_src_fmt.src_cfmt == VEPU541_FMT_YUV422SP ||
               reg_frm->reg0198_src_fmt.src_cfmt == VEPU580_FMT_YUV444P) ?
              stridey : stridey / 2;

    if (reg_frm->reg0198_src_fmt.src_cfmt == VEPU580_FMT_YUV444SP)
        stridec = stridey * 2;

    if (reg_frm->reg0198_src_fmt.src_cfmt < VEPU541_FMT_NONE) {
        const VepuRgb2YuvCfg *cfg_coeffs = cfg_coeffs = get_rgb2yuv_cfg(prep_cfg->range, prep_cfg->color);

        hal_h265e_dbg_simple("input color range %d colorspace %d", prep_cfg->range, prep_cfg->color);

        reg_frm->reg0199_src_udfy.csc_wgt_r2y = cfg_coeffs->_2y.r_coeff;
        reg_frm->reg0199_src_udfy.csc_wgt_g2y = cfg_coeffs->_2y.g_coeff;
        reg_frm->reg0199_src_udfy.csc_wgt_b2y = cfg_coeffs->_2y.b_coeff;

        reg_frm->reg0200_src_udfu.csc_wgt_r2u = cfg_coeffs->_2u.r_coeff;
        reg_frm->reg0200_src_udfu.csc_wgt_g2u = cfg_coeffs->_2u.g_coeff;
        reg_frm->reg0200_src_udfu.csc_wgt_b2u = cfg_coeffs->_2u.b_coeff;

        reg_frm->reg0201_src_udfv.csc_wgt_r2v = cfg_coeffs->_2v.r_coeff;
        reg_frm->reg0201_src_udfv.csc_wgt_g2v = cfg_coeffs->_2v.g_coeff;
        reg_frm->reg0201_src_udfv.csc_wgt_b2v = cfg_coeffs->_2v.b_coeff;

        reg_frm->reg0202_src_udfo.csc_ofst_y = cfg_coeffs->_2y.offset;
        reg_frm->reg0202_src_udfo.csc_ofst_u = cfg_coeffs->_2u.offset;
        reg_frm->reg0202_src_udfo.csc_ofst_v = cfg_coeffs->_2v.offset;

        hal_h265e_dbg_simple("use color range %d colorspace %d", cfg_coeffs->dst_range, cfg_coeffs->color);
    }

    reg_frm->reg0205_src_strd0.src_strd0  = stridey;
    reg_frm->reg0206_src_strd1.src_strd1  = stridec;

    return MPP_OK;
}

static void vepu510_h265_set_slice_regs(H265eSyntax_new *syn, H265eVepu510Frame *regs)
{
    regs->reg0237_synt_sps.smpl_adpt_ofst_e     = syn->pp.sample_adaptive_offset_enabled_flag;
    regs->reg0237_synt_sps.num_st_ref_pic       = syn->pp.num_short_term_ref_pic_sets;
    regs->reg0237_synt_sps.num_lt_ref_pic       = syn->pp.num_long_term_ref_pics_sps;
    regs->reg0237_synt_sps.lt_ref_pic_prsnt     = syn->pp.long_term_ref_pics_present_flag;
    regs->reg0237_synt_sps.tmpl_mvp_e           = syn->pp.sps_temporal_mvp_enabled_flag;
    regs->reg0237_synt_sps.log2_max_poc_lsb     = syn->pp.log2_max_pic_order_cnt_lsb_minus4;
    regs->reg0237_synt_sps.strg_intra_smth      = syn->pp.strong_intra_smoothing_enabled_flag;

    regs->reg0238_synt_pps.dpdnt_sli_seg_en     = syn->pp.dependent_slice_segments_enabled_flag;
    regs->reg0238_synt_pps.out_flg_prsnt_flg    = syn->pp.output_flag_present_flag;
    regs->reg0238_synt_pps.num_extr_sli_hdr     = syn->pp.num_extra_slice_header_bits;
    regs->reg0238_synt_pps.sgn_dat_hid_en       = syn->pp.sign_data_hiding_enabled_flag;
    regs->reg0238_synt_pps.cbc_init_prsnt_flg   = syn->pp.cabac_init_present_flag;
    regs->reg0238_synt_pps.pic_init_qp          = syn->pp.init_qp_minus26 + 26;
    regs->reg0238_synt_pps.cu_qp_dlt_en         = syn->pp.cu_qp_delta_enabled_flag;
    regs->reg0238_synt_pps.chrm_qp_ofst_prsn    = syn->pp.pps_slice_chroma_qp_offsets_present_flag;
    regs->reg0238_synt_pps.lp_fltr_acrs_sli     = syn->pp.pps_loop_filter_across_slices_enabled_flag;
    regs->reg0238_synt_pps.dblk_fltr_ovrd_en    = syn->pp.deblocking_filter_override_enabled_flag;
    regs->reg0238_synt_pps.lst_mdfy_prsnt_flg   = syn->pp.lists_modification_present_flag;
    regs->reg0238_synt_pps.sli_seg_hdr_extn     = syn->pp.slice_segment_header_extension_present_flag;
    regs->reg0238_synt_pps.cu_qp_dlt_depth      = syn->pp.diff_cu_qp_delta_depth;
    regs->reg0238_synt_pps.lpf_fltr_acrs_til    = syn->pp.loop_filter_across_tiles_enabled_flag;

    regs->reg0239_synt_sli0.cbc_init_flg        = syn->sp.cbc_init_flg;
    regs->reg0239_synt_sli0.mvd_l1_zero_flg     = syn->sp.mvd_l1_zero_flg;
    regs->reg0239_synt_sli0.mrg_up_flg          = syn->sp.merge_up_flag;
    regs->reg0239_synt_sli0.mrg_lft_flg         = syn->sp.merge_left_flag;
    regs->reg0239_synt_sli0.ref_pic_lst_mdf_l0  = syn->sp.ref_pic_lst_mdf_l0;

    regs->reg0239_synt_sli0.num_refidx_l1_act   = syn->sp.num_refidx_l1_act;
    regs->reg0239_synt_sli0.num_refidx_l0_act   = syn->sp.num_refidx_l0_act;

    regs->reg0239_synt_sli0.num_refidx_act_ovrd = syn->sp.num_refidx_act_ovrd;

    regs->reg0239_synt_sli0.sli_sao_chrm_flg    = syn->sp.sli_sao_chrm_flg;
    regs->reg0239_synt_sli0.sli_sao_luma_flg    = syn->sp.sli_sao_luma_flg;
    regs->reg0239_synt_sli0.sli_tmprl_mvp_e     = syn->sp.sli_tmprl_mvp_en;
    regs->reg0192_enc_pic.num_pic_tot_cur       = syn->sp.tot_poc_num;

    regs->reg0239_synt_sli0.pic_out_flg         = syn->sp.pic_out_flg;
    regs->reg0239_synt_sli0.sli_type            = syn->sp.slice_type;
    regs->reg0239_synt_sli0.sli_rsrv_flg        = syn->sp.slice_rsrv_flg;
    regs->reg0239_synt_sli0.dpdnt_sli_seg_flg   = syn->sp.dpdnt_sli_seg_flg;
    regs->reg0239_synt_sli0.sli_pps_id          = syn->sp.sli_pps_id;
    regs->reg0239_synt_sli0.no_out_pri_pic      = syn->sp.no_out_pri_pic;

    regs->reg0240_synt_sli1.sp_tc_ofst_div2       = syn->sp.sli_tc_ofst_div2;;
    regs->reg0240_synt_sli1.sp_beta_ofst_div2     = syn->sp.sli_beta_ofst_div2;
    regs->reg0240_synt_sli1.sli_lp_fltr_acrs_sli  = syn->sp.sli_lp_fltr_acrs_sli;
    regs->reg0240_synt_sli1.sp_dblk_fltr_dis      = syn->sp.sli_dblk_fltr_dis;
    regs->reg0240_synt_sli1.dblk_fltr_ovrd_flg    = syn->sp.dblk_fltr_ovrd_flg;
    regs->reg0240_synt_sli1.sli_cb_qp_ofst        = syn->sp.sli_cb_qp_ofst;
    regs->reg0240_synt_sli1.max_mrg_cnd           = syn->sp.max_mrg_cnd;

    regs->reg0240_synt_sli1.col_ref_idx           = syn->sp.col_ref_idx;
    regs->reg0240_synt_sli1.col_frm_l0_flg        = syn->sp.col_frm_l0_flg;
    regs->reg0241_synt_sli2.sli_poc_lsb           = syn->sp.sli_poc_lsb;
    regs->reg0241_synt_sli2.sli_hdr_ext_len       = syn->sp.sli_hdr_ext_len;
}

static void vepu510_h265_set_ref_regs(H265eSyntax_new *syn, H265eVepu510Frame *regs)
{
    regs->reg0242_synt_refm0.st_ref_pic_flg = syn->sp.st_ref_pic_flg;
    regs->reg0242_synt_refm0.poc_lsb_lt0 = syn->sp.poc_lsb_lt0;
    regs->reg0242_synt_refm0.num_lt_pic = syn->sp.num_lt_pic;

    regs->reg0243_synt_refm1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->reg0243_synt_refm1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->reg0243_synt_refm1.used_by_lt_flg0 = syn->sp.used_by_lt_flg0;
    regs->reg0243_synt_refm1.used_by_lt_flg1 = syn->sp.used_by_lt_flg1;
    regs->reg0243_synt_refm1.used_by_lt_flg2 = syn->sp.used_by_lt_flg2;
    regs->reg0243_synt_refm1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->reg0243_synt_refm1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->reg0243_synt_refm1.dlt_poc_msb_prsnt1 = syn->sp.dlt_poc_msb_prsnt1;
    regs->reg0243_synt_refm1.num_negative_pics = syn->sp.num_neg_pic;
    regs->reg0243_synt_refm1.num_pos_pic = syn->sp.num_pos_pic;

    regs->reg0243_synt_refm1.used_by_s0_flg = syn->sp.used_by_s0_flg;
    regs->reg0244_synt_refm2.dlt_poc_s0_m10 = syn->sp.dlt_poc_s0_m10;
    regs->reg0244_synt_refm2.dlt_poc_s0_m11 = syn->sp.dlt_poc_s0_m11;
    regs->reg0245_synt_refm3.dlt_poc_s0_m12 = syn->sp.dlt_poc_s0_m12;
    regs->reg0245_synt_refm3.dlt_poc_s0_m13 = syn->sp.dlt_poc_s0_m13;

    regs->reg0246_synt_long_refm0.poc_lsb_lt1 = syn->sp.poc_lsb_lt1;
    regs->reg0247_synt_long_refm1.dlt_poc_msb_cycl1 = syn->sp.dlt_poc_msb_cycl1;
    regs->reg0246_synt_long_refm0.poc_lsb_lt2 = syn->sp.poc_lsb_lt2;
    regs->reg0243_synt_refm1.dlt_poc_msb_prsnt2 = syn->sp.dlt_poc_msb_prsnt2;
    regs->reg0247_synt_long_refm1.dlt_poc_msb_cycl2 = syn->sp.dlt_poc_msb_cycl2;
    regs->reg0240_synt_sli1.lst_entry_l0 = syn->sp.lst_entry_l0;
    regs->reg0239_synt_sli0.ref_pic_lst_mdf_l0 = syn->sp.ref_pic_lst_mdf_l0;
}

static void vepu510_h265_set_me_regs(H265eV510HalContext *ctx, H265eSyntax_new *syn, H265eVepu510Frame *regs)
{
    regs->reg0220_me_rnge.cime_srch_dwnh    = 15;
    regs->reg0220_me_rnge.cime_srch_uph     = 14;
    regs->reg0220_me_rnge.cime_srch_rgtw    = 12;
    regs->reg0220_me_rnge.cime_srch_lftw    = 12;
    regs->reg0221_me_cfg.rme_srch_h         = 3;
    regs->reg0221_me_cfg.rme_srch_v         = 3;

    regs->reg0221_me_cfg.srgn_max_num       = 72;
    regs->reg0221_me_cfg.cime_dist_thre     = 1024;
    regs->reg0221_me_cfg.rme_dis            = 0;
    regs->reg0221_me_cfg.fme_dis            = 0;
    regs->reg0220_me_rnge.dlt_frm_num       = 0x1;

    if (syn->pp.sps_temporal_mvp_enabled_flag &&
        (ctx->frame_type != INTRA_FRAME)) {
        if (ctx->last_frame_type == INTRA_FRAME) {
            regs->reg0222_me_cach.colmv_load = 0;
        } else {
            regs->reg0222_me_cach.colmv_load = 1;
        }
        regs->reg0222_me_cach.colmv_stor    = 1;
    }

    regs->reg0222_me_cach.cime_zero_thre    = 1024;
    regs->reg0222_me_cach.fme_prefsu_en     = 0;
}

void vepu510_h265_set_hw_address(H265eV510HalContext *ctx, H265eVepu510Frame *regs, HalEncTask *task)
{
    HalEncTask *enc_task = task;
    HalBuf *recon_buf, *ref_buf;
    MppBuffer md_info_buf = enc_task->md_info;
    Vepu510H265eFrmCfg *frm = ctx->frm;
    H265eSyntax_new *syn = ctx->syn;

    hal_h265e_enter();

    regs->reg0160_adr_src0  = mpp_buffer_get_fd(enc_task->input);
    regs->reg0161_adr_src1  = regs->reg0160_adr_src0;
    regs->reg0162_adr_src2  = regs->reg0160_adr_src0;

    recon_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_curr_idx);
    ref_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_refr_idx);

    if (!syn->sp.non_reference_flag) {
        regs->reg0163_rfpw_h_addr  = mpp_buffer_get_fd(recon_buf->buf[0]);
        regs->reg0164_rfpw_b_addr  = regs->reg0163_rfpw_h_addr;
        mpp_dev_multi_offset_update(ctx->reg_cfg, 164, ctx->fbc_header_len);
    }
    regs->reg0165_rfpr_h_addr = mpp_buffer_get_fd(ref_buf->buf[0]);
    regs->reg0166_rfpr_b_addr = regs->reg0165_rfpr_h_addr;
    regs->reg0167_cmvw_addr = mpp_buffer_get_fd(recon_buf->buf[2]);
    regs->reg0168_cmvr_addr = mpp_buffer_get_fd(ref_buf->buf[2]);
    regs->reg0169_dspw_addr = mpp_buffer_get_fd(recon_buf->buf[1]);
    regs->reg0170_dspr_addr = mpp_buffer_get_fd(ref_buf->buf[1]);

    mpp_dev_multi_offset_update(ctx->reg_cfg, 166, ctx->fbc_header_len);

    if (md_info_buf) {
        regs->reg0192_enc_pic.mei_stor    = 1;
        regs->reg0171_meiw_addr = mpp_buffer_get_fd(md_info_buf);
    } else {
        regs->reg0192_enc_pic.mei_stor    = 0;
        regs->reg0171_meiw_addr = 0;
    }

    regs->reg0172_bsbt_addr = mpp_buffer_get_fd(enc_task->output);
    /* TODO: stream size relative with syntax */
    regs->reg0173_bsbb_addr  = regs->reg0172_bsbt_addr;
    regs->reg0175_bsbr_addr  = regs->reg0172_bsbt_addr;
    regs->reg0174_adr_bsbs   = regs->reg0172_bsbt_addr;

    regs->reg0180_adr_rfpt_h = 0xffffffff;
    regs->reg0181_adr_rfpb_h = 0;
    regs->reg0182_adr_rfpt_b = 0xffffffff;
    regs->reg0183_adr_rfpb_b = 0;

    mpp_dev_multi_offset_update(ctx->reg_cfg, 174, mpp_packet_get_length(task->packet));
    mpp_dev_multi_offset_update(ctx->reg_cfg, 172, mpp_buffer_get_size(enc_task->output));

    regs->reg0204_pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->reg0204_pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);
}

static MPP_RET vepu510_h265e_save_pass1_patch(H265eV510RegSet *regs, H265eV510HalContext *ctx,
                                              RK_S32 tiles_enabled_flag)
{
    H265eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_S32 width = ctx->cfg->prep.width;
    RK_S32 height = ctx->cfg->prep.height;
    RK_S32 width_align = MPP_ALIGN(width, 16);
    RK_S32 height_align = MPP_ALIGN(height, 16);

    if (NULL == ctx->buf_pass1) {
        mpp_buffer_get(NULL, &ctx->buf_pass1, width_align * height_align * 3 / 2);
        if (!ctx->buf_pass1) {
            mpp_err("buf_pass1 malloc fail, debreath invaild");
            return MPP_NOK;
        }
    }

    reg_frm->reg0192_enc_pic.cur_frm_ref = 1;
    reg_frm->reg0163_rfpw_h_addr = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_frm->reg0164_rfpw_b_addr = reg_frm->reg0163_rfpw_h_addr;
    reg_frm->reg0192_enc_pic.rec_fbc_dis = 1;

    if (tiles_enabled_flag)
        reg_frm->reg0238_synt_pps.lpf_fltr_acrs_til = 0;

    mpp_dev_multi_offset_update(ctx->reg_cfg, 164, 0);

    /* NOTE: disable split to avoid lowdelay slice output */
    regs->reg_frm.reg0216_sli_splt.sli_splt = 0;
    regs->reg_frm.reg0192_enc_pic.slen_fifo = 0;

    return MPP_OK;
}

static MPP_RET vepu510_h265e_use_pass1_patch(H265eV510RegSet *regs, H265eV510HalContext *ctx)
{
    H265eVepu510ControlCfg *reg_ctl = &regs->reg_ctl;
    H265eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_U32 hor_stride = MPP_ALIGN(ctx->cfg->prep.width, 16);
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    MPP_RET ret = MPP_OK;

    hal_h265e_dbg_func("enter\n");

    reg_ctl->reg0012_dtrns_map.src_bus_edin = fmt->src_endian;
    reg_frm->reg0198_src_fmt.src_cfmt = VEPU541_FMT_YUV420SP;
    reg_frm->reg0198_src_fmt.alpha_swap = 0;
    reg_frm->reg0198_src_fmt.rbuv_swap = 0;
    reg_frm->reg0198_src_fmt.out_fmt = 1;
    regs->reg_frm.reg0198_src_fmt.src_rcne   = 1;

    reg_frm->reg0205_src_strd0.src_strd0 = hor_stride;
    reg_frm->reg0206_src_strd1.src_strd1 = 3 * hor_stride;

    reg_frm->reg0203_src_proc.src_mirr = 0;
    reg_frm->reg0203_src_proc.src_rot = 0;

    reg_frm->reg0160_adr_src0 = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_frm->reg0161_adr_src1 = reg_frm->reg0160_adr_src0;
    reg_frm->reg0162_adr_src2 = 0;

    /* input cb addr */
    ret = mpp_dev_multi_offset_update(ctx->reg_cfg, 161, 2 * hor_stride);
    if (ret)
        mpp_err_f("set input cb addr offset failed %d\n", ret);

    return MPP_OK;
}

static void setup_vepu510_ext_line_buf(H265eV510HalContext *ctx, H265eV510RegSet *regs)
{
    MppDevRcbInfoCfg rcb_cfg;
    RK_S32 offset = 0;
    RK_S32 fd;

    if (ctx->ext_line_buf) {
        fd = mpp_buffer_get_fd(ctx->ext_line_buf);
        offset = ctx->ext_line_buf_size;

        regs->reg_frm.reg0179_adr_ebufb = fd;
        regs->reg_frm.reg0178_adr_ebuft = fd;
        mpp_dev_multi_offset_update(ctx->reg_cfg, 178, ctx->ext_line_buf_size);
    } else {
        regs->reg_frm.reg0179_adr_ebufb = 0;
        regs->reg_frm.reg0178_adr_ebuft = 0;
    }

    /* rcb info for sram */
    rcb_cfg.reg_idx = 179;
    rcb_cfg.size = offset;

    mpp_dev_ioctl(ctx->dev, MPP_DEV_RCB_INFO, &rcb_cfg);

    rcb_cfg.reg_idx = 178;
    rcb_cfg.size = 0;

    mpp_dev_ioctl(ctx->dev, MPP_DEV_RCB_INFO, &rcb_cfg);
}

static MPP_RET setup_vepu510_dual_core(H265eV510HalContext *ctx)
{
    Vepu510H265eFrmCfg *frm = ctx->frm;
    H265eV510RegSet *regs = frm->regs_set;
    H265eVepu510Frame        *reg_frm = &regs->reg_frm;
    RK_U32 dchs_ofst = 9;
    RK_U32 dchs_dly = 0;
    RK_U32 dchs_rxe  = 1;

    if (ctx->task_cnt == 1)
        return MPP_OK;

    if (ctx->frame_type == INTRA_FRAME) {
        ctx->curr_idx = 0;
        ctx->prev_idx = 0;
        dchs_rxe = 0;
    }

    reg_frm->reg0193_dual_core.dchs_txid = ctx->curr_idx;
    reg_frm->reg0193_dual_core.dchs_rxid = ctx->prev_idx;
    reg_frm->reg0193_dual_core.dchs_txe = 1;
    reg_frm->reg0193_dual_core.dchs_rxe = dchs_rxe;
    reg_frm->reg0193_dual_core.dchs_ofst = dchs_ofst;
    reg_frm->reg0193_dual_core.dchs_dly = dchs_dly;

    ctx->prev_idx = ctx->curr_idx++;
    if (ctx->curr_idx > 3)
        ctx->curr_idx = 0;

    return MPP_OK;
}

static void setup_vepu510_split(H265eVepu510Frame *regs, MppEncSliceSplit *cfg)
{
    hal_h265e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        regs->reg0216_sli_splt.sli_splt = 0;
        regs->reg0216_sli_splt.sli_splt_mode = 0;
        regs->reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg0216_sli_splt.sli_max_num_m1 = 0;
        regs->reg0216_sli_splt.sli_flsh = 0;
        regs->reg0218_sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg0217_sli_byte.sli_splt_byte = 0;
        regs->reg0192_enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        regs->reg0216_sli_splt.sli_splt = 1;
        regs->reg0216_sli_splt.sli_splt_mode = 0;
        regs->reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg0216_sli_splt.sli_max_num_m1 = 500;
        regs->reg0216_sli_splt.sli_flsh = 1;
        regs->reg0218_sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg0217_sli_byte.sli_splt_byte = cfg->split_arg;//4096
        regs->reg0192_enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        regs->reg0216_sli_splt.sli_splt = 1;
        regs->reg0216_sli_splt.sli_splt_mode = 1;
        regs->reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg0216_sli_splt.sli_max_num_m1 = 500;
        regs->reg0216_sli_splt.sli_flsh = 1;
        regs->reg0218_sli_cnum.sli_splt_cnum_m1 = cfg->split_arg - 1;

        regs->reg0217_sli_byte.sli_splt_byte = 0;
        regs->reg0192_enc_pic.slen_fifo = 0;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }

    cfg->change = 0;

    hal_h265e_dbg_func("leave\n");
}

MPP_RET hal_h265e_v510_gen_regs(void *hal, HalEncTask *task)
{
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    HalEncTask *enc_task = task;
    EncRcTask *rc_task = enc_task->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    H265eSyntax_new *syn = ctx->syn;
    Vepu510H265eFrmCfg *frm_cfg = ctx->frm;
    H265eV510RegSet *regs = frm_cfg->regs_set;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 pic_wd32, pic_h32;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    MppEncSliceSplit *slice_cfg = &ctx->cfg->split;
    H265eVepu510ControlCfg *reg_ctl = &regs->reg_ctl;
    H265eVepu510Frame        *reg_frm = &regs->reg_frm;
    H265eVepu510RcRoi *reg_klut = &regs->reg_rc_roi;
    MPP_RET ret = MPP_OK;

    hal_h265e_enter();
    pic_width_align8 = (syn->pp.pic_width + 7) & (~7);
    pic_height_align8 = (syn->pp.pic_height + 7) & (~7);
    pic_wd32 = (syn->pp.pic_width +  31) / 32;
    pic_h32 = (syn->pp.pic_height + 31) / 32;

    hal_h265e_dbg_simple("frame %d | type %d | start gen regs",
                         ctx->frame_cnt, ctx->frame_type);

    memset(regs, 0, sizeof(H265eV510RegSet));

    reg_ctl->reg0004_enc_strt.lkt_num      = 0;
    reg_ctl->reg0004_enc_strt.vepu_cmd     = ctx->enc_mode;
    reg_ctl->reg0005_enc_clr.safe_clr      = 0x0;
    reg_ctl->reg0005_enc_clr.force_clr     = 0x0;

    reg_ctl->reg0008_int_en.enc_done_en        = 1;
    reg_ctl->reg0008_int_en.lkt_node_done_en   = 1;
    reg_ctl->reg0008_int_en.sclr_done_en       = 1;
    reg_ctl->reg0008_int_en.vslc_done_en       = 1;
    reg_ctl->reg0008_int_en.vbsf_oflw_en       = 1;
    reg_ctl->reg0008_int_en.vbuf_lens_en       = 1;
    reg_ctl->reg0008_int_en.enc_err_en         = 1;
    reg_ctl->reg0008_int_en.dvbm_fcfg_en       = 1;
    reg_ctl->reg0008_int_en.wdg_en             = 1;
    reg_ctl->reg0008_int_en.lkt_err_int_en     = 0;
    reg_ctl->reg0008_int_en.lkt_err_stop_en    = 1;
    reg_ctl->reg0008_int_en.lkt_force_stop_en  = 1;
    reg_ctl->reg0008_int_en.jslc_done_en       = 1;
    reg_ctl->reg0008_int_en.jbsf_oflw_en       = 1;
    reg_ctl->reg0008_int_en.jbuf_lens_en       = 1;
    reg_ctl->reg0008_int_en.dvbm_dcnt_en       = 1;

    reg_ctl->reg0012_dtrns_map.jpeg_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.src_bus_edin     = 0x0;
    reg_ctl->reg0012_dtrns_map.meiw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.bsw_bus_edin     = 0x7;
    reg_ctl->reg0012_dtrns_map.lktr_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.roir_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.lktw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.rec_nfbc_bus_edin   = 0x0;

    reg_ctl->reg0013_dtrns_cfg.axi_brsp_cke     = 0x0;
    reg_ctl->reg0014_enc_wdg.vs_load_thd        = 0;
    reg_ctl->reg0014_enc_wdg.rfp_load_thd       = 0;

    reg_ctl->reg0021_func_en.cke                = 1;
    reg_ctl->reg0021_func_en.resetn_hw_en       = 1;
    reg_ctl->reg0021_func_en.rfpr_err_e         = 1;

    reg_frm->reg0196_enc_rsl.pic_wd8_m1    = pic_width_align8 / 8 - 1;
    reg_frm->reg0197_src_fill.pic_wfill    = (syn->pp.pic_width & 0x7)
                                             ? (8 - (syn->pp.pic_width & 0x7)) : 0;
    reg_frm->reg0196_enc_rsl.pic_hd8_m1    = pic_height_align8 / 8 - 1;
    reg_frm->reg0197_src_fill.pic_hfill    = (syn->pp.pic_height & 0x7)
                                             ? (8 - (syn->pp.pic_height & 0x7)) : 0;

    reg_frm->reg0192_enc_pic.enc_stnd      = 1; //H265
    reg_frm->reg0192_enc_pic.cur_frm_ref   = !syn->sp.non_reference_flag; //current frame will be refered
    reg_frm->reg0192_enc_pic.bs_scp        = 1;
    reg_frm->reg0192_enc_pic.log2_ctu_num  = mpp_ceil_log2(pic_wd32 * pic_h32);

    reg_frm->reg0203_src_proc.src_mirr     = 0;
    reg_frm->reg0203_src_proc.src_rot      = 0;
    reg_frm->reg0203_src_proc.tile4x4_en   = 0;

    reg_klut->klut_ofst.chrm_klut_ofst = (ctx->frame_type == INTRA_FRAME) ? 6 :
                                         (ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC ? 9 : 6);

    reg_frm->reg0216_sli_splt.sli_splt_mode     = syn->sp.sli_splt_mode;
    reg_frm->reg0216_sli_splt.sli_splt_cpst     = syn->sp.sli_splt_cpst;
    reg_frm->reg0216_sli_splt.sli_splt          = syn->sp.sli_splt;
    reg_frm->reg0216_sli_splt.sli_flsh          = syn->sp.sli_flsh;
    reg_frm->reg0216_sli_splt.sli_max_num_m1    = syn->sp.sli_max_num_m1;

    reg_frm->reg0218_sli_cnum.sli_splt_cnum_m1  = syn->sp.sli_splt_cnum_m1;
    reg_frm->reg0217_sli_byte.sli_splt_byte = syn->sp.sli_splt_byte;
    reg_frm->reg0248_sao_cfg.sao_lambda_multi = 5;

    setup_vepu510_split(reg_frm, &ctx->cfg->split);
    reg_frm->reg0192_enc_pic.slen_fifo = slice_cfg->split_out ? 1 : 0;

    if (ctx->task_cnt > 1)
        setup_vepu510_dual_core(ctx);

    vepu510_h265_set_me_regs(ctx, syn, reg_frm);

    reg_frm->reg0232_rdo_cfg.chrm_spcl   = 0;
    reg_frm->reg0232_rdo_cfg.cu_inter_e    = 0x0092;
    reg_frm->reg0232_rdo_cfg.cu_intra_e    = 0xe;
    reg_frm->reg0232_rdo_cfg.lambda_qp_use_avg_cu16_flag = 1;
    reg_frm->reg0232_rdo_cfg.yuvskip_calc_en = 1;
    reg_frm->reg0232_rdo_cfg.atf_e = 1;
    reg_frm->reg0232_rdo_cfg.atr_e = 1;

    if (syn->pp.num_long_term_ref_pics_sps) {
        reg_frm->reg0232_rdo_cfg.ltm_col   = 0;
        reg_frm->reg0232_rdo_cfg.ltm_idx0l0 = 1;
    } else {
        reg_frm->reg0232_rdo_cfg.ltm_col   = 0;
        reg_frm->reg0232_rdo_cfg.ltm_idx0l0 = 0;
    }

    reg_frm->reg0232_rdo_cfg.ccwa_e = 1;
    reg_frm->reg0232_rdo_cfg.scl_lst_sel = syn->pp.scaling_list_enabled_flag;
    {
        RK_U32 i_nal_type = 0;

        /* TODO: extend syn->frame_coding_type definition */
        if (ctx->frame_type == INTRA_FRAME) {
            /* reset ref pictures */
            i_nal_type    = NAL_IDR_W_RADL;
        } else if (ctx->frame_type == INTER_P_FRAME ) {
            i_nal_type    = NAL_TRAIL_R;
        } else {
            i_nal_type    = NAL_TRAIL_R;
        }
        reg_frm->reg0236_synt_nal.nal_unit_type    = i_nal_type;
    }

    vepu510_h265_set_hw_address(ctx, reg_frm, task);
    vepu510_h265_set_pp_regs(regs, fmt, &ctx->cfg->prep);
    vepu510_h265_set_rc_regs(ctx, regs, task);
    vepu510_h265_set_slice_regs(syn, reg_frm);
    vepu510_h265_set_ref_regs(syn, reg_frm);
    ret = vepu510_h265_set_patch_info(syn, (Vepu541Fmt)fmt->format, ctx->reg_cfg, enc_task);
    if (ret)
        return ret;

    setup_vepu510_ext_line_buf(ctx, regs);

    /* ROI configure */
    if (ctx->roi_data)
        vepu510_set_roi(&regs->reg_rc_roi.roi_cfg, ctx->roi_data,
                        ctx->cfg->prep.width, ctx->cfg->prep.height);
    /*paramet cfg*/
    vepu510_h265_global_cfg_set(ctx, regs);

    /* two pass register patch */
    if (frm->save_pass1)
        vepu510_h265e_save_pass1_patch(regs, ctx, syn->pp.tiles_enabled_flag);

    if (frm->use_pass1)
        vepu510_h265e_use_pass1_patch(regs, ctx);


    ctx->frame_num++;

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_v510_start(void *hal, HalEncTask *enc_task)
{
    MPP_RET ret = MPP_OK;
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    Vepu510H265eFrmCfg *frm = ctx->frm;
    RK_U32 *regs = (RK_U32*)frm->regs_set;
    H265eV510RegSet *hw_regs = frm->regs_set;
    H265eV510StatusElem *reg_out = (H265eV510StatusElem *)frm->regs_ret;
    MppDevRegWrCfg cfg;
    MppDevRegRdCfg cfg1;
    RK_U32 i = 0;

    hal_h265e_enter();
    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return e arly",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    cfg.reg = (RK_U32*)&hw_regs->reg_ctl;
    cfg.size = sizeof(H265eVepu510ControlCfg);
    cfg.offset = VEPU510_CTL_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_CTL_REGS) {
        regs = (RK_U32*)&hw_regs->reg_ctl;
        for (i = 0; i < sizeof(H265eVepu510ControlCfg) / 4; i++) {
            hal_h265e_dbg_ctl("ctl reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_frm;
    cfg.size = sizeof(H265eVepu510Frame);
    cfg.offset = VEPU510_FRAME_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_REGS) {
        regs = (RK_U32*)(&hw_regs->reg_frm);
        for (i = 0; i < 32; i++) {
            hal_h265e_dbg_regs("hw add cfg reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
        regs += 32;
        for (i = 0; i < (sizeof(H265eVepu510Frame) - 128) / 4; i++) {
            hal_h265e_dbg_regs("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }
    cfg.reg = &hw_regs->reg_rc_roi;
    cfg.size = sizeof(H265eVepu510RcRoi);
    cfg.offset = VEPU510_RC_ROI_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_RCKUT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_rc_roi;
        for (i = 0; i < sizeof(H265eVepu510RcRoi) / 4; i++) {
            hal_h265e_dbg_rckut("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg =  &hw_regs->reg_param;
    cfg.size = sizeof(H265eVepu510Param);
    cfg.offset = VEPU510_PARAM_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_param;
        for (i = 0; i < sizeof(H265eVepu510Param) / 4; i++) {
            hal_h265e_dbg_wgt("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_sqi;
    cfg.size = sizeof(Vepu510Sqi);
    cfg.offset = VEPU510_SQI_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFS, ctx->reg_cfg);
    if (ret) {
        mpp_err_f("set register offsets failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->hw_status;
    cfg1.size = sizeof(RK_U32);
    cfg1.offset = VEPU510_REG_BASE_HW_STATUS;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->st;
    cfg1.size = sizeof(H265eV510StatusElem) - 4;
    cfg1.offset = VEPU510_STATUS_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }
    hal_h265e_leave();
    return ret;
}

static MPP_RET vepu510_h265_set_feedback(H265eV510HalContext *ctx, HalEncTask *enc_task)
{
    EncRcTaskInfo *hal_rc_ret = (EncRcTaskInfo *)&enc_task->rc_task->info;
    Vepu510H265eFrmCfg *frm = ctx->frms[enc_task->flags.reg_idx];
    Vepu510H265Fbk  *fb = &frm->feedback;
    MppEncCfgSet    *cfg = ctx->cfg;
    RK_S32 mb64_num = ((cfg->prep.width + 63) / 64) * ((cfg->prep.height + 63) / 64);
    RK_S32 mb8_num = (mb64_num << 6);
    RK_S32 mb4_num = (mb8_num << 2);
    H265eV510StatusElem *elem = (H265eV510StatusElem *)frm->regs_ret;
    RK_U32 hw_status = elem->hw_status;

    hal_h265e_enter();

    fb->qp_sum += elem->st.qp_sum;
    fb->out_strm_size += elem->st.bs_lgth_l32;
    fb->sse_sum += (RK_S64)(elem->st.sse_h32 << 16) +
                   ((elem->st.st_sse_bsl.sse_l16 >> 16) & 0xffff) ;

    fb->hw_status = hw_status;
    hal_h265e_dbg_detail("hw_status: 0x%08x", hw_status);
    if (hw_status & RKV_ENC_INT_LINKTABLE_FINISH)
        hal_h265e_err("RKV_ENC_INT_LINKTABLE_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_FRAME_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_ONE_FRAME_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_SLICE_FINISH)
        hal_h265e_err("RKV_ENC_INT_ONE_SLICE_FINISH");

    if (hw_status & RKV_ENC_INT_SAFE_CLEAR_FINISH)
        hal_h265e_err("RKV_ENC_INT_SAFE_CLEAR_FINISH");

    if (hw_status & RKV_ENC_INT_BIT_STREAM_OVERFLOW)
        hal_h265e_err("RKV_ENC_INT_BIT_STREAM_OVERFLOW");

    if (hw_status & RKV_ENC_INT_BUS_WRITE_FULL)
        hal_h265e_err("RKV_ENC_INT_BUS_WRITE_FULL");

    if (hw_status & RKV_ENC_INT_BUS_WRITE_ERROR)
        hal_h265e_err("RKV_ENC_INT_BUS_WRITE_ERROR");

    if (hw_status & RKV_ENC_INT_BUS_READ_ERROR)
        hal_h265e_err("RKV_ENC_INT_BUS_READ_ERROR");

    if (hw_status & RKV_ENC_INT_TIMEOUT_ERROR)
        hal_h265e_err("RKV_ENC_INT_TIMEOUT_ERROR");

    fb->st_mb_num += elem->st.st_bnum_b16.num_b16;

    fb->st_lvl64_inter_num += elem->st.st_pnum_p64.pnum_p64;
    fb->st_lvl32_inter_num += elem->st.st_pnum_p32.pnum_p32;
    fb->st_lvl32_intra_num += elem->st.st_pnum_i32.pnum_i32;
    fb->st_lvl16_inter_num += elem->st.st_pnum_p16.pnum_p16;
    fb->st_lvl16_intra_num += elem->st.st_pnum_i16.pnum_i16;
    fb->st_lvl8_inter_num  += elem->st.st_pnum_p8.pnum_p8;
    fb->st_lvl8_intra_num  += elem->st.st_pnum_i8.pnum_i8;
    fb->st_lvl4_intra_num  += elem->st.st_pnum_i4.pnum_i4;
    memcpy(&fb->st_cu_num_qp[0], &elem->st.st_b8_qp, 52 * sizeof(RK_U32));

    hal_rc_ret->bit_real += fb->out_strm_size * 8;

    if (fb->st_mb_num) {
        fb->st_madi = fb->st_madi / fb->st_mb_num;
    } else {
        fb->st_madi = 0;
    }
    if (fb->st_ctu_num) {
        fb->st_madp = fb->st_madp / fb->st_ctu_num;
    } else {
        fb->st_madp = 0;
    }

    if (mb4_num > 0)
        hal_rc_ret->iblk4_prop =  ((((fb->st_lvl4_intra_num + fb->st_lvl8_intra_num) << 2) +
                                    (fb->st_lvl16_intra_num << 4) +
                                    (fb->st_lvl32_intra_num << 6)) << 8) / mb4_num;

    if (mb64_num > 0) {
        /*
        hal_cfg[k].inter_lv8_prop = ((fb->st_lvl8_inter_num + (fb->st_lvl16_inter_num << 2) +
                                      (fb->st_lvl32_inter_num << 4) +
                                      (fb->st_lvl64_inter_num << 6)) << 8) / mb8_num;*/

        hal_rc_ret->quality_real = fb->qp_sum / mb8_num;
        // hal_cfg[k].sse          = fb->sse_sum / mb64_num;
    }

    hal_rc_ret->madi = fb->st_madi;
    hal_rc_ret->madp = fb->st_madp;
    hal_h265e_leave();
    return MPP_OK;
}


//#define DUMP_DATA
MPP_RET hal_h265e_v510_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    HalEncTask *enc_task = task;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu510H265eFrmCfg *frm = ctx->frms[task_idx];
    H265eV510StatusElem *elem = (H265eV510StatusElem *)frm->regs_ret;
    hal_h265e_enter();

    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);

#ifdef DUMP_DATA
    static FILE *fp_fbd = NULL;
    static FILE *fp_fbh = NULL;
    static FILE *fp_dws = NULL;
    HalBuf *recon_buf;
    static RK_U32 frm_num = 0;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;
    recon_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.recon_pic.slot_idx);
    char file_name[20] = "";
    size_t rec_size = mpp_buffer_get_size(recon_buf->buf[0]);
    size_t dws_size = mpp_buffer_get_size(recon_buf->buf[1]);

    void *ptr = mpp_buffer_get_ptr(recon_buf->buf[0]);
    void *dws_ptr = mpp_buffer_get_ptr(recon_buf->buf[1]);

    sprintf(&file_name[0], "fbd%d.bin", frm_num);
    if (fp_fbd != NULL) {
        fclose(fp_fbd);
        fp_fbd = NULL;
    } else {
        fp_fbd = fopen(file_name, "wb+");
    }
    if (fp_fbd) {
        fwrite(ptr + ctx->fbc_header_len, 1, rec_size - ctx->fbc_header_len, fp_fbd);
        fflush(fp_fbd);
    }

    sprintf(&file_name[0], "fbh%d.bin", frm_num);

    if (fp_fbh != NULL) {
        fclose(fp_fbh);
        fp_fbh = NULL;
    } else {
        fp_fbh = fopen(file_name, "wb+");
    }

    if (fp_fbh) {
        fwrite(ptr , 1, ctx->fbc_header_len, fp_fbh);
        fflush(fp_fbh);
    }

    sprintf(&file_name[0], "dws%d.bin", frm_num);

    if (fp_dws != NULL) {
        fclose(fp_dws);
        fp_dws = NULL;
    } else {
        fp_dws = fopen(file_name, "wb+");
    }

    if (fp_dws) {
        fwrite(dws_ptr , 1, dws_size, fp_dws);
        fflush(fp_dws);
    }
    frm_num++;
#endif
    if (ret)
        mpp_err_f("poll cmd failed %d status %d \n", ret, elem->hw_status);

    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v510_get_task(void *hal, HalEncTask *task)
{
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    Vepu510H265eFrmCfg *frm_cfg = NULL;
    MppFrame frame = task->frame;
    EncFrmStatus  *frm_status = &task->rc_task->frm;
    RK_S32 task_idx = ctx->task_idx;

    hal_h265e_enter();

    ctx->syn = (H265eSyntax_new *)task->syntax.data;
    ctx->dpb = (H265eDpb*)ctx->syn->dpb;

    if (vepu510_h265_setup_hal_bufs(ctx)) {
        hal_h265e_err("vepu541_h265_allocate_buffers failed, free buffers and return\n");
        task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
        return MPP_ERR_MALLOC;
    }

    ctx->last_frame_type = ctx->frame_type;

    frm_cfg = ctx->frms[task_idx];
    ctx->frm = frm_cfg;

    if (frm_status->is_intra) {
        ctx->frame_type = INTRA_FRAME;
    } else {
        ctx->frame_type = INTER_P_FRAME;
    }
    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(frame);

        mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void **)&ctx->roi_data);
    }

    task->flags.reg_idx = ctx->task_idx;
    ctx->ext_line_buf = ctx->ext_line_bufs[ctx->task_idx];
    frm_cfg->frame_count = ctx->frame_count++;

    ctx->task_idx++;
    if (ctx->task_idx >= ctx->task_cnt)
        ctx->task_idx = 0;

    frm_cfg->hal_curr_idx = ctx->syn->sp.recon_pic.slot_idx;
    frm_cfg->hal_refr_idx = ctx->syn->sp.ref_pic.slot_idx;

    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_curr_idx);
    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_refr_idx);

    memset(&frm_cfg->feedback, 0, sizeof(Vepu510H265Fbk));

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_v510_ret_task(void *hal, HalEncTask *task)
{
    H265eV510HalContext *ctx = (H265eV510HalContext *)hal;
    HalEncTask *enc_task = task;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu510H265eFrmCfg *frm = ctx->frms[task_idx];
    Vepu510H265Fbk *fb = &frm->feedback;
    EncRcTaskInfo *rc_info = &task->rc_task->info;

    hal_h265e_enter();

    vepu510_h265_set_feedback(ctx, enc_task);

    rc_info->sse = fb->sse_sum;
    rc_info->lvl64_inter_num = fb->st_lvl64_inter_num;
    rc_info->lvl32_inter_num = fb->st_lvl32_inter_num;
    rc_info->lvl16_inter_num = fb->st_lvl16_inter_num;
    rc_info->lvl8_inter_num  = fb->st_lvl8_inter_num;
    rc_info->lvl32_intra_num = fb->st_lvl32_intra_num;
    rc_info->lvl16_intra_num = fb->st_lvl16_intra_num;
    rc_info->lvl8_intra_num  = fb->st_lvl8_intra_num;
    rc_info->lvl4_intra_num  = fb->st_lvl4_intra_num;

    enc_task->hw_length = fb->out_strm_size;
    enc_task->length += fb->out_strm_size;

    h265e_dpb_hal_end(ctx->dpb, frm->hal_curr_idx);
    h265e_dpb_hal_end(ctx->dpb, frm->hal_refr_idx);

    hal_h265e_dbg_detail("output stream size %d\n", fb->out_strm_size);

    hal_h265e_leave();
    return MPP_OK;
}

const MppEncHalApi hal_h265e_vepu510 = {
    "hal_h265e_v510",
    MPP_VIDEO_CodingHEVC,
    sizeof(H265eV510HalContext),
    0,
    hal_h265e_v510_init,
    hal_h265e_v510_deinit,
    hal_h265e_vepu510_prepare,
    hal_h265e_v510_get_task,
    hal_h265e_v510_gen_regs,
    hal_h265e_v510_start,
    hal_h265e_v510_wait,
    NULL,
    NULL,
    hal_h265e_v510_ret_task,
};
