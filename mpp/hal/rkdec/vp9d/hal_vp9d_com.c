/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#include <string.h>

#include "mpp_mem.h"
#include "mpp_bitput.h"

#include "vp9d_syntax.h"
#include "hal_vp9d_com.h"

const vp9_prob vp9_kf_y_mode_prob[INTRA_MODES][INTRA_MODES][INTRA_MODES - 1] = {
    {
        // above = dc
        { 137,  30,  42, 148, 151, 207,  70,  52,  91 },  // left = dc
        {  92,  45, 102, 136, 116, 180,  74,  90, 100 },  // left = v
        {  73,  32,  19, 187, 222, 215,  46,  34, 100 },  // left = h
        {  91,  30,  32, 116, 121, 186,  93,  86,  94 },  // left = d45
        {  72,  35,  36, 149,  68, 206,  68,  63, 105 },  // left = d135
        {  73,  31,  28, 138,  57, 124,  55, 122, 151 },  // left = d117
        {  67,  23,  21, 140, 126, 197,  40,  37, 171 },  // left = d153
        {  86,  27,  28, 128, 154, 212,  45,  43,  53 },  // left = d207
        {  74,  32,  27, 107,  86, 160,  63, 134, 102 },  // left = d63
        {  59,  67,  44, 140, 161, 202,  78,  67, 119 }   // left = tm
    }, {  // above = v
        {  63,  36, 126, 146, 123, 158,  60,  90,  96 },  // left = dc
        {  43,  46, 168, 134, 107, 128,  69, 142,  92 },  // left = v
        {  44,  29,  68, 159, 201, 177,  50,  57,  77 },  // left = h
        {  58,  38,  76, 114,  97, 172,  78, 133,  92 },  // left = d45
        {  46,  41,  76, 140,  63, 184,  69, 112,  57 },  // left = d135
        {  38,  32,  85, 140,  46, 112,  54, 151, 133 },  // left = d117
        {  39,  27,  61, 131, 110, 175,  44,  75, 136 },  // left = d153
        {  52,  30,  74, 113, 130, 175,  51,  64,  58 },  // left = d207
        {  47,  35,  80, 100,  74, 143,  64, 163,  74 },  // left = d63
        {  36,  61, 116, 114, 128, 162,  80, 125,  82 }   // left = tm
    }, {  // above = h
        {  82,  26,  26, 171, 208, 204,  44,  32, 105 },  // left = dc
        {  55,  44,  68, 166, 179, 192,  57,  57, 108 },  // left = v
        {  42,  26,  11, 199, 241, 228,  23,  15,  85 },  // left = h
        {  68,  42,  19, 131, 160, 199,  55,  52,  83 },  // left = d45
        {  58,  50,  25, 139, 115, 232,  39,  52, 118 },  // left = d135
        {  50,  35,  33, 153, 104, 162,  64,  59, 131 },  // left = d117
        {  44,  24,  16, 150, 177, 202,  33,  19, 156 },  // left = d153
        {  55,  27,  12, 153, 203, 218,  26,  27,  49 },  // left = d207
        {  53,  49,  21, 110, 116, 168,  59,  80,  76 },  // left = d63
        {  38,  72,  19, 168, 203, 212,  50,  50, 107 }   // left = tm
    }, {  // above = d45
        { 103,  26,  36, 129, 132, 201,  83,  80,  93 },  // left = dc
        {  59,  38,  83, 112, 103, 162,  98, 136,  90 },  // left = v
        {  62,  30,  23, 158, 200, 207,  59,  57,  50 },  // left = h
        {  67,  30,  29,  84,  86, 191, 102,  91,  59 },  // left = d45
        {  60,  32,  33, 112,  71, 220,  64,  89, 104 },  // left = d135
        {  53,  26,  34, 130,  56, 149,  84, 120, 103 },  // left = d117
        {  53,  21,  23, 133, 109, 210,  56,  77, 172 },  // left = d153
        {  77,  19,  29, 112, 142, 228,  55,  66,  36 },  // left = d207
        {  61,  29,  29,  93,  97, 165,  83, 175, 162 },  // left = d63
        {  47,  47,  43, 114, 137, 181, 100,  99,  95 }   // left = tm
    }, {  // above = d135
        {  69,  23,  29, 128,  83, 199,  46,  44, 101 },  // left = dc
        {  53,  40,  55, 139,  69, 183,  61,  80, 110 },  // left = v
        {  40,  29,  19, 161, 180, 207,  43,  24,  91 },  // left = h
        {  60,  34,  19, 105,  61, 198,  53,  64,  89 },  // left = d45
        {  52,  31,  22, 158,  40, 209,  58,  62,  89 },  // left = d135
        {  44,  31,  29, 147,  46, 158,  56, 102, 198 },  // left = d117
        {  35,  19,  12, 135,  87, 209,  41,  45, 167 },  // left = d153
        {  55,  25,  21, 118,  95, 215,  38,  39,  66 },  // left = d207
        {  51,  38,  25, 113,  58, 164,  70,  93,  97 },  // left = d63
        {  47,  54,  34, 146, 108, 203,  72, 103, 151 }   // left = tm
    }, {  // above = d117
        {  64,  19,  37, 156,  66, 138,  49,  95, 133 },  // left = dc
        {  46,  27,  80, 150,  55, 124,  55, 121, 135 },  // left = v
        {  36,  23,  27, 165, 149, 166,  54,  64, 118 },  // left = h
        {  53,  21,  36, 131,  63, 163,  60, 109,  81 },  // left = d45
        {  40,  26,  35, 154,  40, 185,  51,  97, 123 },  // left = d135
        {  35,  19,  34, 179,  19,  97,  48, 129, 124 },  // left = d117
        {  36,  20,  26, 136,  62, 164,  33,  77, 154 },  // left = d153
        {  45,  18,  32, 130,  90, 157,  40,  79,  91 },  // left = d207
        {  45,  26,  28, 129,  45, 129,  49, 147, 123 },  // left = d63
        {  38,  44,  51, 136,  74, 162,  57,  97, 121 }   // left = tm
    }, {  // above = d153
        {  75,  17,  22, 136, 138, 185,  32,  34, 166 },  // left = dc
        {  56,  39,  58, 133, 117, 173,  48,  53, 187 },  // left = v
        {  35,  21,  12, 161, 212, 207,  20,  23, 145 },  // left = h
        {  56,  29,  19, 117, 109, 181,  55,  68, 112 },  // left = d45
        {  47,  29,  17, 153,  64, 220,  59,  51, 114 },  // left = d135
        {  46,  16,  24, 136,  76, 147,  41,  64, 172 },  // left = d117
        {  34,  17,  11, 108, 152, 187,  13,  15, 209 },  // left = d153
        {  51,  24,  14, 115, 133, 209,  32,  26, 104 },  // left = d207
        {  55,  30,  18, 122,  79, 179,  44,  88, 116 },  // left = d63
        {  37,  49,  25, 129, 168, 164,  41,  54, 148 }   // left = tm
    }, {  // above = d207
        {  82,  22,  32, 127, 143, 213,  39,  41,  70 },  // left = dc
        {  62,  44,  61, 123, 105, 189,  48,  57,  64 },  // left = v
        {  47,  25,  17, 175, 222, 220,  24,  30,  86 },  // left = h
        {  68,  36,  17, 106, 102, 206,  59,  74,  74 },  // left = d45
        {  57,  39,  23, 151,  68, 216,  55,  63,  58 },  // left = d135
        {  49,  30,  35, 141,  70, 168,  82,  40, 115 },  // left = d117
        {  51,  25,  15, 136, 129, 202,  38,  35, 139 },  // left = d153
        {  68,  26,  16, 111, 141, 215,  29,  28,  28 },  // left = d207
        {  59,  39,  19, 114,  75, 180,  77, 104,  42 },  // left = d63
        {  40,  61,  26, 126, 152, 206,  61,  59,  93 }   // left = tm
    }, {  // above = d63
        {  78,  23,  39, 111, 117, 170,  74, 124,  94 },  // left = dc
        {  48,  34,  86, 101,  92, 146,  78, 179, 134 },  // left = v
        {  47,  22,  24, 138, 187, 178,  68,  69,  59 },  // left = h
        {  56,  25,  33, 105, 112, 187,  95, 177, 129 },  // left = d45
        {  48,  31,  27, 114,  63, 183,  82, 116,  56 },  // left = d135
        {  43,  28,  37, 121,  63, 123,  61, 192, 169 },  // left = d117
        {  42,  17,  24, 109,  97, 177,  56,  76, 122 },  // left = d153
        {  58,  18,  28, 105, 139, 182,  70,  92,  63 },  // left = d207
        {  46,  23,  32,  74,  86, 150,  67, 183,  88 },  // left = d63
        {  36,  38,  48,  92, 122, 165,  88, 137,  91 }   // left = tm
    }, {  // above = tm
        {  65,  70,  60, 155, 159, 199,  61,  60,  81 },  // left = dc
        {  44,  78, 115, 132, 119, 173,  71, 112,  93 },  // left = v
        {  39,  38,  21, 184, 227, 206,  42,  32,  64 },  // left = h
        {  58,  47,  36, 124, 137, 193,  80,  82,  78 },  // left = d45
        {  49,  50,  35, 144,  95, 205,  63,  78,  59 },  // left = d135
        {  41,  53,  52, 148,  71, 142,  65, 128,  51 },  // left = d117
        {  40,  36,  28, 143, 143, 202,  40,  55, 137 },  // left = d153
        {  52,  34,  29, 129, 183, 227,  42,  35,  43 },  // left = d207
        {  42,  44,  44, 104, 105, 164,  64, 130,  80 },  // left = d63
        {  43,  81,  53, 140, 169, 204,  68,  84,  72 }   // left = tm
    }
};

const vp9_prob vp9_kf_uv_mode_prob[INTRA_MODES][INTRA_MODES - 1] = {
    { 144,  11,  54, 157, 195, 130,  46,  58, 108 },  // y = dc
    { 118,  15, 123, 148, 131, 101,  44,  93, 131 },  // y = v
    { 113,  12,  23, 188, 226, 142,  26,  32, 125 },  // y = h
    { 120,  11,  50, 123, 163, 135,  64,  77, 103 },  // y = d45
    { 113,   9,  36, 155, 111, 157,  32,  44, 161 },  // y = d135
    { 116,   9,  55, 176,  76,  96,  37,  61, 149 },  // y = d117
    { 115,   9,  28, 141, 161, 167,  21,  25, 193 },  // y = d153
    { 120,  12,  32, 145, 195, 142,  32,  38,  86 },  // y = d207
    { 116,  12,  64, 120, 140, 125,  49, 115, 121 },  // y = d63
    { 102,  19,  66, 162, 182, 122,  35,  59, 128 }   // y = tm
};

const vp9_prob vp9_kf_partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1] = {
    // 8x8 -> 4x4
    { 158,  97,  94 },  // a/l both not split
    {  93,  24,  99 },  // a split, l not split
    {  85, 119,  44 },  // l split, a not split
    {  62,  59,  67 },  // a/l both split
    // 16x16 -> 8x8
    { 149,  53,  53 },  // a/l both not split
    {  94,  20,  48 },  // a split, l not split
    {  83,  53,  24 },  // l split, a not split
    {  52,  18,  18 },  // a/l both split
    // 32x32 -> 16x16
    { 150,  40,  39 },  // a/l both not split
    {  78,  12,  26 },  // a split, l not split
    {  67,  33,  11 },  // l split, a not split
    {  24,   7,   5 },  // a/l both split
    // 64x64 -> 32x32
    { 174,  35,  49 },  // a/l both not split
    {  68,  11,  27 },  // a split, l not split
    {  57,  15,   9 },  // l split, a not split
    {  12,   3,   3 },  // a/l both split
};

RK_U32 vp9_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 64);
}

RK_U32 vp9_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 256) | 256;
}

MPP_RET hal_vp9d_output_probe(void *buf, void *dxva)
{
    RK_S32 i, j, k, m, n;
    RK_S32 fifo_len = 304;
    RK_U64 *probe_packet = NULL;
    BitputCtx_t bp;
    DXVA_PicParams_VP9 *pic_param = (DXVA_PicParams_VP9*)dxva;
    RK_S32 intraFlag = (!pic_param->frame_type || pic_param->intra_only);
    vp9_prob partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1];
    vp9_prob uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];

    memset(buf, 0, 304 * 8);

    if (intraFlag) {
        memcpy(partition_probs, vp9_kf_partition_probs, sizeof(partition_probs));
        memcpy(uv_mode_prob, vp9_kf_uv_mode_prob, sizeof(uv_mode_prob));
    } else {
        memcpy(partition_probs, pic_param->prob.partition, sizeof(partition_probs));
        memcpy(uv_mode_prob, pic_param->prob.uv_mode, sizeof(uv_mode_prob));
    }

    probe_packet = mpp_calloc(RK_U64, fifo_len + 1);
    mpp_set_bitput_ctx(&bp, probe_packet, fifo_len);
    //sb info  5 x 128 bit
    for (i = 0; i < PARTITION_CONTEXTS; i++) //kf_partition_prob
        for (j = 0; j < PARTITION_TYPES - 1; j++)
            mpp_put_bits(&bp, partition_probs[i][j], 8); //48

    for (i = 0; i < PREDICTION_PROBS; i++) //Segment_id_pred_prob //3
        mpp_put_bits(&bp, pic_param->stVP9Segments.pred_probs[i], 8);

    for (i = 0; i < SEG_TREE_PROBS; i++) //Segment_id_probs
        mpp_put_bits(&bp, pic_param->stVP9Segments.tree_probs[i], 8); //7

    for (i = 0; i < SKIP_CONTEXTS; i++) //Skip_flag_probs //3
        mpp_put_bits(&bp, pic_param->prob.skip[i], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //6
        for (j = 0; j < TX_SIZES - 1; j++)
            mpp_put_bits(&bp, pic_param->prob.tx32p[i][j], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //4
        for (j = 0; j < TX_SIZES - 2; j++)
            mpp_put_bits(&bp, pic_param->prob.tx16p[i][j], 8);

    for (i = 0; i < TX_SIZE_CONTEXTS; i++) //Tx_size_probs //2
        mpp_put_bits(&bp, pic_param->prob.tx8p[i], 8);

    for (i = 0; i < INTRA_INTER_CONTEXTS; i++) //Tx_size_probs //4
        mpp_put_bits(&bp, pic_param->prob.intra[i], 8);

    mpp_put_align(&bp, 128, 0);
    if (intraFlag) { //intra probs
        //intra only //149 x 128 bit ,aligned to 152 x 128 bit
        //coeff releated prob   64 x 128 bit
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++)
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][0][k][m][n], 8);

                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }
                }
                mpp_put_align(&bp, 128, 0);
            }

        //intra mode prob  80 x 128 bit
        for (i = 0; i < INTRA_MODES; i++) { //vp9_kf_y_mode_prob
            RK_S32 byte_count = 0;
            for (j = 0; j < INTRA_MODES; j++)
                for (k = 0; k < INTRA_MODES - 1; k++) {
                    mpp_put_bits(&bp, vp9_kf_y_mode_prob[i][j][k], 8);
                    byte_count++;
                    if (byte_count == 27) {
                        byte_count = 0;
                        mpp_put_align(&bp, 128, 0);
                    }

                }
            if (i < 4) {
                for (m = 0; m < (i < 3 ? 23 : 21); m++)
                    mpp_put_bits(&bp, ((vp9_prob *)(&vp9_kf_uv_mode_prob[0][0]))[i * 23 + m], 8);
                for (; m < 23; m++)
                    mpp_put_bits(&bp, 0, 8);
            } else {
                for (m = 0; m < 23; m++)
                    mpp_put_bits(&bp, 0, 8);
            }
            mpp_put_align(&bp, 128, 0);
        }
        //align to 152 x 128 bit
        for (i = 0; i < INTER_PROB_SIZE_ALIGN_TO_128 - INTRA_PROB_SIZE_ALIGN_TO_128; i++) { //aligned to 153 x 256 bit
            mpp_put_bits(&bp, 0, 8);
            mpp_put_align(&bp, 128, 0);
        }
    } else {
        //inter probs
        //151 x 128 bit ,aligned to 152 x 128 bit
        //inter only

        //intra_y_mode & inter_block info   6 x 128 bit
        for (i = 0; i < BLOCK_SIZE_GROUPS; i++) //intra_y_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.y_mode[i][j], 8);

        for (i = 0; i < COMP_INTER_CONTEXTS; i++) //reference_mode prob
            mpp_put_bits(&bp, pic_param->prob.comp[i], 8);

        for (i = 0; i < REF_CONTEXTS; i++) //comp ref bit
            mpp_put_bits(&bp, pic_param->prob.comp_ref[i], 8);

        for (i = 0; i < REF_CONTEXTS; i++) //single ref bit
            for (j = 0; j < 2; j++)
                mpp_put_bits(&bp, pic_param->prob.single_ref[i][j], 8);

        for (i = 0; i < INTER_MODE_CONTEXTS; i++) //mv mode bit
            for (j = 0; j < INTER_MODES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_mode[i][j], 8);


        for (i = 0; i < SWITCHABLE_FILTER_CONTEXTS; i++) //comp ref bit
            for (j = 0; j < SWITCHABLE_FILTERS - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.filter[i][j], 8);

        mpp_put_align(&bp, 128, 0);

        //128 x 128bit
        //coeff releated
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++)
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][0][k][m][n], 8);
                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }
                }
                mpp_put_align(&bp, 128, 0);
            }
        for (i = 0; i < TX_SIZES; i++)
            for (j = 0; j < PLANE_TYPES; j++) {
                RK_S32 byte_count = 0;
                for (k = 0; k < COEF_BANDS; k++) {
                    for (m = 0; m < COEFF_CONTEXTS; m++) {
                        for (n = 0; n < UNCONSTRAINED_NODES; n++) {
                            mpp_put_bits(&bp, pic_param->prob.coef[i][j][1][k][m][n], 8);
                            byte_count++;
                            if (byte_count == 27) {
                                mpp_put_align(&bp, 128, 0);
                                byte_count = 0;
                            }
                        }

                    }
                }
                mpp_put_align(&bp, 128, 0);
            }

        //intra uv mode 6 x 128
        for (i = 0; i < 3; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);

        for (; i < 6; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);

        for (; i < 9; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);
        mpp_put_align(&bp, 128, 0);
        for (; i < INTRA_MODES; i++) //intra_uv_mode
            for (j = 0; j < INTRA_MODES - 1; j++)
                mpp_put_bits(&bp, uv_mode_prob[i][j], 8);

        mpp_put_align(&bp, 128, 0);
        mpp_put_bits(&bp, 0, 8);
        mpp_put_align(&bp, 128, 0);

        //mv releated 6 x 128
        for (i = 0; i < MV_JOINTS - 1; i++) //mv_joint_type
            mpp_put_bits(&bp, pic_param->prob.mv_joint[i], 8);

        for (i = 0; i < 2; i++) { //sign bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].sign, 8);
        }
        for (i = 0; i < 2; i++) { //classes bit
            for (j = 0; j < MV_CLASSES - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].classes[j], 8);
        }
        for (i = 0; i < 2; i++) { //classe0 bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0, 8);
        }
        for (i = 0; i < 2; i++) { // bits
            for (j = 0; j < MV_OFFSET_BITS; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].bits[j], 8);
        }
        for (i = 0; i < 2; i++) { //class0_fp bit
            for (j = 0; j < CLASS0_SIZE; j++)
                for (k = 0; k < MV_FP_SIZE - 1; k++)
                    mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0_fp[j][k], 8);
        }
        for (i = 0; i < 2; i++) { //comp ref bit
            for (j = 0; j < MV_FP_SIZE - 1; j++)
                mpp_put_bits(&bp, pic_param->prob.mv_comp[i].fp[j], 8);
        }
        for (i = 0; i < 2; i++) { //class0_hp bit

            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].class0_hp, 8);
        }
        for (i = 0; i < 2; i++) { //hp bit
            mpp_put_bits(&bp, pic_param->prob.mv_comp[i].hp, 8);
        }
        mpp_put_align(&bp, 128, 0);
    }

    memcpy(buf, probe_packet, 304 * 8);

#ifdef dump
    if (intraFlag) {
        fwrite(probe_packet, 1, 302 * 8, vp9_fp);
    } else {
        fwrite(probe_packet, 1, 304 * 8, vp9_fp);
    }
    fflush(vp9_fp);
#endif

    MPP_FREE(probe_packet);

    return 0;
}

void hal_vp9d_update_counts(void *buf, void *dxva)
{
    DXVA_PicParams_VP9 *s = (DXVA_PicParams_VP9*)dxva;
    RK_S32 i, j, m, n, k;
    RK_U32 *eob_coef;
    RK_S32 ref_type;
#ifdef dump
    RK_U32 count_length;
#endif
    RK_U32 com_len = 0;

#ifdef dump
    if (!(s->frame_type == 0 || s->intra_only)) //inter
        count_length = (213 * 64 + 576 * 5 * 32) / 8;
    else //intra
        count_length = (49 * 64 + 288 * 5 * 32) / 8;

    fwrite(buf, 1, count_length, vp9_fp1);
    fflush(vp9_fp1);
#endif
    if ((s->frame_type == 0 || s->intra_only)) {
        com_len = sizeof(s->counts.partition) + sizeof(s->counts.skip) + sizeof(s->counts.intra)
                  + sizeof(s->counts.tx32p) + sizeof(s->counts.tx16p) + sizeof(s->counts.tx8p);
    } else {
        com_len = sizeof(s->counts) - sizeof(s->counts.coef) - sizeof(s->counts.eob);
    }
    eob_coef = (RK_U32 *)(buf + com_len);
    memcpy(&s->counts, buf, com_len);
    ref_type = (!(s->frame_type == 0 || s->intra_only)) ? 2 : 1;
    if (ref_type == 1) {
        memset(s->counts.eob, 0, sizeof(s->counts.eob));
        memset(s->counts.coef, 0, sizeof(s->counts.coef));
    }
    for (i = 0; i < ref_type; i++) {
        for (j = 0; j < 4; j++) {
            for (m = 0; m < 2; m++) {
                for (n = 0; n < 6; n++) {
                    for (k = 0; k < 6; k++) {
                        s->counts.eob[j][m][i][n][k][0] = eob_coef[1];
                        s->counts.eob[j][m][i][n][k][1] = eob_coef[0] - eob_coef[1]; //ffmpeg need do  branch_ct[UNCONSTRAINED_NODES][2] =  { neob, eob_counts[i][j][k][l] - neob },
                        s->counts.coef[j][m][i][n][k][0] = eob_coef[2];
                        s->counts.coef[j][m][i][n][k][1] = eob_coef[3];
                        s->counts.coef[j][m][i][n][k][2] = eob_coef[4];
                        eob_coef += 5;
                    }
                }
            }
        }
    }
}
