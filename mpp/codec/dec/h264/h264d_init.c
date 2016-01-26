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

#include "mpp_mem.h"

#include "h264d_log.h"
#include "h264d_init.h"
#include "h264d_dpb.h"
#include "h264d_scalist.h"
#include "h264d_fill.h"


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
    ASSERT(0);
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
    ASSERT(0);
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
    ASSERT(0);
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

static void dpb_mark_malloc(H264dVideoCtx_t *p_Vid,  H264_StorePic_t *dec_pic)
{
    RK_U8 idx = 1;
    H264_DpbMark_t *cur_mark = NULL;
    RK_U32 hor_stride = 0, ver_stride = 0;
    H264_DecCtx_t *p_Dec = p_Vid->p_Dec;
    H264_DpbMark_t *p_mark = p_Vid->p_Dec->dpb_mark;
	RK_S32 structure = dec_pic->structure;
	RK_S32 layer_id = dec_pic->layer_id;
    if (!dec_pic->combine_flag) {
        while (p_mark[idx].out_flag || p_mark[idx].top_used || p_mark[idx].bot_used) {
            idx++;
        }
        ASSERT(idx <= MAX_MARK_SIZE);
        mpp_buf_slot_get_unused(p_Vid->p_Dec->frame_slots, &p_mark[idx].slot_idx);
        cur_mark = &p_mark[idx];
        cur_mark->out_flag = 1;
        if (p_Vid->g_framecnt == 255) {
            idx = idx;
        }
        H264D_LOG("[extra_data][Malloc] lay_id=%d, g_framecnt=%d, mark_idx=%d, slot_idx=%d, pts=%lld \n", layer_id,
                  p_Vid->g_framecnt, cur_mark->mark_idx, cur_mark->slot_idx, p_Vid->p_Inp->in_pts);
        if ((YUV420 == p_Vid->yuv_format) && (8 == p_Vid->bit_depth_luma)) {
            mpp_frame_set_fmt(cur_mark->frame, MPP_FMT_YUV420SP);
        } else if ((YUV420 == p_Vid->yuv_format) && (10 == p_Vid->bit_depth_luma)) {
            mpp_frame_set_fmt(cur_mark->frame, MPP_FMT_YUV420SP_10BIT);
            H264D_LOG(" alloc_picture ----- MPP_FMT_YUV420SP_10BIT ------ \n");
        } else if ((YUV422 == p_Vid->yuv_format) && (8 == p_Vid->bit_depth_luma)) {
            mpp_frame_set_fmt(cur_mark->frame, MPP_FMT_YUV422SP);
        } else if ((YUV422 == p_Vid->yuv_format) && (10 == p_Vid->bit_depth_luma)) {
            mpp_frame_set_fmt(cur_mark->frame, MPP_FMT_YUV422SP_10BIT);
        }
        hor_stride = ((p_Vid->width * p_Vid->bit_depth_luma + 127) & (~127)) / 8;
        ver_stride = p_Vid->height;
        hor_stride = MPP_ALIGN(hor_stride, 256) | 256;
        ver_stride = MPP_ALIGN(ver_stride, 16);
        mpp_frame_set_hor_stride(cur_mark->frame, hor_stride);  // before crop
        mpp_frame_set_ver_stride(cur_mark->frame, ver_stride);
        mpp_frame_set_width(cur_mark->frame,  p_Vid->width_after_crop);  // after crop
        mpp_frame_set_height(cur_mark->frame, p_Vid->height_after_crop);
        H264D_LOG("hor_stride=%d, ver_stride=%d, width=%d, height=%d, crop_width=%d, crop_height =%d \n", hor_stride,
                  ver_stride, p_Vid->width, p_Vid->height, p_Vid->width_after_crop, p_Vid->height_after_crop);
        mpp_frame_set_pts(cur_mark->frame, p_Vid->p_Cur->last_pts);
        mpp_frame_set_dts(cur_mark->frame, p_Vid->p_Cur->last_dts);
        mpp_buf_slot_set_prop(p_Dec->frame_slots, cur_mark->slot_idx, SLOT_FRAME, cur_mark->frame);
        FPRINT(g_debug_file0, "[Malloc] g_framecnt=%d, mark_idx=%d, slot_idx=%d, lay_id=%d, pts=%lld \n",
               p_Vid->g_framecnt, cur_mark->mark_idx, cur_mark->slot_idx, layer_id, p_Vid->p_Inp->in_pts);
        p_Vid->active_dpb_mark[layer_id] = cur_mark;
    }
    cur_mark = p_Vid->active_dpb_mark[layer_id];
    if (structure == FRAME || structure == TOP_FIELD) {
        cur_mark->top_used += 1;
    }
    if (structure == FRAME || structure == BOTTOM_FIELD) {
        cur_mark->bot_used += 1;
    }
    p_Vid->p_Dec->in_task->output = cur_mark->slot_idx;
    mpp_buf_slot_set_flag(p_Dec->frame_slots, cur_mark->slot_idx, SLOT_HAL_OUTPUT);
	dec_pic->mem_mark = p_Vid->active_dpb_mark[layer_id];
    dec_pic->mem_mark->pic = dec_pic;
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
    currSlice->toppoc = p_Vid->last_toppoc[currSlice->layer_id];
    currSlice->bottompoc = p_Vid->last_bottompoc[currSlice->layer_id];
    currSlice->framepoc = p_Vid->last_framepoc[currSlice->layer_id];
    currSlice->ThisPOC = p_Vid->last_thispoc[currSlice->layer_id];
    FUN_CHECK(ret = decode_poc(p_Vid, currSlice));  //!< calculate POC

    dec_pic->top_poc = currSlice->toppoc;
    dec_pic->bottom_poc = currSlice->bottompoc;
    dec_pic->frame_poc = currSlice->framepoc;
    dec_pic->ThisPOC = currSlice->ThisPOC;

    p_Vid->last_toppoc[currSlice->layer_id] = currSlice->toppoc;
    p_Vid->last_bottompoc[currSlice->layer_id] = currSlice->bottompoc;
    p_Vid->last_framepoc[currSlice->layer_id] = currSlice->framepoc;
    p_Vid->last_thispoc[currSlice->layer_id] = currSlice->ThisPOC;

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
    dec_pic->view_id = currSlice->view_id;
    dec_pic->inter_view_flag = currSlice->inter_view_flag;
    dec_pic->anchor_pic_flag = currSlice->anchor_pic_flag;
    if (dec_pic->layer_id == 1) {
        if ((p_Vid->profile_idc == MVC_HIGH) || (p_Vid->profile_idc == STEREO_HIGH)) {
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


    dec_pic->combine_flag = get_filed_dpb_combine_flag(p_Dpb, dec_pic);
    dec_pic->mem_malloc_type = Mem_Malloc;
    dpb_mark_malloc(p_Vid, dec_pic); // malloc dpb_memory
    dec_pic->colmv_no_used_flag = 0;
    p_Vid->last_pic_structure = dec_pic->structure;
    p_Vid->dec_pic = dec_pic;

    //mpp_log_f("[DEC_OUT]line=%d, func=alloc_decpic, cur_slot_idx=%d", __LINE__, dec_pic->mem_mark->slot_idx);
    //!< set mpp_frame width && height && pts && get slot_frame
    //cur_mark = dec_pic->mem_mark;
    //p_Vid->p_Dec->in_task->output = cur_mark->slot_idx;
    //mpp_frame_set_width(cur_mark->frame, p_Vid->width);
    //mpp_frame_set_height(cur_mark->frame, p_Vid->height);
    //mpp_frame_set_pts(cur_mark->frame, p_Vid->p_Inp->pts);

    return ret = MPP_OK;
__FAILED:
    MPP_FREE(dec_pic);
    ASSERT(0);

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

    RK_U32(*is_ref)(H264_StorePic_t *s) = (long_term) ? is_long_ref : is_short_ref;

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
    ASSERT(0);
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
    ASSERT(0);
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
    ASSERT(0);
    MPP_FREE(fs_list0);
    MPP_FREE(fs_list1);
    MPP_FREE(fs_listlt);
    MPP_FREE(currSlice->fs_listinterview0);
    MPP_FREE(currSlice->fs_listinterview1);

    return ret;
}

/*!
***********************************************************************
* \brief
*    check parser is end and then configure register
***********************************************************************
*/
//extern "C"
MPP_RET init_picture(H264_SLICE_t *currSlice)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_DecCtx_t *p_Dec = currSlice->p_Vid->p_Dec;
    H264dVideoCtx_t *p_Vid = currSlice->p_Vid;

    FUN_CHECK(ret = alloc_decpic(currSlice));
    //!< idr_memory_management MVC_layer, idr_flag==1
    if (currSlice->layer_id && !currSlice->svc_extension_flag && !currSlice->mvcExt.non_idr_flag) {
        ASSERT(currSlice->layer_id == 1);
        FUN_CHECK(ret = idr_memory_management(p_Vid->p_Dpb_layer[currSlice->layer_id], p_Vid->dec_pic));
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
    prepare_init_scanlist(currSlice);
    fill_picparams(currSlice->p_Vid, &p_Dec->dxva_ctx->pp);
    fill_scanlist(currSlice->p_Vid, &p_Dec->dxva_ctx->qm);

    return ret = MPP_OK;
__FAILED:
    return ret;
}

