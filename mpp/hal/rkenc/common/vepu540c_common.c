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

#define MODULE_TAG  "vepu540c_common"

#include <linux/string.h>

#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "jpege_syntax.h"
#include "vepu5xx_common.h"
#include "vepu540c_common.h"
#include "hal_enc_task.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"

MPP_RET vepu540c_set_roi(void *roi_reg_base, MppEncROICfg * roi,
                         RK_S32 w, RK_S32 h)
{
    MppEncROIRegion *region = roi->regions;
    Vepu540cRoiCfg  *roi_cfg = (Vepu540cRoiCfg *)roi_reg_base;
    Vepu540cRoiRegion *reg_regions = &roi_cfg->regions[0];
    MPP_RET ret = MPP_NOK;
    RK_S32 i = 0;
    memset(reg_regions, 0, sizeof(Vepu540cRoiRegion) * 8);
    if (NULL == roi_cfg || NULL == roi) {
        mpp_err_f("invalid buf %p roi %p\n", roi_cfg, roi);
        goto DONE;
    }

    if (roi->number > VEPU540C_MAX_ROI_NUM) {
        mpp_err_f("invalid region number %d\n", roi->number);
        goto DONE;
    }

    /* check region config */
    ret = MPP_OK;
    for (i = 0; i < (RK_S32) roi->number; i++, region++) {
        if (region->x + region->w > w || region->y + region->h > h)
            ret = MPP_NOK;

        if (region->intra > 1
            || region->qp_area_idx >= VEPU540C_MAX_ROI_NUM
            || region->area_map_en > 1 || region->abs_qp_en > 1)
            ret = MPP_NOK;

        if ((region->abs_qp_en && region->quality > 51) ||
            (!region->abs_qp_en
             && (region->quality > 51 || region->quality < -51)))
            ret = MPP_NOK;

        if (ret) {
            mpp_err_f("region %d invalid param:\n", i);
            mpp_err_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
                      region->x, region->y, region->w, region->h, w,
                      h);
            mpp_err_f("force intra %d qp area index %d\n",
                      region->intra, region->qp_area_idx);
            mpp_err_f("abs qp mode %d value %d\n",
                      region->abs_qp_en, region->quality);
            goto DONE;
        }
        reg_regions->roi_pos_lt.roi_lt_x = MPP_ALIGN(region->x, 16) >> 4;
        reg_regions->roi_pos_lt.roi_lt_y = MPP_ALIGN(region->y, 16) >> 4;
        reg_regions->roi_pos_rb.roi_rb_x = MPP_ALIGN(region->x + region->w, 16) >> 4;
        reg_regions->roi_pos_rb.roi_rb_y = MPP_ALIGN(region->y + region->h, 16) >> 4;
        reg_regions->roi_base.roi_qp_value = region->quality;
        reg_regions->roi_base.roi_qp_adj_mode = region->abs_qp_en;
        reg_regions->roi_base.roi_en = 1;
        reg_regions->roi_base.roi_pri = 0x1f;
        if (region->intra) {
            reg_regions->roi_mdc.roi_mdc_intra16 = 1;
            reg_regions->roi_mdc.roi0_mdc_intra32_hevc = 1;
        }
        reg_regions++;
    }

DONE:
    return ret;
}

static MPP_RET vepu540c_jpeg_set_patch_info(MppDev dev, JpegeSyntax *syn,
                                            VepuFmt input_fmt,
                                            HalEncTask *task)
{
    RK_U32 hor_stride = syn->hor_stride;
    RK_U32 ver_stride = syn->ver_stride ? syn->ver_stride : syn->height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(task->frame))) {
        u_offset = mpp_frame_get_fbc_offset(task->frame);
        v_offset = 0;
        mpp_log("fbc case u_offset = %d", u_offset);
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
            mpp_err("unknown color space: %d\n", input_fmt);
            u_offset = frame_size;
            v_offset = frame_size * 5 / 4;
        }
        }
    }

    /* input cb addr */
    if (u_offset)
        mpp_dev_set_reg_offset(dev, 265, u_offset);

    /* input cr addr */
    if (v_offset)
        mpp_dev_set_reg_offset(dev, 266, v_offset);

    return ret;
}


MPP_RET vepu540c_set_jpeg_reg(Vepu540cJpegCfg *cfg)
{
    HalEncTask *task = ( HalEncTask *)cfg->enc_task;
    JpegeSyntax *syn = (JpegeSyntax *)task->syntax.data;
    Vepu540cJpegReg *regs = (Vepu540cJpegReg *)cfg->jpeg_reg_base;
    VepuFmtCfg *fmt = (VepuFmtCfg *)cfg->input_fmt;
    RK_U32 pic_width_align8, pic_height_align8;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    pic_width_align8 = (syn->width + 7) & (~7);
    pic_height_align8 = (syn->height + 7) & (~7);

    regs->reg0264_adr_src0 =  mpp_buffer_get_fd(task->input);
    regs->reg0265_adr_src1 = regs->reg0264_adr_src0;
    regs->reg0266_adr_src2 = regs->reg0264_adr_src0;

    vepu540c_jpeg_set_patch_info(cfg->dev, syn, (VepuFmt)fmt->format, task);

    regs->reg0256_adr_bsbt = mpp_buffer_get_fd(task->output);
    regs->reg0257_adr_bsbb = regs->reg0256_adr_bsbt;
    regs->reg0258_adr_bsbs = regs->reg0256_adr_bsbt;
    regs->reg0259_adr_bsbr = regs->reg0256_adr_bsbt;

    mpp_dev_set_reg_offset(cfg->dev, 258, mpp_packet_get_length(task->packet));
    mpp_dev_set_reg_offset(cfg->dev, 256, mpp_buffer_get_size(task->output));

    regs->reg0272_enc_rsl.pic_wd8_m1    = pic_width_align8 / 8 - 1;
    regs->reg0273_src_fill.pic_wfill    = (syn->width & 0x7)
                                          ? (8 - (syn->width & 0x7)) : 0;
    regs->reg0272_enc_rsl.pic_hd8_m1    = pic_height_align8 / 8 - 1;
    regs->reg0273_src_fill.pic_hfill    = (syn->height & 0x7)
                                          ? (8 - (syn->height & 0x7)) : 0;

    regs->reg0274_src_fmt.src_cfmt = fmt->format;
    regs->reg0274_src_fmt.alpha_swap = fmt->alpha_swap;
    regs->reg0274_src_fmt.rbuv_swap  = fmt->rbuv_swap;
    regs->reg0274_src_fmt.src_range_trns_en  = 0;
    regs->reg0274_src_fmt.src_range_trns_sel = 0;
    regs->reg0274_src_fmt.chroma_ds_mode     = 0;
    regs->reg0274_src_fmt.out_fmt = 1;

    regs->reg0279_src_proc.src_mirr = 0 ;//prep_cfg->mirroring > 0;
    regs->reg0279_src_proc.src_rot = syn->rotation;

    if (syn->hor_stride) {
        stridey = syn->hor_stride;
    } else {
        if (regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_BGRA8888)
            stridey = syn->width * 4;
        else if (regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_BGR888)
            stridey = syn->width * 3;
        else if (regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_BGR565 ||
                 regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_YUYV422 ||
                 regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_UYVY422)
            stridey = syn->width * 2;
    }

    stridec = (regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_YUV422SP ||
               regs->reg0274_src_fmt.src_cfmt == VEPU5xx_FMT_YUV420SP) ?
              stridey : stridey / 2;

    if (regs->reg0274_src_fmt.src_cfmt < VEPU5xx_FMT_ARGB1555) {
        regs->reg0275_src_udfy.csc_wgt_r2y = 66;
        regs->reg0275_src_udfy.csc_wgt_g2y = 129;
        regs->reg0275_src_udfy.csc_wgt_b2y = 25;

        regs->reg0276_src_udfu.csc_wgt_r2u = -38;
        regs->reg0276_src_udfu.csc_wgt_g2u = -74;
        regs->reg0276_src_udfu.csc_wgt_b2u = 112;

        regs->reg0277_src_udfv.csc_wgt_r2v = 112;
        regs->reg0277_src_udfv.csc_wgt_g2v = -94;
        regs->reg0277_src_udfv.csc_wgt_b2v = -18;

        regs->reg0278_src_udfo.csc_ofst_y = 16;
        regs->reg0278_src_udfo.csc_ofst_u = 128;
        regs->reg0278_src_udfo.csc_ofst_v = 128;
    }
    regs->reg0281_src_strd0.src_strd0  = stridey;
    regs->reg0282_src_strd1.src_strd1  = stridec;
    regs->reg0280_pic_ofst.pic_ofst_y = mpp_frame_get_offset_y(task->frame);
    regs->reg0280_pic_ofst.pic_ofst_x = mpp_frame_get_offset_x(task->frame);
    //to be done

    // no 0283 ?
    // regs->reg0283_src_flt.pp_corner_filter_strength = 0;
    // regs->reg0283_src_flt.pp_edge_filter_strength = 0;
    // regs->reg0283_src_flt.pp_internal_filter_strength = 0;

    regs->reg0284_y_cfg.bias_y = 0;
    regs->reg0285_u_cfg.bias_u = 0;
    regs->reg0286_v_cfg.bias_v = 0;

    regs->reg0287_base_cfg.jpeg_ri  = 0;
    regs->reg0287_base_cfg.jpeg_out_mode = 0;
    regs->reg0287_base_cfg.jpeg_start_rst_m = 0;
    regs->reg0287_base_cfg.jpeg_pic_last_ecs = 1;
    regs->reg0287_base_cfg.jpeg_slen_fifo = 0;
    regs->reg0287_base_cfg.jpeg_stnd = 1; //enable

    regs->reg0288_uvc_cfg.uvc_partition0_len = 0;
    regs->reg0288_uvc_cfg.uvc_partition_len = 0;
    regs->reg0288_uvc_cfg.uvc_skip_len = 0;
    return MPP_OK;
}
