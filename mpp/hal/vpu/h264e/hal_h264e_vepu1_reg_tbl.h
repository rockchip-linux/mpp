/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#ifndef __HAL_H264E_VEPU1_REG_TBL_H__
#define __HAL_H264E_VEPU1_REG_TBL_H__

#include "hal_h264e_vepu.h"

/* RK3288 Encoder registers. */
#define VEPU_REG_INTERRUPT          0x004
#define     VEPU_REG_INTERRUPT_SLICE_READY      BIT(8)
#define     VEPU_REG_INTERRUPT_TIMEOUT          BIT(6)
#define     VEPU_REG_INTERRUPT_BUFFER_FULL      BIT(5)
#define     VEPU_REG_INTERRUPT_RESET            BIT(4)
#define     VEPU_REG_INTERRUPT_BUS_ERROR        BIT(3)
#define     VEPU_REG_INTERRUPT_FRAME_READY      BIT(2)
#define     VEPU_REG_INTERRUPT_DIS_BIT          BIT(1)
#define     VEPU_REG_INTERRUPT_BIT              BIT(0)

#define VEPU_REG_AXI_CTRL           0x008
#define     VEPU_REG_AXI_CTRL_WRITE_ID(x)       (((x) & 0xff) << 24)
#define     VEPU_REG_AXI_CTRL_READ_ID(x)        (((x) & 0xff) << 16)
#define     VEPU_REG_OUTPUT_SWAP16              BIT(15)
#define     VEPU_REG_INPUT_SWAP16               BIT(14)
#define     VEPU_REG_AXI_CTRL_BURST_LEN(x)      (((x) & 0x3f) << 8)
#define     VEPU_REG_AXI_CTRL_BURST_DISABLE     BIT(7)
#define     VEPU_REG_AXI_CTRL_INCREMENT_MODE    BIT(6)
#define     VEPU_REG_AXI_CTRL_BURST_DISCARD     BIT(5)
#define     VEPU_REG_CLK_GATING_EN              BIT(4)
#define     VEPU_REG_OUTPUT_SWAP32              BIT(3)
#define     VEPU_REG_INPUT_SWAP32               BIT(2)
#define     VEPU_REG_OUTPUT_SWAP8               BIT(1)
#define     VEPU_REG_INPUT_SWAP8                BIT(0)

#define VEPU_REG_ADDR_OUTPUT_STREAM     0x014
#define VEPU_REG_ADDR_OUTPUT_CTRL       0x018
#define VEPU_REG_ADDR_REF_LUMA          0x01c
#define VEPU_REG_ADDR_REF_CHROMA        0x020

#define VEPU_REG_ADDR_REC_LUMA          0x024
#define VEPU_REG_ADDR_REC_CHROMA        0x028

#define VEPU_REG_ADDR_IN_LUMA           0x02c
#define VEPU_REG_ADDR_IN_CB             0x030
#define VEPU_REG_ADDR_IN_CR             0x034

#define VEPU_REG_ENCODE_CTRL            0x038
#define     VEPU_REG_INTERRUPT_TIMEOUT_EN           BIT(31)
#define     VEPU_REG_MV_WRITE_EN                    BIT(30)
#define     VEPU_REG_SIZE_TABLE_PRESENT             BIT(29)
#define     VEPU_REG_INTERRUPT_SLICE_READY_EN       BIT(28)
#define     VEPU_REG_MB_WIDTH(x)                    (((x) & 0x1ff) << 19)
#define     VEPU_REG_MB_HEIGHT(x)                   (((x) & 0x1ff) << 10)
#define     VEPU_REG_RECON_WRITE_DIS                BIT(6)
#define     VEPU_REG_PIC_TYPE(x)                    (((x) & 0x3) << 3)
#define     VEPU_REG_ENCODE_FORMAT(x)               (((x) & 0x3) << 1)
#define     VEPU_REG_ENCODE_ENABLE                  BIT(0)

#define VEPU_REG_ENC_INPUT_IMAGE_CTRL  0x03c
#define     VEPU_REG_IN_IMG_CHROMA_OFFSET(x)    (((x) & 0x7) << 29)
#define     VEPU_REG_IN_IMG_LUMA_OFFSET(x)      (((x) & 0x7) << 26)
#define     VEPU_REG_IN_IMG_CTRL_ROW_LEN(x)     (((x) & 0x3fff) << 12)
#define     VEPU_REG_IN_IMG_CTRL_OVRFLR_D4(x)   (((x) & 0x3) << 10)
#define     VEPU_REG_IN_IMG_CTRL_OVRFLB(x)      (((x) & 0xf) << 6)
#define     VEPU_REG_IN_IMG_CTRL_FMT(x)         (((x) & 0xf) << 2)
#define     VEPU_REG_IN_IMG_ROTATE_MODE(x)      (((x) & 0x3) << 0)

#define VEPU_REG_ENC_CTRL0          0x040
#define     VEPU_REG_PPS_INIT_QP(x)                 (((x) & 0x3f) << 26)
#define     VEPU_REG_SLICE_FILTER_ALPHA(x)          (((x) & 0xf) << 22)
#define     VEPU_REG_SLICE_FILTER_BETA(x)           (((x) & 0xf) << 18)
#define     VEPU_REG_CHROMA_QP_OFFSET(x)            (((x) & 0x1f) << 13)
#define     VEPU_REG_IDR_PIC_ID(x)                  (((x) & 0xf) << 1)
#define     VEPU_REG_CONSTRAINED_INTRA_PREDICTION   BIT(0)

#define VEPU_REG_ENC_CTRL1          0x044
#define     VEPU_REG_PPS_ID(x)              (((x) & 0xff) << 24)
#define     VEPU_REG_INTRA_PRED_MODE(x)     (((x) & 0xff) << 16)
#define     VEPU_REG_FRAME_NUM(x)           (((x) & 0xffff) << 0)

#define VEPU_REG_ENC_CTRL2          0x048
#define     VEPU_REG_DEBLOCKING_FILTER_MODE(x)  (((x) & 0x3) << 30)
#define     VEPU_REG_H264_SLICE_SIZE(x)         (((x) & 0x7f) << 23)
#define     VEPU_REG_DISABLE_QUARTER_PIXEL_MV   BIT(22)
#define     VEPU_REG_H264_TRANS8X8_MODE         BIT(21)
#define     VEPU_REG_CABAC_INIT_IDC(x)          (((x) & 0x3) << 19)
#define     VEPU_REG_ENTROPY_CODING_MODE        BIT(18)
#define     VEPU_REG_H264_INTER4X4_MODE         BIT(17)
#define     VEPU_REG_H264_STREAM_MODE           BIT(16)
#define     VEPU_REG_INTRA16X16_MODE(x)         (((x) & 0xffff) << 0)

#define VEPU_REG_ENC_CTRL3         0x04c
#define     VEPU_REG_SPLIT_MV_MODE_EN           BIT(30)
#define     VEPU_REG_QMV_PENALTY(x)             (((x) & 0x3ff) << 20)
#define     VEPU_REG_4MV_PENALTY(x)             (((x) & 0x3ff) << 10)
#define     VEPU_REG_1MV_PENALTY(x)             (((x) & 0x3ff) << 0)

#define VEPU_REG_ENC_CTRL_4           0x054
#define     VEPU_REG_SKIP_MACROBLOCK_PENALTY(x)     (((x) & 0xff) << 24)
#define     VEPU_REG_COMPLETED_SLICES(x)            (((x) & 0xff) << 16)
#define     VEPU_REG_INTER_MODE(x)                  (((x) & 0xffff) << 0)

#define VEPU_REG_STR_HDR_REM_MSB        0x058
#define VEPU_REG_STR_HDR_REM_LSB        0x05c
#define VEPU_REG_STR_BUF_LIMIT          0x060

#define VEPU_REG_MAD_CTRL            0x064
#define     VEPU_REG_MAD_QP_ADJUSTMENT(x)   (((x) & 0xf) << 28)
#define     VEPU_REG_MAD_THRESHOLD(x)       (((x) & 0x3f) << 22)
#define     VEPU_REG_QP_SUM(x)              (((x) & 0x001fffff) * 2)

#define VEPU_REG_QP_VAL             0x06c
#define     VEPU_REG_H264_LUMA_INIT_QP(x)   (((x) & 0x3f) << 26)
#define     VEPU_REG_H264_QP_MAX(x)         (((x) & 0x3f) << 20)
#define     VEPU_REG_H264_QP_MIN(x)         (((x) & 0x3f) << 14)
#define     VEPU_REG_H264_CHKPT_DISTANCE(x) (((x) & 0xfff) << 0)

#define VEPU_REG_CHECKPOINT(i)                  (0x070 + ((i) * 0x4))
#define     VEPU_REG_CHECKPOINT_CHECK0(x)       (((x) & 0xffff))
#define     VEPU_REG_CHECKPOINT_CHECK1(x)       (((x) & 0xffff) << 16)
#define     VEPU_REG_CHECKPOINT_RESULT(x)       ((((x) >> (16 - 16 \
                             * (i & 1))) & 0xffff) \
                             * 32)

#define VEPU_REG_CHKPT_WORD_ERR(i)              (0x084 + ((i) * 0x4))
#define     VEPU_REG_CHKPT_WORD_ERR_CHK0(x)     (((x) & 0xffff))
#define     VEPU_REG_CHKPT_WORD_ERR_CHK1(x)     (((x) & 0xffff) << 16)

#define VEPU_REG_CHKPT_DELTA_QP         0x090
#define     VEPU_REG_CHKPT_DELTA_QP_CHK0(x)     (((x) & 0x0f) << 0)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK1(x)     (((x) & 0x0f) << 4)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK2(x)     (((x) & 0x0f) << 8)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK3(x)     (((x) & 0x0f) << 12)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK4(x)     (((x) & 0x0f) << 16)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK5(x)     (((x) & 0x0f) << 20)
#define     VEPU_REG_CHKPT_DELTA_QP_CHK6(x)     (((x) & 0x0f) << 24)

#define VEPU_REG_RLC_CTRL            0x094
#define     VEPU_REG_STREAM_START_OFFSET(x) (((x) & 0x3f) << 23)
#define     VEPU_REG_RLC_SUM            (((x) & 0x007fffff) << 0)
#define     VEPU_REG_RLC_SUM_OUT(x)         (((x) & 0x007fffff) * 4)

#define VEPU_REG_MB_CTRL            0x098
#define     VEPU_REG_MB_CNT_SET(x)          (((x) & 0xffff) << 16)
#define     VEPU_REG_MB_CNT_OUT(x)          (((x) & 0xffff) << 0)

#define VEPU_REG_ADDR_NEXT_PIC          0x09c

#define VEPU_REG_STABLILIZATION_OUTPUT      0x0a0
#define     VEPU_REG_STABLE_MODE_SEL(x)     (((x) & 0x3) << 30)
#define     VEPU_REG_STABLE_MIN_VALUE(x)    (((x) & 0xffffff) << 0)

#define VEPU_REG_STABLE_MOTION_SUM      0x0a4
#define VEPU_REG_ADDR_CABAC_TBL         0x0cc
#define VEPU_REG_ADDR_MV_OUT            0x0d0

#define VEPU_REG_RGB2YUV_CONVERSION_COEF1   0x0d4
#define     VEPU_REG_RGB2YUV_CONVERSION_COEFB(x)    (((x) & 0xffff) << 16)
#define     VEPU_REG_RGB2YUV_CONVERSION_COEFA(x)    (((x) & 0xffff) << 0)

#define VEPU_REG_RGB2YUV_CONVERSION_COEF2   0x0d8
#define     VEPU_REG_RGB2YUV_CONVERSION_COEFE(x)    (((x) & 0xffff) << 16)
#define     VEPU_REG_RGB2YUV_CONVERSION_COEFC(x)    (((x) & 0xffff) << 0)

#define VEPU_REG_RGB_MASK_MSB           0x0dc
#define     VEPU_REG_RGB_MASK_B_MSB(x)              (((x) & 0x1f) << 26)
#define     VEPU_REG_RGB_MASK_G_MSB(x)              (((x) & 0x1f) << 21)
#define     VEPU_REG_RGB_MASK_R_MSB(x)              (((x) & 0x1f) << 16)
#define     VEPU_REG_RGB2YUV_CONVERSION_COEFF(x)    (((x) & 0xffff) << 0)

#define VEPU_REG_INTRA_AREA_CTRL        0x0e0
#define     VEPU_REG_INTRA_AREA_LEFT(x)         (((x) & 0xff) << 24)
#define     VEPU_REG_INTRA_AREA_RIGHT(x)        (((x) & 0xff) << 16)
#define     VEPU_REG_INTRA_AREA_TOP(x)          (((x) & 0xff) << 8)
#define     VEPU_REG_INTRA_AREA_BOTTOM(x)       (((x) & 0xff) << 0)

#define VEPU_REG_CIR_INTRA_CTRL         0x0e4
#define     VEPU_REG_CIR_INTRA_FIRST_MB(x)      (((x) & 0xffff) << 16)
#define     VEPU_REG_CIR_INTRA_INTERVAL(x)      (((x) & 0xffff) << 0)

#define VEPU_REG_ROI1               0x0f0
#define     VEPU_REG_ROI1_LEFT_MB(x)            (((x) & 0xff) << 24)
#define     VEPU_REG_ROI1_RIGHT_MB(x)           (((x) & 0xff) << 16)
#define     VEPU_REG_ROI1_TOP_MB(x)             (((x) & 0xff) << 8)
#define     VEPU_REG_ROI1_BOTTOM_MB(x)          (((x) & 0xff) << 0)

#define VEPU_REG_ROI2               0x0f4
#define     VEPU_REG_ROI2_LEFT_MB(x)            (((x) & 0xff) << 24)
#define     VEPU_REG_ROI2_RIGHT_MB(x)           (((x) & 0xff) << 16)
#define     VEPU_REG_ROI2_TOP_MB(x)             (((x) & 0xff) << 8)
#define     VEPU_REG_ROI2_BOTTOM_MB(x)          (((x) & 0xff) << 0)

#define VEPU_REG_MVC_RELATE         0x0f8
#define     VEPU_REG_ZERO_MV_FAVOR_D2(x)        (((x) & 0xf) << 28)
#define     VEPU_REG_PENALTY_4X4MV(x)           (((x) & 0x1ff) << 19)
#define     VEPU_REG_MVC_PRIORITY_ID(x)         (((x) & 0x7) << 16)
#define     VEPU_REG_MVC_VIEW_ID(x)             (((x) & 0x7) << 13)
#define     VEPU_REG_MVC_TEMPORAL_ID(x)         (((x) & 0x7) << 10)
#define     VEPU_REG_MVC_ANCHOR_PIC_FLAG        BIT(9)
#define     VEPU_REG_MVC_INTER_VIEW_FLAG        BIT(8)
#define     VEPU_REG_ROI_QP_DELTA_1             (((x) & 0xf) << 4)
#define     VEPU_REG_ROI_QP_DELTA_2             (((x) & 0xf) << 0)

#define VEPU_REG_DMV_PENALTY_TBL(i)     (0x180 + ((i) * 0x4))
#define     VEPU_REG_DMV_PENALTY_TABLE_BIT(x, i)        (x << i * 8)

#define VEPU_REG_DMV_Q_PIXEL_PENALTY_TBL(i) (0x200 + ((i) * 0x4))
#define     VEPU_REG_DMV_Q_PIXEL_PENALTY_TABLE_BIT(x, i)    (x << i * 8)

#define     VEPU_H264E_VEPU1_NUM_REGS  164

typedef struct h264e_vepu1_reg_set_t {
    RK_U32 val[VEPU_H264E_VEPU1_NUM_REGS];
} h264e_vepu1_reg_set;

#endif
