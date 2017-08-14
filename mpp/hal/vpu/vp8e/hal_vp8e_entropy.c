/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "hal_vp8e_entropy"

#include <string.h>

#include "mpp_common.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_entropy.h"
#include "hal_vp8e_putbit.h"
#include "hal_vp8e_table.h"

#define COST_BOOL(prob, bin)\
        ((bin) ? vp8_prob_cost_tbl[255 - (prob)] : vp8_prob_cost_tbl[prob])

static RK_U32 calc_mvprob(RK_U32 left, RK_U32 right, RK_U32 prob)
{
    RK_U32 p;

    if (left + right) {
        p = (left * 255) / (left + right);
        p &= -2;
        if (!p)
            p = 1;
    } else
        p = prob;

    return p;
}

static RK_U32 update_prob(RK_U32 prob, RK_U32 left, RK_U32 right,
                          RK_U32 old_prob, RK_U32 new_prob, RK_U32 fixed)
{
    RK_S32 u, s;

    u = (RK_S32)fixed + ((vp8_prob_cost_tbl[255 - prob] - vp8_prob_cost_tbl[prob]) >> 8);
    s = ((RK_S32)left * (vp8_prob_cost_tbl[old_prob] - vp8_prob_cost_tbl[new_prob]) +
         (RK_S32)right * (vp8_prob_cost_tbl[255 - old_prob] - vp8_prob_cost_tbl[255 - new_prob])) >> 8;

    return (s > u);
}

static RK_S32 get_cost_tree(Vp8eTree const *tree, RK_S32 *prob)
{
    RK_S32 bit_cost = 0;
    RK_S32 value = tree->value;
    RK_S32 number = tree->number;
    RK_S32 const *index = tree->index;

    while (number--) {
        bit_cost += COST_BOOL(prob[*index++], (value >> number) & 1);
    }

    return bit_cost;
}

MPP_RET vp8e_init_entropy(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;
    Vp8eHalEntropy *entropy = &ctx->entropy;

    entropy->update_coeff_prob_flag = 0;
    if (!ctx->sps.refresh_entropy || ctx->picbuf.cur_pic->i_frame) {
        if (!entropy->default_coeff_prob_flag) {
            memcpy(entropy->coeff_prob, default_prob_coeff_tbl, sizeof(default_prob_coeff_tbl));
            entropy->update_coeff_prob_flag = 1;
        }
        memcpy(entropy->mv_prob, default_prob_mv_tbl, sizeof(default_prob_mv_tbl));
        entropy->default_coeff_prob_flag = 1;
    }

    memcpy(entropy->old_coeff_prob, entropy->coeff_prob, sizeof(entropy->coeff_prob));
    if (ctx->frame_cnt == 0 || ctx->key_frm_next)
        memcpy(entropy->old_mv_prob, entropy->mv_prob, sizeof(entropy->mv_prob));

    entropy->skip_false_prob = default_skip_false_prob_tbl[ctx->rc->qp_hdr];

    if (ctx->frame_cnt == 0)
        return MPP_OK;

    if ((!ctx->picbuf.cur_pic->ipf) && (!ctx->picbuf.cur_pic->grf) &&
        (!ctx->picbuf.cur_pic->arf))
        return MPP_OK;

    if (ctx->prev_frame_lost)
        return MPP_OK;
    {
        RK_S32 i, j, k, l;

        RK_U32 p, left, right;
        RK_U32 old_p, upd_p = 0;

        RK_U32 type;
        RK_U32 branch_cnt[2];
        RK_U16 *p_tmp = NULL;

        RK_U16 *p_cnt = (RK_U16 *)mpp_buffer_get_ptr(buffers->hw_prob_count_buf);

        for (i = 0; i < 4; i++) {
            for (j = 0; j < 7; j++) {
                for (k = 0; k < 3; k++) {
                    RK_S32 tmp, ii;

                    tmp = i * 7 * 3 + j * 3 + k;
                    tmp += 2 * 4 * 7 * 3;
                    ii = offset_tbl[tmp];

                    right = ii >= 0 ? p_cnt[ii] : 0;

                    for (l = 2; l--;) {
                        old_p = entropy->coeff_prob[i][j][k][l];
                        old_p = coeff_update_prob_tbl[i][j][k][l];

                        tmp -= 4 * 7 * 3;
                        ii = offset_tbl[tmp];
                        left = ii >= 0 ? p_cnt[ii] : 0;
                        if (left + right) {
                            p = ((left * 256) + ((left + right) >> 1)) / (left + right);
                            if (p > 255) p = 255;
                        } else
                            p = old_p;

                        if (update_prob(upd_p, left, right, old_p, p, 8)) {
                            entropy->coeff_prob[i][j][k][l] = p;
                            entropy->update_coeff_prob_flag = 1;
                        }
                        right += left;
                    }
                }
            }
        }

        if (entropy->update_coeff_prob_flag)
            entropy->default_coeff_prob_flag = 0;

        p_tmp = p_cnt + VP8_PROB_COUNT_MV_OFFSET;
        for (i = 0; i < 2; i++) {
            left  = *p_tmp++;
            right = *p_tmp++;

            p = calc_mvprob(left, right, entropy->old_mv_prob[i][0]);

            if (update_prob(mv_update_prob_tbl[i][0], left, right,
                            entropy->old_mv_prob[i][0], p, 6))
                entropy->mv_prob[i][0] = p;

            right += left;
            left = *p_tmp++;
            right -= left - p_tmp[0];

            p = calc_mvprob(left, right, entropy->old_mv_prob[i][1]);
            if (update_prob(mv_update_prob_tbl[i][1], left, right,
                            entropy->old_mv_prob[i][1], p, 6))
                entropy->mv_prob[i][1] = p;

            for (j = 0; j < 2; j++) {
                left = *p_tmp++;
                right = *p_tmp++;
                p = calc_mvprob(left, right, entropy->old_mv_prob[i][4 + j]);
                if (update_prob(mv_update_prob_tbl[i][4 + j], left, right,
                                entropy->old_mv_prob[i][4 + j], p, 6))
                    entropy->mv_prob[i][4 + j] = p;
                branch_cnt[j] = left + right;
            }

            p = calc_mvprob(branch_cnt[0], branch_cnt[1], entropy->old_mv_prob[i][3]);
            if (update_prob(mv_update_prob_tbl[i][3], branch_cnt[0], branch_cnt[1],
                            entropy->old_mv_prob[i][3], p, 6))
                entropy->mv_prob[i][3] = p;

            type = branch_cnt[0] + branch_cnt[1];

            for (j = 0; j < 2; j++) {
                left = *p_tmp++;
                right = *p_tmp++;
                p = calc_mvprob(left, right, entropy->old_mv_prob[i][7 + j]);
                if (update_prob(mv_update_prob_tbl[i][7 + j], left, right,
                                entropy->old_mv_prob[i][7 + j], p, 6))
                    entropy->mv_prob[i][7 + j] = p;
                branch_cnt[j] = left + right;
            }

            p = calc_mvprob(branch_cnt[0], branch_cnt[1], entropy->old_mv_prob[i][6]);
            if (update_prob(mv_update_prob_tbl[i][6], branch_cnt[0], branch_cnt[1],
                            entropy->old_mv_prob[i][6], p, 6))
                entropy->mv_prob[i][6] = p;

            p = calc_mvprob(type, branch_cnt[0] + branch_cnt[1],
                            entropy->old_mv_prob[i][2]);
            if (update_prob(mv_update_prob_tbl[i][2], type, branch_cnt[0] + branch_cnt[1],
                            entropy->old_mv_prob[i][2], p, 6))
                entropy->mv_prob[i][2] = p;
        }
    }
    {
        entropy->last_prob = 255;
        entropy->gf_prob = 128;
        memcpy(entropy->y_mode_prob, y_mode_prob_tbl, sizeof(y_mode_prob_tbl));
        memcpy(entropy->uv_mode_prob, uv_mode_prob_tbl, sizeof(uv_mode_prob_tbl));
    }
    return MPP_OK;
}

MPP_RET vp8e_swap_endian(RK_U32 *buf, RK_U32 bytes)
{
    RK_U32 i = 0;
    RK_S32 words = bytes / 4;

    while (words > 0) {
        RK_U32 val = buf[i];
        RK_U32 tmp = 0;

        tmp |= (val & 0xFF) << 24;
        tmp |= (val & 0xFF00) << 8;
        tmp |= (val & 0xFF0000) >> 8;
        tmp |= (val & 0xFF000000) >> 24;

        {
            RK_U32 val2 = buf[i + 1];
            RK_U32 tmp2 = 0;

            tmp2 |= (val2 & 0xFF) << 24;
            tmp2 |= (val2 & 0xFF00) << 8;
            tmp2 |= (val2 & 0xFF0000) >> 8;
            tmp2 |= (val2 & 0xFF000000) >> 24;

            buf[i] = tmp2;
            words--;
            i++;
        }

        buf[i] = tmp;
        words--;
        i++;
    }
    return MPP_OK;
}


MPP_RET vp8e_write_entropy_tables(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;
    Vp8eHalEntropy *entropy = &ctx->entropy;

    RK_U8 *table = (RK_U8 *)mpp_buffer_get_ptr(buffers->hw_cabac_table_buf);
    /* Write probability tables to ASIC linear memory, reg + mem */
    memset(table, 0, 56);  // use 56 bytes
    table[0] = entropy->skip_false_prob;
    table[1] = entropy->intra_prob;
    table[2] = entropy->last_prob;
    table[3] = entropy->gf_prob;
    table[4] = entropy->segment_prob[0];
    table[5] = entropy->segment_prob[1];
    table[6] = entropy->segment_prob[2];

    table[8]  = entropy->y_mode_prob[0];
    table[9]  = entropy->y_mode_prob[1];
    table[10] = entropy->y_mode_prob[2];
    table[11] = entropy->y_mode_prob[3];
    table[12] = entropy->uv_mode_prob[0];
    table[13] = entropy->uv_mode_prob[1];
    table[14] = entropy->uv_mode_prob[2];

    /* MV probabilities x+y: short, sign, size 8-9 */
    table[16] = entropy->mv_prob[1][0];
    table[17] = entropy->mv_prob[0][0];
    table[18] = entropy->mv_prob[1][1];
    table[19] = entropy->mv_prob[0][1];
    table[20] = entropy->mv_prob[1][17];
    table[21] = entropy->mv_prob[1][18];
    table[22] = entropy->mv_prob[0][17];
    table[23] = entropy->mv_prob[0][18];
    {
        RK_S32 i, j, k, l;
        /* MV X size */
        for (i = 0; i < 8; i++)
            table[24 + i] = entropy->mv_prob[1][9 + i];

        /* MV Y size */
        for (i = 0; i < 8; i++)
            table[32 + i] = entropy->mv_prob[0][9 + i];

        /* MV X short tree */
        for (i = 0; i < 7; i++)
            table[40 + i] = entropy->mv_prob[1][2 + i];

        /* MV Y short tree */
        for (i = 0; i < 7; i++)
            table[48 + i] = entropy->mv_prob[0][2 + i];

        /* Update the ASIC table when needed. */
        if (entropy->update_coeff_prob_flag) {
            table += 56;
            /* DCT coeff probabilities 0-2, two fields per line. */
            for (i = 0; i < 4; i++)
                for (j = 0; j < 8; j++)
                    for (k = 0; k < 3; k++) {
                        for (l = 0; l < 3; l++) {
                            *table++ = entropy->coeff_prob[i][j][k][l];
                        }
                        *table++ = 0;
                    }

            /* ASIC second probability table in ext mem.
             * DCT coeff probabilities 4 5 6 7 8 9 10 3 on each line.
             * coeff 3 was moved from first table to second so it is last. */
            for (i = 0; i < 4; i++)
                for (j = 0; j < 8; j++)
                    for (k = 0; k < 3; k++) {
                        for (l = 4; l < 11; l++) {
                            *table++ = entropy->coeff_prob[i][j][k][l];
                        }
                        *table++ = entropy->coeff_prob[i][j][k][3];
                    }
        }
    }
    table = mpp_buffer_get_ptr(buffers->hw_cabac_table_buf);
    if (entropy->update_coeff_prob_flag)
        vp8e_swap_endian((RK_U32 *) table, 56 + 8 * 48 + 8 * 96);
    else
        vp8e_swap_endian((RK_U32 *) table, 56);

    return MPP_OK;
}

RK_S32 vp8e_calc_cost_mv(RK_S32 mvd, RK_S32 *mv_prob)
{
    RK_S32 i = 0;
    RK_S32 bit_cost = 0;

    RK_S32 tmp = MPP_ABS(mvd >> 1);

    if (tmp < 8) {
        bit_cost += COST_BOOL(mv_prob[0], 0);
        bit_cost += get_cost_tree(&mv_tree_tbl[tmp], mv_prob + 2);
        if (!tmp)
            return bit_cost;

        /* Sign */
        bit_cost += COST_BOOL(mv_prob[1], mvd < 0);
        return bit_cost;
    }

    bit_cost += COST_BOOL(mv_prob[0], 1);

    for (i = 0; i < 3; i++) {
        bit_cost += COST_BOOL(mv_prob[9 + i], (tmp >> i) & 1);
    }

    for (i = 9; i > 3; i--) {
        bit_cost += COST_BOOL(mv_prob[9 + i], (tmp >> i) & 1);
    }

    if (tmp > 15) {
        bit_cost += COST_BOOL(mv_prob[9 + 3], (tmp >> 3) & 1);
    }

    bit_cost += COST_BOOL(mv_prob[1], mvd < 0);

    return bit_cost;
}

MPP_RET vp8e_calc_coeff_prob(Vp8ePutBitBuf *bitbuf, RK_S32 (*curr)[4][8][3][11],
                             RK_S32 (*prev)[4][8][3][11])
{
    RK_S32 i, j, k, l;
    RK_S32 prob, new, old;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 8; j++) {
            for (k = 0; k < 3; k++) {
                for (l = 0; l < 11; l++) {
                    prob = coeff_update_prob_tbl[i][j][k][l];
                    old = (RK_S32) (*prev)[i][j][k][l];
                    new = (RK_S32) (*curr)[i][j][k][l];

                    if (new == old) {
                        vp8e_put_bool(bitbuf, prob, 0);
                    } else {
                        vp8e_put_bool(bitbuf, prob, 1);
                        vp8e_put_lit(bitbuf, new, 8);
                    }
                }
            }
        }
    }
    return MPP_OK;
}

MPP_RET vp8e_calc_mv_prob(Vp8ePutBitBuf *bitbuf, RK_S32 (*curr)[2][19],
                          RK_S32 (*prev)[2][19])
{
    RK_S32 i, j;
    RK_S32 prob, new, old;

    for (i = 0; i < 2; i++) {
        for (j = 0; j < 19; j++) {
            prob = mv_update_prob_tbl[i][j];
            old = (RK_S32) (*prev)[i][j];
            new = (RK_S32) (*curr)[i][j];

            if (new == old) {
                vp8e_put_bool(bitbuf, prob, 0);
            } else {
                vp8e_put_bool(bitbuf, prob, 1);
                vp8e_put_lit(bitbuf, new >> 1, 7);
            }
        }
    }
    return MPP_OK;
}
