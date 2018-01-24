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

void calc_data_crc(RK_U8 *dat, RK_U32 len, DataCrc *crc)
{
    RK_U32 i = 0;
    RK_U8 *dat8 = NULL;
    RK_U32 *dat32 = NULL;
    RK_U32 sum = 0, xor = 0;

    /*calc sum */
    dat8 = dat;
    for (i = 0; i < len; i++)
        sum += dat8[i];

    /*calc xor */
    dat32 = (RK_U32 *)dat;
    for (i = 0; i < len / 4; i++)
        xor ^= dat32[i];

    if (len % 4) {
        RK_U32 val = 0;
        dat8 = (RK_U8 *)&val;
        for (i = (len / 4) * 4; i < len; i++)
            dat8[i % 4] = dat[i];
        xor ^= val;
    }

    crc->len = len;
    crc->sum = sum;
    crc->xor = xor;
}

void write_data_crc(FILE *fp, DataCrc *crc)
{
    if (fp) {
        fprintf(fp, "%d, %08x, %08x\n", crc->len, crc->sum, crc->xor);
        fflush(fp);
    }
}

void read_data_crc(FILE *fp, DataCrc *crc)
{
    if (fp) {
        RK_S32 ret = 0;
        ret = fscanf(fp, "%d, %08x, %08x\n", &crc->len, &crc->sum, &crc->xor);
        if (ret == EOF)
            mpp_err_f("unexpected EOF found\n");
    }
}

void calc_frm_crc(MppFrame frame, FrmCrc *crc)
{
    RK_U32 y = 0, x = 0;
    RK_U8 *dat8 = NULL;
    RK_U32 *dat32 = NULL;
    RK_U32 sum = 0, xor = 0;

    RK_U32 width  = mpp_frame_get_width(frame);
    RK_U32 height = mpp_frame_get_height(frame);
    RK_U32 stride = mpp_frame_get_hor_stride(frame);
    RK_U8 *buf = (RK_U8 *)mpp_buffer_get_ptr(mpp_frame_get_buffer(frame));

    /* luma */
    dat8 = buf;
    for (y = 0; y < height; y++)
        for (x = 0; x < width; x++)
            sum += dat8[y * stride + x];

    for (y = 0; y < height; y++) {
        dat32 = (RK_U32 *)&dat8[y * stride];
        for (x = 0; x < width / 4; x++)
            xor ^= dat32[x];
    }
    crc->luma.len = height * width;
    crc->luma.sum = sum;
    crc->luma.xor = xor;

    /* chroma */
    dat8 = buf + height * stride;
    for (y = 0; y < height / 2; y++)
        for (x = 0; x < width; x++)
            sum += dat8[y * stride + x];

    for (y = 0; y < height / 2; y++) {
        dat32 = (RK_U32 *)&dat8[y * stride];
        for (x = 0; x < width / 4; x++)
            xor ^= dat32[x];
    }
    crc->chroma.len = height * width / 2;
    crc->chroma.sum = sum;
    crc->chroma.xor = xor;
}

void write_frm_crc(FILE *fp, FrmCrc *crc)
{
    if (fp) {
        fprintf(fp, "%d, %08x, %08x, %d, %08x, %08x\n",
                crc->luma.len, crc->luma.sum, crc->luma.xor,
                crc->chroma.len, crc->chroma.sum, crc->chroma.xor);
        fflush(fp);
    }
}

void read_frm_crc(FILE *fp, FrmCrc *crc)
{
    if (fp) {
        RK_S32 ret = 0;
        ret = fscanf(fp, "%d, %08x, %08x, %d, %08x, %08x\n",
                     &crc->luma.len, &crc->luma.sum, &crc->luma.xor,
                     &crc->chroma.len, &crc->chroma.sum, &crc->chroma.xor);
        if (ret == EOF)
            mpp_err_f("unexpected EOF found\n");
    }
}


