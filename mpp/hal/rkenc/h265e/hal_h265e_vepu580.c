/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG  "hal_h265e_v580"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_soc.h"
#include "mpp_common.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_dmabuf.h"

#include "hal_h265e_debug.h"
#include "h265e_syntax_new.h"
#include "hal_h265e_stream_amend.h"
#include "hal_bufs.h"
#include "rkv_enc_def.h"
#include "h265e_dpb.h"
#include "vepu5xx_common.h"
#include "vepu580_common.h"
#include "hal_h265e_vepu580.h"
#include "hal_h265e_vepu580_reg.h"
#include "mpp_enc_cb_param.h"
#include "vepu5xx.h"

#include "mpp_service.h"

#define MAX_FRAME_TASK_NUM      2
#define MAX_TILE_NUM            4
#define MAX_REGS_SET            ((MAX_FRAME_TASK_NUM) * (MAX_TILE_NUM))

#define hal_h265e_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)

typedef struct vepu580_h265_fbk_t {
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
    RK_U32 st_md_sad_b16num0;
    RK_U32 st_md_sad_b16num1;
    RK_U32 st_md_sad_b16num2;
    RK_U32 st_md_sad_b16num3;
    RK_U32 st_madi_b16num0;
    RK_U32 st_madi_b16num1;
    RK_U32 st_madi_b16num2;
    RK_U32 st_madi_b16num3;
    RK_U32 st_mb_num;
    RK_U32 st_ctu_num;
} Vepu580H265Fbk;

typedef struct Vepu580RoiHevcBsCfg_t {
    RK_U8 amv_en        : 1;
    RK_U8 qp_adj        : 1;
    RK_U8 force_split   : 1;
    RK_U8 force_intra   : 2;
    RK_U8 force_inter   : 2;
} Vepu580RoiHevcBsCfg;

typedef struct Vepu580MdInfo_t {
    RK_U8 vld;
    RK_U16 sad[16];
} Vepu580MdInfo;

typedef struct Vepu580RoiHevcQpCfg_t {
    RK_U16 reserved     : 4;
    /*
     * Qp area index
     * The choosed qp area index.
     */
    RK_U16 qp_area_idx  : 4;
    /*
     * Qp_adj
     * Qp_adj
     * in absolute qp mode qp_adj is the final qp used by encoder
     * in relative qp mode qp_adj is a adjustment to final qp
     */
    RK_S16 qp_adj       : 7;
    /*
     * Qp_adj_mode
     * Qp adjustment mode
     * 1 - absolute qp mode:
     *     the 16x16 MB qp is set to the qp_adj value
     * 0 - relative qp mode
     *     the 16x16 MB qp is adjusted by qp_adj value
     */
    RK_U16 qp_adj_mode  : 1;
} Vepu580RoiHevcQpCfg;

typedef struct Vepu580H265eFrmCfg_t {
    RK_S32              frame_count;
    RK_S32              frame_type;

    /* dchs cfg on frame parallel */
    RK_S32              dchs_curr_idx;
    RK_S32              dchs_prev_idx;

    /* hal dpb management slot idx */
    RK_S32              hal_curr_idx;
    RK_S32              hal_refr_idx;

    /* regs cfg */
    H265eV580RegSet     *regs_set[MAX_TILE_NUM];
    H265eV580StatusElem *regs_ret[MAX_TILE_NUM];

    /* hardware return info collection cfg */
    Vepu580H265Fbk      feedback;

    /* tile buffer */
    MppBuffer           hw_tile_buf[MAX_TILE_NUM];
    MppBuffer           hw_tile_stream[MAX_TILE_NUM - 1];

    /* osd cfg */
    Vepu5xxOsdCfg       osd_cfg;
    void                *roi_data;

    /* gdr roi cfg */
    MppBuffer           roi_base_cfg_buf;
    void                *roi_base_cfg_sw_buf;
    RK_S32              roi_base_buf_size;

    /* variable length cfg */
    MppDevRegOffCfgs    *reg_cfg;
} Vepu580H265eFrmCfg;

typedef struct H265eV580HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    Vepu580H265eFrmCfg  *frms[MAX_FRAME_TASK_NUM];

    /* current used frame config */
    Vepu580H265eFrmCfg  *frm;

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

    /* debug cfg */
    void                *dump_files;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    MppBufferGroup      roi_grp;
    MppBufferGroup      qpmap_grp;

    MppEncCfgSet        *cfg;
    H265eSyntax_new     *syn;
    H265eDpb            *dpb;

    /* single frame tile parallel info */
    MppBufferGroup      tile_grp;
    RK_U32              tile_num;
    RK_U32              tile_parall_en;
    RK_U32              tile_dump_err;

    MppBuffer           buf_pass1;

    RK_U32              enc_mode;
    RK_U32              frame_size;
    RK_S32              max_buf_cnt;
    RK_S32              hdr_status;
    void                *input_fmt;
    RK_U8               *src_buf;
    RK_U8               *dst_buf;
    RK_S32              buf_size;
    HalBufs             dpb_bufs;
    RK_S32              fbc_header_len;

    MppDevPollCfg       *poll_cfgs;
    MppCbCtx            *output_cb;

    /* finetune */
    void                *tune;
    MppBuffer           md_info_buf; /* md info buffer for deblurring */
    MppBuffer           qpmap_base_cfg_buf;
    MppBuffer           qpmap_qp_cfg_buf;
    RK_U8*              md_flag_buf;
    RK_S32              md_info_buf_size;
    RK_S32              qpmap_base_cfg_size;
    RK_S32              qpmap_qp_cfg_size;
    RK_S32              md_flag_size;
} H265eV580HalContext;

static RK_U32 aq_thd_default[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 35
};

static RK_U32 h265e_mode_bias[16] = {
    0,  2,  4,  6,
    8,  10, 12, 14,
    16, 18, 20, 24,
    28, 32, 64, 128
};

static RK_S32 aq_qp_dealt_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    5,  7,  8,  9,
};


static RK_U16 lvl32_intra_cst_thd[4] = {2, 6, 16, 36};

static RK_U16 lvl16_intra_cst_thd[4] = {2, 6, 16, 36};

static RK_U8 lvl32_intra_cst_wgt[8] = {23, 22, 21, 20, 22, 24, 26};

static RK_U8 lvl16_intra_cst_wgt[8] = {17, 17, 17, 18, 17, 18, 18};

#include "hal_h265e_vepu580_tune.c"

static void vepu580_h265_set_me_ram(H265eSyntax_new *syn, hevc_vepu580_base *regs,
                                    RK_U32 index, RK_S32 tile_start_x)
{
    RK_S32 frm_sta = 0, frm_end = 0, pic_w = 0;
    RK_S32 srch_w = regs->reg0220_me_rnge.cme_srch_h * 4;
    RK_S32 srch_h = regs->reg0220_me_rnge.cme_srch_v * 4;
    RK_S32 x_gmv = regs->reg0224_gmv.gmv_x;
    RK_S32 y_gmv = regs->reg0224_gmv.gmv_y;
    RK_U32 pic_wd64 = ((regs->reg0196_enc_rsl.pic_wd8_m1 + 1) * 8 + 63) / 64;

    if (!syn->pp.tiles_enabled_flag) {
        if (x_gmv - srch_w < 0) {
            frm_sta = (x_gmv - srch_w - 15) / 16;
        } else {
            frm_sta = (x_gmv - srch_w) / 16;
        }
        frm_sta = mpp_clip(frm_sta, 0, pic_wd64 - 1);
        if (x_gmv + srch_w < 0) {
            frm_end = pic_wd64 - 1 + (x_gmv + srch_w) / 16;
        } else {
            frm_end = pic_wd64 - 1 + (x_gmv + srch_w + 15) / 16;
        }
        frm_end = mpp_clip(frm_end, 0, pic_wd64 - 1);
    } else {
        RK_S32 tile_ctu_stax = tile_start_x;
        RK_S32 tile_ctu_endx = tile_start_x + syn->pp.column_width_minus1[index];

        if (x_gmv - srch_w < 0) {
            frm_sta = tile_ctu_stax + (x_gmv - srch_w - 15) / 16;
        } else {
            frm_sta = tile_ctu_stax + (x_gmv - srch_w) / 16;
        }
        frm_sta = mpp_clip(frm_sta, 0, pic_wd64 - 1);

        if (x_gmv + srch_w < 0) {
            frm_end = tile_ctu_endx + (x_gmv + srch_w) / 16;
        } else {
            frm_end = tile_ctu_endx + (x_gmv + srch_w + 15) / 16;
        }
        frm_end = mpp_clip(frm_end, 0, pic_wd64 - 1);
    }
    pic_w = (frm_end - frm_sta + 1) * 64;
    regs->reg0222_me_cach.cme_linebuf_w = (pic_w ? pic_w : 64) / 64;
    {
        RK_U32 cime_rama_max = 2464;
        RK_U32 ctu_4_h = 4, ramb_h;
        RK_U32 cur_srch_16_w, cur_srch_4_h, cur_srch_max;
        RK_U32 cime_rama_h = ctu_4_h;

        if ((x_gmv % 16 - srch_w % 16) < 0) {
            cur_srch_16_w = (16 + (x_gmv % 16 - srch_w % 16) % 16 + srch_w * 2 + 15) / 16 + 1;
        } else {
            cur_srch_16_w = ((x_gmv % 16 - srch_w % 16) % 16 + srch_w * 2 + 15) / 16 + 1;
        }
        if ((y_gmv %  4 - srch_h %  4) < 0) {
            cur_srch_4_h = (4 + (y_gmv %  4 - srch_h %  4) %  4 + srch_h * 2 +  3) / 4  + ctu_4_h;
        } else {
            cur_srch_4_h = ((y_gmv %  4 - srch_h %  4) %  4 + srch_h * 2 +  3) / 4  + ctu_4_h;
        }
        cur_srch_max = MPP_ALIGN(cur_srch_4_h, 4);
        if (regs->reg0222_me_cach.cme_linebuf_w < cur_srch_16_w) {
            cur_srch_16_w = regs->reg0222_me_cach.cme_linebuf_w;
        }
        ramb_h = cur_srch_4_h;
        while ((cime_rama_h < cur_srch_max) && (cime_rama_max >
                                                ((cime_rama_h - ctu_4_h) * regs->reg0222_me_cach.cme_linebuf_w * 4 + (ramb_h * 4 * cur_srch_16_w)))) {
            cime_rama_h = cime_rama_h + ctu_4_h;
            if (ramb_h > 2 * ctu_4_h) {
                ramb_h = ramb_h - ctu_4_h;
            } else {
                ramb_h = ctu_4_h;
            }
        }
        if (cur_srch_4_h == ctu_4_h) {
            cime_rama_h = cime_rama_h + ctu_4_h;
            ramb_h = 0;
        }
        if (cime_rama_max < ((cime_rama_h - ctu_4_h) * regs->reg0222_me_cach.cme_linebuf_w * 4 + (ramb_h * 4 * cur_srch_16_w))) {
            cime_rama_h = cime_rama_h - ctu_4_h;
        }
        regs->reg0222_me_cach.cme_rama_h = cime_rama_h;        /* cime_rama_max */

        {
            RK_U32 ram_col_h = (cime_rama_h - ctu_4_h) / ctu_4_h;
            regs->reg0222_me_cach.cme_rama_max = ram_col_h * regs->reg0222_me_cach.cme_linebuf_w + cur_srch_16_w;
        }

    }

    hal_h265e_dbg_detail("cime_rama_h %d, cime_rama_max %d, cime_linebuf_w %d",
                         regs->reg0222_me_cach.cme_rama_h, regs->reg0222_me_cach.cme_rama_max, regs->reg0222_me_cach.cme_linebuf_w);
}

static MPP_RET vepu580_h265_setup_hal_bufs(H265eV580HalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    RK_U32 frame_size;
    RK_S32 mb_wd64, mb_h64;
    MppEncRefCfg ref_cfg = ctx->cfg->ref_cfg;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_S32 old_max_cnt = ctx->max_buf_cnt;
    RK_S32 new_max_cnt = 4;

    hal_h265e_enter();

    mb_wd64 = (prep->width + 63) / 64;
    mb_h64 = (prep->height + 63) / 64 + 1;

    frame_size = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16);
    vepu5xx_set_fmt(fmt, ctx->cfg->prep.format);

    if (ref_cfg) {
        MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(ref_cfg);
        new_max_cnt = MPP_MAX(new_max_cnt, info->dpb_size + 1);
    }

    if (frame_size > ctx->frame_size || new_max_cnt > old_max_cnt) {
        size_t size[3] = {0};

        hal_bufs_deinit(ctx->dpb_bufs);
        hal_bufs_init(&ctx->dpb_bufs);

        ctx->fbc_header_len = MPP_ALIGN(((mb_wd64 * mb_h64) << 6), SZ_8K);
        size[0] = ctx->fbc_header_len + ((mb_wd64 * mb_h64) << 12) * 3 / 2; //fbc_h + fbc_b
        size[1] = (mb_wd64 * mb_h64 << 8);
        size[2] = MPP_ALIGN(mb_wd64 * mb_h64 * 16 * 6, 256);
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

static void vepu580_h265_sobel_cfg(hevc_vepu580_wgt *reg)
{
    reg->pre_intra_cla0_B0.pre_intra_cla0_m0 = 10;
    reg->pre_intra_cla0_B0.pre_intra_cla0_m1 = 11;
    reg->pre_intra_cla0_B0.pre_intra_cla0_m2 = 12;
    reg->pre_intra_cla0_B0.pre_intra_cla0_m3 = 13;
    reg->pre_intra_cla0_B0.pre_intra_cla0_m4 = 14;

    reg->pre_intra_cla0_B1.pre_intra_cla0_m5 = 9;
    reg->pre_intra_cla0_B1.pre_intra_cla0_m6 = 15;
    reg->pre_intra_cla0_B1.pre_intra_cla0_m7 = 8;
    reg->pre_intra_cla0_B1.pre_intra_cla0_m8 = 16;
    reg->pre_intra_cla0_B1.pre_intra_cla0_m9 = 7;

    reg->pre_intra_cla1_B0.pre_intra_cla1_m0 = 10;
    reg->pre_intra_cla1_B0.pre_intra_cla1_m1 = 9;
    reg->pre_intra_cla1_B0.pre_intra_cla1_m2 = 8;
    reg->pre_intra_cla1_B0.pre_intra_cla1_m3 = 7;
    reg->pre_intra_cla1_B0.pre_intra_cla1_m4 = 6;
    reg->pre_intra_cla1_B1.pre_intra_cla1_m5 = 11;
    reg->pre_intra_cla1_B1.pre_intra_cla1_m6 = 5;
    reg->pre_intra_cla1_B1.pre_intra_cla1_m7 = 12;
    reg->pre_intra_cla1_B1.pre_intra_cla1_m8 = 4;
    reg->pre_intra_cla1_B1.pre_intra_cla1_m9 = 13;

    reg->pre_intra_cla2_B0.pre_intra_cla2_m0 = 18;
    reg->pre_intra_cla2_B0.pre_intra_cla2_m1 = 17;
    reg->pre_intra_cla2_B0.pre_intra_cla2_m2 = 16;
    reg->pre_intra_cla2_B0.pre_intra_cla2_m3 = 15;
    reg->pre_intra_cla2_B0.pre_intra_cla2_m4 = 14;
    reg->pre_intra_cla2_B1.pre_intra_cla2_m5 = 19;
    reg->pre_intra_cla2_B1.pre_intra_cla2_m6 = 13;
    reg->pre_intra_cla2_B1.pre_intra_cla2_m7 = 20;
    reg->pre_intra_cla2_B1.pre_intra_cla2_m8 = 12;
    reg->pre_intra_cla2_B1.pre_intra_cla2_m9 = 21;

    reg->pre_intra_cla3_B0.pre_intra_cla3_m0 = 18;
    reg->pre_intra_cla3_B0.pre_intra_cla3_m1 = 19;
    reg->pre_intra_cla3_B0.pre_intra_cla3_m2 = 20;
    reg->pre_intra_cla3_B0.pre_intra_cla3_m3 = 21;
    reg->pre_intra_cla3_B0.pre_intra_cla3_m4 = 22;
    reg->pre_intra_cla3_B1.pre_intra_cla3_m5 = 17;
    reg->pre_intra_cla3_B1.pre_intra_cla3_m6 = 23;
    reg->pre_intra_cla3_B1.pre_intra_cla3_m7 = 16;
    reg->pre_intra_cla3_B1.pre_intra_cla3_m8 = 24;
    reg->pre_intra_cla3_B1.pre_intra_cla3_m9 = 15;

    reg->pre_intra_cla4_B0.pre_intra_cla4_m0 = 25;
    reg->pre_intra_cla4_B0.pre_intra_cla4_m1 = 26;
    reg->pre_intra_cla4_B0.pre_intra_cla4_m2 = 24;
    reg->pre_intra_cla4_B0.pre_intra_cla4_m3 = 23;
    reg->pre_intra_cla4_B0.pre_intra_cla4_m4 = 22;
    reg->pre_intra_cla4_B1.pre_intra_cla4_m5 = 27;
    reg->pre_intra_cla4_B1.pre_intra_cla4_m6 = 21;
    reg->pre_intra_cla4_B1.pre_intra_cla4_m7 = 28;
    reg->pre_intra_cla4_B1.pre_intra_cla4_m8 = 20;
    reg->pre_intra_cla4_B1.pre_intra_cla4_m9 = 29;

    reg->pre_intra_cla5_B0.pre_intra_cla5_m0 = 27;
    reg->pre_intra_cla5_B0.pre_intra_cla5_m1 = 26;
    reg->pre_intra_cla5_B0.pre_intra_cla5_m2 = 28;
    reg->pre_intra_cla5_B0.pre_intra_cla5_m3 = 29;
    reg->pre_intra_cla5_B0.pre_intra_cla5_m4 = 30;
    reg->pre_intra_cla5_B1.pre_intra_cla5_m5 = 25;
    reg->pre_intra_cla5_B1.pre_intra_cla5_m6 = 31;
    reg->pre_intra_cla5_B1.pre_intra_cla5_m7 = 24;
    reg->pre_intra_cla5_B1.pre_intra_cla5_m8 = 32;
    reg->pre_intra_cla5_B1.pre_intra_cla5_m9 = 23;

    reg->pre_intra_cla6_B0.pre_intra_cla6_m0 = 34;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m1 = 33;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m2 = 32;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m3 = 31;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m4 = 30;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m5 = 2;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m6 = 29;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m7 = 3;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m8 = 28;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m9 = 4;

    reg->pre_intra_cla7_B0.pre_intra_cla7_m0 = 34;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m1 = 2;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m2 = 3;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m3 = 4;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m4 = 5;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m5 = 33;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m6 = 6;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m7 = 32;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m8 = 7;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m9 = 31;

    reg->pre_intra_cla8_B0.pre_intra_cla8_m0 = 10;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m1 = 26;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m2 = 18;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m3 = 34;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m4 = 6;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m5 = 14;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m6 = 22;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m7 = 30;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m8 = 2;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m9 = 24;

    reg->pre_intra_cla9_B0.pre_intra_cla9_m0 = 0;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m1 = 0;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m2 = 0;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m3 = 0;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m4 = 0;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m5 = 0;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m6 = 0;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m7 = 0;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m8 = 0;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m9 = 0;

    reg->pre_intra_cla10_B0.pre_intra_cla10_m0 = 0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m1 = 0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m2 = 0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m3 = 0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m4 = 0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m5 = 0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m6 = 0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m7 = 0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m8 = 0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m9 = 0;

    reg->pre_intra_cla11_B0.pre_intra_cla11_m0 = 0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m1 = 0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m2 = 0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m3 = 0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m4 = 0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m5 = 0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m6 = 0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m7 = 0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m8 = 0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m9 = 0;

    reg->pre_intra_cla12_B0.pre_intra_cla12_m0 = 0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m1 = 0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m2 = 0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m3 = 0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m4 = 0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m5 = 0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m6 = 0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m7 = 0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m8 = 0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m9 = 0;

    reg->pre_intra_cla13_B0.pre_intra_cla13_m0 = 0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m1 = 0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m2 = 0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m3 = 0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m4 = 0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m5 = 0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m6 = 0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m7 = 0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m8 = 0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m9 = 0;

    reg->pre_intra_cla14_B0.pre_intra_cla14_m0 = 0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m1 = 0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m2 = 0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m3 = 0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m4 = 0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m5 = 0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m6 = 0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m7 = 0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m8 = 0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m9 = 0;

    reg->pre_intra_cla15_B0.pre_intra_cla15_m0 = 0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m1 = 0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m2 = 0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m3 = 0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m4 = 0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m5 = 0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m6 = 0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m7 = 0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m8 = 0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m9 = 0;

    reg->pre_intra_cla16_B0.pre_intra_cla16_m0 = 0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m1 = 0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m2 = 0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m3 = 0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m4 = 0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m5 = 0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m6 = 0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m7 = 0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m8 = 0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m9 = 0;

    reg->i16_sobel_t.intra_l16_sobel_t0 = 64;
    reg->i16_sobel_t.intra_l16_sobel_t1 = 200;
    reg->i16_sobel_a_00.intra_l16_sobel_a0_qp0 = 32;
    reg->i16_sobel_a_00.intra_l16_sobel_a0_qp1 = 32;
    reg->i16_sobel_a_00.intra_l16_sobel_a0_qp2 = 32;
    reg->i16_sobel_a_00.intra_l16_sobel_a0_qp3 = 32;
    reg->i16_sobel_a_00.intra_l16_sobel_a0_qp4 = 32;
    reg->i16_sobel_a_01.intra_l16_sobel_a0_qp5 = 32;
    reg->i16_sobel_a_01.intra_l16_sobel_a0_qp6 = 32;
    reg->i16_sobel_a_01.intra_l16_sobel_a0_qp7 = 32;
    reg->i16_sobel_a_01.intra_l16_sobel_a0_qp8 = 32;
    reg->i16_sobel_b_00.intra_l16_sobel_b0_qp0 = 0;
    reg->i16_sobel_b_00.intra_l16_sobel_b0_qp1 = 0;
    reg->i16_sobel_b_01.intra_l16_sobel_b0_qp2 = 0;
    reg->i16_sobel_b_01.intra_l16_sobel_b0_qp3 = 0;
    reg->i16_sobel_b_02.intra_l16_sobel_b0_qp4 = 0;
    reg->i16_sobel_b_02.intra_l16_sobel_b0_qp5 = 0;
    reg->i16_sobel_b_03.intra_l16_sobel_b0_qp6 = 0;
    reg->i16_sobel_b_03.intra_l16_sobel_b0_qp7 = 0;
    reg->i16_sobel_b_04.intra_l16_sobel_b0_qp8 = 0;
    reg->i16_sobel_c_00.intra_l16_sobel_c0_qp0  = 13;
    reg->i16_sobel_c_00.intra_l16_sobel_c0_qp1  = 13;
    reg->i16_sobel_c_00.intra_l16_sobel_c0_qp2  = 13;
    reg->i16_sobel_c_00.intra_l16_sobel_c0_qp3  = 13;
    reg->i16_sobel_c_00.intra_l16_sobel_c0_qp4  = 13;
    reg->i16_sobel_c_01.intra_l16_sobel_c0_qp5  = 13;
    reg->i16_sobel_c_01.intra_l16_sobel_c0_qp6  = 13;
    reg->i16_sobel_c_01.intra_l16_sobel_c0_qp7  = 13;
    reg->i16_sobel_c_01.intra_l16_sobel_c0_qp8  = 13;
    reg->i16_sobel_d_00.intra_l16_sobel_d0_qp0 = 23750;
    reg->i16_sobel_d_00.intra_l16_sobel_d0_qp1 = 23750;
    reg->i16_sobel_d_01.intra_l16_sobel_d0_qp2 = 23750;
    reg->i16_sobel_d_01.intra_l16_sobel_d0_qp3 = 23750;
    reg->i16_sobel_d_02.intra_l16_sobel_d0_qp4 = 23750;
    reg->i16_sobel_d_02.intra_l16_sobel_d0_qp5 = 23750;
    reg->i16_sobel_d_03.intra_l16_sobel_d0_qp6 = 23750;
    reg->i16_sobel_d_03.intra_l16_sobel_d0_qp7 = 23750;
    reg->i16_sobel_d_04.intra_l16_sobel_d0_qp8 = 23750;

    reg->intra_l16_sobel_e0_qp0_low = 20000;
    reg->intra_l16_sobel_e0_qp1_low = 20000;
    reg->intra_l16_sobel_e0_qp2_low = 20000;
    reg->intra_l16_sobel_e0_qp3_low = 20000;
    reg->intra_l16_sobel_e0_qp4_low = 20000;
    reg->intra_l16_sobel_e0_qp5_low = 20000;
    reg->intra_l16_sobel_e0_qp6_low = 20000;
    reg->intra_l16_sobel_e0_qp7_low = 20000;
    reg->intra_l16_sobel_e0_qp8_low = 20000;
    reg->i16_sobel_e_01.intra_l16_sobel_e0_qp0_high = 0;
    reg->i16_sobel_e_03.intra_l16_sobel_e0_qp1_high = 0;
    reg->i16_sobel_e_05.intra_l16_sobel_e0_qp2_high = 0;
    reg->i16_sobel_e_07.intra_l16_sobel_e0_qp3_high = 0;
    reg->i16_sobel_e_09.intra_l16_sobel_e0_qp4_high = 0;
    reg->i16_sobel_e_11.intra_l16_sobel_e0_qp5_high = 0;
    reg->i16_sobel_e_13.intra_l16_sobel_e0_qp6_high = 0;
    reg->i16_sobel_e_15.intra_l16_sobel_e0_qp7_high = 0;
    reg->i16_sobel_e_17.intra_l16_sobel_e0_qp8_high = 0;

    reg->i32_sobel_t_00.intra_l32_sobel_t2 = 64;
    reg->i32_sobel_t_00.intra_l32_sobel_t3 = 400;
    reg->i32_sobel_t_01.intra_l32_sobel_t4 = 8;
    reg->i32_sobel_t_02.intra_l32_sobel_t5 = 100;
    reg->i32_sobel_t_02.intra_l32_sobel_t6 = 100;

    reg->i32_sobel_a.intra_l32_sobel_a1_qp0 = 18;
    reg->i32_sobel_a.intra_l32_sobel_a1_qp1 = 18;
    reg->i32_sobel_a.intra_l32_sobel_a1_qp2 = 18;
    reg->i32_sobel_a.intra_l32_sobel_a1_qp3 = 18;
    reg->i32_sobel_a.intra_l32_sobel_a1_qp4 = 18;

    reg->i32_sobel_b_00.intra_l32_sobel_b1_qp0 = 0;
    reg->i32_sobel_b_00.intra_l32_sobel_b1_qp1 = 0;
    reg->i32_sobel_b_01.intra_l32_sobel_b1_qp2 = 0;
    reg->i32_sobel_b_01.intra_l32_sobel_b1_qp3 = 0;
    reg->i32_sobel_b_02.intra_l32_sobel_b1_qp4 = 0;

    reg->i32_sobel_c.intra_l32_sobel_c1_qp0 = 16;
    reg->i32_sobel_c.intra_l32_sobel_c1_qp1 = 16;
    reg->i32_sobel_c.intra_l32_sobel_c1_qp2 = 16;
    reg->i32_sobel_c.intra_l32_sobel_c1_qp3 = 16;
    reg->i32_sobel_c.intra_l32_sobel_c1_qp4 = 16;

    reg->i32_sobel_d_00.intra_l32_sobel_d1_qp0 = 0;
    reg->i32_sobel_d_00.intra_l32_sobel_d1_qp1 = 0;
    reg->i32_sobel_d_01.intra_l32_sobel_d1_qp2 = 0;
    reg->i32_sobel_d_01.intra_l32_sobel_d1_qp3 = 0;
    reg->i32_sobel_d_02.intra_l32_sobel_d1_qp4 = 0;

    reg->intra_l32_sobel_e1_qp0_low = 20000;
    reg->intra_l32_sobel_e1_qp1_low = 20000;
    reg->intra_l32_sobel_e1_qp2_low = 20000;
    reg->intra_l32_sobel_e1_qp3_low = 20000;
    reg->intra_l32_sobel_e1_qp4_low = 20000;

    reg->i32_sobel_e_01.intra_l32_sobel_e1_qp0_high = 0;
    reg->i32_sobel_e_03.intra_l32_sobel_e1_qp1_high = 0;
    reg->i32_sobel_e_05.intra_l32_sobel_e1_qp2_high = 0;
    reg->i32_sobel_e_07.intra_l32_sobel_e1_qp3_high = 0;
    reg->i32_sobel_e_09.intra_l32_sobel_e1_qp4_high = 0;
}

static void vepu580_h265_rdo_bias_cfg (vepu580_rdo_cfg *reg, MppEncHwCfg *hw)
{
    RdoAtfCfg* p_rdo_atf;
    RdoAtfSkipCfg* p_rdo_atf_skip;
    RK_U8 bias = h265e_mode_bias[hw->mode_bias[4]];

    p_rdo_atf = &reg->rdo_b64_inter_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    if (hw->skip_bias_en) {
        bias = h265e_mode_bias[hw->skip_bias];

        p_rdo_atf_skip = &reg->rdo_b64_skip_atf;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = hw->skip_sad < 4 ? hw->skip_sad : 4;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = hw->skip_sad < 6 ? hw->skip_sad : 6;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = hw->skip_sad;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = bias > 24 ? bias : 24;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = bias < 4 ? bias : 4;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = bias < 6 ? bias : 6;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = bias < 8 ? bias : 8;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = bias < 10 ? bias : 10;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = bias < 10 ? bias : 10;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = bias < 10 ? bias : 10;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = bias < 14 ? bias : 14;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = bias < 14 ? bias : 14;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = bias;
    }

    bias = h265e_mode_bias[hw->mode_bias[0]];

    p_rdo_atf = &reg->rdo_b32_intra_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias > 26 ? bias : 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias > 24 ? bias : 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias > 23 ? bias : 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias > 21 ? bias : 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias > 19 ? bias : 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias > 18 ? bias : 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    bias = h265e_mode_bias[hw->mode_bias[5]];

    p_rdo_atf = &reg->rdo_b32_inter_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    if (hw->skip_bias_en) {
        bias = h265e_mode_bias[hw->skip_bias];

        p_rdo_atf_skip = &reg->rdo_b32_skip_atf;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = hw->skip_sad < 4 ? hw->skip_sad : 4;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = hw->skip_sad < 6 ? hw->skip_sad : 6;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = hw->skip_sad;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias > 18 ? bias : 18;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = bias < 11 ? bias : 11;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = bias < 11 ? bias : 11;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = bias < 11 ? bias : 11;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = bias < 13 ? bias : 13;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = bias < 13 ? bias : 13;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = bias < 13 ? bias : 13;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = bias < 15 ? bias : 15;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = bias;
    }

    bias = h265e_mode_bias[hw->mode_bias[1]];

    p_rdo_atf = &reg->rdo_b16_intra_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias > 26 ? bias : 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias > 24 ? bias : 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias > 23 ? bias : 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias > 21 ? bias : 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias > 19 ? bias : 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias > 18 ? bias : 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    bias = h265e_mode_bias[hw->mode_bias[6]];

    p_rdo_atf = &reg->rdo_b16_inter_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    if (hw->skip_bias_en) {
        bias = h265e_mode_bias[hw->skip_bias];

        p_rdo_atf_skip = &reg->rdo_b16_skip_atf;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = hw->skip_sad < 48 ? hw->skip_sad : 48;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = hw->skip_sad;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = bias;
    }

    bias = h265e_mode_bias[hw->mode_bias[2]];

    p_rdo_atf = &reg->rdo_b8_intra_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias > 26 ? bias : 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias > 25 ? bias : 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias > 24 ? bias : 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias > 23 ? bias : 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias > 21 ? bias : 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias > 19 ? bias : 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias > 18 ? bias : 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    bias = h265e_mode_bias[hw->mode_bias[7]];

    p_rdo_atf = &reg->rdo_b8_inter_atf;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = bias;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = bias;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = bias;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = bias;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = bias;

    if (hw->skip_bias_en) {
        bias = h265e_mode_bias[hw->skip_bias];

        p_rdo_atf_skip = &reg->rdo_b8_skip_atf;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = hw->skip_sad < 24 ? hw->skip_sad : 24;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = hw->skip_sad < 48 ? hw->skip_sad : 48;
        p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = hw->skip_sad;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = bias;
        p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = bias;
    }
}

static void vepu580_h265_rdo_cfg (vepu580_rdo_cfg *reg)
{
    RdoAtfCfg* p_rdo_atf;
    RdoAtfSkipCfg* p_rdo_atf_skip;
    reg->rdo_sqi_cfg.rdo_segment_en = 1;
    reg->rdo_sqi_cfg.rdo_smear_en = 1;
    p_rdo_atf = &reg->rdo_b64_inter_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 32;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 144;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 300;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf_skip = &reg->rdo_b64_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 4;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 6;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 8;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd10  = 31;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd11  = 400;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd20  = 31;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd21  = 400;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd30  = 31;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd31  = 400;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd40  = 31;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd41  = 400;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = 10;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = 10;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = 10;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = 14;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = 14;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = 16;

    p_rdo_atf = &reg->rdo_b32_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf = &reg->rdo_b32_inter_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 32;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 144;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 300;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf_skip = &reg->rdo_b32_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 4;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 6;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 8;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd10  = 31;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd11  = 400;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd20  = 31;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd21  = 400;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd30  = 31;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd31  = 400;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd40  = 31;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd41  = 400;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 18;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = 11;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = 11;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = 11;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = 13;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = 13;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = 13;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = 15;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = 16;

    p_rdo_atf = &reg->rdo_b16_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf = &reg->rdo_b16_inter_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 32;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 144;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 300;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf_skip = &reg->rdo_b16_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 48;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 96;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd10  = 31;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd11  = 400;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd20  = 31;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd21  = 400;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd30  = 31;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd31  = 400;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd40  = 31;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd41  = 400;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = 16;

    p_rdo_atf = &reg->rdo_b8_intra_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 48;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 64;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 26;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 25;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 25;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 24;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 23;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 21;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 19;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 18;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf = &reg->rdo_b8_inter_atf;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 32;
    p_rdo_atf->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 144;
    p_rdo_atf->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 300;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd00  = 31;
    p_rdo_atf->rdo_b_var_thd0.cu_rdo_var_thd01  = 400;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd10  = 31;
    p_rdo_atf->rdo_b_var_thd1.cu_rdo_var_thd11  = 400;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd20  = 31;
    p_rdo_atf->rdo_b_var_thd2.cu_rdo_var_thd21  = 400;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd30  = 31;
    p_rdo_atf->rdo_b_var_thd3.cu_rdo_var_thd31  = 400;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt01  = 16;
    p_rdo_atf->rdo_b_atf_wgt0.cu_rdo_atf_wgt02  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf->rdo_b_atf_wgt1.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf->rdo_b_atf_wgt2.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf->rdo_b_atf_wgt3.cu_rdo_atf_wgt32  = 16;

    p_rdo_atf_skip = &reg->rdo_b8_skip_atf;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd0 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd0.cu_rdo_cime_thd1 = 24;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd2 = 48;
    p_rdo_atf_skip->rdo_b_cime_thd1.cu_rdo_cime_thd3 = 96;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd10  = 31;
    p_rdo_atf_skip->rdo_b_var_thd0.cu_rdo_var_thd11  = 400;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd20  = 31;
    p_rdo_atf_skip->rdo_b_var_thd1.cu_rdo_var_thd21  = 400;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd30  = 31;
    p_rdo_atf_skip->rdo_b_var_thd2.cu_rdo_var_thd31  = 400;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd40  = 31;
    p_rdo_atf_skip->rdo_b_var_thd3.cu_rdo_var_thd41  = 400;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt00  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt10  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt11  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt0.cu_rdo_atf_wgt12  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt20  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt21  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt1.cu_rdo_atf_wgt22  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt30  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt31  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt2.cu_rdo_atf_wgt32  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt40  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt41  = 16;
    p_rdo_atf_skip->rdo_b_atf_wgt3.cu_rdo_atf_wgt42  = 16;

    reg->rdo_segment_b64_thd0.rdo_segment_cu64_th0 = 160;
    reg->rdo_segment_b64_thd0.rdo_segment_cu64_th1 = 96;
    reg->rdo_segment_b64_thd1.rdo_segment_cu64_th2 = 30;
    reg->rdo_segment_b64_thd1.rdo_segment_cu64_th3 = 0;
    reg->rdo_segment_b64_thd1.rdo_segment_cu64_th4 = 1;
    reg->rdo_segment_b64_thd1.rdo_segment_cu64_th5_minus1 = 4;
    reg->rdo_segment_b64_thd1.rdo_segment_cu64_th6_minus1 = 11;

    reg->rdo_segment_b32_thd0.rdo_segment_cu32_th0 = 160;
    reg->rdo_segment_b32_thd0.rdo_segment_cu32_th1 = 96;
    reg->rdo_segment_b32_thd1.rdo_segment_cu32_th2 = 30;
    reg->rdo_segment_b32_thd1.rdo_segment_cu32_th3 = 0;
    reg->rdo_segment_b32_thd1.rdo_segment_cu32_th4 = 1;
    reg->rdo_segment_b32_thd1.rdo_segment_cu32_th5_minus1 = 2;
    reg->rdo_segment_b32_thd1.rdo_segment_cu32_th6_minus1 = 3;

    reg->rdo_segment_multi.rdo_segment_cu64_multi = 22;
    reg->rdo_segment_multi.rdo_segment_cu32_multi = 22;
    reg->rdo_segment_multi.rdo_smear_cu16_multi = 6;

    reg->rdo_b16_smear_thd0.rdo_smear_cu16_cime_sad_th0 = 64;
    reg->rdo_b16_smear_thd0.rdo_smear_cu16_cime_sad_th1 = 32;
    reg->rdo_b16_smear_thd1.rdo_smear_cu16_cime_sad_th2 = 36;
    reg->rdo_b16_smear_thd1.rdo_smear_cu16_cime_sad_th3 = 64;


    reg->preintra_b32_cst_var_thd.pre_intra32_cst_var_th00 = 9;
    reg->preintra_b32_cst_var_thd.pre_intra32_cst_var_th01 = 4;
    reg->preintra_b32_cst_var_thd.pre_intra32_mode_th = 5;

    reg->preintra_b32_cst_wgt.pre_intra32_cst_wgt00 = 31;
    reg->preintra_b32_cst_wgt.pre_intra32_cst_wgt01 = 25;

    reg->preintra_b16_cst_var_thd.pre_intra16_cst_var_th00 = 9;
    reg->preintra_b16_cst_var_thd.pre_intra16_cst_var_th01 = 4;
    reg->preintra_b16_cst_var_thd.pre_intra16_mode_th = 5;

    reg->preintra_b16_cst_wgt.pre_intra16_cst_wgt00 = 31;
    reg->preintra_b16_cst_wgt.pre_intra16_cst_wgt01 = 25;
}

static void vepu580_h265_scl_cfg(vepu580_rdo_cfg *reg)
{
    static RK_U32 vepu580_h265_scl_tab[] = {
        /* 0x2200 */
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0c31, 0x10001000, 0x10001000, 0x0d790f0f, 0x0a3d0ba3,
        0x10001000, 0x0e390f0f, 0x0ba30ccd, 0x08d40a3d, 0x10001000, 0x0c310e39, 0x097b0aab, 0x071c0842,
        0x0f0f0f0f, 0x0aab0ccd, 0x07500889, 0x0572063e, 0x0d790e39, 0x097b0ba3, 0x05d10750, 0x03f004be,
        0x0ba30c31, 0x08420a3d, 0x04be063e, 0x02e903a8, 0x0a3d0aab, 0x071c08d4, 0x03f00572, 0x023a02e9,
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0c31, 0x10001000, 0x10001000, 0x0d790f0f, 0x0a3d0ba3,
        0x10001000, 0x0e390f0f, 0x0ba30ccd, 0x08d40a3d, 0x10001000, 0x0c310e39, 0x097b0aab, 0x071c0842,
        0x0f0f0f0f, 0x0aab0ccd, 0x07500889, 0x0572063e, 0x0d790e39, 0x097b0ba3, 0x05d10750, 0x03f004be,
        0x0ba30c31, 0x08420a3d, 0x04be063e, 0x02e903a8, 0x0a3d0aab, 0x071c08d4, 0x03f00572, 0x023a02e9,
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0c31, 0x10001000, 0x10001000, 0x0d790f0f, 0x0a3d0ba3,
        0x10001000, 0x0e390f0f, 0x0ba30ccd, 0x08d40a3d, 0x10001000, 0x0c310e39, 0x097b0aab, 0x071c0842,
        0x0f0f0f0f, 0x0aab0ccd, 0x07500889, 0x0572063e, 0x0d790e39, 0x097b0ba3, 0x05d10750, 0x03f004be,
        0x0ba30c31, 0x08420a3d, 0x04be063e, 0x02e903a8, 0x0a3d0aab, 0x071c08d4, 0x03f00572, 0x023a02e9,
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x10001000, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab,
        0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925,
        0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925, 0x04be063e,
        0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x039b04be, 0x0a3d0aab, 0x07c20925, 0x04be063e, 0x02d0039b,
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x10001000, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab,
        0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925,
        0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925, 0x04be063e,
        0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x039b04be, 0x0a3d0aab, 0x07c20925, 0x04be063e, 0x02d0039b,
        0x10001000, 0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x10001000, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab,
        0x10001000, 0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x0f0f1000, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925,
        0x0e390f0f, 0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x0ccd0e39, 0x0a3d0aab, 0x07c20925, 0x04be063e,
        0x0aab0ccd, 0x09250a3d, 0x063e07c2, 0x039b04be, 0x0a3d0aab, 0x07c20925, 0x04be063e, 0x02d0039b,
        0x10001000, 0x10001000, 0x10001000, 0x10101010, 0x0e390f0f, 0x0aab0c31, 0x10001000, 0x18151211,
        0x10001000, 0x10001000, 0x0e390f0f, 0x10101010, 0x0d790f0f, 0x0a3d0ba3, 0x0aab0c31, 0x19161311,
        0x10001000, 0x0e390f0f, 0x10001000, 0x12111010, 0x0ba30ccd, 0x08d40a3d, 0x10001000, 0x1d191614,
        0x10001000, 0x0c310e39, 0x0d790f0f, 0x15121010, 0x097b0aab, 0x071c0842, 0x0a3d0ba3, 0x241f1b18,
        0x0f0f0f0f, 0x0aab0ccd, 0x10001000, 0x18141111, 0x07500889, 0x0572063e, 0x0e390f0f, 0x2f29231e,
        0x0d790e39, 0x097b0ba3, 0x0ba30ccd, 0x1b161312, 0x05d10750, 0x03f004be, 0x08d40a3d, 0x41362c23,
        0x0ba30c31, 0x08420a3d, 0x10001000, 0x1f191615, 0x04be063e, 0x02e903a8, 0x0c310e39, 0x58463629,
        0x0a3d0aab, 0x071c08d4, 0x097b0aab, 0x241d1918, 0x03f00572, 0x023a02e9, 0x071c0842, 0x7358412f,
        0x10001000, 0x10001000, 0x0f0f0f0f, 0x10101010, 0x0e390f0f, 0x0aab0c31, 0x0aab0ccd, 0x18151211,
        0x10001000, 0x10001000, 0x07500889, 0x10101010, 0x0d790f0f, 0x0a3d0ba3, 0x0572063e, 0x19161311,
        0x10001000, 0x0e390f0f, 0x0d790e39, 0x12111010, 0x0ba30ccd, 0x08d40a3d, 0x097b0ba3, 0x1d191614,
        0x10001000, 0x0c310e39, 0x05d10750, 0x15121010, 0x097b0aab, 0x071c0842, 0x03f004be, 0x241f1b18,
        0x0f0f0f0f, 0x0aab0ccd, 0x0ba30c31, 0x18141111, 0x07500889, 0x0572063e, 0x08420a3d, 0x2f29231e,
        0x0d790e39, 0x097b0ba3, 0x04be063e, 0x1b161312, 0x05d10750, 0x03f004be, 0x02e903a8, 0x41362c23,
        0x0ba30c31, 0x08420a3d, 0x0a3d0aab, 0x1f191615, 0x04be063e, 0x02e903a8, 0x071c08d4, 0x58463629,
        0x0a3d0aab, 0x071c08d4, 0x03f00572, 0x241d1918, 0x03f00572, 0x023a02e9, 0x023a02e9, 0x7358412f,
        0x10001000, 0x10001000, 0x10001000, 0x10101010, 0x0e390f0f, 0x0aab0c31, 0x10001000, 0x18151211,
        0x10001000, 0x10001000, 0x0e390f0f, 0x10101010, 0x0d790f0f, 0x0a3d0ba3, 0x0aab0ccd, 0x19161311,
        0x10001000, 0x0e390f0f, 0x10001000, 0x12111010, 0x0ba30ccd, 0x08d40a3d, 0x0f0f1000, 0x1d191614,
        0x10001000, 0x0c310e39, 0x0ccd0e39, 0x15121010, 0x097b0aab, 0x071c0842, 0x0a3d0aab, 0x241f1b18,
        0x0f0f0f0f, 0x0aab0ccd, 0x10001000, 0x18141111, 0x07500889, 0x0572063e, 0x0e390f0f, 0x2f29231e,
        0x0d790e39, 0x097b0ba3, 0x0aab0ccd, 0x1b161312, 0x05d10750, 0x03f004be, 0x09250a3d, 0x41362c23,
        0x0ba30c31, 0x08420a3d, 0x0f0f1000, 0x1f191615, 0x04be063e, 0x02e903a8, 0x0ccd0e39, 0x58463629,
        0x0a3d0aab, 0x071c08d4, 0x0a3d0aab, 0x241d1918, 0x03f00572, 0x023a02e9, 0x07c20925, 0x7358412f,
        0x10001000, 0x10001000, 0x0e390f0f, 0x10101010, 0x0e390f0f, 0x0aab0ccd, 0x0aab0ccd, 0x18141211,
        0x10001000, 0x0f0f1000, 0x09250a3d, 0x11101010, 0x0ccd0e39, 0x0a3d0aab, 0x063e07c2, 0x19181412,
        0x10001000, 0x0e390f0f, 0x0ccd0e39, 0x12111010, 0x0aab0ccd, 0x09250a3d, 0x0a3d0aab, 0x1c191814,
        0x0f0f1000, 0x0ccd0e39, 0x07c20925, 0x14121110, 0x0a3d0aab, 0x07c20925, 0x04be063e, 0x211c1918,
        0x0e390f0f, 0x0aab0ccd, 0x0aab0ccd, 0x18141211, 0x09250a3d, 0x063e07c2, 0x09250a3d, 0x29211c19,
        0x0ccd0e39, 0x0a3d0aab, 0x063e07c2, 0x19181412, 0x07c20925, 0x04be063e, 0x039b04be, 0x3629211c,
        0x0aab0ccd, 0x09250a3d, 0x0a3d0aab, 0x1c191814, 0x063e07c2, 0x039b04be, 0x07c20925, 0x47362921,
        0x0a3d0aab, 0x07c20925, 0x04be063e, 0x211c1918, 0x04be063e, 0x02d0039b, 0x02d0039b, 0x5b473629,
        0x10001000, 0x10001000, 0x10101010, 0x10101010, 0x0e390f0f, 0x0aab0ccd, 0x10101010, 0x18141211,
        0x10001000, 0x0f0f1000, 0x12111211, 0x11101010, 0x0ccd0e39, 0x0a3d0aab, 0x18141815, 0x19181412,
        0x10001000, 0x0e390f0f, 0x10101010, 0x12111010, 0x0aab0ccd, 0x09250a3d, 0x11101010, 0x1c191814,
        0x0f0f1000, 0x0ccd0e39, 0x14121311, 0x14121110, 0x0a3d0aab, 0x07c20925, 0x19181916, 0x211c1918,
        0x0e390f0f, 0x0aab0ccd, 0x10101010, 0x18141211, 0x09250a3d, 0x063e07c2, 0x12111211, 0x29211c19,
        0x0ccd0e39, 0x0a3d0aab, 0x18141614, 0x19181412, 0x07c20925, 0x04be063e, 0x1c191d19, 0x3629211c,
        0x0aab0ccd, 0x09250a3d, 0x11101010, 0x1c191814, 0x063e07c2, 0x039b04be, 0x14121512, 0x47362921,
        0x0a3d0aab, 0x07c20925, 0x19181b18, 0x211c1918, 0x04be063e, 0x02d0039b, 0x211c241f, 0x5b473629,
        0x10001000, 0x10001000, 0x12111111, 0x10101010, 0x0e390f0f, 0x0aab0ccd, 0x18141814, 0x18141211,
        0x10001000, 0x0f0f1000, 0x1c19231e, 0x11101010, 0x0ccd0e39, 0x0a3d0aab, 0x29212f29, 0x19181412,
        0x10001000, 0x0e390f0f, 0x14121312, 0x12111010, 0x0aab0ccd, 0x09250a3d, 0x19181b16, 0x1c191814,
        0x0f0f1000, 0x0ccd0e39, 0x211c2c23, 0x14121110, 0x0a3d0aab, 0x07c20925, 0x36294136, 0x211c1918,
        0x0e390f0f, 0x0aab0ccd, 0x18141615, 0x18141211, 0x09250a3d, 0x063e07c2, 0x1c191f19, 0x29211c19,
        0x0ccd0e39, 0x0a3d0aab, 0x29213629, 0x19181412, 0x07c20925, 0x04be063e, 0x47365846, 0x3629211c,
        0x0aab0ccd, 0x09250a3d, 0x19181918, 0x1c191814, 0x063e07c2, 0x039b04be, 0x211c241d, 0x47362921,
        0x0a3d0aab, 0x07c20925, 0x3629412f, 0x211c1918, 0x04be063e, 0x02d0039b, 0x5b477358, 0x5b473629,
        0x10101010, 0x18151211, 0x10101010, 0x18141211, 0x10101010, 0x19161311, 0x11101010, 0x19181412,
        0x12111010, 0x1d191614, 0x12111010, 0x1c191814, 0x15121010, 0x241f1b18, 0x14121110, 0x211c1918,
        0x18141111, 0x2f29231e, 0x18141211, 0x29211c19, 0x1b161312, 0x41362c23, 0x19181412, 0x3629211c,
        0x1f191615, 0x58463629, 0x1c191814, 0x47362921, 0x241d1918, 0x7358412f, 0x211c1918, 0x5b473629,
        0x10101010, 0x18151211, 0x10101010, 0x18141211, 0x10101010, 0x19161311, 0x11101010, 0x19181412,
        0x12111010, 0x1d191614, 0x12111010, 0x1c191814, 0x15121010, 0x241f1b18, 0x14121110, 0x211c1918,
        0x18141111, 0x2f29231e, 0x18141211, 0x29211c19, 0x1b161312, 0x41362c23, 0x19181412, 0x3629211c,
        0x1f191615, 0x58463629, 0x1c191814, 0x47362921, 0x241d1918, 0x7358412f, 0x211c1918, 0x5b473629,
        0x10101010, 0x18151211, 0x10101010, 0x18141211, 0x10101010, 0x19161311, 0x11101010, 0x19181412,
        0x12111010, 0x1d191614, 0x12111010, 0x1c191814, 0x15121010, 0x241f1b18, 0x14121110, 0x211c1918,
        0x18141111, 0x2f29231e, 0x18141211, 0x29211c19, 0x1b161312, 0x41362c23, 0x19181412, 0x3629211c,
        0x1f191615, 0x58463629, 0x1c191814, 0x47362921, 0x241d1918, 0x7358412f, 0x211c1918, 0x5b473629,
        0x10001000, 0x10001000, 0x10001000, 0x10001000, 0x10101010, 0x10101010,
    };

    hal_h265e_dbg_func("enter\n");

    memcpy(&reg->scaling_list_reg[0], vepu580_h265_scl_tab, sizeof(vepu580_h265_scl_tab));

    hal_h265e_dbg_func("leave\n");
}
static void vepu580_h265_global_cfg_set(H265eV580HalContext *ctx, H265eV580RegSet *regs)
{
    MppEncHwCfg *hw = &ctx->cfg->hw;
    RK_U32 i;
    hevc_vepu580_rc_klut *rc_regs = &regs->reg_rc_klut;
    hevc_vepu580_wgt *reg_wgt = &regs->reg_wgt;
    vepu580_rdo_cfg  *reg_rdo = &regs->reg_rdo;

    vepu580_h265_sobel_cfg(reg_wgt);
    vepu580_h265_rdo_cfg(reg_rdo);
    vepu580_h265_rdo_bias_cfg(reg_rdo, hw);
    vepu580_h265_scl_cfg(reg_rdo);

    memcpy(&reg_wgt->iprd_wgt_qp_hevc_0_51[0], lamd_satd_qp, sizeof(lamd_satd_qp));

    if (ctx->frame_type == INTRA_FRAME) {
        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            rc_regs->aq_tthd[i] = hw->aq_thrd_i[i];
            rc_regs->aq_step[i] = hw->aq_step_i[i] & 0x3f;
        }
        memcpy(&reg_wgt->rdo_wgta_qp_grpa_0_51[0], lamd_moda_qp, sizeof(lamd_moda_qp));
    } else {
        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            rc_regs->aq_tthd[i] = hw->aq_thrd_p[i];
            rc_regs->aq_step[i] = hw->aq_step_p[i] & 0x3f;
        }
        memcpy(&reg_wgt->rdo_wgta_qp_grpa_0_51[0], lamd_modb_qp, sizeof(lamd_modb_qp));
    }
    //to be done
    rc_regs->madi_cfg.madi_mode    = 0;
    rc_regs->madi_cfg.madi_thd     = 25;
    rc_regs->md_sad_thd.md_sad_thd0 = 20;
    rc_regs->md_sad_thd.md_sad_thd1 = 30;
    rc_regs->md_sad_thd.md_sad_thd2 = 40;
    rc_regs->madi_thd.madi_thd0    = 25;
    rc_regs->madi_thd.madi_thd1    = 35;
    rc_regs->madi_thd.madi_thd2    = 45;
    reg_wgt->reg1484_qnt_bias_comb.qnt_bias_i = 171;
    reg_wgt->reg1484_qnt_bias_comb.qnt_bias_p = 85;

    memcpy(&reg_wgt->lvl32_intra_CST_THD0, lvl32_intra_cst_thd, sizeof(lvl32_intra_cst_thd));
    memcpy(&reg_wgt->lvl16_intra_CST_THD0, lvl16_intra_cst_thd, sizeof(lvl16_intra_cst_thd));
    memcpy(&reg_wgt->lvl32_intra_CST_WGT0, lvl32_intra_cst_wgt, sizeof(lvl32_intra_cst_wgt));
    memcpy(&reg_wgt->lvl16_intra_CST_WGT0, lvl16_intra_cst_wgt, sizeof(lvl16_intra_cst_wgt));

    reg_wgt->cime_sqi_cfg.cime_sad_mod_sel      = 0;
    reg_wgt->cime_sqi_cfg.cime_sad_use_big_block = 0;
    reg_wgt->cime_sqi_cfg.cime_pmv_set_zero     = 1;
    reg_wgt->cime_sqi_cfg.cime_pmv_num          = 3;
    reg_wgt->cime_sqi_thd.cime_mvd_th0 = 32;
    reg_wgt->cime_sqi_thd.cime_mvd_th1 = 80;
    reg_wgt->cime_sqi_thd.cime_mvd_th2 = 128;
    reg_wgt->cime_sqi_multi0.cime_multi0 = 16;
    reg_wgt->cime_sqi_multi0.cime_multi1 = 32;
    reg_wgt->cime_sqi_multi1.cime_multi2 = 96;
    reg_wgt->cime_sqi_multi1.cime_multi3 = 96;
    reg_wgt->rime_sqi_thd.cime_sad_th0 = 48;
    reg_wgt->rime_sqi_thd.rime_mvd_th0 = 3;
    reg_wgt->rime_sqi_thd.rime_mvd_th1 = 8;
    reg_wgt->rime_sqi_multi.rime_multi0 = 16;
    reg_wgt->rime_sqi_multi.rime_multi1 = 16;
    reg_wgt->rime_sqi_multi.rime_multi2 = 128;
    reg_wgt->fme_sqi_thd0.cime_sad_pu16_th = 16;
    reg_wgt->fme_sqi_thd0.cime_sad_pu32_th = 16;
    reg_wgt->fme_sqi_thd1.cime_sad_pu64_th = 16;
    reg_wgt->fme_sqi_thd1.move_lambda = 1;
}

MPP_RET hal_h265e_v580_deinit(void *hal)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    RK_S32 i;

    hal_h265e_enter();

    for (i = 0; i < ctx->task_cnt; i++) {
        Vepu580H265eFrmCfg *frm = ctx->frms[i];
        RK_U32 j;

        if (!frm)
            continue;

        for (j = 0; j < MAX_TILE_NUM; j++) {
            MPP_FREE(frm->regs_set[j]);
            MPP_FREE(frm->regs_ret[j]);
        }

        for (j = 0; j < MAX_TILE_NUM; j++) {
            if (frm->hw_tile_buf[j]) {
                mpp_buffer_put(frm->hw_tile_buf[j]);
                frm->hw_tile_buf[j] = NULL;
            }
        }

        for (j = 0; j < MAX_TILE_NUM - 1; j++) {
            if (frm->hw_tile_stream[j]) {
                mpp_buffer_put(frm->hw_tile_stream[j]);
                frm->hw_tile_stream[j] = NULL;
            }
        }

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

    MPP_FREE(ctx->poll_cfgs);
    MPP_FREE(ctx->input_fmt);
    hal_bufs_deinit(ctx->dpb_bufs);

    if (ctx->tile_grp) {
        mpp_buffer_group_put(ctx->tile_grp);
        ctx->tile_grp = NULL;
    }

    if (ctx->roi_grp) {
        mpp_buffer_group_put(ctx->roi_grp);
        ctx->roi_grp = NULL;
    }

    if (ctx->buf_pass1) {
        mpp_buffer_put(ctx->buf_pass1);
        ctx->buf_pass1 = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }

    if (ctx->tune) {
        vepu580_h265e_tune_deinit(ctx->tune);
        ctx->tune = NULL;
    }

    if (ctx->md_info_buf) {
        mpp_buffer_put(ctx->md_info_buf);
        ctx->md_info_buf = NULL;
    }

    if (ctx->qpmap_base_cfg_buf) {
        mpp_buffer_put(ctx->qpmap_base_cfg_buf);
        ctx->qpmap_base_cfg_buf = NULL;
    }

    if (ctx->qpmap_qp_cfg_buf) {
        mpp_buffer_put(ctx->qpmap_qp_cfg_buf);
        ctx->qpmap_qp_cfg_buf = NULL;
    }

    if (ctx->md_flag_buf) {
        MPP_FREE(ctx->md_flag_buf);
    }

    if (ctx->qpmap_grp) {
        mpp_buffer_group_put(ctx->qpmap_grp);
        ctx->qpmap_grp = NULL;
    }

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_v580_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    RK_S32 i = 0;

    mpp_env_get_u32("hal_h265e_debug", &hal_h265e_debug, 0);

    hal_h265e_enter();

    ctx->input_fmt     = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg           = cfg->cfg;
    hal_bufs_init(&ctx->dpb_bufs);

    ctx->frame_count = 0;
    ctx->enc_mode = RKV_ENC_MODE;
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        goto DONE;
    }

    ctx->dev = cfg->dev;
    ctx->task_cnt = cfg->task_cnt;
    mpp_assert(ctx->task_cnt && ctx->task_cnt <= MAX_FRAME_TASK_NUM);

    if (ctx->task_cnt > MAX_FRAME_TASK_NUM)
        ctx->task_cnt = MAX_FRAME_TASK_NUM;

    for (i = 0; i < cfg->task_cnt; i++) {
        Vepu580H265eFrmCfg *frm_cfg = mpp_calloc(Vepu580H265eFrmCfg, 1);

        frm_cfg->regs_set[0] = mpp_calloc(H265eV580RegSet, 1);
        frm_cfg->regs_ret[0] = mpp_calloc(H265eV580StatusElem, 1);

        frm_cfg->osd_cfg.reg_base = &frm_cfg->regs_set[0]->reg_osd_cfg;

        /* setup osd cfg */
        frm_cfg->osd_cfg.dev = ctx->dev;
        frm_cfg->osd_cfg.plt_cfg = &ctx->cfg->plt_cfg;
        frm_cfg->osd_cfg.osd_data = NULL;
        frm_cfg->osd_cfg.osd_data2 = NULL;
        mpp_dev_multi_offset_init(&frm_cfg->reg_cfg, 24);
        frm_cfg->osd_cfg.reg_cfg = frm_cfg->reg_cfg;

        frm_cfg->frame_type = INTRA_FRAME;

        ctx->frms[i] = frm_cfg;
    }

    {   /* setup default hardware config */
        MppEncHwCfg *hw = &cfg->cfg->hw;
        RK_U32 j;

        hw->qp_delta_row_i = 2;
        hw->qp_delta_row   = 2;
        hw->qbias_i = 171;
        hw->qbias_p = 85;
        hw->qbias_en = 0;

        memcpy(hw->aq_thrd_i, aq_thd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, aq_thd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, aq_qp_dealt_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, aq_qp_dealt_default, sizeof(hw->aq_step_p));

        for (j = 0; j < MPP_ARRAY_ELEMS(hw->mode_bias); j++)
            hw->mode_bias[j] = 8;

        hw->skip_sad  = 8;
        hw->skip_bias = 8;
    }

    ctx->tune = vepu580_h265e_tune_init(ctx);

    {
        // check parall tile ability
        const MppServiceCmdCap *cap = mpp_get_mpp_service_cmd_cap();

        ctx->tile_parall_en = cap->send_cmd > MPP_CMD_SET_SESSION_FD;
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
DONE:
    if (ret)
        hal_h265e_v580_deinit(hal);

    hal_h265e_leave();
    return ret;
}
static MPP_RET hal_h265e_vepu580_prepare(void *hal)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h265e_dbg_func("enter %p\n", hal);

    if (prep->change & (MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT)) {
        RK_S32 i;

        // pre-alloc required buffers to reduce first frame delay
        vepu580_h265_setup_hal_bufs(ctx);
        for (i = 0; i < ctx->max_buf_cnt; i++)
            hal_bufs_get_buf(ctx->dpb_bufs, i);

        prep->change = 0;
    }

    hal_h265e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET
vepu580_h265_set_patch_info(MppDevRegOffCfgs *cfgs, H265eSyntax_new *syn,
                            VepuFmt input_fmt, HalEncTask *task)
{
    MppFrameFormat fmt = mpp_frame_get_fmt(task->frame);
    RK_U32 hor_stride = syn->pp.hor_stride;
    RK_U32 ver_stride = syn->pp.ver_stride ? syn->pp.ver_stride : syn->pp.pic_height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (task->rc_task->frm.use_pass1)
        fmt = MPP_FMT_YUV420SP;

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        u_offset = mpp_frame_get_fbc_offset(task->frame);
        v_offset = 0;
    } else {
        switch (input_fmt) {
        case VEPU5xx_FMT_YUV420P: {
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        } break;
        case VEPU5xx_FMT_YUV420SP:
        case VEPU5xx_FMT_YUV422SP: {
            u_offset = frame_size;
            v_offset = frame_size;
        } break;
        case VEPU5xx_FMT_YUV422P: {
            u_offset = frame_size;
            v_offset = frame_size * 3 / 2;
        } break;
        case VEPU5xx_FMT_YUV400:
        case VEPU5xx_FMT_YUYV422:
        case VEPU5xx_FMT_UYVY422: {
            u_offset = 0;
            v_offset = 0;
        } break;
        case VEPU5xx_FMT_YUV444SP : {
            u_offset = hor_stride * ver_stride;
            v_offset = hor_stride * ver_stride;
        } break;
        case VEPU5xx_FMT_YUV444P : {
            u_offset = hor_stride * ver_stride;
            v_offset = hor_stride * ver_stride * 2;
        } break;
        case VEPU5xx_FMT_BGR565:
        case VEPU5xx_FMT_BGR888:
        case VEPU5xx_FMT_BGRA8888: {
            u_offset = 0;
            v_offset = 0;
        } break;
        default: {
            hal_h265e_err("unknown color space: %d\n", input_fmt);
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        }
        }
    }

    /* input cb addr */
    ret = mpp_dev_multi_offset_update(cfgs, 161, u_offset);
    if (ret)
        mpp_err_f("set input cb addr offset failed %d\n", ret);

    /* input cr addr */
    ret = mpp_dev_multi_offset_update(cfgs, 162, v_offset);
    if (ret)
        mpp_err_f("set input cr addr offset failed %d\n", ret);

    return ret;
}

typedef struct refresh_area {
    RK_S32 roi_ctu_x_sta;
    RK_S32 roi_ctu_y_sta;
    RK_S32 roi_ctu_x_end;
    RK_S32 roi_ctu_y_end;
} RefreshArea;

static MPP_RET cal_refresh_area(RK_S32 ctu_w, RK_S32 ctu_h, RK_U32 refresh_idx,
                                MppEncRcRefreshMode refresh_mode, RK_U32 refresh_num, RefreshArea *area)
{
    MPP_RET ret = MPP_OK;
    RK_U32 refresh_ctu_h = 0;

    if (refresh_mode == MPP_ENC_RC_INTRA_REFRESH_ROW) {
        area->roi_ctu_x_sta = 0;
        area->roi_ctu_x_end = ctu_w - 1;

        if (refresh_idx > 0) {
            refresh_ctu_h = refresh_num + 1;
            area->roi_ctu_y_sta = refresh_num * refresh_idx - 1;
        } else {
            refresh_ctu_h = refresh_num;
            area->roi_ctu_y_sta = 0;
        }

        area->roi_ctu_y_end = area->roi_ctu_y_sta + refresh_ctu_h - 1;
    } else {
        area->roi_ctu_y_sta = 0;
        area->roi_ctu_y_end = ctu_h - 1;

        if (refresh_idx > 0) {
            refresh_ctu_h = refresh_num + 1;
            area->roi_ctu_x_sta = refresh_num * refresh_idx - 1;
        } else {
            refresh_ctu_h = refresh_num;
            area->roi_ctu_x_sta = 0;
        }

        area->roi_ctu_x_end = area->roi_ctu_x_sta + refresh_ctu_h - 1;
    }

    area->roi_ctu_x_end = MPP_MIN(area->roi_ctu_x_end, ctu_w - 1);
    area->roi_ctu_y_end = MPP_MIN(area->roi_ctu_y_end, ctu_h - 1);
    area->roi_ctu_x_sta = MPP_MAX(area->roi_ctu_x_sta, 0);
    area->roi_ctu_y_sta = MPP_MAX(area->roi_ctu_y_sta, 0);

    hal_h265e_dbg_detail("size in ctu : %d x %d, refresh_num %d, refresh_idx %d, area x[%d, %d], y[%d, %d]",
                         ctu_w, ctu_h, refresh_num, refresh_idx,
                         area->roi_ctu_x_sta, area->roi_ctu_x_end,
                         area->roi_ctu_y_sta, area->roi_ctu_y_end);

    return ret;
}

static MPP_RET setup_intra_refresh(H265eV580HalContext *ctx, RK_U32 refresh_idx)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;
    H265eV580RegSet *regs = frm->regs_set[0];
    RK_U32 w = ctx->cfg->prep.width;
    RK_U32 h = ctx->cfg->prep.height;
    RK_S32 ctu_w = MPP_ALIGN(w, 64) / 64;
    RK_S32 ctu_h = MPP_ALIGN(h, 64) / 64;
    RK_S32 roi_base_cfg_buf_size = ctu_w * ctu_h * 64;
    MppEncROICfg2 *external_roi_cfg = (MppEncROICfg2 *)frm->roi_data;
    RK_U8 *roi_base_cfg_hw_ptr = NULL;
    RK_S32 roi_base_cfg_buf_fd = 0;
    RefreshArea cur_area;
    RK_S32 j, k;
    MPP_RET ret = MPP_OK;

    hal_h265e_dbg_func("enter\n");

    if (!ctx->cfg->rc.refresh_en) {
        ret = MPP_ERR_VALUE;
        goto __RET;
    }

    if (frm->roi_data) {
        roi_base_cfg_hw_ptr = mpp_buffer_get_ptr(external_roi_cfg->base_cfg_buf);
        roi_base_cfg_buf_fd = mpp_buffer_get_fd(external_roi_cfg->base_cfg_buf);
    } else {
        if (frm->roi_base_buf_size < roi_base_cfg_buf_size) {
            if (NULL == ctx->roi_grp)
                mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);
            if (frm->roi_base_cfg_buf)
                mpp_buffer_put(frm->roi_base_cfg_buf);
            MPP_FREE(frm->roi_base_cfg_sw_buf);
            frm->roi_base_cfg_sw_buf = mpp_malloc(RK_U8, roi_base_cfg_buf_size);
            mpp_buffer_get(ctx->roi_grp, &frm->roi_base_cfg_buf, roi_base_cfg_buf_size);
        }
        roi_base_cfg_hw_ptr = mpp_buffer_get_ptr(frm->roi_base_cfg_buf);
        roi_base_cfg_buf_fd = mpp_buffer_get_fd(frm->roi_base_cfg_buf);
    }

    frm->roi_base_buf_size = roi_base_cfg_buf_size;

    memset(frm->roi_base_cfg_sw_buf, 0, roi_base_cfg_buf_size);

    if (MPP_OK != cal_refresh_area(ctu_w, ctu_h, refresh_idx, ctx->cfg->rc.refresh_mode, ctx->cfg->rc.refresh_num, &cur_area)) {
        ret = MPP_ERR_VALUE;
        mpp_err_f("setting refresh area out of range");
        goto __RET;
    }

    RK_U8 *ptr = frm->roi_base_cfg_sw_buf;
    for (j = 0; j < ctu_h; j++) {
        for (k = 0; k < ctu_w; k++) {
            if (j <= cur_area.roi_ctu_y_end && j >= cur_area.roi_ctu_y_sta &&
                k <= cur_area.roi_ctu_x_end && k >= cur_area.roi_ctu_x_sta) {

                memset(ptr + 22, 0x55, 20); //176~336
                *(ptr + 21) = 0x54; //170~175
                *(ptr + 42) = 0x05; //336~339
            }
            ptr += 64;
        }
    }

    memcpy(roi_base_cfg_hw_ptr, frm->roi_base_cfg_sw_buf, roi_base_cfg_buf_size);

    if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_ROW)
        regs->reg_base.reg0220_me_rnge.cme_srch_v = 1;
    else if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_COL)
        regs->reg_base.reg0220_me_rnge.cme_srch_h = 1;

    regs->reg_base.reg0192_enc_pic.roi_en = 1;
    regs->reg_base.reg0178_roi_addr = roi_base_cfg_buf_fd;
    mpp_buffer_sync_end(frm->roi_base_cfg_buf);

__RET:
    hal_h265e_dbg_func("leave, ret %d\n", ret);
    return ret;
}


static MPP_RET vepu580_h265_set_roi_regs(H265eV580HalContext *ctx, hevc_vepu580_base *regs)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;

    /* memset register on start so do not clear registers again here */
    if (frm->roi_data) {
        /* roi setup */
        RK_U32 ctu_w = MPP_ALIGN(ctx->cfg->prep.width, 64) / 64;
        RK_U32 ctu_h  = MPP_ALIGN(ctx->cfg->prep.height, 64) / 64;
        RK_U32 base_cfg_size = ctu_w * ctu_h * 64;
        RK_U32 qp_cfg_size   = ctu_w * ctu_h * 256;
        RK_U32 amv_cfg_size  = ctu_w * ctu_h * 512;
        RK_U32 mv_cfg_size   = ctu_w * ctu_h * 4;
        MppEncROICfg2 *cfg   = (MppEncROICfg2 *)frm->roi_data;

        if (mpp_buffer_get_size(cfg->base_cfg_buf) >= base_cfg_size) {
            regs->reg0192_enc_pic.roi_en = 1;
            regs->reg0178_roi_addr = mpp_buffer_get_fd(cfg->base_cfg_buf);
        } else {
            mpp_err("roi base cfg buf not enough, roi is invalid");
        }

        if (cfg->roi_qp_en) {
            if (mpp_buffer_get_size(cfg->qp_cfg_buf) >= qp_cfg_size) {
                regs->reg0179_roi_qp_addr = mpp_buffer_get_fd(cfg->qp_cfg_buf);
                regs->reg0228_roi_en.roi_qp_en = 1;
            } else {
                mpp_err("roi qp cfg buf not enough, roi is invalid");
            }
        }

        if (cfg->roi_amv_en) {
            if (mpp_buffer_get_size(cfg->amv_cfg_buf) >= amv_cfg_size) {
                regs->reg0180_roi_amv_addr = mpp_buffer_get_fd(cfg->amv_cfg_buf);
                regs->reg0228_roi_en.roi_amv_en = 1;
            } else {
                mpp_err("roi amv cfg buf not enough, roi is invalid");
            }
        }

        if (cfg->roi_mv_en) {
            if (mpp_buffer_get_size(cfg->mv_cfg_buf) >= mv_cfg_size) {
                regs->reg0181_roi_mv_addr = mpp_buffer_get_fd(cfg->mv_cfg_buf);
                regs->reg0228_roi_en.roi_mv_en = 1;
            } else {
                mpp_err("roi mv cfg buf not enough, roi is invalid");
            }
        }
    }

    return MPP_OK;
}

static MPP_RET vepu580_h265_set_rc_regs(H265eV580HalContext *ctx, H265eV580RegSet *regs, HalEncTask *task)
{
    H265eSyntax_new *syn = ctx->syn;
    EncRcTaskInfo *rc_cfg = &task->rc_task->info;
    hevc_vepu580_base *reg_base = &regs->reg_base;
    hevc_vepu580_rc_klut *reg_rc = &regs->reg_rc_klut;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncHwCfg *hw = &cfg->hw;
    MppEncCodecCfg *codec = &cfg->codec;
    MppEncH265Cfg *h265 = &codec->h265;
    RK_S32 mb_wd64, mb_h64;
    mb_wd64 = (syn->pp.pic_width + 63) / 64;
    mb_h64 = (syn->pp.pic_height + 63) / 64;

    RK_U32 ctu_target_bits_mul_16 = (rc_cfg->bit_target << 4) / (mb_wd64 * mb_h64);
    RK_U32 ctu_target_bits;
    RK_S32 negative_bits_thd, positive_bits_thd;

    if (rc->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        reg_base->reg0192_enc_pic.pic_qp    = rc_cfg->quality_target;
        reg_base->reg0240_synt_sli1.sli_qp  = rc_cfg->quality_target;

        reg_base->reg213_rc_qp.rc_max_qp   = rc_cfg->quality_target;
        reg_base->reg213_rc_qp.rc_min_qp   = rc_cfg->quality_target;
    } else {
        if (ctu_target_bits_mul_16 >= 0x100000) {
            ctu_target_bits_mul_16 = 0x50000;
        }
        ctu_target_bits = (ctu_target_bits_mul_16 * mb_wd64) >> 4;
        negative_bits_thd = 0 - 5 * ctu_target_bits / 16;
        positive_bits_thd = 5 * ctu_target_bits / 16;

        reg_base->reg0192_enc_pic.pic_qp    = rc_cfg->quality_target;
        reg_base->reg0240_synt_sli1.sli_qp  = rc_cfg->quality_target;
        reg_base->reg212_rc_cfg.rc_en      = 1;
        reg_base->reg212_rc_cfg.aq_en  = 1;
        reg_base->reg212_rc_cfg.aq_mode    = 1;
        reg_base->reg212_rc_cfg.rc_ctu_num = mb_wd64;
        reg_base->reg213_rc_qp.rc_qp_range = (ctx->frame_type == INTRA_FRAME) ?
                                             hw->qp_delta_row_i : hw->qp_delta_row;
        reg_base->reg213_rc_qp.rc_max_qp   = rc_cfg->quality_max;
        reg_base->reg213_rc_qp.rc_min_qp   = rc_cfg->quality_min;
        reg_base->reg214_rc_tgt.ctu_ebit  = ctu_target_bits_mul_16;

        reg_rc->rc_dthd_0_8[0] = 2 * negative_bits_thd;
        reg_rc->rc_dthd_0_8[1] = negative_bits_thd;
        reg_rc->rc_dthd_0_8[2] = positive_bits_thd;
        reg_rc->rc_dthd_0_8[3] = 2 * positive_bits_thd;
        reg_rc->rc_dthd_0_8[4] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[5] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[6] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[7] = 0x7FFFFFFF;
        reg_rc->rc_dthd_0_8[8] = 0x7FFFFFFF;

        reg_rc->rc_adj0.qp_adj0    = -2;
        reg_rc->rc_adj0.qp_adj1    = -1;
        reg_rc->rc_adj0.qp_adj2    = 0;
        reg_rc->rc_adj0.qp_adj3    = 1;
        reg_rc->rc_adj0.qp_adj4    = 2;
        reg_rc->rc_adj1.qp_adj5    = 0;
        reg_rc->rc_adj1.qp_adj6    = 0;
        reg_rc->rc_adj1.qp_adj7    = 0;
        reg_rc->rc_adj1.qp_adj8    = 0;

        if (rc->rc_mode == MPP_ENC_RC_MODE_SMTRC) {
            reg_base->reg213_rc_qp.rc_qp_range = 0;
        }
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
    reg_rc->roi_qthd3.qpmap_mode  = h265->qpmap_mode;

    return MPP_OK;
}

static MPP_RET vepu580_h265_set_pp_regs(H265eV580RegSet *regs, VepuFmtCfg *fmt,
                                        MppEncPrepCfg *prep_cfg, HalEncTask *task)
{
    hevc_vepu580_control_cfg *reg_ctl = &regs->reg_ctl;
    hevc_vepu580_base        *reg_base = &regs->reg_base;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    reg_ctl->reg0012_dtrns_map.src_bus_edin = fmt->src_endian;
    reg_base->reg0198_src_fmt.src_cfmt = fmt->format;
    reg_base->reg0198_src_fmt.alpha_swap = fmt->alpha_swap;
    reg_base->reg0198_src_fmt.rbuv_swap = fmt->rbuv_swap;
    reg_base->reg0198_src_fmt.out_fmt = (prep_cfg->format == MPP_FMT_YUV400) ? 0 : 1;
    reg_base->reg0203_src_proc.src_mirr = prep_cfg->mirroring > 0;
    reg_base->reg0203_src_proc.src_rot = prep_cfg->rotation;

    if (MPP_FRAME_FMT_IS_YUV(prep_cfg->format))
        reg_base->reg0198_src_fmt.src_range = 1;
    else
        reg_base->reg0198_src_fmt.src_range = (prep_cfg->range == MPP_FRAME_RANGE_JPEG) ? 1 : 0;

    if (MPP_FRAME_FMT_IS_FBC(prep_cfg->format)) {
        stridey = mpp_frame_get_fbc_hdr_stride(task->frame);
        if (!stridey)
            stridey = MPP_ALIGN(prep_cfg->width, 16);
    } else if (prep_cfg->hor_stride) {
        stridey = prep_cfg->hor_stride;
    } else {
        if (fmt->format == VEPU5xx_FMT_BGRA8888 )
            stridey = prep_cfg->width * 4;
        else if (fmt->format == VEPU5xx_FMT_BGR888 )
            stridey = prep_cfg->width * 3;
        else if (fmt->format == VEPU5xx_FMT_BGR565 ||
                 fmt->format == VEPU5xx_FMT_YUYV422 ||
                 fmt->format == VEPU5xx_FMT_UYVY422)
            stridey = prep_cfg->width * 2;
        else
            stridey = prep_cfg->width;
    }

    switch (fmt->format) {
    case VEPU5xx_FMT_YUV444SP : {
        stridec = stridey * 2;
    } break;
    case VEPU5xx_FMT_YUV422SP :
    case VEPU5xx_FMT_YUV420SP :
    case VEPU5xx_FMT_YUV444P : {
        stridec = stridey;
    } break;
    default : {
        stridec = stridey / 2;
    } break;
    }

    if (reg_base->reg0198_src_fmt.src_cfmt < VEPU5xx_FMT_ARGB1555) {
        const VepuRgb2YuvCfg *cfg_coeffs = cfg_coeffs = get_rgb2yuv_cfg(prep_cfg->range, prep_cfg->color);

        hal_h265e_dbg_simple("input color range %d colorspace %d", prep_cfg->range, prep_cfg->color);

        reg_base->reg0199_src_udfy.csc_wgt_r2y = cfg_coeffs->_2y.r_coeff;
        reg_base->reg0199_src_udfy.csc_wgt_g2y = cfg_coeffs->_2y.g_coeff;
        reg_base->reg0199_src_udfy.csc_wgt_b2y = cfg_coeffs->_2y.b_coeff;

        reg_base->reg0200_src_udfu.csc_wgt_r2u = cfg_coeffs->_2u.r_coeff;
        reg_base->reg0200_src_udfu.csc_wgt_g2u = cfg_coeffs->_2u.g_coeff;
        reg_base->reg0200_src_udfu.csc_wgt_b2u = cfg_coeffs->_2u.b_coeff;

        reg_base->reg0201_src_udfv.csc_wgt_r2v = cfg_coeffs->_2v.r_coeff;
        reg_base->reg0201_src_udfv.csc_wgt_g2v = cfg_coeffs->_2v.g_coeff;
        reg_base->reg0201_src_udfv.csc_wgt_b2v = cfg_coeffs->_2v.b_coeff;

        reg_base->reg0202_src_udfo.csc_ofst_y = cfg_coeffs->_2y.offset;
        reg_base->reg0202_src_udfo.csc_ofst_u = cfg_coeffs->_2u.offset;
        reg_base->reg0202_src_udfo.csc_ofst_v = cfg_coeffs->_2v.offset;

        hal_h265e_dbg_simple("use color range %d colorspace %d", cfg_coeffs->dst_range, cfg_coeffs->color);
    }

    reg_base->reg0205_src_strd0.src_strd0  = stridey;
    reg_base->reg0206_src_strd1.src_strd1  = stridec;

    return MPP_OK;
}

static void vepu580_h265_set_slice_regs(H265eSyntax_new *syn, hevc_vepu580_base *regs)
{
    regs->reg0237_synt_sps.smpl_adpt_ofst_e    = syn->pp.sample_adaptive_offset_enabled_flag;//slice->m_sps->m_bUseSAO;
    regs->reg0237_synt_sps.num_st_ref_pic       = syn->pp.num_short_term_ref_pic_sets;
    regs->reg0237_synt_sps.num_lt_ref_pic       = syn->pp.num_long_term_ref_pics_sps;
    regs->reg0237_synt_sps.lt_ref_pic_prsnt     = syn->pp.long_term_ref_pics_present_flag;
    regs->reg0237_synt_sps.tmpl_mvp_e          = syn->pp.sps_temporal_mvp_enabled_flag;
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


    regs->reg0240_synt_sli1.sp_tc_ofst_div2       = syn->sp.sli_tc_ofst_div2;
    regs->reg0240_synt_sli1.sp_beta_ofst_div2     = syn->sp.sli_beta_ofst_div2;
    regs->reg0240_synt_sli1.sli_lp_fltr_acrs_sli  = syn->sp.sli_lp_fltr_acrs_sli;
    regs->reg0240_synt_sli1.sp_dblk_fltr_dis      = syn->sp.sli_dblk_fltr_dis;
    regs->reg0240_synt_sli1.dblk_fltr_ovrd_flg    = syn->sp.dblk_fltr_ovrd_flg;
    regs->reg0240_synt_sli1.sli_cb_qp_ofst        = syn->pp.pps_slice_chroma_qp_offsets_present_flag ?
                                                    syn->sp.sli_cb_qp_ofst : syn->pp.pps_cb_qp_offset;
    regs->reg0240_synt_sli1.max_mrg_cnd           = syn->sp.max_mrg_cnd;

    regs->reg0240_synt_sli1.col_ref_idx           = syn->sp.col_ref_idx;
    regs->reg0240_synt_sli1.col_frm_l0_flg        = syn->sp.col_frm_l0_flg;
    regs->reg0241_synt_sli2.sli_poc_lsb           = syn->sp.sli_poc_lsb;
    regs->reg0241_synt_sli2.sli_hdr_ext_len       = syn->sp.sli_hdr_ext_len;

}

static void vepu580_h265_set_ref_regs(H265eSyntax_new *syn, hevc_vepu580_base *regs)
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

    return;
}

static MPP_RET hal_h265e_v580_send_regs(MppDev dev, H265eV580RegSet *hw_regs, H265eV580StatusElem *reg_out)
{
    RK_U32 *regs = (RK_U32*)hw_regs;
    MppDevRegWrCfg cfg;
    MppDevRegRdCfg cfg1;
    MPP_RET ret = MPP_OK;
    RK_U32 i;

    cfg.reg = (RK_U32*)&hw_regs->reg_ctl;
    cfg.size = sizeof(hevc_vepu580_control_cfg);
    cfg.offset = VEPU580_CTL_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto FAILE;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_CTL_REGS) {
        regs = (RK_U32*)&hw_regs->reg_ctl;
        for (i = 0; i < sizeof(hevc_vepu580_control_cfg) / 4; i++) {
            hal_h265e_dbg_ctl("ctl reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_base;
    cfg.size = sizeof(hevc_vepu580_base);
    cfg.offset = VEPU580_BASE_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto FAILE;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_REGS) {
        regs = (RK_U32*)(&hw_regs->reg_base);
        for (i = 0; i < 32; i++) {
            hal_h265e_dbg_regs("hw add cfg reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
        regs += 32;
        for (i = 0; i < (sizeof(hevc_vepu580_base) - 128) / 4; i++) {
            hal_h265e_dbg_regs("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_rc_klut;
    cfg.size = sizeof(hevc_vepu580_rc_klut);
    cfg.offset = VEPU580_RCKULT_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);

    if (hal_h265e_debug & HAL_H265E_DBG_RCKUT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_rc_klut;
        for (i = 0; i < sizeof(hevc_vepu580_rc_klut) / 4; i++) {
            hal_h265e_dbg_rckut("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    if (ret) {
        mpp_err_f("set rc kult  write failed %d\n", ret);
        goto FAILE;
    }

    cfg.reg = &hw_regs->reg_wgt;
    cfg.size = sizeof(hevc_vepu580_wgt);
    cfg.offset = VEPU580_WEG_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto FAILE;
    }

    if (hal_h265e_debug & HAL_H265E_DBG_WGT_REGS) {
        regs = (RK_U32*)&hw_regs->reg_wgt;
        for (i = 0; i < sizeof(hevc_vepu580_wgt) / 4; i++) {
            hal_h265e_dbg_wgt("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }
    }

    cfg.reg = &hw_regs->reg_rdo;
    cfg.size = sizeof(vepu580_rdo_cfg);
    cfg.offset = VEPU580_RDOCFG_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto FAILE;
    }

    cfg.reg = &hw_regs->reg_osd_cfg;
    cfg.size = sizeof(vepu580_osd_cfg);
    cfg.offset = VEPU580_OSD_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &cfg);
    if (ret) {
        mpp_err_f("set register write failed %d\n", ret);
        goto FAILE;
    }

    cfg1.reg = &reg_out->hw_status;
    cfg1.size = sizeof(RK_U32);
    cfg1.offset = VEPU580_REG_BASE_HW_STATUS;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        goto FAILE;
    }

    cfg1.reg = &reg_out->st;
    cfg1.size = sizeof(H265eV580StatusElem) - 4;
    cfg1.offset = VEPU580_STATUS_OFFSET;

    ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &cfg1);
    if (ret) {
        mpp_err_f("set register read failed %d\n", ret);
        goto FAILE;
    }
FAILE:
    return ret;
}

static MPP_RET setup_vepu580_dual_core(H265eV580HalContext *ctx)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;
    H265eV580RegSet *regs = frm->regs_set[0];
    hevc_vepu580_base *reg_base = &regs->reg_base;
    RK_U32 dchs_ofst = 9;
    RK_U32 dchs_rxe  = 1;

    if (frm->frame_type == INTRA_FRAME) {
        ctx->curr_idx = 0;
        ctx->prev_idx = 0;
        dchs_rxe = 0;
    }

    reg_base->reg0193_dual_core.dchs_txid = ctx->curr_idx;
    reg_base->reg0193_dual_core.dchs_rxid = ctx->prev_idx;
    reg_base->reg0193_dual_core.dchs_txe = 1;
    reg_base->reg0193_dual_core.dchs_rxe = dchs_rxe;
    reg_base->reg0193_dual_core.dchs_ofst = dchs_ofst;

    ctx->prev_idx = ctx->curr_idx++;
    if (ctx->curr_idx > 3)
        ctx->curr_idx = 0;

    return MPP_OK;
}

static void vepu580_h265_set_me_regs(H265eV580HalContext *ctx, H265eSyntax_new *syn, hevc_vepu580_base *regs)
{

    RK_U32 cime_w = 11, cime_h = 7;
    RK_S32 merangx = (cime_w + 1) * 32;
    RK_S32 merangy = (cime_h + 1) * 32;
    RK_S32 pic_wd64 = MPP_ALIGN(syn->pp.pic_width, 64) >> 6;

    if (merangx > 384) {
        merangx = 384;
    }
    if (merangy > 320) {
        merangy = 320;
    }

    if (syn->pp.pic_width  < merangx + 60 || syn->pp.pic_width  <= 352) {
        if (merangx > syn->pp.pic_width ) {
            merangx = syn->pp.pic_width;
        }
        merangx = merangx / 4 * 2;
    }

    if (syn->pp.pic_height < merangy + 60 || syn->pp.pic_height <= 288) {
        if (merangy > syn->pp.pic_height) {
            merangy = syn->pp.pic_height;
        }
        merangy = merangy / 4 * 2;
    }

    {
        RK_S32 merange_x = merangx / 2;
        RK_S32 merange_y = merangy / 2;
        RK_S32 mxneg = ((-(merange_x << 2)) >> 2) / 4;
        RK_S32 myneg = ((-(merange_y << 2)) >> 2) / 4;
        RK_S32 mxpos = (((merange_x << 2) - 4) >> 2) / 4;
        RK_S32 mypos = (((merange_y << 2) - 4) >> 2) / 4;

        mxneg = MPP_MIN(abs(mxneg), mxpos) * 4;
        myneg = MPP_MIN(abs(myneg), mypos) * 4;

        merangx = mxneg * 2;
        merangy = myneg * 2;
    }
    regs->reg0220_me_rnge.cme_srch_h    = merangx / 32;
    regs->reg0220_me_rnge.cme_srch_v    = merangy / 32;

    regs->reg0220_me_rnge.rme_srch_h    = 7;
    regs->reg0220_me_rnge.rme_srch_v    = 5;
    regs->reg0220_me_rnge.dlt_frm_num    = 0x1;

    regs->reg0221_me_cfg.pmv_mdst_h    = 5;
    regs->reg0221_me_cfg.pmv_mdst_v    = 5;
    regs->reg0221_me_cfg.mv_limit      = 0;
    regs->reg0221_me_cfg.pmv_num        = 2;

    //regs->reg0221_me_cfg.rme_dis      = 0;
    // regs->reg0221_me_cfg.rme_dis        = 2;



    if (syn->pp.sps_temporal_mvp_enabled_flag &&
        (ctx->frame_type != INTRA_FRAME)) {
        if (ctx->last_frame_type == INTRA_FRAME) {
            regs->reg0221_me_cfg.colmv_load    = 0;
        } else {
            regs->reg0221_me_cfg.colmv_load    = 1;
        }
        regs->reg0221_me_cfg.colmv_stor   = 1;
    }

    if (syn->pp.pic_width > 2688) {
        regs->reg0222_me_cach.cme_rama_h = 12;
    } else if (syn->pp.pic_width > 2048) {
        regs->reg0222_me_cach.cme_rama_h = 16;
    } else {
        regs->reg0222_me_cach.cme_rama_h = 20;
    }

    {
        RK_S32 swin_scope_wd16 = (regs->reg0220_me_rnge.cme_srch_h  + 3 + 1) / 4 * 2 + 1;
        RK_S32 tmpMin = (regs->reg0220_me_rnge.cme_srch_v + 3) / 4 * 2 + 1;
        if (regs->reg0222_me_cach.cme_rama_h / 4 < tmpMin) {
            tmpMin = regs->reg0222_me_cach.cme_rama_h / 4;
        }
        regs->reg0222_me_cach.cme_rama_max =
            (pic_wd64 * (tmpMin - 1)) + ((pic_wd64 >= swin_scope_wd16) ? swin_scope_wd16 : pic_wd64 * 2);
    }
    regs->reg0222_me_cach.cach_l2_tag      = 0x0;

    pic_wd64 = pic_wd64 << 6;

    if (pic_wd64 <= 512)
        regs->reg0222_me_cach.cach_l2_tag  = 0x0;
    else if (pic_wd64 <= 1024)
        regs->reg0222_me_cach.cach_l2_tag  = 0x1;
    else if (pic_wd64 <= 2048)
        regs->reg0222_me_cach.cach_l2_tag  = 0x2;
    else if (pic_wd64 <= 4096)
        regs->reg0222_me_cach.cach_l2_tag  = 0x3;

}

static MppBuffer vepu580_h265_get_md_info_buf(H265eV580HalContext *ctx)
{
    RK_S32 w = ctx->cfg->prep.width;
    RK_S32 h = ctx->cfg->prep.height;
    RK_S32 buf_size = (MPP_ALIGN(w, 64) >> 6) * (MPP_ALIGN(h, 64) >> 6) * 32;

    /* prepare md info internal buffer when deblur is enabled */
    if (ctx->cfg->tune.deblur_en && (buf_size > ctx->md_info_buf_size)) {
        if (ctx->md_info_buf) {
            mpp_buffer_put(ctx->md_info_buf);
            ctx->md_info_buf = NULL;
        }

        if (!ctx->qpmap_grp)
            mpp_buffer_group_get_internal(&ctx->qpmap_grp, MPP_BUFFER_TYPE_ION);

        mpp_buffer_get(ctx->qpmap_grp, &ctx->md_info_buf, buf_size);
        if (!ctx->md_info_buf)
            mpp_err_f("failed to get md info buffer\n");
        else {
            hal_h265e_dbg_flow("get md info internal buffer %p size %d %d\n",
                               ctx->md_info_buf, buf_size, ctx->md_info_buf_size);
            ctx->md_info_buf_size = buf_size;
        }
    }

    return ctx->md_info_buf;
}

void vepu580_h265_set_hw_address(H265eV580HalContext *ctx, HalEncTask *task)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;
    hevc_vepu580_base *regs = &frm->regs_set[0]->reg_base;
    HalEncTask *enc_task = task;
    HalBuf *recon_buf, *ref_buf;
    H265eSyntax_new *syn = ctx->syn;

    hal_h265e_enter();

    regs->reg0160_adr_src0     = mpp_buffer_get_fd(enc_task->input);
    regs->reg0161_adr_src1     = regs->reg0160_adr_src0;
    regs->reg0162_adr_src2     = regs->reg0160_adr_src0;

    recon_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_curr_idx);
    ref_buf = hal_bufs_get_buf(ctx->dpb_bufs, frm->hal_refr_idx);

    if (!syn->sp.non_reference_flag) {
        regs->reg0163_rfpw_h_addr  = mpp_buffer_get_fd(recon_buf->buf[0]);
        regs->reg0164_rfpw_b_addr  = regs->reg0163_rfpw_h_addr;

        mpp_dev_multi_offset_update(frm->reg_cfg, 164, ctx->fbc_header_len);
    }
    regs->reg0165_rfpr_h_addr = mpp_buffer_get_fd(ref_buf->buf[0]);
    regs->reg0166_rfpr_b_addr = regs->reg0165_rfpr_h_addr;
    regs->reg0167_cmvw_addr = mpp_buffer_get_fd(recon_buf->buf[2]);
    regs->reg0168_cmvr_addr = mpp_buffer_get_fd(ref_buf->buf[2]);
    regs->reg0169_dspw_addr = mpp_buffer_get_fd(recon_buf->buf[1]);
    regs->reg0170_dspr_addr = mpp_buffer_get_fd(ref_buf->buf[1]);

    mpp_dev_multi_offset_update(frm->reg_cfg, 166, ctx->fbc_header_len);

    if (syn->pp.tiles_enabled_flag) {
        RK_U32 tile_num = (syn->pp.num_tile_columns_minus1 + 1) * (syn->pp.num_tile_rows_minus1 + 1);
        RK_U32 i = 0;
        RK_U32 max_tile_buf_size = MPP_ALIGN(((MPP_ALIGN(syn->pp.pic_height, 64) + 64) << 5), 256);

        if (NULL == ctx->tile_grp)
            mpp_buffer_group_get_internal(&ctx->tile_grp, MPP_BUFFER_TYPE_ION);

        mpp_assert(ctx->tile_grp);

        for (i = 0; i < MAX_TILE_NUM; i++) {
            if (NULL == frm->hw_tile_buf[i]) {
                mpp_buffer_get(ctx->tile_grp, &frm->hw_tile_buf[i], max_tile_buf_size);
            }
        }

        if (NULL == frm->hw_tile_stream[0]) {
            mpp_buffer_get(ctx->tile_grp, &frm->hw_tile_stream[0], ctx->frame_size / tile_num);
        }

        if (tile_num > 2) {
            if (NULL == frm->hw_tile_stream[1]) {
                mpp_buffer_get(ctx->tile_grp, &frm->hw_tile_stream[1], ctx->frame_size / tile_num);
            }
            if (NULL == frm->hw_tile_stream[2]) {
                mpp_buffer_get(ctx->tile_grp, &frm->hw_tile_stream[2], ctx->frame_size / tile_num);
            }
        }

        regs->reg0176_lpfw_addr  = mpp_buffer_get_fd(frm->hw_tile_buf[0]);
        regs->reg0177_lpfr_addr  = mpp_buffer_get_fd(frm->hw_tile_buf[1]);
    }

    {
        if (!enc_task->md_info)
            enc_task->md_info = vepu580_h265_get_md_info_buf(ctx);

        if (enc_task->md_info) {
            regs->reg0192_enc_pic.mei_stor    = 1;
            regs->reg0171_meiw_addr = mpp_buffer_get_fd(enc_task->md_info);
        } else {
            regs->reg0192_enc_pic.mei_stor    = 0;
            regs->reg0171_meiw_addr = 0;
        }
    }

    regs->reg0172_bsbt_addr = mpp_buffer_get_fd(enc_task->output);
    /* TODO: stream size relative with syntax */
    regs->reg0173_bsbb_addr = regs->reg0172_bsbt_addr;
    regs->reg0174_bsbr_addr = regs->reg0172_bsbt_addr;
    regs->reg0175_adr_bsbs  = regs->reg0172_bsbt_addr;

    mpp_dev_multi_offset_update(frm->reg_cfg, 175, mpp_packet_get_length(task->packet));
    mpp_dev_multi_offset_update(frm->reg_cfg, 172, mpp_buffer_get_size(enc_task->output));

    regs->reg0204_pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->reg0204_pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);
}

static MPP_RET vepu580_h265e_save_pass1_patch(H265eV580RegSet *regs, H265eV580HalContext *ctx,
                                              RK_S32 tiles_enabled_flag)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;
    hevc_vepu580_base *reg_base = &regs->reg_base;
    RK_S32 width = ctx->cfg->prep.width;
    RK_S32 height = ctx->cfg->prep.height;
    RK_S32 width_align = MPP_ALIGN(width, 64);
    RK_S32 height_align = MPP_ALIGN(height, 16);

    if (NULL == ctx->buf_pass1) {
        mpp_buffer_get(NULL, &ctx->buf_pass1, width_align * height_align * 3 / 2);
        if (!ctx->buf_pass1) {
            mpp_err("buf_pass1 malloc fail, debreath invaild");
            return MPP_NOK;
        }
    }

    reg_base->reg0192_enc_pic.cur_frm_ref = 1;
    reg_base->reg0163_rfpw_h_addr = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_base->reg0164_rfpw_b_addr = reg_base->reg0163_rfpw_h_addr;
    reg_base->reg0192_enc_pic.rec_fbc_dis = 1;

    if (tiles_enabled_flag)
        reg_base->reg0238_synt_pps.lpf_fltr_acrs_til = 0;

    mpp_dev_multi_offset_update(frm->reg_cfg, 164, width_align * height_align);

    /* NOTE: disable split to avoid lowdelay slice output */
    regs->reg_base.reg0216_sli_splt.sli_splt = 0;
    regs->reg_base.reg0192_enc_pic.slen_fifo = 0;

    return MPP_OK;
}

static MPP_RET vepu580_h265e_use_pass1_patch(H265eV580RegSet *regs, H265eV580HalContext *ctx)
{
    Vepu580H265eFrmCfg *frm = ctx->frm;
    hevc_vepu580_control_cfg *reg_ctl = &regs->reg_ctl;
    hevc_vepu580_base *reg_base = &regs->reg_base;
    RK_U32 hor_stride = MPP_ALIGN(ctx->cfg->prep.width, 64);
    RK_U32 ver_stride = MPP_ALIGN(ctx->cfg->prep.height, 16);
    RK_U32 frame_size = hor_stride * ver_stride;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    MPP_RET ret = MPP_OK;

    reg_ctl->reg0012_dtrns_map.src_bus_edin = fmt->src_endian;
    reg_base->reg0198_src_fmt.src_cfmt = VEPU5xx_FMT_YUV420SP;
    reg_base->reg0198_src_fmt.alpha_swap = 0;
    reg_base->reg0198_src_fmt.rbuv_swap = 0;
    reg_base->reg0198_src_fmt.out_fmt = 1;
    reg_base->reg0203_src_proc.afbcd_en = 0;
    reg_base->reg0205_src_strd0.src_strd0 = hor_stride;
    reg_base->reg0206_src_strd1.src_strd1 = hor_stride;
    reg_base->reg0203_src_proc.src_mirr = 0;
    reg_base->reg0203_src_proc.src_rot = 0;
    reg_base->reg0204_pic_ofst.pic_ofst_x = 0;
    reg_base->reg0204_pic_ofst.pic_ofst_y = 0;
    reg_base->reg0160_adr_src0 = mpp_buffer_get_fd(ctx->buf_pass1);
    reg_base->reg0161_adr_src1 = reg_base->reg0160_adr_src0;
    reg_base->reg0162_adr_src2 = reg_base->reg0160_adr_src0;

    /* input cb addr */
    ret = mpp_dev_multi_offset_update(frm->reg_cfg, 161, frame_size);
    if (ret)
        mpp_err_f("set input cb addr offset failed %d\n", ret);

    /* input cr addr */
    ret = mpp_dev_multi_offset_update(frm->reg_cfg, 162, frame_size);
    if (ret)
        mpp_err_f("set input cr addr offset failed %d\n", ret);

    return MPP_OK;
}

static void vepu580_setup_split(H265eV580RegSet *regs, MppEncCfgSet *enc_cfg, RK_U32 title_en)
{
    MppEncSliceSplit *cfg = &enc_cfg->split;

    hal_h265e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        regs->reg_base.reg0216_sli_splt.sli_splt = 0;
        regs->reg_base.reg0216_sli_splt.sli_splt_mode = 0;
        regs->reg_base.reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg_base.reg0216_sli_splt.sli_max_num_m1 = 0;
        regs->reg_base.reg0216_sli_splt.sli_flsh = 0;
        regs->reg_base.reg0218_sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg_base.reg0217_sli_byte.sli_splt_byte = 0;
        regs->reg_base.reg0192_enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        regs->reg_base.reg0216_sli_splt.sli_splt = 1;
        regs->reg_base.reg0216_sli_splt.sli_splt_mode = 0;
        regs->reg_base.reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg_base.reg0216_sli_splt.sli_max_num_m1 = 500;
        regs->reg_base.reg0216_sli_splt.sli_flsh = 1;
        regs->reg_base.reg0218_sli_cnum.sli_splt_cnum_m1 = 0;

        regs->reg_base.reg0217_sli_byte.sli_splt_byte = cfg->split_arg;
        regs->reg_base.reg0192_enc_pic.slen_fifo = cfg->split_out ? 1 : 0;
        regs->reg_ctl.reg0008_int_en.slc_done_en = regs->reg_base.reg0192_enc_pic.slen_fifo;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        RK_U32 mb_w = MPP_ALIGN(enc_cfg->prep.width, 64) / 64;
        RK_U32 mb_h = MPP_ALIGN(enc_cfg->prep.height, 64) / 64;
        RK_U32 slice_num = 0;

        if (title_en)
            mb_w = mb_w / 2;

        slice_num = (mb_w * mb_h + cfg->split_arg - 1) / cfg->split_arg;

        regs->reg_base.reg0216_sli_splt.sli_splt = 1;
        regs->reg_base.reg0216_sli_splt.sli_splt_mode = 1;
        regs->reg_base.reg0216_sli_splt.sli_splt_cpst = 0;
        regs->reg_base.reg0216_sli_splt.sli_max_num_m1 = 500;
        regs->reg_base.reg0216_sli_splt.sli_flsh = 1;
        regs->reg_base.reg0218_sli_cnum.sli_splt_cnum_m1 = cfg->split_arg - 1;

        regs->reg_base.reg0217_sli_byte.sli_splt_byte = 0;
        regs->reg_base.reg0192_enc_pic.slen_fifo = cfg->split_out ? 1 : 0;

        if ((cfg->split_out & MPP_ENC_SPLIT_OUT_LOWDELAY) ||
            (regs->reg_base.reg0192_enc_pic.slen_fifo && (slice_num > VEPU580_SLICE_FIFO_LEN)))
            regs->reg_ctl.reg0008_int_en.slc_done_en = 1 ;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }

    cfg->change = 0;

    hal_h265e_dbg_func("leave\n");
}

MPP_RET hal_h265e_v580_gen_regs(void *hal, HalEncTask *task)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    HalEncTask *enc_task = task;
    EncRcTask *rc_task = enc_task->rc_task;
    EncFrmStatus *frm = &rc_task->frm;
    H265eSyntax_new *syn = ctx->syn;
    Vepu580H265eFrmCfg *frm_cfg = ctx->frm;
    H265eV580RegSet *regs = frm_cfg->regs_set[0];
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 pic_wd64, pic_h64;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    hevc_vepu580_control_cfg *reg_ctl = &regs->reg_ctl;
    hevc_vepu580_base        *reg_base = &regs->reg_base;
    hevc_vepu580_rc_klut *reg_klut = &regs->reg_rc_klut;
    MppEncCfgSet *cfg = ctx->cfg;

    hal_h265e_enter();
    pic_width_align8 = (syn->pp.pic_width + 7) & (~7);
    pic_height_align8 = (syn->pp.pic_height + 7) & (~7);
    pic_wd64 = (syn->pp.pic_width + 63) / 64;
    pic_h64 = (syn->pp.pic_height + 63) / 64;

    hal_h265e_dbg_simple("frame %d | type %d | start gen regs",
                         frm_cfg->frame_count, ctx->frame_type);

    memset(regs, 0, sizeof(H265eV580RegSet));

    if (ctx->task_cnt > 1)
        setup_vepu580_dual_core(ctx);

    reg_ctl->reg0004_enc_strt.lkt_num      = 0;
    reg_ctl->reg0004_enc_strt.vepu_cmd     = ctx->enc_mode;
    reg_ctl->reg0005_enc_clr.safe_clr      = 0x0;
    reg_ctl->reg0005_enc_clr.force_clr     = 0x0;

    reg_ctl->reg0008_int_en.enc_done_en         = 1;
    reg_ctl->reg0008_int_en.lkt_node_done_en    = 1;
    reg_ctl->reg0008_int_en.sclr_done_en        = 1;
    reg_ctl->reg0008_int_en.slc_done_en         = 0;
    reg_ctl->reg0008_int_en.bsf_oflw_en         = 1;
    reg_ctl->reg0008_int_en.brsp_otsd_en        = 1;
    reg_ctl->reg0008_int_en.wbus_err_en         = 1;
    reg_ctl->reg0008_int_en.rbus_err_en         = 1;
    reg_ctl->reg0008_int_en.wdg_en              = 1;
    reg_ctl->reg0008_int_en.lkt_err_int_en      = 0;

    reg_ctl->reg0012_dtrns_map.lpfw_bus_ordr    = 0x0;
    reg_ctl->reg0012_dtrns_map.cmvw_bus_ordr    = 0x0;
    reg_ctl->reg0012_dtrns_map.dspw_bus_ordr    = 0x0;
    reg_ctl->reg0012_dtrns_map.rfpw_bus_ordr    = 0x0;
    reg_ctl->reg0012_dtrns_map.src_bus_edin     = 0x0;
    reg_ctl->reg0012_dtrns_map.meiw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.bsw_bus_edin     = 0x7;
    reg_ctl->reg0012_dtrns_map.lktr_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.roir_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.lktw_bus_edin    = 0x0;
    reg_ctl->reg0012_dtrns_map.afbc_bsize       = 0x1;
    reg_ctl->reg0012_dtrns_map.ebufw_bus_ordr   = 0x0;

    reg_ctl->reg0013_dtrns_cfg.dspr_otsd        = (ctx->frame_type == INTER_P_FRAME);
    reg_ctl->reg0013_dtrns_cfg.axi_brsp_cke     = 0x0;
    reg_ctl->reg0014_enc_wdg.vs_load_thd        = 0x1fffff;
    reg_ctl->reg0014_enc_wdg.rfp_load_thd       = 0;

    reg_ctl->reg0021_func_en.cke                = 1;
    reg_ctl->reg0021_func_en.resetn_hw_en       = 1;
    reg_ctl->reg0021_func_en.enc_done_tmvp_en   = 1;

    reg_base->reg0196_enc_rsl.pic_wd8_m1    = pic_width_align8 / 8 - 1;
    reg_base->reg0197_src_fill.pic_wfill    = (syn->pp.pic_width & 0x7)
                                              ? (8 - (syn->pp.pic_width & 0x7)) : 0;
    reg_base->reg0196_enc_rsl.pic_hd8_m1    = pic_height_align8 / 8 - 1;
    reg_base->reg0197_src_fill.pic_hfill    = (syn->pp.pic_height & 0x7)
                                              ? (8 - (syn->pp.pic_height & 0x7)) : 0;

    reg_base->reg0192_enc_pic.enc_stnd      = 1; //H265
    reg_base->reg0192_enc_pic.cur_frm_ref   = !syn->sp.non_reference_flag; //current frame will be refered
    reg_base->reg0192_enc_pic.bs_scp        = 1;
    reg_base->reg0192_enc_pic.log2_ctu_num  = ceil(log2((double)pic_wd64 * pic_h64));

    reg_base->reg0203_src_proc.src_mirr = 0;
    reg_base->reg0203_src_proc.src_rot  = 0;
    reg_base->reg0203_src_proc.txa_en   = 1;
    reg_base->reg0203_src_proc.afbcd_en = (MPP_FRAME_FMT_IS_FBC(syn->pp.mpp_format)) ? 1 : 0;

    reg_klut->klut_ofst.chrm_klut_ofst = (ctx->frame_type == INTRA_FRAME) ? 0 : 3;
    memcpy(&reg_klut->klut_wgt0, &klut_weight[0], sizeof(klut_weight));

    vepu580_h265_set_me_regs(ctx, syn, reg_base);

    reg_base->reg0232_rdo_cfg.chrm_spcl   = 1;
    reg_base->reg0232_rdo_cfg.cu_inter_e    = 0x06db;
    reg_base->reg0232_rdo_cfg.cu_intra_e    = 0xf;

    if (syn->pp.num_long_term_ref_pics_sps) {
        reg_base->reg0232_rdo_cfg.ltm_col   = 0;
        reg_base->reg0232_rdo_cfg.ltm_idx0l0 = 1;
    } else {
        reg_base->reg0232_rdo_cfg.ltm_col   = 0;
        reg_base->reg0232_rdo_cfg.ltm_idx0l0 = 0;
    }

    reg_base->reg0232_rdo_cfg.ccwa_e = 1;
    reg_base->reg0232_rdo_cfg.scl_lst_sel = syn->pp.scaling_list_enabled_flag;
    reg_base->reg0236_synt_nal.nal_unit_type = h265e_get_nal_type(&syn->sp, ctx->frame_type);

    vepu580_h265_set_hw_address(ctx, task);
    vepu580_h265_set_pp_regs(regs, fmt, &ctx->cfg->prep, task);
    vepu580_h265_set_rc_regs(ctx, regs, task);
    vepu580_h265_set_slice_regs(syn, reg_base);
    vepu580_h265_set_ref_regs(syn, reg_base);

    vepu580_set_osd(&ctx->frm->osd_cfg);
    /* ROI configure */
    vepu580_h265_set_roi_regs(ctx, reg_base);
    if (frm->is_i_refresh)
        setup_intra_refresh(ctx, frm->seq_idx % ctx->cfg->rc.gop);

    if (cfg->tune.deblur_en && (!rc_task->info.complex_scene) &&
        cfg->rc.rc_mode == MPP_ENC_RC_MODE_SMTRC && task->md_info &&
        cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC) {
        if (MPP_OK != vepu580_setup_qpmap_buf(ctx))
            mpp_err("qpmap malloc buffer failed!\n");
    }

    /*paramet cfg*/
    vepu580_h265_global_cfg_set(ctx, regs);

    vepu580_h265e_tune_reg_patch(ctx->tune);

    vepu580_setup_split(regs, cfg, syn->pp.tiles_enabled_flag);

    hal_h265e_leave();
    return MPP_OK;
}

void hal_h265e_v580_set_uniform_tile(hevc_vepu580_base *regs, H265eSyntax_new *syn,
                                     RK_U32 index, RK_S32 tile_start_x)
{
    if (syn->pp.tiles_enabled_flag) {
        RK_S32 mb_h = MPP_ALIGN(syn->pp.pic_height, 64) / 64;
        RK_S32 tile_width = syn->pp.column_width_minus1[index] + 1;

        if (!regs->reg0192_enc_pic.cur_frm_ref &&
            !(regs->reg0238_synt_pps.lpf_fltr_acrs_til &&
              regs->reg0238_synt_pps.lp_fltr_acrs_sli &&
              regs->reg0240_synt_sli1.sli_lp_fltr_acrs_sli &&
              regs->reg0237_synt_sps.smpl_adpt_ofst_e &&
              (regs->reg0239_synt_sli0.sli_sao_luma_flg ||
               (regs->reg0239_synt_sli0.sli_sao_chrm_flg && regs->reg0198_src_fmt.out_fmt)))) {
            regs->reg0193_dual_core.dchs_txe = 0;
            regs->reg0193_dual_core.dchs_rxe = 0;
        } else if (index > 0) {
            regs->reg0193_dual_core.dchs_txid = index;
            regs->reg0193_dual_core.dchs_rxid = index - 1;
            regs->reg0193_dual_core.dchs_txe = 1;
            regs->reg0193_dual_core.dchs_rxe = 1;
        } else {
            regs->reg0193_dual_core.dchs_txid = index;
            regs->reg0193_dual_core.dchs_rxid = 0;
            regs->reg0193_dual_core.dchs_txe = 1;
            regs->reg0193_dual_core.dchs_rxe = 0;
        }
        /* dual core runtime offset should set to 2 to avoid data access conflict */
        regs->reg0193_dual_core.dchs_ofst = 2;

        if (index == syn->pp.num_tile_columns_minus1) {
            regs->reg0193_dual_core.dchs_txid = 0;
            regs->reg0193_dual_core.dchs_txe = 0;
        }
        regs->reg0252_tile_cfg.tile_w_m1 = tile_width - 1;
        regs->reg0252_tile_cfg.tile_h_m1 = mb_h - 1;
        regs->reg212_rc_cfg.rc_ctu_num   = tile_width;
        regs->reg0252_tile_cfg.tile_en = syn->pp.tiles_enabled_flag;
        regs->reg0253_tile_pos.tile_x = tile_start_x;
        regs->reg0253_tile_pos.tile_y = 0;

        hal_h265e_dbg_detail("tile_x %d, rc_ctu_num %d, tile_width_m1 %d",
                             regs->reg0253_tile_pos.tile_x, regs->reg212_rc_cfg.rc_ctu_num,
                             regs->reg0252_tile_cfg.tile_w_m1);
    }
}

MPP_RET hal_h265e_v580_start(void *hal, HalEncTask *enc_task)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    H265eSyntax_new *syn = ctx->syn;
    RK_U32 tile_num = (syn->pp.num_tile_columns_minus1 + 1) * (syn->pp.num_tile_rows_minus1 + 1);
    RK_U32 stream_len = 0;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    Vepu580H265eFrmCfg *frm = ctx->frm;
    RK_U32 k = 0;
    MPP_RET ret = MPP_OK;
    RK_S32 tile_start_x = 0;

    hal_h265e_enter();

    ctx->tile_num = tile_num;

    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return e arly",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (tile_num > MAX_TILE_NUM) {
        mpp_log("tile_num big then support %d, max %d", tile_num, MAX_TILE_NUM);
        return MPP_NOK;
    }

    for (k = 0; k < tile_num; k++) {
        H265eV580RegSet *hw_regs = frm->regs_set[k];
        hevc_vepu580_base *reg_base = NULL;
        H265eV580StatusElem *reg_out = frm->regs_ret[k];

        if (!hw_regs) {
            hw_regs = mpp_malloc(H265eV580RegSet, 1);
            frm->regs_set[k] = hw_regs;
        }
        if (!reg_out) {
            reg_out = mpp_malloc(H265eV580StatusElem, 1);
            frm->regs_ret[k] = reg_out;
        }

        reg_base = &hw_regs->reg_base;

        if (k)
            memcpy(hw_regs, frm->regs_set[0], sizeof(*hw_regs));

        vepu580_h265_set_me_ram(syn, reg_base, k, tile_start_x);

        /* set input info */
        vepu580_h265_set_patch_info(frm->reg_cfg, syn, (VepuFmt)fmt->format, enc_task);
        if (tile_num > 1)
            hal_h265e_v580_set_uniform_tile(reg_base, syn, k, tile_start_x);

        if (k) {
            RK_U32 offset = 0;

            reg_base->reg0176_lpfw_addr  = mpp_buffer_get_fd(frm->hw_tile_buf[k]);
            reg_base->reg0177_lpfr_addr  = mpp_buffer_get_fd(frm->hw_tile_buf[k - 1]);

            if (!ctx->tile_parall_en) {
                offset = mpp_packet_get_length(enc_task->packet);
                offset += stream_len;

                reg_base->reg0173_bsbb_addr = mpp_buffer_get_fd(enc_task->output);

                mpp_dev_multi_offset_update(frm->reg_cfg, 175, offset);
                mpp_dev_multi_offset_update(frm->reg_cfg, 172, mpp_buffer_get_size(enc_task->output));
            } else {
                reg_base->reg0172_bsbt_addr = mpp_buffer_get_fd(frm->hw_tile_stream[k - 1]);
                /* TODO: stream size relative with syntax */
                reg_base->reg0173_bsbb_addr = reg_base->reg0172_bsbt_addr;
                reg_base->reg0174_bsbr_addr = reg_base->reg0172_bsbt_addr;
                reg_base->reg0175_adr_bsbs  = reg_base->reg0172_bsbt_addr;

                mpp_dev_multi_offset_update(frm->reg_cfg, 175, 0);
                mpp_dev_multi_offset_update(frm->reg_cfg, 172, mpp_buffer_get_size(frm->hw_tile_stream[k - 1]));
            }

            offset = ctx->fbc_header_len;

            mpp_dev_multi_offset_update(frm->reg_cfg, 166, offset);
            mpp_dev_multi_offset_update(frm->reg_cfg, 164, offset);
        }

        if (enc_task->rc_task->frm.save_pass1)
            vepu580_h265e_save_pass1_patch(hw_regs, ctx, syn->pp.tiles_enabled_flag);

        if (enc_task->rc_task->frm.use_pass1)
            vepu580_h265e_use_pass1_patch(hw_regs, ctx);

        hal_h265e_v580_send_regs(ctx->dev, hw_regs, reg_out);

        mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_OFFS, frm->reg_cfg);

        if (k < tile_num - 1) {
            if (!ctx->tile_parall_en) {
                Vepu580H265Fbk *fb = &frm->feedback;

                ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
                if (ret) {
                    mpp_err_f("send cmd failed %d\n", ret);
                }

                ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
                if (ret) {
                    mpp_err_f("poll cmd failed %d\n", ret);
                    ret = MPP_ERR_VPUHW;
                }
                stream_len += reg_out->st.bs_lgth_l32;
                fb->qp_sum += reg_out->st.qp_sum;
                fb->out_strm_size += reg_out->st.bs_lgth_l32;
                fb->sse_sum += (RK_S64)(reg_out->st.sse_h32 << 16) +
                               (reg_out->st.st_sse_bsl.sse_l16 & 0xffff);
                fb->st_madi += reg_out->st.madi;
                fb->st_madp += reg_out->st.madp;
                fb->st_mb_num += reg_out->st.st_bnum_b16.num_b16;
                fb->st_ctu_num += reg_out->st.st_bnum_cme.num_ctu;
            } else
                mpp_dev_ioctl(ctx->dev, MPP_DEV_DELIMIT, NULL);
        }
        tile_start_x += (syn->pp.column_width_minus1[k] + 1);
    }

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }
    hal_h265e_leave();
    return ret;
}

static MPP_RET vepu580_h265_set_feedback(H265eV580HalContext *ctx, HalEncTask *enc_task, RK_U32 index)
{
    EncRcTaskInfo *hal_rc_ret = (EncRcTaskInfo *)&enc_task->rc_task->info;
    Vepu580H265eFrmCfg *frm = ctx->frms[enc_task->flags.reg_idx];
    Vepu580H265Fbk *fb = &frm->feedback;
    MppEncCfgSet *cfg = ctx->cfg;
    RK_S32 mb64_num = ((cfg->prep.width + 63) / 64) * ((cfg->prep.height + 63) / 64);
    RK_S32 mb8_num = (mb64_num << 6);
    RK_S32 mb4_num = (mb8_num << 2);

    hal_h265e_enter();
    H265eV580StatusElem *elem = frm->regs_ret[index];
    RK_U32 hw_status = elem->hw_status;

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

    fb->st_madi += elem->st.madi;
    fb->st_madp += elem->st.madp;
    fb->st_mb_num += elem->st.st_bnum_b16.num_b16;
    fb->st_ctu_num += elem->st.st_bnum_cme.num_ctu;

    fb->st_lvl64_inter_num += elem->st.st_pnum_p64.pnum_p64;
    fb->st_lvl32_inter_num += elem->st.st_pnum_p32.pnum_p32;
    fb->st_lvl32_intra_num += elem->st.st_pnum_i32.pnum_i32;
    fb->st_lvl16_inter_num += elem->st.st_pnum_p16.pnum_p16;
    fb->st_lvl16_intra_num += elem->st.st_pnum_i16.pnum_i16;
    fb->st_lvl8_inter_num  += elem->st.st_pnum_p8.pnum_p8;
    fb->st_lvl8_intra_num  += elem->st.st_pnum_i8.pnum_i8;
    fb->st_lvl4_intra_num  += elem->st.st_pnum_i4.pnum_i4;
    memcpy(&fb->st_cu_num_qp[0], &elem->st.st_b8_qp0, 52 * sizeof(RK_U32));

    if (index == (ctx->tile_num - 1)) {
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
            hal_rc_ret->iblk4_prop = ((((fb->st_lvl4_intra_num + fb->st_lvl8_intra_num) << 2) +
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

        hal_rc_ret->madi += fb->st_madi;
        hal_rc_ret->madp += fb->st_madp;
    }
    hal_h265e_leave();
    return MPP_OK;
}

void save_to_file(char *name, void *ptr, size_t size)
{
    FILE *fp = fopen(name, "w+b");
    if (fp) {
        fwrite(ptr, 1, size, fp);
        fclose(fp);
    } else
        mpp_err("create file %s failed\n", name);
}

void dump_files(H265eV580HalContext *ctx, HalEncTask *enc_task)
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

    snprintf(name, name_len, "/data/refr_fbd_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, ptr + ctx->fbc_header_len, buf_size - ctx->fbc_header_len);

    snprintf(name, name_len, "/data/refr_fbh_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, ptr, ctx->fbc_header_len);

    snprintf(name, name_len, "/data/refr_dsp_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, dws_ptr, dws_size);

    hal_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.recon_pic.slot_idx);
    buf_size = mpp_buffer_get_size(hal_buf->buf[0]);
    dws_size = mpp_buffer_get_size(hal_buf->buf[1]);
    ptr = mpp_buffer_get_ptr(hal_buf->buf[0]);
    dws_ptr = mpp_buffer_get_ptr(hal_buf->buf[1]);

    snprintf(name, name_len, "/data/recn_fbd_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, ptr + ctx->fbc_header_len, buf_size - ctx->fbc_header_len);

    snprintf(name, name_len, "/data/recn_fbh_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, ptr, ctx->fbc_header_len);

    snprintf(name, name_len, "/data/recn_dsp_%d_frm%d.bin", pid, frm_num);
    save_to_file(name, dws_ptr, dws_size);
}

static MPP_RET hal_h265e_vepu580_status_check(RK_U32 hw_status)
{
    MPP_RET ret = MPP_OK;

    if (hw_status & RKV_ENC_INT_LINKTABLE_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_LINKTABLE_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_FRAME_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_ONE_FRAME_FINISH");

    if (hw_status & RKV_ENC_INT_ONE_SLICE_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_ONE_SLICE_FINISH");

    if (hw_status & RKV_ENC_INT_SAFE_CLEAR_FINISH)
        hal_h265e_dbg_detail("RKV_ENC_INT_SAFE_CLEAR_FINISH");

    if (hw_status & RKV_ENC_INT_BIT_STREAM_OVERFLOW)
        hal_h265e_err("RKV_ENC_INT_BIT_STREAM_OVERFLOW");

    if (hw_status & RKV_ENC_INT_BUS_WRITE_FULL) {
        hal_h265e_err("RKV_ENC_INT_BUS_WRITE_FULL");
        ret = MPP_NOK;
    }

    if (hw_status & RKV_ENC_INT_BUS_WRITE_ERROR) {
        hal_h265e_err("RKV_ENC_INT_BUS_WRITE_ERROR");
        ret = MPP_NOK;
    }

    if (hw_status & RKV_ENC_INT_BUS_READ_ERROR) {
        hal_h265e_err("RKV_ENC_INT_BUS_READ_ERROR");
        ret = MPP_NOK;
    }

    if (hw_status & RKV_ENC_INT_TIMEOUT_ERROR) {
        hal_h265e_err("RKV_ENC_INT_TIMEOUT_ERROR");
        ret = MPP_NOK;
    }

    return ret;
}

MPP_RET hal_h265e_v580_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    HalEncTask *enc_task = task;
    RK_U32 split_out = ctx->cfg->split.split_out;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu580H265eFrmCfg *frm = ctx->frms[task_idx];

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
        RK_U32 finish_cnt = 0;
        RK_U32 tile1_offset = 0;
        MppPacket pkt = enc_task->packet;
        RK_U32 offset = mpp_packet_get_length(pkt);
        RK_U32 seg_offset = offset;
        void* ptr = mpp_packet_get_pos(pkt);
        H265eV580RegSet *regs = frm->regs_set[0];
        hevc_vepu580_base *reg_base = &regs->reg_base;
        RK_U32 type = reg_base->reg0236_synt_nal.nal_unit_type;
        MppDevPollCfg *poll_cfg = (MppDevPollCfg *)((char *)ctx->poll_cfgs);

        param.task = task;
        param.base = mpp_packet_get_data(task->packet);

        do {
            RK_S32 i = 0;
            poll_cfg->poll_type = 0;
            poll_cfg->poll_ret  = 0;
            poll_cfg->count_max = split_out & MPP_ENC_SPLIT_OUT_LOWDELAY ? 1 : ctx->poll_slice_max;
            poll_cfg->count_ret = 0;

            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, poll_cfg);

            for (i = 0; i < poll_cfg->count_ret; i++) {
                slice_last = poll_cfg->slice_info[i].last;
                slice_len = poll_cfg->slice_info[i].length;
                param.length = slice_len;

                if (finish_cnt > 0) {
                    MppBuffer buf = frm->hw_tile_stream[finish_cnt - 1];
                    void *tile1_ptr  = mpp_buffer_get_ptr(buf);

                    mpp_buffer_sync_ro_partial_begin(buf, tile1_offset, slice_len);
                    memcpy(ptr + seg_offset, tile1_ptr + tile1_offset, slice_len);
                    tile1_offset += slice_len;
                } else {
                    MppBuffer buf = enc_task->output;

                    mpp_buffer_sync_ro_partial_begin(buf, offset, slice_len);
                }

                ctx->output_cb->cmd = ENC_OUTPUT_SLICE;
                if (slice_last) {
                    finish_cnt++;
                    tile1_offset = 0;
                    if (ctx->tile_parall_en) {
                        if (finish_cnt + 1 > ctx->tile_num) {
                            ctx->output_cb->cmd = ENC_OUTPUT_FINISH;
                        }
                    }
                }

                mpp_packet_add_segment_info(pkt, type, seg_offset, slice_len);

                if (split_out & MPP_ENC_SPLIT_OUT_LOWDELAY)
                    mpp_callback(ctx->output_cb, &param);

                seg_offset += slice_len;
            }

            if (ctx->tile_parall_en) {
                if (finish_cnt + 1 > ctx->tile_num) {
                    break;
                }
            } else if (slice_last) {
                break;
            }
        } while (1);
    } else {
        H265eV580StatusElem *elem = frm->regs_ret[0];
        H265eV580RegSet *regs = frm->regs_set[0];
        hevc_vepu580_base *reg_base = &regs->reg_base;
        RK_U32 type = reg_base->reg0236_synt_nal.nal_unit_type;
        MppPacket pkt = enc_task->packet;
        RK_U32 offset = mpp_packet_get_length(pkt);
        RK_U32 i = 0;

        if (ctx->tile_parall_en) {
            for (i = 0; i < ctx->tile_num; i ++)
                ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        } else {
            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
        }

        for (i = 0; i < ctx->tile_num; i++) {
            H265eV580StatusElem *elem_ret = frm->regs_ret[i];
            RK_U32 hw_status = elem_ret->hw_status;
            RK_U32 tile_size = elem_ret->st.bs_lgth_l32;

            ret = hal_h265e_vepu580_status_check(hw_status);
            if (ret)
                break;
            mpp_packet_add_segment_info(pkt, type, offset, tile_size);
            offset += tile_size;

            if (ctx->tile_dump_err &&
                (hw_status & (RKV_ENC_INT_BUS_WRITE_ERROR | RKV_ENC_INT_BUS_READ_ERROR))) {
                dump_files(ctx, enc_task);
                break;
            }
        }

        if (ret)
            mpp_err_f("poll cmd failed %d status %d \n", ret, elem->hw_status);
    }

    hal_h265e_leave();

    return ret;
}

MPP_RET hal_h265e_v580_get_task(void *hal, HalEncTask *task)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    Vepu580H265eFrmCfg *frm_cfg = NULL;
    EncFrmStatus  *frm_status = &task->rc_task->frm;
    MppFrame frame = task->frame;
    RK_S32 task_idx = ctx->task_idx;

    hal_h265e_enter();

    task->part_first = 1;
    task->part_last = 0;
    task->flags.reg_idx = task_idx;

    if (!frm_status->reencode) {
        ctx->syn = (H265eSyntax_new *)task->syntax.data;
        ctx->dpb = (H265eDpb*)ctx->syn->dpb;

        if (vepu580_h265_setup_hal_bufs(ctx)) {
            hal_h265e_err("vepu541_h265_allocate_buffers failed, free buffers and return\n");
            task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
            return MPP_ERR_MALLOC;
        }

        ctx->last_frame_type = ctx->frame_type;

        frm_cfg = ctx->frms[task_idx];
        ctx->frm = frm_cfg;

        if (mpp_frame_has_meta(task->frame)) {
            MppMeta meta = mpp_frame_get_meta(frame);

            mpp_meta_get_ptr_d(meta, KEY_ROI_DATA2, (void **)&frm_cfg->roi_data, NULL);
            mpp_meta_get_ptr_d(meta, KEY_OSD_DATA, (void **)&frm_cfg->osd_cfg.osd_data, NULL);
            mpp_meta_get_ptr_d(meta, KEY_OSD_DATA2, (void **)&frm_cfg->osd_cfg.osd_data2, NULL);
        } else {
            frm_cfg->roi_data = NULL;
            frm_cfg->osd_cfg.osd_data = NULL;
            frm_cfg->osd_cfg.osd_data2 = NULL;
        }

        frm_cfg->frame_count = ctx->frame_count++;

        ctx->task_idx++;
        if (ctx->task_idx >= ctx->task_cnt)
            ctx->task_idx = 0;

        frm_cfg->hal_curr_idx = ctx->syn->sp.recon_pic.slot_idx;
        frm_cfg->hal_refr_idx = ctx->syn->sp.ref_pic.slot_idx;
    } else {
        /* reencode path may change the frame type */
        frm_cfg = ctx->frm;
    }

    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_curr_idx);
    h265e_dpb_hal_start(ctx->dpb, frm_cfg->hal_refr_idx);

    ctx->frame_type = (frm_status->is_intra) ? INTRA_FRAME : INTER_P_FRAME;
    frm_cfg->frame_type = ctx->frame_type;
    mpp_dev_multi_offset_reset(frm_cfg->reg_cfg);
    memset(&frm_cfg->feedback, 0, sizeof(frm_cfg->feedback));

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_v580_ret_task(void *hal, HalEncTask *task)
{
    H265eV580HalContext *ctx = (H265eV580HalContext *)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    HalEncTask *enc_task = task;
    RK_S32 task_idx = task->flags.reg_idx;
    Vepu580H265eFrmCfg *frm = ctx->frms[task_idx];
    Vepu580H265Fbk *fb = &frm->feedback;
    H265eSyntax_new *syn = (H265eSyntax_new *) enc_task->syntax.data;
    RK_U32 offset = mpp_packet_get_length(enc_task->packet);

    hal_h265e_enter();

    if (ctx->tile_parall_en) {
        RK_U32 i = 0, stream_len = 0;
        void* ptr = mpp_packet_get_pos(enc_task->packet);

        for (i = 0; i < ctx->tile_num; i ++) {
            vepu580_h265_set_feedback(ctx, enc_task, i);
            if (!ctx->cfg->split.split_out) {
                if (i) {  //copy tile 1 stream
                    RK_U32 len = fb->out_strm_size - stream_len;
                    MppBuffer buf = frm->hw_tile_stream[i - 1];
                    RK_U8 *tile1_ptr  = mpp_buffer_get_ptr(buf);

                    mpp_buffer_sync_partial_begin(buf, 0, len);

                    if (syn->sp.temporal_id && len > 5)
                        tile1_ptr[5] = (tile1_ptr[5] & 0xf8) | ((syn->sp.temporal_id + 1) & 0x7);

                    memcpy(ptr + stream_len + offset, tile1_ptr, len);
                } else {
                    MppBuffer buf = enc_task->output;
                    RK_U32 len = fb->out_strm_size;
                    RK_U8 *stream_ptr = (RK_U8 *) ptr;

                    mpp_buffer_sync_partial_begin(buf, offset, len);

                    if (syn->sp.temporal_id) {
                        stream_ptr[5] = (stream_ptr[5] & 0xf8) | ((syn->sp.temporal_id + 1) & 0x7);
                    }
                }
                stream_len = fb->out_strm_size;
            }
        }
    } else {
        vepu580_h265_set_feedback(ctx, enc_task, ctx->tile_num - 1);
        mpp_buffer_sync_partial_begin(enc_task->output, offset, fb->out_strm_size);
        hal_h265e_amend_temporal_id(task, fb->out_strm_size);
    }

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

    vepu580_h265e_tune_stat_update(ctx->tune, rc_info);

    h265e_dpb_hal_end(ctx->dpb, frm->hal_curr_idx);
    h265e_dpb_hal_end(ctx->dpb, frm->hal_refr_idx);

    hal_h265e_dbg_detail("output stream size %d\n", fb->out_strm_size);

    hal_h265e_leave();
    return MPP_OK;
}

const MppEncHalApi hal_h265e_vepu580 = {
    "hal_h265e_v580",
    MPP_VIDEO_CodingHEVC,
    sizeof(H265eV580HalContext),
    0,
    hal_h265e_v580_init,
    hal_h265e_v580_deinit,
    hal_h265e_vepu580_prepare,
    hal_h265e_v580_get_task,
    hal_h265e_v580_gen_regs,
    hal_h265e_v580_start,
    hal_h265e_v580_wait,
    NULL,
    NULL,
    hal_h265e_v580_ret_task,
};
