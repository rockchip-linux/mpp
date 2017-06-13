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

#define MODULE_TAG "utils"

#include <string.h>

#include "mpp_log.h"
#include "utils.h"

void _show_options(int count, OptionInfo *options)
{
    int i;
    for (i = 0; i < count; i++) {
        mpp_log("-%s  %-16s\t%s\n",
                options[i].name, options[i].argname, options[i].help);
    }
}

void dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width    = 0;
    RK_U32 height   = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;
    MppFrameFormat fmt  = MPP_FMT_YUV420SP;
    MppBuffer buffer    = NULL;
    RK_U8 *base = NULL;

    if (NULL == fp || NULL == frame)
        return ;

    width    = mpp_frame_get_width(frame);
    height   = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    fmt      = mpp_frame_get_fmt(frame);
    buffer   = mpp_frame_get_buffer(frame);

    if (NULL == buffer)
        return ;

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride) {
            fwrite(base_y, 1, width, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride) {
            fwrite(base_c, 1, width, fp);
        }
    } break;
    default : {
        mpp_err("not supported format %d\n", fmt);
    } break;
    }
}

void calc_frm_checksum(MppFrame frame, RK_U8 *sum)
{
    RK_U32 checksum = 0;
    RK_U8 xor_mask;
    RK_U32 y, x;

    RK_U32 width  = mpp_frame_get_width(frame);
    RK_U32 height = mpp_frame_get_height(frame);
    RK_U32 stride = mpp_frame_get_hor_stride(frame);
    RK_U8 *buf = (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame));

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            xor_mask = (x & 0xff) ^ (y & 0xff) ^ (x >> 8) ^ (y >> 8);
            checksum = (checksum + ((buf[y * stride + x] & 0xff) ^ xor_mask)) & 0xffffffff;
        }
    }

    sum[0] = (checksum >> 24) & 0xff;
    sum[1] = (checksum >> 16) & 0xff;
    sum[2] = (checksum >> 8)  & 0xff;
    sum[3] =  checksum      & 0xff;
}

void write_checksum(FILE *fp, RK_U8 *sum)
{
    RK_S32 i;

    for (i = 0; i < 16; i++)
        fprintf(fp, "%02hhx", sum[i]);

    fprintf(fp, "\n");
}

void read_checksum(FILE *fp, RK_U8 *sum)
{
    RK_S32 i;

    for (i = 0; i < 16; i++)
        fscanf(fp, "%02hhx", sum + i);

    fscanf(fp, "\n");
}

