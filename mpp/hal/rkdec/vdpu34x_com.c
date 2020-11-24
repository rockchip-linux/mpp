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

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"

#include "vdpu34x_com.h"

#define UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size) \
    do { \
        *offsets++ = offset; \
        *sizes++ = buf_size; \
        offset += buf_size; \
    } while (0)

RK_S32 get_rcb_buf_size(RK_S32 *sizes, RK_S32 *offsets, RK_S32 width, RK_S32 height)
{
    RK_S32 align = SZ_4K;
    RK_S32 offset = 0;
    RK_S32 buf_size;

    mpp_assert(sizes);

    buf_size = MPP_ALIGN(width * RCB_INTRA_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_TRANSD_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(height * RCB_TRANSD_COL_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_STRMD_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_INTER_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(height * RCB_INTER_COL_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_DBLK_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_SAO_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(width * RCB_FBC_ROW_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    buf_size = MPP_ALIGN(height * RCB_FILT_COL_COEF, align);
    UPDATE_SIZE_OFFSET(sizes, offsets, offset, buf_size);

    return offset;
}

void vdpu34x_setup_rcb(Vdpu34xRegCommonAddr *reg, MppBuffer buf, RK_S32 *offset)
{
    RK_S32 fd = mpp_buffer_get_fd(buf);

    reg->reg133_rcb_intra_base          = fd + (offset[0] << 10);
    reg->reg134_rcb_transd_row_base     = fd + (offset[1] << 10);
    reg->reg135_rcb_transd_col_base     = fd + (offset[2] << 10);
    reg->reg136_rcb_streamd_row_base    = fd + (offset[3] << 10);
    reg->reg137_rcb_inter_row_base      = fd + (offset[4] << 10);
    reg->reg138_rcb_inter_col_base      = fd + (offset[5] << 10);
    reg->reg139_rcb_dblk_base           = fd + (offset[6] << 10);
    reg->reg140_rcb_sao_base            = fd + (offset[7] << 10);
    reg->reg141_rcb_fbc_base            = fd + (offset[8] << 10);
    reg->reg142_rcb_filter_col_base     = fd + (offset[9] << 10);
}
