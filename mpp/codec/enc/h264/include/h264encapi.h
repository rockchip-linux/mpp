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

#ifndef __H264ENCAPI_H__
#define __H264ENCAPI_H__

#include "h264_syntax.h"
#include "h264e_syntax.h"
#include "mpp_frame.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "H264Instance.h"

/* Function return values */
typedef enum {
    H264ENC_OK = 0,
    H264ENC_FRAME_READY = 1,

    H264ENC_ERROR = -1,
    H264ENC_NULL_ARGUMENT = -2,
    H264ENC_INVALID_ARGUMENT = -3,
    H264ENC_MEMORY_ERROR = -4,
    H264ENC_EWL_ERROR = -5,
    H264ENC_EWL_MEMORY_ERROR = -6,
    H264ENC_INVALID_STATUS = -7,
    H264ENC_OUTPUT_BUFFER_OVERFLOW = -8,
    H264ENC_HW_BUS_ERROR = -9,
    H264ENC_HW_DATA_ERROR = -10,
    H264ENC_HW_TIMEOUT = -11,
    H264ENC_HW_RESERVED = -12,
    H264ENC_SYSTEM_ERROR = -13,
    H264ENC_INSTANCE_ERROR = -14,
    H264ENC_HRD_ERROR = -15,
    H264ENC_HW_RESET = -16
} H264EncRet;

/* Picture rotation for pre-processing */
typedef enum {
    H264ENC_ROTATE_0 = 0,
    H264ENC_ROTATE_90R = 1, /* Rotate 90 degrees clockwise */
    H264ENC_ROTATE_90L = 2  /* Rotate 90 degrees counter-clockwise */
} H264EncPictureRotation;

/* Picture color space conversion (RGB input) for pre-processing */
typedef enum {
    H264ENC_RGBTOYUV_BT601 = 0, /* Color conversion according to BT.601 */
    H264ENC_RGBTOYUV_BT709 = 1, /* Color conversion according to BT.709 */
    H264ENC_RGBTOYUV_USER_DEFINED = 2   /* User defined color conversion */
} H264EncColorConversionType;

enum H264EncStatus {
    H264ENCSTAT_INIT = 0xA1,
    H264ENCSTAT_START_STREAM,
    H264ENCSTAT_START_FRAME,
    H264ENCSTAT_ERROR
};

/* Stream type for initialization */
typedef enum {
    H264ENC_BYTE_STREAM = 0,    /* H.264 annex B: NAL unit starts with
                                     * hex bytes '00 00 00 01' */
    H264ENC_NAL_UNIT_STREAM = 1 /* Plain NAL units without startcode */
} H264EncStreamType;

/* Picture type for encoding */
typedef enum {
    H264ENC_INTRA_FRAME = 0,
    H264ENC_PREDICTED_FRAME = 1,
    H264ENC_NOTCODED_FRAME  /* Used just as a return value */
} H264EncPictureCodingType;


/*------------------------------------------------------------------------------
    3. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Coding control parameters */
typedef struct {
    RK_U32 sliceSize;       /* Slice size in macroblock rows,
                              * 0 to encode each picture in one slice,
                              * [0..height/16]
                              */
    RK_U32 seiMessages;     /* Insert picture timing and buffering
                              * period SEI messages into the stream,
                              * [0,1]
                              */
    RK_U32 videoFullRange;  /* Input video signal sample range, [0,1]
                              * 0 = Y range in [16..235],
                              * Cb&Cr range in [16..240]
                              * 1 = Y, Cb and Cr range in [0..255]
                              */
    RK_U32 constrainedIntraPrediction; /* 0 = No constrains,
                                         * 1 = Only use intra neighbours */
    RK_U32 disableDeblockingFilter;    /* 0 = Filter enabled,
                                         * 1 = Filter disabled,
                                         * 2 = Filter disabled on slice edges */
    RK_U32 sampleAspectRatioWidth; /* Horizontal size of the sample aspect
                                     * ratio (in arbitrary units), 0 for
                                     * unspecified, [0..65535]
                                     */
    RK_U32 sampleAspectRatioHeight;    /* Vertical size of the sample aspect ratio
                                         * (in same units as sampleAspectRatioWidth)
                                         * 0 for unspecified, [0..65535]
                                         */
    RK_U32 enableCabac;     /* 0 = CAVLC - Base profile,
                              * 1 = CABAC - Main profile */
    RK_U32 cabacInitIdc;    /* [0,2] */
    RK_U32 transform8x8Mode;   /* Enable 8x8 transform mode, High profile
                                 * 0=disabled, 1=adaptive 8x8, 2=always 8x8 */
} H264EncCodingCtrl;


/* Input pre-processing */
typedef struct {
    H264EncColorConversionType type;
    RK_U16 coeffA;          /* User defined color conversion coefficient */
    RK_U16 coeffB;          /* User defined color conversion coefficient */
    RK_U16 coeffC;          /* User defined color conversion coefficient */
    RK_U16 coeffE;          /* User defined color conversion coefficient */
    RK_U16 coeffF;          /* User defined color conversion coefficient */
} H264EncColorConversion;

typedef struct {
    RK_U32 origWidth;
    RK_U32 origHeight;
    RK_U32 xOffset;
    RK_U32 yOffset;
    MppFrameFormat inputType;
    H264EncPictureRotation rotation;
    RK_U32 videoStabilization;
    H264EncColorConversion colorConversion;
} H264EncPreProcessingCfg;

/* Version information */
typedef struct {
    RK_U32 major;           /* Encoder API major version */
    RK_U32 minor;           /* Encoder API minor version */
} H264EncApiVersion;

typedef struct {
    RK_U32 swBuild;         /* Software build ID */
    RK_U32 hwBuild;         /* Hardware build ID */
} H264EncBuild;

/* Encoder input structure */
typedef struct {
    RK_U32 busLuma;         /* Bus address for input picture
                              * planar format: luminance component
                              * semiplanar format: luminance component
                              * interleaved format: whole picture
                              */
    RK_U32 busChromaU;      /* Bus address for input chrominance
                              * planar format: cb component
                              * semiplanar format: both chrominance
                              * interleaved format: not used
                              */
    RK_U32 busChromaV;      /* Bus address for input chrominance
                              * planar format: cr component
                              * semiplanar format: not used
                              * interleaved format: not used
                              */
    RK_U32 timeIncrement;   /* The previous picture duration in units
                              * of frameRateDenom/frameRateNum.
                              * 0 for the very first picture.
                              */
    RK_U32 *pOutBuf;        /* Pointer to output stream buffer */
    RK_U32 busOutBuf;       /* Bus address of output stream buffer */
    RK_U32 outBufSize;      /* Size of output stream buffer in bytes */

    H264EncPictureCodingType codingType;    /* Proposed picture coding type,
                                                 * INTRA/PREDICTED
                                                 */
    RK_U32 busLumaStab;     /* bus address of next picture to stabilize (luminance) */
} H264EncIn;

/* Encoder output structure */
typedef struct {
    H264EncPictureCodingType codingType;    /* Realized picture coding type,
                                                 * INTRA/PREDICTED/NOTCODED
                                                 */
    RK_U32 streamSize;      /* Size of output stream in bytes */
} H264EncOut;

/* Configuration info for initialization
 * Width and height are picture dimensions after rotation
 * Width and height are restricted by level limitations
 */
typedef struct {
    H264EncStreamType streamType;   /* Byte stream / Plain NAL units */
    /* Stream Profile will be automatically decided,
     * CABAC -> main/high, 8x8-transform -> high */
    H264Profile profile;
    H264Level   level;
    RK_U32 width;           /* Encoded picture width in pixels, multiple of 4 */
    RK_U32 height;          /* Encoded picture height in pixels, multiple of 2 */
    RK_U32 frameRateNum;    /* The stream time scale, [1..65535] */
    RK_U32 frameRateDenom;  /* Maximum frame rate is frameRateNum/frameRateDenom
                              * in frames/second. The actual frame rate will be
                              * defined by timeIncrement of encoded pictures,
                              * [1..frameRateNum] */
    RK_U32 enable_cabac;
    RK_U32 transform8x8_mode;
    RK_U32 pic_init_qp;
    RK_U32 chroma_qp_index_offset;
    RK_U32 pic_luma_height;
    RK_U32 pic_luma_width;
    MppFrameFormat input_image_format;
    RK_U32 second_chroma_qp_index_offset;
    RK_U32 pps_id;
} H264EncConfig;

/* Rate control parameters */
typedef struct {
    RK_U32 pictureRc;       /* Adjust QP between pictures, [0,1] */
    RK_U32 mbRc;            /* Adjust QP inside picture, [0,1] */
    RK_U32 pictureSkip;     /* Allow rate control to skip pictures, [0,1] */
    RK_U32 intraPicRate;    /* intra period, namely keyframe_max_interval */
    RK_S32 qpHdr;           /* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
    RK_U32 qpMin;           /* Minimum QP for any picture, [0..51] */
    RK_U32 qpMax;           /* Maximum QP for any picture, [0..51] */
    RK_U32 bitPerSecond;    /* Target bitrate in bits/second, this is
                              * needed if pictureRc, mbRc, pictureSkip or
                              * hrd is enabled
                              */
    RK_U32 hrd;             /* Hypothetical Reference Decoder model, [0,1]
                              * restricts the instantaneous bitrate and
                              * total bit amount of every coded picture.
                              * Enabling HRD will cause tight constrains
                              * on the operation of the rate control
                              */
    RK_U32 hrdCpbSize;      /* Size of Coded Picture Buffer in HRD (bits) */
    RK_U32 gopLen;          /* Length for Group of Pictures, indicates
                              * the distance of two intra pictures,
                              * including first intra [1..150]
                              */
    RK_S32 intraQpDelta;    /* Intra QP delta. intraQP = QP + intraQpDelta
                              * This can be used to change the relative quality
                              * of the Intra pictures or to lower the size
                              * of Intra pictures.
                              */
    RK_U32 fixedIntraQp;    /* Fixed QP value for all Intra pictures,
                              * 0 = Rate control calculates intra QP.
                              */
    RK_S32 mbQpAdjustment;  /* Encoder uses MAD thresholding to recognize
                              * macroblocks with least details. This value is
                              * used to adjust the QP of these macroblocks
                              * increasing the subjective quality. [-8..7]
                              */
} H264EncRateCtrl;

typedef struct {
    RK_U32 encStatus;
    RK_U32 lumWidthSrc;  // TODO  need to think again  modify by lance 2016.06.15
    RK_U32 lumHeightSrc;  // TODO  need to think again  modify by lance 2016.06.15
    RK_U32 mbPerFrame;
    RK_U32 mbPerRow;
    RK_U32 mbPerCol;
    RK_U32 frameCnt;
    RK_U32 fillerNalSize;
    RK_U32 testId;
    stream_s stream;
    preProcess_s preProcess;
    sps_s seqParameterSet;
    pps_s picParameterSet;
    slice_s slice;
    h264RateControl_s rateControl;
    madTable_s mad;
    asicData_s asic;
    const void *inst;
    RK_U32 time_debug_init;
    RK_U32 intraPeriodCnt;  //  count the frame amount from last intra frame,
    // then determine next frame to which type to be encoded
    H264EncIn encIn;        // put input struct into instance, todo    modify by lance 2016.05.31
    H264EncOut encOut;      //  put input struct into instance, todo    modify by lance 2016.05.31
    RK_U32 intraPicRate;        // set I frame interval, and is 30 default

    h264e_control_extra_info_cfg info;
    H264EncConfig   enc_cfg;
    H264EncRateCtrl enc_rc_cfg;

    // data for hal
    h264e_syntax    syntax;
} H264ECtx;

#define H264E_DBG_FUNCTION          (0x00000001)

extern RK_U32 h264e_debug;

#define h264e_dbg(flag, fmt, ...)   _mpp_dbg(h264e_debug, flag, fmt, ## __VA_ARGS__)
#define h264e_dbg_f(flag, fmt, ...) _mpp_dbg_f(h264e_debug, flag, fmt, ## __VA_ARGS__)

#define h264e_dbg_func(fmt, ...)    h264e_dbg_f(H264E_DBG_FUNCTION, fmt, ## __VA_ARGS__)

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Initialization & release */
H264EncRet H264EncInit(H264ECtx *inst);
H264EncRet H264EncRelease(H264ECtx *inst);

/* Encoder configuration before stream generation */
H264EncRet H264EncCfg(H264ECtx *inst, const H264EncConfig * pEncConfig);
H264EncRet H264EncSetCodingCtrl(H264ECtx *inst, const H264EncCodingCtrl *
                                pCodingParams);

/* Encoder configuration before and during stream generation */
H264EncRet H264EncSetRateCtrl(H264ECtx *inst,
                              const H264EncRateCtrl * pRateCtrl);

/* Encoder user data insertion during stream generation */
H264EncRet H264EncSetSeiUserData(H264ECtx *inst, const RK_U8 * pUserData,
                                 RK_U32 userDataSize);

/* Stream generation */

/* H264EncStrmStart generates the SPS and PPS. SPS is the first NAL unit and PPS
 * is the second NAL unit. NaluSizeBuf indicates the size of NAL units.
 */
H264EncRet H264EncStrmStart(H264ECtx *inst, const H264EncIn * pEncIn,
                            H264EncOut * pEncOut);

/* H264EncStrmEncode encodes one video frame. If SEI messages are enabled the
 * first NAL unit is a SEI message.
 */
H264EncRet H264EncStrmEncode(H264ECtx *inst, const H264EncIn * pEncIn,
                             H264EncOut * pEncOut, h264e_syntax *syntax_data);

H264EncRet H264EncStrmEncodeAfter(H264ECtx *inst,
                                  H264EncOut * pEncOut, MPP_RET vpuWaitResult);

/* H264EncStrmEnd ends a stream with an EOS code. */
H264EncRet H264EncStrmEnd(H264ECtx *inst, const H264EncIn * pEncIn,
                          H264EncOut * pEncOut);

#ifdef __cplusplus
}
#endif

#endif /*__H264ENCAPI_H__*/
