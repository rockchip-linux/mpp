/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_h264e_vepu510"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_enc_cb_param.h"

#include "rkv_enc_def.h"
#include "hal_h264e_debug.h"
#include "hal_bufs.h"
#include "hal_h264e_vepu510_reg.h"
#include "hal_h264e_vepu510.h"
#include "hal_h264e_stream_amend.h"
#include "h264e_dpb.h"

#include "vepu5xx.h"
#include "vepu5xx_common.h"
#include "vepu541_common.h"
#include "vepu510_common.h"

#define DUMP_REG                0
#define MAX_TASK_CNT            2
#define VEPU540C_MAX_ROI_NUM    8

/* Custom Quant Matrices: Joint Video Team */
static RK_U8 vepu510_h264_cqm_jvt8i[64] = {
    6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42
};

static RK_U8 vepu510_h264_cqm_jvt8p[64] = {
    9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35
};

typedef struct Vepu510RoiH264BsCfg_t {
    RK_U64 force_inter   : 42;
    RK_U64 mode_mask     : 9;
    RK_U64 reserved      : 10;
    RK_U64 force_intra   : 1;
    RK_U64 qp_adj_en     : 1;
    RK_U64 amv_en        : 1;
} Vepu510RoiH264BsCfg;

typedef struct Vepu510H264Fbk_t {
    RK_U32 hw_status;       /* 0:corret, 1:error */
    RK_U32 frame_type;
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
    RK_U32 st_smear_cnt[5];
} Vepu510H264Fbk;

typedef struct HalH264eVepu510Ctx_t {
    MppEncCfgSet            *cfg;

    MppDev                  dev;
    RK_S32                  frame_cnt;
    RK_U32                  task_cnt;

    /* buffers management */
    HalBufs                 hw_recn;
    RK_S32                  pixel_buf_fbc_hdr_size;
    RK_S32                  pixel_buf_fbc_bdy_size;
    RK_S32                  pixel_buf_size;
    RK_S32                  thumb_buf_size;
    RK_S32                  max_buf_cnt;
    MppDevRegOffCfgs        *offsets;

    /* external line buffer over 4K */
    MppBufferGroup          ext_line_buf_grp;
    MppBuffer               ext_line_bufs[MAX_TASK_CNT];
    RK_S32                  ext_line_buf_size;

    /* syntax for input from enc_impl */
    RK_U32                  updated;
    H264eSps                *sps;
    H264ePps                *pps;
    H264eDpb                *dpb;
    H264eFrmInfo            *frms;

    /* async encode TSVC info */
    H264eReorderInfo        *reorder;
    H264eMarkingInfo        *marking;

    /* syntax for output to enc_impl */
    EncRcTaskInfo           hal_rc_cfg;

    /* roi */
    void                    *roi_data;
    MppBufferGroup          roi_grp;
    MppBuffer               roi_base_cfg_buf;
    RK_S32                  roi_base_buf_size;

    /* two-pass deflicker */
    MppBuffer               buf_pass1;

    /* register */
    HalVepu510RegSet        *regs_sets;
    HalH264eVepuStreamAmend *amend_sets;

    H264ePrefixNal          *prefix_sets;
    H264eSlice              *slice_sets;

    /* frame parallel info */
    RK_S32                  task_idx;
    RK_S32                  curr_idx;
    RK_S32                  prev_idx;
    HalVepu510RegSet        *regs_set;
    HalH264eVepuStreamAmend *amend;
    H264ePrefixNal          *prefix;
    H264eSlice              *slice;

    MppBuffer               ext_line_buf;

    /* slice low delay output callback */
    MppCbCtx                *output_cb;
    RK_S32                  poll_slice_max;
    RK_S32                  poll_cfg_size;
    MppDevPollCfg           *poll_cfgs;

    Vepu510H264Fbk          feedback;
    Vepu510H264Fbk          last_frame_fb;

    void                    *tune;
    RK_S32                  smart_en;
    RK_S32                  qpmap_en;
} HalH264eVepu510Ctx;

#include "hal_h264e_vepu510_tune.c"

static RK_S32 h264_aq_tthd_default[16] = {
    0,  0,  0,  0,  3,  3,  5,  5,
    8,  8,  8, 15, 15, 20, 25, 25
};

static RK_S32 h264_P_aq_step_default[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  5,  7,  8
};

static RK_S32 h264_I_aq_step_default[16] = {
    -8, -7, -6, -5, -4, -3, -2, -1,
    0,  1,  2,  3,  4,  5,  7,  8
};

static void setup_ext_line_bufs(HalH264eVepu510Ctx *ctx)
{
    RK_U32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i])
            continue;

        mpp_buffer_get(ctx->ext_line_buf_grp, &ctx->ext_line_bufs[i],
                       ctx->ext_line_buf_size);
    }
}

static void clear_ext_line_bufs(HalH264eVepu510Ctx *ctx)
{
    RK_U32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i]) {
            mpp_buffer_put(ctx->ext_line_bufs[i]);
            ctx->ext_line_bufs[i] = NULL;
        }
    }
}

static MPP_RET hal_h264e_vepu510_deinit(void *hal)
{
    HalH264eVepu510Ctx *p = (HalH264eVepu510Ctx *)hal;
    RK_U32 i;

    hal_h264e_dbg_func("enter %p\n", p);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    clear_ext_line_bufs(p);

    if (p->amend_sets) {
        for (i = 0; i < p->task_cnt; i++)
            h264e_vepu_stream_amend_deinit(&p->amend_sets[i]);
    }

    MPP_FREE(p->regs_sets);
    MPP_FREE(p->amend_sets);
    MPP_FREE(p->prefix_sets);
    MPP_FREE(p->slice_sets);
    MPP_FREE(p->reorder);
    MPP_FREE(p->marking);
    MPP_FREE(p->poll_cfgs);

    if (p->ext_line_buf_grp) {
        mpp_buffer_group_put(p->ext_line_buf_grp);
        p->ext_line_buf_grp = NULL;
    }

    if (p->hw_recn) {
        hal_bufs_deinit(p->hw_recn);
        p->hw_recn = NULL;
    }

    if (p->roi_base_cfg_buf) {
        mpp_buffer_put(p->roi_base_cfg_buf);
        p->roi_base_cfg_buf = NULL;
        p->roi_base_buf_size = 0;
    }

    if (p->roi_grp) {
        mpp_buffer_group_put(p->roi_grp);
        p->roi_grp = NULL;
    }

    if (p->offsets) {
        mpp_dev_multi_offset_deinit(p->offsets);
        p->offsets = NULL;
    }

    if (p->buf_pass1) {
        mpp_buffer_put(p->buf_pass1);
        p->buf_pass1 = NULL;
    }

    if (p->tune) {
        vepu510_h264e_tune_deinit(p->tune);
        p->tune = NULL;
    }

    hal_h264e_dbg_func("leave %p\n", p);

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu510_init(void *hal, MppEncHalCfg *cfg)
{
    HalH264eVepu510Ctx *p = (HalH264eVepu510Ctx *)hal;
    MPP_RET ret = MPP_OK;
    RK_U32 i;

    hal_h264e_dbg_func("enter %p\n", p);

    p->cfg = cfg->cfg;

    /* update output to MppEnc */
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        goto DONE;
    }
    p->dev = cfg->dev;
    p->task_cnt = cfg->task_cnt;
    mpp_assert(p->task_cnt && p->task_cnt <= MAX_TASK_CNT);

    ret = hal_bufs_init(&p->hw_recn);
    if (ret) {
        mpp_err_f("init vepu buffer failed ret: %d\n", ret);
        goto DONE;
    }

    p->regs_sets = mpp_malloc(HalVepu510RegSet, p->task_cnt);
    if (NULL == p->regs_sets) {
        ret = MPP_ERR_MALLOC;
        mpp_err_f("init register buffer failed\n");
        goto DONE;
    }

    p->amend_sets = mpp_malloc(HalH264eVepuStreamAmend, p->task_cnt);
    if (NULL == p->amend_sets) {
        ret = MPP_ERR_MALLOC;
        mpp_err_f("init amend data failed\n");
        goto DONE;
    }

    if (p->task_cnt > 1) {
        p->prefix_sets = mpp_malloc(H264ePrefixNal, p->task_cnt);
        if (NULL == p->prefix_sets) {
            ret = MPP_ERR_MALLOC;
            mpp_err_f("init amend data failed\n");
            goto DONE;
        }

        p->slice_sets = mpp_malloc(H264eSlice, p->task_cnt);
        if (NULL == p->slice_sets) {
            ret = MPP_ERR_MALLOC;
            mpp_err_f("init amend data failed\n");
            goto DONE;
        }

        p->reorder = mpp_malloc(H264eReorderInfo, 1);
        if (NULL == p->reorder) {
            ret = MPP_ERR_MALLOC;
            mpp_err_f("init amend data failed\n");
            goto DONE;
        }

        p->marking = mpp_malloc(H264eMarkingInfo, 1);
        if (NULL == p->marking) {
            ret = MPP_ERR_MALLOC;
            mpp_err_f("init amend data failed\n");
            goto DONE;
        }
    }

    p->poll_slice_max = 8;
    p->poll_cfg_size = (sizeof(p->poll_cfgs) + sizeof(RK_S32) * p->poll_slice_max);
    p->poll_cfgs = mpp_malloc_size(MppDevPollCfg, p->poll_cfg_size * p->task_cnt);
    if (NULL == p->poll_cfgs) {
        ret = MPP_ERR_MALLOC;
        mpp_err_f("init poll cfg buffer failed\n");
        goto DONE;
    }

    {   /* setup default hardware config */
        MppEncHwCfg *hw = &cfg->cfg->hw;

        hw->qp_delta_row_i  = 1;
        hw->qp_delta_row    = 2;
        hw->extra_buf       = 1;
        hw->qbias_i         = 683;
        hw->qbias_p         = 341;
        hw->qbias_en        = 0;

        memcpy(hw->aq_thrd_i, h264_aq_tthd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, h264_aq_tthd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, h264_I_aq_step_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, h264_P_aq_step_default, sizeof(hw->aq_step_p));

        for (i = 0; i < MPP_ARRAY_ELEMS(hw->mode_bias); i++)
            hw->mode_bias[i] = 8;

        hw->skip_sad  = 8;
        hw->skip_bias = 8;
    }

    mpp_dev_multi_offset_init(&p->offsets, 24);
    p->output_cb = cfg->output_cb;
    cfg->cap_recn_out = 1;
    for (i = 0; i < p->task_cnt; i++)
        h264e_vepu_stream_amend_init(&p->amend_sets[i]);

    p->tune = vepu510_h264e_tune_init(p);

DONE:
    if (ret)
        hal_h264e_vepu510_deinit(hal);

    hal_h264e_dbg_func("leave %p\n", p);
    return ret;
}

/*
 * NOTE: recon / refer buffer is FBC data buffer.
 * And FBC data require extra 16 lines space for hardware io.
 */
static void setup_hal_bufs(HalH264eVepu510Ctx *ctx)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    RK_S32 alignment_w = 64;
    RK_S32 alignment_h = 16;
    RK_S32 aligned_w = MPP_ALIGN(prep->width,  alignment_w);
    RK_S32 aligned_h = MPP_ALIGN(prep->height, alignment_h) + 16;
    RK_S32 pixel_buf_fbc_hdr_size = MPP_ALIGN(aligned_w * aligned_h / 64, SZ_8K);
    RK_S32 pixel_buf_fbc_bdy_size = aligned_w * aligned_h * 3 / 2;
    RK_S32 pixel_buf_size = pixel_buf_fbc_hdr_size + pixel_buf_fbc_bdy_size;
    RK_S32 thumb_buf_size = MPP_ALIGN(aligned_w / 64 * aligned_h / 64 * 256, SZ_8K);
    RK_S32 old_max_cnt = ctx->max_buf_cnt;
    RK_S32 new_max_cnt = 4;
    MppEncRefCfg ref_cfg = cfg->ref_cfg;

    if (ref_cfg) {
        MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(ref_cfg);
        if (new_max_cnt < MPP_MAX(new_max_cnt, info->dpb_size + 1))
            new_max_cnt = MPP_MAX(new_max_cnt, info->dpb_size + 1);
    }

    if (aligned_w > SZ_4K) {
        RK_S32 ctu_w = (aligned_w + 63) / 64;
        RK_S32 ext_line_buf_size = ((ctu_w - 53) * 53 + 15) / 16 * 16 * 16;

        if (NULL == ctx->ext_line_buf_grp)
            mpp_buffer_group_get_internal(&ctx->ext_line_buf_grp, MPP_BUFFER_TYPE_ION);
        else if (ext_line_buf_size != ctx->ext_line_buf_size) {
            clear_ext_line_bufs(ctx);
            mpp_buffer_group_clear(ctx->ext_line_buf_grp);
        }

        mpp_assert(ctx->ext_line_buf_grp);

        ctx->ext_line_buf_size = ext_line_buf_size;
        setup_ext_line_bufs(ctx);
    } else {
        clear_ext_line_bufs(ctx);
        if (ctx->ext_line_buf_grp) {
            mpp_buffer_group_clear(ctx->ext_line_buf_grp);
            mpp_buffer_group_put(ctx->ext_line_buf_grp);
            ctx->ext_line_buf_grp = NULL;
        }
        ctx->ext_line_buf_size = 0;
    }

    if ((ctx->pixel_buf_fbc_hdr_size != pixel_buf_fbc_hdr_size) ||
        (ctx->pixel_buf_fbc_bdy_size != pixel_buf_fbc_bdy_size) ||
        (ctx->pixel_buf_size != pixel_buf_size) ||
        (ctx->thumb_buf_size != thumb_buf_size) ||
        (new_max_cnt > old_max_cnt)) {
        size_t sizes[3];

        hal_h264e_dbg_detail("frame size %d -> %d max count %d -> %d\n",
                             ctx->pixel_buf_size, pixel_buf_size,
                             old_max_cnt, new_max_cnt);

        /* pixel buffer */
        sizes[0] = pixel_buf_size;
        /* thumb buffer */
        sizes[1] = thumb_buf_size;
        /* smear buffer */
        sizes[2] = MPP_ALIGN(aligned_w / 64, 16) * MPP_ALIGN(aligned_h / 16, 16);
        new_max_cnt = MPP_MAX(new_max_cnt, old_max_cnt);

        hal_bufs_setup(ctx->hw_recn, new_max_cnt, MPP_ARRAY_ELEMS(sizes), sizes);

        ctx->pixel_buf_fbc_hdr_size = pixel_buf_fbc_hdr_size;
        ctx->pixel_buf_fbc_bdy_size = pixel_buf_fbc_bdy_size;
        ctx->pixel_buf_size = pixel_buf_size;
        ctx->thumb_buf_size = thumb_buf_size;
        ctx->max_buf_cnt = new_max_cnt;
    }
}

static MPP_RET hal_h264e_vepu510_prepare(void *hal)
{
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (prep->change & (MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT)) {
        RK_S32 i;

        // pre-alloc required buffers to reduce first frame delay
        setup_hal_bufs(ctx);
        for (i = 0; i < ctx->max_buf_cnt; i++)
            hal_bufs_get_buf(ctx->hw_recn, i);

        prep->change = 0;
    }

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static RK_U32 update_vepu510_syntax(HalH264eVepu510Ctx *ctx, MppSyntax *syntax)
{
    H264eSyntaxDesc *desc = syntax->data;
    RK_S32 syn_num = syntax->number;
    RK_U32 updated = 0;
    RK_S32 i;

    for (i = 0; i < syn_num; i++, desc++) {
        switch (desc->type) {
        case H264E_SYN_CFG : {
            hal_h264e_dbg_detail("update cfg");
            ctx->cfg = desc->p;
        } break;
        case H264E_SYN_SPS : {
            hal_h264e_dbg_detail("update sps");
            ctx->sps = desc->p;
        } break;
        case H264E_SYN_PPS : {
            hal_h264e_dbg_detail("update pps");
            ctx->pps = desc->p;
        } break;
        case H264E_SYN_DPB : {
            hal_h264e_dbg_detail("update dpb");
            ctx->dpb = desc->p;
        } break;
        case H264E_SYN_SLICE : {
            hal_h264e_dbg_detail("update slice");
            ctx->slice = desc->p;
        } break;
        case H264E_SYN_FRAME : {
            hal_h264e_dbg_detail("update frames");
            ctx->frms = desc->p;
        } break;
        case H264E_SYN_PREFIX : {
            hal_h264e_dbg_detail("update prefix nal");
            ctx->prefix = desc->p;
        } break;
        default : {
            mpp_log_f("invalid syntax type %d\n", desc->type);
        } break;
        }

        updated |= SYN_TYPE_FLAG(desc->type);
    }

    return updated;
}

static MPP_RET hal_h264e_vepu510_get_task(void *hal, HalEncTask *task)
{
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    MppEncCfgSet *cfg_set = ctx->cfg;
    MppEncRefCfgImpl *ref = (MppEncRefCfgImpl *)cfg_set->ref_cfg;
    MppEncH264HwCfg *hw_cfg = &cfg_set->codec.h264.hw_cfg;
    RK_U32 updated = update_vepu510_syntax(ctx, &task->syntax);
    EncFrmStatus *frm_status = &task->rc_task->frm;
    H264eFrmInfo *frms = ctx->frms;

    hal_h264e_dbg_func("enter %p\n", hal);

    ctx->smart_en = (ctx->cfg->rc.rc_mode == MPP_ENC_RC_MODE_SMTRC);
    ctx->qpmap_en = ctx->cfg->tune.deblur_en;

    if (updated & SYN_TYPE_FLAG(H264E_SYN_CFG))
        setup_hal_bufs(ctx);

    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(task->frame);
        mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void **)&ctx->roi_data);
    }

    if (!frm_status->reencode)
        ctx->last_frame_fb = ctx->feedback;

    if (ctx->dpb) {
        h264e_dpb_hal_start(ctx->dpb, frms->curr_idx);
        h264e_dpb_hal_start(ctx->dpb, frms->refr_idx);
    }

    task->flags.reg_idx = ctx->task_idx;
    task->flags.curr_idx = frms->curr_idx;
    task->flags.refr_idx = frms->refr_idx;
    task->part_first = 1;
    task->part_last = 0;

    ctx->ext_line_buf = ctx->ext_line_bufs[ctx->task_idx];
    ctx->regs_set = &ctx->regs_sets[ctx->task_idx];
    ctx->amend = &ctx->amend_sets[ctx->task_idx];

    /* if not VEPU1/2, update log2_max_frame_num_minus4 in hw_cfg */
    hw_cfg->hw_log2_max_frame_num_minus4 = ctx->sps->log2_max_frame_num_minus4;

    if (ctx->task_cnt > 1 && (ref->lt_cfg_cnt || ref->st_cfg_cnt > 1)) {
        H264ePrefixNal *prefix = &ctx->prefix_sets[ctx->task_idx];
        H264eSlice *slice = &ctx->slice_sets[ctx->task_idx];

        //store async encode TSVC info
        if (ctx->prefix)
            memcpy(prefix, ctx->prefix, sizeof(H264ePrefixNal));
        else
            prefix = NULL;

        if (ctx->slice) {
            memcpy(slice, ctx->slice, sizeof(H264eSlice));

            /*
             * Generally, reorder and marking are shared by dpb and slice.
             * However, async encoding TSVC will change reorder and marking in each task.
             * Therefore, malloc a special space for async encoding TSVC.
             */
            ctx->amend->reorder = ctx->reorder;
            ctx->amend->marking = ctx->marking;
        }

        h264e_vepu_stream_amend_config(ctx->amend, task->packet, ctx->cfg,
                                       slice, prefix);
    } else {
        h264e_vepu_stream_amend_config(ctx->amend, task->packet, ctx->cfg,
                                       ctx->slice, ctx->prefix);
    }

    if (ctx->task_cnt > 1)
        ctx->task_idx = !ctx->task_idx;

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static void setup_vepu510_normal(HalVepu510RegSet *regs)
{
    hal_h264e_dbg_func("enter\n");
    /* reg000 VERSION is read only */

    /* reg001 ENC_STRT */
    regs->reg_ctl.enc_strt.lkt_num           = 0;
    regs->reg_ctl.enc_strt.vepu_cmd          = 1;

    regs->reg_ctl.opt_strg.cke                = 1;
    regs->reg_ctl.opt_strg.resetn_hw_en       = 1;

    /* reg002 ENC_CLR */
    regs->reg_ctl.enc_clr.safe_clr           = 0;
    regs->reg_ctl.enc_clr.force_clr          = 0;

    /* reg004 INT_EN */
    regs->reg_ctl.int_en.enc_done_en        = 1;
    regs->reg_ctl.int_en.lkt_node_done_en   = 1;
    regs->reg_ctl.int_en.sclr_done_en       = 1;
    regs->reg_ctl.int_en.vslc_done_en       = 0;
    regs->reg_ctl.int_en.vbsf_oflw_en       = 1;
    regs->reg_ctl.int_en.vbuf_lens_en       = 1;
    regs->reg_ctl.int_en.enc_err_en         = 1;

    regs->reg_ctl.int_en.wdg_en             = 1;
    regs->reg_ctl.int_en.vsrc_err_en        = 1;
    regs->reg_ctl.int_en.wdg_en             = 1;
    regs->reg_ctl.int_en.lkt_err_int_en     = 1;
    regs->reg_ctl.int_en.lkt_err_stop_en    = 1;
    regs->reg_ctl.int_en.lkt_force_stop_en  = 1;
    regs->reg_ctl.int_en.jslc_done_en       = 1;
    regs->reg_ctl.int_en.jbsf_oflw_en       = 1;
    regs->reg_ctl.int_en.jbuf_lens_en       = 1;
    regs->reg_ctl.int_en.dvbm_err_en        = 0;

    /* reg005 INT_MSK */
    regs->reg_ctl.int_msk.enc_done_msk        = 0;
    regs->reg_ctl.int_msk.lkt_node_done_msk   = 0;
    regs->reg_ctl.int_msk.sclr_done_msk       = 0;
    regs->reg_ctl.int_msk.vslc_done_msk       = 0;
    regs->reg_ctl.int_msk.vbsf_oflw_msk       = 0;
    regs->reg_ctl.int_msk.vbuf_lens_msk       = 0;
    regs->reg_ctl.int_msk.enc_err_msk         = 0;
    regs->reg_ctl.int_msk.vsrc_err_msk        = 0;
    regs->reg_ctl.int_msk.wdg_msk             = 0;
    regs->reg_ctl.int_msk.lkt_err_int_msk     = 0;
    regs->reg_ctl.int_msk.lkt_err_stop_msk    = 0;
    regs->reg_ctl.int_msk.lkt_force_stop_msk  = 0;
    regs->reg_ctl.int_msk.jslc_done_msk       = 0;
    regs->reg_ctl.int_msk.jbsf_oflw_msk       = 0;
    regs->reg_ctl.int_msk.jbuf_lens_msk       = 0;
    regs->reg_ctl.int_msk.dvbm_err_msk        = 0;

    /* reg006 INT_CLR is not set */
    /* reg007 INT_STA is read only */
    /* reg008 ~ reg0011 gap */
    regs->reg_ctl.enc_wdg.vs_load_thd       = 0;

    /* reg015 DTRNS_MAP */
    regs->reg_ctl.dtrns_map.jpeg_bus_edin      = 0;
    regs->reg_ctl.dtrns_map.src_bus_edin       = 0;
    regs->reg_ctl.dtrns_map.meiw_bus_edin      = 0;
    regs->reg_ctl.dtrns_map.bsw_bus_edin       = 7;
    regs->reg_ctl.dtrns_map.lktw_bus_edin      = 0;
    regs->reg_ctl.dtrns_map.rec_nfbc_bus_edin  = 0;

    regs->reg_ctl.dtrns_cfg.axi_brsp_cke   = 0;

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET setup_vepu510_prep(HalVepu510RegSet *regs, MppEncPrepCfg *prep)
{
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    VepuFmtCfg cfg;
    MppFrameFormat fmt = prep->format;
    MPP_RET ret = vepu541_set_fmt(&cfg, fmt);
    RK_U32 hw_fmt = cfg.format;
    RK_S32 y_stride;
    RK_S32 c_stride;

    hal_h264e_dbg_func("enter\n");

    /* do nothing when color format is not supported */
    if (ret)
        return ret;

    reg_frm->common.enc_rsl.pic_wd8_m1 = MPP_ALIGN(prep->width, 16) / 8 - 1;
    reg_frm->common.src_fill.pic_wfill = MPP_ALIGN(prep->width, 16) - prep->width;
    reg_frm->common.enc_rsl.pic_hd8_m1 = MPP_ALIGN(prep->height, 16) / 8 - 1;
    reg_frm->common.src_fill.pic_hfill = MPP_ALIGN(prep->height, 16) - prep->height;

    regs->reg_ctl.dtrns_map.src_bus_edin = cfg.src_endian;

    reg_frm->common.src_fmt.src_cfmt   = hw_fmt;
    reg_frm->common.src_fmt.alpha_swap = cfg.alpha_swap;
    reg_frm->common.src_fmt.rbuv_swap  = cfg.rbuv_swap;
    reg_frm->common.src_fmt.out_fmt = ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV400 ? 0 : 1);

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        mpp_err("Unsupported FBC format input.\n");

        return MPP_ERR_VALUE;
    } else if (prep->hor_stride) {
        if (MPP_FRAME_FMT_IS_TILE(fmt)) {
            switch (fmt & MPP_FRAME_FMT_MASK) {
            case MPP_FMT_YUV400:
                y_stride = prep->hor_stride * 4;
                break;
            case MPP_FMT_YUV420P:
            case MPP_FMT_YUV420SP:
                y_stride = prep->hor_stride * 4 * 3 / 2;
                break;
            case MPP_FMT_YUV422P:
            case MPP_FMT_YUV422SP:
                y_stride = prep->hor_stride * 4 * 2;
                break;
            case MPP_FMT_YUV444P:
            case MPP_FMT_YUV444SP:
                y_stride = prep->hor_stride * 4 * 3;
                break;
            default:
                mpp_err("Unsupported input format 0x%08x, with TILE mask.\n", fmt);
                return MPP_ERR_VALUE;
                break;
            }
        } else {
            y_stride = prep->hor_stride;
        }
    } else {
        if (hw_fmt == VEPU541_FMT_BGRA8888 )
            y_stride = prep->width * 4;
        else if (hw_fmt == VEPU541_FMT_BGR888 )
            y_stride = prep->width * 3;
        else if (hw_fmt == VEPU541_FMT_BGR565 ||
                 hw_fmt == VEPU541_FMT_YUYV422 ||
                 hw_fmt == VEPU541_FMT_UYVY422)
            y_stride = prep->width * 2;
        else
            y_stride = prep->width;
    }

    switch (hw_fmt) {
    case VEPU580_FMT_YUV444SP : {
        c_stride = y_stride * 2;
    } break;
    case VEPU541_FMT_YUV422SP :
    case VEPU541_FMT_YUV420SP :
    case VEPU580_FMT_YUV444P : {
        c_stride = y_stride;
    } break;
    default : {
        c_stride = y_stride / 2;
    } break;
    }

    if (hw_fmt < VEPU541_FMT_NONE) {
        const VepuRgb2YuvCfg *cfg_coeffs = get_rgb2yuv_cfg(prep->range, prep->color);

        hal_h264e_dbg_flow("input color range %d colorspace %d", prep->range, prep->color);

        reg_frm->common.src_udfy.csc_wgt_b2y = cfg_coeffs->_2y.b_coeff;
        reg_frm->common.src_udfy.csc_wgt_g2y = cfg_coeffs->_2y.g_coeff;
        reg_frm->common.src_udfy.csc_wgt_r2y = cfg_coeffs->_2y.r_coeff;

        reg_frm->common.src_udfu.csc_wgt_b2u = cfg_coeffs->_2u.b_coeff;
        reg_frm->common.src_udfu.csc_wgt_g2u = cfg_coeffs->_2u.g_coeff;
        reg_frm->common.src_udfu.csc_wgt_r2u = cfg_coeffs->_2u.r_coeff;

        reg_frm->common.src_udfv.csc_wgt_b2v = cfg_coeffs->_2v.b_coeff;
        reg_frm->common.src_udfv.csc_wgt_g2v = cfg_coeffs->_2v.g_coeff;
        reg_frm->common.src_udfv.csc_wgt_r2v = cfg_coeffs->_2v.r_coeff;

        reg_frm->common.src_udfo.csc_ofst_y  = cfg_coeffs->_2y.offset;
        reg_frm->common.src_udfo.csc_ofst_u  = cfg_coeffs->_2u.offset;
        reg_frm->common.src_udfo.csc_ofst_v  = cfg_coeffs->_2v.offset;

        hal_h264e_dbg_flow("use color range %d colorspace %d", cfg_coeffs->dst_range, cfg_coeffs->color);
    } else {
        reg_frm->common.src_udfy.csc_wgt_b2y = cfg.weight[0];
        reg_frm->common.src_udfy.csc_wgt_g2y = cfg.weight[1];
        reg_frm->common.src_udfy.csc_wgt_r2y = cfg.weight[2];

        reg_frm->common.src_udfu.csc_wgt_b2u = cfg.weight[3];
        reg_frm->common.src_udfu.csc_wgt_g2u = cfg.weight[4];
        reg_frm->common.src_udfu.csc_wgt_r2u = cfg.weight[5];

        reg_frm->common.src_udfv.csc_wgt_b2v = cfg.weight[6];
        reg_frm->common.src_udfv.csc_wgt_g2v = cfg.weight[7];
        reg_frm->common.src_udfv.csc_wgt_r2v = cfg.weight[8];

        reg_frm->common.src_udfo.csc_ofst_y  = cfg.offset[0];
        reg_frm->common.src_udfo.csc_ofst_u  = cfg.offset[1];
        reg_frm->common.src_udfo.csc_ofst_v  = cfg.offset[2];
    }

    reg_frm->common.src_strd0.src_strd0  = y_stride;
    reg_frm->common.src_strd1.src_strd1  = c_stride;

    reg_frm->common.src_proc.src_mirr    = prep->mirroring > 0;
    reg_frm->common.src_proc.src_rot     = prep->rotation;

    if (MPP_FRAME_FMT_IS_TILE(fmt))
        reg_frm->common.src_proc.tile4x4_en = 1;
    else
        reg_frm->common.src_proc.tile4x4_en = 0;

    reg_frm->sli_cfg.mv_v_lmt_thd = 0;
    reg_frm->sli_cfg.mv_v_lmt_en  = 0;

    reg_frm->common.pic_ofst.pic_ofst_y  = 0;
    reg_frm->common.pic_ofst.pic_ofst_x  = 0;

    hal_h264e_dbg_func("leave\n");

    return ret;
}

static MPP_RET vepu510_h264e_save_pass1_patch(HalVepu510RegSet *regs, HalH264eVepu510Ctx *ctx)
{
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_S32 width_align = MPP_ALIGN(ctx->cfg->prep.width, 16);
    RK_S32 height_align = MPP_ALIGN(ctx->cfg->prep.height, 16);

    if (NULL == ctx->buf_pass1) {
        mpp_buffer_get(NULL, &ctx->buf_pass1, width_align * height_align * 3 / 2);
        if (!ctx->buf_pass1) {
            mpp_err("buf_pass1 malloc fail, debreath invaild");
            return MPP_NOK;
        }
    }

    reg_frm->common.enc_pic.cur_frm_ref = 1;
    reg_frm->common.rfpw_h_addr = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_frm->common.rfpw_b_addr = reg_frm->common.rfpw_h_addr;
    reg_frm->common.enc_pic.rec_fbc_dis = 1;

    mpp_dev_multi_offset_update(ctx->offsets, 164, 0);

    /* NOTE: disable split to avoid lowdelay slice output */
    reg_frm->common.sli_splt.sli_splt = 0;
    reg_frm->common.enc_pic.slen_fifo = 0;

    return MPP_OK;
}

static MPP_RET vepu510_h264e_use_pass1_patch(HalVepu510RegSet *regs, HalH264eVepu510Ctx *ctx)
{
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_S32 fd_in = mpp_buffer_get_fd(ctx->buf_pass1);
    RK_S32 y_stride;
    RK_S32 c_stride;

    hal_h264e_dbg_func("enter\n");

    reg_frm->common.src_fmt.src_cfmt   = VEPU541_FMT_YUV420SP;
    reg_frm->common.src_fmt.alpha_swap = 0;
    reg_frm->common.src_fmt.rbuv_swap  = 0;
    reg_frm->common.src_fmt.out_fmt    = 1;
    reg_frm->common.src_fmt.src_rcne   = 1;
    y_stride = MPP_ALIGN(prep->width, 16);
    c_stride = y_stride;

    reg_frm->common.src_strd0.src_strd0 = y_stride;
    reg_frm->common.src_strd1.src_strd1 = 3 * c_stride;

    reg_frm->common.src_proc.src_mirr   = 0;
    reg_frm->common.src_proc.src_rot    = 0;

    reg_frm->common.pic_ofst.pic_ofst_y = 0;
    reg_frm->common.pic_ofst.pic_ofst_x = 0;


    reg_frm->common.adr_src0   = fd_in;
    reg_frm->common.adr_src1   = fd_in;
    reg_frm->common.adr_src2   = fd_in;

    mpp_dev_multi_offset_update(ctx->offsets, 161, 2 * y_stride);

    hal_h264e_dbg_func("leave\n");
    return MPP_OK;
}

static void setup_vepu510_codec(HalVepu510RegSet *regs, H264eSps *sps,
                                H264ePps *pps, H264eSlice *slice)
{
    H264eVepu510Frame *reg_frm = &regs->reg_frm;

    hal_h264e_dbg_func("enter\n");

    reg_frm->common.enc_pic.enc_stnd       = 0;
    reg_frm->common.enc_pic.cur_frm_ref    = slice->nal_reference_idc > 0;
    reg_frm->common.enc_pic.bs_scp         = 1;

    reg_frm->synt_nal.nal_ref_idc    = slice->nal_reference_idc;
    reg_frm->synt_nal.nal_unit_type  = slice->nalu_type;

    reg_frm->synt_sps.max_fnum       = sps->log2_max_frame_num_minus4;
    reg_frm->synt_sps.drct_8x8       = sps->direct8x8_inference;
    reg_frm->synt_sps.mpoc_lm4       = sps->log2_max_poc_lsb_minus4;

    reg_frm->synt_pps.etpy_mode      = pps->entropy_coding_mode;
    reg_frm->synt_pps.trns_8x8       = pps->transform_8x8_mode;
    reg_frm->synt_pps.csip_flag      = pps->constrained_intra_pred;
    reg_frm->synt_pps.num_ref0_idx   = pps->num_ref_idx_l0_default_active - 1;
    reg_frm->synt_pps.num_ref1_idx   = pps->num_ref_idx_l1_default_active - 1;
    reg_frm->synt_pps.pic_init_qp    = pps->pic_init_qp;
    reg_frm->synt_pps.cb_ofst        = pps->chroma_qp_index_offset;
    reg_frm->synt_pps.cr_ofst        = pps->second_chroma_qp_index_offset;
    reg_frm->synt_pps.dbf_cp_flg     = pps->deblocking_filter_control;

    reg_frm->synt_sli0.sli_type       = (slice->slice_type == H264_I_SLICE) ? (2) : (0);
    reg_frm->synt_sli0.pps_id         = slice->pic_parameter_set_id;
    reg_frm->synt_sli0.drct_smvp      = 0;
    reg_frm->synt_sli0.num_ref_ovrd   = slice->num_ref_idx_override;
    reg_frm->synt_sli0.cbc_init_idc   = slice->cabac_init_idc;
    reg_frm->synt_sli0.frm_num        = slice->frame_num;

    reg_frm->synt_sli1.idr_pid        = (slice->slice_type == H264_I_SLICE) ? slice->idr_pic_id : (RK_U32)(-1);
    reg_frm->synt_sli1.poc_lsb        = slice->pic_order_cnt_lsb;


    reg_frm->synt_sli2.dis_dblk_idc   = slice->disable_deblocking_filter_idc;
    reg_frm->synt_sli2.sli_alph_ofst  = slice->slice_alpha_c0_offset_div2;

    h264e_reorder_rd_rewind(slice->reorder);
    {   /* reorder process */
        H264eRplmo rplmo;
        MPP_RET ret = h264e_reorder_rd_op(slice->reorder, &rplmo);

        if (MPP_OK == ret) {
            reg_frm->synt_sli2.ref_list0_rodr = 1;
            reg_frm->synt_sli2.rodr_pic_idx   = rplmo.modification_of_pic_nums_idc;

            switch (rplmo.modification_of_pic_nums_idc) {
            case 0 :
            case 1 : {
                reg_frm->synt_sli2.rodr_pic_num   = rplmo.abs_diff_pic_num_minus1;
            } break;
            case 2 : {
                reg_frm->synt_sli2.rodr_pic_num   = rplmo.long_term_pic_idx;
            } break;
            default : {
                mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                          rplmo.modification_of_pic_nums_idc);
            } break;
            }
        } else {
            // slice->ref_pic_list_modification_flag;
            reg_frm->synt_sli2.ref_list0_rodr = 0;
            reg_frm->synt_sli2.rodr_pic_idx   = 0;
            reg_frm->synt_sli2.rodr_pic_num   = 0;
        }
    }

    /* clear all mmco arg first */
    reg_frm->synt_refm0.nopp_flg               = 0;
    reg_frm->synt_refm0.ltrf_flg               = 0;
    reg_frm->synt_refm0.arpm_flg               = 0;
    reg_frm->synt_refm0.mmco4_pre              = 0;
    reg_frm->synt_refm0.mmco_type0             = 0;
    reg_frm->synt_refm0.mmco_parm0             = 0;
    reg_frm->synt_refm0.mmco_type1             = 0;
    reg_frm->synt_refm1.mmco_parm1             = 0;
    reg_frm->synt_refm0.mmco_type2             = 0;
    reg_frm->synt_refm1.mmco_parm2             = 0;
    reg_frm->synt_refm2.long_term_frame_idx0   = 0;
    reg_frm->synt_refm2.long_term_frame_idx1   = 0;
    reg_frm->synt_refm2.long_term_frame_idx2   = 0;

    h264e_marking_rd_rewind(slice->marking);

    /* only update used parameter */
    if (slice->slice_type == H264_I_SLICE) {
        reg_frm->synt_refm0.nopp_flg       = slice->no_output_of_prior_pics;
        reg_frm->synt_refm0.ltrf_flg       = slice->long_term_reference_flag;
    } else {
        if (!h264e_marking_is_empty(slice->marking)) {
            H264eMmco mmco;

            reg_frm->synt_refm0.arpm_flg       = 1;

            /* max 3 mmco */
            do {
                RK_S32 type = 0;
                RK_S32 param_0 = 0;
                RK_S32 param_1 = 0;

                h264e_marking_rd_op(slice->marking, &mmco);
                type = mmco.mmco;
                switch (type) {
                case 1 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                } break;
                case 2 : {
                    param_0 = mmco.long_term_pic_num;
                } break;
                case 3 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                    param_1 = mmco.long_term_frame_idx;
                } break;
                case 4 : {
                    param_0 = mmco.max_long_term_frame_idx_plus1;
                } break;
                case 5 : {
                } break;
                case 6 : {
                    param_0 = mmco.long_term_frame_idx;
                } break;
                default : {
                    mpp_err_f("unsupported mmco 0 %d\n", type);
                    type = 0;
                } break;
                }

                reg_frm->synt_refm0.mmco_type0 = type;
                reg_frm->synt_refm0.mmco_parm0 = param_0;
                reg_frm->synt_refm2.long_term_frame_idx0 = param_1;

                if (h264e_marking_is_empty(slice->marking))
                    break;

                h264e_marking_rd_op(slice->marking, &mmco);
                type = mmco.mmco;
                param_0 = 0;
                param_1 = 0;
                switch (type) {
                case 1 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                } break;
                case 2 : {
                    param_0 = mmco.long_term_pic_num;
                } break;
                case 3 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                    param_1 = mmco.long_term_frame_idx;
                } break;
                case 4 : {
                    param_0 = mmco.max_long_term_frame_idx_plus1;
                } break;
                case 5 : {
                } break;
                case 6 : {
                    param_0 = mmco.long_term_frame_idx;
                } break;
                default : {
                    mpp_err_f("unsupported mmco 0 %d\n", type);
                    type = 0;
                } break;
                }

                reg_frm->synt_refm0.mmco_type1 = type;
                reg_frm->synt_refm1.mmco_parm1 = param_0;
                reg_frm->synt_refm2.long_term_frame_idx1 = param_1;

                if (h264e_marking_is_empty(slice->marking))
                    break;

                h264e_marking_rd_op(slice->marking, &mmco);
                type = mmco.mmco;
                param_0 = 0;
                param_1 = 0;
                switch (type) {
                case 1 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                } break;
                case 2 : {
                    param_0 = mmco.long_term_pic_num;
                } break;
                case 3 : {
                    param_0 = mmco.difference_of_pic_nums_minus1;
                    param_1 = mmco.long_term_frame_idx;
                } break;
                case 4 : {
                    param_0 = mmco.max_long_term_frame_idx_plus1;
                } break;
                case 5 : {
                } break;
                case 6 : {
                    param_0 = mmco.long_term_frame_idx;
                } break;
                default : {
                    mpp_err_f("unsupported mmco 0 %d\n", type);
                    type = 0;
                } break;
                }

                reg_frm->synt_refm0.mmco_type2 = type;
                reg_frm->synt_refm1.mmco_parm2 = param_0;
                reg_frm->synt_refm2.long_term_frame_idx2 = param_1;
            } while (0);
        }
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_rdo_pred(HalH264eVepu510Ctx *ctx, H264eSps *sps,
                                   H264ePps *pps, H264eSlice *slice)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_U32 is_ipc_scene = (ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC);

    hal_h264e_dbg_func("enter\n");

    if (slice->slice_type == H264_I_SLICE) {
        regs->reg_rc_roi.klut_ofst.chrm_klut_ofst = 6;
    } else {
        regs->reg_rc_roi.klut_ofst.chrm_klut_ofst = is_ipc_scene ? 9 : 6;
    }

    reg_frm->rdo_cfg.rect_size      = (sps->profile_idc == H264_PROFILE_BASELINE &&
                                       sps->level_idc <= H264_LEVEL_3_0) ? 1 : 0;
    reg_frm->rdo_cfg.vlc_lmt        = (sps->profile_idc < H264_PROFILE_MAIN) &&
                                      !pps->entropy_coding_mode;
    reg_frm->rdo_cfg.chrm_spcl      = 1;
    reg_frm->rdo_cfg.ccwa_e         = 1;
    reg_frm->rdo_cfg.scl_lst_sel    = pps->pic_scaling_matrix_present;
    reg_frm->rdo_cfg.atf_e          = ctx->cfg->tune.anti_flicker_str > 0;
    reg_frm->rdo_cfg.atr_e          = ctx->cfg->tune.atr_str_i > 0;
    reg_frm->rdo_cfg.atr_mult_sel_e = 1;
    reg_frm->iprd_csts.rdo_mark_mode = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_rc_base(HalVepu510RegSet *regs, HalH264eVepu510Ctx *ctx, EncRcTask *rc_task)
{
    H264eSps *sps = ctx->sps;
    H264eSlice *slice = ctx->slice;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncHwCfg *hw = &cfg->hw;
    EncRcTaskInfo *rc_info = &rc_task->info;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_S32 mb_w = sps->pic_width_in_mbs;
    RK_S32 mb_h = sps->pic_height_in_mbs;
    RK_U32 qp_target = rc_info->quality_target;
    RK_U32 qp_min = rc_info->quality_min;
    RK_U32 qp_max = rc_info->quality_max;
    RK_U32 qpmap_mode = 1;
    RK_S32 mb_target_bits_mul_16 = (rc_info->bit_target << 4) / (mb_w * mb_h);
    RK_S32 mb_target_bits;
    RK_S32 negative_bits_thd;
    RK_S32 positive_bits_thd;

    hal_h264e_dbg_rc("bittarget %d qp [%d %d %d]\n", rc_info->bit_target,
                     qp_min, qp_target, qp_max);

    hal_h264e_dbg_func("enter\n");

    regs->reg_rc_roi.roi_qthd0.qpmin_area0    = qp_min;
    regs->reg_rc_roi.roi_qthd0.qpmax_area0    = qp_max;
    regs->reg_rc_roi.roi_qthd0.qpmin_area1    = qp_min;
    regs->reg_rc_roi.roi_qthd0.qpmax_area1    = qp_max;
    regs->reg_rc_roi.roi_qthd0.qpmin_area2    = qp_min;

    regs->reg_rc_roi.roi_qthd1.qpmax_area2    = qp_max;
    regs->reg_rc_roi.roi_qthd1.qpmin_area3    = qp_min;
    regs->reg_rc_roi.roi_qthd1.qpmax_area3    = qp_max;
    regs->reg_rc_roi.roi_qthd1.qpmin_area4    = qp_min;
    regs->reg_rc_roi.roi_qthd1.qpmax_area4    = qp_max;

    regs->reg_rc_roi.roi_qthd2.qpmin_area5    = qp_min;
    regs->reg_rc_roi.roi_qthd2.qpmax_area5    = qp_max;
    regs->reg_rc_roi.roi_qthd2.qpmin_area6    = qp_min;
    regs->reg_rc_roi.roi_qthd2.qpmax_area6    = qp_max;
    regs->reg_rc_roi.roi_qthd2.qpmin_area7    = qp_min;

    regs->reg_rc_roi.roi_qthd3.qpmax_area7    = qp_max;
    regs->reg_rc_roi.roi_qthd3.qpmap_mode     = qpmap_mode;

    if (rc->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        reg_frm->common.enc_pic.pic_qp    = rc_info->quality_target;
        reg_frm->common.rc_qp.rc_max_qp   = rc_info->quality_target;
        reg_frm->common.rc_qp.rc_min_qp   = rc_info->quality_target;

        return;
    }

    if (mb_target_bits_mul_16 >= 0x100000)
        mb_target_bits_mul_16 = 0x50000;

    mb_target_bits = (mb_target_bits_mul_16 * mb_w) >> 4;
    negative_bits_thd = 0 - 5 * mb_target_bits / 16;
    positive_bits_thd = 5 * mb_target_bits / 16;

    reg_frm->common.enc_pic.pic_qp         = qp_target;

    reg_frm->common.rc_cfg.rc_en          = 1;
    reg_frm->common.rc_cfg.aq_en          = 1;
    reg_frm->common.rc_cfg.rc_ctu_num     = mb_w;

    reg_frm->common.rc_qp.rc_max_qp       = qp_max;
    reg_frm->common.rc_qp.rc_min_qp       = qp_min;
    reg_frm->common.rc_tgt.ctu_ebit       = mb_target_bits_mul_16;

    if (rc->rc_mode == MPP_ENC_RC_MODE_SMTRC) {
        reg_frm->common.rc_qp.rc_qp_range = 0;
    } else {
        reg_frm->common.rc_qp.rc_qp_range = (slice->slice_type == H264_I_SLICE) ?
                                            hw->qp_delta_row_i : hw->qp_delta_row;
    }

    {
        /* fixed frame level QP */
        RK_S32 fqp_min, fqp_max;

        if (slice->slice_type == H264_I_SLICE) {
            fqp_min = rc->fqp_min_i;
            fqp_max = rc->fqp_max_i;
        } else {
            fqp_min = rc->fqp_min_p;
            fqp_max = rc->fqp_max_p;
        }

        if ((fqp_min == fqp_max) && (fqp_min >= 1) && (fqp_max <= 51)) {
            reg_frm->common.enc_pic.pic_qp = fqp_min;
            reg_frm->common.rc_qp.rc_qp_range = 0;
        }
    }

    regs->reg_rc_roi.rc_adj0.qp_adj0        = -2;
    regs->reg_rc_roi.rc_adj0.qp_adj1        = -1;
    regs->reg_rc_roi.rc_adj0.qp_adj2        = 0;
    regs->reg_rc_roi.rc_adj0.qp_adj3        = 1;
    regs->reg_rc_roi.rc_adj0.qp_adj4        = 2;
    regs->reg_rc_roi.rc_adj1.qp_adj5        = 0;
    regs->reg_rc_roi.rc_adj1.qp_adj6        = 0;
    regs->reg_rc_roi.rc_adj1.qp_adj7        = 0;
    regs->reg_rc_roi.rc_adj1.qp_adj8        = 0;

    regs->reg_rc_roi.rc_dthd_0_8[0] = 4 * negative_bits_thd;
    regs->reg_rc_roi.rc_dthd_0_8[1] = negative_bits_thd;
    regs->reg_rc_roi.rc_dthd_0_8[2] = positive_bits_thd;
    regs->reg_rc_roi.rc_dthd_0_8[3] = 4 * positive_bits_thd;
    regs->reg_rc_roi.rc_dthd_0_8[4] = 0x7FFFFFFF;
    regs->reg_rc_roi.rc_dthd_0_8[5] = 0x7FFFFFFF;
    regs->reg_rc_roi.rc_dthd_0_8[6] = 0x7FFFFFFF;
    regs->reg_rc_roi.rc_dthd_0_8[7] = 0x7FFFFFFF;
    regs->reg_rc_roi.rc_dthd_0_8[8] = 0x7FFFFFFF;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_io_buf(HalVepu510RegSet *regs, MppDevRegOffCfgs *offsets,
                                 HalEncTask *task)
{
    MppFrame frm = task->frame;
    MppPacket pkt = task->packet;
    MppBuffer buf_in = mpp_frame_get_buffer(frm);
    MppBuffer buf_out = task->output;
    MppFrameFormat fmt = mpp_frame_get_fmt(frm);
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_S32 hor_stride = mpp_frame_get_hor_stride(frm);
    RK_S32 ver_stride = mpp_frame_get_ver_stride(frm);
    RK_S32 fd_in = mpp_buffer_get_fd(buf_in);
    RK_U32 off_in[2] = {0};
    RK_U32 off_out = mpp_packet_get_length(pkt);
    size_t siz_out = mpp_buffer_get_size(buf_out);
    RK_S32 fd_out = mpp_buffer_get_fd(buf_out);

    hal_h264e_dbg_func("enter\n");

    reg_frm->common.adr_src0   = fd_in;
    reg_frm->common.adr_src1   = fd_in;
    reg_frm->common.adr_src2   = fd_in;

    reg_frm->common.bsbt_addr  = fd_out;
    reg_frm->common.bsbb_addr  = fd_out;
    reg_frm->common.adr_bsbs   = fd_out;
    reg_frm->common.bsbr_addr  = fd_out;

    reg_frm->common.rfpt_h_addr = 0xffffffff;
    reg_frm->common.rfpb_h_addr = 0;
    reg_frm->common.rfpt_b_addr = 0xffffffff;
    reg_frm->common.adr_rfpb_b  = 0;

    if (MPP_FRAME_FMT_IS_YUV(fmt)) {
        VepuFmtCfg cfg;

        vepu541_set_fmt(&cfg, fmt);
        switch (cfg.format) {
        case VEPU541_FMT_BGRA8888 :
        case VEPU541_FMT_BGR888 :
        case VEPU541_FMT_BGR565 : {
            off_in[0] = 0;
            off_in[1] = 0;
        } break;
        case VEPU541_FMT_YUV420SP :
        case VEPU541_FMT_YUV422SP : {
            off_in[0] = hor_stride * ver_stride;
            off_in[1] = hor_stride * ver_stride;
        } break;
        case VEPU541_FMT_YUV422P : {
            off_in[0] = hor_stride * ver_stride;
            off_in[1] = hor_stride * ver_stride * 3 / 2;
        } break;
        case VEPU541_FMT_YUV420P : {
            off_in[0] = hor_stride * ver_stride;
            off_in[1] = hor_stride * ver_stride * 5 / 4;
        } break;
        case VEPU540_FMT_YUV400 :
        case VEPU541_FMT_YUYV422 :
        case VEPU541_FMT_UYVY422 : {
            off_in[0] = 0;
            off_in[1] = 0;
        } break;
        case VEPU580_FMT_YUV444SP : {
            off_in[0] = hor_stride * ver_stride;
            off_in[1] = hor_stride * ver_stride;
        } break;
        case VEPU580_FMT_YUV444P : {
            off_in[0] = hor_stride * ver_stride;
            off_in[1] = hor_stride * ver_stride * 2;
        } break;
        case VEPU541_FMT_NONE :
        default : {
            off_in[0] = 0;
            off_in[1] = 0;
        } break;
        }
    }

    mpp_dev_multi_offset_update(offsets, 161, off_in[0]);
    mpp_dev_multi_offset_update(offsets, 162, off_in[1]);
    mpp_dev_multi_offset_update(offsets, 172, siz_out);
    mpp_dev_multi_offset_update(offsets, 174, off_out);

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET vepu510_h264_set_one_roi(void *buf, MppEncROIRegion *region, RK_S32 w, RK_S32 h)
{
    Vepu510RoiH264BsCfg *ptr = (Vepu510RoiH264BsCfg *)buf;
    RK_S32 mb_w = MPP_ALIGN(w, 16) / 16;
    RK_S32 mb_h = MPP_ALIGN(h, 16) / 16;
    RK_S32 stride_h = MPP_ALIGN(mb_w, 4);
    Vepu510RoiH264BsCfg cfg;
    MPP_RET ret = MPP_NOK;

    if (NULL == buf || NULL == region) {
        mpp_err_f("invalid buf %p roi %p\n", buf, region);
        goto DONE;
    }

    RK_S32 roi_width  = (region->w + 15) / 16;
    RK_S32 roi_height = (region->h + 15) / 16;
    RK_S32 pos_x_init = region->x / 16;
    RK_S32 pos_y_init = region->y / 16;
    RK_S32 pos_x_end  = pos_x_init + roi_width;
    RK_S32 pos_y_end  = pos_y_init + roi_height;
    RK_S32 x, y;

    pos_x_end = MPP_MIN(pos_x_end, mb_w);
    pos_y_end = MPP_MIN(pos_y_end, mb_h);
    pos_x_init = MPP_MAX(pos_x_init, 0);
    pos_y_init = MPP_MAX(pos_y_init, 0);

    mpp_assert(pos_x_end > pos_x_init);
    mpp_assert(pos_y_end > pos_y_init);

    cfg.force_intra = 1;

    ptr += pos_y_init * stride_h + pos_x_init;
    roi_width = pos_x_end - pos_x_init;
    roi_height = pos_y_end - pos_y_init;

    for (y = 0; y < roi_height; y++) {
        Vepu510RoiH264BsCfg *dst = ptr;

        for (x = 0; x < roi_width; x++, dst++)
            memcpy(dst, &cfg, sizeof(cfg));

        ptr += stride_h;
    }
DONE:
    return ret;
}

static MPP_RET setup_vepu510_intra_refresh(HalVepu510RegSet *regs, HalH264eVepu510Ctx *ctx, RK_U32 refresh_idx)
{
    MPP_RET ret = MPP_OK;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 w = mb_w * 16;
    RK_U32 h = mb_h * 16;
    MppEncROIRegion *region = NULL;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    RK_U32 refresh_num = ctx->cfg->rc.refresh_num;
    RK_U32 stride_h = MPP_ALIGN(mb_w, 4);
    RK_U32 stride_v = MPP_ALIGN(mb_h, 4);
    RK_U32 roi_base_buf_size = stride_h * stride_v * 8;
    RK_U32 i = 0;

    hal_h264e_dbg_func("enter\n");

    if (!ctx->cfg->rc.refresh_en) {
        ret = MPP_ERR_VALUE;
        goto RET;
    }

    if (NULL == ctx->roi_base_cfg_buf) {
        if (NULL == ctx->roi_grp)
            mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);
        mpp_buffer_get(ctx->roi_grp, &ctx->roi_base_cfg_buf, roi_base_buf_size);
        ctx->roi_base_buf_size = roi_base_buf_size;
    }

    mpp_assert(ctx->roi_base_cfg_buf);
    void *base_cfg_buf = mpp_buffer_get_ptr(ctx->roi_base_cfg_buf);
    Vepu510RoiH264BsCfg base_cfg;
    Vepu510RoiH264BsCfg *base_cfg_ptr = (Vepu510RoiH264BsCfg *)base_cfg_buf;

    base_cfg.force_intra = 0;
    base_cfg.qp_adj_en   = 1;

    for (i = 0; i < stride_h * stride_v; i++, base_cfg_ptr++)
        memcpy(base_cfg_ptr, &base_cfg, sizeof(base_cfg));

    region = mpp_calloc(MppEncROIRegion, 1);

    if (NULL == region) {
        mpp_err_f("Failed to calloc for MppEncROIRegion !\n");
        ret = MPP_ERR_MALLOC;
    }

    if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_ROW) {
        region->x = 0;
        region->w = w;
        if (refresh_idx > 0) {
            region->y = refresh_idx * 16 * refresh_num - 32;
            region->h = 16 * refresh_num + 32;
        } else {
            region->y = refresh_idx * 16 * refresh_num;
            region->h = 16 * refresh_num;
        }
        reg_frm->common.me_rnge.cime_srch_uph = 1;
    } else if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_COL) {
        region->y = 0;
        region->h = h;
        if (refresh_idx > 0) {
            region->x = refresh_idx * 16 * refresh_num - 32;
            region->w = 16 * refresh_num + 32;
        } else {
            region->x = refresh_idx * 16 * refresh_num;
            region->w = 16 * refresh_num;
        }
        reg_frm->common.me_rnge.cime_srch_dwnh = 1;
    }

    region->intra = 1;
    region->quality = -ctx->cfg->rc.qp_delta_ip;

    region->area_map_en = 1;
    region->qp_area_idx = 1;
    region->abs_qp_en = 0;

    vepu510_h264_set_one_roi(base_cfg_buf, region, w, h);
    mpp_free(region);
RET:
    hal_h264e_dbg_func("leave, ret %d\n", ret);
    return ret;
}

static void setup_vepu510_recn_refr(HalH264eVepu510Ctx *ctx, HalVepu510RegSet *regs)
{

    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    H264eFrmInfo *frms = ctx->frms;
    HalBufs bufs = ctx->hw_recn;
    RK_S32 fbc_hdr_size = ctx->pixel_buf_fbc_hdr_size;

    HalBuf *curr = hal_bufs_get_buf(bufs, frms->curr_idx);
    HalBuf *refr = hal_bufs_get_buf(bufs, frms->refr_idx);

    hal_h264e_dbg_func("enter\n");

    if (curr && curr->cnt) {
        MppBuffer buf_pixel = curr->buf[0];
        MppBuffer buf_thumb = curr->buf[1];
        MppBuffer buf_smear = curr->buf[2];
        RK_S32 fd = mpp_buffer_get_fd(buf_pixel);

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        reg_frm->common.rfpw_h_addr = fd;
        reg_frm->common.rfpw_b_addr = fd;
        reg_frm->common.dspw_addr = mpp_buffer_get_fd(buf_thumb);
        reg_frm->common.adr_smear_wr = mpp_buffer_get_fd(buf_smear);
    }

    if (refr && refr->cnt) {
        MppBuffer buf_pixel = refr->buf[0];
        MppBuffer buf_thumb = refr->buf[1];
        MppBuffer buf_smear = curr->buf[2];
        RK_S32 fd = mpp_buffer_get_fd(buf_pixel);

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        reg_frm->common.rfpr_h_addr = fd;
        reg_frm->common.rfpr_b_addr = fd;
        reg_frm->common.dspr_addr = mpp_buffer_get_fd(buf_thumb);
        reg_frm->common.adr_smear_rd = mpp_buffer_get_fd(buf_smear);
    }
    mpp_dev_multi_offset_update(ctx->offsets, 164, fbc_hdr_size);
    mpp_dev_multi_offset_update(ctx->offsets, 166, fbc_hdr_size);

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_split(HalVepu510RegSet *regs, MppEncCfgSet *enc_cfg)
{
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    MppEncSliceSplit *cfg = &enc_cfg->split;

    hal_h264e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        reg_frm->common.sli_splt.sli_splt         = 0;
        reg_frm->common.sli_splt.sli_splt_mode    = 0;
        reg_frm->common.sli_splt.sli_splt_cpst    = 0;
        reg_frm->common.sli_splt.sli_max_num_m1   = 0;
        reg_frm->common.sli_splt.sli_flsh         = 0;
        reg_frm->common.sli_cnum.sli_splt_cnum_m1 = 0;

        reg_frm->common.sli_byte.sli_splt_byte    = 0;
        reg_frm->common.enc_pic.slen_fifo         = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        reg_frm->common.sli_splt.sli_splt         = 1;
        reg_frm->common.sli_splt.sli_splt_mode    = 0;
        reg_frm->common.sli_splt.sli_splt_cpst    = 0;
        reg_frm->common.sli_splt.sli_max_num_m1   = 500;
        reg_frm->common.sli_splt.sli_flsh         = 1;
        reg_frm->common.sli_cnum.sli_splt_cnum_m1 = 0;

        reg_frm->common.sli_byte.sli_splt_byte    = cfg->split_arg;
        reg_frm->common.enc_pic.slen_fifo         = cfg->split_out ? 1 : 0;
        regs->reg_ctl.int_en.vslc_done_en         = reg_frm->common.enc_pic.slen_fifo;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        RK_U32 mb_w = MPP_ALIGN(enc_cfg->prep.width, 16) / 16;
        RK_U32 mb_h = MPP_ALIGN(enc_cfg->prep.height, 16) / 16;
        RK_U32 slice_num = (mb_w * mb_h + cfg->split_arg - 1) / cfg->split_arg;

        reg_frm->common.sli_splt.sli_splt         = 1;
        reg_frm->common.sli_splt.sli_splt_mode    = 1;
        reg_frm->common.sli_splt.sli_splt_cpst    = 0;
        reg_frm->common.sli_splt.sli_max_num_m1   = 500;
        reg_frm->common.sli_splt.sli_flsh         = 1;
        reg_frm->common.sli_cnum.sli_splt_cnum_m1 = cfg->split_arg - 1;

        reg_frm->common.sli_byte.sli_splt_byte    = 0;
        reg_frm->common.enc_pic.slen_fifo         = cfg->split_out ? 1 : 0;
        if ((cfg->split_out & MPP_ENC_SPLIT_OUT_LOWDELAY) ||
            (regs->reg_frm.common.enc_pic.slen_fifo && (slice_num > VEPU510_SLICE_FIFO_LEN)))
            regs->reg_ctl.int_en.vslc_done_en = 1;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }
    cfg->change = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_me(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    H264eVepu510Param *reg_param = &regs->reg_param;
    MppEncSceneMode sm = ctx->cfg->tune.scene_mode;

    hal_h264e_dbg_func("enter\n");

    reg_frm->common.me_rnge.cime_srch_dwnh    = 15;
    reg_frm->common.me_rnge.cime_srch_uph     = 15;
    reg_frm->common.me_rnge.cime_srch_rgtw    = 12;
    reg_frm->common.me_rnge.cime_srch_lftw    = 12;
    reg_frm->common.me_cfg.rme_srch_h         = 3;
    reg_frm->common.me_cfg.rme_srch_v         = 3;

    reg_frm->common.me_cfg.srgn_max_num       = 54;
    reg_frm->common.me_cfg.cime_dist_thre     = 1024;
    reg_frm->common.me_cfg.rme_dis            = 0;
    reg_frm->common.me_cfg.fme_dis            = 0;
    reg_frm->common.me_rnge.dlt_frm_num       = 0x0;
    reg_frm->common.me_cach.cime_zero_thre    = 64;

    /* CIME: 0x1760 - 0x176C */
    reg_param->me_sqi_comb.cime_pmv_num = 1;
    reg_param->me_sqi_comb.cime_fuse    = 1;
    reg_param->me_sqi_comb.itp_mode     = 0;
    reg_param->me_sqi_comb.move_lambda  = 0;
    reg_param->me_sqi_comb.rime_lvl_mrg     = 1;
    reg_param->me_sqi_comb.rime_prelvl_en   = 0;
    reg_param->me_sqi_comb.rime_prersu_en   = 0;
    reg_param->cime_mvd_th_comb.cime_mvd_th0 = 16;
    reg_param->cime_mvd_th_comb.cime_mvd_th1 = 48;
    reg_param->cime_mvd_th_comb.cime_mvd_th2 = 80;
    reg_param->cime_madp_th_comb.cime_madp_th = 16;
    reg_param->cime_multi_comb.cime_multi0 = 8;
    reg_param->cime_multi_comb.cime_multi1 = 12;
    reg_param->cime_multi_comb.cime_multi2 = 16;
    reg_param->cime_multi_comb.cime_multi3 = 20;

    /* RFME: 0x1770 - 0x1778 */
    reg_param->rime_mvd_th_comb.rime_mvd_th0  = 1;
    reg_param->rime_mvd_th_comb.rime_mvd_th1  = 2;
    reg_param->rime_mvd_th_comb.fme_madp_th   = 0;
    reg_param->rime_madp_th_comb.rime_madp_th0 = 8;
    reg_param->rime_madp_th_comb.rime_madp_th1 = 16;
    reg_param->rime_multi_comb.rime_multi0 = 4;
    reg_param->rime_multi_comb.rime_multi1 = 8;
    reg_param->rime_multi_comb.rime_multi2 = 12;
    reg_param->cmv_st_th_comb.cmv_th0 = 64;
    reg_param->cmv_st_th_comb.cmv_th1 = 96;
    reg_param->cmv_st_th_comb.cmv_th2 = 128;

    if (sm != MPP_ENC_SCENE_MODE_IPC) {
        /* disable subjective optimization */
        reg_param->cime_madp_th_comb.cime_madp_th = 0;
        reg_param->rime_madp_th_comb.rime_madp_th0 = 0;
        reg_param->rime_madp_th_comb.rime_madp_th1 = 0;
        reg_param->cime_multi_comb.cime_multi0 = 4;
        reg_param->cime_multi_comb.cime_multi1 = 4;
        reg_param->cime_multi_comb.cime_multi2 = 4;
        reg_param->cime_multi_comb.cime_multi3 = 4;
        reg_param->rime_multi_comb.rime_multi0 = 4;
        reg_param->rime_multi_comb.rime_multi1 = 4;
        reg_param->rime_multi_comb.rime_multi2 = 4;
    }

    /* 0x1064 */
    regs->reg_rc_roi.madi_st_thd.madi_th0 = 5;
    regs->reg_rc_roi.madi_st_thd.madi_th1 = 12;
    regs->reg_rc_roi.madi_st_thd.madi_th2 = 20;
    /* 0x1068 */
    regs->reg_rc_roi.madp_st_thd0.madp_th0 = 4 << 4;
    regs->reg_rc_roi.madp_st_thd0.madp_th1 = 9 << 4;
    /* 0x106C */
    regs->reg_rc_roi.madp_st_thd1.madp_th2 = 15 << 4;

    hal_h264e_dbg_func("leave\n");
}

#define H264E_LAMBDA_TAB_SIZE       (52 * sizeof(RK_U32))

static RK_U32 h264e_lambda_default[60] = {
    0x00000005, 0x00000006, 0x00000007, 0x00000009,
    0x0000000b, 0x0000000e, 0x00000012, 0x00000016,
    0x0000001c, 0x00000024, 0x0000002d, 0x00000039,
    0x00000048, 0x0000005b, 0x00000073, 0x00000091,
    0x000000b6, 0x000000e6, 0x00000122, 0x0000016d,
    0x000001cc, 0x00000244, 0x000002db, 0x00000399,
    0x00000489, 0x000005b6, 0x00000733, 0x00000912,
    0x00000b6d, 0x00000e66, 0x00001224, 0x000016db,
    0x00001ccc, 0x00002449, 0x00002db7, 0x00003999,
    0x00004892, 0x00005b6f, 0x00007333, 0x00009124,
    0x0000b6de, 0x0000e666, 0x00012249, 0x00016dbc,
    0x0001cccc, 0x00024492, 0x0002db79, 0x00039999,
    0x00048924, 0x0005b6f2, 0x00073333, 0x00091249,
    0x000b6de5, 0x000e6666, 0x00122492, 0x0016dbcb,
    0x001ccccc, 0x00244924, 0x002db796, 0x00399998,
};

static RK_U32 h264e_lambda_cvr[60] = {
    0x00000009, 0x0000000b, 0x0000000e, 0x00000011,
    0x00000016, 0x0000001b, 0x00000022, 0x0000002b,
    0x00000036, 0x00000045, 0x00000056, 0x0000006d,
    0x00000089, 0x000000ad, 0x000000da, 0x00000112,
    0x00000159, 0x000001b3, 0x00000224, 0x000002b3,
    0x00000366, 0x00000449, 0x00000566, 0x000006cd,
    0x00000891, 0x00000acb, 0x00000d9a, 0x000013c1,
    0x000018e4, 0x00001f5c, 0x00002783, 0x000031c8,
    0x00003eb8, 0x00004f06, 0x00006390, 0x00008e14,
    0x0000b302, 0x0000e18a, 0x00011c29, 0x00016605,
    0x0001c313, 0x00027ae1, 0x00031fe6, 0x0003efcf,
    0x0004f5c3, 0x0006e785, 0x0008b2ef, 0x000af5c3,
    0x000f1e7a, 0x00130c7f, 0x00180000, 0x001e3cf4,
    0x002618fe, 0x00300000, 0x003c79e8, 0x004c31fc,
    0x00600000, 0x0078f3d0, 0x009863f8, 0x0c000000,
};

static void
setup_vepu510_l2(HalH264eVepu510Ctx *ctx, MppEncHwCfg *hw)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    MppEncSceneMode sm = ctx->cfg->tune.scene_mode;
    RK_S32 lambda_idx = ctx->cfg->tune.lambda_idx_i; //TODO: lambda_idx_p

    hal_h264e_dbg_func("enter\n");

    if (sm == MPP_ENC_SCENE_MODE_IPC) {
        memcpy(regs->reg_param.rdo_wgta_qp_grpa_0_51,
               &h264e_lambda_default[lambda_idx], H264E_LAMBDA_TAB_SIZE);
    } else {
        memcpy(regs->reg_param.rdo_wgta_qp_grpa_0_51,
               &h264e_lambda_cvr[lambda_idx], H264E_LAMBDA_TAB_SIZE);
    }

    if (hw->qbias_en) {
        regs->reg_param.qnt_bias_comb.qnt_f_bias_i = hw->qbias_i;
        regs->reg_param.qnt_bias_comb.qnt_f_bias_p = hw->qbias_p;
    } else {
        regs->reg_param.qnt_bias_comb.qnt_f_bias_i = 683;
        regs->reg_param.qnt_bias_comb.qnt_f_bias_p = 341;
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_ext_line_buf(HalVepu510RegSet *regs, HalH264eVepu510Ctx *ctx)
{
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    MppDevRcbInfoCfg rcb_cfg;
    RK_S32 offset = 0;
    RK_S32 fd;

    if (!ctx->ext_line_buf) {
        reg_frm->common.ebufb_addr = 0;
        reg_frm->common.ebufb_addr = 0;
        return;
    }

    fd = mpp_buffer_get_fd(ctx->ext_line_buf);
    offset = ctx->ext_line_buf_size;

    reg_frm->common.ebuft_addr = fd;
    reg_frm->common.ebufb_addr = fd;

    mpp_dev_multi_offset_update(ctx->offsets, 178, offset);

    /* rcb info for sram */
    rcb_cfg.reg_idx = 179;
    rcb_cfg.size = offset;

    mpp_dev_ioctl(ctx->dev, MPP_DEV_RCB_INFO, &rcb_cfg);

    rcb_cfg.reg_idx = 178;
    rcb_cfg.size = 0;

    mpp_dev_ioctl(ctx->dev, MPP_DEV_RCB_INFO, &rcb_cfg);
}

static MPP_RET setup_vepu510_dual_core(HalH264eVepu510Ctx *ctx, H264SliceType slice_type)
{
    H264eVepu510Frame *reg_frm = &ctx->regs_set->reg_frm;
    RK_U32 dchs_ofst = 9;
    RK_U32 dchs_rxe  = 1;
    RK_U32 dchs_dly  = 0;

    if (ctx->task_cnt == 1)
        return MPP_OK;

    if (slice_type == H264_I_SLICE) {
        ctx->curr_idx = 0;
        ctx->prev_idx = 0;
        dchs_rxe = 0;
    }

    reg_frm->common.dual_core.dchs_txid = ctx->curr_idx;
    reg_frm->common.dual_core.dchs_rxid = ctx->prev_idx;
    reg_frm->common.dual_core.dchs_txe  = 1;
    reg_frm->common.dual_core.dchs_rxe  = dchs_rxe;
    reg_frm->common.dual_core.dchs_ofst = dchs_ofst;
    reg_frm->common.dual_core.dchs_dly  = dchs_dly;

    ctx->prev_idx = ctx->curr_idx++;
    if (ctx->curr_idx > 3)
        ctx->curr_idx = 0;

    return MPP_OK;
}

static void setup_vepu510_aq(HalH264eVepu510Ctx *ctx)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncHwCfg *hw = &cfg->hw;
    Vepu510RcRoi *s = &ctx->regs_set->reg_rc_roi;
    RK_S32 *aq_step, *aq_thd;
    RK_U8 i;

    if (ctx->slice->slice_type == H264_I_SLICE) {
        aq_thd = (RK_S32 *)&hw->aq_thrd_i[0];
        aq_step = &hw->aq_step_i[0];
    } else {
        aq_thd = (RK_S32 *)&hw->aq_thrd_p[0];
        aq_step = &hw->aq_step_p[0];
    }

    for (i = 0; i < 16; i++)
        s->aq_tthd[i] = aq_thd[i] & 0xff;

    s->aq_stp0.aq_stp_s0 = aq_step[0] & 0x1f;
    s->aq_stp0.aq_stp_0t1 = aq_step[1] & 0x1f;
    s->aq_stp0.aq_stp_1t2 = aq_step[2] & 0x1f;
    s->aq_stp0.aq_stp_2t3 = aq_step[3] & 0x1f;
    s->aq_stp0.aq_stp_3t4 = aq_step[4] & 0x1f;
    s->aq_stp0.aq_stp_4t5 = aq_step[5] & 0x1f;
    s->aq_stp1.aq_stp_5t6 = aq_step[6] & 0x1f;
    s->aq_stp1.aq_stp_6t7 = aq_step[7] & 0x1f;
    s->aq_stp1.aq_stp_7t8 = 0;
    s->aq_stp1.aq_stp_8t9 = aq_step[8] & 0x1f;
    s->aq_stp1.aq_stp_9t10 = aq_step[9] & 0x1f;
    s->aq_stp1.aq_stp_10t11 = aq_step[10] & 0x1f;
    s->aq_stp2.aq_stp_11t12 = aq_step[11] & 0x1f;
    s->aq_stp2.aq_stp_12t13 = aq_step[12] & 0x1f;
    s->aq_stp2.aq_stp_13t14 = aq_step[13] & 0x1f;
    s->aq_stp2.aq_stp_14t15 = aq_step[14] & 0x1f;
    s->aq_stp2.aq_stp_b15 = aq_step[15] & 0x1f;
}

static void setup_vepu510_anti_stripe(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Param *s = &regs->reg_param;
    RK_S32 str = ctx->cfg->tune.atl_str;

    s->iprd_tthdy4_0.iprd_tthdy4_0 = 1;
    s->iprd_tthdy4_0.iprd_tthdy4_1 = 3;
    s->iprd_tthdy4_1.iprd_tthdy4_2 = 6;
    s->iprd_tthdy4_1.iprd_tthdy4_3 = 8;
    s->iprd_tthdc8_0.iprd_tthdc8_0 = 1;
    s->iprd_tthdc8_0.iprd_tthdc8_1 = 3;
    s->iprd_tthdc8_1.iprd_tthdc8_2 = 6;
    s->iprd_tthdc8_1.iprd_tthdc8_3 = 8;
    s->iprd_tthdy8_0.iprd_tthdy8_0 = 1;
    s->iprd_tthdy8_0.iprd_tthdy8_1 = 3;
    s->iprd_tthdy8_1.iprd_tthdy8_2 = 6;
    s->iprd_tthdy8_1.iprd_tthdy8_3 = 8;

    if (ctx->cfg->tune.scene_mode != MPP_ENC_SCENE_MODE_IPC)
        s->iprd_tthd_ul.iprd_tthd_ul = 4095; /* disable anti-stripe */
    else
        s->iprd_tthd_ul.iprd_tthd_ul = str ? 4 : 255;

    s->iprd_wgty8.iprd_wgty8_0 = str ? 22 : 16;
    s->iprd_wgty8.iprd_wgty8_1 = str ? 23 : 16;
    s->iprd_wgty8.iprd_wgty8_2 = str ? 20 : 16;
    s->iprd_wgty8.iprd_wgty8_3 = str ? 22 : 16;
    s->iprd_wgty4.iprd_wgty4_0 = str ? 22 : 16;
    s->iprd_wgty4.iprd_wgty4_1 = str ? 26 : 16;
    s->iprd_wgty4.iprd_wgty4_2 = str ? 20 : 16;
    s->iprd_wgty4.iprd_wgty4_3 = str ? 22 : 16;
    s->iprd_wgty16.iprd_wgty16_0 = 22;
    s->iprd_wgty16.iprd_wgty16_1 = 26;
    s->iprd_wgty16.iprd_wgty16_2 = 20;
    s->iprd_wgty16.iprd_wgty16_3 = 22;
    s->iprd_wgtc8.iprd_wgtc8_0 = 18;
    s->iprd_wgtc8.iprd_wgtc8_1 = 21;
    s->iprd_wgtc8.iprd_wgtc8_2 = 20;
    s->iprd_wgtc8.iprd_wgtc8_3 = 19;
}

static void setup_vepu510_anti_ringing(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Param *s = &regs->reg_param;
    MppEncSceneMode sm = ctx->cfg->tune.scene_mode;

    s->atr_thd1.thdqp = (sm == MPP_ENC_SCENE_MODE_IPC) ? 32 : 45;
    if (ctx->slice->slice_type == H264_I_SLICE) {
        s->atr_thd0.thd0 = 1;
        s->atr_thd0.thd1 = 2;
        s->atr_thd1.thd2 = 6;
        s->atr_wgt16.atr_lv16_wgt0 = 16;
        s->atr_wgt16.atr_lv16_wgt1 = 16;
        s->atr_wgt16.atr_lv16_wgt2 = 16;

        if (sm == MPP_ENC_SCENE_MODE_IPC) {
            s->atr_wgt8.atr_lv8_wgt0 = 22;
            s->atr_wgt8.atr_lv8_wgt1 = 21;
            s->atr_wgt8.atr_lv8_wgt2 = 20;
            s->atr_wgt4.atr_lv4_wgt0 = 20;
            s->atr_wgt4.atr_lv4_wgt1 = 18;
            s->atr_wgt4.atr_lv4_wgt2 = 16;
        } else {
            s->atr_wgt8.atr_lv8_wgt0 = 18;
            s->atr_wgt8.atr_lv8_wgt1 = 17;
            s->atr_wgt8.atr_lv8_wgt2 = 18;
            s->atr_wgt4.atr_lv4_wgt0 = 16;
            s->atr_wgt4.atr_lv4_wgt1 = 16;
            s->atr_wgt4.atr_lv4_wgt2 = 16;
        }
    } else {
        if (sm == MPP_ENC_SCENE_MODE_IPC) {
            s->atr_thd0.thd0 = 2;
            s->atr_thd0.thd1 = 4;
            s->atr_thd1.thd2 = 9;
            s->atr_wgt16.atr_lv16_wgt0 = 25;
            s->atr_wgt16.atr_lv16_wgt1 = 20;
            s->atr_wgt16.atr_lv16_wgt2 = 16;
            s->atr_wgt8.atr_lv8_wgt0 = 25;
            s->atr_wgt8.atr_lv8_wgt1 = 20;
            s->atr_wgt8.atr_lv8_wgt2 = 18;
            s->atr_wgt4.atr_lv4_wgt0 = 25;
            s->atr_wgt4.atr_lv4_wgt1 = 20;
            s->atr_wgt4.atr_lv4_wgt2 = 16;
        } else {
            s->atr_thd0.thd0 = 1;
            s->atr_thd0.thd1 = 2;
            s->atr_thd1.thd2 = 7;
            s->atr_wgt16.atr_lv16_wgt0 = 23;
            s->atr_wgt16.atr_lv16_wgt1 = 22;
            s->atr_wgt16.atr_lv16_wgt2 = 20;
            s->atr_wgt8.atr_lv8_wgt0 = 24;
            s->atr_wgt8.atr_lv8_wgt1 = 24;
            s->atr_wgt8.atr_lv8_wgt2 = 24;
            s->atr_wgt4.atr_lv4_wgt0 = 23;
            s->atr_wgt4.atr_lv4_wgt1 = 22;
            s->atr_wgt4.atr_lv4_wgt2 = 20;
        }
    }
}

static void setup_vepu510_anti_flicker(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Sqi *reg = &regs->reg_sqi;
    RK_U32 str = ctx->cfg->tune.anti_flicker_str;
    rdo_skip_par *p_skip = NULL;
    rdo_noskip_par *p_no_skip = NULL;

    static RK_U8 pskip_atf_th0[4] = { 0, 0, 0, 1 };
    static RK_U8 pskip_atf_th1[4] = { 7, 7, 7, 10 };
    static RK_U8 pskip_atf_wgt0[4] = { 16, 16, 16, 20 };
    static RK_U8 pskip_atf_wgt1[4] = { 16, 16, 14, 16 };
    static RK_U8 intra_atf_th0[4] = { 8, 16, 20, 20 };
    static RK_U8 intra_atf_th1[4] = { 16, 32, 40, 40 };
    static RK_U8 intra_atf_th2[4] = { 32, 56, 72, 72 };
    static RK_U8 intra_atf_wgt0[4] = { 16, 24, 27, 27 };
    static RK_U8 intra_atf_wgt1[4] = { 16, 22, 25, 25 };
    static RK_U8 intra_atf_wgt2[4] = { 16, 19, 20, 20 };

    p_skip = &reg->rdo_b16_skip;
    p_skip->atf_thd0.madp_thd0 = pskip_atf_th0[str];
    p_skip->atf_thd0.madp_thd1 = pskip_atf_th1[str];
    p_skip->atf_thd1.madp_thd2 = 15;
    p_skip->atf_thd1.madp_thd3 = 25;
    p_skip->atf_wgt0.wgt0 = pskip_atf_wgt0[str];
    p_skip->atf_wgt0.wgt1 = pskip_atf_wgt1[str];
    p_skip->atf_wgt0.wgt2 = 16;
    p_skip->atf_wgt0.wgt3 = 16;
    p_skip->atf_wgt1.wgt4 = 16;

    p_no_skip = &reg->rdo_b16_inter;
    p_no_skip->ratf_thd0.madp_thd0 = 20;
    p_no_skip->ratf_thd0.madp_thd1 = 40;
    p_no_skip->ratf_thd1.madp_thd2 = 72;
    p_no_skip->atf_wgt.wgt0 = 16;
    p_no_skip->atf_wgt.wgt1 = 16;
    p_no_skip->atf_wgt.wgt2 = 16;
    p_no_skip->atf_wgt.wgt3 = 16;

    p_no_skip = &reg->rdo_b16_intra;
    p_no_skip->ratf_thd0.madp_thd0 = intra_atf_th0[str];
    p_no_skip->ratf_thd0.madp_thd1 = intra_atf_th1[str];
    p_no_skip->ratf_thd1.madp_thd2 = intra_atf_th2[str];
    p_no_skip->atf_wgt.wgt0 = intra_atf_wgt0[str];
    p_no_skip->atf_wgt.wgt1 = intra_atf_wgt1[str];
    p_no_skip->atf_wgt.wgt2 = intra_atf_wgt2[str];
    p_no_skip->atf_wgt.wgt3 = 16;

    reg->rdo_b16_intra_atf_cnt_thd.thd0 = 1;
    reg->rdo_b16_intra_atf_cnt_thd.thd1 = 4;
    reg->rdo_b16_intra_atf_cnt_thd.thd2 = 1;
    reg->rdo_b16_intra_atf_cnt_thd.thd3 = 4;

    reg->rdo_atf_resi_thd.big_th0 = 16;
    reg->rdo_atf_resi_thd.big_th1 = 16;
    reg->rdo_atf_resi_thd.small_th0 = 8;
    reg->rdo_atf_resi_thd.small_th1 = 8;
}

static void setup_vepu510_anti_smear(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Sqi *reg = &regs->reg_sqi;
    H264eSlice *slice = ctx->slice;
    Vepu510H264Fbk *last_fb = &ctx->last_frame_fb;
    RK_U32 mb_cnt = last_fb->st_mb_num;
    RK_U32 *smear_cnt = last_fb->st_smear_cnt;
    RK_S32 deblur_str = ctx->cfg->tune.deblur_str;
    RK_S32 delta_qp = 0;
    RK_S32 flg0 = smear_cnt[4] < (mb_cnt >> 6);
    RK_S32 flg1 = 1, flg2 = 0, flg3 = 0;
    RK_S32 smear_multi[4] = { 9, 12, 16, 16 };

    hal_h264e_dbg_func("enter\n");

    if ((smear_cnt[3] < ((5 * mb_cnt) >> 10)) ||
        (smear_cnt[3] < ((1126 * MPP_MAX3(smear_cnt[0], smear_cnt[1], smear_cnt[2])) >> 10)) ||
        (deblur_str == 6) || (deblur_str == 7))
        flg1 = 0;

    flg3 = flg1 ? 3 : (smear_cnt[4] > ((102 * mb_cnt) >> 10)) ? 2 :
           (smear_cnt[4] > ((66 * mb_cnt) >> 10)) ? 1 : 0;

    if (ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC) {
        reg->smear_opt_cfg.rdo_smear_en = ctx->qpmap_en;
        if (ctx->qpmap_en && deblur_str > 3)
            reg->smear_opt_cfg.rdo_smear_lvl16_multi = smear_multi[flg3];
        else
            reg->smear_opt_cfg.rdo_smear_lvl16_multi = flg0 ? 9 : 12;
    } else {
        reg->smear_opt_cfg.rdo_smear_en = 0;
        reg->smear_opt_cfg.rdo_smear_lvl16_multi = 16;
    }

    if (ctx->qpmap_en && deblur_str > 3) {
        flg2 = 1;
        if (smear_cnt[2] + smear_cnt[3] > (3 * smear_cnt[4] / 4))
            delta_qp = 1;
        if (smear_cnt[4] < (mb_cnt >> 4))
            delta_qp -= 8;
        else if (smear_cnt[4] < ((3 * mb_cnt) >> 5))
            delta_qp -= 7;
        else
            delta_qp -= 6;

        if (flg3 == 2)
            delta_qp = 0;
        else if (flg3 == 1)
            delta_qp = -2;
    } else {
        if (smear_cnt[2] + smear_cnt[3] > smear_cnt[4] / 2)
            delta_qp = 1;
        if (smear_cnt[4] < (mb_cnt >> 8))
            delta_qp -= (deblur_str < 2) ? 6 : 8;
        else if (smear_cnt[4] < (mb_cnt >> 7))
            delta_qp -= (deblur_str < 2) ? 5 : 6;
        else if (smear_cnt[4] < (mb_cnt >> 6))
            delta_qp -= (deblur_str < 2) ? 3 : 4;
        else
            delta_qp -= 1;
    }
    reg->smear_opt_cfg.rdo_smear_dlt_qp = delta_qp;

    if ((H264_I_SLICE == slice->slice_type) ||
        (H264_I_SLICE == last_fb->frame_type))
        reg->smear_opt_cfg.stated_mode = 1;
    else
        reg->smear_opt_cfg.stated_mode = 2;

    reg->smear_madp_thd0.madp_cur_thd0 = 0;
    reg->smear_madp_thd0.madp_cur_thd1 = flg2 ? 48 : 24;
    reg->smear_madp_thd1.madp_cur_thd2 = flg2 ? 64 : 48;
    reg->smear_madp_thd1.madp_cur_thd3 = flg2 ? 72 : 64;
    reg->smear_madp_thd2.madp_around_thd0 = flg2 ? 4095 : 16;
    reg->smear_madp_thd2.madp_around_thd1 = 32;
    reg->smear_madp_thd3.madp_around_thd2 = 48;
    reg->smear_madp_thd3.madp_around_thd3 = flg2 ? 0 : 96;
    reg->smear_madp_thd4.madp_around_thd4 = 48;
    reg->smear_madp_thd4.madp_around_thd5 = 24;
    reg->smear_madp_thd5.madp_ref_thd0 = flg2 ? 64 : 96;
    reg->smear_madp_thd5.madp_ref_thd1 = 48;

    reg->smear_cnt_thd0.cnt_cur_thd0 = flg2 ?  2 : 1;
    reg->smear_cnt_thd0.cnt_cur_thd1 = flg2 ?  5 : 3;
    reg->smear_cnt_thd0.cnt_cur_thd2 = 1;
    reg->smear_cnt_thd0.cnt_cur_thd3 = 3;
    reg->smear_cnt_thd1.cnt_around_thd0 = 1;
    reg->smear_cnt_thd1.cnt_around_thd1 = 4;
    reg->smear_cnt_thd1.cnt_around_thd2 = 1;
    reg->smear_cnt_thd1.cnt_around_thd3 = 4;
    reg->smear_cnt_thd2.cnt_around_thd4 = 0;
    reg->smear_cnt_thd2.cnt_around_thd5 = 3;
    reg->smear_cnt_thd2.cnt_around_thd6 = 0;
    reg->smear_cnt_thd2.cnt_around_thd7 = 3;
    reg->smear_cnt_thd3.cnt_ref_thd0 = 1;
    reg->smear_cnt_thd3.cnt_ref_thd1 = 3;

    reg->smear_resi_thd0.resi_small_cur_th0 = 6;
    reg->smear_resi_thd0.resi_big_cur_th0 = 9;
    reg->smear_resi_thd0.resi_small_cur_th1 = 6;
    reg->smear_resi_thd0.resi_big_cur_th1 = 9;
    reg->smear_resi_thd1.resi_small_around_th0 = 6;
    reg->smear_resi_thd1.resi_big_around_th0 = 11;
    reg->smear_resi_thd1.resi_small_around_th1 = 6;
    reg->smear_resi_thd1.resi_big_around_th1 = 8;
    reg->smear_resi_thd2.resi_small_around_th2 = 9;
    reg->smear_resi_thd2.resi_big_around_th2 = 20;
    reg->smear_resi_thd2.resi_small_around_th3 = 6;
    reg->smear_resi_thd2.resi_big_around_th3 = 20;
    reg->smear_resi_thd3.resi_small_ref_th0 = 7;
    reg->smear_resi_thd3.resi_big_ref_th0 = 16;
    reg->smear_resi_thd4.resi_th0 = flg2 ? 0 : 10;
    reg->smear_resi_thd4.resi_th1 = flg2 ? 0 : 6;

    reg->smear_st_thd.madp_cnt_th0 = flg2 ? 0 : 1;
    reg->smear_st_thd.madp_cnt_th1 = flg2 ? 0 : 5;
    reg->smear_st_thd.madp_cnt_th2 = flg2 ? 0 : 1;
    reg->smear_st_thd.madp_cnt_th3 = flg2 ? 0 : 3;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu510_scaling_list(HalH264eVepu510Ctx *ctx)
{
    HalVepu510RegSet *regs = ctx->regs_set;
    Vepu510SclCfg *s = &regs->reg_scl;
    RK_U8 *p = (RK_U8 *)&s->tu8_intra_y[0];
    RK_U8 idx;

    hal_h264e_dbg_func("enter\n");

    /* intra4x4 and inter4x4 is not supported on VEPU500.
     * valid range: 0x2200 ~ 0x221F
     */
    if (ctx->pps->pic_scaling_matrix_present == 1) {
        for (idx = 0; idx < 64; idx++) {
            p[idx] = vepu510_h264_cqm_jvt8i[63 - idx]; /* intra8x8 */
            p[idx + 64] = vepu510_h264_cqm_jvt8p[63 - idx]; /* inter8x8 */
        }
    } else if (ctx->pps->pic_scaling_matrix_present == 2) {
        //TODO: Update scaling list for (scaling_list_mode == 2)
        mpp_log_f("scaling_list_mode 2 is not supported yet\n");
    }

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET hal_h264e_vepu510_gen_regs(void *hal, HalEncTask *task)
{
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    HalVepu510RegSet *regs = ctx->regs_set;
    H264eVepu510Frame *reg_frm = &regs->reg_frm;
    MppEncCfgSet *cfg = ctx->cfg;
    H264eSps *sps = ctx->sps;
    H264ePps *pps = ctx->pps;
    H264eSlice *slice = ctx->slice;
    EncRcTask *rc_task = task->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    MPP_RET ret = MPP_OK;
    EncFrmStatus *frm_status = &task->rc_task->frm;

    hal_h264e_dbg_func("enter %p\n", hal);
    hal_h264e_dbg_detail("frame %d generate regs now", ctx->frms->seq_idx);

    /* register setup */
    memset(regs, 0, sizeof(*regs));

    setup_vepu510_normal(regs);
    ret = setup_vepu510_prep(regs, &ctx->cfg->prep);
    if (ret)
        return ret;

    setup_vepu510_dual_core(ctx, slice->slice_type);
    setup_vepu510_codec(regs, sps, pps, slice);
    setup_vepu510_rdo_pred(ctx, sps, pps, slice);
    setup_vepu510_aq(ctx);
    setup_vepu510_anti_stripe(ctx);
    setup_vepu510_anti_ringing(ctx);
    setup_vepu510_anti_flicker(ctx);
    setup_vepu510_anti_smear(ctx);
    setup_vepu510_scaling_list(ctx);

    setup_vepu510_rc_base(regs, ctx, rc_task);
    setup_vepu510_io_buf(regs, ctx->offsets, task);
    setup_vepu510_recn_refr(ctx, regs);

    reg_frm->common.meiw_addr = task->md_info ? mpp_buffer_get_fd(task->md_info) : 0;
    reg_frm->common.enc_pic.mei_stor = 0;

    reg_frm->common.pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    reg_frm->common.pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);

    setup_vepu510_split(regs, cfg);
    setup_vepu510_me(ctx);

    if (frm_status->is_i_refresh)
        setup_vepu510_intra_refresh(regs, ctx, frm_status->seq_idx % cfg->rc.gop);

    setup_vepu510_l2(ctx, &cfg->hw);
    setup_vepu510_ext_line_buf(regs, ctx);
    if (ctx->roi_data)
        vepu510_set_roi(&ctx->regs_set->reg_rc_roi.roi_cfg, ctx->roi_data,
                        ctx->cfg->prep.width, ctx->cfg->prep.height);

    vepu510_h264e_tune_reg_patch(ctx->tune);

    /* two pass register patch */
    if (frm->save_pass1)
        vepu510_h264e_save_pass1_patch(regs, ctx);

    if (frm->use_pass1)
        vepu510_h264e_use_pass1_patch(regs, ctx);

    ctx->frame_cnt++;

    hal_h264e_dbg_func("leave %p\n", hal);
    return MPP_OK;
}

static MPP_RET hal_h264e_vepu510_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    HalVepu510RegSet *regs = ctx->regs_set;

    (void) task;

    hal_h264e_dbg_func("enter %p\n", hal);

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->reg_ctl;
        wr_cfg.size = sizeof(regs->reg_ctl);
        wr_cfg.offset = VEPU510_CTL_OFFSET;
#if DUMP_REG
        {
            RK_U32 i;
            RK_U32 *reg = (RK_U32)wr_cfg.reg;
            for ( i = 0; i < sizeof(regs->reg_ctl) / sizeof(RK_U32); i++) {
                mpp_log("reg[%d] = 0x%08x\n", i, reg[i]);
            }

        }
#endif
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->reg_frm;
        wr_cfg.size = sizeof(regs->reg_frm);
        wr_cfg.offset = VEPU510_FRAME_OFFSET;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->reg_rc_roi;
        wr_cfg.size = sizeof(regs->reg_rc_roi);
        wr_cfg.offset = VEPU510_RC_ROI_OFFSET;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->reg_param;
        wr_cfg.size = sizeof(regs->reg_param);
        wr_cfg.offset = VEPU510_PARAM_OFFSET;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->reg_sqi;
        wr_cfg.size = sizeof(regs->reg_sqi);
        wr_cfg.offset = VEPU510_SQI_OFFSET;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->reg_scl;
        wr_cfg.size = sizeof(regs->reg_scl);
        wr_cfg.offset = VEPU510_SCL_OFFSET ;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFS, ctx->offsets);
        if (ret) {
            mpp_err_f("set register offsets failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->reg_ctl.int_sta;
        rd_cfg.size = sizeof(RK_U32);
        rd_cfg.offset = VEPU510_REG_BASE_HW_STATUS;
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->reg_st;
        rd_cfg.size = sizeof(regs->reg_st);
        rd_cfg.offset = VEPU510_STATUS_OFFSET;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* send request to hardware */
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    hal_h264e_dbg_func("leave %p\n", hal);

    return ret;
}

static MPP_RET hal_h264e_vepu510_status_check(HalVepu510RegSet *regs)
{
    MPP_RET ret = MPP_OK;

    if (regs->reg_ctl.int_sta.lkt_node_done_sta)
        hal_h264e_dbg_detail("lkt_done finish");

    if (regs->reg_ctl.int_sta.enc_done_sta)
        hal_h264e_dbg_detail("enc_done finish");

    if (regs->reg_ctl.int_sta.vslc_done_sta)
        hal_h264e_dbg_detail("enc_slice finsh");

    if (regs->reg_ctl.int_sta.sclr_done_sta)
        hal_h264e_dbg_detail("safe clear finsh");

    if (regs->reg_ctl.int_sta.vbsf_oflw_sta) {
        mpp_err_f("bit stream overflow");
        ret = MPP_NOK;
    }

    if (regs->reg_ctl.int_sta.vbuf_lens_sta) {
        mpp_err_f("bus write full");
        ret = MPP_NOK;
    }

    if (regs->reg_ctl.int_sta.enc_err_sta) {
        mpp_err_f("bus error");
        ret = MPP_NOK;
    }

    if (regs->reg_ctl.int_sta.wdg_sta) {
        mpp_err_f("wdg timeout");
        ret = MPP_NOK;
    }

    return ret;
}

static MPP_RET hal_h264e_vepu510_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    HalVepu510RegSet *regs = &ctx->regs_sets[task->flags.reg_idx];
    RK_U32 split_out = ctx->cfg->split.split_out;
    MppPacket pkt = task->packet;
    RK_S32 offset = mpp_packet_get_length(pkt);
    H264NaluType type = task->rc_task->frm.is_idr ?  H264_NALU_TYPE_IDR : H264_NALU_TYPE_SLICE;
    MppEncH264HwCfg *hw_cfg = &ctx->cfg->codec.h264.hw_cfg;
    RK_S32 i;

    hal_h264e_dbg_func("enter %p\n", hal);

    /* if pass1 mode, it will disable split mode and the split out need to be disable */
    if (task->rc_task->frm.save_pass1)
        split_out = 0;

    /* update split_out in hw_cfg */
    hw_cfg->hw_split_out = split_out;

    if (split_out) {
        EncOutParam param;
        RK_U32 slice_len;
        RK_U32 slice_last;
        MppDevPollCfg *poll_cfg = (MppDevPollCfg *)((char *)ctx->poll_cfgs +
                                                    task->flags.reg_idx * ctx->poll_cfg_size);
        param.task = task;
        param.base = mpp_packet_get_data(task->packet);

        do {
            poll_cfg->poll_type = 0;
            poll_cfg->poll_ret  = 0;
            poll_cfg->count_max = ctx->poll_slice_max;
            poll_cfg->count_ret = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, poll_cfg);

            for (i = 0; i < poll_cfg->count_ret; i++) {
                slice_last = poll_cfg->slice_info[i].last;
                slice_len = poll_cfg->slice_info[i].length;

                mpp_packet_add_segment_info(pkt, type, offset, slice_len);
                offset += slice_len;

                if (split_out & MPP_ENC_SPLIT_OUT_LOWDELAY) {
                    param.length = slice_len;

                    if (slice_last)
                        ctx->output_cb->cmd = ENC_OUTPUT_FINISH;
                    else
                        ctx->output_cb->cmd = ENC_OUTPUT_SLICE;

                    mpp_callback(ctx->output_cb, &param);
                }
            }
        } while (!slice_last);

        ret = hal_h264e_vepu510_status_check(regs);
        if (!ret)
            task->hw_length += regs->reg_st.bs_lgth_l32;
    } else {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret) {
            mpp_err_f("poll cmd failed %d\n", ret);
            ret = MPP_ERR_VPUHW;
        } else {
            ret = hal_h264e_vepu510_status_check(regs);
            if (!ret)
                task->hw_length += regs->reg_st.bs_lgth_l32;
        }

        mpp_packet_add_segment_info(pkt, type, offset, regs->reg_st.bs_lgth_l32);
    }

    if (!(split_out & MPP_ENC_SPLIT_OUT_LOWDELAY) && !ret) {
        HalH264eVepuStreamAmend *amend = &ctx->amend_sets[task->flags.reg_idx];

        if (amend->enable) {
            amend->old_length = task->hw_length;
            amend->slice->is_multi_slice = (ctx->cfg->split.split_mode > 0);
            h264e_vepu_stream_amend_proc(amend, &ctx->cfg->codec.h264.hw_cfg);
            task->hw_length = amend->new_length;
        } else if (amend->prefix) {
            /* check prefix value */
            amend->old_length = task->hw_length;
            h264e_vepu_stream_amend_sync_ref_idc(amend);
        }
    }

    hal_h264e_dbg_func("leave %p ret %d\n", hal, ret);

    return ret;
}

static MPP_RET hal_h264e_vepu510_ret_task(void * hal, HalEncTask * task)
{
    HalH264eVepu510Ctx *ctx = (HalH264eVepu510Ctx *)hal;
    HalVepu510RegSet *regs = &ctx->regs_sets[task->flags.reg_idx];
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    Vepu510H264Fbk *fb = &ctx->feedback;
    Vepu510Status *reg_st = &regs->reg_st;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 mbs = mb_w * mb_h;

    hal_h264e_dbg_func("enter %p\n", hal);

    fb->st_mb_num = mbs;
    fb->st_smear_cnt[0] = reg_st->st_smear_cnt.rdo_smear_cnt0 * 4;
    fb->st_smear_cnt[1] = reg_st->st_smear_cnt.rdo_smear_cnt1 * 4;
    fb->st_smear_cnt[2] = reg_st->st_smear_cnt.rdo_smear_cnt2 * 4;
    fb->st_smear_cnt[3] = reg_st->st_smear_cnt.rdo_smear_cnt3 * 4;
    fb->st_smear_cnt[4] = fb->st_smear_cnt[0] + fb->st_smear_cnt[1] +
                          fb->st_smear_cnt[2] + fb->st_smear_cnt[3];
    fb->frame_type = ctx->slice->slice_type;

    // update total hardware length
    task->length += task->hw_length;

    // setup bit length for rate control
    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = regs->reg_st.qp_sum / mbs;
    rc_info->iblk4_prop = (regs->reg_st.st_pnum_i4.pnum_i4 +
                           regs->reg_st.st_pnum_i8.pnum_i8 +
                           regs->reg_st.st_pnum_i16.pnum_i16) * 256 / mbs;

    rc_info->sse = ((RK_S64)regs->reg_st.sse_h32 << 16) +
                   (regs->reg_st.st_sse_bsl.sse_l16 & 0xffff);
    rc_info->lvl16_inter_num = regs->reg_st.st_pnum_p16.pnum_p16;
    rc_info->lvl8_inter_num  = regs->reg_st.st_pnum_p8.pnum_p8;
    rc_info->lvl16_intra_num = regs->reg_st.st_pnum_i16.pnum_i16;
    rc_info->lvl8_intra_num  = regs->reg_st.st_pnum_i8.pnum_i8;
    rc_info->lvl4_intra_num  = regs->reg_st.st_pnum_i4.pnum_i4;

    ctx->hal_rc_cfg.bit_real = rc_info->bit_real;
    ctx->hal_rc_cfg.quality_real = rc_info->quality_real;
    ctx->hal_rc_cfg.iblk4_prop = rc_info->iblk4_prop;

    task->hal_ret.data   = &ctx->hal_rc_cfg;
    task->hal_ret.number = 1;

    mpp_dev_multi_offset_reset(ctx->offsets);

    if (ctx->dpb) {
        h264e_dpb_hal_end(ctx->dpb, task->flags.curr_idx);
        h264e_dpb_hal_end(ctx->dpb, task->flags.refr_idx);
    }

    vepu510_h264e_tune_stat_update(ctx->tune, task);

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

const MppEncHalApi hal_h264e_vepu510 = {
    .name       = "hal_h264e_vepu510",
    .coding     = MPP_VIDEO_CodingAVC,
    .ctx_size   = sizeof(HalH264eVepu510Ctx),
    .flag       = 0,
    .init       = hal_h264e_vepu510_init,
    .deinit     = hal_h264e_vepu510_deinit,
    .prepare    = hal_h264e_vepu510_prepare,
    .get_task   = hal_h264e_vepu510_get_task,
    .gen_regs   = hal_h264e_vepu510_gen_regs,
    .start      = hal_h264e_vepu510_start,
    .wait       = hal_h264e_vepu510_wait,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_h264e_vepu510_ret_task,
};
