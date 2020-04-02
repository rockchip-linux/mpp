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

#define MODULE_TAG "mpi_enc_utils"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpi_enc_utils.h"

MPP_RET mpi_enc_gen_osd_data(MppEncOSDData *osd_data, MppBuffer osd_buf, RK_U32 frame_cnt)
{
    RK_U32 k = 0;
    RK_U32 buf_size = 0;
    RK_U32 buf_offset = 0;
    RK_U8 *buf = mpp_buffer_get_ptr(osd_buf);

    osd_data->num_region = 8;
    osd_data->buf = osd_buf;

    for (k = 0; k < osd_data->num_region; k++) {
        MppEncOSDRegion *region = &osd_data->region[k];
        RK_U8 idx = k;

        region->enable = 1;
        region->inverse = frame_cnt & 1;
        region->start_mb_x = k * 3;
        region->start_mb_y = k * 2;
        region->num_mb_x = 2;
        region->num_mb_y = 2;

        buf_size = region->num_mb_x * region->num_mb_y * 256;
        buf_offset = k * buf_size;
        osd_data->region[k].buf_offset = buf_offset;

        memset(buf + buf_offset, idx, buf_size);
    }

    return MPP_OK;
}

MPP_RET mpi_enc_gen_osd_plt(MppEncOSDPlt *osd_plt, RK_U32 *table)
{
    RK_U32 k = 0;

    if (osd_plt->data && table) {
        for (k = 0; k < 256; k++)
            osd_plt->data[k].val = table[k % 8];
    }
    return MPP_OK;
}

