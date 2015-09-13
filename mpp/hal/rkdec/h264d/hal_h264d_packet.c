
/*
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

#include <stdio.h>
#include <string.h>

#include "mpp_mem.h"
#include "hal_task.h"

#include "dxva_syntax.h"
#include "h264d_log.h"
#include "h264d_syntax.h"
#include "hal_h264d_fifo.h"
#include "hal_h264d_api.h"
#include "hal_h264d_packet.h"
#include "hal_h264d_global.h"

#define  MODULE_TAG  "hal_h264d_packet"

//!< Header
#define     H264dREG_HEADER    0x48474552
#define     H264dPPS_HEADER    0x48535050
#define     H264dSCL_HEADER    0x534c4353
#define     H264dRPS_HEADER    0x48535052
#define     H264dCRC_HEADER    0x48435243
#define     H264dSTM_HEADER    0x4d525453
#define     H264dERR_HEADER    0x524f5245


enum {
    H264ScalingList4x4Length = 16,
    H264ScalingList8x8Length = 64,
} ScalingListLength;


static void write_sps_to_fifo(H264dHalCtx_t *p_hal, FifoCtx_t *pkt)
{
    fifo_write_bits(pkt,  -1,                                           4, "seq_parameter_set_id");   //!< not used in hard
    fifo_write_bits(pkt,  -1,                                           8, "profile_idc");            //!< not used in hard
    fifo_write_bits(pkt,  -1,                                           1, "constraint_set3_flag");   //!< not used in hard
    fifo_write_bits(pkt,  p_hal->pp->chroma_format_idc,                 2, "chroma_format_idc");
    fifo_write_bits(pkt, (p_hal->pp->bit_depth_luma_minus8 + 8),        3, "bit_depth_luma");
    fifo_write_bits(pkt, (p_hal->pp->bit_depth_chroma_minus8 + 8),      3, "bit_depth_chroma");
    fifo_write_bits(pkt,   0,                                           1, "qpprime_y_zero_transform_bypass_flag"); //!< not supported in hard
    fifo_write_bits(pkt,  p_hal->pp->log2_max_frame_num_minus4,         4, "log2_max_frame_num_minus4");
    fifo_write_bits(pkt,  p_hal->pp->num_ref_frames,                    5, "max_num_ref_frames");
    fifo_write_bits(pkt,  p_hal->pp->pic_order_cnt_type,                2, "pic_order_cnt_type");
    fifo_write_bits(pkt,  p_hal->pp->log2_max_pic_order_cnt_lsb_minus4, 4, "log2_max_pic_order_cnt_lsb_minus4");
    fifo_write_bits(pkt,  p_hal->pp->delta_pic_order_always_zero_flag,  1, "delta_pic_order_always_zero_flag");
    fifo_write_bits(pkt, (p_hal->pp->wFrameWidthInMbsMinus1 + 1),       9, "pic_width_in_mbs");
    fifo_write_bits(pkt, (p_hal->pp->wFrameHeightInMbsMinus1 + 1),      9, "pic_height_in_mbs");
    fifo_write_bits(pkt,  p_hal->pp->frame_mbs_only_flag,               1, "frame_mbs_only_flag");
    fifo_write_bits(pkt,  p_hal->pp->MbaffFrameFlag,                    1, "mb_adaptive_frame_field_flag");
    fifo_write_bits(pkt,  p_hal->pp->direct_8x8_inference_flag,         1, "direct_8x8_inference_flag");

    fifo_write_bits(pkt,  1,                                   1, "mvc_extension_enable");
    fifo_write_bits(pkt, (p_hal->pp->num_views_minus1 + 1),    2, "num_views");
    fifo_write_bits(pkt,  p_hal->pp->view_id[0],              10, "view_id[2]");
    fifo_write_bits(pkt,  p_hal->pp->view_id[1],              10, "view_id[2]");
    fifo_write_bits(pkt,  p_hal->pp->num_anchor_refs_l0[0],    1, "num_anchor_refs_l0");
    if (p_hal->pp->num_anchor_refs_l0[0]) {
        fifo_write_bits(pkt, p_hal->pp->anchor_ref_l0[0][0],  10, "anchor_ref_l0");
    } else {
        fifo_write_bits(pkt, 0, 10, "anchor_ref_l0");
    }
    fifo_write_bits(pkt, p_hal->pp->num_anchor_refs_l1[0],     1, "num_anchor_refs_l1");
    if (p_hal->pp->num_anchor_refs_l1[0]) {
        fifo_write_bits(pkt, p_hal->pp->anchor_ref_l1[0][0],  10, "anchor_ref_l1");
    } else {
        fifo_write_bits(pkt, 0, 10, "anchor_ref_l1");
    }
    fifo_write_bits(pkt, p_hal->pp->num_non_anchor_refs_l0[0],    1, "num_non_anchor_refs_l0");
    if (p_hal->pp->num_non_anchor_refs_l0[0]) {
        fifo_write_bits(pkt, p_hal->pp->non_anchor_ref_l0[0][0], 10, "non_anchor_ref_l0");
    } else {
        fifo_write_bits(pkt, 0, 10, "non_anchor_ref_l0");
    }
    fifo_write_bits(pkt, p_hal->pp->num_non_anchor_refs_l1[0],    1, "num_non_anchor_refs_l1");
    if (p_hal->pp->num_non_anchor_refs_l1[0]) {
        fifo_write_bits(pkt, p_hal->pp->non_anchor_ref_l1[0][0], 10, "non_anchor_ref_l1");
    } else {
        fifo_write_bits(pkt, 0, 10, "non_anchor_ref_l1");
    }
    fifo_align_bits(pkt, 32);
}

static void write_pps_to_fifo(H264dHalCtx_t *p_hal, FifoCtx_t *pkt)
{
    fifo_write_bits(pkt, -1,                                                8, "pps_pic_parameter_set_id");
    fifo_write_bits(pkt, -1,                                                5, "pps_seq_parameter_set_id");
    fifo_write_bits(pkt, p_hal->pp->entropy_coding_mode_flag,               1, "entropy_coding_mode_flag");
    fifo_write_bits(pkt, p_hal->pp->pic_order_present_flag,                 1, "bottom_field_pic_order_in_frame_present_flag");
    fifo_write_bits(pkt, p_hal->pp->num_ref_idx_l0_active_minus1,           5, "num_ref_idx_l0_default_active_minus1");
    fifo_write_bits(pkt, p_hal->pp->num_ref_idx_l1_active_minus1,           5, "num_ref_idx_l1_default_active_minus1");
    fifo_write_bits(pkt, p_hal->pp->weighted_pred_flag,                     1, "weighted_pred_flag");
    fifo_write_bits(pkt, p_hal->pp->weighted_bipred_idc,                    2, "weighted_bipred_idc");
    fifo_write_bits(pkt, p_hal->pp->pic_init_qp_minus26,                    7, "pic_init_qp_minus26");
    fifo_write_bits(pkt, p_hal->pp->pic_init_qs_minus26,                    6, "pic_init_qs_minus26");
    fifo_write_bits(pkt, p_hal->pp->chroma_qp_index_offset,                 5, "chroma_qp_index_offset");
    fifo_write_bits(pkt, p_hal->pp->deblocking_filter_control_present_flag, 1, "deblocking_filter_control_present_flag");
    fifo_write_bits(pkt, p_hal->pp->constrained_intra_pred_flag,            1, "constrained_intra_pred_flag");
    fifo_write_bits(pkt, p_hal->pp->redundant_pic_cnt_present_flag,         1, "redundant_pic_cnt_present_flag");
    fifo_write_bits(pkt, p_hal->pp->transform_8x8_mode_flag,                1, "transform_8x8_mode_flag");
    fifo_write_bits(pkt, p_hal->pp->second_chroma_qp_index_offset,          5, "second_chroma_qp_index_offset");
    fifo_write_bits(pkt, p_hal->pp->scaleing_list_enable_flag,              1, "scaleing_list_enable_flag");
    fifo_write_bits(pkt, 0/*(RK_U32)p_hal->pkts.scanlist.pbuf*/,               32, "Scaleing_list_address");

}


/*!
***********************************************************************
* \brief
*    reset fifo packet
***********************************************************************
*/
//extern "C"
void reset_fifo_packet(H264_FifoPkt_t *pkt)
{
    if (pkt) {
        fifo_packet_reset(&pkt->spspps);
        fifo_packet_reset(&pkt->rps);
        fifo_packet_reset(&pkt->strm);
        fifo_packet_reset(&pkt->scanlist);
        fifo_packet_reset(&pkt->reg);
    }
}

/*!
***********************************************************************
* \brief
*    free fifo pakcket
***********************************************************************
*/
//extern "C"
void free_fifo_packet(H264_FifoPkt_t *pkt)
{
    if (pkt) {
        mpp_free(pkt->spspps.pbuf);
        mpp_free(pkt->rps.pbuf);
        mpp_free(pkt->scanlist.pbuf);
        mpp_free(pkt->reg.pbuf);
    }
}
/*!
***********************************************************************
* \brief
*    alloc fifo packet
***********************************************************************
*/
//extern "C"
MPP_RET alloc_fifo_packet(H264dLogCtx_t *logctx, H264_FifoPkt_t *pkts)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    LogCtx_t *log_driver = NULL;
    RK_U32 pps_size   = 256 * 32 + 16;
    RK_U32 rps_size   = 128 + 16;
    RK_U32 strm_size  = 0;
    RK_U32 sclst_size = 256;
    RK_U32 regs_size  = 512;

    //!< initial packages header and malloc buffer
    FUN_CHECK(ret = fifo_packet_init(&pkts->spspps,   H264dPPS_HEADER, pps_size));
    FUN_CHECK(ret = fifo_packet_init(&pkts->rps,      H264dRPS_HEADER, rps_size));
    FUN_CHECK(ret = fifo_packet_init(&pkts->strm,     H264dSTM_HEADER, strm_size));
    FUN_CHECK(ret = fifo_packet_init(&pkts->scanlist, H264dSCL_HEADER, sclst_size));
    FUN_CHECK(ret = fifo_packet_init(&pkts->reg,      H264dREG_HEADER, regs_size));

    //!< set logctx
    pkts->spspps.logctx   = logctx->parr[LOG_WRITE_SPSPPS];
    pkts->rps.logctx      = logctx->parr[LOG_WRITE_RPS];
    pkts->scanlist.logctx = logctx->parr[LOG_WRITE_SCANLIST];
    pkts->reg.logctx      = logctx->parr[LOG_WRITE_REG];

    //!< set bitstream file output
    log_driver = logctx->parr[LOG_FPGA];
    if (log_driver && log_driver->flag->write_en) {
        pkts->spspps.fp_data   = log_driver->fp;
        pkts->rps.fp_data      = log_driver->fp;
        pkts->scanlist.fp_data = log_driver->fp;
        pkts->reg.fp_data      = log_driver->fp;
        pkts->strm.fp_data     = log_driver->fp;
    } else {
        pkts->spspps.fp_data   = NULL;
        pkts->rps.fp_data      = NULL;
        pkts->scanlist.fp_data = NULL;
        pkts->reg.fp_data      = NULL;
        pkts->strm.fp_data     = NULL;
    }
    return ret = MPP_OK;
__FAILED:
    free_fifo_packet(pkts);

    return ret;
}

/*!
***********************************************************************
* \brief
*    expalin decoder buffer dest
***********************************************************************
*/
//extern "C"
void explain_input_buffer(void *hal, HalDecTask *task)
{
    RK_U32 i = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA2_DecodeBufferDesc *pdes = (DXVA2_DecodeBufferDesc *)task->syntax.data;
    for (i = 0; i < task->syntax.number; i++) {
        switch (pdes[i].CompressedBufferType) {
        case DXVA2_PictureParametersBufferType:
            p_hal->pp = (DXVA_PicParams_H264_MVC *)pdes[i].pvPVPState;
            break;
        case DXVA2_InverseQuantizationMatrixBufferType:
            p_hal->qm = (DXVA_Qmatrix_H264 *)pdes[i].pvPVPState;
            break;
        case DXVA2_SliceControlBufferType:
            p_hal->slice_num = pdes[i].DataSize / sizeof(DXVA_Slice_H264_Long);
            p_hal->slice_long = (DXVA_Slice_H264_Long *)pdes[i].pvPVPState;
            break;
        case DXVA2_BitStreamDateBufferType:
            p_hal->bitstream = (RK_U8 *)pdes[i].pvPVPState;
            p_hal->strm_len = pdes[i].DataSize;
            break;
        default:
            break;
        }
    }
}
/*!
***********************************************************************
* \brief
*    prepare sps_sps packet
***********************************************************************
*/
//extern "C"
void prepare_spspps_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0;
    RK_S32 is_long_term = 0, voidx = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    fifo_packet_reset(pkt);
    LogInfo(pkt->logctx, "------------------ Frame SPS_PPS begin ------------------------");
    write_sps_to_fifo(p_hal, pkt);
    write_pps_to_fifo(p_hal, pkt);

    for (i = 0; i < 16; i++) {
        is_long_term = (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) ? p_hal->pp->RefFrameList[i].AssociatedFlag : 0;
        fifo_write_bits(pkt, is_long_term, 1, "is_long_term");
    }
    for (i = 0; i < 16; i++) {
        voidx = (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) ? p_hal->pp->RefPicLayerIdList[i] : 0;
        fifo_write_bits(pkt, voidx, 1, "voidx");
    }
    fifo_align_bits(pkt, 64);
    fifo_fwrite_data(pkt);  //!< "PPSH" header 32 bit
}
/*!
***********************************************************************
* \brief
*    prepare frame rps packet
***********************************************************************
*/
//extern "C"
void prepare_framerps_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    fifo_packet_reset(pkt);
    LogInfo(pkt->logctx, "------------------ Frame RPS begin ------------------------");
    for (i = 0; i < 16; i++) {
        fifo_write_bits(pkt, p_hal->pp->FrameNumList[i], 16, "frame_num_wrap");
    }
    for (i = 0; i < 16; i++) {
        fifo_write_bits(pkt, 0, 1, "NULL");
    }
    for (i = 0; i < 16; i++) {
        fifo_write_bits(pkt, p_hal->pp->RefPicLayerIdList[i], 1, "voidx");
    }
    for (i = 0; i < 32; i++) {
        dpb_valid   = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx     = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx       = dpb_valid ? p_hal->pp->RefPicLayerIdList[dpb_idx] : 0;
        fifo_write_bits(pkt, dpb_idx | (dpb_valid << 4), 5, "dpb_idx");
        fifo_write_bits(pkt, bottom_flag, 1, "bottom_flag");
        fifo_write_bits(pkt, voidx, 1, "voidx");
    }
    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            dpb_valid   = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx     = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx       = dpb_valid ? p_hal->pp->RefPicLayerIdList[dpb_idx] : 0;
            fifo_write_bits(pkt, dpb_idx | (dpb_valid << 4), 5, "dpb_idx");
            fifo_write_bits(pkt, bottom_flag, 1, "bottom_flag");
            fifo_write_bits(pkt, voidx, 1, "voidx");
        }
    }
    fifo_align_bits(pkt, 128);
    fifo_flush_bits(pkt);
    fifo_fwrite_data(pkt); //!< "RPSH" header 32 bit
}
/*!
***********************************************************************
* \brief
*    prepare scanlist packet
***********************************************************************
*/
//extern "C"
void prepare_scanlist_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    if (p_hal->pp->scaleing_list_enable_flag) {
        fifo_packet_reset(pkt);
        LogInfo(pkt->logctx, "------------------ Scanlist begin ------------------------");
        for (i = 0; i < 6; ++i) { //!< 4x4, 6 lists
            fifo_write_bytes(pkt, p_hal->qm->bScalingLists4x4[i], H264ScalingList4x4Length);
        }
        for (i = 0; i < 2; ++i) {
            fifo_write_bytes(pkt, p_hal->qm->bScalingLists8x8[i], H264ScalingList8x8Length);
        }
        fifo_fwrite_data(pkt); //!< "SCLS" header 32 bit
    }
}
/*!
***********************************************************************
* \brief
*    prepare stream packet
***********************************************************************
*/
//extern "C"
void prepare_stream_packet(void *hal, FifoCtx_t *pkt)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    fifo_packet_reset(pkt);
    pkt->pbuf  = (RK_U64 *)p_hal->bitstream;
    pkt->index = p_hal->strm_len / 8;
    fifo_fwrite_data(pkt); // "STRM" header 32 bit
}
/*!
***********************************************************************
* \brief
*    generate register packet
***********************************************************************
*/
//extern "C"
void generate_regs(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0;
    RK_U8 bit_depth[3] = { 0 };
    RK_U32 sw_y_hor_virstride = 0;
    RK_U32 sw_uv_hor_virstride = 0;
    RK_U32 sw_y_virstride = 0;
    RK_U32 yuv_virstride = 0;
    RK_U32 pic_h[3] = { 0 }, pic_w[3] = { 0 };
    RK_U32 mb_width = 0, mb_height = 0, mv_size = 0;

    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    H264_REGS_t *p_regs = p_hal->regs;
    memset(p_regs, 0, sizeof(H264_REGS_t));

    if (p_regs->swreg2_sysctrl.sw_rlc_mode == 1) {
        p_regs->swreg5_stream_rlc_len.sw_stream_len = 0;
    } else {
        p_regs->swreg5_stream_rlc_len.sw_stream_len = p_hal->strm_len;
    }
    //!< caculate the yuv_frame_size and mv_size
    mb_width  = p_hal->pp->wFrameWidthInMbsMinus1 + 1;
    mb_height = (2 - p_hal->pp->frame_mbs_only_flag) * (p_hal->pp->wFrameHeightInMbsMinus1 + 1);
    mv_size = mb_width * mb_height * 8; // 64bit per 4x4
    bit_depth[0] = p_hal->pp->bit_depth_luma_minus8 + 8;
    if (p_hal->pp->chroma_format_idc) { // Y420 Y422
        bit_depth[1] = p_hal->pp->bit_depth_chroma_minus8 + 8;
    } else { //!< Y400
        bit_depth[1] = 0;
    }
    pic_h[0] = mb_height * 16;
    if (p_hal->pp->chroma_format_idc == 2) { // Y422
        pic_h[1] = mb_height * 16;
    } else { //!< Y420
        pic_h[1] = mb_height * 8;
    }
    pic_w[0] = mb_width * 16;
    pic_w[1] = mb_width * 16;
    sw_y_hor_virstride  = ((pic_w[0] * bit_depth[0] + 127) & (~127)) / 128;
    sw_uv_hor_virstride = ((pic_w[1] * bit_depth[1] + 127) & (~127)) / 128;

    if (sw_y_hor_virstride > p_regs->swreg3_picpar.sw_y_hor_virstride) {
        p_regs->swreg3_picpar.sw_y_hor_virstride = sw_y_hor_virstride;
    }
    if (sw_uv_hor_virstride > p_regs->swreg3_picpar.sw_uv_hor_virstride) {
        p_regs->swreg3_picpar.sw_uv_hor_virstride = sw_uv_hor_virstride;
    }
    sw_y_virstride = pic_h[0] * p_regs->swreg3_picpar.sw_y_hor_virstride;
    if (sw_y_virstride > p_regs->swreg8_y_virstride.sw_y_virstride) {
        p_regs->swreg8_y_virstride.sw_y_virstride = sw_y_virstride;
    }
    if (p_hal->pp->chroma_format_idc == 0) { // YUV400
        yuv_virstride = p_regs->swreg8_y_virstride.sw_y_virstride;
    } else {
        yuv_virstride = pic_h[0] * p_regs->swreg3_picpar.sw_y_hor_virstride +
                        pic_h[1] * p_regs->swreg3_picpar.sw_uv_hor_virstride;
    }
    if (yuv_virstride > p_regs->swreg9_yuv_virstride.sw_yuv_virstride) {
        p_regs->swreg9_yuv_virstride.sw_yuv_virstride = yuv_virstride;
    }
    p_regs->compare_len = (p_regs->swreg9_yuv_virstride.sw_yuv_virstride + mv_size) * 2;
    if (p_regs->swreg2_sysctrl.sw_h264_rps_mode) { // rps_mode == 1
        p_regs->swreg43_rps_base.sw_rps_base += 0x8;
    }
    p_regs->swreg3_picpar.sw_slice_num_lowbits = 0x7ff; // p_Vid->iNumOfSlicesDecoded & 0x7ff
    p_regs->swreg3_picpar.sw_slice_num_highbit = 1;     // (p_Vid->iNumOfSlicesDecoded >> 11) & 1
    //!< set current
    p_regs->swreg40_cur_poc.sw_cur_poc = p_hal->pp->CurrFieldOrderCnt[0];
    p_regs->swreg74_h264_cur_poc1.sw_h264_cur_poc1 = p_hal->pp->CurrFieldOrderCnt[1];
    p_regs->swreg7_decout_base.sw_decout_base = p_hal->pp->CurrPic.Index7Bits;
    //!< set reference
    for (i = 0; i < 15; i++) {
        p_regs->swreg25_39_refer0_14_poc[i] = (i & 1) ? p_hal->pp->FieldOrderCntList[i / 2][1] : p_hal->pp->FieldOrderCntList[i / 2][0];
        p_regs->swreg49_63_refer15_29_poc[i] = (i & 1) ? p_hal->pp->FieldOrderCntList[(i + 15) / 2][0] : p_hal->pp->FieldOrderCntList[(i + 15) / 2][1];
        p_regs->swreg10_24_refer0_14_base[i].sw_ref_field = (p_hal->pp->RefPicFiledFlags >> i) & 0x01;
        p_regs->swreg10_24_refer0_14_base[i].sw_ref_topfield_used = (p_hal->pp->UsedForReferenceFlags >> (2 * i + 0)) & 0x01;
        p_regs->swreg10_24_refer0_14_base[i].sw_ref_botfield_used = (p_hal->pp->UsedForReferenceFlags >> (2 * i + 1)) & 0x01;
        p_regs->swreg10_24_refer0_14_base[i].sw_ref_colmv_use_flag = (p_hal->pp->RefPicColmvUsedFlags >> i) & 0x01;
        if (p_hal->pp->RefFrameList[i].bPicEntry != 0xff) {
            p_regs->swreg10_24_refer0_14_base[i].sw_refer_base = p_hal->pp->RefFrameList[i].Index7Bits;
        }
    }
    p_regs->swreg72_refer30_poc = p_hal->pp->FieldOrderCntList[i][0];
    p_regs->swreg73_refer31_poc = p_hal->pp->FieldOrderCntList[i][1];
    p_regs->swreg48_refer15_base.sw_ref_field = (p_hal->pp->RefPicFiledFlags >> 15) & 0x01;
    p_regs->swreg48_refer15_base.sw_ref_topfield_used = (p_hal->pp->UsedForReferenceFlags >> 30) & 0x01;
    p_regs->swreg48_refer15_base.sw_ref_botfield_used = (p_hal->pp->UsedForReferenceFlags >> 31) & 0x01;
    p_regs->swreg48_refer15_base.sw_ref_colmv_use_flag = (p_hal->pp->RefPicColmvUsedFlags >> 15) & 0x01;
    if (p_hal->pp->RefFrameList[15].bPicEntry != 0xff) {
        p_regs->swreg48_refer15_base.sw_refer_base = p_hal->pp->RefFrameList[15].Index7Bits;
    }

    fifo_packet_reset(pkt);
    fifo_write_bytes(pkt, (void *)p_hal->regs,       sizeof(H264_REGS_t));
    fifo_write_bytes(pkt, (void *)p_hal->mmu_regs,   sizeof(H264_MMU_t));
    fifo_write_bytes(pkt, (void *)p_hal->cache_regs, sizeof(H264_CACHE_t));
    fifo_align_bits(pkt, 64);
    fifo_flush_bits(pkt);
    fifo_fwrite_data(pkt); //!< "REGH" header 32 bit
}

