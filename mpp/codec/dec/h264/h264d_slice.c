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

#define MODULE_TAG "h264d_slice"

#include "mpp_mem.h"

#include "h264d_log.h"
#include "h264d_slice.h"
#include "h264d_sps.h"
#include "h264d_pps.h"

static void free_slice_drpm_buffer(H264_SLICE_t *currSlice)  // dec_ref_pic_marking
{
    H264_DRPM_t *tmp_drpm = NULL;

    while (currSlice->dec_ref_pic_marking_buffer) {
        tmp_drpm = currSlice->dec_ref_pic_marking_buffer;
        currSlice->dec_ref_pic_marking_buffer = tmp_drpm->Next;
        MPP_FREE(tmp_drpm);
    }
}

static MPP_RET ref_pic_list_mvc_modification(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 modification_of_pic_nums_idc = 0;
    BitReadCtx_t *p_bitctx = &currSlice->p_Cur->bitctx;

    alloc_ref_pic_list_reordering_buffer(currSlice);
    if ((currSlice->slice_type % 5) != I_SLICE && (currSlice->slice_type % 5) != SI_SLICE) {
        READ_ONEBIT(p_bitctx, &currSlice->ref_pic_list_reordering_flag[LIST_0], "ref_pic_list_reordering_flag");
        if (currSlice->ref_pic_list_reordering_flag[LIST_0]) {
            i = 0;
            do {
                READ_UE(p_bitctx, &modification_of_pic_nums_idc, "modification_of_pic_nums_idc");
                currSlice->modification_of_pic_nums_idc[LIST_0][i] = modification_of_pic_nums_idc;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    READ_UE(p_bitctx, &currSlice->abs_diff_pic_num_minus1[LIST_0][i], "abs_diff_pic_num_minus1_lx");
                } else {
                    if (modification_of_pic_nums_idc == 2) {
                        READ_UE(p_bitctx, &currSlice->long_term_pic_idx[LIST_0][i], "long_term_pic_idx");
                    } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                        READ_UE(p_bitctx, &currSlice->abs_diff_view_idx_minus1[LIST_0][i], "abs_diff_view_idx_minus1");
                    }
                }
                i++;
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    if (currSlice->slice_type % 5 == B_SLICE) {
        READ_ONEBIT(p_bitctx, &currSlice->ref_pic_list_reordering_flag[LIST_1], "ref_pic_list_reordering_flag");
        if (currSlice->ref_pic_list_reordering_flag[LIST_1]) {
            i = 0;
            do {
                READ_UE(p_bitctx, &modification_of_pic_nums_idc, "modification_of_pic_nums_idc");
                currSlice->modification_of_pic_nums_idc[LIST_1][i] = modification_of_pic_nums_idc;
                if (modification_of_pic_nums_idc == 0 || modification_of_pic_nums_idc == 1) {
                    READ_UE(p_bitctx, &currSlice->abs_diff_pic_num_minus1[LIST_1][i], "abs_diff_pic_num_minus1_lx");
                } else {
                    if (modification_of_pic_nums_idc == 2) {
                        READ_UE(p_bitctx, &currSlice->long_term_pic_idx[LIST_1][i], "long_term_pic_idx");
                    } else if (modification_of_pic_nums_idc == 4 || modification_of_pic_nums_idc == 5) {
                        READ_UE(p_bitctx, &currSlice->abs_diff_view_idx_minus1[LIST_1][i], "abs_diff_view_idx_minus1");
                    }
                }
                i++;
            } while (modification_of_pic_nums_idc != 3);
        }
    }
    ASSERT(currSlice->redundant_pic_cnt == 0);  //!< not support reference index of redundant slices

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;
}

static MPP_RET pred_weight_table(H264_SLICE_t *currSlice)
{
    RK_S32 se_tmp = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 i = 0, j = 0, temp = 0;
    BitReadCtx_t *p_bitctx = &currSlice->p_Cur->bitctx;

    READ_UE(p_bitctx, &temp, "log2_weight_denom");
    if (currSlice->active_sps->chroma_format_idc) {
        READ_UE(p_bitctx, &temp, "log2_weight_denom");
    }
    for (i = 0; i < currSlice->num_ref_idx_active[LIST_0]; i++) {
        READ_ONEBIT(p_bitctx, &temp, "luma_weight_flag_l0");
        if (temp) {
            READ_SE(p_bitctx, &se_tmp, "pred_weight"); //!< slice->wp_weight[LIST_0][i][0]
            READ_SE(p_bitctx, &se_tmp, "pred_offset"); //!< slice->wp_offset[LIST_0][i][0]
        }
        if (currSlice->active_sps->chroma_format_idc) {
            READ_ONEBIT(p_bitctx, &temp, "chroma_weight_flag_l0");
            for (j = 1; j < 3; j++) {
                if (temp) { //!< chroma_weight_flag_l0
                    READ_SE(p_bitctx, &se_tmp, "pred_weight"); //!< slice->wp_weight[LIST_0][i][j]
                    READ_SE(p_bitctx, &se_tmp, "pred_offset"); //!< slice->wp_offset[LIST_0][i][j]
                }
            }
        }
    }

    if ((currSlice->slice_type == B_SLICE) && currSlice->p_Vid->active_pps->weighted_bipred_idc == 1) {
        for (i = 0; i < currSlice->num_ref_idx_active[LIST_1]; i++) {
            READ_ONEBIT(p_bitctx, &temp, "luma_weight_flag_l1");
            if (temp) {
                READ_SE(p_bitctx, &se_tmp, "pred_weight"); //!< slice->wp_weight[LIST_1][i][0]
                READ_SE(p_bitctx, &se_tmp, "pred_offset"); //!< slice->wp_offset[LIST_1][i][0]
            }
            if (currSlice->active_sps->chroma_format_idc) {
                READ_ONEBIT(p_bitctx, &temp, "chroma_weight_flag_l1");
                for (j = 1; j < 3; j++) {
                    if (temp) { // chroma_weight_flag_l1
                        READ_SE(p_bitctx, &se_tmp, "pred_weight"); //!< slice->wp_weight[LIST_1][i][j]
                        READ_SE(p_bitctx, &se_tmp, "pred_offset"); //!< slice->wp_offset[LIST_1][i][j]
                    }
                }
            }
        }
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    return ret = p_bitctx->ret;
}

static MPP_RET dec_ref_pic_marking(H264_SLICE_t *pSlice)
{
    RK_U32 val = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 drpm_used_bits = 0;
    H264_DRPM_t *tmp_drpm = NULL, *tmp_drpm2 = NULL;
    H264dVideoCtx_t *p_Vid = pSlice->p_Vid;
    BitReadCtx_t *p_bitctx = &pSlice->p_Cur->bitctx;

    drpm_used_bits = p_bitctx->used_bits;
    pSlice->drpm_used_bitlen = 0;
    //!< free old buffer content
    free_slice_drpm_buffer(pSlice);
    if (pSlice->idr_flag ||
        (pSlice->svc_extension_flag == 0 && pSlice->mvcExt.non_idr_flag == 0)) {
        READ_ONEBIT(p_bitctx, &pSlice->no_output_of_prior_pics_flag, "no_output_of_prior_pics_flag");
        p_Vid->no_output_of_prior_pics_flag = pSlice->no_output_of_prior_pics_flag;
        READ_ONEBIT(p_bitctx, &pSlice->long_term_reference_flag, "long_term_reference_flag");
    } else {
        READ_ONEBIT(p_bitctx, &pSlice->adaptive_ref_pic_buffering_flag, "adaptive_ref_pic_buffering_flag");

        if (pSlice->adaptive_ref_pic_buffering_flag) {
            do { //!< read Memory Management Control Operation
                tmp_drpm = mpp_calloc(H264_DRPM_t, 1);
                MEM_CHECK(ret, tmp_drpm);
                tmp_drpm->Next = NULL;
                READ_UE(p_bitctx, &val, "memory_management_control_operation");
                tmp_drpm->memory_management_control_operation = val;

                if ((val == 1) || (val == 3)) {
                    READ_UE(p_bitctx, &tmp_drpm->difference_of_pic_nums_minus1, "difference_of_pic_nums_minus1");
                }
                if (val == 2) {
                    READ_UE(p_bitctx, &tmp_drpm->long_term_pic_num, "long_term_pic_num");
                }
                if ((val == 3) || (val == 6)) {
                    READ_UE(p_bitctx, &tmp_drpm->long_term_frame_idx, "long_term_frame_idx");
                }
                if (val == 4) {
                    READ_UE(p_bitctx, &tmp_drpm->max_long_term_frame_idx_plus1, "max_long_term_frame_idx_plus1");
                }
                // add command
                if (pSlice->dec_ref_pic_marking_buffer == NULL) {
                    pSlice->dec_ref_pic_marking_buffer = tmp_drpm;
                } else {
                    tmp_drpm2 = pSlice->dec_ref_pic_marking_buffer;
                    while (tmp_drpm2->Next != NULL) {
                        tmp_drpm2 = tmp_drpm2->Next;
                    }
                    tmp_drpm2->Next = tmp_drpm;
                }

            } while (val != 0);
        }
    }
    pSlice->drpm_used_bitlen = p_bitctx->used_bits - drpm_used_bits;
    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:

    return ret;
}

static void init_slice_parmeters(H264_SLICE_t *currSlice)
{
    H264dVideoCtx_t *p_Vid   = currSlice->p_Vid;
    H264_Nalu_t    *cur_nalu = &currSlice->p_Cur->nalu;

    //--- init slice syntax
    currSlice->idr_flag = ((cur_nalu->nalu_type == NALU_TYPE_IDR)
                           || (currSlice->mvcExt.valid && !currSlice->mvcExt.non_idr_flag));
    currSlice->nal_reference_idc = cur_nalu->nal_reference_idc;
    if ((!currSlice->svc_extension_flag) || currSlice->mvcExt.iPrefixNALU) { // MVC or have prefixNALU
        currSlice->view_id = currSlice->mvcExt.view_id;
        currSlice->inter_view_flag = currSlice->mvcExt.inter_view_flag;
        currSlice->anchor_pic_flag = currSlice->mvcExt.anchor_pic_flag;
    } else if (currSlice->svc_extension_flag == -1) { // normal AVC
        currSlice->view_id = currSlice->mvcExt.valid ? p_Vid->active_subsps->view_id[0] : 0;
        currSlice->inter_view_flag = 1;
        currSlice->anchor_pic_flag = currSlice->idr_flag;
    }
    currSlice->layer_id = currSlice->view_id;
    if (currSlice->layer_id >= 0) { // if not found, layer_id == -1
        currSlice->p_Dpb = p_Vid->p_Dpb_layer[currSlice->layer_id];
    }
}

static MPP_RET check_sps_pps(H264_SPS_t *sps, H264_subSPS_t *subset_sps, H264_PPS_t *pps)
{
    RK_U32 ret = 0;

    ret |= (sps->seq_parameter_set_id > 63);
    ret |= (sps->separate_colour_plane_flag == 1);
    ret |= (sps->chroma_format_idc == 3);
    ret |= (sps->bit_depth_luma_minus8 > 2);
    ret |= (sps->bit_depth_chroma_minus8 > 2);
    ret |= (sps->log2_max_frame_num_minus4 > 12);
    ret |= (sps->pic_order_cnt_type > 2);
    ret |= (sps->log2_max_pic_order_cnt_lsb_minus4 > 12);
    ret |= (sps->num_ref_frames_in_pic_order_cnt_cycle > 255);
    ret |= (sps->max_num_ref_frames > 16);
    ret |= (sps->pic_width_in_mbs_minus1 > 4095);
    ret |= (sps->pic_height_in_map_units_minus1 > 2303);
    ret |= (sps->pic_height_in_map_units_minus1 > 2303);
    if (ret) {
        H264D_ERR("sps has error, sps_id=%d", sps->seq_parameter_set_id);
        goto __FAILED;
    }
    if (subset_sps) { //!< MVC
        ret |= (subset_sps->num_views_minus1 != 1);
        //        ret |= (subset_sps->num_anchor_refs_l0[0] != 1);
        if (subset_sps->num_anchor_refs_l0[0] > 0)
            ret |= (subset_sps->anchor_ref_l0[0][0] != subset_sps->view_id[0]);
        //    ret |= (subset_sps->num_anchor_refs_l1[0] != 1);
        if (subset_sps->num_anchor_refs_l1[0] > 0)
            ret |= (subset_sps->anchor_ref_l1[0][0] != subset_sps->view_id[1]);

        //    ret |= (subset_sps->num_non_anchor_refs_l0[0] != 1);
        if (subset_sps->num_non_anchor_refs_l0[0] > 0)
            ret |= (subset_sps->non_anchor_ref_l0[0][0] != subset_sps->view_id[0]);
        //  ret |= (subset_sps->num_non_anchor_refs_l1[0] != 1);
        if (subset_sps->num_non_anchor_refs_l1[0] > 0)
            ret |= (subset_sps->non_anchor_ref_l1[0][0] != subset_sps->view_id[1]);

        ret |= (subset_sps->num_level_values_signalled_minus1 != 0);
        //ret |= (subset_sps->num_applicable_ops_minus1[0] > 1);
        ret |= (subset_sps->applicable_op_num_target_views_minus1[0][0] > 1);
        ret |= (subset_sps->applicable_op_num_views_minus1[0][0] > 1);
        if (ret) {
            H264D_ERR("subsps has error, sps_id=%d", sps->seq_parameter_set_id);
            goto __FAILED;
        }
    }
    //!< check PPS
    ret |= (pps->pic_parameter_set_id > 255);
    ret |= (pps->seq_parameter_set_id > 63);
    ret |= (pps->num_slice_groups_minus1 > 0);
    ret |= (pps->num_ref_idx_l0_default_active_minus1 > 31);
    ret |= (pps->num_ref_idx_l1_default_active_minus1 > 31);
    ret |= (pps->pic_init_qp_minus26 > 25 || pps->pic_init_qp_minus26 < -(26 + 6 * (RK_S32)sps->bit_depth_luma_minus8));
    ret |= (pps->pic_init_qs_minus26 > 25 || pps->pic_init_qs_minus26 < -26);
    ret |= (pps->chroma_qp_index_offset > 12 || pps->chroma_qp_index_offset < -12);
    ret |= (pps->redundant_pic_cnt_present_flag == 1);
    if (ret) {
        H264D_ERR("pps has error, sps_id=%d, pps_id", sps->seq_parameter_set_id, pps->pic_parameter_set_id);
        goto __FAILED;
    }
    return MPP_OK;
__FAILED:
    return MPP_NOK;
}


static MPP_RET set_slice_user_parmeters(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_PPS_t *cur_pps = NULL;
    H264_SPS_t *cur_sps = NULL;
    H264_subSPS_t *cur_subsps = NULL;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    //!< use parameter set
    cur_pps = &p_Vid->ppsSet[currSlice->pic_parameter_set_id];
    cur_pps = (cur_pps && cur_pps->Valid) ? cur_pps : NULL;
    VAL_CHECK(ret, cur_pps != NULL);

    if (currSlice->mvcExt.valid) {
        cur_sps = &p_Vid->subspsSet[cur_pps->seq_parameter_set_id].sps;
        cur_subsps = &p_Vid->subspsSet[cur_pps->seq_parameter_set_id];
        if (cur_subsps->Valid) {
            if ((RK_S32)currSlice->mvcExt.view_id == cur_subsps->view_id[0]) { // combine subsps to sps
                p_Vid->active_mvc_sps_flag = 0;
                cur_subsps = NULL;
                cur_sps = &p_Vid->spsSet[cur_pps->seq_parameter_set_id];
            } else if ((RK_S32)currSlice->mvcExt.view_id == cur_subsps->view_id[1]) {
                p_Vid->active_mvc_sps_flag = 1;
            }
        } else {
            p_Vid->active_mvc_sps_flag = 0;
            cur_sps = &p_Vid->spsSet[cur_pps->seq_parameter_set_id];
            cur_subsps = NULL;
        }
    } else {
        p_Vid->active_mvc_sps_flag = 0;
        cur_sps = &p_Vid->spsSet[cur_pps->seq_parameter_set_id];
        cur_subsps = NULL;
    }

    if (p_Vid->active_mvc_sps_flag) { // layer_id == 1
        cur_subsps = (cur_subsps && cur_subsps->Valid) ? cur_subsps : NULL;
        VAL_CHECK(ret, cur_subsps != NULL);
        cur_sps = &cur_subsps->sps;
    } else { //!< layer_id == 0
        cur_subsps = NULL;
        cur_sps = (cur_sps && cur_sps->Valid) ? cur_sps : NULL;
        VAL_CHECK(ret, cur_sps != NULL);
    }
    VAL_CHECK(ret, check_sps_pps(cur_sps, cur_subsps, cur_pps) != MPP_NOK);

    H264D_DBG(H264D_DBG_PPS_SPS, "[SLICE_HEAD] layer_id=%d,sps_id=%d, pps_id=%d", currSlice->layer_id,
              cur_sps->seq_parameter_set_id, cur_pps->pic_parameter_set_id);

    FUN_CHECK(ret = activate_sps(p_Vid, cur_sps, cur_subsps));
    FUN_CHECK(ret = activate_pps(p_Vid, cur_pps));

    //!< Set SPS to the subset SPS parameters
    if (currSlice->svc_extension_flag == 0) {
        p_Vid->active_subsps = &p_Vid->subspsSet[cur_pps->seq_parameter_set_id];
    }
    currSlice->active_sps = p_Vid->active_sps;
    currSlice->active_pps = p_Vid->active_pps;

    p_Vid->type = currSlice->slice_type;
    return MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    parse SEI information
***********************************************************************
*/
//extern "C"
void recycle_slice(H264_SLICE_t *currSlice)
{
    if (currSlice) {
        free_slice_drpm_buffer(currSlice);
    }
}
void free_ref_pic_list_reordering_buffer(H264_SLICE_t *currSlice)
{
    if (currSlice) {
        MPP_FREE(currSlice->modification_of_pic_nums_idc[LIST_0]);
        MPP_FREE(currSlice->abs_diff_pic_num_minus1[LIST_0]);
        MPP_FREE(currSlice->long_term_pic_idx[LIST_0]);
        MPP_FREE(currSlice->modification_of_pic_nums_idc[LIST_1]);
        MPP_FREE(currSlice->abs_diff_pic_num_minus1[LIST_1]);
        MPP_FREE(currSlice->long_term_pic_idx[LIST_1]);
        MPP_FREE(currSlice->abs_diff_view_idx_minus1[LIST_0]);
        MPP_FREE(currSlice->abs_diff_view_idx_minus1[LIST_1]);
    }
}
MPP_RET alloc_ref_pic_list_reordering_buffer(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    currSlice->modification_of_pic_nums_idc[LIST_0] = NULL;
    currSlice->abs_diff_pic_num_minus1[LIST_0]      = NULL;
    currSlice->long_term_pic_idx[LIST_0]            = NULL;
    currSlice->abs_diff_view_idx_minus1[LIST_0]     = NULL;
    if (currSlice->slice_type != I_SLICE && currSlice->slice_type != SI_SLICE)  {
        RK_U32 size = currSlice->num_ref_idx_active[LIST_0] + 1;
        MEM_CHECK(ret, currSlice->modification_of_pic_nums_idc[LIST_0] = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->abs_diff_pic_num_minus1[LIST_0]      = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->long_term_pic_idx[LIST_0]            = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->abs_diff_view_idx_minus1[LIST_0]     = mpp_calloc(RK_U32, size));
    }
    currSlice->modification_of_pic_nums_idc[LIST_1] = NULL;
    currSlice->abs_diff_pic_num_minus1[LIST_1]      = NULL;
    currSlice->long_term_pic_idx[LIST_1]            = NULL;
    currSlice->abs_diff_view_idx_minus1[LIST_1]     = NULL;
    if (currSlice->slice_type == B_SLICE)  {
        RK_U32 size = currSlice->num_ref_idx_active[LIST_1] + 1;
        MEM_CHECK(ret, currSlice->modification_of_pic_nums_idc[LIST_1] = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->abs_diff_pic_num_minus1[LIST_1]      = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->long_term_pic_idx[LIST_1]            = mpp_calloc(RK_U32, size));
        MEM_CHECK(ret, currSlice->abs_diff_view_idx_minus1[LIST_1]     = mpp_calloc(RK_U32, size));
    }
    return ret = MPP_OK;
__FAILED:
    free_ref_pic_list_reordering_buffer(currSlice);
    return ret;
}
/*!
***********************************************************************
* \brief
*    parse SEI information
***********************************************************************
*/
//extern "C"
MPP_RET process_slice(H264_SLICE_t *currSlice)
{
    RK_U32 temp = 0;
    RK_U32 poc_used_bits = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dLogCtx_t *logctx = currSlice->logctx;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264dCurCtx_t *p_Cur = currSlice->p_Cur;
    BitReadCtx_t *p_bitctx = &p_Cur->bitctx;

    FunctionIn(logctx->parr[RUN_PARSE]);
    //!< initial value
    currSlice->p_Dpb_layer[0] = p_Vid->p_Dpb_layer[0];
    currSlice->p_Dpb_layer[1] = p_Vid->p_Dpb_layer[1];
    set_bitread_logctx(p_bitctx, logctx->parr[LOG_READ_SLICE]);

    LogInfo(p_bitctx->ctx, "----------------------------- SLICE begin --------------------------------");
    //!< read slice head syntax
    READ_UE(p_bitctx, &currSlice->start_mb_nr, "first_mb_in_slice");
    //FPRINT(g_debug_file1, "first_mb_in_slice = %d \n", currSlice->start_mb_nr);
    if (currSlice->start_mb_nr == 0/*is_new_picture(currSlice)*/) {
        READ_UE(p_bitctx, &temp, "slice_type");
        p_Vid->slice_type = currSlice->slice_type = temp % 5;
        READ_UE(p_bitctx, &currSlice->pic_parameter_set_id, "slice_pic_parameter_set_id");
        init_slice_parmeters(currSlice);
        FUN_CHECK(ret = set_slice_user_parmeters(currSlice));
        //!< read rest slice header syntax
        READ_BITS(p_bitctx, currSlice->active_sps->log2_max_frame_num_minus4 + 4, &currSlice->frame_num, "frame_num");
        if (currSlice->active_sps->frame_mbs_only_flag) { //!< user in_slice info
            p_Vid->structure = FRAME;
            currSlice->field_pic_flag = 0;
            currSlice->bottom_field_flag = 0;
        } else {
            READ_ONEBIT(p_bitctx, &currSlice->field_pic_flag, "field_pic_flag");
            if (currSlice->field_pic_flag) {
                READ_ONEBIT(p_bitctx, &currSlice->bottom_field_flag, "field_pic_flag");
                p_Vid->structure = currSlice->bottom_field_flag ? BOTTOM_FIELD : TOP_FIELD;
            } else {
                p_Vid->structure = FRAME;
                currSlice->bottom_field_flag = 0;
            }
        }
        currSlice->structure = p_Vid->structure;
        currSlice->mb_aff_frame_flag = (currSlice->active_sps->mb_adaptive_frame_field_flag && (currSlice->field_pic_flag == 0));
        if (currSlice->idr_flag) {
            READ_UE(p_bitctx, &currSlice->idr_pic_id, "idr_pic_id");
        } else if (currSlice->svc_extension_flag == 0 && currSlice->mvcExt.non_idr_flag == 0) {
            READ_UE(p_bitctx, &currSlice->idr_pic_id, "idr_pic_id");
        }
        poc_used_bits = p_bitctx->used_bits; //!< init poc used bits
        if (currSlice->active_sps->pic_order_cnt_type == 0) {
            READ_BITS(p_bitctx, currSlice->active_sps->log2_max_pic_order_cnt_lsb_minus4 + 4, &currSlice->pic_order_cnt_lsb, "pic_order_cnt_lsb");
            if (currSlice->p_Vid->active_pps->bottom_field_pic_order_in_frame_present_flag == 1
                && !currSlice->field_pic_flag) {
                READ_SE(p_bitctx, &currSlice->delta_pic_order_cnt_bottom, "delta_pic_order_cnt_bottom");
            } else {
                currSlice->delta_pic_order_cnt_bottom = 0;
            }
        }
        if (currSlice->active_sps->pic_order_cnt_type == 1) {
            if (!currSlice->active_sps->delta_pic_order_always_zero_flag) {
                READ_SE(p_bitctx, &currSlice->delta_pic_order_cnt[0], "delta_pic_order_cnt[0]");

                if (currSlice->p_Vid->active_pps->bottom_field_pic_order_in_frame_present_flag == 1 && !currSlice->field_pic_flag) {
                    READ_SE(p_bitctx, &currSlice->delta_pic_order_cnt[1], "delta_pic_order_cnt[1]");
                } else {
                    currSlice->delta_pic_order_cnt[1] = 0;  //!< set to zero if not in stream
                }
            } else {
                currSlice->delta_pic_order_cnt[0] = 0;
                currSlice->delta_pic_order_cnt[1] = 0;
            }
        }
        currSlice->poc_used_bitlen = p_bitctx->used_bits - poc_used_bits; //!< calculate poc used bit length
        //!< redundant_pic_cnt is missing here
        ASSERT(currSlice->p_Vid->active_pps->redundant_pic_cnt_present_flag == 0); // add by dw, high 4:2:2 profile not support
        if (currSlice->p_Vid->active_pps->redundant_pic_cnt_present_flag) {
            READ_UE(p_bitctx, &currSlice->redundant_pic_cnt, "redundant_pic_cnt");
        }

        if (currSlice->slice_type == B_SLICE) {
            READ_ONEBIT(p_bitctx, &currSlice->direct_spatial_mv_pred_flag, "direct_spatial_mv_pred_flag");
        } else {
            currSlice->direct_spatial_mv_pred_flag = 0;
        }
        currSlice->num_ref_idx_active[LIST_0] = currSlice->p_Vid->active_pps->num_ref_idx_l0_default_active_minus1 + 1;
        currSlice->num_ref_idx_active[LIST_1] = currSlice->p_Vid->active_pps->num_ref_idx_l1_default_active_minus1 + 1;

        if (currSlice->slice_type == P_SLICE
            || currSlice->slice_type == SP_SLICE || currSlice->slice_type == B_SLICE) {
            //!< direct_spatial_mv_pred_flag
            READ_ONEBIT(p_bitctx, &currSlice->num_ref_idx_override_flag, "num_ref_idx_override_flag");
            if (currSlice->num_ref_idx_override_flag) {
                READ_UE(p_bitctx, &currSlice->num_ref_idx_active[LIST_0], "num_ref_idx_active0");
                currSlice->num_ref_idx_active[LIST_0] += 1;
                if (currSlice->slice_type == B_SLICE) {
                    READ_UE(p_bitctx, &currSlice->num_ref_idx_active[LIST_1], "num_ref_idx_active1");
                    currSlice->num_ref_idx_active[LIST_1] += 1;
                }
            }
        }
        if (currSlice->slice_type != B_SLICE) {
            currSlice->num_ref_idx_active[LIST_1] = 0;
        }
        //!<----------------------------

        //FPRINT(g_debug_file1, "--- new frame ---- \n");
        FUN_CHECK(ret = ref_pic_list_mvc_modification(currSlice));
        if ((currSlice->p_Vid->active_pps->weighted_pred_flag
             && (currSlice->slice_type == P_SLICE || currSlice->slice_type == SP_SLICE))
            || (currSlice->p_Vid->active_pps->weighted_bipred_idc == 1 && (currSlice->slice_type == B_SLICE))) {
            FUN_CHECK(ret = pred_weight_table(currSlice));
        }
        currSlice->drpm_used_bitlen = 0;
        if (currSlice->nal_reference_idc) {
            FUN_CHECK(ret = dec_ref_pic_marking(currSlice));
        }
        if (g_max_bytes < (p_bitctx->used_bits >> 3)) {
            g_max_bytes = (p_bitctx->used_bits >> 3);
        }
        currSlice->is_new_picture = 1;
    }
    FunctionOut(logctx->parr[RUN_PARSE]);

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:
    recycle_slice(currSlice);

    return ret;
}
