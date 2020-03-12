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

#define MODULE_TAG "hal_h264e_vepu541"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_device_msg.h"
#include "mpp_device_patch.h"
#include "mpp_rc.h"

#include "hal_h264e_debug.h"
#include "h264e_syntax_new.h"
#include "h264e_slice.h"
#include "h264e_stream.h"

#include "hal_bufs.h"
#include "hal_h264e_vepu541.h"
#include "hal_h264e_vepu541_reg.h"
#include "hal_h264e_vepu541_reg_l2.h"
#include "vepu541_common.h"

typedef struct HalH264eVepu541Ctx_t {
    MppEncCfgSet            *cfg;

    MppDevCtx               dev_ctx;
    RegExtraInfo            dev_patch;
    RK_S32                  frame_cnt;

    /* buffers management */
    HalBufs                 hw_recn;
    RK_S32                  pixel_buf_fbc_hdr_size;
    RK_S32                  pixel_buf_fbc_bdy_size;
    RK_S32                  pixel_buf_size;
    RK_S32                  thumb_buf_size;

    /* syntax for input from enc_impl */
    RK_U32                  updated;
    SynH264eSps             *sps;
    SynH264ePps             *pps;
    H264eSlice              *slice;
    H264eFrmInfo            *frms;
    H264eReorderInfo        *reorder;
    H264eMarkingInfo        *marking;
    RcSyntax                *rc_syn;

    /* syntax for output to enc_impl */
    EncRcTaskInfo                hal_rc_cfg;

    /* roi */
    MppEncROICfg            *roi_data;
    MppBufferGroup          roi_grp;
    MppBuffer               roi_buf;
    RK_S32                  roi_buf_size;

    /* osd */
    RK_U32                  plt_type;
    MppEncOSDPlt            *osd_plt;
    MppEncOSDData           *osd_data;

    /* register */
    Vepu541H264eRegSet      regs_set;
    Vepu541H264eRegL2Set    regs_l2_set;
    Vepu541H264eRegRet      regs_ret;
} HalH264eVepu541Ctx;

static RK_U32 h264e_klut_weight_i[24] = {
    0x0a000010, 0x00064000, 0x14000020, 0x000c8000,
    0x28000040, 0x00194000, 0x50800080, 0x0032c000,
    0xa1000100, 0x00658000, 0x42800200, 0x00cb0001,
    0x85000400, 0x01964002, 0x0a000800, 0x032c8005,
    0x14001000, 0x0659400a, 0x28802000, 0x0cb2c014,
    0x51004000, 0x1965c028, 0xa2808000, 0x00000050,
};

static RK_U32 h264e_klut_weight_p[24] = {
    0x28000040, 0x00194000, 0x50800080, 0x0032c000,
    0xa1000100, 0x00658000, 0x42800200, 0x00cb0001,
    0x85000400, 0x01964002, 0x0a000800, 0x032c8005,
    0x14001000, 0x0659400a, 0x28802000, 0x0cb2c014,
    0x51004000, 0x1965c028, 0xa2808000, 0x32cbc050,
    0x4500ffff, 0x659780a1, 0x8a81fffe, 0x00000142,
};

static RK_U32 dump_l1_reg = 0;
static RK_U32 dump_l2_reg = 0;

static MPP_RET hal_h264e_vepu541_deinit(void *hal)
{
    HalH264eVepu541Ctx *p = (HalH264eVepu541Ctx *)hal;

    hal_h264e_dbg_func("enter %p\n", p);

    if (p->dev_ctx) {
        mpp_device_deinit(p->dev_ctx);
        p->dev_ctx = NULL;
    }

    if (p->hw_recn) {
        hal_bufs_deinit(p->hw_recn);
        p->hw_recn = NULL;
    }

    hal_h264e_dbg_func("leave %p\n", p);

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu541_init(void *hal, MppEncHalCfg *cfg)
{
    HalH264eVepu541Ctx *p = (HalH264eVepu541Ctx *)hal;
    MPP_RET ret = MPP_OK;
    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,            /* type */
        .coding = MPP_VIDEO_CodingAVC,  /* coding */
        .platform = HAVE_RKVENC,        /* platform */
        .pp_enable = 0,                 /* pp_enable */
    };

    hal_h264e_dbg_func("enter %p\n", p);

    p->cfg = cfg->cfg;

    ret = mpp_device_init(&p->dev_ctx, &dev_cfg);
    if (ret) {
        mpp_err_f("mpp_device_init failed ret: %d\n", ret);
        goto DONE;
    }

    ret = hal_bufs_init(&p->hw_recn);
    if (ret) {
        mpp_err_f("init vepu buffer failed ret: %d\n", ret);
        goto DONE;
    }

DONE:
    if (ret)
        hal_h264e_vepu541_deinit(hal);

    hal_h264e_dbg_func("leave %p\n", p);
    return ret;
}

static RK_U32 update_vepu541_syntax(HalH264eVepu541Ctx *ctx, MppSyntax *syntax)
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
        case H264E_SYN_SLICE : {
            hal_h264e_dbg_detail("update slice");
            ctx->slice = desc->p;
        } break;
        case H264E_SYN_FRAME : {
            hal_h264e_dbg_detail("update frames");
            ctx->frms = desc->p;
        } break;
        case H264E_SYN_RC : {
            hal_h264e_dbg_detail("update rc");
            ctx->rc_syn = desc->p;
        } break;
        case H264E_SYN_OSD_PLT : {
            hal_h264e_dbg_detail("update osd plt");
            ctx->osd_plt = desc->p;
        } break;
        case H264E_SYN_OSD : {
            hal_h264e_dbg_detail("update osd data");
            ctx->osd_data = desc->p;
        } break;
        case H264E_SYN_ROI : {
            hal_h264e_dbg_detail("update roi");
            ctx->roi_data = desc->p;
        } break;
        default : {
            mpp_log_f("invalid syntax type %d\n", desc->type);
        } break;
        }

        updated |= SYN_TYPE_FLAG(desc->type);
    }

    return updated;
}

static MPP_RET hal_h264e_vepu541_get_task(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    RK_U32 updated = update_vepu541_syntax(ctx, &task->syntax);
    MppEncPrepCfg *prep = &ctx->cfg->prep;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (updated & SYN_TYPE_FLAG(H264E_SYN_CFG)) {
        RK_S32 alignment = 16;
        RK_S32 aligned_w = MPP_ALIGN(prep->width,  alignment);
        RK_S32 aligned_h = MPP_ALIGN(prep->height, alignment);
        RK_S32 pixel_buf_fbc_hdr_size = MPP_ALIGN(aligned_w * aligned_h / 64, SZ_8K);
        RK_S32 pixel_buf_fbc_bdy_size = aligned_w * aligned_h * 2;
        RK_S32 pixel_buf_size = pixel_buf_fbc_hdr_size + pixel_buf_fbc_bdy_size;
        RK_S32 thumb_buf_size = MPP_ALIGN(aligned_w / 64 * aligned_w / 64 * 256, SZ_8K);

        if ((ctx->pixel_buf_fbc_hdr_size != pixel_buf_fbc_hdr_size) ||
            (ctx->pixel_buf_fbc_bdy_size != pixel_buf_fbc_bdy_size) ||
            (ctx->pixel_buf_size != pixel_buf_size) ||
            (ctx->thumb_buf_size != thumb_buf_size)) {
            size_t sizes[2];
            RK_S32 max_cnt = ctx->sps->num_ref_frames + 1;

            /* pixel buffer */
            sizes[0] = pixel_buf_size;
            /* thumb buffer */
            sizes[1] = thumb_buf_size;

            hal_bufs_setup(ctx->hw_recn, max_cnt, 2, sizes);

            ctx->pixel_buf_fbc_hdr_size = pixel_buf_fbc_hdr_size;
            ctx->pixel_buf_fbc_bdy_size = pixel_buf_fbc_bdy_size;
            ctx->pixel_buf_size = pixel_buf_size;
            ctx->thumb_buf_size = thumb_buf_size;
        }
    }

    ctx->roi_data = NULL;
    ctx->osd_data = NULL;

    MppMeta meta = mpp_frame_get_meta(task->frame);
    mpp_meta_get_ptr(meta, KEY_ROI_DATA, (void **)&ctx->roi_data);
    mpp_meta_get_ptr(meta, KEY_OSD_DATA, (void **)&ctx->osd_data);

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static void setup_vepu541_normal(Vepu541H264eRegSet *regs)
{
    hal_h264e_dbg_func("enter\n");
    /* reg000 VERSION is read only */

    /* reg001 ENC_STRT */
    regs->reg001.lkt_num            = 0;
    regs->reg001.rkvenc_cmd         = 1;
    regs->reg001.clk_gate_en        = 1;

    /* reg002 ENC_CLR */
    regs->reg002.safe_clr           = 0;
    regs->reg002.force_clr          = 0;

    /* reg003 LKT_ADDR */
    regs->reg003.lkt_addr           = 0;

    /* reg004 INT_EN */
    regs->reg004.enc_done_en        = 1;
    regs->reg004.lkt_done_en        = 1;
    regs->reg004.sclr_done_en       = 1;
    regs->reg004.enc_slice_done_en  = 1;
    regs->reg004.oflw_done_en       = 1;
    regs->reg004.brsp_done_en       = 1;
    regs->reg004.berr_done_en       = 1;
    regs->reg004.rerr_done_en       = 1;
    regs->reg004.wdg_done_en        = 1;

    /* reg005 INT_MSK */
    regs->reg005.enc_done_msk       = 0;
    regs->reg005.lkt_done_msk       = 0;
    regs->reg005.sclr_done_msk      = 0;
    regs->reg005.enc_slice_done_msk = 0;
    regs->reg005.oflw_done_msk      = 0;
    regs->reg005.brsp_done_msk      = 0;
    regs->reg005.berr_done_msk      = 0;
    regs->reg005.rerr_done_msk      = 0;
    regs->reg005.wdg_done_msk       = 0;

    /* reg006 INT_CLR is not set */
    /* reg007 INT_STA is read only */
    /* reg008 ~ reg0011 gap */
    regs->reg014.vs_load_thd        = 0;
    regs->reg014.rfp_load_thrd      = 0;

    /* reg015 DTRNS_MAP */
    regs->reg015.cmvw_bus_ordr      = 0;
    regs->reg015.dspw_bus_ordr      = 0;
    regs->reg015.rfpw_bus_ordr      = 0;
    regs->reg015.src_bus_edin       = 0;
    regs->reg015.meiw_bus_edin      = 0;
    regs->reg015.bsw_bus_edin       = 7;
    regs->reg015.lktr_bus_edin      = 0;
    regs->reg015.roir_bus_edin      = 0;
    regs->reg015.lktw_bus_edin      = 0;
    regs->reg015.afbc_bsize         = 1;

    /* reg016 DTRNS_CFG */
    regs->reg016.axi_brsp_cke       = 0;
    regs->reg016.dspr_otsd          = 0;

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET setup_vepu541_prep(Vepu541H264eRegSet *regs, MppEncPrepCfg *prep)
{
    VepuFmtCfg cfg;
    MPP_RET ret = vepu541_set_fmt(&cfg, prep);
    RK_U32 hw_fmt = cfg.format;
    RK_S32 y_stride;
    RK_S32 c_stride;

    hal_h264e_dbg_func("enter\n");

    /* reg012 ENC_RSL */
    regs->reg012.pic_wd8_m1 = MPP_ALIGN(prep->width, 8) / 8 - 1;
    regs->reg012.pic_wfill  = prep->width & 0x7;
    regs->reg012.pic_hd8_m1 = MPP_ALIGN(prep->height, 8) / 8 - 1;
    regs->reg012.pic_hfill  = prep->height & 0x7;

    /* reg022 SRC_PROC */
    regs->reg017.src_cfmt   = hw_fmt;
    regs->reg017.alpha_swap = cfg.alpha_swap;
    regs->reg017.rbuv_swap  = cfg.rbuv_swap;
    regs->reg017.src_range  = cfg.src_range;

    y_stride = (prep->hor_stride) ? (prep->hor_stride) : (prep->width);

    switch (hw_fmt) {
    case VEPU541_FMT_BGRA8888 : {
        y_stride = y_stride * 4;
    } break;
    case VEPU541_FMT_BGR888 : {
        y_stride = y_stride * 3;
    } break;
    case VEPU541_FMT_BGR565 :
    case VEPU541_FMT_YUYV422 :
    case VEPU541_FMT_UYVY422 : {
        y_stride = y_stride * 2;
    } break;
    }
    c_stride = (hw_fmt == VEPU541_FMT_YUV422SP || hw_fmt == VEPU541_FMT_YUV420SP) ?
               y_stride : y_stride / 2;

    if (hw_fmt < VEPU541_FMT_NONE) {
        regs->reg018.csc_wgt_b2y    = 25;
        regs->reg018.csc_wgt_g2y    = 129;
        regs->reg018.csc_wgt_r2y    = 66;

        regs->reg019.csc_wgt_b2u    = 112;
        regs->reg019.csc_wgt_g2u    = -74;
        regs->reg019.csc_wgt_r2u    = -38;

        regs->reg020.csc_wgt_b2v    = -18;
        regs->reg020.csc_wgt_g2v    = -94;
        regs->reg020.csc_wgt_r2v    = 112;

        regs->reg021.csc_ofst_y     = 15;
        regs->reg021.csc_ofst_u     = 128;
        regs->reg021.csc_ofst_v     = 128;
    } else {
        regs->reg018.csc_wgt_b2y    = cfg.weight[0];
        regs->reg018.csc_wgt_g2y    = cfg.weight[1];
        regs->reg018.csc_wgt_r2y    = cfg.weight[2];

        regs->reg019.csc_wgt_b2u    = cfg.weight[3];
        regs->reg019.csc_wgt_g2u    = cfg.weight[4];
        regs->reg019.csc_wgt_r2u    = cfg.weight[5];

        regs->reg020.csc_wgt_b2v    = cfg.weight[6];
        regs->reg020.csc_wgt_g2v    = cfg.weight[7];
        regs->reg020.csc_wgt_r2v    = cfg.weight[8];

        regs->reg021.csc_ofst_y     = cfg.offset[0];
        regs->reg021.csc_ofst_u     = cfg.offset[1];
        regs->reg021.csc_ofst_v     = cfg.offset[2];
    }

    regs->reg022.src_mirr       = prep->mirroring > 0;
    regs->reg022.src_rot        = prep->rotation;
    regs->reg022.txa_en         = 1;
    regs->reg022.afbcd_en       = 0;

    regs->reg068.pic_ofst_y     = 0;
    regs->reg068.pic_ofst_x     = 0;

    regs->reg069.src_strd0      = y_stride;
    regs->reg069.src_strd1      = c_stride;

    hal_h264e_dbg_func("leave\n");

    return ret;
}

static void setup_vepu541_codec(Vepu541H264eRegSet *regs, SynH264eSps *sps,
                                SynH264ePps *pps, H264eSlice *slice)
{
    hal_h264e_dbg_func("enter\n");

    regs->reg013.enc_stnd       = 0;
    regs->reg013.cur_frm_ref    = slice->nal_reference_idc > 0;
    regs->reg013.bs_scp         = 1;
    regs->reg013.lamb_mod_sel   = (slice->slice_type == H264_I_SLICE) ? 0 : 1;
    regs->reg013.atr_thd_sel    = 0;
    regs->reg013.node_int       = 0;

    regs->reg103.nal_ref_idc    = slice->nal_reference_idc;
    regs->reg103.nal_unit_type  = slice->nalu_type;

    regs->reg104.max_fnum       = sps->log2_max_frame_num_minus4;
    regs->reg104.drct_8x8       = sps->direct8x8_inference;
    regs->reg104.mpoc_lm4       = sps->log2_max_poc_lsb_minus4;

    regs->reg105.etpy_mode      = pps->entropy_coding_mode;
    regs->reg105.trns_8x8       = pps->transform_8x8_mode;
    regs->reg105.csip_flag      = pps->constrained_intra_pred;
    regs->reg105.num_ref0_idx   = pps->num_ref_idx_l0_default_active - 1;
    regs->reg105.num_ref1_idx   = pps->num_ref_idx_l1_default_active - 1;
    regs->reg105.pic_init_qp    = pps->pic_init_qp;
    regs->reg105.cb_ofst        = pps->chroma_qp_index_offset;
    regs->reg105.cr_ofst        = pps->second_chroma_qp_index_offset;
    regs->reg105.wght_pred      = pps->weighted_pred;
    regs->reg105.dbf_cp_flg     = pps->deblocking_filter_control;

    regs->reg106.sli_type       = (slice->slice_type == H264_I_SLICE) ? (2) : (0);
    regs->reg106.pps_id         = slice->pic_parameter_set_id;
    regs->reg106.drct_smvp      = 0;
    regs->reg106.num_ref_ovrd   = slice->num_ref_idx_override;
    regs->reg106.cbc_init_idc   = slice->cabac_init_idc;
    regs->reg106.frm_num        = slice->frame_num;

    regs->reg107.idr_pic_id     = (slice->slice_type == H264_I_SLICE) ? slice->idr_pic_id : (RK_U32)(-1);
    regs->reg107.poc_lsb        = slice->pic_order_cnt_lsb;

    // TODO: reorder implement
    regs->reg108.ref_list0_rodr = 0; //slice->ref_pic_list_modification_flag;
    regs->reg108.rodr_pic_idx   = 0;
    regs->reg108.dis_dblk_idc   = slice->disable_deblocking_filter_idc;
    regs->reg108.sli_alph_ofst  = slice->slice_alpha_c0_offset_div2;
    regs->reg108.rodr_pic_num   = 0;

    if (slice->slice_type == H264_I_SLICE) {
        regs->reg109.nopp_flg       = slice->no_output_of_prior_pics;
        regs->reg109.ltrf_flg       = slice->long_term_reference_flag;
        regs->reg109.arpm_flg       = 0;
        regs->reg109.mmco4_pre      = 0;
        regs->reg109.mmco_type0     = 0;
        regs->reg109.mmco_parm0     = 0;
        regs->reg109.mmco_type1     = 0;
        regs->reg110.mmco_parm1     = 0;
        regs->reg109.mmco_type2     = 0;
        regs->reg110.mmco_parm2     = 0;
    } else {
        regs->reg109.nopp_flg       = 0;
        regs->reg109.ltrf_flg       = 0;
        regs->reg109.arpm_flg       = 1;
        regs->reg109.mmco4_pre      = 0;
        regs->reg109.mmco_type0     = 1;
        regs->reg109.mmco_parm0     = 0;
        regs->reg109.mmco_type1     = 1;
        regs->reg110.mmco_parm1     = 0;
        regs->reg109.mmco_type2     = 0;
        regs->reg110.mmco_parm2     = 0;
    }

    regs->reg114.long_term_frame_idx0   = 0;
    regs->reg114.long_term_frame_idx1   = 0;
    regs->reg114.long_term_frame_idx2   = 0;

    regs->reg115.dlt_poc_s0_m12 = 0;
    regs->reg115.dlt_poc_s0_m13 = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_rdo_pred(Vepu541H264eRegSet *regs, SynH264eSps *sps,
                                   SynH264ePps *pps, H264eSlice *slice)
{
    hal_h264e_dbg_func("enter\n");

    if (slice->slice_type == H264_I_SLICE) {
        regs->reg025.chrm_klut_ofst = 0;
        memcpy(&regs->reg026, h264e_klut_weight_i, sizeof(h264e_klut_weight_i));
    } else {
        regs->reg025.chrm_klut_ofst = 6;
        memcpy(&regs->reg026, h264e_klut_weight_p, sizeof(h264e_klut_weight_p));
    }

    regs->reg101.vthd_y         = 9;
    regs->reg101.vthd_c         = 63;

    regs->reg102.rect_size      = (sps->profile_idc == H264_PROFILE_BASELINE &&
                                   sps->level_idc <= H264_LEVEL_3_0) ? 1 : 0;
    regs->reg102.inter_4x4      = (sps->level_idc > H264_LEVEL_4_2) ? 0 : 1;
    regs->reg102.vlc_lmt        = (sps->profile_idc < H264_PROFILE_MAIN) &&
                                  !pps->entropy_coding_mode;
    regs->reg102.chrm_spcl      = 1;
    regs->reg102.rdo_mask       = 0;
    regs->reg102.ccwa_e         = 0;
    regs->reg102.scl_lst_sel    = pps->pic_scaling_matrix_present;
    regs->reg102.atr_e          = 1;
    regs->reg102.atf_edg        = 3;
    regs->reg102.atf_lvl_e      = 1;
    regs->reg102.atf_intra_e    = 1;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_rc_base(Vepu541H264eRegSet *regs, SynH264eSps *sps,
                                  EncRcTask *rc_task)
{
    EncRcTaskInfo *rc_info = &rc_task->info;
    RK_U32 qp_target = rc_info->quality_target;
    RK_U32 qp_min = rc_info->quality_max;
    RK_U32 qp_max = rc_info->quality_min;
    RK_U32 qpmap_mode = 1;

    hal_h264e_dbg_func("enter\n");

    regs->reg013.pic_qp         = qp_target;

    regs->reg050.rc_en          = 0;
    regs->reg050.aq_en          = 0;
    regs->reg050.aq_mode        = 0;
    regs->reg050.rc_ctu_num     = sps->pic_width_in_mbs;

    regs->reg051.rc_qp_range    = 0;
    regs->reg051.rc_max_qp      = qp_max;
    regs->reg051.rc_min_qp      = qp_min;

    regs->reg052.ctu_ebit       = 0;

    regs->reg053.qp_adj0        = 0;
    regs->reg053.qp_adj1        = 0;
    regs->reg053.qp_adj2        = 0;
    regs->reg053.qp_adj3        = 0;
    regs->reg053.qp_adj4        = 0;
    regs->reg054.qp_adj5        = 0;
    regs->reg054.qp_adj6        = 0;
    regs->reg054.qp_adj7        = 0;
    regs->reg054.qp_adj8        = 0;

    regs->reg055_063.rc_dthd[0] = 0;
    regs->reg055_063.rc_dthd[1] = 0;
    regs->reg055_063.rc_dthd[2] = 0;
    regs->reg055_063.rc_dthd[3] = 0;
    regs->reg055_063.rc_dthd[4] = 0;
    regs->reg055_063.rc_dthd[5] = 0;
    regs->reg055_063.rc_dthd[6] = 0;
    regs->reg055_063.rc_dthd[7] = 0;
    regs->reg055_063.rc_dthd[8] = 0;

    regs->reg064.qpmin_area0    = 21;
    regs->reg064.qpmax_area0    = 50;
    regs->reg064.qpmin_area1    = 20;
    regs->reg064.qpmax_area1    = 50;
    regs->reg064.qpmin_area2    = 20;

    regs->reg065.qpmax_area2    = 50;
    regs->reg065.qpmin_area3    = 20;
    regs->reg065.qpmax_area3    = 50;
    regs->reg065.qpmin_area4    = 20;
    regs->reg065.qpmax_area4    = 50;

    regs->reg066.qpmin_area5    = 20;
    regs->reg066.qpmax_area5    = 50;
    regs->reg066.qpmin_area6    = 20;
    regs->reg066.qpmax_area6    = 50;
    regs->reg066.qpmin_area7    = 20;

    regs->reg067.qpmax_area7    = 49;
    regs->reg067.qpmap_mode     = qpmap_mode;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_orig(Vepu541H264eRegSet *regs, RegExtraInfo *info,
                               MppDevCtx dev, MppFrame frm)
{
    MppBuffer buf = mpp_frame_get_buffer(frm);
    MppFrameFormat fmt = mpp_frame_get_fmt(frm);
    RK_S32 hor_stride = mpp_frame_get_hor_stride(frm);
    RK_S32 ver_stride = mpp_frame_get_ver_stride(frm);
    RK_S32 fd = mpp_buffer_get_fd(buf);
    RK_U32 offset[2] = {0};

    hal_h264e_dbg_func("enter\n");

    regs->reg070.adr_src0 = fd;
    regs->reg071.adr_src1 = fd;
    regs->reg072.adr_src2 = fd;

    // Use new request to send the offset info
    if (MPP_FRAME_FMT_IS_YUV(fmt)) {
        if (fmt == MPP_FMT_YUV420SP || fmt == MPP_FMT_YUV422SP) {
            offset[0] = hor_stride * ver_stride;
            offset[1] = hor_stride * ver_stride;
        } else if (fmt == MPP_FMT_YUV420P) {
            offset[0] = hor_stride * ver_stride;
            offset[1] = hor_stride * ver_stride * 5 / 4;
        } else if (fmt == MPP_FMT_YUV422P) {
            offset[0] = hor_stride * ver_stride;
            offset[1] = hor_stride * ver_stride * 3 / 2;
        } else {
            mpp_err_f("unsupported yuv format %x\n", fmt);
        }
    }

    mpp_device_patch_init(info);
    mpp_device_patch_add((RK_U32 *)regs, info, 71, offset[0]);
    mpp_device_patch_add((RK_U32 *)regs, info, 72, offset[1]);
    mpp_device_send_extra_info(dev, info);

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_roi(Vepu541H264eRegSet *regs, HalH264eVepu541Ctx *ctx)
{
    MppEncROICfg *roi = ctx->roi_data;
    RK_U32 w = ctx->sps->pic_width_in_mbs * 16;
    RK_U32 h = ctx->sps->pic_height_in_mbs * 16;

    hal_h264e_dbg_func("enter\n");

    /* roi setup */
    if (roi && roi->number && roi->regions) {
        RK_S32 roi_buf_size = vepu541_get_roi_buf_size(w, h);

        if (!ctx->roi_buf || roi_buf_size != ctx->roi_buf_size) {
            if (NULL == ctx->roi_grp)
                mpp_buffer_group_get_internal(&ctx->roi_grp, MPP_BUFFER_TYPE_ION);
            else if (roi_buf_size != ctx->roi_buf_size) {
                if (ctx->roi_buf) {
                    mpp_buffer_put(ctx->roi_buf);
                    ctx->roi_buf = NULL;
                }
                mpp_buffer_group_clear(ctx->roi_grp);
            }

            mpp_assert(ctx->roi_grp);

            if (NULL == ctx->roi_buf)
                mpp_buffer_get(ctx->roi_grp, &ctx->roi_buf, roi_buf_size);

            ctx->roi_buf_size = roi_buf_size;
        }

        mpp_assert(ctx->roi_buf);
        RK_S32 fd = mpp_buffer_get_fd(ctx->roi_buf);
        void *buf = mpp_buffer_get_ptr(ctx->roi_buf);

        regs->reg013.roi_enc = 1;
        regs->reg073.roi_addr = fd;

        vepu541_set_roi(buf, roi, w, h);
    } else {
        regs->reg013.roi_enc = 0;
        regs->reg073.roi_addr = 0;
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_recn_refr(Vepu541H264eRegSet *regs, H264eFrmInfo *frms,
                                    HalBufs bufs, RK_S32 fbc_hdr_size)
{
    HalBuf *curr = hal_bufs_get_buf(bufs, frms->curr_idx);
    HalBuf *refr = hal_bufs_get_buf(bufs, frms->refr_idx);

    hal_h264e_dbg_func("enter\n");

    if (curr && curr->cnt) {
        MppBuffer buf_pixel = curr->buf[0];
        MppBuffer buf_thumb = curr->buf[1];

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        regs->reg074.rfpw_h_addr = mpp_buffer_get_fd(buf_pixel);
        regs->reg075.rfpw_b_addr = regs->reg074.rfpw_h_addr + (fbc_hdr_size << 10);
        regs->reg080.dspw_addr = mpp_buffer_get_fd(buf_thumb);
    }

    if (refr && refr->cnt) {
        MppBuffer buf_pixel = refr->buf[0];
        MppBuffer buf_thumb = refr->buf[1];

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        regs->reg076.rfpr_h_addr = mpp_buffer_get_fd(buf_pixel);
        regs->reg077.rfpr_b_addr = regs->reg076.rfpr_h_addr + (fbc_hdr_size << 10);
        regs->reg081.dspr_addr = mpp_buffer_get_fd(buf_thumb);
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_output(Vepu541H264eRegSet *regs, MppBuffer out)
{
    size_t size = mpp_buffer_get_size(out);
    RK_S32 fd = mpp_buffer_get_fd(out);

    hal_h264e_dbg_func("enter\n");

    regs->reg083.bsbt_addr = fd + (size << 10);
    regs->reg084.bsbb_addr = fd;
    regs->reg085.bsbr_addr = fd;
    regs->reg086.adr_bsbs  = fd;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_split(Vepu541H264eRegSet *regs, MppEncSliceSplit *cfg)
{
    hal_h264e_dbg_func("enter\n");

    switch (cfg->split_mode) {
    case MPP_ENC_SPLIT_NONE : {
        regs->reg087.sli_splt = 0;
        regs->reg087.sli_splt_mode = 0;
        regs->reg087.sli_splt_cpst = 0;
        regs->reg087.sli_max_num_m1 = 0;
        regs->reg087.sli_flsh = 0;
        regs->reg087.sli_splt_cnum_m1 = 0;

        regs->reg088.sli_splt_byte = 0;
        regs->reg013.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_BYTE : {
        regs->reg087.sli_splt = 1;
        regs->reg087.sli_splt_mode = 0;
        regs->reg087.sli_splt_cpst = 0;
        regs->reg087.sli_max_num_m1 = 500;
        regs->reg087.sli_flsh = 1;
        regs->reg087.sli_splt_cnum_m1 = 0;

        regs->reg088.sli_splt_byte = cfg->split_arg;
        regs->reg013.slen_fifo = 0;
    } break;
    case MPP_ENC_SPLIT_BY_CTU : {
        regs->reg087.sli_splt = 1;
        regs->reg087.sli_splt_mode = 1;
        regs->reg087.sli_splt_cpst = 0;
        regs->reg087.sli_max_num_m1 = 500;
        regs->reg087.sli_flsh = 1;
        regs->reg087.sli_splt_cnum_m1 = cfg->split_arg - 1;

        regs->reg088.sli_splt_byte = 0;
        regs->reg013.slen_fifo = 0;
    } break;
    default : {
        mpp_log_f("invalide slice split mode %d\n", cfg->split_mode);
    } break;
    }

    cfg->change = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_me(Vepu541H264eRegSet *regs, SynH264eSps *sps,
                             H264eSlice *slice)
{
    RK_S32 level_idc = sps->level_idc;
    RK_S32 pic_w = sps->pic_width_in_mbs * 16;
    RK_S32 pic_wd64 = (pic_w + 63) / 64;
    RK_S32 cime_w = 176;
    RK_S32 cime_h = 122;
    RK_S32 cime_blk_w_max = 44;
    RK_S32 cime_blk_h_max = 28;

    hal_h264e_dbg_func("enter\n");
    /*
     * Step 1. limit the mv range by level_idc
     * For level 1 and level 1b the vertical MV range is [-64,+63.75]
     * For level 1.1, 1.2, 1.3 and 2 the vertical MV range is [-128,+127.75]
     */
    switch (level_idc) {
    case H264_LEVEL_1_0 :
    case H264_LEVEL_1_b : {
        cime_blk_h_max = 12;
    } break;
    case H264_LEVEL_1_1 :
    case H264_LEVEL_1_2 :
    case H264_LEVEL_1_3 :
    case H264_LEVEL_2_0 : {
        cime_blk_h_max = 28;
    } break;
    default : {
        cime_blk_h_max = 28;
    } break;
    }

    if (cime_w < cime_blk_w_max * 4)
        cime_blk_w_max = cime_w / 4;

    if (cime_h < cime_blk_h_max * 4)
        cime_blk_h_max = cime_h / 4;

    /*
     * Step 2. limit the mv range by image size
     */
    if (cime_blk_w_max / 4 * 2 > (sps->pic_width_in_mbs * 2 + 1) / 2)
        cime_blk_w_max = (sps->pic_width_in_mbs * 2 + 1) / 2 / 2 * 4;

    if (cime_blk_h_max / 4 > MPP_ALIGN(sps->pic_height_in_mbs * 16, 64) / 128 * 4)
        cime_blk_h_max = MPP_ALIGN(sps->pic_height_in_mbs * 16, 64) / 128 * 16;

    regs->reg089.cme_srch_h     = cime_blk_w_max / 4;
    regs->reg089.cme_srch_v     = cime_blk_h_max / 4;
    regs->reg089.rme_srch_h     = 7;
    regs->reg089.rme_srch_v     = 5;
    regs->reg089.dlt_frm_num    = 0;

    if (slice->slice_type == H264_I_SLICE) {
        regs->reg090.pmv_mdst_h = 0;
        regs->reg090.pmv_mdst_v = 0;
    } else {
        regs->reg090.pmv_mdst_h = 5;
        regs->reg090.pmv_mdst_v = 0;
    }
    regs->reg090.mv_limit       = 2;
    regs->reg090.pmv_num        = 2;

    if (pic_w > 3584) {
        regs->reg091.cme_rama_h = 8;
    } else if (pic_w > 3136) {
        regs->reg091.cme_rama_h = 9;
    } else if (pic_w > 2816) {
        regs->reg091.cme_rama_h = 10;
    } else if (pic_w > 2560) {
        regs->reg091.cme_rama_h = 11;
    } else if (pic_w > 2368) {
        regs->reg091.cme_rama_h = 12;
    } else if (pic_w > 2176) {
        regs->reg091.cme_rama_h = 13;
    } else if (pic_w > 2048) {
        regs->reg091.cme_rama_h = 14;
    } else if (pic_w > 1856) {
        regs->reg091.cme_rama_h = 15;
    } else if (pic_w > 1792) {
        regs->reg091.cme_rama_h = 16;
    } else {
        regs->reg091.cme_rama_h = 17;
    }

    {
        RK_S32 swin_all_4_ver = 2 * regs->reg089.cme_srch_v + 1;
        RK_S32 swin_all_16_hor = (regs->reg089.cme_srch_h * 4 + 15) / 16 * 2 + 1;

        if (swin_all_4_ver < regs->reg091.cme_rama_h)
            regs->reg091.cme_rama_max = (swin_all_4_ver - 1) * pic_wd64 + swin_all_16_hor;
        else
            regs->reg091.cme_rama_max = (regs->reg091.cme_rama_h - 1) * pic_wd64 + swin_all_16_hor;
    }

    pic_wd64 = pic_wd64 << 6;

    if (pic_wd64 <= 512)
        regs->reg091.cach_l2_map = 0x0;
    else if (pic_wd64 <= 1024)
        regs->reg091.cach_l2_map = 0x1;
    else if (pic_wd64 <= 2048)
        regs->reg091.cach_l2_map = 0x2;
    else if (pic_wd64 <= 4096)
        regs->reg091.cach_l2_map = 0x3;
    else
        regs->reg091.cach_l2_map = 0x0;

    regs->reg091.cach_l2_map = 0x0;

    hal_h264e_dbg_func("leave\n");
}

static RK_U32 h264e_wgt_qp_grpa_default[52] = {
    0x0000000e, 0x00000012, 0x00000016, 0x0000001c,
    0x00000024, 0x0000002d, 0x00000039, 0x00000048,
    0x0000005b, 0x00000073, 0x00000091, 0x000000b6,
    0x000000e6, 0x00000122, 0x0000016d, 0x000001cc,
    0x00000244, 0x000002db, 0x00000399, 0x00000489,
    0x000005b6, 0x00000733, 0x00000912, 0x00000b6d,
    0x00000e66, 0x00001224, 0x000016db, 0x00001ccc,
    0x00002449, 0x00002db7, 0x00003999, 0x00004892,
    0x00005b6f, 0x00007333, 0x00009124, 0x0000b6de,
    0x0000e666, 0x00012249, 0x00016dbc, 0x0001cccc,
    0x00024492, 0x0002db79, 0x00039999, 0x00048924,
    0x0005b6f2, 0x00073333, 0x00091249, 0x000b6de5,
    0x000e6666, 0x00122492, 0x0016dbcb, 0x001ccccc,
};

static RK_U32 h264e_wgt_qp_grpb_default[52] = {
    0x0000000e, 0x00000012, 0x00000016, 0x0000001c,
    0x00000024, 0x0000002d, 0x00000039, 0x00000048,
    0x0000005b, 0x00000073, 0x00000091, 0x000000b6,
    0x000000e6, 0x00000122, 0x0000016d, 0x000001cc,
    0x00000244, 0x000002db, 0x00000399, 0x00000489,
    0x000005b6, 0x00000733, 0x00000912, 0x00000b6d,
    0x00000e66, 0x00001224, 0x000016db, 0x00001ccc,
    0x00002449, 0x00002db7, 0x00003999, 0x00004892,
    0x00005b6f, 0x00007333, 0x00009124, 0x0000b6de,
    0x0000e666, 0x00012249, 0x00016dbc, 0x0001cccc,
    0x00024492, 0x0002db79, 0x00039999, 0x00048924,
    0x0005b6f2, 0x00073333, 0x00091249, 0x000b6de5,
    0x000e6666, 0x00122492, 0x0016dbcb, 0x001ccccc,
};

static RK_U8 h264_aq_tthd_default[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 35,
};

static RK_S8 h264_aq_step_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    4,  5,  7, 10,
};

static void setup_vepu541_l2(Vepu541H264eRegL2Set *regs, H264eSlice *slice)
{
    RK_U32 i;

    hal_h264e_dbg_func("enter\n");

    memset(regs, 0, sizeof(*regs));

    regs->iprd_tthdy4[0] = 1;
    regs->iprd_tthdy4[1] = 2;
    regs->iprd_tthdy4[2] = 4;
    regs->iprd_tthdy4[3] = 16;

    regs->iprd_tthdc8[0] = 1;
    regs->iprd_tthdc8[1] = 2;
    regs->iprd_tthdc8[2] = 4;
    regs->iprd_tthdc8[3] = 16;

    regs->iprd_tthdy8[0] = 1;
    regs->iprd_tthdy8[1] = 2;
    regs->iprd_tthdy8[2] = 4;
    regs->iprd_tthdy8[3] = 16;

    regs->iprd_tthd_ul = 0x24;

    regs->iprd_wgty8[0] = 0x30;
    regs->iprd_wgty8[1] = 0x3c;
    regs->iprd_wgty8[2] = 0x28;
    regs->iprd_wgty8[3] = 0x30;

    regs->iprd_wgty4[0] = 0x30;
    regs->iprd_wgty4[1] = 0x3c;
    regs->iprd_wgty4[2] = 0x28;
    regs->iprd_wgty4[3] = 0x30;

    regs->iprd_wgty16[0] = 0x30;
    regs->iprd_wgty16[1] = 0x3c;
    regs->iprd_wgty16[2] = 0x28;
    regs->iprd_wgty16[3] = 0x30;

    regs->iprd_wgtc8[0] = 0x24;
    regs->iprd_wgtc8[1] = 0x2a;
    regs->iprd_wgtc8[2] = 0x1c;
    regs->iprd_wgtc8[3] = 0x20;

    /* 000556ab */
    regs->qnt_bias_comb.qnt_bias_i = 683;
    regs->qnt_bias_comb.qnt_bias_p = 341;

    regs->atr_thd0_h264.atr_thd0 = 1;
    regs->atr_thd0_h264.atr_thd1 = 4;

    if (slice->slice_type == H264_I_SLICE)
        regs->atr_thd1_h264.atr_thd2 = 36;
    else
        regs->atr_thd1_h264.atr_thd2 = 49;

    regs->atr_thd1_h264.atr_qp = 45;

    regs->atr_wgt16_h264.atr_lv16_wgt0 = 16;
    regs->atr_wgt16_h264.atr_lv16_wgt1 = 16;
    regs->atr_wgt16_h264.atr_lv16_wgt2 = 16;

    regs->atr_wgt8_h264.atr_lv8_wgt0 = 32;
    regs->atr_wgt8_h264.atr_lv8_wgt1 = 32;
    regs->atr_wgt8_h264.atr_lv8_wgt2 = 32;

    regs->atr_wgt4_h264.atr_lv4_wgt0 = 20;
    regs->atr_wgt4_h264.atr_lv4_wgt1 = 18;
    regs->atr_wgt4_h264.atr_lv4_wgt2 = 16;

    regs->atf_tthd[0] = 0;
    regs->atf_tthd[1] = 25;
    regs->atf_tthd[2] = 100;
    regs->atf_tthd[3] = 169;

    regs->atf_sthd0_h264.atf_sthd_10 = 30;
    regs->atf_sthd0_h264.atf_sthd_max = 60;

    regs->atf_sthd1_h264.atf_sthd_11 = 40;
    regs->atf_sthd1_h264.atf_sthd_20 = 30;

    regs->atf_wgt0_h264.atf_wgt10 = 23;
    regs->atf_wgt0_h264.atf_wgt11 = 22;

    regs->atf_wgt1_h264.atf_wgt12 = 20;
    regs->atf_wgt1_h264.atf_wgt20 = 20;

    regs->atf_wgt2_h264.atf_wgt21 = 20;
    regs->atf_wgt2_h264.atf_wgt30 = 22;

    regs->atf_ofst0_h264.atf_ofst10 = 3500;
    regs->atf_ofst0_h264.atf_ofst11 = 3500;

    regs->atf_ofst1_h264.atf_ofst12 = 0;
    regs->atf_ofst1_h264.atf_ofst20 = 3500;

    regs->atf_ofst2_h264.atf_ofst21 = 1000;
    regs->atf_ofst2_h264.atf_ofst30 = 0;

    regs->iprd_wgt_qp[0]  = 0;
    /* ~ */
    regs->iprd_wgt_qp[51] = 0;

    memcpy(regs->wgt_qp_grpa, h264e_wgt_qp_grpa_default, sizeof(regs->wgt_qp_grpa));
    memcpy(regs->wgt_qp_grpb, h264e_wgt_qp_grpb_default, sizeof(regs->wgt_qp_grpb));

    regs->madi_mode = 0;

    memcpy(regs->aq_tthd, h264_aq_tthd_default, sizeof(regs->aq_tthd));

    for (i = 0; i < MPP_ARRAY_ELEMS(regs->aq_step); i++)
        regs->aq_step[i] = h264_aq_step_default[i] & 0x3f;

    regs->rme_mvd_penalty.mvd_pnlt_e = 0;
    regs->rme_mvd_penalty.mvd_pnlt_coef = 16;
    regs->rme_mvd_penalty.mvd_pnlt_cnst = 1024;
    regs->rme_mvd_penalty.mvd_pnlt_lthd = 12;
    regs->rme_mvd_penalty.mvd_pnlt_hthd = 12;

    regs->atr1_thd0_h264.atr1_thd0 = 1;
    regs->atr1_thd0_h264.atr1_thd1 = 4;
    regs->atr1_thd1_h264.atr1_thd2 = 49;

    mpp_env_get_u32("dump_l2_reg", &dump_l2_reg, 0);

    if (dump_l2_reg) {
        mpp_log("L2 reg dump start:\n");
        RK_U32 *p = (RK_U32 *)regs;

        for (i = 0; i < (sizeof(*regs) / sizeof(RK_U32)); i++)
            mpp_log("%04x %08x\n", 4 + i * 4, p[i]);

        mpp_log("L2 reg done\n");
    }

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET hal_h264e_vepu541_gen_regs(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    Vepu541H264eRegSet *regs = &ctx->regs_set;
    MppEncCfgSet *cfg = ctx->cfg;
    SynH264eSps *sps = ctx->sps;
    SynH264ePps *pps = ctx->pps;
    H264eSlice *slice = ctx->slice;

    hal_h264e_dbg_func("enter %p\n", hal);
    hal_h264e_dbg_detail("frame %d generate regs now", ctx->frms->seq_idx);

    /* register setup */
    memset(regs, 0, sizeof(*regs));

    setup_vepu541_normal(regs);
    setup_vepu541_prep(regs, &ctx->cfg->prep);
    setup_vepu541_codec(regs, sps, pps, slice);
    setup_vepu541_rdo_pred(regs, sps, pps, slice);
    setup_vepu541_rc_base(regs, sps, task->rc_task);
    setup_vepu541_orig(regs, ctx->dev_ctx, &ctx->dev_patch, task->frame);
    setup_vepu541_roi(regs, ctx);
    setup_vepu541_recn_refr(regs, ctx->frms, ctx->hw_recn,
                            ctx->pixel_buf_fbc_hdr_size);

    regs->reg082.meiw_addr = task->mv_info ? mpp_buffer_get_fd(task->mv_info) : 0;

    setup_vepu541_output(regs, task->output);
    setup_vepu541_split(regs, &cfg->split);
    setup_vepu541_me(regs, sps, slice);
    vepu541_set_osd_region(regs, ctx->dev_ctx, ctx->osd_data, ctx->osd_plt);
    setup_vepu541_l2(&ctx->regs_l2_set, slice);

    mpp_env_get_u32("dump_l1_reg", &dump_l1_reg, 0);

    if (dump_l1_reg) {
        mpp_log("L1 reg dump start:\n");
        RK_U32 *p = (RK_U32 *)regs;
        RK_S32 n = 0x1D0 / sizeof(RK_U32);
        RK_S32 i;

        for (i = 0; i < n; i++)
            mpp_log("%04x %08x\n", i * 4, p[i]);

        mpp_log("L1 reg done\n");
    }

    ctx->frame_cnt++;

    hal_h264e_dbg_func("leave %p\n", hal);
    return MPP_OK;
}

static MPP_RET hal_h264e_vepu541_start(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    MppDevReqV1 req;

    memset(&req, 0, sizeof(req));

    (void) task;

    hal_h264e_dbg_func("enter %p\n", hal);

    /* write L2 registers */
    req.cmd = MPP_CMD_SET_REG_WRITE;
    req.flag = 0;
    req.offset = VEPU541_REG_BASE_L2;
    req.size = sizeof(Vepu541H264eRegL2Set);
    req.data = &ctx->regs_l2_set;
    mpp_device_add_request(ctx->dev_ctx, &req);

    /* write L1 registers */
    req.cmd = MPP_CMD_SET_REG_WRITE;
    req.size = sizeof(Vepu541H264eRegSet);
    req.offset = 0;
    req.data = &ctx->regs_set;
    mpp_device_add_request(ctx->dev_ctx, &req);

    /* write extra info for address registers */

    /* set output request */
    memset(&req, 0, sizeof(req));
    req.flag = 0;
    req.cmd = MPP_CMD_SET_REG_READ;
    req.size = sizeof(RK_U32);
    req.data = &ctx->regs_ret.hw_status;
    req.offset = VEPU541_REG_BASE_HW_STATUS;
    mpp_device_add_request(ctx->dev_ctx, &req);

    memset(&req, 0, sizeof(req));
    req.flag = 0;
    req.cmd = MPP_CMD_SET_REG_READ;
    req.size = sizeof(ctx->regs_ret) - 4;
    req.data = &ctx->regs_ret.st_bsl;
    req.offset = VEPU541_REG_BASE_STATISTICS;
    mpp_device_add_request(ctx->dev_ctx, &req);
    /* send request to hardware */
    mpp_device_send_request(ctx->dev_ctx);

    hal_h264e_dbg_func("leave %p\n", hal);

    return ret;
}

static MPP_RET hal_h264e_vepu541_wait(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    MppDevReqV1 req;

    (void) task;

    hal_h264e_dbg_func("enter %p\n", hal);

    memset(&req, 0, sizeof(req));
    req.cmd = MPP_CMD_POLL_HW_FINISH;
    mpp_device_add_request(ctx->dev_ctx, &req);

    mpp_device_send_request(ctx->dev_ctx);

    task->length = ctx->regs_ret.st_bsl.bs_lgth;

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu541_ret_task(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 mbs = mb_w * mb_h;

    hal_h264e_dbg_func("enter %p\n", hal);

    ctx->hal_rc_cfg.bit_real = task->length;
    ctx->hal_rc_cfg.quality_real = mbs;

    task->hal_ret.data   = &ctx->hal_rc_cfg;
    task->hal_ret.number = 1;

    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

const MppEncHalApi hal_h264e_vepu541 = {
    .name       = "hal_h264e_vepu541",
    .coding     = MPP_VIDEO_CodingAVC,
    .ctx_size   = sizeof(HalH264eVepu541Ctx),
    .flag       = 0,
    .init       = hal_h264e_vepu541_init,
    .deinit     = hal_h264e_vepu541_deinit,
    .get_task   = hal_h264e_vepu541_get_task,
    .gen_regs   = hal_h264e_vepu541_gen_regs,
    .start      = hal_h264e_vepu541_start,
    .wait       = hal_h264e_vepu541_wait,
    .ret_task   = hal_h264e_vepu541_ret_task,
};
