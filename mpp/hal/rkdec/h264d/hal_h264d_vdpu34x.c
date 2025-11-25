/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_h264d_vdpu34x"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"

#include "mpp_device.h"

#include "hal_h264d_global.h"
#include "hal_h264d_vdpu34x.h"
#include "vdpu34x_h264d.h"
#include "mpp_dec_cb_param.h"
#include "vdpu_com.h"
#include "hal_h264d_ctx.h"
#include "hal_h264d_com.h"

/* Number registers for the decoder */
#define DEC_VDPU34X_REGISTERS               276

#define VDPU34X_CABAC_TAB_SIZE              (928*4 + 128)       /* bytes */
#define VDPU34X_SPSPPS_UNIT_SIZE            (48)                /* bytes */
#define VDPU34X_SPSPPS_SIZE                 (256*48 + 128)      /* bytes */
#define VDPU34X_RPS_SIZE                    (128 + 128 + 128)   /* bytes */
#define VDPU34X_SCALING_LIST_SIZE           (6*16+2*64 + 128)   /* bytes */
#define VDPU34X_ERROR_INFO_SIZE             (256*144*4)         /* bytes */

#define VDPU34X_CABAC_TAB_ALIGNED_SIZE      (MPP_ALIGN(VDPU34X_CABAC_TAB_SIZE, SZ_4K))
#define VDPU34X_ERROR_INFO_ALIGNED_SIZE     (0)
#define VDPU34X_SPSPPS_ALIGNED_SIZE         (MPP_ALIGN(VDPU34X_SPSPPS_SIZE, SZ_4K))
#define VDPU34X_RPS_ALIGNED_SIZE            (MPP_ALIGN(VDPU34X_RPS_SIZE, SZ_4K))
#define VDPU34X_SCALING_LIST_ALIGNED_SIZE   (MPP_ALIGN(VDPU34X_SCALING_LIST_SIZE, SZ_4K))
#define VDPU34X_STREAM_INFO_SET_SIZE        (VDPU34X_SPSPPS_ALIGNED_SIZE + \
                                             VDPU34X_RPS_ALIGNED_SIZE + \
                                             VDPU34X_SCALING_LIST_ALIGNED_SIZE)

#define VDPU34X_CABAC_TAB_OFFSET            (0)
#define VDPU34X_ERROR_INFO_OFFSET           (VDPU34X_CABAC_TAB_OFFSET + VDPU34X_CABAC_TAB_ALIGNED_SIZE)
#define VDPU34X_STREAM_INFO_OFFSET_BASE     (VDPU34X_ERROR_INFO_OFFSET + VDPU34X_ERROR_INFO_ALIGNED_SIZE)
#define VDPU34X_SPSPPS_OFFSET(pos)          (VDPU34X_STREAM_INFO_OFFSET_BASE + (VDPU34X_STREAM_INFO_SET_SIZE * pos))
#define VDPU34X_RPS_OFFSET(pos)             (VDPU34X_SPSPPS_OFFSET(pos) + VDPU34X_SPSPPS_ALIGNED_SIZE)
#define VDPU34X_SCALING_LIST_OFFSET(pos)    (VDPU34X_RPS_OFFSET(pos) + VDPU34X_RPS_ALIGNED_SIZE)
#define VDPU34X_INFO_BUFFER_SIZE(cnt)       (VDPU34X_STREAM_INFO_OFFSET_BASE + (VDPU34X_STREAM_INFO_SET_SIZE * cnt))

#define VDPU34X_SPS_PPS_LEN     (43)

#define SET_REF_INFO(regs, index, field, value)\
    do{ \
        switch(index){\
        case 0: regs.reg99.ref0_##field = value; break;\
        case 1: regs.reg99.ref1_##field = value; break;\
        case 2: regs.reg99.ref2_##field = value; break;\
        case 3: regs.reg99.ref3_##field = value; break;\
        case 4: regs.reg100.ref4_##field = value; break;\
        case 5: regs.reg100.ref5_##field = value; break;\
        case 6: regs.reg100.ref6_##field = value; break;\
        case 7: regs.reg100.ref7_##field = value; break;\
        case 8: regs.reg101.ref8_##field = value; break;\
        case 9: regs.reg101.ref9_##field = value; break;\
        case 10: regs.reg101.ref10_##field = value; break;\
        case 11: regs.reg101.ref11_##field = value; break;\
        case 12: regs.reg102.ref12_##field = value; break;\
        case 13: regs.reg102.ref13_##field = value; break;\
        case 14: regs.reg102.ref14_##field = value; break;\
        case 15: regs.reg102.ref15_##field = value; break;\
        default: break;}\
    }while(0)

#define SET_POC_HIGNBIT_INFO(regs, index, field, value)\
    do{ \
        switch(index){\
        case 0: regs.reg200.ref0_##field = value; break;\
        case 1: regs.reg200.ref1_##field = value; break;\
        case 2: regs.reg200.ref2_##field = value; break;\
        case 3: regs.reg200.ref3_##field = value; break;\
        case 4: regs.reg200.ref4_##field = value; break;\
        case 5: regs.reg200.ref5_##field = value; break;\
        case 6: regs.reg200.ref6_##field = value; break;\
        case 7: regs.reg200.ref7_##field = value; break;\
        case 8: regs.reg201.ref8_##field = value; break;\
        case 9: regs.reg201.ref9_##field = value; break;\
        case 10: regs.reg201.ref10_##field = value; break;\
        case 11: regs.reg201.ref11_##field = value; break;\
        case 12: regs.reg201.ref12_##field = value; break;\
        case 13: regs.reg201.ref13_##field = value; break;\
        case 14: regs.reg201.ref14_##field = value; break;\
        case 15: regs.reg201.ref15_##field = value; break;\
        case 16: regs.reg202.ref16_##field = value; break;\
        case 17: regs.reg202.ref17_##field = value; break;\
        case 18: regs.reg202.ref18_##field = value; break;\
        case 19: regs.reg202.ref19_##field = value; break;\
        case 20: regs.reg202.ref20_##field = value; break;\
        case 21: regs.reg202.ref21_##field = value; break;\
        case 22: regs.reg202.ref22_##field = value; break;\
        case 23: regs.reg202.ref23_##field = value; break;\
        case 24: regs.reg203.ref24_##field = value; break;\
        case 25: regs.reg203.ref25_##field = value; break;\
        case 26: regs.reg203.ref26_##field = value; break;\
        case 27: regs.reg203.ref27_##field = value; break;\
        case 28: regs.reg203.ref28_##field = value; break;\
        case 29: regs.reg203.ref29_##field = value; break;\
        case 30: regs.reg203.ref30_##field = value; break;\
        case 31: regs.reg203.ref31_##field = value; break;\
        default: break;}\
    }while(0)

MPP_RET vdpu34x_h264d_deinit(void *hal);

static MPP_RET prepare_spspps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0;
    RK_S32 is_long_term = 0, voidx = 0;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    RK_U32 tmp = 0;
    BitputCtx_t bp;

    mpp_set_bitput_ctx(&bp, data, len);

    if (!p_hal->fast_mode && !pp->spspps_update) {
        bp.index = VDPU34X_SPS_PPS_LEN >> 3;
        bp.bitpos = (VDPU34X_SPS_PPS_LEN & 0x7) << 3;
    } else {
        //!< sps syntax
        mpp_put_bits(&bp, -1, 13); //!< sps_id 4bit && profile_idc 8bit && constraint_set3_flag 1bit
        mpp_put_bits(&bp, pp->chroma_format_idc, 2);
        mpp_put_bits(&bp, pp->bit_depth_luma_minus8, 3);
        mpp_put_bits(&bp, pp->bit_depth_chroma_minus8, 3);
        mpp_put_bits(&bp, 0, 1);   //!< qpprime_y_zero_transform_bypass_flag
        mpp_put_bits(&bp, pp->log2_max_frame_num_minus4, 4);
        mpp_put_bits(&bp, pp->num_ref_frames, 5);
        mpp_put_bits(&bp, pp->pic_order_cnt_type, 2);
        mpp_put_bits(&bp, pp->log2_max_pic_order_cnt_lsb_minus4, 4);
        mpp_put_bits(&bp, pp->delta_pic_order_always_zero_flag, 1);
        mpp_put_bits(&bp, (pp->wFrameWidthInMbsMinus1 + 1), 12);
        mpp_put_bits(&bp, (pp->wFrameHeightInMbsMinus1 + 1), 12);
        mpp_put_bits(&bp, pp->frame_mbs_only_flag, 1);
        mpp_put_bits(&bp, pp->MbaffFrameFlag, 1);
        mpp_put_bits(&bp, pp->direct_8x8_inference_flag, 1);

        mpp_put_bits(&bp, 1, 1);    //!< mvc_extension_enable
        mpp_put_bits(&bp, (pp->num_views_minus1 + 1), 2);
        mpp_put_bits(&bp, pp->view_id[0], 10);
        mpp_put_bits(&bp, pp->view_id[1], 10);
        mpp_put_bits(&bp, pp->num_anchor_refs_l0[0], 1);
        if (pp->num_anchor_refs_l0[0]) {
            mpp_put_bits(&bp, pp->anchor_ref_l0[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10);
        }
        mpp_put_bits(&bp, pp->num_anchor_refs_l1[0], 1);
        if (pp->num_anchor_refs_l1[0]) {
            mpp_put_bits(&bp, pp->anchor_ref_l1[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10); //!< anchor_ref_l1
        }
        mpp_put_bits(&bp, pp->num_non_anchor_refs_l0[0], 1);
        if (pp->num_non_anchor_refs_l0[0]) {
            mpp_put_bits(&bp, pp->non_anchor_ref_l0[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10); //!< non_anchor_ref_l0
        }
        mpp_put_bits(&bp, pp->num_non_anchor_refs_l1[0], 1);
        if (pp->num_non_anchor_refs_l1[0]) {
            mpp_put_bits(&bp, pp->non_anchor_ref_l1[0][0], 10);
        } else {
            mpp_put_bits(&bp, 0, 10);//!< non_anchor_ref_l1
        }
        mpp_put_align(&bp, 128, 0);
        //!< pps syntax
        mpp_put_bits(&bp, -1, 13); //!< pps_id 8bit && sps_id 5bit
        mpp_put_bits(&bp, pp->entropy_coding_mode_flag, 1);
        mpp_put_bits(&bp, pp->pic_order_present_flag, 1);
        mpp_put_bits(&bp, pp->num_ref_idx_l0_active_minus1, 5);
        mpp_put_bits(&bp, pp->num_ref_idx_l1_active_minus1, 5);
        mpp_put_bits(&bp, pp->weighted_pred_flag, 1);
        mpp_put_bits(&bp, pp->weighted_bipred_idc, 2);
        mpp_put_bits(&bp, pp->pic_init_qp_minus26, 7);
        mpp_put_bits(&bp, pp->pic_init_qs_minus26, 6);
        mpp_put_bits(&bp, pp->chroma_qp_index_offset, 5);
        mpp_put_bits(&bp, pp->deblocking_filter_control_present_flag, 1);
        mpp_put_bits(&bp, pp->constrained_intra_pred_flag, 1);
        mpp_put_bits(&bp, pp->redundant_pic_cnt_present_flag, 1);
        mpp_put_bits(&bp, pp->transform_8x8_mode_flag, 1);
        mpp_put_bits(&bp, pp->second_chroma_qp_index_offset, 5);
        mpp_put_bits(&bp, pp->scaleing_list_enable_flag, 1);
        mpp_put_bits(&bp, 0, 32);// scanlist buffer has another addr
    }

    //!< set dpb
    for (i = 0; i < 16; i++) {
        is_long_term = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefFrameList[i].AssociatedFlag : 0;
        tmp |= (RK_U32)(is_long_term & 0x1) << i;
    }
    for (i = 0; i < 16; i++) {
        voidx = (pp->RefFrameList[i].bPicEntry != 0xff) ? pp->RefPicLayerIdList[i] : 0;
        tmp |= (RK_U32)(voidx & 0x1) << (i + 16);
    }
    mpp_put_bits(&bp, tmp, 32);
    mpp_put_align(&bp, 64, 0);

    return MPP_OK;
}

static MPP_RET prepare_framerps(H264dHalCtx_t *p_hal, RK_U64 *data, RK_U32 len)
{
    RK_S32 i = 0, j = 0;
    RK_S32 dpb_idx = 0, voidx = 0;
    RK_S32 dpb_valid = 0, bottom_flag = 0;
    RK_U32 max_frame_num = 0;
    RK_U16 frame_num_wrap = 0;
    RK_U32 tmp = 0;

    BitputCtx_t bp;
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;

    mpp_set_bitput_ctx(&bp, data, len);
    mpp_put_align(&bp, 128, 0);
    max_frame_num = 1 << (pp->log2_max_frame_num_minus4 + 4);
    for (i = 0; i < 16; i++) {
        if ((pp->NonExistingFrameFlags >> i) & 0x01) {
            frame_num_wrap = 0;
        } else {
            if (pp->RefFrameList[i].AssociatedFlag) {
                frame_num_wrap = pp->FrameNumList[i];
            } else {
                frame_num_wrap = (pp->FrameNumList[i] > pp->frame_num) ?
                                 (pp->FrameNumList[i] - max_frame_num) : pp->FrameNumList[i];
            }
        }

        mpp_put_bits(&bp, frame_num_wrap, 16);
    }

    mpp_put_bits(&bp, 0, 16);//!< NULL
    tmp = 0;
    for (i = 0; i < 16; i++) {
        tmp |= (RK_U32)pp->RefPicLayerIdList[i] << i;
    }
    mpp_put_bits(&bp, tmp, 16);

    for (i = 0; i < 32; i++) {
        tmp = 0;
        dpb_valid = (p_hal->slice_long[0].RefPicList[0][i].bPicEntry == 0xff) ? 0 : 1;
        dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].Index7Bits : 0;
        bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[0][i].AssociatedFlag : 0;
        voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;
        tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
        tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
        tmp |= (RK_U32)(voidx & 0x1) << 6;
        mpp_put_bits(&bp, tmp, 7);
    }
    for (j = 1; j < 3; j++) {
        for (i = 0; i < 32; i++) {
            tmp = 0;
            dpb_valid = (p_hal->slice_long[0].RefPicList[j][i].bPicEntry == 0xff) ? 0 : 1;
            dpb_idx = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].Index7Bits : 0;
            bottom_flag = (dpb_valid != 0) ? p_hal->slice_long[0].RefPicList[j][i].AssociatedFlag : 0;
            voidx = (dpb_valid != 0) ? pp->RefPicLayerIdList[dpb_idx] : 0;
            tmp |= (RK_U32)(dpb_idx | (dpb_valid << 4)) & 0x1f;
            tmp |= (RK_U32)(bottom_flag & 0x1) << 5;
            tmp |= (RK_U32)(voidx & 0x1) << 6;
            mpp_put_bits(&bp, tmp, 7);
        }
    }
    mpp_put_align(&bp, 128, 0);

    return MPP_OK;
}

static MPP_RET prepare_scanlist(H264dHalCtx_t *p_hal, RK_U8 *data, RK_U32 len)
{
    RK_U32 i = 0, j = 0, n = 0;

    if (p_hal->pp->scaleing_list_enable_flag) {
        for (i = 0; i < 6; i++) { //!< 4x4, 6 lists
            for (j = 0; j < 16; j++) {
                data[n++] = p_hal->qm->bScalingLists4x4[i][j];
            }
        }
        for (i = 0; i < 2; i++) { //!< 8x8, 2 lists
            for (j = 0; j < 64; j++) {
                data[n++] = p_hal->qm->bScalingLists8x8[i][j];
            }
        }
    }
    mpp_assert(n <= len);

    return MPP_OK;
}

static MPP_RET set_registers(H264dHalCtx_t *p_hal, Vdpu34xH264dRegSet *regs, HalTaskInfo *task)
{
    DXVA_PicParams_H264_MVC *pp = p_hal->pp;
    Vdpu34xRegCommon *common = &regs->common;
    HalBuf *mv_buf = NULL;

    // memset(regs, 0, sizeof(Vdpu34xH264dRegSet));
    memset(&regs->h264d_highpoc, 0, sizeof(regs->h264d_highpoc));
    common->reg016_str_len = p_hal->strm_len;
    common->reg013.cur_pic_is_idr = p_hal->slice_long->idr_flag;
    common->reg012.colmv_compress_en = (pp->frame_mbs_only_flag != 0) ? 1 : 0;
    common->reg028.sw_poc_arb_flag = 0;

    /* caculate the yuv_frame_size */
    {
        MppFrame mframe = NULL;
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;

        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        hor_virstride = mpp_frame_get_hor_stride(mframe);
        ver_virstride = mpp_frame_get_ver_stride(mframe);
        y_virstride = hor_virstride * ver_virstride;

        if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
            RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
            RK_U32 fbd_offset = MPP_ALIGN(fbc_hdr_stride * (ver_virstride + 16) / 16, SZ_4K);

            common->reg012.fbc_e = 1;
            common->reg018.y_hor_virstride = fbc_hdr_stride / 16;
            common->reg019.uv_hor_virstride = fbc_hdr_stride / 16;
            common->reg020_fbc_payload_off.payload_st_offset = fbd_offset >> 4;
        } else {
            common->reg012.fbc_e = 0;
            common->reg018.y_hor_virstride = hor_virstride / 16;
            common->reg019.uv_hor_virstride = hor_virstride / 16;
            common->reg020_y_virstride.y_virstride = y_virstride / 16;
        }
    }
    /* set current frame info */
    {
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;

        regs->h264d_param.reg65.cur_top_poc = pp->CurrFieldOrderCnt[0];
        regs->h264d_param.reg66.cur_bot_poc = pp->CurrFieldOrderCnt[1];
        mpp_buf_slot_get_prop(p_hal->frame_slots, pp->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        fd = mpp_buffer_get_fd(mbuffer);
        regs->common_addr.reg130_decout_base = fd;

        //colmv_cur_base
        mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, pp->CurrPic.Index7Bits);
        regs->common_addr.reg131_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
        regs->common_addr.reg132_error_ref_base = fd;
        /*
         * poc_hight bit[0] :
         *  0 -> top field or frame
         *  1 -> bot field
         */
        if (pp->field_pic_flag)
            regs->h264d_highpoc.reg204.cur_poc_highbit = pp->CurrPic.AssociatedFlag;
        else
            regs->h264d_highpoc.reg204.cur_poc_highbit = 0;
    }
    /* set reference info */
    {
        RK_S32 i = 0;
        RK_S32 ref_index = -1;
        RK_S32 near_index = -1;
        MppBuffer mbuffer = NULL;
        RK_U32 min_frame_num  = 0;
        MppFrame mframe = NULL;

        for (i = 0; i <= 15; i++) {
            RK_U32 field_flag = (pp->RefPicFiledFlags >> i) & 0x01;
            RK_U32 top_used = (pp->UsedForReferenceFlags >> (2 * i + 0)) & 0x01;
            RK_U32 bot_used = (pp->UsedForReferenceFlags >> (2 * i + 1)) & 0x01;

            regs->h264d_param.reg67_98_ref_poc[2 * i] = pp->FieldOrderCntList[i][0];
            regs->h264d_param.reg67_98_ref_poc[2 * i + 1] = pp->FieldOrderCntList[i][1];
            SET_REF_INFO(regs->h264d_param, i, field, field_flag);
            SET_REF_INFO(regs->h264d_param, i, topfield_used, top_used);
            SET_REF_INFO(regs->h264d_param, i, botfield_used, bot_used);
            SET_REF_INFO(regs->h264d_param, i, colmv_use_flag, (pp->RefPicColmvUsedFlags >> i) & 0x01);

            if (pp->RefFrameList[i].bPicEntry != 0xff) {
                ref_index = pp->RefFrameList[i].Index7Bits;
                near_index = pp->RefFrameList[i].Index7Bits;
            } else {
                ref_index = (near_index < 0) ? pp->CurrPic.Index7Bits : near_index;
            }

            if (pp->field_pic_flag) {
                SET_POC_HIGNBIT_INFO(regs->h264d_highpoc, 2 * i, poc_highbit, 0);
                SET_POC_HIGNBIT_INFO(regs->h264d_highpoc, 2 * i + 1, poc_highbit, 1);
            } else if (ref_index == pp->CurrPic.Index7Bits) {
                SET_POC_HIGNBIT_INFO(regs->h264d_highpoc, 2 * i, poc_highbit, 3);
                SET_POC_HIGNBIT_INFO(regs->h264d_highpoc, 2 * i + 1, poc_highbit, 3);
            }

            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_BUFFER, &mbuffer);
            mpp_buf_slot_get_prop(p_hal->frame_slots, ref_index, SLOT_FRAME_PTR, &mframe);

            if (pp->FrameNumList[i] < pp->frame_num &&
                pp->FrameNumList[i] > min_frame_num &&
                (!mpp_frame_get_errinfo(mframe))) {
                min_frame_num = pp->FrameNumList[i];
                regs->common_addr.reg132_error_ref_base =  mpp_buffer_get_fd(mbuffer);
                if (!pp->weighted_pred_flag)
                    common->reg021.error_intra_mode = 0;
            }

            regs->h264d_addr.ref_base[i] = mpp_buffer_get_fd(mbuffer);
            mv_buf = hal_bufs_get_buf(p_hal->cmv_bufs, ref_index);
            regs->h264d_addr.colmv_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
        }
    }
    /* set input */
    {
        MppBuffer mbuffer = NULL;
        Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;

        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->common_addr.reg128_rlc_base = mpp_buffer_get_fd(mbuffer);
        regs->common_addr.reg129_rlcwrite_base = regs->common_addr.reg128_rlc_base;

        regs->h264d_addr.cabactbl_base = reg_ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 197, reg_ctx->offset_cabac);
    }

    return MPP_OK;
}

static MPP_RET init_common_regs(Vdpu34xH264dRegSet *regs)
{
    Vdpu34xRegCommon *common = &regs->common;

    common->reg009.dec_mode = 1;  //!< h264
    common->reg015.rlc_mode = 0;

    common->reg011.buf_empty_en = 1;
    common->reg011.dec_timeout_e = 1;

    common->reg010.dec_e = 1;
    common->reg017.slice_num = 0x3fff;

    common->reg012.wait_reset_en = 1;
    common->reg013.h26x_error_mode = 1;
    common->reg013.colmv_error_mode = 1;
    common->reg013.h26x_streamd_error_mode = 1;
    common->reg021.error_deb_en = 1;
    common->reg021.inter_error_prc_mode = 0;
    common->reg021.error_intra_mode = 1;

    if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) {
        common->reg024.cabac_err_en_lowbits = 0;
        common->reg025.cabac_err_en_highbits = 0;
        common->reg026.swreg_block_gating_e = 0xfffef;
    } else {
        common->reg024.cabac_err_en_lowbits = 0xffffffff;
        common->reg025.cabac_err_en_highbits = 0x3ff3ffff;
        common->reg026.swreg_block_gating_e = 0xfffff;
    }
    common->reg026.reg_cfg_gating_en = 1;
    common->reg032_timeout_threshold = 0x3ffff;

    common->reg011.dec_clkgate_e = 1;
    common->reg011.dec_e_strmd_clkgate_dis = 0;
    common->reg011.dec_timeout_e = 1;

    common->reg013.timeout_mode = 1;

    return MPP_OK;
}

MPP_RET vdpu34x_h264d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Vdpu3xxH264dRegCtx)));
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    RK_U32 max_cnt = (p_hal->fast_mode != 0) ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;

    //!< malloc buffers
    reg_ctx->spspps = mpp_calloc(RK_U8, VDPU34X_SPSPPS_UNIT_SIZE);
    reg_ctx->rps = mpp_calloc(RK_U8, VDPU34X_RPS_SIZE);
    reg_ctx->sclst = mpp_calloc(RK_U8, VDPU34X_SCALING_LIST_SIZE);
    FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs,
                                   VDPU34X_INFO_BUFFER_SIZE(max_cnt)));
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);
    reg_ctx->offset_cabac = VDPU34X_CABAC_TAB_OFFSET;
    reg_ctx->offset_errinfo = VDPU34X_ERROR_INFO_OFFSET;
    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu34xH264dRegSet, 1);
        init_common_regs(reg_ctx->reg_buf[i].regs);
        reg_ctx->offset_spspps[i] = VDPU34X_SPSPPS_OFFSET(i);
        reg_ctx->offset_rps[i] = VDPU34X_RPS_OFFSET(i);
        reg_ctx->offset_sclst[i] = VDPU34X_SCALING_LIST_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->spspps_offset = reg_ctx->offset_spspps[0];
        reg_ctx->rps_offset = reg_ctx->offset_rps[0];
        reg_ctx->sclst_offset = reg_ctx->offset_sclst[0];
    }

    //!< copy cabac table bytes
    memcpy((char *)reg_ctx->bufs_ptr + reg_ctx->offset_cabac,
           (void *)rkv_cabac_table, sizeof(rkv_cabac_table));

    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_16);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, mpp_align_16);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);

    if (cfg->hal_fbc_adj_cfg) {
        cfg->hal_fbc_adj_cfg->func = vdpu34x_afbc_align_calc;
        cfg->hal_fbc_adj_cfg->expand = 16;
    }

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu34x_h264d_deinit(hal);

    return ret;
}

MPP_RET vdpu34x_h264d_deinit(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;

    RK_U32 i = 0;
    RK_U32 loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    mpp_buffer_put(reg_ctx->bufs);

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_ctx->reg_buf[i].regs);

    loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(reg_ctx->rcb_buf) : 1;
    for (i = 0; i < loop; i++) {
        if (reg_ctx->rcb_buf[i]) {
            mpp_buffer_put(reg_ctx->rcb_buf[i]);
            reg_ctx->rcb_buf[i] = NULL;
        }
    }

    if (p_hal->cmv_bufs) {
        hal_bufs_deinit(p_hal->cmv_bufs);
        p_hal->cmv_bufs = NULL;
    }

    MPP_FREE(reg_ctx->spspps);
    MPP_FREE(reg_ctx->rps);
    MPP_FREE(reg_ctx->sclst);

    MPP_FREE(p_hal->reg_ctx);

    return MPP_OK;
}

static void h264d_refine_rcb_size(H264dHalCtx_t *p_hal, VdpuRcbInfo *rcb_info,
                                  Vdpu34xH264dRegSet *regs,
                                  RK_S32 width, RK_S32 height)
{
    RK_U32 rcb_bits = 0;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;

    width = MPP_ALIGN(width, H264_CTU_SIZE);
    height = MPP_ALIGN(height, H264_CTU_SIZE);
    /* RCB_STRMD_ROW */
    if (width > 4096)
        rcb_bits = ((width + 15) / 16) * 154 * ((mbaff != 0) ? 2 : 1);
    else
        rcb_bits = 0;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_TRANSD_ROW */
    if (width > 8192)
        rcb_bits = ((width - 8192 + 3) / 4) * 2;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_TRANSD_COL */
    if (height > 8192)
        rcb_bits = ((height - 8192 + 3) / 4) * 2;
    else
        rcb_bits = 0;
    rcb_info[RCB_TRANSD_COL].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_ROW */
    rcb_bits = width * 42;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_INTER_COL */
    rcb_info[RCB_INTER_COL].size = 0;
    /* RCB_INTRA_ROW */
    rcb_bits = width * 44;
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_DBLK_ROW */
    rcb_bits = width * (2 + ((mbaff != 0) ? 12 : 6) * bit_depth);
    rcb_info[RCB_DBLK_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_SAO_ROW */
    rcb_info[RCB_SAO_ROW].size = 0;
    /* RCB_FBC_ROW */
    if (regs->common.reg012.fbc_e) {
        rcb_bits = (chroma_format_idc > 1) ? (2 * width * bit_depth) : 0;
    } else
        rcb_bits = 0;
    rcb_info[RCB_FBC_ROW].size = MPP_RCB_BYTES(rcb_bits);
    /* RCB_FILT_COL */
    rcb_info[RCB_FILT_COL].size = 0;
}

static void hal_h264d_rcb_info_update(void *hal, Vdpu34xH264dRegSet *regs)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t*)hal;
    RK_U32 mbaff = p_hal->pp->MbaffFrameFlag;
    RK_U32 bit_depth = p_hal->pp->bit_depth_luma_minus8 + 8;
    RK_U32 chroma_format_idc = p_hal->pp->chroma_format_idc;
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);

    if ( ctx->bit_depth != bit_depth ||
         ctx->chroma_format_idc != chroma_format_idc ||
         ctx->mbaff != mbaff ||
         ctx->width != width ||
         ctx->height != height) {
        RK_U32 i;
        RK_U32 loop = (p_hal->fast_mode != 0) ? MPP_ARRAY_ELEMS(ctx->reg_buf) : 1;

        ctx->rcb_buf_size = vdpu34x_get_rcb_buf_size(ctx->rcb_info, width, height);
        h264d_refine_rcb_size(hal, ctx->rcb_info, regs, width, height);
        for (i = 0; i < loop; i++) {
            MppBuffer rcb_buf = ctx->rcb_buf[i];

            if (rcb_buf) {
                mpp_buffer_put(rcb_buf);
                ctx->rcb_buf[i] = NULL;
            }
            mpp_buffer_get(p_hal->buf_group, &rcb_buf, ctx->rcb_buf_size);
            ctx->rcb_buf[i] = rcb_buf;
        }

        if (mbaff && 2 == chroma_format_idc)
            mpp_err("Warning: rkv do not support H.264 4:2:2 MBAFF decoding.");

        ctx->bit_depth      = bit_depth;
        ctx->width          = width;
        ctx->height         = height;
        ctx->mbaff          = mbaff;
        ctx->chroma_format_idc = chroma_format_idc;
    }
}

static MPP_RET vdpu34x_h264d_setup_colmv_buf(void *hal, RK_U32 width, RK_U32 height)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    RK_U32 ctu_size = 16, colmv_size = 4, colmv_byte = 16;
    RK_U32 colmv_compress = (p_hal->pp->frame_mbs_only_flag != 0) ? 1 : 0;
    RK_S32 mv_size;

    mv_size = vdpu34x_get_colmv_size(width, height, ctu_size, colmv_byte, colmv_size, colmv_compress);

    /* if is field mode is enabled enlarge colmv buffer and disable colmv compression */
    if (!p_hal->pp->frame_mbs_only_flag)
        mv_size *= 2;

    if (p_hal->cmv_bufs == NULL || p_hal->mv_size < mv_size) {
        size_t size = mv_size;

        if (p_hal->cmv_bufs) {
            hal_bufs_deinit(p_hal->cmv_bufs);
            p_hal->cmv_bufs = NULL;
        }

        hal_bufs_init(&p_hal->cmv_bufs);
        if (p_hal->cmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            return MPP_NOK;
        }
        p_hal->mv_size = mv_size;
        p_hal->mv_count = mpp_buf_slot_get_count(p_hal->frame_slots);
        hal_bufs_setup(p_hal->cmv_bufs, p_hal->mv_count, 1, &size);
    }

    return MPP_OK;
}

MPP_RET vdpu34x_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    RK_S32 width = MPP_ALIGN((p_hal->pp->wFrameWidthInMbsMinus1 + 1) << 4, 64);
    RK_S32 height = MPP_ALIGN((p_hal->pp->wFrameHeightInMbsMinus1 + 1) << 4, 64);
    Vdpu3xxH264dRegCtx *ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu34xH264dRegSet *regs = ctx->regs;
    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

    if (p_hal->fast_mode) {
        RK_U32 i = 0;
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                regs = ctx->reg_buf[i].regs;

                ctx->spspps_offset = ctx->offset_spspps[i];
                ctx->rps_offset = ctx->offset_rps[i];
                ctx->sclst_offset = ctx->offset_sclst[i];
                ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

    if (vdpu34x_h264d_setup_colmv_buf(hal, width, height))
        goto __RETURN;
    prepare_spspps(p_hal, (RK_U64 *)ctx->spspps, VDPU34X_SPSPPS_UNIT_SIZE / 8);
    prepare_framerps(p_hal, (RK_U64 *)ctx->rps, VDPU34X_RPS_SIZE / 8);
    prepare_scanlist(p_hal, ctx->sclst, VDPU34X_SCALING_LIST_SIZE);
    set_registers(p_hal, regs, task);

    //!< copy datas
    RK_U32 i = 0;
    if (!p_hal->fast_mode && !p_hal->pp->spspps_update) {
        RK_U32 offset = 0;
        RK_U32 len = VDPU34X_SPS_PPS_LEN; //!< sps+pps data length
        for (i = 0; i < 256; i++) {
            offset = ctx->spspps_offset + (VDPU34X_SPSPPS_UNIT_SIZE * i) + len;
            memcpy((char *)ctx->bufs_ptr + offset, (char *)ctx->spspps + len, VDPU34X_SPSPPS_UNIT_SIZE - len);
        }
    } else {
        RK_U32 offset = 0;
        for (i = 0; i < 256; i++) {
            offset = ctx->spspps_offset + (VDPU34X_SPSPPS_UNIT_SIZE * i);
            memcpy((char *)ctx->bufs_ptr + offset, (void *)ctx->spspps, VDPU34X_SPSPPS_UNIT_SIZE);
        }
    }

    regs->h264d_addr.pps_base = ctx->bufs_fd;
    mpp_dev_set_reg_offset(p_hal->dev, 161, ctx->spspps_offset);

    memcpy((char *)ctx->bufs_ptr + ctx->rps_offset, (void *)ctx->rps, VDPU34X_RPS_SIZE);
    regs->h264d_addr.rps_base = ctx->bufs_fd;
    mpp_dev_set_reg_offset(p_hal->dev, 163, ctx->rps_offset);

    regs->common.reg012.scanlist_addr_valid_en = 1;
    if (p_hal->pp->scaleing_list_enable_flag) {
        memcpy((char *)ctx->bufs_ptr + ctx->sclst_offset, (void *)ctx->sclst, VDPU34X_SCALING_LIST_SIZE);
        regs->h264d_addr.scanlist_addr = ctx->bufs_fd;
        mpp_dev_set_reg_offset(p_hal->dev, 180, ctx->sclst_offset);
    } else {
        regs->h264d_addr.scanlist_addr = 0;
    }

    hal_h264d_rcb_info_update(p_hal, regs);
    vdpu34x_setup_rcb(&regs->common_addr, p_hal->dev, (p_hal->fast_mode != 0) ?
                      ctx->rcb_buf[task->dec.reg_index] : ctx->rcb_buf[0],
                      ctx->rcb_info);
    vdpu34x_setup_statistic(&regs->common, &regs->statistic);
    mpp_buffer_sync_end(ctx->bufs);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu34x_h264d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    INP_CHECK(ret, NULL == p_hal);

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __RETURN;
    }

    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu34xH264dRegSet *regs = (p_hal->fast_mode != 0) ?
                               reg_ctx->reg_buf[task->dec.reg_index].regs :
                               reg_ctx->regs;
    MppDev dev = p_hal->dev;

    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->common;
        wr_cfg.size = sizeof(regs->common);
        wr_cfg.offset = VDPU34X_OFF_COMMON_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->h264d_param;
        wr_cfg.size = sizeof(regs->h264d_param);
        wr_cfg.offset = VDPU34X_OFF_CODEC_PARAMS_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = VDPU34X_OFF_COMMON_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->h264d_addr;
        wr_cfg.size = sizeof(regs->h264d_addr);
        wr_cfg.offset = VDPU34X_OFF_CODEC_ADDR_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        if (mpp_get_soc_type() == ROCKCHIP_SOC_RK3588) {
            wr_cfg.reg = &regs->h264d_highpoc;
            wr_cfg.size = sizeof(regs->h264d_highpoc);
            wr_cfg.offset = VDPU34X_OFF_POC_HIGHBIT_REGS;
            ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
            if (ret) {
                mpp_err_f("set register write failed %d\n", ret);
                break;
            }
        }

        wr_cfg.reg = &regs->statistic;
        wr_cfg.size = sizeof(regs->statistic);
        wr_cfg.offset = VDPU34X_OFF_STATISTIC_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->irq_status;
        rd_cfg.size = sizeof(regs->irq_status);
        rd_cfg.offset = VDPU34X_OFF_INTERRUPT_REGS;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu34x_set_rcbinfo(dev, reg_ctx->rcb_info);

        /* send request to hardware */
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu34x_h264d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu3xxH264dRegCtx *reg_ctx = (Vdpu3xxH264dRegCtx *)p_hal->reg_ctx;
    Vdpu34xH264dRegSet *p_regs = (p_hal->fast_mode != 0) ?
                                 reg_ctx->reg_buf[task->dec.reg_index].regs :
                                 reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        (task->dec.flags.ref_err && !p_hal->cfg->base.disable_error)) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);

__SKIP_HARD:
    if (p_hal->dec_cb) {
        DecCbHalDone param;

        param.task = (void *)&task->dec;
        param.regs = (RK_U32 *)p_regs;

        if (p_regs->irq_status.reg224.dec_error_sta ||
            (!p_regs->irq_status.reg224.dec_rdy_sta) ||
            p_regs->irq_status.reg224.buf_empty_sta ||
            p_regs->irq_status.reg226.strmd_error_status ||
            p_regs->irq_status.reg227.colmv_error_ref_picidx ||
            p_regs->irq_status.reg225.strmd_detect_error_flag)
            param.hard_err = 1;
        else
            param.hard_err = 0;

        mpp_callback(p_hal->dec_cb, &param);
    }
    memset(&p_regs->irq_status.reg224, 0, sizeof(RK_U32));
    if (p_hal->fast_mode) {
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;
    }

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu34x_h264d_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);
        RK_U32 imgwidth = mpp_frame_get_width((MppFrame)param);
        RK_U32 imgheight = mpp_frame_get_height((MppFrame)param);

        mpp_log("control info: fmt %d, w %d, h %d\n", fmt, imgwidth, imgheight);
        if (fmt == MPP_FMT_YUV422SP) {
            mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, mpp_align_wxh2yuv422);
        }
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu34x_afbc_align_calc(p_hal->frame_slots, (MppFrame)param, 16);
        } else if (imgwidth > 1920 || imgheight > 1088) {
            mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, mpp_align_256_odd);
        }
        break;
    }
    case MPP_DEC_SET_OUTPUT_FORMAT: {

    } break;
    default:
        break;
    }

__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_h264d_vdpu34x = {
    .name     = "h264d_vdpu34x",
    .type     = MPP_CTX_DEC,
    .coding   = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(Vdpu3xxH264dRegCtx),
    .flag     = 0,
    .init     = vdpu34x_h264d_init,
    .deinit   = vdpu3xx_h264d_deinit,
    .reg_gen  = vdpu34x_h264d_gen_regs,
    .start    = vdpu34x_h264d_start,
    .wait     = vdpu34x_h264d_wait,
    .reset    = vdpu_h264d_reset,
    .flush    = vdpu_h264d_flush,
    .control  = vdpu34x_h264d_control,
};
