#define MODULE_TAG "hal_h264e_rk"
#include <string.h>
#include <math.h>
#include "vpu.h"
#include "mpp_common.h"
#include "mpp_mem.h"
#include "hal_h264e.h"
#include "hal_h264e_rkv.h"

#define RKVENC_FRAME_TYPE_AUTO          0x0000  /* Let x264 choose the right type */
#define RKVENC_FRAME_TYPE_IDR           0x0001
#define RKVENC_FRAME_TYPE_I             0x0002
#define RKVENC_FRAME_TYPE_P             0x0003
#define RKVENC_FRAME_TYPE_BREF          0x0004  /* Non-disposable B-frame */
#define RKVENC_FRAME_TYPE_B             0x0005
#define RKVENC_FRAME_TYPE_KEYFRAME      0x0006  /* IDR or I depending on b_open_gop option */
#define RKVENC_IS_TYPE_I(x) ((x)==RKVENC_FRAME_TYPE_I || (x)==RKVENC_FRAME_TYPE_IDR)
#define RKVENC_IS_TYPE_B(x) ((x)==RKVENC_FRAME_TYPE_B || (x)==RKVENC_FRAME_TYPE_BREF)
#define RKVENC_IS_DISPOSABLE(type) ( type == RKVENC_FRAME_TYPE_B )

const RK_S32 h264e_csp_idx_map[RKV_H264E_CSP_BUTT] = {RKV_H264E_CSP2_BGRA, RKV_H264E_CSP2_BGR, RKV_H264E_CSP2_RGB, 0, RKV_H264E_CSP2_NV16, RKV_H264E_CSP2_I422, RKV_H264E_CSP2_NV12,
                                                      RKV_H264E_CSP2_I420, RKV_H264E_CSP2_RGB, RKV_H264E_CSP2_RGB
                                                     };

static const RK_U32 h264e_h3d_tbl[40] = {
    0x0b080400, 0x1815120f, 0x23201e1b, 0x2c2a2725,
    0x33312f2d, 0x38373634, 0x3d3c3b39, 0x403f3e3d,
    0x42414140, 0x43434342, 0x44444444, 0x44444444,
    0x44444444, 0x43434344, 0x42424343, 0x40414142,
    0x3d3e3f40, 0x393a3b3c, 0x35363738, 0x30313334,
    0x2c2d2e2f, 0x28292a2b, 0x23242526, 0x20202122,
    0x191b1d1f, 0x14151618, 0x0f101112, 0x0b0c0d0e,
    0x08090a0a, 0x06070708, 0x05050506, 0x03040404,
    0x02020303, 0x01010102, 0x00010101, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000
};

static const RK_U8 h264e_ue_size_tab[256] = {
    1, 1, 3, 3, 5, 5, 5, 5, 7, 7, 7, 7, 7, 7, 7, 7,
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
};

/* default quant matrices */
static const RK_U8 h264e_rkv_cqm_jvt4i[16] = {
    6, 13, 20, 28,
    13, 20, 28, 32,
    20, 28, 32, 37,
    28, 32, 37, 42
};
static const RK_U8 h264e_rkv_cqm_jvt4p[16] = {
    10, 14, 20, 24,
    14, 20, 24, 27,
    20, 24, 27, 30,
    24, 27, 30, 34
};
static const RK_U8 h264e_rkv_cqm_jvt8i[64] = {
    6, 10, 13, 16, 18, 23, 25, 27,
    10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31,
    16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36,
    23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40,
    27, 29, 31, 33, 36, 38, 40, 42
};
static const RK_U8 h264e_rkv_cqm_jvt8p[64] = {
    9, 13, 15, 17, 19, 21, 22, 24,
    13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27,
    17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30,
    21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33,
    24, 25, 27, 28, 30, 32, 33, 35
};

static const RK_U8 h264e_rkv_cqm_flat16[64] = {
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16
};
static const RK_U8 * const h264e_rkv_cqm_jvt[8] = {
    h264e_rkv_cqm_jvt4i, h264e_rkv_cqm_jvt4p,
    h264e_rkv_cqm_jvt4i, h264e_rkv_cqm_jvt4p,
    h264e_rkv_cqm_jvt8i, h264e_rkv_cqm_jvt8p,
    h264e_rkv_cqm_jvt8i, h264e_rkv_cqm_jvt8p
};

/* zigzags are transposed with respect to the tables in the standard */
static const RK_U8 h264e_rkv_zigzag_scan4[2][16] = {
    {
        // frame
        0,  4,  1,  2,  5,  8, 12,  9,  6,  3,  7, 10, 13, 14, 11, 15
    },
    {
        // field
        0,  1,  4,  2,  3,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15
    }
};
static const RK_U8 h264e_rkv_zigzag_scan8[2][64] = {
    {
        0,  8,  1,  2,  9, 16, 24, 17, 10,  3,  4, 11, 18, 25, 32, 40,
        33, 26, 19, 12,  5,  6, 13, 20, 27, 34, 41, 48, 56, 49, 42, 35,
        28, 21, 14,  7, 15, 22, 29, 36, 43, 50, 57, 58, 51, 44, 37, 30,
        23, 31, 38, 45, 52, 59, 60, 53, 46, 39, 47, 54, 61, 62, 55, 63
    },
    {
        0,  1,  2,  8,  9,  3,  4, 10, 16, 11,  5,  6,  7, 12, 17, 24,
        18, 13, 14, 15, 19, 25, 32, 26, 20, 21, 22, 23, 27, 33, 40, 34,
        28, 29, 30, 31, 35, 41, 48, 42, 36, 37, 38, 39, 43, 49, 50, 44,
        45, 46, 47, 51, 56, 57, 52, 53, 54, 55, 58, 59, 60, 61, 62, 63
    }
};

#if (RKV_H264E_NUM_REFS > 1)
void hal_h264e_rkv_frame_push( h264e_hal_rkv_frame **list, h264e_hal_rkv_frame *frame )
{
    RK_S32 i = 0;
    while ( list[i] ) i++;
    list[i] = frame;
    h264e_hal_log_detail("frame push list[%d] %p", i, frame);

}


static h264e_hal_rkv_frame *hal_h264e_rkv_frame_new(h264e_hal_rkv_dpb_ctx *dpb_ctx)
{
    RK_S32 k = 0;
    h264e_hal_rkv_frame *frame_buf = dpb_ctx->frame_buf;
    RK_S32 num_buf = MPP_ARRAY_ELEMS(dpb_ctx->frame_buf);
    h264e_hal_rkv_frame *new_frame = NULL;
    h264e_hal_debug_enter();
    for (k = 0; k < num_buf; k++) {
        if (!frame_buf[k].hw_buf_used) {
            new_frame = &frame_buf[k];
            frame_buf[k].hw_buf_used = 1;
            break;
        }
    }

    if (!new_frame) {
        mpp_err("!new_frame, new_frame get failed");
        return NULL;
    }

    h264e_hal_debug_leave();
    return new_frame;
}

h264e_hal_rkv_frame *hal_h264e_rkv_frame_pop( h264e_hal_rkv_frame **list )
{
    h264e_hal_rkv_frame *frame;
    RK_S32 i = 0;
    mpp_assert( list[0] );
    while ( list[i + 1] ) i++;
    frame = list[i];
    list[i] = NULL;
    h264e_hal_log_detail("frame pop list[%d] %p", i, frame);
    return frame;
}

void hal_h264e_rkv_frame_unshift( h264e_hal_rkv_frame **list, h264e_hal_rkv_frame *frame )
{
    RK_S32 i = 0;
    while ( list[i] ) i++;
    while ( i-- )
        list[i + 1] = list[i];
    list[0] = frame;
    h264e_hal_log_detail("frame unshift list[0] %p", frame);
}

h264e_hal_rkv_frame *hal_h264e_rkv_frame_shift( h264e_hal_rkv_frame **list )
{
    h264e_hal_rkv_frame *frame = list[0];
    RK_S32 i;
    for ( i = 0; list[i]; i++ )
        list[i] = list[i + 1];
    mpp_assert(frame);
    h264e_hal_log_detail("frame shift list[0] %p", frame);
    return frame;
}


void hal_h264e_rkv_frame_push_unused( h264e_hal_rkv_dpb_ctx *dpb_ctx, h264e_hal_rkv_frame *frame )
{
    h264e_hal_debug_enter();
    mpp_assert( frame->i_reference_count > 0 );
    frame->i_reference_count--;
    if ( frame->i_reference_count == 0 )
        hal_h264e_rkv_frame_push( dpb_ctx->frames.unused, frame );
    h264e_hal_debug_leave();
}

h264e_hal_rkv_frame *hal_h264e_rkv_frame_pop_unused( h264e_hal_context *ctx)
{
    h264e_hal_rkv_frame *frame = NULL;

    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;

    h264e_hal_debug_enter();
    if ( dpb_ctx->frames.unused[0] )
        frame = hal_h264e_rkv_frame_pop( dpb_ctx->frames.unused );
    else {
        frame = hal_h264e_rkv_frame_new( dpb_ctx );
    }

    if ( !frame ) {
        mpp_log("!frame, return NULL");
        return NULL;
    }
    frame->i_reference_count = 1;
    frame->b_keyframe = 0;
    frame->b_corrupt = 0;
    frame->long_term_flag = 0;
    frame->reorder_longterm_flag = 0;

    h264e_hal_debug_leave();

    return frame;
}

static void hal_h264e_rkv_reference_reset( h264e_hal_rkv_dpb_ctx *dpb_ctx )
{
    h264e_hal_debug_enter();
    while ( dpb_ctx->frames.reference[0] )
        hal_h264e_rkv_frame_push_unused( dpb_ctx, hal_h264e_rkv_frame_pop( dpb_ctx->frames.reference ) );
    dpb_ctx->fdec->i_poc = 0;
    h264e_hal_debug_leave();
}

#if 0
static void hal_h264e_rkv_reference_hierarchy_reset( h264e_hal_context *ctx)
{
    RK_S32 ref;
    RK_S32 i = 0;
    RK_S32 b_hasdelayframe = 0;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_rkv_out *extra_info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps = &extra_info->sps;
    RK_S32 i_num_reorder_frames = sps->vui.i_num_reorder_frames;

    /* look for delay frames -- chain must only contain frames that are disposable */
    for ( i = 0; dpb_ctx->frames.current[i] && RKVENC_IS_DISPOSABLE( dpb_ctx->frames.current[i]->i_type ); i++ )
        b_hasdelayframe |= dpb_ctx->frames.current[i]->i_coded
                           != dpb_ctx->frames.current[i]->i_frame + i_num_reorder_frames;

    /* This function must handle b-pyramid and clear frames for open-gop */
    if (!b_hasdelayframe && dpb_ctx->frames.i_poc_last_open_gop == -1 )
        return;

    /* Remove last BREF. There will never be old BREFs in the
     * dpb during a BREF decode when pyramid == STRICT */
    for ( ref = 0; dpb_ctx->frames.reference[ref]; ref++ ) {
        if (dpb_ctx->frames.reference[ref]->i_poc < dpb_ctx->frames.i_poc_last_open_gop &&
            dpb_ctx->i_type != H264E_HAL_SLICE_TYPE_B ) {
            RK_S32 diff = dpb_ctx->i_frame_num - dpb_ctx->frames.reference[ref]->i_frame_num;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count].i_difference_of_pic_nums = diff;
            dpb_ctx->mmco[dpb_ctx->i_mmco_command_count++].i_poc = dpb_ctx->frames.reference[ref]->i_poc;
            hal_h264e_rkv_frame_push_unused( dpb_ctx, hal_h264e_rkv_frame_shift( &dpb_ctx->frames.reference[ref] ) );
            dpb_ctx->b_ref_reorder[0] = 1;
            ref--;
        }
    }
}
#endif

static RK_S32 hal_h264e_rkv_reference_distance( h264e_hal_ref_param *ref_cfg, h264e_hal_rkv_dpb_ctx *dpb_ctx, h264e_hal_rkv_frame *frame )
{
    if ( ref_cfg->i_frame_packing == 5 )
        return abs((dpb_ctx->fdec->i_frame_cnt & ~1) - (frame->i_frame_cnt & ~1)) +
               ((dpb_ctx->fdec->i_frame_cnt & 1) != (frame->i_frame_cnt & 1));
    else
        return abs(dpb_ctx->fdec->i_frame_cnt - frame->i_frame_cnt);
}


static void hal_h264e_rkv_reference_build_list(h264e_hal_context *ctx)
{
    RK_S32 b_ok = 1, i = 0, list = 0, j = 0;
    h264e_hal_rkv_out *extra_info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps = &extra_info->sps;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_param *par = &ctx->param;
    h264e_hal_ref_param *ref_cfg = &par->ref;
    h264e_hal_rkv_frame *fdec = dpb_ctx->fdec;
    RK_S32 i_poc = fdec->i_poc;

    h264e_hal_debug_enter();
    /* build ref list 0/1 */
    dpb_ctx->i_ref[0] = 0;
    dpb_ctx->i_ref[1] = 0;

    if ( dpb_ctx->i_slice_type == H264E_HAL_SLICE_TYPE_I ) {
        if ( ref_cfg->i_long_term_en && ref_cfg->i_frame_reference > 1 )
            ref_cfg->hw_longterm_mode ^= 1;       //0 and 1, circle; If ref==1 , longterm mode only 1;

        if ( ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0))
            fdec->long_term_flag = 1;
        mpp_log("dpb_ctx->i_slice_type == SLICE_TYPE_I, return");
        return;
    }

    h264e_hal_log_detail("fdec->i_poc: %d", fdec->i_poc);
    for ( i = 0; dpb_ctx->frames.reference[i]; i++ ) {
        if ( dpb_ctx->frames.reference[i]->b_corrupt )
            continue;
        if ( dpb_ctx->frames.reference[i]->i_poc < i_poc )
            dpb_ctx->fref[0][dpb_ctx->i_ref[0]++] = dpb_ctx->frames.reference[i];
        else if ( dpb_ctx->frames.reference[i]->i_poc > i_poc )
            dpb_ctx->fref[1][dpb_ctx->i_ref[1]++] = dpb_ctx->frames.reference[i];
    }

    h264e_hal_log_detail("dpb_ctx->i_ref[0]: %d", dpb_ctx->i_ref[0]);
    h264e_hal_log_detail("dpb_ctx->i_ref[1]: %d", dpb_ctx->i_ref[1]);

    if ( dpb_ctx->i_mmco_remove_from_end ) {
        /* Order ref0 for MMCO remove */
        do {
            b_ok = 1;
            for (i = 0; i < dpb_ctx->i_ref[0] - 1; i++ ) {
                if ( dpb_ctx->fref[0][i]->i_frame_cnt < dpb_ctx->fref[0][i + 1]->i_frame_cnt ) {
                    MPP_SWAP( h264e_hal_rkv_frame *, dpb_ctx->fref[0][i], dpb_ctx->fref[0][i + 1] );
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
                if ( hal_h264e_rkv_reference_distance( ref_cfg, dpb_ctx, dpb_ctx->fref[list][i]   ) >
                     hal_h264e_rkv_reference_distance( ref_cfg, dpb_ctx, dpb_ctx->fref[list][i + 1] ) ) {
                    MPP_SWAP( h264e_hal_rkv_frame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i + 1] );
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
                    MPP_SWAP(h264e_hal_rkv_frame *, dpb_ctx->fref[0][j], dpb_ctx->fref[0][j + 1]);
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
                MPP_SWAP(h264e_hal_rkv_frame *, dpb_ctx->fref[list][i], dpb_ctx->fref[list][i - 1]);
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


#if 1
    for (i = 0; dpb_ctx->frames.reference[i]; i++) {
        h264e_hal_rkv_frame *frame = dpb_ctx->frames.reference[i];
        h264e_hal_log_detail("dpb frame_cnt %d: reference[%d] %p, poc %d, frame_num %d, frame_cnt %d",
                             ctx->frame_cnt, i, frame, frame->i_poc, frame->i_frame_num, frame->i_frame_cnt);
    }
    h264e_hal_log_detail("dpb frame_cnt %d: recon frame %p, poc %d, frame_num %d, frame_cnt %d", ctx->frame_cnt, dpb_ctx->fdec,
                         dpb_ctx->fdec->i_poc, dpb_ctx->fdec->i_frame_num, dpb_ctx->fdec->i_frame_cnt);
    h264e_hal_log_detail("dpb frame_cnt %d: refer frame %p, poc %d, frame_num %d, frame_cnt %d", ctx->frame_cnt, dpb_ctx->fref[0][0],
                         dpb_ctx->fref[0][0]->i_poc, dpb_ctx->fref[0][0]->i_frame_num, dpb_ctx->fref[0][0]->i_frame_cnt);
#endif

    h264e_hal_debug_leave();
}

static MPP_RET hal_h264e_rkv_reference_update( h264e_hal_context *ctx)
{
    RK_S32 i = 0, j = 0;
    h264e_hal_rkv_out *extra_info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps = &extra_info->sps;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_ref_param *ref_cfg = &ctx->param.ref;

    h264e_hal_debug_enter();
    if ( !dpb_ctx->fdec->b_kept_as_ref ) {
        mpp_log("!dpb_ctx->fdec->b_kept_as_ref, return early");
        return MPP_OK;
    }

    /* apply mmco from previous frame. */
    for ( i = 0; i < dpb_ctx->i_mmco_command_count; i++ ) {
        RK_S32 mmco = dpb_ctx->mmco[i].memory_management_control_operation;
        for ( j = 0; dpb_ctx->frames.reference[j]; j++ ) {
            if ( dpb_ctx->frames.reference[j]->i_poc == dpb_ctx->mmco[i].i_poc && (mmco == 1 || mmco == 2) )
                hal_h264e_rkv_frame_push_unused( dpb_ctx, hal_h264e_rkv_frame_shift( &dpb_ctx->frames.reference[j] ) );
        }
    }

    /* move frame in the buffer */
    h264e_hal_log_detail("try to push dpb_ctx->fdec %p, poc %d", dpb_ctx->fdec, dpb_ctx->fdec->i_poc);
    hal_h264e_rkv_frame_push( dpb_ctx->frames.reference, dpb_ctx->fdec );
    if ( ref_cfg->i_long_term_en ) {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] ) {
            for (i = 0; i < 17; i++) {
                if (dpb_ctx->frames.reference[i]->long_term_flag == 0) { //if longterm , don't remove;
                    hal_h264e_rkv_frame_push_unused(dpb_ctx, hal_h264e_rkv_frame_shift(&dpb_ctx->frames.reference[i]));
                    break;
                }
                mpp_assert(i != 16);
            }
        }
    } else {
        if ( dpb_ctx->frames.reference[sps->i_num_ref_frames] )
            hal_h264e_rkv_frame_push_unused( dpb_ctx, hal_h264e_rkv_frame_shift( dpb_ctx->frames.reference ) );
    }

    h264e_hal_debug_leave();
    return MPP_OK;
}



static MPP_RET hal_h264e_rkv_reference_frame_set( h264e_hal_context *ctx, h264e_syntax *syn)
{
    RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;
    RK_S32 list = 0, k = 0;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_rkv_out *extra_info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps = &extra_info->sps;
    h264e_hal_ref_param *ref_cfg = &ctx->param.ref;

    h264e_hal_debug_enter();

    dpb_ctx->fdec = hal_h264e_rkv_frame_pop_unused(ctx);
    if ( !dpb_ctx->fdec ) {
        mpp_log("!dpb_ctx->fdec, current recon buf get failed");
        return MPP_NOK;
    }

    dpb_ctx->i_max_ref0 = ref_cfg->i_frame_reference;
    dpb_ctx->i_max_ref1 = H264E_HAL_MIN( sps->vui.i_num_reorder_frames, ref_cfg->i_frame_reference );

    if (syn->frame_coding_type == RKVENC_FRAME_TYPE_IDR) {
        dpb_ctx->i_frame_num = 0;
        dpb_ctx->frames.i_last_idr = dpb_ctx->i_frame_cnt;
    }

    dpb_ctx->fdec->i_frame_cnt = dpb_ctx->i_frame_cnt;
    dpb_ctx->fdec->i_frame_num = dpb_ctx->i_frame_num;
    dpb_ctx->fdec->i_frame_type = syn->frame_coding_type;
    dpb_ctx->fdec->i_poc = 2 * ( dpb_ctx->fdec->i_frame_cnt - H264E_HAL_MAX( dpb_ctx->frames.i_last_idr, 0 ) );


    if ( !RKVENC_IS_TYPE_I( dpb_ctx->fdec->i_frame_type ) ) {
        RK_S32 valid_refs_left = 0;
        for ( k = 0; dpb_ctx->frames.reference[k]; k++ )
            if ( !dpb_ctx->frames.reference[k]->b_corrupt )
                valid_refs_left++;
        /* No valid reference frames left: force an IDR. */
        if ( !valid_refs_left ) {
            dpb_ctx->fdec->b_keyframe = 1;
            dpb_ctx->fdec->i_frame_type = RKVENC_FRAME_TYPE_IDR;
        }
    }
    if ( dpb_ctx->fdec->b_keyframe )
        dpb_ctx->frames.i_last_keyframe = dpb_ctx->fdec->i_frame_cnt;


    dpb_ctx->i_mmco_command_count =
        dpb_ctx->i_mmco_remove_from_end = 0;
    dpb_ctx->b_ref_pic_list_reordering[0] = 0;
    dpb_ctx->b_ref_pic_list_reordering[1] = 0;

    /* calculate nal type and nal ref idc */
    if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_IDR ) { //TODO: extend syn->frame_coding_type definition
        /* reset ref pictures */
        i_nal_type    = RKVENC_NAL_SLICE_IDR;
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGHEST;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
        hal_h264e_rkv_reference_reset(dpb_ctx);
    } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_I ) {
        i_nal_type    = RKVENC_NAL_SLICE;
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_I;
    } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_P ) {
        i_nal_type    = RKVENC_NAL_SLICE;
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_P;
    } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_BREF ) {
        i_nal_type    = RKVENC_NAL_SLICE;
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    } else { /* B frame */
        i_nal_type    = RKVENC_NAL_SLICE;
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_DISPOSABLE;
        dpb_ctx->i_slice_type = H264E_HAL_SLICE_TYPE_B;
    }

    dpb_ctx->fdec->b_kept_as_ref = i_nal_ref_idc != RKVENC_NAL_PRIORITY_DISPOSABLE;// && h->param.i_keyint_max > 1;


    if (syn->keyframe_max_interval == 1)
        i_nal_ref_idc = RKVENC_NAL_PRIORITY_LOW;

    dpb_ctx->i_nal_ref_idc = i_nal_ref_idc;
    dpb_ctx->i_nal_type = i_nal_type;


    dpb_ctx->fdec->i_pts = dpb_ctx->fdec->i_pts;


    /* build list */
    hal_h264e_rkv_reference_build_list(ctx);

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

    if (dpb_ctx->i_nal_type == RKVENC_NAL_SLICE_IDR) {
        if (ref_cfg->i_long_term_en && ref_cfg->hw_longterm_mode && ((dpb_ctx->fdec->i_frame_cnt % ref_cfg->i_long_term_internal) == 0) )
            dpb_ctx->i_long_term_reference_flag = 1;
        dpb_ctx->i_idr_pic_id = dpb_ctx->i_tmp_idr_pic_id;
        dpb_ctx->i_tmp_idr_pic_id ^= 1;
    } else {
        dpb_ctx->i_long_term_reference_flag = 0;
        dpb_ctx->i_idr_pic_id = -1;
    }


    h264e_hal_debug_leave();

    return MPP_OK;
}


static MPP_RET hal_h264e_rkv_reference_frame_update( h264e_hal_context *ctx)
{
    //h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;

    h264e_hal_debug_enter();
    if (MPP_OK != hal_h264e_rkv_reference_update(ctx)) {
        mpp_log("reference update failed, return now");
        return MPP_NOK;
    }

    h264e_hal_debug_leave();
    return MPP_OK;
}

#endif //RKV_H264E_NUM_REFS > 1


static MPP_RET hal_h264e_rkv_free_buffers(h264e_hal_context *ctx)
{
    RK_S32 k = 0;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    h264e_hal_debug_enter();
    for (k = 0; k < 2; k++) {
        if (buffers->hw_pp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_pp_buf[k])) {
                mpp_err("hw_pp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < 2; k++) {
        if (buffers->hw_dsp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_dsp_buf[k])) {
                mpp_err("hw_dsp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_mei_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_mei_buf[k])) {
                mpp_err("hw_mei_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < 2; k++) {
        if (buffers->hw_cmv_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_cmv_buf[k])) {
                mpp_err("hw_cmv_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_roi_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_roi_buf[k])) {
                mpp_err("hw_roi_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    {
        RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
        for (k = 0; k < num_buf; k++) {
            if (buffers->hw_rec_buf[k]) {
                if (MPP_OK != mpp_buffer_put(buffers->hw_rec_buf[k])) {
                    mpp_err("hw_rec_buf[%d] put failed", k);
                    return MPP_NOK;
                }
            }
        }
    }



    for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (buffers->hw_buf_grp[k]) {
            if (MPP_OK != mpp_buffer_group_put(buffers->hw_buf_grp[k])) {
                mpp_err("buf group[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    h264e_hal_debug_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_rkv_allocate_buffers(h264e_hal_context *ctx, h264e_syntax *syn)
{
    RK_S32 k = 0;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 frame_size = ((syn->pic_luma_width + 15) & (~15)) * ((syn->pic_luma_height + 15) & (~15)) * 3 / 2;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_rkv_frame *frame_buf = dpb_ctx->frame_buf;

    h264e_hal_debug_enter();
    //TODO: reduce buf size
    for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (MPP_OK != mpp_buffer_group_get_internal(&buffers->hw_buf_grp[k], MPP_BUFFER_TYPE_ION)) {
            mpp_err("buf group[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }

    for (k = 0; k < 2; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_PP], &buffers->hw_pp_buf[k], frame_size)) {
            mpp_err("hw_pp_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }
    for (k = 0; k < 2; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_DSP], &buffers->hw_dsp_buf[k], frame_size)) {
            mpp_err("hw_dsp_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_MEI], &buffers->hw_mei_buf[k], frame_size)) {
            mpp_err("hw_mei_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }
    for (k = 0; k < 2; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_CMV], &buffers->hw_cmv_buf[k], frame_size)) {
            mpp_err("hw_cmv_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_ROI], &buffers->hw_roi_buf[k], frame_size)) {
            mpp_err("hw_roi_buf[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }


    {
        RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
        for (k = 0; k < num_buf; k++) {
            if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_REC], &buffers->hw_rec_buf[k], frame_size)) {
                mpp_err("hw_rec_buf[%d] get failed", k);
                return MPP_ERR_MALLOC;
            }
            frame_buf[k].hw_buf = buffers->hw_rec_buf[k];
        }
    }


    h264e_hal_debug_leave();
    return MPP_OK;
}


static void hal_h264e_rkv_set_param(h264e_hal_param *p)
{
    h264e_hal_vui_param *vui = &p->vui;
    h264e_hal_ref_param *ref = &p->ref;

    p->constrained_intra = 0;

    vui->i_sar_height   = 0;
    vui->i_sar_width    = 0;
    vui->i_overscan     = 0;
    vui->i_vidformat    = 5;
    vui->b_fullrange    = 0;
    vui->i_colorprim    = 2;
    vui->i_transfer     = 2;
    vui->i_colmatrix    = -1;
    vui->i_chroma_loc   = 0;

    ref->i_frame_reference = RKV_H264E_NUM_REFS;
    ref->i_dpb_size = RKV_H264E_NUM_REFS;
    ref->i_ref_pos = 1;
    ref->i_long_term_en = RKV_H264E_LONGTERM_REF_EN;
    ref->hw_longterm_mode = 0;
    ref->i_long_term_internal = 0;
    ref->i_frame_packing = -1;
}

static void hal_h264e_rkv_adjust_param(h264e_hal_context *ctx)
{
    h264e_hal_param *par = &ctx->param;
    (void)par;
}

MPP_RET hal_h264e_rkv_set_sps(h264e_hal_sps *sps, h264e_hal_param *par, h264e_control_extra_info_cfg *cfg)
{
    h264e_hal_vui_param vui = par->vui;
    h264e_hal_ref_param ref_cfg = par->ref;
    RK_S32 max_frame_num = 0;

    //default settings
    RK_S32 i_bframe_pyramid = 0;
    RK_S32 i_bframe = 0;
    RK_S32 b_intra_refresh = 0;
    RK_S32 Log2MaxFrameNum = 16;
    RK_S32 Sw_log2_max_pic_order_cnt_lsb_minus4 = 12;
    RK_S32 b_interlaced = 0;
    RK_S32 b_fake_interlaced = 0;
    RK_S32 crop_rect_left = 0;
    RK_S32 crop_rect_right = 0;
    RK_S32 crop_rect_top = 0;
    RK_S32 crop_rect_bottom = 0;
    RK_S32 i_timebase_num = 1;
    RK_S32 i_timebase_den = 25;
    RK_S32 b_vfr_input = 0;
    RK_S32 i_nal_hrd = 0;
    RK_S32 b_pic_struct = 0;
    RK_S32 analyse_mv_range = 128; //TODO: relative to level_idc, transplant x264_validate_levels.
    RK_S32 csp = h264e_csp_idx_map[cfg->input_image_format] & RKV_H264E_CSP2_MASK;
    RK_S32 i_dpb_size = ref_cfg.i_dpb_size;
    RK_S32 i_frame_reference = ref_cfg.i_frame_reference;

    sps->i_id = 0;
    sps->i_mb_width = ( cfg->pic_luma_width + 15 ) / 16;
    sps->i_mb_height = ( cfg->pic_luma_height + 15 ) / 16;
    sps->i_chroma_format_idc = csp >= RKV_H264E_CSP2_I444 ? RKV_H264E_CHROMA_444 :
                               csp >= RKV_H264E_CSP2_I422 ? RKV_H264E_CHROMA_422 : RKV_H264E_CHROMA_420;

    sps->b_qpprime_y_zero_transform_bypass = 0; //param->rc.i_rc_method == X264_RC_CQP && param->rc.i_qp_constant == 0;
    sps->i_profile_idc = cfg->profile_idc;

    sps->b_constraint_set0  = sps->i_profile_idc == H264_PROFILE_BASELINE;
    /* x264 doesn't support the features that are in Baseline and not in Main,
     * namely arbitrary_slice_order and slice_groups. */
    sps->b_constraint_set1  = sps->i_profile_idc <= H264_PROFILE_MAIN;
    /* Never set constraint_set2, it is not necessary and not used in real world. */
    sps->b_constraint_set2  = 0;
    sps->b_constraint_set3  = 0;

    sps->i_level_idc = cfg->level_idc;
    if ( cfg->level_idc == 9 && ( sps->i_profile_idc == H264_PROFILE_BASELINE || sps->i_profile_idc == H264_PROFILE_MAIN ) ) {
        sps->b_constraint_set3 = 1; /* level 1b with Baseline or Main profile is signalled via constraint_set3 */
        sps->i_level_idc      = 11;
    }
    /* Intra profiles */
    if ( cfg->keyframe_max_interval == 1 && sps->i_profile_idc > H264_PROFILE_HIGH )
        sps->b_constraint_set3 = 1;

    sps->vui.i_num_reorder_frames = i_bframe_pyramid ? 2 : i_bframe ? 1 : 0;
    /* extra slot with pyramid so that we don't have to override the
     * order of forgetting old pictures */
    sps->vui.i_max_dec_frame_buffering =
        sps->i_num_ref_frames = H264E_HAL_MIN(H264E_REF_MAX, H264E_HAL_MAX4(i_frame_reference, 1 + sps->vui.i_num_reorder_frames,
                                                                            i_bframe_pyramid ? 4 : 1, i_dpb_size)); //TODO: multi refs
    sps->i_num_ref_frames -= i_bframe_pyramid == RKV_H264E_B_PYRAMID_STRICT; //TODO: multi refs
    if ( cfg->keyframe_max_interval == 1 ) {
        sps->i_num_ref_frames = 0;
        sps->vui.i_max_dec_frame_buffering = 0;
    }

    /* number of refs + current frame */
    max_frame_num = sps->vui.i_max_dec_frame_buffering * (!!i_bframe_pyramid + 1) + 1;
    /* Intra refresh cannot write a recovery time greater than max frame num-1 */
    if (b_intra_refresh ) {
        RK_S32 time_to_recovery = H264E_HAL_MIN( sps->i_mb_width - 1, cfg->keyframe_max_interval) + i_bframe - 1;
        max_frame_num = H264E_HAL_MAX( max_frame_num, time_to_recovery + 1 );
    }

    sps->i_log2_max_frame_num = Log2MaxFrameNum;//fix by gsl  org 16, at least 4
    while ( (1 << sps->i_log2_max_frame_num) <= max_frame_num )
        sps->i_log2_max_frame_num++;

    sps->i_poc_type = 0;

    if ( sps->i_poc_type == 0 ) {
        RK_S32 max_delta_poc = (i_bframe + 2) * (!!i_bframe_pyramid + 1) * 2;
        sps->i_log2_max_poc_lsb = Sw_log2_max_pic_order_cnt_lsb_minus4 + 4;
        while ( (1 << sps->i_log2_max_poc_lsb) <= max_delta_poc * 2 )
            sps->i_log2_max_poc_lsb++;
    }

    sps->b_vui = 1;

    sps->b_gaps_in_frame_num_value_allowed = 0;
    sps->b_frame_mbs_only = !(b_interlaced || b_fake_interlaced);
    if ( !sps->b_frame_mbs_only )
        sps->i_mb_height = ( sps->i_mb_height + 1 ) & ~1;
    sps->b_mb_adaptive_frame_field = b_interlaced;
    sps->b_direct8x8_inference = 1;

    sps->crop.i_left   = crop_rect_left;
    sps->crop.i_top    = crop_rect_top;
    sps->crop.i_right  = crop_rect_right + sps->i_mb_width * 16 - cfg->pic_luma_width;
    sps->crop.i_bottom = (crop_rect_bottom + sps->i_mb_height * 16 - cfg->pic_luma_height) >> !sps->b_frame_mbs_only;
    sps->b_crop = sps->crop.i_left  || sps->crop.i_top ||
                  sps->crop.i_right || sps->crop.i_bottom;

    sps->vui.b_aspect_ratio_info_present = 0;
    if (vui.i_sar_width > 0 && vui.i_sar_height > 0 ) {
        sps->vui.b_aspect_ratio_info_present = 1;
        sps->vui.i_sar_width = vui.i_sar_width;
        sps->vui.i_sar_height = vui.i_sar_height;
    }

    sps->vui.b_overscan_info_present = vui.i_overscan > 0 && vui.i_overscan <= 2;
    if ( sps->vui.b_overscan_info_present )
        sps->vui.b_overscan_info = ( vui.i_overscan == 2 ? 1 : 0 );

    sps->vui.b_signal_type_present = 0;
    sps->vui.i_vidformat = ( vui.i_vidformat >= 0 && vui.i_vidformat <= 5 ? vui.i_vidformat : 5 );
    sps->vui.b_fullrange = ( vui.b_fullrange >= 0 && vui.b_fullrange <= 1 ? vui.b_fullrange :
                             ( csp >= RKV_H264E_CSP2_BGR ? 1 : 0 ) );
    sps->vui.b_color_description_present = 0;

    sps->vui.i_colorprim = ( vui.i_colorprim >= 0 && vui.i_colorprim <=  9 ? vui.i_colorprim : 2 );
    sps->vui.i_transfer  = ( vui.i_transfer  >= 0 && vui.i_transfer  <= 15 ? vui.i_transfer  : 2 );
    sps->vui.i_colmatrix = ( vui.i_colmatrix >= 0 && vui.i_colmatrix <= 10 ? vui.i_colmatrix :
                             ( csp >= RKV_H264E_CSP2_BGR ? 0 : 2 ) );
    if ( sps->vui.i_colorprim != 2 ||
         sps->vui.i_transfer  != 2 ||
         sps->vui.i_colmatrix != 2 ) {
        sps->vui.b_color_description_present = 1;
    }

    if ( sps->vui.i_vidformat != 5 ||
         sps->vui.b_fullrange ||
         sps->vui.b_color_description_present ) {
        sps->vui.b_signal_type_present = 1;
    }

    /* FIXME: not sufficient for interlaced video */
    sps->vui.b_chroma_loc_info_present = vui.i_chroma_loc > 0 && vui.i_chroma_loc <= 5 &&
                                         sps->i_chroma_format_idc == RKV_H264E_CHROMA_420;
    if ( sps->vui.b_chroma_loc_info_present ) {
        sps->vui.i_chroma_loc_top = vui.i_chroma_loc;
        sps->vui.i_chroma_loc_bottom = vui.i_chroma_loc;
    }

    sps->vui.b_timing_info_present = i_timebase_num > 0 && i_timebase_den > 0;

    if ( sps->vui.b_timing_info_present ) {
        sps->vui.i_num_units_in_tick = i_timebase_num;
        sps->vui.i_time_scale = i_timebase_den * 2;
        sps->vui.b_fixed_frame_rate = !b_vfr_input;
    }

    sps->vui.b_vcl_hrd_parameters_present = 0; // we don't support VCL HRD
    sps->vui.b_nal_hrd_parameters_present = !!i_nal_hrd;
    sps->vui.b_pic_struct_present = b_pic_struct;

    // NOTE: HRD related parts of the SPS are initialised in x264_ratecontrol_init_reconfigurable

    sps->vui.b_bitstream_restriction = cfg->keyframe_max_interval > 1;
    if ( sps->vui.b_bitstream_restriction ) {
        sps->vui.b_motion_vectors_over_pic_boundaries = 1;
        sps->vui.i_max_bytes_per_pic_denom = 0;
        sps->vui.i_max_bits_per_mb_denom = 0;
        sps->vui.i_log2_max_mv_length_horizontal =
            sps->vui.i_log2_max_mv_length_vertical = (RK_S32)log2f((float)H264E_HAL_MAX( 1, analyse_mv_range * 4 - 1 ) ) + 1;
    }

    return MPP_OK;
}


RK_S32 hal_h264e_rkv_stream_get_pos(h264e_hal_rkv_stream *s)
{
    return (8 * (s->p - s->p_start) + (4 * 8) - s->i_left);
}


MPP_RET hal_h264e_rkv_stream_init(h264e_hal_rkv_stream *s)
{
    RK_S32 offset = 0;
    RK_U8 *p_buf = NULL;
    s->buf = mpp_calloc(RK_U8, 512); //SPS+PPS
    p_buf = s->buf + 8; //NOTE

    offset = (size_t)(p_buf) & 3;
    s->p = s->p_start = p_buf - offset;
    //s->p_end = (RK_U8*)s->buf + i_data;
    s->i_left = (4 - offset) * 8;
    //s->cur_bits = endian_fix32(M32(s->p));
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16) + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;

    return MPP_OK;
}


MPP_RET hal_h264e_rkv_stream_reset(h264e_hal_rkv_stream *s)
{
    RK_S32 offset = 0;
    RK_U8 *p_buf = NULL;
    p_buf = s->buf + 8; //NOTE
    h264e_hal_debug_enter();

    offset = (size_t)(p_buf) & 3;
    s->p = s->p_start = p_buf - offset;
    //s->p_end = (RK_U8*)s->buf + i_data;
    s->i_left = (4 - offset) * 8;
    //s->cur_bits = endian_fix32(M32(s->p));
    s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16) + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
    s->cur_bits >>= (4 - offset) * 8;
    s->count_bit = 0;
    h264e_hal_debug_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_deinit(h264e_hal_rkv_stream *s)
{
    MPP_FREE(s->buf);

    s->p = NULL;
    s->p_start = NULL;
    //s->p_end = NULL;

    s->i_left = 0;
    s->cur_bits = 0;
    s->count_bit = 0;

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_realign(h264e_hal_rkv_stream *s)
{
    RK_S32 offset = (size_t)(s->p) & 3; //used to judge alignment
    if (offset) {
        s->p = s->p - offset; //move pointer to 32bit aligned pos
        s->i_left = (4 - offset) * 8; //init
        //s->cur_bits = endian_fix32(M32(s->p));
        s->cur_bits = (*(s->p) << 24) + (*(s->p + 1) << 16) + (*(s->p + 2) << 8) + (*(s->p + 3) << 0);
        s->cur_bits >>= (4 - offset) * 8; //shift right the invalid bit
    }

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_write_with_log(h264e_hal_rkv_stream *s, RK_S32 i_count, RK_U32 val, char *name)
{
    RK_U32 i_bits = val;

    h264e_hal_log_detail("write bits name %s, count %d, val %d", name, i_count, val);

    s->count_bit += i_count;
    if (i_count < s->i_left) {
        s->cur_bits = (s->cur_bits << i_count) | i_bits;
        s->i_left -= i_count;
    } else {
        i_count -= s->i_left;
        s->cur_bits = (s->cur_bits << s->i_left) | (i_bits >> i_count);
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = 0;
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->cur_bits = i_bits;
        s->i_left = 32 - i_count;
    }
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_write1_with_log(h264e_hal_rkv_stream *s, RK_U32 val, char *name)
{
    RK_U32 i_bit = val;

    h264e_hal_log_detail("write 1 bit name %s, val %d", name, val);

    s->count_bit += 1;
    s->cur_bits <<= 1;
    s->cur_bits |= i_bit;
    s->i_left--;
    if (s->i_left == 4 * 8 - 32) {
        //M32(s->p) = endian_fix32(s->cur_bits);
        *(s->p) = (s->cur_bits >> 24) & 0xff;
        *(s->p + 1) = (s->cur_bits >> 16) & 0xff;
        *(s->p + 2) = (s->cur_bits >> 8) & 0xff;
        *(s->p + 3) = (s->cur_bits >> 0) & 0xff;
        s->p += 4;
        s->i_left = 4 * 8;
    }
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_write_ue_with_log(h264e_hal_rkv_stream *s, RK_U32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = ++val;

    h264e_hal_log_detail("write UE bits name %s, val %d (2 steps below are real writting)", name, val);
    if (tmp >= 0x10000) {
        size = 32;
        tmp >>= 16;
    }
    if (tmp >= 0x100) {
        size += 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    hal_h264e_rkv_stream_write_with_log(s, size >> 1, 0, name);
    hal_h264e_rkv_stream_write_with_log(s, (size >> 1) + 1, val, name);

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_stream_write_se_with_log(h264e_hal_rkv_stream *s, RK_S32 val, char *name)
{
    RK_S32 size = 0;
    RK_S32 tmp = 1 - val * 2;
    if (tmp < 0)
        tmp = val * 2;

    val = tmp;
    if (tmp >= 0x100) {
        size = 16;
        tmp >>= 8;
    }
    size += h264e_ue_size_tab[tmp];

    return hal_h264e_rkv_stream_write_with_log(s, size, val, name);
}

MPP_RET hal_h264e_rkv_stream_write32(h264e_hal_rkv_stream *s, RK_U32 i_bits, char *name)
{
    hal_h264e_rkv_stream_write_with_log(s, 16, i_bits >> 16, name);
    hal_h264e_rkv_stream_write_with_log(s, 16, i_bits      , name);

    return MPP_OK;
}

static RK_S32 hal_h264e_rkv_stream_size_se( RK_S32 val )
{
    RK_S32 tmp = 1 - val * 2;
    if ( tmp < 0 ) tmp = val * 2;
    if ( tmp < 256 )
        return h264e_ue_size_tab[tmp];
    else
        return h264e_ue_size_tab[tmp >> 8] + 16;
}


MPP_RET hal_h264e_rkv_stream_rbsp_trailing(h264e_hal_rkv_stream *s)
{
    //align bits, 1+N zeros.
    hal_h264e_rkv_stream_write1_with_log(s, 1, "align_1_bit");
    hal_h264e_rkv_stream_write_with_log(s, s->i_left & 7, 0, "align_N_bits");

    return MPP_OK;
}

/* Write the rest of cur_bits to the bitstream; results in a bitstream no longer 32-bit aligned. */
MPP_RET hal_h264e_rkv_stream_flush(h264e_hal_rkv_stream *s)
{
    //do 4 bytes aligned
    //M32(s->p) = endian_fix32(s->cur_bits << (s->i_left & 31));
    RK_U32 tmp_bit = s->cur_bits << (s->i_left & 31);
    *(s->p) = (tmp_bit >> 24) & 0xff;
    *(s->p + 1) = (tmp_bit >> 16) & 0xff;
    *(s->p + 2) = (tmp_bit >> 8) & 0xff;
    *(s->p + 3) = (tmp_bit >> 0) & 0xff;
    s->p += 4 - (s->i_left >> 3);//p point to bit which aligned, rather than the pos next to 4-byte alinged
    s->i_left = 4 * 8;

    return MPP_OK;
}

void hal_h264e_rkv_nals_init(h264e_hal_rkv_out *out)
{
    out->nal_buf = mpp_calloc(RK_U8, 512);
    out->nal_num = 0;
}

void hal_h264e_rkv_nals_deinit(h264e_hal_rkv_out *out)
{
    MPP_FREE(out->nal_buf);

    out->nal_num = 0;
}

void hal_h264e_rkv_nal_start(h264e_hal_rkv_out *out, RK_S32 i_type, RK_S32 i_ref_idc)
{
    h264e_hal_rkv_stream *s = &out->stream;
    RK_U8 *stream_buf = s->p_start;
    out->nal[out->nal_num].i_ref_idc = i_ref_idc;
    out->nal[out->nal_num].i_type = i_type;
    out->nal[out->nal_num].b_long_startcode = 1;

    out->nal[out->nal_num].i_payload = 0;
    out->nal[out->nal_num].p_payload = &stream_buf[hal_h264e_rkv_stream_get_pos(s) / 8];
    out->nal[out->nal_num].i_padding = 0;
}

void hal_h264e_rkv_nal_end(h264e_hal_rkv_out *out)
{
    h264e_hal_rkv_nal *nal = &(out->nal[out->nal_num]);
    h264e_hal_rkv_stream *s = &out->stream;
    RK_U8 *stream_buf = s->p_start;
    RK_U8 *end = &stream_buf[hal_h264e_rkv_stream_get_pos(s) / 8];
    nal->i_payload = end - nal->p_payload;
    /* Assembly implementation of nal_escape reads past the end of the input.
    * While undefined padding wouldn't actually affect the output, it makes valgrind unhappy. */
    memset(end, 0xff, 64);
    //if (h->param.nalu_process)
    //  h->param.nalu_process(h, nal, h->fenc->opaque);
    out->nal_num++;
    //return x264_nal_check_buffer(h);
}

RK_U8 *hal_h264e_rkv_nal_escape_c(RK_U8 *dst, RK_U8 *src, RK_U8 *end)
{
    if (src < end) *dst++ = *src++;
    if (src < end) *dst++ = *src++;
    while (src < end) {
        if (src[0] <= 0x03 && !dst[-2] && !dst[-1])
            *dst++ = 0x03;
        *dst++ = *src++;
    }
    return dst;
}

void hal_h264e_rkv_nal_encode(RK_U8 *dst, h264e_hal_rkv_nal *nal)
{
    RK_S32 b_annexb = 1;
    RK_S32 size = 0;
    RK_U8 *src = nal->p_payload;
    RK_U8 *end = nal->p_payload + nal->i_payload;
    RK_U8 *orig_dst = dst;

    if (b_annexb) {
        //if (nal->b_long_startcode)//fix by gsl
        //*dst++ = 0x00;//fix by gsl
        *dst++ = 0x00;
        *dst++ = 0x00;
        *dst++ = 0x01;
    } else /* save room for size later */
        dst += 4;

    /* nal header */
    *dst++ = (0x00 << 7) | (nal->i_ref_idc << 5) | nal->i_type;

    dst = hal_h264e_rkv_nal_escape_c(dst, src, end);
    size = (dst - orig_dst) - 4;

    /* Write the size header for mp4/etc */
    if (!b_annexb) {
        /* Size doesn't include the size of the header we're writing now. */
        orig_dst[0] = size >> 24;
        orig_dst[1] = size >> 16;
        orig_dst[2] = size >> 8;
        orig_dst[3] = size >> 0;
    }

    nal->i_payload = size + 4;
    nal->p_payload = orig_dst;
}



MPP_RET hal_h264e_rkv_encapsulate_nals(h264e_hal_rkv_out *out)
{
    RK_S32 i = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 nal_size = 0;
    RK_S32 necessary_size = 0;
    RK_U8 *nal_buffer = out->nal_buf;
    RK_S32 nal_num = out->nal_num;
    h264e_hal_rkv_nal *nal = out->nal;

    h264e_hal_debug_enter();

    for (i = 0; i < nal_num; i++)
        nal_size += nal[i].i_payload;

    /* Worst-case NAL unit escaping: reallocate the buffer if it's too small. */
    necessary_size = nal_size * 3 / 2 + nal_num * 4 + 4 + 64;
    for (i = 0; i < nal_num; i++)
        necessary_size += nal[i].i_padding;

    for (i = 0; i < nal_num; i++) {
        nal[i].b_long_startcode = !i ||
                                  nal[i].i_type == RKVENC_NAL_SPS ||
                                  nal[i].i_type == RKVENC_NAL_PPS ||
                                  i_avcintra_class;
        hal_h264e_rkv_nal_encode(nal_buffer, &nal[i]);
        nal_buffer += nal[i].i_payload;
    }

    mpp_log("nals total size: %d bytes",  nal_buffer - out->nal_buf);

    h264e_hal_debug_leave();

    return MPP_OK;
}



MPP_RET hal_h264e_rkv_sps_write(h264e_hal_sps *sps, h264e_hal_rkv_stream *s)
{
    h264e_hal_debug_enter();

    s->count_bit = 0;
    hal_h264e_rkv_stream_realign(s);
    hal_h264e_rkv_stream_write_with_log(s, 8, sps->i_profile_idc, "profile_idc");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set0, "constraint_set0_flag");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set1, "constraint_set1_flag");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set2, "constraint_set2_flag");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_constraint_set3, "constraint_set3_flag");

    hal_h264e_rkv_stream_write_with_log(s, 4, 0, "reserved_zero_4bits");

    hal_h264e_rkv_stream_write_with_log(s, 8, sps->i_level_idc, "level_idc");

    hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_id, "seq_parameter_set_id");

    if (sps->i_profile_idc >= H264_PROFILE_HIGH) {
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_chroma_format_idc, "chroma_format_idc");
        if (sps->i_chroma_format_idc == RKV_H264E_CHROMA_444)
            hal_h264e_rkv_stream_write1_with_log(s, 0, "separate_colour_plane_flag");
        hal_h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_luma_minus8");
        hal_h264e_rkv_stream_write_ue_with_log(s, H264_BIT_DEPTH - 8, "bit_depth_chroma_minus8");
        hal_h264e_rkv_stream_write1_with_log(s, sps->b_qpprime_y_zero_transform_bypass, "qpprime_y_zero_transform_bypass_flag");
        hal_h264e_rkv_stream_write1_with_log(s, 0, "seq_scaling_matrix_present_flag");
    }

    hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_frame_num - 4, "log2_max_frame_num_minus4");
    hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_poc_type, "delta_pic_order_always_zero_flag");
    if (sps->i_poc_type == 0)
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_log2_max_poc_lsb - 4, "log2_max_pic_order_cnt_lsb_minus4");
    hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_num_ref_frames, "max_num_ref_frames");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_gaps_in_frame_num_value_allowed, "gaps_in_frame_num_value_allowed_flag");
    hal_h264e_rkv_stream_write_ue_with_log(s, sps->i_mb_width - 1, "pic_width_in_mbs_minus1");
    hal_h264e_rkv_stream_write_ue_with_log(s, (sps->i_mb_height >> !sps->b_frame_mbs_only) - 1, "pic_height_in_map_units_minus1");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_frame_mbs_only, "frame_mbs_only_flag");
    if (!sps->b_frame_mbs_only)
        hal_h264e_rkv_stream_write1_with_log(s, sps->b_mb_adaptive_frame_field, "mb_adaptive_frame_field_flag");
    hal_h264e_rkv_stream_write1_with_log(s, sps->b_direct8x8_inference, "direct_8x8_inference_flag");

    hal_h264e_rkv_stream_write1_with_log(s, sps->b_crop, "frame_cropping_flag");
    if (sps->b_crop) {
        RK_S32 h_shift = sps->i_chroma_format_idc == RKV_H264E_CHROMA_420 || sps->i_chroma_format_idc == RKV_H264E_CHROMA_422;
        RK_S32 v_shift = sps->i_chroma_format_idc == RKV_H264E_CHROMA_420;
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_left >> h_shift, "frame_crop_left_offset");
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_right >> h_shift, "frame_crop_right_offset");
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_top >> v_shift, "frame_crop_top_offset");
        hal_h264e_rkv_stream_write_ue_with_log(s, sps->crop.i_bottom >> v_shift, "frame_crop_bottom_offset");
    }

    hal_h264e_rkv_stream_write1_with_log(s, sps->b_vui, "vui_parameters_present_flag");
    if (sps->b_vui) {
        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_aspect_ratio_info_present, "aspect_ratio_info_present_flag");
        if (sps->vui.b_aspect_ratio_info_present) {
            RK_S32 i = 0;
            static const struct { RK_U8 w, h, sar; } sar[] = {
                // aspect_ratio_idc = 0 -> unspecified
                { 1, 1, 1 }, { 12, 11, 2 }, { 10, 11, 3 }, { 16, 11, 4 },
                { 40, 33, 5 }, { 24, 11, 6 }, { 20, 11, 7 }, { 32, 11, 8 },
                { 80, 33, 9 }, { 18, 11, 10 }, { 15, 11, 11 }, { 64, 33, 12 },
                { 160, 99, 13 }, { 4, 3, 14 }, { 3, 2, 15 }, { 2, 1, 16 },
                // aspect_ratio_idc = [17..254] -> reserved
                { 0, 0, 255 }
            };
            for (i = 0; sar[i].sar != 255; i++) {
                if (sar[i].w == sps->vui.i_sar_width &&
                    sar[i].h == sps->vui.i_sar_height)
                    break;
            }
            hal_h264e_rkv_stream_write_with_log(s, 8, sar[i].sar, "aspect_ratio_idc");
            if (sar[i].sar == 255) { /* aspect_ratio_idc (extended) */
                hal_h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_width, "sar_width");
                hal_h264e_rkv_stream_write_with_log(s, 16, sps->vui.i_sar_height, "sar_height");
            }
        }

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info_present, "overscan_info_present_flag");
        if (sps->vui.b_overscan_info_present)
            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_overscan_info, "overscan_appropriate_flag");

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_signal_type_present, "video_signal_type_present_flag");
        if (sps->vui.b_signal_type_present) {
            hal_h264e_rkv_stream_write_with_log(s, 3, sps->vui.i_vidformat, "video_format");
            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_fullrange, "video_full_range_flag");
            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_color_description_present, "colour_description_present_flag");
            if (sps->vui.b_color_description_present) {
                hal_h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colorprim, "colour_primaries");
                hal_h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_transfer, "transfer_characteristics");
                hal_h264e_rkv_stream_write_with_log(s, 8, sps->vui.i_colmatrix, "matrix_coefficients");
            }
        }

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_chroma_loc_info_present, "chroma_loc_info_present_flag");
        if (sps->vui.b_chroma_loc_info_present) {
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_top, "chroma_loc_info_present_flag");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_chroma_loc_bottom, "chroma_sample_loc_type_bottom_field");
        }

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_timing_info_present, "chroma_sample_loc_type_bottom_field");
        if (sps->vui.b_timing_info_present) {
            hal_h264e_rkv_stream_write32(s, sps->vui.i_num_units_in_tick, "num_units_in_tick");
            hal_h264e_rkv_stream_write32(s, sps->vui.i_time_scale, "time_scale");
            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_fixed_frame_rate, "fixed_frame_rate_flag");
        }

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_nal_hrd_parameters_present, "time_scale");
        if (sps->vui.b_nal_hrd_parameters_present) {
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_cnt - 1, "cpb_cnt_minus1");
            hal_h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_bit_rate_scale, "bit_rate_scale");
            hal_h264e_rkv_stream_write_with_log(s, 4, sps->vui.hrd.i_cpb_size_scale, "cpb_size_scale");

            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_bit_rate_value - 1, "bit_rate_value_minus1");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.hrd.i_cpb_size_value - 1, "cpb_size_value_minus1");

            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.hrd.b_cbr_hrd, "cbr_flag");

            hal_h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_initial_cpb_removal_delay_length - 1, "initial_cpb_removal_delay_length_minus1");
            hal_h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_cpb_removal_delay_length - 1, "cpb_removal_delay_length_minus1");
            hal_h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_dpb_output_delay_length - 1, "dpb_output_delay_length_minus1");
            hal_h264e_rkv_stream_write_with_log(s, 5, sps->vui.hrd.i_time_offset_length, "time_offset_length");
        }

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_vcl_hrd_parameters_present, "vcl_hrd_parameters_present_flag");

        if (sps->vui.b_nal_hrd_parameters_present || sps->vui.b_vcl_hrd_parameters_present)
            hal_h264e_rkv_stream_write1_with_log(s, 0, "low_delay_hrd_flag");   /* low_delay_hrd_flag */

        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_pic_struct_present, "pic_struct_present_flag");
        hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_bitstream_restriction, "bitstream_restriction_flag");
        if (sps->vui.b_bitstream_restriction) {
            hal_h264e_rkv_stream_write1_with_log(s, sps->vui.b_motion_vectors_over_pic_boundaries, "motion_vectors_over_pic_boundaries_flag");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bytes_per_pic_denom, "max_bytes_per_pic_denom");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_bits_per_mb_denom, "max_bits_per_mb_denom");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_horizontal, "log2_max_mv_length_horizontal");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_log2_max_mv_length_vertical, "log2_max_mv_length_vertical");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_num_reorder_frames, "max_num_reorder_frames");
            hal_h264e_rkv_stream_write_ue_with_log(s, sps->vui.i_max_dec_frame_buffering, "max_num_reorder_frames");
        }
    }
    hal_h264e_rkv_stream_rbsp_trailing(s);
    hal_h264e_rkv_stream_flush(s);

    mpp_log("write pure sps head size: %d bits", s->count_bit);

    h264e_hal_debug_leave();

    return MPP_OK;
}


/*
static void transpose( RK_U8 *buf, RK_S32 w )
{
    RK_S32 i=0, j=0;
    for(i = 0; i < w; i++ )
        for(j = 0; j < i; j++ )
            MPP_SWAP( RK_U8, buf[w*i+j], buf[w*j+i] );
}
*/

MPP_RET hal_h264e_rkv_set_pps(h264e_hal_pps *pps, h264e_hal_param *par, h264e_control_extra_info_cfg *cfg, h264e_hal_sps *sps)
{
    RK_S32 k = 0;
    RK_S32 i_avcintra_class = 0;
    RK_S32 b_interlaced = 0;
    RK_S32 analyse_weighted_pred = 0;
    RK_S32 analyse_b_weighted_bipred = 0;
    RK_S32 pps_init_qp = -1; //TODO: merge with syn
    RK_S32 Sw_deblock_filter_ctrl_present_flag = 1;
    RK_S32 b_cqm_preset = 0;

    pps->i_id = cfg->pps_id;
    pps->i_sps_id = sps->i_id;
    pps->b_cabac = cfg->enable_cabac;

    pps->b_pic_order = !i_avcintra_class && b_interlaced;
    pps->i_num_slice_groups = 1;

    pps->i_num_ref_idx_l0_default_active = 1;
    pps->i_num_ref_idx_l1_default_active = 1;

    pps->b_weighted_pred = analyse_weighted_pred > 0;
    pps->i_weighted_bipred_idc = analyse_b_weighted_bipred ? 2 : 0;

    pps->i_pic_init_qp = cfg->pic_init_qp;
    if (pps_init_qp >= 0 && pps_init_qp <= 51) {
        pps->i_pic_init_qp = pps_init_qp;
    }
    pps->i_pic_init_qs = 26 + H264_QP_BD_OFFSET;

    pps->i_chroma_qp_index_offset = cfg->chroma_qp_index_offset;
    pps->i_second_chroma_qp_index_offset = cfg->second_chroma_qp_index_offset;
    pps->b_deblocking_filter_control = Sw_deblock_filter_ctrl_present_flag;
    pps->b_constrained_intra_pred = par->constrained_intra;
    pps->b_redundant_pic_cnt = 0;

    pps->b_transform_8x8_mode = cfg->transform8x8_mode ? 1 : 0;

    pps->b_cqm_preset = b_cqm_preset;

    switch ( pps->b_cqm_preset ) {
    case RKV_H264E_CQM_FLAT:
        for (k = 0; k < 8; k++ )
            pps->scaling_list[k] = h264e_rkv_cqm_flat16;
        break;
    case RKV_H264E_CQM_JVT:
        for (k = 0; k < 8; k++ )
            pps->scaling_list[k] = h264e_rkv_cqm_jvt[k];
        break;
    case RKV_H264E_CQM_CUSTOM:
        /* match the transposed DCT & zigzag */
        mpp_err("CQM_CUSTOM mode is not supported now");
        return MPP_NOK;
        //break;
    default:
        mpp_err("invalid cqm_preset mode");
        return MPP_NOK;
        //break;
    }

    return MPP_OK;
}

static void hal_h264e_rkv_scaling_list_write( h264e_hal_rkv_stream *s, h264e_hal_pps *pps, RK_S32 idx )
{
    RK_S32 k = 0;
    const RK_S32 len = idx < 4 ? 16 : 64;
    const RK_U8 *zigzag = idx < 4 ? h264e_rkv_zigzag_scan4[0] : h264e_rkv_zigzag_scan8[0];
    const RK_U8 *list = pps->scaling_list[idx];
    const RK_U8 *def_list = (idx == RKV_H264E_CQM_4IC) ? pps->scaling_list[RKV_H264E_CQM_4IY]
                            : (idx == RKV_H264E_CQM_4PC) ? pps->scaling_list[RKV_H264E_CQM_4PY]
                            : (idx == RKV_H264E_CQM_8IC + 4) ? pps->scaling_list[RKV_H264E_CQM_8IY + 4]
                            : (idx == RKV_H264E_CQM_8PC + 4) ? pps->scaling_list[RKV_H264E_CQM_8PY + 4]
                            : h264e_rkv_cqm_jvt[idx];
    if ( !memcmp( list, def_list, len ) )
        hal_h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_present_flag");   // scaling_list_present_flag
    else if ( !memcmp( list, h264e_rkv_cqm_jvt[idx], len ) ) {
        hal_h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag
        hal_h264e_rkv_stream_write_se_with_log( s, -8, "use_jvt_list"); // use jvt list
    } else {
        RK_S32 run;
        hal_h264e_rkv_stream_write1_with_log( s, 1, "scaling_list_present_flag");   // scaling_list_present_flag

        // try run-length compression of trailing values
        for ( run = len; run > 1; run-- )
            if ( list[zigzag[run - 1]] != list[zigzag[run - 2]] )
                break;
        if ( run < len && len - run < hal_h264e_rkv_stream_size_se( (RK_S8) - list[zigzag[run]] ) )
            run = len;

        for ( k = 0; k < run; k++ )
            hal_h264e_rkv_stream_write_se_with_log( s, (RK_S8)(list[zigzag[k]] - (k > 0 ? list[zigzag[k - 1]] : 8)), "delta_scale"); // delta

        if ( run < len )
            hal_h264e_rkv_stream_write_se_with_log( s, (RK_S8) - list[zigzag[run]], "-scale");
    }
}

MPP_RET hal_h264e_rkv_pps_write(h264e_hal_pps *pps, h264e_hal_sps *sps, h264e_hal_rkv_stream *s)
{
    h264e_hal_debug_enter();

    s->count_bit = 0;
    hal_h264e_rkv_stream_realign( s );

    hal_h264e_rkv_stream_write_ue_with_log( s, pps->i_id, "pic_parameter_set_id");
    hal_h264e_rkv_stream_write_ue_with_log( s, pps->i_sps_id, "seq_parameter_set_id");

    hal_h264e_rkv_stream_write1_with_log( s, pps->b_cabac, "entropy_coding_mode_flag");
    hal_h264e_rkv_stream_write1_with_log( s, pps->b_pic_order, "bottom_field_pic_order_in_frame_present_flag");
    hal_h264e_rkv_stream_write_ue_with_log( s, pps->i_num_slice_groups - 1, "num_slice_groups_minus1");

    hal_h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l0_default_active - 1, "num_ref_idx_l0_default_active_minus1");
    hal_h264e_rkv_stream_write_ue_with_log( s, pps->i_num_ref_idx_l1_default_active - 1, "num_ref_idx_l1_default_active_minus1");
    hal_h264e_rkv_stream_write1_with_log( s, pps->b_weighted_pred, "weighted_pred_flag");
    hal_h264e_rkv_stream_write_with_log( s, 2, pps->i_weighted_bipred_idc, "weighted_bipred_idc");

    hal_h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qp - 26 - H264_QP_BD_OFFSET, "pic_init_qp_minus26");
    hal_h264e_rkv_stream_write_se_with_log( s, pps->i_pic_init_qs - 26 - H264_QP_BD_OFFSET, "pic_init_qs_minus26");
    hal_h264e_rkv_stream_write_se_with_log( s, pps->i_chroma_qp_index_offset, "chroma_qp_index_offset");

    hal_h264e_rkv_stream_write1_with_log( s, pps->b_deblocking_filter_control, "deblocking_filter_control_present_flag");
    hal_h264e_rkv_stream_write1_with_log( s, pps->b_constrained_intra_pred, "constrained_intra_pred_flag");
    hal_h264e_rkv_stream_write1_with_log( s, pps->b_redundant_pic_cnt, "redundant_pic_cnt_present_flag");

    if ( pps->b_transform_8x8_mode || pps->b_cqm_preset != RKV_H264E_CQM_FLAT ) {
        hal_h264e_rkv_stream_write1_with_log( s, pps->b_transform_8x8_mode, "transform_8x8_mode_flag");
        hal_h264e_rkv_stream_write1_with_log( s, (pps->b_cqm_preset != RKV_H264E_CQM_FLAT), "pic_scaling_matrix_present_flag");
        if ( pps->b_cqm_preset != RKV_H264E_CQM_FLAT ) {
            hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_4IY);
            hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_4IC );
            hal_h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_4PY );
            hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_4PC );
            hal_h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag"); // Cr = Cb TODO:replaced with real name
            if ( pps->b_transform_8x8_mode ) {
                if ( sps->i_chroma_format_idc == RKV_H264E_CHROMA_444 ) {
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8IY + 4 );
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8IC + 4 );
                    hal_h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8PY + 4 );
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8PC + 4 );
                    hal_h264e_rkv_stream_write1_with_log( s, 0, "scaling_list_end_flag" ); // Cr = Cb TODO:replaced with real name
                } else {
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8IY + 4 );
                    hal_h264e_rkv_scaling_list_write( s, pps, RKV_H264E_CQM_8PY + 4 );
                }
            }
        }
        hal_h264e_rkv_stream_write_se_with_log( s, pps->i_second_chroma_qp_index_offset, "second_chroma_qp_index_offset");
    }

    hal_h264e_rkv_stream_rbsp_trailing( s );
    hal_h264e_rkv_stream_flush( s );

    mpp_log("write pure pps size: %d bits", s->count_bit);

    h264e_hal_debug_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_rkv_init_extra_info(h264e_hal_rkv_out *extra_info)
{
    hal_h264e_rkv_nals_init(extra_info);
    hal_h264e_rkv_stream_init(&extra_info->stream);

    return MPP_OK;
}

static MPP_RET hal_h264e_rkv_deinit_extra_info(void *extra_info)
{
    h264e_hal_rkv_out *info = (h264e_hal_rkv_out *)extra_info;
    hal_h264e_rkv_stream_deinit(&info->stream);
    hal_h264e_rkv_nals_deinit(info);

    return MPP_OK;
}

static MPP_RET hal_h264e_rkv_set_extra_info(h264e_hal_context *ctx, void *param)
{
    h264e_control_extra_info_cfg *cfg = (h264e_control_extra_info_cfg *)param;
    h264e_hal_param *par = &ctx->param;
    h264e_hal_rkv_out *info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps_info = &info->sps;
    h264e_hal_pps *pps_info = &info->pps;

    h264e_hal_debug_enter();
    info->nal_num = 0;
    hal_h264e_rkv_stream_reset(&info->stream);

    hal_h264e_rkv_nal_start(info, RKVENC_NAL_SPS, RKVENC_NAL_PRIORITY_HIGHEST);
    hal_h264e_rkv_set_sps(sps_info, par, cfg);
    hal_h264e_rkv_sps_write(sps_info, &info->stream);
    hal_h264e_rkv_nal_end(info);

    hal_h264e_rkv_nal_start(info, RKVENC_NAL_PPS, RKVENC_NAL_PRIORITY_HIGHEST);
    hal_h264e_rkv_set_pps(pps_info, par, cfg, sps_info);
    hal_h264e_rkv_pps_write(pps_info, sps_info, &info->stream);
    hal_h264e_rkv_nal_end(info);

    hal_h264e_rkv_encapsulate_nals(info);

    h264e_hal_debug_leave();

    return MPP_OK;
}

static MPP_RET hal_h264e_rkv_get_extra_info(void *dst_extra_info, void *src_extra_info)
{
    RK_S32 k = 0;
    h264e_hal_rkv_out *src = (h264e_hal_rkv_out *)src_extra_info;
    h264e_control_extra_info *dst = (h264e_control_extra_info *)dst_extra_info;
    h264e_hal_debug_enter();
    if (dst == NULL) {
        mpp_err("dst_extra_info is NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    mpp_log("get nal num: %d", src->nal_num);
    dst->size = 0;
    for (k = 0; k < src->nal_num; k++) {
        mpp_log("get extra info nal type %d, size %d bytes", src->nal[k].i_type, src->nal[k].i_payload);

        memcpy(dst->buf + dst->size, src->nal[k].p_payload, src->nal[k].i_payload);
        dst->size += src->nal[k].i_payload;
    }
    h264e_hal_debug_leave();
    return MPP_OK;
}


static void hal_h264e_rkv_reference_deinit( h264e_hal_rkv_dpb_ctx *dpb_ctx)
{
    h264e_hal_rkv_dpb_ctx *d_ctx = (h264e_hal_rkv_dpb_ctx *)dpb_ctx;

    h264e_hal_debug_enter();

    MPP_FREE(d_ctx->frames.unused);

    h264e_hal_debug_leave();
}


static void hal_h264e_rkv_reference_init( void *dpb,  h264e_hal_param *par)
{
    //RK_S32 k = 0;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)dpb;
    h264e_hal_ref_param *ref_cfg = &par->ref;

    h264e_hal_debug_enter();
    memset(dpb_ctx, 0, sizeof(h264e_hal_rkv_dpb_ctx));

    dpb_ctx->frames.unused = mpp_calloc(h264e_hal_rkv_frame *, H264E_REF_MAX + 1);
    //for(k=0; k<RKV_H264E_REF_MAX+1; k++) {
    //    dpb_ctx->frames.reference[k] = &dpb_ctx->frame_buf[k];
    //    mpp_log("dpb_ctx->frames.reference[%d]: %p", k, dpb_ctx->frames.reference[k]);
    //}
    dpb_ctx->i_long_term_reference_flag = ref_cfg->i_long_term_en;
    dpb_ctx->i_tmp_idr_pic_id = 0;

    h264e_hal_debug_leave();
}

MPP_RET hal_h264e_rkv_init(void *hal, MppHalCfg *cfg)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = NULL;
    h264e_hal_debug_enter();

    hal_h264e_rkv_set_param(&ctx->param);

    ctx->ioctl_input    = mpp_calloc(h264e_rkv_ioctl_input, 1);
    ctx->ioctl_output   = mpp_calloc(h264e_rkv_ioctl_output, 1);
    ctx->regs           = mpp_calloc(h264e_rkv_reg_set, RKV_H264E_LINKTABLE_FRAME_NUM);
    ctx->buffers        = mpp_calloc(h264e_hal_rkv_buffers, 1);
    ctx->extra_info     = mpp_calloc(h264e_hal_rkv_out, 1);
    ctx->dpb_ctx        = mpp_calloc(h264e_hal_rkv_dpb_ctx, 1);
    hal_h264e_rkv_init_extra_info(ctx->extra_info);
    hal_h264e_rkv_reference_init(ctx->dpb_ctx, &ctx->param);

    ctx->int_cb = cfg->hal_int_cb;
    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->frame_cnt_send_ready = 0;
    ctx->num_frames_to_send = 1;

    /* support multi-refs */
    dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    dpb_ctx->i_frame_cnt = 0;
    dpb_ctx->i_frame_num = 0;

    ctx->vpu_socket = -1;
    ctx->vpu_client = VPU_ENC_RKV;
    mpp_log("vpu client: %d", ctx->vpu_client);
#ifdef ANDROID
    if (ctx->vpu_socket <= 0) {
        ctx->vpu_socket = VPUClientInit(ctx->vpu_client);
        if (ctx->vpu_socket <= 0) {
            mpp_err("get vpu_socket(%d) <=0, failed. \n", ctx->vpu_socket);
            return MPP_ERR_UNKNOW;
        } else {
            VPUHwEncConfig_t hwCfg;
            mpp_log("get vpu_socket(%d), success. \n", ctx->vpu_socket);
            memset(&hwCfg, 0, sizeof(VPUHwEncConfig_t));
            if (VPUClientGetHwCfg(ctx->vpu_socket, (RK_U32*)&hwCfg, sizeof(hwCfg))) {
                mpp_err("h264enc # Get HwCfg failed, release vpu\n");
                VPUClientRelease(ctx->vpu_socket);
                return MPP_NOK;
            }
        }
    }
#endif

    h264e_hal_debug_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_deinit(void *hal)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_hal_debug_enter();

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->ioctl_input);
    MPP_FREE(ctx->ioctl_output);

    if (ctx->buffers) {
        hal_h264e_rkv_free_buffers(ctx);
        MPP_FREE(ctx->buffers);
    }

    if (ctx->extra_info) {
        hal_h264e_rkv_deinit_extra_info(ctx->extra_info);
        MPP_FREE(ctx->extra_info);
    }

    if (ctx->dpb_ctx) {
        hal_h264e_rkv_reference_deinit(ctx->dpb_ctx);
        MPP_FREE(ctx->dpb_ctx);
    }


#ifdef ANDROID
    if (ctx->vpu_socket <= 0) {
        mpp_err("invalid vpu socket: %d", ctx->vpu_socket);
        return MPP_NOK;
    }

    if (VPU_SUCCESS != VPUClientRelease(ctx->vpu_socket)) {
        mpp_err("VPUClientRelease failed");
        return MPP_ERR_VPUHW;
    }
#endif


    h264e_hal_debug_leave();
    return MPP_OK;
}


MPP_RET hal_h264e_rkv_set_ioctl_extra_info(h264e_rkv_ioctl_extra_info *extra_info, h264e_syntax *syn)
{
    h264e_rkv_ioctl_extra_info_elem *info = NULL;
    RK_U32 frame_size = syn->pic_luma_width * syn->pic_luma_height; // TODO: according to yuv format

    extra_info->magic = 0;
    extra_info->cnt = 2;

    /* input cb addr */
    info = &extra_info->elem[0];
    info->reg_idx = 71;
    info->offset  = frame_size;

    /* input cr addr */
    info = &extra_info->elem[1];
    info->reg_idx = 72;
    info->offset  = frame_size * 5 / 4; //TODO: relevant with YUV format

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_gen_regs(void *hal, HalTaskInfo *task)
{
    RK_S32 k = 0;

    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_hal_param *par = &ctx->param;
    h264e_rkv_reg_set *regs = NULL;
    h264e_syntax *syn = (h264e_syntax *)task->enc.syntax.data;
    h264e_rkv_ioctl_input *ioctl_info = (h264e_rkv_ioctl_input *)ctx->ioctl_input;
    h264e_rkv_reg_set *reg_list = (h264e_rkv_reg_set *)ctx->regs;
    h264e_hal_rkv_dpb_ctx *dpb_ctx = (h264e_hal_rkv_dpb_ctx *)ctx->dpb_ctx;
    h264e_hal_rkv_out *extra_info = (h264e_hal_rkv_out *)ctx->extra_info;
    h264e_hal_sps *sps = &extra_info->sps;
    h264e_hal_pps *pps = &extra_info->pps;

    RK_S32 pic_width_align16 = (syn->pic_luma_width + 15) & (~15);
    RK_S32 pic_height_align16 = (syn->pic_luma_height + 15) & (~15);
    RK_S32 pic_width_in_blk64 = (syn->pic_luma_width + 63) / 64;
    h264e_hal_rkv_buffers *bufs = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 mul_buf_idx = ctx->frame_cnt % RKV_H264E_LINKTABLE_FRAME_NUM;
    RK_U32 buf2_idx = ctx->frame_cnt % 2;
    //RK_S32 pic_height_align64 = (syn->pic_luma_height + 63) & (~63);
    ctx->enc_mode = RKV_H264E_ENC_MODE;

    h264e_hal_debug_enter();

    mpp_assert(sps->i_level_idc   == syn->level_idc);
    mpp_assert(sps->i_profile_idc == syn->profile_idc);

    hal_h264e_rkv_adjust_param(ctx); //TODO: future expansion

    if (ctx->frame_cnt == 0) {
        if (MPP_OK != hal_h264e_rkv_allocate_buffers(ctx, syn)) {
            mpp_log("hal_h264e_rkv_allocate_buffers failed, free now");
            hal_h264e_rkv_free_buffers(ctx);
        }
    }

    if (ctx->enc_mode == 2 || ctx->enc_mode == 3) { //link table mode
        RK_U32 idx = ctx->frame_cnt_gen_ready;
        ctx->num_frames_to_send = RKV_H264E_LINKTABLE_EACH_NUM;
        if (idx == 0) {
            ioctl_info->enc_mode = ctx->enc_mode;
            ioctl_info->frame_num = ctx->num_frames_to_send;
        }
        regs = &reg_list[idx];
        ioctl_info->reg_info[idx].reg_num = sizeof(h264e_rkv_reg_set) / 4;
        hal_h264e_rkv_set_ioctl_extra_info(&ioctl_info->reg_info[idx].extra_info, syn);
    } else {
        ctx->num_frames_to_send = 1;
        ioctl_info->frame_num = ctx->num_frames_to_send;
        ioctl_info->enc_mode = ctx->enc_mode;
        regs = &reg_list[0];
        ioctl_info->reg_info[0].reg_num = sizeof(h264e_rkv_reg_set) / 4;
        hal_h264e_rkv_set_ioctl_extra_info(&ioctl_info->reg_info[0].extra_info, syn);
    }

#if (RKV_H264E_NUM_REFS > 1)
    if (MPP_OK != hal_h264e_rkv_reference_frame_set(ctx, syn)) {
        mpp_log("hal_h264e_rkv_reference_frame_set failed, multi-ref error");
    }
#endif

    mpp_log("generate regs when frame_cnt_gen_ready/(num_frames_to_send-1): %d/%d",
            ctx->frame_cnt_gen_ready, ctx->num_frames_to_send - 1);


    if (syn->frame_num != dpb_ctx->i_frame_num)
        mpp_err("syn vs dpb_ctx: syn->frame_num(%d) != dpb_ctx->i_frame_num(%d)", syn->frame_num, dpb_ctx->i_frame_num);
    if (syn->idr_pic_id != (RK_U16)dpb_ctx->i_idr_pic_id)
        mpp_err("syn vs dpb_ctx: syn->idr_pic_id(%d) != dpb_ctx->i_idr_pic_id(%d)", syn->idr_pic_id, dpb_ctx->i_idr_pic_id);
    if (syn->pic_order_cnt_lsb != (dpb_ctx->fdec->i_poc & ((1 << sps->i_log2_max_poc_lsb) - 1)))
        mpp_err("syn vs dpb_ctx: syn->pic_order_cnt_lsb(%d) != (dpb_ctx->fdec->i_poc & ((1 << sps->i_log2_max_poc_lsb) - 1))(%d)",
                syn->pic_order_cnt_lsb, (dpb_ctx->fdec->i_poc & ((1 << sps->i_log2_max_poc_lsb) - 1)));

    memset(regs, 0, sizeof(h264e_rkv_reg_set));

    regs->swreg01.rkvenc_ver      = 0x1;

    regs->swreg02.lkt_num = 0; //syn->link_table_en? h->param.i_frame_total : 0;
    regs->swreg02.rkvenc_cmd = ctx->enc_mode;
    //regs->swreg02.rkvenc_cmd = syn->link_table_en ? 0x2 : 0x1;
    regs->swreg02.enc_cke = 0; //syn->swreg02.enc_cke;

    regs->swreg03.safe_clr = 0x0;

    regs->swreg04.lkt_addr = 0x10000000;//syn->swreg04.lkt_addr;

    regs->swreg05.ofe_fnsh    = 1; //syn->swreg05.ofe_fnsh;
    regs->swreg05.lkt_fnsh    = 0; //syn->swreg05.lkt_fnsh;
    regs->swreg05.clr_fnsh    = 0; //syn->swreg05.clr_fnsh;
    regs->swreg05.ose_fnsh    = 0; //syn->swreg05.ose_fnsh;
    regs->swreg05.bs_ovflr    = 1; //syn->swreg05.bs_ovflr;
    regs->swreg05.brsp_ful    = 1; //syn->swreg05.brsp_ful;
    regs->swreg05.brsp_err    = 1; //syn->swreg05.brsp_err;
    regs->swreg05.rrsp_err    = 1; //syn->swreg05.rrsp_err;
    regs->swreg05.tmt_err     = 1; //syn->swreg05.tmt_err ;

    regs->swreg07.clr_ofe_fnsh    = 0x1;
    regs->swreg07.clr_lkt_fnsh    = 0x1;
    regs->swreg07.clr_clr_fnsh    = 0x1;
    regs->swreg07.clr_ose_fnsh    = 0x1;
    regs->swreg07.clr_bs_ovflr    = 0x1;
    regs->swreg07.clr_brsp_ful    = 0x1;
    regs->swreg07.clr_brsp_err    = 0x1;
    regs->swreg07.clr_rrsp_err    = 0x1;
    regs->swreg07.clr_tmt_err     = 0x1;


    regs->swreg09.pic_wd8_m1    = pic_width_align16 / 8 - 1;
    regs->swreg09.pic_wfill     = (syn->pic_luma_width & 0xf) ? (16 - (syn->pic_luma_width & 0xf)) : 0;
    regs->swreg09.pic_hd8_m1    = pic_height_align16 / 8 - 1;
    regs->swreg09.pic_hfill     = (syn->pic_luma_height & 0xf) ? (16 - (syn->pic_luma_height & 0xf)) : 0;

    regs->swreg10.enc_stnd       = 0; //H264
    regs->swreg10.roi_enc        = 0; //syn->swreg10.roi_enc;
    regs->swreg10.cur_frm_ref    = ((ctx->frame_cnt + 1) % syn->keyframe_max_interval) != 0; //syn->swreg10.cur_frm_ref;       //Current frame should be refered in future
    regs->swreg10.mei_stor       = 0; //syn->swreg10.mei_stor;
    regs->swreg10.bs_scp         = 1; //syn->swreg10.bs_scp;
    regs->swreg10.pic_qp         = syn->qp;//syn->swreg10.pic_qp; //if CQP, pic_qp=qp constant.
    regs->swreg10.slice_int      = 0; //syn->swreg10.slice_int;
    regs->swreg10.node_int       = 0; //syn->swreg10.node_int;//node_int_frame_pos

    regs->swreg11.ppln_enc_lmt     = 2; //syn->swreg11.ppln_enc_lmt;
    regs->swreg11.rfp_load_thrd    = 0; //syn->swreg11.rfp_load_thrd;

    regs->swreg12.src_bus_ordr     = 0x0;
    regs->swreg12.cmvw_bus_ordr    = 0x0;
    regs->swreg12.dspw_bus_ordr    = 0x0;
    regs->swreg12.rfpw_bus_ordr    = 0x0;
    regs->swreg12.src_bus_edin     = 0x0;//syn->swreg12.src_bus_edin;
    regs->swreg12.meiw_bus_edin    = 0x0;
    regs->swreg12.bsw_bus_edin     = 0x7;
    regs->swreg12.lktr_bus_edin    = 0x0;
    regs->swreg12.ctur_bus_edin    = 0x0;
    regs->swreg12.lktw_bus_edin    = 0x0;
    regs->swreg12.rfp_map_dcol     = 0x0;
    regs->swreg12.rfp_map_sctr     = 0x0;

    regs->swreg13.axi_brsp_cke      = 0x7f; //syn->swreg13.axi_brsp_cke;
    regs->swreg13.cime_dspw_orsd    = 0x0;

    regs->swreg14.src_aswap       = 0; //syn->swreg14.src_aswap;
    regs->swreg14.src_cswap       = 0; //syn->swreg14.src_cswap;
    regs->swreg14.src_cfmt        = syn->input_image_format; //syn->swreg14.src_cfmt;          //src_cfmt
    regs->swreg14.src_clip_dis    = 0; //syn->swreg14.src_clip_dis;

    regs->swreg15.wght_b2y    = 0; //syn->swreg15.wght_b2y;
    regs->swreg15.wght_g2y    = 0; //syn->swreg15.wght_g2y;
    regs->swreg15.wght_r2y    = 0; //ssyn->swreg15.wght_r2y;

    regs->swreg16.wght_b2u    = 0; //ssyn->swreg16.wght_b2u;
    regs->swreg16.wght_g2u    = 0; //ssyn->swreg16.wght_g2u;
    regs->swreg16.wght_r2u    = 0; //ssyn->swreg16.wght_r2u;

    regs->swreg17.wght_b2v    = 0; //ssyn->swreg17.wght_b2v;
    regs->swreg17.wght_g2v    = 0; //ssyn->swreg17.wght_g2v;
    regs->swreg17.wght_r2v    = 0; //ssyn->swreg17.wght_r2v;

    regs->swreg18.ofst_rgb2v    = 0; //ssyn->swreg18.ofst_rgb2v;
    regs->swreg18.ofst_rgb2u    = 0; //ssyn->swreg18.ofst_rgb2u;
    regs->swreg18.ofst_rgb2y    = 0; //ssyn->swreg18.ofst_rgb2y;

    regs->swreg19.src_tfltr         = 0; //syn->swreg19.src_tfltr;
    regs->swreg19.src_tfltr_we      = 0; //syn->swreg19.src_tfltr_we;
    regs->swreg19.src_tfltr_bw      = 0; //syn->swreg19.src_tfltr_bw;
    regs->swreg19.src_sfltr         = 0; //syn->swreg19.src_sfltr;
    regs->swreg19.src_mfltr_thrd    = 0; //syn->swreg19.src_mfltr_thrd;
    regs->swreg19.src_mfltr_y       = 0; //syn->swreg19.src_mfltr_y;
    regs->swreg19.src_mfltr_c       = 0; //syn->swreg19.src_mfltr_c;
    regs->swreg19.src_bfltr_strg    = 0; //syn->swreg19.src_bfltr_strg;
    regs->swreg19.src_bfltr         = 0; //syn->swreg19.src_bfltr;
    regs->swreg19.src_mbflt_odr     = 0; //syn->swreg19.src_mbflt_odr;
    regs->swreg19.src_matf_y        = 0; //syn->swreg19.src_matf_y;
    regs->swreg19.src_matf_c        = 0; //syn->swreg19.src_matf_c;
    regs->swreg19.src_shp_y         = 0; //syn->swreg19.src_shp_y;
    regs->swreg19.src_shp_c         = 0; //syn->swreg19.src_shp_c;
    regs->swreg19.src_shp_div       = 0; //syn->swreg19.src_shp_div;
    regs->swreg19.src_shp_thld      = 0; //syn->swreg19.src_shp_thld;
    regs->swreg19.src_mirr          = 0; //syn->swreg19.src_mirr;
    regs->swreg19.src_rot           = 0; //syn->swreg19.src_rot;
    regs->swreg19.src_matf_itsy     = 0; //syn->swreg19.src_matf_itsy;

    regs->swreg20.tfltr_thld_y    = 0; //syn->swreg20.tfltr_thld_y;
    regs->swreg20.tfltr_thld_c    = 0; //syn->swreg20.tfltr_thld_c;

    for (k = 0; k < 5; k++)
        regs->swreg21_scr_stbl[k] = 0; //syn->swreg21_scr_stbl[k];

    for (k = 0; k < 40; k++)
        regs->swreg22_h3d_tbl[k]  = h264e_h3d_tbl[k];

    {
        RK_U32 stridey = 0, stridec = 0;
        stridey = (regs->swreg19.src_rot == 1 || regs->swreg19.src_rot == 3) ? (syn->pic_luma_height - 1) : (syn->pic_luma_width - 1);
        if (regs->swreg14.src_cfmt == 0 )
            stridey = (stridey + 1) * 4 - 1;
        else if (regs->swreg14.src_cfmt == 1 )
            stridey = (stridey + 1) * 3 - 1;
        else if ( regs->swreg14.src_cfmt == 2 || regs->swreg14.src_cfmt == 8 || regs->swreg14.src_cfmt == 9 )
            stridey = (stridey + 1) * 2 - 1;
        stridec = (regs->swreg14.src_cfmt == 4 || regs->swreg14.src_cfmt == 6) ? stridey : ((stridey + 1) / 2 - 1);
        regs->swreg23.src_ystrid    = stridey; //syn->swreg23.src_ystrid;
        regs->swreg23.src_cstrid    = stridec; //syn->swreg23.src_cstrid;    ////YUV420 planar;
    }

    regs->swreg24_adr_srcy     = syn->input_luma_addr; //syn->addr_cfg.adr_srcy;
    regs->swreg25_adr_srcu     = syn->input_cb_addr; //syn->addr_cfg.adr_srcu;
    regs->swreg26_adr_srcv     = syn->input_cr_addr; //syn->addr_cfg.adr_srcv;
    regs->swreg27_fltw_addr    = mpp_buffer_get_fd(bufs->hw_pp_buf[buf2_idx]);
    regs->swreg28_fltr_addr    = mpp_buffer_get_fd(bufs->hw_pp_buf[1 - buf2_idx]);
    regs->swreg29_ctuc_addr    = mpp_buffer_get_fd(bufs->hw_roi_buf[mul_buf_idx]); //syn->addr_cfg.ctuc_addr;
    regs->swreg30_rfpw_addr    = mpp_buffer_get_fd(dpb_ctx->fdec->hw_buf);//syn->addr_cfg.rfpw_addr; //TODO: extend recon luma buf
    if (dpb_ctx->fref[0][0])
        regs->swreg31_rfpr_addr    = mpp_buffer_get_fd(dpb_ctx->fref[0][0]->hw_buf); //syn->addr_cfg.rfpr_addr;
    regs->swreg32_cmvw_addr    = mpp_buffer_get_fd(bufs->hw_cmv_buf[buf2_idx]);
    regs->swreg33_cmvr_addr    = mpp_buffer_get_fd(bufs->hw_cmv_buf[1 - buf2_idx]);
    regs->swreg34_dspw_addr    = mpp_buffer_get_fd(bufs->hw_dsp_buf[buf2_idx]); //syn->addr_cfg.dspw_addr;
    regs->swreg35_dspr_addr    = mpp_buffer_get_fd(bufs->hw_dsp_buf[1 - buf2_idx]); //syn->addr_cfg.dspr_addr;
    regs->swreg36_meiw_addr    = mpp_buffer_get_fd(bufs->hw_mei_buf[mul_buf_idx]);
    regs->swreg37_bsbt_addr    = syn->output_strm_addr;
    if (VPUClientGetIOMMUStatus() > 0)
        regs->swreg38_bsbb_addr = regs->swreg37_bsbt_addr | (pic_width_align16 * pic_height_align16 * 3) << 10; // TODO: stream size relative with syntax
    else
        regs->swreg38_bsbb_addr = regs->swreg37_bsbt_addr + (pic_width_align16 * pic_height_align16 * 3);
    regs->swreg39_bsbr_addr    = regs->swreg38_bsbb_addr;
    regs->swreg40_bsbw_addr    = regs->swreg37_bsbt_addr; //syn->addr_cfg.bsbw_addr;

    //mpp_log("regs->swreg37_bsbt_addr: %08x", regs->swreg37_bsbt_addr);
    //mpp_log("regs->swreg38_bsbb_addr: %08x", regs->swreg38_bsbb_addr);
    //mpp_log("regs->swreg39_bsbr_addr: %08x", regs->swreg39_bsbr_addr);
    //mpp_log("regs->swreg40_bsbw_addr: %08x", regs->swreg40_bsbw_addr);

    regs->swreg41.sli_cut         = 0; //syn->swreg41.sli_cut;
    regs->swreg41.sli_cut_mode    = 0; //syn->swreg41.sli_cut_mode;
    regs->swreg41.sli_cut_bmod    = 1; //syn->swreg41.sli_cut_bmod;
    regs->swreg41.sli_max_num     = 0; //syn->swreg41.sli_max_num;
    regs->swreg41.sli_out_mode    = 0; //syn->swreg41.sli_out_mode;
    regs->swreg41.sli_cut_cnum    = 0; //syn->swreg41.sli_cut_cnum;

    regs->swreg42.sli_cut_byte    = 0; //syn->swreg42.sli_cut_byte;

    {
        RK_U32 cime_wid_4p = 0, cime_hei_4p = 0;
        if ( sps->i_level_idc == 10 || sps->i_level_idc == 9 ) { //9 is level 1b;
            cime_wid_4p = 44;
            cime_hei_4p = 12;
        } else if ( sps->i_level_idc == 11 || sps->i_level_idc == 12 || sps->i_level_idc == 13 || sps->i_level_idc == 20 ) {
            cime_wid_4p = 44;
            cime_hei_4p = 28;
        } else {
            cime_wid_4p = 44;
            cime_hei_4p = 28;
        }

        if (176 < cime_wid_4p * 4)
            cime_wid_4p = 176 / 4;

        if (112 < cime_hei_4p * 4)
            cime_hei_4p = 112 / 4;

        if (cime_hei_4p / 4 * 2 > (RK_U32)(regs->swreg09.pic_hd8_m1 + 2) / 2)
            cime_hei_4p = (regs->swreg09.pic_hd8_m1 + 2) / 2 / 2 * 4;

        if (cime_wid_4p / 4 > (RK_U32)(((regs->swreg09.pic_wd8_m1 + 1) * 8 + 63) / 64 * 64 / 128 * 4))
            cime_wid_4p = ((regs->swreg09.pic_wd8_m1 + 1) * 8 + 63) / 64 * 64 / 128 * 4 * 4;

        regs->swreg43.cime_srch_h    = cime_wid_4p / 4; //syn->swreg43.cime_srch_h;
        regs->swreg43.cime_srch_v    = cime_hei_4p / 4; //syn->swreg43.cime_srch_v;
    }

    regs->swreg43.rime_srch_h    = 7; //syn->swreg43.rime_srch_h;
    regs->swreg43.rime_srch_v    = 5; //syn->swreg43.rime_srch_v;
    regs->swreg43.dlt_frm_num    = 0x0;

    regs->swreg44.pmv_mdst_h    = 5; //syn->swreg44.pmv_mdst_h;
    regs->swreg44.pmv_mdst_v    = 5; //syn->swreg44.pmv_mdst_v;
    regs->swreg44.mv_limit      = (sps->i_level_idc > 20) ? 2 : ((sps->i_level_idc >= 11) ? 1 : 0); //syn->swreg44.mv_limit;
    regs->swreg44.mv_num        = 3; //syn->swreg44.mv_num;


    if (pic_width_align16 > 3584)
        regs->swreg45.cime_rama_h = 8;
    else if (pic_width_align16 > 3136)
        regs->swreg45.cime_rama_h = 9;
    else if (pic_width_align16 > 2816)
        regs->swreg45.cime_rama_h = 10;
    else if (pic_width_align16 > 2560)
        regs->swreg45.cime_rama_h = 11;
    else if (pic_width_align16 > 2368)
        regs->swreg45.cime_rama_h = 12;
    else if (pic_width_align16 > 2176)
        regs->swreg45.cime_rama_h = 13;
    else if (pic_width_align16 > 2048)
        regs->swreg45.cime_rama_h = 14;
    else if (pic_width_align16 > 1856)
        regs->swreg45.cime_rama_h = 15;
    else if (pic_width_align16 > 1792)
        regs->swreg45.cime_rama_h = 16;
    else
        regs->swreg45.cime_rama_h = 17;

    {
        RK_U32 i_swin_all_4_h = (2 * regs->swreg43.cime_srch_v + 1);
        RK_U32 i_swin_all_16_w = ((regs->swreg43.cime_srch_h * 4 + 15) / 16 * 2 + 1);
        if (i_swin_all_4_h < regs->swreg45.cime_rama_h)
            regs->swreg45.cime_rama_max = (i_swin_all_4_h - 1) * pic_width_in_blk64 + i_swin_all_16_w;
        else
            regs->swreg45.cime_rama_max = (regs->swreg45.cime_rama_h - 1) * pic_width_in_blk64 + i_swin_all_16_w;
    }

    regs->swreg45.cach_l1_dtmr     = 0x3; //syn->swreg45.cach_l1_dtmr;
    regs->swreg45.cach_l2_tag      = 0x0;
    if (pic_width_align16 <= 512)
        regs->swreg45.cach_l2_tag  = 0x0;
    else if (pic_width_align16 <= 1024)
        regs->swreg45.cach_l2_tag  = 0x1;
    else if (pic_width_align16 <= 2048)
        regs->swreg45.cach_l2_tag  = 0x2;
    else if (pic_width_align16 <= 4096)
        regs->swreg45.cach_l2_tag  = 0x3;

    regs->swreg46.rc_en         = 0; //syn->swreg46.rc_en;
    regs->swreg46.rc_mode       = 0; //syn->swreg46.rc_mode;
    regs->swreg46.aqmode_en     = 0; //syn->swreg46.aqmode_en;
    regs->swreg46.aq_strg       = 0; //syn->swreg46.aq_strg;
    regs->swreg46.rc_ctu_num    = pic_width_align16 / 16 * 3; //syn->swreg46.rc_ctu_num;

    regs->swreg47.bits_error0    = 0; //syn->swreg47.bits_error0;
    regs->swreg47.bits_error1    = 0; //syn->swreg47.bits_error1;
    regs->swreg48.bits_error2    = 0; //syn->swreg48.bits_error2;
    regs->swreg48.bits_error3    = 0; //syn->swreg48.bits_error3;
    regs->swreg49.bits_error4    = 0; //syn->swreg49.bits_error4;
    regs->swreg49.bits_error5    = 0; //syn->swreg49.bits_error5;
    regs->swreg50.bits_error6    = 0; //syn->swreg50.bits_error6;
    regs->swreg50.bits_error7    = 0; //syn->swreg50.bits_error7;
    regs->swreg51.bits_error8    = 0; //syn->swreg51.bits_error8;

    regs->swreg52.qp_adjuest0    = 0; //syn->swreg52.qp_adjuest0;
    regs->swreg52.qp_adjuest1    = 0; //syn->swreg52.qp_adjuest1;
    regs->swreg52.qp_adjuest2    = 0; //syn->swreg52.qp_adjuest2;
    regs->swreg52.qp_adjuest3    = 0; //syn->swreg52.qp_adjuest3;
    regs->swreg52.qp_adjuest4    = 0; //syn->swreg52.qp_adjuest4;
    regs->swreg52.qp_adjuest5    = 0; //syn->swreg52.qp_adjuest5;
    regs->swreg53.qp_adjuest6    = 0; //syn->swreg53.qp_adjuest6;
    regs->swreg53.qp_adjuest7    = 0; //syn->swreg53.qp_adjuest7;
    regs->swreg53.qp_adjuest8    = 0; //syn->swreg53.qp_adjuest8;

    regs->swreg54.rc_qp_mod      = 3; //syn->swreg54.rc_qp_mod;
    regs->swreg54.rc_fact0       = 16; //syn->swreg54.rc_fact0;
    regs->swreg54.rc_fact1       = 0; //syn->swreg54.rc_fact1;
    regs->swreg54.rc_qp_range    = 4; //yn->swreg54.rc_qp_range;
    regs->swreg54.rc_max_qp      = 51; //syn->swreg54.rc_max_qp;
    regs->swreg54.rc_min_qp      = 1; //syn->swreg54.rc_min_qp;

    regs->swreg55.ctu_ebits    = 0; //syn->swreg55.ctu_ebits;

    regs->swreg56.rect_size         = (sps->i_profile_idc == H264_PROFILE_BASELINE && sps->i_level_idc <= 30);
    regs->swreg56.inter_4x4         = !syn->h264_inter4x4_disabled; //NOTE!
    regs->swreg56.arb_sel           = 0; //syn->swreg56.arb_sel;
    regs->swreg56.vlc_lmt           = (sps->i_profile_idc < H264_PROFILE_HIGH && !syn->enable_cabac);
    regs->swreg56.rdo_mark          = 0; //syn->swreg56.rdo_mark;
    /*if (syn->transform8x8_mode == 0 && (syn->swreg56.rdo_mark & 0xb5) == 0xb5) //NOTE: bug may exist here
    {
        mpp_err("RdoMark and trans8x8 conflict!");
        mpp_assert(0);
        return MPP_NOK;
    }*/

    {
        RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;
        if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_IDR ) { //TODO: extend syn->frame_coding_type definition
            /* reset ref pictures */
            i_nal_type    = RKVENC_NAL_SLICE_IDR;
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGHEST;
        } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_I ) {
            i_nal_type    = RKVENC_NAL_SLICE;
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_P ) {
            i_nal_type    = RKVENC_NAL_SLICE;
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if ( syn->frame_coding_type == RKVENC_FRAME_TYPE_BREF ) {
            i_nal_type    = RKVENC_NAL_SLICE;
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_HIGH;
        } else { /* B frame */

            i_nal_type    = RKVENC_NAL_SLICE;
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_DISPOSABLE;
        }
        if (syn->keyframe_max_interval == 1)
            i_nal_ref_idc = RKVENC_NAL_PRIORITY_LOW;

        regs->swreg57.nal_ref_idc      = i_nal_ref_idc; //syn->swreg57.nal_ref_idc;
        regs->swreg57.nal_unit_type    = i_nal_type; //syn->swreg57.nal_unit_type;
    }


    regs->swreg58.max_fnum    = sps->i_log2_max_frame_num - 4; //syn->swreg58.max_fnum;
    regs->swreg58.drct_8x8    = 1;      //syn->swreg58.drct_8x8;
    regs->swreg58.mpoc_lm4    = sps->i_log2_max_poc_lsb - 4; //syn->swreg58.mpoc_lm4;

    regs->swreg59.etpy_mode       = syn->enable_cabac;
    regs->swreg59.trns_8x8        = 1; //syn->swreg59.trns_8x8;
    regs->swreg59.csip_flg        = par->constrained_intra; //syn->swreg59.csip_flg;
    regs->swreg59.num_ref0_idx    = pps->i_num_ref_idx_l0_default_active - 1; //syn->swreg59.num_ref0_idx;
    regs->swreg59.num_ref1_idx    = pps->i_num_ref_idx_l1_default_active - 1; //syn->swreg59.num_ref1_idx;
    regs->swreg59.pic_init_qp     = syn->pic_init_qp - H264_QP_BD_OFFSET;
    regs->swreg59.cb_ofst         = 0; //syn->chroma_qp_index_offset;
    regs->swreg59.cr_ofst         = 0; //syn->second_chroma_qp_index_offset;
    regs->swreg59.wght_pred       = 0x0;
    regs->swreg59.dbf_cp_flg      = 1; //syn->deblocking_filter_control;

    regs->swreg60.sli_type        = syn->slice_type; //syn->swreg60.sli_type;
    regs->swreg60.pps_id          = syn->pps_id;
    regs->swreg60.drct_smvp       = 0x0;
    regs->swreg60.num_ref_ovrd    = 0;
    regs->swreg60.cbc_init_idc    = syn->cabac_init_idc;

    regs->swreg60.frm_num         = dpb_ctx->i_frame_num;

    regs->swreg61.idr_pid    = dpb_ctx->i_idr_pic_id;
    regs->swreg61.poc_lsb    = dpb_ctx->fdec->i_poc & ((1 << sps->i_log2_max_poc_lsb) - 1);

    regs->swreg62.rodr_pic_idx      = dpb_ctx->ref_pic_list_order[0][0].idc; //syn->swreg62.rodr_pic_idx;
    regs->swreg62.ref_list0_rodr    = dpb_ctx->b_ref_pic_list_reordering[0]; //syn->swreg62.ref_list0_rodr;
    regs->swreg62.sli_beta_ofst     = 0; //syn->slice_beta_offset;
    regs->swreg62.sli_alph_ofst     = 0; //syn->slice_alpha_offset;
    regs->swreg62.dis_dblk_idc      = 0; //syn->disable_deblocking_filter_idc;
    regs->swreg62.rodr_pic_num      = dpb_ctx->ref_pic_list_order[0][0].arg; //syn->swreg62.rodr_pic_num;

    {
        RK_S32 mmco4_pre = dpb_ctx->i_mmco_command_count > 0 && (dpb_ctx->mmco[0].memory_management_control_operation == 4);
        regs->swreg63.nopp_flg     = 0x0;
        regs->swreg63.ltrf_flg     = dpb_ctx->i_long_term_reference_flag; //syn->swreg63.ltrf_flg;
        regs->swreg63.arpm_flg     = dpb_ctx->i_mmco_command_count; //syn->swreg63.arpm_flg;
        regs->swreg63.mmco4_pre    = mmco4_pre;
        regs->swreg63.mmco_0       = dpb_ctx->i_mmco_command_count > mmco4_pre ? dpb_ctx->mmco[mmco4_pre].memory_management_control_operation : 0;
        regs->swreg63.dopn_m1_0    = dpb_ctx->i_mmco_command_count > mmco4_pre ? dpb_ctx->mmco[mmco4_pre].i_difference_of_pic_nums - 1 : 0;; //syn->swreg63.dopn_m1_0;

        regs->swreg64.mmco_1       = dpb_ctx->i_mmco_command_count > (mmco4_pre + 1) ? dpb_ctx->mmco[(mmco4_pre + 1)].memory_management_control_operation : 0; //syn->swreg64.mmco_1;
        regs->swreg64.dopn_m1_1    = dpb_ctx->i_mmco_command_count > (mmco4_pre + 1) ? dpb_ctx->mmco[(mmco4_pre + 1)].i_difference_of_pic_nums - 1 : 0; //syn->swreg64.dopn_m1_1;
    }


    regs->swreg65.osd_en         = 0; //syn->swreg65.osd_en;
    regs->swreg65.osd_inv        = 0; //syn->swreg65.osd_inv;
    regs->swreg65.osd_clk_sel    = 1;
    regs->swreg65.osd_plt_type   = 0; //syn->swreg65.osd_plt_type;

    regs->swreg66.osd_inv_r0    = 0; //syn->swreg66.osd_inv_r0;
    regs->swreg66.osd_inv_r1    = 0; //syn->swreg66.osd_inv_r1;
    regs->swreg66.osd_inv_r2    = 0; //syn->swreg66.osd_inv_r2;
    regs->swreg66.osd_inv_r3    = 0; //syn->swreg66.osd_inv_r3;
    regs->swreg66.osd_inv_r4    = 0; //syn->swreg66.osd_inv_r4;
    regs->swreg66.osd_inv_r5    = 0; //syn->swreg66.osd_inv_r5;
    regs->swreg66.osd_inv_r6    = 0; //syn->swreg66.osd_inv_r6;
    regs->swreg66.osd_inv_r7    = 0; //syn->swreg66.osd_inv_r7;

    for (k = 0; k < 8; k++) {
        regs->swreg67_osd_pos[k].lt_pos_x = 0; //syn->swreg67_osd_pos[k].lt_pos_x;
        regs->swreg67_osd_pos[k].lt_pos_y = 0; //syn->swreg67_osd_pos[k].lt_pos_y;
        regs->swreg67_osd_pos[k].rd_pos_x = 0; //syn->swreg67_osd_pos[k].rd_pos_x;
        regs->swreg67_osd_pos[k].rd_pos_y = 0; //syn->swreg67_osd_pos[k].rd_pos_y;
    }

    for (k = 0; k < 8; k++)
        regs->swreg68_indx_addr_i[k]       = 0; //syn->swreg68_indx_addr_i[k];

    regs->swreg69.bs_lgth    = 0x0;

    regs->swreg70.sse_l32    = 0x0;

    regs->swreg71.qp_sum     = 0x0;
    regs->swreg71.sse_h8     = 0x0;

    regs->swreg72.slice_scnum    = 0x0;
    regs->swreg72.slice_slnum    = 0x0;

    regs->swreg73.st_enc      = 0x0;
    regs->swreg73.axiw_cln    = 0x0;
    regs->swreg73.axir_cln    = 0x0;

    regs->swreg74.fnum_enc    = 0x0;
    regs->swreg74.fnum_cfg    = 0x0;
    regs->swreg74.fnum_int    = 0x0;

    regs->swreg75.node_addr    = 0x0;

    regs->swreg76.bsbw_addr    = 0x0; //syn->swreg77.bsbw_addr; read only
    regs->swreg76.Bsbw_ovfl    = 0x0;

#if !RKV_H264E_REMOVE_UNNECESSARY_REGS
    regs->swreg77.axib_idl    = 0x0;
    regs->swreg77.axib_ful    = 0x0;
    regs->swreg77.axib_err    = 0x0;
    regs->swreg77.axir_err    = 0x0;

    regs->swreg78.slice_num    = 0x0;

    regs->swreg79.slice_len    = 0x0;

    for (k = 0; k < 256; k++)
        regs->swreg80_osd_indx_tab_i[k]    = 0; //syn->swreg73_osd_indx_tab_i[k];

    regs->swreg81.axip0_work    = 0x0;
    regs->swreg81.axip0_clr     = 0x0;

    regs->swreg82.axip0_ltcy_id     = 0x0;
    regs->swreg82.axip0_ltcy_thr    = 0x0;

    regs->swreg83.axip0_cnt_type    = 0x0;
    regs->swreg83.axip0_cnt_ddr     = 0x0;
    regs->swreg83.axip0_cnt_rid     = 0x0;
    regs->swreg83.axip0_cnt_wid     = 0x0;

    regs->swreg84.axip1_work    = 0x0;
    regs->swreg84.axip1_clr     = 0x0;

    regs->swreg85.axip1_ltcy_id     = 0x0;
    regs->swreg85.axip1_ltcy_thr    = 0x0;

    regs->swreg86.axip1_cnt_type    = 0x0;
    regs->swreg86.axip1_cnt_ddr    = 0x0;
    regs->swreg86.axip1_cnt_rid    = 0x0;
    regs->swreg86.axip1_cnt_wid    = 0x0;

    regs->swreg87.axip0_cnt_type    = 0x0;

    regs->swreg88.axip0_num_ltcy    = 0x0;

    regs->swreg89.axip0_sum_ltcy    = 0x0;

    regs->swreg90.axip0_byte_rd    = 0x0;

    regs->swreg91.axip0_byte_wr    = 0x0;

    regs->swreg92.axip0_wrk_cyc    = 0x0;

    regs->swreg93.axip1_cnt_type    = 0x0;

    regs->swreg94.axip1_num_ltcy    = 0x0;

    regs->swreg95.axip1_sum_ltcy    = 0x0;

    regs->swreg96.axip1_byte_rd    = 0x0;

    regs->swreg97.axip1_byte_wr    = 0x0;

    regs->swreg98.axip1_wrk_cyc    = 0x0;
#endif //#if !RKV_H264E_REMOVE_UNNECESSARY_REGS

#if (RKV_H264E_NUM_REFS > 1)
    hal_h264e_rkv_reference_frame_update(ctx);
#endif
    dpb_ctx->i_frame_cnt++;
    if (dpb_ctx->i_nal_ref_idc != RKVENC_NAL_PRIORITY_DISPOSABLE)
        dpb_ctx->i_frame_num ++;

    ctx->frame_cnt_gen_ready++;
    ctx->frame_cnt++;

    h264e_hal_debug_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_rkv_reg_set *reg_list = (h264e_rkv_reg_set *)ctx->regs;
    RK_U32 length = 0, k = 0;
    h264e_rkv_ioctl_input *ioctl_info = (h264e_rkv_ioctl_input *)ctx->ioctl_input;

    h264e_hal_debug_enter();
    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        mpp_log("frame_cnt_gen_ready(%d) != num_frames_to_send(%d), start hardware later",
                ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    }

    mpp_log("memcpy %d frames' regs from reg list to reg info", ioctl_info->frame_num);
    for (k = 0; k < ioctl_info->frame_num; k++)
        memcpy(&ioctl_info->reg_info[k].regs, &reg_list[k], sizeof(h264e_rkv_reg_set));

    length = (sizeof(ioctl_info->enc_mode) + sizeof(ioctl_info->frame_num) +
              sizeof(ioctl_info->reg_info[0]) * ioctl_info->frame_num) >> 2;

    ctx->frame_cnt_send_ready ++;

    (void)task;

#ifdef ANDROID
    if (ctx->vpu_socket <= 0) {
        mpp_err("invalid vpu socket: %d", ctx->vpu_socket);
        return MPP_NOK;
    }

    mpp_log("vpu client is sending %d regs", length);
    if (MPP_OK != VPUClientSendReg(ctx->vpu_socket, (RK_U32 *)ioctl_info, length)) {
        mpp_err("VPUClientSendReg Failed!!!");
        return  MPP_ERR_VPUHW;
    } else {
        mpp_log("VPUClientSendReg successfully!");
    }
#else
    (void)length;
#endif

    h264e_hal_debug_leave();

    return ret;
}

static MPP_RET hal_h264e_rkv_set_feedback(h264e_feedback *fb, h264e_rkv_ioctl_output *out)
{
    (void)fb;
    (void)out;

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_wait(void *hal, HalTaskInfo *task)
{
    VPU_CMD_TYPE cmd = 0;
    RK_S32 hw_ret = 0;
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_rkv_ioctl_output *reg_out = (h264e_rkv_ioctl_output *)ctx->ioctl_output;
    RK_S32 length = (sizeof(reg_out->frame_num) + sizeof(reg_out->elem[0]) * ctx->num_frames_to_send) >> 2;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    (void)task;
    h264e_hal_debug_enter();

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        mpp_log("frame_cnt_gen_ready(%d) != num_frames_to_send(%d), wait hardware later",
                ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    } else {
        ctx->frame_cnt_gen_ready = 0;
        if (ctx->enc_mode == 3) {
            mpp_log("only for test enc_mode 3 ..."); //TODO: remove later
            if (ctx->frame_cnt_send_ready != RKV_H264E_LINKTABLE_FRAME_NUM) {
                mpp_log("frame_cnt_send_ready(%d) != RKV_H264E_LINKTABLE_FRAME_NUM(%d), wait hardware later",
                        ctx->frame_cnt_send_ready, RKV_H264E_LINKTABLE_FRAME_NUM);
                return MPP_OK;
            } else {
                ctx->frame_cnt_send_ready = 0;
            }
        }
    }

#ifdef ANDROID
    if (ctx->vpu_socket <= 0) {
        mpp_err("invalid vpu socket: %d", ctx->vpu_socket);
        return MPP_NOK;
    }

    mpp_log("VPUClientWaitResult expect length %d\n", length);

    hw_ret = VPUClientWaitResult(ctx->vpu_socket, (RK_U32 *)reg_out,
                                 length, &cmd, NULL);

    mpp_log("VPUClientWaitResult: ret %d, cmd %d, len %d\n", hw_ret, cmd, length);


    if ((VPU_SUCCESS != hw_ret) || (cmd != VPU_SEND_CONFIG_ACK_OK))
        mpp_log("hardware wait error");

    if (hw_ret != MPP_OK) {
        mpp_log("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }
#else
    (void)hw_ret;
    (void)length;
    (void)cmd;
#endif

    if (int_cb.callBack) {
        hal_h264e_rkv_set_feedback(fb, reg_out);
        int_cb.callBack(int_cb.opaque, fb);
    }

    h264e_hal_debug_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_reset(void *hal)
{
    (void)hal;
    h264e_hal_debug_enter();

    h264e_hal_debug_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_flush(void *hal)
{
    (void)hal;
    h264e_hal_debug_enter();

    h264e_hal_debug_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_control(void *hal, RK_S32 cmd_type, void *param)
{
    h264e_hal_context *ctx = (h264e_hal_context *)hal;
    h264e_hal_debug_enter();

    mpp_log("hal_h264e_rkv_control cmd 0x%x, info %p", cmd_type, param);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
        hal_h264e_rkv_set_extra_info(ctx, param);
        break;
    }
    case MPP_ENC_GET_EXTRA_INFO: {
        hal_h264e_rkv_get_extra_info(param, ctx->extra_info);
        break;
    }
    default : {
        mpp_err("unrecognizable cmd type %d", cmd_type);
    } break;
    }

    h264e_hal_debug_leave();
    return MPP_OK;
}

