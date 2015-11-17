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

#define MODULE_TAG "h264d_dpb"

#include <stdio.h>
#include <string.h>

#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_buf_slot.h"

#include "h264d_log.h"
#include "h264d_scalist.h"
#include "h264d_dpb.h"

#ifndef INT_MIN
#define INT_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#endif
#ifndef INT_MAX
#define INT_MAX       2147483647      /* maximum (signed) int value */
#endif
static RK_S32 RoundLog2(RK_S32 iValue)
{
    RK_S32 iRet = 0;
    RK_S32 iValue_square = iValue * iValue;

    while ((1 << (iRet + 1)) <= iValue_square) {
        ++iRet;
    }
    iRet = (iRet + 1) >> 1;

    return iRet;
}

static RK_S32 getDpbSize(H264dVideoCtx_t *p_Vid, H264_SPS_t *active_sps)
{
    RK_S32 size = 0, num_views = 0;
    RK_S32 pic_size = (active_sps->pic_width_in_mbs_minus1 + 1)
                      * (active_sps->pic_height_in_map_units_minus1 + 1) * (active_sps->frame_mbs_only_flag ? 1 : 2) * 384;

    switch (active_sps->level_idc) {
    case 9:
        size = 152064;
        break;
    case 10:
        size = 152064;
        break;
    case 11:
        if (!is_prext_profile(active_sps->profile_idc) && (active_sps->constrained_set3_flag == 1))
            size = 152064;
        else
            size = 345600;
        break;
    case 12:
        size = 912384;
        break;
    case 13:
        size = 912384;
        break;
    case 20:
        size = 912384;
        break;
    case 21:
        size = 1824768;
        break;
    case 22:
        size = 3110400;
        break;
    case 30:
        size = 3110400;
        break;
    case 31:
        size = 6912000;
        break;
    case 32:
        size = 7864320;
        break;
    case 40:
        size = 12582912;
        break;
    case 41:
        size = 12582912;
        break;
    case 42:
        size = 13369344;
        break;
    case 50:
        size = 42393600;
        break;
    case 51:
        size = 70778880;
        break;
    case 52:
        size = 70778880;
        break;
    default:
        ASSERT(0);
        break;
    }
    size /= pic_size;
    if (p_Vid->active_mvc_sps_flag &&
        (p_Vid->profile_idc == MVC_HIGH || p_Vid->profile_idc == STEREO_HIGH)) {
        num_views = p_Vid->active_subsps->num_views_minus1 + 1;
        size = MPP_MIN(2 * size, MPP_MAX(1, RoundLog2(num_views)) * 16) / num_views;
    } else {
        size = MPP_MIN(size, 16);
    }

    return size;
}

static RK_S32 get_pic_num_x(H264_StorePic_t *p, RK_S32 difference_of_pic_nums_minus1)
{
    RK_S32 currPicNum;

    if (p->structure == FRAME)
        currPicNum = p->frame_num;
    else
        currPicNum = 2 * p->frame_num + 1;

    return currPicNum - (difference_of_pic_nums_minus1 + 1);
}

static void unmark_for_reference(H264_DecCtx_t *p_Dec, H264_FrameStore_t* fs)
{
    struct h264_store_pic_t *cur_pic = NULL;

    if (fs->is_used & 1) {
        if (fs->top_field) {
            fs->top_field->used_for_reference = 0;
            cur_pic = fs->top_field;
        }
    }
    if (fs->is_used & 2) {
        if (fs->bottom_field) {
            fs->bottom_field->used_for_reference = 0;
            cur_pic = fs->bottom_field;
        }
    }
    if (fs->is_used == 3) {
        if (fs->top_field && fs->bottom_field) {
            fs->top_field->used_for_reference = 0;
            fs->bottom_field->used_for_reference = 0;
        }
        fs->frame->used_for_reference = 0;
        cur_pic = fs->frame;
    }
    fs->is_reference = 0;
    if (cur_pic->mem_malloc_type == Mem_Malloc || cur_pic->mem_malloc_type == Mem_Clone) {
        if (cur_pic->used_for_reference == 0) {
            //mpp_buf_slot_clr_flag(p_Dec->frame_slots, cur_pic->mem_mark->slot_idx, SLOT_CODEC_USE);
        }
    }
	(void)p_Dec;
}

static RK_U32 is_short_term_reference(H264_FrameStore_t* fs)
{
    if (fs->is_used == 3) { // frame
        if ((fs->frame->used_for_reference) && (!fs->frame->is_long_term)) {
            return 1;
        }
    }

    if (fs->is_used & 1) { // top field
        if (fs->top_field) {
            if ((fs->top_field->used_for_reference) && (!fs->top_field->is_long_term)) {
                return 1;
            }
        }
    }

    if (fs->is_used & 2) { // bottom field
        if (fs->bottom_field) {
            if ((fs->bottom_field->used_for_reference) && (!fs->bottom_field->is_long_term)) {
                return 1;
            }
        }
    }
    return 0;
}

static RK_U32 is_long_term_reference(H264_FrameStore_t* fs)
{

    if (fs->is_used == 3) { // frame
        if ((fs->frame->used_for_reference) && (fs->frame->is_long_term)) {
            return 1;
        }
    }

    if (fs->is_used & 1) { // top field
        if (fs->top_field) {
            if ((fs->top_field->used_for_reference) && (fs->top_field->is_long_term)) {
                return 1;
            }
        }
    }

    if (fs->is_used & 2) { // bottom field
        if (fs->bottom_field) {
            if ((fs->bottom_field->used_for_reference) && (fs->bottom_field->is_long_term)) {
                return 1;
            }
        }
    }
    return 0;
}

static void unmark_for_long_term_reference(H264_FrameStore_t* fs)
{
    if (fs->is_used & 1) {
        if (fs->top_field) {
            fs->top_field->used_for_reference = 0;
            fs->top_field->is_long_term = 0;
        }
    }
    if (fs->is_used & 2) {
        if (fs->bottom_field) {
            fs->bottom_field->used_for_reference = 0;
            fs->bottom_field->is_long_term = 0;
        }
    }
    if (fs->is_used == 3) {
        if (fs->top_field && fs->bottom_field) {
            fs->top_field->used_for_reference = 0;
            fs->top_field->is_long_term = 0;
            fs->bottom_field->used_for_reference = 0;
            fs->bottom_field->is_long_term = 0;
        }
        fs->frame->used_for_reference = 0;
        fs->frame->is_long_term = 0;
    }

    fs->is_reference = 0;
    fs->is_long_term = 0;
}

static void mm_unmark_short_term_for_reference(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p, RK_S32 difference_of_pic_nums_minus1)
{
    RK_S32 picNumX = 0;
    RK_U32 i = 0;

    picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);

    for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
        if (p->structure == FRAME) {
            if ((p_Dpb->fs_ref[i]->is_reference == 3) && (p_Dpb->fs_ref[i]->is_long_term == 0)) {
                if (p_Dpb->fs_ref[i]->frame->pic_num == picNumX) {
                    unmark_for_reference(p_Dpb->p_Vid->p_Dec, p_Dpb->fs_ref[i]);
                    return;
                }
            }
        } else {
            if ((p_Dpb->fs_ref[i]->is_reference & 1) && (!(p_Dpb->fs_ref[i]->is_long_term & 1))) {
                if (p_Dpb->fs_ref[i]->top_field->pic_num == picNumX) {
                    p_Dpb->fs_ref[i]->top_field->used_for_reference = 0;
                    p_Dpb->fs_ref[i]->is_reference &= 2;
                    if (p_Dpb->fs_ref[i]->is_used == 3) {
                        p_Dpb->fs_ref[i]->frame->used_for_reference = 0;
                    }
                    return;
                }
            }
            if ((p_Dpb->fs_ref[i]->is_reference & 2) && (!(p_Dpb->fs_ref[i]->is_long_term & 2))) {
                if (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX) {
                    p_Dpb->fs_ref[i]->bottom_field->used_for_reference = 0;
                    p_Dpb->fs_ref[i]->is_reference &= 1;
                    if (p_Dpb->fs_ref[i]->is_used == 3) {
                        p_Dpb->fs_ref[i]->frame->used_for_reference = 0;
                    }
                    return;
                }
            }
        }
    }
}

static void mm_unmark_long_term_for_reference(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p, RK_S32 long_term_pic_num)
{
    RK_U32 i = 0;

    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
        if (p->structure == FRAME) {
            if ((p_Dpb->fs_ltref[i]->is_reference == 3) && (p_Dpb->fs_ltref[i]->is_long_term == 3)) {
                if (p_Dpb->fs_ltref[i]->frame->long_term_pic_num == long_term_pic_num) {
                    unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                }
            }
        } else {
            if ((p_Dpb->fs_ltref[i]->is_reference & 1) && ((p_Dpb->fs_ltref[i]->is_long_term & 1))) {
                if (p_Dpb->fs_ltref[i]->top_field->long_term_pic_num == long_term_pic_num) {
                    p_Dpb->fs_ltref[i]->top_field->used_for_reference = 0;
                    p_Dpb->fs_ltref[i]->top_field->is_long_term = 0;
                    p_Dpb->fs_ltref[i]->is_reference &= 2;
                    p_Dpb->fs_ltref[i]->is_long_term &= 2;
                    if (p_Dpb->fs_ltref[i]->is_used == 3) {
                        p_Dpb->fs_ltref[i]->frame->used_for_reference = 0;
                        p_Dpb->fs_ltref[i]->frame->is_long_term = 0;
                    }
                    return;
                }
            }
            if ((p_Dpb->fs_ltref[i]->is_reference & 2) && ((p_Dpb->fs_ltref[i]->is_long_term & 2))) {
                if (p_Dpb->fs_ltref[i]->bottom_field->long_term_pic_num == long_term_pic_num) {
                    p_Dpb->fs_ltref[i]->bottom_field->used_for_reference = 0;
                    p_Dpb->fs_ltref[i]->bottom_field->is_long_term = 0;
                    p_Dpb->fs_ltref[i]->is_reference &= 1;
                    p_Dpb->fs_ltref[i]->is_long_term &= 1;
                    if (p_Dpb->fs_ltref[i]->is_used == 3) {
                        p_Dpb->fs_ltref[i]->frame->used_for_reference = 0;
                        p_Dpb->fs_ltref[i]->frame->is_long_term = 0;
                    }
                    return;
                }
            }
        }
    }
}

static void unmark_long_term_frame_for_reference_by_frame_idx(H264_DpbBuf_t *p_Dpb, RK_S32 long_term_frame_idx)
{
    RK_U32 i = 0;
    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
        if (p_Dpb->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx) {
            unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
        }
    }
}

static MPP_RET unmark_long_term_field_for_reference_by_frame_idx(H264_DpbBuf_t *p_Dpb, RK_S32 structure,
                                                                 RK_S32 long_term_frame_idx, RK_S32 mark_current, RK_U32 curr_frame_num, RK_S32 curr_pic_num)
{
    RK_U8 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    VAL_CHECK(ret, structure != FRAME);
    if (curr_pic_num < 0)
        curr_pic_num += (2 * p_Vid->max_frame_num);

    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
        if (p_Dpb->fs_ltref[i]->long_term_frame_idx == long_term_frame_idx) {
            if (structure == TOP_FIELD) {
                if (p_Dpb->fs_ltref[i]->is_long_term == 3) {
                    unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                } else {
                    if (p_Dpb->fs_ltref[i]->is_long_term == 1) {
                        unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                    } else {
                        if (mark_current) {
                            if (p_Dpb->last_picture) {
                                if ((p_Dpb->last_picture != p_Dpb->fs_ltref[i]) || p_Dpb->last_picture->frame_num != curr_frame_num)
                                    unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            } else {
                                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            }
                        } else {
                            if ((p_Dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1)) {
                                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            }
                        }
                    }
                }
            }
            if (structure == BOTTOM_FIELD) {
                if (p_Dpb->fs_ltref[i]->is_long_term == 3) {
                    unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                } else {
                    if (p_Dpb->fs_ltref[i]->is_long_term == 2) {
                        unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                    } else {
                        if (mark_current) {
                            if (p_Dpb->last_picture) {
                                if ((p_Dpb->last_picture != p_Dpb->fs_ltref[i]) || p_Dpb->last_picture->frame_num != curr_frame_num)
                                    unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            } else {
                                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            }
                        } else {
                            if ((p_Dpb->fs_ltref[i]->frame_num) != (unsigned)(curr_pic_num >> 1)) {
                                unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
                            }
                        }
                    }
                }
            }
        }
    }
    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void mark_pic_long_term(H264_DpbBuf_t *p_Dpb, H264_StorePic_t* p, RK_S32 long_term_frame_idx, RK_S32 picNumX)
{
    RK_U32 i = 0;
    RK_S32 add_top = 0, add_bottom = 0;
    LogCtx_t *runlog = p_Dpb->p_Vid->p_Dec->logctx.parr[RUN_PARSE];

    if (p->structure == FRAME) {
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_reference == 3) {
                if ((!p_Dpb->fs_ref[i]->frame->is_long_term) && (p_Dpb->fs_ref[i]->frame->pic_num == picNumX)) {
                    p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_frame_idx = long_term_frame_idx;
                    p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
                    p_Dpb->fs_ref[i]->frame->is_long_term = 1;

                    if (p_Dpb->fs_ref[i]->top_field && p_Dpb->fs_ref[i]->bottom_field) {
                        p_Dpb->fs_ref[i]->top_field->long_term_frame_idx = p_Dpb->fs_ref[i]->bottom_field->long_term_frame_idx = long_term_frame_idx;
                        p_Dpb->fs_ref[i]->top_field->long_term_pic_num = long_term_frame_idx;
                        p_Dpb->fs_ref[i]->bottom_field->long_term_pic_num = long_term_frame_idx;
                        p_Dpb->fs_ref[i]->top_field->is_long_term = p_Dpb->fs_ref[i]->bottom_field->is_long_term = 1;
                    }
                    p_Dpb->fs_ref[i]->is_long_term = 3;
                    return;
                }
            }
        }
        LogWarnning(runlog, "reference frame for long term marking not found.");
    } else {
        if (p->structure == TOP_FIELD) {
            add_top = 1;
            add_bottom = 0;
        } else {
            add_top = 0;
            add_bottom = 1;
        }
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_reference & 1) {
                if ((!p_Dpb->fs_ref[i]->top_field->is_long_term) && (p_Dpb->fs_ref[i]->top_field->pic_num == picNumX)) {
                    if ((p_Dpb->fs_ref[i]->is_long_term) && (p_Dpb->fs_ref[i]->long_term_frame_idx != long_term_frame_idx)) {
                        LogWarnning(runlog, "assigning long_term_frame_idx different from other field.");
                    }
                    p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->top_field->long_term_frame_idx = long_term_frame_idx;
                    p_Dpb->fs_ref[i]->top_field->long_term_pic_num = 2 * long_term_frame_idx + add_top;
                    p_Dpb->fs_ref[i]->top_field->is_long_term = 1;
                    p_Dpb->fs_ref[i]->is_long_term |= 1;
                    if (p_Dpb->fs_ref[i]->is_long_term == 3) {
                        p_Dpb->fs_ref[i]->frame->is_long_term = 1;
                        p_Dpb->fs_ref[i]->frame->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
                    }
                    return;
                }
            }
            if (p_Dpb->fs_ref[i]->is_reference & 2) {
                if ((!p_Dpb->fs_ref[i]->bottom_field->is_long_term) && (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX)) {
                    if ((p_Dpb->fs_ref[i]->is_long_term) && (p_Dpb->fs_ref[i]->long_term_frame_idx != long_term_frame_idx)) {
                        LogWarnning(runlog, "assigning long_term_frame_idx different from other field.");
                    }

                    p_Dpb->fs_ref[i]->long_term_frame_idx = p_Dpb->fs_ref[i]->bottom_field->long_term_frame_idx
                                                            = long_term_frame_idx;
                    p_Dpb->fs_ref[i]->bottom_field->long_term_pic_num = 2 * long_term_frame_idx + add_bottom;
                    p_Dpb->fs_ref[i]->bottom_field->is_long_term = 1;
                    p_Dpb->fs_ref[i]->is_long_term |= 2;
                    if (p_Dpb->fs_ref[i]->is_long_term == 3) {
                        p_Dpb->fs_ref[i]->frame->is_long_term = 1;
                        p_Dpb->fs_ref[i]->frame->long_term_frame_idx = p_Dpb->fs_ref[i]->frame->long_term_pic_num = long_term_frame_idx;
                    }
                    return;
                }
            }
        }
        LogWarnning(runlog, "reference field for long term marking not found.");
    }
}

static MPP_RET mm_assign_long_term_frame_idx(H264_DpbBuf_t *p_Dpb, H264_StorePic_t* p, RK_S32 difference_of_pic_nums_minus1, RK_S32 long_term_frame_idx)
{
    RK_S32 picNumX = 0;
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    picNumX = get_pic_num_x(p, difference_of_pic_nums_minus1);
    //!< remove frames/fields with same long_term_frame_idx
    if (p->structure == FRAME) {
        unmark_long_term_frame_for_reference_by_frame_idx(p_Dpb, long_term_frame_idx);
    } else {
        PictureStructure structure = FRAME;

        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            if (p_Dpb->fs_ref[i]->is_reference & 1) {
                if (p_Dpb->fs_ref[i]->top_field->pic_num == picNumX) {
                    structure = TOP_FIELD;
                    break;
                }
            }
            if (p_Dpb->fs_ref[i]->is_reference & 2) {
                if (p_Dpb->fs_ref[i]->bottom_field->pic_num == picNumX) {
                    structure = BOTTOM_FIELD;
                    break;
                }
            }
        }
        VAL_CHECK(ret, structure != FRAME);
        FUN_CHECK(ret = unmark_long_term_field_for_reference_by_frame_idx(p_Dpb, structure, long_term_frame_idx, 0, 0, picNumX));
    }
    mark_pic_long_term(p_Dpb, p, long_term_frame_idx, picNumX);

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static void mm_update_max_long_term_frame_idx(H264_DpbBuf_t *p_Dpb, RK_S32 max_long_term_frame_idx_plus1)
{
    RK_U32 i = 0;

    p_Dpb->max_long_term_pic_idx = max_long_term_frame_idx_plus1 - 1;

    // check for invalid frames
    for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
        if (p_Dpb->fs_ltref[i]->long_term_frame_idx > p_Dpb->max_long_term_pic_idx) {
            unmark_for_long_term_reference(p_Dpb->fs_ltref[i]);
        }
    }
}

static void mm_unmark_all_short_term_for_reference(H264_DpbBuf_t *p_Dpb)
{
    RK_U32 i = 0;
    for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
        unmark_for_reference(p_Dpb->p_Vid->p_Dec, p_Dpb->fs_ref[i]);
    }
    update_ref_list(p_Dpb);
}

static void mm_unmark_all_long_term_for_reference(H264_DpbBuf_t *p_Dpb)
{
    mm_update_max_long_term_frame_idx(p_Dpb, 0);
}

static void mm_mark_current_picture_long_term(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p, RK_S32 long_term_frame_idx)
{
    // remove long term pictures with same long_term_frame_idx
    if (p->structure == FRAME) {
        unmark_long_term_frame_for_reference_by_frame_idx(p_Dpb, long_term_frame_idx);
    } else {
        unmark_long_term_field_for_reference_by_frame_idx(p_Dpb, p->structure, long_term_frame_idx, 1, p->pic_num, 0);
    }

    p->is_long_term = 1;
    p->long_term_frame_idx = long_term_frame_idx;
}

static void sliding_window_memory_management(H264_DpbBuf_t *p_Dpb)
{
    RK_U32 i = 0;

    // if this is a reference pic with sliding window, unmark first ref frame
    if (p_Dpb->ref_frames_in_buffer == MPP_MAX(1, p_Dpb->num_ref_frames) - p_Dpb->ltref_frames_in_buffer) {
        for (i = 0; i < p_Dpb->used_size; i++) {
            if (p_Dpb->fs[i]->is_reference && (!(p_Dpb->fs[i]->is_long_term))) {
                unmark_for_reference(p_Dpb->p_Vid->p_Dec, p_Dpb->fs[i]);
                update_ref_list(p_Dpb);
                break;
            }
        }
    }
}

static void check_num_ref(H264_DpbBuf_t *p_Dpb)
{
    LogCtx_t *runlog = p_Dpb->p_Vid->p_Dec->logctx.parr[RUN_PARSE];

    if ((RK_S32)(p_Dpb->ltref_frames_in_buffer + p_Dpb->ref_frames_in_buffer) > MPP_MAX(1, p_Dpb->num_ref_frames)) {
        sliding_window_memory_management(p_Dpb);
        LogWarnning(runlog, "Max number of reference frames exceeded");
    }
}

static RK_U32 is_used_for_reference(H264_FrameStore_t* fs)
{
    RK_U8 is_used_flag = 0;

    if (fs->is_reference) {
        return is_used_flag = 1;
    }

    if (fs->is_used == 3) { // frame
        if (fs->frame->used_for_reference) {
            return is_used_flag = 1;
        }
    }

    if (fs->is_used & 1) { // top field
        if (fs->top_field) {
            if (fs->top_field->used_for_reference) {
                return is_used_flag = 1;
            }
        }
    }

    if (fs->is_used & 2) { // bottom field
        if (fs->bottom_field) {
            if (fs->bottom_field->used_for_reference) {
                return is_used_flag = 1;
            }
        }
    }
    return is_used_flag = 0;
}

static void free_dpb_memory_index(H264_DecCtx_t *p_Dec, H264_DpbMark_t *p, RK_S32 structure)
{

    if (structure == FRAME) {
        p->top_used = (p->top_used > 0) ? (p->top_used - 1) : 0;
        p->bot_used = (p->bot_used > 0) ? (p->bot_used - 1) : 0;
    } else if (structure == TOP_FIELD) {
        p->top_used = (p->top_used > 0) ? (p->top_used - 1) : 0;
    } else if (structure == BOTTOM_FIELD) {
        p->bot_used = (p->bot_used > 0) ? (p->bot_used - 1) : 0;
    }
    if (p->top_used == 0 && p->bot_used == 0 && (p->slot_idx != -1)) {
        if (p_Dec->p_Vid->g_framecnt == 692) {
            p = p;
        }
        mpp_buf_slot_clr_flag(p_Dec->frame_slots, p->slot_idx, SLOT_CODEC_USE);

		//mpp_buf_slot_clr_flag(p_Dec->frame_slots, p->slot_idx, SLOT_HAL_INPUT);
		//mpp_buf_slot_clr_flag(p_Dec->frame_slots, p->slot_idx, SLOT_HAL_OUTPUT);
        //mpp_buf_slot_clr_flag(p_Dec->frame_slots, p->slot_idx, SLOT_QUEUE_USE);
		//FPRINT(g_debug_file1, "[FREE] g_frame_no=%d, structure=%d, mark_idx=%d, slot_idx=%d \n", p_Dec->p_Vid->g_framecnt, structure, p->mark_idx, p->slot_idx);
		//mpp_print_slot_flag_info(g_debug_file1, p_Dec->frame_slots, p->slot_idx);

        
        p->slot_idx = -1;
    }
}

static MPP_RET remove_frame_from_dpb(H264_DpbBuf_t *p_Dpb, RK_S32 pos)
{
    RK_U32  i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_FrameStore_t* tmp = NULL;
    H264_FrameStore_t* fs = p_Dpb->fs[pos];
    H264_DecCtx_t *p_Dec = p_Dpb->p_Vid->p_Dec;
    LogCtx_t *runlog = p_Dec->logctx.parr[RUN_PARSE];

    switch (fs->is_used) {
    case 3:
        if (fs->frame)           free_storable_picture(p_Dec, fs->frame);
        if (fs->top_field)       free_storable_picture(p_Dec, fs->top_field);
        if (fs->bottom_field)    free_storable_picture(p_Dec, fs->bottom_field);
        fs->frame = NULL;
        fs->top_field = NULL;
        fs->bottom_field = NULL;
        break;
    case 2:
        if (fs->bottom_field)    free_storable_picture(p_Dec, fs->bottom_field);
        fs->bottom_field = NULL;
        break;
    case 1:
        if (fs->top_field)        free_storable_picture(p_Dec, fs->top_field);
        fs->top_field = NULL;
        break;
    case 0:
        break;
    default:
        LogError(runlog, "invalid frame store type.");
        goto __FAILED;
    }
    fs->is_used = 0;
    fs->is_long_term = 0;
    fs->is_reference = 0;
    fs->is_orig_reference = 0;

    // move empty framestore to end of buffer
    tmp = p_Dpb->fs[pos];

    for (i = pos; i < p_Dpb->used_size - 1; i++) {
        p_Dpb->fs[i] = p_Dpb->fs[i + 1];
    }
    p_Dpb->fs[p_Dpb->used_size - 1] = tmp;
    p_Dpb->used_size--;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret = MPP_NOK;
}

static MPP_RET remove_unused_frame_from_dpb(H264_DpbBuf_t *p_Dpb)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    // check for frames that were already output and no longer used for reference
    for (i = 0; i < p_Dpb->used_size; i++) {
        if (p_Dpb->fs[i]->is_output && (!is_used_for_reference(p_Dpb->fs[i]))) {
            FUN_CHECK(ret = remove_frame_from_dpb(p_Dpb, i));
            return MPP_OK;
        }
    }
__FAILED:
    return ret;
}

static MPP_RET get_smallest_poc(H264_DpbBuf_t *p_Dpb, RK_S32 *poc, RK_S32 *pos)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    VAL_CHECK(ret, p_Dpb->used_size > 0); //!< "Cannot determine smallest POC, DPB empty."
    *pos = -1;
    *poc = INT_MAX;
    for (i = 0; i < p_Dpb->used_size; i++) {
        if ((*poc > p_Dpb->fs[i]->poc) && (!p_Dpb->fs[i]->is_output)) {
            *poc = p_Dpb->fs[i]->poc;
            *pos = i;
        }
    }

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static H264_FrameStore_t *alloc_frame_store()
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_FrameStore_t *f = mpp_calloc(H264_FrameStore_t, 1);
    MEM_CHECK(ret, f);

    f->is_used = 0;
    f->is_reference = 0;
    f->is_long_term = 0;
    f->is_orig_reference = 0;
    f->is_output = 0;

    f->frame = NULL;
    f->top_field = NULL;
    f->bottom_field = NULL;

    return f;
__FAILED:
    (void)ret;
    return NULL;
}

static MPP_RET dpb_combine_field_yuv(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs, RK_U8 combine_flag)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (!fs->frame) {
        fs->frame = alloc_storable_picture(p_Vid, FRAME);
        MEM_CHECK(ret, fs->frame);
        ASSERT(fs->top_field->colmv_no_used_flag == fs->bottom_field->colmv_no_used_flag);
        fs->frame->colmv_no_used_flag = fs->top_field->colmv_no_used_flag;
        if (combine_flag) {
            ASSERT(fs->top_field->mem_mark->mark_idx == fs->bottom_field->mem_mark->mark_idx);
            ASSERT(fs->top_field->mem_mark->slot_idx == fs->bottom_field->mem_mark->slot_idx);
            fs->frame->mem_malloc_type = fs->top_field->mem_malloc_type;
            fs->frame->mem_mark = fs->top_field->mem_mark;
        } else if (fs->is_used == 0x1) { // unpaired, have top
            ASSERT(fs->bottom_field->mem_malloc_type == Mem_UnPaired);
            fs->frame->mem_mark = fs->top_field->mem_mark;
        } else if (fs->is_used == 0x02) { // unpaired, have bottom
            ASSERT(fs->top_field->mem_malloc_type == Mem_UnPaired);
            fs->frame->mem_mark = fs->bottom_field->mem_mark;
        } else {
            ASSERT(fs->is_used == 0x03);
        }
    }
    fs->poc = fs->frame->poc = fs->frame->frame_poc = MPP_MIN(fs->top_field->poc, fs->bottom_field->poc);
    fs->bottom_field->frame_poc = fs->top_field->frame_poc = fs->frame->poc;
    fs->bottom_field->top_poc = fs->frame->top_poc = fs->top_field->poc;
    fs->top_field->bottom_poc = fs->frame->bottom_poc = fs->bottom_field->poc;
    fs->frame->used_for_reference = (fs->top_field->used_for_reference && fs->bottom_field->used_for_reference);
    fs->frame->is_long_term = (fs->top_field->is_long_term && fs->bottom_field->is_long_term);
    if (fs->frame->is_long_term) {
        fs->frame->long_term_frame_idx = fs->long_term_frame_idx;
    }
    fs->frame->top_field = fs->top_field;
    fs->frame->bottom_field = fs->bottom_field;
    fs->frame->frame = fs->frame;
    fs->frame->chroma_format_idc = fs->top_field->chroma_format_idc;

    fs->frame->frame_cropping_flag = fs->top_field->frame_cropping_flag;
    if (fs->frame->frame_cropping_flag) {
        fs->frame->frame_crop_top_offset = fs->top_field->frame_crop_top_offset;
        fs->frame->frame_crop_bottom_offset = fs->top_field->frame_crop_bottom_offset;
        fs->frame->frame_crop_left_offset = fs->top_field->frame_crop_left_offset;
        fs->frame->frame_crop_right_offset = fs->top_field->frame_crop_right_offset;
    }
    fs->top_field->frame = fs->bottom_field->frame = fs->frame;
    fs->top_field->top_field = fs->top_field;
    fs->top_field->bottom_field = fs->bottom_field;
    fs->bottom_field->top_field = fs->top_field;
    fs->bottom_field->bottom_field = fs->bottom_field;

    fs->frame->is_mmco_5 = fs->top_field->is_mmco_5 || fs->bottom_field->is_mmco_5;
    fs->frame->poc_mmco5 = MPP_MIN(fs->top_field->top_poc_mmco5, fs->bottom_field->bot_poc_mmco5);
    fs->frame->top_poc_mmco5 = fs->top_field->top_poc_mmco5;
    fs->frame->bot_poc_mmco5 = fs->top_field->bot_poc_mmco5;

    return ret = MPP_OK;
__FAILED:
    return ret ;
}

static void write_picture(H264_StorePic_t *p, H264dVideoCtx_t *p_Vid)
{
    H264_DpbMark_t *p_mark = NULL;
	MppFrame frame;
    if (p->mem_malloc_type == Mem_Malloc /*|| p->mem_malloc_type == Mem_Clone*/) {
        p_mark = p->mem_mark;
        //mpp_log_f(" [Write_picture] In");
		mpp_buf_slot_get_prop(p_Vid->p_Dec->frame_slots, p_mark->slot_idx, SLOT_FRAME_PTR, &frame);	
		//mpp_log("[dispaly] layer_id %d pts %lld\n", p->layer_id, mpp_frame_get_pts(frame));

		mpp_frame_set_viewid(frame, p->layer_id);
		//if (p->layer_id == 0) {
		//	mpp_frame_set_display(frame, 0);
		//}
		//else 
		{
			mpp_frame_set_display(frame, 1);			
		}
		p_Vid->p_Dec->last_frame_slot_idx = p_mark->slot_idx;

		mpp_buf_slot_set_flag(p_Vid->p_Dec->frame_slots, p_mark->slot_idx, SLOT_QUEUE_USE);
		mpp_buf_slot_enqueue(p_Vid->p_Dec->frame_slots, p_mark->slot_idx, QUEUE_DISPLAY);

        FPRINT(g_debug_file0, "[WRITE_PICTURE] lay_id=%d, g_frame_no=%d, mark_idx=%d, slot_idx=%d, pts=%lld \n", 
			p->layer_id, p_Vid->g_framecnt, p_mark->mark_idx, p_mark->slot_idx, mpp_frame_get_pts(frame));
        //mpp_print_slot_flag_info(g_debug_file1, p_Vid->p_Dec->frame_slots, p_mark->slot_idx);
        //mpp_log_f(" [Write_picture] Out");

        LogInfo(p_Vid->p_Dec->logctx.parr[RUN_PARSE], "[WRITE_PICTURE] g_frame_cnt=%d", p_Vid->g_framecnt);
    }

}

static MPP_RET write_unpaired_field(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_StorePic_t *p = NULL;

    VAL_CHECK(ret, fs->is_used < 3);
    if (fs->is_used & 0x01) { // we have a top field, construct an empty bottom field
        p = fs->top_field;
        fs->bottom_field = alloc_storable_picture(p_Vid, BOTTOM_FIELD);
        MEM_CHECK(ret, fs->bottom_field);
        fs->bottom_field->mem_malloc_type = Mem_UnPaired;
        fs->bottom_field->chroma_format_idc = p->chroma_format_idc;
        FUN_CHECK(ret = dpb_combine_field_yuv(p_Vid, fs, 0));
        fs->frame->view_id = fs->view_id;
        write_picture(fs->frame, p_Vid);
    }

    if (fs->is_used & 0x02) { // we have a bottom field, construct an empty top field
        p = fs->bottom_field;
        fs->top_field = alloc_storable_picture(p_Vid, TOP_FIELD);
        MEM_CHECK(ret, fs->top_field);
        fs->top_field->mem_malloc_type = Mem_UnPaired;
        fs->top_field->chroma_format_idc = p->chroma_format_idc;
        fs->top_field->frame_cropping_flag = fs->bottom_field->frame_cropping_flag;
        if (fs->top_field->frame_cropping_flag) {
            fs->top_field->frame_crop_top_offset = fs->bottom_field->frame_crop_top_offset;
            fs->top_field->frame_crop_bottom_offset = fs->bottom_field->frame_crop_bottom_offset;
            fs->top_field->frame_crop_left_offset = fs->bottom_field->frame_crop_left_offset;
            fs->top_field->frame_crop_right_offset = fs->bottom_field->frame_crop_right_offset;
        }
        FUN_CHECK(ret = dpb_combine_field_yuv(p_Vid, fs, 0));
        fs->frame->view_id = fs->view_id;
        write_picture(fs->frame, p_Vid);
    }
    fs->is_used = 3;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET flush_direct_output(H264dVideoCtx_t *p_Vid)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    FUN_CHECK(ret = write_unpaired_field(p_Vid, &p_Vid->out_buffer));
    free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.frame);
    p_Vid->out_buffer.frame = NULL;
    free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.top_field);
    p_Vid->out_buffer.top_field = NULL;
    free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.bottom_field);
    p_Vid->out_buffer.bottom_field = NULL;
    p_Vid->out_buffer.is_used = 0;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET write_stored_frame(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    //!< make sure no direct output field is pending
    FUN_CHECK(ret = flush_direct_output(p_Vid));

    if (fs->is_used < 3) {
        FUN_CHECK(ret = write_unpaired_field(p_Vid, fs));
    } else {
        write_picture(fs->frame, p_Vid);
    }

    fs->is_output = 1;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET output_one_frame_from_dpb(H264_DpbBuf_t *p_Dpb)
{
    RK_S32 poc = 0, pos = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    //!< diagnostics
    VAL_CHECK(ret, p_Dpb->used_size > 0); //!< Cannot output frame, DPB empty
    //!< find smallest POC
    FUN_CHECK(ret = get_smallest_poc(p_Dpb, &poc, &pos));
    VAL_CHECK(ret, pos != -1);
    //!< JVT-P072 ends
    FUN_CHECK(ret = write_stored_frame(p_Vid, p_Dpb->fs[pos]));
    //!< free frame store and move empty store to end of buffer
    if (!is_used_for_reference(p_Dpb->fs[pos])) {
        FUN_CHECK(ret = remove_frame_from_dpb(p_Dpb, pos));
    }
    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET adaptive_memory_management(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p)
{
    H264_DRPM_t *tmp_drpm = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    p_Vid->last_has_mmco_5 = 0;
    VAL_CHECK(ret, !p->idr_flag && p->adaptive_ref_pic_buffering_flag);
    while (p->dec_ref_pic_marking_buffer) {
        tmp_drpm = p->dec_ref_pic_marking_buffer;
        switch (tmp_drpm->memory_management_control_operation) {
        case 0:
            VAL_CHECK(ret, tmp_drpm->Next == NULL);
            break;
        case 1:
            mm_unmark_short_term_for_reference(p_Dpb, p, tmp_drpm->difference_of_pic_nums_minus1);
            update_ref_list(p_Dpb);
            break;
        case 2:
            mm_unmark_long_term_for_reference(p_Dpb, p, tmp_drpm->long_term_pic_num);
            update_ltref_list(p_Dpb);
            break;
        case 3:
            mm_assign_long_term_frame_idx(p_Dpb, p, tmp_drpm->difference_of_pic_nums_minus1, tmp_drpm->long_term_frame_idx);
            update_ref_list(p_Dpb);
            update_ltref_list(p_Dpb);
            break;
        case 4:
            mm_update_max_long_term_frame_idx(p_Dpb, tmp_drpm->max_long_term_frame_idx_plus1);
            update_ltref_list(p_Dpb);
            break;
        case 5:
            mm_unmark_all_short_term_for_reference(p_Dpb);
            mm_unmark_all_long_term_for_reference(p_Dpb);
            p_Vid->last_has_mmco_5 = 1;
            break;
        case 6:
            mm_mark_current_picture_long_term(p_Dpb, p, tmp_drpm->long_term_frame_idx);
            check_num_ref(p_Dpb);
            break;
        default:
            ret = MPP_NOK;
            goto __FAILED;
        }
        p->dec_ref_pic_marking_buffer = tmp_drpm->Next;
        MPP_FREE(tmp_drpm);
    }
    if (p_Vid->last_has_mmco_5) { //!< similar IDR frame
        p->pic_num = p->frame_num = 0;
        switch (p->structure) {
        case TOP_FIELD:
            p->is_mmco_5 = 1;
            p->top_poc_mmco5 = p->top_poc;
            p->poc = p->top_poc = 0;
            break;

        case BOTTOM_FIELD:
            p->is_mmco_5 = 1;
            p->bot_poc_mmco5 = p->bottom_poc;
            p->poc = p->bottom_poc = 0;
            break;

        case FRAME:
            p->is_mmco_5 = 1;
            p->top_poc_mmco5 = p->top_poc;
            p->bot_poc_mmco5 = p->bottom_poc;
            p->poc_mmco5 = MPP_MIN(p->top_poc, p->bottom_poc);
            p->top_poc -= p->poc;
            p->bottom_poc -= p->poc;

            p->poc = MPP_MIN(p->top_poc, p->bottom_poc);
            p->frame_poc = p->poc;
            break;

        }
        if (p->layer_id == 0) {
            FUN_CHECK(ret = flush_dpb(p_Vid->p_Dpb_layer[0], 1));
        } else {
            FUN_CHECK(ret = flush_dpb(p_Vid->p_Dpb_layer[1], 2));
        }

    }
    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET dpb_split_field(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_StorePic_t *frame = fs->frame;
    H264_StorePic_t *fs_top = NULL, *fs_btm = NULL;

    fs->poc = frame->poc;
    if (!frame->frame_mbs_only_flag) {
        fs_top = fs->top_field    = alloc_storable_picture(p_Vid, TOP_FIELD);
        fs_btm = fs->bottom_field = alloc_storable_picture(p_Vid, BOTTOM_FIELD);
        MEM_CHECK(ret, fs_top && fs_btm);
        fs_top->colmv_no_used_flag = frame->colmv_no_used_flag;
        fs_btm->colmv_no_used_flag = frame->colmv_no_used_flag;

        if (frame->mem_malloc_type == Mem_Malloc || frame->mem_malloc_type == Mem_Clone) {
            fs_top->mem_mark = frame->mem_mark;
            fs_btm->mem_mark = frame->mem_mark;
            fs_top->mem_malloc_type = frame->mem_malloc_type;
            fs_btm->mem_malloc_type = frame->mem_malloc_type;
            frame->mem_mark->bot_used += 1;    // picture memory add 1
            frame->mem_mark->top_used += 1;
        }
        fs_top->poc = frame->top_poc;
        fs_btm->poc = frame->bottom_poc;
        fs_top->layer_id = frame->layer_id;
        fs_btm->layer_id = frame->layer_id;

        fs_top->view_id = frame->view_id;
        fs_btm->view_id = frame->view_id;
        fs_top->frame_poc = frame->frame_poc;

        fs_top->bottom_poc = fs_btm->bottom_poc = frame->bottom_poc;
        fs_top->top_poc = fs_btm->top_poc = frame->top_poc;
        fs_btm->frame_poc = frame->frame_poc;

        fs_top->used_for_reference = fs_btm->used_for_reference = frame->used_for_reference;
        fs_top->is_long_term = fs_btm->is_long_term = frame->is_long_term;
        fs->long_term_frame_idx = fs_top->long_term_frame_idx = fs_btm->long_term_frame_idx = frame->long_term_frame_idx;
        fs_top->mb_aff_frame_flag = fs_btm->mb_aff_frame_flag = frame->mb_aff_frame_flag;

        frame->top_field = fs_top;
        frame->bottom_field = fs_btm;
        frame->frame = frame;
        fs_top->bottom_field = fs_btm;
        fs_top->frame = frame;
        fs_top->top_field = fs_top;
        fs_btm->top_field = fs_top;
        fs_btm->frame = frame;
        fs_btm->bottom_field = fs_btm;

        fs_top->is_mmco_5 = frame->is_mmco_5;
        fs_btm->is_mmco_5 = frame->is_mmco_5;
        fs_top->poc_mmco5 = frame->poc_mmco5;
        fs_btm->poc_mmco5 = frame->poc_mmco5;
        fs_top->top_poc_mmco5 = frame->top_poc_mmco5;
        fs_btm->bot_poc_mmco5 = frame->bot_poc_mmco5;

        fs_top->view_id = fs_btm->view_id = fs->view_id;
        fs_top->inter_view_flag = fs->inter_view_flag[0];
        fs_btm->inter_view_flag = fs->inter_view_flag[1];

        fs_top->chroma_format_idc = fs_btm->chroma_format_idc = frame->chroma_format_idc;
        fs_top->iCodingType = fs_btm->iCodingType = frame->iCodingType;
    } else {
        fs->top_field = NULL;
        fs->bottom_field = NULL;
        frame->top_field = NULL;
        frame->bottom_field = NULL;
        frame->frame = frame;
    }
    return ret = MPP_OK;
__FAILED:
    MPP_FREE(fs->top_field);
    MPP_FREE(fs->bottom_field);
    ASSERT(0);
    return ret;
}

static MPP_RET dpb_combine_field(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs, RK_U8 combine_flag)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    FUN_CHECK(ret = dpb_combine_field_yuv(p_Vid, fs, combine_flag));
    fs->frame->layer_id = fs->layer_id;
    fs->frame->view_id = fs->view_id;
    fs->frame->iCodingType = fs->top_field->iCodingType; //FIELD_CODING;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET direct_output(H264dVideoCtx_t *p_Vid, H264_StorePic_t *p)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (p->structure == FRAME) {
        //!< we have a frame (or complementary field pair), so output it directly
        FUN_CHECK(ret = flush_direct_output(p_Vid));
        write_picture(p, p_Vid);
        free_storable_picture(p_Vid->p_Dec, p);
        goto __RETURN;
    }

    if (p->structure == TOP_FIELD) {
        if (p_Vid->out_buffer.is_used & 1) {
            FUN_CHECK(ret = flush_direct_output(p_Vid));
        }
        p_Vid->out_buffer.top_field = p;
        p_Vid->out_buffer.is_used |= 1;
    }

    if (p->structure == BOTTOM_FIELD) {
        if (p_Vid->out_buffer.is_used & 2) {
            FUN_CHECK(ret = flush_direct_output(p_Vid));
        }
        p_Vid->out_buffer.bottom_field = p;
        p_Vid->out_buffer.is_used |= 2;
    }

    if (p_Vid->out_buffer.is_used == 3) {
        //!< we have both fields, so output them
        FUN_CHECK(ret = dpb_combine_field_yuv(p_Vid, &p_Vid->out_buffer, 0));
        p_Vid->out_buffer.frame->view_id = p_Vid->out_buffer.view_id;
        write_picture(p_Vid->out_buffer.frame, p_Vid);
        free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.frame);
        p_Vid->out_buffer.frame = NULL;
        free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.top_field);
        p_Vid->out_buffer.top_field = NULL;
        free_storable_picture(p_Vid->p_Dec, p_Vid->out_buffer.bottom_field);
        p_Vid->out_buffer.bottom_field = NULL;
        p_Vid->out_buffer.is_used = 0;
    }

__RETURN:
    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

static MPP_RET store_picture_in_dpb(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p)
{
    RK_S32 min_poc = 0, min_pos = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    VAL_CHECK(ret, NULL != p);  //!< if frame, check for new store
    //!< deal with all frames in dpb
    p_Vid->last_has_mmco_5 = 0;
    p_Vid->last_pic_bottom_field = p->structure == BOTTOM_FIELD;
    if (p->idr_flag) {
        FUN_CHECK(ret = idr_memory_management(p_Dpb, p));
    } else {    //!< adaptive memory management
        if (p->used_for_reference && p->adaptive_ref_pic_buffering_flag) {
            FUN_CHECK(ret = adaptive_memory_management(p_Dpb, p));
        }
    }
    //!< if necessary, combine top and botteom to frame
    if (get_filed_dpb_combine_flag(p_Dpb, p)) {
        FUN_CHECK(ret = insert_picture_in_dpb(p_Vid, p_Dpb->last_picture, p, 1));  //!< field_dpb_combine
        update_ref_list(p_Dpb);
        update_ltref_list(p_Dpb);
        p_Dpb->last_picture = NULL;
        goto __RETURN;
    }
    //!< sliding window
    if (!p->idr_flag && p->used_for_reference && !p->adaptive_ref_pic_buffering_flag) {
        ASSERT(!p->idr_flag);
        sliding_window_memory_management(p_Dpb);
        p->is_long_term = 0;
    }
    //!< when is full, first try to remove unused frames
    if (p_Dpb->used_size == p_Dpb->size) {
        remove_unused_frame_from_dpb(p_Dpb);
    }
    //!< then output frames until one can be removed
    while (p_Dpb->used_size == p_Dpb->size) {
        //!< non-reference frames may be output directly
        FUN_CHECK(ret = get_smallest_poc(p_Dpb, &min_poc, &min_pos));
        //!< current not used reference
        if (!p->used_for_reference) {
            if ((-1 == min_pos) || (p->poc < min_poc)) {
                FUN_CHECK(ret = direct_output(p_Vid, p));  //!< output frame
                goto __RETURN;
            }
        }
        //!< used for reference, so, flush a frame
        if ((-1 == min_pos) || (p->poc < min_poc)) {
            FUN_CHECK(ret = remove_frame_from_dpb(p_Dpb, 0));
        }
        FUN_CHECK(ret = output_one_frame_from_dpb(p_Dpb));
    }
    //!< store current decoder picture at end of dpb
    FUN_CHECK(ret = insert_picture_in_dpb(p_Vid, p_Dpb->fs[p_Dpb->used_size], p, 0));
    if (p->structure != FRAME) {
        p_Dpb->last_picture = p_Dpb->fs[p_Dpb->used_size];
    } else {
        p_Dpb->last_picture = NULL;
    }
    p_Dpb->used_size++;
    update_ref_list(p_Dpb);
    update_ltref_list(p_Dpb);

    mpp_buf_slot_set_flag(p_Vid->p_Dec->frame_slots, p->mem_mark->slot_idx, SLOT_CODEC_USE);

__RETURN:
    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

/*!
***********************************************************************
* \brief
*    free frame store picture
***********************************************************************
*/
//extern "C"
void free_frame_store(H264_DecCtx_t *p_Dec, H264_FrameStore_t* f)
{
    if (f) {
        if (f->frame) {
            free_storable_picture(p_Dec, f->frame);
            f->frame = NULL;
        }
        if (f->top_field) {
            free_storable_picture(p_Dec, f->top_field);
            f->top_field = NULL;
        }
        if (f->bottom_field) {
            free_storable_picture(p_Dec, f->bottom_field);
            f->bottom_field = NULL;
        }
        MPP_FREE(f);
    }
}

/*!
***********************************************************************
* \brief
*    free dpb
***********************************************************************
*/
//extern "C"
void free_dpb(H264_DpbBuf_t *p_Dpb)
{
    RK_U32 i = 0;
    H264dVideoCtx_t *p_Vid = p_Dpb->p_Vid;

    if (p_Dpb->fs) {
        for (i = 0; i < p_Dpb->size; i++) {
            free_frame_store(p_Vid->p_Dec, p_Dpb->fs[i]);
        }
        MPP_FREE(p_Dpb->fs);
    }
    MPP_FREE(p_Dpb->fs_ref);
    MPP_FREE(p_Dpb->fs_ltref);
    if (p_Dpb->fs_ilref) {
        for (i = 0; i < 1; i++) {
            free_frame_store(p_Vid->p_Dec, p_Dpb->fs_ilref[i]);
        }
        MPP_FREE(p_Dpb->fs_ilref);
    }
    p_Dpb->last_output_view_id = -1;
    p_Dpb->last_output_poc = INT_MIN;
    p_Dpb->init_done = 0;
    if (p_Vid->no_ref_pic) {
        free_storable_picture(p_Vid->p_Dec, p_Vid->no_ref_pic);
        p_Vid->no_ref_pic = NULL;
    }
}

/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
void update_ref_list(H264_DpbBuf_t *p_Dpb)
{
    RK_U8 i = 0, j = 0;
    for (i = 0, j = 0; i < p_Dpb->used_size; i++) {
        if (is_short_term_reference(p_Dpb->fs[i])) {
            p_Dpb->fs_ref[j++] = p_Dpb->fs[i];
        }
    }

    p_Dpb->ref_frames_in_buffer = j;

    while (j < p_Dpb->size) {
        p_Dpb->fs_ref[j++] = NULL;
    }
}
/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
void update_ltref_list(H264_DpbBuf_t *p_Dpb)
{
    RK_U8 i = 0, j = 0;
    for (i = 0, j = 0; i < p_Dpb->used_size; i++) {
        if (is_long_term_reference(p_Dpb->fs[i])) {
            p_Dpb->fs_ltref[j++] = p_Dpb->fs[i];
        }
    }

    p_Dpb->ltref_frames_in_buffer = j;

    while (j < p_Dpb->size) {
        p_Dpb->fs_ltref[j++] = NULL;
    }
}

/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
MPP_RET idr_memory_management(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p)
{
    RK_U32 i = 0;
    RK_S32 type = -1;
    MPP_RET ret = MPP_ERR_UNKNOW;

    if (p->no_output_of_prior_pics_flag) {
        //!< free all stored pictures
        for (i = 0; i < p_Dpb->used_size; i++) {
            //!< reset all reference settings
            free_frame_store(p_Dpb->p_Vid->p_Dec, p_Dpb->fs[i]);
            p_Dpb->fs[i] = alloc_frame_store();
            MEM_CHECK(ret, p_Dpb->fs[i]);
        }
        for (i = 0; i < p_Dpb->ref_frames_in_buffer; i++) {
            p_Dpb->fs_ref[i] = NULL;
        }
        for (i = 0; i < p_Dpb->ltref_frames_in_buffer; i++) {
            p_Dpb->fs_ltref[i] = NULL;
        }
        p_Dpb->used_size = 0;
    } else {
        type = (p->layer_id == 0) ? 1 : 2;
        FUN_CHECK(ret = flush_dpb(p_Dpb, type));
    }
    p_Dpb->last_picture = NULL;

    update_ref_list(p_Dpb);
    update_ltref_list(p_Dpb);
    p_Dpb->last_output_poc = INT_MIN;

    if (p->long_term_reference_flag) {
        p_Dpb->max_long_term_pic_idx = 0;
        p->is_long_term = 1;
        p->long_term_frame_idx = 0;
    } else {
        p_Dpb->max_long_term_pic_idx = -1;
        p->is_long_term = 0;
    }
    p_Dpb->last_output_view_id = -1;

    return ret = MPP_OK;

__FAILED:
    for (i = 0; i < p_Dpb->used_size; i++) { // when error and free malloc buf
        free_frame_store(p_Dpb->p_Vid->p_Dec, p_Dpb->fs[i]);
    }
    ASSERT(0);
    return ret;
}


/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
MPP_RET insert_picture_in_dpb(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs, H264_StorePic_t *p, RK_U8 combine_flag)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    ASSERT(p  != NULL);
    ASSERT(fs != NULL);
    switch (p->structure) {
    case FRAME:
        fs->frame = p;
        fs->is_used = 3;
        if (p->used_for_reference) {
            fs->is_reference = 3;
            fs->is_orig_reference = 3;
            if (p->is_long_term) {
                fs->is_long_term = 3;
                fs->long_term_frame_idx = p->long_term_frame_idx;
            }
        }
        fs->layer_id = p->layer_id;
        fs->view_id = p->view_id;
        fs->inter_view_flag[0] = fs->inter_view_flag[1] = p->inter_view_flag;
        fs->anchor_pic_flag[0] = fs->anchor_pic_flag[1] = p->anchor_pic_flag;
        FUN_CHECK(ret = dpb_split_field(p_Vid, fs));
        break;
    case TOP_FIELD:
        fs->top_field = p;
        fs->is_used |= 1;
        fs->layer_id = p->layer_id;
        fs->view_id = p->view_id;
        fs->inter_view_flag[0] = p->inter_view_flag;
        fs->anchor_pic_flag[0] = p->anchor_pic_flag;
        if (p->used_for_reference) {
            fs->is_reference |= 1;
            fs->is_orig_reference |= 1;
            if (p->is_long_term) {
                fs->is_long_term |= 1;
                fs->long_term_frame_idx = p->long_term_frame_idx;
            }
        }
        if (fs->is_used == 3) {
            FUN_CHECK(ret = dpb_combine_field(p_Vid, fs, combine_flag));
        } else {
            fs->poc = p->poc;
        }
        break;
    case BOTTOM_FIELD:
        fs->bottom_field = p;
        fs->is_used |= 2;
        fs->layer_id = p->layer_id;
        fs->view_id = p->view_id;
        fs->inter_view_flag[1] = p->inter_view_flag;
        fs->anchor_pic_flag[1] = p->anchor_pic_flag;
        if (p->used_for_reference) {
            fs->is_reference |= 2;
            fs->is_orig_reference |= 2;
            if (p->is_long_term) {
                fs->is_long_term |= 2;
                fs->long_term_frame_idx = p->long_term_frame_idx;
            }
        }
        if (fs->is_used == 3) {
            FUN_CHECK(ret = dpb_combine_field(p_Vid, fs, combine_flag));
        } else {
            fs->poc = p->poc;
        }
        break;
    }
    fs->frame_num = p->pic_num;
    fs->is_output = p->is_output;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}


/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
void free_storable_picture(H264_DecCtx_t *p_Dec, H264_StorePic_t *p)
{
    if (p) {
        if (p->mem_malloc_type == Mem_Malloc || p->mem_malloc_type == Mem_Clone) {
            free_dpb_memory_index(p_Dec, p->mem_mark, p->structure);
        }

        MPP_FREE(p);
    }
}

/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"
H264_StorePic_t *alloc_storable_picture(H264dVideoCtx_t *p_Vid, RK_S32 structure)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_StorePic_t *s = mpp_calloc(H264_StorePic_t, 1);

    MEM_CHECK(ret, s);
    s->view_id = -1;
    s->structure = structure;
    (void)p_Vid;

    return s;
__FAILED:
    (void)ret;
    return NULL;
}

/*!
***********************************************************************
* \brief
*    alloc one picture
***********************************************************************
*/
//extern "C"

RK_U32 get_filed_dpb_combine_flag(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p)
{
    RK_U32 combine_flag = 0;

    if ((p->structure == TOP_FIELD) || (p->structure == BOTTOM_FIELD)) {
        // check for frame store with same pic_number
        if (p_Dpb->last_picture) {
            if ((RK_S32)p_Dpb->last_picture->frame_num == p->pic_num) {
                if (((p->structure == TOP_FIELD) && (p_Dpb->last_picture->is_used == 2))
                    || ((p->structure == BOTTOM_FIELD) && (p_Dpb->last_picture->is_used == 1))) {
                    if ((p->used_for_reference && p_Dpb->last_picture->is_orig_reference) ||
                        (!p->used_for_reference && !p_Dpb->last_picture->is_orig_reference)) {
                        return combine_flag = 1;
                    }
                }
            }
        }
    }

    return combine_flag = 0;
}

/*!
***********************************************************************
* \brief
*    init dpb
***********************************************************************
*/
//extern "C"
MPP_RET init_dpb(H264dVideoCtx_t *p_Vid, H264_DpbBuf_t *p_Dpb, RK_S32 type)  // type=1 AVC type=2 MVC
{
    RK_U32 i = 0;
    RK_S32 PicSizeInMbs = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_SPS_t *active_sps = p_Vid->active_sps;
    LogCtx_t *runlog = p_Dpb->p_Vid->p_Dec->logctx.parr[RUN_PARSE];

    p_Dpb->p_Vid = p_Vid;
    if (p_Dpb->init_done) {
        free_dpb(p_Dpb);
    }
    p_Dpb->size = getDpbSize(p_Vid, active_sps) + (type == 2 ? 0 : 1);
    p_Dpb->num_ref_frames = active_sps->max_num_ref_frames;
    if (active_sps->max_dec_frame_buffering < active_sps->max_num_ref_frames) {
        LogError(runlog, "DPB size at specified level is smaller than reference frames");
        goto __FAILED;
    }
    p_Dpb->used_size = 0;
    p_Dpb->last_picture = NULL;
    p_Dpb->ref_frames_in_buffer = 0;
    p_Dpb->ltref_frames_in_buffer = 0;
    //--------
    p_Dpb->fs       = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
    p_Dpb->fs_ref   = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
    p_Dpb->fs_ltref = mpp_calloc(H264_FrameStore_t*, p_Dpb->size);
    p_Dpb->fs_ilref = mpp_calloc(H264_FrameStore_t*, 1);  //!< inter-layer reference (for multi-layered codecs)
    MEM_CHECK(ret, p_Dpb->fs && p_Dpb->fs_ref && p_Dpb->fs_ltref && p_Dpb->fs_ilref);
    for (i = 0; i < p_Dpb->size; i++) {
        p_Dpb->fs[i] = alloc_frame_store();
        MEM_CHECK(ret, p_Dpb->fs[i]);
        p_Dpb->fs_ref[i] = NULL;
        p_Dpb->fs_ltref[i] = NULL;
        p_Dpb->fs[i]->layer_id = -1;
        p_Dpb->fs[i]->view_id = -1;
        p_Dpb->fs[i]->inter_view_flag[0] = p_Dpb->fs[i]->inter_view_flag[1] = 0;
        p_Dpb->fs[i]->anchor_pic_flag[0] = p_Dpb->fs[i]->anchor_pic_flag[1] = 0;
    }
    if (type == 2) {
        p_Dpb->fs_ilref[0] = alloc_frame_store();
        MEM_CHECK(ret, p_Dpb->fs_ilref[0]);
        //!< These may need some cleanups
        p_Dpb->fs_ilref[0]->view_id = -1;
        p_Dpb->fs_ilref[0]->inter_view_flag[0] = p_Dpb->fs_ilref[0]->inter_view_flag[1] = 0;
        p_Dpb->fs_ilref[0]->anchor_pic_flag[0] = p_Dpb->fs_ilref[0]->anchor_pic_flag[1] = 0;
        //!< given that this is in a different buffer, do we even need proc_flag anymore?
    } else {
        p_Dpb->fs_ilref[0] = NULL;
    }
    //!< allocate a dummy storable picture
    if (!p_Vid->no_ref_pic) {
        PicSizeInMbs = (p_Vid->active_sps->pic_height_in_map_units_minus1 + 1) * (p_Vid->active_sps->pic_width_in_mbs_minus1 + 1);
        p_Vid->no_ref_pic = alloc_storable_picture(p_Vid, FRAME);
        MEM_CHECK(ret, p_Vid->no_ref_pic);
        p_Vid->no_ref_pic->top_field = p_Vid->no_ref_pic;
        p_Vid->no_ref_pic->bottom_field = p_Vid->no_ref_pic;
        p_Vid->no_ref_pic->frame = p_Vid->no_ref_pic;
    }
    p_Dpb->last_output_poc = INT_MIN;
    p_Dpb->last_output_view_id = -1;
    p_Vid->last_has_mmco_5 = 0;
    p_Dpb->init_done = 1;
    (void)PicSizeInMbs;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}
/*!
***********************************************************************
* \brief
*    flush dpb
***********************************************************************
*/
//extern "C"

MPP_RET flush_dpb(H264_DpbBuf_t *p_Dpb, RK_S32 type)
{
    RK_U32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;

    //!< diagnostics
    if (!p_Dpb->init_done) {
        goto __RETURN;
    }
    //!< mark all frames unused
    for (i = 0; i < p_Dpb->used_size; i++) {
        VAL_CHECK(ret, p_Dpb->fs[i]->layer_id == p_Dpb->layer_id);
        unmark_for_reference(p_Dpb->p_Vid->p_Dec, p_Dpb->fs[i]);
    }
    //!< mark all inter-view reference unsed
    //if (type == 2)
    //{
    //  //!< free
    //  free_frame_store(p_Dpb->p_Vid->p_Dec, p_Dpb->fs_ilref[0]);
    //  //!< malloc
    //  p_Dpb->fs_ilref[0] = alloc_frame_store();
    //  MEM_CHECK(ret, p_Dpb->fs_ilref[0]);
    //  //!< These may need some cleanups
    //  p_Dpb->fs_ilref[0]->view_id = -1;
    //  p_Dpb->fs_ilref[0]->inter_view_flag[0] = p_Dpb->fs_ilref[0]->inter_view_flag[1] = 0;
    //  p_Dpb->fs_ilref[0]->anchor_pic_flag[0] = p_Dpb->fs_ilref[0]->anchor_pic_flag[1] = 0;
    //}
    while (!remove_unused_frame_from_dpb(p_Dpb));
    //!< output frames in POC order
    while (p_Dpb->used_size) {
        FUN_CHECK(ret = output_one_frame_from_dpb(p_Dpb));
    }
    p_Dpb->last_output_poc = INT_MIN;
	(void)type;
__RETURN:
    return ret = MPP_OK;
__FAILED:
    return ret;
}

/*!
***********************************************************************
* \brief
*    parse sps and process sps
***********************************************************************
*/
//extern "C"
MPP_RET exit_picture(H264dVideoCtx_t *p_Vid, H264_StorePic_t **dec_pic)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    //!< return if the last picture has already been finished
    if (!(*dec_pic) || !p_Vid->exit_picture_flag
        || !p_Vid->have_outpicture_flag || !p_Vid->iNumOfSlicesDecoded) {
        goto __RETURN;
    }

#if 0
    set_curframe_poc((*dec_pic), &p_Vid->p_Dec->regs);
#endif
    FUN_CHECK(ret = store_picture_in_dpb(p_Vid->p_Dpb_layer[(*dec_pic)->layer_id], *dec_pic));
//   FPRINT(g_debug_file0, "decoder ending, g_framecnt=%d \n", p_Vid->g_framecnt);
    //FPRINT(g_debug_file1, "decoder ending, g_framecnt=%d \n", p_Vid->g_framecnt);
    //mpp_log("[TRACE] exit picture, g_framecnt=%d \n", p_Vid->g_framecnt++);

#if 0
    update_all_logctx_framenum(&p_Vid->p_Dec->logctx, p_Vid->g_framecnt);
#endif
    *dec_pic = NULL;

__RETURN:
    return ret = MPP_OK;

__FAILED:
    ASSERT(0);
    return ret;
}


static void reset_dpb_info(H264_DpbInfo_t *p)
{
	p->picbuf = NULL;
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



static MPP_RET adjust_input(H264_SLICE_t *currSlice)
{
	RK_U32 i = 0, j = 0;
	RK_U32 find_flag = 0;

	H264_DecCtx_t *p_Dec = currSlice->p_Dec;
	H264_DpbInfo_t *new_dpb = p_Dec->dpb_info;
	H264_DpbInfo_t *old_dpb = p_Dec->dpb_old[currSlice->layer_id];


	//for (i = 0; i < MAX_DPB_SIZE; i++) {
	//	if (new_dpb[i].picbuf) {
	//		FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", i, new_dpb[i].frame_num,  new_dpb[i].frame_num);
	//		FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", new_dpb[i].slot_index, new_dpb[i].is_long_term,
	//			new_dpb[i].TOP_POC);
	//	}
	//}
	//FPRINT(g_debug_file0, "--------- [new DPB] -------- \n");
	//for (i = 0; i < MAX_DPB_SIZE; i++) {
	//	if (old_dpb[i].picbuf) {
	//		FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", i, old_dpb[i].frame_num,  old_dpb[i].frame_num);
	//		FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", old_dpb[i].slot_index, old_dpb[i].is_long_term,
	//			old_dpb[i].TOP_POC);
	//	}
	//}
	//FPRINT(g_debug_file0, "--------- [Old DPB] -------- \n");

	//!< delete old dpb
	for (i = 0; i < MAX_DPB_SIZE; i++) {
		find_flag = 0;
		if (old_dpb[i].picbuf) {
			for (j = 0; j < MAX_DPB_SIZE; j++) {
				if(new_dpb[j].picbuf) {
					find_flag = (old_dpb[i].frame_num == new_dpb[j].frame_num) ? 1 : 0;
					find_flag = (old_dpb[i].slot_index == new_dpb[j].slot_index) ? find_flag : 0;
					if (new_dpb[j].is_used & 0x1) {
						find_flag = (old_dpb[i].TOP_POC == new_dpb[j].TOP_POC) ? find_flag : 0;
					}
					if (new_dpb[j].is_used & 0x2) {
						find_flag = (old_dpb[i].BOT_POC == new_dpb[j].BOT_POC) ? find_flag : 0;
					}
					if (find_flag) { //!< found
						new_dpb[j].have_same = 1;
						break;
					}
				}
			}
		}
		//!< not found
		if (find_flag == 0) {
			reset_dpb_info(&old_dpb[i]);
		}
	}
	//!< add new dpb
	for (j = 0; j < MAX_DPB_SIZE; j++) {
		if ((new_dpb[j].picbuf == 0) || new_dpb[j].have_same) {
			continue;
		}
		for (i = 0; i < MAX_DPB_SIZE; i++) {
			if (old_dpb[i].picbuf == 0) {
				old_dpb[i] = new_dpb[j];
				break;
			}
		}
	}
	//memcpy(new_dpb, old_dpb, MAX_DPB_SIZE*sizeof(H264_DpbInfo_t));
	memset(new_dpb, 0, MAX_DPB_SIZE*sizeof(H264_DpbInfo_t));
	for (i = 0, j = 0; i < MAX_DPB_SIZE; i++) {
		if (old_dpb[i].picbuf) {
			new_dpb[j] = old_dpb[i];
			//FPRINT(g_debug_file0, "i=%2d, picnum=%d, framenum=%2d, ", j, old_dpb[j].frame_num,  old_dpb[j].frame_num);
			//FPRINT(g_debug_file0, " dbp_idx=%d, longterm=%d, poc=%d \n", old_dpb[j].slot_index, old_dpb[j].is_long_term,
			//	old_dpb[i].TOP_POC);
			j++;
		}
	}	
	for (; j < 16; j++) {
		reset_dpb_info(&new_dpb[j]);
	}
	memcpy(old_dpb, new_dpb, MAX_DPB_SIZE*sizeof(H264_DpbInfo_t));

	return MPP_OK;
}



/*!
***********************************************************************
* \brief
*    prepare dpb info for regsiter syntax
***********************************************************************
*/
//extern "C"
MPP_RET prepare_init_dpb_info(H264_SLICE_t *currSlice)
{
    RK_U32 i = 0, j = 0;
    H264_DpbBuf_t *p_Dpb = currSlice->p_Dpb;
    H264_DecCtx_t *p_Dec = currSlice->p_Dec;

    memset(p_Dec->dpb_info, 0, MAX_DPB_SIZE * sizeof(H264_DpbInfo_t));
	memset(&p_Dec->in_task->refer, -1, sizeof(p_Dec->in_task->refer));
    if (currSlice->idr_flag && (currSlice->layer_id == 0)) { // idr_flag==1 && layer_id==0
		memset(p_Dec->dpb_old[0], 0, MAX_DPB_SIZE * sizeof(H264_DpbInfo_t));
		memset(p_Dec->dpb_old[1], 0, MAX_DPB_SIZE * sizeof(H264_DpbInfo_t));
        goto __RETURN;
    }
    //!<---- reference
    for (i = 0, j = 0; j < p_Dpb->ref_frames_in_buffer; i++, j++) {
        if (p_Dpb->fs_ref[j]->is_used == 3) {
            p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ref[j]->frame;
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
                p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ref[j]->top_field;

                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = 0;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ref[j]->top_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ref[j]->top_field->colmv_no_used_flag ? 0 : 1);
            } else { // if(p_Dpb->fs_ref[j]->is_used & 0x2) // bottom
                p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ref[j]->bottom_field;
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
    //!<---- long term reference
    for (j = 0; j < p_Dpb->ltref_frames_in_buffer; i++, j++) {
        if (p_Dpb->fs_ltref[j]->is_used == 3) {
            p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ltref[j]->frame;

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
                p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ltref[j]->top_field;
                p_Dec->dpb_info[i].TOP_POC = p_Dpb->fs_ltref[j]->top_field->poc;
                p_Dec->dpb_info[i].BOT_POC = 0;
                p_Dec->dpb_info[i].field_flag = 1;
                p_Dec->dpb_info[i].slot_index = p_Dpb->fs_ltref[j]->top_field->mem_mark->slot_idx;
                p_Dec->dpb_info[i].colmv_is_used = (p_Dpb->fs_ltref[j]->top_field->colmv_no_used_flag ? 0 : 1);
                p_Dec->dpb_info[i].long_term_pic_num = p_Dpb->fs_ltref[j]->top_field->long_term_pic_num;
            } else { // if(p_Dpb->fs_ref[j]->is_used & 0x2)
                p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ltref[j]->bottom_field;
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

#if 1
	//!< inter-layer reference (for multi-layered codecs)
	for (j = 0; j < p_Dpb->used_size_il; i++, j++) {
		if (currSlice->structure == FRAME && p_Dpb->fs_ilref[j]->is_used == 3) {
			if (p_Dpb->fs_ilref[j]->inter_view_flag[0] == 0 && p_Dpb->fs_ilref[j]->inter_view_flag[1] == 0)
				break;
			p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ilref[j]->frame;

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
				p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ilref[j]->top_field;

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
				p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ilref[j]->top_field;

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
				p_Dec->dpb_info[i].picbuf = p_Dpb->fs_ilref[j]->bottom_field;

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

	//for(j = 0; j < i; j++)    {
	//	FPRINT(g_debug_file1, "i=%2d, picnum=%d, framenum=%2d, ", j, p_Dec->dpb_info[j].frame_num, p_Dec->dpb_info[j].frame_num);
	//	FPRINT(g_debug_file1, " dbp_idx=%d, longterm=%d, poc=%d \n", p_Dec->dpb_info[j].slot_index, p_Dec->dpb_info[j].is_long_term,
	//		p_Dec->dpb_info[j].TOP_POC);
	//}
	//FPRINT(g_debug_file1, "--------- [FLUSH_CNT] flush framecnt=%d -------- \n", p_Dec->p_Vid->g_framecnt);

	//if (p_Dec->p_Vid->g_framecnt == 255) {
	//    i = i;
	//}

	//!< reset left parameters
	for (; i < 16; i++) {
		reset_dpb_info(&p_Dec->dpb_info[i]);
	}

	//FPRINT(g_debug_file0, "---g_framecnt=%d\n", p_Dec->p_Vid->g_framecnt);
	//for (i = 0; i< 16; i++)
	//{
	//  FPRINT(g_debug_file0, "i=%d, top_poc=%d, bot_poc=%d, frame_num=%d, is_lt=%d, voidx=%d\n", i, p_Dec->dpb_info[i].TOP_POC,
	//      p_Dec->dpb_info[i].BOT_POC, p_Dec->dpb_info[i].frame_num, p_Dec->dpb_info[i].is_ilt_flag, p_Dec->dpb_info[i].voidx);
	//}



	adjust_input(currSlice);

	for (i = 0; i < MAX_DPB_SIZE; i++) {
		if (p_Dec->dpb_info[i].picbuf && (p_Dec->dpb_info[i].slot_index >= 0)) {
			p_Dec->in_task->refer[i] = p_Dec->dpb_info[i].slot_index;
			mpp_buf_slot_set_flag(p_Dec->frame_slots, p_Dec->dpb_info[i].slot_index, SLOT_HAL_INPUT);
			mpp_buf_slot_set_flag(p_Dec->frame_slots, p_Dec->dpb_info[i].slot_index, SLOT_CODEC_USE);
		}
	}

__RETURN:
    return MPP_OK;
}


/*!
***********************************************************************
* \brief
*    prepare dpb info for regsiter syntax
***********************************************************************
*/
//extern "C"
MPP_RET prepare_init_ref_info(H264_SLICE_t *currSlice)
{
    void *picbuf = NULL;
    RK_U32 i = 0, j = 0, k = 0;
    RK_S32  poc = 0, TOP_POC = 0, BOT_POC = 0;
    RK_S32  layer_id = 0, voidx = 0, is_used = 0;
    H264_DecCtx_t *p_Dec = currSlice->p_Dec;

    memset(p_Dec->refpic_info_p,    0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));
    memset(p_Dec->refpic_info_b[0], 0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));
    memset(p_Dec->refpic_info_b[1], 0, MAX_REF_SIZE * sizeof(H264_RefPicInfo_t));

    if (currSlice->idr_flag && (currSlice->layer_id == 0)) { // idr_flag==1 && layer_id==0
        goto __RETURN;
    }
    //!<------ set listP -------
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
                picbuf  = p_Dec->dpb_info[i].picbuf;
                TOP_POC = p_Dec->dpb_info[i].TOP_POC;
                BOT_POC = p_Dec->dpb_info[i].BOT_POC;
                voidx   = p_Dec->dpb_info[i].voidx;
                is_used = p_Dec->dpb_info[i].is_used;
                if (currSlice->structure == FRAME && picbuf) {
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
            ASSERT(i < 16);
            p_Dec->refpic_info_p[j].dpb_idx = i;

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
                for (i = 0; i < 16; i++) {
                    picbuf  = p_Dec->dpb_info[i].picbuf;
                    TOP_POC = p_Dec->dpb_info[i].TOP_POC;
                    BOT_POC = p_Dec->dpb_info[i].BOT_POC;
                    voidx   = p_Dec->dpb_info[i].voidx;
                    is_used = p_Dec->dpb_info[i].is_used;
                    if (currSlice->structure == FRAME && picbuf) {
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
                ASSERT(i < 16);
                p_Dec->refpic_info_b[k][j].dpb_idx = i;

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
__RETURN:
    return MPP_OK;
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
    p_Dec->p_Vid->exit_picture_flag    = 1;
    p_Dec->p_Vid->have_outpicture_flag = 1;
    FUN_CHECK(ret = exit_picture(p_Dec->p_Vid, &p_Dec->p_Vid->dec_pic));
    p_Dec->p_Vid->iNumOfSlicesDecoded = 0;
    p_Dec->p_Vid->exit_picture_flag   = 0;

    return ret = MPP_OK;
__FAILED:
    ASSERT(0);
    return ret;
}

