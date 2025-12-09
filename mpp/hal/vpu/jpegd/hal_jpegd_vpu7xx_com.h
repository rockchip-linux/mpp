/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_JPEGD_VPU7xx_COM_H
#define HAL_JPEGD_VPU7xx_COM_H

#include "jpegd_syntax.h"
#include "hal_jpegd_base.h"

#define RKV_JPEGD_LITTLE_ENDIAN (0)
#define RKV_JPEGD_BIG_ENDIAN    (1)

#define SCALEDOWN_DISABLE       (0)
#define SCALEDOWN_HALF          (1)
#define SCALEDOWN_QUARTER       (2)
#define SCALEDOWN_ONE_EIGHTS    (3)

#define OUTPUT_RASTER           (0)
#define OUTPUT_TILE             (1)

#define TIMEOUT_MODE_CYCLE_24   (0)
#define TIMEOUT_MODE_CYCLE_18   (1)

#define OUT_SEQUENCE_RASTER     (0)
#define OUT_SEQUENCE_TILE       (1)

#define YUV_TO_RGB_REC_BT601    (0)
#define YUV_TO_RGB_REC_BT709    (1)

#define YUV_TO_RGB_FULL_RANGE   (1)
#define YUV_TO_RGB_LIMIT_RANGE  (0)

#define YUV_OUT_FMT_NO_TRANS    (0)
#define YUV_OUT_FMT_2_RGB888    (1)
#define YUV_OUT_FMT_2_RGB565    (2)
// Not support YUV400 transmit to NV12
#define YUV_OUT_FMT_2_NV12      (3)
// Only support YUV422 or YUV444, YUV444 should scaledown uv
#define YUV_OUT_FMT_2_YUYV      (4)

#define YUV_MODE_400            (0)
#define YUV_MODE_411            (1)
#define YUV_MODE_420            (2)
#define YUV_MODE_422            (3)
#define YUV_MODE_440            (4)
#define YUV_MODE_444            (5)

#define BIT_DEPTH_8             (0)
#define BIT_DEPTH_12            (1)

// No quantization/huffman table or table is the same as previous
#define TBL_ENTRY_0             (0)
// Grayscale picture with only 1 quantization/huffman table
#define TBL_ENTRY_1             (1)
// Common case, one table for luma, one for chroma
#define TBL_ENTRY_2             (2)
// 3 table entries, one for luma, one for cb, one for cr
#define TBL_ENTRY_3             (3)

// Restart interval marker disable
#define RST_DISABLE             (0)
// Restart interval marker enable
#define RST_ENABLE              (1)

// Support 8-bit precision only
#define NB_COMPONENTS (3)
#define RKD_QUANTIZATION_TBL_SIZE (8*8*2*NB_COMPONENTS)
#define RKD_HUFFMAN_MINCODE_TBL_SIZE (16*3*2*NB_COMPONENTS)
#define RKD_HUFFMAN_VALUE_TBL_SIZE (16*12*NB_COMPONENTS)
#define RKD_HUFFMAN_MINCODE_TBL_OFFSET (RKD_QUANTIZATION_TBL_SIZE)
#define RKD_HUFFMAN_VALUE_TBL_OFFSET (RKD_HUFFMAN_MINCODE_TBL_OFFSET + MPP_ALIGN(RKD_HUFFMAN_MINCODE_TBL_SIZE, 64))
#define RKD_TABLE_SIZE (RKD_HUFFMAN_VALUE_TBL_OFFSET + RKD_HUFFMAN_VALUE_TBL_SIZE)

MPP_RET jpegd_vpu7xx_write_qtbl(JpegdHalCtx *ctx, JpegdSyntax *syntax);
MPP_RET jpegd_vpu7xx_write_htbl(JpegdHalCtx *ctx, JpegdSyntax *jpegd_syntax);

#endif /* HAL_JPEGD_VPU7xx_COM_H */