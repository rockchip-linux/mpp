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
#include "mpp_soc.h"
#include "mpp_frame.h"
#include "mpp_common.h"
#include "mpp_device.h"
#include "mpp_frame_impl.h"
#include "mpp_rc.h"

#include "hal_h264e_debug.h"
#include "h264e_sps.h"
#include "h264e_pps.h"
#include "h264e_slice.h"

#include "hal_bufs.h"
#include "hal_h264e_vepu541.h"
#include "hal_h264e_vepu541_reg.h"
#include "hal_h264e_vepu541_reg_l2.h"
#include "vepu541_common.h"

typedef struct HalH264eVepu541Ctx_t {
    MppEncCfgSet            *cfg;

    MppDev                  dev;
    RK_U32                  is_vepu540;
    RK_S32                  frame_cnt;

    /* buffers management */
    HalBufs                 hw_recn;
    RK_S32                  pixel_buf_fbc_hdr_size;
    RK_S32                  pixel_buf_fbc_bdy_size;
    RK_S32                  pixel_buf_size;
    RK_S32                  thumb_buf_size;
    RK_S32                  max_buf_cnt;

    /* syntax for input from enc_impl */
    RK_U32                  updated;
    H264eSps                *sps;
    H264ePps                *pps;
    H264eSlice              *slice;
    H264eFrmInfo            *frms;
    H264eReorderInfo        *reorder;
    H264eMarkingInfo        *marking;
    H264ePrefixNal          *prefix;

    /* syntax for output to enc_impl */
    EncRcTaskInfo           hal_rc_cfg;

    /* roi */
    MppEncROICfg            *roi_data;
    MppEncROICfg2           *roi_data2;
    MppBufferGroup          roi_grp;
    MppBuffer               roi_buf;
    RK_S32                  roi_buf_size;
    MppBuffer               qpmap;

    /* osd */
    Vepu541OsdCfg           osd_cfg;

    /* register */
    Vepu541H264eRegSet      regs_set;
    Vepu541H264eRegL2Set    regs_l2_set;
    Vepu541H264eRegRet      regs_ret;
} HalH264eVepu541Ctx;

#define CHROMA_KLUT_TAB_SIZE    (24 * sizeof(RK_U32))

static RK_U32 h264e_klut_weight[30] = {
    0x0a000010, 0x00064000, 0x14000020, 0x000c8000,
    0x28000040, 0x00194000, 0x50800080, 0x0032c000,
    0xa1000100, 0x00658000, 0x42800200, 0x00cb0001,
    0x85000400, 0x01964002, 0x0a000800, 0x032c8005,
    0x14001000, 0x0659400a, 0x28802000, 0x0cb2c014,
    0x51004000, 0x1965c028, 0xa2808000, 0x32cbc050,
    0x4500ffff, 0x659780a1, 0x8a81fffe, 0xCC000142,
    0xFF83FFFF, 0x000001FF,
};

static RK_U32 dump_l1_reg = 0;
static RK_U32 dump_l2_reg = 0;

static RK_S32 h264_aq_tthd_default[16] = {
    0,  0,  0,  0,
    3,  3,  5,  5,
    8,  8,  8,  15,
    15, 20, 25, 25,
};

static RK_S32 h264_P_aq_step_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  2,  3,
    4,  5,  7,  8,
};

static RK_S32 h264_I_aq_step_default[16] = {
    -8, -7, -6, -5,
    -4, -3, -2, -1,
    0,  1,  3,  3,
    4,  5,  8,  8,
};

static MPP_RET hal_h264e_vepu541_deinit(void *hal)
{
    HalH264eVepu541Ctx *p = (HalH264eVepu541Ctx *)hal;

    hal_h264e_dbg_func("enter %p\n", p);

    if (p->dev) {
        mpp_dev_deinit(p->dev);
        p->dev = NULL;
    }

    if (p->roi_buf) {
        mpp_buffer_put(p->roi_buf);
        p->roi_buf = NULL;
    }

    if (p->roi_grp) {
        mpp_buffer_group_put(p->roi_grp);
        p->roi_grp = NULL;
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

    {
        const char *soc_name = mpp_get_soc_name();
        if (strstr(soc_name, "rk3566") || strstr(soc_name, "rk3568"))
            p->is_vepu540 = 1;
        else
            p->is_vepu540 = 0;
    }

    ret = hal_bufs_init(&p->hw_recn);
    if (ret) {
        mpp_err_f("init vepu buffer failed ret: %d\n", ret);
        goto DONE;
    }

    p->osd_cfg.reg_base = &p->regs_set;
    p->osd_cfg.dev = p->dev;
    p->osd_cfg.reg_cfg = NULL;
    p->osd_cfg.plt_cfg = &p->cfg->plt_cfg;
    p->osd_cfg.osd_data = NULL;
    p->osd_cfg.osd_data2 = NULL;

    {   /* setup default hardware config */
        MppEncHwCfg *hw = &cfg->cfg->hw;

        hw->qp_delta_row_i  = 0;
        hw->qp_delta_row    = 2;

        memcpy(hw->aq_thrd_i, h264_aq_tthd_default, sizeof(hw->aq_thrd_i));
        memcpy(hw->aq_thrd_p, h264_aq_tthd_default, sizeof(hw->aq_thrd_p));
        memcpy(hw->aq_step_i, h264_I_aq_step_default, sizeof(hw->aq_step_i));
        memcpy(hw->aq_step_p, h264_P_aq_step_default, sizeof(hw->aq_step_p));
    }

DONE:
    if (ret)
        hal_h264e_vepu541_deinit(hal);

    hal_h264e_dbg_func("leave %p\n", p);
    return ret;
}

static void setup_hal_bufs(HalH264eVepu541Ctx *ctx)
{
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    RK_S32 alignment = 64;
    RK_S32 aligned_w = MPP_ALIGN(prep->width,  alignment);
    RK_S32 aligned_h = MPP_ALIGN(prep->height, alignment) + 16;
    RK_S32 pixel_buf_fbc_hdr_size = MPP_ALIGN(aligned_w * aligned_h / 64, SZ_8K);
    RK_S32 pixel_buf_fbc_bdy_size = aligned_w * aligned_h * 3 / 2;
    RK_S32 pixel_buf_size = pixel_buf_fbc_hdr_size + pixel_buf_fbc_bdy_size;
    RK_S32 thumb_buf_size = MPP_ALIGN(aligned_w / 64 * aligned_h / 64 * 256, SZ_8K);
    RK_S32 old_max_cnt = ctx->max_buf_cnt;
    RK_S32 new_max_cnt = 2;
    MppEncRefCfg ref_cfg = cfg->ref_cfg;
    if (ref_cfg) {
        MppEncCpbInfo *info = mpp_enc_ref_cfg_get_cpb_info(ref_cfg);
        if (new_max_cnt < MPP_MAX(new_max_cnt, info->dpb_size + 1))
            new_max_cnt = MPP_MAX(new_max_cnt, info->dpb_size + 1);
    }

    if ((ctx->pixel_buf_fbc_hdr_size != pixel_buf_fbc_hdr_size) ||
        (ctx->pixel_buf_fbc_bdy_size != pixel_buf_fbc_bdy_size) ||
        (ctx->pixel_buf_size != pixel_buf_size) ||
        (ctx->thumb_buf_size != thumb_buf_size) ||
        (new_max_cnt > old_max_cnt)) {
        size_t sizes[2];

        hal_h264e_dbg_detail("frame size %d -> %d max count %d -> %d\n",
                             ctx->pixel_buf_size, pixel_buf_size,
                             old_max_cnt, new_max_cnt);

        /* pixel buffer */
        sizes[0] = pixel_buf_size;
        /* thumb buffer */
        sizes[1] = thumb_buf_size;
        new_max_cnt = MPP_MAX(new_max_cnt, old_max_cnt);

        hal_bufs_setup(ctx->hw_recn, new_max_cnt, 2, sizes);

        ctx->pixel_buf_fbc_hdr_size = pixel_buf_fbc_hdr_size;
        ctx->pixel_buf_fbc_bdy_size = pixel_buf_fbc_bdy_size;
        ctx->pixel_buf_size = pixel_buf_size;
        ctx->thumb_buf_size = thumb_buf_size;
        ctx->max_buf_cnt = new_max_cnt;
    }
}

static MPP_RET hal_h264e_vepu541_prepare(void *hal)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
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
        case H264E_SYN_DPB : {
            hal_h264e_dbg_detail("update dpb");
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

static MPP_RET hal_h264e_vepu541_get_task(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    RK_U32 updated = update_vepu541_syntax(ctx, &task->syntax);
    EncFrmStatus *frm_status = &task->rc_task->frm;

    hal_h264e_dbg_func("enter %p\n", hal);

    if (updated & SYN_TYPE_FLAG(H264E_SYN_CFG))
        setup_hal_bufs(ctx);

    if (!frm_status->reencode && mpp_frame_has_meta(task->frame)) {
        MppMeta meta = mpp_frame_get_meta(task->frame);

        mpp_meta_get_ptr_d(meta, KEY_ROI_DATA, (void **)&ctx->roi_data, NULL);
        mpp_meta_get_ptr_d(meta, KEY_ROI_DATA2, (void **)&ctx->roi_data2, NULL);
        mpp_meta_get_ptr_d(meta, KEY_OSD_DATA, (void **)&ctx->osd_cfg.osd_data, NULL);
        mpp_meta_get_ptr_d(meta, KEY_OSD_DATA2, (void **)&ctx->osd_cfg.osd_data2, NULL);
        mpp_meta_get_buffer_d(meta, KEY_QPMAP0, &ctx->qpmap, NULL);
    }
    hal_h264e_dbg_func("leave %p\n", hal);

    return MPP_OK;
}

static void setup_vepu541_normal(Vepu541H264eRegSet *regs, RK_U32 is_vepu540)
{
    hal_h264e_dbg_func("enter\n");
    /* reg000 VERSION is read only */

    /* reg001 ENC_STRT */
    regs->reg001.lkt_num            = 0;
    regs->reg001.rkvenc_cmd         = 1;
    regs->reg001.clk_gate_en        = 1;
    regs->reg001.resetn_hw_en       = 0;
    regs->reg001.enc_done_tmvp_en   = 1;

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
    regs->reg004.wdg_done_en        = 0;

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
    if (is_vepu540) {
        /* vepu540 */
        regs->reg016.vepu540.axi_brsp_cke   = 0;
        regs->reg016.vepu540.dspr_otsd      = 1;
    } else {
        /* vepu541 */
        regs->reg016.vepu541.axi_brsp_cke   = 0;
        regs->reg016.vepu541.dspr_otsd      = 1;
    }

    hal_h264e_dbg_func("leave\n");
}

static MPP_RET setup_vepu541_prep(Vepu541H264eRegSet *regs, MppEncPrepCfg *prep)
{
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

    /* reg012 ENC_RSL */
    regs->reg012.pic_wd8_m1 = MPP_ALIGN(prep->width, 16) / 8 - 1;
    regs->reg012.pic_wfill  = MPP_ALIGN(prep->width, 16) - prep->width;
    regs->reg012.pic_hd8_m1 = MPP_ALIGN(prep->height, 16) / 8 - 1;
    regs->reg012.pic_hfill  = MPP_ALIGN(prep->height, 16) - prep->height;

    /* reg015 DTRNS_MAP */
    regs->reg015.src_bus_edin = cfg.src_endian;

    /* reg022 SRC_PROC */
    regs->reg017.src_cfmt   = hw_fmt;
    regs->reg017.alpha_swap = cfg.alpha_swap;
    regs->reg017.rbuv_swap  = cfg.rbuv_swap;
    regs->reg017.src_range  = cfg.src_range;
    regs->reg017.out_fmt_cfg = 0;

    y_stride = (MPP_FRAME_FMT_IS_FBC(fmt)) ? (MPP_ALIGN(prep->width, 16)) :
               (prep->hor_stride) ? (prep->hor_stride) : (prep->width);
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

    regs->reg022.afbcd_en   = MPP_FRAME_FMT_IS_FBC(fmt) ? 1 : 0;
    regs->reg069.src_strd0  = y_stride;
    regs->reg069.src_strd1  = c_stride;

    regs->reg022.src_mirr   = prep->mirroring > 0;
    regs->reg022.src_rot    = prep->rotation;
    regs->reg022.txa_en     = 1;

    regs->reg023.sli_crs_en = 1;

    regs->reg068.pic_ofst_y = 0;
    regs->reg068.pic_ofst_x = 0;

    hal_h264e_dbg_func("leave\n");

    return ret;
}

static void setup_vepu541_codec(Vepu541H264eRegSet *regs, H264eSps *sps,
                                H264ePps *pps, H264eSlice *slice)
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


    regs->reg108.dis_dblk_idc   = slice->disable_deblocking_filter_idc;
    regs->reg108.sli_alph_ofst  = slice->slice_alpha_c0_offset_div2;

    h264e_reorder_rd_rewind(slice->reorder);
    {   /* reorder process */
        H264eRplmo rplmo;
        MPP_RET ret = h264e_reorder_rd_op(slice->reorder, &rplmo);

        if (MPP_OK == ret) {
            regs->reg108.ref_list0_rodr = 1;
            regs->reg108.rodr_pic_idx   = rplmo.modification_of_pic_nums_idc;

            switch (rplmo.modification_of_pic_nums_idc) {
            case 0 :
            case 1 : {
                regs->reg108.rodr_pic_num   = rplmo.abs_diff_pic_num_minus1;
            } break;
            case 2 : {
                regs->reg108.rodr_pic_num   = rplmo.long_term_pic_idx;
            } break;
            default : {
                mpp_err_f("invalid modification_of_pic_nums_idc %d\n",
                          rplmo.modification_of_pic_nums_idc);
            } break;
            }
        } else {
            // slice->ref_pic_list_modification_flag;
            regs->reg108.ref_list0_rodr = 0;
            regs->reg108.rodr_pic_idx   = 0;
            regs->reg108.rodr_pic_num   = 0;
        }
    }

    /* clear all mmco arg first */
    regs->reg109.nopp_flg               = 0;
    regs->reg109.ltrf_flg               = 0;
    regs->reg109.arpm_flg               = 0;
    regs->reg109.mmco4_pre              = 0;
    regs->reg109.mmco_type0             = 0;
    regs->reg109.mmco_parm0             = 0;
    regs->reg109.mmco_type1             = 0;
    regs->reg110.mmco_parm1             = 0;
    regs->reg109.mmco_type2             = 0;
    regs->reg110.mmco_parm2             = 0;
    regs->reg114.long_term_frame_idx0   = 0;
    regs->reg114.long_term_frame_idx1   = 0;
    regs->reg114.long_term_frame_idx2   = 0;

    h264e_marking_rd_rewind(slice->marking);

    /* only update used parameter */
    if (slice->slice_type == H264_I_SLICE) {
        regs->reg109.nopp_flg       = slice->no_output_of_prior_pics;
        regs->reg109.ltrf_flg       = slice->long_term_reference_flag;
    } else {
        if (!h264e_marking_is_empty(slice->marking)) {
            H264eMmco mmco;

            regs->reg109.arpm_flg       = 1;

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

                regs->reg109.mmco_type0 = type;
                regs->reg109.mmco_parm0 = param_0;
                regs->reg114.long_term_frame_idx0 = param_1;

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

                regs->reg109.mmco_type1 = type;
                regs->reg110.mmco_parm1 = param_0;
                regs->reg114.long_term_frame_idx1 = param_1;

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

                regs->reg109.mmco_type2 = type;
                regs->reg110.mmco_parm2 = param_0;
                regs->reg114.long_term_frame_idx2 = param_1;
            } while (0);
        }
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_rdo_pred(Vepu541H264eRegSet *regs, H264eSps *sps,
                                   H264ePps *pps, H264eSlice *slice)
{
    hal_h264e_dbg_func("enter\n");

    if (slice->slice_type == H264_I_SLICE) {
        regs->reg025.chrm_klut_ofst = 0;
        memcpy(&regs->reg026, &h264e_klut_weight[0], CHROMA_KLUT_TAB_SIZE);
    } else {
        regs->reg025.chrm_klut_ofst = 3;
        memcpy(&regs->reg026, &h264e_klut_weight[4], CHROMA_KLUT_TAB_SIZE);
    }

    regs->reg101.vthd_y         = 9;
    regs->reg101.vthd_c         = 63;

    regs->reg102.rect_size      = (sps->profile_idc == H264_PROFILE_BASELINE &&
                                   sps->level_idc <= H264_LEVEL_3_0) ? 1 : 0;
    regs->reg102.inter_4x4      = 0;
    regs->reg102.vlc_lmt        = (sps->profile_idc < H264_PROFILE_MAIN) &&
                                  !pps->entropy_coding_mode;
    regs->reg102.chrm_spcl      = 1;
    regs->reg102.rdo_mask       = 24;
    regs->reg102.ccwa_e         = 1;
    regs->reg102.scl_lst_sel    = pps->pic_scaling_matrix_present;
    regs->reg102.scl_lst_sel_   = pps->pic_scaling_matrix_present;
    regs->reg102.atr_e          = 1;
    regs->reg102.atf_edg        = 0;
    regs->reg102.atf_lvl_e      = 0;
    regs->reg102.atf_intra_e    = 1;
    regs->reg102.satd_byps_flg  = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_rc_base(Vepu541H264eRegSet *regs, H264eSps *sps,
                                  H264eSlice *slice, MppEncHwCfg *hw,
                                  EncRcTask *rc_task)
{
    EncRcTaskInfo *rc_info = &rc_task->info;
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

    if (mb_target_bits_mul_16 >= 0x100000) {
        mb_target_bits_mul_16 = 0x50000;
    }

    mb_target_bits = (mb_target_bits_mul_16 * mb_w) >> 4;
    negative_bits_thd = 0 - mb_target_bits / 4;
    positive_bits_thd = mb_target_bits / 4;

    hal_h264e_dbg_func("enter\n");

    regs->reg013.pic_qp         = qp_target;

    regs->reg050.rc_en          = 1;
    regs->reg050.aq_en          = 1;
    regs->reg050.aq_mode        = 0;
    regs->reg050.rc_ctu_num     = mb_w;

    regs->reg051.rc_qp_range    = (slice->slice_type == H264_I_SLICE) ?
                                  hw->qp_delta_row_i : hw->qp_delta_row;
    regs->reg051.rc_max_qp      = qp_max;
    regs->reg051.rc_min_qp      = qp_min;

    regs->reg052.ctu_ebit       = mb_target_bits_mul_16;

    regs->reg053.qp_adj0        = -1;
    regs->reg053.qp_adj1        = 0;
    regs->reg053.qp_adj2        = 0;
    regs->reg053.qp_adj3        = 0;
    regs->reg053.qp_adj4        = 0;
    regs->reg054.qp_adj5        = 0;
    regs->reg054.qp_adj6        = 0;
    regs->reg054.qp_adj7        = 0;
    regs->reg054.qp_adj8        = 1;

    regs->reg055_063.rc_dthd[0] = negative_bits_thd;
    regs->reg055_063.rc_dthd[1] = positive_bits_thd;
    regs->reg055_063.rc_dthd[2] = positive_bits_thd;
    regs->reg055_063.rc_dthd[3] = positive_bits_thd;
    regs->reg055_063.rc_dthd[4] = positive_bits_thd;
    regs->reg055_063.rc_dthd[5] = positive_bits_thd;
    regs->reg055_063.rc_dthd[6] = positive_bits_thd;
    regs->reg055_063.rc_dthd[7] = positive_bits_thd;
    regs->reg055_063.rc_dthd[8] = positive_bits_thd;

    regs->reg064.qpmin_area0    = qp_min;
    regs->reg064.qpmax_area0    = qp_max;
    regs->reg064.qpmin_area1    = qp_min;
    regs->reg064.qpmax_area1    = qp_max;
    regs->reg064.qpmin_area2    = qp_min;

    regs->reg065.qpmax_area2    = qp_max;
    regs->reg065.qpmin_area3    = qp_min;
    regs->reg065.qpmax_area3    = qp_max;
    regs->reg065.qpmin_area4    = qp_min;
    regs->reg065.qpmax_area4    = qp_max;

    regs->reg066.qpmin_area5    = qp_min;
    regs->reg066.qpmax_area5    = qp_max;
    regs->reg066.qpmin_area6    = qp_min;
    regs->reg066.qpmax_area6    = qp_max;
    regs->reg066.qpmin_area7    = qp_min;

    regs->reg067.qpmax_area7    = qp_max;
    regs->reg067.qpmap_mode     = qpmap_mode;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_io_buf(Vepu541H264eRegSet *regs, MppDev dev,
                                 HalEncTask *task)
{
    MppFrame frm = task->frame;
    MppPacket pkt = task->packet;
    MppBuffer buf_in = mpp_frame_get_buffer(frm);
    MppBuffer buf_out = task->output;
    MppFrameFormat fmt = mpp_frame_get_fmt(frm);
    RK_S32 hor_stride = mpp_frame_get_hor_stride(frm);
    RK_S32 ver_stride = mpp_frame_get_ver_stride(frm);
    RK_S32 fd_in = mpp_buffer_get_fd(buf_in);
    RK_U32 off_in[2] = {0};
    RK_U32 off_out = mpp_packet_get_length(pkt);
    size_t siz_out = mpp_buffer_get_size(buf_out);
    RK_S32 fd_out = mpp_buffer_get_fd(buf_out);

    hal_h264e_dbg_func("enter\n");

    regs->reg070.adr_src0   = fd_in;
    regs->reg071.adr_src1   = fd_in;
    regs->reg072.adr_src2   = fd_in;

    regs->reg084.bsbb_addr  = fd_out;
    regs->reg085.bsbr_addr  = fd_out;
    regs->reg086.adr_bsbs   = fd_out;
    regs->reg083.bsbt_addr  = fd_out;

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        off_in[0] = mpp_frame_get_fbc_offset(frm);;
        off_in[1] = 0;
    } else if (MPP_FRAME_FMT_IS_YUV(fmt)) {
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
        case VEPU541_FMT_YUYV422 :
        case VEPU541_FMT_UYVY422 : {
            off_in[0] = 0;
            off_in[1] = 0;
        } break;
        case VEPU541_FMT_NONE :
        default : {
            off_in[0] = 0;
            off_in[1] = 0;
        } break;
        }
    }

    MppDevRegOffsetCfg trans_cfg;

    trans_cfg.reg_idx = 71;
    trans_cfg.offset = off_in[0];
    mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    trans_cfg.reg_idx = 72;
    trans_cfg.offset = off_in[1];
    mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    trans_cfg.reg_idx = 83;
    trans_cfg.offset = siz_out;
    mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    trans_cfg.reg_idx = 86;
    trans_cfg.offset = off_out;
    mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_roi(Vepu541H264eRegSet *regs, HalH264eVepu541Ctx *ctx)
{

    hal_h264e_dbg_func("enter\n");

    if (ctx->roi_data2) {
        MppEncROICfg2 *cfg = ( MppEncROICfg2 *)ctx->roi_data2;

        regs->reg013.roi_enc = 1;
        regs->reg073.roi_addr = mpp_buffer_get_fd(cfg->base_cfg_buf);
    } else if (ctx->qpmap) {
        regs->reg013.roi_enc = 1;
        regs->reg073.roi_addr = mpp_buffer_get_fd(ctx->qpmap);
    } else {
        MppEncROICfg *roi = ctx->roi_data;
        RK_U32 w = ctx->sps->pic_width_in_mbs * 16;
        RK_U32 h = ctx->sps->pic_height_in_mbs * 16;

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
    }

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_recn_refr(Vepu541H264eRegSet *regs, MppDev dev,
                                    H264eFrmInfo *frms, HalBufs bufs,
                                    RK_S32 fbc_hdr_size)
{
    HalBuf *curr = hal_bufs_get_buf(bufs, frms->curr_idx);
    HalBuf *refr = hal_bufs_get_buf(bufs, frms->refr_idx);
    MppDevRegOffsetCfg trans_cfg;

    hal_h264e_dbg_func("enter\n");

    if (curr && curr->cnt) {
        MppBuffer buf_pixel = curr->buf[0];
        MppBuffer buf_thumb = curr->buf[1];
        RK_S32 fd = mpp_buffer_get_fd(buf_pixel);

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        regs->reg074.rfpw_h_addr = fd;
        regs->reg075.rfpw_b_addr = fd;
        regs->reg080.dspw_addr = mpp_buffer_get_fd(buf_thumb);

        trans_cfg.reg_idx = 75;
        trans_cfg.offset = fbc_hdr_size;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (refr && refr->cnt) {
        MppBuffer buf_pixel = refr->buf[0];
        MppBuffer buf_thumb = refr->buf[1];
        RK_S32 fd = mpp_buffer_get_fd(buf_pixel);

        mpp_assert(buf_pixel);
        mpp_assert(buf_thumb);

        regs->reg076.rfpr_h_addr = fd;
        regs->reg077.rfpr_b_addr = fd;
        regs->reg081.dspr_addr = mpp_buffer_get_fd(buf_thumb);

        trans_cfg.reg_idx = 77;
        trans_cfg.offset = fbc_hdr_size;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

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

static void setup_vepu540_force_slice_split(Vepu541H264eRegSet *regs, RK_S32 width)
{
    RK_S32 mb_w = MPP_ALIGN(width, 16) >> 4;

    hal_h264e_dbg_func("enter\n");

    regs->reg087.sli_splt = 1;
    regs->reg087.sli_splt_mode = 1;
    regs->reg087.sli_splt_cpst = 0;
    regs->reg087.sli_max_num_m1 = 500;
    regs->reg087.sli_flsh = 1;
    regs->reg087.sli_splt_cnum_m1 = mb_w - 1;

    regs->reg088.sli_splt_byte = 0;
    regs->reg013.slen_fifo = 0;
    regs->reg023.sli_crs_en = 0;

    hal_h264e_dbg_func("leave\n");
}

static void setup_vepu541_me(Vepu541H264eRegSet *regs, H264eSps *sps,
                             H264eSlice *slice, RK_U32 is_vepu540)
{
    RK_S32 level_idc = sps->level_idc;
    RK_S32 pic_w = sps->pic_width_in_mbs * 16;
    RK_S32 pic_wd64 = (pic_w + 63) / 64;
    RK_S32 cime_w = 176;
    RK_S32 cime_h = 112;
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
        regs->reg090.pmv_mdst_v = 5;
    }
    regs->reg090.mv_limit       = 2;
    regs->reg090.pmv_num        = 2;

    if (is_vepu540) {
        RK_S32 w_temp = 1296;
        RK_S32 h_temp = 1;
        RK_S32 h_val_0 = 1;
        RK_S32 h_val_1 = 18;
        RK_S32 temp0, temp1;
        RK_U32 pic_temp = ((regs->reg012.pic_wd8_m1 + 1) * 8 + 63) / 64 * 64;
        RK_S32 cime_linebuf_w =  pic_temp / 64;

        regs->reg091.cme_linebuf_w = cime_linebuf_w;

        while ((w_temp > ((h_temp - h_val_0)*cime_linebuf_w * 4 + ((h_val_1 - h_temp) * 4 * 7)))
               && (h_temp < 17)) {
            h_temp = h_temp + h_val_0;
        }
        if (w_temp < ((h_temp - h_val_0)*cime_linebuf_w * 4 + ((h_val_1 - h_temp) * 4 * 7)))
            h_temp = h_temp - h_val_0;

        regs->reg091.cme_rama_h = h_temp;

        RK_S32 swin_scope_wd16 = (regs->reg089.cme_srch_h + 3) / 4 * 2 + 1;

        temp0 = 2 * regs->reg089.cme_srch_v + 1;
        if (temp0 > regs->reg091.cme_rama_h)
            temp0 = regs->reg091.cme_rama_h;

        temp1 = 0;
        if (pic_wd64 >= swin_scope_wd16)
            temp1 = swin_scope_wd16;
        else
            temp1 = pic_wd64 * 2;
        regs->reg091.cme_rama_max = pic_wd64 * (temp0 - 1) + temp1;
    } else {
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
    }
    hal_h264e_dbg_func("leave\n");
}

#define H264E_LAMBDA_TAB_SIZE       (52 * sizeof(RK_U32))

static RK_U32 h264e_lambda_default[58] = {
    0x00000003, 0x00000005, 0x00000006, 0x00000007,
    0x00000009, 0x0000000b, 0x0000000e, 0x00000012,
    0x00000016, 0x0000001c, 0x00000024, 0x0000002d,
    0x00000039, 0x00000048, 0x0000005b, 0x00000073,
    0x00000091, 0x000000b6, 0x000000e6, 0x00000122,
    0x0000016d, 0x000001cc, 0x00000244, 0x000002db,
    0x00000399, 0x00000489, 0x000005b6, 0x00000733,
    0x00000912, 0x00000b6d, 0x00000e66, 0x00001224,
    0x000016db, 0x00001ccc, 0x00002449, 0x00002db7,
    0x00003999, 0x00004892, 0x00005b6f, 0x00007333,
    0x00009124, 0x0000b6de, 0x0000e666, 0x00012249,
    0x00016dbc, 0x0001cccc, 0x00024492, 0x0002db79,
    0x00039999, 0x00048924, 0x0005b6f2, 0x00073333,
    0x00091249, 0x000b6de5, 0x000e6666, 0x00122492,
    0x0016dbcb, 0x001ccccc,
};

static void setup_vepu541_l2(Vepu541H264eRegL2Set *regs, H264eSlice *slice, MppEncHwCfg *hw)
{
    RK_U32 i;

    hal_h264e_dbg_func("enter\n");

    memset(regs, 0, sizeof(*regs));

    regs->iprd_tthdy4[0] = 1;
    regs->iprd_tthdy4[1] = 4;
    regs->iprd_tthdy4[2] = 9;
    regs->iprd_tthdy4[3] = 36;

    regs->iprd_tthdc8[0] = 1;
    regs->iprd_tthdc8[1] = 4;
    regs->iprd_tthdc8[2] = 9;
    regs->iprd_tthdc8[3] = 36;

    regs->iprd_tthdy8[0] = 1;
    regs->iprd_tthdy8[1] = 4;
    regs->iprd_tthdy8[2] = 9;
    regs->iprd_tthdy8[3] = 36;

    regs->iprd_tthd_ul = 0x0;

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
    regs->qnt_bias_comb.qnt_bias_p = 171;

    regs->atr_thd0_h264.atr_thd0 = 1;
    regs->atr_thd0_h264.atr_thd1 = 4;

    if (slice->slice_type == H264_I_SLICE) {
        regs->qnt_bias_comb.qnt_bias_i = 683;
        regs->atr_thd1_h264.atr_thd2 = 36;
        regs->atr_wgt16_h264.atr_lv16_wgt0 = 16;
        regs->atr_wgt16_h264.atr_lv16_wgt1 = 16;
        regs->atr_wgt16_h264.atr_lv16_wgt2 = 16;

        regs->atr_wgt8_h264.atr_lv8_wgt0 = 32;
        regs->atr_wgt8_h264.atr_lv8_wgt1 = 32;
        regs->atr_wgt8_h264.atr_lv8_wgt2 = 32;

        regs->atr_wgt4_h264.atr_lv4_wgt0 = 20;
        regs->atr_wgt4_h264.atr_lv4_wgt1 = 18;
        regs->atr_wgt4_h264.atr_lv4_wgt2 = 16;
    } else {
        regs->qnt_bias_comb.qnt_bias_i = 583;
        regs->atr_thd1_h264.atr_thd2 = 81;
        regs->atr_wgt16_h264.atr_lv16_wgt0 = 28;
        regs->atr_wgt16_h264.atr_lv16_wgt1 = 27;
        regs->atr_wgt16_h264.atr_lv16_wgt2 = 23;

        regs->atr_wgt8_h264.atr_lv8_wgt0 = 32;
        regs->atr_wgt8_h264.atr_lv8_wgt1 = 32;
        regs->atr_wgt8_h264.atr_lv8_wgt2 = 32;

        regs->atr_wgt4_h264.atr_lv4_wgt0 = 28;
        regs->atr_wgt4_h264.atr_lv4_wgt1 = 27;
        regs->atr_wgt4_h264.atr_lv4_wgt2 = 23;
    }

    regs->atr_thd1_h264.atr_qp = 45;
    regs->atf_tthd[0] = 0;
    regs->atf_tthd[1] = 64;
    regs->atf_tthd[2] = 144;
    regs->atf_tthd[3] = 2500;

    regs->atf_sthd0_h264.atf_sthd_10 = 80;
    regs->atf_sthd0_h264.atf_sthd_max = 280;

    regs->atf_sthd1_h264.atf_sthd_11 = 144;
    regs->atf_sthd1_h264.atf_sthd_20 = 192;

    regs->atf_wgt0_h264.atf_wgt10 = 26;
    regs->atf_wgt0_h264.atf_wgt11 = 24;

    regs->atf_wgt1_h264.atf_wgt12 = 19;
    regs->atf_wgt1_h264.atf_wgt20 = 22;

    regs->atf_wgt2_h264.atf_wgt21 = 19;
    regs->atf_wgt2_h264.atf_wgt30 = 19;

    regs->atf_ofst0_h264.atf_ofst10 = 3500;
    regs->atf_ofst0_h264.atf_ofst11 = 3500;

    regs->atf_ofst1_h264.atf_ofst12 = 0;
    regs->atf_ofst1_h264.atf_ofst20 = 3500;

    regs->atf_ofst2_h264.atf_ofst21 = 1000;
    regs->atf_ofst2_h264.atf_ofst30 = 0;

    regs->iprd_wgt_qp[0]  = 0;
    /* ~ */
    regs->iprd_wgt_qp[51] = 0;

    memcpy(regs->wgt_qp_grpa, &h264e_lambda_default[6], H264E_LAMBDA_TAB_SIZE);
    memcpy(regs->wgt_qp_grpb, &h264e_lambda_default[5], H264E_LAMBDA_TAB_SIZE);

    regs->madi_mode = 0;

    memcpy(regs->aq_tthd, h264_aq_tthd_default, sizeof(regs->aq_tthd));

    if (slice->slice_type == H264_I_SLICE) {
        for (i = 0; i < MPP_ARRAY_ELEMS(regs->aq_step); i++) {
            regs->aq_tthd[i] = hw->aq_thrd_i[i];
            regs->aq_step[i] = hw->aq_step_i[i] & 0x3f;
        }
    } else {
        for (i = 0; i < MPP_ARRAY_ELEMS(regs->aq_step); i++) {
            regs->aq_tthd[i] = hw->aq_thrd_p[i];
            regs->aq_step[i] = hw->aq_step_p[i] & 0x3f;
        }
    }

    regs->rme_mvd_penalty.mvd_pnlt_e    = 1;
    regs->rme_mvd_penalty.mvd_pnlt_coef = 1;
    regs->rme_mvd_penalty.mvd_pnlt_cnst = 16000;
    regs->rme_mvd_penalty.mvd_pnlt_lthd = 0;
    regs->rme_mvd_penalty.mvd_pnlt_hthd = 0;

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
    MppEncPrepCfg *prep = &cfg->prep;
    H264eSps *sps = ctx->sps;
    H264ePps *pps = ctx->pps;
    H264eSlice *slice = ctx->slice;
    MPP_RET ret = MPP_OK;

    hal_h264e_dbg_func("enter %p\n", hal);
    hal_h264e_dbg_detail("frame %d generate regs now", ctx->frms->seq_idx);

    /* register setup */
    memset(regs, 0, sizeof(*regs));

    setup_vepu541_normal(regs, ctx->is_vepu540);
    ret = setup_vepu541_prep(regs, &ctx->cfg->prep);
    if (ret)
        return ret;

    setup_vepu541_codec(regs, sps, pps, slice);
    setup_vepu541_rdo_pred(regs, sps, pps, slice);
    setup_vepu541_rc_base(regs, sps, slice, &cfg->hw, task->rc_task);
    setup_vepu541_io_buf(regs, ctx->dev, task);
    setup_vepu541_roi(regs, ctx);
    setup_vepu541_recn_refr(regs, ctx->dev, ctx->frms, ctx->hw_recn,
                            ctx->pixel_buf_fbc_hdr_size);

    regs->reg082.meiw_addr = task->md_info ? mpp_buffer_get_fd(task->md_info) : 0;

    regs->reg068.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->reg068.pic_ofst_x = mpp_frame_get_offset_x(task->frame);

    setup_vepu541_split(regs, &cfg->split);
    if (ctx->is_vepu540 && prep->width > 1920)
        setup_vepu540_force_slice_split(regs, prep->width);

    setup_vepu541_me(regs, sps, slice, ctx->is_vepu540);

    if (ctx->is_vepu540)
        vepu540_set_osd(&ctx->osd_cfg);
    else
        vepu541_set_osd(&ctx->osd_cfg);

    setup_vepu541_l2(&ctx->regs_l2_set, slice, &cfg->hw);

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

    (void) task;

    hal_h264e_dbg_func("enter %p\n", hal);

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        /* write L2 registers */
        wr_cfg.reg = &ctx->regs_l2_set;
        wr_cfg.size = sizeof(Vepu541H264eRegL2Set);
        wr_cfg.offset = VEPU541_REG_BASE_L2;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        /* write L1 registers */
        wr_cfg.reg = &ctx->regs_set;
        wr_cfg.size = sizeof(Vepu541H264eRegSet);
        wr_cfg.offset = 0;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        /* set output request */
        rd_cfg.reg = &ctx->regs_ret.hw_status;
        rd_cfg.size = sizeof(RK_U32);
        rd_cfg.offset = VEPU541_REG_BASE_HW_STATUS;

        ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &ctx->regs_ret.st_bsl;
        rd_cfg.size = sizeof(ctx->regs_ret) - 4;
        rd_cfg.offset = VEPU541_REG_BASE_STATISTICS;

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

static MPP_RET hal_h264e_vepu541_status_check(void *hal)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    Vepu541H264eRegRet *regs_ret = &ctx->regs_ret;

    if (regs_ret->hw_status.lkt_done_sta)
        hal_h264e_dbg_detail("lkt_done finish");

    if (regs_ret->hw_status.enc_done_sta)
        hal_h264e_dbg_detail("enc_done finish");

    if (regs_ret->hw_status.enc_slice_done_sta)
        hal_h264e_dbg_detail("enc_slice finsh");

    if (regs_ret->hw_status.sclr_done_sta)
        hal_h264e_dbg_detail("safe clear finsh");

    if (regs_ret->hw_status.oflw_done_sta)
        mpp_err_f("bit stream overflow");

    if (regs_ret->hw_status.brsp_done_sta)
        mpp_err_f("bus write full");

    if (regs_ret->hw_status.berr_done_sta)
        mpp_err_f("bus write error");

    if (regs_ret->hw_status.rerr_done_sta)
        mpp_err_f("bus read error");

    if (regs_ret->hw_status.wdg_done_sta)
        mpp_err_f("wdg timeout");

    return MPP_OK;
}

static MPP_RET hal_h264e_vepu541_wait(void *hal, HalEncTask *task)
{
    MPP_RET ret = MPP_OK;
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;

    hal_h264e_dbg_func("enter %p\n", hal);

    ret = mpp_dev_ioctl(ctx->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret) {
        mpp_err_f("poll cmd failed %d\n", ret);
        ret = MPP_ERR_VPUHW;
    } else {
        hal_h264e_vepu541_status_check(hal);
        task->hw_length += ctx->regs_ret.st_bsl.bs_lgth;
    }

    hal_h264e_dbg_func("leave %p\n", hal);

    return ret;
}

static MPP_RET hal_h264e_vepu541_ret_task(void *hal, HalEncTask *task)
{
    HalH264eVepu541Ctx *ctx = (HalH264eVepu541Ctx *)hal;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    RK_U32 mb_w = ctx->sps->pic_width_in_mbs;
    RK_U32 mb_h = ctx->sps->pic_height_in_mbs;
    RK_U32 mbs = mb_w * mb_h;

    hal_h264e_dbg_func("enter %p\n", hal);

    // update total hardware length
    task->length += task->hw_length;

    // setup bit length for rate control
    rc_info->bit_real = task->hw_length * 8;
    rc_info->quality_real = ctx->regs_ret.st_sse_qp.qp_sum / mbs;
    rc_info->madi = (!ctx->regs_ret.st_mb_num) ? 0 :
                    ctx->regs_ret.st_madi /  ctx->regs_ret.st_mb_num;
    rc_info->madp = (!ctx->regs_ret.st_ctu_num) ? 0 :
                    ctx->regs_ret.st_madp / ctx->regs_ret.st_ctu_num;
    rc_info->iblk4_prop = (ctx->regs_ret.st_lvl4_intra_num +
                           ctx->regs_ret.st_lvl8_intra_num +
                           ctx->regs_ret.st_lvl16_intra_num) * 256 / mbs;

    ctx->hal_rc_cfg.bit_real = rc_info->bit_real;
    ctx->hal_rc_cfg.quality_real = rc_info->quality_real;
    ctx->hal_rc_cfg.iblk4_prop = rc_info->iblk4_prop;

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
    .prepare    = hal_h264e_vepu541_prepare,
    .get_task   = hal_h264e_vepu541_get_task,
    .gen_regs   = hal_h264e_vepu541_gen_regs,
    .start      = hal_h264e_vepu541_start,
    .wait       = hal_h264e_vepu541_wait,
    .part_start = NULL,
    .part_wait  = NULL,
    .ret_task   = hal_h264e_vepu541_ret_task,
};
