/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#define MODULE_TAG  "hal_h265e_v541"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_soc.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_frame_impl.h"

#include "h265e_syntax_new.h"
#include "hal_h265e_debug.h"
#include "hal_h265e_stream_amend.h"
#include "hal_h265e_vepu541_reg.h"
#include "hal_h265e_vepu54x_reg_l2.h"
#include "vepu541_common.h"
#include "vepu5xx_common.h"
#include "rkv_enc_def.h"
#include "mpp_enc_hal.h"
#include "hal_bufs.h"
#include "mpp_enc_ref.h"

#define hal_h265e_err(fmt, ...) \
    do {\
        mpp_err_f(fmt, ## __VA_ARGS__);\
    } while (0)
#define VEPU541_L2_SIZE  768

#define TILE_BUF_SIZE  MPP_ALIGN(128 * 1024, 256)

typedef struct vepu541_h265_fbk_t {
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
} vepu541_h265_fbk;

typedef struct H265eV541HalContext_t {
    MppEncHalApi        api;
    MppDev              dev;
    void                *regs;
    void                *l2_regs;
    void                *reg_out;

    vepu541_h265_fbk    feedback;
    void                *dump_files;
    RK_U32              frame_cnt_gen_ready;

    RK_S32              frame_type;
    RK_S32              last_frame_type;

    /* @frame_cnt starts from ZERO */
    RK_U32              frame_cnt;
    Vepu5xxOsdCfg       osd_cfg;
    MppEncROICfg        *roi_data;
    MppEncROICfg2       *roi_data2;
    Vepu541RoiCfg       *roi_buf_tmp;
    MppBufferGroup      roi_grp;
    MppBuffer           roi_buf;
    RK_U32              roi_buf_size;
    MppBuffer           qpmap;

    MppEncCfgSet        *cfg;

    MppBufferGroup      tile_grp;
    MppBuffer           hw_tile_buf[2];

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
    RK_U32              is_vepu540;
    RK_S32              fbc_header_len;
} H265eV541HalContext;

static RK_U32 aq_thd_default[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 35
};

static RK_S32 aq_qp_dealt_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    4,  5,  7,  8,
};

static RK_U16 lvl32_intra_cst_thd[4] = {2, 6, 16, 36};

static RK_U16 lvl16_intra_cst_thd[4] = {2, 6, 16, 36};

static RK_U8 lvl32_intra_cst_wgt[8] = {23, 22, 21, 20, 22, 24, 26};

static RK_U8 lvl16_intra_cst_wgt[8] = {17, 17, 17, 18, 17, 18, 18};

static RK_U16 atr_thd[4] = {0, 0, 0, 0};

static RK_U8 lvl16_4_atr_wgt[12] = {0};

//RK_U16 atf_thd[14] =  {0, 4, 0, 4, 0, 4, 0, 4, 0, 4, 16, 16, 16, 16}; /*thd 4, sad 4, wgt 6*/
static RK_U16 atf_thd[14] =  {4, 36, 4, 36, 4, 36, 4, 36, 0, 4, 22, 22, 22, 22}; /*thd 4, sad 4, wgt 6*/

static RK_U16 atf_sad_ofst[4] = {0, 0, 0, 0};

static MPP_RET vepu54x_h265_setup_hal_bufs(H265eV541HalContext *ctx)
{
    MPP_RET ret = MPP_OK;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    RK_U32 frame_size;
    VepuFmt input_fmt = VEPU5xx_FMT_YUV420P;
    RK_S32 mb_wd64, mb_h64;
    MppEncRefCfg ref_cfg = ctx->cfg->ref_cfg;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_S32 old_max_cnt = ctx->max_buf_cnt;
    RK_S32 new_max_cnt = 2;

    hal_h265e_enter();

    mb_wd64 = (prep->width + 63) / 64;
    mb_h64 = (prep->height + 63) / 64 + 1;

    frame_size = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16);
    vepu5xx_set_fmt(fmt, ctx->cfg->prep.format);
    input_fmt = (VepuFmt)fmt->format;
    switch (input_fmt) {
    case VEPU5xx_FMT_YUV400:
        break;
    case VEPU5xx_FMT_YUV420P:
    case VEPU5xx_FMT_YUV420SP: {
        frame_size = frame_size * 3 / 2;
    } break;
    case VEPU5xx_FMT_YUV422P:
    case VEPU5xx_FMT_YUV422SP:
    case VEPU5xx_FMT_YUYV422:
    case VEPU5xx_FMT_UYVY422:
    case VEPU5xx_FMT_BGR565: {
        frame_size *= 2;
    } break;
    case VEPU5xx_FMT_BGR888: {
        frame_size *= 3;
    } break;
    case VEPU5xx_FMT_BGRA8888: {
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

    if (frame_size > ctx->frame_size || new_max_cnt > old_max_cnt) {
        size_t size[3] = {0};

        hal_bufs_deinit(ctx->dpb_bufs);
        hal_bufs_init(&ctx->dpb_bufs);

        ctx->fbc_header_len = MPP_ALIGN(((mb_wd64 * mb_h64) << 6), SZ_8K);
        size[0] = ctx->fbc_header_len + ((mb_wd64 * mb_h64) << 12) * 3 / 2; //fbc_h + fbc_b
        size[1] = (mb_wd64 * mb_h64 << 8);
        size[2] = MPP_ALIGN(mb_wd64 * mb_h64 * 16 * 4, 256);
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

static void vepu540_h265_set_l2_regs(H265eV54xL2RegSet *reg)
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
    ;
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
    ;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m0 = 34;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m1 = 33;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m2 = 32;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m3 = 31;
    reg->pre_intra_cla6_B0.pre_intra_cla6_m4 = 30;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m5 = 2 ;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m6 = 29;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m7 = 3 ;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m8 = 28;
    reg->pre_intra_cla6_B1.pre_intra_cla6_m9 = 4 ;
    ;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m0 = 34;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m1 = 2 ;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m2 = 3 ;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m3 = 4 ;
    reg->pre_intra_cla7_B0.pre_intra_cla7_m4 = 5 ;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m5 = 33;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m6 = 6 ;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m7 = 32;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m8 = 7 ;
    reg->pre_intra_cla7_B1.pre_intra_cla7_m9 = 31;
    ;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m0 = 10;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m1 = 26;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m2 = 18;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m3 = 34;
    reg->pre_intra_cla8_B0.pre_intra_cla8_m4 = 6 ;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m5 = 14;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m6 = 22;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m7 = 30;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m8 = 2 ;
    reg->pre_intra_cla8_B1.pre_intra_cla8_m9 = 24;
    ;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m0 = 0 ;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m1 = 0 ;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m2 = 0 ;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m3 = 0 ;
    reg->pre_intra_cla9_B0.pre_intra_cla9_m4 = 0 ;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m5 = 0 ;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m6 = 0 ;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m7 = 0 ;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m8 = 0 ;
    reg->pre_intra_cla9_B1.pre_intra_cla9_m9 = 0 ;

    reg->pre_intra_cla10_B0.pre_intra_cla10_m0 =  0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m1 =  0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m2 =  0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m3 =  0;
    reg->pre_intra_cla10_B0.pre_intra_cla10_m4 =  0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m5 =  0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m6 =  0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m7 =  0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m8 =  0;
    reg->pre_intra_cla10_B1.pre_intra_cla10_m9 =  0;

    reg->pre_intra_cla11_B0.pre_intra_cla11_m0 =  0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m1 =  0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m2 =  0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m3 =  0;
    reg->pre_intra_cla11_B0.pre_intra_cla11_m4 =  0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m5 =  0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m6 =  0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m7 =  0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m8 =  0;
    reg->pre_intra_cla11_B1.pre_intra_cla11_m9 =  0;

    reg->pre_intra_cla12_B0.pre_intra_cla12_m0 =  0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m1 =  0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m2 =  0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m3 =  0;
    reg->pre_intra_cla12_B0.pre_intra_cla12_m4 =  0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m5 =  0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m6 =  0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m7 =  0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m8 =  0;
    reg->pre_intra_cla12_B1.pre_intra_cla12_m9 =  0;

    reg->pre_intra_cla13_B0.pre_intra_cla13_m0 =  0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m1 =  0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m2 =  0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m3 =  0;
    reg->pre_intra_cla13_B0.pre_intra_cla13_m4 =  0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m5 =  0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m6 =  0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m7 =  0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m8 =  0;
    reg->pre_intra_cla13_B1.pre_intra_cla13_m9 =  0;

    reg->pre_intra_cla14_B0.pre_intra_cla14_m0 =  0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m1 =  0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m2 =  0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m3 =  0;
    reg->pre_intra_cla14_B0.pre_intra_cla14_m4 =  0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m5 =  0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m6 =  0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m7 =  0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m8 =  0;
    reg->pre_intra_cla14_B1.pre_intra_cla14_m9 =  0;

    reg->pre_intra_cla15_B0.pre_intra_cla15_m0 =  0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m1 =  0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m2 =  0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m3 =  0;
    reg->pre_intra_cla15_B0.pre_intra_cla15_m4 =  0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m5 =  0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m6 =  0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m7 =  0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m8 =  0;
    reg->pre_intra_cla15_B1.pre_intra_cla15_m9 =  0;

    reg->pre_intra_cla16_B0.pre_intra_cla16_m0 =  0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m1 =  0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m2 =  0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m3 =  0;
    reg->pre_intra_cla16_B0.pre_intra_cla16_m4 =  0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m5 =  0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m6 =  0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m7 =  0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m8 =  0;
    reg->pre_intra_cla16_B1.pre_intra_cla16_m9 =  0;

    reg->i16_sobel_t_hevc.intra_l16_sobel_t0 = 64 ;
    reg->i16_sobel_t_hevc.intra_l16_sobel_t1 = 200;
    reg->i16_sobel_a_00_hevc.intra_l16_sobel_a0_qp0 = 32 ;
    reg->i16_sobel_a_00_hevc.intra_l16_sobel_a0_qp1 = 32 ;
    reg->i16_sobel_a_00_hevc.intra_l16_sobel_a0_qp2 = 32 ;
    reg->i16_sobel_a_00_hevc.intra_l16_sobel_a0_qp3 = 32 ;
    reg->i16_sobel_a_00_hevc.intra_l16_sobel_a0_qp4 = 32 ;
    reg->i16_sobel_a_01_hevc.intra_l16_sobel_a0_qp5 = 32 ;
    reg->i16_sobel_a_01_hevc.intra_l16_sobel_a0_qp6 = 32 ;
    reg->i16_sobel_a_01_hevc.intra_l16_sobel_a0_qp7 = 32 ;
    reg->i16_sobel_a_01_hevc.intra_l16_sobel_a0_qp8 = 32 ;
    reg->i16_sobel_b_00_hevc.intra_l16_sobel_b0_qp0 =  0 ;
    reg->i16_sobel_b_00_hevc.intra_l16_sobel_b0_qp1 =  0 ;
    reg->i16_sobel_b_01_hevc.intra_l16_sobel_b0_qp2 =  0 ;
    reg->i16_sobel_b_01_hevc.intra_l16_sobel_b0_qp3 =  0 ;
    reg->i16_sobel_b_02_hevc.intra_l16_sobel_b0_qp4 =  0 ;
    reg->i16_sobel_b_02_hevc.intra_l16_sobel_b0_qp5 =  0 ;
    reg->i16_sobel_b_03_hevc.intra_l16_sobel_b0_qp6 =  0 ;
    reg->i16_sobel_b_03_hevc.intra_l16_sobel_b0_qp7 =  0 ;
    reg->i16_sobel_b_04_hevc.intra_l16_sobel_b0_qp8 =  0 ;
    reg->i16_sobel_c_00_hevc.intra_l16_sobel_c0_qp0 = 13;
    reg->i16_sobel_c_00_hevc.intra_l16_sobel_c0_qp1 = 13;
    reg->i16_sobel_c_00_hevc.intra_l16_sobel_c0_qp2 = 13;
    reg->i16_sobel_c_00_hevc.intra_l16_sobel_c0_qp3 = 13;
    reg->i16_sobel_c_00_hevc.intra_l16_sobel_c0_qp4 = 13;
    reg->i16_sobel_c_01_hevc.intra_l16_sobel_c0_qp5 = 13;
    reg->i16_sobel_c_01_hevc.intra_l16_sobel_c0_qp6 = 13;
    reg->i16_sobel_c_01_hevc.intra_l16_sobel_c0_qp7 = 13;
    reg->i16_sobel_c_01_hevc.intra_l16_sobel_c0_qp8 = 13;
    reg->i16_sobel_d_00_hevc.intra_l16_sobel_d0_qp0 =  23750;
    reg->i16_sobel_d_00_hevc.intra_l16_sobel_d0_qp1 =  23750;
    reg->i16_sobel_d_01_hevc.intra_l16_sobel_d0_qp2 =  23750;
    reg->i16_sobel_d_01_hevc.intra_l16_sobel_d0_qp3 =  23750;
    reg->i16_sobel_d_02_hevc.intra_l16_sobel_d0_qp4 =  23750;
    reg->i16_sobel_d_02_hevc.intra_l16_sobel_d0_qp5 =  23750;
    reg->i16_sobel_d_03_hevc.intra_l16_sobel_d0_qp6 =  23750;
    reg->i16_sobel_d_03_hevc.intra_l16_sobel_d0_qp7 =  23750;
    reg->i16_sobel_d_04_hevc.intra_l16_sobel_d0_qp8 =  23750;

    reg->i16_sobel_e_00_17_hevc[0]  = 20000;
    reg->i16_sobel_e_00_17_hevc[2]  = 20000;
    reg->i16_sobel_e_00_17_hevc[4]  = 20000;
    reg->i16_sobel_e_00_17_hevc[6]  = 20000;
    reg->i16_sobel_e_00_17_hevc[8]  = 20000;
    reg->i16_sobel_e_00_17_hevc[10] = 20000;
    reg->i16_sobel_e_00_17_hevc[12] = 20000;
    reg->i16_sobel_e_00_17_hevc[14] = 20000;
    reg->i16_sobel_e_00_17_hevc[16] = 20000;
    reg->i16_sobel_e_00_17_hevc[1]  =  0;
    reg->i16_sobel_e_00_17_hevc[3]  =  0;
    reg->i16_sobel_e_00_17_hevc[5]  =  0;
    reg->i16_sobel_e_00_17_hevc[7]  =  0;
    reg->i16_sobel_e_00_17_hevc[9]  =  0;
    reg->i16_sobel_e_00_17_hevc[11] =  0;
    reg->i16_sobel_e_00_17_hevc[13] =  0;
    reg->i16_sobel_e_00_17_hevc[15] =  0;
    reg->i16_sobel_e_00_17_hevc[17] =  0;

    reg->i32_sobel_t_00_hevc.intra_l32_sobel_t2 = 640 ;
    reg->i32_sobel_t_00_hevc.intra_l32_sobel_t3 = 0  ;
    reg->i32_sobel_t_01_hevc.intra_l32_sobel_t4 = 8  ;
    reg->i32_sobel_t_02_hevc.intra_l32_sobel_t5 = 100 ;
    reg->i32_sobel_t_02_hevc.intra_l32_sobel_t6 = 100 ;

    reg->i32_sobel_a_hevc.intra_l32_sobel_a1_qp0 =  18;
    reg->i32_sobel_a_hevc.intra_l32_sobel_a1_qp1 =  18;
    reg->i32_sobel_a_hevc.intra_l32_sobel_a1_qp2 =  18;
    reg->i32_sobel_a_hevc.intra_l32_sobel_a1_qp3 =  18;
    reg->i32_sobel_a_hevc.intra_l32_sobel_a1_qp4 =  18;

    reg->i32_sobel_b_00_hevc.intra_l32_sobel_b1_qp0 =  0;
    reg->i32_sobel_b_00_hevc.intra_l32_sobel_b1_qp1 =  0;
    reg->i32_sobel_b_01_hevc.intra_l32_sobel_b1_qp2 =  0;
    reg->i32_sobel_b_01_hevc.intra_l32_sobel_b1_qp3 =  0;
    reg->i32_sobel_b_02_hevc.intra_l32_sobel_b1_qp4 =  0;

    reg->i32_sobel_c_hevc.intra_l32_sobel_c1_qp0 = 18;
    reg->i32_sobel_c_hevc.intra_l32_sobel_c1_qp1 = 18;
    reg->i32_sobel_c_hevc.intra_l32_sobel_c1_qp2 = 18;
    reg->i32_sobel_c_hevc.intra_l32_sobel_c1_qp3 = 18;
    reg->i32_sobel_c_hevc.intra_l32_sobel_c1_qp4 = 18;

    reg->i32_sobel_d_00_hevc.intra_l32_sobel_d1_qp0 =  0;
    reg->i32_sobel_d_00_hevc.intra_l32_sobel_d1_qp1 =  0;
    reg->i32_sobel_d_01_hevc.intra_l32_sobel_d1_qp2 =  0;
    reg->i32_sobel_d_01_hevc.intra_l32_sobel_d1_qp3 =  0;
    reg->i32_sobel_d_02_hevc.intra_l32_sobel_d1_qp4 =  0;

    reg->i32_sobel_e_00_09_hevc[0] =  20000;
    reg->i32_sobel_e_00_09_hevc[2] =  20000;
    reg->i32_sobel_e_00_09_hevc[4] =  20000;
    reg->i32_sobel_e_00_09_hevc[6] =  20000;
    reg->i32_sobel_e_00_09_hevc[8] =  20000;

    reg->i32_sobel_e_00_09_hevc[1] =  0;
    reg->i32_sobel_e_00_09_hevc[3] =  0;
    reg->i32_sobel_e_00_09_hevc[5] =  0;
    reg->i32_sobel_e_00_09_hevc[7] =  0;
    reg->i32_sobel_e_00_09_hevc[9] =  0;
}
static void vepu541_h265_set_l2_regs(H265eV541HalContext *ctx, H265eV54xL2RegSet *regs)
{
    MppEncHwCfg *hw = &ctx->cfg->hw;
    RK_U32 i;

    memcpy(&regs->lvl32_intra_CST_THD0, lvl32_intra_cst_thd, sizeof(lvl32_intra_cst_thd));
    memcpy(&regs->lvl16_intra_CST_THD0, lvl16_intra_cst_thd, sizeof(lvl16_intra_cst_thd));
    memcpy(&regs->lvl32_intra_CST_WGT0, lvl32_intra_cst_wgt, sizeof(lvl32_intra_cst_wgt));
    memcpy(&regs->lvl16_intra_CST_WGT0, lvl16_intra_cst_wgt, sizeof(lvl16_intra_cst_wgt));
    regs->rdo_quant.quant_f_bias_I = 171;
    regs->rdo_quant.quant_f_bias_P = 85;
    memcpy(&regs->atr_thd0, atr_thd, sizeof(atr_thd));
    memcpy(&regs->lvl16_atr_wgt, lvl16_4_atr_wgt, sizeof(lvl16_4_atr_wgt));
    if (!ctx->is_vepu540) {
        memcpy(&regs->thd_541.atf_thd0, atf_thd, sizeof(atf_thd));
        regs->thd_541.atf_thd0.atf_thd0_i32 = 0;
        regs->thd_541.atf_thd0.atf_thd1_i32 = 63;
        regs->thd_541.atf_thd1.atf_thd0_i16 = 0;
        regs->thd_541.atf_thd1.atf_thd1_i16 = 63;
        regs->thd_541.atf_sad_thd0.atf_thd0_p64 = 0;
        regs->thd_541.atf_sad_thd0.atf_thd1_p64 = 63;
        regs->thd_541.atf_sad_thd1.atf_thd0_p32 = 0;
        regs->thd_541.atf_sad_thd1.atf_thd1_p32 = 63;
        regs->thd_541.atf_sad_wgt0.atf_thd0_p16 = 0;
        regs->thd_541.atf_sad_wgt0.atf_thd1_p16 = 63;
    } else {
        memcpy(&regs->thd_540.atf_thd0, atf_thd, sizeof(atf_thd));
        regs->thd_540.atf_thd0.atf_thd0_i32 = 0;
        regs->thd_540.atf_thd0.atf_thd1_i32 = 63;
        regs->thd_540.atf_thd1.atf_thd0_i16 = 0;
        regs->thd_540.atf_thd1.atf_thd1_i16 = 63;
        regs->thd_540.atf_sad_thd0.atf_thd0_p64 = 0;
        regs->thd_540.atf_sad_thd0.atf_thd1_p64 = 63;
        regs->thd_540.atf_sad_thd1.atf_thd0_p32 = 0;
        regs->thd_540.atf_sad_thd1.atf_thd1_p32 = 63;
        regs->thd_540.atf_sad_wgt0.atf_thd0_p16 = 0;
        regs->thd_540.atf_sad_wgt0.atf_thd1_p16 = 63;
        vepu540_h265_set_l2_regs(regs);
    }

    if (ctx->cfg->tune.scene_mode == MPP_ENC_SCENE_MODE_IPC) {
        regs->atf_sad_wgt1.atf_wgt_i16 = 19;
        regs->atf_sad_wgt1.atf_wgt_i32 = 19;
        regs->atf_sad_wgt2.atf_wgt_p32 = 13;
        regs->atf_sad_wgt2.atf_wgt_p64 = 13;
        regs->atf_sad_ofst0.atf_wgt_p16 = 13;
    } else {
        regs->atf_sad_wgt1.atf_wgt_i16 = 16;
        regs->atf_sad_wgt1.atf_wgt_i32 = 16;
        regs->atf_sad_wgt2.atf_wgt_p32 = 16;
        regs->atf_sad_wgt2.atf_wgt_p64 = 16;
        regs->atf_sad_ofst0.atf_wgt_p16 = 16;
    }

    memcpy(&regs->atf_sad_ofst1, atf_sad_ofst, sizeof(atf_sad_ofst));
    memcpy(&regs->lamd_satd_qp[0], lamd_satd_qp, sizeof(lamd_satd_qp));
    memcpy(&regs->lamd_moda_qp[0], lamd_moda_qp, sizeof(lamd_moda_qp));
    memcpy(&regs->lamd_modb_qp[0], lamd_modb_qp, sizeof(lamd_modb_qp));

    if (ctx->frame_type == INTRA_FRAME) {
        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            regs->aq_tthd[i] = hw->aq_thrd_i[i];
            regs->aq_step[i] = hw->aq_step_i[i] & 0x3f;
        }
    } else {
        for (i = 0; i < MPP_ARRAY_ELEMS(aq_thd_default); i++) {
            regs->aq_tthd[i] = hw->aq_thrd_p[i];
            regs->aq_step[i] = hw->aq_step_p[i] & 0x3f;
        }
    }

    if (hw->qbias_en) {
        regs->rdo_quant.quant_f_bias_I = hw->qbias_i;
        regs->rdo_quant.quant_f_bias_P = hw->qbias_p;
    }

    MppDevRegWrCfg cfg;
    cfg.reg = regs;
    if (ctx->is_vepu540) {
        cfg.size = sizeof(H265eV54xL2RegSet);
    } else {
        cfg.size = VEPU541_L2_SIZE ;
    }
    cfg.offset = VEPU541_REG_BASE_L2;
    mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
}

MPP_RET hal_h265e_v541_init(void *hal, MppEncHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;

    mpp_env_get_u32("hal_h265e_debug", &hal_h265e_debug, 0);
    hal_h265e_enter();
    ctx->reg_out        = mpp_calloc(H265eV541IoctlOutputElem, 1);
    ctx->regs           = mpp_calloc(H265eV541RegSet, 1);
    ctx->l2_regs        = mpp_calloc(H265eV54xL2RegSet, 1);
    ctx->input_fmt      = mpp_calloc(VepuFmtCfg, 1);
    ctx->cfg            = cfg->cfg;
    hal_bufs_init(&ctx->dpb_bufs);

    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->enc_mode = RKV_ENC_MODE;
    cfg->type = VPU_CLIENT_RKVENC;
    ret = mpp_dev_init(&cfg->dev, cfg->type);
    if (ret) {
        mpp_err_f("mpp_dev_init failed. ret: %d\n", ret);
        return ret;
    }

    ctx->dev = cfg->dev;
    {
        RockchipSocType soc_type = mpp_get_soc_type();

        if (soc_type == ROCKCHIP_SOC_RK3566 || soc_type == ROCKCHIP_SOC_RK3568)
            ctx->is_vepu540 = 1;
    }

    ctx->osd_cfg.reg_base = ctx->regs;
    ctx->osd_cfg.dev = ctx->dev;
    ctx->osd_cfg.reg_cfg = NULL;
    ctx->osd_cfg.plt_cfg = &ctx->cfg->plt_cfg;
    ctx->osd_cfg.osd_data = NULL;
    ctx->osd_cfg.osd_data2 = NULL;

    ctx->frame_type = INTRA_FRAME;

    {   /* setup default hardware config */
        MppEncHwCfg *hw = &cfg->cfg->hw;

        hw->qp_delta_row_i  = 0;
        hw->qp_delta_row    = 1;
        hw->qbias_i         = 171;
        hw->qbias_p         = 85;
        hw->qbias_en        = 0;

        memcpy(hw->aq_thrd_i, aq_thd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, aq_thd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, aq_qp_dealt_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, aq_qp_dealt_default, sizeof(hw->aq_step_p));
    }

    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v541_deinit(void *hal)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    hal_h265e_enter();
    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->l2_regs);
    MPP_FREE(ctx->reg_out);
    MPP_FREE(ctx->input_fmt);
    MPP_FREE(ctx->roi_buf_tmp);

    if (ctx->roi_buf) {
        mpp_buffer_put(ctx->roi_buf);
        ctx->roi_buf = NULL;
    }
    hal_bufs_deinit(ctx->dpb_bufs);

    if (ctx->roi_grp) {
        mpp_buffer_group_put(ctx->roi_grp);
        ctx->roi_grp = NULL;
    }

    if (ctx->hw_tile_buf[0]) {
        mpp_buffer_put(ctx->hw_tile_buf[0]);
        ctx->hw_tile_buf[0] = NULL;
    }

    if (ctx->hw_tile_buf[1]) {
        mpp_buffer_put(ctx->hw_tile_buf[1]);
        ctx->hw_tile_buf[1] = NULL;
    }

    if (ctx->tile_grp) {
        mpp_buffer_group_put(ctx->tile_grp);
        ctx->tile_grp = NULL;
    }

    if (ctx->dev) {
        mpp_dev_deinit(ctx->dev);
        ctx->dev = NULL;
    }
    hal_h265e_leave();
    return MPP_OK;
}

static MPP_RET hal_h265e_vepu54x_prepare(void *hal)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h265e_dbg_func("enter %p\n", hal);

    if (prep->change & (MPP_ENC_PREP_CFG_CHANGE_INPUT | MPP_ENC_PREP_CFG_CHANGE_FORMAT)) {
        RK_S32 i;

        // pre-alloc required buffers to reduce first frame delay
        vepu54x_h265_setup_hal_bufs(ctx);
        for (i = 0; i < ctx->max_buf_cnt; i++)
            hal_bufs_get_buf(ctx->dpb_bufs, i);

        prep->change = 0;
    }

    hal_h265e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET
vepu541_h265_set_patch_info(MppDev dev, H265eSyntax_new *syn, VepuFmt input_fmt, HalEncTask *task)
{
    RK_U32 hor_stride = syn->pp.hor_stride;
    RK_U32 ver_stride = syn->pp.ver_stride ? syn->pp.ver_stride : syn->pp.pic_height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
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
        case VEPU5xx_FMT_YUYV422:
        case VEPU5xx_FMT_UYVY422: {
            u_offset = 0;
            v_offset = 0;
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
    if (u_offset) {
        mpp_dev_set_reg_offset(dev, 71, u_offset);
        if (ret)
            mpp_err_f("set input cb addr offset failed %d\n", ret);
    }

    /* input cr addr */
    if (v_offset) {
        mpp_dev_set_reg_offset(dev, 72, v_offset);
        if (ret)
            mpp_err_f("set input cr addr offset failed %d\n", ret);
    }

    mpp_dev_set_reg_offset(dev, 83, mpp_buffer_get_size(task->output));
    if (ret)
        mpp_err_f("set output max addr offset failed %d\n", ret);

    return ret;
}

MPP_RET vepu541_h265_set_roi(void *dst_buf, void *src_buf, RK_S32 w, RK_S32 h)
{
    Vepu541RoiCfg *src = (Vepu541RoiCfg *)src_buf;
    Vepu541RoiCfg *dst = (Vepu541RoiCfg *)dst_buf;
    RK_S32 mb_w = MPP_ALIGN(w, 64) / 64;
    RK_S32 mb_h = MPP_ALIGN(h, 64) / 64;
    RK_S32 ctu_line = mb_w;
    RK_S32 i, j, cu16cnt;

    for (j = 0; j < mb_h; j++) {
        for ( i = 0; i < mb_w; i++) {
            RK_S32 ctu_addr = j * ctu_line + i;
            RK_S32 cu16_num_line = ctu_line * 4;
            for ( cu16cnt = 0; cu16cnt < 16; cu16cnt++) {
                RK_S32 cu16_x;
                RK_S32 cu16_y;
                RK_S32 cu16_addr_in_frame;
                cu16_x = cu16cnt & 3;
                cu16_y = cu16cnt / 4;
                cu16_x += i * 4;
                cu16_y += j * 4;
                cu16_addr_in_frame = cu16_x + cu16_y * cu16_num_line;
                dst[ctu_addr * 16 + cu16cnt] = src[cu16_addr_in_frame];
            }
        }
    }
    return MPP_OK;
}

static MPP_RET setup_vepu541_intra_refresh(H265eV541RegSet *regs, H265eV541HalContext *ctx, RK_U32 refresh_idx)
{
    MPP_RET ret = MPP_OK;
    RK_U32 h = ctx->cfg->prep.height;
    RK_U32 w = ctx->cfg->prep.width;
    MppEncROIRegion *region = NULL;
    RK_U32 stride_h = MPP_ALIGN(w / 16, 4);
    RK_U32 stride_v = MPP_ALIGN(h / 16, 4);
    RK_U32 i = 0;
    RK_U32 roi_buf_size;

    hal_h265e_dbg_func("enter\n");

    if (!ctx->cfg->rc.refresh_en) {
        ret = MPP_ERR_VALUE;
        goto RET;
    }

    roi_buf_size = vepu541_get_roi_buf_size(w, h);
    if (ctx->roi_buf_size < roi_buf_size) {
        if (NULL == ctx->roi_grp)
            mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);
        if (ctx->roi_buf)
            mpp_buffer_put(ctx->roi_buf);
        MPP_FREE(ctx->roi_buf_tmp);
        ctx->roi_buf_tmp = mpp_calloc_size(Vepu541RoiCfg, roi_buf_size);
        mpp_buffer_get(ctx->roi_grp, &ctx->roi_buf, roi_buf_size);
        ctx->roi_buf_size = roi_buf_size;
    }

    mpp_assert(ctx->roi_buf);
    mpp_assert(ctx->roi_buf_tmp);
    RK_S32 fd = mpp_buffer_get_fd(ctx->roi_buf);
    void *buf = ctx->roi_buf_tmp;
    void *dst_buf = mpp_buffer_get_ptr(ctx->roi_buf);
    Vepu541RoiCfg cfg;
    Vepu541RoiCfg *ptr = (Vepu541RoiCfg *)buf;
    cfg.force_intra = 0;
    cfg.reserved    = 0;
    cfg.qp_area_idx = 0;
    cfg.qp_area_en  = 1;
    cfg.qp_adj      = 0;
    cfg.qp_adj_mode = 0;

    for (i = 0; i < stride_h * stride_v; i++, ptr++)
        memcpy(ptr, &cfg, sizeof(cfg));

    region = mpp_calloc(MppEncROIRegion, 1);

    if (NULL == region) {
        mpp_err_f("Failed to calloc for MppEncROIRegion !\n");
        ret = MPP_ERR_MALLOC;
    }

    if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_ROW) {
        region->x = 0;
        region->w = w;
        if (refresh_idx > 0) {
            region->y = refresh_idx * 64 * ctx->cfg->rc.refresh_num - 128;
            region->h = 64 * ctx->cfg->rc.refresh_num + 128;
        } else {
            region->y = refresh_idx * 64 * ctx->cfg->rc.refresh_num;
            region->h = 64 * ctx->cfg->rc.refresh_num;
        }
        regs->me_rnge.cime_srch_v = 1;
    } else if (ctx->cfg->rc.refresh_mode == MPP_ENC_RC_INTRA_REFRESH_COL) {
        region->y = 0;
        region->h = h;
        if (refresh_idx > 0) {
            region->x = refresh_idx * 64 * ctx->cfg->rc.refresh_num - 128;
            region->w = 64 * ctx->cfg->rc.refresh_num + 128;
        } else {
            region->x = refresh_idx * 64 * ctx->cfg->rc.refresh_num;
            region->w = 64 * ctx->cfg->rc.refresh_num;
        }
        regs->me_rnge.cime_srch_h = 1;
    }

    region->intra = 1;
    region->quality = -ctx->cfg->rc.qp_delta_ip;

    region->area_map_en = 1;
    region->qp_area_idx = 1;
    region->abs_qp_en = 0;

    regs->enc_pic.roi_en = 1;
    regs->roi_addr_hevc = fd;
    vepu541_set_one_roi(buf, region, w, h);
    vepu541_h265_set_roi(dst_buf, buf, w, h);
    mpp_free(region);
    mpp_buffer_sync_end(ctx->roi_buf);

RET:
    hal_h265e_dbg_func("leave, ret %d\n", ret);
    return ret;
}

static MPP_RET
vepu541_h265_set_roi_regs(H265eV541HalContext *ctx, H265eV541RegSet *regs)
{
    if (ctx->roi_data2) {
        MppEncROICfg2 *cfg = ( MppEncROICfg2 *)ctx->roi_data2;

        regs->enc_pic.roi_en = 1;
        regs->roi_addr_hevc = mpp_buffer_get_fd(cfg->base_cfg_buf);
    } else if (ctx->qpmap) {
        regs->enc_pic.roi_en = 1;
        regs->roi_addr_hevc = mpp_buffer_get_fd(ctx->qpmap);
    } else {
        MppEncROICfg *cfg = (MppEncROICfg*)ctx->roi_data;
        RK_U32 h =  ctx->cfg->prep.height;
        RK_U32 w = ctx->cfg->prep.width;
        RK_U8 *roi_base;

        if (!cfg)
            return MPP_OK;

        if (cfg->number && cfg->regions) {
            RK_U32 roi_buf_size = vepu541_get_roi_buf_size(w, h);

            if (!ctx->roi_buf || roi_buf_size != ctx->roi_buf_size) {
                if (NULL == ctx->roi_grp)
                    mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);
                else if (roi_buf_size != ctx->roi_buf_size) {
                    if (ctx->roi_buf) {
                        mpp_buffer_put(ctx->roi_buf);
                        ctx->roi_buf = NULL;
                    }
                    MPP_FREE(ctx->roi_buf_tmp);
                    mpp_buffer_group_clear(ctx->roi_grp);
                }
                mpp_assert(ctx->roi_grp);
                if (NULL == ctx->roi_buf)
                    mpp_buffer_get(ctx->roi_grp, &ctx->roi_buf, roi_buf_size);

                if (ctx->roi_buf_tmp == NULL)
                    ctx->roi_buf_tmp = (Vepu541RoiCfg*)mpp_malloc(RK_U8, roi_buf_size);

                ctx->roi_buf_size = roi_buf_size;
            }

            regs->enc_pic.roi_en = 1;
            regs->roi_addr_hevc = mpp_buffer_get_fd(ctx->roi_buf);
            roi_base = (RK_U8 *)mpp_buffer_get_ptr(ctx->roi_buf);
            vepu541_set_roi(ctx->roi_buf_tmp, cfg, w, h);
            vepu541_h265_set_roi(roi_base, ctx->roi_buf_tmp, w, h);
        }
    }

    return MPP_OK;
}

static MPP_RET vepu541_h265_set_rc_regs(H265eV541HalContext *ctx, H265eV541RegSet *regs, HalEncTask *task)
{
    H265eSyntax_new *syn = (H265eSyntax_new *)task->syntax.data;
    EncRcTaskInfo *rc_cfg = &task->rc_task->info;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncHwCfg *hw = &cfg->hw;
    MppEncH265Cfg *h265 = &cfg->h265;
    RK_S32 mb_wd64, mb_h64;
    mb_wd64 = (syn->pp.pic_width + 63) / 64;
    mb_h64 = (syn->pp.pic_height + 63) / 64;

    RK_U32 ctu_target_bits_mul_16 = (rc_cfg->bit_target << 4) / (mb_wd64 * mb_h64);
    RK_U32 ctu_target_bits;
    RK_S32 negative_bits_thd, positive_bits_thd;

    if (rc->rc_mode == MPP_ENC_RC_MODE_FIXQP) {
        regs->enc_pic.pic_qp    = rc_cfg->quality_target;
        regs->synt_sli1.sli_qp  = rc_cfg->quality_target;

        regs->rc_qp.rc_max_qp   = rc_cfg->quality_target;
        regs->rc_qp.rc_min_qp   = rc_cfg->quality_target;
    } else {
        if (ctu_target_bits_mul_16 >= 0x100000) {
            ctu_target_bits_mul_16 = 0x50000;
        }
        ctu_target_bits = (ctu_target_bits_mul_16 * mb_wd64) >> 4;
        negative_bits_thd = 0 - 5 * ctu_target_bits / 16;
        positive_bits_thd = 5 * ctu_target_bits / 16;

        regs->enc_pic.pic_qp    = rc_cfg->quality_target;
        regs->synt_sli1.sli_qp  = rc_cfg->quality_target;
        regs->rc_cfg.rc_en      = 1;
        regs->rc_cfg.aqmode_en  = 1;
        regs->rc_cfg.qp_mode    = 1;

        regs->rc_cfg.rc_ctu_num = mb_wd64;

        regs->rc_qp.rc_qp_range = (ctx->frame_type == INTRA_FRAME) ?
                                  hw->qp_delta_row_i : hw->qp_delta_row;
        regs->rc_qp.rc_max_qp   = rc_cfg->quality_max;
        regs->rc_qp.rc_min_qp   = rc_cfg->quality_min;
        regs->rc_tgt.ctu_ebits  = ctu_target_bits_mul_16;

        regs->rc_erp0.bits_thd0 = 2 * negative_bits_thd;
        regs->rc_erp1.bits_thd1 = negative_bits_thd;
        regs->rc_erp2.bits_thd2 = positive_bits_thd;
        regs->rc_erp3.bits_thd3 = 2 * positive_bits_thd;
        regs->rc_erp4.bits_thd4 = 0x7FFFFFFF;
        regs->rc_erp5.bits_thd5 = 0x7FFFFFFF;
        regs->rc_erp6.bits_thd6 = 0x7FFFFFFF;
        regs->rc_erp7.bits_thd7 = 0x7FFFFFFF;
        regs->rc_erp8.bits_thd8 = 0x7FFFFFFF;

        regs->rc_adj0.qp_adjust0    = -2;
        regs->rc_adj0.qp_adjust1    = -1;
        regs->rc_adj0.qp_adjust2    = 0;
        regs->rc_adj0.qp_adjust3    = 1;
        regs->rc_adj0.qp_adjust4    = 2;
        regs->rc_adj1.qp_adjust5    = 0;
        regs->rc_adj1.qp_adjust6    = 0;
        regs->rc_adj1.qp_adjust7    = 0;
        regs->rc_adj1.qp_adjust8    = 0;

        regs->qpmap0.qpmin_area0 = h265->qpmin_map[0] > 0 ? h265->qpmin_map[0] : rc_cfg->quality_min;
        regs->qpmap0.qpmax_area0 = h265->qpmax_map[0] > 0 ? h265->qpmax_map[0] : rc_cfg->quality_max;
        regs->qpmap0.qpmin_area1 = h265->qpmin_map[1] > 0 ? h265->qpmin_map[1] : rc_cfg->quality_min;
        regs->qpmap0.qpmax_area1 = h265->qpmax_map[1] > 0 ? h265->qpmax_map[1] : rc_cfg->quality_max;
        regs->qpmap0.qpmin_area2 = h265->qpmin_map[2] > 0 ? h265->qpmin_map[2] : rc_cfg->quality_min;;
        regs->qpmap1.qpmax_area2 = h265->qpmax_map[2] > 0 ? h265->qpmax_map[2] : rc_cfg->quality_max;
        regs->qpmap1.qpmin_area3 = h265->qpmin_map[3] > 0 ? h265->qpmin_map[3] : rc_cfg->quality_min;;
        regs->qpmap1.qpmax_area3 = h265->qpmax_map[3] > 0 ? h265->qpmax_map[3] : rc_cfg->quality_max;
        regs->qpmap1.qpmin_area4 = h265->qpmin_map[4] > 0 ? h265->qpmin_map[4] : rc_cfg->quality_min;;
        regs->qpmap1.qpmax_area4 = h265->qpmax_map[4] > 0 ? h265->qpmax_map[4] : rc_cfg->quality_max;
        regs->qpmap2.qpmin_area5 = h265->qpmin_map[5] > 0 ? h265->qpmin_map[5] : rc_cfg->quality_min;;
        regs->qpmap2.qpmax_area5 = h265->qpmax_map[5] > 0 ? h265->qpmax_map[5] : rc_cfg->quality_max;
        regs->qpmap2.qpmin_area6 = h265->qpmin_map[6] > 0 ? h265->qpmin_map[6] : rc_cfg->quality_min;;
        regs->qpmap2.qpmax_area6 = h265->qpmax_map[6] > 0 ? h265->qpmax_map[6] : rc_cfg->quality_max;
        regs->qpmap2.qpmin_area7 = h265->qpmin_map[7] > 0 ? h265->qpmin_map[7] : rc_cfg->quality_min;;
        regs->qpmap3.qpmax_area7 = h265->qpmax_map[7] > 0 ? h265->qpmax_map[7] : rc_cfg->quality_max;
        regs->qpmap3.qpmap_mode  = h265->qpmap_mode;
    }
    if (ctx->frame_type == INTRA_FRAME) {
        regs->enc_pic.rdo_wgt_sel = 0;
    } else {
        regs->enc_pic.rdo_wgt_sel = 1;
    }
    return MPP_OK;
}

static MPP_RET vepu541_h265_set_pp_regs(VepuFmtCfg *fmt, H265eV541HalContext *ctx, HalEncTask *task)
{
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;
    H265eV541RegSet *regs = ctx->regs;
    MppEncPrepCfg *prep_cfg = &ctx->cfg->prep;
    MppFrameFormat prep_fmt = prep_cfg->format;

    regs->dtrns_map.src_bus_edin = fmt->src_endian;
    regs->src_fmt.src_cfmt = fmt->format;
    regs->src_fmt.alpha_swap = fmt->alpha_swap;
    regs->src_fmt.rbuv_swap = fmt->rbuv_swap;
    regs->src_fmt.src_range = fmt->src_range;
    regs->src_proc.src_mirr = prep_cfg->mirroring > 0;
    regs->src_proc.src_rot = prep_cfg->rotation;

    if (!ctx->frame_num && (prep_fmt == MPP_FMT_YUV420SP_VU || prep_fmt == MPP_FMT_YUV422SP_VU))
        mpp_logw("Warning: nv21/nv42 fmt not supported, will encode as nv12/nv24.\n");

    if (MPP_FRAME_FMT_IS_FBC(prep_fmt)) {
        stridey = mpp_frame_get_fbc_hdr_stride(task->frame);
        if (!stridey)
            stridey = MPP_ALIGN(prep_cfg->hor_stride, 16);
    } else if (prep_cfg->hor_stride) {
        stridey = prep_cfg->hor_stride;
    } else {
        if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGRA8888)
            stridey = prep_cfg->width * 4;
        else if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGR888)
            stridey = prep_cfg->width * 3;
        else if (regs->src_fmt.src_cfmt == VEPU5xx_FMT_BGR565 ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUYV422 ||
                 regs->src_fmt.src_cfmt == VEPU5xx_FMT_UYVY422)
            stridey = prep_cfg->width * 2;
    }

    stridec = (regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV422SP ||
               regs->src_fmt.src_cfmt == VEPU5xx_FMT_YUV420SP) ?
              stridey : stridey / 2;

    if (regs->src_fmt.src_cfmt < VEPU5xx_FMT_ARGB1555) {
        const VepuRgb2YuvCfg *cfg_coeffs = cfg_coeffs = get_rgb2yuv_cfg(prep_cfg->range, prep_cfg->color);

        hal_h265e_dbg_simple("input color range %d colorspace %d", prep_cfg->range, prep_cfg->color);

        regs->src_udfy.wght_r2y = cfg_coeffs->_2y.r_coeff;
        regs->src_udfy.wght_g2y = cfg_coeffs->_2y.g_coeff;
        regs->src_udfy.wght_b2y = cfg_coeffs->_2y.b_coeff;

        regs->src_udfu.wght_r2u = cfg_coeffs->_2u.r_coeff;
        regs->src_udfu.wght_g2u = cfg_coeffs->_2u.g_coeff;
        regs->src_udfu.wght_b2u = cfg_coeffs->_2u.b_coeff;

        regs->src_udfv.wght_r2v = cfg_coeffs->_2v.r_coeff;
        regs->src_udfv.wght_g2v = cfg_coeffs->_2v.g_coeff;
        regs->src_udfv.wght_b2v = cfg_coeffs->_2v.b_coeff;

        regs->src_udfo.ofst_y = cfg_coeffs->_2y.offset;
        regs->src_udfo.ofst_u = cfg_coeffs->_2u.offset;
        regs->src_udfo.ofst_v = cfg_coeffs->_2v.offset;

        hal_h265e_dbg_simple("use color range %d colorspace %d", cfg_coeffs->dst_range, cfg_coeffs->color);
    }

    regs->src_strid.src_ystrid  = stridey;
    regs->src_strid.src_cstrid  = stridec;

    return MPP_OK;
}

static void vepu541_h265_set_slice_regs(H265eSyntax_new *syn, H265eV541RegSet *regs)
{
    regs->synt_sps.smpl_adpt_ofst_en    = syn->pp.sample_adaptive_offset_enabled_flag;//slice->m_sps->m_bUseSAO;
    regs->synt_sps.num_st_ref_pic       = syn->pp.num_short_term_ref_pic_sets;
    regs->synt_sps.num_lt_ref_pic       = syn->pp.num_long_term_ref_pics_sps;
    regs->synt_sps.lt_ref_pic_prsnt     = syn->pp.long_term_ref_pics_present_flag;
    regs->synt_sps.tmpl_mvp_en          = syn->pp.sps_temporal_mvp_enabled_flag;
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
    regs->synt_sli0.merge_up_flag       = syn->sp.merge_up_flag;
    regs->synt_sli0.merge_left_flag     = syn->sp.merge_left_flag;
    regs->synt_sli0.ref_pic_lst_mdf_l0  = syn->sp.ref_pic_lst_mdf_l0;

    regs->synt_sli0.num_refidx_l1_act   = syn->sp.num_refidx_l1_act;
    regs->synt_sli0.num_refidx_l0_act   = syn->sp.num_refidx_l0_act;

    regs->synt_sli0.num_refidx_act_ovrd = syn->sp.num_refidx_act_ovrd;

    regs->synt_sli0.sli_sao_chrm_flg    = syn->sp.sli_sao_chrm_flg;
    regs->synt_sli0.sli_sao_luma_flg    = syn->sp.sli_sao_luma_flg;
    regs->synt_sli0.sli_tmprl_mvp_en    = syn->sp.sli_tmprl_mvp_en;
    regs->enc_pic.tot_poc_num           = syn->sp.tot_poc_num;

    regs->synt_sli0.pic_out_flg         = syn->sp.pic_out_flg;
    regs->synt_sli0.sli_type            = syn->sp.slice_type;
    regs->synt_sli0.sli_rsrv_flg        = syn->sp.slice_rsrv_flg;
    regs->synt_sli0.dpdnt_sli_seg_flg   = syn->sp.dpdnt_sli_seg_flg;
    regs->synt_sli0.sli_pps_id          = syn->sp.sli_pps_id;
    regs->synt_sli0.no_out_pri_pic      = syn->sp.no_out_pri_pic;


    regs->synt_sli1.sli_tc_ofst_div2      = syn->sp.sli_tc_ofst_div2;;
    regs->synt_sli1.sli_beta_ofst_div2    = syn->sp.sli_beta_ofst_div2;
    regs->synt_sli1.sli_lp_fltr_acrs_sli  = syn->sp.sli_lp_fltr_acrs_sli;
    regs->synt_sli1.sli_dblk_fltr_dis     = syn->sp.sli_dblk_fltr_dis;
    regs->synt_sli1.dblk_fltr_ovrd_flg    = syn->sp.dblk_fltr_ovrd_flg;
    regs->synt_sli1.sli_cb_qp_ofst        = syn->pp.pps_slice_chroma_qp_offsets_present_flag ?
                                            syn->sp.sli_cb_qp_ofst : syn->pp.pps_cb_qp_offset;
    regs->synt_sli1.max_mrg_cnd           = syn->sp.max_mrg_cnd;

    regs->synt_sli1.col_ref_idx           = syn->sp.col_ref_idx;
    regs->synt_sli1.col_frm_l0_flg        = syn->sp.col_frm_l0_flg;
    regs->synt_sli2_rodr.sli_poc_lsb      = syn->sp.sli_poc_lsb;
    regs->synt_sli2_rodr.sli_hdr_ext_len  = syn->sp.sli_hdr_ext_len;

}

static void vepu541_h265_set_ref_regs(H265eSyntax_new *syn, H265eV541RegSet *regs)
{
    regs->synt_ref_mark0.st_ref_pic_flg = syn->sp.st_ref_pic_flg;
    regs->synt_ref_mark0.poc_lsb_lt0 = syn->sp.poc_lsb_lt0;
    regs->synt_ref_mark0.num_lt_pic = syn->sp.num_lt_pic;

    regs->synt_ref_mark1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_ref_mark1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_ref_mark1.used_by_lt_flg0 = syn->sp.used_by_lt_flg0;
    regs->synt_ref_mark1.used_by_lt_flg1 = syn->sp.used_by_lt_flg1;
    regs->synt_ref_mark1.used_by_lt_flg2 = syn->sp.used_by_lt_flg2;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt0 = syn->sp.dlt_poc_msb_prsnt0;
    regs->synt_ref_mark1.dlt_poc_msb_cycl0 = syn->sp.dlt_poc_msb_cycl0;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt1 = syn->sp.dlt_poc_msb_prsnt1;
    regs->synt_ref_mark1.num_neg_pic = syn->sp.num_neg_pic;
    regs->synt_ref_mark1.num_pos_pic = syn->sp.num_pos_pic;

    regs->synt_ref_mark1.used_by_s0_flg = syn->sp.used_by_s0_flg;
    regs->synt_ref_mark2.dlt_poc_s0_m10 = syn->sp.dlt_poc_s0_m10;
    regs->synt_ref_mark2.dlt_poc_s0_m11 = syn->sp.dlt_poc_s0_m11;
    regs->synt_ref_mark3.dlt_poc_s0_m12 = syn->sp.dlt_poc_s0_m12;
    regs->synt_ref_mark3.dlt_poc_s0_m13 = syn->sp.dlt_poc_s0_m13;

    regs->synt_ref_mark4.poc_lsb_lt1 = syn->sp.poc_lsb_lt1;
    regs->synt_ref_mark5.dlt_poc_msb_cycl1 = syn->sp.dlt_poc_msb_cycl1;
    regs->synt_ref_mark4.poc_lsb_lt2 = syn->sp.poc_lsb_lt2;
    regs->synt_ref_mark1.dlt_poc_msb_prsnt2 = syn->sp.dlt_poc_msb_prsnt2;
    regs->synt_ref_mark5.dlt_poc_msb_cycl2 = syn->sp.dlt_poc_msb_cycl2;
    regs->synt_sli1.lst_entry_l0 = syn->sp.lst_entry_l0;
    regs->synt_sli0.ref_pic_lst_mdf_l0 = syn->sp.ref_pic_lst_mdf_l0;

    return;
}
static void vepu541_h265_set_me_regs(H265eV541HalContext *ctx, H265eSyntax_new *syn, H265eV541RegSet *regs)
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
            merangx =  syn->pp.pic_width;
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
    regs->me_rnge.cime_srch_h    = merangx / 32;
    regs->me_rnge.cime_srch_v    = merangy / 32;

    regs->me_rnge.rime_srch_h    = 7;
    regs->me_rnge.rime_srch_v    = 5;
    regs->me_rnge.dlt_frm_num    = 0x1;

    regs->me_cnst.pmv_mdst_h    = 5;
    regs->me_cnst.pmv_mdst_v    = 5;
    regs->me_cnst.mv_limit      = 0;
    regs->me_cnst.mv_num        = 2;

    if (syn->pp.sps_temporal_mvp_enabled_flag &&
        (ctx->frame_type != INTRA_FRAME)) {
        if (ctx->last_frame_type == INTRA_FRAME) {
            regs->me_cnst.colmv_load    = 0;
        } else {
            regs->me_cnst.colmv_load    = 1;
        }
        regs->me_cnst.colmv_store   = 1;
    }

    if (syn->pp.pic_width > 2688) {
        regs->me_ram.cime_rama_h = 12;
    } else if (syn->pp.pic_width > 2048) {
        regs->me_ram.cime_rama_h = 16;
    } else {
        regs->me_ram.cime_rama_h = 20;
    }

    {
        RK_S32 swin_scope_wd16 = (regs->me_rnge.cime_srch_h  + 3 + 1) / 4 * 2 + 1;
        RK_S32 tmpMin = (regs->me_rnge.cime_srch_v + 3) / 4 * 2 + 1;
        if (regs->me_ram.cime_rama_h / 4 < tmpMin) {
            tmpMin = regs->me_ram.cime_rama_h / 4;
        }
        regs->me_ram.cime_rama_max =
            (pic_wd64 * (tmpMin - 1)) + ((pic_wd64 >= swin_scope_wd16) ? swin_scope_wd16 : pic_wd64 * 2);
    }
    regs->me_ram.cach_l2_tag      = 0x0;

    pic_wd64 = pic_wd64 << 6;

    if (pic_wd64 <= 512)
        regs->me_ram.cach_l2_tag  = 0x0;
    else if (pic_wd64 <= 1024)
        regs->me_ram.cach_l2_tag  = 0x1;
    else if (pic_wd64 <= 2048)
        regs->me_ram.cach_l2_tag  = 0x2;
    else if (pic_wd64 <= 4096)
        regs->me_ram.cach_l2_tag  = 0x3;
}

static void vepu540_h265_set_me_ram(H265eSyntax_new *syn, H265eV541RegSet *regs,
                                    RK_U32 index, RK_S32 tile_start_x)
{
    RK_U32 cime_w = 11, cime_h = 7;
    RK_U32 pic_cime_temp = 0;
    if (syn->pp.tiles_enabled_flag == 0) {
        pic_cime_temp = ((regs->enc_rsl.pic_wd8_m1 + 1) * 8 + 63) / 64 * 64;
        regs->me_ram.cime_linebuf_w = pic_cime_temp / 64;
    } else {
        RK_S32 pic_wd64 = MPP_ALIGN(syn->pp.pic_width, 64) >> 6;
        RK_S32 tile_ctu_stax = tile_start_x;
        RK_S32 tile_ctu_endx = tile_start_x + syn->pp.column_width_minus1[index];
        RK_S32 cime_srch_w = regs->me_rnge.cime_srch_h;

        if (tile_ctu_stax < (cime_srch_w + 3) / 4) {
            if (tile_ctu_endx + 1 + (cime_srch_w + 3) / 4 > pic_wd64)
                pic_cime_temp = pic_wd64 * 64;
            else
                pic_cime_temp = (tile_ctu_endx + 1 + (cime_srch_w + 3) / 4) * 64;
        } else {
            if (tile_ctu_endx + 1 + (cime_srch_w + 3) / 4 > pic_wd64)
                pic_cime_temp = (pic_wd64 - tile_ctu_stax + (cime_srch_w + 3) / 4) * 64;
            else
                pic_cime_temp = (tile_ctu_endx - tile_ctu_stax + 1 + (cime_srch_w + 3) / 4 * 2) * 64;
        }
        regs->me_ram.cime_linebuf_w = pic_cime_temp / 64;

    }

    {

        RK_S32 w_temp = 1296;
        RK_S32 h_temp = 4;
        RK_S32 h_val_0 = 4;
        RK_S32 h_val_1 = 24;

        while ((w_temp > ((h_temp - h_val_0)*regs->me_ram.cime_linebuf_w * 4 + ((h_val_1 - h_temp) * 4 * 7)))
               && (h_temp < 17)) {
            h_temp = h_temp + h_val_0;
        }
        if (w_temp < (RK_S32)((h_temp - h_val_0)*regs->me_ram.cime_linebuf_w * 4 + ((h_val_1 - h_temp) * 4 * 7)))
            h_temp = h_temp - h_val_0;

        regs->me_ram.cime_rama_h = h_temp;
    }

    // calc cime_rama_max
    {
        RK_S32 pic_wd64 = pic_cime_temp / 64;
        RK_S32 swin_scope_wd16 = (cime_w + 3 + 1) / 4 * 2 + 1;
        RK_S32 tmpMin = (cime_h + 3) / 4 * 2 + 1;
        if (regs->me_ram.cime_rama_h / 4 < tmpMin) {
            tmpMin = regs->me_ram.cime_rama_h / 4;
        }
        regs->me_ram.cime_rama_max = (pic_wd64 * (tmpMin - 1)) + ((pic_wd64 >= swin_scope_wd16) ? swin_scope_wd16 : pic_wd64 * 2);
    }

    hal_h265e_dbg_detail("cime_rama_h %d, cime_rama_max %d, cime_linebuf_w %d",
                         regs->me_ram.cime_rama_h, regs->me_ram.cime_rama_max, regs->me_ram.cime_linebuf_w);
}
void vepu54x_h265_set_hw_address(H265eV541HalContext *ctx, H265eV541RegSet *regs, HalEncTask *task)
{
    HalEncTask *enc_task = task;
    HalBuf *recon_buf, *ref_buf;
    MppBuffer md_info_buf = enc_task->md_info;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;

    hal_h265e_enter();

    regs->adr_srcy_hevc     = mpp_buffer_get_fd(enc_task->input);
    regs->adr_srcu_hevc     = regs->adr_srcy_hevc;
    regs->adr_srcv_hevc     = regs->adr_srcy_hevc;

    recon_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.recon_pic.slot_idx);
    ref_buf = hal_bufs_get_buf(ctx->dpb_bufs, syn->sp.ref_pic.slot_idx);
    if (!syn->sp.non_reference_flag) {
        regs->rfpw_h_addr_hevc  = mpp_buffer_get_fd(recon_buf->buf[0]);
        regs->rfpw_b_addr_hevc  = regs->rfpw_h_addr_hevc;

        mpp_dev_set_reg_offset(ctx->dev, 75, ctx->fbc_header_len);
    }

    regs->dspw_addr_hevc = mpp_buffer_get_fd(recon_buf->buf[1]);
    regs->cmvw_addr_hevc  = mpp_buffer_get_fd(recon_buf->buf[2]);
    regs->rfpr_h_addr_hevc = mpp_buffer_get_fd(ref_buf->buf[0]);
    regs->rfpr_b_addr_hevc = regs->rfpr_h_addr_hevc;
    regs->dspr_addr_hevc = mpp_buffer_get_fd(ref_buf->buf[1]);
    regs->cmvr_addr_hevc = mpp_buffer_get_fd(ref_buf->buf[2]);

    mpp_dev_set_reg_offset(ctx->dev, 77, ctx->fbc_header_len);

    if (syn->pp.tiles_enabled_flag) {
        if (NULL == ctx->tile_grp)
            mpp_buffer_group_get_internal(&ctx->tile_grp, MPP_BUFFER_TYPE_ION);

        mpp_assert(ctx->tile_grp);

        if (NULL == ctx->hw_tile_buf[0]) {
            mpp_buffer_get(ctx->tile_grp, &ctx->hw_tile_buf[0], TILE_BUF_SIZE);
        }

        if (NULL == ctx->hw_tile_buf[1]) {
            mpp_buffer_get(ctx->tile_grp, &ctx->hw_tile_buf[1], TILE_BUF_SIZE);
        }

        regs->lpfw_addr_hevc  = mpp_buffer_get_fd(ctx->hw_tile_buf[0]);
        regs->lpfr_addr_hevc = mpp_buffer_get_fd(ctx->hw_tile_buf[1]);
    }

    if (md_info_buf) {
        regs->enc_pic.mei_stor    = 1;
        regs->meiw_addr_hevc = mpp_buffer_get_fd(md_info_buf);
    } else {
        regs->enc_pic.mei_stor    = 0;
        regs->meiw_addr_hevc = 0;
    }

    regs->bsbb_addr_hevc    = mpp_buffer_get_fd(enc_task->output);
    /* TODO: stream size relative with syntax */
    regs->bsbt_addr_hevc    = regs->bsbb_addr_hevc;
    regs->bsbr_addr_hevc    = regs->bsbb_addr_hevc;
    regs->bsbw_addr_hevc    = regs->bsbb_addr_hevc;

    mpp_dev_set_reg_offset(ctx->dev, 86, mpp_packet_get_length(task->packet));

    regs->pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);
}

static void setup_vepu541_split(H265eV541RegSet *regs, MppEncSliceSplit *cfg)
{
    hal_h265e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        regs->sli_spl.sli_splt = 0;
        regs->sli_spl.sli_splt_mode = 0;
        regs->sli_spl.sli_splt_cpst = 0;
        regs->sli_spl.sli_max_num_m1 = 0;
        regs->sli_spl.sli_flsh = 0;
        regs->sli_spl.sli_splt_cnum_m1 = 0;

        regs->sli_spl_byte.sli_splt_byte = 0;
        regs->enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        regs->sli_spl.sli_splt = 1;
        regs->sli_spl.sli_splt_mode = 0;
        regs->sli_spl.sli_splt_cpst = 0;
        regs->sli_spl.sli_max_num_m1 = 500;
        regs->sli_spl.sli_flsh = 1;
        regs->sli_spl.sli_splt_cnum_m1 = 0;

        regs->sli_spl_byte.sli_splt_byte = cfg->split_arg;
        regs->enc_pic.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        regs->sli_spl.sli_splt = 1;
        regs->sli_spl.sli_splt_mode = 1;
        regs->sli_spl.sli_splt_cpst = 0;
        regs->sli_spl.sli_max_num_m1 = 500;
        regs->sli_spl.sli_flsh = 1;
        regs->sli_spl.sli_splt_cnum_m1 = cfg->split_arg - 1;

        regs->sli_spl_byte.sli_splt_byte = 0;
        regs->enc_pic.slen_fifo = 0;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }

    cfg->change = 0;

    hal_h265e_dbg_func("leave\n");
}

MPP_RET hal_h265e_v541_gen_regs(void *hal, HalEncTask *task)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    HalEncTask *enc_task = task;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;
    H265eV541RegSet *regs = ctx->regs;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 pic_wd64, pic_h64;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    EncFrmStatus *frm_status = &task->rc_task->frm;

    hal_h265e_enter();
    pic_width_align8 = (syn->pp.pic_width + 7) & (~7);
    pic_height_align8 = (syn->pp.pic_height + 7) & (~7);
    pic_wd64 = (syn->pp.pic_width + 63) / 64;
    pic_h64 = (syn->pp.pic_height + 63) / 64;

    hal_h265e_dbg_simple("frame %d | type %d | start gen regs",
                         ctx->frame_cnt, ctx->frame_type);


    memset(regs, 0, sizeof(H265eV541RegSet));
    regs->enc_strt.lkt_num      = 0;
    regs->enc_strt.rkvenc_cmd   = ctx->enc_mode;
    regs->enc_strt.enc_cke      = 1;
    regs->enc_clr.safe_clr      = 0x0;

    regs->lkt_addr.lkt_addr     = 0x0;
    regs->int_en.enc_done_en    = 1;
    regs->int_en.lkt_done_en    = 1;
    regs->int_en.sclr_done_en   = 1;
    regs->int_en.slc_done_en    = 1;
    regs->int_en.bsf_ovflw_en   = 1;
    regs->int_en.brsp_ostd_en   = 1;
    regs->int_en.wbus_err_en    = 1;
    regs->int_en.rbus_err_en    = 1;
    regs->int_en.wdg_en         = 0;

    regs->enc_rsl.pic_wd8_m1    = pic_width_align8 / 8 - 1;
    regs->enc_rsl.pic_wfill     = (syn->pp.pic_width & 0x7)
                                  ? (8 - (syn->pp.pic_width & 0x7)) : 0;
    regs->enc_rsl.pic_hd8_m1    = pic_height_align8 / 8 - 1;
    regs->enc_rsl.pic_hfill     = (syn->pp.pic_height & 0x7)
                                  ? (8 - (syn->pp.pic_height & 0x7)) : 0;

    regs->enc_pic.enc_stnd      = 1; //H265
    regs->enc_pic.cur_frm_ref   = !syn->sp.non_reference_flag; //current frame will be refered
    regs->enc_pic.bs_scp        = 1;
    regs->enc_pic.node_int      = 0;
    regs->enc_pic.log2_ctu_num  = ceil(log2((double)pic_wd64 * pic_h64));

    regs->enc_pic.rdo_wgt_sel   = (ctx->frame_type == INTRA_FRAME) ? 0 : 1;

    regs->enc_wdg.vs_load_thd   = 0;
    regs->enc_wdg.rfp_load_thd  = 0;

    if (ctx->is_vepu540) {
        regs->dtrns_cfg_540.cime_dspw_orsd  = (ctx->frame_type == INTER_P_FRAME);
        regs->dtrns_cfg_540.axi_brsp_cke    = 0x0;
    } else {
        regs->dtrns_cfg_541.cime_dspw_orsd  = (ctx->frame_type == INTER_P_FRAME);
        regs->dtrns_cfg_541.axi_brsp_cke    = 0x0;
    }

    regs->dtrns_map.lpfw_bus_ordr   = 0x0;
    regs->dtrns_map.cmvw_bus_ordr   = 0x0;
    regs->dtrns_map.dspw_bus_ordr   = 0x0;
    regs->dtrns_map.rfpw_bus_ordr   = 0x0;
    regs->dtrns_map.src_bus_edin    = 0x0;
    regs->dtrns_map.meiw_bus_edin   = 0x0;
    regs->dtrns_map.bsw_bus_edin    = 0x7;
    regs->dtrns_map.lktr_bus_edin   = 0x0;
    regs->dtrns_map.roir_bus_edin   = 0x0;
    regs->dtrns_map.lktw_bus_edin   = 0x0;
    regs->dtrns_map.afbc_bsize      = 0x1;


    regs->src_proc.src_mirr = 0;
    regs->src_proc.src_rot  = 0;
    regs->src_proc.txa_en   = 1;
    regs->src_proc.afbcd_en = (MPP_FRAME_FMT_IS_FBC(syn->pp.mpp_format)) ? 1 : 0;

    if (!ctx->is_vepu540)
        vepu541_h265_set_patch_info(ctx->dev, syn, (VepuFmt)fmt->format, task);

    regs->klut_ofst.chrm_kult_ofst = (ctx->frame_type == INTRA_FRAME) ? 0 : 3;
    memcpy(&regs->klut_wgt0, &klut_weight[0], sizeof(klut_weight));

    regs->adr_srcy_hevc     = mpp_buffer_get_fd(enc_task->input);
    regs->adr_srcu_hevc     = regs->adr_srcy_hevc;
    regs->adr_srcv_hevc     = regs->adr_srcy_hevc;

    setup_vepu541_split(regs, &ctx->cfg->split);

    vepu541_h265_set_me_regs(ctx, syn, regs);

    regs->rdo_cfg.chrm_special   = 1;
    regs->rdo_cfg.cu_inter_en    = 0xf;
    regs->rdo_cfg.cu_intra_en    = 0xf;

    if (syn->pp.num_long_term_ref_pics_sps) {
        regs->rdo_cfg.ltm_col   = 0;
        regs->rdo_cfg.ltm_idx0l0 = 1;
    } else {
        regs->rdo_cfg.ltm_col   = 0;
        regs->rdo_cfg.ltm_idx0l0 = 0;
    }

    regs->rdo_cfg.chrm_klut_en = 1;
    regs->rdo_cfg.seq_scaling_matrix_present_flg = syn->pp.scaling_list_enabled_flag;
    regs->synt_nal.nal_unit_type = h265e_get_nal_type(&syn->sp, ctx->frame_type);

    vepu54x_h265_set_hw_address(ctx, regs, task);
    vepu541_h265_set_pp_regs(fmt, ctx, task);

    vepu541_h265_set_rc_regs(ctx, regs, task);

    vepu541_h265_set_slice_regs(syn, regs);

    vepu541_h265_set_ref_regs(syn, regs);
    if (ctx->is_vepu540) {
        vepu540_set_osd(&ctx->osd_cfg);
    } else {
        vepu541_set_osd(&ctx->osd_cfg);
    }
    /* ROI configure */
    vepu541_h265_set_roi_regs(ctx, regs);

    if (frm_status->is_i_refresh)
        setup_vepu541_intra_refresh(regs, ctx, frm_status->seq_idx % ctx->cfg->rc.gop);

    ctx->frame_num++;

    hal_h265e_leave();
    return MPP_OK;
}
void hal_h265e_v540_set_uniform_tile(H265eV541RegSet *regs, H265eSyntax_new *syn,
                                     RK_U32 index, RK_S32 tile_start_x)
{
    if (syn->pp.tiles_enabled_flag) {
        RK_S32 mb_h = MPP_ALIGN(syn->pp.pic_height, 64) / 64;
        RK_S32 tile_width = syn->pp.column_width_minus1[index] + 1;

        regs->tile_cfg.tile_width_m1 = tile_width - 1;
        regs->tile_cfg.tile_height_m1 = mb_h - 1;
        regs->rc_cfg.rc_ctu_num   = tile_width;
        regs->tile_cfg.tile_en = syn->pp.tiles_enabled_flag;
        regs->tile_pos.tile_x = tile_start_x;
        regs->tile_pos.tile_y = 0;
        if (index > 0) {
            RK_U32 tmp = regs->lpfr_addr_hevc;
            regs->lpfr_addr_hevc = regs->lpfw_addr_hevc;
            regs->lpfw_addr_hevc = tmp;
        }

        hal_h265e_dbg_detail("tile_x %d, rc_ctu_num %d, tile_width_m1 %d",
                             regs->tile_pos.tile_x, regs->rc_cfg.rc_ctu_num,
                             regs->tile_cfg.tile_width_m1);
    }
}

MPP_RET hal_h265e_v540_start(void *hal, HalEncTask *enc_task)
{
    MPP_RET ret = MPP_OK;
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    RK_U32 length = 0, k = 0;
    H265eV541IoctlOutputElem *reg_out = (H265eV541IoctlOutputElem *)ctx->reg_out;
    H265eSyntax_new *syn = (H265eSyntax_new *)enc_task->syntax.data;
    RK_U32 title_num = (syn->pp.num_tile_columns_minus1 + 1) * (syn->pp.num_tile_rows_minus1 + 1);
    hal_h265e_enter();
    RK_U32 stream_len = 0;
    VepuFmtCfg *fmt = (VepuFmtCfg *)ctx->input_fmt;
    RK_S32 tile_start_x = 0;

    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return e arly",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    for (k = 0; k < title_num; k++) {    //v540 no support link list
        RK_U32 i;
        RK_U32 *regs = (RK_U32*)ctx->regs;
        H265eV541RegSet *hw_regs = ctx->regs;
        MppDevRegWrCfg cfg;
        MppDevRegRdCfg cfg1;

        vepu540_h265_set_me_ram(syn, hw_regs, k, tile_start_x);

        /* set input info */
        vepu541_h265_set_l2_regs(ctx, (H265eV54xL2RegSet*)ctx->l2_regs);
        vepu541_h265_set_patch_info(ctx->dev, syn, (VepuFmt)fmt->format, enc_task);
        if (title_num > 1)
            hal_h265e_v540_set_uniform_tile(hw_regs, syn, k, tile_start_x);
        if (k > 0) {
            RK_U32 offset = mpp_packet_get_length(enc_task->packet);

            offset += stream_len;
            hw_regs->bsbb_addr_hevc    = mpp_buffer_get_fd(enc_task->output);
            hw_regs->bsbw_addr_hevc    = hw_regs->bsbb_addr_hevc;
            mpp_dev_set_reg_offset(ctx->dev, 86, offset);
            mpp_dev_set_reg_offset(ctx->dev, 75, ctx->fbc_header_len);
            mpp_dev_set_reg_offset(ctx->dev, 77, ctx->fbc_header_len);
        }

        cfg.reg = ctx->regs;
        cfg.size = sizeof(H265eV541RegSet);
        cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }


        cfg1.reg = &reg_out->hw_status;
        cfg1.size = sizeof(RK_U32);
        cfg1.offset = VEPU541_REG_BASE_HW_STATUS;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        cfg1.reg = &reg_out->st_bsl;
        cfg1.size = sizeof(H265eV541IoctlOutputElem) - 4;
        cfg1.offset = VEPU541_REG_BASE_STATISTICS;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }
        for (i = 0; i < sizeof(H265eV541RegSet) / 4; i++) {
            hal_h265e_dbg_regs("set reg[%04x]: 0%08x\n", i * 4, regs[i]);
        }

        if (k < title_num - 1) {
            vepu541_h265_fbk *fb = &ctx->feedback;
            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
            if (ret) {
                mpp_err_f("send cmd failed %d\n", ret);
                break;
            }
            ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);

            stream_len += reg_out->st_bsl.bs_lgth;
            fb->qp_sum += reg_out->st_sse_qp.qp_sum;
            fb->out_strm_size += reg_out->st_bsl.bs_lgth;
            fb->sse_sum += reg_out->st_sse_l32.sse_l32 +
                           ((RK_S64)(reg_out->st_sse_qp.sse_h8 & 0xff) << 32);
            fb->st_madi += reg_out->st_madi;
            fb->st_madp += reg_out->st_madi;
            fb->st_mb_num += reg_out->st_mb_num;
            fb->st_ctu_num += reg_out->st_ctu_num;

            if (ret) {
                mpp_err_f("poll cmd failed %d\n", ret);
                ret = MPP_ERR_VPUHW;
            }

        }
        tile_start_x += (syn->pp.column_width_minus1[k] + 1);
    }

    hal_h265e_dbg_detail("vpu client is sending %d regs", length);
    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
    if (ret) {
        mpp_err_f("send cmd failed %d\n", ret);
    }
    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v541_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    RK_U32 length = 0;
    HalEncTask *enc_task = task;
    H265eV541IoctlOutputElem *reg_out = (H265eV541IoctlOutputElem *)ctx->reg_out;
    RK_U32 i;
    RK_U32 *regs = (RK_U32*)ctx->regs;
    hal_h265e_enter();

    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }
    vepu541_h265_set_l2_regs(ctx, (H265eV54xL2RegSet*)ctx->l2_regs);

    do {
        MppDevRegWrCfg cfg;

        cfg.reg = ctx->regs;
        cfg.size = sizeof(H265eV541RegSet);
        cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        MppDevRegRdCfg cfg1;

        cfg1.reg = &reg_out->hw_status;
        cfg1.size = sizeof(RK_U32);
        cfg1.offset = VEPU541_REG_BASE_HW_STATUS;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        cfg1.reg = &reg_out->st_bsl;
        cfg1.size = sizeof(H265eV541IoctlOutputElem) - 4;
        cfg1.offset = VEPU541_REG_BASE_STATISTICS;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &cfg1);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

    for (i = 0; i < sizeof(H265eV541RegSet) / 4; i++) {
        hal_h265e_dbg_regs("set reg[%04d]: 0%08x\n", i, regs[i]);
    }

    hal_h265e_dbg_detail("vpu client is sending %d regs", length);
    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v54x_start(void *hal, HalEncTask *task)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    task->hw_length = 0;
    if (ctx->is_vepu540) {
        return hal_h265e_v540_start(hal, task);
    } else {
        return hal_h265e_v541_start(hal, task);
    }
    return MPP_OK;
}

static MPP_RET vepu541_h265_set_feedback(H265eV541HalContext *ctx, HalEncTask *enc_task)
{
    EncRcTaskInfo *hal_rc_ret = (EncRcTaskInfo *)&enc_task->rc_task->info;
    vepu541_h265_fbk *fb = &ctx->feedback;
    MppEncCfgSet    *cfg = ctx->cfg;
    RK_S32 mb64_num = ((cfg->prep.width + 63) / 64) * ((cfg->prep.height + 63) / 64);
    RK_S32 mb8_num = (mb64_num << 6);
    RK_S32 mb4_num = (mb8_num << 2);

    hal_h265e_enter();
    H265eV541IoctlOutputElem *elem = (H265eV541IoctlOutputElem *)ctx->reg_out;
    RK_U32 hw_status = elem->hw_status;
    fb->qp_sum += elem->st_sse_qp.qp_sum;
    fb->out_strm_size += elem->st_bsl.bs_lgth;
    fb->sse_sum += elem->st_sse_l32.sse_l32 +
                   ((RK_S64)(elem->st_sse_qp.sse_h8 & 0xff) << 32);

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

    fb->st_madi += elem->st_madi;
    fb->st_madp += elem->st_madp;
    fb->st_mb_num += elem->st_mb_num;
    fb->st_ctu_num += elem->st_ctu_num;

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

    fb->st_lvl64_inter_num = elem->st_lvl64_inter_num;
    fb->st_lvl32_inter_num = elem->st_lvl32_inter_num;
    fb->st_lvl32_intra_num = elem->st_lvl32_intra_num;
    fb->st_lvl16_inter_num = elem->st_lvl16_inter_num;
    fb->st_lvl16_intra_num = elem->st_lvl16_intra_num;
    fb->st_lvl8_inter_num  = elem->st_lvl8_inter_num;
    fb->st_lvl8_intra_num  = elem->st_lvl8_intra_num;
    fb->st_lvl4_intra_num  = elem->st_lvl4_intra_num;
    memcpy(&fb->st_cu_num_qp[0], &elem->st_cu_num_qp[0], sizeof(elem->st_cu_num_qp));

    hal_rc_ret->madi = fb->st_madi;
    hal_rc_ret->madp = fb->st_madp;
    hal_rc_ret->bit_real = fb->out_strm_size * 8;

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

    hal_h265e_leave();

    return MPP_OK;
}


MPP_RET hal_h265e_v541_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    HalEncTask *enc_task = task;
    H265eV541IoctlOutputElem *elem = (H265eV541IoctlOutputElem *)ctx->reg_out;
    hal_h265e_enter();
    if (enc_task->flags.err) {
        hal_h265e_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }
    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d status %d \n", ret, elem->hw_status);

    hal_h265e_leave();
    return ret;
}

MPP_RET hal_h265e_v541_get_task(void *hal, HalEncTask *task)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    MppFrame frame = task->frame;
    EncFrmStatus  *frm_status = &task->rc_task->frm;

    hal_h265e_enter();

    if (vepu54x_h265_setup_hal_bufs(ctx)) {
        hal_h265e_err("vepu541_h265_allocate_buffers failed, free buffers and return\n");
        task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
        return MPP_ERR_MALLOC;
    }

    if (!frm_status->reencode)
        ctx->last_frame_type = ctx->frame_type;

    if (frm_status->is_intra) {
        ctx->frame_type = INTRA_FRAME;
    } else {
        ctx->frame_type = INTER_P_FRAME;
    }
    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(frame);

        mpp_meta_get_ptr_d(meta, KEY_ROI_DATA, (void **)&ctx->roi_data, NULL);
        mpp_meta_get_ptr_d(meta, KEY_ROI_DATA2, (void **)&ctx->roi_data2, NULL);
        mpp_meta_get_ptr_d(meta, KEY_OSD_DATA, (void **)&ctx->osd_cfg.osd_data, NULL);
        mpp_meta_get_ptr_d(meta, KEY_OSD_DATA2, (void **)&ctx->osd_cfg.osd_data2, NULL);
        mpp_meta_get_buffer_d(meta, KEY_QPMAP0, &ctx->qpmap, NULL);
    }
    memset(&ctx->feedback, 0, sizeof(vepu541_h265_fbk));

    hal_h265e_leave();
    return MPP_OK;
}

MPP_RET hal_h265e_v541_ret_task(void *hal, HalEncTask *task)
{
    H265eV541HalContext *ctx = (H265eV541HalContext *)hal;
    HalEncTask *enc_task = task;
    vepu541_h265_fbk *fb = &ctx->feedback;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    RK_U32 offset = mpp_packet_get_length(enc_task->packet);

    hal_h265e_enter();

    vepu541_h265_set_feedback(ctx, enc_task);
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

    hal_h265e_dbg_detail("output stream size %d\n", fb->out_strm_size);

    hal_h265e_leave();
    return MPP_OK;
}

const MppEncHalApi hal_h265e_vepu541 = {
    "h265e_v541_v2",
    MPP_VIDEO_CodingHEVC,
    sizeof(H265eV541HalContext),
    0,
    hal_h265e_v541_init,
    hal_h265e_v541_deinit,
    hal_h265e_vepu54x_prepare,
    hal_h265e_v541_get_task,
    hal_h265e_v541_gen_regs,
    hal_h265e_v54x_start,
    hal_h265e_v541_wait,
    NULL,
    NULL,
    hal_h265e_v541_ret_task,
};

