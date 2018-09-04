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

#ifndef _PPOP_H_
#define _PPOP_H_

#include <sys/types.h>

typedef RK_S32     status_t;

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
    RK_U32 srcAddr;           // 16 align
    RK_U32 srcFormat;
    RK_U32 srcWidth;          // 16 align max 2048
    RK_U32 srcHeight;         // 16 align max 2048
    RK_U32 srcHStride;        // 16 align max 2048
    RK_U32 srcVStride;        // 16 align max 2048
    RK_U32 srcCrop8R;         // crop rigth
    RK_U32 srcCrop8D;         // crop down
    RK_U32 srcX;                  // src x
    RK_U32 srcY;                  // src y
    RK_U32 srcReserv[2];

    RK_U32 dstAddr;           // 16 align
    RK_U32 dstFormat;
    RK_U32 dstWidth;          // 16 align max 2048
    RK_U32 dstHeight;         // 16 align max 2048
    RK_U32 dstHStride;        // 16 align max 2048
    RK_U32 dstVStride;        // 16 align max 2048
    RK_U32 dstReserv[2];
    RK_U32 dstX;                  // dst x
    RK_U32 dstY;                  // dst y

    RK_U32 vpuFd;             // VPUClient handle
    RK_U32 rotation;          // rotation angel
    RK_U32 yuvFullRange;      // yuv is full range or not, set yuv trans table
    RK_U32 deinterlace;       // do deinterlace or not
    RK_U32 optReserv[13];
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
status_t ppOpSet(PP_OP_HANDLE hnd, PP_SET_OPT opt, RK_U32 val);
status_t ppOpPerform(PP_OP_HANDLE hnd);
status_t ppOpSync(PP_OP_HANDLE hnd);
status_t ppOpRelease(PP_OP_HANDLE hnd);

}

#endif  // _PPOP_H_

