/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "vepu511_common"

#include <string.h>

#include "kmpp_obj.h"
#include "kmpp_buffer.h"

#include "mpp_log.h"
#include "mpp_common.h"
#include "vepu511_common.h"
#include "jpege_syntax.h"
#include "vepu5xx_common.h"
#include "hal_enc_task.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_debug.h"
#include "mpp_mem.h"

MPP_RET vepu511_set_osd(Vepu511OsdCfg * cfg, Vepu511Osd *osd_reg)
{
    Vepu511Osd *regs = osd_reg;
    MppEncOSDData3 *osd_ptr = cfg->osd_data3;
    Vepu511OsdRegion *osd_regions = &regs->osd_regions[0];
    MppEncOSDRegion3 *region = osd_ptr->region;
    RK_U32 i = 0;

    if (NULL == osd_regions) {
        mpp_err_f("invalid reg_regions %p\n", osd_regions);
    }

    memset(osd_regions, 0, sizeof(Vepu511OsdRegion) * 8);
    if (osd_ptr->num_region > 8) {
        mpp_err_f("do NOT support more than 8 regions invalid num %d\n",
                  osd_ptr->num_region);
        mpp_assert(osd_ptr->num_region <= 8);
        return MPP_NOK;
    }

    for (i = 0; i < osd_ptr->num_region; i++, region++) {
        Vepu511OsdRegion *reg = &osd_reg->osd_regions[i];
        VepuFmtCfg fmt_cfg;
        MppFrameFormat fmt = region->fmt;
        KmppBuffer buffer = NULL;
        KmppBufCfg buf_cfg = NULL;

        vepu5xx_set_fmt(&fmt_cfg, fmt);
        reg->cfg0.osd_en = region->enable;
        reg->cfg0.osd_range_trns_en = region->range_trns_en;
        reg->cfg0.osd_range_trns_sel = region->range_trns_sel;
        reg->cfg0.osd_fmt = fmt_cfg.format;
        reg->cfg0.osd_rbuv_swap = region->rbuv_swap;
        reg->cfg1.osd_lt_xcrd = region->lt_x;
        reg->cfg1.osd_lt_ycrd = region->lt_y;
        reg->cfg2.osd_rb_xcrd = region->rb_x;
        reg->cfg2.osd_rb_ycrd = region->rb_y;

        reg->cfg1.osd_endn = region->osd_endn;
        reg->cfg5.osd_stride = region->stride;
        reg->cfg5.osd_ch_ds_mode = region->ch_ds_mode;
        reg->cfg0.osd_alpha_swap = region->alpha_cfg.alpha_swap;
        reg->cfg0.osd_fg_alpha = region->alpha_cfg.fg_alpha;
        reg->cfg0.osd_fg_alpha_sel = region->alpha_cfg.fg_alpha_sel;
        reg->cfg0.osd_qp_adj_en = region->qp_cfg.qp_adj_en;
        reg->cfg8.osd_qp_adj_sel = region->qp_cfg.qp_adj_sel;
        reg->cfg8.osd_qp = region->qp_cfg.qp;
        reg->cfg8.osd_qp_max = region->qp_cfg.qp_max;
        reg->cfg8.osd_qp_min = region->qp_cfg.qp_min;
        reg->cfg8.osd_qp_prj = region->qp_cfg.qp_prj;

        kmpp_obj_get_by_sptr_f(&buffer, &region->osd_buf);
        if (buffer) {
            buf_cfg = kmpp_buffer_to_cfg(buffer);
            kmpp_buf_cfg_get_fd(buf_cfg, (RK_S32 *)&reg->osd_st_addr);
        }
        memcpy(reg->lut, region->lut, sizeof(region->lut));
    }

    regs->osd_whi_cfg0.osd_csc_yr = 77;
    regs->osd_whi_cfg0.osd_csc_yg = 150;
    regs->osd_whi_cfg0.osd_csc_yb = 29;

    regs->osd_whi_cfg1.osd_csc_ur = -43;
    regs->osd_whi_cfg1.osd_csc_ug = -85;
    regs->osd_whi_cfg1.osd_csc_ub = 128;

    regs->osd_whi_cfg2.osd_csc_vr = 128;
    regs->osd_whi_cfg2.osd_csc_vg = -107;
    regs->osd_whi_cfg2.osd_csc_vb = -21;

    regs->osd_whi_cfg3.osd_csc_ofst_y = 0;
    regs->osd_whi_cfg3.osd_csc_ofst_u = 128;
    regs->osd_whi_cfg3.osd_csc_ofst_v = 128;

    return MPP_OK;
}

MPP_RET vepu511_set_roi(Vepu511RoiCfg *roi_reg_base, MppEncROICfg * roi, RK_S32 w, RK_S32 h)
{
    MppEncROIRegion *region = roi->regions;
    Vepu511RoiCfg  *roi_cfg = (Vepu511RoiCfg *)roi_reg_base;
    Vepu511RoiRegion *reg_regions = &roi_cfg->regions[0];
    MPP_RET ret = MPP_NOK;
    RK_S32 i = 0;

    memset(reg_regions, 0, sizeof(Vepu511RoiRegion) * 8);
    if (NULL == roi_cfg || NULL == roi) {
        mpp_err_f("invalid buf %p roi %p\n", roi_cfg, roi);
        goto DONE;
    }

    if (roi->number > VEPU511_MAX_ROI_NUM) {
        mpp_err_f("invalid region number %d\n", roi->number);
        goto DONE;
    }

    /* check region config */
    ret = MPP_OK;
    for (i = 0; i < (RK_S32) roi->number; i++, region++) {
        if (region->x + region->w > w || region->y + region->h > h)
            ret = MPP_NOK;

        if (region->intra > 1 || region->qp_area_idx >= VEPU511_MAX_ROI_NUM ||
            region->area_map_en > 1 || region->abs_qp_en > 1)
            ret = MPP_NOK;

        if ((region->abs_qp_en && region->quality > 51) ||
            (!region->abs_qp_en && (region->quality > 51 || region->quality < -51)))
            ret = MPP_NOK;

        if (ret) {
            mpp_err_f("region %d invalid param:\n", i);
            mpp_err_f("position [%d:%d:%d:%d] vs [%d:%d]\n",
                      region->x, region->y, region->w, region->h, w, h);
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
            reg_regions->reg1063.roi0_mdc0_hevc.mdc_intra16 = 1;
            reg_regions->roi_mdc_hevc.mdc_intra32 = 1;
        }
        reg_regions++;
    }

DONE:
    return ret;
}


