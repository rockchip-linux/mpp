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
    [RCB_INTRA_ROW] = 6,    /* RCB_INTRA_ROW_COEF */
    [RCB_TRANSD_ROW] = 1,   /* RCB_TRANSD_ROW_COEF */
    [RCB_TRANSD_COL] = 1,   /* RCB_TRANSD_COL_COEF */
    [RCB_STRMD_ROW] = 3,    /* RCB_STRMD_ROW_COEF */
    [RCB_INTER_ROW] = 6,    /* RCB_INTER_ROW_COEF */
    [RCB_INTER_COL] = 3,    /* RCB_INTER_COL_COEF */
    [RCB_DBLK_ROW] = 22,    /* RCB_DBLK_ROW_COEF */
    [RCB_SAO_ROW] = 6,      /* RCB_SAO_ROW_COEF */
    [RCB_FBC_ROW] = 11,     /* RCB_FBC_ROW_COEF */
    [RCB_FILT_COL] = 67,    /* RCB_FILT_COL_COEF */
};

#define UPDATE_SIZE_OFFSET(regs, reg, sizes, offsets, offset, len, idx) \
    do { \
        RK_S32 buf_size = MPP_ALIGN(len * rcb_coeff[idx], RCB_ALLINE_SIZE);\
        regs[idx] = reg; \
        offsets[idx] = offset; \
        sizes[idx] = buf_size; \
        offset += buf_size; \
    } while (0)

RK_S32 get_rcb_buf_size(RK_S32 *regs, RK_S32 *sizes,
                        RK_S32 *offsets, RK_S32 width, RK_S32 height)
{
    RK_S32 offset = 0;

    mpp_assert(sizes);

    UPDATE_SIZE_OFFSET(regs, 139, sizes, offsets, offset, width, RCB_DBLK_ROW);
    UPDATE_SIZE_OFFSET(regs, 133, sizes, offsets, offset, width, RCB_INTRA_ROW);
    UPDATE_SIZE_OFFSET(regs, 134, sizes, offsets, offset, width, RCB_TRANSD_ROW);
    UPDATE_SIZE_OFFSET(regs, 136, sizes, offsets, offset, width, RCB_STRMD_ROW);
    UPDATE_SIZE_OFFSET(regs, 137, sizes, offsets, offset, width, RCB_INTER_ROW);
    UPDATE_SIZE_OFFSET(regs, 140, sizes, offsets, offset, width, RCB_SAO_ROW);
    UPDATE_SIZE_OFFSET(regs, 141, sizes, offsets, offset, width, RCB_FBC_ROW);
    /* col rcb */
    UPDATE_SIZE_OFFSET(regs, 135, sizes, offsets, offset, height, RCB_TRANSD_COL);
    UPDATE_SIZE_OFFSET(regs, 138, sizes, offsets, offset, height, RCB_INTER_COL);
    UPDATE_SIZE_OFFSET(regs, 142, sizes, offsets, offset, height, RCB_FILT_COL);

    return offset;
}

void vdpu34x_setup_rcb(Vdpu34xRegCommonAddr *reg, MppBuffer buf, RK_S32 *offset)
{
    RK_S32 fd = mpp_buffer_get_fd(buf);

    reg->reg139_rcb_dblk_base           = fd + (offset[RCB_DBLK_ROW] << 10);
    reg->reg133_rcb_intra_base          = fd + (offset[RCB_INTRA_ROW] << 10);
    reg->reg134_rcb_transd_row_base     = fd + (offset[RCB_TRANSD_ROW] << 10);
    reg->reg136_rcb_streamd_row_base    = fd + (offset[RCB_STRMD_ROW] << 10);
    reg->reg137_rcb_inter_row_base      = fd + (offset[RCB_INTER_ROW] << 10);
    reg->reg140_rcb_sao_base            = fd + (offset[RCB_SAO_ROW] << 10);
    reg->reg141_rcb_fbc_base            = fd + (offset[RCB_FBC_ROW] << 10);
    reg->reg135_rcb_transd_col_base     = fd + (offset[RCB_TRANSD_COL] << 10);
    reg->reg138_rcb_inter_col_base      = fd + (offset[RCB_INTER_COL] << 10);
    reg->reg142_rcb_filter_col_base     = fd + (offset[RCB_FILT_COL] << 10);
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
