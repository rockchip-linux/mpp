/*
 *
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

#ifndef __JPEGD_PARSER_H__
#define __JPEGD_PARSER_H__

#include "mpp_bitread.h"
#include "mpp_common.h"
#include "mpp_frame.h"
#include "limits.h"
#include <string.h>
#include <mpp_mem.h>
#include "mpp_dec.h"
#include "mpp_buf_slot.h"
#include "mpp_packet.h"

#include "jpegd_syntax.h"

/* Max amount of stream */
#define DEC_RK70_MAX_STREAM         ((1<<24)-1)

#define JPEGDEC_RK70_MIN_BUFFER 256//5120
#define JPEGDEC_RK70_MAX_BUFFER 16776960
#define JPEGDEC_MAX_SLICE_SIZE 4096
#define JPEGDEC_TABLE_SIZE 544
#define JPEGDEC_MIN_WIDTH 48
#define JPEGDEC_MIN_HEIGHT 48
#define JPEGDEC_MAX_WIDTH 4672
#define JPEGDEC_MAX_HEIGHT 4672
#define JPEGDEC_MAX_PIXEL_AMOUNT 16370688
#define JPEGDEC_MAX_WIDTH_8190 8176
#define JPEGDEC_MAX_HEIGHT_8190 8176
#define JPEGDEC_MAX_PIXEL_AMOUNT_8190 66846976
#define JPEGDEC_MAX_SLICE_SIZE_8190 8100
#define JPEGDEC_MAX_WIDTH_TN 256
#define JPEGDEC_MAX_HEIGHT_TN 256

//#define JPEGDEC_BASELINE_TABLE_SIZE 544
#define JPEGDEC_PROGRESSIVE_TABLE_SIZE 576
#define JPEGDEC_QP_BASE 32
#define JPEGDEC_AC1_BASE 48
#define JPEGDEC_AC2_BASE 88
#define JPEGDEC_DC1_BASE 129
#define JPEGDEC_DC2_BASE 132
#define JPEGDEC_DC3_BASE 135

#define PP_IN_FORMAT_YUV422INTERLAVE            0
#define PP_IN_FORMAT_YUV420SEMI                 1
#define PP_IN_FORMAT_YUV420PLANAR               2
#define PP_IN_FORMAT_YUV400                     3
#define PP_IN_FORMAT_YUV422SEMI                 4
#define PP_IN_FORMAT_YUV420SEMITIELED           5
#define PP_IN_FORMAT_YUV440SEMI                 6
#define PP_IN_FORMAT_YUV444_SEMI                7
#define PP_IN_FORMAT_YUV411_SEMI                8

#define PP_OUT_FORMAT_RGB565                    0
#define PP_OUT_FORMAT_ARGB                       1
#define PP_OUT_FORMAT_YUV422INTERLAVE    3
#define PP_OUT_FORMAT_YUV420INTERLAVE    5

/* progressive */
#define JPEGDEC_COEFF_SIZE 96

/* Timeout value for the VPUWaitHwReady() call. */
/* Set to -1 for an unspecified value */
#ifndef DEC_RK70_TIMEOUT_LENGTH
#define DEC_RK70_TIMEOUT_LENGTH     (-1)
#endif

enum {
    JPEGDEC_NO_UNITS = 0,   /* No units, X and Y specify
                                 * the pixel aspect ratio    */
    JPEGDEC_DOTS_PER_INCH = 1,  /* X and Y are dots per inch */
    JPEGDEC_DOTS_PER_CM = 2 /* X and Y are dots per cm   */
};

enum {
    JPEGDEC_THUMBNAIL_JPEG = 0x10,
    JPEGDEC_THUMBNAIL_NOT_SUPPORTED_FORMAT = 0x11,
    JPEGDEC_NO_THUMBNAIL = 0x12
};

enum {
    JPEGDEC_IMAGE = 0,
    JPEGDEC_THUMBNAIL = 1
};

enum {
    SOF0 = 0xC0,
    SOF1 = 0xC1,
    SOF2 = 0xC2,
    SOF3 = 0xC3,
    SOF5 = 0xC5,
    SOF6 = 0xC6,
    SOF7 = 0xC7,
    SOF9 = 0xC8,
    SOF10 = 0xCA,
    SOF11 = 0xCB,
    SOF13 = 0xCD,
    SOF14 = 0xCE,
    SOF15 = 0xCF,
    JPG = 0xC8,
    DHT = 0xC4,
    DAC = 0xCC,
    SOI = 0xD8,
    EOI = 0xD9,
    SOS = 0xDA,
    DQT = 0xDB,
    DNL = 0xDC,
    DRI = 0xDD,
    DHP = 0xDE,
    EXP = 0xDF,
    APP0 = 0xE0,
    APP1 = 0xE1,
    APP2 = 0xE2,
    APP3 = 0xE3,
    APP4 = 0xE4,
    APP5 = 0xE5,
    APP6 = 0xE6,
    APP7 = 0xE7,
    APP8 = 0xE8,
    APP9 = 0xE9,
    APP10 = 0xEA,
    APP11 = 0xEB,
    APP12 = 0xEC,
    APP13 = 0xED,
    APP14 = 0xEE,
    APP15 = 0xEF,
    JPG0 = 0xF0,
    JPG1 = 0xF1,
    JPG2 = 0xF2,
    JPG3 = 0xF3,
    JPG4 = 0xF4,
    JPG5 = 0xF5,
    JPG6 = 0xF6,
    JPG7 = 0xF7,
    JPG8 = 0xF8,
    JPG9 = 0xF9,
    JPG10 = 0xFA,
    JPG11 = 0xFB,
    JPG12 = 0xFC,
    JPG13 = 0xFD,
    COM = 0xFE,
    TEM = 0x01,
    RST0 = 0xD0,
    RST1 = 0xD1,
    RST2 = 0xD2,
    RST3 = 0xD3,
    RST4 = 0xD4,
    RST5 = 0xD5,
    RST6 = 0xD6,
    RST7 = 0xD7
};

typedef struct JpegParserContext {
    MppBufSlots packet_slots;
    MppBufSlots frame_slots;
    RK_S32      frame_slot_index; /* slot index for output */
    RK_U8 *recv_buffer;
    JpegSyntaxParam *pSyntax;
    JpegDecImageInfo imageInfo;

    RK_U32 streamLength;   /* input stream length or buffer size */
    RK_U32 bufferSize; /* input stream buffer size */
    RK_U32 decImageType;   /* Full image or Thumbnail to be decoded */
    RK_U32 sliceMbSet; /* slice mode: mcu rows to decode */
    RK_U32 color_conv;
    RK_U32 dri_en;

    MppPacket input_packet;
    MppFrame output_frame;
    RK_U32 is8190;
    RK_U32 fuseBurned;
    RK_U32 minSupportedWidth;
    RK_U32 minSupportedHeight;
    RK_U32 maxSupportedWidth;
    RK_U32 maxSupportedHeight;
    RK_U32 maxSupportedPixelAmount;
    RK_U32 maxSupportedSliceSize;
    RK_U32 extensionsSupported;

    RK_S64 pts;
    RK_U32 eos;
    RK_U32 parser_debug_enable;
    RK_U32 input_jpeg_count;
} JpegParserContext;

#endif /* __JPEGD_PARSER_H__ */
