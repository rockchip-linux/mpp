/*
 *
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
/*
   * File:
   * ppOp.h
   * Description:
   * Global struct definition in VPU module
   * Author:
   *     Wu JunMin
   * Date:
   *    2015-03-31 14:43:40
 */
#ifndef _PPOP_H_
#define _PPOP_H_

#ifdef ANDROID
#include <sys/types.h>

typedef int32_t     status_t;

namespace android
{

#define PP_IN_FORMAT_YUV422INTERLAVE                0
#define PP_IN_FORMAT_YUV420SEMI                     1
#define PP_IN_FORMAT_YUV420PLANAR                   2
#define PP_IN_FORMAT_YUV400                         3
#define PP_IN_FORMAT_YUV422SEMI                     4
#define PP_IN_FORMAT_YUV420SEMITIELED               5
#define PP_IN_FORMAT_YUV440SEMI                     6
#define PP_IN_FORMAT_YUV444_SEMI                    7
#define PP_IN_FORMAT_YUV411_SEMI                    8

#define PP_OUT_FORMAT_RGB565                        0
#define PP_OUT_FORMAT_ARGB                          1
#define PP_OUT_FORMAT_ABGR                          2
#define PP_OUT_FORMAT_YUV422INTERLAVE               3
#define PP_OUT_FORMAT_YUV420INTERLAVE               5

#define PP_ROTATION_NONE                                0U
#define PP_ROTATION_RIGHT_90                            1U
#define PP_ROTATION_LEFT_90                             2U
#define PP_ROTATION_HOR_FLIP                            3U
#define PP_ROTATION_VER_FLIP                            4U
#define PP_ROTATION_180                                 5U

typedef struct {
    uint32_t srcAddr;           // 16 align
    uint32_t srcFormat;
    uint32_t srcWidth;          // 16 align max 2048
    uint32_t srcHeight;         // 16 align max 2048
    uint32_t srcHStride;        // 16 align max 2048
    uint32_t srcVStride;        // 16 align max 2048
    uint32_t srcCrop8R;         // crop rigth
    uint32_t srcCrop8D;         // crop down
    uint32_t srcX;                  // src x
    uint32_t srcY;                  // src y
    uint32_t srcReserv[2];

    uint32_t dstAddr;           // 16 align
    uint32_t dstFormat;
    uint32_t dstWidth;          // 16 align max 2048
    uint32_t dstHeight;         // 16 align max 2048
    uint32_t dstHStride;        // 16 align max 2048
    uint32_t dstVStride;        // 16 align max 2048
    uint32_t dstReserv[2];
    uint32_t dstX;                  // dst x
    uint32_t dstY;                  // dst y

    uint32_t vpuFd;             // VPUClient handle
    uint32_t rotation;          // rotation angel
    uint32_t yuvFullRange;      // yuv is full range or not, set yuv trans table
    uint32_t deinterlace;       // do deinterlace or not
    uint32_t optReserv[13];
} PP_OPERATION;


typedef enum {
    PP_SET_SRC_ADDR         = 0,
    PP_SET_SRC_FORMAT,
    PP_SET_SRC_WIDTH,
    PP_SET_SRC_HEIGHT,
    PP_SET_SRC_HSTRIDE,
    PP_SET_SRC_VSTRIDE,

    PP_SET_DST_ADDR         = 8,
    PP_SET_DST_FORMAT,
    PP_SET_DST_WIDTH,
    PP_SET_DST_HEIGHT,
    PP_SET_DST_HSTRIDE,
    PP_SET_DST_VSTRIDE,

    PP_SET_VPU_FD           = 16,           // important must be set or use ppOpSet to set this fd
    PP_SET_ROTATION,
    PP_SET_YUV_RANGE,
    PP_SET_DEINTERLACE,

    PP_SET_BUTT             = 32,
} PP_SET_OPT;

typedef void* PP_OP_HANDLE;

status_t ppOpInit(PP_OP_HANDLE *hnd, PP_OPERATION *init);
status_t ppOpSet(PP_OP_HANDLE hnd, PP_SET_OPT opt, uint32_t val);
status_t ppOpPerform(PP_OP_HANDLE hnd);
status_t ppOpSync(PP_OP_HANDLE hnd);
status_t ppOpRelease(PP_OP_HANDLE hnd);

}
#endif
#endif  // _PPOP_H_

