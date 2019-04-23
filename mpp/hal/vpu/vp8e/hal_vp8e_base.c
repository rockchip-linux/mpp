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

#define MODULE_TAG "hal_vp8e_base"

#include <string.h>

#include "mpp_log.h"
#include "mpp_hal.h"
#include "mpp_buffer.h"
#include "mpp_common.h"

#include "hal_vp8e_base.h"
#include "hal_vp8e_putbit.h"
#include "hal_vp8e_table.h"
#include "hal_vp8e_debug.h"

static MPP_RET set_frame_params(void *hal)
{

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    {
        RK_S32 i;
        Pps *pps = ctx->ppss.pps;
        Vp8eSps *sps = &ctx->sps;

        for (i = 0; i < 4; i++) {
            pps->qp_sgm[i] = ctx->rc->qp_hdr;
            pps->level_sgm[i] = sps->filter_level;
        }
    }

    return MPP_OK;
}

static MPP_RET set_filter(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eSps *sps = &ctx->sps;

    if (sps->auto_filter_level) {
        RK_U32 qp = 36;
        Pps *p_pps = ctx->ppss.pps;
        if (ctx->frame_type == VP8E_FRM_KEY) {
            RK_S32 tmp = (qp * 64) / 128 + 8;
            sps->filter_level = MPP_CLIP3(0, 63, tmp);
            p_pps->level_sgm[0] = MPP_CLIP3(0, 63, (p_pps->qp_sgm[0] * 64) / 128 + 8);
            p_pps->level_sgm[1] = MPP_CLIP3(0, 63, (p_pps->qp_sgm[1] * 64) / 128 + 8);
            p_pps->level_sgm[2] = MPP_CLIP3(0, 63, (p_pps->qp_sgm[2] * 64) / 128 + 8);
            p_pps->level_sgm[3] = MPP_CLIP3(0, 63, (p_pps->qp_sgm[3] * 64) / 128 + 8);
        } else {
            sps->filter_level = inter_level_tbl[qp];
            p_pps->level_sgm[0] = inter_level_tbl[p_pps->qp_sgm[0]];
            p_pps->level_sgm[1] = inter_level_tbl[p_pps->qp_sgm[1]];
            p_pps->level_sgm[2] = inter_level_tbl[p_pps->qp_sgm[2]];
            p_pps->level_sgm[3] = inter_level_tbl[p_pps->qp_sgm[3]];
        }
    }

    if (sps->auto_filter_sharpness) {
        sps->filter_sharpness = 0;
    }

    if (!sps->filter_delta_enable)
        return MPP_NOK;

    if (sps->filter_delta_enable == 2) {
        sps->filter_delta_enable = 1;
        return MPP_NOK;
    }

    if (sps->filter_level == 0) {
        sps->ref_delta[0] =  0;      /* Intra frame */
        sps->ref_delta[1] =  0;      /* Last frame */
        sps->ref_delta[2] =  0;      /* Golden frame */
        sps->ref_delta[3] =  0;      /* Altref frame */
        sps->mode_delta[0] = 0;      /* BPRED */
        sps->mode_delta[1] = 0;      /* Zero */
        sps->mode_delta[2] = 0;      /* New mv */
        sps->mode_delta[3] = 0;      /* Split mv */
        return MPP_NOK;
    }

    if (!ctx->picbuf.cur_pic->ipf && !ctx->picbuf.cur_pic->grf &&
        !ctx->picbuf.cur_pic->arf) {
        memcpy(sps->ref_delta, sps->old_ref_delta, sizeof(sps->ref_delta));
        memcpy(sps->mode_delta, sps->old_mode_delta, sizeof(sps->mode_delta));
        return MPP_NOK;
    }

    sps->ref_delta[0] =  2;      /* Intra frame */
    sps->ref_delta[1] =  0;      /* Last frame */
    sps->ref_delta[2] = -2;      /* Golden frame */
    sps->ref_delta[3] = -2;      /* Altref frame */

    sps->mode_delta[0] =  4;     /* BPRED */
    sps->mode_delta[1] = -2;     /* Zero */
    sps->mode_delta[2] =  2;     /* New mv */
    sps->mode_delta[3] =  4;     /* Split mv */

    {
        RK_U32 i = 0;
        for (i = 0; i < 4; i++) {
            sps->ref_delta[i] = MPP_CLIP3(-0x3f, 0x3f, sps->ref_delta[i]);
            sps->mode_delta[i] = MPP_CLIP3(-0x3f, 0x3f, sps->mode_delta[i]);
        }
    }
    return MPP_OK;
}

static MPP_RET set_segmentation(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8ePps *ppss = &ctx->ppss;
    Pps *pps = ppss->pps;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;

    {
        RK_S32 qp = ctx->rc->qp_hdr;

        if (hw_cfg->roi1_delta_qp)
            pps->qp_sgm[1] = MPP_CLIP3(0, 127, qp - hw_cfg->roi1_delta_qp);

        if (hw_cfg->roi2_delta_qp)
            pps->qp_sgm[2] = MPP_CLIP3(0, 127, qp - hw_cfg->roi2_delta_qp);
    }

    {
        RK_U32 x, y, mb, mask, id;
        RK_U32 *map = mpp_buffer_get_ptr(buffers->hw_segment_map_buf);
        RK_U32 *map_bck = map;
        RK_U32 mapSize = (ctx->mb_per_frame + 15) / 16 * 8;

        if (hw_cfg->roi1_delta_qp || hw_cfg->roi2_delta_qp) {
            pps->segment_enabled = 1;

            memset(pps->sgm.id_cnt, 0, sizeof(pps->sgm.id_cnt));

            for (y = 0, mb = 0, mask = 0; y < ctx->mb_per_col; y++) {
                for (x = 0; x < ctx->mb_per_row; x++) {
                    id = 0;
                    if ((x >= hw_cfg->roi1_left) && (x <= hw_cfg->roi1_right) &&
                        (y >= hw_cfg->roi1_top) && (y <= hw_cfg->roi1_bottom))
                        id = 1;
                    if ((x >= hw_cfg->roi1_left) && (x <= hw_cfg->roi2_right) &&
                        (y >= hw_cfg->roi2_top) && (y <= hw_cfg->roi2_bottom))
                        id = 2;

                    pps->sgm.id_cnt[id]++;

                    mask |= id << (28 - 4 * (mb % 8));
                    if ((mb % 8) == 7) {
                        *map++ = mask;
                        mask = 0;
                    }
                    mb++;
                }
            }
            *map++ = mask;
            vp8e_swap_endian(map_bck, mapSize);
        } else if (pps->segment_enabled && pps->sgm.map_modified) {
            memset(pps->sgm.id_cnt, 0, sizeof(pps->sgm.id_cnt));

            for (mb = 0, mask = 0; mb < mapSize / 4; mb++) {
                mask = map[mb];
                for (x = 0; x < 8; x++) {
                    if (mb * 8 + x < ctx->mb_per_frame) {
                        id = (mask >> (28 - 4 * x)) & 0xF;
                        pps->sgm.id_cnt[id]++;
                    }
                }
            }
            vp8e_swap_endian(map_bck, mapSize);
        }
    }
    if (ctx->picbuf.cur_pic->i_frame || !pps->segment_enabled) {
        memset(ppss->qp_sgm, 0xff, sizeof(ppss->qp_sgm));
        memset(ppss->level_sgm, 0xff, sizeof(ppss->level_sgm));
        ppss->prev_pps = NULL;
    } else
        ppss->prev_pps = ppss->pps;

    return MPP_OK;
}

static void set_intra_pred_penalties(Vp8eHwCfg *hw_cfg, RK_U32 qp)
{
    RK_S32 i, tmp;

    /* Intra 4x4 mode */
    tmp = qp * 2 + 8;
    for (i = 0; i < 10; i++) {
        hw_cfg->intra_b_mode_penalty[i] = (intra4_mode_tree_penalty_tbl[i] * tmp) >> 8;
    }

    /* Intra 16x16 mode */
    tmp = qp * 2 + 64;
    for (i = 0; i < 4; i++) {
        hw_cfg->intra_mode_penalty[i] = (intra16_mode_tree_penalty_tbl[i] * tmp) >> 8;
    }

    /* If favor has not been set earlier by testId use default */
    if (hw_cfg->intra_16_favor == -1)
        hw_cfg->intra_16_favor = qp * 1024 / 128;
}

static void set_hdr_segmentation(Vp8ePutBitBuf *bitbuf, Vp8ePps *ppss,
                                 Vp8eHalEntropy *entropy)
{
    RK_S32 i, tmp;
    RK_U8 data_modified = 0;

    Pps  *pps = ppss->pps;
    Sgm  *sgm = &ppss->pps->sgm;

    if (memcmp(ppss->qp_sgm, pps->qp_sgm, sizeof(ppss->qp_sgm)))
        data_modified = 1;

    if (memcmp(ppss->level_sgm, pps->level_sgm, sizeof(ppss->level_sgm)))
        data_modified = 1;

    if (!ppss->prev_pps) {
        sgm->map_modified = 1;
    }

    vp8e_put_lit(bitbuf, sgm->map_modified, 1);
    vp8e_put_lit(bitbuf, data_modified, 1);

    if (data_modified) {
        vp8e_put_lit(bitbuf, 1, 1);

        for (i = 0; i < SGM_CNT; i++) {
            tmp = pps->qp_sgm[i];
            vp8e_put_lit(bitbuf, 1, 1);
            vp8e_put_lit(bitbuf, MPP_ABS(tmp), 7);
            vp8e_put_lit(bitbuf, tmp < 0, 1);
        }

        for (i = 0; i < SGM_CNT; i++) {
            tmp = pps->level_sgm[i];
            vp8e_put_lit(bitbuf, 1, 1);
            vp8e_put_lit(bitbuf, MPP_ABS(tmp), 6);
            vp8e_put_lit(bitbuf, tmp < 0, 1);
        }
    }

    if (sgm->map_modified) {
        RK_S32 sum1 = sgm->id_cnt[0] + sgm->id_cnt[1];
        RK_S32 sum2 = sgm->id_cnt[2] + sgm->id_cnt[3];

        tmp = 255 * sum1 / (sum1 + sum2);
        entropy->segment_prob[0] = MPP_CLIP3(1, 255, tmp);

        tmp = sum1 ? 255 * sgm->id_cnt[0] / sum1 : 255;
        entropy->segment_prob[1] = MPP_CLIP3(1, 255, tmp);

        tmp = sum2 ? 255 * sgm->id_cnt[2] / sum2 : 255;
        entropy->segment_prob[2] = MPP_CLIP3(1, 255, tmp);

        for (i = 0; i < 3; i++) {
            if (sgm->id_cnt[i] != 0) {
                vp8e_put_lit(bitbuf, 1, 1);
                vp8e_put_lit(bitbuf, entropy->segment_prob[i], 8);
            } else {
                vp8e_put_lit(bitbuf, 0, 1);
            }
        }
    }

    memcpy(ppss->qp_sgm, pps->qp_sgm, sizeof(ppss->qp_sgm));
    memcpy(ppss->level_sgm, pps->level_sgm, sizeof(ppss->level_sgm));
}

static void set_filter_level_delta(Vp8ePutBitBuf *bitbuf, Vp8eSps *sps)
{
    RK_S32 i, tmp;
    RK_U8  update = 0;
    RK_S32 mode_update[4];
    RK_S32 ref_update[4];

    for (i = 0; i < 4; i++) {
        mode_update[i] = sps->mode_delta[i] != sps->old_mode_delta[i];
        ref_update[i] = sps->ref_delta[i] != sps->old_ref_delta[i];
        if (mode_update[i] || ref_update[i])
            update = 1;
    }

    if (!sps->refresh_entropy)
        update = 1;

    vp8e_put_lit(bitbuf, update, 1);
    if (!update)
        return;

    for (i = 0; i < 4; i++) {
        vp8e_put_lit(bitbuf, ref_update[i], 1);
        if (ref_update[i]) {
            tmp = sps->ref_delta[i];
            vp8e_put_lit(bitbuf, MPP_ABS(tmp), 6);
            vp8e_put_lit(bitbuf, tmp < 0, 1);
        }
    }

    for (i = 0; i < 4; i++) {
        vp8e_put_lit(bitbuf, mode_update[i], 1);
        if (mode_update[i]) {
            tmp = sps->mode_delta[i];
            vp8e_put_lit(bitbuf, MPP_ABS(tmp), 6);
            vp8e_put_lit(bitbuf, tmp < 0, 1);
        }
    }

    memcpy(sps->old_ref_delta, sps->ref_delta, sizeof(sps->ref_delta));
    memcpy(sps->old_mode_delta, sps->mode_delta, sizeof(sps->mode_delta));
}

static MPP_RET set_frame_header(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eSps *sps = &ctx->sps;
    Pps *pps = ctx->ppss.pps;

    Vp8ePutBitBuf *bitbuf = &ctx->bitbuf[1];
    HalVp8eRefPic  *cur_pic = ctx->picbuf.cur_pic;
    Vp8eHalEntropy *entropy = &ctx->entropy;

    if (cur_pic->i_frame) {
        vp8e_put_lit(bitbuf, sps->color_type, 1);

        vp8e_put_lit(bitbuf, sps->clamp_type, 1);
    }

    vp8e_put_lit(bitbuf, pps->segment_enabled, 1);
    if (pps->segment_enabled)
        set_hdr_segmentation(bitbuf, &ctx->ppss, entropy);

    vp8e_put_lit(bitbuf, sps->filter_type, 1);

    vp8e_put_lit(bitbuf, sps->filter_level, 6);

    vp8e_put_lit(bitbuf, sps->filter_sharpness, 3);

    vp8e_put_lit(bitbuf, sps->filter_delta_enable, 1);
    if (sps->filter_delta_enable) {
        /* Filter level delta references reset by key frame */
        if (cur_pic->i_frame) {
            memset(sps->old_ref_delta, 0, sizeof(sps->ref_delta));
            memset(sps->old_mode_delta, 0, sizeof(sps->mode_delta));
        }
        set_filter_level_delta(bitbuf, sps);
    }

    vp8e_put_lit(bitbuf, sps->dct_partitions, 2);

    vp8e_put_lit(bitbuf, ctx->rc->qp_hdr, 7);

    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_put_lit(bitbuf, 0, 1);

    if (!cur_pic->i_frame) {
        HalVp8eRefPic  *ref_pic_list = ctx->picbuf.ref_pic_list;

        vp8e_put_lit(bitbuf, cur_pic->grf, 1); /* Grf refresh */
        vp8e_put_lit(bitbuf, cur_pic->arf, 1); /* Arf refresh */

        if (!cur_pic->grf) {
            if (ref_pic_list[0].grf) {
                vp8e_put_lit(bitbuf, 1, 2);    /* Ipf -> grf */
            } else if (ref_pic_list[2].grf) {
                vp8e_put_lit(bitbuf, 2, 2);    /* Arf -> grf */
            } else {
                vp8e_put_lit(bitbuf, 0, 2);    /* Not updated */
            }
        }

        if (!cur_pic->arf) {
            if (ref_pic_list[0].arf) {
                vp8e_put_lit(bitbuf, 1, 2);    /* Ipf -> arf */
            } else if (ref_pic_list[1].arf) {
                vp8e_put_lit(bitbuf, 2, 2);    /* Grf -> arf */
            } else {
                vp8e_put_lit(bitbuf, 0, 2);    /* Not updated */
            }
        }

        vp8e_put_lit(bitbuf, sps->sing_bias[1], 1); /* Grf */
        vp8e_put_lit(bitbuf, sps->sing_bias[2], 1); /* Arf */
    }

    vp8e_put_lit(bitbuf, sps->refresh_entropy, 1);
    if (!cur_pic->i_frame) {
        vp8e_put_lit(bitbuf, cur_pic->ipf, 1);
    }
    vp8e_calc_coeff_prob(bitbuf, &entropy->coeff_prob, &entropy->old_coeff_prob);
    vp8e_put_lit(bitbuf, 1, 1);
    vp8e_put_lit(bitbuf, entropy->skip_false_prob, 8);

    if (cur_pic->i_frame)
        return MPP_OK;

    vp8e_put_lit(bitbuf, entropy->intra_prob, 8);
    vp8e_put_lit(bitbuf, entropy->last_prob, 8);
    vp8e_put_lit(bitbuf, entropy->gf_prob, 8);
    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_put_lit(bitbuf, 0, 1);
    vp8e_calc_mv_prob(bitbuf, &entropy->mv_prob, &entropy->old_mv_prob);

    return MPP_OK;
}

static MPP_RET set_new_frame(void *hal)
{
    RK_S32 i;
    RK_S32 qp;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eSps *sps = &ctx->sps;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;

    hw_cfg->output_strm_size /= 8;
    hw_cfg->output_strm_size &= (~0x07);

    {
        RK_S32 offset = hw_cfg->output_strm_base >> 10;
        RK_S32 fd = hw_cfg->output_strm_base & 0x3ff;

        offset += ctx->bitbuf[1].byte_cnt;
        hw_cfg->output_strm_base = fd | ((offset & (~7)) << 10);
        hw_cfg->first_free_bit = (offset & 0x07) * 8;
    }

    if (hw_cfg->first_free_bit != 0) {
        RK_U32 val;
        RK_U8 *pTmp = (RK_U8 *)((size_t)(ctx->bitbuf[1].data) & (~0x07));

        for (val = 6; val >= hw_cfg->first_free_bit / 8; val--) {
            pTmp[val] = 0;
        }

        val =  pTmp[0] << 24;
        val |= pTmp[1] << 16;
        val |= pTmp[2] << 8;
        val |= pTmp[3];

        hw_cfg->strm_start_msb = val;

        if (hw_cfg->first_free_bit > 32) {
            val = pTmp[4] << 24;
            val |= pTmp[5] << 16;
            val |= pTmp[6] << 8;

            hw_cfg->strm_start_lsb = val;
        } else {
            hw_cfg->strm_start_lsb = 0;
        }
    } else {
        hw_cfg->strm_start_msb = hw_cfg->strm_start_lsb = 0;
    }

    if (sps->quarter_pixel_mv == 0) {
        hw_cfg->disable_qp_mv = 1;
    } else if (sps->quarter_pixel_mv == 1) {
        if (ctx->mb_per_frame > 8160)
            hw_cfg->disable_qp_mv = 1;
        else
            hw_cfg->disable_qp_mv = 0;
    } else {
        hw_cfg->disable_qp_mv = 0;
    }

    hw_cfg->enable_cabac = 1;

    if (sps->split_mv == 0)
        hw_cfg->split_mv_mode = 0;
    else if (sps->split_mv == 1) {
        if (ctx->mb_per_frame > 1584)
            hw_cfg->split_mv_mode = 0;
        else
            hw_cfg->split_mv_mode = 1;
    } else
        hw_cfg->split_mv_mode = 1;

    qp = ctx->rc->qp_hdr;
    if (hw_cfg->inter_favor == -1) {
        RK_S32 tmp = 128 - ctx->entropy.intra_prob;

        if (tmp < 0) {
            hw_cfg->inter_favor = tmp & 0xFFFF;
        } else {
            tmp = qp * 2 - 40;
            hw_cfg->inter_favor = MPP_MAX(0, tmp);
        }
    }

    if (hw_cfg->diff_mv_penalty[0] == -1)
        hw_cfg->diff_mv_penalty[0] = 64 / 2;
    if (hw_cfg->diff_mv_penalty[1] == -1)
        hw_cfg->diff_mv_penalty[1] = 60 / 2 * 32;
    if (hw_cfg->diff_mv_penalty[2] == -1)
        hw_cfg->diff_mv_penalty[2] = 8;
    if (hw_cfg->skip_penalty == -1)
        hw_cfg->skip_penalty = (qp >= 100) ? (3 * qp / 4) : 0;    /* Zero/nearest/near */
    if (hw_cfg->golden_penalty == -1)
        hw_cfg->golden_penalty = MPP_MAX(0, 5 * qp / 4 - 10);
    if (hw_cfg->split_penalty[0] == 0)
        hw_cfg->split_penalty[0] = MPP_MIN(1023, vp8_split_penalty_tbl[qp] / 2);
    if (hw_cfg->split_penalty[1] == 0)
        hw_cfg->split_penalty[1] = MPP_MIN(1023, (2 * vp8_split_penalty_tbl[qp] + 40) / 4);
    if (hw_cfg->split_penalty[3] == 0)
        hw_cfg->split_penalty[3] = MPP_MIN(511, (8 * vp8_split_penalty_tbl[qp] + 500) / 16);

    for (i = 0; i < 128; i++) {
        RK_S32 y, x;

        hw_cfg->dmv_penalty[i] = i * 2;
        y = vp8e_calc_cost_mv(i * 2, ctx->entropy.mv_prob[0]); /* mv y */
        x = vp8e_calc_cost_mv(i * 2, ctx->entropy.mv_prob[1]); /* mv x */
        hw_cfg->dmv_qpel_penalty[i] = MPP_MIN(255, (y + x + 1) / 2 * weight_tbl[qp] >> 8);
    }

    for (i = 0; i < 4; i++) {
        qp = ctx->ppss.pps->qp_sgm[i];
        hw_cfg->y1_quant_dc[i] = ctx->qp_y1[qp].quant[0];
        hw_cfg->y1_quant_ac[i] = ctx->qp_y1[qp].quant[1];
        hw_cfg->y2_quant_dc[i] = ctx->qp_y2[qp].quant[0];
        hw_cfg->y2_quant_ac[i] = ctx->qp_y2[qp].quant[1];
        hw_cfg->ch_quant_dc[i] = ctx->qp_ch[qp].quant[0];
        hw_cfg->ch_quant_ac[i] = ctx->qp_ch[qp].quant[1];
        hw_cfg->y1_zbin_dc[i] = ctx->qp_y1[qp].zbin[0];
        hw_cfg->y1_zbin_ac[i] = ctx->qp_y1[qp].zbin[1];
        hw_cfg->y2_zbin_dc[i] = ctx->qp_y2[qp].zbin[0];
        hw_cfg->y2_zbin_ac[i] = ctx->qp_y2[qp].zbin[1];
        hw_cfg->ch_zbin_dc[i] = ctx->qp_ch[qp].zbin[0];
        hw_cfg->ch_zbin_ac[i] = ctx->qp_ch[qp].zbin[1];
        hw_cfg->y1_round_dc[i] = ctx->qp_y1[qp].round[0];
        hw_cfg->y1_round_ac[i] = ctx->qp_y1[qp].round[1];
        hw_cfg->y2_round_dc[i] = ctx->qp_y2[qp].round[0];
        hw_cfg->y2_round_ac[i] = ctx->qp_y2[qp].round[1];
        hw_cfg->ch_round_dc[i] = ctx->qp_ch[qp].round[0];
        hw_cfg->ch_round_ac[i] = ctx->qp_ch[qp].round[1];
        hw_cfg->y1_dequant_dc[i] = ctx->qp_y1[qp].dequant[0];
        hw_cfg->y1_dequant_ac[i] = ctx->qp_y1[qp].dequant[1];
        hw_cfg->y2_dequant_dc[i] = ctx->qp_y2[qp].dequant[0];
        hw_cfg->y2_dequant_ac[i] = ctx->qp_y2[qp].dequant[1];
        hw_cfg->ch_dequant_dc[i] = ctx->qp_ch[qp].dequant[0];
        hw_cfg->ch_dequant_ac[i] = ctx->qp_ch[qp].dequant[1];

        hw_cfg->filter_level[i] = ctx->ppss.pps->level_sgm[i];
    }

    hw_cfg->bool_enc_value = ctx->bitbuf[1].bottom;
    hw_cfg->bool_enc_value_bits = 24 - ctx->bitbuf[1].bits_left;
    hw_cfg->bool_enc_range = ctx->bitbuf[1].range;

    if (ctx->picbuf.cur_pic->i_frame)
        hw_cfg->frame_coding_type = 1;
    else
        hw_cfg->frame_coding_type = 0;

    hw_cfg->size_tbl_base = mpp_buffer_get_fd(buffers->hw_size_table_buf);

    hw_cfg->dct_partitions = sps->dct_partitions;
    hw_cfg->filter_disable = sps->filter_type;
    hw_cfg->filter_sharpness = sps->filter_sharpness;
    hw_cfg->segment_enable = ctx->ppss.pps->segment_enabled;
    hw_cfg->segment_map_update = ctx->ppss.pps->sgm.map_modified;

    ctx->ppss.pps->sgm.map_modified = 0;

    for (i = 0; i < 4; i++) {
        hw_cfg->lf_ref_delta[i] = sps->ref_delta[i];
        hw_cfg->lf_mode_delta[i] = sps->mode_delta[i];
    }

    set_intra_pred_penalties(hw_cfg, qp);

    memset(mpp_buffer_get_ptr(buffers->hw_prob_count_buf),
           0, VP8_PROB_COUNT_BUF_SIZE);

    return MPP_OK;
}

static MPP_RET set_code_frame(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *) hal;

    vp8e_init_entropy(ctx);
    set_segmentation(ctx);
    set_filter(ctx);
    set_frame_header(ctx);
    set_new_frame(ctx);
    vp8e_write_entropy_tables(ctx);

    return MPP_OK;
}

static void reset_refpic(HalVp8eRefPic *refPic)
{
    refPic->poc = -1;
    refPic->i_frame = 0;
    refPic->p_frame = 0;
    refPic->show = 0;
    refPic->ipf = 0;
    refPic->arf = 0;
    refPic->grf = 0;
    refPic->search = 0;
}

static void init_ref_pic_list(HalVp8ePicBuf *picbuf)
{
    RK_S32 i, j;

    HalVp8eRefPic *ref_pic = picbuf->ref_pic;
    HalVp8eRefPic *cur_pic = picbuf->cur_pic;
    HalVp8eRefPic *ref_pic_list = picbuf->ref_pic_list;

    j = 0;
    for (i = 0; i < picbuf->size + 1; i++) {
        if (ref_pic[i].ipf && (&ref_pic[i] != cur_pic)) {
            ref_pic_list[j++] = ref_pic[i];
            break;
        }
    }

    for (i = 0; i < picbuf->size + 1; i++) {
        if (ref_pic[i].grf && (&ref_pic[i] != cur_pic)) {
            ref_pic_list[j++] = ref_pic[i];
            break;
        }
    }

    for (i = 0; i < picbuf->size + 1; i++) {
        if (ref_pic[i].arf && (&ref_pic[i] != cur_pic)) {
            ref_pic_list[j] = ref_pic[i];
            break;
        }
    }

    for (i = 0; i < picbuf->size; i++) {
        ref_pic_list[i].ipf = 0;
        ref_pic_list[i].grf = 0;
        ref_pic_list[i].arf = 0;
    }
}

static MPP_RET init_picbuf(void *hal)
{
    RK_S32 i = 0;

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    HalVp8ePicBuf *buf = &ctx->picbuf;

    if (buf->cur_pic->i_frame) {
        buf->cur_pic->p_frame = 0;
        buf->cur_pic->ipf = 1;
        buf->cur_pic->grf = 1;
        buf->cur_pic->arf = 1;

        for (i = 0; i < 4; i++) {
            if (&buf->ref_pic[i] != buf->cur_pic) {
                reset_refpic(&buf->ref_pic[i]);
            }
        }
    }

    for (i = 0; i < 3; i++) {
        reset_refpic(&buf->ref_pic_list[i]);
    }

    init_ref_pic_list(buf);

    return MPP_OK;
}

static MPP_RET set_picbuf_ref(void *hal)
{
    RK_S32 i = 0;

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    HalVp8ePicBuf *pic_buf = &ctx->picbuf;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    HalVp8eRefPic *ref_pic_list = pic_buf->ref_pic_list;

    {
        RK_S32 no_grf = 0;
        RK_S32 no_arf = 0;
        if (pic_buf->size < 2) {
            no_grf = 1;
            pic_buf->cur_pic->grf = 0;
        }
        if (pic_buf->size < 3) {
            no_arf = 1;
            pic_buf->cur_pic->arf = 0;
        }

        for (i = 0; i < pic_buf->size; i++) {
            if (pic_buf->cur_pic->grf || no_grf)
                pic_buf->ref_pic_list[i].grf = 0;
            if (pic_buf->cur_pic->arf || no_arf)
                pic_buf->ref_pic_list[i].arf = 0;
        }
    }
    {
        RK_S32 ref_idx = -1;
        RK_S32 ref_idx2 = -1;

        for (i = 0; i < 3; i++) {
            if ((i < pic_buf->size) && ref_pic_list[i].search) {
                if (ref_idx == -1)
                    ref_idx = i;
                else if (ref_idx2 == -1)
                    ref_idx2 = i;
                else
                    ref_pic_list[i].search = 0;
            } else {
                ref_pic_list[i].search = 0;
            }
        }

        if (ref_idx == -1)
            ref_idx = 0;

        hw_cfg->mv_ref_idx[0] = hw_cfg->mv_ref_idx[1] = ref_idx;

        if (pic_buf->cur_pic->p_frame) {
            pic_buf->ref_pic_list[ref_idx].search = 1;

            hw_cfg->internal_img_lum_base_r[0] = ref_pic_list[ref_idx].picture.lum;
            hw_cfg->internal_img_chr_base_r[0] = ref_pic_list[ref_idx].picture.cb;
            hw_cfg->internal_img_lum_base_r[1] = ref_pic_list[ref_idx].picture.lum;
            hw_cfg->internal_img_chr_base_r[1] = ref_pic_list[ref_idx].picture.cb;
            hw_cfg->mv_ref_idx[0] = hw_cfg->mv_ref_idx[1] = ref_idx;
            hw_cfg->ref2_enable = 0;

            if (ref_idx2 != -1) {
                hw_cfg->internal_img_lum_base_r[1] = ref_pic_list[ref_idx2].picture.lum;
                hw_cfg->internal_img_chr_base_r[1] = ref_pic_list[ref_idx2].picture.cb;
                hw_cfg->mv_ref_idx[1] = ref_idx2;
                hw_cfg->ref2_enable = 1;
            }
        }
    }
    hw_cfg->rec_write_disable = 0;

    if (!pic_buf->cur_pic->picture.lum) {
        HalVp8eRefPic *cur_pic = pic_buf->cur_pic;
        HalVp8eRefPic *cand;
        RK_S32 recIdx = -1;

        for (i = 0; i < pic_buf->size + 1; i++) {
            cand = &pic_buf->ref_pic[i];
            if (cand == cur_pic)
                continue;
            if (((cur_pic->ipf | cand->ipf) == cur_pic->ipf) &&
                ((cur_pic->grf | cand->grf) == cur_pic->grf) &&
                ((cur_pic->arf | cand->arf) == cur_pic->arf))
                recIdx = i;
        }

        if (recIdx >= 0) {
            cur_pic->picture.lum = pic_buf->ref_pic[recIdx].picture.lum;
            pic_buf->ref_pic[recIdx].picture.lum = 0;
        } else {
            hw_cfg->rec_write_disable = 1;
        }
    }

    hw_cfg->internal_img_lum_base_w = pic_buf->cur_pic->picture.lum;
    hw_cfg->internal_img_chr_base_w = pic_buf->cur_pic->picture.cb;

    return MPP_OK;
}

static void write_ivf_header(void *hal, RK_U8 *out)
{
    RK_U8 data[IVF_HDR_BYTES] = {0};

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    MppEncPrepCfg *prep = &ctx->cfg->prep;
    MppEncRcCfg *rc = &ctx->cfg->rc;

    data[0] = 'D';
    data[1] = 'K';
    data[2] = 'I';
    data[3] = 'F';

    data[6] = 32;

    data[8] = 'V';
    data[9] = 'P';
    data[10] = '8';
    data[11] = '0';

    data[12] = prep->width & 0xff;
    data[13] = (prep->width >> 8) & 0xff;
    data[14] = prep->height & 0xff;
    data[15] = (prep->height >> 8) & 0xff;

    data[16] = rc->fps_out_num & 0xff;
    data[17] = (rc->fps_out_num >> 8) & 0xff;
    data[18] = (rc->fps_out_num >> 16) & 0xff;
    data[19] = (rc->fps_out_num >> 24) & 0xff;

    data[20] = rc->fps_out_denorm & 0xff;
    data[21] = (rc->fps_out_denorm >> 8) & 0xff;
    data[22] = (rc->fps_out_denorm >> 16) & 0xff;
    data[23] = (rc->fps_out_denorm >> 24) & 0xff;

    data[24] = ctx->frame_cnt & 0xff;
    data[25] = (ctx->frame_cnt >> 8) & 0xff;
    data[26] = (ctx->frame_cnt >> 16) & 0xff;
    data[27] = (ctx->frame_cnt >> 24) & 0xff;

    memcpy(out, data, IVF_HDR_BYTES);
}

static void write_ivf_frame(void *hal, RK_U8 *out)
{
    RK_U8 data[IVF_FRM_BYTES];

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    RK_S32 byte_cnt = ctx->frame_size;

    data[0] = byte_cnt & 0xff;
    data[1] = (byte_cnt >> 8) & 0xff;
    data[2] = (byte_cnt >> 16) & 0xff;
    data[3] = (byte_cnt >> 24) & 0xff;

    data[4]  = ctx->frame_cnt & 0xff;
    data[5]  = (ctx->frame_cnt >> 8) & 0xff;
    data[6]  = (ctx->frame_cnt >> 16) & 0xff;
    data[7]  = (ctx->frame_cnt >> 24) & 0xff;
    data[8]  = (ctx->frame_cnt >> 32) & 0xff;
    data[9]  = (ctx->frame_cnt >> 40) & 0xff;
    data[10] = (ctx->frame_cnt >> 48) & 0xff;
    data[11] = (ctx->frame_cnt >> 56) & 0xff;

    memcpy(out, data, IVF_FRM_BYTES);
}

static MPP_RET set_frame_tag(void *hal)
{
    RK_S32 tmp = 0;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    HalVp8ePicBuf *pic_buf = &ctx->picbuf;
    HalVp8eRefPic *cur_pic = pic_buf->cur_pic;
    Vp8ePutBitBuf *bitbuf = &ctx->bitbuf[0];

    tmp = ((ctx->bitbuf[1].byte_cnt) << 5) |
          ((cur_pic->show ? 1 : 0) << 4) |
          (ctx->sps.profile << 1) |
          (cur_pic->i_frame ? 0 : 1);

    vp8e_put_byte(bitbuf, tmp & 0xff);

    vp8e_put_byte(bitbuf, (tmp >> 8) & 0xff);

    vp8e_put_byte(bitbuf, (tmp >> 16) & 0xff);

    if (!cur_pic->i_frame)
        return MPP_NOK;

    vp8e_put_byte(bitbuf, 0x9d);
    vp8e_put_byte(bitbuf, 0x01);
    vp8e_put_byte(bitbuf, 0x2a);

    tmp = ctx->sps.pic_width_in_pixel | (ctx->sps.horizontal_scaling << 14);
    vp8e_put_byte(bitbuf, tmp & 0xff);
    vp8e_put_byte(bitbuf, tmp >> 8);

    tmp = ctx->sps.pic_height_in_pixel | (ctx->sps.vertical_scaling << 14);
    vp8e_put_byte(bitbuf, tmp & 0xff);
    vp8e_put_byte(bitbuf, tmp >> 8);

    return MPP_OK;
}

static MPP_RET set_data_part_size(void *hal)
{
    RK_S32 i = 0;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (!ctx->sps.dct_partitions)
        return MPP_NOK;

    for (i = 2; i < ctx->sps.partition_cnt - 1; i++) {
        Vp8ePutBitBuf *bitbuf = ctx->bitbuf;
        RK_S32  tmp = bitbuf[i].data - bitbuf[i].p_data;
        vp8e_put_byte(&bitbuf[1], tmp & 0xff);
        vp8e_put_byte(&bitbuf[1], (tmp >> 8) & 0xff);
        vp8e_put_byte(&bitbuf[1], (tmp >> 16) & 0xff);
    }

    return MPP_OK;
}

static MPP_RET update_picbuf(HalVp8ePicBuf *picbuf)
{
    RK_S32 i , j;

    HalVp8eRefPic *ref_pic_list = picbuf->ref_pic_list;
    HalVp8eRefPic *ref_pic      = picbuf->ref_pic;
    HalVp8eRefPic *cur_pic      = picbuf->cur_pic;

    picbuf->last_pic = picbuf->cur_pic;

    for (i = 0; i < picbuf->size + 1; i++) {
        if (&ref_pic[i] == cur_pic)
            continue;
        if (cur_pic->ipf)
            ref_pic[i].ipf = 0;
        if (cur_pic->grf)
            ref_pic[i].grf = 0;
        if (cur_pic->arf)
            ref_pic[i].arf = 0;
    }

    for (i = 0; i < picbuf->size; i++) {
        for (j = 0; j < picbuf->size + 1; j++) {
            if (ref_pic_list[i].grf)
                ref_pic[j].grf = 0;
            if (ref_pic_list[i].arf)
                ref_pic[j].arf = 0;
        }
    }

    for (i = 0; i < picbuf->size; i++) {
        if (ref_pic_list[i].grf)
            ref_pic_list[i].refPic->grf = 1;
        if (ref_pic_list[i].arf)
            ref_pic_list[i].refPic->arf = 1;
    }

    for (i = 0; i < picbuf->size + 1; i++) {
        HalVp8eRefPic *tmp = &ref_pic[i];
        if (!tmp->ipf && !tmp->arf && !tmp->grf) {
            picbuf->cur_pic = &ref_pic[i];
            break;
        }
    }

    return MPP_OK;
}

static MPP_RET set_parameter(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eSps *sps = &ctx->sps;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    MppEncPrepCfg *set = &ctx->cfg->prep;

    RK_S32 width  = 16 * ((set->width + 15) / 16);
    RK_S32 height = 16 * ((set->height + 15) / 16);
    RK_U32 stride = MPP_ALIGN(width, 16);

    //TODO handle rotation

    ctx->mb_per_frame = width / 16 * height / 16;
    ctx->mb_per_row = width / 16;
    ctx->mb_per_col = width / 16;

    ctx->gop_len = ctx->cfg->rc.gop;
    ctx->bit_rate = ctx->cfg->rc.bps_target;

    sps->pic_width_in_pixel    = width;
    sps->pic_height_in_pixel   = height;
    sps->pic_width_in_mbs      = width / 16;
    sps->pic_height_in_mbs     = height / 16;
    sps->horizontal_scaling    = 0;
    sps->vertical_scaling      = 0;
    sps->color_type            = 0;
    sps->clamp_type            = 0;
    sps->dct_partitions        = 0;
    sps->partition_cnt         = 2 + (1 << sps->dct_partitions);
    sps->profile               = 1;
    sps->filter_type           = 0;
    sps->filter_level          = 0;
    sps->filter_sharpness      = 0;
    sps->auto_filter_level     = 1;
    sps->auto_filter_sharpness = 1;
    sps->quarter_pixel_mv      = 1;
    sps->split_mv              = 1;
    sps->refresh_entropy       = 1;
    memset(sps->sing_bias, 0, sizeof(sps->sing_bias));

    sps->filter_delta_enable = 1;
    memset(sps->ref_delta, 0, sizeof(sps->ref_delta));
    memset(sps->mode_delta, 0, sizeof(sps->mode_delta));

    hw_cfg->input_rotation = set->rotation;

    {
        RK_U32 tmp = 0;
        RK_U32 hor_offset_src = 0;
        RK_U32 ver_offset_src = 0;
        RK_U8 video_stab = 0;

        if (set->format == MPP_FMT_YUV420SP || set->format == MPP_FMT_YUV420P) {
            tmp = ver_offset_src;
            tmp *= stride;
            tmp += hor_offset_src;
            hw_cfg->input_lum_base += (tmp & (~7));
            hw_cfg->input_luma_base_offset = tmp & 7;

            if (video_stab)
                hw_cfg->vs_next_luma_base += (tmp & (~7));

            if (set->format == MPP_FMT_YUV420P) {
                tmp = ver_offset_src / 2;
                tmp *= stride / 2;
                tmp += hor_offset_src / 2;

                hw_cfg->input_cb_base += (tmp & (~7));
                hw_cfg->input_cr_base += (tmp & (~7));
                hw_cfg->input_chroma_base_offset = tmp & 7;
            } else {
                tmp = ver_offset_src / 2;
                tmp *= stride / 2;
                tmp += hor_offset_src / 2;
                tmp *= 2;

                hw_cfg->input_cb_base += (tmp & (~7));
                hw_cfg->input_chroma_base_offset = tmp & 7;
            }
        } else if (set->format <= MPP_FMT_BGR444 && set->format >= MPP_FMT_RGB565) {
            tmp = ver_offset_src;
            tmp *= stride;
            tmp += hor_offset_src;
            tmp *= 2;

            hw_cfg->input_lum_base += (tmp & (~7));
            hw_cfg->input_luma_base_offset = tmp & 7;
            hw_cfg->input_chroma_base_offset = (hw_cfg->input_luma_base_offset / 4) * 4;

            if (video_stab)
                hw_cfg->vs_next_luma_base += (tmp & (~7));
        } else {
            tmp = ver_offset_src;
            tmp *= stride;
            tmp += hor_offset_src;
            tmp *= 4;

            hw_cfg->input_lum_base += (tmp & (~7));
            hw_cfg->input_luma_base_offset = (tmp & 7) / 2;

            if (video_stab)
                hw_cfg->vs_next_luma_base += (tmp & (~7));
        }

        hw_cfg->mbs_in_row = (set->width + 15) / 16;
        hw_cfg->mbs_in_col = (set->height + 15) / 16;
        hw_cfg->pixels_on_row = stride;
    }
    if (set->width & 0x0F)
        hw_cfg->x_fill = (16 - (set->width & 0x0F)) / 4;
    else
        hw_cfg->x_fill = 0;

    if (set->height & 0x0F)
        hw_cfg->y_fill = 16 - (set->height & 0x0F);
    else
        hw_cfg->y_fill = 0;

    hw_cfg->vs_mode = 0;

    switch (set->color) {
    case MPP_FRAME_SPC_RGB:     /* BT.601 */
    default:
        /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
        * Cb = 0.5647 (B - Y) + 128
        * Cr = 0.7132 (R - Y) + 128
        */
        hw_cfg->rgb_coeff_a = 19589;
        hw_cfg->rgb_coeff_b = 38443;
        hw_cfg->rgb_coeff_c = 7504;
        hw_cfg->rgb_coeff_e = 37008;
        hw_cfg->rgb_coeff_f = 46740;
        break;

    case MPP_FRAME_SPC_BT709:         /* BT.709 */
        /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
         * Cb = 0.5389 (B - Y) + 128
         * Cr = 0.6350 (R - Y) + 128
         */
        hw_cfg->rgb_coeff_a = 13933;
        hw_cfg->rgb_coeff_b = 46871;
        hw_cfg->rgb_coeff_c = 732;
        hw_cfg->rgb_coeff_e = 35317;
        hw_cfg->rgb_coeff_f = 41615;
        break;
    }

    hw_cfg->r_mask_msb = hw_cfg->g_mask_msb = hw_cfg->b_mask_msb = 0;
    switch (set->format) {
    case MPP_FMT_YUV420P:
        hw_cfg->input_format = INPUT_YUV420PLANAR;
        break;
    case MPP_FMT_YUV420SP:
        hw_cfg->input_format = INPUT_YUV420SEMIPLANAR;
        break;
    case MPP_FMT_YUV422_YUYV:
        hw_cfg->input_format = INPUT_YUYV422INTERLEAVED;
        break;
    case MPP_FMT_YUV422_UYVY:
        hw_cfg->input_format = INPUT_UYVY422INTERLEAVED;
        break;
    case MPP_FMT_RGB565:
        hw_cfg->input_format = INPUT_RGB565;
        hw_cfg->r_mask_msb = 15;
        hw_cfg->g_mask_msb = 10;
        hw_cfg->b_mask_msb = 4;
        break;
    case MPP_FMT_BGR565:
        hw_cfg->input_format = INPUT_RGB565;
        hw_cfg->b_mask_msb = 15;
        hw_cfg->g_mask_msb = 10;
        hw_cfg->r_mask_msb = 4;
        break;
    case MPP_FMT_RGB555:
        hw_cfg->input_format = INPUT_RGB555;
        hw_cfg->r_mask_msb = 14;
        hw_cfg->g_mask_msb = 9;
        hw_cfg->b_mask_msb = 4;
        break;
    case MPP_FMT_BGR555:
        hw_cfg->input_format = INPUT_RGB555;
        hw_cfg->b_mask_msb = 14;
        hw_cfg->g_mask_msb = 9;
        hw_cfg->r_mask_msb = 4;
        break;
    case MPP_FMT_RGB444:
        hw_cfg->input_format = INPUT_RGB444;
        hw_cfg->r_mask_msb = 11;
        hw_cfg->g_mask_msb = 7;
        hw_cfg->b_mask_msb = 3;
        break;
    case MPP_FMT_BGR444:
        hw_cfg->input_format = INPUT_RGB444;
        hw_cfg->b_mask_msb = 11;
        hw_cfg->g_mask_msb = 7;
        hw_cfg->r_mask_msb = 3;
        break;
    case MPP_FMT_RGB888:
        hw_cfg->input_format = INPUT_RGB888;
        hw_cfg->r_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->b_mask_msb = 7;
        break;
    case MPP_FMT_BGR888:
        hw_cfg->input_format = INPUT_RGB888;
        hw_cfg->b_mask_msb = 23;
        hw_cfg->g_mask_msb = 15;
        hw_cfg->r_mask_msb = 7;
        break;
    case MPP_FMT_RGB101010:
        hw_cfg->input_format = INPUT_RGB101010;
        hw_cfg->r_mask_msb = 29;
        hw_cfg->g_mask_msb = 19;
        hw_cfg->b_mask_msb = 9;
        break;
    case MPP_FMT_BGR101010:
        hw_cfg->input_format = INPUT_RGB101010;
        hw_cfg->b_mask_msb = 29;
        hw_cfg->g_mask_msb = 19;
        hw_cfg->r_mask_msb = 9;
        break;
    default:
        mpp_err("unsupported color format %d", set->format);
        return MPP_NOK;
        break;
    }

    return MPP_OK;
}

static MPP_RET set_picbuf(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    // find one dpb for current picture
    {
        RK_U32 i = 0;
        RK_S32 width  = ctx->sps.pic_width_in_mbs * 16;
        RK_S32 height = ctx->sps.pic_height_in_mbs * 16;
        HalVp8ePicBuf *picbuf = &ctx->picbuf;

        memset(picbuf->ref_pic, 0, sizeof(picbuf->ref_pic));
        memset(picbuf->ref_pic_list, 0, sizeof(picbuf->ref_pic_list));

        for (i = 0; i < REF_FRAME_COUNT + 1; i++) {
            picbuf->ref_pic[i].picture.lum_width  = width;
            picbuf->ref_pic[i].picture.lum_height = height;
            picbuf->ref_pic[i].picture.ch_width   = width / 2;
            picbuf->ref_pic[i].picture.ch_height  = height / 2;
            picbuf->ref_pic[i].picture.lum = 0;
            picbuf->ref_pic[i].picture.cb  = 0;
        }

        picbuf->cur_pic = &picbuf->ref_pic[0];
    }

    ctx->ppss.size = 1;
    ctx->ppss.store = (Pps *)mpp_calloc(Pps, 1);
    if (ctx->ppss.store == NULL) {
        mpp_err("failed to malloc ppss store.\n");
        goto __ERR_RET;
    }

    ctx->ppss.pps = ctx->ppss.store;
    ctx->ppss.pps->segment_enabled  = 0;
    ctx->ppss.pps->sgm.map_modified = 0;

    return MPP_OK;

__ERR_RET:
    return MPP_NOK;
}

static MPP_RET alloc_buffer(void *hal)
{
    MPP_RET ret = MPP_OK;

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;
    MppEncPrepCfg *pre = &ctx->cfg->prep;
    RK_U32 mb_total = ctx->mb_per_frame;
    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;

    //set coding format as VP8
    hw_cfg->coding_type = 1;

    ret = mpp_buffer_group_get_internal(&buffers->hw_buf_grp,
                                        MPP_BUFFER_TYPE_ION);
    if (ret) {
        mpp_err("buf group get failed ret %d\n", ret);
        goto __ERR_RET;
    }

    //add 128 bytes to avoid kernel crash
    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_luma_buf,
                         mb_total * (16 * 16) + 128);
    if (ret) {
        mpp_err("hw_luma_buf get failed ret %d\n", ret);
        goto __ERR_RET;
    }
    {
        RK_U32 i = 0;
        for (i = 0; i < 2; i++) {
            //add 128 bytes to avoid kernel crash
            ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_cbcr_buf[i],
                                 mb_total * (2 * 8 * 8) + 128);
            if (ret) {
                mpp_err("hw_cbcr_buf[%d] get failed ret %d\n", i, ret);
                goto __ERR_RET;
            }
        }
    }
    hw_cfg->internal_img_lum_base_w = mpp_buffer_get_fd(buffers->hw_luma_buf);
    hw_cfg->internal_img_chr_base_w = mpp_buffer_get_fd(buffers->hw_cbcr_buf[0]);

    hw_cfg->internal_img_lum_base_r[0] = mpp_buffer_get_fd(buffers->hw_luma_buf);
    hw_cfg->internal_img_chr_base_r[0] = mpp_buffer_get_fd(buffers->hw_cbcr_buf[1]);
    {
        /* NAL size table, table size must be 64-bit multiple,
         * space for SEI, MVC prefix, filler and zero at the end of table.
         * At least 1 macroblock row in every slice.
         * Also used for VP8 partitions. */
        RK_U32 size_tbl = MPP_ALIGN(sizeof(RK_U32) * (pre->height + 4), 8);
        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_size_table_buf, size_tbl);
        if (ret) {
            mpp_err("hw_size_table_buf get failed ret %d\n", ret);
            goto __ERR_RET;
        }
    }
    {
        RK_U32  cabac_tbl_size = 8 * 55 + 8 * 96;
        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_cabac_table_buf,
                             cabac_tbl_size);
        if (ret) {
            mpp_err("hw_cabac_table_buf get failed\n");
            goto __ERR_RET;
        }
    }
    hw_cfg->cabac_tbl_base = mpp_buffer_get_fd(buffers->hw_cabac_table_buf);

    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_mv_output_buf,
                         mb_total * 4);
    if (ret) {
        mpp_err("hw_mv_output_buf get failed ret %d\n", ret);
        goto __ERR_RET;
    }

    hw_cfg->mv_output_base = mpp_buffer_get_fd(buffers->hw_mv_output_buf);

    memset(mpp_buffer_get_ptr(buffers->hw_mv_output_buf), 0, sizeof(RK_U32) * mb_total);

    ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_prob_count_buf, VP8_PROB_COUNT_BUF_SIZE);
    if (ret) {
        mpp_err("hw_prob_count_buf get failed ret %d\n", ret);
        goto __ERR_RET;
    }

    hw_cfg->prob_count_base = mpp_buffer_get_fd(buffers->hw_prob_count_buf);
    {
        /* VP8: Segmentation map, 4 bits/mb, 64-bit multiple. */
        RK_U32 segment_map_size = (mb_total * 4 + 63) / 64 * 8;

        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_segment_map_buf, segment_map_size);
        if (ret) {
            mpp_err("hw_segment_map_buf get failed ret %d\n", ret);
            goto __ERR_RET;
        }

        hw_cfg->segment_map_base = mpp_buffer_get_fd(buffers->hw_segment_map_buf);
        memset(mpp_buffer_get_ptr(buffers->hw_segment_map_buf), 0, segment_map_size / 4);

    }
    {
        RK_U32 i = 0;

        ctx->picbuf.size = 1;
        for (i = 0; i < 1; i++)
            ctx->picbuf.ref_pic[i].picture.lum = mpp_buffer_get_fd(buffers->hw_luma_buf);
        for (i = 0; i < 2; i++)
            ctx->picbuf.ref_pic[i].picture.cb = mpp_buffer_get_fd(buffers->hw_cbcr_buf[i]);
    }
    {
        RK_U32 pic_size = MPP_ALIGN(pre->width, 16) * MPP_ALIGN(pre->height, 16) * 3 / 2;
        RK_U32 out_size = pic_size / 2;

        ret = mpp_buffer_get(buffers->hw_buf_grp, &buffers->hw_out_buf,  out_size);
        if (ret) {
            mpp_err("hw_out_buf get failed ret %d\n", ret);
            goto __ERR_RET;
        }
    }
    ctx->regs = mpp_calloc(RK_U32, ctx->reg_size);
    if (!ctx->regs) {
        mpp_err("failed to calloc regs.\n");
        goto __ERR_RET;
    }

    hw_cfg->intra_16_favor     = -1;
    hw_cfg->prev_mode_favor    = -1;
    hw_cfg->inter_favor        = -1;
    hw_cfg->skip_penalty       = -1;
    hw_cfg->diff_mv_penalty[0] = -1;
    hw_cfg->diff_mv_penalty[1] = -1;
    hw_cfg->diff_mv_penalty[2] = -1;
    hw_cfg->split_penalty[0]   = 0;
    hw_cfg->split_penalty[1]   = 0;
    hw_cfg->split_penalty[2]   = 0x3FF;
    hw_cfg->split_penalty[3]   = 0;
    hw_cfg->zero_mv_favor      = 0;

    hw_cfg->intra_area_top  = hw_cfg->intra_area_bottom = 255;
    hw_cfg->intra_area_left = hw_cfg->intra_area_right  = 255;
    hw_cfg->roi1_top  = hw_cfg->roi1_bottom = 255;
    hw_cfg->roi1_left = hw_cfg->roi1_right  = 255;
    hw_cfg->roi2_top  = hw_cfg->roi2_bottom = 255;
    hw_cfg->roi2_left = hw_cfg->roi2_right  = 255;

    return ret;

__ERR_RET:
    if (buffers)
        hal_vp8e_buf_free(hal);

    return ret;
}

MPP_RET hal_vp8e_enc_strm_code(void *hal, HalTaskInfo *task)
{
    HalVp8eCtx  *ctx  = (HalVp8eCtx *)hal;
    Vp8eHwCfg *hw_cfg = &ctx->hw_cfg;

    MppEncCfgSet  *cfg = ctx->cfg;
    MppEncPrepCfg *prep = &cfg->prep;

    {
        RK_U32 i = 0;
        for (i = 0; i < 9; i++) {
            ctx->p_out_buf[i] = NULL;
            ctx->stream_size[i] = 0;
        }
    }

    {
        HalEncTask *enc_task = &task->enc;
        RK_U32 hor_stride = MPP_ALIGN(prep->width, 16);
        RK_U32 ver_stride = MPP_ALIGN(prep->height, 16);
        RK_U32 offset_uv  = hor_stride * ver_stride;

        hw_cfg->input_lum_base = mpp_buffer_get_fd(enc_task->input);
        hw_cfg->input_cb_base  = hw_cfg->input_lum_base + (offset_uv << 10);
        hw_cfg->input_cr_base  = hw_cfg->input_cb_base + (offset_uv << 8);
    }

    // split memory for vp8 partition
    {
        RK_S32 offset = 0;
        Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;
        RK_U8 *p_end = NULL;
        RK_U32 buf_size = mpp_buffer_get_size(buffers->hw_out_buf);
        RK_U32 bus_addr = mpp_buffer_get_fd(buffers->hw_out_buf);
        RK_U8 *p_start = mpp_buffer_get_ptr(buffers->hw_out_buf);

        p_end = p_start + 3;
        if (ctx->frame_type == VP8E_FRM_KEY)
            p_end += 7;// frame tag len:I frame 10 byte, P frmae 3 byte.
        vp8e_set_buffer(&ctx->bitbuf[0], p_start, p_end - p_start);

        offset = p_end - p_start;
        hw_cfg->output_strm_base = bus_addr + (offset << 10);

        p_start = p_end;
        p_end = p_start + buf_size / 10;
        p_end = (RK_U8 *)((size_t)p_end & ~0x7);
        vp8e_set_buffer(&ctx->bitbuf[1], p_start, p_end - p_start);

        offset += p_end - p_start;
        hw_cfg->partition_Base[0] = bus_addr | (offset << 10);

        p_start = p_end;
        p_end = mpp_buffer_get_ptr(buffers->hw_out_buf) + buf_size;
        p_end = (RK_U8 *)((size_t)p_end & ~0x7);
        vp8e_set_buffer(&ctx->bitbuf[2], p_start, p_end - p_start);

        offset += p_end - p_start;
        hw_cfg->partition_Base[1] = bus_addr | (offset << 10);
        hw_cfg->output_strm_size = p_end - p_start;

        p_start = p_end;
    }

    {

        HalVp8ePicBuf *pic_buf = &ctx->picbuf;
        pic_buf->cur_pic->show = 1;
        pic_buf->cur_pic->poc = ctx->frame_cnt;
        pic_buf->cur_pic->i_frame = (ctx->frame_type == VP8E_FRM_KEY);

        init_picbuf(ctx);

        if (ctx->frame_type == VP8E_FRM_P) {
            pic_buf->cur_pic->p_frame = 1;
            pic_buf->cur_pic->arf = 1;
            pic_buf->cur_pic->grf = 1;
            pic_buf->cur_pic->ipf = 1;
            pic_buf->ref_pic_list[0].search = 1;
            pic_buf->ref_pic_list[1].search = 1;
            pic_buf->ref_pic_list[2].search = 1;
        }

        if (ctx->rc->frame_coded == 0)
            return MPP_OK;

        if (ctx->rc->golden_picture_rate) {
            pic_buf->cur_pic->grf = 1;
            if (!pic_buf->cur_pic->arf)
                pic_buf->ref_pic_list[1].arf = 1;
        }
    }
    set_frame_params(ctx);
    set_picbuf_ref(ctx);
    set_code_frame(ctx);

    return MPP_OK;
}

MPP_RET hal_vp8e_init_qp_table(void *hal)
{
    RK_S32 i = 0, j = 0;

    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    for (i = 0; i < QINDEX_RANGE; i++) {
        RK_S32 tmp = 0;
        Vp8eQp * qp = &ctx->qp_y1[i];
        for (j = 0; j < 2; j++) {
            if (j == 0) {
                tmp = dc_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
            } else {
                tmp = ac_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
            }

            qp->quant[j] = MPP_MIN((1 << 16) / tmp, 0x3FFF);
            qp->zbin[j]  = ((q_zbin_factors_tbl[i] * tmp) + 64) >> 7;
            qp->round[j]   = (q_rounding_factors_tbl[i] * tmp) >> 7;
            qp->dequant[j] = tmp;
        }

        qp = &ctx->qp_y2[i];
        for (j = 0; j < 2; j++) {
            if (j == 0) {
                tmp = dc_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
                tmp = tmp * 2;
            } else {
                tmp = ac_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
                tmp = (tmp * 155) / 100;
                if (tmp < 8)
                    tmp = 8;
            }
            qp->quant[j]   = MPP_MIN((1 << 16) / tmp, 0x3FFF);
            qp->zbin[j]    = ((q_zbin_factors_tbl[i] * tmp) + 64) >> 7;
            qp->round[j]   = (q_rounding_factors_tbl[i] * tmp) >> 7;
            qp->dequant[j] = tmp;
        }

        qp = &ctx->qp_ch[i];
        for (j = 0; j < 2; j++) {
            if (j == 0) {
                tmp = dc_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
                if (tmp > 132)
                    tmp = 132;
            } else {
                tmp = ac_q_lookup_tbl[MPP_CLIP3(0, QINDEX_RANGE - 1, i)];
            }
            qp->quant[j]   = MPP_MIN((1 << 16) / tmp, 0x3FFF);
            qp->zbin[j]    = ((q_zbin_factors_tbl[i] * tmp) + 64) >> 7;
            qp->round[j]   = (q_rounding_factors_tbl[i] * tmp) >> 7;
            qp->dequant[j] = tmp;
        }
    }

    return MPP_OK;
}

MPP_RET hal_vp8e_update_buffers(void *hal, HalTaskInfo *task)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;
    const RK_U32 hw_offset = ctx->hw_cfg.first_free_bit / 8;
    RK_U32 *part = (RK_U32 *)mpp_buffer_get_ptr(buffers->hw_size_table_buf);

    ctx->bitbuf[1].byte_cnt += part[0] - hw_offset;
    ctx->bitbuf[1].data += part[0] - hw_offset;

    ctx->bitbuf[2].byte_cnt = part[1];
    ctx->bitbuf[2].data += part[1];
    ctx->bitbuf[3].byte_cnt = part[2];
    ctx->bitbuf[3].data += part[2];

    set_frame_tag(ctx);

    if (vp8e_buffer_gap(&ctx->bitbuf[1], 4) == MPP_OK) {
        set_data_part_size(ctx);
    }

    ctx->prev_frame_lost = 0;

    ctx->p_out_buf[0] = (RK_U32 *)ctx->bitbuf[0].p_data;
    ctx->p_out_buf[1] = (RK_U32 *)ctx->bitbuf[2].p_data;
    if (ctx->sps.dct_partitions)
        ctx->p_out_buf[2] = (RK_U32 *)ctx->bitbuf[3].p_data;

    ctx->stream_size[0] = ctx->bitbuf[0].byte_cnt +
                          ctx->bitbuf[1].byte_cnt;
    ctx->stream_size[1] = ctx->bitbuf[2].byte_cnt;

    if (ctx->sps.dct_partitions)
        ctx->stream_size[2] = ctx->bitbuf[3].byte_cnt;

    ctx->frame_size = ctx->stream_size[0] + ctx->stream_size[1] +
                      ctx->stream_size[2];

    update_picbuf(&ctx->picbuf);
    {
        HalEncTask *enc_task = &task->enc;
        RK_U8 *p_out = mpp_buffer_get_ptr(enc_task->output);

        if (ctx->frame_cnt == 0) {
            write_ivf_header(hal, p_out);

            p_out += IVF_HDR_BYTES;
            enc_task->length += IVF_HDR_BYTES;
        }

        if (ctx->frame_size) {
            write_ivf_frame(ctx, p_out);

            p_out += IVF_FRM_BYTES;
            enc_task->length += IVF_FRM_BYTES;
        }

        memcpy(p_out, ctx->p_out_buf[0], ctx->stream_size[0]);
        p_out += ctx->stream_size[0];
        enc_task->length += ctx->stream_size[0];

        memcpy(p_out, ctx->p_out_buf[1], ctx->stream_size[1]);
        p_out += ctx->stream_size[1];
        enc_task->length += ctx->stream_size[1];

        memcpy(p_out, ctx->p_out_buf[2], ctx->stream_size[2]);
        p_out += ctx->stream_size[2];
        enc_task->length += ctx->stream_size[2];
    }
    return MPP_OK;
}

MPP_RET hal_vp8e_setup(void *hal)
{
    MPP_RET ret = MPP_OK;
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;

    if (set_parameter(ctx)) {
        mpp_err("set vp8e parameter failed");
        return MPP_NOK;
    }

    if (set_picbuf(ctx)) {
        mpp_err("set vp8e picbuf failed, no enough memory");
        return MPP_ERR_NOMEM;
    }

    ret = alloc_buffer(ctx);

    return ret;
}

MPP_RET hal_vp8e_buf_free(void *hal)
{
    HalVp8eCtx *ctx = (HalVp8eCtx *)hal;
    Vp8eVpuBuf *buffers = (Vp8eVpuBuf *)ctx->buffers;

    if (buffers->hw_luma_buf) {
        mpp_buffer_put(buffers->hw_luma_buf);
        buffers->hw_luma_buf = NULL;
    }

    {
        RK_U32 i = 0;
        for (i = 0; i < 2; i++) {
            if (buffers->hw_cbcr_buf[i]) {
                mpp_buffer_put(buffers->hw_cbcr_buf[i]);
                buffers->hw_cbcr_buf[i] = NULL;
            }
        }
    }

    if (buffers->hw_size_table_buf) {
        mpp_buffer_put(buffers->hw_size_table_buf);
        buffers->hw_size_table_buf = NULL;
    }

    if (buffers->hw_cabac_table_buf) {
        mpp_buffer_put(buffers->hw_cabac_table_buf);
        buffers->hw_cabac_table_buf = NULL;
    }

    if (buffers->hw_mv_output_buf) {
        mpp_buffer_put(buffers->hw_mv_output_buf);
        buffers->hw_mv_output_buf = NULL;
    }

    if (buffers->hw_prob_count_buf) {
        mpp_buffer_put(buffers->hw_prob_count_buf);
        buffers->hw_prob_count_buf = NULL;
    }

    if (buffers->hw_segment_map_buf) {
        mpp_buffer_put(buffers->hw_segment_map_buf);
        buffers->hw_segment_map_buf = NULL;
    }

    if (buffers->hw_out_buf) {
        mpp_buffer_put(buffers->hw_out_buf);
        buffers->hw_out_buf = NULL;
    }

    if (buffers->hw_buf_grp) {
        mpp_buffer_group_put(buffers->hw_buf_grp);
        buffers->hw_buf_grp = NULL;
    }

    return MPP_OK;
}
