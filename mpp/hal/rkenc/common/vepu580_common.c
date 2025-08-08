/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG  "vepu580_common"

#include "mpp_debug.h"
#include "vepu580_common.h"

MPP_RET vepu580_set_osd(Vepu5xxOsdCfg *cfg)
{
    Vepu580OsdReg *regs = (Vepu580OsdReg *)cfg->reg_base;
    MppDev dev = cfg->dev;
    MppDevRegOffCfgs *reg_cfg = cfg->reg_cfg;
    MppEncOSDPltCfg *plt_cfg = cfg->plt_cfg;
    MppEncOSDData2 osd;

    if (copy2osd2(&osd, cfg->osd_data, cfg->osd_data2))
        return MPP_NOK;

    if (osd.num_region == 0)
        return MPP_OK;

    if (osd.num_region > 8) {
        mpp_err_f("do NOT support more than 8 regions invalid num %d\n",
                  osd.num_region);
        mpp_assert(osd.num_region <= 8);
        return MPP_NOK;
    }

    if (plt_cfg->type == MPP_ENC_OSD_PLT_TYPE_USERDEF) {
        memcpy(regs->plt_data, plt_cfg->plt, sizeof(MppEncOSDPlt));
        regs->reg3074.osd_plt_cks = 1;
        regs->reg3074.osd_plt_typ = VEPU5xx_OSD_PLT_TYPE_USERDEF;
    } else {
        regs->reg3074.osd_plt_cks = 0;
        regs->reg3074.osd_plt_typ = VEPU5xx_OSD_PLT_TYPE_DEFAULT;
    }

    regs->reg3074.osd_e = 0;
    regs->reg3072.osd_lu_inv_en = 0;
    regs->reg3072.osd_ch_inv_en = 0;
    regs->reg3072.osd_lu_inv_msk = 0;
    regs->reg3072.osd_ch_inv_msk = 0;

    RK_U32 num = osd.num_region;
    RK_U32 k = 0;
    MppEncOSDRegion2 *region = osd.region;
    MppEncOSDRegion2 *tmp = region;

    for (k = 0; k < num; k++, tmp++) {
        regs->reg3074.osd_e          |= tmp->enable << k;
        regs->reg3072.osd_lu_inv_en  |= (tmp->inverse) ? (1 << k) : 0;
        regs->reg3072.osd_ch_inv_en  |= (tmp->inverse) ? (1 << k) : 0;

        if (tmp->enable && tmp->num_mb_x && tmp->num_mb_y) {
            Vepu580OsdPos *pos = &regs->osd_pos[k];
            size_t blk_len = tmp->num_mb_x * tmp->num_mb_y * 256;
            RK_S32 fd = -1;
            size_t buf_size = 0;

            pos->osd_lt_x = tmp->start_mb_x;
            pos->osd_lt_y = tmp->start_mb_y;
            pos->osd_rb_x = tmp->start_mb_x + tmp->num_mb_x - 1;
            pos->osd_rb_y = tmp->start_mb_y + tmp->num_mb_y - 1;

            buf_size = mpp_buffer_get_size(tmp->buf);
            fd = mpp_buffer_get_fd(tmp->buf);
            if (fd < 0) {
                mpp_err_f("invalid osd buffer fd %d\n", fd);
                return MPP_NOK;
            }
            regs->osd_addr[k] = fd;

            if (tmp->buf_offset) {
                if (reg_cfg)
                    mpp_dev_multi_offset_update(reg_cfg, VEPU580_OSD_ADDR_IDX_BASE + k, tmp->buf_offset);
                else
                    mpp_dev_set_reg_offset(dev, VEPU580_OSD_ADDR_IDX_BASE + k, tmp->buf_offset);
            }

            /* There should be enough buffer and offset should be 16B aligned */
            if (buf_size < tmp->buf_offset + blk_len ||
                (tmp->buf_offset & 0xf)) {
                mpp_err_f("invalid osd cfg: %d x:y:w:h:off %d:%d:%d:%d:%x size %x\n",
                          k, tmp->start_mb_x, tmp->start_mb_y,
                          tmp->num_mb_x, tmp->num_mb_y, tmp->buf_offset, buf_size);
            }
        }
    }

    SET_OSD_INV_THR(0, regs->reg3073, region);
    SET_OSD_INV_THR(1, regs->reg3073, region);
    SET_OSD_INV_THR(2, regs->reg3073, region);
    SET_OSD_INV_THR(3, regs->reg3073, region);
    SET_OSD_INV_THR(4, regs->reg3073, region);
    SET_OSD_INV_THR(5, regs->reg3073, region);
    SET_OSD_INV_THR(6, regs->reg3073, region);
    SET_OSD_INV_THR(7, regs->reg3073, region);

    return MPP_OK;
}
