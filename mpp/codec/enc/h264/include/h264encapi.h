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

#include "basetype.h"
#include "h264e_syntax.h"

#ifdef __cplusplus
extern "C"
{
#endif

#include "H264Instance.h"

/*------------------------------------------------------------------------------
    1. Type definition for encoder instance
------------------------------------------------------------------------------*/

typedef const void *H264EncInst;

/*------------------------------------------------------------------------------
    2. Enumerations for API parameters
------------------------------------------------------------------------------*/

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

/* Stream type for initialization */
typedef enum {
    H264ENC_BYTE_STREAM = 0,    /* H.264 annex B: NAL unit starts with
                                     * hex bytes '00 00 00 01' */
    H264ENC_NAL_UNIT_STREAM = 1 /* Plain NAL units without startcode */
} H264EncStreamType;

/* Level for initialization */
typedef enum {
    H264ENC_LEVEL_1 = 10,
    H264ENC_LEVEL_1_b = 99,
    H264ENC_LEVEL_1_1 = 11,
    H264ENC_LEVEL_1_2 = 12,
    H264ENC_LEVEL_1_3 = 13,
    H264ENC_LEVEL_2 = 20,
    H264ENC_LEVEL_2_1 = 21,
    H264ENC_LEVEL_2_2 = 22,
    H264ENC_LEVEL_3 = 30,
    H264ENC_LEVEL_3_1 = 31,
    H264ENC_LEVEL_3_2 = 32,
    H264ENC_LEVEL_4_0 = 40,
    H264ENC_LEVEL_4_1 = 41
} H264EncLevel;

/* Picture YUV type for initialization */
typedef enum {
    H264ENC_YUV420_PLANAR = 0,  /* YYYY... UUUU... VVVV */
    H264ENC_YUV420_SEMIPLANAR = 1,  /* YYYY... UVUVUV...    */
    H264ENC_YUV422_INTERLEAVED_YUYV = 2,    /* YUYVYUYV...          */
    H264ENC_YUV422_INTERLEAVED_UYVY = 3,    /* UYVYUYVY...          */
    H264ENC_RGB565 = 4, /* 16-bit RGB           */
    H264ENC_BGR565 = 5, /* 16-bit RGB           */
    H264ENC_RGB555 = 6, /* 15-bit RGB           */
    H264ENC_BGR555 = 7, /* 15-bit RGB           */
    H264ENC_RGB444 = 8, /* 12-bit RGB           */
    H264ENC_BGR444 = 9, /* 12-bit RGB           */
    H264ENC_RGB888 = 10,    /* 24-bit RGB           */
    H264ENC_BGR888 = 11,    /* 24-bit RGB           */
    H264ENC_RGB101010 = 12, /* 30-bit RGB           */
    H264ENC_BGR101010 = 13  /* 30-bit RGB           */
} H264EncPictureFormat;

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

/* Complexity level */
typedef enum {
    H264ENC_COMPLEXITY_1 = 1
} H264EncComplexityLevel;

/*------------------------------------------------------------------------------
    3. Structures for API function parameters
------------------------------------------------------------------------------*/

/* Configuration info for initialization
 * Width and height are picture dimensions after rotation
 * Width and height are restricted by level limitations
 */
typedef struct {
    H264EncStreamType streamType;   /* Byte stream / Plain NAL units */
    /* Stream Profile will be automatically decided,
     * CABAC -> main/high, 8x8-transform -> high */
    h264e_profile profile;
    H264EncLevel level;
    u32 width;           /* Encoded picture width in pixels, multiple of 4 */
    u32 height;          /* Encoded picture height in pixels, multiple of 2 */
    u32 frameRateNum;    /* The stream time scale, [1..65535] */
    u32 frameRateDenom;  /* Maximum frame rate is frameRateNum/frameRateDenom
                              * in frames/second. The actual frame rate will be
                              * defined by timeIncrement of encoded pictures,
                              * [1..frameRateNum] */
    H264EncComplexityLevel complexityLevel; /* For compatibility */
    RK_U32 intraPicRate;  // intra period
    RK_U32 enable_cabac;
    RK_U32 transform8x8_mode;
    RK_U32 pic_init_qp;
    RK_U32 chroma_qp_index_offset;
    RK_U32 pic_luma_height;
    RK_U32 pic_luma_width;
    H264EncPictureFormat input_image_format;
    RK_U32 keyframe_max_interval;
    RK_U32 second_chroma_qp_index_offset;
    RK_U32 pps_id;
} H264EncConfig;

/* Coding control parameters */
typedef struct {
    u32 sliceSize;       /* Slice size in macroblock rows,
                              * 0 to encode each picture in one slice,
                              * [0..height/16]
                              */
    u32 seiMessages;     /* Insert picture timing and buffering
                              * period SEI messages into the stream,
                              * [0,1]
                              */
    u32 videoFullRange;  /* Input video signal sample range, [0,1]
                              * 0 = Y range in [16..235],
                              * Cb&Cr range in [16..240]
                              * 1 = Y, Cb and Cr range in [0..255]
                              */
    u32 constrainedIntraPrediction; /* 0 = No constrains,
                                         * 1 = Only use intra neighbours */
    u32 disableDeblockingFilter;    /* 0 = Filter enabled,
                                         * 1 = Filter disabled,
                                         * 2 = Filter disabled on slice edges */
    u32 sampleAspectRatioWidth; /* Horizontal size of the sample aspect
                                     * ratio (in arbitrary units), 0 for
                                     * unspecified, [0..65535]
                                     */
    u32 sampleAspectRatioHeight;    /* Vertical size of the sample aspect ratio
                                         * (in same units as sampleAspectRatioWidth)
                                         * 0 for unspecified, [0..65535]
                                         */
    u32 enableCabac;     /* 0 = CAVLC - Base profile,
                              * 1 = CABAC - Main profile */
    u32 cabacInitIdc;    /* [0,2] */
    u32 transform8x8Mode;   /* Enable 8x8 transform mode, High profile
                                 * 0=disabled, 1=adaptive 8x8, 2=always 8x8 */
} H264EncCodingCtrl;

/* Rate control parameters */
typedef struct {
    u32 pictureRc;       /* Adjust QP between pictures, [0,1] */
    u32 mbRc;            /* Adjust QP inside picture, [0,1] */
    u32 pictureSkip;     /* Allow rate control to skip pictures, [0,1] */
    i32 qpHdr;           /* QP for next encoded picture, [-1..51]
                              * -1 = Let rate control calculate initial QP
                              * This QP is used for all pictures if
                              * HRD and pictureRc and mbRc are disabled
                              * If HRD is enabled it may override this QP
                              */
    u32 qpMin;           /* Minimum QP for any picture, [0..51] */
    u32 qpMax;           /* Maximum QP for any picture, [0..51] */
    u32 bitPerSecond;    /* Target bitrate in bits/second, this is
                              * needed if pictureRc, mbRc, pictureSkip or
                              * hrd is enabled
                              */
    u32 hrd;             /* Hypothetical Reference Decoder model, [0,1]
                              * restricts the instantaneous bitrate and
                              * total bit amount of every coded picture.
                              * Enabling HRD will cause tight constrains
                              * on the operation of the rate control
                              */
    u32 hrdCpbSize;      /* Size of Coded Picture Buffer in HRD (bits) */
    u32 gopLen;          /* Length for Group of Pictures, indicates
                              * the distance of two intra pictures,
                              * including first intra [1..150]
                              */
    i32 intraQpDelta;    /* Intra QP delta. intraQP = QP + intraQpDelta
                              * This can be used to change the relative quality
                              * of the Intra pictures or to lower the size
                              * of Intra pictures.
                              */
    u32 fixedIntraQp;    /* Fixed QP value for all Intra pictures,
                              * 0 = Rate control calculates intra QP.
                              */
    i32 mbQpAdjustment;  /* Encoder uses MAD thresholding to recognize
                              * macroblocks with least details. This value is
                              * used to adjust the QP of these macroblocks
                              * increasing the subjective quality. [-8..7]
                              */
} H264EncRateCtrl;

/* Input pre-processing */
typedef struct {
    H264EncColorConversionType type;
    u16 coeffA;          /* User defined color conversion coefficient */
    u16 coeffB;          /* User defined color conversion coefficient */
    u16 coeffC;          /* User defined color conversion coefficient */
    u16 coeffE;          /* User defined color conversion coefficient */
    u16 coeffF;          /* User defined color conversion coefficient */
} H264EncColorConversion;

typedef struct {
    u32 origWidth;
    u32 origHeight;
    u32 xOffset;
    u32 yOffset;
    H264EncPictureFormat inputType;
    H264EncPictureRotation rotation;
    u32 videoStabilization;
    H264EncColorConversion colorConversion;
} H264EncPreProcessingCfg;

/* Version information */
typedef struct {
    u32 major;           /* Encoder API major version */
    u32 minor;           /* Encoder API minor version */
} H264EncApiVersion;

typedef struct {
    u32 swBuild;         /* Software build ID */
    u32 hwBuild;         /* Hardware build ID */
} H264EncBuild;

/*------------------------------------------------------------------------------
    4. Encoder API function prototypes
------------------------------------------------------------------------------*/

/* Version information */
H264EncApiVersion H264EncGetApiVersion(void);
H264EncBuild H264EncGetBuild(void);

/* Initialization & release */
H264EncRet H264EncInit(const H264EncConfig * pEncConfig,
                       h264Instance_s * instAddr);
H264EncRet H264EncRelease(H264EncInst inst);

/* Encoder configuration before stream generation */
H264EncRet H264EncSetCodingCtrl(H264EncInst inst, const H264EncCodingCtrl *
                                pCodingParams);
H264EncRet H264EncGetCodingCtrl(H264EncInst inst, H264EncCodingCtrl *
                                pCodingParams);

/* Encoder configuration before and during stream generation */
H264EncRet H264EncSetRateCtrl(H264EncInst inst,
                              const H264EncRateCtrl * pRateCtrl);
H264EncRet H264EncGetRateCtrl(H264EncInst inst,
                              H264EncRateCtrl * pRateCtrl);

H264EncRet H264EncSetPreProcessing(H264EncInst inst,
                                   const H264EncPreProcessingCfg *
                                   pPreProcCfg);
H264EncRet H264EncGetPreProcessing(H264EncInst inst,
                                   H264EncPreProcessingCfg * pPreProcCfg);

/* Encoder user data insertion during stream generation */
H264EncRet H264EncSetSeiUserData(H264EncInst inst, const u8 * pUserData,
                                 u32 userDataSize);

/* Stream generation */

/* H264EncStrmStart generates the SPS and PPS. SPS is the first NAL unit and PPS
 * is the second NAL unit. NaluSizeBuf indicates the size of NAL units.
 */
H264EncRet H264EncStrmStart(H264EncInst inst, const H264EncIn * pEncIn,
                            H264EncOut * pEncOut);

/* H264EncStrmEncode encodes one video frame. If SEI messages are enabled the
 * first NAL unit is a SEI message.
 */
H264EncRet H264EncStrmEncode(H264EncInst inst, const H264EncIn * pEncIn,
                             H264EncOut * pEncOut, h264e_syntax *syntax_data);

H264EncRet H264EncStrmEncodeAfter(H264EncInst inst,
                                  H264EncOut * pEncOut, MPP_RET vpuWaitResult);  // add by lance 2016.05.07

/* H264EncStrmEnd ends a stream with an EOS code. */
H264EncRet H264EncStrmEnd(H264EncInst inst, const H264EncIn * pEncIn,
                          H264EncOut * pEncOut);

/*------------------------------------------------------------------------------
    5. Encoder API tracing callback function
------------------------------------------------------------------------------*/

void H264EncTrace(const char *msg);

#ifdef __cplusplus
}
#endif

#endif /*__H264ENCAPI_H__*/
