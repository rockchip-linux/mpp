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

#ifndef __H264_INSTANCE_H__
#define __H264_INSTANCE_H__

#include "mpp_log.h"

#include "enccommon.h"
#include "encpreprocess.h"
#include "encasiccontroller.h"

#include "H264SequenceParameterSet.h"
#include "H264PictureParameterSet.h"
#include "H264Slice.h"
#include "H264RateControl.h"
#include "H264Mad.h"

enum H264EncStatus {
    H264ENCSTAT_INIT = 0xA1,
    H264ENCSTAT_START_STREAM,
    H264ENCSTAT_START_FRAME,
    H264ENCSTAT_ERROR
};

/* Picture type for encoding */
typedef enum {
    H264ENC_INTRA_FRAME = 0,
    H264ENC_PREDICTED_FRAME = 1,
    H264ENC_NOTCODED_FRAME  /* Used just as a return value */
} H264EncPictureCodingType;

/* Encoder input structure */
typedef struct {
    u32 busLuma;         /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    u32 busChromaU;      /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    u32 busChromaV;      /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    u32 timeIncrement;   /* The previous picture duration in units
                              * of frameRateDenom/frameRateNum.
                              * 0 for the very first picture.
                              */
    u32 *pOutBuf;        /* Pointer to output stream buffer */
    u32 busOutBuf;       /* Bus address of output stream buffer */
    u32 outBufSize;      /* Size of output stream buffer in bytes */

    H264EncPictureCodingType codingType;    /* Proposed picture coding type,
                                                 * INTRA/PREDICTED
                                                 */
    u32 busLumaStab;     /* bus address of next picture to stabilize (luminance) */
} H264EncIn;

/* Encoder output structure */
typedef struct {
    H264EncPictureCodingType codingType;    /* Realized picture coding type,
                                                 * INTRA/PREDICTED/NOTCODED
                                                 */
    u32 streamSize;      /* Size of output stream in bytes */
} H264EncOut;

typedef struct {
    u32 encStatus;
    RK_U32 lumWidthSrc;  // TODO  need to think again  modify by lance 2016.06.15
    RK_U32 lumHeightSrc;  // TODO  need to think again  modify by lance 2016.06.15
    u32 mbPerFrame;
    u32 mbPerRow;
    u32 mbPerCol;
    u32 frameCnt;
    u32 fillerNalSize;
    u32 testId;
    stream_s stream;
    preProcess_s preProcess;
    sps_s seqParameterSet;
    pps_s picParameterSet;
    slice_s slice;
    h264RateControl_s rateControl;
    madTable_s mad;
    asicData_s asic;
    const void *inst;
    u32 time_debug_init;
    RK_U32 intraPeriodCnt;  //  count the frame amount from last intra frame,
    // then determine next frame to which type to be encoded
    H264EncIn encIn;        // put input struct into instance, todo    modify by lance 2016.05.31
    H264EncOut encOut;      //  put input struct into instance, todo    modify by lance 2016.05.31
    RK_U32 intraPicRate;        // set I frame interval, and is 30 default
} h264Instance_s;

#define H264E_DBG_FUNCTION          (0x00000001)

extern RK_U32 h264e_debug;

#define h264e_dbg(flag, fmt, ...)   _mpp_dbg(h264e_debug, flag, fmt, ## __VA_ARGS__)
#define h264e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h264e_debug, flag, fmt, ## __VA_ARGS__)

#define h264e_dbg_func(fmt, ...)    h264e_dbg_f(H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)

#endif
