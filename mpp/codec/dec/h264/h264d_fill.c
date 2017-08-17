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

#define MODULE_TAG "h264d_fill"

#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"

#include "h264d_fill.h"

static MPP_RET realloc_slice_list(H264dDxvaCtx_t *dxva_ctx)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    dxva_ctx->max_slice_size += ADD_SLICE_SIZE;
    dxva_ctx->slice_long = mpp_realloc(dxva_ctx->slice_long,
                                       DXVA_Slice_H264_Long,
                                       dxva_ctx->max_slice_size);
    MEM_CHECK(ret, dxva_ctx->slice_long);

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static MPP_RET fill_slice_stream(H264dDxvaCtx_t *dxva_ctx, H264_Nalu_t *p_nal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    DXVA_Slice_H264_Long  *p_long = NULL;

    if (dxva_ctx->slice_count >= dxva_ctx->max_slice_size) {
        FUN_CHECK(ret = realloc_slice_list(dxva_ctx));
    }
    p_long = &dxva_ctx->slice_long[dxva_ctx->slice_count];
    memset(p_long, 0, sizeof(DXVA_Slice_H264_Long));
    p_long->BSNALunitDataLocation  = dxva_ctx->strm_offset;
    p_long->wBadSliceChopping  = 0; //!< set to 0 in Rock-Chip RKVDEC IP

    (void)p_nal;
    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void fill_picture_entry(DXVA_PicEntry_H264 *pic, RK_U32 index, RK_U32 flag)
{
    ASSERT((index & 0x7f) == index && (flag & 0x01) == flag);
    pic->bPicEntry = index | (flag << 7);
}


/*!
***********************************************************************
* \brief
*    fill picture parameters
***********************************************************************
*/
//extern "C"
void fill_scanlist(H264dVideoCtx_t *p_Vid, DXVA_Qmatrix_H264 *qm)
{
    RK_S32 i = 0, j = 0;

    memset(qm, 0, sizeof(DXVA_Qmatrix_H264));
    for (i = 0; i < 6; ++i) { //!< 4x4, 6 lists
        for (j = 0; j < H264ScalingList4x4Length; j++) {
            qm->bScalingLists4x4[i][j] = p_Vid->qmatrix[i][j];
        }
    }
    for (i = 6; i < ((p_Vid->active_sps->chroma_format_idc != YUV444) ? 8 : 12); ++i) {
        for (j = 0; j < H264ScalingList8x8Length; j++) {
            qm->bScalingLists8x8[i - 6][j] = p_Vid->qmatrix[i][j];
        }
    }
}
/*!
***********************************************************************
* \brief
*    fill picture parameters
***********************************************************************
*/
//extern "C"
void fill_picparams(H264dVideoCtx_t *p_Vid, DXVA_PicParams_H264_MVC *pp)
{
    RK_U32 i = 0, j = 0;
    H264_StorePic_t *dec_pic = p_Vid->dec_pic;
    H264_DpbInfo_t *dpb_info = p_Vid->p_Dec->dpb_info;

    memset(pp, 0, sizeof(DXVA_PicParams_H264_MVC));
    //!< Configure current picture
    fill_picture_entry(&pp->CurrPic, dec_pic->mem_mark->slot_idx, dec_pic->structure == BOTTOM_FIELD);
    //!< Configure the set of references
    pp->UsedForReferenceFlags = 0;
    pp->NonExistingFrameFlags = 0;
    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if (dpb_info[i].refpic) {
            fill_picture_entry(&pp->RefFrameList[i], dpb_info[i].slot_index, dpb_info[i].is_long_term);
            pp->FieldOrderCntList[i][0] = dpb_info[i].TOP_POC;
            pp->FieldOrderCntList[i][1] = dpb_info[i].BOT_POC;
            pp->FrameNumList[i] = dpb_info[i].is_long_term ? dpb_info[i].long_term_frame_idx : dpb_info[i].frame_num;
            pp->LongTermPicNumList[i] = dpb_info[i].long_term_pic_num;
            if (dpb_info[i].is_used & 0x01) { //!< top_field
                pp->UsedForReferenceFlags |= 1 << (2 * i + 0);
            }
            if (dpb_info[i].is_used & 0x02) { //!< bot_field
                pp->UsedForReferenceFlags |= 1 << (2 * i + 1);
            }
        } else {
            pp->RefFrameList[i].bPicEntry = 0xff;
            pp->FieldOrderCntList[i][0] = 0;
            pp->FieldOrderCntList[i][1] = 0;
            pp->FrameNumList[i] = 0;
        }
    }
    pp->wFrameWidthInMbsMinus1  = p_Vid->active_sps->pic_width_in_mbs_minus1;
    pp->wFrameHeightInMbsMinus1 = p_Vid->active_sps->pic_height_in_map_units_minus1;
    pp->num_ref_frames = p_Vid->active_sps->max_num_ref_frames;

    pp->wBitFields = ((dec_pic->iCodingType == FIELD_CODING) << 0)  //!< field_pic_flag
                     | (dec_pic->mb_aff_frame_flag << 1) //!< MbaffFrameFlag
                     | (0 << 2)   //!< residual_colour_transform_flag
                     | (0 << 3)   //!< sp_for_switch_flag
                     | (p_Vid->active_sps->chroma_format_idc << 4)  //!< chroma_format_idc
                     | (dec_pic->used_for_reference << 6) //!< RefPicFlag
                     | (p_Vid->active_pps->constrained_intra_pred_flag << 7) //!< constrained_intra_pred_flag
                     | (p_Vid->active_pps->weighted_pred_flag << 8)  //!< weighted_pred_flag
                     | (p_Vid->active_pps->weighted_bipred_idc << 9)  //!< weighted_bipred_idc
                     | (1 << 11)   //!< MbsConsecutiveFlag
                     | (p_Vid->active_sps->frame_mbs_only_flag << 12) //!< frame_mbs_only_flag
                     | (p_Vid->active_pps->transform_8x8_mode_flag << 13) //!< transform_8x8_mode_flag
                     | ((p_Vid->active_sps->level_idc >= 31) << 14) //!< MinLumaBipredSize8x8Flag
                     | (1 << 15);  //!< IntraPicFlag (Modified if we detect a non-intra slice in dxva2_h264_decode_slice)

    pp->bit_depth_luma_minus8   = p_Vid->active_sps->bit_depth_luma_minus8;
    pp->bit_depth_chroma_minus8 = p_Vid->active_sps->bit_depth_chroma_minus8;
    pp->Reserved16Bits = 3;  //!< FIXME is there a way to detect the right mode

    pp->StatusReportFeedbackNumber = 1 /*+ ctx->report_id++*/;

    pp->CurrFieldOrderCnt[0] = 0;
    if (dec_pic->structure == TOP_FIELD || dec_pic->structure == FRAME) {
        pp->CurrFieldOrderCnt[0] = dec_pic->top_poc;
    }
    pp->CurrFieldOrderCnt[1] = 0;
    if (dec_pic->structure == BOTTOM_FIELD || dec_pic->structure == FRAME) {
        pp->CurrFieldOrderCnt[1] = dec_pic->bottom_poc;
    }
    pp->pic_init_qs_minus26 = p_Vid->active_pps->pic_init_qs_minus26;
    pp->chroma_qp_index_offset = p_Vid->active_pps->chroma_qp_index_offset;
    pp->second_chroma_qp_index_offset = p_Vid->active_pps->second_chroma_qp_index_offset;
    pp->ContinuationFlag = 1;
    pp->pic_init_qp_minus26 = p_Vid->active_pps->pic_init_qp_minus26;
    pp->num_ref_idx_l0_active_minus1 = p_Vid->active_pps->num_ref_idx_l0_default_active_minus1;
    pp->num_ref_idx_l1_active_minus1 = p_Vid->active_pps->num_ref_idx_l1_default_active_minus1;
    pp->Reserved8BitsA = 0;

    pp->frame_num = dec_pic->frame_num;
    pp->log2_max_frame_num_minus4 = p_Vid->active_sps->log2_max_frame_num_minus4;
    pp->pic_order_cnt_type = p_Vid->active_sps->pic_order_cnt_type;
    if (pp->pic_order_cnt_type == 0) {
        pp->log2_max_pic_order_cnt_lsb_minus4 = p_Vid->active_sps->log2_max_pic_order_cnt_lsb_minus4;
    } else if (pp->pic_order_cnt_type == 1) {
        pp->delta_pic_order_always_zero_flag = p_Vid->active_sps->delta_pic_order_always_zero_flag;
    }
    pp->direct_8x8_inference_flag = p_Vid->active_sps->direct_8x8_inference_flag;
    pp->entropy_coding_mode_flag = p_Vid->active_pps->entropy_coding_mode_flag;
    pp->pic_order_present_flag = p_Vid->active_pps->bottom_field_pic_order_in_frame_present_flag;
    pp->num_slice_groups_minus1 = p_Vid->active_pps->num_slice_groups_minus1;
    pp->slice_group_map_type = p_Vid->active_pps->slice_group_map_type;
    pp->deblocking_filter_control_present_flag = p_Vid->active_pps->deblocking_filter_control_present_flag;
    pp->redundant_pic_cnt_present_flag = p_Vid->active_pps->redundant_pic_cnt_present_flag;
    pp->Reserved8BitsB = 0;
    /* FMO is not implemented and is not implemented by FFmpeg neither */
    pp->slice_group_change_rate_minus1 = 0;

    //!< Following are H.264 MVC Specific parameters
    if (p_Vid->active_subsps) {
        RK_U16 num_views = 0;
        pp->num_views_minus1 = p_Vid->active_subsps->num_views_minus1;
        num_views = 1 + pp->num_views_minus1;
        ASSERT(num_views <= 16);

        for (i = 0; i < num_views; i++) {
            pp->view_id[i] = p_Vid->active_subsps->view_id[i];
            pp->num_anchor_refs_l0[i] = p_Vid->active_subsps->num_anchor_refs_l0[i];
            for (j = 0; j < pp->num_anchor_refs_l0[i]; j++) {
                pp->anchor_ref_l0[i][j] = p_Vid->active_subsps->anchor_ref_l0[i][j];
            }
            pp->num_anchor_refs_l1[i] = p_Vid->active_subsps->num_anchor_refs_l1[i];
            for (j = 0; j < pp->num_anchor_refs_l1[i]; j++) {
                pp->anchor_ref_l1[i][j] = p_Vid->active_subsps->anchor_ref_l1[i][j];
            }
            pp->num_non_anchor_refs_l0[i] = p_Vid->active_subsps->num_non_anchor_refs_l0[i];
            for (j = 0; j < pp->num_non_anchor_refs_l0[i]; j++) {
                pp->non_anchor_ref_l0[i][j] = p_Vid->active_subsps->non_anchor_ref_l0[i][j];
            }
            pp->num_non_anchor_refs_l1[i] = p_Vid->active_subsps->num_non_anchor_refs_l1[i];
            for (j = 0; j < pp->num_non_anchor_refs_l1[i]; j++) {
                pp->non_anchor_ref_l1[i][j] = p_Vid->active_subsps->non_anchor_ref_l1[i][j];
            }
        }

        for (i = num_views; i < 16; i++)
            pp->view_id[i] = 0xffff;
    }
    pp->curr_view_id = dec_pic->view_id;
    pp->anchor_pic_flag = dec_pic->anchor_pic_flag;
    pp->inter_view_flag = dec_pic->inter_view_flag;
    for (i = 0; i < 16; i++) {
        pp->ViewIDList[i] = dpb_info[i].view_id;
    }
    //!< add in Rock-chip RKVDEC IP
    pp->curr_layer_id = dec_pic->layer_id;
    pp->UsedForInTerviewflags = 0;
    for (i = 0; i < MPP_ARRAY_ELEMS(pp->RefFrameList); i++) {
        if (dpb_info[i].colmv_is_used) {
            pp->RefPicColmvUsedFlags |= 1 << i;
        }
        if (dpb_info[i].field_flag) {
            pp->RefPicFiledFlags |= 1 << i;
        }
        if (dpb_info[i].is_ilt_flag) {
            pp->UsedForInTerviewflags |= 1 << i;
        }
        pp->RefPicLayerIdList[i] = dpb_info[i].voidx;
    }
    if (p_Vid->active_pps->pic_scaling_matrix_present_flag
        || p_Vid->active_sps->seq_scaling_matrix_present_flag) {
        pp->scaleing_list_enable_flag = 1;
    } else {
        pp->scaleing_list_enable_flag = 0;
    }
}

/*!
***********************************************************************
* \brief
*    fill slice short struct
***********************************************************************
*/
//extern "C"
MPP_RET fill_slice_syntax(H264_SLICE_t *currSlice, H264dDxvaCtx_t *dxva_ctx)
{
    RK_U32 list = 0, i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    DXVA_Slice_H264_Long *p_long  = NULL;
    RK_S32 dpb_idx = 0, dpb_valid = 0, bottom_flag = 0;

    FUN_CHECK(ret = fill_slice_stream(dxva_ctx, &currSlice->p_Cur->nalu));
    p_long  = &dxva_ctx->slice_long[dxva_ctx->slice_count];
    //!< fill slice long contents
    p_long->first_mb_in_slice = currSlice->start_mb_nr;
    p_long->NumMbsForSlice = 0;       //!< XXX it is set once we have all slices
    p_long->slice_type = currSlice->slice_type;
    p_long->num_ref_idx_l0_active_minus1 = currSlice->active_pps->num_ref_idx_l0_default_active_minus1;
    p_long->num_ref_idx_l1_active_minus1 = currSlice->active_pps->num_ref_idx_l1_default_active_minus1;
    p_long->redundant_pic_cnt = currSlice->redundant_pic_cnt;
    p_long->direct_spatial_mv_pred_flag = currSlice->direct_spatial_mv_pred_flag;
    p_long->slice_id = dxva_ctx->slice_count;
    //!< add parameters
    p_long->active_sps_id = currSlice->active_sps->seq_parameter_set_id;
    p_long->active_pps_id = currSlice->active_pps->pic_parameter_set_id;
    p_long->idr_pic_id = currSlice->idr_pic_id;
    p_long->idr_flag = currSlice->idr_flag;
    p_long->drpm_used_bitlen = currSlice->drpm_used_bitlen;
    p_long->poc_used_bitlen = currSlice->poc_used_bitlen;
    p_long->nal_ref_idc = currSlice->nal_reference_idc;
    p_long->profileIdc = currSlice->active_sps->profile_idc;




    for (i = 0; i < MPP_ARRAY_ELEMS(p_long->RefPicList[0]); i++) {
        dpb_idx = currSlice->p_Dec->refpic_info_p[i].dpb_idx;
        dpb_valid = currSlice->p_Dec->refpic_info_p[i].valid;
        //dpb_valid = (currSlice->p_Dec->dpb_info[dpb_idx].refpic ? 1 : 0);
        if (dpb_valid) {
            bottom_flag = currSlice->p_Dec->refpic_info_p[i].bottom_flag;
            fill_picture_entry(&p_long->RefPicList[0][i], dpb_idx, bottom_flag);
        } else {
            p_long->RefPicList[0][i].bPicEntry = 0xff;
        }
    }

    for (list = 0; list < 2; list++) {
        for (i = 0; i < MPP_ARRAY_ELEMS(p_long->RefPicList[list + 1]); i++) {
            dpb_idx = currSlice->p_Dec->refpic_info_b[list][i].dpb_idx;
            dpb_valid = currSlice->p_Dec->refpic_info_b[list][i].valid;
            //dpb_valid = (currSlice->p_Dec->dpb_info[dpb_idx].refpic ? 1 : 0);
            if (dpb_valid) {
                bottom_flag = currSlice->p_Dec->refpic_info_b[list][i].bottom_flag;
                fill_picture_entry(&p_long->RefPicList[list + 1][i], dpb_idx, bottom_flag);
            } else {
                p_long->RefPicList[list + 1][i].bPicEntry = 0xff;
            }
        }
    }
    dxva_ctx->slice_count++;

    return ret = MPP_OK;
__FAILED:
    return ret;
}


/*!
***********************************************************************
* \brief
*    check parser is end and then configure register
***********************************************************************
*/
//extern "C"
void commit_buffer(H264dDxvaCtx_t *dxva_ctx)
{
    H264dSyntax_t *p_syn = &dxva_ctx->syn;
    DXVA2_DecodeBufferDesc *p_dec = NULL;

    p_syn->num = 0;
    //!< commit picture paramters
    p_dec = &p_syn->buf[p_syn->num++];
    memset(p_dec, 0, sizeof(DXVA2_DecodeBufferDesc));
    p_dec->CompressedBufferType = DXVA2_PictureParametersBufferType;
    p_dec->pvPVPState = (void *)&dxva_ctx->pp;
    p_dec->DataSize = sizeof(DXVA_PicParams_H264_MVC);
    p_dec->NumMBsInBuffer = 0;
    //!< commit scanlist Qmatrix
    p_dec = &p_syn->buf[p_syn->num++];
    memset(p_dec, 0, sizeof(DXVA2_DecodeBufferDesc));
    p_dec->CompressedBufferType = DXVA2_InverseQuantizationMatrixBufferType;
    p_dec->pvPVPState = (void *)&dxva_ctx->qm;
    p_dec->DataSize = sizeof(DXVA_Qmatrix_H264);
    p_dec->NumMBsInBuffer = 0;
    //!< commit bitstream
    p_dec = &p_syn->buf[p_syn->num++];
    memset(p_dec, 0, sizeof(DXVA2_DecodeBufferDesc));
    p_dec->CompressedBufferType = DXVA2_BitStreamDateBufferType;
    p_dec->DataSize = MPP_ALIGN(dxva_ctx->strm_offset, 16);
    memset(dxva_ctx->bitstream + dxva_ctx->strm_offset, 0, p_dec->DataSize - dxva_ctx->strm_offset);
    p_dec->pvPVPState = (void *)dxva_ctx->bitstream;
    //!< commit slice control, DXVA_Slice_H264_Long
    p_dec = &p_syn->buf[p_syn->num++];
    memset(p_dec, 0, sizeof(DXVA2_DecodeBufferDesc));
    p_dec->CompressedBufferType = DXVA2_SliceControlBufferType;
    p_dec->NumMBsInBuffer = (dxva_ctx->pp.wFrameHeightInMbsMinus1 + 1)
                            * (dxva_ctx->pp.wFrameWidthInMbsMinus1 + 1);
    p_dec->pvPVPState = dxva_ctx->slice_long;
    p_dec->DataSize   = dxva_ctx->slice_count * sizeof(DXVA_Slice_H264_Long);

    //!< reset dxva parameters
    dxva_ctx->slice_count = 0;
    dxva_ctx->strm_offset = 0;
}
