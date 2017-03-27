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

#ifndef __JPEGD_SYNTAX__
#define __JPEGD_SYNTAX__

#include "vpu_api.h"

#define MIN_NUMBER_OF_COMPONENTS              (1)
#define MAX_NUMBER_OF_COMPONENTS              (3)

#define JPEGDEC_YCbCr400                      (0x080000U)
#define JPEGDEC_YCbCr440                      (0x010004U)
#define JPEGDEC_YCbCr411_SEMIPLANAR           (0x100000U)
#define JPEGDEC_YCbCr444_SEMIPLANAR           (0x200000U)

#define JPEGDEC_BASELINE                      (0x0)
#define JPEGDEC_PROGRESSIVE                   (0x1)
#define JPEGDEC_NONINTERLEAVED                (0x2)

#define JPEGDEC_YUV400                        (0)
#define JPEGDEC_YUV420                        (2)
#define JPEGDEC_YUV422                        (3)
#define JPEGDEC_YUV444                        (4)
#define JPEGDEC_YUV440                        (5)
#define JPEGDEC_YUV411                        (6)

#define JPEGD_STREAM_BUFF_SIZE                (512*1024)
#define JPEGDEC_BASELINE_TABLE_SIZE           (544)

typedef enum {
    JPEGDEC_SLICE_READY = 2,
    JPEGDEC_FRAME_READY = 1,
    JPEGDEC_STRM_PROCESSED = 3,
    JPEGDEC_SCAN_PROCESSED = 4,
    JPEGDEC_OK = 0,
    JPEGDEC_ERROR = -1,
    JPEGDEC_UNSUPPORTED = -2,
    JPEGDEC_PARAM_ERROR = -3,
    JPEGDEC_MEMFAIL = -4,
    JPEGDEC_INITFAIL = -5,
    JPEGDEC_INVALID_STREAM_LENGTH = -6,
    JPEGDEC_STRM_ERROR = -7,
    JPEGDEC_INVALID_INPUT_BUFFER_SIZE = -8,
    JPEGDEC_HW_RESERVED = -9,
    JPEGDEC_INCREASE_INPUT_BUFFER = -10,
    JPEGDEC_SLICE_MODE_UNSUPPORTED = -11,
    JPEGDEC_DWL_HW_TIMEOUT = -253,
    JPEGDEC_DWL_ERROR = -254,
    JPEGDEC_HW_BUS_ERROR = -255,
    JPEGDEC_SYSTEM_ERROR = -256,
    JPEGDEC_FORMAT_NOT_SUPPORTED = -1000
} JpegDecRet;

typedef struct {
    RK_U32 C;  /* Component id */
    RK_U32 H;  /* Horizontal sampling factor */
    RK_U32 V;  /* Vertical sampling factor */
    RK_U32 Tq; /* Quantization table destination selector */
} Components;

typedef struct {
    RK_U8 *pStartOfStream;
    RK_U8 *pCurrPos;
    RK_U32 streamBus; /* physical address */
    RK_U32 bitPosInByte;
    RK_U32 streamLength;
    RK_U32 readBits;
    RK_U32 appnFlag;
    RK_U32 thumbnail;
    RK_U32 returnSosMarker;
} StreamStorage;

typedef struct {
    RK_U8 *pStartOfImage;
    RK_U8 *pLum;
    RK_U8 *pCr;
    RK_U8 *pCb;
    RK_U32 imageReady;
    RK_U32 headerReady;
    RK_U32 size;
    RK_U32 sizeLuma;
    RK_U32 sizeChroma;
    RK_U32 ready;
    RK_U32 columns[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 pixelsPerRow[MAX_NUMBER_OF_COMPONENTS];
} ImageData;

typedef struct {
    RK_U32 Lf;
    RK_U32 P;
    RK_U32 Y;
    RK_U32 hwY;
    RK_U32 X;
    RK_U32 hwX;
    RK_U32 Nf; /* Number of components in frame */
    RK_U32 codingType;
    RK_U32 numMcuInFrame;
    RK_U32 numMcuInRow;
    RK_U32 mcuNumber;
    RK_U32 nextRstNumber;
    RK_U32 Ri;
    RK_U32 driPeriod;
    RK_U32 block;
    RK_U32 row;
    RK_U32 col;
    RK_U32 cIndex;
    RK_U32 *pBuffer;
    RK_U32 bufferBus;
    RK_S32 *pBufferCb;
    RK_S32 *pBufferCr;
    VPUMemLinear_t pTableBase;
    RK_U32 numBlocks[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 blocksPerRow[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 useAcOffset[MAX_NUMBER_OF_COMPONENTS];
    Components component[MAX_NUMBER_OF_COMPONENTS];
} FrameInfo;

typedef struct {
    RK_U32 Ls;
    RK_U32 Ns;
    RK_U32 Cs[MAX_NUMBER_OF_COMPONENTS];   /* Scan component selector */
    RK_U32 Td[MAX_NUMBER_OF_COMPONENTS];   /* Selects table for DC */
    RK_U32 Ta[MAX_NUMBER_OF_COMPONENTS];   /* Selects table for AC */
    RK_U32 Ss;
    RK_U32 Se;
    RK_U32 Ah;
    RK_U32 Al;
    RK_U32 index;
    RK_S32 numIdctRows;
    RK_S32 pred[MAX_NUMBER_OF_COMPONENTS];
} ScanInfo;

typedef struct {
    RK_U32 sliceHeight;
    RK_U32 amountOfQTables;
    RK_U32 yCbCrMode;
    RK_U32 yCbCr422;
    RK_U32 column;
    RK_U32 X; /* width */
    RK_U32 Y; /* height */
    RK_U32 memSize;
    RK_U32 SliceCount;
    RK_U32 SliceReadyForPause;
    RK_U32 SliceMBCutValue;
    RK_U32 pipeline;
    RK_U32 userAllocMem;
    RK_U32 sliceMbSetValue;
    RK_U32 timeout;
    RK_U32 rlcMode;
    RK_U32 lumaPos;
    RK_U32 chromaPos;
    RK_U32 sliceStartCount;
    RK_U32 amountOfSlices;
    RK_U32 noSliceIrqForUser;
    RK_U32 sliceLimitReached;
    RK_U32 inputBufferEmpty;
    RK_U32 fillRight;
    RK_U32 fillBottom;
    RK_U32 streamEnd;
    RK_U32 streamEndFlag;
    RK_U32 inputBufferLen;
    RK_U32 inputStreaming;
    RK_U32 decodedStreamLen;
    RK_U32 init;
    RK_U32 initThumb;
    RK_U32 initBufferSize;
    RK_S32 dcRes[MAX_NUMBER_OF_COMPONENTS];
    VPUMemLinear_t outLuma;
    VPUMemLinear_t outChroma;
    VPUMemLinear_t outChroma2;
    VPUMemLinear_t givenOutLuma;
    VPUMemLinear_t givenOutChroma;
    VPUMemLinear_t givenOutChroma2;
    RK_S32 pred[MAX_NUMBER_OF_COMPONENTS];
    /* progressive parameters */
    RK_U32 nonInterleaved;
    RK_U32 componentId;
    RK_U32 operationType;
    RK_U32 operationTypeThumb;
    RK_U32 progressiveScanReady;
    RK_U32 nonInterleavedScanReady;
    RK_U32 allocated;
    RK_U32 yCbCrModeOrig;
    RK_U32 getInfoYCbCrMode;
    RK_U32 components[MAX_NUMBER_OF_COMPONENTS];
    VPUMemLinear_t pCoeffBase;

    RK_U32 fillX;
    RK_U32 fillY;

    RK_U32 progressiveFinish;
    RK_U32 pfCompId;
    RK_U32 pfNeeded[MAX_NUMBER_OF_COMPONENTS];
    VPUMemLinear_t tmpStrm;
} DecInfo;

typedef struct {
    VPUMemLinear_t outLumaBuffer;
    VPUMemLinear_t outChromaBuffer;
    VPUMemLinear_t outChromaBuffer2;
} JpegAsicBuffers;

typedef struct {
    RK_U32 bits[16];
    RK_U32 *vals;
    RK_U32 tableLength;
    RK_U32 start;
    RK_U32 last;
} VlcTable;

typedef struct {
    RK_U32 Lh;
    VlcTable acTable0;
    VlcTable acTable1;
    VlcTable acTable2;
    VlcTable acTable3;
    VlcTable dcTable0;
    VlcTable dcTable1;
    VlcTable dcTable2;
    VlcTable dcTable3;
    VlcTable *table;
} HuffmanTables;

typedef struct {
    RK_U32 Lq; /* Quantization table definition length */
    RK_U32 table0[64];
    RK_U32 table1[64];
    RK_U32 table2[64];
    RK_U32 table3[64];
    RK_U32 *table;
} QuantTables;

/* Control interface between decoder and pp */
/* decoder writes, pp read-only */
typedef struct DecPpInterface_ {
    enum {
        DECPP_IDLE = 0,
        DECPP_RUNNING,  /* PP was started */
        DECPP_PIC_READY, /* PP has finished a picture */
        DECPP_PIC_NOT_FINISHED /* PP still processing a picture */
    } ppStatus; /* Decoder keeps track of what it asked the pp to do */

    enum {
        MULTIBUFFER_UNINIT = 0, /* buffering mode not yet decided */
        MULTIBUFFER_DISABLED,   /* Single buffer legacy mode */
        MULTIBUFFER_SEMIMODE,   /* enabled but full pipel cannot be used */
        MULTIBUFFER_FULLMODE    /* enabled and full pipeline successful */
    } multiBufStat;

    RK_U32 inputBusLuma;
    RK_U32 inputBusChroma;
    RK_U32 bottomBusLuma;
    RK_U32 bottomBusChroma;
    RK_U32 picStruct;           /* structure of input picture */
    RK_U32 topField;
    RK_U32 inwidth;
    RK_U32 inheight;
    RK_U32 usePipeline;         /* only this variance be used */
    RK_U32 littleEndian;
    RK_U32 wordSwap;
    RK_U32 croppedW;
    RK_U32 croppedH;

    RK_U32 bufferIndex;         /* multibuffer, where to put PP output */
    RK_U32 displayIndex;        /* multibuffer, next picture in display order */
    RK_U32 prevAnchorDisplayIndex;

    /* VC-1 */
    RK_U32 rangeRed;
    RK_U32 rangeMapYEnable;
    RK_U32 rangeMapYCoeff;
    RK_U32 rangeMapCEnable;
    RK_U32 rangeMapCCoeff;
} DecPpInterface;

typedef struct {
    int enable;
    int outFomart; /* =0,RGB565;=1,ARGB 8888 */
    int scale_denom;
    int shouldDither;
    int cropX;
    int cropY;
    int cropW;
    int cropH;
} PostProcessInfo;

/* Image information */
typedef struct {
    RK_U32 displayWidth;
    RK_U32 displayHeight;
    RK_U32 outputWidth;    /* Number of pixels/line in the image  */
    RK_U32 outputHeight;   /* Number of lines in in the image     */
    RK_U32 version;
    RK_U32 units;
    RK_U32 xDensity;
    RK_U32 yDensity;
    RK_U32 outputFormat;
    RK_U32 codingMode;  /* JPEGDEC_BASELINE
                         * JPEGDEC_PROGRESSIVE
                         * JPEGDEC_NONINTERLEAVED
                         */

    RK_U32 thumbnailType;  /* Thumbnail exist or not or not supported */
    RK_U32 displayWidthThumb;
    RK_U32 displayHeightThumb;
    RK_U32 outputWidthThumb;   /* Number of pixels/line in the image  */
    RK_U32 outputHeightThumb;  /* Number of lines in in the image     */
    RK_U32 outputFormatThumb;  /* JPEGDEC_YCbCr400
                                 * JPEGDEC_YCbCr420
                                 * JPEGDEC_YCbCr422
                                 */
    RK_U32 codingModeThumb;    /* JPEGDEC_BASELINE
                                 * JPEGDEC_PROGRESSIVE
                                 * JPEGDEC_NONINTERLEAVED
                                 */
} JpegDecImageInfo;

typedef struct JpegSyntaxParam {
    StreamStorage          stream;
    FrameInfo              frame;
    ImageData              image;
    ScanInfo               scan;
    DecInfo                info;
    HuffmanTables          vlc;
    QuantTables            quant;
    JpegDecImageInfo       imageInfo;
    RK_U32                 ppInputFomart;
    PostProcessInfo        ppInfo;
    RK_U32                 ppScaleW;
    RK_U32                 ppScaleH;
    JpegAsicBuffers        asicBuff;
    DecPpInterface         ppControl;
    const void             *ppInstance;
} JpegSyntaxParam;

#endif /*__JPEGD_SYNTAX__*/
