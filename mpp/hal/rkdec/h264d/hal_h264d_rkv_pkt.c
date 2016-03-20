
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

#define MODULE_TAG "hal_h264d_rkv_pkt"

#include <stdio.h>
#include <string.h>

#include "vpu.h"
#include "mpp_mem.h"
#include "mpp_buffer.h"
#include "hal_task.h"

#include "dxva_syntax.h"
#include "h264d_log.h"
#include "h264d_syntax.h"
#include "hal_h264d_fifo.h"
#include "hal_h264d_api.h"
#include "hal_h264d_global.h"
#include "hal_h264d_rkv_pkt.h"
#include "hal_h264d_rkv_reg.h"

//!< Header
#define     H264dREG_HEADER    0x48474552
#define     H264dPPS_HEADER    0x48535050
#define     H264dSCL_HEADER    0x534c4353
#define     H264dRPS_HEADER    0x48535052
#define     H264dCRC_HEADER    0x48435243
#define     H264dSTM_HEADER    0x4d525453
#define     H264dERR_HEADER    0x524f5245


#define  FPGA_TEST   0

const enum
{
    H264ScalingList4x4Length = 16,
    H264ScalingList8x8Length = 64,
} ScalingListLength;



static void rkv_write_sps_to_fifo(H264dHalCtx_t *p_hal, FifoCtx_t *pkt)
{
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

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
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}

static void rkv_write_pps_to_fifo(H264dHalCtx_t *p_hal, FifoCtx_t *pkt)
{
    RK_U32 Scaleing_list_address = 0;
    RK_U32 offset = RKV_CABAC_TAB_SIZE + RKV_SPSPPS_SIZE + RKV_RPS_SIZE;
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    fifo_write_bits(pkt, -1,                                                    8, "pps_pic_parameter_set_id");
    fifo_write_bits(pkt, -1,                                                    5, "pps_seq_parameter_set_id");
    fifo_write_bits(pkt, p_hal->pp->entropy_coding_mode_flag,                   1, "entropy_coding_mode_flag");
    fifo_write_bits(pkt, p_hal->pp->pic_order_present_flag,                     1, "bottom_field_pic_order_in_frame_present_flag");
    fifo_write_bits(pkt, p_hal->pp->num_ref_idx_l0_active_minus1,               5, "num_ref_idx_l0_default_active_minus1");
    fifo_write_bits(pkt, p_hal->pp->num_ref_idx_l1_active_minus1,               5, "num_ref_idx_l1_default_active_minus1");
    fifo_write_bits(pkt, p_hal->pp->weighted_pred_flag,                         1, "weighted_pred_flag");
    fifo_write_bits(pkt, p_hal->pp->weighted_bipred_idc,                        2, "weighted_bipred_idc");
    fifo_write_bits(pkt, p_hal->pp->pic_init_qp_minus26,                        7, "pic_init_qp_minus26");
    fifo_write_bits(pkt, p_hal->pp->pic_init_qs_minus26,                        6, "pic_init_qs_minus26");
    fifo_write_bits(pkt, p_hal->pp->chroma_qp_index_offset,                     5, "chroma_qp_index_offset");
    fifo_write_bits(pkt, p_hal->pp->deblocking_filter_control_present_flag,     1, "deblocking_filter_control_present_flag");
    fifo_write_bits(pkt, p_hal->pp->constrained_intra_pred_flag,                1, "constrained_intra_pred_flag");
    fifo_write_bits(pkt, p_hal->pp->redundant_pic_cnt_present_flag,             1, "redundant_pic_cnt_present_flag");
    fifo_write_bits(pkt, p_hal->pp->transform_8x8_mode_flag,                    1, "transform_8x8_mode_flag");
    fifo_write_bits(pkt, p_hal->pp->second_chroma_qp_index_offset,              5, "second_chroma_qp_index_offset");
    fifo_write_bits(pkt, p_hal->pp->scaleing_list_enable_flag,                  1, "scaleing_list_enable_flag");

    Scaleing_list_address = mpp_buffer_get_fd(p_hal->cabac_buf);
    //mpp_log("fd=%08x, offset=%d", Scaleing_list_address, offset);
    if (VPUClientGetIOMMUStatus() > 0) {
        Scaleing_list_address |= offset << 10;
    } else {
        Scaleing_list_address += offset;
    }
#if FPGA_TEST
    fifo_write_bits(pkt, 0, 32, "Scaleing_list_address");
#else
    fifo_write_bits(pkt, Scaleing_list_address, 32, "Scaleing_list_address");
#endif
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}


/*!
***********************************************************************
* \brief
*    reset fifo packet
***********************************************************************
*/
//extern "C"
void rkv_reset_fifo_packet(H264dRkvPkt_t *pkt)
{
    if (pkt) {
        fifo_packet_reset(&pkt->spspps);
        fifo_packet_reset(&pkt->rps);
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
void rkv_free_fifo_packet(H264dRkvPkt_t *pkt)
{
    if (pkt) {
        MPP_FREE(pkt->spspps.pbuf);
        MPP_FREE(pkt->rps.pbuf);
        MPP_FREE(pkt->scanlist.pbuf);
        MPP_FREE(pkt->reg.pbuf);
    }
}
/*!
***********************************************************************
* \brief
*    alloc fifo packet
***********************************************************************
*/
//extern "C"
MPP_RET rkv_alloc_fifo_packet(H264dLogCtx_t *logctx, H264dRkvPkt_t *pkts)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    LogCtx_t *log_driver = NULL;
    RK_U32 pps_size   = RKV_SPSPPS_SIZE + 64;
    RK_U32 rps_size   = RKV_RPS_SIZE + 64;
    RK_U32 sclst_size = RKV_SCALING_LIST_SIZE + 64;
    RK_U32 regs_size  = sizeof(H264dRkvRegs_t);

    FunctionIn(logctx->parr[RUN_HAL]);
    //!< initial packages header and malloc buffer
    FUN_CHECK(ret = fifo_packet_alloc(&pkts->spspps,   H264dPPS_HEADER, pps_size));
    FUN_CHECK(ret = fifo_packet_alloc(&pkts->rps,      H264dRPS_HEADER, rps_size));
    FUN_CHECK(ret = fifo_packet_alloc(&pkts->scanlist, H264dSCL_HEADER, sclst_size));
    FUN_CHECK(ret = fifo_packet_alloc(&pkts->reg,      H264dREG_HEADER, regs_size));

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
    } else {
        pkts->spspps.fp_data   = NULL;
        pkts->rps.fp_data      = NULL;
        pkts->scanlist.fp_data = NULL;
        pkts->reg.fp_data      = NULL;
    }
    FunctionOut(logctx->parr[RUN_HAL]);
    return ret = MPP_OK;
__FAILED:
    rkv_free_fifo_packet(pkts);

    return ret;
}

/*!
***********************************************************************
* \brief
*    prepare sps_sps packet
***********************************************************************
*/
//extern "C"
void rkv_prepare_spspps_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0;
    RK_S32 is_long_term = 0, voidx = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    fifo_packet_reset(pkt);
    LogInfo(pkt->logctx, "------------------ Frame SPS_PPS begin ------------------------");
    rkv_write_sps_to_fifo(p_hal, pkt);
    rkv_write_pps_to_fifo(p_hal, pkt);

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
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}
/*!
***********************************************************************
* \brief
*    prepare frame rps packet
***********************************************************************
*/
//extern "C"
void rkv_prepare_framerps_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    RK_U32 max_frame_num = 0;
    RK_U16 frame_num_wrap = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA_PicParams_H264_MVC  *pp = p_hal->pp;
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
    fifo_packet_reset(pkt);
    LogInfo(pkt->logctx, "------------------ Frame RPS begin ------------------------");
    max_frame_num = 1 << (pp->log2_max_frame_num_minus4 + 4);
    //FPRINT(g_debug_file0, "--------\n");
    for (i = 0; i < 16; i++) {
        if ((pp->NonExistingFrameFlags >> i) & 0x01) {
            frame_num_wrap = 0;
        } else {
            if (pp->RefFrameList[i].AssociatedFlag) {
                frame_num_wrap = pp->FrameNumList[i];
            } else {
                frame_num_wrap = (pp->FrameNumList[i] > pp->frame_num) ? (pp->FrameNumList[i] - max_frame_num) : pp->FrameNumList[i];
            }
        }
        //FPRINT(g_debug_file0, "i=%d, frame_num_wrap=%d \n", i, frame_num_wrap);
        fifo_write_bits(pkt, frame_num_wrap, 16, "frame_num_wrap");
    }
    for (i = 0; i < 16; i++) {
        fifo_write_bits(pkt, 0, 1, "NULL");
    }
    for (i = 0; i < 16; i++) {
        fifo_write_bits(pkt, pp->RefPicLayerIdList[i], 1, "voidx");
    }
    for (i = 0; i < 32; i++) {
        dpb_valid   = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx     = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx       = dpb_valid ? pp->RefPicLayerIdList[dpb_idx] : 0;
        fifo_write_bits(pkt, dpb_idx | (dpb_valid << 4), 5, "dpb_idx");
        fifo_write_bits(pkt, bottom_flag, 1, "bottom_flag");
        fifo_write_bits(pkt, voidx, 1, "voidx");
    }
    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            dpb_valid   = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx     = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = dpb_valid ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx       = dpb_valid ? pp->RefPicLayerIdList[dpb_idx] : 0;
            fifo_write_bits(pkt, dpb_idx | (dpb_valid << 4), 5, "dpb_idx");
            fifo_write_bits(pkt, bottom_flag, 1, "bottom_flag");
            fifo_write_bits(pkt, voidx, 1, "voidx");
        }
    }
    fifo_align_bits(pkt, 128);
    fifo_flush_bits(pkt);
    fifo_fwrite_data(pkt); //!< "RPSH" header 32 bit
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}
/*!
***********************************************************************
* \brief
*    prepare scanlist packet
***********************************************************************
*/
//extern "C"
void rkv_prepare_scanlist_packet(void *hal, FifoCtx_t *pkt)
{
    RK_S32 i = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    FunctionIn(p_hal->logctx.parr[RUN_HAL]);
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
    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}

/*!
***********************************************************************
* \brief
*    generate register packet
***********************************************************************
*/
//extern "C"
void rkv_generate_regs(void *hal, HalTaskInfo *task, FifoCtx_t *pkt)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    H264dRkvRegs_t *p_regs = (H264dRkvRegs_t *)p_hal->regs;

    FunctionIn(p_hal->logctx.parr[RUN_HAL]);

    memset(p_regs, 0, sizeof(H264dRkvRegs_t));
    //!< set dec_mode && rlc_mode && rps_mode && slice_num
    {
        p_regs->swreg2_sysctrl.sw_dec_mode = 1;  //!< h264
        if (p_regs->swreg2_sysctrl.sw_rlc_mode == 1) {
            p_regs->swreg5_stream_rlc_len.sw_stream_len = 0;
        } else {
            p_regs->swreg5_stream_rlc_len.sw_stream_len = (RK_U32)mpp_packet_get_length(task->dec.input_packet);
        }
        if (p_regs->swreg2_sysctrl.sw_h264_rps_mode) { // rps_mode == 1
            p_regs->swreg43_rps_base.sw_rps_base += 0x8;
        }
        p_regs->swreg3_picpar.sw_slice_num_lowbits = 0x7ff; // p_Vid->iNumOfSlicesDecoded & 0x7ff
        p_regs->swreg3_picpar.sw_slice_num_highbit = 1;     // (p_Vid->iNumOfSlicesDecoded >> 11) & 1
    }
    //!< caculate the yuv_frame_size
    {
        MppFrame cur_frame = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 yuv_virstride = 0;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &cur_frame);
        hor_virstride = mpp_frame_get_hor_stride(cur_frame);
        ver_virstride = mpp_frame_get_ver_stride(cur_frame);
        y_virstride = hor_virstride * ver_virstride;

        if (pp->chroma_format_idc == 0) { //!< Y400
            yuv_virstride = y_virstride;
        } else if (pp->chroma_format_idc == 1) { //!< Y420
            yuv_virstride += y_virstride + y_virstride / 2;
        } else if (pp->chroma_format_idc == 2) { //!< Y422
            yuv_virstride += 2 * y_virstride;
        }
        p_regs->swreg3_picpar.sw_y_hor_virstride = hor_virstride / 16;
        p_regs->swreg3_picpar.sw_uv_hor_virstride = hor_virstride / 16;
        p_regs->swreg8_y_virstride.sw_y_virstride = y_virstride / 16;
        p_regs->swreg9_yuv_virstride.sw_yuv_virstride = yuv_virstride / 16;
    }
    //!< caculate mv_size
    {
        RK_U32 mb_width = 0, mb_height = 0, mv_size = 0;
        mb_width = pp->wFrameWidthInMbsMinus1 + 1;
        mb_height = (2 - pp->frame_mbs_only_flag) * (pp->wFrameHeightInMbsMinus1 + 1);
        mv_size = mb_width * mb_height * 8; // 64bit per 4x4
        p_regs->compare_len = (p_regs->swreg9_yuv_virstride.sw_yuv_virstride + mv_size) * 2;
    }
    //!< set current
    {
        MppBuffer frame_buf = NULL;
        p_regs->swreg40_cur_poc.sw_cur_poc = pp->CurrFieldOrderCnt[0];
        p_regs->swreg74_h264_cur_poc1.sw_h264_cur_poc1 = pp->CurrFieldOrderCnt[1];
        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &frame_buf); //!< current out phy addr
#if FPGA_TEST
        p_regs->swreg7_decout_base.sw_decout_base = pp->CurrPic.Index7Bits + 1;
#else
        p_regs->swreg7_decout_base.sw_decout_base = mpp_buffer_get_fd(frame_buf);
#endif
    }
    //!< set reference
    {
        RK_S32 i = 0;
        RK_S32 ref_index  = -1;
        RK_S32 near_index = -1;
        MppBuffer frame_buf = NULL;

        for (i = 0; i < 15; i++) {
            p_regs->swreg25_39_refer0_14_poc[i] = (i & 1) ? pp->FieldOrderCntList[i / 2][1] : pp->FieldOrderCntList[i / 2][0];
            p_regs->swreg49_63_refer15_29_poc[i] = (i & 1) ? pp->FieldOrderCntList[(i + 15) / 2][0] : pp->FieldOrderCntList[(i + 15) / 2][1];
            p_regs->swreg10_24_refer0_14_base[i].sw_ref_field = (pp->RefPicFiledFlags >> i) & 0x01;
            p_regs->swreg10_24_refer0_14_base[i].sw_ref_topfield_used = (pp->UsedForReferenceFlags >> (2 * i + 0)) & 0x01;
            p_regs->swreg10_24_refer0_14_base[i].sw_ref_botfield_used = (pp->UsedForReferenceFlags >> (2 * i + 1)) & 0x01;
            p_regs->swreg10_24_refer0_14_base[i].sw_ref_colmv_use_flag = (pp->RefPicColmvUsedFlags >> i) & 0x01;
#if FPGA_TEST
            p_regs->swreg10_24_refer0_14_base[i].sw_refer_base = pp->RefFrameList[i].Index7Bits + 1;
#else
            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                ref_index  = pp->RefFrameList[i].Index7Bits;
                near_index = pp->RefFrameList[i].Index7Bits;
            } else {
                ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
            }
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &frame_buf); //!< reference phy addr
            p_regs->swreg10_24_refer0_14_base[i].sw_refer_base = mpp_buffer_get_fd(frame_buf);
#endif
        }
        p_regs->swreg72_refer30_poc = pp->FieldOrderCntList[15][0];
        p_regs->swreg73_refer31_poc = pp->FieldOrderCntList[15][1];
        p_regs->swreg48_refer15_base.sw_ref_field = (pp->RefPicFiledFlags >> 15) & 0x01;
        p_regs->swreg48_refer15_base.sw_ref_topfield_used = (pp->UsedForReferenceFlags >> 30) & 0x01;
        p_regs->swreg48_refer15_base.sw_ref_botfield_used = (pp->UsedForReferenceFlags >> 31) & 0x01;
        p_regs->swreg48_refer15_base.sw_ref_colmv_use_flag = (pp->RefPicColmvUsedFlags >> 15) & 0x01;
#if FPGA_TEST
        p_regs->swreg48_refer15_base.sw_refer_base = pp->RefFrameList[15].Index7Bits + 1;
#else
        if (pp->RefFrameList[15].bPicEntry != 0xff) {
            ref_index = pp->RefFrameList[15].Index7Bits;
        } else {
            ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
        }
        mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &frame_buf); //!< reference phy addr
        p_regs->swreg48_refer15_base.sw_refer_base = mpp_buffer_get_fd(frame_buf);
#endif
    }

#if FPGA_TEST
    p_regs->swreg4_strm_rlc_base.sw_strm_rlc_base = 0;
    p_regs->swreg6_cabactbl_prob_base.sw_cabactbl_base = 0;
    p_regs->swreg41_rlcwrite_base.sw_rlcwrite_base = 0;
#else
    {
        MppBuffer bitstream_buf = NULL;
        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &bitstream_buf);
        p_regs->swreg4_strm_rlc_base.sw_strm_rlc_base = mpp_buffer_get_fd(bitstream_buf);
        p_regs->swreg6_cabactbl_prob_base.sw_cabactbl_base = mpp_buffer_get_fd(p_hal->cabac_buf);
        p_regs->swreg41_rlcwrite_base.sw_rlcwrite_base = p_regs->swreg4_strm_rlc_base.sw_strm_rlc_base;
    }
#endif
    fifo_packet_reset(pkt);
    fifo_write_bytes(pkt, (void *)p_hal->regs, 80 * sizeof(RK_U32));
    {
        RK_U32 i = 0;
        RK_U32 *ptr = (RK_U32*)p_hal->regs;
        LogInfo(pkt->logctx, "------------------ Frame REG begin ------------------------");
        for (i = 0; i < 80; i++) {
            LogInfo(pkt->logctx, "             reg[%3d] = %08x \n", i, ptr[i]);
        }
    }
    fifo_align_bits(pkt, 64);
    fifo_flush_bits(pkt);
    fifo_fwrite_data(pkt); //!< "REGH" header 32 bit

    FunctionOut(p_hal->logctx.parr[RUN_HAL]);
}

/*!
***********************************************************************
* \brief
*   for debug
***********************************************************************
*/
void rkv_fprint_fifo_data(FILE *fp, FifoCtx_t *pkt)
{
    RK_U32 i = 0, pkt_size = 0;
    RK_U8 *ptr = (RK_U8 *)pkt->pbuf;
    if (fp) {
        pkt_size = pkt->index * sizeof(RK_U64);
        fprintf(fp, "------ Header=%08x, size=%d ------ \n", pkt->header, pkt_size);
        for (i = 0; i < pkt_size;) {
            fprintf(fp, "0x%02x, ", ptr[i]);
            i++;
            if ((i % 16) == 0) {
                fprintf(fp, "\n");
            }
        }
        fprintf(fp, "\n\n");
    }
}
