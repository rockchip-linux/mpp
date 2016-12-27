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

#define MODULE_TAG "h264d_pps"

#include <string.h>

#include "mpp_err.h"

#include "h264d_pps.h"
#include "h264d_scalist.h"
#include "h264d_dpb.h"

static void reset_curpps_data(H264_PPS_t *cur_pps)
{
    memset(cur_pps, 0, sizeof(H264_PPS_t));
    cur_pps->seq_parameter_set_id = 0;  // reset
    cur_pps->pic_parameter_set_id = 0;
}

static MPP_RET parse_pps_calingLists(BitReadCtx_t *p_bitctx, H264_SPS_t *sps, H264_PPS_t *pps)
{
    RK_S32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    for (i = 0; i < 6; ++i) {
        READ_ONEBIT(p_bitctx, &pps->pic_scaling_list_present_flag[i]);

        if (pps->pic_scaling_list_present_flag[i]) {
            FUN_CHECK (ret = parse_scalingList(p_bitctx, H264ScalingList4x4Length,
                                               pps->ScalingList4x4[i], &pps->UseDefaultScalingMatrix4x4Flag[i]));
        }
    }
    if (pps->transform_8x8_mode_flag) {
        for (i = 0; i < ((sps->chroma_format_idc != 3) ? 2 : 6); ++i) {
            READ_ONEBIT(p_bitctx, &pps->pic_scaling_list_present_flag[i + 6]);
            if (pps->pic_scaling_list_present_flag[i + 6]) {
                FUN_CHECK(ret = parse_scalingList(p_bitctx, H264ScalingList8x8Length,
                                                  pps->ScalingList8x8[i], &pps->UseDefaultScalingMatrix8x8Flag[i]));
            }
        }
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET parser_pps(BitReadCtx_t *p_bitctx, H264_SPS_t *cur_sps, H264_PPS_t *cur_pps)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    READ_UE(p_bitctx, &cur_pps->pic_parameter_set_id);
    READ_UE(p_bitctx, &cur_pps->seq_parameter_set_id);
    //VAL_CHECK(ret, cur_pps->seq_parameter_set_id < 32);
    if (cur_pps->seq_parameter_set_id < 0 || cur_pps->seq_parameter_set_id > 32) {
        cur_pps->seq_parameter_set_id = 0;
    }
    if (cur_pps->pic_parameter_set_id < 0 || cur_pps->pic_parameter_set_id > 256)    {
        cur_pps->pic_parameter_set_id = 0;
    }
    READ_ONEBIT(p_bitctx, &cur_pps->entropy_coding_mode_flag);
    READ_ONEBIT(p_bitctx, &cur_pps->bottom_field_pic_order_in_frame_present_flag);

    READ_UE(p_bitctx, &cur_pps->num_slice_groups_minus1);
    VAL_CHECK(ret, cur_pps->num_slice_groups_minus1 <= 1);
    READ_UE(p_bitctx, &cur_pps->num_ref_idx_l0_default_active_minus1);
    VAL_CHECK(ret, cur_pps->num_ref_idx_l0_default_active_minus1 < 32);
    READ_UE(p_bitctx, &cur_pps->num_ref_idx_l1_default_active_minus1);
    VAL_CHECK(ret, cur_pps->num_ref_idx_l1_default_active_minus1 < 32);
    READ_ONEBIT(p_bitctx, &cur_pps->weighted_pred_flag);
    READ_BITS(p_bitctx, 2, &cur_pps->weighted_bipred_idc);
    VAL_CHECK(ret, cur_pps->weighted_bipred_idc < 3);
    READ_SE(p_bitctx, &cur_pps->pic_init_qp_minus26);
    READ_SE(p_bitctx, &cur_pps->pic_init_qs_minus26);
    READ_SE(p_bitctx, &cur_pps->chroma_qp_index_offset);
    cur_pps->second_chroma_qp_index_offset = cur_pps->chroma_qp_index_offset;
    READ_ONEBIT(p_bitctx, &cur_pps->deblocking_filter_control_present_flag);
    READ_ONEBIT(p_bitctx, &cur_pps->constrained_intra_pred_flag);
    READ_ONEBIT(p_bitctx, &cur_pps->redundant_pic_cnt_present_flag);
    VAL_CHECK(ret , cur_pps->redundant_pic_cnt_present_flag == 0);

    if (mpp_has_more_rbsp_data(p_bitctx)) {
        READ_ONEBIT(p_bitctx, &cur_pps->transform_8x8_mode_flag);
        READ_ONEBIT(p_bitctx, &cur_pps->pic_scaling_matrix_present_flag);
        if (cur_pps->pic_scaling_matrix_present_flag) {
            H264D_WARNNING("Picture scaling matrix present.");
            FUN_CHECK(ret = parse_pps_calingLists(p_bitctx, cur_sps, cur_pps));
        }
        READ_SE(p_bitctx, &cur_pps->second_chroma_qp_index_offset);
    } else {
        cur_pps->transform_8x8_mode_flag = 0;
        cur_pps->second_chroma_qp_index_offset = cur_pps->chroma_qp_index_offset;
    }
    cur_pps->Valid = 1;

    return ret = MPP_OK;

__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:

    return ret;
}


/*!
***********************************************************************
* \brief
*    parser pps and process pps
***********************************************************************
*/
//extern "C"
MPP_RET process_pps(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dCurCtx_t *p_Cur = currSlice->p_Cur;
    BitReadCtx_t *p_bitctx = &p_Cur->bitctx;
    H264_PPS_t *cur_pps = &p_Cur->pps;

    reset_curpps_data(cur_pps);// reset

    FUN_CHECK(ret = parser_pps(p_bitctx, &p_Cur->sps, cur_pps));
    //!< MakePPSavailable
    ASSERT(cur_pps->Valid == 1);
    memcpy(&currSlice->p_Vid->ppsSet[cur_pps->pic_parameter_set_id], cur_pps, sizeof(H264_PPS_t));

    return ret = MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    prase sps and process sps
***********************************************************************
*/
//extern "C"
MPP_RET activate_pps(H264dVideoCtx_t *p_Vid, H264_PPS_t *pps)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, !p_Vid && !pps);
    if (p_Vid->active_pps != pps) {
        if (p_Vid->dec_pic) {
            //!< return if the last picture has already been finished
            FUN_CHECK(ret = exit_picture(p_Vid, &p_Vid->dec_pic));
        }
        p_Vid->active_pps = pps;
    }
__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}
