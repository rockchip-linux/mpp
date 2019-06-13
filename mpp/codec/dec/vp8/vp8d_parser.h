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

#ifndef __VP8D_PARSER_H__
#define __VP8D_PARSER_H__

#include "mpp_bitread.h"
#include "mpp_mem.h"

#include "mpp_dec.h"

#include "vp8d_syntax.h"
#include "vp8d_data.h"

#define VP8HWD_VP7             1
#define VP8HWD_VP8             2
#define VP8HWD_WEBP     3

#define DEC_MODE_VP7  9
#define DEC_MODE_VP8 10

#define MAX_NBR_OF_SEGMENTS             (4)
#define MAX_NBR_OF_MB_REF_LF_DELTAS     (4)
#define MAX_NBR_OF_MB_MODE_LF_DELTAS    (4)

#define MAX_NBR_OF_DCT_PARTITIONS       (8)

#define MAX_NBR_OF_VP7_MB_FEATURES      (4)

#define VP8D_BUF_SIZE_BITMEM   (512 * 1024)
#define VP8D_PROB_TABLE_SIZE  (1<<16) /* TODO */
#define VP8D_MAX_SEGMAP_SIZE  (2048 + 1024)  //1920*1080 /* TODO */
#define VP8_KEY_FRAME_START_CODE    0x9d012a

#define VP8D_DBG_FUNCTION          (0x00000001)
#define VP8D_DBG_WARNNING          (0x00000004)
#define VP8D_DBG_LOG               (0x00000008)
#define VP8D_DBG_SEC_HEADER        (0x00000010)


typedef enum {
    VP8_YCbCr_BT601,
    VP8_CUSTOM
} vpColorSpace_e;

typedef struct {
    RK_U32 lowvalue;
    RK_U32 range;
    RK_U32 value;
    RK_S32 count;
    RK_U32 pos;
    RK_U8 *buffer;
    RK_U32 BitCounter;
    RK_U32 streamEndPos;
    RK_U32 strmError;
} vpBoolCoder_t;

typedef struct {
    RK_U8              probLuma16x16PredMode[4];
    RK_U8              probChromaPredMode[3];
    RK_U8              probMvContext[2][VP8_MV_PROBS_PER_COMPONENT];
    RK_U8              probCoeffs[4][8][3][11];
} vp8EntropyProbs_t;

typedef struct VP8Frame {
    MppFrame f;
    RK_S32 slot_index;
    RK_S8 ref_count;
} VP8Frame;


typedef struct VP8DParserContext {
    DXVA_PicParams_VP8 *dxva_ctx;
    RK_U8           *bitstream_sw_buf;
    RK_U32          max_stream_size;
    RK_U32          stream_size;

    VP8Frame       *frame_out;
    VP8Frame       *frame_ref;
    VP8Frame       *frame_golden;
    VP8Frame       *frame_alternate;

    vpBoolCoder_t       bitstr;

    RK_U32             decMode;

    /* Current frame dimensions */
    RK_U32             width;
    RK_U32             height;
    RK_U32             scaledWidth;
    RK_U32             scaledHeight;

    RK_U8             vpVersion;
    RK_U32             vpProfile;

    RK_U32             keyFrame;

    RK_U8              coeffSkipMode;

    /* DCT coefficient partitions */
    RK_U32             offsetToDctParts;
    RK_U32             nbrDctPartitions;
    RK_U32             dctPartitionOffsets[MAX_NBR_OF_DCT_PARTITIONS];

    vpColorSpace_e     colorSpace;
    RK_U32             clamping;
    RK_U32             showFrame;


    RK_U32             refreshGolden;
    RK_U32             refreshAlternate;
    RK_U32             refreshLast;
    RK_U32             refreshEntropyProbs;
    RK_U32             copyBufferToGolden;
    RK_U32             copyBufferToAlternate;

    RK_U32             refFrameSignBias[2];
    RK_U32             useAsReference;
    RK_U32             loopFilterType;
    RK_U32             loopFilterLevel;
    RK_U32             loopFilterSharpness;

    /* Quantization parameters */
    RK_S8             qpYAc, qpYDc, qpY2Ac, qpY2Dc, qpChAc, qpChDc;

    /* From here down, frame-to-frame persisting stuff */
    RK_U32             vp7ScanOrder[16];
    RK_U32             vp7PrevScanOrder[16];

    /* Probabilities */
    RK_U32             probIntra;
    RK_U32             probRefLast;
    RK_U32             probRefGolden;
    RK_U32             probMbSkipFalse;
    RK_U32             probSegment[3];
    vp8EntropyProbs_t entropy, entropyLast;

    /* Segment and macroblock specific values */
    RK_U32             segmentationEnabled;
    RK_U32             segmentationMapUpdate;
    RK_U32             segmentFeatureMode; /* delta/abs */
    RK_S32             segmentQp[MAX_NBR_OF_SEGMENTS];
    RK_S32             segmentLoopfilter[MAX_NBR_OF_SEGMENTS];
    RK_U32             modeRefLfEnabled;
    RK_S32             mbRefLfDelta[MAX_NBR_OF_MB_REF_LF_DELTAS];
    RK_S32             mbModeLfDelta[MAX_NBR_OF_MB_MODE_LF_DELTAS];

    RK_U32             frameTagSize;

    /* Value to remember last frames prediction for hits into most
    * probable reference frame */
    RK_U32             refbuPredHits;


    RK_S32          dcPred[2];
    RK_S32          dcMatch[2];

    RK_U32          frame_cnt;
    RK_U64          pts;

    RK_U32          needKeyFrame;
    MppPacket       input_packet;
    RK_U32          eos;

    MppBufSlots packet_slots;
    MppBufSlots frame_slots;

    // FILE *fp_dbg_file[VP8D_DBG_FILE_NUM];
    FILE *fp_dbg_yuv;
} VP8DParserContext_t;

MPP_RET  vp8d_parser_init   (void *ctx, ParserCfg *cfg);
MPP_RET  vp8d_parser_deinit (void *ctx);
MPP_RET  vp8d_parser_reset  (void *ctx);
MPP_RET  vp8d_parser_flush  (void *ctx);
MPP_RET  vp8d_parser_control(void *ctx, RK_S32 cmd_type, void *param);
MPP_RET  vp8d_parser_prepare(void *ctx, MppPacket pkt, HalDecTask *task);
MPP_RET  vp8d_parser_parse  (void *ctx, HalDecTask *task);
MPP_RET  vp8d_parser_callback(void *ctx, void *hal_info);

#endif

