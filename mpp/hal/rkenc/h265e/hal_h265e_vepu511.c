/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "hal_h265e_v511"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_enc_cb_param.h"

#include "rkv_enc_def.h"
#include "h265e_syntax_new.h"
#include "h265e_dpb.h"
#include "hal_bufs.h"
#include "hal_h265e_debug.h"
#include "hal_h265e_vepu511.h"
#include "hal_h265e_vepu511_reg.h"
#include "hal_h265e_stream_amend.h"

#include "vepu5xx_common.h"
#include "vepu541_common.h"
#include "vepu511_common.h"

#define MAX_FRAME_TASK_NUM      2
#define H265E_LAMBDA_TAB_SIZE  (52 * sizeof(RK_U32))
#define H265E_SMEAR_STR_NUM    (8)

#define hal_h265e_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

typedef struct Vepu511H265Fbk_t {
    RK_U32 hw_status; /* 0:corret, 1:error */
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
    RK_S32 reg_idx;
    RK_U32 acc_cover16_num;
    RK_U32 acc_bndry16_num;
    RK_U32 acc_zero_mv;
    RK_S8 tgt_sub_real_lvl[6];
} Vepu511H265Fbk;

typedef struct Vepu511H265eFrmCfg_t {
    RK_S32              frame_count;
    RK_S32              frame_type;

    /* dchs cfg on frame parallel */
    RK_S32              dchs_curr_idx;
    RK_S32              dchs_prev_idx;

    /* hal dpb management slot idx */
    RK_S32              hal_curr_idx;
    RK_S32              hal_refr_idx;

    /* regs cfg */
    H265eV511RegSet     *regs_set;
    H265eV511StatusElem *regs_ret;

    /* hardware return info collection cfg */
    Vepu511H265Fbk      feedback;

    void                *roi_data;

    /* roi buffer for qpmap or gdr */
    MppBuffer           roir_buf;
    RK_S32              roir_buf_size;
    void                *roi_base_cfg_sw_buf;

    /* variable length cfg */
    MppDevRegOffCfgs    *reg_cfg;
} Vepu511H265eFrmCfg;

typedef struct H265eV511HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;
    void                *reg_out;
    Vepu511H265eFrmCfg  *frms[MAX_FRAME_TASK_NUM];

    /* current used frame config */
    Vepu511H265eFrmCfg  *frm;

    /* slice split poll cfg */
    RK_S32              poll_slice_max;
    RK_S32              poll_cfg_size;
    MppDevPollCfg       *poll_cfgs;
    MppCbCtx            *output_cb;

    /* @frame_cnt starts from ZERO */
    RK_S32              frame_count;

    /* frame parallel info */
    RK_S32              task_cnt;
    RK_S32              task_idx;

    /* dchs cfg */
    RK_S32              curr_idx;
    RK_S32              prev_idx;

    Vepu511H265Fbk      feedback;
    Vepu511H265Fbk      last_frame_fb;
    void                *dump_files;
    RK_U32              frame_cnt_gen_ready;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    MppBufferGroup      roi_grp;
    void                *roi_data;
    Vepu511OsdCfg       osd_cfg;

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

    RK_S32              qpmap_en;
    RK_S32              smart_en;

    /* external line buffer over 3K */
    MppBufferGroup      ext_line_buf_grp;
    RK_S32              ext_line_buf_size;
    MppBuffer           ext_line_buf;
    MppBuffer           buf_pass1;
    MppBuffer           ext_line_bufs[MAX_FRAME_TASK_NUM];

    void                *tune;
} H265eV511HalContext;

static const  RK_U32 lambda_tbl_pre_intra[52] = {
    4206,   4945,   5814,   6835,   8035,   9446,   11105,  13056,
    15348,  18044,  21213,  24938,  29318,  34467,  40521,  47637,
    56003,  65839,  77402,  90996,  106977, 125765, 147852, 173819,
    204346, 240234, 983,    1206,   1479,   1813,   2223,   2727,
    3344,   4100,   5028,   6166,   7561,   9272,   11371,  13944,
    17099,  20969,  25714,  31533,  38669,  47420,  58150,  71310,
    87447, 107236, 131504, 161263,
};

static const  RK_U32 lambda_tbl_pre_inter[52] = {
    760,    959,    1210,   1526,   1925,   2428,   3063,   3864,
    4874,   6147,   7754,   9781,   12337,  15562,  19629,  24760,
    31231,  39394,  49691,  62678,  79061,  99725,  125790, 158668,
    200140, 252451, 579,    730,    919,    1159,   1461,   1993,
    2898,   3652,   4601,   5411,   6818,   7362,   9276,   11688,
    14725,  18553,  25324,  31906,  40200,  50649,  68724,  74217,
    101300, 127630, 148435, 187017,
};

static RK_U32 rdo_lambda_table_I[60] = {
    0x00000012, 0x00000017,
    0x0000001d, 0x00000024, 0x0000002e, 0x0000003a,
    0x00000049, 0x0000005c, 0x00000074, 0x00000092,
    0x000000b8, 0x000000e8, 0x00000124, 0x00000170,
    0x000001cf, 0x00000248, 0x000002df, 0x0000039f,
    0x0000048f, 0x000005bf, 0x0000073d, 0x0000091f,
    0x00000b7e, 0x00000e7a, 0x0000123d, 0x000016fb,
    0x00001cf4, 0x0000247b, 0x00002df6, 0x000039e9,
    0x000048f6, 0x00005bed, 0x000073d1, 0x000091ec,
    0x0000b7d9, 0x0000e7a2, 0x000123d7, 0x00016fb2,
    0x0001cf44, 0x000247ae, 0x0002df64, 0x00039e89,
    0x00048f5c, 0x0005bec8, 0x00073d12, 0x00091eb8,
    0x000b7d90, 0x000e7a23, 0x00123d71, 0x0016fb20,
    0x001cf446, 0x00247ae1, 0x002df640, 0x0039e88c,
    0x0048f5c3, 0x005bec81, 0x0073d119, 0x0091eb85,
    0x00b7d902, 0x00e7a232
};

static RK_U32 rdo_lambda_table_P[60] = {
    0x0000002c, 0x00000038, 0x00000044, 0x00000058,
    0x00000070, 0x00000089, 0x000000b0, 0x000000e0,
    0x00000112, 0x00000160, 0x000001c0, 0x00000224,
    0x000002c0, 0x00000380, 0x00000448, 0x00000580,
    0x00000700, 0x00000890, 0x00000b00, 0x00000e00,
    0x00001120, 0x00001600, 0x00001c00, 0x00002240,
    0x00002c00, 0x00003800, 0x00004480, 0x00005800,
    0x00007000, 0x00008900, 0x0000b000, 0x0000e000,
    0x00011200, 0x00016000, 0x0001c000, 0x00022400,
    0x0002c000, 0x00038000, 0x00044800, 0x00058000,
    0x00070000, 0x00089000, 0x000b0000, 0x000e0000,
    0x00112000, 0x00160000, 0x001c0000, 0x00224000,
    0x002c0000, 0x00380000, 0x00448000, 0x00580000,
    0x00700000, 0x00890000, 0x00b00000, 0x00e00000,
    0x01120000, 0x01600000, 0x01c00000, 0x02240000,
};

static RK_U8 vepu511_h265_cqm_intra8[64] = {
    16, 16, 16, 16, 17, 18, 21, 24,
    16, 16, 16, 16, 17, 19, 22, 25,
    16, 16, 17, 18, 20, 22, 25, 29,
    16, 16, 18, 21, 24, 27, 31, 36,
    17, 17, 20, 24, 30, 35, 41, 47,
    18, 19, 22, 27, 35, 44, 54, 65,
    21, 22, 25, 31, 41, 54, 70, 88,
    24, 25, 29, 36, 47, 65, 88, 115
};

static RK_U8 vepu511_h265_cqm_inter8[64] = {
    16, 16, 16, 16, 17, 18, 20, 24,
    16, 16, 16, 17, 18, 20, 24, 25,
    16, 16, 17, 18, 20, 24, 25, 28,
    16, 17, 18, 20, 24, 25, 28, 33,
    17, 18, 20, 24, 25, 28, 33, 41,
    18, 20, 24, 25, 28, 33, 41, 54,
    20, 24, 25, 28, 33, 41, 54, 71,
    24, 25, 28, 33, 41, 54, 71, 91
};

void save_to_file_511(char *name, void *ptr, size_t size)
{
    FILE *fp = fopen(name, "w+b");
    if (fp) {
        fwrite(ptr, 1, size, fp);
        fclose(fp);
    } else
        mpp_err("create file %s failed\n", name);
}

void vepu511_h265e_dump(H265eV511HalContext *ctx, HalEncTask *enc_task)
{
    H265eSyntax_new *syn = ctx->syn;
    HalBuf *hal_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.ref_pic.slot_idx);
    size_t buf_size = mpp_buffer_get_size(hal_buf->buf[0]);
    size_t dws_size = mpp_buffer_get_size(hal_buf->buf[1]);
    void *ptr = mpp_buffer_get_ptr(hal_buf->buf[0]);
    void *dws_ptr = mpp_buffer_get_ptr(hal_buf->buf[1]);
    RK_U32 frm_num = ctx->frms[enc_task->flags.reg_idx]->frame_count;
    RK_S32 pid = getpid();
    char name[128];
    size_t name_len = sizeof(name) - 1;

    snprintf(name, name_len, "/mnt/sdcard/dump/refr_fbd_%d_frm%d.bin", pid, frm_num);
    save_to_file_511(name, ptr + ctx->fbc_header_len, buf_size - ctx->fbc_header_len);

    snprintf(name, name_len, "/mnt/sdcard/dump/refr_fbh_%d_frm%d.bin", pid, frm_num);
    save_to_file_511(name, ptr, ctx->fbc_header_len);

    snprintf(name, name_len, "/mnt/sdcard/dump/refr_dsp_%d_frm%d.bin", pid, frm_num);
    save_to_file_511(name, dws_ptr, dws_size);

    hal_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.recon_pic.slot_idx);
    buf_size = mpp_buffer_get_size(hal_buf->buf[0]);
    dws_size = mpp_buffer_get_size(hal_buf->buf[1]);
    ptr = mpp_buffer_get_ptr(hal_buf->buf[0]);
    dws_ptr = mpp_buffer_get_ptr(hal_buf->buf[1]);

    snprintf(name, name_len, "/mnt/sdcard/dump/recn_fbd_%d_frm%d_slot%d.bin", pid, frm_num,  syn->sp.recon_pic.slot_idx);
    save_to_file_511(name, ptr + ctx->fbc_header_len, buf_size - ctx->fbc_header_len);

    snprintf(name, name_len, "/mnt/sdcard/dump/recn_fbh_%d_frm%d_slot%d.bin", pid, frm_num,  syn->sp.recon_pic.slot_idx);
    save_to_file_511(name, ptr, ctx->fbc_header_len);

    snprintf(name, name_len, "/mnt/sdcard/dump/recn_dsp_%d_frm%d_slot%d.bin", pid, frm_num,  syn->sp.recon_pic.slot_idx);
    save_to_file_511(name, dws_ptr, dws_size);

}

static void setup_ext_line_bufs(H265eV511HalContext *ctx)
{
    RK_S32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i])
            continue;

        mpp_buffer_get(ctx->ext_line_buf_grp, &ctx->ext_line_bufs[i],
                       ctx->ext_line_buf_size);
    }
}

static void clear_ext_line_bufs(H265eV511HalContext *ctx)
{
    RK_S32 i;

    for (i = 0; i < ctx->task_cnt; i++) {
        if (ctx->ext_line_bufs[i]) {
            mpp_buffer_put(ctx->ext_line_bufs[i]);
            ctx->ext_line_bufs[i] = NULL;
        }
    }
}

static MPP_RET vepu511_h265_setup_hal_bufs(H265eV511HalContext *ctx)
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
        size_t size[4] = {0};
        RK_S32 ctu_w = (prep->width + 31) / 32;
        RK_S32 ctu_h = (prep->height + 31) / 32;

        hal_bufs_deinit(ctx->dpb_bufs);
        hal_bufs_init(&ctx->dpb_bufs);

        ctx->fbc_header_len = MPP_ALIGN(((mb_wd64 * mb_h64) << 6), SZ_8K);
        size[0] = ctx->fbc_header_len + ((mb_wd64 * mb_h64) << 12) * 3 / 2; //fbc_h + fbc_b
        size[1] = (mb_wd64 * mb_h64 << 8);
        size[2] = MPP_ALIGN(mb_wd64 * mb_h64 * 16 * 4, 256) * 16;
        /* smear bufs */
        size[3] = MPP_ALIGN(ctu_w, 16) * MPP_ALIGN(ctu_h, 16);
        new_max_cnt = MPP_MAX(new_max_cnt, old_max_cnt);

        hal_h265e_dbg_detail("frame size %d -> %d max count %d -> %d\n",
                             ctx->frame_size, frame_size, old_max_cnt, new_max_cnt);

        hal_bufs_setup(ctx->dpb_bufs, new_max_cnt, MPP_ARRAY_ELEMS(size), size);

        ctx->frame_size = frame_size;
        ctx->max_buf_cnt = new_max_cnt;
    }
    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_vepu511_deinit(void *hal)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    RK_S32 i = 0;

    hal_h265e_enter();
    MPP_FREE(ctx->poll_cfgs);
    MPP_FREE(ctx->input_fmt);
    hal_bufs_deinit(ctx->dpb_bufs);

    for (i = 0; i < ctx->task_cnt; i++) {
        Vepu511H265eFrmCfg *frm = ctx->frms[i];

        if (!frm)
            continue;

        if (frm->roir_buf) {
            mpp_buffer_put(frm->roir_buf);
            frm->roir_buf = NULL;
            frm->roir_buf_size = 0;
        }

        MPP_FREE(frm->roi_base_cfg_sw_buf);

        if (frm->reg_cfg) {
            mpp_dev_multi_offset_deinit(frm->reg_cfg);
            frm->reg_cfg = NULL;
        }

        MPP_FREE(frm->regs_set);
        MPP_FREE(frm->regs_ret);
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

    if (ctx->roi_grp) {
        mpp_buffer_group_put(ctx->roi_grp);
        ctx->roi_grp = NULL;
    }

    if (ctx->tune) {
        // vepu511_h265e_tune_deinit(ctx->tune);
        ctx->tune = NULL;
    }

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_vepu511_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    RK_S32 i = 0;

    mpp_env_get_u32("hal_h265e_debug", &hal_h265e_debug, 0);
    hal_h265e_enter();

    ctx->task_cnt = cfg->task_cnt;
    mpp_assert(ctx->task_cnt && ctx->task_cnt <= MAX_FRAME_TASK_NUM);
    if (ctx->task_cnt > MAX_FRAME_TASK_NUM)
        ctx->task_cnt = MAX_FRAME_TASK_NUM;

    for (i = 0; i < ctx->task_cnt; i++) {
        Vepu511H265eFrmCfg *frm_cfg = mpp_calloc(Vepu511H265eFrmCfg, 1);

        frm_cfg->regs_set = mpp_calloc(H265eV511RegSet, 1);
        frm_cfg->regs_ret = mpp_calloc(H265eV511StatusElem, 1);
        frm_cfg->frame_type = INTRA_FRAME;
        ctx->frms[i] = frm_cfg;
    }

    ctx->input_fmt      = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg            = cfg->cfg;
    hal_bufs_init(&ctx->dpb_bufs);

    ctx->frame_count = -1;
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

        for (j = 0; j < MPP_ARRAY_ELEMS(hw->mode_bias); j++)
            hw->mode_bias[j] = 8;
    }

    ctx->poll_slice_max = 8;
    ctx->poll_cfg_size = (sizeof(ctx->poll_cfgs) + sizeof(RK_S32) * ctx->poll_slice_max) * 2;
    ctx->poll_cfgs = mpp_malloc_size(MppDevPollCfg, ctx->poll_cfg_size);

    if (NULL == ctx->poll_cfgs) {
        ret = MPP_ERR_MALLOC;
        mpp_err_f("init poll cfg buffer failed\n");
        goto DONE;
    }

    ctx->output_cb = cfg->output_cb;
    cfg->cap_recn_out = 1;

    // ctx->tune = vepu511_h265e_tune_init(ctx);

DONE:
    if (ret)
        hal_h265e_vepu511_deinit(hal);

    hal_h265e_leave();
    return ret;
}

static MPP_RET hal_h265e_vepu511_prepare(void *hal)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h265e_dbg_func("enter %p\n", hal);

    if (prep->change & (MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT)) {
        RK_S32 i;

        // pre-alloc required buffers to reduce first frame delay
        vepu511_h265_setup_hal_bufs(ctx);
        for (i = 0; i < ctx->max_buf_cnt; i++)
            hal_bufs_get_buf(ctx->dpb_bufs, i);

        prep->change = 0;
    }

    hal_h265e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET
vepu511_h265_set_patch_info(H265eSyntax_new *syn, Vepu541Fmt input_fmt,  MppDevRegOffCfgs *offsets, HalEncTask *task)
{
    RK_U32 hor_stride = syn->pp.hor_stride;
    RK_U32 ver_stride = syn->pp.ver_stride ? syn->pp.ver_stride : syn->pp.pic_height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
        u_offset = mpp_frame_get_fbc_offset(task->frame);
        v_offset = u_offset;
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

static MPP_RET vepu511_h265e_save_pass1_patch(H265eV511RegSet *regs, H265eV511HalContext *ctx,
                                              RK_S32 tiles_enabled_flag)
{
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
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

    reg_frm->common.enc_pic.cur_frm_ref = 1;
    reg_frm->common.rfpw_h_addr = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_frm->common.rfpw_b_addr = reg_frm->common.rfpw_h_addr;
    reg_frm->common.enc_pic.rec_fbc_dis = 1;

    if (tiles_enabled_flag)
        reg_frm->synt_pps.lpf_fltr_acrs_til = 0;

    mpp_dev_multi_offset_update(ctx->reg_cfg, 164, width_align * height_align);

    /* NOTE: disable split to avoid lowdelay slice output */
    reg_frm->common.sli_splt.sli_splt = 0;
    reg_frm->common.enc_pic.slen_fifo = 0;

    return MPP_OK;
}

static MPP_RET vepu511_h265e_use_pass1_patch(H265eV511RegSet *regs, H265eV511HalContext *ctx)
{
    Vepu511ControlCfg *reg_ctl = &regs->reg_ctl;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    RK_S32 width_align = MPP_ALIGN(ctx->cfg->prep.width, 16);
    RK_S32 height_align = MPP_ALIGN(ctx->cfg->prep.height, 16);
    RK_S32 y_stride = width_align;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    MPP_RET ret = MPP_OK;

    hal_h265e_dbg_func("enter\n");

    reg_frm->common.enc_pic.rfpr_compress_mode = 1;
    reg_ctl->dtrns_map.src_bus_edin = fmt->src_endian;
    reg_frm->common.src_fmt.src_cfmt = VEPU541_FMT_YUV420SP;
    reg_frm->common.src_fmt.alpha_swap = 0;
    reg_frm->common.src_fmt.rbuv_swap = 0;
    reg_frm->common.src_fmt.out_fmt = 1;

    reg_frm->common.src_strd0.src_strd0 = y_stride;
    reg_frm->common.src_strd1.src_strd1 = y_stride;

    reg_frm->common.src_proc.src_mirr = 0;
    reg_frm->common.src_proc.src_rot = 0;

    reg_frm->common.adr_src0 = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_frm->common.adr_src1 = reg_frm->common.adr_src0;

    /* input cb addr */
    ret = mpp_dev_multi_offset_update(ctx->reg_cfg, 161, width_align * height_align);
    if (ret)
        mpp_err_f("set input cb addr offset failed %d\n", ret);

    return MPP_OK;
}

static void setup_vepu511_ext_line_buf(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    RK_S32 fd;

    if (ctx->ext_line_buf) {
        fd = mpp_buffer_get_fd(ctx->ext_line_buf);

        reg_frm->common.ebufb_addr = fd;
        reg_frm->common.ebuft_addr = fd;
        mpp_dev_multi_offset_update(ctx->reg_cfg, 178, ctx->ext_line_buf_size);
    } else {
        reg_frm->common.ebufb_addr = 0;
        reg_frm->common.ebuft_addr = 0;
    }
}

static void vepu511_h265_set_scaling_list(H265eV511RegSet *regs)
{
    H265eVepu511SclCfg *s = &regs->reg_scl;
    RK_U8 *p = (RK_U8 *)&s->tu8_intra_y[0];
    RK_U32 scl_lst_sel = regs->reg_frm.rdo_cfg.scl_lst_sel;
    RK_U8 idx;

    hal_h265e_dbg_func("enter\n");

    if (scl_lst_sel == 1) {
        for (idx = 0; idx < 64; idx++) {
            /* TU8 intra Y/U/V */
            p[idx + 64 * 0] = vepu511_h265_cqm_intra8[63 - idx];

            p[idx + 64 * 1] = vepu511_h265_cqm_intra8[63 - idx];
            p[idx + 64 * 2] = vepu511_h265_cqm_intra8[63 - idx];

            /* TU8 inter Y/U/V */
            p[idx + 64 * 3] = vepu511_h265_cqm_inter8[63 - idx];
            p[idx + 64 * 4] = vepu511_h265_cqm_inter8[63 - idx];
            p[idx + 64 * 5] = vepu511_h265_cqm_inter8[63 - idx];

            /* TU16 intra Y/U/V AC */
            p[idx + 64 * 6] = vepu511_h265_cqm_intra8[63 - idx];
            p[idx + 64 * 7] = vepu511_h265_cqm_intra8[63 - idx];
            p[idx + 64 * 8] = vepu511_h265_cqm_intra8[63 - idx];

            /* TU16 inter Y/U/V AC */
            p[idx + 64 *  9] = vepu511_h265_cqm_inter8[63 - idx];
            p[idx + 64 * 10] = vepu511_h265_cqm_inter8[63 - idx];
            p[idx + 64 * 11] = vepu511_h265_cqm_inter8[63 - idx];

            /* TU32 intra/inter Y AC */
            p[idx + 64 * 12] = vepu511_h265_cqm_intra8[63 - idx];
            p[idx + 64 * 13] = vepu511_h265_cqm_inter8[63 - idx];
        }

        s->tu_dc0.tu16_intra_y_dc = 16;
        s->tu_dc0.tu16_intra_u_dc = 16;
        s->tu_dc0.tu16_intra_v_dc = 16;
        s->tu_dc0.tu16_inter_y_dc = 16;
        s->tu_dc1.tu16_inter_u_dc = 16;
        s->tu_dc1.tu16_inter_v_dc = 16;
        s->tu_dc1.tu32_intra_y_dc = 16;
        s->tu_dc1.tu32_inter_y_dc = 16;
    } else if (scl_lst_sel == 2) {
        mpp_log_f("scaling_list_mode 2 is not supported yet\n");
    }

    hal_h265e_dbg_func("leave\n");
}

static void vepu511_h265_set_normal(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    Vepu511ControlCfg *reg_ctl = &regs->reg_ctl;

    reg_ctl->enc_strt.lkt_num      = 0;
    reg_ctl->enc_strt.vepu_cmd     = ctx->enc_mode;
    reg_ctl->enc_clr.safe_clr      = 0;
    reg_ctl->enc_clr.force_clr     = 0;

    reg_ctl->int_en.enc_done_en        = 1;
    reg_ctl->int_en.lkt_node_done_en   = 1;
    reg_ctl->int_en.sclr_done_en       = 1;
    reg_ctl->int_en.vslc_done_en       = 1;
    reg_ctl->int_en.vbsf_oflw_en       = 1;
    reg_ctl->int_en.vbuf_lens_en       = 1;
    reg_ctl->int_en.enc_err_en         = 1;
    reg_ctl->int_en.vsrc_err_en        = 1;
    reg_ctl->int_en.wdg_en             = 1;
    reg_ctl->int_en.lkt_err_int_en     = 1;
    reg_ctl->int_en.lkt_err_int_en     = 1;
    reg_ctl->int_en.lkt_err_stop_en    = 1;
    reg_ctl->int_en.lkt_force_stop_en  = 1;
    reg_ctl->int_en.jslc_done_en       = 1;
    reg_ctl->int_en.jbsf_oflw_en       = 1;
    reg_ctl->int_en.jbuf_lens_en       = 1;
    reg_ctl->int_en.dvbm_err_en        = 0;

    reg_ctl->int_clr.enc_done_clr = 1;

    reg_ctl->dtrns_map.jpeg_bus_edin    = 0x7;
    reg_ctl->int_clr.enc_done_clr = 1;

    reg_ctl->dtrns_map.jpeg_bus_edin     = 0x7;
    reg_ctl->dtrns_map.src_bus_edin      = 0x0;
    reg_ctl->dtrns_map.meiw_bus_edin     = 0x0;
    reg_ctl->dtrns_map.bsw_bus_edin      = 0x7;
    reg_ctl->dtrns_map.lktr_bus_edin     = 0x0;
    reg_ctl->dtrns_map.roir_bus_edin     = 0x0;
    reg_ctl->dtrns_map.lktw_bus_edin     = 0x0;
    reg_ctl->dtrns_map.rec_nfbc_bus_edin = 0x0;

    reg_ctl->dtrns_cfg.axi_brsp_cke     = 0x3ff;
    reg_ctl->dtrns_cfg.axi_brsp_cke     = 0x3ff;
    reg_ctl->enc_wdg.vs_load_thd        = 0;
    reg_ctl->opt_strg.cke               = 1;
    reg_ctl->opt_strg.resetn_hw_en      = 0;
    reg_ctl->opt_strg.rfpr_err_e        = 1;
    reg_ctl->opt_strg.sram_ckg_en       = 0;

    /* enable rdo clk gating */
    {
        RK_U32 *rdo_ckg = (RK_U32*)&regs->reg_ctl.reg0022.rdo_ckg_hevc;

        *rdo_ckg = 0x0;
    }

}

static void vepu511_h265_set_prep(void *hal, HalEncTask *task, H265eV511RegSet *regs)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    Vepu511RcRoi *reg_klut = &regs->reg_rc_roi;
    H265eSyntax_new *syn = (H265eSyntax_new *)task->syntax.data;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 pic_wd32, pic_h32;
    MppEncSceneMode sm = ctx->cfg->tune.scene_mode;

    hal_h265e_enter();

    pic_width_align8 = (syn->pp.pic_width + 7) & (~7);
    pic_height_align8 = (syn->pp.pic_height + 7) & (~7);
    pic_wd32 = (syn->pp.pic_width +  31) / 32;
    pic_h32 = (syn->pp.pic_height + 31) / 32;

    reg_frm->common.enc_rsl.pic_wd8_m1 = pic_width_align8 / 8 - 1;
    reg_frm->common.enc_rsl.pic_hd8_m1 = pic_height_align8 / 8 - 1;
    reg_frm->common.src_fill.pic_wfill = (syn->pp.pic_width & 0x7)
                                         ? (8 - (syn->pp.pic_width & 0x7)) : 0;
    reg_frm->common.src_fill.pic_hfill = (syn->pp.pic_height & 0x7)
                                         ? (8 - (syn->pp.pic_height & 0x7)) : 0;

    /* H.265 mode */
    reg_frm->common.enc_pic.enc_stnd      = 1;
    /* current frame will be refered */
    reg_frm->common.enc_pic.cur_frm_ref   = !syn->sp.non_reference_flag;

    reg_frm->common.enc_pic.bs_scp        = 1;
    reg_frm->common.enc_pic.log2_ctu_num_hevc  = mpp_ceil_log2(pic_wd32 * pic_h32);

    reg_klut->klut_ofst.chrm_klut_ofst = (ctx->frame_type == INTRA_FRAME) ? 6 :
                                         (sm == MPP_ENC_SCENE_MODE_IPC ? 9 : 6);

    reg_frm->common.enc_pic.rfpr_compress_mode = 0;
    reg_frm->common.enc_pic.rec_fbc_dis = 0;

    reg_frm->rdo_cfg.chrm_spcl  = 0;
    reg_frm->rdo_cfg.cu_inter_e = 0x5b;

    if (syn->pp.num_long_term_ref_pics_sps) {
        reg_frm->rdo_cfg.ltm_col = 0;
        reg_frm->rdo_cfg.ltm_idx0l0 = 1;
    } else {
        reg_frm->rdo_cfg.ltm_col = 0;
        reg_frm->rdo_cfg.ltm_idx0l0 = 0;
    }

    reg_frm->rdo_cfg.ccwa_e = 1;
    reg_frm->rdo_cfg.scl_lst_sel = syn->pp.scaling_list_enabled_flag;

    {
        RK_U32 i_nal_type = 0;

        if (ctx->frame_type == INTRA_FRAME)
            i_nal_type = NAL_IDR_W_RADL;
        else if (ctx->frame_type == INTER_P_FRAME )
            i_nal_type = NAL_TRAIL_R;
        else
            i_nal_type = NAL_TRAIL_R;

        reg_frm->synt_nal.nal_unit_type = i_nal_type;
    }

    reg_frm->rdo_intra_mode.intra_pu4_mode_num  = 1;
    reg_frm->rdo_intra_mode.intra_pu8_mode_num  = 2;
    reg_frm->rdo_intra_mode.intra_pu16_mode_num = 2;
    reg_frm->rdo_intra_mode.intra_pu32_mode_num = 2;

}

static void vepu511_h265_set_split(H265eV511RegSet *regs, MppEncCfgSet *enc_cfg)
{
    MppEncSliceSplit *cfg = &enc_cfg->split;

    hal_h265e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        regs->reg_frm.common.sli_splt.sli_splt = 0;
        regs->reg_frm.common.sli_splt.sli_splt_mode = 0;
        regs->reg_frm.common.sli_splt.sli_splt_cpst = 0;
        regs->reg_frm.common.sli_splt.sli_max_num_m1 = 0;
        regs->reg_frm.common.sli_splt.sli_flsh = 0;
        regs->reg_frm.common.sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg_frm.common.sli_byte.sli_splt_byte = 0;
        regs->reg_frm.common.enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        regs->reg_frm.common.sli_splt.sli_splt = 1;
        regs->reg_frm.common.sli_splt.sli_splt_mode = 0;
        regs->reg_frm.common.sli_splt.sli_splt_cpst = 0;
        regs->reg_frm.common.sli_splt.sli_max_num_m1 = 500;
        regs->reg_frm.common.sli_splt.sli_flsh = 1;
        regs->reg_frm.common.sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg_frm.common.sli_byte.sli_splt_byte = cfg->split_arg;
        regs->reg_frm.common.enc_pic.slen_fifo = cfg->split_out ? 1 : 0;
        regs->reg_ctl.int_en.vslc_done_en = cfg->split_out ? 1 : 0;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        regs->reg_frm.common.sli_splt.sli_splt = 1;
        regs->reg_frm.common.sli_splt.sli_splt_mode = 1;
        regs->reg_frm.common.sli_splt.sli_splt_cpst = 0;
        regs->reg_frm.common.sli_splt.sli_max_num_m1 = 500;
        regs->reg_frm.common.sli_splt.sli_flsh = 1;
        regs->reg_frm.common.sli_cnum.sli_splt_cnum_m1 = cfg->split_arg - 1;

        regs->reg_frm.common.sli_byte.sli_splt_byte = 0;
        regs->reg_frm.common.enc_pic.slen_fifo = cfg->split_out ? 1 : 0;
        regs->reg_ctl.int_en.vslc_done_en = cfg->split_out ? 1 : 0;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }
    cfg->change = 0;
    hal_h265e_dbg_func("leave\n");
}

static void vepu511_h265_set_me_regs(H265eV511HalContext *ctx, H265eSyntax_new *syn, H265eV511RegSet *regs)
{
    H265eVepu511Param *s = &regs->reg_param;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;

    reg_frm->common.me_rnge.cime_srch_dwnh = 15;
    reg_frm->common.me_rnge.cime_srch_uph  = 15;
    reg_frm->common.me_rnge.cime_srch_rgtw = 12;
    reg_frm->common.me_rnge.cime_srch_lftw = 12;
    reg_frm->common.me_cfg.rme_srch_h      = 3;
    reg_frm->common.me_cfg.rme_srch_v      = 3;

    reg_frm->common.me_cfg.srgn_max_num      = 72;
    reg_frm->common.me_cfg.cime_dist_thre    = 1024;
    reg_frm->common.me_cfg.rme_dis           = 0;
    reg_frm->common.me_cfg.fme_dis           = 0;
    reg_frm->common.me_rnge.dlt_frm_num      = 0x1;

    if (syn->pp.sps_temporal_mvp_enabled_flag && (ctx->frame_type != INTRA_FRAME)) {
        if (ctx->last_frame_fb.frame_type == INTRA_FRAME)
            reg_frm->common.me_cach.colmv_load_hevc = 0;
        else
            reg_frm->common.me_cach.colmv_load_hevc = 1;

        reg_frm->common.me_cach.colmv_stor_hevc = 1;
    }

    reg_frm->common.me_cach.cime_zero_thre = 64;
    reg_frm->common.me_cach.fme_prefsu_en = 0;

    /* CIME: 0x1760 - 0x176C */
    s->me_sqi_comb.cime_pmv_num = 1;
    s->me_sqi_comb.cime_fuse   = 1;
    s->me_sqi_comb.move_lambda = 2;
    s->me_sqi_comb.rime_lvl_mrg     = 0;
    s->me_sqi_comb.rime_prelvl_en   = 3;
    s->me_sqi_comb.rime_prersu_en   = 0;
    s->me_sqi_comb.fme_lvl_mrg = 0;
    s->cime_mvd_th_comb.cime_mvd_th0 = 8;
    s->cime_mvd_th_comb.cime_mvd_th1 = 20;
    s->cime_mvd_th_comb.cime_mvd_th2 = 32;
    s->cime_madp_th_comb.cime_madp_th = 16;
    s->cime_madp_th_comb.ratio_consi_cfg = 8;
    s->cime_madp_th_comb.ratio_bmv_dist = 8;
    s->cime_multi_comb.cime_multi0 = 8;
    s->cime_multi_comb.cime_multi1 = 12;
    s->cime_multi_comb.cime_multi2 = 16;
    s->cime_multi_comb.cime_multi3 = 20;

    /* RFME: 0x1770 - 0x177C */
    s->rime_mvd_th_comb.rime_mvd_th0  = 1;
    s->rime_mvd_th_comb.rime_mvd_th1  = 2;
    s->rime_mvd_th_comb.fme_madp_th   = 0;
    s->rime_madp_th_comb.rime_madp_th0 = 8;
    s->rime_madp_th_comb.rime_madp_th1 = 16;
    s->rime_multi_comb.rime_multi0 = 4;
    s->rime_multi_comb.rime_multi1 = 8;
    s->rime_multi_comb.rime_multi2 = 12;
    s->cmv_st_th_comb.cmv_th0 = 64;
    s->cmv_st_th_comb.cmv_th1 = 96;
    s->cmv_st_th_comb.cmv_th2 = 128;

    if (ctx->cfg->tune.scene_mode != MPP_ENC_SCENE_MODE_IPC) {
        s->cime_madp_th_comb.cime_madp_th = 0;
        s->rime_madp_th_comb.rime_madp_th0 = 0;
        s->rime_madp_th_comb.rime_madp_th1 = 0;
        s->cime_multi_comb.cime_multi0 = 4;
        s->cime_multi_comb.cime_multi1 = 4;
        s->cime_multi_comb.cime_multi2 = 4;
        s->cime_multi_comb.cime_multi3 = 4;
        s->rime_multi_comb.rime_multi0 = 4;
        s->rime_multi_comb.rime_multi1 = 4;
        s->rime_multi_comb.rime_multi2 = 4;
    } else if (ctx->smart_en) {
        s->cime_multi_comb.cime_multi0 = 4;
        s->cime_multi_comb.cime_multi1 = 6;
        s->cime_multi_comb.cime_multi2 = 8;
        s->cime_multi_comb.cime_multi3 = 12;
        s->rime_multi_comb.rime_multi0 = 4;
        s->rime_multi_comb.rime_multi1 = 6;
        s->rime_multi_comb.rime_multi2 = 8;
    }

    s->rime_mvd_th_comb.fme_madp_th  = 0;
    s->rime_multi_comb.rime_multi0   = 0;
    s->rime_multi_comb.rime_multi1   = 0;
    s->rime_multi_comb.rime_multi2   = 0;
}

static void vepu511_h265_set_hw_address(H265eV511HalContext *ctx, H265eVepu511Frame *regs,
                                        HalEncTask *task)
{
    HalEncTask *enc_task = task;
    HalBuf *recon_buf, *ref_buf;
    MppBuffer md_info_buf = enc_task->md_info;
    Vepu511H265eFrmCfg *frm = ctx->frm;
    H265eSyntax_new *syn = ctx->syn;

    hal_h265e_enter();

    regs->common.adr_src0  = mpp_buffer_get_fd(enc_task->input);
    regs->common.adr_src1  = regs->common.adr_src0;
    regs->common.adr_src2  = regs->common.adr_src0;

    recon_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_curr_idx);
    ref_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_refr_idx);

    if (!syn->sp.non_reference_flag) {
        regs->common.rfpw_h_addr  = mpp_buffer_get_fd(recon_buf->buf[0]);
        regs->common.rfpw_b_addr  = regs->common.rfpw_h_addr;
        mpp_dev_multi_offset_update(ctx->reg_cfg, 164, ctx->fbc_header_len);
    }
    regs->common.rfpr_h_addr = mpp_buffer_get_fd(ref_buf->buf[0]);
    regs->common.rfpr_b_addr = regs->common.rfpr_h_addr;
    regs->common.colmvw_addr = mpp_buffer_get_fd(recon_buf->buf[2]);
    regs->common.colmvr_addr = mpp_buffer_get_fd(ref_buf->buf[2]);
    regs->common.dspw_addr = mpp_buffer_get_fd(recon_buf->buf[1]);
    regs->common.dspr_addr = mpp_buffer_get_fd(ref_buf->buf[1]);

    mpp_dev_multi_offset_update(ctx->reg_cfg, 166, ctx->fbc_header_len);

    if (md_info_buf) {
        regs->common.enc_pic.mei_stor = 1;
        regs->common.meiw_addr = mpp_buffer_get_fd(md_info_buf);
    } else {
        regs->common.enc_pic.mei_stor = 0;
        regs->common.meiw_addr = 0;
    }

    regs->common.bsbt_addr = mpp_buffer_get_fd(enc_task->output);
    /* TODO: stream size relative with syntax */
    regs->common.bsbb_addr  = regs->common.bsbt_addr;
    regs->common.bsbr_addr  = regs->common.bsbt_addr;
    regs->common.adr_bsbs   = regs->common.bsbt_addr;

    regs->common.rfpt_h_addr = 0xffffffff;
    regs->common.rfpb_h_addr = 0;
    regs->common.rfpt_b_addr = 0xffffffff;
    regs->common.adr_rfpb_b  = 0;
    regs->common.adr_roir    = 0;

    mpp_dev_multi_offset_update(ctx->reg_cfg, 174, mpp_packet_get_length(task->packet));
    mpp_dev_multi_offset_update(ctx->reg_cfg, 172, mpp_buffer_get_size(enc_task->output));

    regs->common.pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->common.pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);

    /* smear bufs */
    regs->common.adr_smear_rd = mpp_buffer_get_fd(ref_buf->buf[3]);
    regs->common.adr_smear_wr = mpp_buffer_get_fd(recon_buf->buf[3]);
}

static void vepu511_h265_set_pp_regs(H265eV511RegSet *regs, VepuFmtCfg *fmt,
                                     MppEncPrepCfg *prep_cfg, HalEncTask *task)
{
    Vepu511ControlCfg *reg_ctl = &regs->reg_ctl;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    reg_ctl->dtrns_map.src_bus_edin = fmt->src_endian;
    reg_frm->common.src_fmt.src_cfmt = fmt->format;
    reg_frm->common.src_fmt.alpha_swap = fmt->alpha_swap;
    reg_frm->common.src_fmt.rbuv_swap = fmt->rbuv_swap;

    reg_frm->common.src_fmt.out_fmt = (prep_cfg->format == MPP_FMT_YUV400) ? 0 : 1;

    reg_frm->common.src_proc.src_mirr = prep_cfg->mirroring > 0;
    reg_frm->common.src_proc.src_rot = prep_cfg->rotation;

    if (MPP_FRAME_FMT_IS_FBC(prep_cfg->format)) {
        reg_frm->common.src_proc.rkfbcd_en = 1;

        stridey = mpp_frame_get_fbc_hdr_stride(task->frame);
        if (!stridey)
            stridey = MPP_ALIGN(prep_cfg->hor_stride, 64) >> 2;
    } else if (prep_cfg->hor_stride)
        stridey = prep_cfg->hor_stride;
    else {
        if (reg_frm->common.src_fmt.src_cfmt == VEPU541_FMT_BGRA8888 )
            stridey = prep_cfg->width * 4;
        else if (reg_frm->common.src_fmt.src_cfmt == VEPU541_FMT_BGR888 )
            stridey = prep_cfg->width * 3;
        else if (reg_frm->common.src_fmt.src_cfmt == VEPU541_FMT_BGR565 ||
                 reg_frm->common.src_fmt.src_cfmt == VEPU541_FMT_YUYV422 ||
                 reg_frm->common.src_fmt.src_cfmt == VEPU541_FMT_UYVY422)
            stridey = prep_cfg->width * 2;
    }

    switch (fmt->format) {
    case VEPU580_FMT_YUV444SP : {
        stridec = stridey * 2;
    } break;
    case VEPU541_FMT_YUV422SP :
    case VEPU541_FMT_YUV420SP :
    case VEPU580_FMT_YUV444P : {
        stridec = stridey;
    } break;
    default : {
        stridec = stridey / 2;
    } break;
    }

    if (reg_frm->common.src_fmt.src_cfmt < VEPU541_FMT_NONE) {
        reg_frm->common.src_udfy.csc_wgt_r2y = 77;
        reg_frm->common.src_udfy.csc_wgt_g2y = 150;
        reg_frm->common.src_udfy.csc_wgt_b2y = 29;

        reg_frm->common.src_udfu.csc_wgt_r2u = -43;
        reg_frm->common.src_udfu.csc_wgt_g2u = -85;
        reg_frm->common.src_udfu.csc_wgt_b2u = 128;

        reg_frm->common.src_udfv.csc_wgt_r2v = 128;
        reg_frm->common.src_udfv.csc_wgt_g2v = -107;
        reg_frm->common.src_udfv.csc_wgt_b2v = -21;

        reg_frm->common.src_udfo.csc_ofst_y = 0;
        reg_frm->common.src_udfo.csc_ofst_u = 128;
        reg_frm->common.src_udfo.csc_ofst_v = 128;
    }

    reg_frm->common.src_strd0.src_strd0  = stridey;
    reg_frm->common.src_strd1.src_strd1  = stridec;
}

static void vepu511_h265_set_vsp_filtering(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    // H265eV511RegSet *regs = ctx->regs;
    H265eVepu511Frame *s = &regs->reg_frm;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncHwCfg *hw = &cfg->hw;
    RK_U8 bit_chg_lvl = ctx->last_frame_fb.tgt_sub_real_lvl[5]; /* [0, 2] */
    RK_U8 corner_str = 0, edge_str = 0, internal_str = 0; /* [0, 3] */

    if (cfg->tune.deblur_en && (cfg->tune.deblur_str % 2 == 0) &&
        (hw->flt_str_i == 0) && (hw->flt_str_p == 0)) {
        if (bit_chg_lvl == 2 && ctx->frame_type != INTRA_FRAME) {
            corner_str = 3;
            edge_str = 3;
            internal_str = 3;
        } else if (bit_chg_lvl > 0) {
            corner_str = 2;
            edge_str = 2;
            internal_str = 2;
        }
    } else {
        if (ctx->frame_type == INTRA_FRAME) {
            corner_str = hw->flt_str_i;
            edge_str = hw->flt_str_i;
            internal_str = hw->flt_str_i;
        } else {
            corner_str = hw->flt_str_p;
            edge_str = hw->flt_str_p;
            internal_str = hw->flt_str_p;
        }
    }

    s->common.src_flt_cfg.pp_corner_filter_strength = corner_str;
    s->common.src_flt_cfg.pp_edge_filter_strength = edge_str;
    s->common.src_flt_cfg.pp_internal_filter_strength = internal_str;
}

static void vepu511_h265_set_rc_regs(H265eV511HalContext *ctx, H265eV511RegSet *regs,
                                     HalEncTask *task)
{
    H265eSyntax_new *syn = (H265eSyntax_new *)task->syntax.data;
    EncRcTaskInfo *rc_cfg = &task->rc_task->info;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    Vepu511RcRoi *reg_rc = &regs->reg_rc_roi;
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
        reg_frm->common.enc_pic.pic_qp    = rc_cfg->quality_target;
        reg_frm->synt_sli1.sli_qp         = rc_cfg->quality_target;
        reg_frm->common.rc_qp.rc_max_qp   = rc_cfg->quality_target;
        reg_frm->common.rc_qp.rc_min_qp   = rc_cfg->quality_target;
        reg_frm->common.rc_cfg.rc_ctu_num = 1;
    } else {
        if (ctu_target_bits_mul_16 >= 0x100000) {
            ctu_target_bits_mul_16 = 0x50000;
        }
        ctu_target_bits = (ctu_target_bits_mul_16 * mb_wd32) >> 4;
        negative_bits_thd = 0 - 5 * ctu_target_bits / 16;
        positive_bits_thd = 5 * ctu_target_bits / 16;

        reg_frm->common.enc_pic.pic_qp      = rc_cfg->quality_target;
        reg_frm->synt_sli1.sli_qp           = rc_cfg->quality_target;
        reg_frm->common.rc_cfg.rc_en        = 1;
        reg_frm->common.rc_cfg.aq_en        = 1;
        reg_frm->common.rc_cfg.rc_ctu_num   = mb_wd32;

        reg_frm->common.rc_qp.rc_max_qp     = rc_cfg->quality_max;
        reg_frm->common.rc_qp.rc_min_qp     = rc_cfg->quality_min;
        reg_frm->common.rc_tgt.ctu_ebit     = ctu_target_bits_mul_16;

        if (ctx->smart_en) {
            reg_frm->common.rc_qp.rc_qp_range = 0;
        } else {
            reg_frm->common.rc_qp.rc_qp_range = (ctx->frame_type == INTRA_FRAME) ?
                                                hw->qp_delta_row_i : hw->qp_delta_row;
        }

        {
            /* fixed frame qp */
            RK_S32 fqp_min, fqp_max;

            if (ctx->frame_type == INTRA_FRAME) {
                fqp_min = rc->fqp_min_i;
                fqp_max = rc->fqp_max_i;
            } else {
                fqp_min = rc->fqp_min_p;
                fqp_max = rc->fqp_max_p;
            }

            if ((fqp_min == fqp_max) && (fqp_min >= 0) && (fqp_max <= 51)) {
                reg_frm->common.enc_pic.pic_qp = fqp_min;
                reg_frm->synt_sli1.sli_qp  = fqp_min;
                reg_frm->common.rc_qp.rc_qp_range = 0;
            }
        }

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
    }

    reg_rc->roi_qthd0.qpmin_area0 = h265->qpmin_map[0] > 0 ? h265->qpmin_map[0] : rc_cfg->quality_min;
    reg_rc->roi_qthd0.qpmax_area0 = h265->qpmax_map[0] > 0 ? h265->qpmax_map[0] : rc_cfg->quality_max;
    reg_rc->roi_qthd0.qpmin_area1 = h265->qpmin_map[1] > 0 ? h265->qpmin_map[1] : rc_cfg->quality_min;
    reg_rc->roi_qthd0.qpmax_area1 = h265->qpmax_map[1] > 0 ? h265->qpmax_map[1] : rc_cfg->quality_max;
    reg_rc->roi_qthd0.qpmin_area2 = h265->qpmin_map[2] > 0 ? h265->qpmin_map[2] : rc_cfg->quality_min;
    reg_rc->roi_qthd1.qpmax_area2 = h265->qpmax_map[2] > 0 ? h265->qpmax_map[2] : rc_cfg->quality_max;
    reg_rc->roi_qthd1.qpmin_area3 = h265->qpmin_map[3] > 0 ? h265->qpmin_map[3] : rc_cfg->quality_min;
    reg_rc->roi_qthd1.qpmax_area3 = h265->qpmax_map[3] > 0 ? h265->qpmax_map[3] : rc_cfg->quality_max;
    reg_rc->roi_qthd1.qpmin_area4 = h265->qpmin_map[4] > 0 ? h265->qpmin_map[4] : rc_cfg->quality_min;
    reg_rc->roi_qthd1.qpmax_area4 = h265->qpmax_map[4] > 0 ? h265->qpmax_map[4] : rc_cfg->quality_max;
    reg_rc->roi_qthd2.qpmin_area5 = h265->qpmin_map[5] > 0 ? h265->qpmin_map[5] : rc_cfg->quality_min;
    reg_rc->roi_qthd2.qpmax_area5 = h265->qpmax_map[5] > 0 ? h265->qpmax_map[5] : rc_cfg->quality_max;
    reg_rc->roi_qthd2.qpmin_area6 = h265->qpmin_map[6] > 0 ? h265->qpmin_map[6] : rc_cfg->quality_min;
    reg_rc->roi_qthd2.qpmax_area6 = h265->qpmax_map[6] > 0 ? h265->qpmax_map[6] : rc_cfg->quality_max;
    reg_rc->roi_qthd2.qpmin_area7 = h265->qpmin_map[7] > 0 ? h265->qpmin_map[7] : rc_cfg->quality_min;
    reg_rc->roi_qthd3.qpmax_area7 = h265->qpmax_map[7] > 0 ? h265->qpmax_map[7] : rc_cfg->quality_max;
}

static void vepu511_h265_set_quant_regs(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    MppEncHwCfg *hw = &ctx->cfg->hw;
    // H265eV511RegSet *regs = ctx->regs;
    H265eVepu511Param *s = &regs->reg_param;
    RK_U8 th0 = 3, th1 = 6, th2 = 13;
    RK_U16 bias_i0 = 171, bias_i1 = 171, bias_i2 = 171, bias_i3 = 171;
    RK_U16 bias_p0 = 85, bias_p1 = 85, bias_p2 = 85, bias_p3 = 85;
    RK_U32 frm_type = ctx->frame_type;

    if (!hw->qbias_en) {
        if (ctx->smart_en) {
            bias_i0 = bias_i1 = bias_i3 = 144;
            bias_i2 = (frm_type == INTRA_FRAME) ? 144 : 171;
        } else {
            bias_i0 = bias_i1 = bias_i3 = 171;
            bias_i2 = (frm_type == INTRA_FRAME) ? 171 : 220;
        }

        /* used for venc_info log */
        hw->qbias_arr[IFRAME_THD0] = hw->qbias_arr[PFRAME_THD0] = th0;
        hw->qbias_arr[IFRAME_THD1] = hw->qbias_arr[PFRAME_THD1] = th1;
        hw->qbias_arr[IFRAME_THD2] = hw->qbias_arr[PFRAME_THD2] = th2;
        hw->qbias_arr[IFRAME_BIAS0] = hw->qbias_arr[PFRAME_IBLK_BIAS0] = bias_i0;
        hw->qbias_arr[IFRAME_BIAS1] = hw->qbias_arr[PFRAME_IBLK_BIAS1] = bias_i1;
        hw->qbias_arr[IFRAME_BIAS2] = hw->qbias_arr[PFRAME_IBLK_BIAS2] = bias_i2;
        hw->qbias_arr[IFRAME_BIAS3] = hw->qbias_arr[PFRAME_IBLK_BIAS3] = bias_i3;
        hw->qbias_arr[PFRAME_PBLK_BIAS0] = bias_p0;
        hw->qbias_arr[PFRAME_PBLK_BIAS1] = bias_p1;
        hw->qbias_arr[PFRAME_PBLK_BIAS2] = bias_p2;
        hw->qbias_arr[PFRAME_PBLK_BIAS3] = bias_p3;
    } else {
        if (frm_type == INTRA_FRAME) {
            th0 = hw->qbias_arr[IFRAME_THD0];
            th1 = hw->qbias_arr[IFRAME_THD1];
            th2 = hw->qbias_arr[IFRAME_THD2];
            bias_i0 = hw->qbias_arr[IFRAME_BIAS0];
            bias_i1 = hw->qbias_arr[IFRAME_BIAS1];
            bias_i2 = hw->qbias_arr[IFRAME_BIAS2];
            bias_i3 = hw->qbias_arr[IFRAME_BIAS3];
        } else {
            th0 = hw->qbias_arr[PFRAME_THD0];
            th1 = hw->qbias_arr[PFRAME_THD1];
            th2 = hw->qbias_arr[PFRAME_THD2];
            bias_i0 = hw->qbias_arr[PFRAME_IBLK_BIAS0];
            bias_i1 = hw->qbias_arr[PFRAME_IBLK_BIAS1];
            bias_i2 = hw->qbias_arr[PFRAME_IBLK_BIAS2];
            bias_i3 = hw->qbias_arr[PFRAME_IBLK_BIAS3];
            bias_p0 = hw->qbias_arr[PFRAME_PBLK_BIAS0];
            bias_p1 = hw->qbias_arr[PFRAME_PBLK_BIAS1];
            bias_p2 = hw->qbias_arr[PFRAME_PBLK_BIAS2];
            bias_p3 = hw->qbias_arr[PFRAME_PBLK_BIAS3];
        }
    }

    s->bias_madi_thd_comb.bias_madi_th0 = th0;
    s->bias_madi_thd_comb.bias_madi_th1 = th1;
    s->bias_madi_thd_comb.bias_madi_th2 = th2;
    s->qnt0_i_bias_comb.bias_i_val0 = bias_i0;
    s->qnt0_i_bias_comb.bias_i_val1 = bias_i1;
    s->qnt0_i_bias_comb.bias_i_val2 = bias_i2;
    s->qnt1_i_bias_comb.bias_i_val3 = bias_i3;
    s->qnt0_p_bias_comb.bias_p_val0 = bias_p0;
    s->qnt0_p_bias_comb.bias_p_val1 = bias_p1;
    s->qnt0_p_bias_comb.bias_p_val2 = bias_p2;
    s->qnt1_p_bias_comb.bias_p_val3 = bias_p3;
}

static void vepu511_h265_set_atr_regs(H265eV511RegSet *regs)
{
    H265eVepu511Sqi *s = &regs->reg_sqi;
    RK_U32 str = 0;

    /* 0 - disable; 1 - weak; 2 - medium; 3 - strong */
    if (str == 0) {
        s->block_opt_cfg.block_en = 0; /* block_en and cmplx_en are not used so far(20240708) */
        s->cmplx_opt_cfg.cmplx_en = 0;
        s->line_opt_cfg.line_en = 0;
    } else {
        s->block_opt_cfg.block_en = 0;
        s->cmplx_opt_cfg.cmplx_en = 0;
        s->line_opt_cfg.line_en = 1;
    }

    s->subj_opt_cfg.subj_opt_en = 0;
    s->subj_opt_cfg.subj_opt_strength = 3;
    s->subj_opt_cfg.aq_subj_en = 0;
    s->subj_opt_cfg.aq_subj_strength = 4;
    s->subj_opt_cfg.bndry_cmplx_static_choose_en = 0;
    s->subj_opt_cfg.feature_cal_en = 0;
    s->subj_opt_dpth_thd.common_thre_num_grdn_point_dep0   = 64;
    s->subj_opt_dpth_thd.common_thre_num_grdn_point_dep1   = 32;
    s->subj_opt_dpth_thd.common_thre_num_grdn_point_dep2   = 16;

    if (str == 3) {
        s->block_opt_cfg.block_thre_cst_best_mad      = 1000;
        s->block_opt_cfg.block_thre_cst_best_grdn_blk = 39;
        s->block_opt_cfg.thre_num_grdnt_point_cmplx   = 3;
        s->block_opt_cfg.block_delta_qp_flag          = 3;

        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep0 = 4000;
        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep1 = 2000;

        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_mad_dep2       = 200;
        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_grdn_blk_dep0  = 977;

        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep1 = 0;
        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep2 = 488;

        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep0 = 4;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep1 = 30;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep2 = 30;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep0   = 7;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep1   = 6;

        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep0 = 1;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep1 = 50;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep2 = 50;

        s->subj_opt_dqp0.line_thre_qp   = 20;
        s->subj_opt_dqp0.block_strength = 4;
        s->subj_opt_dqp0.block_thre_qp  = 30;
        s->subj_opt_dqp0.cmplx_strength = 4;
        s->subj_opt_dqp0.cmplx_thre_qp  = 34;
        s->subj_opt_dqp0.cmplx_thre_max_grdn_blk = 32;
    } else if (str == 2) {
        s->block_opt_cfg.block_thre_cst_best_mad      = 1000;
        s->block_opt_cfg.block_thre_cst_best_grdn_blk = 39;
        s->block_opt_cfg.thre_num_grdnt_point_cmplx   = 3;
        s->block_opt_cfg.block_delta_qp_flag          = 3;

        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep0 = 4000;
        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep1 = 2000;

        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_mad_dep2      = 200;
        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_grdn_blk_dep0 = 977;

        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep1 = 0;
        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep2 = 488;

        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep0 = 3;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep1 = 20;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep2 = 20;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep0   = 7;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep1   = 8;

        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep0 = 1;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep1 = 60;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep2 = 60;

        s->subj_opt_dqp0.line_thre_qp            = 25;
        s->subj_opt_dqp0.block_strength          = 4;
        s->subj_opt_dqp0.block_thre_qp           = 30;
        s->subj_opt_dqp0.cmplx_strength          = 4;
        s->subj_opt_dqp0.cmplx_thre_qp           = 34;
        s->subj_opt_dqp0.cmplx_thre_max_grdn_blk = 32;
    } else {
        s->block_opt_cfg.block_thre_cst_best_mad      = 1000;
        s->block_opt_cfg.block_thre_cst_best_grdn_blk = 39;
        s->block_opt_cfg.thre_num_grdnt_point_cmplx   = 3;
        s->block_opt_cfg.block_delta_qp_flag          = 3;

        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep0 = 6000;
        s->cmplx_opt_cfg.cmplx_thre_cst_best_mad_dep1 = 2000;

        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_mad_dep2       = 300;
        s->cmplx_bst_mad_thd.cmplx_thre_cst_best_grdn_blk_dep0  = 1280;

        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep1 = 0;
        s->cmplx_bst_grdn_thd.cmplx_thre_cst_best_grdn_blk_dep2 = 512;

        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep0 = 3;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep1 = 20;
        s->line_opt_cfg.line_thre_min_cst_best_grdn_blk_dep2 = 20;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep0   = 7;
        s->line_opt_cfg.line_thre_ratio_best_grdn_blk_dep1   = 8;

        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep0 = 1;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep1 = 70;
        s->line_cst_bst_grdn.line_thre_max_cst_best_grdn_blk_dep2 = 70;

        s->subj_opt_dqp0.line_thre_qp            = 30;
        s->subj_opt_dqp0.block_strength          = 4;
        s->subj_opt_dqp0.block_thre_qp           = 30;
        s->subj_opt_dqp0.cmplx_strength          = 4;
        s->subj_opt_dqp0.cmplx_thre_qp           = 34;
        s->subj_opt_dqp0.cmplx_thre_max_grdn_blk = 32;
    }
}

static void vepu511_h265_set_smear_regs(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    H265eVepu511Sqi *s = &regs->reg_sqi;
    RK_S32 frm_num = ctx->frame_num;
    RK_S32 gop = (ctx->cfg->rc.gop > 0) ? ctx->cfg->rc.gop : 1; //TODO: gop = 0
    RK_U32 cover_num = ctx->last_frame_fb.acc_cover16_num;
    RK_U32 bndry_num = ctx->last_frame_fb.acc_bndry16_num;
    RK_U32 st_ctu_num = ctx->last_frame_fb.st_ctu_num;
    RK_S32 str = ctx->cfg->tune.deblur_str;
    RK_S16 flag_cover = 0;
    RK_S16 flag_bndry = 0;

    static RK_U8 qp_strength[H265E_SMEAR_STR_NUM] = { 4, 6, 7, 7, 3, 5, 7, 7 };
    static RK_U8 smear_strength[H265E_SMEAR_STR_NUM] = { 1, 1, 1, 1, 1, 1, 1, 1 };
    static RK_U8 bndry_intra_r_dep0[H265E_SMEAR_STR_NUM] = { 240, 240, 240, 240, 240, 240, 240, 240 };
    static RK_U8 bndry_intra_r_dep1[H265E_SMEAR_STR_NUM] = { 240, 240, 240, 240, 240, 240, 240, 240 };
    static RK_U8 thre_madp_stc_cover0[H265E_SMEAR_STR_NUM] = { 20, 22, 22, 22, 20, 22, 22, 30 };
    static RK_U8 thre_madp_stc_cover1[H265E_SMEAR_STR_NUM] = { 20, 22, 22, 22, 20, 22, 22, 30 };
    static RK_U8 thre_madp_mov_cover0[H265E_SMEAR_STR_NUM] = { 10, 9, 9, 9, 10, 9, 9, 6 };
    static RK_U8 thre_madp_mov_cover1[H265E_SMEAR_STR_NUM] = { 10, 9, 9, 9, 10, 9, 9, 6 };

    static RK_U8 flag_cover_thd0[H265E_SMEAR_STR_NUM] = { 12, 13, 13, 13, 12, 13, 13, 17 };
    static RK_U8 flag_cover_thd1[H265E_SMEAR_STR_NUM] = { 61, 70, 70, 70, 61, 70, 70, 90 };
    static RK_U8 flag_bndry_thd0[H265E_SMEAR_STR_NUM] = { 12, 12, 12, 12, 12, 12, 12, 12 };
    static RK_U8 flag_bndry_thd1[H265E_SMEAR_STR_NUM] = { 73, 73, 73, 73, 73, 73, 73, 73 };

    static RK_S8 flag_cover_wgt[3] = { 1, 0, -3 };
    static RK_S8 flag_bndry_wgt[3] = { 0, 0, 0 };
    static RK_S8 flag_bndry_intra_wgt0[3] = { -12, 0, 12 };
    static RK_S8 flag_bndry_intra_wgt1[3] = { -12, 0, 12 };

    flag_cover = (cover_num * 1000 < flag_cover_thd0[str] * st_ctu_num) ? 0 :
                 (cover_num * 1000 < flag_cover_thd1[str] * st_ctu_num) ? 1 : 2;

    flag_bndry = (bndry_num * 1000 < flag_bndry_thd0[str] * st_ctu_num) ? 0 :
                 (bndry_num * 1000 < flag_bndry_thd1[str] * st_ctu_num) ? 1 : 2;

    /* anti smear */
    s->smear_opt_cfg0.anti_smear_en = ctx->cfg->tune.deblur_en;
    s->smear_opt_cfg0.smear_strength = (smear_strength[str] > 2) ?
                                       (smear_strength[str] + flag_bndry_wgt[flag_bndry]) : smear_strength[str];

    s->smear_opt_cfg0.thre_mv_inconfor_cime       = 8;
    s->smear_opt_cfg0.thre_mv_confor_cime         = 2;
    s->smear_opt_cfg0.thre_mv_inconfor_cime_gmv   = 8;
    s->smear_opt_cfg0.thre_mv_confor_cime_gmv     = 2;
    s->smear_opt_cfg0.thre_num_mv_confor_cime     = 3;
    s->smear_opt_cfg0.thre_num_mv_confor_cime_gmv = 2;
    s->smear_opt_cfg0.frm_static                  = 1;

    s->smear_opt_cfg0.smear_load_en = ((frm_num % gop == 0) ||
                                       (s->smear_opt_cfg0.frm_static == 0) || (frm_num % gop == 1)) ? 0 : 1;
    s->smear_opt_cfg0.smear_stor_en = ((frm_num % gop == 0) ||
                                       (s->smear_opt_cfg0.frm_static == 0) || (frm_num % gop == gop - 1)) ? 0 : 1;
    s->smear_opt_cfg1.dist0_frm_avg               = 0;
    s->smear_opt_cfg1.thre_dsp_static             = 10;
    s->smear_opt_cfg1.thre_dsp_mov                = 15;
    s->smear_opt_cfg1.thre_dist_mv_confor_cime    = 32;

    s->smear_madp_thd.thre_madp_stc_dep0          = 10;
    s->smear_madp_thd.thre_madp_stc_dep1          = 8;
    s->smear_madp_thd.thre_madp_stc_dep2          = 8;
    s->smear_madp_thd.thre_madp_mov_dep0          = 16;
    s->smear_madp_thd.thre_madp_mov_dep1          = 18;
    s->smear_madp_thd.thre_madp_mov_dep2          = 20;

    s->smear_stat_thd.thre_num_pt_stc_dep0        = 47;
    s->smear_stat_thd.thre_num_pt_stc_dep1        = 11;
    s->smear_stat_thd.thre_num_pt_stc_dep2        = 3;
    s->smear_stat_thd.thre_num_pt_mov_dep0        = 47;
    s->smear_stat_thd.thre_num_pt_mov_dep1        = 11;
    s->smear_stat_thd.thre_num_pt_mov_dep2        = 3;

    s->smear_bmv_dist_thd0.confor_cime_gmv0      = 21;
    s->smear_bmv_dist_thd0.confor_cime_gmv1      = 16;
    s->smear_bmv_dist_thd0.inconfor_cime_gmv0    = 48;
    s->smear_bmv_dist_thd0.inconfor_cime_gmv1    = 34;

    s->smear_bmv_dist_thd1.inconfor_cime_gmv2    = 32;
    s->smear_bmv_dist_thd1.inconfor_cime_gmv3    = 29;
    s->smear_bmv_dist_thd1.inconfor_cime_gmv4    = 27;

    s->smear_min_bndry_gmv.thre_min_num_confor_csu0_bndry_cime_gmv      = 0;
    s->smear_min_bndry_gmv.thre_max_num_confor_csu0_bndry_cime_gmv      = 3;
    s->smear_min_bndry_gmv.thre_min_num_inconfor_csu0_bndry_cime_gmv    = 0;
    s->smear_min_bndry_gmv.thre_max_num_inconfor_csu0_bndry_cime_gmv    = 3;
    s->smear_min_bndry_gmv.thre_split_dep0                              = 2;
    s->smear_min_bndry_gmv.thre_zero_srgn                               = 8;
    s->smear_min_bndry_gmv.madi_thre_dep0                               = 22;
    s->smear_min_bndry_gmv.madi_thre_dep1                               = 18;

    s->smear_madp_cov_thd.thre_madp_stc_cover0    = thre_madp_stc_cover0[str];
    s->smear_madp_cov_thd.thre_madp_stc_cover1    = thre_madp_stc_cover1[str];
    s->smear_madp_cov_thd.thre_madp_mov_cover0    = thre_madp_mov_cover0[str];
    s->smear_madp_cov_thd.thre_madp_mov_cover1    = thre_madp_mov_cover1[str];
    s->smear_madp_cov_thd.smear_qp_strength       = qp_strength[str] +
                                                    flag_cover_wgt[flag_cover];
    s->smear_madp_cov_thd.smear_thre_qp           = 30;

    s->subj_opt_dqp1.bndry_rdo_mode_intra_jcoef_d0   = bndry_intra_r_dep0[str] +
                                                       flag_bndry_intra_wgt0[flag_bndry];
    s->subj_opt_dqp1.bndry_rdo_mode_intra_jcoef_d1   = bndry_intra_r_dep1[str] +
                                                       flag_bndry_intra_wgt1[flag_bndry];

    s->subj_opt_dqp1.skin_thre_qp = 31;
    s->subj_opt_dqp1.skin_thre_madp = 64;
    s->subj_opt_dqp1.bndry_rdo_mode_intra_jcoef_d0 = 15;
    s->subj_opt_dqp1.bndry_rdo_mode_intra_jcoef_d1 = 14;
    s->subj_opt_dqp1.smear_frame_thre_qp = 35;
    s->subj_opt_rdo_split.line_rdo_split_rcoef_d0 = 11;
    s->subj_opt_rdo_split.line_rdo_split_rcoef_d1 = 13;

    s->subj_opt_inrar_coef.cover_rmd_mode_intra_jcoef_d0 = 8;
    s->subj_opt_inrar_coef.cover_rmd_mode_intra_jcoef_d1 = 8;
    s->subj_opt_inrar_coef.cover_rdo_mode_intra_jcoef_d0 = 12;
    s->subj_opt_inrar_coef.cover_rdo_mode_intra_jcoef_d1 = 10;
    s->subj_opt_inrar_coef.cover_rdoq_rcoef_d0 = 7;
    s->subj_opt_inrar_coef.cover_rdoq_rcoef_d1 = 7;

    s->smear_opt_cfc_coef.cfc_rmd_mode_intra_jcoef_d0 = 20;
    s->smear_opt_cfc_coef.cfc_rmd_mode_intra_jcoef_d1 = 20;
    s->smear_opt_cfc_coef.cfc_rdo_mode_intra_jcoef_d0 = 20;
    s->smear_opt_cfc_coef.cfc_rdo_mode_intra_jcoef_d1 = 20;
    s->smear_opt_cfc_coef.cfc_rdoq_rcoef_d0 = 7;
    s->smear_opt_cfc_coef.cfc_rdoq_rcoef_d1 = 7;

    s->subj_opt_rdo_split.choose_cu32_split_jcoef = 20;
    s->subj_opt_rdo_split.choose_cu16_split_jcoef = 8;
}

static void vepu511_h265_set_anti_stripe_regs(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    H265eVepu511Sqi *s = &regs->reg_sqi;
    pre_cst_par* pre_i32 = (pre_cst_par*)&s->preintra32_cst;
    pre_cst_par* pre_i16 = (pre_cst_par*)&s->preintra16_cst;

    pre_i32->cst_wgt3.anti_strp_e = !!ctx->cfg->tune.atl_str;

    pre_i32->cst_madi_thd0.madi_thd0 = 5;
    pre_i32->cst_madi_thd0.madi_thd1 = 15;
    pre_i32->cst_madi_thd0.madi_thd2 = 5;
    pre_i32->cst_madi_thd0.madi_thd3 = 3;
    pre_i32->cst_madi_thd1.madi_thd4 = 3;
    pre_i32->cst_madi_thd1.madi_thd5 = 6;
    pre_i32->cst_madi_thd1.madi_thd6 = 7;
    pre_i32->cst_madi_thd1.madi_thd7 = 5;
    pre_i32->cst_madi_thd2.madi_thd8 = 10;
    pre_i32->cst_madi_thd2.madi_thd9 = 5;
    pre_i32->cst_madi_thd2.madi_thd10 = 7;
    pre_i32->cst_madi_thd2.madi_thd11 = 5;
    pre_i32->cst_madi_thd3.madi_thd12 = 7;
    pre_i32->cst_madi_thd3.madi_thd13 = 5;
    pre_i32->cst_madi_thd3.mode_th = 5;

    pre_i32->cst_wgt0.wgt0 = 20;
    pre_i32->cst_wgt0.wgt1 = 18;
    pre_i32->cst_wgt0.wgt2 = 19;
    pre_i32->cst_wgt0.wgt3 = 18;
    pre_i32->cst_wgt1.wgt4 = 12;
    pre_i32->cst_wgt1.wgt5 = 6;
    pre_i32->cst_wgt1.wgt6 = 13;
    pre_i32->cst_wgt1.wgt7 = 9;
    pre_i32->cst_wgt2.wgt8 = 12;
    pre_i32->cst_wgt2.wgt9 = 6;
    pre_i32->cst_wgt2.wgt10 = 13;
    pre_i32->cst_wgt2.wgt11 = 9;
    pre_i32->cst_wgt3.wgt12 = 18;
    pre_i32->cst_wgt3.wgt13 = 17;
    pre_i32->cst_wgt3.wgt14 = 17;

    pre_i16->cst_madi_thd0.madi_thd0 = 5;
    pre_i16->cst_madi_thd0.madi_thd1 = 15;
    pre_i16->cst_madi_thd0.madi_thd2 = 5;
    pre_i16->cst_madi_thd0.madi_thd3 = 3;
    pre_i16->cst_madi_thd1.madi_thd4 = 3;
    pre_i16->cst_madi_thd1.madi_thd5 = 6;
    pre_i16->cst_madi_thd1.madi_thd6 = 7;
    pre_i16->cst_madi_thd1.madi_thd7 = 5;
    pre_i16->cst_madi_thd2.madi_thd8 = 10;
    pre_i16->cst_madi_thd2.madi_thd9 = 5;
    pre_i16->cst_madi_thd2.madi_thd10 = 7;
    pre_i16->cst_madi_thd2.madi_thd11 = 5;
    pre_i16->cst_madi_thd3.madi_thd12 = 7;
    pre_i16->cst_madi_thd3.madi_thd13 = 5;
    pre_i16->cst_madi_thd3.mode_th = 5;

    pre_i16->cst_wgt0.wgt0 = 20;
    pre_i16->cst_wgt0.wgt1 = 18;
    pre_i16->cst_wgt0.wgt2 = 19;
    pre_i16->cst_wgt0.wgt3 = 18;
    pre_i16->cst_wgt1.wgt4 = 12;
    pre_i16->cst_wgt1.wgt5 = 6;
    pre_i16->cst_wgt1.wgt6 = 13;
    pre_i16->cst_wgt1.wgt7 = 9;
    pre_i16->cst_wgt2.wgt8 = 12;
    pre_i16->cst_wgt2.wgt9 = 6;
    pre_i16->cst_wgt2.wgt10 = 13;
    pre_i16->cst_wgt2.wgt11 = 9;
    pre_i16->cst_wgt3.wgt12 = 18;
    pre_i16->cst_wgt3.wgt13 = 17;
    pre_i16->cst_wgt3.wgt14 = 17;

    pre_i32->cst_madi_thd3.qp_thd = 28;
    pre_i32->cst_wgt3.lambda_mv_bit_0 = 5; // lv32
    pre_i32->cst_wgt3.lambda_mv_bit_1 = 4; // lv16
    pre_i16->cst_wgt3.lambda_mv_bit_0 = 4; // lv8
    pre_i16->cst_wgt3.lambda_mv_bit_1 = 3; // lv4
}

static MPP_RET vepu511_h265_set_rdo_regs(H265eV511RegSet *regs)
{
    Vepu511RcRoi *reg_rc = &regs->reg_rc_roi;

    reg_rc->cudecis_thd0.base_thre_rough_mad32_intra           = 9;
    reg_rc->cudecis_thd0.delta0_thre_rough_mad32_intra         = 10;
    reg_rc->cudecis_thd0.delta1_thre_rough_mad32_intra         = 55;
    reg_rc->cudecis_thd0.delta2_thre_rough_mad32_intra         = 55;
    reg_rc->cudecis_thd0.delta3_thre_rough_mad32_intra         = 66;
    reg_rc->cudecis_thd0.delta4_thre_rough_mad32_intra_low5    = 2;

    reg_rc->cudecis_thd1.delta4_thre_rough_mad32_intra_high2   = 2;
    reg_rc->cudecis_thd1.delta5_thre_rough_mad32_intra         = 74;
    reg_rc->cudecis_thd1.delta6_thre_rough_mad32_intra         = 106;
    reg_rc->cudecis_thd1.base_thre_fine_mad32_intra            = 8;
    reg_rc->cudecis_thd1.delta0_thre_fine_mad32_intra          = 0;
    reg_rc->cudecis_thd1.delta1_thre_fine_mad32_intra          = 13;
    reg_rc->cudecis_thd1.delta2_thre_fine_mad32_intra_low3     = 6;

    reg_rc->cudecis_thd2.delta2_thre_fine_mad32_intra_high2    = 1;
    reg_rc->cudecis_thd2.delta3_thre_fine_mad32_intra          = 17;
    reg_rc->cudecis_thd2.delta4_thre_fine_mad32_intra          = 23;
    reg_rc->cudecis_thd2.delta5_thre_fine_mad32_intra          = 50;
    reg_rc->cudecis_thd2.delta6_thre_fine_mad32_intra          = 54;
    reg_rc->cudecis_thd2.base_thre_str_edge_mad32_intra        = 6;
    reg_rc->cudecis_thd2.delta0_thre_str_edge_mad32_intra      = 0;
    reg_rc->cudecis_thd2.delta1_thre_str_edge_mad32_intra      = 0;

    reg_rc->cudecis_thd3.delta2_thre_str_edge_mad32_intra      = 3;
    reg_rc->cudecis_thd3.delta3_thre_str_edge_mad32_intra      = 8;
    reg_rc->cudecis_thd3.base_thre_str_edge_bgrad32_intra      = 25;
    reg_rc->cudecis_thd3.delta0_thre_str_edge_bgrad32_intra    = 0;
    reg_rc->cudecis_thd3.delta1_thre_str_edge_bgrad32_intra    = 0;
    reg_rc->cudecis_thd3.delta2_thre_str_edge_bgrad32_intra    = 7;
    reg_rc->cudecis_thd3.delta3_thre_str_edge_bgrad32_intra    = 19;
    reg_rc->cudecis_thd3.base_thre_mad16_intra                 = 6;
    reg_rc->cudecis_thd3.delta0_thre_mad16_intra               = 0;

    reg_rc->cudecis_thd4.delta1_thre_mad16_intra          = 3;
    reg_rc->cudecis_thd4.delta2_thre_mad16_intra          = 3;
    reg_rc->cudecis_thd4.delta3_thre_mad16_intra          = 24;
    reg_rc->cudecis_thd4.delta4_thre_mad16_intra          = 28;
    reg_rc->cudecis_thd4.delta5_thre_mad16_intra          = 40;
    reg_rc->cudecis_thd4.delta6_thre_mad16_intra          = 52;
    reg_rc->cudecis_thd4.delta0_thre_mad16_ratio_intra    = 7;

    reg_rc->cudecis_thd5.delta1_thre_mad16_ratio_intra           =  7;
    reg_rc->cudecis_thd5.delta2_thre_mad16_ratio_intra           =  2;
    reg_rc->cudecis_thd5.delta3_thre_mad16_ratio_intra           =  2;
    reg_rc->cudecis_thd5.delta4_thre_mad16_ratio_intra           =  0;
    reg_rc->cudecis_thd5.delta5_thre_mad16_ratio_intra           =  0;
    reg_rc->cudecis_thd5.delta6_thre_mad16_ratio_intra           =  0;
    reg_rc->cudecis_thd5.delta7_thre_mad16_ratio_intra           =  4;
    reg_rc->cudecis_thd5.delta0_thre_rough_bgrad32_intra         =  1;
    reg_rc->cudecis_thd5.delta1_thre_rough_bgrad32_intra         =  5;
    reg_rc->cudecis_thd5.delta2_thre_rough_bgrad32_intra_low4    =  8;

    reg_rc->cudecis_thd6.delta2_thre_rough_bgrad32_intra_high2    = 2;
    reg_rc->cudecis_thd6.delta3_thre_rough_bgrad32_intra          = 540;
    reg_rc->cudecis_thd6.delta4_thre_rough_bgrad32_intra          = 692;
    reg_rc->cudecis_thd6.delta5_thre_rough_bgrad32_intra_low10    = 866;

    reg_rc->cudecis_thd7.delta5_thre_rough_bgrad32_intra_high1   = 1;
    reg_rc->cudecis_thd7.delta6_thre_rough_bgrad32_intra         = 3286;
    reg_rc->cudecis_thd7.delta7_thre_rough_bgrad32_intra         = 6620;
    reg_rc->cudecis_thd7.delta0_thre_bgrad16_ratio_intra         = 8;
    reg_rc->cudecis_thd7.delta1_thre_bgrad16_ratio_intra_low2    = 3;

    reg_rc->cudecis_thd8.delta1_thre_bgrad16_ratio_intra_high2    = 2;
    reg_rc->cudecis_thd8.delta2_thre_bgrad16_ratio_intra          = 15;
    reg_rc->cudecis_thd8.delta3_thre_bgrad16_ratio_intra          = 15;
    reg_rc->cudecis_thd8.delta4_thre_bgrad16_ratio_intra          = 13;
    reg_rc->cudecis_thd8.delta5_thre_bgrad16_ratio_intra          = 13;
    reg_rc->cudecis_thd8.delta6_thre_bgrad16_ratio_intra          = 7;
    reg_rc->cudecis_thd8.delta7_thre_bgrad16_ratio_intra          = 15;
    reg_rc->cudecis_thd8.delta0_thre_fme_ratio_inter              = 4;
    reg_rc->cudecis_thd8.delta1_thre_fme_ratio_inter              = 4;

    reg_rc->cudecis_thd9.delta2_thre_fme_ratio_inter    = 3;
    reg_rc->cudecis_thd9.delta3_thre_fme_ratio_inter    = 2;
    reg_rc->cudecis_thd9.delta4_thre_fme_ratio_inter    = 0;
    reg_rc->cudecis_thd9.delta5_thre_fme_ratio_inter    = 0;
    reg_rc->cudecis_thd9.delta6_thre_fme_ratio_inter    = 0;
    reg_rc->cudecis_thd9.delta7_thre_fme_ratio_inter    = 0;
    reg_rc->cudecis_thd9.base_thre_fme32_inter          = 4;
    reg_rc->cudecis_thd9.delta0_thre_fme32_inter        = 2;
    reg_rc->cudecis_thd9.delta1_thre_fme32_inter        = 7;
    reg_rc->cudecis_thd9.delta2_thre_fme32_inter        = 12;

    reg_rc->cudecis_thd10.delta3_thre_fme32_inter    = 23;
    reg_rc->cudecis_thd10.delta4_thre_fme32_inter    = 41;
    reg_rc->cudecis_thd10.delta5_thre_fme32_inter    = 71;
    reg_rc->cudecis_thd10.delta6_thre_fme32_inter    = 123;
    reg_rc->cudecis_thd10.thre_cme32_inter           = 48;

    reg_rc->cudecis_thd11.delta0_thre_mad_fme_ratio_inter    = 0;
    reg_rc->cudecis_thd11.delta1_thre_mad_fme_ratio_inter    = 7;
    reg_rc->cudecis_thd11.delta2_thre_mad_fme_ratio_inter    = 7;
    reg_rc->cudecis_thd11.delta3_thre_mad_fme_ratio_inter    = 6;
    reg_rc->cudecis_thd11.delta4_thre_mad_fme_ratio_inter    = 5;
    reg_rc->cudecis_thd11.delta5_thre_mad_fme_ratio_inter    = 4;
    reg_rc->cudecis_thd11.delta6_thre_mad_fme_ratio_inter    = 4;
    reg_rc->cudecis_thd11.delta7_thre_mad_fme_ratio_inter    = 4;

    reg_rc->cudecis_thd12.delta0_thre_mad_fme_ratio_inter    = 1;
    reg_rc->cudecis_thd12.delta1_thre_mad_fme_ratio_inter    = 3;
    reg_rc->cudecis_thd12.delta2_thre_mad_fme_ratio_inter    = 6;
    reg_rc->cudecis_thd12.delta3_thre_mad_fme_ratio_inter    = 9;
    reg_rc->cudecis_thd12.delta4_thre_mad_fme_ratio_inter    = 10;
    reg_rc->cudecis_thd12.delta5_thre_mad_fme_ratio_inter    = 11;
    reg_rc->cudecis_thd12.delta6_thre_mad_fme_ratio_inter    = 12;
    reg_rc->cudecis_thd12.delta7_thre_mad_fme_ratio_inter    = 15;

    return MPP_OK;
}

static void vepu511_h265_set_sao_regs(H265eV511RegSet *regs)
{
    H265eVepu511Sqi *sqi = &regs->reg_sqi;

    /* Weight values are set to 4 to disable SAO subjective optimization.
     * They are not under the control of anti_blur_en.
     */
    sqi->subj_anti_blur_wgt3.merge_cost_dist_eo_wgt0 = 4;
    sqi->subj_anti_blur_wgt3.merge_cost_dist_bo_wgt0 = 4;
    sqi->subj_anti_blur_wgt4.merge_cost_dist_eo_wgt1 = 4;
    sqi->subj_anti_blur_wgt4.merge_cost_dist_bo_wgt1 = 4;
    sqi->subj_anti_blur_wgt4.merge_cost_bit_eo_wgt0 = 4;
    sqi->subj_anti_blur_wgt4.merge_cost_bit_bo_wgt0 = 4;
}

static void vepu511_h265_set_slice_regs(H265eSyntax_new *syn, H265eVepu511Frame *regs)
{
    regs->synt_sps.smpl_adpt_ofst_e     = syn->pp.sample_adaptive_offset_enabled_flag;
    regs->synt_sps.num_st_ref_pic       = syn->pp.num_short_term_ref_pic_sets;
    regs->synt_sps.num_lt_ref_pic       = syn->pp.num_long_term_ref_pics_sps;
    regs->synt_sps.lt_ref_pic_prsnt     = syn->pp.long_term_ref_pics_present_flag;
    regs->synt_sps.tmpl_mvp_e           = syn->pp.sps_temporal_mvp_enabled_flag;
    regs->synt_sps.log2_max_poc_lsb     = syn->pp.log2_max_pic_order_cnt_lsb_minus4;
    regs->synt_sps.strg_intra_smth      = syn->pp.strong_intra_smoothing_enabled_flag;

    regs->synt_pps.dpdnt_sli_seg_en     = syn->pp.dependent_slice_segments_enabled_flag;
    regs->synt_pps.out_flg_prsnt_flg    = syn->pp.output_flag_present_flag;
    regs->synt_pps.num_extr_sli_hdr     = syn->pp.num_extra_slice_header_bits;
    regs->synt_pps.sgn_dat_hid_en       = syn->pp.sign_data_hiding_enabled_flag;
    regs->synt_pps.cbc_init_prsnt_flg   = syn->pp.cabac_init_present_flag;
    regs->synt_pps.pic_init_qp          = syn->pp.init_qp_minus26 + 26;
    regs->synt_pps.cu_qp_dlt_en         = syn->pp.cu_qp_delta_enabled_flag;
    regs->synt_pps.chrm_qp_ofst_prsn    = syn->pp.pps_slice_chroma_qp_offsets_present_flag;
    regs->synt_pps.lp_fltr_acrs_sli     = syn->pp.pps_loop_filter_across_slices_enabled_flag;
    regs->synt_pps.dblk_fltr_ovrd_en    = syn->pp.deblocking_filter_override_enabled_flag;
    regs->synt_pps.lst_mdfy_prsnt_flg   = syn->pp.lists_modification_present_flag;
    regs->synt_pps.sli_seg_hdr_extn     = syn->pp.slice_segment_header_extension_present_flag;
    regs->synt_pps.cu_qp_dlt_depth      = syn->pp.diff_cu_qp_delta_depth;
    regs->synt_pps.lpf_fltr_acrs_til    = syn->pp.loop_filter_across_tiles_enabled_flag;

    regs->synt_sli0.cbc_init_flg        = syn->sp.cbc_init_flg;
    regs->synt_sli0.mvd_l1_zero_flg     = syn->sp.mvd_l1_zero_flg;
    regs->synt_sli0.ref_pic_lst_mdf_l0  = syn->sp.ref_pic_lst_mdf_l0;

    regs->synt_sli0.num_refidx_l1_act   = syn->sp.num_refidx_l1_act;
    regs->synt_sli0.num_refidx_l0_act   = syn->sp.num_refidx_l0_act;

    regs->synt_sli0.num_refidx_act_ovrd = syn->sp.num_refidx_act_ovrd;

    regs->synt_sli0.sli_sao_chrm_flg    = syn->sp.sli_sao_chrm_flg;
    regs->synt_sli0.sli_sao_luma_flg    = syn->sp.sli_sao_luma_flg;
    regs->synt_sli0.sli_tmprl_mvp_e     = syn->sp.sli_tmprl_mvp_en;
    regs->common.enc_pic.num_pic_tot_cur_hevc  = syn->sp.tot_poc_num;

    regs->synt_sli0.pic_out_flg         = syn->sp.pic_out_flg;
    regs->synt_sli0.sli_type            = syn->sp.slice_type;
    regs->synt_sli0.sli_rsrv_flg        = syn->sp.slice_rsrv_flg;
    regs->synt_sli0.dpdnt_sli_seg_flg   = syn->sp.dpdnt_sli_seg_flg;
    regs->synt_sli0.sli_pps_id          = syn->sp.sli_pps_id;
    regs->synt_sli0.no_out_pri_pic      = syn->sp.no_out_pri_pic;


    regs->synt_sli1.sp_tc_ofst_div2       = syn->sp.sli_tc_ofst_div2;;
    regs->synt_sli1.sp_beta_ofst_div2     = syn->sp.sli_beta_ofst_div2;
    regs->synt_sli1.sli_lp_fltr_acrs_sli  = syn->sp.sli_lp_fltr_acrs_sli;
    regs->synt_sli1.sp_dblk_fltr_dis      = syn->sp.sli_dblk_fltr_dis;
    regs->synt_sli1.dblk_fltr_ovrd_flg    = syn->sp.dblk_fltr_ovrd_flg;
    regs->synt_sli1.sli_cb_qp_ofst = syn->pp.pps_slice_chroma_qp_offsets_present_flag ?
                                     syn->sp.sli_cb_qp_ofst : syn->pp.pps_cb_qp_offset;
    regs->synt_sli1.max_mrg_cnd           = syn->sp.max_mrg_cnd;

    regs->synt_sli1.col_ref_idx           = syn->sp.col_ref_idx;
    regs->synt_sli1.col_frm_l0_flg        = syn->sp.col_frm_l0_flg;
    regs->synt_sli2.sli_poc_lsb           = syn->sp.sli_poc_lsb;
    regs->synt_sli2.sli_hdr_ext_len       = syn->sp.sli_hdr_ext_len;
}

static void vepu511_h265_set_ref_regs(H265eSyntax_new *syn, H265eVepu511Frame *regs)
{
    regs->synt_refm0.st_ref_pic_flg = syn->sp.st_ref_pic_flg;
    regs->synt_refm0.poc_lsb_lt0 = syn->sp.poc_lsb_lt0;
    regs->synt_refm0.num_lt_pic = syn->sp.num_lt_pic;

    regs->synt_refm1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_refm1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_refm1.used_by_lt_flg0 = syn->sp.used_by_lt_flg0;
    regs->synt_refm1.used_by_lt_flg1 = syn->sp.used_by_lt_flg1;
    regs->synt_refm1.used_by_lt_flg2 = syn->sp.used_by_lt_flg2;
    regs->synt_refm1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_refm1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_refm1.dlt_poc_msb_prsnt1 = syn->sp.dlt_poc_msb_prsnt1;
    regs->synt_refm1.num_negative_pics = syn->sp.num_neg_pic;
    regs->synt_refm1.num_pos_pic = syn->sp.num_pos_pic;

    regs->synt_refm1.used_by_s0_flg = syn->sp.used_by_s0_flg;
    regs->synt_refm2.dlt_poc_s0_m10 = syn->sp.dlt_poc_s0_m10;
    regs->synt_refm2.dlt_poc_s0_m11 = syn->sp.dlt_poc_s0_m11;
    regs->synt_refm3.dlt_poc_s0_m12 = syn->sp.dlt_poc_s0_m12;
    regs->synt_refm3.dlt_poc_s0_m13 = syn->sp.dlt_poc_s0_m13;

    regs->synt_long_refm0.poc_lsb_lt1 = syn->sp.poc_lsb_lt1;
    regs->synt_long_refm1.dlt_poc_msb_cycl1 = syn->sp.dlt_poc_msb_cycl1;
    regs->synt_long_refm0.poc_lsb_lt2 = syn->sp.poc_lsb_lt2;
    regs->synt_refm1.dlt_poc_msb_prsnt2 = syn->sp.dlt_poc_msb_prsnt2;
    regs->synt_long_refm1.dlt_poc_msb_cycl2 = syn->sp.dlt_poc_msb_cycl2;
    regs->synt_sli1.lst_entry_l0 = syn->sp.lst_entry_l0;
    regs->synt_sli0.ref_pic_lst_mdf_l0 = syn->sp.ref_pic_lst_mdf_l0;

    return;
}

static void vepu511_h265_set_atf_regs(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    H265eVepu511Sqi *reg = &regs->reg_sqi;
    RK_U32 str = ctx->cfg->tune.atf_str;
    rdo_b32_skip_par *p_rdo_b32_skip = NULL;
    rdo_b32_noskip_par *p_rdo_b32_noskip = NULL;
    rdo_skip_par *p_rdo_skip = NULL;
    rdo_noskip_par *p_rdo_noskip = NULL;

    static RK_U16 b32_skip_thd2[4] = { 15, 15, 15, 200 };
    static RK_U16 b32_skip_thd3[4] = { 72, 72, 72, 1000 };
    static RK_U8 b32_skip_wgt0[4] = { 16, 20, 20, 16 };
    static RK_U8 b32_skip_wgt3[4] = { 16, 16, 16, 17 };
    static RK_U16 b16_skip_thd2[4] = { 15, 15, 15, 200 };
    static RK_U16 b16_skip_thd3[4] = { 25, 25, 25, 1000 };
    static RK_U8 b16_skip_wgt0[4] = { 16, 20, 20, 16 };
    static RK_U8 b16_skip_wgt3[4] = { 16, 16, 16, 17 };
    static RK_U16 b32_intra_thd0[4] = { 20, 20, 20, 24 };
    static RK_U16 b32_intra_thd1[4] = { 40, 40, 40, 48 };
    static RK_U16 b32_intra_thd2[4] = { 60, 72, 72, 96 };
    static RK_U8 b32_intra_wgt0[4] = { 16, 22, 27, 28 };
    static RK_U8 b32_intra_wgt1[4] = { 16, 20, 25, 26 };
    static RK_U8 b32_intra_wgt2[4] = { 16, 18, 20, 24 };
    static RK_U16 b16_intra_thd0[4] = { 20, 20, 20, 24 };
    static RK_U16 b16_intra_thd1[4] = { 40, 40, 40, 48 };
    static RK_U16 b16_intra_thd2[4] = { 60, 72, 72, 96 };
    static RK_U8 b16_intra_wgt0[4] = { 16, 22, 27, 28 };
    static RK_U8 b16_intra_wgt1[4] = { 16, 20, 25, 26 };
    static RK_U8 b16_intra_wgt2[4] = { 16, 18, 20, 24 };

    regs->reg_frm.rdo_cfg.atf_e = !!str;

    p_rdo_b32_skip = &reg->rdo_b32_skip;
    p_rdo_b32_skip->atf_thd0.madp_thd0 = 5;
    p_rdo_b32_skip->atf_thd0.madp_thd1 = 10;
    p_rdo_b32_skip->atf_thd1.madp_thd2 = b32_skip_thd2[str];
    p_rdo_b32_skip->atf_thd1.madp_thd3 = b32_skip_thd3[str];
    p_rdo_b32_skip->atf_wgt0.wgt0 = b32_skip_wgt0[str];
    p_rdo_b32_skip->atf_wgt0.wgt1 = 16;
    p_rdo_b32_skip->atf_wgt0.wgt2 = 16;
    p_rdo_b32_skip->atf_wgt0.wgt3 = b32_skip_wgt3[str];
    p_rdo_b32_skip->atf_thd0.flckr_frame_qp_en = 1;
    p_rdo_b32_skip->atf_thd0.flckr_lgt_chng_en = 1;

    p_rdo_b32_noskip = &reg->rdo_b32_inter;
    p_rdo_b32_noskip->atf_thd0.madp_thd0 = 20;
    p_rdo_b32_noskip->atf_thd0.madp_thd1 = 40;
    p_rdo_b32_noskip->atf_thd1.madp_thd2 = 72;
    p_rdo_b32_noskip->atf_wgt.wgt0 = 16;
    p_rdo_b32_noskip->atf_wgt.wgt1 = 16;
    p_rdo_b32_noskip->atf_wgt.wgt2 = 16;

    p_rdo_noskip = &reg->rdo_b32_intra;
    p_rdo_noskip->ratf_thd0.madp_thd0 = b32_intra_thd0[str];
    p_rdo_noskip->ratf_thd0.madp_thd1 = b32_intra_thd1[str];
    p_rdo_noskip->ratf_thd1.madp_thd2 = b32_intra_thd2[str];
    p_rdo_noskip->atf_wgt.wgt0 = b32_intra_wgt0[str];
    p_rdo_noskip->atf_wgt.wgt1 = b32_intra_wgt1[str];
    p_rdo_noskip->atf_wgt.wgt2 = b32_intra_wgt2[str];

    p_rdo_skip = &reg->rdo_b16_skip;
    p_rdo_skip->atf_thd0.madp_thd0 = 1;
    p_rdo_skip->atf_thd0.madp_thd1 = 10;
    p_rdo_skip->atf_thd1.madp_thd2 = b16_skip_thd2[str];
    p_rdo_skip->atf_thd1.madp_thd3 = b16_skip_thd3[str];
    p_rdo_skip->atf_wgt0.wgt0 = b16_skip_wgt0[str];
    p_rdo_skip->atf_wgt0.wgt1 = 16;
    p_rdo_skip->atf_wgt0.wgt2 = 16;
    p_rdo_skip->atf_wgt0.wgt3 = b16_skip_wgt3[str];

    p_rdo_noskip = &reg->rdo_b16_inter;
    p_rdo_noskip->ratf_thd0.madp_thd0 = 20;
    p_rdo_noskip->ratf_thd0.madp_thd1 = 40;
    p_rdo_noskip->ratf_thd1.madp_thd2 = 72;
    p_rdo_noskip->atf_wgt.wgt0 = 16;
    p_rdo_noskip->atf_wgt.wgt1 = 16;
    p_rdo_noskip->atf_wgt.wgt2 = 16;
    p_rdo_noskip->atf_wgt.wgt3 = 16;

    p_rdo_noskip = &reg->rdo_b16_intra;
    p_rdo_noskip->ratf_thd0.madp_thd0 = b16_intra_thd0[str];
    p_rdo_noskip->ratf_thd0.madp_thd1 = b16_intra_thd1[str];
    p_rdo_noskip->ratf_thd1.madp_thd2 = b16_intra_thd2[str];
    p_rdo_noskip->atf_wgt.wgt0 = b16_intra_wgt0[str];
    p_rdo_noskip->atf_wgt.wgt1 = b16_intra_wgt1[str];
    p_rdo_noskip->atf_wgt.wgt2 = b16_intra_wgt2[str];
    p_rdo_noskip->atf_wgt.wgt3 = 16;
}

static void vepu511_h265_set_aq(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    MppEncHwCfg *hw = &ctx->cfg->hw;
    Vepu511RcRoi *rc_regs =  &regs->reg_rc_roi;
    RK_U8* thd = (RK_U8*)&rc_regs->aq_tthd0;
    RK_S32 *aq_step, *aq_rnge;
    RK_U32 *aq_thd;
    RK_U32 i;

    if (ctx->frame_type == INTRA_FRAME) {
        aq_thd = &hw->aq_thrd_i[0];
        aq_step = &hw->aq_step_i[0];
        aq_rnge = &hw->aq_rnge_arr[0];
    } else {
        aq_thd = &hw->aq_thrd_p[0];
        aq_step = &hw->aq_step_p[0];
        aq_rnge = &hw->aq_rnge_arr[5];
    }

    rc_regs->aq_stp0.aq_stp_s0 = aq_step[0] & 0x1f;
    rc_regs->aq_stp0.aq_stp_0t1 = aq_step[1] & 0x1f;
    rc_regs->aq_stp0.aq_stp_1t2 = aq_step[2] & 0x1f;
    rc_regs->aq_stp0.aq_stp_2t3 = aq_step[3] & 0x1f;
    rc_regs->aq_stp0.aq_stp_3t4 = aq_step[4] & 0x1f;
    rc_regs->aq_stp0.aq_stp_4t5 = aq_step[5] & 0x1f;
    rc_regs->aq_stp1.aq_stp_5t6 = aq_step[6] & 0x1f;
    rc_regs->aq_stp1.aq_stp_6t7 = aq_step[7] & 0x1f;
    rc_regs->aq_stp1.aq_stp_7t8 = 0;
    rc_regs->aq_stp1.aq_stp_8t9 = aq_step[8] & 0x1f;
    rc_regs->aq_stp1.aq_stp_9t10 = aq_step[9] & 0x1f;
    rc_regs->aq_stp1.aq_stp_10t11 = aq_step[10] & 0x1f;
    rc_regs->aq_stp2.aq_stp_11t12 = aq_step[11] & 0x1f;
    rc_regs->aq_stp2.aq_stp_12t13 = aq_step[12] & 0x1f;
    rc_regs->aq_stp2.aq_stp_13t14 = aq_step[13] & 0x1f;
    rc_regs->aq_stp2.aq_stp_14t15 = aq_step[14] & 0x1f;
    rc_regs->aq_stp2.aq_stp_b15 = aq_step[15];

    for (i = 0; i < 16; i++)
        thd[i] = aq_thd[i];

    rc_regs->aq_clip.aq16_rnge = aq_rnge[0];
    rc_regs->aq_clip.aq32_rnge = aq_rnge[1];
    rc_regs->aq_clip.aq8_rnge = aq_rnge[2];
    rc_regs->aq_clip.aq16_dif0 = aq_rnge[3];
    rc_regs->aq_clip.aq16_dif1 = aq_rnge[4];

    rc_regs->aq_clip.aq_rme_en = 1;
    rc_regs->aq_clip.aq_cme_en = 1;
}

static void vepu511_h265_global_cfg_set(H265eV511HalContext *ctx, H265eV511RegSet *regs)
{
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    H265eVepu511Param *reg_param = &regs->reg_param;
    RK_S32 lambda_idx_p = ctx->cfg->tune.lambda_idx_i;

    reg_frm->sao_cfg.sao_lambda_multi = ctx->cfg->codec.h265.sao_cfg.sao_bit_ratio;

    if (ctx->frame_type == INTRA_FRAME) {
        memcpy(&reg_param->pprd_lamb_satd_0_51[0], lambda_tbl_pre_intra, sizeof(lambda_tbl_pre_intra));
    } else {
        memcpy(&reg_param->pprd_lamb_satd_0_51[0], lambda_tbl_pre_inter, sizeof(lambda_tbl_pre_inter));
    }

    {
        RK_U32 *lambda_tbl;

        if (ctx->frame_type == INTRA_FRAME) {
            lambda_tbl = &rdo_lambda_table_I[lambda_idx_p];
        } else {
            lambda_idx_p = ctx->cfg->tune.lambda_idx_p;
            lambda_tbl = &rdo_lambda_table_P[lambda_idx_p];
        }

        memcpy(&reg_param->rdo_wgta_qp_grpa_0_51[0], lambda_tbl, H265E_LAMBDA_TAB_SIZE);
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
    regs->reg_param.prmd_intra_lamb_ofst.lambda_luma_offset = 11;
    regs->reg_param.prmd_intra_lamb_ofst.lambda_chroma_offset = 11;

}

MPP_RET hal_h265e_vepu511_gen_regs(void *hal, HalEncTask *task)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    Vepu511H265eFrmCfg *frm_cfg = ctx->frm;
    H265eV511RegSet *regs = frm_cfg->regs_set;
    MPP_RET ret = MPP_OK;

    HalEncTask *enc_task = task;
    H265eSyntax_new *syn = ctx->syn;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    EncFrmStatus *frm = &task->rc_task->frm;

    hal_h265e_enter();

    hal_h265e_dbg_simple("frame %d | type %d | start gen regs11",
                         ctx->frame_num, ctx->frame_type);

    memset(regs, 0, sizeof(H265eV511RegSet));

    vepu511_h265_set_normal(ctx, regs);
    vepu511_h265_set_prep(ctx, task, regs);
    vepu511_h265_set_me_regs(ctx, syn , regs);
    vepu511_h265_set_split(regs, ctx->cfg);
    vepu511_h265_set_hw_address(ctx, reg_frm, task);
    vepu511_h265_set_pp_regs(regs, fmt, &ctx->cfg->prep, task);
    vepu511_h265_set_vsp_filtering(ctx, regs);
    vepu511_h265_set_rc_regs(ctx, regs, task);
    vepu511_h265_set_rdo_regs(regs);
    vepu511_h265_set_quant_regs(ctx, regs);
    vepu511_h265_set_sao_regs(regs);
    vepu511_h265_set_slice_regs(syn, reg_frm);
    vepu511_h265_set_ref_regs(syn, reg_frm);

    ret = vepu511_h265_set_patch_info(syn, (Vepu541Fmt)fmt->format, ctx->reg_cfg, enc_task);
    if (ret)
        return ret;

    setup_vepu511_ext_line_buf(ctx, regs);
    vepu511_h265_set_atf_regs(ctx, regs);
    vepu511_h265_set_anti_stripe_regs(ctx, regs);
    vepu511_h265_set_atr_regs(regs);
    vepu511_h265_set_smear_regs(ctx, regs);
    vepu511_h265_set_scaling_list(regs);
    vepu511_h265_set_aq(ctx, regs);

    if (ctx->osd_cfg.osd_data3)
        vepu511_set_osd(&ctx->osd_cfg, &regs->reg_osd.osd_comb_cfg);

    if (ctx->roi_data)
        vepu511_set_roi(&regs->reg_rc_roi.roi_cfg, ctx->roi_data,
                        ctx->cfg->prep.width, ctx->cfg->prep.height);

    /*paramet cfg*/
    vepu511_h265_global_cfg_set(ctx, regs);

    /* two pass register patch */
    if (frm->save_pass1)
        vepu511_h265e_save_pass1_patch(regs, ctx, syn->pp.tiles_enabled_flag);

    if (frm->use_pass1)
        vepu511_h265e_use_pass1_patch(regs, ctx);

    ctx->frame_num++;

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_vepu511_start(void *hal, HalEncTask *enc_task)
{
    MPP_RET ret = MPP_OK;
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    Vepu511H265eFrmCfg *frm = ctx->frm;
    RK_U32 *regs = (RK_U32*)frm->regs_set;
    H265eV511RegSet *hw_regs = frm->regs_set;
    H265eV511StatusElem *reg_out = (H265eV511StatusElem *)frm->regs_ret;
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
    cfg.size = sizeof(Vepu511ControlCfg);
    cfg.offset = VEPU511_CTL_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_CTL_REGS) {
        regs = (RK_U32*)&hw_regs->reg_ctl;
        for (i = 0; i < sizeof(Vepu511ControlCfg) / 4; i++) {
            hal_h265e_dbg_ctl("ctl reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_frm;
    cfg.size = sizeof(H265eVepu511Frame);
    cfg.offset = VEPU511_FRAME_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_REGS) {
        regs = (RK_U32*)(&hw_regs->reg_frm);
        for (i = 0; i < 32; i++) {
            hal_h265e_dbg_regs("hw add cfg reg[%04x]: 0x%08x\n", i * 4, regs[i]);
        }
        regs += 32;
        for (i = 0; i < (sizeof(H265eVepu511Frame) - 128) / 4; i++) {
            hal_h265e_dbg_regs("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }
    cfg.reg = &hw_regs->reg_rc_roi;
    cfg.size = sizeof(Vepu511RcRoi);
    cfg.offset = VEPU511_RC_ROI_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_RCKUT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_rc_roi;
        for (i = 0; i < sizeof(Vepu511RcRoi) / 4; i++) {
            hal_h265e_dbg_rckut("set rc roi reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg =  &hw_regs->reg_param;
    cfg.size = sizeof(H265eVepu511Param);
    cfg.offset = VEPU511_PARAM_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_param;
        for (i = 0; i < sizeof(H265eVepu511Param) / 4; i++) {
            hal_h265e_dbg_wgt("set param reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_sqi;
    cfg.size = sizeof(H265eVepu511Sqi);
    cfg.offset = VEPU511_SQI_OFFSET;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_sqi;
        for (i = 0; i < sizeof(H265eVepu511Sqi) / 4; i++) {
            hal_h265e_dbg_wgt("set sqi reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_scl;
    cfg.size = sizeof(hw_regs->reg_scl);
    cfg.offset = VEPU511_SCL_OFFSET ;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_scl;
        for (i = 0; i < sizeof(H265eVepu511SclCfg) / 4; i++) {
            hal_h265e_dbg_wgt("set scl reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_osd;
    cfg.size = sizeof(hw_regs->reg_osd);
    cfg.offset = VEPU511_OSD_OFFSET ;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        return ret;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_osd;
        for (i = 0; i < sizeof(Vepu511OsdRegs) / 4; i++) {
            hal_h265e_dbg_wgt("set osd reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFS, ctx->reg_cfg);
    if (ret) {
        mpp_err_f("set register offsets failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->hw_status;
    cfg1.size = sizeof(RK_U32);
    cfg1.offset = VEPU511_REG_BASE_HW_STATUS;

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        return ret;
    }

    cfg1.reg = &reg_out->st;
    cfg1.size = sizeof(H265eV511StatusElem) - 4;
    cfg1.offset = VEPU511_STATUS_OFFSET;

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

static MPP_RET vepu511_h265_set_feedback(H265eV511HalContext *ctx, HalEncTask *enc_task)
{
    EncRcTaskInfo *hal_rc_ret = (EncRcTaskInfo *)&enc_task->rc_task->info;
    Vepu511H265eFrmCfg *frm = ctx->frms[enc_task->flags.reg_idx];
    Vepu511H265Fbk  *fb = &frm->feedback;
    MppEncCfgSet    *cfg = ctx->cfg;
    RK_S32 mb8_num = MPP_ALIGN(cfg->prep.width, 8) * MPP_ALIGN(cfg->prep.height, 8) / 64;
    RK_S32 mb4_num = (mb8_num << 2);
    H265eV511StatusElem *elem = (H265eV511StatusElem *)frm->regs_ret;
    RK_U32 hw_status = elem->hw_status;

    hal_h265e_enter();

    fb->qp_sum += elem->st.qp_sum;
    fb->out_strm_size += elem->st.bs_lgth_l32;
    fb->sse_sum += (RK_S64)(elem->st.sse_h32 << 16) +
                   (elem->st.st_sse_bsl.sse_l16 & 0xffff);

    fb->hw_status = hw_status;
    hal_h265e_dbg_detail("hw_status: 0x%08x", hw_status);
    if (hw_status & RKV_ENC_INT_LINKTABLE_FINISH)
        hal_h265e_err("RKV_ENC_INT_LINKTABLE_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_FRAME_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_ONE_FRAME_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_SLICE_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_ONE_SLICE_FINISH");

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

    ctx->feedback.acc_cover16_num = elem->st.st_skin_sum1.num1_point_skin;
    ctx->feedback.acc_bndry16_num = elem->st.st_skin_sum2.num2_point_skin;
    ctx->feedback.acc_zero_mv = elem->st.acc_zero_mv;
    ctx->feedback.st_ctu_num = elem->st.st_bnum_b16.num_b16;
    memcpy(&fb->st_cu_num_qp[0], &elem->st.st_b8_qp, 52 * sizeof(RK_U32));

    if (mb4_num > 0)
        hal_rc_ret->iblk4_prop =  ((((fb->st_lvl4_intra_num + fb->st_lvl8_intra_num) << 2) +
                                    (fb->st_lvl16_intra_num << 4) +
                                    (fb->st_lvl32_intra_num << 6)) << 8) / mb4_num;

    if (mb8_num > 0) {
        hal_rc_ret->quality_real = fb->qp_sum / mb8_num;
    }

    hal_h265e_leave();
    return MPP_OK;
}

static MPP_RET hal_h265e_vepu511_status_check(H265eV511RegSet *regs)
{
    MPP_RET ret = MPP_OK;

    if (regs->reg_ctl.int_sta.lkt_node_done_sta)
        hal_h265e_dbg_detail("lkt_done finish");

    if (regs->reg_ctl.int_sta.enc_done_sta)
        hal_h265e_dbg_detail("enc_done finish");

    if (regs->reg_ctl.int_sta.vslc_done_sta)
        hal_h265e_dbg_detail("enc_slice finsh");

    if (regs->reg_ctl.int_sta.sclr_done_sta)
        hal_h265e_dbg_detail("safe clear finsh");

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

//#define DUMP_DATA
MPP_RET hal_h265e_vepu511_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    HalEncTask *enc_task = task;
    MppPacket pkt = enc_task->packet;
    RK_U32 split_out = ctx->cfg->split.split_out;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu511H265eFrmCfg *frm = ctx->frms[task_idx];
    H265eV511RegSet *regs = frm->regs_set;
    RK_U32 offset = mpp_packet_get_length(pkt);
    RK_U32 seg_offset = offset;
    H265eVepu511Frame *reg_frm = &regs->reg_frm;
    RK_U32 type = reg_frm->synt_nal.nal_unit_type;
    H265eV511StatusElem *elem = (H265eV511StatusElem *)frm->regs_ret;

    hal_h265e_enter();

    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    /* if pass1 mode, it will disable split mode and the split out need to be disable */
    if (enc_task->rc_task->frm.save_pass1)
        split_out = 0;

    if (split_out) {
        EncOutParam param;
        RK_U32 slice_len = 0;
        RK_U32 slice_last = 0;
        MppDevPollCfg *poll_cfg = (MppDevPollCfg *)((char *)ctx->poll_cfgs);
        param.task = task;
        param.base = mpp_packet_get_data(task->packet);

        do {
            RK_S32 i = 0;
            poll_cfg->poll_type = 0;
            poll_cfg->poll_ret  = 0;
            poll_cfg->count_max = ctx->poll_slice_max;
            poll_cfg->count_ret = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, poll_cfg);
            for (i = 0; i < poll_cfg->count_ret; i++) {
                slice_last = poll_cfg->slice_info[i].last;
                slice_len = poll_cfg->slice_info[i].length;
                param.length = slice_len;

                mpp_packet_add_segment_info(pkt, type, seg_offset, slice_len);
                seg_offset += slice_len;

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

        ret = hal_h265e_vepu511_status_check(regs);
        if (!ret)
            task->hw_length += elem->st.bs_lgth_l32;

    } else {
        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        if (ret) {
            mpp_err_f("poll cmd failed %d\n", ret);
            ret = MPP_ERR_VPUHW;
        } else {
            ret = hal_h265e_vepu511_status_check(regs);
            if (!ret)
                task->hw_length += elem->st.bs_lgth_l32;
        }
        mpp_packet_add_segment_info(pkt, type, offset, elem->st.bs_lgth_l32);
    }

#ifdef DUMP_DATA
    vepu511_h265e_dump(ctx, task);
#endif

    if (ret)
        mpp_err_f("poll cmd failed %d status %d \n", ret, elem->hw_status);

    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_vepu511_get_task(void *hal, HalEncTask *task)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    Vepu511H265eFrmCfg *frm_cfg = NULL;
    MppFrame frame = task->frame;
    EncFrmStatus  *frm_status = &task->rc_task->frm;
    RK_S32 task_idx = ctx->task_idx;

    hal_h265e_enter();

    ctx->syn = (H265eSyntax_new *)task->syntax.data;
    ctx->dpb = (H265eDpb*)ctx->syn->dpb;
    ctx->smart_en = (ctx->cfg->rc.rc_mode == MPP_ENC_RC_MODE_SMTRC);
    ctx->qpmap_en = ctx->cfg->tune.deblur_en;

    if (vepu511_h265_setup_hal_bufs(ctx)) {
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
        mpp_meta_get_ptr_d(meta, KEY_OSD_DATA3, (void **)&ctx->osd_cfg.osd_data3, NULL);
    }

    task->flags.reg_idx = ctx->task_idx;
    ctx->ext_line_buf = ctx->ext_line_bufs[ctx->task_idx];
    frm_cfg->frame_count = ++ctx->frame_count;

    ctx->task_idx++;
    if (ctx->task_idx >= ctx->task_cnt)
        ctx->task_idx = 0;

    frm_cfg->hal_curr_idx = ctx->syn->sp.recon_pic.slot_idx;
    frm_cfg->hal_refr_idx = ctx->syn->sp.ref_pic.slot_idx;

    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_curr_idx);
    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_refr_idx);

    memset(&frm_cfg->feedback, 0, sizeof(Vepu511H265Fbk));

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_vepu511_ret_task(void *hal, HalEncTask *task)
{
    H265eV511HalContext *ctx = (H265eV511HalContext *)hal;
    HalEncTask *enc_task = task;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu511H265eFrmCfg *frm = ctx->frms[task_idx];
    Vepu511H265Fbk *fb = &frm->feedback;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    RK_U32 offset = mpp_packet_get_length(enc_task->packet);

    hal_h265e_enter();

    vepu511_h265_set_feedback(ctx, enc_task);
    mpp_buffer_sync_partial_begin(enc_task->output, offset, fb->out_strm_size);
    hal_h265e_amend_temporal_id(task, fb->out_strm_size);

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

    // vepu511_h265e_tune_stat_update(ctx->tune, enc_task);

    hal_h265e_dbg_detail("output stream size %d\n", fb->out_strm_size);
    hal_h265e_leave();
    return MPP_OK;
}

const MppEncHalApi hal_h265e_vepu511 = {
    .name       = "hal_h265e_v511",
    .coding     = MPP_VIDEO_CodingHEVC,
    .ctx_size   = sizeof(H265eV511HalContext),
    .flag       = 0,
    .init       = hal_h265e_vepu511_init,
    .deinit     = hal_h265e_vepu511_deinit,
    .prepare    = hal_h265e_vepu511_prepare,
    .get_task   = hal_h265e_vepu511_get_task,
    .gen_regs   = hal_h265e_vepu511_gen_regs,
    .start      = hal_h265e_vepu511_start,
    .wait       = hal_h265e_vepu511_wait,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_h265e_vepu511_ret_task,
};
