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

static MPP_RET interpret_picture_timing_info(
    BitReadCtx_t *p_bitctx,
    H264_SEI_t *sei_msg,
    H264_VUI_t vui_seq_parameters)
{
    RK_S32 cpb_removal_delay_length = 0;
    RK_S32 dpb_output_delay_length = 0;
    RK_S32 time_offset_length = 0;
    RK_S32 cpb_dpb_delays_present_flag = 0;
    RK_U32 i = 0;
    H264_SEI_PIC_TIMING_t *pic_timing = NULL;
    RK_U32 num_clock_ts[9] = {1, 1, 1, 2, 2, 3, 3, 2, 3};

    pic_timing = &(sei_msg->pic_timing);

    if (vui_seq_parameters.nal_hrd_parameters_present_flag) {
        cpb_removal_delay_length =
            vui_seq_parameters.nal_hrd_parameters.cpb_removal_delay_length_minus1;
        dpb_output_delay_length =
            vui_seq_parameters.nal_hrd_parameters.dpb_output_delay_length_minus1;
        time_offset_length =
            vui_seq_parameters.nal_hrd_parameters.time_offset_length;
        cpb_dpb_delays_present_flag = 1;
    } else if (vui_seq_parameters.vcl_hrd_parameters_present_flag) {
        cpb_removal_delay_length =
            vui_seq_parameters.vcl_hrd_parameters.cpb_removal_delay_length_minus1;
        dpb_output_delay_length =
            vui_seq_parameters.vcl_hrd_parameters.dpb_output_delay_length_minus1;
        time_offset_length =
            vui_seq_parameters.vcl_hrd_parameters.time_offset_length;
        cpb_dpb_delays_present_flag = 1;
    }

    if (cpb_dpb_delays_present_flag) {
        READ_BITS(p_bitctx, cpb_removal_delay_length, &pic_timing->cpb_removal_delay);
        READ_BITS(p_bitctx, dpb_output_delay_length, &pic_timing->dpb_output_delay);
    }

    if (vui_seq_parameters.pic_struct_present_flag) {
        READ_BITS(p_bitctx, 4, &pic_timing->pic_struct);
        if (pic_timing->pic_struct > 8 || pic_timing->pic_struct < 0) {
            goto __BITREAD_ERR;
        }

        for (i = 0; i < num_clock_ts[pic_timing->pic_struct]; i++) {
            READ_BITS(p_bitctx, 1, &pic_timing->clock_timestamp_flag[i]);

            if (pic_timing->clock_timestamp_flag[i]) {
                READ_BITS(p_bitctx, 2, &pic_timing->ct_type[i]);
                READ_BITS(p_bitctx, 1, &pic_timing->nuit_field_based_flag[i]);

                READ_BITS(p_bitctx, 5, &pic_timing->counting_type[i]);
                if (pic_timing->counting_type[i] > 6
                    || pic_timing->counting_type[i] < 0) {
                    goto __BITREAD_ERR;
                }

                READ_BITS(p_bitctx, 1, &pic_timing->full_timestamp_flag[i]);
                READ_BITS(p_bitctx, 1, &pic_timing->discontinuity_flag[i]);
                READ_BITS(p_bitctx, 1, &pic_timing->cnt_dropped_flag[i]);

                READ_BITS(p_bitctx, 8, &pic_timing->n_frames[i]);

                if (pic_timing->full_timestamp_flag[i]) {
                    READ_BITS(p_bitctx, 6, &pic_timing->seconds_value[i]);
                    if (pic_timing->seconds_value[i] > 59) {
                        goto __BITREAD_ERR;
                    }

                    READ_BITS(p_bitctx, 6, &pic_timing->minutes_value[i]);
                    if (pic_timing->minutes_value[i] > 59) {
                        goto __BITREAD_ERR;
                    }

                    READ_BITS(p_bitctx, 5, &pic_timing->hours_value[i]);
                    if (pic_timing->hours_value[i] > 23) {
                        goto __BITREAD_ERR;
                    }
                } else {
                    READ_BITS(p_bitctx, 1, &pic_timing->seconds_flag[i]);
                    if (pic_timing->seconds_flag[i]) {
                        READ_BITS(p_bitctx, 6, &pic_timing->seconds_value[i]);
                        if (pic_timing->seconds_value[i] > 59) {
                            goto __BITREAD_ERR;
                        }

                        READ_BITS(p_bitctx, 1, &pic_timing->minutes_flag[i]);
                        if (pic_timing->minutes_flag[i]) {
                            READ_BITS(p_bitctx, 6, &pic_timing->minutes_value[i]);
                            if (pic_timing->minutes_value[i] > 59) {
                                goto __BITREAD_ERR;
                            }

                            READ_BITS(p_bitctx, 1, &pic_timing->hours_flag[i]);
                            if (pic_timing->hours_flag[i]) {
                                READ_BITS(p_bitctx, 5, &pic_timing->hours_value[i]);
                                if (pic_timing->hours_value[i] > 23) {
                                    goto __BITREAD_ERR;
                                }
                            }
                        }
                    }
                }
                if (time_offset_length) {
                    RK_S32 tmp;
                    READ_BITS(p_bitctx, time_offset_length, &tmp);
                    /* following "converts" timeOffsetLength-bit signed
                     * integer into i32 */
                    /*lint -save -e701 -e702 */
                    tmp <<= (32 - time_offset_length);
                    tmp >>= (32 - time_offset_length);
                    /*lint -restore */
                    pic_timing->time_offset[i] = tmp;
                } else
                    pic_timing->time_offset[i] = 0;
            }
        }
    }

    return MPP_OK;
__BITREAD_ERR:
    return MPP_ERR_STREAM;
}

static MPP_RET interpret_buffering_period_info(
    BitReadCtx_t *p_bitctx,
    H264_SEI_t *sei_msg,
    H264_VUI_t vui_seq_parameters)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 i = 0;
    READ_UE(p_bitctx, &sei_msg->seq_parameter_set_id);

    if (sei_msg->seq_parameter_set_id > 31) {
        goto __BITREAD_ERR;
    }

    if (vui_seq_parameters.nal_hrd_parameters_present_flag) {
        for (i = 0; i < vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1; i++) {
            SKIP_BITS(p_bitctx,
                      vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1); //initial_cpb_removal_delay
            SKIP_BITS(p_bitctx,
                      vui_seq_parameters.nal_hrd_parameters.initial_cpb_removal_delay_length_minus1); //initial_cpb_removal_delay_offset
        }
    }


    if (vui_seq_parameters.vcl_hrd_parameters_present_flag) {
        for (i = 0; i < vui_seq_parameters.vcl_hrd_parameters.cpb_cnt_minus1; i++) {
            SKIP_BITS(p_bitctx,
                      vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1); //initial_cpb_removal_delay
            SKIP_BITS(p_bitctx,
                      vui_seq_parameters.vcl_hrd_parameters.initial_cpb_removal_delay_length_minus1); //initial_cpb_removal_delay_offset
        }
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;

}

static MPP_RET interpret_reserved_info(RK_S32 size, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg)
{
    RK_U8 payload_byte = 0;
    RK_S32 i = 0;

    for (i = 0; i < size; i++) {
        SKIP_BITS(p_bitctx, 8);
    }
    (void)p_bitctx;
    (void)sei_msg;
    (void)payload_byte;
__BITREAD_ERR:
    return p_bitctx->ret;
}

static MPP_RET parserSEI(H264_SLICE_t *cur_slice, BitReadCtx_t *p_bitctx, H264_SEI_t *sei_msg, RK_U8 *msg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dVideoCtx_t  *p_Vid = cur_slice->p_Vid;
    H264_SPS_t *sps = NULL;
    H264_subSPS_t *subset_sps = NULL;

    (void)msg;

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

    H264D_DBG(H264D_DBG_SEI, "[SEI_info] type=%d size: %d\n", sei_msg->type, sei_msg->payload_size);
    switch (sei_msg->type) {
    case  SEI_BUFFERING_PERIOD: {
        FUN_CHECK(ret = interpret_buffering_period_info(p_bitctx, sei_msg, p_Vid->active_sps->vui_seq_parameters));
    }
    break;
    case  SEI_PIC_TIMING: {
        interpret_picture_timing_info(p_bitctx,
                                      sei_msg,
                                      p_Vid->active_sps->vui_seq_parameters);
    }
    break;
    case  SEI_USER_DATA_UNREGISTERED:
        break;
    default: {
        interpret_reserved_info(sei_msg->payload_size, p_bitctx, sei_msg);
    }
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
    RK_S32  tmp_byte = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264_SEI_t *sei_msg  = &currSlice->p_Cur->sei;
    BitReadCtx_t *p_bitctx = &currSlice->p_Cur->bitctx;
    RK_U32 next;

    sei_msg->mvc_scalable_nesting_flag = 0;  //init to false
    sei_msg->p_Dec = currSlice->p_Dec;
    do {
        tmp_byte = 0xFF;
        sei_msg->type = 0;
        while (tmp_byte == 0xFF) {
            READ_BITS(p_bitctx, 8, &tmp_byte);
            sei_msg->type += tmp_byte;
        }

        tmp_byte = 0xFF;
        sei_msg->payload_size = 0;
        while (tmp_byte == 0xFF) {
            READ_BITS(p_bitctx, 8, &tmp_byte);
            sei_msg->payload_size += tmp_byte;
        }

        next = p_bitctx->used_bits + 8 * sei_msg->payload_size;
        //--- read sei info
        FUN_CHECK(ret = parserSEI(currSlice, p_bitctx, sei_msg, p_bitctx->data_));
        //--- set offset to read next sei nal
        if (SEI_MVC_SCALABLE_NESTING == sei_msg->type) {
            sei_msg->payload_size = ((p_bitctx->used_bits + 0x07) >> 3);
        }

        SKIP_BITS(p_bitctx, next - p_bitctx->used_bits);

    } while (mpp_has_more_rbsp_data(p_bitctx));    // more_rbsp_data()  msg[offset] != 0x80

    return ret = MPP_OK;
__BITREAD_ERR:
__FAILED:
    return ret;
}
