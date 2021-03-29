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
#define MODULE_TAG "vepu_common"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_frame.h"

#include "vepu_common.h"

static VepuFormatCfg vepu_yuv_cfg[MPP_FMT_YUV_BUTT] = {
    //MPP_FMT_YUV420SP
    { .format = VEPU_FMT_YUV420SEMIPLANAR, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV420SP_10BIT
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422SP
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422SP_10BIT
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV420P
    { .format = VEPU_FMT_YUV420PLANAR, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV420SP_VU
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422P
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422SP_VU
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422_YUYV
    { .format = VEPU_FMT_YUYV422INTERLEAVED, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422_YVYU
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422_UYVY
    { .format = VEPU_FMT_UYVY422INTERLEAVED, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV422_VYUY
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV400
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV440SP
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV411SP
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_YUV444SP
    { .format = VEPU_FMT_BUTT, .r_mask = 0, .g_mask = 0, .b_mask = 0, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
};

static VepuFormatCfg vepu_rgb_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    //MPP_FMT_RGB565, ffmpeg: rgb565be, bin(rrrr,rggg,gggb,bbbb) mem MSB-->LSB(gggb,bbbb,rrrr,rggg)
    { .format = VEPU_FMT_RGB565,    .r_mask = 15, .g_mask = 10, .b_mask =  4, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR565, ffmpeg: bgr565be, bin(bbbb,bggg,gggr,rrrr) mem MSB-->LSB(gggr,rrrr,bbbb,bggg)
    { .format = VEPU_FMT_RGB565,    .r_mask =  4, .g_mask = 10, .b_mask = 15, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_RGB555, ffmpeg: rgb555be, bin(0rrr,rrgg,gggb,bbbb) mem MSB-->LSB(gggb,bbbb,0rrr,rrgg)
    { .format = VEPU_FMT_RGB555,    .r_mask = 14, .g_mask =  9, .b_mask =  4, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR555, ffmpeg: bgr555be, bin(0bbb,bbgg,gggr,rrrr) mem MSB-->LSB(gggr,rrrr,0bbb,bbgg)
    { .format = VEPU_FMT_RGB555,    .r_mask =  4, .g_mask =  9, .b_mask = 14, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_RGB444, ffmpeg: rgb444be, bin(0000,rrrr,gggg,bbbb)
    { .format = VEPU_FMT_RGB444,    .r_mask = 11, .g_mask =  7, .b_mask =  3, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR444, ffmpeg: bgr444be, bin(0000,bbbb,gggg,rrrr)
    { .format = VEPU_FMT_RGB444,    .r_mask =  3, .g_mask =  7, .b_mask = 11, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_RGB888, ffmpeg: rgb24, bin(rrrr,rrrr,gggg,gggg,bbbb,bbbb)
    { .format = VEPU_FMT_BUTT,      .r_mask =  0, .g_mask =  0, .b_mask =  0, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 0, },
    //MPP_FMT_BGR888, ffmpeg: bgr24, bin(bbbb,bbbb,gggg,gggg,rrrr,rrrr)
    { .format = VEPU_FMT_BUTT,      .r_mask =  0, .g_mask =  0, .b_mask =  0, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 0, },
    //MPP_FMT_RGB101010, bin(00rr,rrrr,rrrr,gggg,gggg,ggbb,bbbb,bbbb)
    { .format = VEPU_FMT_RGB101010, .r_mask = 29, .g_mask = 19, .b_mask =  9, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR101010, bin(00bb,bbbb,bbbb,gggg,gggg,ggrr,rrrr,rrrr)
    { .format = VEPU_FMT_RGB101010, .r_mask =  9, .g_mask = 19, .b_mask = 29, .swap_8_in = 1, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_ARGB8888, argb, bin(aaaa,aaaa,rrrr,rrrr,gggg,gggg,bbbb,bbbb)
    { .format = VEPU_FMT_RGB888,    .r_mask = 15, .g_mask = 23, .b_mask = 31, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 1, },
    //MPP_FMT_ABGR8888, ffmepg: rgba, bin(aaaa,aaaa,bbbb,bbbb,gggg,gggg,rrrr,rrrr)
    { .format = VEPU_FMT_RGB888,    .r_mask = 31, .g_mask = 23, .b_mask = 15, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 1, },
    //MPP_FMT_BGRA8888, ffmpeg: bgra, bin(bbbb,bbbb,gggg,gggg,rrrr,rrrr,aaaa,aaaa)
    { .format = VEPU_FMT_RGB888,    .r_mask = 23, .g_mask = 15, .b_mask =  7, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 1, },
    //MPP_FMT_RGBA8888, ffmpeg: rgba, bin(rrrr,rrrr,gggg,gggg,bbbb,bbbb,aaaa,aaaa)
    { .format = VEPU_FMT_RGB888,    .r_mask =  7, .g_mask = 15, .b_mask = 23, .swap_8_in = 0, .swap_16_in = 0, .swap_32_in = 1, },
};
static VepuFormatCfg vepu_rgb_le_cfg[MPP_FMT_RGB_BUTT - MPP_FRAME_FMT_RGB] = {
    //for little endian format
    //MPP_FMT_RGB565LE, ffmpeg: rgb565le, bin(gggb,bbbb,rrrr,rggg)
    { .format = VEPU_FMT_RGB565,    .r_mask = 15, .g_mask = 10, .b_mask =  4, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR565LE, ffmpeg: bgr565le, bin(gggr,rrrr,bbbb,bggg)
    { .format = VEPU_FMT_RGB565,    .r_mask =  4, .g_mask = 10, .b_mask = 15, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_RGB555LE, ffmpeg: rgb555le, bin(gggb,bbbb,0rrr,rrgg)
    { .format = VEPU_FMT_RGB555,    .r_mask = 14, .g_mask =  9, .b_mask =  4, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR555LE, ffmpeg: bgr555le, bin(gggr,rrrr,0bbb,bbgg)
    { .format = VEPU_FMT_RGB555,    .r_mask =  4, .g_mask =  9, .b_mask = 14, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_RGB444LE, ffmpeg: rgb444le, bin(gggg,bbbb,0000,rrrr)
    { .format = VEPU_FMT_RGB444,    .r_mask = 11, .g_mask =  7, .b_mask =  3, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
    //MPP_FMT_BGR444LE, ffmpeg: bgr444le, bin(gggg,rrrr,0000,bbbb)
    { .format = VEPU_FMT_RGB444,    .r_mask =  3, .g_mask =  7, .b_mask = 11, .swap_8_in = 0, .swap_16_in = 1, .swap_32_in = 1, },
};

MPP_RET get_vepu_fmt(VepuFormatCfg *cfg, MppFrameFormat format)
{
    VepuFormatCfg *fmt_cfg = NULL;
    MPP_RET ret = MPP_OK;

    if (MPP_FRAME_FMT_IS_FBC(format)) {
        mpp_err_f("unsupport frame format %x\n", format);
    } else if (MPP_FRAME_FMT_IS_YUV(format)) {
        if (!MPP_FRAME_FMT_IS_LE(format))
            fmt_cfg = &vepu_yuv_cfg[format - MPP_FRAME_FMT_YUV];
    } else if (MPP_FRAME_FMT_IS_RGB(format)) {
        if (MPP_FRAME_FMT_IS_LE(format)) {
            fmt_cfg = &vepu_rgb_le_cfg[(format & MPP_FRAME_FMT_MASK) - MPP_FRAME_FMT_RGB];
        } else
            fmt_cfg = &vepu_rgb_cfg[format - MPP_FRAME_FMT_RGB];
    } else {
        memset(cfg, 0, sizeof(*cfg));
        cfg->format = VEPU_FMT_BUTT;
    }

    if (fmt_cfg && fmt_cfg->format != VEPU_FMT_BUTT) {
        memcpy(cfg, fmt_cfg, sizeof(*cfg));
    } else {
        mpp_err_f("unsupport frame format %x\n", format);
        cfg->format = VEPU_FMT_BUTT;
        ret = MPP_NOK;
    }

    return ret;
}

static RK_S32 check_stride_by_pixel(RK_S32 workaround, RK_S32 width,
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

static RK_S32 check_8_pixel_aligned(RK_S32 workaround, RK_S32 hor_stride,
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

RK_U32 get_vepu_pixel_stride(VepuStrideCfg *cfg, RK_U32 width, RK_U32 stride, MppFrameFormat fmt)
{
    RK_U32 hor_stride = stride;
    RK_U32 pixel_size = 1;

    if (cfg->fmt != fmt) {
        memset(cfg, 0, sizeof(VepuStrideCfg));
        cfg->fmt = fmt;
    }

    if (cfg->stride != stride || cfg->width != width) {
        cfg->not_8_pixel = 0;
        cfg->is_pixel_stride = 0;
        cfg->stride = stride;
        cfg->width = width;
    }

    switch (fmt & MPP_FRAME_FMT_MASK) {
    case MPP_FMT_YUV420SP : {
        if (check_8_pixel_aligned(cfg->not_8_pixel, hor_stride, 8, 1, "YUV420SP")) {
            hor_stride = MPP_ALIGN(hor_stride, 8);
            cfg->not_8_pixel = 1;
        }
    } break;
    case MPP_FMT_YUV420P : {
        if (check_8_pixel_aligned(cfg->not_8_pixel, hor_stride, 16, 1, "YUV420P")) {
            hor_stride = MPP_ALIGN(hor_stride, 8);
            cfg->not_8_pixel = 1;
        }
    } break;
    case MPP_FMT_YUV422_YUYV :
    case MPP_FMT_YUV422_UYVY : {
        if (check_stride_by_pixel(cfg->is_pixel_stride, cfg->width,
                                  hor_stride, 2)) {
            hor_stride *= 2;
            cfg->is_pixel_stride = 1;
        }

        if (check_8_pixel_aligned(cfg->not_8_pixel, hor_stride, 8, 2, "YUV422_interleave")) {
            hor_stride = MPP_ALIGN(hor_stride, 16);
            cfg->not_8_pixel = 1;
        }

        hor_stride /= 2;
        pixel_size = 2;
    } break;
    case MPP_FMT_RGB565 :
    case MPP_FMT_BGR565 :
    case MPP_FMT_RGB555 :
    case MPP_FMT_BGR555 :
    case MPP_FMT_RGB444 :
    case MPP_FMT_BGR444 : {
        if (check_stride_by_pixel(cfg->is_pixel_stride, cfg->width,
                                  hor_stride, 2)) {
            hor_stride *= 2;
            cfg->is_pixel_stride = 1;
        }

        if (check_8_pixel_aligned(cfg->not_8_pixel, hor_stride, 8, 2, "32bit RGB")) {
            hor_stride = MPP_ALIGN(hor_stride, 16);
            cfg->not_8_pixel = 1;
        }

        hor_stride /= 2;
        pixel_size = 2;
    } break;
    case MPP_FMT_ARGB8888 :
    case MPP_FMT_ABGR8888 :
    case MPP_FMT_RGBA8888 :
    case MPP_FMT_BGRA8888 :
    case MPP_FMT_RGB101010 :
    case MPP_FMT_BGR101010 : {
        if (check_stride_by_pixel(cfg->is_pixel_stride, cfg->width,
                                  hor_stride, 4)) {
            hor_stride *= 4;
            cfg->is_pixel_stride = 1;
        }

        if (check_8_pixel_aligned(cfg->not_8_pixel, hor_stride, 8, 4, "16bit RGB")) {
            hor_stride = MPP_ALIGN(hor_stride, 32);
            cfg->not_8_pixel = 1;
        }

        hor_stride /= 4;
        pixel_size = 4;
    } break;
    default: {
        mpp_err_f("invalid fmt %d", fmt);
    }
    }
    cfg->pixel_stride = hor_stride;
    cfg->pixel_size = pixel_size;

    return hor_stride;
}

MPP_RET get_vepu_offset_cfg(VepuOffsetCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    MppFrameFormat fmt  = cfg->fmt;
    RK_U32 hor_stride   = cfg->hor_stride;
    RK_U32 ver_stride   = cfg->ver_stride;
    RK_U32 offset_x     = cfg->offset_x;
    RK_U32 offset_y     = cfg->offset_y;
    RK_U32 offset_c     = hor_stride * ver_stride;

    memset(cfg->offset_byte, 0, sizeof(cfg->offset_byte));
    memset(cfg->offset_pixel, 0, sizeof(cfg->offset_pixel));

    if (offset_x || offset_y) {
        switch (fmt) {
        case MPP_FMT_YUV420SP : {
            cfg->offset_byte[0] = offset_y * hor_stride + offset_x;
            cfg->offset_byte[1] = offset_y / 2 * hor_stride + offset_x + offset_c;
        } break;
        case MPP_FMT_YUV420P : {
            cfg->offset_byte[0] = offset_y * hor_stride + offset_x;
            cfg->offset_byte[1] = (offset_y / 2) * (hor_stride / 2) + (offset_x / 2) + offset_c;
            cfg->offset_byte[2] = (offset_y / 2) * (hor_stride / 2) + (offset_x / 2) + offset_c * 5 / 4;
        } break;
        case MPP_FMT_YUV422_YUYV :
        case MPP_FMT_YUV422_UYVY : {
            mpp_assert((offset_x & 1) == 0);
            cfg->offset_byte[0] = offset_y * hor_stride + offset_x * 4;
        } break;
        case MPP_FMT_RGB565 :
        case MPP_FMT_BGR565 :
        case MPP_FMT_RGB555 :
        case MPP_FMT_BGR555 :
        case MPP_FMT_RGB444 :
        case MPP_FMT_BGR444 : {
            cfg->offset_byte[0] = offset_y * hor_stride + offset_x * 2;
        } break;
        case MPP_FMT_RGB101010 :
        case MPP_FMT_BGR101010 :
        case MPP_FMT_ARGB8888 :
        case MPP_FMT_ABGR8888 :
        case MPP_FMT_BGRA8888 :
        case MPP_FMT_RGBA8888 : {
            cfg->offset_byte[0] = offset_y * hor_stride + offset_x * 4;
        } break;
        default : {
        } break;
        }
    } else {
        switch (fmt) {
        case MPP_FMT_YUV420SP :
        case MPP_FMT_YUV420P : {
            RK_U32 offset = hor_stride * ver_stride;

            cfg->offset_byte[1] = offset;

            if (fmt == MPP_FMT_YUV420P)
                offset = hor_stride * ver_stride * 5 / 4;

            cfg->offset_byte[2] = offset;
        } break;
        default : {
        } break;
        }
    }

    return ret;
}
