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

#define MODULE_TAG  "h264d_sei"

#include <stdio.h>
#include <string.h>

#include "h264d_global.h"
#include "h264d_sps.h"
#include "h264d_sei.h"

#ifndef _WIN32
#include <ctype.h>

static char *_strupr(char *str)
{
    char *orign = str;
    for (; *str != '\0'; str++) {
        *str = toupper(*str);
    }
    return orign;
}
#endif

static void interpret_spare_pic()
{

}
static void interpret_subsequence_info()
{

}
static void interpret_subsequence_layer_characteristics_info()
{

}
static void interpret_subsequence_characteristics_info()
{

}
static void interpret_scene_information()
{

}
static void interpret_user_data_registered_itu_t_t35_info()
{

}

static MPP_RET interpret_user_data_unregistered_info(RK_U8 *payload, RK_S32 size, H264_SEI_t *sei_msg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    H264D_DBG(H264D_DBG_SEI, "[SEI_info] user data info, size=%d", size);
    ASSERT(size >= 16);

    sei_msg->user_data_DivX_flag = strstr(_strupr((char *)&payload[16]), "DIVX") ? 1 : 0;
    if (sei_msg->user_data_DivX_flag) {
        H264D_ERR("DivX is not supported. \n");
        sei_msg->p_Dec->errctx.un_spt_flag = 1;
        ret = MPP_NOK;
        goto __FAILED;
    }

    return ret = MPP_OK;
__FAILED:
    return ret;
}

static void interpret_pan_scan_rect_info()
{

}
static void interpret_filler_payload_info()
{

}
static void interpret_dec_ref_pic_marking_repetition_info()
{

}
static void interpret_full_frame_freeze_info()
{

}
static void interpret_full_frame_freeze_release_info()
{

}
static void interpret_full_frame_snapshot_info()
{

}
static void interpret_progressive_refinement_start_info()
{

}
static void interpret_progressive_refinement_end_info()
{

}
static void interpret_motion_constrained_slice_group_set_info()
{

}
static void interpret_picture_timing_info()
{

}
static void interpret_film_grain_characteristics_info()
{

}
static void interpret_deblocking_filter_display_preference_info()
{

}
static void interpret_stereo_video_info_info()
{

}
static void interpret_post_filter_hints_info()
{

}
static void interpret_tone_mapping()
{

}
static void interpret_frame_packing_arrangement_info()
{

}
static void interpret_mvc_scalability_info()
{

}

static MPP_RET interpret_recovery_point_info(RK_U8 *payload, RK_S32 size, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    mpp_set_bitread_ctx(p_bitctx, payload, size);
    mpp_set_pre_detection(p_bitctx);

    READ_UE(p_bitctx, &sei_msg->recovery_point.recovery_frame_cnt);
    READ_ONEBIT(p_bitctx, &sei_msg->recovery_point.exact_match_flag);
    READ_ONEBIT(p_bitctx, &sei_msg->recovery_point.broken_link_flag);
    READ_BITS(p_bitctx, 2, &sei_msg->recovery_point.changing_slice_group_idc);

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;
}


static MPP_RET interpret_mvc_scalable_nesting_info(RK_U8 *payload, RK_S32 size, BitReadCtx_t *p_bitctx)
{
    RK_S32 i;
    RK_U32 operation_point_flag;
    RK_U32 all_view_components_in_au_flag;
    RK_S32 num_view_components_minus1;
    RK_S32 sei_view_id;
    RK_S32 num_view_components_op_minus1;
    RK_S32 sei_op_view_id;
    RK_S32 sei_op_temporal_id;
    MPP_RET ret = MPP_ERR_UNKNOW;

    BitReadCtx_t tmp_strmdata = { 0 };
    BitReadCtx_t *p_strm = &tmp_strmdata;
    mpp_set_bitread_ctx(p_strm, payload, size);
    mpp_set_pre_detection(p_bitctx);

    READ_ONEBIT(p_strm, &operation_point_flag);
    if (!operation_point_flag) {
        READ_ONEBIT(p_strm, &all_view_components_in_au_flag);
        READ_UE(p_strm, &num_view_components_minus1);
        for (i = 0; i <= num_view_components_minus1; i++) {
            READ_BITS(p_strm, 10, &sei_view_id);
        }
    } else {
        READ_UE(p_strm, &num_view_components_op_minus1);
        for (i = 0; i <= num_view_components_op_minus1; i++) {
            READ_BITS(p_strm, 10, &sei_op_view_id);
        }
        READ_BITS(p_strm, 3, &sei_op_temporal_id);
    }

    p_bitctx->used_bits = p_strm->used_bits;
    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;
}

static MPP_RET interpret_buffering_period_info(RK_U8 *payload, RK_S32 size, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    BitReadCtx_t tmp_strmdata = { 0 };

    p_bitctx = &tmp_strmdata;
    mpp_set_bitread_ctx(p_bitctx, payload, size);
    mpp_set_pre_detection(p_bitctx);
    READ_UE(p_bitctx, &sei_msg->seq_parameter_set_id);
    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;

}

static void interpret_reserved_info(RK_U8 *payload, RK_S32 size, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg)
{
    RK_S32 offset = 0;
    RK_U8 payload_byte = 0;

    while (offset < size) {
        payload_byte = payload[offset];
        offset++;
    }
    (void)p_bitctx;
    (void)sei_msg;
    (void)payload_byte;
}

static MPP_RET parserSEI(H264_SLICE_t *cur_slice, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg, RK_U8 *msg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t  *p_Vid = cur_slice->p_Vid;

    H264D_DBG(H264D_DBG_SEI, "[SEI_info] type=%d \n", sei_msg->type);
    switch (sei_msg->type) {
    case  SEI_BUFFERING_PERIOD:
        FUN_CHECK(ret = interpret_buffering_period_info(msg, sei_msg->payload_size, p_bitctx, sei_msg));
        {
            H264_SPS_t *sps = NULL;
            H264_subSPS_t *subset_sps = NULL;
            if (sei_msg->mvc_scalable_nesting_flag) {
                p_Vid->active_mvc_sps_flag = 1;
                sps = NULL;
                subset_sps = &p_Vid->subspsSet[sei_msg->seq_parameter_set_id];
            } else {
                p_Vid->active_mvc_sps_flag = 0;
                sps = &p_Vid->spsSet[sei_msg->seq_parameter_set_id];
                subset_sps = NULL;
            }
            p_Vid->exit_picture_flag = 1;
            FUN_CHECK(ret = activate_sps(p_Vid, sps, subset_sps));
        }
        break;
    case  SEI_PIC_TIMING:
        interpret_picture_timing_info();
        break;
    case  SEI_PAN_SCAN_RECT:
        interpret_pan_scan_rect_info();
        break;
    case  SEI_FILLER_PAYLOAD:
        interpret_filler_payload_info();
        break;
    case  SEI_USER_DATA_REGISTERED_ITU_T_T35:
        interpret_user_data_registered_itu_t_t35_info();
        break;
    case  SEI_USER_DATA_UNREGISTERED:
        FUN_CHECK(ret = interpret_user_data_unregistered_info(msg, sei_msg->payload_size, sei_msg));
        break;
    case  SEI_RECOVERY_POINT:
        FUN_CHECK(ret = interpret_recovery_point_info(msg, sei_msg->payload_size, p_bitctx, sei_msg));
        break;
    case  SEI_DEC_REF_PIC_MARKING_REPETITION:
        interpret_dec_ref_pic_marking_repetition_info();
        break;
    case  SEI_SPARE_PIC:
        interpret_spare_pic();
        break;
    case  SEI_SCENE_INFO:
        interpret_scene_information();
        break;
    case  SEI_SUB_SEQ_INFO:
        interpret_subsequence_info();
        break;
    case  SEI_SUB_SEQ_LAYER_CHARACTERISTICS:
        interpret_subsequence_layer_characteristics_info();
        break;
    case  SEI_SUB_SEQ_CHARACTERISTICS:
        interpret_subsequence_characteristics_info();
        break;
    case  SEI_FULL_FRAME_FREEZE:
        interpret_full_frame_freeze_info();
        break;
    case  SEI_FULL_FRAME_FREEZE_RELEASE:
        interpret_full_frame_freeze_release_info();
        break;
    case  SEI_FULL_FRAME_SNAPSHOT:
        interpret_full_frame_snapshot_info();
        break;
    case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START:
        interpret_progressive_refinement_start_info();
        break;
    case  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END:
        interpret_progressive_refinement_end_info();
        break;
    case  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET:
        interpret_motion_constrained_slice_group_set_info();
    case  SEI_FILM_GRAIN_CHARACTERISTICS:
        interpret_film_grain_characteristics_info();
        break;
    case  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE:
        interpret_deblocking_filter_display_preference_info();
        break;
    case  SEI_STEREO_VIDEO_INFO:
        interpret_stereo_video_info_info();
        break;
    case  SEI_TONE_MAPPING:
        interpret_tone_mapping();
        break;
    case  SEI_POST_FILTER_HINTS:
        interpret_post_filter_hints_info();
        break;
    case  SEI_FRAME_PACKING_ARRANGEMENT:
        interpret_frame_packing_arrangement_info();
        break;
    case SEI_MVC_SCALABLE_NESTING:
        FUN_CHECK(ret = interpret_mvc_scalable_nesting_info(msg, sei_msg->payload_size, p_bitctx));
        sei_msg->mvc_scalable_nesting_flag = 1;
        break;
    case SEI_VIEW_SCALABILITY_INFO:
        interpret_mvc_scalability_info();
        break;
    default:
        interpret_reserved_info(msg, sei_msg->payload_size, p_bitctx, sei_msg);
        break;
    }

    return ret = MPP_OK;
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
MPP_RET process_sei(H264_SLICE_t *currSlice)
{
    RK_S32  nn = 0;
    RK_S32  tmp_byte = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_SEI_t *sei_msg  = &currSlice->p_Cur->sei;
    BitReadCtx_t *p_bitctx = &currSlice->p_Cur->bitctx;

    memset(sei_msg, 0, sizeof(*sei_msg));
    sei_msg->mvc_scalable_nesting_flag = 0;  //init to false
    sei_msg->p_Dec = currSlice->p_Dec;
    do {
        sei_msg->type = 0;
        READ_BITS(p_bitctx, 8, &tmp_byte);
        while (tmp_byte == 0xFF) {
            sei_msg->type += 255;
            READ_BITS(p_bitctx, 8, &tmp_byte);
        }
        sei_msg->type += tmp_byte;   // this is the last byte

        sei_msg->payload_size = 0;
        READ_BITS(p_bitctx, 8, &tmp_byte);
        while (tmp_byte == 0xFF) {
            sei_msg->payload_size += 255;
            READ_BITS(p_bitctx, 8, &tmp_byte);
        }
        sei_msg->payload_size += tmp_byte;   // this is the last byte

        //--- read sei info
        FUN_CHECK(ret = parserSEI(currSlice, p_bitctx, sei_msg, p_bitctx->data_));
        //--- set offset to read next sei nal
        if (SEI_MVC_SCALABLE_NESTING == sei_msg->type) {
            sei_msg->payload_size = ((p_bitctx->used_bits + 0x07) >> 3);
        }
        for (nn = 0; nn < sei_msg->payload_size; nn++) { // read bytes
            READ_BITS(p_bitctx, 8, &tmp_byte);
        }

    } while ((p_bitctx->data_[0] != 0x80) && (p_bitctx->bytes_left_ > 1));    // more_rbsp_data()  msg[offset] != 0x80

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:
    return ret;
}
