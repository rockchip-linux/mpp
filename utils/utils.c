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

#include "mpp_mem.h"
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
    case MPP_FMT_YUV422SP : {
        /* YUV422SP -> YUV422P for better display */
        RK_U32 i, j;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
        RK_U8 *tmp_u = tmp;
        RK_U8 *tmp_v = tmp + width * height / 2;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        for (i = 0; i < height; i++, base_c += h_stride) {
            for (j = 0; j < width / 2; j++) {
                tmp_u[j] = base_c[2 * j + 0];
                tmp_v[j] = base_c[2 * j + 1];
            }
            tmp_u += width / 2;
            tmp_v += width / 2;
        }

        fwrite(tmp, 1, width * height, fp);
        mpp_free(tmp);
    } break;
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
    case MPP_FMT_YUV420P : {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride) {
            fwrite(base_y, 1, width, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride / 2) {
            fwrite(base_c, 1, width / 2, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride / 2) {
            fwrite(base_c, 1, width / 2, fp);
        }
    } break;
    case MPP_FMT_YUV444SP : {
        /* YUV444SP -> YUV444P for better display */
        RK_U32 i, j;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
        RK_U8 *tmp_u = tmp;
        RK_U8 *tmp_v = tmp + width * height;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        for (i = 0; i < height; i++, base_c += h_stride * 2) {
            for (j = 0; j < width; j++) {
                tmp_u[j] = base_c[2 * j + 0];
                tmp_v[j] = base_c[2 * j + 1];
            }
            tmp_u += width;
            tmp_v += width;
        }

        fwrite(tmp, 1, width * height * 2, fp);
        mpp_free(tmp);
    } break;
    case MPP_FMT_YUV400: {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height);

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        mpp_free(tmp);
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
    crc->vor = xor;
}

void write_data_crc(FILE *fp, DataCrc *crc)
{
    if (fp) {
        fprintf(fp, "%d, %08x, %08x\n", crc->len, crc->sum, crc->vor);
        fflush(fp);
    }
}

void read_data_crc(FILE *fp, DataCrc *crc)
{
    if (fp) {
        RK_S32 ret = 0;
        ret = fscanf(fp, "%d, %08x, %08x\n", &crc->len, &crc->sum, &crc->vor);
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
    crc->luma.vor = xor;

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
    crc->chroma.vor = xor;
}

void write_frm_crc(FILE *fp, FrmCrc *crc)
{
    if (fp) {
        fprintf(fp, "%d, %08x, %08x, %d, %08x, %08x\n",
                crc->luma.len, crc->luma.sum, crc->luma.vor,
                crc->chroma.len, crc->chroma.sum, crc->chroma.vor);
        fflush(fp);
    }
}

void read_frm_crc(FILE *fp, FrmCrc *crc)
{
    if (fp) {
        RK_S32 ret = 0;
        ret = fscanf(fp, "%d, %08x, %08x, %d, %08x, %08x\n",
                     &crc->luma.len, &crc->luma.sum, &crc->luma.vor,
                     &crc->chroma.len, &crc->chroma.sum, &crc->chroma.vor);
        if (ret == EOF)
            mpp_err_f("unexpected EOF found\n");
    }
}

MPP_RET read_yuv_image(RK_U8 *buf, FILE *fp, RK_U32 width, RK_U32 height,
                       RK_U32 hor_stride, RK_U32 ver_stride, MppFrameFormat fmt)
{
    MPP_RET ret = MPP_OK;
    RK_U32 read_size;
    RK_U32 row = 0;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_u = buf_y + hor_stride * ver_stride; // NOTE: diff from gen_yuv_image
    RK_U8 *buf_v = buf_u + hor_stride * ver_stride / 4; // NOTE: diff from gen_yuv_image

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file luma failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file cb failed");
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    case MPP_FMT_YUV420P : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                mpp_err_f("read ori yuv file luma failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                mpp_err_f("read ori yuv file cb failed");
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_v + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                mpp_err_f("read ori yuv file cr failed");
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    case MPP_FMT_ARGB8888 : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride * 4, 1, width * 4, fp);
        }
    } break;
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_UYVY : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width * 2, fp);
        }
    } break;
    default : {
        mpp_err_f("read image do not support fmt %d\n", fmt);
        ret = MPP_ERR_VALUE;
    } break;
    }

err:

    return ret;
}

MPP_RET fill_yuv_image(RK_U8 *buf, RK_U32 width, RK_U32 height,
                       RK_U32 hor_stride, RK_U32 ver_stride, MppFrameFormat fmt,
                       RK_U32 frame_count)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_c = buf + hor_stride * ver_stride;
    RK_U32 x, y;

    switch (fmt) {
    case MPP_FMT_YUV420SP : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height / 2; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 2 + 0] = 128 + y + frame_count * 2;
                p[x * 2 + 1] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV420P : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height / 2; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 128 + y + frame_count * 2;
            }
        }

        p = buf_c + hor_stride * ver_stride / 4;
        for (y = 0; y < height / 2; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 64 + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422_UYVY : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 4 + 1] = x * 2 + 0 + y + frame_count * 3;
                p[x * 4 + 3] = x * 2 + 1 + y + frame_count * 3;
                p[x * 4 + 0] = 128 + y + frame_count * 2;
                p[x * 4 + 2] = 64  + x + frame_count * 5;
            }
        }
    } break;
    default : {
        mpp_err_f("filling function do not support type %d\n", fmt);
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

RK_S32 parse_config_line(const char *str, OpsLine *info)
{
    RK_S32 cnt = sscanf(str, "%*[^,],%d,%[^,],%llu,%llu\n",
                        &info->index, info->cmd,
                        &info->value1, &info->value2);

    return cnt;
}
