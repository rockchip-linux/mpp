/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "vdpu382_com"

#include <string.h>
#include <stdlib.h>
#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"
#include "mpp_frame_impl.h"
#include "mpp_env.h"

#include "vdpu382_com.h"

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

static RK_S32 update_size_offset(Vdpu382RcbInfo *info, RK_U32 reg,
                                 RK_S32 offset, RK_S32 len, RK_S32 idx)
{
    RK_S32 buf_size = 0;

    buf_size = MPP_ALIGN(len * rcb_coeff[idx], RCB_ALLINE_SIZE);
    info[idx].reg = reg;
    info[idx].offset = offset;
    info[idx].size = buf_size;

    return buf_size;
}

RK_S32 vdpu382_get_rcb_buf_size(Vdpu382RcbInfo *info, RK_S32 width, RK_S32 height)
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

void vdpu382_setup_rcb(Vdpu382RegCommonAddr *reg, MppDev dev, MppBuffer buf, Vdpu382RcbInfo *info)
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

RK_S32 vdpu382_compare_rcb_size(const void *a, const void *b)
{
    RK_S32 val = 0;
    Vdpu382RcbInfo *p0 = (Vdpu382RcbInfo *)a;
    Vdpu382RcbInfo *p1 = (Vdpu382RcbInfo *)b;

    val = (p0->size > p1->size) ? -1 : 1;

    return val;
}

RK_S32 vdpu382_set_rcbinfo(MppDev dev, Vdpu382RcbInfo *rcb_info)
{
    MppDevRcbInfoCfg rcb_cfg;
    RK_U32 i;
    /*
     * RCB_SET_BY_SIZE_SORT_MODE: by size sort
     * RCB_SET_BY_PRIORITY_MODE: by priority
     */
    Vdpu382RcbSetMode_e set_rcb_mode = RCB_SET_BY_PRIORITY_MODE;
    RK_U32 rcb_priority[RCB_BUF_COUNT] = {
        RCB_DBLK_ROW,
        RCB_INTRA_ROW,
        RCB_SAO_ROW,
        RCB_INTER_ROW,
        RCB_FBC_ROW,
        RCB_TRANSD_ROW,
        RCB_STRMD_ROW,
        RCB_INTER_COL,
        RCB_FILT_COL,
        RCB_TRANSD_COL,
    };

    switch (set_rcb_mode) {
    case RCB_SET_BY_SIZE_SORT_MODE : {
        Vdpu382RcbInfo info[RCB_BUF_COUNT];

        memcpy(info, rcb_info, sizeof(info));
        qsort(info, MPP_ARRAY_ELEMS(info),
              sizeof(info[0]), vdpu382_compare_rcb_size);

        for (i = 0; i < MPP_ARRAY_ELEMS(info); i++) {
            rcb_cfg.reg_idx = info[i].reg;
            rcb_cfg.size = info[i].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            } else
                break;
        }
    } break;
    case RCB_SET_BY_PRIORITY_MODE : {
        Vdpu382RcbInfo *info = rcb_info;
        RK_U32 index = 0;

        for (i = 0; i < MPP_ARRAY_ELEMS(rcb_priority); i ++) {
            index = rcb_priority[i];

            rcb_cfg.reg_idx = info[index].reg;
            rcb_cfg.size = info[index].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            }
        }
    } break;
    default:
        break;
    }

    return 0;
}

void vdpu382_setup_statistic(Vdpu382RegCommon *com, Vdpu382RegStatistic *sta)
{
    com->reg011.pix_range_detection_e = 1;

    memset(sta, 0, sizeof(*sta));

    sta->reg256.axi_perf_work_e = 1;
    sta->reg256.axi_perf_clr_e = 1;
    sta->reg256.axi_cnt_type = 1;

    sta->reg257.addr_align_type = 1;

    /* set hurry */
    sta->reg270.axi_rd_hurry_level = 3;
    sta->reg270.axi_wr_hurry_level = 1;
    sta->reg270.axi_wr_qos = 1;
    sta->reg270.axi_rd_qos = 3;
    sta->reg270.bus2mc_buffer_qos_level = 255;
    sta->reg271_wr_wait_cycle_qos = 0;
}

void vdpu382_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand)
{
    RK_U32 ver_stride = 0;
    RK_U32 img_height = mpp_frame_get_height(frame);
    RK_U32 img_width = mpp_frame_get_width(frame);
    RK_U32 hdr_stride = (*compat_ext_fbc_hdr_256_odd) ?
                        (MPP_ALIGN(img_width, 256) | 256) :
                        (MPP_ALIGN(img_width, 64));

    mpp_slots_set_prop(slots, SLOTS_HOR_ALIGN, mpp_align_64);
    mpp_slots_set_prop(slots, SLOTS_VER_ALIGN, mpp_align_16);

    mpp_frame_set_fbc_hdr_stride(frame, hdr_stride);

    ver_stride = mpp_align_16(img_height);
    if (*compat_ext_fbc_buf_size) {
        ver_stride += expand;
    }
    mpp_frame_set_ver_stride(frame, ver_stride);
}

void vdpu382_setup_down_scale(MppFrame frame, MppDev dev, Vdpu382RegCommon *com)
{
    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
    RK_U32 down_scale_ver =  MPP_ALIGN(ver_stride >> 1, 16);
    RK_U32 down_scale_hor =  MPP_ALIGN(hor_stride >> 1, 16);
    MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    MppMeta meta = mpp_frame_get_meta(frame);
    RK_U32 down_scale_y_offset = 0;
    RK_U32 down_scale_uv_offset = 0;

    if (MPP_FRAME_FMT_IS_FBC(fmt))
        down_scale_y_offset = mpp_frame_get_fbc_size(frame);
    else
        down_scale_y_offset = ver_stride * hor_stride * 3 / 2;

    com->reg012.scale_down_en = 1;
    com->reg029.scale_down_y_wratio = 2;
    com->reg029.scale_down_y_hratio = 2;
    com->reg029.scale_down_c_wratio = 2;
    com->reg029.scale_down_c_hratio = 2;
    com->reg030.y_scale_down_hor_stride =  MPP_ALIGN(down_scale_hor, 16) >> 4;
    com->reg031.uv_scale_down_hor_stride = MPP_ALIGN(down_scale_hor, 16) >> 4;

    down_scale_y_offset = MPP_ALIGN(down_scale_y_offset, 16);
    mpp_dev_set_reg_offset(dev, 198, down_scale_y_offset);
    mpp_meta_set_s32(meta, KEY_DEC_TBN_Y_OFFSET, down_scale_y_offset);

    down_scale_uv_offset = down_scale_y_offset + down_scale_hor * down_scale_ver;
    mpp_dev_set_reg_offset(dev, 199, down_scale_uv_offset);
    mpp_meta_set_s32(meta, KEY_DEC_TBN_UV_OFFSET, down_scale_uv_offset);
}
