/*
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

#include "mpp_common.h"

#include "h264_syntax.h"
#include "hal_h264e_rkv_nal.h"
#include "hal_h264e_rkv_dpb.h"

static H264eRkvFrame *h264e_rkv_frame_new(H264eRkvDpbCtx *dpb_ctx)
{
    RK_S32 k = 0;
    H264eRkvFrame *frame_buf = dpb_ctx->frame_buf;
    RK_S32 num_buf = MPP_ARRAY_ELEMS(dpb_ctx->frame_buf);
    H264eRkvFrame *new_frame = NULL;
    h264e_hal_enter();
    for (k = 0; k < num_buf; k++) {
        if (!frame_buf[k].hw_buf_used) {
            new_frame = &frame_buf[k];
            frame_buf[k].hw_buf_used = 1;
            break;
        }
    }

    if (!new_frame) {
        h264e_hal_err("!new_frame, new_frame get failed");
        return NULL;
    }

    h264e_hal_leave();
    return new_frame;
}

static
void h264e_rkv_frame_push(H264eRkvFrame **list, H264eRkvFrame *frame)
{
    RK_S32 i = 0;
    while ( list[i] ) i++;
    list[i] = frame;
    h264e_hal_dbg(H264E_DBG_DPB, "frame push list[%d] %p", i, frame);

}

static void h264e_rkv_frame_push_unused(H264eRkvDpbCtx *dpb_ctx,
                                        H264eRkvFrame *frame)
{
    h264e_hal_enter();
    mpp_assert( frame->i_reference_count > 0 );
    frame->i_reference_count--;
    if ( frame->i_reference_count == 0 )
        h264e_rkv_frame_push( dpb_ctx->frames.unused, frame );
    h264e_hal_leave();
}

static H264eRkvFrame *h264e_rkv_frame_pop( H264eRkvFrame **list )
{
    H264eRkvFrame *frame;
    RK_S32 i = 0;
    mpp_assert( list[0] );
    while ( list[i + 1] ) i++;
    frame = list[i];
    list[i] = NULL;
    h264e_hal_dbg(H264E_DBG_DPB, "frame pop list[%d] %p", i, frame);
    return frame;
}

static
H264eRkvFrame *h264e_rkv_frame_pop_unused( H264eHalContext *ctx)
{
    H264eRkvFrame *frame = NULL;

    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;

    h264e_hal_enter();
    if ( dpb_ctx->frames.unused[0] )
        frame = h264e_rkv_frame_pop( dpb_ctx->frames.unused );
    else {
        frame = h264e_rkv_frame_new( dpb_ctx );
    }

    if ( !frame ) {
        h264e_hal_err("!frame, return NULL");
        return NULL;
    }
    frame->i_reference_count = 1;
    frame->b_keyframe = 0;
    frame->b_corrupt = 0;
    frame->long_term_flag = 0;
    frame->reorder_longterm_flag = 0;

    h264e_hal_leave();

    return frame;
}

static H264eRkvFrame *h264e_rkv_frame_shift( H264eRkvFrame **list )
{
    H264eRkvFrame *frame = list[0];
    RK_S32 i;
    for ( i = 0; list[i]; i++ )
        list[i] = list[i + 1];
    mpp_assert(frame);
    h264e_hal_dbg(H264E_DBG_DPB, "frame shift list[0] %p", frame);
    return frame;
}

static MPP_RET h264e_rkv_reference_update( H264eHalContext *ctx)
{
    RK_S32 i = 0, j = 0;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRefParam *ref_cfg = &ctx->param.ref;

    h264e_hal_enter();
    if ( !dpb_ctx->fdec->b_kept_as_ref ) {
        h264e_hal_err("!dpb_ctx->fdec->b_kept_as_ref, return early");
        return MPP_OK;
    }

    /* apply mmco from previous frame. */
    for (i = 0; i < dpb_ctx->i_mmco_command_count; i++) {
        RK_S32 mmco = dpb_ctx->mmco[i].memory_management_control_operation;
        for (j = 0; dpb_ctx->frames.reference[j]; j++) {
            if (dpb_ctx->frames.reference[j]->i_poc == dpb_ctx->mmco[i].i_poc &&
                (mmco == 1 || mmco == 2))
                h264e_rkv_frame_push_unused(dpb_ctx, h264e_rkv_frame_shift(&dpb_ctx->frames.reference[j]));
        }
    }

    /* move frame in the buffer */
    h264e_hal_dbg(H264E_DBG_DPB, "try to push dpb_ctx->fdec %p, poc %d",
                  dpb_ctx->fdec, dpb_ctx->fdec->i_poc);
    h264e_rkv_frame_push( dpb_ctx->frames.reference, dpb_ctx->fdec );
    if ( ref_cfg->i_long_term_en ) {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] ) {
            for (i = 0; i < 17; i++) {
                if (dpb_ctx->frames.reference[i]->long_term_flag == 0) {
                    //if longterm , don't remove;
                    h264e_rkv_frame_push_unused(dpb_ctx, h264e_rkv_frame_shift(&dpb_ctx->frames.reference[i]));
                    break;
                }
                mpp_assert(i != 16);
            }
        }
    } else {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] )
            h264e_rkv_frame_push_unused(dpb_ctx, h264e_rkv_frame_shift(dpb_ctx->frames.reference));
    }

    h264e_hal_leave();
    return MPP_OK;
}

static void h264e_rkv_reference_reset(H264eRkvDpbCtx *dpb_ctx)
{
    h264e_hal_enter();
    while (dpb_ctx->frames.reference[0])
        h264e_rkv_frame_push_unused(dpb_ctx, h264e_rkv_frame_pop(dpb_ctx->frames.reference));
    dpb_ctx->fdec->i_poc = 0;
    h264e_hal_leave();
}

static RK_S32
h264e_rkv_reference_distance(H264eRkvDpbCtx *dpb_ctx, H264eRkvFrame *frame )
{
    return abs(dpb_ctx->fdec->i_frame_cnt - frame->i_frame_cnt);
}

static void h264e_rkv_reference_build_list(H264eHalContext *ctx)
{
    RK_S32 b_ok = 1, i = 0, list = 0, j = 0;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eHalParam *par = &ctx->param;
    H264eRefParam *ref_cfg = &par->ref;
    H264eRkvFrame *fdec = dpb_ctx->fdec;
    RK_S32 i_poc = fdec->i_poc;

    h264e_hal_enter();
    /* build ref list 0/1 */
    dpb_ctx->i_ref[0] = 0;
    dpb_ctx->i_ref[1] = 0;

    if ( dpb_ctx->i_slice_type == H264E_HAL_SLICE_TYPE_I ) {
        if ( ref_cfg->i_long_term_en && ref_cfg->i_frame_reference > 1 )
            ref_cfg->hw_longterm_mode ^= 1;  //0 and 1, circle; If ref==1 , longterm mode only 1;

        if (ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode &&
            ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0))
            fdec->long_term_flag = 1;
        h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_slice_type == SLICE_TYPE_I, return");
        return;
    }

    h264e_hal_dbg(H264E_DBG_DPB, "fdec->i_poc: %d", fdec->i_poc);
    for ( i = 0; dpb_ctx->frames.reference[i]; i++ ) {
        if ( dpb_ctx->frames.reference[i]->b_corrupt )
            continue;
        if ( dpb_ctx->frames.reference[i]->i_poc < i_poc )
            dpb_ctx->fref[0][dpb_ctx->i_ref[0]++] = dpb_ctx->frames.reference[i];
        else if ( dpb_ctx->frames.reference[i]->i_poc > i_poc )
            dpb_ctx->fref[1][dpb_ctx->i_ref[1]++] = dpb_ctx->frames.reference[i];
    }

    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_ref[0]: %d", dpb_ctx->i_ref[0]);
    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->i_ref[1]: %d", dpb_ctx->i_ref[1]);

    if ( dpb_ctx->i_mmco_remove_from_end ) {
        /* Order ref0 for MMCO remove */
        do {
            b_ok = 1;
            for (i = 0; i < dpb_ctx->i_ref[0] - 1; i++ ) {
                if ( dpb_ctx->fref[0][i]->i_frame_cnt < dpb_ctx->fref[0][i + 1]->i_frame_cnt ) {
                    MPP_SWAP( H264eRkvFrame *, dpb_ctx->fref[0][i], dpb_ctx->fref[0][i + 1] );
                    b_ok = 0;
                    break;
                }
            }
        } while ( !b_ok );

        for ( i = dpb_ctx->i_ref[0] - 1; i >= dpb_ctx->i_ref[0] - dpb_ctx->i_mmco_remove_from_end; i-- ) {
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][i]->i_frame_num;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;
        }
    }

    /* Order reference lists by distance from the current frame. */
    for ( list = 0; list < 1; list++ ) {    //only one list
        dpb_ctx->fref_nearest[list] = dpb_ctx->fref[list][0];
        do {
            b_ok = 1;
            for ( i = 0; i < dpb_ctx->i_ref[list] - 1; i++ ) {
                if ( list ? dpb_ctx->fref[list][i + 1]->i_poc < dpb_ctx->fref_nearest[list]->i_poc
                     : dpb_ctx->fref[list][i + 1]->i_poc > dpb_ctx->fref_nearest[list]->i_poc )
                    dpb_ctx->fref_nearest[list] = dpb_ctx->fref[list][i + 1];
                if ( h264e_rkv_reference_distance(dpb_ctx, dpb_ctx->fref[list][i]   ) >
                     h264e_rkv_reference_distance(dpb_ctx, dpb_ctx->fref[list][i + 1] ) ) {
                    MPP_SWAP( H264eRkvFrame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i + 1] );
                    b_ok = 0;
                    break;
                }
            }
        } while ( !b_ok );
    }
    //Order long_term frame to last lists  , only one long_term frame
    if (ref_cfg->i_long_term_en) {
        for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
            if (dpb_ctx->fref[0][i]->long_term_flag == 1) {
                for (j = i; j < dpb_ctx->i_ref[0] - 1; j++) {
                    MPP_SWAP(H264eRkvFrame *, dpb_ctx->fref[0][j], dpb_ctx->fref[0][j + 1]);
                }
                break;
            }
        }
    }

    //reorder, when in longterm_mode "1", don't reorder;
    if (ref_cfg->i_frame_reference > 1 && ref_cfg->i_ref_pos && ref_cfg->i_ref_pos < dpb_ctx->i_ref[0])
        dpb_ctx->b_ref_pic_list_reordering[0] = 1;
    else
        dpb_ctx->b_ref_pic_list_reordering[0] = 0;
    if (dpb_ctx->b_ref_pic_list_reordering[0]) {
        for (list = 0; list < 1; list++) {
            if (dpb_ctx->fref[0][ref_cfg->i_ref_pos]->long_term_flag) {
                fdec->reorder_longterm_flag = 1;
            }
            for (i = ref_cfg->i_ref_pos; i >= 1; i--) {
                MPP_SWAP(H264eRkvFrame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i - 1]);
            }
        }
    }

    //first P frame mark max_long_term_frame_idx_plus1
    if ( ref_cfg->i_long_term_en && dpb_ctx->fdec->i_frame_num == 1 ) {
        dpb_ctx->i_mmco_command_count = 0;

        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = 0;
        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 4;
        dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 2;  //  max_long_term_frame_idx_plus1 , slice will plus 1, is 0;
    } else
        dpb_ctx->i_mmco_command_count = 0;

    //longterm marking
    if ( ref_cfg->i_long_term_en && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0)) {
        RK_S32 reflist_longterm = 0;
        RK_S32 reflist_longidx  = 0;
        RK_S32 reflist_short_to_long  = 0;
        RK_S32 reflist_short_to_long_idx  = 0;

        //search frame for transferring short to long;
        if (ref_cfg->hw_longterm_mode == 0 || dpb_ctx->fdec->i_frame_num == 1) {
            for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
                if (!dpb_ctx->fref[0][i]->long_term_flag && (ref_cfg->i_ref_pos + 1) == i) {
                    reflist_short_to_long++;
                    reflist_short_to_long_idx = i;
                    break;
                }
            }
            //clear ref longterm flag
            for (i = 0; i < dpb_ctx->i_ref[0]; i++) {
                if (dpb_ctx->fref[0][i]->long_term_flag) {
                    reflist_longterm++;
                    reflist_longidx = i;
                    dpb_ctx->fref[0][i]->long_term_flag = 0;
                }
            }
        }

        mpp_assert(reflist_longterm <= 1);

        //marking ref longterm to unused;
        if ( reflist_longterm == 1 ) {
            i = reflist_longidx;

            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 2;     //long_term_pic_num
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 1;
        } else if ( dpb_ctx->i_ref[0] >= sps->i_num_ref_frames && dpb_ctx->i_ref[0] > 1 ) { //if dpb is full, so it need release a short term ref frame;
            //if longterm marking is same with release short term, change release short term;
            RK_S32 pos = ((reflist_short_to_long && reflist_short_to_long_idx == (dpb_ctx->i_ref[0] - 1)) || dpb_ctx->fref[0][dpb_ctx->i_ref[0] - 1]->long_term_flag) + 1;
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][dpb_ctx->i_ref[0] - pos]->i_frame_num;

            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][dpb_ctx->i_ref[0] - pos]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;  // difference_of_pic_nums_minus1
        }

        //marking curr pic to longterm;
        if ( ref_cfg->hw_longterm_mode && dpb_ctx->fdec->i_frame_num == 1) {
            fdec->long_term_flag = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = 0;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 6;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = 1;  //  long_term_frame_idx ;
        } else if ( reflist_short_to_long ) { //Assign a long-term frame index to a short-term picture
            i = reflist_short_to_long_idx;
            RK_S32 diff = dpb_ctx->fdec->i_frame_num - dpb_ctx->fref[0][i]->i_frame_num;

            dpb_ctx->fref[0][i]->long_term_flag = 1;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_poc = dpb_ctx->fref[0][i]->i_poc;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].memory_management_control_operation = 3; //short term to long term;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_difference_of_pic_nums = diff;  //  difference_of_pic_nums_minus1 , slice will minus 1, is 0;
        }
    } else {
        if (!(ref_cfg->i_long_term_en && dpb_ctx->fdec->i_frame_num == 1))
            dpb_ctx->i_mmco_command_count = 0;
    }

    dpb_ctx->i_ref[1] = H264E_HAL_MIN( dpb_ctx->i_ref[1], dpb_ctx->i_max_ref1 );
    dpb_ctx->i_ref[0] = H264E_HAL_MIN( dpb_ctx->i_ref[0], dpb_ctx->i_max_ref0 );
    dpb_ctx->i_ref[0] = H264E_HAL_MIN( dpb_ctx->i_ref[0], ref_cfg->i_frame_reference ); // if reconfig() has lowered the limit

    /* EXP: add duplicates */
    mpp_assert( dpb_ctx->i_ref[0] + dpb_ctx->i_ref[1] <= H264E_REF_MAX );

    h264e_hal_leave();
}

MPP_RET h264e_rkv_reference_frame_update( H264eHalContext *ctx)
{
    h264e_hal_enter();
    if (MPP_OK != h264e_rkv_reference_update(ctx)) {
        h264e_hal_err("reference update failed, return now");
        return MPP_NOK;
    }

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET
h264e_rkv_reference_frame_set( H264eHalContext *ctx, H264eHwCfg *syn)
{
    RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;
    RK_S32 list = 0, k = 0;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264eRefParam *ref_cfg = &ctx->param.ref;

    h264e_hal_enter();

    dpb_ctx->fdec = h264e_rkv_frame_pop_unused(ctx);
    if ( !dpb_ctx->fdec ) {
        h264e_hal_err("!dpb_ctx->fdec, current recon buf get failed");
        return MPP_NOK;
    }

    dpb_ctx->i_max_ref0 = ref_cfg->i_frame_reference;
    dpb_ctx->i_max_ref1 = H264E_HAL_MIN(sps->vui.i_num_reorder_frames,
                                        ref_cfg->i_frame_reference);

    if (syn->coding_type == RKVENC_CODING_TYPE_IDR) {
        dpb_ctx->i_frame_num = 0;
        dpb_ctx->frames.i_last_idr = dpb_ctx->i_frame_cnt;
    }

    dpb_ctx->fdec->i_frame_cnt = dpb_ctx->i_frame_cnt;
    dpb_ctx->fdec->i_frame_num = dpb_ctx->i_frame_num;
    dpb_ctx->fdec->i_frame_type = syn->coding_type;
    dpb_ctx->fdec->i_poc = 2 * (dpb_ctx->fdec->i_frame_cnt -
                                H264E_HAL_MAX(dpb_ctx->frames.i_last_idr, 0));

    if ( !RKVENC_IS_TYPE_I( dpb_ctx->fdec->i_frame_type ) ) {
        RK_S32 valid_refs_left = 0;
        for ( k = 0; dpb_ctx->frames.reference[k]; k++ )
            if ( !dpb_ctx->frames.reference[k]->b_corrupt )
                valid_refs_left++;
        /* No valid reference frames left: force an IDR. */
        if ( !valid_refs_left ) {
            dpb_ctx->fdec->b_keyframe = 1;
            dpb_ctx->fdec->i_frame_type = RKVENC_CODING_TYPE_IDR;
        }
    }
    if ( dpb_ctx->fdec->b_keyframe )
        dpb_ctx->frames.i_last_keyframe = dpb_ctx->fdec->i_frame_cnt;


    dpb_ctx->i_mmco_command_count =
        dpb_ctx->i_mmco_remove_from_end = 0;
    dpb_ctx->b_ref_pic_list_reordering[0] = 0;
    dpb_ctx->b_ref_pic_list_reordering[1] = 0;

    /* calculate nal type and nal ref idc */
    if (syn->coding_type == RKVENC_CODING_TYPE_IDR) {
        //TODO: extend syn->frame_coding_type definition
        /* reset ref pictures */
        i_nal_type    = H264E_NAL_SLICE_IDR;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGHEST;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
        h264e_rkv_reference_reset(dpb_ctx);
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_I ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_P ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_P;
    } else if ( syn->coding_type == RKVENC_CODING_TYPE_BREF ) {
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    } else { /* B frame */
        i_nal_type    = H264E_NAL_SLICE;
        i_nal_ref_idc = H264E_NAL_PRIORITY_DISPOSABLE;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    }

    dpb_ctx->fdec->b_kept_as_ref = i_nal_ref_idc != H264E_NAL_PRIORITY_DISPOSABLE;// && h->param.i_keyint_max > 1;

    if (sps->keyframe_max_interval == 1)
        i_nal_ref_idc = H264E_NAL_PRIORITY_LOW;

    dpb_ctx->i_nal_ref_idc = i_nal_ref_idc;
    dpb_ctx->i_nal_type = i_nal_type;
    dpb_ctx->fdec->i_pts = dpb_ctx->fdec->i_pts;

    /* build list */
    h264e_rkv_reference_build_list(ctx);

    /* set syntax (slice header) */

    /* If the ref list isn't in the default order, construct reordering header */
    for (list = 0; list < 2; list++ ) {
        if ( dpb_ctx->b_ref_pic_list_reordering[list] ) {
            RK_S32 pred_frame_num = dpb_ctx->fdec->i_frame_num & ((1 << sps->i_log2_max_frame_num) - 1);
            for ( k = 0; k < dpb_ctx->i_ref[list]; k++ ) {
                if ( dpb_ctx->fdec->reorder_longterm_flag ) { //
                    dpb_ctx->fdec->reorder_longterm_flag = 0;     //clear reorder_longterm_flag
                    dpb_ctx->ref_pic_list_order[list][k].idc = 2; //reorder long_term_pic_num;
                    dpb_ctx->ref_pic_list_order[list][k].arg = 0; //long_term_pic_num
                    break;      //NOTE: RK feature: only reorder one time
                } else {
                    RK_S32 lx_frame_num = dpb_ctx->fref[list][k]->i_frame_num  & ((1 << sps->i_log2_max_frame_num) - 1);
                    RK_S32 diff = lx_frame_num - pred_frame_num;
                    dpb_ctx->ref_pic_list_order[list][k].idc = ( diff > 0 );
                    dpb_ctx->ref_pic_list_order[list][k].arg = (abs(diff) - 1) & ((1 << sps->i_log2_max_frame_num) - 1);
                    pred_frame_num = dpb_ctx->fref[list][k]->i_frame_num;
                    break;      //NOTE: RK feature: only reorder one time
                }
            }
        }
    }

    if (dpb_ctx->i_nal_type == H264E_NAL_SLICE_IDR) {
        if (ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0) )
            dpb_ctx->i_long_term_reference_flag = 1;
        dpb_ctx->i_idr_pic_id = dpb_ctx->i_tmp_idr_pic_id;
        dpb_ctx->i_tmp_idr_pic_id ^= 1;
    } else {
        dpb_ctx->i_long_term_reference_flag = 0;
        dpb_ctx->i_idr_pic_id = -1;
    }

    h264e_hal_leave();

    return MPP_OK;
}


