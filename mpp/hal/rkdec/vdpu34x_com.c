/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "vdpu34x_com"

#include <string.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"

#include "vdpu34x_com.h"

static RK_U32 rcb_coeff[RCB_BUF_COUNT] = {
    [RCB_INTRA_ROW]     = 6,   /* RCB_INTRA_ROW_COEF */
    [RCB_TRANSD_ROW]    = 1,   /* RCB_TRANSD_ROW_COEF */
    [RCB_TRANSD_COL]    = 1,   /* RCB_TRANSD_COL_COEF */
    [RCB_STRMD_ROW]     = 3,   /* RCB_STRMD_ROW_COEF */
    [RCB_INTER_ROW]     = 6,   /* RCB_INTER_ROW_COEF */
    [RCB_INTER_COL]     = 3,   /* RCB_INTER_COL_COEF */
    [RCB_DBLK_ROW]      = 22,  /* RCB_DBLK_ROW_COEF */
    [RCB_SAO_ROW]       = 6,   /* RCB_SAO_ROW_COEF */
    [RCB_FBC_ROW]       = 11,  /* RCB_FBC_ROW_COEF */
    [RCB_FILT_COL]      = 67,  /* RCB_FILT_COL_COEF */
};

static RK_S32 update_size_offset(Vdpu34xRcbInfo *info, RK_U32 reg,
                                 RK_S32 offset, RK_S32 len, RK_S32 idx)
{
    RK_S32 buf_size = 0;

    buf_size = MPP_ALIGN(len * rcb_coeff[idx], RCB_ALLINE_SIZE);
    info[idx].reg = reg;
    info[idx].offset = offset;
    info[idx].size = buf_size;

    return buf_size;
}

RK_S32 get_rcb_buf_size(Vdpu34xRcbInfo *info, RK_S32 width, RK_S32 height)
{
    RK_S32 offset = 0;

    offset += update_size_offset(info, 139, offset, width, RCB_DBLK_ROW);
    offset += update_size_offset(info, 133, offset, width, RCB_INTRA_ROW);
    offset += update_size_offset(info, 134, offset, width, RCB_TRANSD_ROW);
    offset += update_size_offset(info, 136, offset, width, RCB_STRMD_ROW);
    offset += update_size_offset(info, 137, offset, width, RCB_INTER_ROW);
    offset += update_size_offset(info, 140, offset, width, RCB_SAO_ROW);
    offset += update_size_offset(info, 141, offset, width, RCB_FBC_ROW);
    /* col rcb */
    offset += update_size_offset(info, 135, offset, height, RCB_TRANSD_COL);
    offset += update_size_offset(info, 138, offset, height, RCB_INTER_COL);
    offset += update_size_offset(info, 142, offset, height, RCB_FILT_COL);

    return offset;
}

void vdpu34x_setup_rcb(Vdpu34xRegCommonAddr *reg, MppDev dev, MppBuffer buf, Vdpu34xRcbInfo *info)
{
    MppDevRegOffsetCfg trans_cfg;
    RK_S32 fd = mpp_buffer_get_fd(buf);

    reg->reg139_rcb_dblk_base           = fd;
    reg->reg133_rcb_intra_base          = fd;
    reg->reg134_rcb_transd_row_base     = fd;
    reg->reg136_rcb_streamd_row_base    = fd;
    reg->reg137_rcb_inter_row_base      = fd;
    reg->reg140_rcb_sao_base            = fd;
    reg->reg141_rcb_fbc_base            = fd;
    reg->reg135_rcb_transd_col_base     = fd;
    reg->reg138_rcb_inter_col_base      = fd;
    reg->reg142_rcb_filter_col_base     = fd;

    if (info[RCB_DBLK_ROW].offset) {
        trans_cfg.reg_idx = 139;
        trans_cfg.offset = info[RCB_DBLK_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_INTRA_ROW].offset) {
        trans_cfg.reg_idx = 133;
        trans_cfg.offset = info[RCB_INTRA_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_TRANSD_ROW].offset) {
        trans_cfg.reg_idx = 134;
        trans_cfg.offset = info[RCB_TRANSD_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_STRMD_ROW].offset) {
        trans_cfg.reg_idx = 136;
        trans_cfg.offset = info[RCB_STRMD_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_INTER_ROW].offset) {
        trans_cfg.reg_idx = 137;
        trans_cfg.offset = info[RCB_INTER_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_SAO_ROW].offset) {
        trans_cfg.reg_idx = 140;
        trans_cfg.offset = info[RCB_SAO_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_FBC_ROW].offset) {
        trans_cfg.reg_idx = 141;
        trans_cfg.offset = info[RCB_FBC_ROW].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_TRANSD_COL].offset) {
        trans_cfg.reg_idx = 135;
        trans_cfg.offset = info[RCB_TRANSD_COL].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_INTER_COL].offset) {
        trans_cfg.reg_idx = 138;
        trans_cfg.offset = info[RCB_INTER_COL].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }

    if (info[RCB_FILT_COL].offset) {
        trans_cfg.reg_idx = 142;
        trans_cfg.offset = info[RCB_FILT_COL].offset;
        mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
    }
}

RK_S32 vdpu34x_compare_rcb_size(const void *a, const void *b)
{
    RK_S32 val = 0;
    Vdpu34xRcbInfo *p0 = (Vdpu34xRcbInfo *)a;
    Vdpu34xRcbInfo *p1 = (Vdpu34xRcbInfo *)b;

    val = (p0->size > p1->size) ? -1 : 1;

    return val;
}

void vdpu34x_setup_statistic(Vdpu34xRegCommon *com, Vdpu34xRegStatistic *sta)
{
    com->reg011.pix_range_detection_e = 1;

    memset(sta, 0, sizeof(*sta));

    sta->reg256.axi_perf_work_e = 1;
    sta->reg256.axi_perf_clr_e = 1;
    sta->reg256.axi_cnt_type = 1;

    sta->reg257.addr_align_type = 1;
}
