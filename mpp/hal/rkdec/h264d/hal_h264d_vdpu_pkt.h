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

#ifndef __HAL_H264D_VDPU_PKT_H__
#define __HAL_H264D_VDPU_PKT_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "hal_task.h"
#include "hal_regdrv.h"
#include "h264d_log.h"
#include "hal_h264d_fifo.h"


#define DEC_X170_ALIGN_MASK         0x07

#define DEC_X170_MODE_H264          0x00

#define DEC_X170_IRQ_DEC_RDY        0x01
#define DEC_X170_IRQ_BUS_ERROR      0x02
#define DEC_X170_IRQ_BUFFER_EMPTY   0x04
#define DEC_X170_IRQ_ASO            0x08
#define DEC_X170_IRQ_STREAM_ERROR   0x10
#define DEC_X170_IRQ_TIMEOUT        0x40

/* predefined values of HW system parameters. DO NOT ALTER! */
#define DEC_X170_LITTLE_ENDIAN       1
#define DEC_X170_BIG_ENDIAN          0

#define DEC_X170_BUS_BURST_LENGTH_UNDEFINED         0
#define DEC_X170_BUS_BURST_LENGTH_4                 4
#define DEC_X170_BUS_BURST_LENGTH_8                 8
#define DEC_X170_BUS_BURST_LENGTH_16                16

#define DEC_X170_ASIC_SERVICE_PRIORITY_DEFAULT      0
#define DEC_X170_ASIC_SERVICE_PRIORITY_WR_1         1
#define DEC_X170_ASIC_SERVICE_PRIORITY_WR_2         2
#define DEC_X170_ASIC_SERVICE_PRIORITY_RD_1         3
#define DEC_X170_ASIC_SERVICE_PRIORITY_RD_2         4

#define DEC_X170_OUTPUT_FORMAT_RASTER_SCAN          0
#define DEC_X170_OUTPUT_FORMAT_TILED                1
/* end of predefined values */

/* now what we use */
#ifndef DEC_X170_USING_IRQ
/* Control IRQ generation by decoder hardware */
#define DEC_X170_USING_IRQ                1
#endif

#ifndef DEC_X170_ASIC_SERVICE_PRIORITY
/* hardware intgernal prioriy scheme. better left unchanged */
#define DEC_X170_ASIC_SERVICE_PRIORITY    DEC_X170_ASIC_SERVICE_PRIORITY_DEFAULT
#endif

/* AXI single command multiple data disable not set */
#define DEC_X170_SCMD_DISABLE             0

/* Advanced prefetch disable flag. If disable flag is set, product shall
 * operate akin to 9190 and earlier products. */
#define DEC_X170_APF_DISABLE              0

#ifndef DEC_X170_BUS_BURST_LENGTH
/* how long are the hardware data bursts; better left unchanged    */
#define DEC_X170_BUS_BURST_LENGTH         DEC_X170_BUS_BURST_LENGTH_16
#endif

#ifndef DEC_X170_INPUT_STREAM_ENDIAN
/* this should match the system endianess, so that Decoder reads      */
/* the input stream in the right order                                */
#define DEC_X170_INPUT_STREAM_ENDIAN      DEC_X170_LITTLE_ENDIAN
#endif

#ifndef DEC_X170_OUTPUT_PICTURE_ENDIAN
/* this should match the system endianess, so that Decoder writes     */
/* the output pixel data in the right order                           */
#define DEC_X170_OUTPUT_PICTURE_ENDIAN    DEC_X170_LITTLE_ENDIAN
#endif

#ifndef DEC_X170_LATENCY_COMPENSATION
/* compensation for bus latency; values up to 63 */
#define DEC_X170_LATENCY_COMPENSATION     0
#endif

#ifndef DEC_X170_INTERNAL_CLOCK_GATING
/* clock is gated from decoder structures that are not used */
#define DEC_X170_INTERNAL_CLOCK_GATING    1
#endif

#ifndef DEC_X170_OUTPUT_FORMAT
/* Decoder output picture format in external memory: Raster-scan or     */
/*macroblock tiled i.e. macroblock data written in consecutive addresses */
#define DEC_X170_OUTPUT_FORMAT            DEC_X170_OUTPUT_FORMAT_RASTER_SCAN
#endif

#ifndef DEC_X170_DATA_DISCARD_ENABLE
#define DEC_X170_DATA_DISCARD_ENABLE      0
#endif

/* Decoder output data swap for 32bit words*/
#ifndef DEC_X170_OUTPUT_SWAP_32_ENABLE
#define DEC_X170_OUTPUT_SWAP_32_ENABLE         DEC_X170_LITTLE_ENDIAN
#endif

/* Decoder input data swap(excluding stream data) for 32bit words*/
#ifndef DEC_X170_INPUT_DATA_SWAP_32_ENABLE
#define DEC_X170_INPUT_DATA_SWAP_32_ENABLE     DEC_X170_LITTLE_ENDIAN
#endif

/* Decoder input stream swap for 32bit words */
#ifndef DEC_X170_INPUT_STREAM_SWAP_32_ENABLE
#define DEC_X170_INPUT_STREAM_SWAP_32_ENABLE   DEC_X170_LITTLE_ENDIAN
#endif

/* Decoder input data endian. Do not modify this! */
#ifndef DEC_X170_INPUT_DATA_ENDIAN
#define DEC_X170_INPUT_DATA_ENDIAN             DEC_X170_BIG_ENDIAN
#endif

/* AXI bus read and write ID values used by HW. 0 - 255 */
#ifndef DEC_X170_AXI_ID_R
#define DEC_X170_AXI_ID_R                      0xff
#endif

#ifndef DEC_X170_AXI_ID_W
#define DEC_X170_AXI_ID_W                      0
#endif

/* Check validity of values */
/* data discard and tiled mode can not be on simultaneously */
#if (DEC_X170_DATA_DISCARD_ENABLE && (DEC_X170_OUTPUT_FORMAT == DEC_X170_OUTPUT_FORMAT_TILED))
#error "Bad value specified: DEC_X170_DATA_DISCARD_ENABLE && (DEC_X170_OUTPUT_FORMAT == DEC_X170_OUTPUT_FORMAT_TILED)"
#endif

#if (DEC_X170_OUTPUT_PICTURE_ENDIAN > 1)
#error "Bad value specified for DEC_X170_OUTPUT_PICTURE_ENDIAN"
#endif

#if (DEC_X170_OUTPUT_FORMAT > 1)
#error "Bad value specified for DEC_X170_OUTPUT_FORMAT"
#endif

#if (DEC_X170_BUS_BURST_LENGTH > 31)
#error "Bad value specified for DEC_X170_AMBA_BURST_LENGTH"
#endif

#if (DEC_X170_ASIC_SERVICE_PRIORITY > 4)
#error "Bad value specified for DEC_X170_ASIC_SERVICE_PRIORITY"
#endif

#if (DEC_X170_LATENCY_COMPENSATION > 63)
#error "Bad value specified for DEC_X170_LATENCY_COMPENSATION"
#endif

#if (DEC_X170_OUTPUT_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_OUTPUT_SWAP_32_ENABLE"
#endif

#if (DEC_X170_INPUT_DATA_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_DATA_SWAP_32_ENABLE"
#endif

#if (DEC_X170_INPUT_STREAM_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_STREAM_SWAP_32_ENABLE"
#endif

#if (DEC_X170_OUTPUT_SWAP_32_ENABLE > 1)
#error "Bad value specified for DEC_X170_INPUT_DATA_ENDIAN"
#endif

#if (DEC_X170_DATA_DISCARD_ENABLE > 1)
#error "Bad value specified for DEC_X170_DATA_DISCARD_ENABLE"
#endif

/* Common defines for the decoder */

/* Number registers for the decoder */
#define DEC_X170_REGISTERS          60//101//60

/* Max amount of stream */
#define DEC_X170_MAX_STREAM         (1<<24)

/* Timeout value for the VPUWaitHwReady() call. */
/* Set to -1 for an unspecified value */
#ifndef DEC_X170_TIMEOUT_LENGTH
#define DEC_X170_TIMEOUT_LENGTH     -1
#endif

/* Enable HW internal watchdog timeout IRQ */
#define DEC_X170_HW_TIMEOUT_INT_ENA  1

/* Memory wait states for reference buffer */
#define DEC_X170_REFBU_WIDTH        64
#define DEC_X170_REFBU_LATENCY      20
#define DEC_X170_REFBU_NONSEQ       8
#define DEC_X170_REFBU_SEQ          1



#define VDPU_CABAC_TAB_SIZE        (3680)        /* bytes */
#define VDPU_SCALING_LIST_SIZE     (6*16+2*64)   /* bytes */
#define VDPU_POC_BUF_SIZE          (34*4)        /* bytes */


typedef enum hwif_vdpu_type {
    VDPU_BUILD_VERSION     ,
    VDPU_ID_ASCII_EN       ,
    VDPU_MINOR_VERSION     ,
    VDPU_MAJOR_VERSION     ,
    VDPU_PRO_NUM           ,
    VDPU_DEC_PIC_INF       ,
    VDPU_DEC_TIMEOUT       ,
    VDPU_DEC_SLICE_INT     ,
    VDPU_DEC_ERROR_INT     ,
    VDPU_DEC_ASO_INT       ,
    VDPU_DEC_BUFFER_INT    ,
    VDPU_DEC_BUS_INT       ,
    VDPU_DEC_RDY_INT       ,
    VDPU_DEC_IRQ           ,
    VDPU_DEC_IRQ_DIS       ,
    VDPU_DEC_E             ,
    VDPU_DEC_AXI_RD_ID     ,
    VDPU_DEC_TIMEOUT_E     ,
    VDPU_DEC_STRSWAP32_E   ,
    VDPU_DEC_STRENDIAN_E   ,
    VDPU_DEC_INSWAP32_E    ,
    VDPU_DEC_OUTSWAP32_E   ,
    VDPU_DEC_DATA_DISC_E   ,
    VDPU_DEC_OUT_TILED_E   ,
    VDPU_DEC_LATENCY       ,
    VDPU_DEC_CLK_GATE_E    ,
    VDPU_DEC_IN_ENDIAN     ,
    VDPU_DEC_OUT_ENDIAN    ,
    VDPU_PRIORITY_MODE     ,
    VDPU_DEC_ADV_PRE_DIS   ,
    VDPU_DEC_SCMD_DIS      ,
    VDPU_DEC_MAX_BURST     ,
    VDPU_DEC_MODE          ,
    VDPU_RLC_MODE_E        ,
    VDPU_SKIP_MODE         ,
    VDPU_DIVX3_E           ,
    VDPU_PJPEG_E           ,
    VDPU_PIC_INTERLACE_E   ,
    VDPU_PIC_FIELDMODE_E   ,
    VDPU_PIC_B_E           ,
    VDPU_PIC_INTER_E       ,
    VDPU_PIC_TOPFIELD_E    ,
    VDPU_FWD_INTERLACE_E   ,
    VDPU_SORENSON_E        ,
    VDPU_REF_TOPFIELD_E    ,
    VDPU_DEC_OUT_DIS       ,
    VDPU_FILTERING_DIS     ,
    VDPU_MVC_E             ,
    VDPU_PIC_FIXED_QUANT   ,
    VDPU_WRITE_MVS_E       ,
    VDPU_REFTOPFIRST_E     ,
    VDPU_SEQ_MBAFF_E       ,
    VDPU_PICORD_COUNT_E    ,
    VDPU_DEC_AHB_HLOCK_E   ,
    VDPU_DEC_AXI_WR_ID     ,
    VDPU_PIC_MB_WIDTH      ,
    VDPU_MB_WIDTH_OFF      ,
    VDPU_PIC_MB_HEIGHT_P   ,
    VDPU_MB_HEIGHT_OFF     ,
    VDPU_ALT_SCAN_E        ,
    VDPU_TOPFIELDFIRST_E   ,
    VDPU_REF_FRAMES        ,
    VDPU_PIC_MB_W_EXT      ,
    VDPU_PIC_MB_H_EXT      ,
    VDPU_PIC_REFER_FLAG    ,
    VDPU_STRM_START_BIT    ,
    VDPU_SYNC_MARKER_E     ,
    VDPU_TYPE1_QUANT_E     ,
    VDPU_CH_QP_OFFSET      ,
    VDPU_CH_QP_OFFSET2     ,
    VDPU_FIELDPIC_FLAG_E   ,
    VDPU_INTRADC_VLC_THR   ,
    VDPU_VOP_TIME_INCR     ,
    VDPU_DQ_PROFILE        ,
    VDPU_DQBI_LEVEL        ,
    VDPU_RANGE_RED_FRM_E   ,
    VDPU_FAST_UVMC_E       ,
    VDPU_TRANSDCTAB        ,
    VDPU_TRANSACFRM        ,
    VDPU_TRANSACFRM2       ,
    VDPU_MB_MODE_TAB       ,
    VDPU_MVTAB             ,
    VDPU_CBPTAB            ,
    VDPU_2MV_BLK_PAT_TAB   ,
    VDPU_4MV_BLK_PAT_TAB   ,
    VDPU_QSCALE_TYPE       ,
    VDPU_CON_MV_E          ,
    VDPU_INTRA_DC_PREC     ,
    VDPU_INTRA_VLC_TAB     ,
    VDPU_FRAME_PRED_DCT    ,
    VDPU_JPEG_QTABLES      ,
    VDPU_JPEG_MODE         ,
    VDPU_JPEG_FILRIGHT_E   ,
    VDPU_JPEG_STREAM_ALL   ,
    VDPU_CR_AC_VLCTABLE    ,
    VDPU_CB_AC_VLCTABLE    ,
    VDPU_CR_DC_VLCTABLE    ,
    VDPU_CB_DC_VLCTABLE    ,
    VDPU_CR_DC_VLCTABLE3   ,
    VDPU_CB_DC_VLCTABLE3   ,
    VDPU_STRM1_START_BIT   ,
    VDPU_HUFFMAN_E         ,
    VDPU_MULTISTREAM_E     ,
    VDPU_BOOLEAN_VALUE     ,
    VDPU_BOOLEAN_RANGE     ,
    VDPU_ALPHA_OFFSET      ,
    VDPU_BETA_OFFSET       ,
    VDPU_START_CODE_E      ,
    VDPU_INIT_QP           ,
    VDPU_CH_8PIX_ILEAV_E   ,
    VDPU_STREAM_LEN        ,
    VDPU_CABAC_E           ,
    VDPU_BLACKWHITE_E      ,
    VDPU_DIR_8X8_INFER_E   ,
    VDPU_WEIGHT_PRED_E     ,
    VDPU_WEIGHT_BIPR_IDC   ,
    VDPU_FRAMENUM_LEN      ,
    VDPU_FRAMENUM          ,
    VDPU_BITPLANE0_E       ,
    VDPU_BITPLANE1_E       ,
    VDPU_BITPLANE2_E       ,
    VDPU_ALT_PQUANT        ,
    VDPU_DQ_EDGES          ,
    VDPU_TTMBF             ,
    VDPU_PQINDEX           ,
    VDPU_BILIN_MC_E        ,
    VDPU_UNIQP_E           ,
    VDPU_HALFQP_E          ,
    VDPU_TTFRM             ,
    VDPU_2ND_BYTE_EMUL_E   ,
    VDPU_DQUANT_E          ,
    VDPU_VC1_ADV_E         ,
    VDPU_PJPEG_FILDOWN_E   ,
    VDPU_PJPEG_WDIV8       ,
    VDPU_PJPEG_HDIV8       ,
    VDPU_PJPEG_AH          ,
    VDPU_PJPEG_AL          ,
    VDPU_PJPEG_SS          ,
    VDPU_PJPEG_SE          ,
    VDPU_DCT1_START_BIT    ,
    VDPU_DCT2_START_BIT    ,
    VDPU_CH_MV_RES         ,
    VDPU_INIT_DC_MATCH0    ,
    VDPU_INIT_DC_MATCH1    ,
    VDPU_VP7_VERSION       ,
    VDPU_CONST_INTRA_E     ,
    VDPU_FILT_CTRL_PRES    ,
    VDPU_RDPIC_CNT_PRES    ,
    VDPU_8X8TRANS_FLAG_E   ,
    VDPU_REFPIC_MK_LEN     ,
    VDPU_IDR_PIC_E         ,
    VDPU_IDR_PIC_ID        ,
    VDPU_MV_SCALEFACTOR    ,
    VDPU_REF_DIST_FWD      ,
    VDPU_REF_DIST_BWD      ,
    VDPU_LOOP_FILT_LIMIT   ,
    VDPU_VARIANCE_TEST_E   ,
    VDPU_MV_THRESHOLD      ,
    VDPU_VAR_THRESHOLD     ,
    VDPU_DIVX_IDCT_E       ,
    VDPU_DIVX3_SLICE_SIZE  ,
    VDPU_PJPEG_REST_FREQ   ,
    VDPU_RV_PROFILE        ,
    VDPU_RV_OSV_QUANT      ,
    VDPU_RV_FWD_SCALE      ,
    VDPU_RV_BWD_SCALE      ,
    VDPU_INIT_DC_COMP0     ,
    VDPU_INIT_DC_COMP1     ,
    VDPU_PPS_ID            ,
    VDPU_REFIDX1_ACTIVE    ,
    VDPU_REFIDX0_ACTIVE    ,
    VDPU_POC_LENGTH        ,
    VDPU_ICOMP0_E          ,
    VDPU_ISCALE0           ,
    VDPU_ISHIFT0           ,
    VDPU_STREAM1_LEN       ,
    VDPU_MB_CTRL_BASE      ,
    VDPU_PIC_SLICE_AM      ,
    VDPU_COEFFS_PART_AM    ,
    VDPU_DIFF_MV_BASE      ,
    VDPU_PINIT_RLIST_F9    ,
    VDPU_PINIT_RLIST_F8    ,
    VDPU_PINIT_RLIST_F7    ,
    VDPU_PINIT_RLIST_F6    ,
    VDPU_PINIT_RLIST_F5    ,
    VDPU_PINIT_RLIST_F4    ,
    VDPU_ICOMP1_E          ,
    VDPU_ISCALE1           ,
    VDPU_ISHIFT1           ,
    VDPU_SEGMENT_BASE      ,
    VDPU_SEGMENT_UPD_E     ,
    VDPU_SEGMENT_E         ,
    VDPU_I4X4_OR_DC_BASE   ,
    VDPU_PINIT_RLIST_F15   ,
    VDPU_PINIT_RLIST_F14   ,
    VDPU_PINIT_RLIST_F13   ,
    VDPU_PINIT_RLIST_F12   ,
    VDPU_PINIT_RLIST_F11   ,
    VDPU_PINIT_RLIST_F10   ,
    VDPU_ICOMP2_E          ,
    VDPU_ISCALE2           ,
    VDPU_ISHIFT2           ,
    VDPU_DCT3_START_BIT    ,
    VDPU_DCT4_START_BIT    ,
    VDPU_DCT5_START_BIT    ,
    VDPU_DCT6_START_BIT    ,
    VDPU_DCT7_START_BIT    ,
    VDPU_RLC_VLC_BASE      ,
    VDPU_DEC_OUT_BASE      ,
    VDPU_REFER0_BASE       ,
    VDPU_REFER0_FIELD_E    ,
    VDPU_REFER0_TOPC_E     ,
    VDPU_JPG_CH_OUT_BASE   ,
    VDPU_REFER1_BASE       ,
    VDPU_REFER1_FIELD_E    ,
    VDPU_REFER1_TOPC_E     ,
    VDPU_JPEG_SLICE_H      ,
    VDPU_REFER2_BASE       ,
    VDPU_REFER2_FIELD_E    ,
    VDPU_REFER2_TOPC_E     ,
    VDPU_AC1_CODE6_CNT     ,
    VDPU_AC1_CODE5_CNT     ,
    VDPU_AC1_CODE4_CNT     ,
    VDPU_AC1_CODE3_CNT     ,
    VDPU_AC1_CODE2_CNT     ,
    VDPU_AC1_CODE1_CNT     ,
    VDPU_REFER3_BASE       ,
    VDPU_REFER3_FIELD_E    ,
    VDPU_REFER3_TOPC_E     ,
    VDPU_AC1_CODE10_CNT    ,
    VDPU_AC1_CODE9_CNT     ,
    VDPU_AC1_CODE8_CNT     ,
    VDPU_AC1_CODE7_CNT     ,
    VDPU_REFER4_BASE       ,
    VDPU_REFER4_FIELD_E    ,
    VDPU_REFER4_TOPC_E     ,
    VDPU_PIC_HEADER_LEN    ,
    VDPU_PIC_4MV_E         ,
    VDPU_RANGE_RED_REF_E   ,
    VDPU_VC1_DIFMV_RANGE   ,
    VDPU_MV_RANGE          ,
    VDPU_OVERLAP_E         ,
    VDPU_OVERLAP_METHOD    ,
    VDPU_ALT_SCAN_FLAG_E   ,
    VDPU_FCODE_FWD_HOR     ,
    VDPU_FCODE_FWD_VER     ,
    VDPU_FCODE_BWD_HOR     ,
    VDPU_FCODE_BWD_VER     ,
    VDPU_MV_ACCURACY_FWD   ,
    VDPU_MV_ACCURACY_BWD   ,
    VDPU_MPEG4_VC1_RC      ,
    VDPU_PREV_ANC_TYPE     ,
    VDPU_AC1_CODE14_CNT    ,
    VDPU_AC1_CODE13_CNT    ,
    VDPU_AC1_CODE12_CNT    ,
    VDPU_AC1_CODE11_CNT    ,
    VDPU_GREF_SIGN_BIAS    ,
    VDPU_REFER5_BASE       ,
    VDPU_REFER5_FIELD_E    ,
    VDPU_REFER5_TOPC_E     ,
    VDPU_TRB_PER_TRD_D0    ,
    VDPU_ICOMP3_E          ,
    VDPU_ISCALE3           ,
    VDPU_ISHIFT3           ,
    VDPU_AC2_CODE4_CNT     ,
    VDPU_AC2_CODE3_CNT     ,
    VDPU_AC2_CODE2_CNT     ,
    VDPU_AC2_CODE1_CNT     ,
    VDPU_AC1_CODE16_CNT    ,
    VDPU_AC1_CODE15_CNT    ,
    VDPU_SCAN_MAP_1        ,
    VDPU_SCAN_MAP_2        ,
    VDPU_SCAN_MAP_3        ,
    VDPU_SCAN_MAP_4        ,
    VDPU_SCAN_MAP_5        ,
    VDPU_AREF_SIGN_BIAS    ,
    VDPU_REFER6_BASE       ,
    VDPU_REFER6_FIELD_E    ,
    VDPU_REFER6_TOPC_E     ,
    VDPU_TRB_PER_TRD_DM1   ,
    VDPU_ICOMP4_E          ,
    VDPU_ISCALE4           ,
    VDPU_ISHIFT4           ,
    VDPU_AC2_CODE8_CNT     ,
    VDPU_AC2_CODE7_CNT     ,
    VDPU_AC2_CODE6_CNT     ,
    VDPU_AC2_CODE5_CNT     ,
    VDPU_SCAN_MAP_6        ,
    VDPU_SCAN_MAP_7        ,
    VDPU_SCAN_MAP_8        ,
    VDPU_SCAN_MAP_9        ,
    VDPU_SCAN_MAP_10       ,
    VDPU_REFER7_BASE       ,
    VDPU_REFER7_FIELD_E    ,
    VDPU_REFER7_TOPC_E     ,
    VDPU_TRB_PER_TRD_D1    ,
    VDPU_AC2_CODE12_CNT    ,
    VDPU_AC2_CODE11_CNT    ,
    VDPU_AC2_CODE10_CNT    ,
    VDPU_AC2_CODE9_CNT     ,
    VDPU_SCAN_MAP_11       ,
    VDPU_SCAN_MAP_12       ,
    VDPU_SCAN_MAP_13       ,
    VDPU_SCAN_MAP_14       ,
    VDPU_SCAN_MAP_15       ,
    VDPU_REFER8_BASE       ,
    VDPU_DCT_STRM1_BASE    ,
    VDPU_REFER8_FIELD_E    ,
    VDPU_REFER8_TOPC_E     ,
    VDPU_AC2_CODE16_CNT    ,
    VDPU_AC2_CODE15_CNT    ,
    VDPU_AC2_CODE14_CNT    ,
    VDPU_AC2_CODE13_CNT    ,
    VDPU_SCAN_MAP_16       ,
    VDPU_SCAN_MAP_17       ,
    VDPU_SCAN_MAP_18       ,
    VDPU_SCAN_MAP_19       ,
    VDPU_SCAN_MAP_20       ,
    VDPU_REFER9_BASE       ,
    VDPU_DCT_STRM2_BASE    ,
    VDPU_REFER9_FIELD_E    ,
    VDPU_REFER9_TOPC_E     ,
    VDPU_DC1_CODE8_CNT     ,
    VDPU_DC1_CODE7_CNT     ,
    VDPU_DC1_CODE6_CNT     ,
    VDPU_DC1_CODE5_CNT     ,
    VDPU_DC1_CODE4_CNT     ,
    VDPU_DC1_CODE3_CNT     ,
    VDPU_DC1_CODE2_CNT     ,
    VDPU_DC1_CODE1_CNT     ,
    VDPU_SCAN_MAP_21       ,
    VDPU_SCAN_MAP_22       ,
    VDPU_SCAN_MAP_23       ,
    VDPU_SCAN_MAP_24       ,
    VDPU_SCAN_MAP_25       ,
    VDPU_REFER10_BASE      ,
    VDPU_DCT_STRM3_BASE    ,
    VDPU_REFER10_FIELD_E   ,
    VDPU_REFER10_TOPC_E    ,
    VDPU_DC1_CODE16_CNT    ,
    VDPU_DC1_CODE15_CNT    ,
    VDPU_DC1_CODE14_CNT    ,
    VDPU_DC1_CODE13_CNT    ,
    VDPU_DC1_CODE12_CNT    ,
    VDPU_DC1_CODE11_CNT    ,
    VDPU_DC1_CODE10_CNT    ,
    VDPU_DC1_CODE9_CNT     ,
    VDPU_SCAN_MAP_26       ,
    VDPU_SCAN_MAP_27       ,
    VDPU_SCAN_MAP_28       ,
    VDPU_SCAN_MAP_29       ,
    VDPU_SCAN_MAP_30       ,
    VDPU_REFER11_BASE      ,
    VDPU_DCT_STRM4_BASE    ,
    VDPU_REFER11_FIELD_E   ,
    VDPU_REFER11_TOPC_E    ,
    VDPU_DC2_CODE8_CNT     ,
    VDPU_DC2_CODE7_CNT     ,
    VDPU_DC2_CODE6_CNT     ,
    VDPU_DC2_CODE5_CNT     ,
    VDPU_DC2_CODE4_CNT     ,
    VDPU_DC2_CODE3_CNT     ,
    VDPU_DC2_CODE2_CNT     ,
    VDPU_DC2_CODE1_CNT     ,
    VDPU_SCAN_MAP_31       ,
    VDPU_SCAN_MAP_32       ,
    VDPU_SCAN_MAP_33       ,
    VDPU_SCAN_MAP_34       ,
    VDPU_SCAN_MAP_35       ,
    VDPU_REFER12_BASE      ,
    VDPU_DCT_STRM5_BASE    ,
    VDPU_REFER12_FIELD_E   ,
    VDPU_REFER12_TOPC_E    ,
    VDPU_DC2_CODE16_CNT    ,
    VDPU_DC2_CODE15_CNT    ,
    VDPU_DC2_CODE14_CNT    ,
    VDPU_DC2_CODE13_CNT    ,
    VDPU_DC2_CODE12_CNT    ,
    VDPU_DC2_CODE11_CNT    ,
    VDPU_DC2_CODE10_CNT    ,
    VDPU_DC2_CODE9_CNT     ,
    VDPU_SCAN_MAP_36       ,
    VDPU_SCAN_MAP_37       ,
    VDPU_SCAN_MAP_38       ,
    VDPU_SCAN_MAP_39       ,
    VDPU_SCAN_MAP_40       ,
    VDPU_REFER13_BASE      ,
    VDPU_REFER13_FIELD_E   ,
    VDPU_REFER13_TOPC_E    ,
    VDPU_DC3_CODE8_CNT     ,
    VDPU_DC3_CODE7_CNT     ,
    VDPU_DC3_CODE6_CNT     ,
    VDPU_DC3_CODE5_CNT     ,
    VDPU_DC3_CODE4_CNT     ,
    VDPU_DC3_CODE3_CNT     ,
    VDPU_DC3_CODE2_CNT     ,
    VDPU_DC3_CODE1_CNT     ,
    VDPU_BITPL_CTRL_BASE   ,
    VDPU_REFER14_BASE      ,
    VDPU_DCT_STRM6_BASE    ,
    VDPU_REFER14_FIELD_E   ,
    VDPU_REFER14_TOPC_E    ,
    VDPU_REF_INVD_CUR_1    ,
    VDPU_REF_INVD_CUR_0    ,
    VDPU_DC3_CODE16_CNT    ,
    VDPU_DC3_CODE15_CNT    ,
    VDPU_DC3_CODE14_CNT    ,
    VDPU_DC3_CODE13_CNT    ,
    VDPU_DC3_CODE12_CNT    ,
    VDPU_DC3_CODE11_CNT    ,
    VDPU_DC3_CODE10_CNT    ,
    VDPU_DC3_CODE9_CNT     ,
    VDPU_SCAN_MAP_41       ,
    VDPU_SCAN_MAP_42       ,
    VDPU_SCAN_MAP_43       ,
    VDPU_SCAN_MAP_44       ,
    VDPU_SCAN_MAP_45       ,
    VDPU_REFER15_BASE      ,
    VDPU_INTER_VIEW_BASE   ,
    VDPU_DCT_STRM7_BASE    ,
    VDPU_REFER15_FIELD_E   ,
    VDPU_REFER15_TOPC_E    ,
    VDPU_REF_INVD_CUR_3    ,
    VDPU_REF_INVD_CUR_2    ,
    VDPU_SCAN_MAP_46       ,
    VDPU_SCAN_MAP_47       ,
    VDPU_SCAN_MAP_48       ,
    VDPU_SCAN_MAP_49       ,
    VDPU_SCAN_MAP_50       ,
    VDPU_REFER1_NBR        ,
    VDPU_REFER0_NBR        ,
    VDPU_REF_DIST_CUR_1    ,
    VDPU_REF_DIST_CUR_0    ,
    VDPU_FILT_TYPE         ,
    VDPU_FILT_SHARPNESS    ,
    VDPU_FILT_MB_ADJ_0     ,
    VDPU_FILT_MB_ADJ_1     ,
    VDPU_FILT_MB_ADJ_2     ,
    VDPU_FILT_MB_ADJ_3     ,
    VDPU_REFER3_NBR        ,
    VDPU_REFER2_NBR        ,
    VDPU_SCAN_MAP_51       ,
    VDPU_SCAN_MAP_52       ,
    VDPU_SCAN_MAP_53       ,
    VDPU_SCAN_MAP_54       ,
    VDPU_SCAN_MAP_55       ,
    VDPU_REF_DIST_CUR_3    ,
    VDPU_REF_DIST_CUR_2    ,
    VDPU_FILT_REF_ADJ_0    ,
    VDPU_FILT_REF_ADJ_1    ,
    VDPU_FILT_REF_ADJ_2    ,
    VDPU_FILT_REF_ADJ_3    ,
    VDPU_REFER5_NBR        ,
    VDPU_REFER4_NBR        ,
    VDPU_SCAN_MAP_56       ,
    VDPU_SCAN_MAP_57       ,
    VDPU_SCAN_MAP_58       ,
    VDPU_SCAN_MAP_59       ,
    VDPU_SCAN_MAP_60       ,
    VDPU_REF_INVD_COL_1    ,
    VDPU_REF_INVD_COL_0    ,
    VDPU_FILT_LEVEL_0      ,
    VDPU_FILT_LEVEL_1      ,
    VDPU_FILT_LEVEL_2      ,
    VDPU_FILT_LEVEL_3      ,
    VDPU_REFER7_NBR        ,
    VDPU_REFER6_NBR        ,
    VDPU_SCAN_MAP_61       ,
    VDPU_SCAN_MAP_62       ,
    VDPU_SCAN_MAP_63       ,
    VDPU_REF_INVD_COL_3    ,
    VDPU_REF_INVD_COL_2    ,
    VDPU_QUANT_DELTA_0     ,
    VDPU_QUANT_DELTA_1     ,
    VDPU_QUANT_0           ,
    VDPU_QUANT_1           ,
    VDPU_REFER9_NBR        ,
    VDPU_REFER8_NBR        ,
    VDPU_PRED_BC_TAP_0_3   ,
    VDPU_PRED_BC_TAP_1_0   ,
    VDPU_PRED_BC_TAP_1_1   ,
    VDPU_REFER11_NBR       ,
    VDPU_REFER10_NBR       ,
    VDPU_PRED_BC_TAP_1_2   ,
    VDPU_PRED_BC_TAP_1_3   ,
    VDPU_PRED_BC_TAP_2_0   ,
    VDPU_REFER13_NBR       ,
    VDPU_REFER12_NBR       ,
    VDPU_PRED_BC_TAP_2_1   ,
    VDPU_PRED_BC_TAP_2_2   ,
    VDPU_PRED_BC_TAP_2_3   ,
    VDPU_REFER15_NBR       ,
    VDPU_REFER14_NBR       ,
    VDPU_PRED_BC_TAP_3_0   ,
    VDPU_PRED_BC_TAP_3_1   ,
    VDPU_PRED_BC_TAP_3_2   ,
    VDPU_REFER_LTERM_E     ,
    VDPU_PRED_BC_TAP_3_3   ,
    VDPU_PRED_BC_TAP_4_0   ,
    VDPU_PRED_BC_TAP_4_1   ,
    VDPU_REFER_VALID_E     ,
    VDPU_PRED_BC_TAP_4_2   ,
    VDPU_PRED_BC_TAP_4_3   ,
    VDPU_PRED_BC_TAP_5_0   ,
    VDPU_QTABLE_BASE       ,
    VDPU_DIR_MV_BASE       ,
    VDPU_BINIT_RLIST_B2    ,
    VDPU_BINIT_RLIST_F2    ,
    VDPU_BINIT_RLIST_B1    ,
    VDPU_BINIT_RLIST_F1    ,
    VDPU_BINIT_RLIST_B0    ,
    VDPU_BINIT_RLIST_F0    ,
    VDPU_PRED_BC_TAP_5_1   ,
    VDPU_PRED_BC_TAP_5_2   ,
    VDPU_PRED_BC_TAP_5_3   ,
    VDPU_PJPEG_DCCB_BASE   ,
    VDPU_BINIT_RLIST_B5    ,
    VDPU_BINIT_RLIST_F5    ,
    VDPU_BINIT_RLIST_B4    ,
    VDPU_BINIT_RLIST_F4    ,
    VDPU_BINIT_RLIST_B3    ,
    VDPU_BINIT_RLIST_F3    ,
    VDPU_PRED_BC_TAP_6_0   ,
    VDPU_PRED_BC_TAP_6_1   ,
    VDPU_PRED_BC_TAP_6_2   ,
    VDPU_PJPEG_DCCR_BASE   ,
    VDPU_BINIT_RLIST_B8    ,
    VDPU_BINIT_RLIST_F8    ,
    VDPU_BINIT_RLIST_B7    ,
    VDPU_BINIT_RLIST_F7    ,
    VDPU_BINIT_RLIST_B6    ,
    VDPU_BINIT_RLIST_F6    ,
    VDPU_PRED_BC_TAP_6_3   ,
    VDPU_PRED_BC_TAP_7_0   ,
    VDPU_PRED_BC_TAP_7_1   ,
    VDPU_BINIT_RLIST_B11   ,
    VDPU_BINIT_RLIST_F11   ,
    VDPU_BINIT_RLIST_B10   ,
    VDPU_BINIT_RLIST_F10   ,
    VDPU_BINIT_RLIST_B9    ,
    VDPU_BINIT_RLIST_F9    ,
    VDPU_PRED_BC_TAP_7_2   ,
    VDPU_PRED_BC_TAP_7_3   ,
    VDPU_PRED_TAP_2_M1     ,
    VDPU_PRED_TAP_2_4      ,
    VDPU_PRED_TAP_4_M1     ,
    VDPU_PRED_TAP_4_4      ,
    VDPU_PRED_TAP_6_M1     ,
    VDPU_PRED_TAP_6_4      ,
    VDPU_BINIT_RLIST_B14   ,
    VDPU_BINIT_RLIST_F14   ,
    VDPU_BINIT_RLIST_B13   ,
    VDPU_BINIT_RLIST_F13   ,
    VDPU_BINIT_RLIST_B12   ,
    VDPU_BINIT_RLIST_F12   ,
    VDPU_QUANT_DELTA_2     ,
    VDPU_QUANT_DELTA_3     ,
    VDPU_QUANT_2           ,
    VDPU_QUANT_3           ,
    VDPU_PINIT_RLIST_F3    ,
    VDPU_PINIT_RLIST_F2    ,
    VDPU_PINIT_RLIST_F1    ,
    VDPU_PINIT_RLIST_F0    ,
    VDPU_BINIT_RLIST_B15   ,
    VDPU_BINIT_RLIST_F15   ,
    VDPU_QUANT_DELTA_4     ,
    VDPU_QUANT_4           ,
    VDPU_QUANT_5           ,
    VDPU_STARTMB_X         ,
    VDPU_STARTMB_Y         ,
    VDPU_PRED_BC_TAP_0_0   ,
    VDPU_PRED_BC_TAP_0_1   ,
    VDPU_PRED_BC_TAP_0_2   ,
    VDPU_REFBU_E           ,
    VDPU_REFBU_THR         ,
    VDPU_REFBU_PICID       ,
    VDPU_REFBU_EVAL_E      ,
    VDPU_REFBU_FPARMOD_E   ,
    VDPU_REFBU_Y_OFFSET    ,
    VDPU_REFBU_HIT_SUM     ,
    VDPU_REFBU_INTRA_SUM   ,
    VDPU_REFBU_Y_MV_SUM    ,
    VDPU_REFBU2_BUF_E      ,
    VDPU_REFBU2_THR        ,
    VDPU_REFBU2_PICID      ,
    VDPU_APF_THRESHOLD     ,
    VDPU_REFBU_TOP_SUM     ,
    VDPU_REFBU_BOT_SUM     ,
    VDPU_DEC_CH8PIX_BASE   ,
    VDPU_PP_BUS_INT        ,
    VDPU_PP_RDY_INT        ,
    VDPU_PP_IRQ            ,
    VDPU_PP_IRQ_DIS        ,
    VDPU_PP_PIPELINE_E     ,
    VDPU_PP_E              ,
    VDPU_PP_AXI_RD_ID      ,
    VDPU_PP_AXI_WR_ID      ,
    VDPU_PP_AHB_HLOCK_E    ,
    VDPU_PP_SCMD_DIS       ,
    VDPU_PP_IN_A2_ENDSEL   ,
    VDPU_PP_IN_A1_SWAP32   ,
    VDPU_PP_IN_A1_ENDIAN   ,
    VDPU_PP_IN_SWAP32_E    ,
    VDPU_PP_DATA_DISC_E    ,
    VDPU_PP_CLK_GATE_E     ,
    VDPU_PP_IN_ENDIAN      ,
    VDPU_PP_OUT_ENDIAN     ,
    VDPU_PP_OUT_SWAP32_E   ,
    VDPU_PP_MAX_BURST      ,
    VDPU_DEINT_E           ,
    VDPU_DEINT_THRESHOLD   ,
    VDPU_DEINT_BLEND_E     ,
    VDPU_DEINT_EDGE_DET    ,
    VDPU_PP_IN_LU_BASE     ,
    VDPU_PP_IN_CB_BASE     ,
    VDPU_PP_IN_CR_BASE     ,
    VDPU_PP_OUT_LU_BASE    ,
    VDPU_PP_OUT_CH_BASE    ,
    VDPU_CONTRAST_THR1     ,
    VDPU_CONTRAST_OFF2     ,
    VDPU_CONTRAST_OFF1     ,
    VDPU_PP_IN_START_CH    ,
    VDPU_PP_IN_CR_FIRST    ,
    VDPU_PP_OUT_START_CH   ,
    VDPU_PP_OUT_CR_FIRST   ,
    VDPU_COLOR_COEFFA2     ,
    VDPU_COLOR_COEFFA1     ,
    VDPU_CONTRAST_THR2     ,
    VDPU_COLOR_COEFFD      ,
    VDPU_COLOR_COEFFC      ,
    VDPU_COLOR_COEFFB      ,
    VDPU_CROP_STARTX       ,
    VDPU_ROTATION_MODE     ,
    VDPU_COLOR_COEFFF      ,
    VDPU_COLOR_COEFFE      ,
    VDPU_CROP_STARTY       ,
    VDPU_RANGEMAP_COEF_Y   ,
    VDPU_PP_IN_HEIGHT      ,
    VDPU_PP_IN_WIDTH       ,
    VDPU_PP_BOT_YIN_BASE   ,
    VDPU_PP_BOT_CIN_BASE   ,
    VDPU_RANGEMAP_Y_E      ,
    VDPU_RANGEMAP_C_E      ,
    VDPU_YCBCR_RANGE       ,
    VDPU_RGB_PIX_IN32      ,
    VDPU_RGB_R_PADD        ,
    VDPU_RGB_G_PADD        ,
    VDPU_SCALE_WRATIO      ,
    VDPU_PP_IN_STRUCT      ,
    VDPU_HOR_SCALE_MODE    ,
    VDPU_VER_SCALE_MODE    ,
    VDPU_RGB_B_PADD        ,
    VDPU_SCALE_HRATIO      ,
    VDPU_WSCALE_INVRA      ,
    VDPU_HSCALE_INVRA      ,
    VDPU_R_MASK            ,
    VDPU_G_MASK            ,
    VDPU_B_MASK            ,
    VDPU_PP_IN_FORMAT      ,
    VDPU_PP_OUT_FORMAT     ,
    VDPU_PP_OUT_HEIGHT     ,
    VDPU_PP_OUT_WIDTH      ,
    VDPU_PP_OUT_SWAP16_E   ,
    VDPU_PP_CROP8_R_E      ,
    VDPU_PP_CROP8_D_E      ,
    VDPU_PP_IN_FORMAT_ES   ,
    VDPU_RANGEMAP_COEF_C   ,
    VDPU_MASK1_ABLEND_E    ,
    VDPU_MASK1_STARTY      ,
    VDPU_MASK1_STARTX      ,
    VDPU_MASK2_ABLEND_E    ,
    VDPU_MASK2_STARTY      ,
    VDPU_MASK2_STARTX      ,
    VDPU_EXT_ORIG_WIDTH    ,
    VDPU_MASK1_E           ,
    VDPU_MASK1_ENDY        ,
    VDPU_MASK1_ENDX        ,
    VDPU_MASK2_E           ,
    VDPU_MASK2_ENDY        ,
    VDPU_MASK2_ENDX        ,
    VDPU_RIGHT_CROSS_E     ,
    VDPU_LEFT_CROSS_E      ,
    VDPU_UP_CROSS_E        ,
    VDPU_DOWN_CROSS_E      ,
    VDPU_UP_CROSS          ,
    VDPU_DOWN_CROSS        ,
    VDPU_DITHER_SELECT_R   ,
    VDPU_DITHER_SELECT_G   ,
    VDPU_DITHER_SELECT_B   ,
    VDPU_RIGHT_CROSS       ,
    VDPU_LEFT_CROSS        ,
    VDPU_PP_IN_H_EXT       ,
    VDPU_PP_IN_W_EXT       ,
    VDPU_CROP_STARTY_EXT   ,
    VDPU_CROP_STARTX_EXT   ,
    VDPU_DISPLAY_WIDTH     ,
    VDPU_ABLEND1_BASE      ,
    VDPU_ABLEND2_BASE      ,
    VDPU_DEC_IRQ_STAT      ,

    VDPU_MAX_SIZE          ,
} HwifVdpuType;




typedef struct h264d_vdpu_dpb_t {
    RK_U32    valid;
    RK_U32    have_same;

    RK_S32    TOP_POC;
    RK_S32    BOT_POC;
    RK_U16    frame_num;
    RK_U8     non_exist_frame;
    RK_U8     is_ilt;    //!< in-terview frame flag
    RK_U8     slot_index;
    RK_U32    is_long_term;
    RK_U8     top_used;
    RK_U8     bot_used;
    RK_U32    LongTermPicNum;

} H264dVdpuDpb_t;



typedef struct h264d_old_DXVA_t {
    H264dVdpuDpb_t old_dpb[2][16];
    H264dVdpuDpb_t new_dpb[16];
    RK_U32         new_dpb_cnt;

    H264dVdpuDpb_t ilt_dpb[16];
    RK_U32         ilt_dpb_cnt;
} H264dHalOldDXVA_t;



#ifdef  __cplusplus
extern "C" {
#endif

extern const RK_U32 H264_VDPU_Cabac_table[VDPU_CABAC_TAB_SIZE / 4];
extern const HalRegDrv_t g_vdpu_drv[VDPU_MAX_SIZE + 1];
extern const RK_U32 g_refPicNum[16];
extern const RK_U32 g_refPicList0[16];
extern const RK_U32 g_refPicList1[16];
extern const RK_U32 g_refPicListP[16];
extern const RK_U32 g_refBase[16];


MPP_RET vdpu_set_pic_regs(void *hal, HalRegDrvCtx_t *p_drv);
MPP_RET vdpu_set_vlc_regs(void *hal, HalRegDrvCtx_t *p_drv);
MPP_RET vdpu_set_ref_regs(void *hal, HalRegDrvCtx_t *p_drv);
MPP_RET vdpu_set_asic_regs(void *hal, HalRegDrvCtx_t *p_drv);


#ifdef  __cplusplus
}
#endif



//!<============================
#endif /* __HAL_H264D_VDPU_PKT_H__ */



