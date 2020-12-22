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

#include <ctype.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"
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

static MPP_RET read_with_pixel_width(RK_U8 *buf, RK_S32 width, RK_S32 height,
                                     RK_S32 hor_stride, RK_S32 pix_w, FILE *fp)
{
    RK_S32 row;
    MPP_RET ret = MPP_OK;

    if (hor_stride < width * pix_w) {
        mpp_err_f("invalid %dbit color config: hor_stride %d is smaller then width %d multiply by 4\n",
                  8 * pix_w, hor_stride, width, pix_w);
        mpp_err_f("width  should be defined by pixel count\n");
        mpp_err_f("stride should be defined by byte count\n");

        hor_stride = width * pix_w;
    }

    for (row = 0; row < height; row++) {
        RK_S32 read_size = fread(buf + row * hor_stride, 1, width * pix_w, fp);
        if (read_size != width * pix_w) {
            mpp_err_f("read file failed expect %d vs %d\n",
                      width * pix_w, read_size);
            ret = MPP_NOK;
        }
    }

    return ret;
}

MPP_RET read_image(RK_U8 *buf, FILE *fp, RK_U32 width, RK_U32 height,
                   RK_U32 hor_stride, RK_U32 ver_stride, MppFrameFormat fmt)
{
    MPP_RET ret = MPP_OK;
    RK_U32 read_size;
    RK_U32 row = 0;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_u = buf_y + hor_stride * ver_stride; // NOTE: diff from gen_yuv_image
    RK_U8 *buf_v = buf_u + hor_stride * ver_stride / 4; // NOTE: diff from gen_yuv_image

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        RK_U32 align_w = MPP_ALIGN(width, 16);
        RK_U32 align_h = MPP_ALIGN(height, 16);
        RK_U32 header_size = MPP_ALIGN(align_w * align_h / 16, SZ_4K);

        /* read fbc header first */
        read_size = fread(buf, 1, header_size, fp);
        if (read_size != header_size) {
            mpp_err_f("read fbc file header failed %d vs %d\n",
                      read_size, header_size);
            ret  = MPP_NOK;
            goto err;
        }
        buf += header_size;

        switch (fmt & MPP_FRAME_FMT_MASK) {
        case MPP_FMT_YUV420SP : {
            read_size = fread(buf, 1, align_w * align_h * 3 / 2, fp);
            if (read_size != align_w * align_h * 3 / 2) {
                mpp_err_f("read 420sp fbc file payload failed %d vs %d\n",
                          read_size, align_w * align_h * 3 / 2);
                ret  = MPP_NOK;
                goto err;
            }
        } break;
        case MPP_FMT_YUV422SP : {
            read_size = fread(buf, 1, align_w * align_h * 2, fp);
            if (read_size != align_w * align_h * 2) {
                mpp_err_f("read 422sp fbc file payload failed %d vs %d\n",
                          read_size, align_w * align_h * 2);
                ret  = MPP_NOK;
                goto err;
            }
        } break;
        default : {
            mpp_err_f("not supported fbc format %x\n", fmt);
        } break;
        }

        return MPP_OK;
    }

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    case MPP_FMT_YUV420P : {
        for (row = 0; row < height; row++) {
            read_size = fread(buf_y + row * hor_stride, 1, width, fp);
            if (read_size != width) {
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_u + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                ret  = MPP_NOK;
                goto err;
            }
        }

        for (row = 0; row < height / 2; row++) {
            read_size = fread(buf_v + row * hor_stride / 2, 1, width / 2, fp);
            if (read_size != width / 2) {
                ret  = MPP_NOK;
                goto err;
            }
        }
    } break;
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 :
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 : {
        ret = read_with_pixel_width(buf_y, width, height, hor_stride, 4, fp);
    } break;
    case MPP_FMT_YUV422P :
    case MPP_FMT_YUV422SP :
    case MPP_FMT_BGR444 :
    case MPP_FMT_RGB444 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 :
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_YVYU :
    case MPP_FMT_YUV422_UYVY :
    case MPP_FMT_YUV422_VYUY : {
        ret = read_with_pixel_width(buf_y, width, height, hor_stride, 2, fp);
    } break;
    case MPP_FMT_RGB888 :
    case MPP_FMT_BGR888 : {
        ret = read_with_pixel_width(buf_y, width, height, hor_stride, 3, fp);
    } break;
    default : {
        mpp_err_f("read image do not support fmt %d\n", fmt);
        ret = MPP_ERR_VALUE;
    } break;
    }

err:

    return ret;
}

static void fill_MPP_FMT_YUV420SP(RK_U8 *buf, RK_U32 width, RK_U32 height,
                                  RK_U32 hor_stride, RK_U32 ver_stride,
                                  RK_U32 frame_count)
{
    // MPP_FMT_YUV420SP = ffmpeg: nv12
    // https://www.fourcc.org/pixel-format/yuv-nv12/
    RK_U8 *p = buf;
    RK_U32 x, y;

    for (y = 0; y < height; y++, p += hor_stride) {
        for (x = 0; x < width; x++) {
            p[x] = x + y + frame_count * 3;
        }
    }

    p = buf + hor_stride * ver_stride;
    for (y = 0; y < height / 2; y++, p += hor_stride) {
        for (x = 0; x < width / 2; x++) {
            p[x * 2 + 0] = 128 + y + frame_count * 2;
            p[x * 2 + 1] = 64  + x + frame_count * 5;
        }
    }
}

static void fill_MPP_FMT_YUV422SP(RK_U8 *buf, RK_U32 width, RK_U32 height,
                                  RK_U32 hor_stride, RK_U32 ver_stride,
                                  RK_U32 frame_count)
{
    // MPP_FMT_YUV422SP = ffmpeg: nv16
    // not valid in www.fourcc.org
    RK_U8 *p = buf;
    RK_U32 x, y;

    for (y = 0; y < height; y++, p += hor_stride) {
        for (x = 0; x < width; x++) {
            p[x] = x + y + frame_count * 3;
        }
    }

    p = buf + hor_stride * ver_stride;
    for (y = 0; y < height; y++, p += hor_stride) {
        for (x = 0; x < width / 2; x++) {
            p[x * 2 + 0] = 128 + y / 2 + frame_count * 2;
            p[x * 2 + 1] = 64  + x + frame_count * 5;
        }
    }
}

static void get_rgb_color(RK_U32 *R, RK_U32 *G, RK_U32 *B, RK_S32 x, RK_S32 y, RK_S32 frm_cnt)
{
    // frame 0 -> red
    if (frm_cnt == 0) {
        R[0] = 0xff;
        G[0] = 0;
        B[0] = 0;
        return ;
    }

    // frame 1 -> green
    if (frm_cnt == 1) {
        R[0] = 0;
        G[0] = 0xff;
        B[0] = 0;
        return ;
    }

    // frame 2 -> blue
    if (frm_cnt == 2) {
        R[0] = 0;
        G[0] = 0;
        B[0] = 0xff;
        return ;
    }

    // moving color bar
    RK_U8 Y = (0   +  x + y  + frm_cnt * 3);
    RK_U8 U = (128 + (y / 2) + frm_cnt * 2);
    RK_U8 V = (64  + (x / 2) + frm_cnt * 5);

    RK_S32 _R = Y + ((360 * (V - 128)) >> 8);
    RK_S32 _G = Y - (((88 * (U - 128) + 184 * (V - 128))) >> 8);
    RK_S32 _B = Y + ((455 * (U - 128)) >> 8);

    R[0] = MPP_CLIP3(0, 255, _R);
    G[0] = MPP_CLIP3(0, 255, _G);
    B[0] = MPP_CLIP3(0, 255, _B);
}

static void fill_MPP_FMT_RGB565(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGB565 = ffmpeg: rgb565be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (rrrr,rggg,gggb,bbbb)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 3) & 0x1f) << 11) |
                 (((G >> 2) & 0x3f) <<  5) |
                 (((B >> 3) & 0x1f) <<  0);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_BGR565(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGR565 = ffmpeg: bgr565be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (bbbb,bggg,gggr,rrrr)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 3) & 0x1f) <<  0) |
                 (((G >> 2) & 0x3f) <<  5) |
                 (((B >> 3) & 0x1f) << 11);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_RGB555(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGB555 = ffmpeg: rgb555be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (0rrr,rrgg,gggb,bbbb)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 3) & 0x1f) << 10) |
                 (((G >> 3) & 0x1f) <<  5) |
                 (((B >> 3) & 0x1f) <<  0);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_BGR555(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGR555 = ffmpeg: bgr555be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (0bbb,bbgg,gggr,rrrr)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 3) & 0x1f) <<  0) |
                 (((G >> 3) & 0x1f) <<  5) |
                 (((B >> 3) & 0x1f) << 10);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_RGB444(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGB444 = ffmpeg: rgb444be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (0000,rrrr,gggg,bbbb)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 4) & 0xf) << 8) |
                 (((G >> 4) & 0xf) << 4) |
                 (((B >> 4) & 0xf) << 0);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_BGR444(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGR444 = ffmpeg: bgr444be
    // 16 bit pixel     MSB  -------->  LSB
    //                 (0000,bbbb,gggg,rrrr)
    // big    endian   |  byte 0 |  byte 1 |
    // little endian   |  byte 1 |  byte 0 |
    RK_U16 val = (((R >> 4) & 0xf) << 0) |
                 (((G >> 4) & 0xf) << 4) |
                 (((B >> 4) & 0xf) << 8);
    if (be) {
        p[0] = (val >> 8) & 0xff;
        p[1] = (val >> 0) & 0xff;
    } else {
        p[0] = (val >> 0) & 0xff;
        p[1] = (val >> 8) & 0xff;
    }
}

static void fill_MPP_FMT_RGB888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGB888
    // 24 bit pixel     MSB  -------->  LSB
    //                 (rrrr,rrrr,gggg,gggg,bbbb,bbbb)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |
    // little endian   |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = R;
        p[1] = G;
        p[2] = B;
    } else {
        p[0] = B;
        p[1] = G;
        p[2] = R;
    }
}

static void fill_MPP_FMT_BGR888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGR888
    // 24 bit pixel     MSB  -------->  LSB
    //                 (bbbb,bbbb,gggg,gggg,rrrr,rrrr)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |
    // little endian   |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = B;
        p[1] = G;
        p[2] = R;
    } else {
        p[0] = R;
        p[1] = G;
        p[2] = B;
    }
}

static void fill_MPP_FMT_RGB101010(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGB101010
    // 32 bit pixel     MSB  -------->  LSB
    //                 (00rr,rrrr,rrrr,gggg,gggg,ggbb,bbbb,bbbb)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    RK_U32 val = (((R * 4) & 0x3ff) << 20) |
                 (((G * 4) & 0x3ff) << 10) |
                 (((B * 4) & 0x3ff) <<  0);
    if (be) {
        p[0] = (val >> 24) & 0xff;
        p[1] = (val >> 16) & 0xff;
        p[2] = (val >>  8) & 0xff;
        p[3] = (val >>  0) & 0xff;
    } else {
        p[0] = (val >>  0) & 0xff;
        p[1] = (val >>  8) & 0xff;
        p[2] = (val >> 16) & 0xff;
        p[3] = (val >> 24) & 0xff;
    }
}

static void fill_MPP_FMT_BGR101010(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGR101010
    // 32 bit pixel     MSB  -------->  LSB
    //                 (00bb,bbbb,bbbb,gggg,gggg,ggrr,rrrr,rrrr)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    RK_U32 val = (((R * 4) & 0x3ff) <<  0) |
                 (((G * 4) & 0x3ff) << 10) |
                 (((B * 4) & 0x3ff) << 20);
    if (be) {
        p[0] = (val >> 24) & 0xff;
        p[1] = (val >> 16) & 0xff;
        p[2] = (val >>  8) & 0xff;
        p[3] = (val >>  0) & 0xff;
    } else {
        p[0] = (val >>  0) & 0xff;
        p[1] = (val >>  8) & 0xff;
        p[2] = (val >> 16) & 0xff;
        p[3] = (val >> 24) & 0xff;
    }
}

static void fill_MPP_FMT_ARGB8888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_ARGB8888
    // 32 bit pixel     MSB  -------->  LSB
    //                 (XXXX,XXXX,rrrr,rrrr,gggg,gggg,bbbb,bbbb)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = 0xff;
        p[1] = R;
        p[2] = G;
        p[3] = B;
    } else {
        p[0] = B;
        p[1] = G;
        p[2] = R;
        p[3] = 0xff;
    }
}

static void fill_MPP_FMT_ABGR8888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_ABGR8888
    // 32 bit pixel     MSB  -------->  LSB
    //                 (XXXX,XXXX,bbbb,bbbb,gggg,gggg,rrrr,rrrr)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = 0xff;
        p[1] = B;
        p[2] = G;
        p[3] = R;
    } else {
        p[0] = R;
        p[1] = G;
        p[2] = B;
        p[3] = 0xff;
    }
}

static void fill_MPP_FMT_BGRA8888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_BGRA8888
    // 32 bit pixel     MSB  -------->  LSB
    //                 (bbbb,bbbb,gggg,gggg,rrrr,rrrr,XXXX,XXXX)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = B;
        p[1] = G;
        p[2] = R;
        p[3] = 0xff;
    } else {
        p[0] = 0xff;
        p[1] = R;
        p[2] = G;
        p[3] = B;
    }
}

static void fill_MPP_FMT_RGBA8888(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be)
{
    // MPP_FMT_RGBA8888
    // 32 bit pixel     MSB  -------->  LSB
    //                 (rrrr,rrrr,gggg,gggg,bbbb,bbbb,XXXX,XXXX)
    // big    endian   |  byte 0 |  byte 1 |  byte 2 |  byte 3 |
    // little endian   |  byte 3 |  byte 2 |  byte 1 |  byte 0 |
    if (be) {
        p[0] = R;
        p[1] = G;
        p[2] = B;
        p[3] = 0xff;
    } else {
        p[0] = 0xff;
        p[1] = B;
        p[2] = G;
        p[3] = R;
    }
}

typedef void (*FillRgbFunc)(RK_U8 *p, RK_U32 R, RK_U32 G, RK_U32 B, RK_U32 be);

FillRgbFunc fill_rgb_funcs[] = {
    fill_MPP_FMT_RGB565,
    fill_MPP_FMT_BGR565,
    fill_MPP_FMT_RGB555,
    fill_MPP_FMT_BGR555,
    fill_MPP_FMT_RGB444,
    fill_MPP_FMT_BGR444,
    fill_MPP_FMT_RGB888,
    fill_MPP_FMT_BGR888,
    fill_MPP_FMT_RGB101010,
    fill_MPP_FMT_BGR101010,
    fill_MPP_FMT_ARGB8888,
    fill_MPP_FMT_ABGR8888,
    fill_MPP_FMT_BGRA8888,
    fill_MPP_FMT_RGBA8888,
};

static RK_S32 util_check_stride_by_pixel(RK_S32 workaround, RK_S32 width,
                                         RK_S32 hor_stride, RK_S32 pixel_size)
{
    if (!workaround && hor_stride < width * pixel_size) {
        mpp_log("warning: stride by bytes %d is smarller than width %d mutiple by pixel size %d\n",
                hor_stride, width, pixel_size);
        mpp_log("multiple stride %d by pixel size %d and set new byte stride to %d\n",
                hor_stride, pixel_size, hor_stride * pixel_size);
        workaround = 1;
    }

    return workaround;
}

static RK_S32 util_check_8_pixel_aligned(RK_S32 workaround, RK_S32 hor_stride,
                                         RK_S32 pixel_aign, RK_S32 pixel_size,
                                         const char *fmt_name)
{
    if (!workaround && hor_stride != MPP_ALIGN(hor_stride, pixel_aign * pixel_size)) {
        mpp_log("warning: vepu only support 8 aligned horizontal stride in pixel for %s with pixel size %d\n",
                fmt_name, pixel_size);
        mpp_log("set byte stride to %d to match the requirement\n",
                MPP_ALIGN(hor_stride, pixel_aign * pixel_size));
        workaround = 1;
    }

    return workaround;
}

MPP_RET fill_image(RK_U8 *buf, RK_U32 width, RK_U32 height,
                   RK_U32 hor_stride, RK_U32 ver_stride, MppFrameFormat fmt,
                   RK_U32 frame_count)
{
    MPP_RET ret = MPP_OK;
    RK_U8 *buf_y = buf;
    RK_U8 *buf_c = buf + hor_stride * ver_stride;
    RK_U32 x, y, i;
    static RK_S32 is_pixel_stride = 0;
    static RK_S32 not_8_pixel = 0;

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP : {
        fill_MPP_FMT_YUV420SP(buf, width, height, hor_stride, ver_stride, frame_count);
    } break;
    case MPP_FMT_YUV422SP : {
        fill_MPP_FMT_YUV422SP(buf, width, height, hor_stride, ver_stride, frame_count);
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
    case MPP_FMT_YUV420SP_VU : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height / 2; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 2 + 1] = 128 + y + frame_count * 2;
                p[x * 2 + 0] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422P : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 128 + y / 2 + frame_count * 2;
            }
        }

        p = buf_c + hor_stride * ver_stride / 2;
        for (y = 0; y < height; y++, p += hor_stride / 2) {
            for (x = 0; x < width / 2; x++) {
                p[x] = 64 + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422SP_VU : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }

        p = buf_c;
        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 2 + 1] = 128 + y / 2 + frame_count * 2;
                p[x * 2 + 0] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422_YUYV : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 4 + 0] = x * 2 + 0 + y + frame_count * 3;
                p[x * 4 + 2] = x * 2 + 1 + y + frame_count * 3;
                p[x * 4 + 1] = 128 + y / 2 + frame_count * 2;
                p[x * 4 + 3] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422_YVYU : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 4 + 0] = x * 2 + 0 + y + frame_count * 3;
                p[x * 4 + 2] = x * 2 + 1 + y + frame_count * 3;
                p[x * 4 + 3] = 128 + y / 2 + frame_count * 2;
                p[x * 4 + 1] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422_UYVY : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 4 + 1] = x * 2 + 0 + y + frame_count * 3;
                p[x * 4 + 3] = x * 2 + 1 + y + frame_count * 3;
                p[x * 4 + 0] = 128 + y / 2 + frame_count * 2;
                p[x * 4 + 2] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV422_VYUY : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width / 2; x++) {
                p[x * 4 + 1] = x * 2 + 0 + y + frame_count * 3;
                p[x * 4 + 3] = x * 2 + 1 + y + frame_count * 3;
                p[x * 4 + 2] = 128 + y / 2 + frame_count * 2;
                p[x * 4 + 0] = 64  + x + frame_count * 5;
            }
        }
    } break;
    case MPP_FMT_YUV400 : {
        RK_U8 *p = buf_y;

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0; x < width; x++) {
                p[x] = x + y + frame_count * 3;
            }
        }
    } break;
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 : {
        RK_U8 *p = buf_y;
        RK_U32 pix_w = 2;
        FillRgbFunc fill = fill_rgb_funcs[fmt - MPP_FRAME_FMT_RGB];

        if (util_check_stride_by_pixel(is_pixel_stride, width, hor_stride, pix_w)) {
            hor_stride *= pix_w;
            is_pixel_stride = 1;
        }

        if (util_check_8_pixel_aligned(not_8_pixel, hor_stride,
                                       8, pix_w, "16bit RGB")) {
            hor_stride = MPP_ALIGN(hor_stride, 16);
            not_8_pixel = 1;
        }

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0, i = 0; x < width; x++, i += pix_w) {
                RK_U32 R, G, B;

                get_rgb_color(&R, &G, &B, x, y, frame_count);
                fill(p + i, R, G, B, MPP_FRAME_FMT_IS_BE(fmt));
            }
        }
    } break;
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 :
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGBA8888 : {
        RK_U8 *p = buf_y;
        RK_U32 pix_w = 4;
        FillRgbFunc fill = fill_rgb_funcs[fmt - MPP_FRAME_FMT_RGB];

        if (util_check_stride_by_pixel(is_pixel_stride, width, hor_stride, pix_w)) {
            hor_stride *= pix_w;
            is_pixel_stride = 1;
        }

        if (util_check_8_pixel_aligned(not_8_pixel, hor_stride,
                                       8, pix_w, "32bit RGB")) {
            hor_stride = MPP_ALIGN(hor_stride, 32);
            not_8_pixel = 1;
        }

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0, i = 0; x < width; x++, i += pix_w) {
                RK_U32 R, G, B;

                get_rgb_color(&R, &G, &B, x, y, frame_count);
                fill(p + i, R, G, B, MPP_FRAME_FMT_IS_BE(fmt));
            }
        }
    } break;
    case MPP_FMT_BGR888 :
    case MPP_FMT_RGB888 : {
        RK_U8 *p = buf_y;
        RK_U32 pix_w = 3;
        FillRgbFunc fill = fill_rgb_funcs[fmt - MPP_FRAME_FMT_RGB];

        if (util_check_stride_by_pixel(is_pixel_stride, width, hor_stride, pix_w)) {
            hor_stride *= pix_w;
            is_pixel_stride = 1;
        }

        if (util_check_8_pixel_aligned(not_8_pixel, hor_stride,
                                       8, pix_w, "24bit RGB")) {
            hor_stride = MPP_ALIGN(hor_stride, 24);
            not_8_pixel = 1;
        }

        for (y = 0; y < height; y++, p += hor_stride) {
            for (x = 0, i = 0; x < width; x++, i += pix_w) {
                RK_U32 R, G, B;

                get_rgb_color(&R, &G, &B, x, y, frame_count);
                fill(p + i, R, G, B, MPP_FRAME_FMT_IS_BE(fmt));
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

static void get_extension(const char *file_name, char *extension)
{
    size_t length = strlen(file_name);
    size_t ext_len = 0;
    size_t i = 0;
    const char *p = file_name + length - 1;

    while (p >= file_name) {
        if (p[0] == '.') {
            for (i = 0; i < ext_len; i++)
                extension[i] = tolower(p[i + 1]);

            extension[i] = '\0';
            return ;
        }
        ext_len++;
        p--;
    }

    extension[0] = '\0';
}

typedef struct Ext2FrmFmt_t {
    const char      *ext_name;
    MppFrameFormat  format;
} Ext2FrmFmt;

Ext2FrmFmt map_ext_to_frm_fmt[] = {
    {   "yuv420p",              MPP_FMT_YUV420P,                            },
    {   "yuv420sp",             MPP_FMT_YUV420SP,                           },
    {   "yuv422p",              MPP_FMT_YUV422P,                            },
    {   "yuv422sp",             MPP_FMT_YUV422SP,                           },
    {   "yuv422uyvy",           MPP_FMT_YUV422_UYVY,                        },
    {   "yuv422vyuy",           MPP_FMT_YUV422_VYUY,                        },
    {   "yuv422yuyv",           MPP_FMT_YUV422_YUYV,                        },
    {   "yuv422yvyu",           MPP_FMT_YUV422_YVYU,                        },

    {   "abgr8888",             MPP_FMT_ABGR8888,                           },
    {   "argb8888",             MPP_FMT_ARGB8888,                           },
    {   "bgr565",               MPP_FMT_BGR565,                             },
    {   "bgr888",               MPP_FMT_BGR888,                             },
    {   "bgra8888",             MPP_FMT_BGRA8888,                           },
    {   "rgb565",               MPP_FMT_RGB565,                             },
    {   "rgb888",               MPP_FMT_RGB888,                             },
    {   "rgba8888",             MPP_FMT_RGBA8888,                           },

    {   "fbc",                  MPP_FMT_YUV420SP | MPP_FRAME_FBC_AFBC_V1,   },
};

MPP_RET name_to_frame_format(const char *name, MppFrameFormat *fmt)
{
    RK_U32 i;
    MPP_RET ret = MPP_NOK;
    char ext[50];

    get_extension(name, ext);

    for (i = 0; i < MPP_ARRAY_ELEMS(map_ext_to_frm_fmt); i++) {
        Ext2FrmFmt *info = &map_ext_to_frm_fmt[i];

        if (!strcmp(ext, info->ext_name)) {
            *fmt = info->format;
            ret = MPP_OK;
        }
    }

    return ret;
}

typedef struct Ext2Coding_t {
    const char      *ext_name;
    MppCodingType   coding;
} Ext2Coding;

Ext2Coding map_ext_to_coding[] = {
    {   "h264",             MPP_VIDEO_CodingAVC,    },
    {   "264",              MPP_VIDEO_CodingAVC,    },
    {   "avc",              MPP_VIDEO_CodingAVC,    },

    {   "h265",             MPP_VIDEO_CodingHEVC,   },
    {   "265",              MPP_VIDEO_CodingHEVC,   },
    {   "hevc",             MPP_VIDEO_CodingHEVC,   },

    {   "jpg",              MPP_VIDEO_CodingMJPEG,  },
    {   "jpeg",             MPP_VIDEO_CodingMJPEG,  },
    {   "mjpeg",            MPP_VIDEO_CodingMJPEG,  },
};

MPP_RET name_to_coding_type(const char *name, MppCodingType *coding)
{
    RK_U32 i;
    MPP_RET ret = MPP_NOK;
    char ext[50];

    get_extension(name, ext);

    for (i = 0; i < MPP_ARRAY_ELEMS(map_ext_to_coding); i++) {
        Ext2Coding *info = &map_ext_to_coding[i];

        if (!strcmp(ext, info->ext_name)) {
            *coding = info->coding;
            ret = MPP_OK;
        }
    }

    return ret;
}
