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

#define MODULE_TAG "h264d_init"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_mem.h"

#include "h264d_global.h"
#include "h264d_init.h"
#include "h264d_dpb.h"
#include "h264d_scalist.h"
#include "h264d_fill.h"
#include "h264d_slice.h"

static MPP_RET decode_poc(H264dVideoCtx_t *p_Vid, H264_SLICE_t *pSlice)
{
    RK_S32 i = 0;
    RK_U32 MaxPicOrderCntLsb = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_SPS_t *active_sps = p_Vid->active_sps;
    // for POC mode 0:
    MaxPicOrderCntLsb = (1 << (active_sps->log2_max_pic_order_cnt_lsb_minus4 + 4));

    switch (active_sps->pic_order_cnt_type) {
    case 0: // POC MODE 0
        // 1st
        if (pSlice->idr_flag) {
            p_Vid->PrevPicOrderCntMsb = 0;
            p_Vid->PrevPicOrderCntLsb = 0;
        } else {
            if (p_Vid->last_has_mmco_5) {
                if (p_Vid->last_pic_bottom_field) {
                    p_Vid->PrevPicOrderCntMsb = 0;
                    p_Vid->PrevPicOrderCntLsb = 0;
                } else {
                    p_Vid->PrevPicOrderCntMsb = 0;
                    p_Vid->PrevPicOrderCntLsb = pSlice->toppoc;
                }
            }
        }
        // Calculate the MSBs of current picture
        if (pSlice->pic_order_cnt_lsb  <  p_Vid->PrevPicOrderCntLsb &&
            (p_Vid->PrevPicOrderCntLsb - pSlice->pic_order_cnt_lsb) >= (RK_S32)(MaxPicOrderCntLsb / 2)) {
            pSlice->PicOrderCntMsb = p_Vid->PrevPicOrderCntMsb + MaxPicOrderCntLsb;
        } else if (pSlice->pic_order_cnt_lsb  >  p_Vid->PrevPicOrderCntLsb &&
                   (pSlice->pic_order_cnt_lsb - p_Vid->PrevPicOrderCntLsb) > (RK_S32)(MaxPicOrderCntLsb / 2)) {
            pSlice->PicOrderCntMsb = p_Vid->PrevPicOrderCntMsb - MaxPicOrderCntLsb;
        } else {
            pSlice->PicOrderCntMsb = p_Vid->PrevPicOrderCntMsb;
        }
        // 2nd
        if (pSlice->field_pic_flag == 0) {
            //frame pix
            pSlice->toppoc = pSlice->PicOrderCntMsb + pSlice->pic_order_cnt_lsb;
            pSlice->bottompoc = pSlice->toppoc + pSlice->delta_pic_order_cnt_bottom;
            pSlice->ThisPOC = pSlice->framepoc = (pSlice->toppoc < pSlice->bottompoc) ? pSlice->toppoc : pSlice->bottompoc; // POC200301
        } else if (pSlice->bottom_field_flag == 0) {
            //top field
            pSlice->ThisPOC = pSlice->toppoc = pSlice->PicOrderCntMsb + pSlice->pic_order_cnt_lsb;
        } else {
            //bottom field
            pSlice->ThisPOC = pSlice->bottompoc = pSlice->PicOrderCntMsb + pSlice->pic_order_cnt_lsb;
        }
        pSlice->framepoc = pSlice->ThisPOC;
        p_Vid->ThisPOC = pSlice->ThisPOC;
        //if ( pSlice->frame_num != p_Vid->PreviousFrameNum) //Seems redundant
        p_Vid->PreviousFrameNum = pSlice->frame_num;
        if (pSlice->nal_reference_idc) {
            p_Vid->PrevPicOrderCntLsb = pSlice->pic_order_cnt_lsb;
            p_Vid->PrevPicOrderCntMsb = pSlice->PicOrderCntMsb;
        }
        break;

    case 1: // POC MODE 1
        // 1st
        if (pSlice->idr_flag) {
            p_Vid->FrameNumOffset = 0;     //  first pix of IDRGOP,
            VAL_CHECK(ret, 0 == pSlice->frame_num);
        } else {
            if (p_Vid->last_has_mmco_5) {
                p_Vid->PreviousFrameNumOffset = 0;
                p_Vid->PreviousFrameNum = 0;
            }
            if (pSlice->frame_num < (RK_S32)p_Vid->PreviousFrameNum) {
                //not first pix of IDRGOP
                p_Vid->FrameNumOffset = p_Vid->PreviousFrameNumOffset + p_Vid->max_frame_num;
            } else {
                p_Vid->FrameNumOffset = p_Vid->PreviousFrameNumOffset;
            }
        }
        // 2nd
        if (active_sps->num_ref_frames_in_pic_order_cnt_cycle) {
            pSlice->AbsFrameNum = p_Vid->FrameNumOffset + pSlice->frame_num;
        } else {
            pSlice->AbsFrameNum = 0;
        }
        if ((!pSlice->nal_reference_idc) && pSlice->AbsFrameNum > 0) {
            pSlice->AbsFrameNum--;
        }
        // 3rd
        p_Vid->ExpectedDeltaPerPicOrderCntCycle = 0;
        if (active_sps->num_ref_frames_in_pic_order_cnt_cycle) {
            for (i = 0; i < (RK_S32)active_sps->num_ref_frames_in_pic_order_cnt_cycle; i++) {
                p_Vid->ExpectedDeltaPerPicOrderCntCycle += active_sps->offset_for_ref_frame[i];
            }
        }
        if (pSlice->AbsFrameNum) {
            p_Vid->PicOrderCntCycleCnt = (pSlice->AbsFrameNum - 1) / active_sps->num_ref_frames_in_pic_order_cnt_cycle;
            p_Vid->FrameNumInPicOrderCntCycle = (pSlice->AbsFrameNum - 1) % active_sps->num_ref_frames_in_pic_order_cnt_cycle;
            p_Vid->ExpectedPicOrderCnt = p_Vid->PicOrderCntCycleCnt * p_Vid->ExpectedDeltaPerPicOrderCntCycle;
            for (i = 0; i <= (RK_S32)p_Vid->FrameNumInPicOrderCntCycle; i++)
                p_Vid->ExpectedPicOrderCnt += active_sps->offset_for_ref_frame[i];
        } else {
            p_Vid->ExpectedPicOrderCnt = 0;
        }
        if (!pSlice->nal_reference_idc) {
            p_Vid->ExpectedPicOrderCnt += active_sps->offset_for_non_ref_pic;
        }
        if (pSlice->field_pic_flag == 0) {
            //frame pix
            pSlice->toppoc = p_Vid->ExpectedPicOrderCnt + pSlice->delta_pic_order_cnt[0];
            pSlice->bottompoc = pSlice->toppoc + active_sps->offset_for_top_to_bottom_field + pSlice->delta_pic_order_cnt[1];
            pSlice->ThisPOC = pSlice->framepoc = (pSlice->toppoc < pSlice->bottompoc) ? pSlice->toppoc : pSlice->bottompoc; // POC200301
        } else if (pSlice->bottom_field_flag == 0) {
            //top field
            pSlice->ThisPOC = pSlice->toppoc = p_Vid->ExpectedPicOrderCnt + pSlice->delta_pic_order_cnt[0];
        } else {
            //bottom field
            pSlice->ThisPOC = pSlice->bottompoc = p_Vid->ExpectedPicOrderCnt + active_sps->offset_for_top_to_bottom_field + pSlice->delta_pic_order_cnt[0];
        }
        pSlice->framepoc = pSlice->ThisPOC;
        p_Vid->PreviousFrameNum = pSlice->frame_num;
        p_Vid->PreviousFrameNumOffset = p_Vid->FrameNumOffset;
        break;


    case 2: // POC MODE 2
        if (pSlice->idr_flag) { // IDR picture
            p_Vid->FrameNumOffset = 0;     //  first pix of IDRGOP,
            pSlice->ThisPOC = pSlice->framepoc = pSlice->toppoc = pSlice->bottompoc = 0;
            VAL_CHECK(ret, 0 == pSlice->frame_num);
        } else {
            if (p_Vid->last_has_mmco_5) {
                p_Vid->PreviousFrameNum = 0;
                p_Vid->PreviousFrameNumOffset = 0;
            }
            if (pSlice->frame_num < (RK_S32)p_Vid->PreviousFrameNum) {
                p_Vid->FrameNumOffset = p_Vid->PreviousFrameNumOffset + p_Vid->max_frame_num;
            } else {
                p_Vid->FrameNumOffset = p_Vid->PreviousFrameNumOffset;
            }
            pSlice->AbsFrameNum = p_Vid->FrameNumOffset + pSlice->frame_num;
            if (!pSlice->nal_reference_idc) {
                pSlice->ThisPOC = (2 * pSlice->AbsFrameNum - 1);
            } else {
                pSlice->ThisPOC = (2 * pSlice->AbsFrameNum);
            }
            if (pSlice->field_pic_flag == 0) {
                pSlice->toppoc = pSlice->bottompoc = pSlice->framepoc = pSlice->ThisPOC;
            } else if (pSlice->bottom_field_flag == 0) {
                pSlice->toppoc = pSlice->framepoc = pSlice->ThisPOC;
            } else {
                pSlice->bottompoc = pSlice->framepoc = pSlice->ThisPOC;
            }
        }
        p_Vid->PreviousFrameNum = pSlice->frame_num;
        p_Vid->PreviousFrameNumOffset = p_Vid->FrameNumOffset;
        break;
    default:
        ret = MPP_NOK;
        goto __FAILED;
    }
    return ret = MPP_OK;

__FAILED:
    return ret;
}

static MPP_RET store_proc_picture_in_dpb(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;
    H264_FrameStore_t *fs = p_Dpb->fs_ilref[0];
    H264_DecCtx_t *p_Dec = p_Dpb->p_Vid->p_Dec;

    VAL_CHECK(ret, NULL != p);
    if (p_Dpb->used_size_il > 0) {
        if (fs->frame) {
            free_storable_picture(p_Dec, fs->frame);
            fs->frame = NULL;
        }
        if (fs->top_field) {
            free_storable_picture(p_Dec, fs->top_field);
            fs->top_field = NULL;
        }
        if (fs->bottom_field) {
            free_storable_picture(p_Dec, fs->bottom_field);
            fs->bottom_field = NULL;
        }
        fs->is_used = 0;
        fs->is_reference = 0;
        p_Dpb->used_size_il--;
    }
    if (fs->is_used > 0) { //checking;
        if (p->structure == FRAME) {
            VAL_CHECK(ret, fs->frame == NULL);
        } else if (p->structure == TOP_FIELD) {
            VAL_CHECK(ret, fs->top_field == NULL);
        } else if (p->structure == BOTTOM_FIELD) {
            VAL_CHECK(ret, fs->bottom_field == NULL);
        }
    }
    FUN_CHECK(ret = insert_picture_in_dpb(p_Vid, fs, p, 0));
    if ((p->structure == FRAME && fs->is_used == 3)
        || (p->structure != FRAME && fs->is_used && fs->is_used < 3)) {
        p_Dpb->used_size_il++;
    }

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void dpb_mark_add_used(H264_DpbMark_t *p_mark, RK_S32 structure)
{
    //!<---- index add ----
    if (structure == FRAME || structure == TOP_FIELD) {
        p_mark->top_used += 1;
    }
    if (structure == FRAME || structure == BOTTOM_FIELD) {
        p_mark->bot_used += 1;
    }
}

static H264_StorePic_t* clone_storable_picture(H264dVideoCtx_t *p_Vid, H264_StorePic_t *p_pic)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_StorePic_t *p_stored_pic = alloc_storable_picture(p_Vid, p_Vid->structure);

    MEM_CHECK(ret, p_stored_pic);
    p_stored_pic->mem_malloc_type = Mem_Clone;
    p_stored_pic->mem_mark = p_pic->mem_mark;
    dpb_mark_add_used(p_stored_pic->mem_mark, p_stored_pic->structure);
    p_stored_pic->colmv_no_used_flag = 1;  // clone, colmv is not be used

    p_stored_pic->pic_num = p_pic->pic_num;
    p_stored_pic->frame_num = p_pic->frame_num;
    p_stored_pic->long_term_frame_idx = p_pic->long_term_frame_idx;
    p_stored_pic->long_term_pic_num = p_pic->long_term_pic_num;
    p_stored_pic->is_long_term = 0;
    p_stored_pic->non_existing = p_pic->non_existing;
    p_stored_pic->max_slice_id = p_pic->max_slice_id;
    p_stored_pic->structure = p_pic->structure;

    p_stored_pic->mb_aff_frame_flag = p_pic->mb_aff_frame_flag;
    p_stored_pic->poc = p_pic->poc;
    p_stored_pic->top_poc = p_pic->top_poc;
    p_stored_pic->bottom_poc = p_pic->bottom_poc;
    p_stored_pic->frame_poc = p_pic->frame_poc;
    p_stored_pic->is_mmco_5 = p_pic->is_mmco_5;
    p_stored_pic->poc_mmco5 = p_pic->poc_mmco5;
    p_stored_pic->top_poc_mmco5 = p_pic->top_poc_mmco5;
    p_stored_pic->bot_poc_mmco5 = p_pic->bot_poc_mmco5;
    p_stored_pic->pic_num = p_pic->pic_num;
    p_stored_pic->frame_num = p_pic->frame_num;
    p_stored_pic->slice_type = p_pic->slice_type;
    p_stored_pic->idr_flag = p_pic->idr_flag;
    p_stored_pic->no_output_of_prior_pics_flag = p_pic->no_output_of_prior_pics_flag;
    p_stored_pic->long_term_reference_flag = 0;
    p_stored_pic->adaptive_ref_pic_buffering_flag = 0;
    p_stored_pic->dec_ref_pic_marking_buffer = NULL;
    p_stored_pic->PicWidthInMbs = p_pic->PicWidthInMbs;

    p_stored_pic->chroma_format_idc = p_pic->chroma_format_idc;
    p_stored_pic->frame_mbs_only_flag = p_pic->frame_mbs_only_flag;
    p_stored_pic->frame_cropping_flag = p_pic->frame_cropping_flag;
    if (p_stored_pic->frame_cropping_flag) {
        p_stored_pic->frame_crop_left_offset = p_pic->frame_crop_left_offset;
        p_stored_pic->frame_crop_right_offset = p_pic->frame_crop_right_offset;
        p_stored_pic->frame_crop_top_offset = p_pic->frame_crop_top_offset;
        p_stored_pic->frame_crop_bottom_offset = p_pic->frame_crop_bottom_offset;
    }
    // MVC-related parameters
    p_stored_pic->inter_view_flag = p_pic->inter_view_flag;
    p_stored_pic->anchor_pic_flag = 0;
    p_stored_pic->view_id = p_pic->view_id;
    p_stored_pic->layer_id = p_pic->layer_id;
    p_stored_pic->proc_flag = 1;
    p_stored_pic->is_output = 1;
    p_stored_pic->used_for_reference = 1;

    return p_stored_pic;
__FAILED:
    (void)ret;
    return NULL;
}

static MPP_RET init_mvc_picture(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    RK_S32 poc = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264_DpbBuf_t *p_Dpb = p_Vid->p_Dpb_layer[0];
    H264_StorePic_t *p_pic = NULL;
    H264_FrameStore_t *fs = NULL;
    H264_StorePic_t *p_clone = NULL;

    // find BL reconstructed picture
    if (currSlice->structure == FRAME) {
        for (i = 0; i < p_Dpb->used_size; i++) {
            fs = p_Dpb->fs[i];
            if (fs->frame) {
                poc = fs->frame->is_mmco_5 ? fs->frame->poc_mmco5 : fs->frame->poc;
            }
            if (fs->frame && (fs->frame->layer_id == 0) && (poc == currSlice->framepoc)) {
                p_pic = fs->frame;
                if (!fs->frame->is_mmco_5) {
                    break;
                }
            }
        }
    } else if (currSlice->structure == TOP_FIELD) {
        for (i = 0; i < p_Dpb->used_size; i++) {
            fs = p_Dpb->fs[i];
            if (fs->top_field) {
                poc = fs->top_field->is_mmco_5 ? fs->top_field->top_poc_mmco5 : fs->top_field->top_poc;
            }
            if (fs->top_field && (fs->top_field->layer_id == 0) && (poc == currSlice->toppoc)) {
                p_pic = fs->top_field;
                if (!fs->top_field->is_mmco_5) {
                    break;
                }
            }
        }
    } else {
        for (i = 0; i < p_Dpb->used_size; i++) {
            fs = p_Dpb->fs[i];
            if (fs->bottom_field) {
                poc = fs->bottom_field->is_mmco_5 ? fs->bottom_field->bot_poc_mmco5 : fs->bottom_field->bottom_poc;
            }
            if (fs->bottom_field && (fs->bottom_field->layer_id == 0) && (poc == currSlice->bottompoc)) {
                p_pic = fs->bottom_field;
                if (!fs->bottom_field->is_mmco_5) {
                    break;
                }
            }
        }
    }
    if (p_pic) {
        p_clone = clone_storable_picture(p_Vid, p_pic);
        MEM_CHECK(ret, p_clone);
        FUN_CHECK(ret = store_proc_picture_in_dpb(currSlice->p_Dpb, p_clone));
    }

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static inline RK_U32 rkv_len_align_422(RK_U32 val)
{
    return ((5 * MPP_ALIGN(val, 16)) / 2);
}

static MPP_RET dpb_mark_malloc(H264dVideoCtx_t *p_Vid, H264_StorePic_t *dec_pic)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DpbMark_t *cur_mark = NULL;
    H264_DecCtx_t *p_Dec = p_Vid->p_Dec;
    H264_DpbMark_t *p_mark = p_Vid->p_Dec->dpb_mark;
    RK_S32 structure = dec_pic->structure;
    RK_S32 layer_id = dec_pic->layer_id;

    if (!dec_pic->combine_flag) {
        RK_U8 idx = 0;
        while (p_mark[idx].out_flag || p_mark[idx].top_used
               || p_mark[idx].bot_used) {
            idx++;
            ASSERT(MAX_MARK_SIZE > idx);
        }

        mpp_buf_slot_get_unused(p_Vid->p_Dec->frame_slots, &p_mark[idx].slot_idx);
        if (p_mark[idx].slot_idx < 0) {
            H264D_WARNNING("[dpb_mark_malloc] error, buf_slot has not get.");
            ret = MPP_NOK;
            goto __FAILED;
        }
        cur_mark = &p_mark[idx];

        cur_mark->out_flag = 1;
        {
            MppFrame mframe = NULL;
            RK_U32 hor_stride, ver_stride;

            mpp_frame_init(&mframe);
            if ((YUV420 == p_Vid->yuv_format) && (8 == p_Vid->bit_depth_luma)) {
                mpp_frame_set_fmt(mframe, MPP_FMT_YUV420SP);
            } else if ((YUV420 == p_Vid->yuv_format) && (10 == p_Vid->bit_depth_luma)) {
                mpp_frame_set_fmt(mframe, MPP_FMT_YUV420SP_10BIT);
            } else if ((YUV422 == p_Vid->yuv_format) && (8 == p_Vid->bit_depth_luma)) {
                mpp_frame_set_fmt(mframe, MPP_FMT_YUV422SP);
                mpp_slots_set_prop(p_Dec->frame_slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
            } else if ((YUV422 == p_Vid->yuv_format) && (10 == p_Vid->bit_depth_luma)) {
                mpp_frame_set_fmt(mframe, MPP_FMT_YUV422SP_10BIT);
                mpp_slots_set_prop(p_Dec->frame_slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
            }
            hor_stride = MPP_ALIGN(p_Vid->width * p_Vid->bit_depth_luma, 8) / 8;
            ver_stride = p_Vid->height;
            /* Before cropping */
            mpp_frame_set_hor_stride(mframe, hor_stride);
            mpp_frame_set_ver_stride(mframe, ver_stride);
            /* After cropped */
            mpp_frame_set_width(mframe, p_Vid->width_after_crop);
            mpp_frame_set_height(mframe, p_Vid->height_after_crop);
            mpp_frame_set_pts(mframe, p_Vid->p_Cur->last_pts);
            mpp_frame_set_dts(mframe, p_Vid->p_Cur->last_dts);

            /* Setting the interlace mode for the picture */
            switch (structure) {
            case FRAME:
                mpp_frame_set_mode(mframe, MPP_FRAME_FLAG_FRAME);
                break;
            case TOP_FIELD:
                mpp_frame_set_mode(mframe, MPP_FRAME_FLAG_PAIRED_FIELD
                                   | MPP_FRAME_FLAG_TOP_FIRST);
                break;
            case BOTTOM_FIELD:
                mpp_frame_set_mode(mframe, MPP_FRAME_FLAG_PAIRED_FIELD
                                   | MPP_FRAME_FLAG_BOT_FIRST);
                break;
            default:
                H264D_DBG(H264D_DBG_FIELD_PAIRED, "Unknown interlace mode");
                mpp_assert(0);
            }

            if ((p_Vid->active_sps->vui_parameters_present_flag &&
                 p_Vid->active_sps->vui_seq_parameters.pic_struct_present_flag &&
                 p_Vid->p_Cur->sei.type == SEI_PIC_TIMING) ||
                p_Vid->p_Cur->sei.pic_timing.pic_struct != 0) {
                if (p_Vid->p_Cur->sei.pic_timing.pic_struct == 3 ||
                    p_Vid->p_Cur->sei.pic_timing.pic_struct == 5)
                    mpp_frame_set_mode(mframe, MPP_FRAME_FLAG_PAIRED_FIELD
                                       | MPP_FRAME_FLAG_TOP_FIRST);
                if (p_Vid->p_Cur->sei.pic_timing.pic_struct == 4 ||
                    p_Vid->p_Cur->sei.pic_timing.pic_struct == 6)
                    mpp_frame_set_mode(mframe, MPP_FRAME_FLAG_PAIRED_FIELD
                                       | MPP_FRAME_FLAG_BOT_FIRST);
            }

            //!< set display parameter
            if (p_Vid->active_sps->vui_parameters_present_flag) {
                H264_VUI_t *p = &p_Vid->active_sps->vui_seq_parameters;

                if (p->video_signal_type_present_flag && p->video_full_range_flag)
                    mpp_frame_set_color_range(mframe, MPP_FRAME_RANGE_JPEG);
                else
                    mpp_frame_set_color_range(mframe, MPP_FRAME_RANGE_MPEG);

                if (p->colour_description_present_flag) {
                    mpp_frame_set_color_primaries(mframe, p->colour_primaries);
                    mpp_frame_set_color_trc(mframe, p->transfer_characteristics);
                    mpp_frame_set_colorspace(mframe, p->matrix_coefficients);
                } else {
                    mpp_frame_set_color_primaries(mframe, MPP_FRAME_PRI_UNSPECIFIED);
                    mpp_frame_set_color_trc(mframe, MPP_FRAME_PRI_UNSPECIFIED);
                    mpp_frame_set_colorspace(mframe, MPP_FRAME_SPC_UNSPECIFIED);
                }
            }
            mpp_buf_slot_set_prop(p_Dec->frame_slots, cur_mark->slot_idx, SLOT_FRAME, mframe);
            mpp_frame_deinit(&mframe);
            mpp_buf_slot_get_prop(p_Dec->frame_slots, cur_mark->slot_idx, SLOT_FRAME_PTR, &cur_mark->mframe);
        }

        p_Vid->active_dpb_mark[layer_id] = cur_mark;
    }
    cur_mark = p_Vid->active_dpb_mark[layer_id];
    if (structure == FRAME || structure == TOP_FIELD) {
        cur_mark->top_used += 1;
    }
    if (structure == FRAME || structure == BOTTOM_FIELD) {
        cur_mark->bot_used += 1;
    }
    H264D_DBG(H264D_DBG_DPB_MALLIC,
              "[DPB_malloc] g_framecnt=%d, com_flag=%d, mark_idx=%d, slot_idx=%d, slice_type=%d, struct=%d, lay_id=%d\n",
              p_Vid->g_framecnt, dec_pic->combine_flag, cur_mark->mark_idx,
              cur_mark->slot_idx, dec_pic->slice_type, dec_pic->structure,
              layer_id);

    p_Vid->p_Dec->in_task->output = cur_mark->slot_idx;
    mpp_buf_slot_set_flag(p_Dec->frame_slots, cur_mark->slot_idx, SLOT_HAL_OUTPUT);
    p_Dec->last_frame_slot_idx = cur_mark->slot_idx;
    dec_pic->mem_mark = p_Vid->active_dpb_mark[layer_id];
    dec_pic->mem_mark->pic = dec_pic;

    return ret = MPP_OK;
__FAILED:
    dec_pic->mem_mark = NULL;
    return ret;
}

static MPP_RET check_dpb_discontinuous(H264_StorePic_t *p_last, H264_StorePic_t *dec_pic, H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
#if 1
    if (p_last && dec_pic && (dec_pic->slice_type != I_SLICE)) {
        RK_U32 error_flag = 0;

        if (dec_pic->frame_num == p_last->frame_num ||
            dec_pic->frame_num == ((p_last->frame_num + 1) % currSlice->p_Vid->max_frame_num))
            error_flag = 0;
        else
            error_flag = 1;
        currSlice->p_Dec->errctx.cur_err_flag |= error_flag ? 1 : 0;

        H264D_DBG(H264D_DBG_DISCONTINUOUS, "[discontinuous] last_slice=%d, cur_slice=%d, last_fnum=%d, cur_fnum=%d, last_poc=%d, cur_poc=%d",
                  p_last->slice_type, dec_pic->slice_type, p_last->frame_num, dec_pic->frame_num, p_last->poc, dec_pic->poc);
    }
#endif
    return ret = MPP_OK;
}

static MPP_RET alloc_decpic(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_StorePic_t *dec_pic = NULL;

    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264_SPS_t *active_sps = p_Vid->active_sps;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;

    dec_pic = alloc_storable_picture(p_Vid, currSlice->structure);
    MEM_CHECK(ret, dec_pic);
    currSlice->toppoc    = p_Vid->last_toppoc[currSlice->layer_id];
    currSlice->bottompoc = p_Vid->last_bottompoc[currSlice->layer_id];
    currSlice->framepoc  = p_Vid->last_framepoc[currSlice->layer_id];
    currSlice->ThisPOC   = p_Vid->last_thispoc[currSlice->layer_id];
    FUN_CHECK(ret = decode_poc(p_Vid, currSlice));  //!< calculate POC

    dec_pic->top_poc    = currSlice->toppoc;
    dec_pic->bottom_poc = currSlice->bottompoc;
    dec_pic->frame_poc  = currSlice->framepoc;
    dec_pic->ThisPOC    = currSlice->ThisPOC;

    p_Vid->last_toppoc[currSlice->layer_id]    = currSlice->toppoc;
    p_Vid->last_bottompoc[currSlice->layer_id] = currSlice->bottompoc;
    p_Vid->last_framepoc[currSlice->layer_id]  = currSlice->framepoc;
    p_Vid->last_thispoc[currSlice->layer_id]   = currSlice->ThisPOC;

    if (currSlice->structure == FRAME) {
        if (currSlice->mb_aff_frame_flag) {
            dec_pic->iCodingType = FRAME_MB_PAIR_CODING;
        } else {
            dec_pic->iCodingType = FRAME_CODING;
        }
    } else {
        dec_pic->iCodingType = FIELD_CODING;
    }
    dec_pic->layer_id = currSlice->layer_id;
    dec_pic->view_id  = currSlice->view_id;
    dec_pic->inter_view_flag = currSlice->inter_view_flag;
    dec_pic->anchor_pic_flag = currSlice->anchor_pic_flag;
    if (dec_pic->layer_id == 1) {
        if ((p_Vid->profile_idc == H264_PROFILE_MVC_HIGH) || (p_Vid->profile_idc == H264_PROFILE_STEREO_HIGH)) {
            FUN_CHECK(ret = init_mvc_picture(currSlice));
        }
    }
    if (currSlice->structure == TOP_FIELD) {
        dec_pic->poc = currSlice->toppoc;
    } else if (currSlice->structure == BOTTOM_FIELD) {
        dec_pic->poc = currSlice->bottompoc;
    } else if (currSlice->structure == FRAME) {
        dec_pic->poc = currSlice->framepoc;
    } else {
        ret = MPP_NOK;
        goto __FAILED;
    }
    dec_pic->slice_type = p_Vid->type;
    dec_pic->used_for_reference = (currSlice->nal_reference_idc != 0);
    dec_pic->idr_flag = currSlice->idr_flag;
    dec_pic->no_output_of_prior_pics_flag = currSlice->no_output_of_prior_pics_flag;
    dec_pic->long_term_reference_flag = currSlice->long_term_reference_flag;
    dec_pic->adaptive_ref_pic_buffering_flag = currSlice->adaptive_ref_pic_buffering_flag;
    dec_pic->dec_ref_pic_marking_buffer = currSlice->dec_ref_pic_marking_buffer;

    currSlice->dec_ref_pic_marking_buffer = NULL;
    dec_pic->mb_aff_frame_flag = currSlice->mb_aff_frame_flag;
    dec_pic->PicWidthInMbs = p_Vid->PicWidthInMbs;
    dec_pic->pic_num = currSlice->frame_num;
    dec_pic->frame_num = currSlice->frame_num;
    dec_pic->chroma_format_idc = active_sps->chroma_format_idc;

    dec_pic->frame_mbs_only_flag = active_sps->frame_mbs_only_flag;
    dec_pic->frame_cropping_flag = active_sps->frame_cropping_flag;
    if (dec_pic->frame_cropping_flag) {
        dec_pic->frame_crop_left_offset = active_sps->frame_crop_left_offset;
        dec_pic->frame_crop_right_offset = active_sps->frame_crop_right_offset;
        dec_pic->frame_crop_top_offset = active_sps->frame_crop_top_offset;
        dec_pic->frame_crop_bottom_offset = active_sps->frame_crop_bottom_offset;
    } else {
        dec_pic->frame_crop_left_offset = 0;
        dec_pic->frame_crop_right_offset = 0;
        dec_pic->frame_crop_top_offset = 0;
        dec_pic->frame_crop_bottom_offset = 0;
    }
    dec_pic->width = p_Vid->width;
    dec_pic->height = p_Vid->height;
    dec_pic->width_after_crop = p_Vid->width_after_crop;
    dec_pic->height_after_crop = p_Vid->height_after_crop;
    dec_pic->combine_flag = get_filed_dpb_combine_flag(p_Dpb->last_picture, dec_pic);
    /* malloc dpb_memory */
    FUN_CHECK(ret = dpb_mark_malloc(p_Vid, dec_pic));
    FUN_CHECK(ret = check_dpb_discontinuous(p_Vid->last_pic, dec_pic, currSlice));
    dec_pic->mem_malloc_type = Mem_Malloc;
    dec_pic->colmv_no_used_flag = 0;
    p_Vid->dec_pic = dec_pic;

    return ret = MPP_OK;
__FAILED:
    MPP_FREE(dec_pic);
    p_Vid->dec_pic = NULL;

    return ret;
}

static void update_pic_num(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;
    H264_SPS_t *active_sps = p_Vid->active_sps;

    RK_S32 add_top = 0, add_bottom = 0;
    RK_S32 max_frame_num = 1 << (active_sps->log2_max_frame_num_minus4 + 4);

    if (currSlice->idr_flag) {
        return;
    }
    if (currSlice->structure == FRAME) {
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used == 3) {
                if ((p_Dpb->fs_ref[i]->frame->used_for_reference) && (!p_Dpb->fs_ref[i]->frame->is_long_term)) {
                    if ((RK_S32)p_Dpb->fs_ref[i]->frame_num > currSlice->frame_num) {
                        p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num - max_frame_num;
                    } else {
                        p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num;
                    }
                    p_Dpb->fs_ref[i]->frame->pic_num = p_Dpb->fs_ref[i]->frame_num_wrap;
                }
            }
        }
        //!< update long_term_pic_num
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ltref[i]->is_used == 3) {
                if (p_Dpb->fs_ltref[i]->frame->is_long_term) {
                    p_Dpb->fs_ltref[i]->frame->long_term_pic_num = p_Dpb->fs_ltref[i]->frame->long_term_frame_idx;
                }
            }
        }
    } else {
        if (currSlice->structure == TOP_FIELD) {
            add_top = 1;
            add_bottom = 0;
        } else {
            add_top = 0;
            add_bottom = 1;
        }

        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_reference) {
                if ((RK_S32)p_Dpb->fs_ref[i]->frame_num > currSlice->frame_num) {
                    p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num - max_frame_num;
                } else {
                    p_Dpb->fs_ref[i]->frame_num_wrap = p_Dpb->fs_ref[i]->frame_num;
                }
                if (p_Dpb->fs_ref[i]->is_reference & 1) {
                    p_Dpb->fs_ref[i]->top_field->pic_num = (2 * p_Dpb->fs_ref[i]->frame_num_wrap) + add_top;
                }
                if (p_Dpb->fs_ref[i]->is_reference & 2) {
                    p_Dpb->fs_ref[i]->bottom_field->pic_num = (2 * p_Dpb->fs_ref[i]->frame_num_wrap) + add_bottom;
                }
            }
        }
        //!< update long_term_pic_num
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ltref[i]->is_long_term & 1) {
                p_Dpb->fs_ltref[i]->top_field->long_term_pic_num = 2 * p_Dpb->fs_ltref[i]->top_field->long_term_frame_idx + add_top;
            }
            if (p_Dpb->fs_ltref[i]->is_long_term & 2) {
                p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num = 2 * p_Dpb->fs_ltref[i]->bottom_field->long_term_frame_idx + add_bottom;
            }
        }
    }
}




static RK_S32 compare_pic_by_pic_num_desc(const void *arg1, const void *arg2)
{
    RK_S32 pic_num1 = (*(H264_StorePic_t**)arg1)->pic_num;
    RK_S32 pic_num2 = (*(H264_StorePic_t**)arg2)->pic_num;

    if (pic_num1 < pic_num2)
        return 1;
    if (pic_num1 > pic_num2)
        return -1;
    else
        return 0;
}

static RK_S32 compare_pic_by_lt_pic_num_asc(const void *arg1, const void *arg2)
{
    RK_S32 long_term_pic_num1 = (*(H264_StorePic_t**)arg1)->long_term_pic_num;
    RK_S32 long_term_pic_num2 = (*(H264_StorePic_t**)arg2)->long_term_pic_num;

    if (long_term_pic_num1 < long_term_pic_num2)
        return -1;
    if (long_term_pic_num1 > long_term_pic_num2)
        return 1;
    else
        return 0;
}

static RK_S32 compare_fs_by_frame_num_desc(const void *arg1, const void *arg2)
{
    RK_S32 frame_num_wrap1 = (*(H264_FrameStore_t**)arg1)->frame_num_wrap;
    RK_S32 frame_num_wrap2 = (*(H264_FrameStore_t**)arg2)->frame_num_wrap;
    if (frame_num_wrap1 < frame_num_wrap2)
        return 1;
    if (frame_num_wrap1 > frame_num_wrap2)
        return -1;
    else
        return 0;
}

static RK_S32 compare_fs_by_lt_pic_idx_asc(const void *arg1, const void *arg2)
{
    RK_S32 long_term_frame_idx1 = (*(H264_FrameStore_t**)arg1)->long_term_frame_idx;
    RK_S32 long_term_frame_idx2 = (*(H264_FrameStore_t**)arg2)->long_term_frame_idx;

    if (long_term_frame_idx1 < long_term_frame_idx2)
        return -1;
    else if (long_term_frame_idx1 > long_term_frame_idx2)
        return 1;
    else
        return 0;
}

static RK_U32 is_long_ref(H264_StorePic_t *s)
{
    return ((s->used_for_reference) && (s->is_long_term));
}

static RK_U32 is_short_ref(H264_StorePic_t *s)
{
    return ((s->used_for_reference) && (!(s->is_long_term)));
}

static void gen_pic_list_from_frame_list(RK_S32 currStructure, H264_FrameStore_t **fs_list,
                                         RK_S32 list_idx, H264_StorePic_t **list, RK_U8 *list_size, RK_U32 long_term)
{
    RK_S32 top_idx = 0;
    RK_S32 bot_idx = 0;

    RK_U32(*is_ref)(H264_StorePic_t * s) = (long_term) ? is_long_ref : is_short_ref;

    if (currStructure == TOP_FIELD) {
        while ((top_idx < list_idx) || (bot_idx < list_idx)) {
            for (; top_idx < list_idx; top_idx++) {
                if (fs_list[top_idx]->is_used & 1) {
                    if (is_ref(fs_list[top_idx]->top_field)) {
                        //<! short term ref pic
                        list[(RK_S16)*list_size] = fs_list[top_idx]->top_field;
                        (*list_size)++;
                        top_idx++;
                        break;
                    }
                }
            }
            for (; bot_idx < list_idx; bot_idx++) {
                if (fs_list[bot_idx]->is_used & 2) {
                    if (is_ref(fs_list[bot_idx]->bottom_field)) {
                        //<! short term ref pic
                        list[(RK_S16)*list_size] = fs_list[bot_idx]->bottom_field;
                        (*list_size)++;
                        bot_idx++;
                        break;
                    }
                }
            }
        }
    }
    if (currStructure == BOTTOM_FIELD) {
        while ((top_idx < list_idx) || (bot_idx < list_idx)) {
            for (; bot_idx < list_idx; bot_idx++) {
                if (fs_list[bot_idx]->is_used & 2) {
                    if (is_ref(fs_list[bot_idx]->bottom_field)) {
                        // short term ref pic
                        list[(RK_S16)*list_size] = fs_list[bot_idx]->bottom_field;
                        (*list_size)++;
                        bot_idx++;
                        break;
                    }
                }
            }
            for (; top_idx < list_idx; top_idx++) {
                if (fs_list[top_idx]->is_used & 1) {
                    if (is_ref(fs_list[top_idx]->top_field)) {
                        //!< short term ref pic
                        list[(RK_S16)*list_size] = fs_list[top_idx]->top_field;
                        (*list_size)++;
                        top_idx++;
                        break;
                    }
                }
            }
        }
    }
}

static RK_U32 is_view_id_in_ref_view_list(RK_S32 view_id, RK_S32 *ref_view_id, RK_S32 num_ref_views)
{
    RK_S32 i;
    for (i = 0; i < num_ref_views; i++) {
        if (view_id == ref_view_id[i])
            break;
    }

    return (num_ref_views && (i < num_ref_views));
}

static MPP_RET append_interview_list(H264_DpbBuf_t *p_Dpb,
                                     RK_S32 currPicStructure, RK_S32 list_idx, H264_FrameStore_t **list,
                                     RK_S32 *listXsize, RK_S32 currPOC, RK_S32 curr_layer_id, RK_S32 anchor_pic_flag)
{
    RK_S32 poc = 0;
    RK_S32 fld_idx = 0;
    RK_U32 pic_avail = 0;
    RK_S32 num_ref_views = 0;
    RK_S32 *ref_view_id = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_S32 iVOIdx = curr_layer_id;
    H264_FrameStore_t *fs = p_Dpb->fs_ilref[0];
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    VAL_CHECK(ret, iVOIdx >= 0); //!< Error: iVOIdx: %d is not less than 0
    if (anchor_pic_flag) {
        num_ref_views = list_idx ? p_Vid->active_subsps->num_anchor_refs_l1[iVOIdx] : p_Vid->active_subsps->num_anchor_refs_l0[iVOIdx];
        ref_view_id = list_idx ? p_Vid->active_subsps->anchor_ref_l1[iVOIdx] : p_Vid->active_subsps->anchor_ref_l0[iVOIdx];
    } else {
        num_ref_views = list_idx ? p_Vid->active_subsps->num_non_anchor_refs_l1[iVOIdx] : p_Vid->active_subsps->num_non_anchor_refs_l0[iVOIdx];
        ref_view_id = list_idx ? p_Vid->active_subsps->non_anchor_ref_l1[iVOIdx] : p_Vid->active_subsps->non_anchor_ref_l0[iVOIdx];
    }

    if (currPicStructure == BOTTOM_FIELD)
        fld_idx = 1;
    else
        fld_idx = 0;

    if (currPicStructure == FRAME) {
        pic_avail = (fs->is_used == 3);
        if (pic_avail) {
            poc = fs->frame->is_mmco_5 ? fs->frame->poc_mmco5 : fs->frame->poc;
        }
    } else if (currPicStructure == TOP_FIELD) {
        pic_avail = fs->is_used & 1;
        if (pic_avail) {
            poc = fs->top_field->is_mmco_5 ? fs->top_field->top_poc_mmco5 : fs->top_field->poc;
        }
    } else if (currPicStructure == BOTTOM_FIELD) {
        pic_avail = fs->is_used & 2;
        if (pic_avail) {
            poc = fs->bottom_field->is_mmco_5 ? fs->bottom_field->bot_poc_mmco5 : fs->bottom_field->poc;
        }
    } else {
        pic_avail = 0;
    }

    if (pic_avail && fs->inter_view_flag[fld_idx]) {
        if (poc == currPOC) {
            if (is_view_id_in_ref_view_list(fs->view_id, ref_view_id, num_ref_views)) {
                //!< add one inter-view reference;
                list[*listXsize] = fs;
                //!< next;
                (*listXsize)++;
            }
        }
    }
    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void gen_pic_list_from_frame_interview_list(RK_S32 currStructure,
                                                   H264_FrameStore_t **fs_list, RK_S32 list_idx, H264_StorePic_t **list, RK_U8 *list_size)
{
    RK_S32 i;

    if (currStructure == TOP_FIELD) {
        for (i = 0; i < list_idx; i++) {
            list[(RK_S32)(*list_size)] = fs_list[i]->top_field;
            (*list_size)++;
        }
    }
    if (currStructure == BOTTOM_FIELD) {
        for (i = 0; i < list_idx; i++) {
            list[(RK_S32)(*list_size)] = fs_list[i]->bottom_field;
            (*list_size)++;
        }
    }
}

static RK_S32 compare_pic_by_poc_desc(const void *arg1, const void *arg2)
{
    RK_S32 poc1 = (*(H264_StorePic_t**)arg1)->poc;
    RK_S32 poc2 = (*(H264_StorePic_t**)arg2)->poc;

    if (poc1 < poc2)
        return 1;
    if (poc1 > poc2)
        return -1;
    else
        return 0;
}

static RK_S32 compare_pic_by_poc_asc(const void *arg1, const void *arg2)
{
    RK_S32 poc1 = (*(H264_StorePic_t**)arg1)->poc;
    RK_S32 poc2 = (*(H264_StorePic_t**)arg2)->poc;

    if (poc1 < poc2)
        return -1;
    if (poc1 > poc2)
        return 1;
    else
        return 0;
}

static RK_S32 compare_fs_by_poc_desc(const void *arg1, const void *arg2)
{
    RK_S32 poc1 = (*(H264_FrameStore_t**)arg1)->poc;
    RK_S32 poc2 = (*(H264_FrameStore_t**)arg2)->poc;

    if (poc1 < poc2)
        return 1;
    else if (poc1 > poc2)
        return -1;
    else
        return 0;
}

static RK_S32 compare_fs_by_poc_asc(const void *arg1, const void *arg2)
{
    RK_S32 poc1 = (*(H264_FrameStore_t**)arg1)->poc;
    RK_S32 poc2 = (*(H264_FrameStore_t**)arg2)->poc;

    if (poc1 < poc2)
        return -1;
    else if (poc1 > poc2)
        return 1;
    else
        return 0;
}

static MPP_RET init_lists_p_slice_mvc(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    RK_S32 list0idx = 0;
    RK_S32 listltidx = 0;
    H264_FrameStore_t **fs_list0 = 0;
    H264_FrameStore_t **fs_listlt = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;
    RK_S32 currPOC = currSlice->ThisPOC;
    RK_S32 anchor_pic_flag = currSlice->anchor_pic_flag;

    currSlice->listXsizeP[0] = 0;
    currSlice->listXsizeP[1] = 0;
    currSlice->listinterviewidx0 = 0;
    currSlice->listinterviewidx1 = 0;

    if (currSlice->structure == FRAME) {
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used == 3) {
                if ((p_Dpb->fs_ref[i]->frame->used_for_reference) && (!p_Dpb->fs_ref[i]->frame->is_long_term)) {
                    currSlice->listP[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
                }
            }
        }
        // order list 0 by PicNum
        qsort((void *)currSlice->listP[0], list0idx, sizeof(H264_StorePic_t*), compare_pic_by_pic_num_desc);
        currSlice->listXsizeP[0] = (RK_U8)list0idx;
        // long term handling
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ltref[i]->is_used == 3) {
                if (p_Dpb->fs_ltref[i]->frame->is_long_term) {
                    currSlice->listP[0][list0idx++] = p_Dpb->fs_ltref[i]->frame;
                }
            }
        }
        qsort((void *)&currSlice->listP[0][(RK_S16)currSlice->listXsizeP[0]],
              list0idx - currSlice->listXsizeP[0], sizeof(H264_StorePic_t*), compare_pic_by_lt_pic_num_asc);
        currSlice->listXsizeP[0] = (RK_U8)list0idx;
    } else {
        fs_list0  = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        fs_listlt = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        MEM_CHECK(ret, fs_list0 && fs_listlt);
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_reference) {
                fs_list0[list0idx++] = p_Dpb->fs_ref[i];
            }
        }
        qsort((void *)fs_list0, list0idx, sizeof(H264_FrameStore_t*), compare_fs_by_frame_num_desc);
        currSlice->listXsizeP[0] = 0;
        gen_pic_list_from_frame_list(currSlice->structure, fs_list0, list0idx, currSlice->listP[0], &currSlice->listXsizeP[0], 0);
        // long term handling
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            fs_listlt[listltidx++] = p_Dpb->fs_ltref[i];
        }
        qsort((void *)fs_listlt, listltidx, sizeof(H264_FrameStore_t*), compare_fs_by_lt_pic_idx_asc);
        gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listP[0], &currSlice->listXsizeP[0], 1);
        MPP_FREE(fs_list0);
        MPP_FREE(fs_listlt);
    }

    currSlice->listXsizeP[1] = 0;
    if (currSlice->svc_extension_flag == 0) {
        RK_S32 curr_layer_id = currSlice->layer_id;
        currSlice->fs_listinterview0 = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        MEM_CHECK(ret, currSlice->fs_listinterview0);
        list0idx = currSlice->listXsizeP[0];
        if (currSlice->structure == FRAME) {
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], 0, 0,
                                                  currSlice->fs_listinterview0, &currSlice->listinterviewidx0, currPOC, curr_layer_id, anchor_pic_flag));
            for (i = 0; i < (RK_U32)currSlice->listinterviewidx0; i++) {
                currSlice->listP[0][list0idx++] = currSlice->fs_listinterview0[i]->frame;
            }
            currSlice->listXsizeP[0] = (RK_U8)list0idx;
        } else {
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], currSlice->structure, 0,
                                                  currSlice->fs_listinterview0, &currSlice->listinterviewidx0, currPOC, curr_layer_id, anchor_pic_flag));
            gen_pic_list_from_frame_interview_list(currSlice->structure, currSlice->fs_listinterview0,
                                                   currSlice->listinterviewidx0, currSlice->listP[0], &currSlice->listXsizeP[0]);
        }
    }
    // set the unused list entries to NULL
    for (i = currSlice->listXsizeP[0]; i < (MAX_LIST_SIZE); i++) {
        currSlice->listP[0][i] = p_Vid->no_ref_pic;
    }
    for (i = currSlice->listXsizeP[1]; i < (MAX_LIST_SIZE); i++) {
        currSlice->listP[1][i] = p_Vid->no_ref_pic;
    }
    MPP_FREE(currSlice->fs_listinterview0);

    return ret = MPP_OK;
__FAILED:
    MPP_FREE(fs_list0);
    MPP_FREE(fs_listlt);
    MPP_FREE(currSlice->fs_listinterview0);

    return ret;
}

static MPP_RET init_lists_b_slice_mvc(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    RK_S32 j = 0;
    RK_S32 list0idx = 0;
    RK_S32 list0idx_1 = 0;
    RK_S32 listltidx = 0;
    H264_FrameStore_t **fs_list0 = NULL;
    H264_FrameStore_t **fs_list1 = NULL;
    H264_FrameStore_t **fs_listlt = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;
    RK_S32 currPOC = currSlice->ThisPOC;
    RK_S32 anchor_pic_flag = currSlice->anchor_pic_flag;

    currSlice->listXsizeB[0] = 0;
    currSlice->listXsizeB[1] = 0;
    currSlice->listinterviewidx0 = 0;
    currSlice->listinterviewidx1 = 0;
    // B-Slice
    if (currSlice->structure == FRAME) {
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used == 3) {
                if ((p_Dpb->fs_ref[i]->frame->used_for_reference) && (!p_Dpb->fs_ref[i]->frame->is_long_term)) {
                    if (currSlice->framepoc >= p_Dpb->fs_ref[i]->frame->poc) {
                        currSlice->listB[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
                    }
                }
            }
        }
        qsort((void *)currSlice->listB[0], list0idx, sizeof(H264_StorePic_t*), compare_pic_by_poc_desc);
        list0idx_1 = list0idx;
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used == 3) {
                if ((p_Dpb->fs_ref[i]->frame->used_for_reference) && (!p_Dpb->fs_ref[i]->frame->is_long_term)) {
                    if (currSlice->framepoc < p_Dpb->fs_ref[i]->frame->poc) {
                        currSlice->listB[0][list0idx++] = p_Dpb->fs_ref[i]->frame;
                    }
                }
            }
        }
        qsort((void *)&currSlice->listB[0][list0idx_1], list0idx - list0idx_1, sizeof(H264_StorePic_t*), compare_pic_by_poc_asc);

        for (j = 0; j < list0idx_1; j++) {
            currSlice->listB[1][list0idx - list0idx_1 + j] = currSlice->listB[0][j];
        }
        for (j = list0idx_1; j < list0idx; j++) {
            currSlice->listB[1][j - list0idx_1] = currSlice->listB[0][j];
        }
        currSlice->listXsizeB[0] = currSlice->listXsizeB[1] = (RK_U8)list0idx;
        // long term handling
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ltref[i]->is_used == 3) {
                if (p_Dpb->fs_ltref[i]->frame->is_long_term) {
                    currSlice->listB[0][list0idx] = p_Dpb->fs_ltref[i]->frame;
                    currSlice->listB[1][list0idx++] = p_Dpb->fs_ltref[i]->frame;
                }
            }
        }
        qsort((void *)&currSlice->listB[0][(RK_S16)currSlice->listXsizeB[0]],
              list0idx - currSlice->listXsizeB[0], sizeof(H264_StorePic_t*), compare_pic_by_lt_pic_num_asc);
        qsort((void *)&currSlice->listB[1][(RK_S16)currSlice->listXsizeB[0]],
              list0idx - currSlice->listXsizeB[0], sizeof(H264_StorePic_t*), compare_pic_by_lt_pic_num_asc);
        currSlice->listXsizeB[0] = currSlice->listXsizeB[1] = (RK_U8)list0idx;
    } else {
        fs_list0  = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        fs_list1  = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        fs_listlt = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        MEM_CHECK(ret, fs_list0 && fs_list1 && fs_listlt);
        currSlice->listXsizeB[0] = 0;
        currSlice->listXsizeB[1] = 1;
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used) {
                if (currSlice->ThisPOC >= p_Dpb->fs_ref[i]->poc) {
                    fs_list0[list0idx++] = p_Dpb->fs_ref[i];
                }
            }
        }
        qsort((void *)fs_list0, list0idx, sizeof(H264_FrameStore_t*), compare_fs_by_poc_desc);
        list0idx_1 = list0idx;
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_used) {
                if (currSlice->ThisPOC < p_Dpb->fs_ref[i]->poc) {
                    fs_list0[list0idx++] = p_Dpb->fs_ref[i];
                }
            }
        }
        qsort((void *)&fs_list0[list0idx_1], list0idx - list0idx_1, sizeof(H264_FrameStore_t*), compare_fs_by_poc_asc);

        for (j = 0; j < list0idx_1; j++) {
            fs_list1[list0idx - list0idx_1 + j] = fs_list0[j];
        }
        for (j = list0idx_1; j < list0idx; j++) {
            fs_list1[j - list0idx_1] = fs_list0[j];
        }
        currSlice->listXsizeB[0] = 0;
        currSlice->listXsizeB[1] = 0;
        gen_pic_list_from_frame_list(currSlice->structure, fs_list0, list0idx, currSlice->listB[0], &currSlice->listXsizeB[0], 0);
        gen_pic_list_from_frame_list(currSlice->structure, fs_list1, list0idx, currSlice->listB[1], &currSlice->listXsizeB[1], 0);

        // long term handling
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            fs_listlt[listltidx++] = p_Dpb->fs_ltref[i];
        }
        qsort((void *)fs_listlt, listltidx, sizeof(H264_FrameStore_t*), compare_fs_by_lt_pic_idx_asc);

        gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listB[0], &currSlice->listXsizeB[0], 1);
        gen_pic_list_from_frame_list(currSlice->structure, fs_listlt, listltidx, currSlice->listB[1], &currSlice->listXsizeB[1], 1);

        MPP_FREE(fs_list0);
        MPP_FREE(fs_list1);
        MPP_FREE(fs_listlt);
    }
    if ((currSlice->listXsizeB[0] == currSlice->listXsizeB[1]) && (currSlice->listXsizeB[0] > 1)) {
        // check if lists are identical, if yes swap first two elements of currSlice->listX[1]
        RK_S32 diff = 0;
        for (j = 0; j < currSlice->listXsizeB[0]; j++) {
            if (currSlice->listB[0][j] != currSlice->listB[1][j]) {
                diff = 1;
                break;
            }
        }
        if (!diff) {
            H264_StorePic_t *tmp_s = currSlice->listB[1][0];
            currSlice->listB[1][0] = currSlice->listB[1][1];
            currSlice->listB[1][1] = tmp_s;
        }
    }
    if (currSlice->svc_extension_flag == 0) {
        RK_S32 curr_layer_id = currSlice->layer_id;
        // B-Slice
        currSlice->fs_listinterview0 = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        currSlice->fs_listinterview1 = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
        MEM_CHECK(ret, currSlice->fs_listinterview0 && currSlice->fs_listinterview1);
        list0idx = currSlice->listXsizeB[0];
        if (currSlice->structure == FRAME) {
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], 0, 0,
                                                  currSlice->fs_listinterview0, &currSlice->listinterviewidx0, currPOC, curr_layer_id, anchor_pic_flag));
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], 0, 1,
                                                  currSlice->fs_listinterview1, &currSlice->listinterviewidx1, currPOC, curr_layer_id, anchor_pic_flag));

            for (i = 0; i < (RK_U32)currSlice->listinterviewidx0; i++) {
                currSlice->listB[0][list0idx++] = currSlice->fs_listinterview0[i]->frame;
            }
            currSlice->listXsizeB[0] = (RK_U8)list0idx;
            list0idx = currSlice->listXsizeB[1];
            for (i = 0; i < (RK_U32)currSlice->listinterviewidx1; i++) {
                currSlice->listB[1][list0idx++] = currSlice->fs_listinterview1[i]->frame;
            }
            currSlice->listXsizeB[1] = (RK_U8)list0idx;
        } else {
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], currSlice->structure, 0,
                                                  currSlice->fs_listinterview0, &currSlice->listinterviewidx0, currPOC, curr_layer_id, anchor_pic_flag));
            gen_pic_list_from_frame_interview_list(currSlice->structure, currSlice->fs_listinterview0,
                                                   currSlice->listinterviewidx0, currSlice->listB[0], &currSlice->listXsizeB[0]);
            FUN_CHECK(ret = append_interview_list(p_Vid->p_Dpb_layer[1], currSlice->structure, 1,
                                                  currSlice->fs_listinterview1, &currSlice->listinterviewidx1, currPOC, curr_layer_id, anchor_pic_flag));
            gen_pic_list_from_frame_interview_list(currSlice->structure, currSlice->fs_listinterview1,
                                                   currSlice->listinterviewidx1, currSlice->listB[1], &currSlice->listXsizeB[1]);
        }
    }
    // set the unused list entries to NULL
    for (i = currSlice->listXsizeB[0]; i < (MAX_LIST_SIZE); i++) {
        currSlice->listB[0][i] = p_Vid->no_ref_pic;
    }
    for (i = currSlice->listXsizeB[1]; i < (MAX_LIST_SIZE); i++) {
        currSlice->listB[1][i] = p_Vid->no_ref_pic;
    }
    MPP_FREE(currSlice->fs_listinterview0);
    MPP_FREE(currSlice->fs_listinterview1);

    return ret = MPP_OK;
__FAILED:
    MPP_FREE(fs_list0);
    MPP_FREE(fs_list1);
    MPP_FREE(fs_listlt);
    MPP_FREE(currSlice->fs_listinterview0);
    MPP_FREE(currSlice->fs_listinterview1);

    return ret;
}

static RK_U32 get_short_term_pic(H264_SLICE_t *currSlice, RK_S32 picNum, H264_StorePic_t **find_pic)
{
    RK_U32 i = 0;
    H264_StorePic_t *ret_pic = NULL;
    H264_StorePic_t *near_pic = NULL;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;

    for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
        if (currSlice->structure == FRAME) {
            if ((p_Dpb->fs_ref[i]->is_reference == 3)
                && (!p_Dpb->fs_ref[i]->frame->is_long_term)) {
                if (p_Dpb->fs_ref[i]->frame->pic_num == picNum) {
                    ret_pic = p_Dpb->fs_ref[i]->frame;
                    break;
                } else {
                    near_pic = p_Dpb->fs_ref[i]->frame;
                }
            }
        } else {
            if ((p_Dpb->fs_ref[i]->is_reference & 1)
                && (!p_Dpb->fs_ref[i]->top_field->is_long_term)) {
                if (p_Dpb->fs_ref[i]->top_field->pic_num == picNum) {
                    ret_pic = p_Dpb->fs_ref[i]->top_field;
                    break;
                } else {
                    near_pic = p_Dpb->fs_ref[i]->top_field;
                }
            }
            if ((p_Dpb->fs_ref[i]->is_reference & 2)
                && (!p_Dpb->fs_ref[i]->bottom_field->is_long_term)) {
                if (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNum) {
                    ret_pic = p_Dpb->fs_ref[i]->bottom_field;
                    break;
                } else {
                    near_pic = p_Dpb->fs_ref[i]->bottom_field;
                }
            }
        }
    }
    *find_pic = ret_pic ? ret_pic : near_pic;
    return (ret_pic ? 1 : 0);
}


static H264_StorePic_t *get_long_term_pic(H264_SLICE_t *currSlice, RK_S32 LongtermPicNum)
{
    RK_U32 i = 0;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;

    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
        if (currSlice->structure == FRAME) {
            if (p_Dpb->fs_ltref[i]->is_reference == 3)
                if ((p_Dpb->fs_ltref[i]->frame->is_long_term)
                    && (p_Dpb->fs_ltref[i]->frame->long_term_pic_num == LongtermPicNum))
                    return p_Dpb->fs_ltref[i]->frame;
        } else {
            if (p_Dpb->fs_ltref[i]->is_reference & 1)
                if ((p_Dpb->fs_ltref[i]->top_field->is_long_term)
                    && (p_Dpb->fs_ltref[i]->top_field->long_term_pic_num == LongtermPicNum))
                    return p_Dpb->fs_ltref[i]->top_field;
            if (p_Dpb->fs_ltref[i]->is_reference & 2)
                if ((p_Dpb->fs_ltref[i]->bottom_field->is_long_term)
                    && (p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num == LongtermPicNum))
                    return p_Dpb->fs_ltref[i]->bottom_field;
        }
    }
    return NULL;
}
static RK_U32 check_ref_pic_list(H264_SLICE_t *currSlice, RK_S32 cur_list)
{
    RK_S32 i = 0;
    RK_U32 dpb_error_flag = 0;
    RK_S32 maxPicNum = 0, currPicNum = 0;
    RK_S32 picNumLXNoWrap = 0, picNumLXPred = 0, picNumLX = 0;

    RK_S32 *modification_of_pic_nums_idc = currSlice->modification_of_pic_nums_idc[cur_list];
    RK_S32 *abs_diff_pic_num_minus1 = currSlice->abs_diff_pic_num_minus1[cur_list];
    RK_S32 *long_term_pic_idx = currSlice->long_term_pic_idx[cur_list];
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;

    if (currSlice->structure == FRAME) {
        maxPicNum  = p_Vid->max_frame_num;
        currPicNum = currSlice->frame_num;
    } else {
        maxPicNum  = 2 * p_Vid->max_frame_num;
        currPicNum = 2 * currSlice->frame_num + 1;
    }
    picNumLXPred = currPicNum;
    for (i = 0; modification_of_pic_nums_idc[i] != 3 && i < MAX_REORDER_TIMES; i++) {
        H264_StorePic_t *tmp = NULL;
        RK_U32 error_flag = 0;
        if (modification_of_pic_nums_idc[i] > 3)
            continue;
        if (modification_of_pic_nums_idc[i] < 2) {
            if (modification_of_pic_nums_idc[i] == 0) {
                if ( (picNumLXPred - (abs_diff_pic_num_minus1[i] + 1)) < 0)
                    picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1[i] + 1) + maxPicNum;
                else
                    picNumLXNoWrap = picNumLXPred - (abs_diff_pic_num_minus1[i] + 1);
            } else { //!< (modification_of_pic_nums_idc[i] == 1)
                if (picNumLXPred + (abs_diff_pic_num_minus1[i] + 1) >= maxPicNum)
                    picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1[i] + 1) - maxPicNum;
                else
                    picNumLXNoWrap = picNumLXPred + (abs_diff_pic_num_minus1[i] + 1);
            }
            picNumLXPred = picNumLXNoWrap;
            picNumLX = (picNumLXNoWrap > currPicNum) ? (picNumLXNoWrap - maxPicNum) : picNumLXNoWrap;
            error_flag = 1;
            if (get_short_term_pic(currSlice, picNumLX, &tmp)) { //!< find short reference
                MppFrame mframe = NULL;
                H264D_DBG(H264D_DBG_DPB_REF_ERR, "find short reference, slot_idx=%d.\n", tmp->mem_mark->slot_idx);
                if (tmp && tmp->mem_mark) {
                    mpp_buf_slot_get_prop(p_Vid->p_Dec->frame_slots, tmp->mem_mark->slot_idx, SLOT_FRAME_PTR, &mframe);
                    if (mframe && !mpp_frame_get_errinfo(mframe)) {
                        error_flag = 0;
                    }
                }
            }

        } else { //!< (modification_of_pic_nums_idc[i] == 2)
            tmp = get_long_term_pic(currSlice, long_term_pic_idx[i]);
        }
        dpb_error_flag |= error_flag;
    }

    return dpb_error_flag;
}

static RK_U32 check_ref_dbp_err(H264_DecCtx_t *p_Dec, H264_RefPicInfo_t *pref, RK_U32 active_refs)
{
    RK_U32 i = 0;
    RK_U32 dpb_error_flag = 0;

    for (i = 0; i < MAX_REF_SIZE; i++) {
        if (pref[i].valid) {
            MppFrame mframe = NULL;
            RK_S32 slot_idx = p_Dec->dpb_info[pref[i].dpb_idx].slot_index;
            if (slot_idx < 0) {
                dpb_error_flag |= 1;
                break;
            }
            mpp_buf_slot_get_prop(p_Dec->frame_slots, slot_idx, SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                if (i < active_refs) {
                    dpb_error_flag |= mpp_frame_get_errinfo(mframe);
                }
                H264D_DBG(H264D_DBG_DPB_REF_ERR, "[DPB_REF_ERR] slot_idx=%d, dpb_err[%d]=%d", slot_idx, i, mpp_frame_get_errinfo(mframe));
            }
        }
    }
    return dpb_error_flag;
}

static void check_refer_picture_lists(H264_SLICE_t *currSlice)
{
    H264_DecCtx_t *p_Dec = currSlice->p_Dec;
    H264dErrCtx_t *p_err = &p_Dec->errctx;

    if (I_SLICE == currSlice->slice_type) {
        p_err->dpb_err_flag = 0;
        return;
    }
#if 1

    if ((currSlice->slice_type % 5) != I_SLICE
        && (currSlice->slice_type % 5) != SI_SLICE) {
        if (currSlice->ref_pic_list_reordering_flag[LIST_0]) {
            p_err->cur_err_flag |= check_ref_pic_list(currSlice, 0) ? 1 : 0;
        } else {
            RK_S32 pps_refs = currSlice->active_pps->num_ref_idx_l0_default_active_minus1 + 1;
            RK_S32 over_flag = currSlice->num_ref_idx_override_flag;
            RK_S32 active_l0 = over_flag ? currSlice->num_ref_idx_active[LIST_0] : pps_refs;
            p_err->cur_err_flag |= check_ref_dbp_err(p_Dec, p_Dec->refpic_info_p, active_l0) ? 1 : 0;
            H264D_DBG(H264D_DBG_DPB_REF_ERR, "list0 dpb: cur_err_flag=%d, pps_refs=%d, over_flag=%d, num_ref_l0=%d\n",
                      p_err->cur_err_flag, pps_refs, over_flag, active_l0);
        }
    }
    if (currSlice->slice_type % 5 == B_SLICE) {
        if (currSlice->ref_pic_list_reordering_flag[LIST_1]) {
            p_err->cur_err_flag |= check_ref_pic_list(currSlice, 1) ? 1 : 0;
        } else {
            RK_S32 pps_refs = currSlice->active_pps->num_ref_idx_l1_default_active_minus1 + 1;
            RK_S32 over_flag = currSlice->num_ref_idx_override_flag;
            RK_S32 active_l1 = over_flag ? currSlice->num_ref_idx_active[LIST_1] : pps_refs;
            p_err->cur_err_flag |= check_ref_dbp_err(p_Dec, p_Dec->refpic_info_b[1], active_l1) ? 1 : 0;
            H264D_DBG(H264D_DBG_DPB_REF_ERR, "list1 dpb: cur_err_flag=%d, pps_refs=%d, over_flag=%d, num_ref_l1=%d\n",
                      p_err->cur_err_flag, pps_refs, over_flag, active_l1);
        }
        //!< B_SLICE only has one refer
        if ((currSlice->active_sps->vui_seq_parameters.num_reorder_frames > 1)
            && (currSlice->p_Dpb->ref_frames_in_buffer < 2)) {
            p_err->cur_err_flag |= 1;
            H264D_DBG(H264D_DBG_DPB_REF_ERR, "[DPB_REF_ERR] error, B frame only has one refer");
        }
    }

#endif

}

static void reset_dpb_info(H264_DpbInfo_t *p)
{
    p->refpic = NULL;
    p->TOP_POC = 0;
    p->BOT_POC = 0;
    p->field_flag = 0;
    p->slot_index = -1;
    p->colmv_is_used = 0;
    p->frame_num = 0;
    p->is_long_term = 0;
    p->long_term_pic_num = 0;
    p->voidx = 0;
    p->view_id = 0;
    p->is_used = 0;
}

static MPP_RET prepare_init_dpb_info(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0, j = 0;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;
    H264_DecCtx_t *p_Dec = currSlice->p_Dec;

    //!< reset parameters
    for (i = 0; i < MAX_DPB_SIZE; i++) {
        reset_dpb_info(&p_Dec->dpb_info[i]);
    }
    //!< reference
#if 1
    for (i = 0, j = 0; j < p_Dpb->ref_frames_in_buffer; i++, j++) {
        if (p_Dpb->fs_ref[j]->is_used == 3) {
            p_Dec->dpb_info[i].refpic = p_Dpb->fs_ref[j]->frame;
            if (p_Dpb->fs_ref[j]->frame->iCodingType == FIELD_CODING) {
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ref[j]->bottom_field->poc;
            } else {
                if (p_Dpb->fs_ref[j]->frame->frame_poc != p_Dpb->fs_ref[j]->frame->poc) {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ref[j]->frame->top_poc - p_Dpb->fs_ref[j]->frame->frame_poc;
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ref[j]->frame->bottom_poc - p_Dpb->fs_ref[j]->frame->frame_poc;
                } else {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ref[j]->frame->top_poc;
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ref[j]->frame->bottom_poc;
                }
            }
            p_Dec->dpb_info[i].field_flag = p_Dpb->fs_ref[j]->frame->iCodingType == FIELD_CODING;
            p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ref[j]->frame->mem_mark->slot_idx;
            p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ref[j]->frame->colmv_no_used_flag ? 0 : 1);
        } else if (p_Dpb->fs_ref[j]->is_used) {
            if (p_Dpb->fs_ref[j]->is_used & 0x1) { // top
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ref[j]->top_field;

                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = 0;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ref[j]->top_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ref[j]->top_field->colmv_no_used_flag ? 0 : 1);
            } else { // if(p_Dpb->fs_ref[j]->is_used & 0x2) // bottom
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ref[j]->bottom_field;
                p_Dec->dpb_info[i].TOP_POC = 0;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ref[j]->bottom_field->poc;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ref[j]->bottom_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ref[j]->bottom_field->colmv_no_used_flag ? 0 : 1);
            }
        }
        p_Dec->dpb_info[i].frame_num = p_Dpb->fs_ref[j]->frame_num;
        p_Dec->dpb_info[i].is_long_term = 0;
        p_Dec->dpb_info[i].long_term_pic_num = 0;
        p_Dec->dpb_info[i].long_term_frame_idx = 0;
        p_Dec->dpb_info[i].voidx = p_Dpb->fs_ref[j]->layer_id;
        p_Dec->dpb_info[i].view_id = p_Dpb->fs_ref[j]->view_id;
        p_Dec->dpb_info[i].is_used = p_Dpb->fs_ref[j]->is_used;
    }
#endif
    //!<---- long term reference
#if 1
    for (j = 0; j < p_Dpb->ltref_frames_in_buffer; i++, j++) {
        if (p_Dpb->fs_ltref[j]->is_used == 3) {
            p_Dec->dpb_info[i].refpic = p_Dpb->fs_ltref[j]->frame;

            if (p_Dpb->fs_ltref[j]->frame->iCodingType == FIELD_CODING) {
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ltref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ltref[j]->bottom_field->poc;
            } else {
                if (p_Dpb->fs_ltref[j]->frame->frame_poc != p_Dpb->fs_ltref[j]->frame->poc) {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ltref[j]->frame->top_poc - p_Dpb->fs_ltref[j]->frame->frame_poc;
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ltref[j]->frame->bottom_poc - p_Dpb->fs_ltref[j]->frame->frame_poc;
                } else {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ltref[j]->frame->top_poc;
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ltref[j]->frame->bottom_poc;
                }

            }
            p_Dec->dpb_info[i].field_flag = p_Dpb->fs_ltref[j]->frame->iCodingType == FIELD_CODING;
            p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ltref[j]->frame->mem_mark->slot_idx;
            p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ltref[j]->frame->colmv_no_used_flag ? 0 : 1);
            p_Dec->dpb_info[i].long_term_pic_num = p_Dpb->fs_ltref[j]->frame->long_term_pic_num;
        } else if (p_Dpb->fs_ltref[j]->is_used) {
            if (p_Dpb->fs_ltref[j]->is_used & 0x1) {
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ltref[j]->top_field;
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ltref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = 0;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ltref[j]->top_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ltref[j]->top_field->colmv_no_used_flag ? 0 : 1);
                p_Dec->dpb_info[i].long_term_pic_num = p_Dpb->fs_ltref[j]->top_field->long_term_pic_num;
            } else { // if(p_Dpb->fs_ref[j]->is_used & 0x2)
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ltref[j]->bottom_field;
                p_Dec->dpb_info[i].TOP_POC = 0;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ltref[j]->bottom_field->poc;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ltref[j]->bottom_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ltref[j]->bottom_field->colmv_no_used_flag ? 0 : 1);
                p_Dec->dpb_info[i].long_term_pic_num = p_Dpb->fs_ltref[j]->bottom_field->long_term_pic_num;
            }
        }
        p_Dec->dpb_info[i].frame_num = p_Dpb->fs_ltref[j]->long_term_frame_idx; //long term use long_term_frame_idx
        p_Dec->dpb_info[i].is_long_term = 1;
        p_Dec->dpb_info[i].long_term_frame_idx = p_Dpb->fs_ltref[j]->long_term_frame_idx;
        p_Dec->dpb_info[i].voidx = p_Dpb->fs_ltref[j]->layer_id;
        p_Dec->dpb_info[i].view_id = p_Dpb->fs_ltref[j]->view_id;
        p_Dec->dpb_info[i].is_used = p_Dpb->fs_ltref[j]->is_used;
    }
#endif
    //!< inter-layer reference (for multi-layered codecs)
#if 1
    for (j = 0; j < p_Dpb->used_size_il; i++, j++) {
        if (currSlice->structure == FRAME && p_Dpb->fs_ilref[j]->is_used == 3) {
            if (p_Dpb->fs_ilref[j]->inter_view_flag[0] == 0 && p_Dpb->fs_ilref[j]->inter_view_flag[1] == 0)
                break;
            p_Dec->dpb_info[i].refpic = p_Dpb->fs_ilref[j]->frame;

            if (p_Dpb->fs_ilref[j]->frame->is_mmco_5) {
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->frame->top_poc_mmco5;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->frame->bot_poc_mmco5;
            } else {
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->frame->top_poc;
                p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->frame->bottom_poc;
            }
            p_Dec->dpb_info[i].field_flag = p_Dpb->fs_ilref[j]->frame->iCodingType == FIELD_CODING;
            p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ilref[j]->frame->mem_mark->slot_idx;
            p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ilref[j]->frame->colmv_no_used_flag ? 0 : 1);
        } else if (currSlice->structure != FRAME && p_Dpb->fs_ilref[j]->is_used) {
            if (p_Dpb->fs_ilref[j]->is_used == 0x3) {
                if (p_Dpb->fs_ilref[j]->inter_view_flag[currSlice->structure == BOTTOM_FIELD] == 0)
                    break;
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ilref[j]->top_field;

                if (p_Dpb->fs_ilref[j]->top_field->is_mmco_5) {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->top_field->top_poc_mmco5;
                } else {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->top_field->top_poc;
                }
                if (p_Dpb->fs_ilref[j]->bottom_field->is_mmco_5) {
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->bottom_field->bot_poc_mmco5;
                } else {
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->bottom_field->bottom_poc;
                }
                p_Dec->dpb_info[i].field_flag = p_Dpb->fs_ilref[j]->frame->iCodingType == FIELD_CODING;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ilref[j]->frame->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ilref[j]->frame->colmv_no_used_flag ? 0 : 1);
            }
            if (p_Dpb->fs_ilref[j]->is_used & 0x1) {
                if (p_Dpb->fs_ilref[j]->inter_view_flag[0] == 0)
                    break;
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ilref[j]->top_field;

                if (p_Dpb->fs_ilref[j]->top_field->is_mmco_5) {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->top_field->top_poc_mmco5;
                } else {
                    p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ilref[j]->top_field->top_poc;
                }
                p_Dec->dpb_info[i].BOT_POC = 0;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ilref[j]->top_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ilref[j]->top_field->colmv_no_used_flag ? 0 : 1);
            } else { // if(p_Dpb->fs_ref[j]->is_used & 0x2)
                if (p_Dpb->fs_ilref[j]->inter_view_flag[1] == 0)
                    break;
                p_Dec->dpb_info[i].refpic = p_Dpb->fs_ilref[j]->bottom_field;

                p_Dec->dpb_info[i].TOP_POC = 0;
                if (p_Dpb->fs_ilref[j]->bottom_field->is_mmco_5) {
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->bottom_field->bot_poc_mmco5;
                } else {
                    p_Dec->dpb_info[i].BOT_POC = p_Dpb->fs_ilref[j]->bottom_field->bottom_poc;
                }
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ilref[j]->bottom_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ilref[j]->bottom_field->colmv_no_used_flag ? 0 : 1);
            }
        }
        p_Dec->dpb_info[i].frame_num = p_Dpb->fs_ilref[j]->frame_num;
        p_Dec->dpb_info[i].is_long_term = 0;//p_Dpb->fs_ilref[j]->is_long_term;
        p_Dec->dpb_info[i].is_ilt_flag = 1;
        p_Dec->dpb_info[i].long_term_pic_num = 0;
        p_Dec->dpb_info[i].long_term_frame_idx = 0;
        p_Dec->dpb_info[i].voidx = p_Dpb->fs_ilref[j]->layer_id;
        p_Dec->dpb_info[i].view_id = p_Dpb->fs_ilref[j]->view_id;
        p_Dec->dpb_info[i].is_used = p_Dpb->fs_ilref[j]->is_used;
    }
#endif

    return MPP_OK;
}

static MPP_RET prepare_init_ref_info(H264_SLICE_t *currSlice)
{
    void *refpic = NULL;
    RK_U32 i = 0, j = 0, k = 0;
    RK_S32 poc = 0, TOP_POC = 0, BOT_POC = 0;
    RK_S32 min_poc = 0, max_poc = 0;
    RK_S32 layer_id = 0, voidx = 0, is_used = 0, near_dpb_idx = 0;
    H264_DecCtx_t *p_Dec = currSlice->p_Dec;

    memset(p_Dec->refpic_info_p,    0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));
    memset(p_Dec->refpic_info_b[0], 0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));
    memset(p_Dec->refpic_info_b[1], 0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));

    if (currSlice->idr_flag && (currSlice->layer_id == 0)) { // idr_flag==1 && layer_id==0
        goto __RETURN;
    }
    //!<------ set listP -------
    near_dpb_idx = 0;
    for (j = 0; j < 32; j++) {
        poc = 0;
        layer_id = currSlice->listP[0][j]->layer_id;
        if (j < currSlice->listXsizeP[0]) {
            if (currSlice->listP[0][j]->structure == FRAME) {
                poc = (currSlice->listP[0][j]->is_mmco_5 && currSlice->layer_id && currSlice->listP[0][j]->layer_id == 0)
                      ? currSlice->listP[0][j]->poc_mmco5 : currSlice->listP[0][j]->poc;
            } else if (currSlice->listP[0][j]->structure == TOP_FIELD) {
                poc = (currSlice->listP[0][j]->is_mmco_5 && currSlice->layer_id && currSlice->listP[0][j]->layer_id == 0)
                      ? currSlice->listP[0][j]->top_poc_mmco5 : currSlice->listP[0][j]->top_poc;
            } else {
                poc = (currSlice->listP[0][j]->is_mmco_5 && currSlice->layer_id && currSlice->listP[0][j]->layer_id == 0)
                      ? currSlice->listP[0][j]->bot_poc_mmco5 : currSlice->listP[0][j]->bottom_poc;
            }
            for (i = 0; i < 16; i++) {
                refpic = p_Dec->dpb_info[i].refpic;
                TOP_POC = p_Dec->dpb_info[i].TOP_POC;
                BOT_POC = p_Dec->dpb_info[i].BOT_POC;
                voidx = p_Dec->dpb_info[i].voidx;
                is_used = p_Dec->dpb_info[i].is_used;
                if (currSlice->structure == FRAME && refpic) {
                    if (poc == MPP_MIN(TOP_POC, BOT_POC) && (layer_id == voidx))
                        break;
                } else {
                    if (is_used == 3) {
                        if ((poc == BOT_POC || poc == TOP_POC) && layer_id == voidx)
                            break;
                    } else if (is_used & 1) {
                        if (poc == TOP_POC && layer_id == voidx)
                            break;
                    } else if (is_used & 2) {
                        if (poc == BOT_POC && layer_id == voidx)
                            break;
                    }
                }
            }
            if (i < 16) {
                near_dpb_idx = i;
                p_Dec->refpic_info_p[j].dpb_idx = i;
            } else {
                ASSERT(near_dpb_idx >= 0);
                p_Dec->refpic_info_p[j].dpb_idx = near_dpb_idx;
                p_Dec->errctx.cur_err_flag |= 1;
            }
            if (currSlice->listP[0][j]->structure == BOTTOM_FIELD) {
                p_Dec->refpic_info_p[j].bottom_flag = 1;
            } else {
                p_Dec->refpic_info_p[j].bottom_flag = 0;
            }
            p_Dec->refpic_info_p[j].valid = 1;
        } else {
            p_Dec->refpic_info_p[j].valid = 0;
            p_Dec->refpic_info_p[j].dpb_idx = 0;
            p_Dec->refpic_info_p[j].bottom_flag = 0;
        }
    }
    //!<------  set listB -------
    for (k = 0; k < 2; k++) {
        min_poc =  0xFFFF;
        max_poc = -0xFFFF;
        near_dpb_idx = 0;
        for (j = 0; j < 32; j++) {
            poc = 0;
            layer_id = currSlice->listB[k][j]->layer_id;
            if (j < currSlice->listXsizeB[k]) {
                if (currSlice->listB[k][j]->structure == FRAME) {
                    poc = (currSlice->listB[k][j]->is_mmco_5 && currSlice->layer_id && currSlice->listB[k][j]->layer_id == 0)
                          ? currSlice->listB[k][j]->poc_mmco5 : currSlice->listB[k][j]->poc;
                } else if (currSlice->listB[k][j]->structure == TOP_FIELD) {
                    poc = (currSlice->listB[k][j]->is_mmco_5 && currSlice->layer_id && currSlice->listB[k][j]->layer_id == 0)
                          ? currSlice->listB[k][j]->top_poc_mmco5 : currSlice->listB[k][j]->top_poc;
                } else {
                    poc = (currSlice->listB[k][j]->is_mmco_5 && currSlice->layer_id && currSlice->listB[k][j]->layer_id == 0)
                          ? currSlice->listB[k][j]->bot_poc_mmco5 : currSlice->listB[k][j]->bottom_poc;
                }

                min_poc = MPP_MIN(min_poc, poc);
                max_poc = MPP_MAX(max_poc, poc);
                for (i = 0; i < 16; i++) {
                    refpic = p_Dec->dpb_info[i].refpic;
                    TOP_POC = p_Dec->dpb_info[i].TOP_POC;
                    BOT_POC = p_Dec->dpb_info[i].BOT_POC;
                    voidx = p_Dec->dpb_info[i].voidx;
                    is_used = p_Dec->dpb_info[i].is_used;
                    if (currSlice->structure == FRAME && refpic) {
                        if (poc == MPP_MIN(TOP_POC, BOT_POC) && (layer_id == voidx))
                            break;
                    } else {
                        if (is_used == 3) {
                            if ((poc == BOT_POC || poc == TOP_POC) && layer_id == voidx)
                                break;
                        } else if (is_used & 1) {
                            if (poc == TOP_POC && (layer_id == voidx || (layer_id != voidx && !currSlice->bottom_field_flag)))
                                break;
                        } else if (is_used & 2) {
                            if (poc == BOT_POC && (layer_id == voidx || (layer_id != voidx && currSlice->bottom_field_flag)))
                                break;
                        }
                    }
                }
                if (i < 16) {
                    near_dpb_idx = i;
                    p_Dec->refpic_info_b[k][j].dpb_idx = i;
                } else {
                    ASSERT(near_dpb_idx >= 0);
                    p_Dec->refpic_info_b[k][j].dpb_idx = near_dpb_idx;
                    p_Dec->errctx.cur_err_flag |= 1;
                }
                if (currSlice->listB[k][j]->structure == BOTTOM_FIELD) {
                    p_Dec->refpic_info_b[k][j].bottom_flag = 1;
                } else {
                    p_Dec->refpic_info_b[k][j].bottom_flag = 0;
                }
                p_Dec->refpic_info_b[k][j].valid = 1;
            } else {
                p_Dec->refpic_info_b[k][j].valid = 0;
                p_Dec->refpic_info_b[k][j].dpb_idx = 0;
                p_Dec->refpic_info_b[k][j].bottom_flag = 0;
            }
        }

    }
    //!< check dpb list poc
#if 0
    {
        RK_S32 cur_poc = p_Dec->p_Vid->dec_pic->poc;
        if ((currSlice->slice_type % 5) != I_SLICE
            && (currSlice->slice_type % 5) != SI_SLICE) {
            if (cur_poc < min_poc) {
                p_Dec->errctx.cur_err_flag |= 1;
                H264D_DBG(H264D_DBG_DPB_REF_ERR, "[DPB_REF_ERR] min_poc=%d, dec_poc=%d", min_poc, cur_poc);
            }
        }
        if (currSlice->slice_type % 5 == B_SLICE) {
            if (cur_poc > max_poc) {
                p_Dec->errctx.cur_err_flag |= 1;
                H264D_DBG(H264D_DBG_DPB_REF_ERR, "[DPB_REF_ERR] max_poc=%d, dec_poc=%d", max_poc, cur_poc);
            }
        }
    }
#endif
__RETURN:
    return MPP_OK;
}

static MPP_RET check_refer_dpb_buf_slots(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0;
    RK_S32 slot_idx = 0;
    RK_U32 dpb_used = 0;
    H264_DecCtx_t *p_Dec = NULL;
    H264_DpbMark_t *p_mark = NULL;

    p_Dec = currSlice->p_Dec;
    //!< set buf slot flag
    for (i = 0; i < MAX_DPB_SIZE; i++) {
        if ((NULL != p_Dec->dpb_info[i].refpic) && (p_Dec->dpb_info[i].slot_index >= 0)) {
            p_Dec->in_task->refer[i] = p_Dec->dpb_info[i].slot_index;
            mpp_buf_slot_set_flag(p_Dec->frame_slots, p_Dec->dpb_info[i].slot_index, SLOT_HAL_INPUT);
            mpp_buf_slot_set_flag(p_Dec->frame_slots, p_Dec->dpb_info[i].slot_index, SLOT_CODEC_USE);
        } else {
            p_Dec->in_task->refer[i] = -1;
        }
    }
    //!< dpb info
    if (rkv_h264d_parse_debug & H264D_DBG_DPB_INFO) {
        H264D_DBG(H264D_DBG_DPB_INFO, "[DPB_INFO] cur_slot_idx=%d", p_Dec->in_task->output);
        for (i = 0; i < MAX_DPB_SIZE; i++) {
            slot_idx = p_Dec->in_task->refer[i];
            if (slot_idx >= 0) {
                H264D_DBG(H264D_DBG_DPB_INFO, "[DPB_INFO] ref_slot_idx[%d]=%d", i, slot_idx);
            }
        }
    }
    //!< mark info
    for (i = 0; i < MAX_MARK_SIZE; i++) {
        p_mark = &p_Dec->dpb_mark[i];
        if (p_mark->out_flag && (p_mark->slot_idx >= 0)) {
            dpb_used++;
            if (rkv_h264d_parse_debug & H264D_DBG_DPB_INFO) {
                RK_S32 fd = 0;
                MppFrame mframe = NULL;
                MppBuffer mbuffer = NULL;
                mpp_buf_slot_get_prop(p_Dec->frame_slots, p_mark->slot_idx, SLOT_FRAME_PTR, &mframe);
                mbuffer = mframe ? mpp_frame_get_buffer(mframe) : NULL;
                fd = mbuffer ? mpp_buffer_get_fd(mbuffer) : 0xFF;
                H264D_DBG(H264D_DBG_DPB_INFO, "[DPB_MARK_INFO] slot_idx=%d, top_used=%d, bot_used=%d, out_flag=%d, fd=0x%02x",
                          p_mark->slot_idx, p_mark->top_used, p_mark->bot_used, p_mark->out_flag, fd);
            }
        }
    }
    H264D_DBG(H264D_DBG_DPB_INFO, "[DPB_MARK_INFO] ---------- cur_slot=%d --------------------", p_Dec->in_task->output);

    if (dpb_used > currSlice->p_Dpb->size + 2)  {
        H264D_ERR("[h264d_reset_error]");
        h264d_reset((void *)p_Dec);
        return MPP_NOK;
    }

    return MPP_OK;
}


/*!
***********************************************************************
* \brief
*    flush dpb buffer slot
***********************************************************************
*/
//extern "C"
void flush_dpb_buf_slot(H264_DecCtx_t *p_Dec)
{
    RK_U32 i = 0;
    H264_DpbMark_t *p_mark = NULL;

    for (i = 0; i < MAX_MARK_SIZE; i++) {
        p_mark = &p_Dec->dpb_mark[i];
        if (p_mark && p_mark->out_flag && (p_mark->slot_idx >= 0)) {
            MppFrame mframe = NULL;
            mpp_buf_slot_get_prop(p_Dec->frame_slots, p_mark->slot_idx, SLOT_FRAME_PTR, &mframe);
            if (mframe) {
                H264D_DBG(H264D_DBG_SLOT_FLUSH, "[DPB_BUF_FLUSH] slot_idx=%d, top_used=%d, bot_used=%d",
                          p_mark->slot_idx, p_mark->top_used, p_mark->bot_used);
                mpp_frame_set_discard(mframe, 1);
                mpp_buf_slot_set_flag(p_Dec->frame_slots, p_mark->slot_idx, SLOT_QUEUE_USE);
                mpp_buf_slot_enqueue(p_Dec->frame_slots, p_mark->slot_idx, QUEUE_DISPLAY);
                mpp_buf_slot_clr_flag(p_Dec->frame_slots, p_mark->slot_idx, SLOT_CODEC_USE);
                p_Dec->last_frame_slot_idx = p_mark->slot_idx;
            }
        }
        reset_dpb_mark(p_mark);
    }
}

/*!
***********************************************************************
* \brief
*    reset dpb mark
***********************************************************************
*/
//extern "C"
MPP_RET reset_dpb_mark(H264_DpbMark_t *p_mark)
{
    if (p_mark) {
        p_mark->top_used = 0;
        p_mark->bot_used = 0;
        p_mark->out_flag = 0;
        p_mark->slot_idx = -1;
        p_mark->pic = NULL;
        p_mark->mframe = NULL;
    }

    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*    init picture
***********************************************************************
*/
//extern "C"
MPP_RET init_picture(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec   = currSlice->p_Vid->p_Dec;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;
    H264dErrCtx_t *p_err   = &p_Dec->errctx;

    //!< discard stream before I_SLICE
    p_err->i_slice_no += ((!currSlice->layer_id) && (I_SLICE == currSlice->slice_type)) ? 1 : 0;
    if (!p_err->i_slice_no) {
        H264D_WARNNING("[Discard] Discard slice before I Slice. \n");
        ret = MPP_NOK;
        goto __FAILED;
    }
    FUN_CHECK(ret = alloc_decpic(currSlice));
    if ((p_err->i_slice_no < 2)
        && (!currSlice->layer_id) && (I_SLICE == currSlice->slice_type)) {
        p_err->first_iframe_poc = p_Vid->dec_pic->poc; //!< recoder first i frame poc
    }
    //!< idr_memory_management MVC_layer, idr_flag==1
    if (currSlice->layer_id && !currSlice->svc_extension_flag && !currSlice->mvcExt.non_idr_flag) {
        ASSERT(currSlice->layer_id == 1);
        FUN_CHECK(ret = idr_memory_management(p_Vid->p_Dpb_layer[currSlice->layer_id], p_Vid->dec_pic));
    }

    // if cur pic i frame poc & frame_num <= last pic, flush dpb.
    if (p_Vid->last_pic != NULL && p_Vid->dec_pic->poc != 0) {
        if (p_Vid->last_pic->frame_num >= p_Vid->dec_pic->frame_num
            && p_Vid->last_pic->poc >= p_Vid->dec_pic->poc
            && p_Vid->dec_pic->slice_type == I_SLICE
            && p_Vid->dec_pic->structure == 3) {
            if (currSlice->layer_id == 0) {
                FUN_CHECK(ret = flush_dpb(p_Vid->p_Dpb_layer[0], 1));
            } else {
                FUN_CHECK(ret = flush_dpb(p_Vid->p_Dpb_layer[1], 2));
            }
        }
    }
    update_ref_list(p_Vid->p_Dpb_layer[currSlice->layer_id]);
    update_ltref_list(p_Vid->p_Dpb_layer[currSlice->layer_id]);
    update_pic_num(currSlice);
    //!< reorder
    if (!currSlice->idr_flag || currSlice->layer_id) {
        FUN_CHECK(ret = init_lists_p_slice_mvc(currSlice));
        FUN_CHECK(ret = init_lists_b_slice_mvc(currSlice));
    }
    prepare_init_dpb_info(currSlice);
    prepare_init_ref_info(currSlice);

    FUN_CHECK(ret = check_refer_dpb_buf_slots(currSlice));
    check_refer_picture_lists(currSlice);

    prepare_init_scanlist(currSlice);
    fill_picparams(currSlice->p_Vid, &p_Dec->dxva_ctx->pp);
    fill_scanlist(currSlice->p_Vid, &p_Dec->dxva_ctx->qm);


    return ret = MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    update dpb
***********************************************************************
*/
//extern "C"
MPP_RET update_dpb(H264_DecCtx_t *p_Dec)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    p_Dec->p_Vid->exit_picture_flag = 1;
    p_Dec->p_Vid->have_outpicture_flag = 1;
    ret = exit_picture(p_Dec->p_Vid, &p_Dec->p_Vid->dec_pic);

    p_Dec->p_Vid->iNumOfSlicesDecoded = 0;
    p_Dec->p_Vid->exit_picture_flag = 0;

    return ret;
}

