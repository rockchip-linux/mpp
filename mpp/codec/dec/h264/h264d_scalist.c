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

#include <stdlib.h>
#include <string.h>

#include "h264d_scalist.h"



static RK_U8 ZZ_SCAN[16] =
{ 0, 1, 4, 8, 5, 2, 3, 6, 9, 12, 13, 10, 7, 11, 14, 15 };

static RK_U8 ZZ_SCAN8[64] = {
    0, 1, 8, 16, 9, 2, 3, 10, 17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34, 27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63
};

static RK_S32 Default4x4Intra[H264ScalingList4x4Length] =
{ 6, 13, 20, 28, 13, 20, 28, 32, 20, 28, 32, 37, 28, 32, 37, 42 };

static RK_S32 Default4x4Inter[H264ScalingList4x4Length] =
{ 10, 14, 20, 24, 14, 20, 24, 27, 20, 24, 27, 30, 24, 27, 30, 34 };

static RK_S32 Default8x8Intra[H264ScalingList8x8Length] = {
    6, 10, 13, 16, 18, 23, 25, 27, 10, 11, 16, 18, 23, 25, 27, 29,
    13, 16, 18, 23, 25, 27, 29, 31, 16, 18, 23, 25, 27, 29, 31, 33,
    18, 23, 25, 27, 29, 31, 33, 36, 23, 25, 27, 29, 31, 33, 36, 38,
    25, 27, 29, 31, 33, 36, 38, 40, 27, 29, 31, 33, 36, 38, 40, 42
};

static RK_S32 Default8x8Inter[H264ScalingList8x8Length] = {
    9, 13, 15, 17, 19, 21, 22, 24, 13, 13, 17, 19, 21, 22, 24, 25,
    15, 17, 19, 21, 22, 24, 25, 27, 17, 19, 21, 22, 24, 25, 27, 28,
    19, 21, 22, 24, 25, 27, 28, 30, 21, 22, 24, 25, 27, 28, 30, 32,
    22, 24, 25, 27, 28, 30, 32, 33, 24, 25, 27, 28, 30, 32, 33, 35
};

static RK_S32 Default4x4[16] =
{ 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16 };

static RK_S32 Default8x8[64] = {
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};


static void set_sps_scanlist_matrix(H264_SPS_t *sps, H264dVideoCtx_t *p_Vid)
{
    RK_S32 i = 0;

    for (i = 0; i < 6; ++i) {
        if (!sps->seq_scaling_list_present_flag[i]) { // fall-back rule A
            if (i == 0) {
                p_Vid->qmatrix[i] = Default4x4Intra;
            } else if (i == 3) {
                p_Vid->qmatrix[i] = Default4x4Inter;
            } else {
                p_Vid->qmatrix[i] = p_Vid->qmatrix[i - 1];
            }
        } else {
            if (sps->UseDefaultScalingMatrix4x4Flag[i]) {

                p_Vid->qmatrix[i] = (i < 3) ? Default4x4Intra : Default4x4Inter;
            } else {
                p_Vid->qmatrix[i] = sps->ScalingList4x4[i];
            }
        }
    }

    for (i = 6; i < ((sps->chroma_format_idc != YUV444) ? 8 : 12); ++i) {
        if (!sps->seq_scaling_list_present_flag[i]) { // fall-back rule A
            if (i == 6) {
                p_Vid->qmatrix[i] = Default8x8Intra;
            } else if (i == 7) {
                p_Vid->qmatrix[i] = Default8x8Inter;
            } else {
                p_Vid->qmatrix[i] = p_Vid->qmatrix[i - 2];
            }
        } else {
            if (sps->UseDefaultScalingMatrix8x8Flag[i - 6]) {

                p_Vid->qmatrix[i] = (i == 6 || i == 8 || i == 10) ? Default8x8Intra : Default8x8Inter;
            } else {
                p_Vid->qmatrix[i] = sps->ScalingList8x8[i - 6];
            }
        }
    }
}

static void set_pps_scanlist_matrix(H264_SPS_t *sps, H264_PPS_t *pps, H264dVideoCtx_t *p_Vid)
{
    RK_S32 i = 0;

    for (i = 0; i < 6; ++i) {
        if (!pps->pic_scaling_list_present_flag[i]) { // fall-back rule B
            if (i == 0) {
                if (!sps->seq_scaling_matrix_present_flag) {
                    p_Vid->qmatrix[i] = Default4x4Intra;
                }
            } else if (i == 3) {
                if (!sps->seq_scaling_matrix_present_flag) {
                    p_Vid->qmatrix[i] = Default4x4Inter;
                }
            } else {
                p_Vid->qmatrix[i] = p_Vid->qmatrix[i - 1];
            }
        } else {
            if (pps->UseDefaultScalingMatrix4x4Flag[i]) {
                p_Vid->qmatrix[i] = (i < 3) ? Default4x4Intra : Default4x4Inter;
            } else {
                p_Vid->qmatrix[i] = pps->ScalingList4x4[i];
            }
        }
    }
    for (i = 6; i < ((sps->chroma_format_idc != YUV444) ? 8 : 12); ++i) {
        if (!pps->pic_scaling_list_present_flag[i]) { // fall-back rule B
            if (i == 6) {
                if (!sps->seq_scaling_matrix_present_flag) {
                    p_Vid->qmatrix[i] = Default8x8Intra;
                }
            } else if (i == 7) {
                if (!sps->seq_scaling_matrix_present_flag)
                    p_Vid->qmatrix[i] = Default8x8Inter;
            } else
                p_Vid->qmatrix[i] = p_Vid->qmatrix[i - 2];
        } else {
            if (pps->UseDefaultScalingMatrix8x8Flag[i - 6]) {
                p_Vid->qmatrix[i] = (i == 6 || i == 8 || i == 10) ? Default8x8Intra : Default8x8Inter;
            } else {
                p_Vid->qmatrix[i] = pps->ScalingList8x8[i - 6];
            }
        }
    }
}

/*!
***********************************************************************
* \brief
*    check profile
***********************************************************************
*/
//extern "C"
RK_U32 is_prext_profile(RK_U32 profile_idc)
{
    return (profile_idc >= H264_PROFILE_HIGH || profile_idc == H264_PROFILE_FREXT_CAVLC444) ? 1 : 0;
}

/*!
***********************************************************************
* \brief
*    parse sps and process sps
***********************************************************************
*/
//extern "C"
MPP_RET parse_scalingList(BitReadCtx_t *p_bitctx, RK_S32 size, RK_S32 *scaling_list, RK_S32 *use_default)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_S32 last_scale = 8;
    RK_S32 next_scale = 8;
    RK_S32 delta_scale = 0;
    RK_S32 j = 0, scanj = 0;

    RK_U8  *zz_scan = (size > 16) ? ZZ_SCAN8 : ZZ_SCAN;

    *use_default = 0;
    for (j = 0; j < size; ++j) {
        scanj = zz_scan[j];
        if (next_scale != 0) {
            READ_SE(p_bitctx, &delta_scale);
            next_scale = (last_scale + delta_scale + 256) & 0xff;
            *use_default = (scanj == 0 && next_scale == 0) ? 1 : 0;
        }
        scaling_list[scanj] = (next_scale == 0) ? last_scale : next_scale;
        last_scale = scaling_list[scanj];
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
    return ret;
}


/*!
***********************************************************************
* \brief
*    parse sps and process sps
***********************************************************************
*/
//extern "C"
MPP_RET get_max_dec_frame_buf_size(H264_SPS_t *sps)
{
    RK_S32 size = 0;
    RK_S32 pic_size = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;


    switch (sps->level_idc) {
    case 9:
        size = 152064;
        break;
    case 10:
        size = 152064;
        break;
    case 11:
        if (sps->constrained_set3_flag && !is_prext_profile(sps->profile_idc)) {
            size = 152064;
        } else {
            size = 345600;
        }
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
        ASSERT(0);  // undefined level
        return ret = MPP_NOK;
    }
    pic_size = (sps->pic_width_in_mbs_minus1 + 1)
               * (sps->pic_height_in_map_units_minus1 + 1)
               * (sps->frame_mbs_only_flag ? 1 : 2) * 384;
    size /= pic_size;
    size = MPP_MIN(size, 16);
    sps->max_dec_frame_buffering = size;

    return ret = MPP_OK;
}

/*!
***********************************************************************
* \brief
*    parse sps and process sps
***********************************************************************
*/
//extern "C"
MPP_RET parse_sps_scalinglists(BitReadCtx_t *p_bitctx, H264_SPS_t *sps)
{
    RK_S32 i = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    // Parse scaling_list4x4.
    for (i = 0; i < 6; ++i) {
        READ_ONEBIT(p_bitctx, &sps->seq_scaling_list_present_flag[i]);

        if (sps->seq_scaling_list_present_flag[i]) {
            FUN_CHECK(ret = parse_scalingList(p_bitctx, H264ScalingList4x4Length,
                                              sps->ScalingList4x4[i], &sps->UseDefaultScalingMatrix4x4Flag[i]));
        }
    }
    // Parse scaling_list8x8.
    for (i = 0; i < ((sps->chroma_format_idc != YUV444) ? 2 : 6); ++i) {
        READ_ONEBIT(p_bitctx, &sps->seq_scaling_list_present_flag[6 + i]);
        if (sps->seq_scaling_list_present_flag[6 + i]) {
            FUN_CHECK(ret = parse_scalingList(p_bitctx, H264ScalingList8x8Length,
                                              sps->ScalingList8x8[i], &sps->UseDefaultScalingMatrix8x8Flag[i]));
        }
    }

    return ret = MPP_OK;
__BITREAD_ERR:
    ret = p_bitctx->ret;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    prepare scanlist info to register syntax
***********************************************************************
*/
//extern "C"
MPP_RET prepare_init_scanlist(H264_SLICE_t *currSlice)
{
    RK_S32 i = 0;
    H264_SPS_t *sps = currSlice->p_Vid->active_sps;
    H264_PPS_t *pps = currSlice->p_Vid->active_pps;

    if (!pps->pic_scaling_matrix_present_flag && !sps->seq_scaling_matrix_present_flag) {
        for (i = 0; i < 12; i++) {
            currSlice->p_Vid->qmatrix[i] = (i < 6) ? Default4x4 : Default8x8;
        }
    } else {
        if (sps->seq_scaling_matrix_present_flag) { // check sps first
            set_sps_scanlist_matrix(sps, currSlice->p_Vid);
        }
        if (pps->pic_scaling_matrix_present_flag) { // then check pps
            set_pps_scanlist_matrix(sps, pps, currSlice->p_Vid);
        }
    }
    return MPP_OK;
}
