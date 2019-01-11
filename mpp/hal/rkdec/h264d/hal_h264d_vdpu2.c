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

#define MODULE_TAG "hal_h264d_vdpu_reg"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_device.h"
#include "mpp_common.h"

#include "hal_h264d_global.h"
#include "hal_h264d_api.h"
#include "hal_h264d_common.h"
#include "hal_h264d_vdpu2.h"
#include "hal_h264d_vdpu2_reg.h"

static MPP_RET set_device_regs(H264dHalCtx_t *p_hal, H264dVdpuRegs_t *p_reg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    p_reg->sw53.dec_fmt_sel = 0;   //!< set H264 mode
    p_reg->sw54.dec_out_endian = 1;  //!< little endian
    p_reg->sw54.dec_in_endian = 0;  //!< big endian
    p_reg->sw54.dec_strendian_e = 1; //!< little endian
    p_reg->sw50.dec_tiled_msb  = 0; //!< 0: raster scan  1: tiled
    p_reg->sw56.dec_max_burlen = 16;  //!< (0, 4, 8, 16) choice one
    p_reg->sw50.dec_ascmd0_dis = 0;   //!< disable
    p_reg->sw50.adv_pref_dis = 0; //!< disable
    p_reg->sw52.adv_pref_thrd = 8;
    p_reg->sw50.adtion_latency = 0; //!< compensation for bus latency; values up to 63
    p_reg->sw56.dec_data_discd_en = 0;
    p_reg->sw54.dec_out_wordsp = 1;//!< little endian
    p_reg->sw54.dec_in_wordsp = 1;//!< little endian
    p_reg->sw54.dec_strm_wordsp = 1;//!< little endian
    p_reg->sw57.timeout_sts_en = 1;
    p_reg->sw57.dec_clkgate_en = 1;
    p_reg->sw55.dec_irq_dis = 0;
    //!< set AXI RW IDs
    p_reg->sw56.dec_axi_id_rd = (0xFF & 0xFFU);  //!< 0-255
    p_reg->sw56.dec_axi_id_wr = (0x0 & 0xFFU);  //!< 0-255
    ///!< Set prediction filter taps
    {
        RK_U32 val = 0;
        p_reg->sw59.pflt_set0_tap0 = 1;
        val = (RK_U32)(-5);
        p_reg->sw59.pflt_set0_tap1 = val;
        p_reg->sw59.pflt_set0_tap2 = 20;
    }
    p_reg->sw50.adtion_latency = 0;
    //!< clock_gating  0:clock always on, 1: clock gating module control the key(turn off when decoder free)
    p_reg->sw57.dec_clkgate_en = 1;
    p_reg->sw50.dec_tiled_msb = 0; //!< 0: raster scan  1: tiled
    //!< bus_burst_length = 16, bus burst
    p_reg->sw56.dec_max_burlen = 16;
    p_reg->sw56.dec_data_discd_en = 0;
    (void)p_hal;

    return ret = MPP_OK;
}

static MPP_RET set_refer_pic_idx(H264dVdpuRegs_t *p_regs, RK_U32 i, RK_U16 val)
{
    switch (i) {
    case 0:
        p_regs->sw76.num_ref_idx0 = val;
        break;
    case 1:
        p_regs->sw76.num_ref_idx1 = val;
        break;
    case 2:
        p_regs->sw77.num_ref_idx2 = val;
        break;
    case 3:
        p_regs->sw77.num_ref_idx3 = val;
        break;
    case 4:
        p_regs->sw78.num_ref_idx4 = val;
        break;
    case 5:
        p_regs->sw78.num_ref_idx5 = val;
        break;
    case 6:
        p_regs->sw79.num_ref_idx6 = val;
        break;
    case 7:
        p_regs->sw79.num_ref_idx7 = val;
        break;
    case 8:
        p_regs->sw80.num_ref_idx8 = val;
        break;
    case 9:
        p_regs->sw80.num_ref_idx9 = val;
        break;
    case 10:
        p_regs->sw81.num_ref_idx10 = val;
        break;
    case 11:
        p_regs->sw81.num_ref_idx11 = val;
        break;
    case 12:
        p_regs->sw82.num_ref_idx12 = val;
        break;
    case 13:
        p_regs->sw82.num_ref_idx13 = val;
        break;
    case 14:
        p_regs->sw83.num_ref_idx14 = val;
        break;
    case 15:
        p_regs->sw83.num_ref_idx15 = val;
        break;
    default:
        break;
    }

    return MPP_OK;
}

static MPP_RET set_refer_pic_list_p(H264dVdpuRegs_t *p_regs, RK_U32 i,
                                    RK_U16 val)
{
    switch (i) {
    case 0:
        p_regs->sw106.init_reflist_pf0 = val;
        break;
    case 1:
        p_regs->sw106.init_reflist_pf1 = val;
        break;
    case 2:
        p_regs->sw106.init_reflist_pf2 = val;
        break;
    case 3:
        p_regs->sw106.init_reflist_pf3 = val;
        break;
    case 4:
        p_regs->sw74.init_reflist_pf4 = val;
        break;
    case 5:
        p_regs->sw74.init_reflist_pf5 = val;
        break;
    case 6:
        p_regs->sw74.init_reflist_pf6 = val;
        break;
    case 7:
        p_regs->sw74.init_reflist_pf7 = val;
        break;
    case 8:
        p_regs->sw74.init_reflist_pf8 = val;
        break;
    case 9:
        p_regs->sw74.init_reflist_pf9 = val;
        break;
    case 10:
        p_regs->sw75.init_reflist_pf10 = val;
        break;
    case 11:
        p_regs->sw75.init_reflist_pf11 = val;
        break;
    case 12:
        p_regs->sw75.init_reflist_pf12 = val;
        break;
    case 13:
        p_regs->sw75.init_reflist_pf13 = val;
        break;
    case 14:
        p_regs->sw75.init_reflist_pf14 = val;
        break;
    case 15:
        p_regs->sw75.init_reflist_pf15 = val;
        break;
    default:
        break;
    }

    return MPP_OK;
}

static MPP_RET set_refer_pic_list_b0(H264dVdpuRegs_t *p_regs, RK_U32 i,
                                     RK_U16 val)
{
    switch (i) {
    case 0:
        p_regs->sw100.init_reflist_df0 = val;
        break;
    case 1:
        p_regs->sw100.init_reflist_df1 = val;
        break;
    case 2:
        p_regs->sw100.init_reflist_df2 = val;
        break;
    case 3:
        p_regs->sw100.init_reflist_df3 = val;
        break;
    case 4:
        p_regs->sw100.init_reflist_df4 = val;
        break;
    case 5:
        p_regs->sw100.init_reflist_df5 = val;
        break;
    case 6:
        p_regs->sw101.init_reflist_df6 = val;
        break;
    case 7:
        p_regs->sw101.init_reflist_df7 = val;
        break;
    case 8:
        p_regs->sw101.init_reflist_df8 = val;
        break;
    case 9:
        p_regs->sw101.init_reflist_df9 = val;
        break;
    case 10:
        p_regs->sw101.init_reflist_df10 = val;
        break;
    case 11:
        p_regs->sw101.init_reflist_df11 = val;
        break;
    case 12:
        p_regs->sw102.init_reflist_df12 = val;
        break;
    case 13:
        p_regs->sw102.init_reflist_df13 = val;
        break;
    case 14:
        p_regs->sw102.init_reflist_df14 = val;
        break;
    case 15:
        p_regs->sw102.init_reflist_df15 = val;
        break;
    default:
        break;
    }

    return MPP_OK;
}

static MPP_RET set_refer_pic_list_b1(H264dVdpuRegs_t *p_regs, RK_U32 i,
                                     RK_U16 val)
{
    switch (i) {
    case 0:
        p_regs->sw103.init_reflist_db0 = val;
        break;
    case 1:
        p_regs->sw103.init_reflist_db1 = val;
        break;
    case 2:
        p_regs->sw103.init_reflist_db2 = val;
        break;
    case 3:
        p_regs->sw103.init_reflist_db3 = val;
        break;
    case 4:
        p_regs->sw103.init_reflist_db4 = val;
        break;
    case 5:
        p_regs->sw103.init_reflist_db5 = val;
        break;
    case 6:
        p_regs->sw104.init_reflist_db6 = val;
        break;
    case 7:
        p_regs->sw104.init_reflist_db7 = val;
        break;
    case 8:
        p_regs->sw104.init_reflist_db8 = val;
        break;
    case 9:
        p_regs->sw104.init_reflist_db9 = val;
        break;
    case 10:
        p_regs->sw104.init_reflist_db10 = val;
        break;
    case 11:
        p_regs->sw104.init_reflist_db11 = val;
        break;
    case 12:
        p_regs->sw105.init_reflist_db12 = val;
        break;
    case 13:
        p_regs->sw105.init_reflist_db13 = val;
        break;
    case 14:
        p_regs->sw105.init_reflist_db14 = val;
        break;
    case 15:
        p_regs->sw105.init_reflist_db15 = val;
        break;
    default:
        break;
    }

    return MPP_OK;
}

static MPP_RET set_refer_pic_base_addr(H264dVdpuRegs_t *p_regs, RK_U32 i,
                                       RK_U32 val)
{
    switch (i) {
    case 0:
        p_regs->sw84.ref0_st_addr = val;
        break;
    case 1:
        p_regs->sw85.ref1_st_addr = val;
        break;
    case 2:
        p_regs->sw86.ref2_st_addr = val;
        break;
    case 3:
        p_regs->sw87.ref3_st_addr = val;
        break;
    case 4:
        p_regs->sw88.ref4_st_addr = val;
        break;
    case 5:
        p_regs->sw89.ref5_st_addr = val;
        break;
    case 6:
        p_regs->sw90.ref6_st_addr = val;
        break;
    case 7:
        p_regs->sw91.ref7_st_addr = val;
        break;
    case 8:
        p_regs->sw92.ref8_st_addr = val;
        break;
    case 9:
        p_regs->sw93.ref9_st_addr = val;
        break;
    case 10:
        p_regs->sw94.ref10_st_addr = val;
        break;
    case 11:
        p_regs->sw95.ref11_st_addr = val;
        break;
    case 12:
        p_regs->sw96.ref12_st_addr = val;
        break;
    case 13:
        p_regs->sw97.ref13_st_addr = val;
        break;
    case 14:
        p_regs->sw98.ref14_st_addr = val;
        break;
    case 15:
        p_regs->sw99.ref15_st_addr = val;
        break;
    default:
        break;
    }
    return MPP_OK;
}

static MPP_RET set_pic_regs(H264dHalCtx_t *p_hal, H264dVdpuRegs_t *p_regs)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    p_regs->sw110.pic_mb_w = p_hal->pp->wFrameWidthInMbsMinus1 + 1;
    p_regs->sw110.pic_mb_h = (2 - p_hal->pp->frame_mbs_only_flag)
                             * (p_hal->pp->wFrameHeightInMbsMinus1 + 1);

    return ret = MPP_OK;
}

static MPP_RET set_vlc_regs(H264dHalCtx_t *p_hal, H264dVdpuRegs_t *p_regs)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;

    p_regs->sw57.dec_wr_extmen_dis = 0;
    p_regs->sw57.rlc_mode_en = 0;
    p_regs->sw51.qp_init_val = pp->pic_init_qp_minus26 + 26;
    p_regs->sw114.max_refidx0 = pp->num_ref_idx_l0_active_minus1 + 1;
    p_regs->sw111.max_refnum = pp->num_ref_frames;
    p_regs->sw112.cur_frm_len = pp->log2_max_frame_num_minus4 + 4;
    p_regs->sw112.curfrm_num = pp->frame_num;
    p_regs->sw115.const_intra_en = pp->constrained_intra_pred_flag;
    p_regs->sw112.dblk_ctrl_flag = pp->deblocking_filter_control_present_flag;
    p_regs->sw112.rpcp_flag = pp->redundant_pic_cnt_present_flag;
    p_regs->sw113.refpic_mk_len = p_hal->slice_long[0].drpm_used_bitlen;
    p_regs->sw115.idr_pic_flag = p_hal->slice_long[0].idr_flag;
    p_regs->sw113.idr_pic_id = p_hal->slice_long[0].idr_pic_id;
    p_regs->sw114.pps_id = p_hal->slice_long[0].active_pps_id;
    p_regs->sw114.poc_field_len = p_hal->slice_long[0].poc_used_bitlen;
    /* reference picture flags, TODO separate fields */
    if (pp->field_pic_flag) {
        RK_U32 validFlags = 0;
        RK_U32 longTermTmp = 0, longTermflags = 0;
        for (i = 0; i < 32; i++) {
            if (pp->RefFrameList[i / 2].bPicEntry == 0xff) { //!< invalid
                longTermflags <<= 1;
                validFlags <<= 1;
            } else {
                longTermTmp = pp->RefFrameList[i / 2].AssociatedFlag; //!< get long term flag
                longTermflags = (longTermflags << 1) | longTermTmp;

                validFlags = (validFlags << 1)
                             | ((pp->UsedForReferenceFlags >> i) & 0x01);
            }
        }
        p_regs->sw107.refpic_term_flag = longTermflags;
        p_regs->sw108.refpic_valid_flag = validFlags;
    } else {
        RK_U32 validFlags = 0;
        RK_U32 longTermTmp = 0, longTermflags = 0;
        for (i = 0; i < 16; i++) {
            if (pp->RefFrameList[i].bPicEntry == 0xff) {  //!< invalid
                longTermflags <<= 1;
                validFlags <<= 1;
            } else {
                longTermTmp = pp->RefFrameList[i].AssociatedFlag;
                longTermflags = (longTermflags << 1) | longTermTmp;
                validFlags = (validFlags << 1)
                             | ((pp->UsedForReferenceFlags >> (2 * i)) & 0x03);
            }
        }
        p_regs->sw107.refpic_term_flag = (longTermflags << 16);
        p_regs->sw108.refpic_valid_flag = (validFlags << 16);
    }

    for (i = 0; i < 16; i++) {
        if (pp->RefFrameList[i].bPicEntry != 0xff) { //!< valid
            if (pp->RefFrameList[i].AssociatedFlag) { //!< longterm flag
                set_refer_pic_idx(p_regs, i, pp->LongTermPicNumList[i]); //!< pic_num
            } else {
                set_refer_pic_idx(p_regs, i, pp->FrameNumList[i]); //< frame_num
            }
        }
    }
    p_regs->sw57.rd_cnt_tab_en = 1;
    //!< set poc to buffer
    {
        H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
        RK_U32 *ptr = (RK_U32 *)reg_ctx->poc_ptr;
        //!< set reference reorder poc
        for (i = 0; i < 32; i++) {
            if (pp->RefFrameList[i / 2].bPicEntry != 0xff) {
                *ptr++ = pp->FieldOrderCntList[i / 2][i & 0x1];
            } else {
                *ptr++ = 0;
            }
        }
        //!< set current poc
        if (pp->field_pic_flag || !pp->MbaffFrameFlag) {
            *ptr++ = pp->CurrFieldOrderCnt[0];
            *ptr++ = pp->CurrFieldOrderCnt[1];
        } else {
            *ptr++ = pp->CurrFieldOrderCnt[0];
            *ptr++ = pp->CurrFieldOrderCnt[1];
        }
    }
    p_regs->sw115.cabac_en = pp->entropy_coding_mode_flag;
    //!< stream position update
    {
        MppBuffer bitstream_buf = NULL;
        p_regs->sw57.st_code_exit = 1;
        mpp_buf_slot_get_prop(p_hal->packet_slots,
                              p_hal->in_task->input,
                              SLOT_BUFFER, &bitstream_buf);
        p_regs->sw109.strm_start_bit = 0; //!< sodb stream start bit
        p_regs->sw64.rlc_vlc_st_adr = mpp_buffer_get_fd(bitstream_buf);
        p_regs->sw51.stream_len = p_hal->strm_len;
    }

    return ret = MPP_OK;
}

static MPP_RET set_ref_regs(H264dHalCtx_t *p_hal, H264dVdpuRegs_t *p_regs)
{
    RK_U32 i = 0, j = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    DXVA_Slice_H264_Long *p_long = &p_hal->slice_long[0];

    //!< list0 list1 listP
    for (j = 0; j < 3; j++) {
        for (i = 0; i < 16; i++) {
            RK_U16 val = 0;
            if (p_hal->pp->field_pic_flag) { //!< field
                RK_U32 nn = 0;
                nn = p_hal->pp->CurrPic.AssociatedFlag ? (2 * i + 1) : (2 * i);
                if (p_long->RefPicList[j][nn].bPicEntry == 0xff) {
                    val = vdpu_value_list[i];
                } else {
                    val = p_long->RefPicList[j][nn].Index7Bits;
                }
            } else { //!< frame
                if (p_long->RefPicList[j][i].bPicEntry == 0xff) {
                    val = vdpu_value_list[i];
                } else {
                    val = p_long->RefPicList[j][i].Index7Bits;
                }
            }
            switch (j) {
            case 0:
                set_refer_pic_list_p(p_regs, i, val);
                break;
            case 1:
                set_refer_pic_list_b0(p_regs, i, val);

                break;
            case 2:
                set_refer_pic_list_b1(p_regs, i, val);

                break;
            default:
                break;
            }
        }
    }

    return ret = MPP_OK;
}

static MPP_RET set_asic_regs(H264dHalCtx_t *p_hal, H264dVdpuRegs_t *p_regs)
{
    RK_U32 i = 0, j = 0;
    RK_U32 outPhyAddr = 0;
    MppBuffer frame_buf = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    DXVA_Slice_H264_Long *p_long = &p_hal->slice_long[0];

    /* reference picture physic address */
    for (i = 0, j = 0xff; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        RK_U32 val = 0;
        RK_U32 top_closer = 0;
        RK_U32 field_flag = 0;
        if (pp->RefFrameList[i].bPicEntry != 0xff) {
            mpp_buf_slot_get_prop(p_hal->frame_slots,
                                  pp->RefFrameList[i].Index7Bits,
                                  SLOT_BUFFER, &frame_buf); //!< reference phy addr
            j = i;
        } else {
            mpp_buf_slot_get_prop(p_hal->frame_slots,
                                  pp->CurrPic.Index7Bits,
                                  SLOT_BUFFER, &frame_buf); //!< current out phy addr
        }

        field_flag = ((pp->RefPicFiledFlags >> i) & 0x1) ? 0x2 : 0;
        if (field_flag) {
            RK_U32 used_flag = 0;
            RK_S32 cur_poc = 0;
            RK_S32 ref_poc = 0;

            cur_poc = pp->CurrPic.AssociatedFlag
                      ? pp->CurrFieldOrderCnt[1] : pp->CurrFieldOrderCnt[0];
            used_flag = ((pp->UsedForReferenceFlags >> (2 * i)) & 0x3);
            if (used_flag & 0x3) {
                ref_poc = MPP_MIN(pp->FieldOrderCntList[i][0],
                                  pp->FieldOrderCntList[i][1]);
            } else if (used_flag & 0x2) {
                ref_poc = pp->FieldOrderCntList[i][1];
            } else if (used_flag & 0x1) {
                ref_poc = pp->FieldOrderCntList[i][0];
            }
            top_closer = (cur_poc < ref_poc) ? 0x1 : 0;
        }
        val = top_closer | field_flag;
        val = mpp_buffer_get_fd(frame_buf) | (val << 10);
        set_refer_pic_base_addr(p_regs, i, val);
    }
    /* inter-view reference picture */
    {
        H264dVdpuPriv_t *priv = (H264dVdpuPriv_t *)p_hal->priv;
        if (pp->curr_layer_id && priv->ilt_dpb && priv->ilt_dpb->valid) {
            mpp_buf_slot_get_prop(p_hal->frame_slots,
                                  priv->ilt_dpb->slot_index,
                                  SLOT_BUFFER, &frame_buf);
            p_regs->sw99.ref15_st_addr = mpp_buffer_get_fd(frame_buf); //!< inter-view base, ref15
            p_regs->sw108.refpic_valid_flag |= (pp->field_pic_flag
                                                ? 0x3 : 0x10000);
        }
    }
    p_regs->sw50.dec_fixed_quant = pp->curr_layer_id; //!< VDPU_MVC_E
    p_regs->sw50.dblk_flt_dis = 0; //!< filterDisable = 0;
    mpp_buf_slot_get_prop(p_hal->frame_slots,
                          pp->CurrPic.Index7Bits,
                          SLOT_BUFFER, &frame_buf); //!< current out phy addr
    outPhyAddr = mpp_buffer_get_fd(frame_buf);
    if (pp->field_pic_flag && pp->CurrPic.AssociatedFlag) {
        outPhyAddr |= ((pp->wFrameWidthInMbsMinus1 + 1) * 16) << 10;
    }
    p_regs->sw63.dec_out_st_adr = outPhyAddr; //!< outPhyAddr, pp->CurrPic.Index7Bits
    p_regs->sw110.flt_offset_cb_qp = pp->chroma_qp_index_offset;
    p_regs->sw110.flt_offset_cr_qp = pp->second_chroma_qp_index_offset;
    /* set default value for register[41] to avoid illegal translation fd */
    {
        RK_U32 dirMvOffset = 0;
        RK_U32 picSizeInMbs = 0;

        picSizeInMbs = p_hal->pp->wFrameWidthInMbsMinus1 + 1;
        picSizeInMbs = picSizeInMbs
                       * (2 - pp->frame_mbs_only_flag) * (pp->wFrameHeightInMbsMinus1 + 1);
        dirMvOffset = picSizeInMbs
                      * ((p_hal->pp->chroma_format_idc == 0) ? 256 : 384);
        dirMvOffset += (pp->field_pic_flag && pp->CurrPic.AssociatedFlag)
                       ? (picSizeInMbs * 32) : 0;
        p_regs->sw62.dmmv_st_adr = (mpp_buffer_get_fd(frame_buf) | (dirMvOffset << 6));
    }
    p_regs->sw57.dmmv_wr_en = (p_long->nal_ref_idc != 0) ? 1 : 0; //!< defalut set 1
    p_regs->sw115.dlmv_method_en = pp->direct_8x8_inference_flag;
    p_regs->sw115.weight_pred_en = pp->weighted_pred_flag;
    p_regs->sw111.wp_bslice_sel = pp->weighted_bipred_idc;
    p_regs->sw114.max_refidx1 = (pp->num_ref_idx_l1_active_minus1 + 1);
    p_regs->sw115.fieldpic_flag_exist = (!pp->frame_mbs_only_flag) ? 1 : 0;
    p_regs->sw57.curpic_code_sel = (!pp->frame_mbs_only_flag
                                    && (pp->MbaffFrameFlag || pp->field_pic_flag)) ? 1 : 0;
    p_regs->sw57.curpic_stru_sel = pp->field_pic_flag;
    p_regs->sw57.pic_decfield_sel = (!pp->CurrPic.AssociatedFlag) ? 1 : 0; //!< bottomFieldFlag
    p_regs->sw57.sequ_mbaff_en = pp->MbaffFrameFlag;
    p_regs->sw115.tranf_8x8_flag_en = pp->transform_8x8_mode_flag;
    p_regs->sw115.monochr_en = (p_long->profileIdc >= 100
                                && pp->chroma_format_idc == 0) ? 1 : 0;
    p_regs->sw115.scl_matrix_en = pp->scaleing_list_enable_flag;
    {
        H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
        if (p_hal->pp->scaleing_list_enable_flag) {
            RK_U32 temp = 0;
            RK_U32 *ptr = (RK_U32 *)reg_ctx->sclst_ptr;

            for (i = 0; i < 6; i++) {
                for (j = 0; j < 4; j++) {
                    temp = (p_hal->qm->bScalingLists4x4[i][4 * j + 0] << 24) |
                           (p_hal->qm->bScalingLists4x4[i][4 * j + 1] << 16) |
                           (p_hal->qm->bScalingLists4x4[i][4 * j + 2] << 8) |
                           (p_hal->qm->bScalingLists4x4[i][4 * j + 3]);
                    *ptr++ = temp;
                }
            }
            for (i = 0; i < 2; i++) {
                for (j = 0; j < 16; j++) {
                    temp = (p_hal->qm->bScalingLists8x8[i][4 * j + 0] << 24) |
                           (p_hal->qm->bScalingLists8x8[i][4 * j + 1] << 16) |
                           (p_hal->qm->bScalingLists8x8[i][4 * j + 2] << 8) |
                           (p_hal->qm->bScalingLists8x8[i][4 * j + 3]);
                    *ptr++ = temp;
                }
            }
        }
        p_regs->sw61.qtable_st_adr = mpp_buffer_get_fd(reg_ctx->buf);
    }
    p_regs->sw57.dec_wr_extmen_dis = 0; //!< set defalut 0
    p_regs->sw57.addit_ch_fmt_wen = 0;
    p_regs->sw57.dec_st_work = 1;

    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*    init  VDPU granite decoder
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == hal);

    p_hal->fast_mode = cfg->fast_mode;
    MEM_CHECK(ret, p_hal->priv = mpp_calloc_size(void,
                                                 sizeof(H264dVdpuPriv_t)));
    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(H264dVdpuRegCtx_t)));
    H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
    //!< malloc buffers
    {
        RK_U32 i = 0;
        RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

        RK_U32 buf_size = VDPU_CABAC_TAB_SIZE +  VDPU_POC_BUF_SIZE + VDPU_SCALING_LIST_SIZE;
        for (i = 0; i < loop; i++) {
            FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->reg_buf[i].buf,  buf_size));
            reg_ctx->reg_buf[i].cabac_ptr = mpp_buffer_get_ptr(reg_ctx->reg_buf[i].buf);
            reg_ctx->reg_buf[i].poc_ptr = reg_ctx->reg_buf[i].cabac_ptr + VDPU_CABAC_TAB_SIZE;
            reg_ctx->reg_buf[i].sclst_ptr = reg_ctx->reg_buf[i].poc_ptr + VDPU_POC_BUF_SIZE;
            reg_ctx->reg_buf[i].regs = mpp_calloc_size(void, sizeof(H264dVdpuRegs_t));
            //!< copy cabac table bytes
            memcpy(reg_ctx->reg_buf[i].cabac_ptr, (void *)vdpu_cabac_table,  sizeof(vdpu_cabac_table));
        }
    }

    if (!p_hal->fast_mode) {
        reg_ctx->buf = reg_ctx->reg_buf[0].buf;
        reg_ctx->cabac_ptr = reg_ctx->reg_buf[0].cabac_ptr;
        reg_ctx->poc_ptr = reg_ctx->reg_buf[0].poc_ptr;
        reg_ctx->sclst_ptr = reg_ctx->reg_buf[0].sclst_ptr;
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
    }

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, vdpu_hor_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, vdpu_ver_align);

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu2_h264d_deinit(hal);

    return ret;
}


/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_deinit(void *hal)
{
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;

    RK_U32 i = 0;
    RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;
    for (i = 0; i < loop; i++) {
        MPP_FREE(reg_ctx->reg_buf[i].regs);
        mpp_buffer_put(reg_ctx->reg_buf[i].buf);
    }

    MPP_FREE(p_hal->reg_ctx);
    MPP_FREE(p_hal->priv);

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dVdpuPriv_t *priv = NULL;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == p_hal);
    p_hal->in_task = &task->dec;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }
    priv = p_hal->priv;
    priv->layed_id = p_hal->pp->curr_layer_id;

    H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(reg_ctx->reg_buf); i++) {
            if (!reg_ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                reg_ctx->buf = reg_ctx->reg_buf[i].buf;
                reg_ctx->cabac_ptr = reg_ctx->reg_buf[i].cabac_ptr;
                reg_ctx->poc_ptr = reg_ctx->reg_buf[i].poc_ptr;
                reg_ctx->sclst_ptr = reg_ctx->reg_buf[i].sclst_ptr;
                reg_ctx->regs = reg_ctx->reg_buf[i].regs;
                reg_ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

    FUN_CHECK(ret = adjust_input(priv, &p_hal->slice_long[0], p_hal->pp));
    FUN_CHECK(ret = set_device_regs(p_hal, (H264dVdpuRegs_t *)reg_ctx->regs));
    FUN_CHECK(ret = set_pic_regs(p_hal, (H264dVdpuRegs_t *)reg_ctx->regs));
    FUN_CHECK(ret = set_vlc_regs(p_hal, (H264dVdpuRegs_t *)reg_ctx->regs));
    FUN_CHECK(ret = set_ref_regs(p_hal, (H264dVdpuRegs_t *)reg_ctx->regs));
    FUN_CHECK(ret = set_asic_regs(p_hal, (H264dVdpuRegs_t *)reg_ctx->regs));

__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal  = (H264dHalCtx_t *)hal;
    H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
    H264dVdpuRegs_t *p_regs = (H264dVdpuRegs_t *)reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }
    p_regs->sw57.cache_en = 1;
    p_regs->sw57.pref_sigchan = 1;
    p_regs->sw56.bus_pos_sel = 1;
    p_regs->sw57.intra_dbl3t = 1;
    p_regs->sw57.inter_dblspeed = 1;
    p_regs->sw57.intra_dblspeed = 1;

    ret = mpp_device_send_reg(p_hal->dev_ctx, (RK_U32 *)reg_ctx->regs,
                              DEC_VDPU_REGISTERS);
    if (ret) {
        ret =  MPP_ERR_VPUHW;
        mpp_err("H264 VDPU FlushRegs fail, pid=%d.\n", getpid());
    }

__RETURN:
    (void)task;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t  *p_hal = (H264dHalCtx_t *)hal;
    H264dVdpuRegCtx_t *reg_ctx = (H264dVdpuRegCtx_t *)p_hal->reg_ctx;
    H264dVdpuRegs_t *p_regs = (H264dVdpuRegs_t *)reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __SKIP_HARD;
    }

    {
        RK_S32 wait_ret = -1;
        wait_ret = mpp_device_wait_reg(p_hal->dev_ctx, (RK_U32 *)reg_ctx->regs,
                                       DEC_VDPU_REGISTERS);
        if (wait_ret) {
            ret = MPP_ERR_VPUHW;
            mpp_err("H264 VDPU wait result fail, pid=%d.\n", getpid());
        }
    }

__SKIP_HARD:
    if (p_hal->init_cb.callBack) {
        IOCallbackCtx m_ctx = { 0 };
        m_ctx.device_id = HAL_VDPU;
        if (!p_regs->sw55.dec_rdy_sts) {
            m_ctx.hard_err = 1;
        }
        m_ctx.task = (void *)&task->dec;
        m_ctx.regs = (RK_U32 *)reg_ctx->regs;
        p_hal->init_cb.callBack(p_hal->init_cb.opaque, &m_ctx);
    }
    memset(&p_regs->sw55, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    memset(p_hal->priv, 0, sizeof(H264dVdpuPriv_t));

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

__RETURN:
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET vdpu2_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    (void)hal;
    (void)cmd_type;
    (void)param;
__RETURN:
    return ret = MPP_OK;
}
