/*
*
* Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "avs2d_ps"

#include <string.h>
#include <stdlib.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_debug.h"
#include "mpp_packet_impl.h"
#include "hal_task.h"

#include "avs2d_api.h"
#include "avs2d_ps.h"

static const RK_U32 wq_param_default[2][6] = {
    { 67, 71, 71, 80, 80, 106},
    { 64, 49, 53, 58, 58, 64 }
};

static const RK_U32 g_WqMDefault4x4[16] = {
    64, 64, 64, 68,
    64, 64, 68, 72,
    64, 68, 76, 80,
    72, 76, 84, 96
};

static const RK_U32 g_WqMDefault8x8[64] = {
    64,  64,  64,  64,  68,  68,  72,  76,
    64,  64,  64,  68,  72,  76,  84,  92,
    64,  64,  68,  72,  76,  80,  88,  100,
    64,  68,  72,  80,  84,  92,  100, 28,
    68,  72,  80,  84,  92,  104, 112, 128,
    76,  80,  84,  92,  104, 116, 132, 152,
    96,  100, 104, 116, 124, 140, 164, 188,
    104, 108, 116, 128, 152, 172, 192, 216
};

static const RK_U8 WeightQuantModel[4][64] = {
    //   l a b c d h
    //   0 1 2 3 4 5
    {
        // Mode 0
        0, 0, 0, 4, 4, 4, 5, 5,
        0, 0, 3, 3, 3, 3, 5, 5,
        0, 3, 2, 2, 1, 1, 5, 5,
        4, 3, 2, 2, 1, 5, 5, 5,
        4, 3, 1, 1, 5, 5, 5, 5,
        4, 3, 1, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5
    }, {
        // Mode 1
        0, 0, 0, 4, 4, 4, 5, 5,
        0, 0, 4, 4, 4, 4, 5, 5,
        0, 3, 2, 2, 2, 1, 5, 5,
        3, 3, 2, 2, 1, 5, 5, 5,
        3, 3, 2, 1, 5, 5, 5, 5,
        3, 3, 1, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5
    }, {
        // Mode 2
        0, 0, 0, 4, 4, 3, 5, 5,
        0, 0, 4, 4, 3, 2, 5, 5,
        0, 4, 4, 3, 2, 1, 5, 5,
        4, 4, 3, 2, 1, 5, 5, 5,
        4, 3, 2, 1, 5, 5, 5, 5,
        3, 2, 1, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5
    }, {
        // Mode 3
        0, 0, 0, 3, 2, 1, 5, 5,
        0, 0, 4, 3, 2, 1, 5, 5,
        0, 4, 4, 3, 2, 1, 5, 5,
        3, 3, 3, 3, 2, 5, 5, 5,
        2, 2, 2, 2, 5, 5, 5, 5,
        1, 1, 1, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5,
        5, 5, 5, 5, 5, 5, 5, 5
    }
};

static const RK_U8 WeightQuantModel4x4[4][16] = {
    //   l a b c d h
    //   0 1 2 3 4 5
    {
        // Mode 0
        0, 4, 3, 5,
        4, 2, 1, 5,
        3, 1, 1, 5,
        5, 5, 5, 5
    }, {
        // Mode 1
        0, 4, 4, 5,
        3, 2, 2, 5,
        3, 2, 1, 5,
        5, 5, 5, 5
    }, {
        // Mode 2
        0, 4, 3, 5,
        4, 3, 2, 5,
        3, 2, 1, 5,
        5, 5, 5, 5
    }, {
        // Mode 3
        0, 3, 1, 5,
        3, 4, 2, 5,
        1, 2, 2, 5,
        5, 5, 5, 5
    }
};

static const RK_U32 *wq_get_default_matrix(RK_U32 sizeId)
{
    return (sizeId == 0) ? g_WqMDefault4x4 : g_WqMDefault8x8;
}

static void wq_init_frame_quant_param(Avs2dCtx_t *p_dec)
{
    RK_U32 i, j, k;
    Avs2dPicHeader_t *ph  = &p_dec->ph;

    if (!p_dec->enable_wq || ph->pic_wq_data_index != 1) {
        return;
    }

    //<! determine weight quant param
    if (ph->wq_param_index == 0) {
        for (i = 0; i < WQP_SIZE; i++) {
            ph->pic_wq_param[i] = wq_param_default[DETAILED][i];
        }
    } else if (ph->wq_param_index == 1) {
        for (i = 0; i < WQP_SIZE; i++) {
            ph->pic_wq_param[i] = ph->wq_param_delta1[i] + wq_param_default[UNDETAILED][i];
        }
    } else if (ph->wq_param_index == 2) {
        for (i = 0; i < WQP_SIZE; i++) {
            ph->pic_wq_param[i] = ph->wq_param_delta2[i] + wq_param_default[DETAILED][i];
        }
    }

    //!< reconstruct the weighting matrix
    for (j = 0; j < 8; j++) {
        for (i = 0; i < 8; i++) {
            k = WeightQuantModel[ph->wq_model][j * 8 + i];
            ph->pic_wq_matrix[1][j * 8 + i] = ph->pic_wq_param[k];
        }
    }

    for (j = 0; j < 4; j++) {
        for (i = 0; i < 4; i++) {
            k = WeightQuantModel4x4[ph->wq_model][j * 4 + i];
            ph->pic_wq_matrix[0][j * 4 + i] = ph->pic_wq_param[k];
        }
    }
}

static void wq_update_frame_matrix(Avs2dCtx_t *p_dec)
{
    RK_U32 i, j;
    RK_U32 size_id, wqm_size;
    RK_U32 (*frm_wqm)[64];

    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;
    RK_U32 (*cur_wqm)[64] = p_dec->cur_wq_matrix;

    mpp_assert(ph->pic_wq_data_index <= 2);

    if (!p_dec->enable_wq) {
        for (size_id = 0; size_id < 2; size_id++) {
            for (i = 0; i < 64; i++) {
                cur_wqm[size_id][i] = 64;
            }
        }
    } else {
        if (ph->pic_wq_data_index == 0) {
            frm_wqm = vsh->seq_wq_matrix;
        } else {
            frm_wqm = ph->pic_wq_matrix;
        }

        for (size_id = 0; size_id < 2; size_id++) {
            wqm_size = MPP_MIN(1 << (size_id + 2), 8);
            for (i = 0; i < wqm_size; i++) {
                for (j = 0; j < wqm_size; j++) {
                    cur_wqm[size_id][i * wqm_size + j] = frm_wqm[size_id][i * wqm_size + j];
                }
            }
        }
    }
}

static MPP_RET parse_sequence_wqm(BitReadCtx_t *bitctx, Avs2dSeqHeader_t *vsh)
{
    MPP_RET ret = MPP_OK;
    RK_U8 load_seq_wquant_data_flag;
    RK_U32 i, j, size_id, wqm_size;
    const RK_U32 *seq_wqm;

    READ_ONEBIT(bitctx, &load_seq_wquant_data_flag);

    for (size_id = 0; size_id < 2; size_id++) {
        wqm_size = MPP_MIN(1 << (size_id + 2), 8);
        if (load_seq_wquant_data_flag == 1) {
            for (j = 0; j < wqm_size; j++) {
                for (i = 0; i < wqm_size; i++) {
                    READ_UE(bitctx, &vsh->seq_wq_matrix[size_id][j * wqm_size + i]);
                }
            }
        } else if (load_seq_wquant_data_flag == 0) {
            seq_wqm = wq_get_default_matrix(size_id);
            for (i = 0; i < (wqm_size * wqm_size); i++) {
                vsh->seq_wq_matrix[size_id][i] = seq_wqm[i];
            }
        }
    }

    return ret;
__BITREAD_ERR:
    return ret = bitctx->ret;
}

static MPP_RET parse_one_rps(BitReadCtx_t *bitctx, Avs2dRps_t *rps)
{
    MPP_RET ret = MPP_OK;
    RK_U32 j;

    AVS2D_PARSE_TRACE("In");
    READ_ONEBIT(bitctx, &rps->refered_by_others);

    READ_BITS(bitctx, 3, &rps->num_of_ref);
    AVS2D_PARSE_TRACE("refered_by_others_flag %d, num_Of_ref %d", rps->refered_by_others, rps->num_of_ref);
    if (rps->num_of_ref > AVS2_MAX_REFS) {
        ret = MPP_NOK;
        mpp_err_f("invalid ref num(%d).\n", rps->num_of_ref);
        goto __FAILED;
    }
    for (j = 0; j < rps->num_of_ref; j++) {
        READ_BITS(bitctx, 6, &rps->ref_pic[j]);
        AVS2D_PARSE_TRACE("delta_doi_of_ref_pic[%d]=%d", j, rps->ref_pic[j]);
    }

    READ_BITS(bitctx, 3, &rps->num_to_remove);
    for (j = 0; j < rps->num_to_remove; j++) {
        READ_BITS(bitctx, 6, &rps->remove_pic[j]);
        AVS2D_PARSE_TRACE("num_of_removed_pic[%d]=%d", j, rps->remove_pic[j]);
    }

    READ_MARKER_BIT(bitctx);
    AVS2D_PARSE_TRACE("Out ret %d\n", ret);
    return ret;
__BITREAD_ERR:
    AVS2D_PARSE_TRACE("Out Bit read Err\n");
    return ret = bitctx->ret;
__FAILED:
    AVS2D_PARSE_TRACE("Out Failed\n");
    return ret;
}

MPP_RET avs2d_parse_sequence_header(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;
    RK_U32 val_temp = 0;
    Avs2dRps_t *rps = NULL;
    BitReadCtx_t *bitctx = &p_dec->bitctx;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;

    AVS2D_PARSE_TRACE("Bitread buf %p, data %p, buf_len %d, left %d\n", bitctx->buf, bitctx->data_, bitctx->buf_len, bitctx->bytes_left_);
    memset(vsh, 0, sizeof(Avs2dSeqHeader_t));
    READ_BITS(bitctx, 8, &vsh->profile_id);
    //!< check profile_id
    if (vsh->profile_id != 0x12 && vsh->profile_id != 0x20 && vsh->profile_id != 0x22) {
        ret = MPP_NOK;
        mpp_err_f("profile_id 0x%02x is not supported.\n", vsh->profile_id);
        goto __FAILED;
    }
    READ_BITS(bitctx, 8, &vsh->level_id);
    if (vsh->level_id > 0x6A) {
        ret = MPP_NOK;
        mpp_err_f("level_id 0x%02x is not supported.\n", vsh->level_id);
        goto __FAILED;
    }
    READ_ONEBIT(bitctx, &vsh->progressive_sequence);
    READ_ONEBIT(bitctx, &vsh->field_coded_sequence);
    READ_BITS(bitctx, 14, &vsh->horizontal_size);
    READ_BITS(bitctx, 14, &vsh->vertical_size);
    mpp_err_f("video resolution %dx%d\n", vsh->horizontal_size, vsh->vertical_size);
    if (vsh->horizontal_size < 16 || vsh->vertical_size < 16) {
        ret = MPP_NOK;
        mpp_err_f("invalid sequence width(%d), height(%d).\n",
                  vsh->horizontal_size, vsh->vertical_size);
        goto __FAILED;
    }

    READ_BITS(bitctx, 2,  &vsh->chroma_format);
    if (vsh->chroma_format != CHROMA_420) {
        ret = MPP_NOK;
        mpp_err_f("chroma_format 0x%02x is not supported.\n", vsh->chroma_format);
        goto __FAILED;
    }
    READ_BITS(bitctx, 3, &vsh->sample_precision);
    if (vsh->profile_id == MAIN10_PROFILE) {
        READ_BITS(bitctx, 3, &vsh->encoding_precision);
    } else {
        vsh->encoding_precision = 1;
    }
    vsh->bit_depth = 6 + (vsh->encoding_precision << 1);

    if (vsh->sample_precision < 1 || vsh->sample_precision > 2) {
        ret = MPP_NOK;
        mpp_err_f("sample_precision 0x%02x is not supported.\n", vsh->sample_precision);
        goto __FAILED;
    }
    if (vsh->encoding_precision < 1 || vsh->encoding_precision > 2) {
        ret = MPP_NOK;
        mpp_err_f("encoding_precision 0x%02x is not supported.\n", vsh->encoding_precision);
        goto __FAILED;
    }

    READ_BITS(bitctx, 4, &vsh->aspect_ratio);
    READ_BITS(bitctx, 4, &vsh->frame_rate_code);
    READ_BITS(bitctx, 18, &val_temp); //!< bit_rate_lower_18
    vsh->bit_rate = val_temp << 12;
    READ_MARKER_BIT(bitctx);
    READ_BITS(bitctx, 12, &val_temp); //!< bit_rate_upper_12
    vsh->bit_rate += val_temp;
    READ_ONEBIT(bitctx, &vsh->low_delay);
    READ_MARKER_BIT(bitctx);
    READ_ONEBIT(bitctx, &vsh->enable_temporal_id);
    READ_BITS(bitctx, 18, &vsh->bbv_buffer_size);
    READ_BITS(bitctx, 3, &vsh->lcu_size);
    if (vsh->lcu_size < 4 || vsh->lcu_size > 6) {
        ret = MPP_NOK;
        mpp_err_f("invalid lcu size: %d\n", vsh->lcu_size);
        goto __FAILED;
    }

    READ_ONEBIT(bitctx, &vsh->enable_weighted_quant);
    if (vsh->enable_weighted_quant) {
        parse_sequence_wqm(bitctx, vsh);
    }

    READ_ONEBIT(bitctx, &val_temp);
    vsh->enable_background_picture = val_temp ^ 0x01;
    READ_ONEBIT(bitctx, &vsh->enable_mhp_skip);
    READ_ONEBIT(bitctx, &vsh->enable_dhp);
    READ_ONEBIT(bitctx, &vsh->enable_wsm);
    READ_ONEBIT(bitctx, &vsh->enable_amp);
    READ_ONEBIT(bitctx, &vsh->enable_nsqt);
    READ_ONEBIT(bitctx, &vsh->enable_nsip);
    READ_ONEBIT(bitctx, &vsh->enable_2nd_transform);
    READ_ONEBIT(bitctx, &vsh->enable_sao);
    READ_ONEBIT(bitctx, &vsh->enable_alf);
    READ_ONEBIT(bitctx, &vsh->enable_pmvr);
    READ_MARKER_BIT(bitctx);

    //!< parse rps
    READ_BITS(bitctx, 6, &vsh->num_of_rps);
    for (i = 0; i < vsh->num_of_rps; i++) {
        rps = &vsh->seq_rps[i];
        AVS2D_PARSE_TRACE("--------rcs[%d]--------", i);
        FUN_CHECK(ret = parse_one_rps(bitctx, rps));
    }

    if (vsh->low_delay == 0) {
        READ_BITS(bitctx, 5, &vsh->picture_reorder_delay);
    } else {
        vsh->picture_reorder_delay = 0;
    }
    READ_ONEBIT(bitctx, &vsh->enable_clf);
    READ_BITS(bitctx, 2, &val_temp); //!< reserved 2bits 00
    if (val_temp) {
        AVS2D_DBG(AVS2D_DBG_WARNNING, "reserver bits error.\n");
    }
    return ret;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET read_pic_alf_coeff(BitReadCtx_t *bitctx, RK_S32 *alf_coeff)
{
    MPP_RET ret = MPP_OK;
    RK_U32 j;
    RK_S32 alf_sum = 0;

    for (j = 0; j < ALF_MAX_COEFS; j++) {
        READ_SE(bitctx, &alf_coeff[j]);
        if ((j <= 7 && (alf_coeff[j] < -64 || alf_coeff[j] > 63)) ||
            (j == 8 && (alf_coeff[j] < -1088 || alf_coeff[j] > 1071))) {
            ret = MPP_NOK;
            mpp_err_f("invalid alf coeff(%d).\n", alf_coeff[j]);
            goto __FAILED;
        }

        if (j <= 7) {
            alf_sum += (2 * alf_coeff[j]); //!< avs2 9.12.2
        }
    }
    alf_coeff[8] += (64 - alf_sum);

__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET parse_pic_alf_params(BitReadCtx_t *bitctx, Avs2dPicHeader_t *ph)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i;

    READ_ONEBIT(bitctx, &ph->enable_pic_alf_y);
    READ_ONEBIT(bitctx, &ph->enable_pic_alf_cb);
    READ_ONEBIT(bitctx, &ph->enable_pic_alf_cr);

    if (ph->enable_pic_alf_y) {
        RK_U32 pre_symbole = 0;
        RK_U32 symbol = 0;

        READ_UE(bitctx, &ph->alf_filter_num);
        ph->alf_filter_num += 1;
        if (ph->alf_filter_num > ALF_MAX_FILTERS) {
            ret = MPP_NOK;
            mpp_err_f("invalid alf filter num(%d).\n", ph->alf_filter_num);
            goto __FAILED;
        }

        for (i = 0; i < ph->alf_filter_num; i++) {
            if (i > 0) {
                if (ph->alf_filter_num != 16) {
                    READ_UE(bitctx, &symbol);
                } else {
                    symbol = 1;
                }
                ph->alf_filter_pattern[symbol + pre_symbole] = 1;
                pre_symbole += symbol;
            }

            FUN_CHECK(ret = read_pic_alf_coeff(bitctx, ph->alf_coeff_y[i]));
        }

        if (pre_symbole > 15) {
            ret = MPP_NOK;
            mpp_err_f("invalid alf region distance.\n");
            goto __FAILED;
        }

        //!< @see avs2 9.12.2 eg: 0000111222223333
        ph->alf_coeff_idx_tab[0] = 0;
        for (i = 1; i < ALF_MAX_FILTERS; i++) {
            ph->alf_coeff_idx_tab[i] = (ph->alf_filter_pattern[i]) ?
                                       (ph->alf_coeff_idx_tab[i - 1] + 1) : ph->alf_coeff_idx_tab[i - 1];
        }
    }

    if (ph->enable_pic_alf_cb) {
        FUN_CHECK(ret = read_pic_alf_coeff(bitctx, ph->alf_coeff_cb));
    }

    if (ph->enable_pic_alf_cr) {
        FUN_CHECK(ret = read_pic_alf_coeff(bitctx, ph->alf_coeff_cr));
    }

__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

static MPP_RET parse_picture_header_comm(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 i, j;
    RK_U32 val_temp = 0;

    BitReadCtx_t *bitctx  = &p_dec->bitctx;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;

    AVS2D_PARSE_TRACE("In");
    READ_ONEBIT(bitctx, &val_temp);
    ph->enable_loop_filter = val_temp ^ 0x01;
    if (ph->enable_loop_filter) {
        READ_ONEBIT(bitctx, &ph->loop_filter_parameter_flag);
        if (ph->loop_filter_parameter_flag) {
            READ_SE(bitctx, &ph->alpha_c_offset);
            READ_SE(bitctx, &ph->beta_offset);
            if (ph->alpha_c_offset < -8 || ph->alpha_c_offset > 8 ||
                ph->beta_offset < -8 || ph->beta_offset > 8) {
                ret = MPP_NOK;
                mpp_err_f("invalid alpha_c_offset/beta_offset.\n");
                goto __FAILED;
            }
        } else {
            ph->alpha_c_offset = 0;
            ph->beta_offset    = 0;
        }
    }

    READ_ONEBIT(bitctx, &val_temp);
    ph->enable_chroma_quant_param = val_temp ^ 0x01;
    if (ph->enable_chroma_quant_param) {
        READ_SE(bitctx, &ph->chroma_quant_param_delta_cb);
        READ_SE(bitctx, &ph->chroma_quant_param_delta_cr);
    } else {
        ph->chroma_quant_param_delta_cb = 0;
        ph->chroma_quant_param_delta_cr = 0;
    }
    if (ph->chroma_quant_param_delta_cb < -16 || ph->chroma_quant_param_delta_cb > 16 ||
        ph->chroma_quant_param_delta_cr < -16 || ph->chroma_quant_param_delta_cr > 16) {
        ret = MPP_NOK;
        mpp_err_f("invalid chrome quant param delta.\n");
        goto __FAILED;
    }

    p_dec->enable_wq = 0;
    if (vsh->enable_weighted_quant) {
        READ_ONEBIT(bitctx, &ph->enable_pic_weight_quant);
        if (ph->enable_pic_weight_quant) {
            p_dec->enable_wq = 1;
            READ_BITS(bitctx, 2, &ph->pic_wq_data_index);
            if (ph->pic_wq_data_index == 1) {
                SKIP_BITS(bitctx, 1);
                READ_BITS(bitctx, 2, &ph->wq_param_index);
                READ_BITS(bitctx, 2, &ph->wq_model);
                if (ph->wq_param_index == 1) {
                    for (i = 0; i < 6; i++) {
                        READ_SE(bitctx, &ph->wq_param_delta1[i]);
                    }
                } else if (ph->wq_param_index == 2) {
                    for (i = 0; i < 6; i++) {
                        READ_SE(bitctx, &ph->wq_param_delta2[i]);
                    }
                }
            } else if (ph->pic_wq_data_index == 2) {
                RK_U32 size_id, wqm_size;
                for (size_id = 0; size_id < 2; size_id++) {
                    wqm_size = MPP_MIN(1 << (size_id + 2), 8);
                    for (j = 0; j < wqm_size; j++) {
                        for (i = 0; i < wqm_size; i++) {
                            READ_UE(bitctx, &ph->pic_wq_matrix[size_id][j * wqm_size + i]);
                        }
                    }
                }
            }

        }
    }

    if (vsh->enable_alf) {
        FUN_CHECK(ret = parse_pic_alf_params(bitctx, ph));
    }

    AVS2D_PARSE_TRACE("Out, ret %d\n", ret);
    return ret;
__BITREAD_ERR:
    AVS2D_PARSE_TRACE("Out, Bit read Err");
    return ret = bitctx->ret;
__FAILED:
    AVS2D_PARSE_TRACE("Out, Failed");
    return ret;
}

static MPP_RET parse_picture_header_intra(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 predict = 0;

    BitReadCtx_t *bitctx  = &p_dec->bitctx;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;

    AVS2D_PARSE_TRACE("In.");
    memset(ph, 0, sizeof(Avs2dPicHeader_t));
    ph->picture_type = I_PICTURE;

    AVS2D_PARSE_TRACE("Bitread buf %p, data %p, buf_len %d, left %d\n", bitctx->buf, bitctx->data_, bitctx->buf_len, bitctx->bytes_left_);
    READ_BITS_LONG(bitctx, 32, &ph->bbv_delay);
    AVS2D_PARSE_TRACE("bbv_delay %d\n", ph->bbv_delay);
    READ_ONEBIT(bitctx, &ph->time_code_flag);
    AVS2D_PARSE_TRACE("time_code_flag %d\n", ph->time_code_flag);
    if (ph->time_code_flag) {
        READ_BITS(bitctx, 24, &ph->time_code);
        AVS2D_PARSE_TRACE("time_code %d\n", ph->time_code);
    }

    if (vsh->enable_background_picture) {
        READ_ONEBIT(bitctx, &ph->background_picture_flag);
        AVS2D_PARSE_TRACE("background_picture_flag %d\n", ph->background_picture_flag);
        if (ph->background_picture_flag) {
            READ_ONEBIT(bitctx, &ph->background_picture_output_flag);
            AVS2D_PARSE_TRACE("background_picture_output_flag %d\n", ph->background_picture_output_flag);
            if (ph->background_picture_output_flag) {
                ph->picture_type = G_PICTURE;
            } else {
                ph->picture_type = GB_PICTURE;
            }
        }
    }

    READ_BITS(bitctx, 8, &ph->doi);
    AVS2D_PARSE_TRACE("Picture type %d doi %d\n", ph->picture_type, ph->doi);
    if (vsh->enable_temporal_id) {
        READ_BITS(bitctx, 3, &ph->temporal_id);
        AVS2D_PARSE_TRACE("temporal_id %d\n", ph->temporal_id);
    }

    if (vsh->low_delay == 0 && (!ph->background_picture_flag || ph->background_picture_output_flag)) {
        READ_UE(bitctx, &ph->picture_output_delay);
        AVS2D_PARSE_TRACE("picture_output_delay %d\n", ph->picture_output_delay);
        if (ph->picture_output_delay >= 64) {
            ret = MPP_NOK;
            mpp_err_f("invalid picture output delay(%d) intra.\n", ph->picture_output_delay);
            goto __FAILED;
        }
    } else {
        ph->picture_output_delay = 0;
    }

    READ_ONEBIT(bitctx, &predict);
    AVS2D_PARSE_TRACE("predict %d\n", predict);
    if (predict) {
        RK_U8 rcs_index = 0;
        READ_BITS(bitctx, 5, &rcs_index);
        AVS2D_PARSE_TRACE("rcs_index %d\n", rcs_index);
        p_dec->frm_mgr.cur_rps = vsh->seq_rps[rcs_index];
    } else {
        FUN_CHECK(ret = parse_one_rps(bitctx, &p_dec->frm_mgr.cur_rps));
    }

    if (vsh->low_delay) {
        READ_UE(bitctx, &ph->bbv_check_times);
        AVS2D_PARSE_TRACE("bbv_check_time %d\n", ph->bbv_check_times);
    }

    READ_ONEBIT(bitctx, &ph->progressive_frame);
    AVS2D_PARSE_TRACE("progressive_frame %d\n", ph->progressive_frame);
    if (!ph->progressive_frame) {
        READ_ONEBIT(bitctx, &ph->picture_structure);
        AVS2D_PARSE_TRACE("picture_structure %d\n", ph->picture_structure);
    } else {
        ph->picture_structure = 1; //!< frame picture
    }
    READ_ONEBIT(bitctx, &ph->top_field_first);
    AVS2D_PARSE_TRACE("top_field_first %d\n", ph->top_field_first);
    READ_ONEBIT(bitctx, &ph->repeat_first_field);
    AVS2D_PARSE_TRACE("repeat_first_field %d\n", ph->repeat_first_field);
    if (vsh->field_coded_sequence == 1) { //!< field picture
        READ_ONEBIT(bitctx, &ph->is_top_field);
        AVS2D_PARSE_TRACE("is_top_field %d\n", ph->is_top_field);
        SKIP_BITS(bitctx, 1);
        AVS2D_PARSE_TRACE("skip bits\n");
    }
    READ_ONEBIT(bitctx, &ph->fixed_picture_qp);
    AVS2D_PARSE_TRACE("fixed_picture_qp %d\n", ph->fixed_picture_qp);
    READ_BITS(bitctx, 7, &ph->picture_qp);
    AVS2D_PARSE_TRACE("picture_qp %d\n", ph->picture_qp);
    if (ph->picture_qp > (63 + 8 * (vsh->bit_depth - 8))) {
        ret = MPP_NOK;
        mpp_err_f("invalid picture qp(%d).\n", ph->picture_qp);
        goto __FAILED;
    }

    FUN_CHECK(ret = parse_picture_header_comm(p_dec));

    AVS2D_PARSE_TRACE("Out. ret %d", ret);
    return ret;
__BITREAD_ERR:
    AVS2D_PARSE_TRACE("Out. Bit read ERR");
    return ret = bitctx->ret;
__FAILED:
    AVS2D_PARSE_TRACE("Out. Failed");
    return ret;
}

static MPP_RET parse_picture_header_inter(Avs2dCtx_t *p_dec)
{
    MPP_RET ret = MPP_OK;
    RK_U32 predict = 0;

    BitReadCtx_t *bitctx  = &p_dec->bitctx;
    Avs2dSeqHeader_t *vsh = &p_dec->vsh;
    Avs2dPicHeader_t *ph  = &p_dec->ph;

    memset(ph, 0, sizeof(Avs2dPicHeader_t));
    READ_BITS_LONG(bitctx, 32, &ph->bbv_delay);
    READ_BITS(bitctx, 2, &ph->picture_coding_type);
    if (vsh->enable_background_picture) {
        if (ph->picture_coding_type == 1) {
            READ_ONEBIT(bitctx, &ph->background_pred_flag);
        }

        if (ph->picture_coding_type != 2 && !ph->background_pred_flag) {
            READ_ONEBIT(bitctx, &ph->background_reference_flag); //!< P/F ref G/GB
        }
    }

    if (ph->picture_coding_type == 1) {
        ph->picture_type = ph->background_pred_flag ? S_PICTURE : P_PICTURE;
    } else if (ph->picture_coding_type == 2) {
        ph->picture_type = B_PICTURE;
    } else if (ph->picture_coding_type == 3) {
        ph->picture_type = F_PICTURE;
    } else {
        ret = MPP_NOK;
        mpp_err_f("invalid picture coding type(0).\n");
        goto __FAILED;
    }

    READ_BITS(bitctx, 8, &ph->doi);
    AVS2D_PARSE_TRACE("Picture type %d doi %d\n", ph->picture_type, ph->doi);
    if (vsh->enable_temporal_id) {
        READ_BITS(bitctx, 3, &ph->temporal_id);
    }

    if (vsh->low_delay == 0) {
        READ_UE(bitctx, &ph->picture_output_delay);
        if (ph->picture_output_delay >= 64) {
            ret = MPP_NOK;
            mpp_err_f("invalid picture output delay(%d) intra.\n", ph->picture_output_delay);
            goto __FAILED;
        }
    } else {
        ph->picture_output_delay = 0;
    }

    READ_ONEBIT(bitctx, &predict);
    if (predict) {
        RK_U8 rcs_index = 0;
        READ_BITS(bitctx, 5, &rcs_index);
        p_dec->frm_mgr.cur_rps = vsh->seq_rps[rcs_index];
    } else {
        FUN_CHECK(ret = parse_one_rps(bitctx, &p_dec->frm_mgr.cur_rps));
    }

    if (vsh->low_delay) {
        READ_UE(bitctx, &ph->bbv_check_times);
    }

    READ_ONEBIT(bitctx, &ph->progressive_frame);
    if (!ph->progressive_frame) {
        READ_ONEBIT(bitctx, &ph->picture_structure);
    } else {
        ph->picture_structure = 1; //!< frame picture
    }
    READ_ONEBIT(bitctx, &ph->top_field_first);
    READ_ONEBIT(bitctx, &ph->repeat_first_field);
    if (vsh->field_coded_sequence == 1) { //!< field picture
        READ_ONEBIT(bitctx, &ph->is_top_field);
        SKIP_BITS(bitctx, 1);
    }
    READ_ONEBIT(bitctx, &ph->fixed_picture_qp);
    READ_BITS(bitctx, 7, &ph->picture_qp);
    if (ph->picture_qp > (63 + 8 * (vsh->bit_depth - 8))) {
        ret = MPP_NOK;
        mpp_err_f("invalid picture qp(%d).\n", ph->picture_qp);
        goto __FAILED;
    }
    if (ph->picture_coding_type != 2 || ph->picture_structure == 0) {
        SKIP_BITS(bitctx, 1);
    }
    READ_ONEBIT(bitctx, &ph->enable_random_decodable);

    FUN_CHECK(ret = parse_picture_header_comm(p_dec));

    return ret;
__BITREAD_ERR:
    return ret = bitctx->ret;
__FAILED:
    return ret;
}

MPP_RET avs2d_parse_picture_header(Avs2dCtx_t *p_dec, RK_U32 startcode)
{
    MPP_RET ret = MPP_OK;

    AVS2D_PARSE_TRACE("In.");
    switch (startcode) {
    case AVS2_I_PICTURE_START_CODE:
        ret = parse_picture_header_intra(p_dec);
        break;
    case AVS2_PB_PICTURE_START_CODE:
        ret = parse_picture_header_inter(p_dec);
        break;
    default:
        ret = MPP_NOK;
        mpp_err_f("invalid picture startcode(%d).\n", startcode);
        return ret;
    }

    wq_init_frame_quant_param(p_dec);
    wq_update_frame_matrix(p_dec);

    AVS2D_PARSE_TRACE("Out.");
    return ret;
}


