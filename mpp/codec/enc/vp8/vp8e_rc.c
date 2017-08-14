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

#define MODULE_TAG "vp8e_rc"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_rc.h"

#include "vp8e_syntax.h"
#include "vp8e_debug.h"
#include "vp8e_rc.h"

#define DSCY                         64
#define UPSCALE                    8000
#define I32_MPP_MAX          0x7fffffff
#define QINDEX_RANGE                128
#define RC_ERROR_RESET       0x7fffffff
#define BIT_COUNT_MAX        0x1fffffff
#define BIT_COUNT_MIN        (-BIT_COUNT_MAX)

static const RK_S32 ac_q_lookup_tbl[QINDEX_RANGE] = {
    4,   5,   6,   7,   8,   9,   10,  11,  12,  13,
    14,  15,  16,  17,  18,  19,  20,  21,  22,  23,
    24,  25,  26,  27,  28,  29,  30,  31,  32,  33,
    34,  35,  36,  37,  38,  39,  40,  41,  42,  43,
    44,  45,  46,  47,  48,  49,  50,  51,  52,  53,
    54,  55,  56,  57,  58,  60,  62,  64,  66,  68,
    70,  72,  74,  76,  78,  80,  82,  84,  86,  88,
    90,  92,  94,  96,  98,  100, 102, 104, 106, 108,
    110, 112, 114, 116, 119, 122, 125, 128, 131, 134,
    137, 140, 143, 146, 149, 152, 155, 158, 161, 164,
    167, 170, 173, 177, 181, 185, 189, 193, 197, 201,
    205, 209, 213, 217, 221, 225, 229, 234, 239, 245,
    249, 254, 259, 264, 269, 274, 279, 284
};

static RK_S32 initial_qp(RK_S32 bits, RK_S32 pels)
{
    RK_S32 i = -1;
    static const RK_S32 qp_tbl[2][12] = {
        {47, 57, 73, 93, 122, 155, 214, 294, 373, 506, 781, 0x7FFFFFFF},
        {120, 110, 100, 90, 80, 70, 60, 50, 40, 30, 20, 10}
    };

    if (bits > 1000000)
        return 10;

    pels >>= 8;
    bits >>= 5;

    bits *= pels + 250;
    bits /= 350 + (3 * pels) / 4;
    bits = axb_div_c(bits, UPSCALE, pels << 6);

    while (qp_tbl[0][++i] < bits);

    return qp_tbl[1][i];
}

static MPP_RET update_rc_error(Vp8eLinReg *p, RK_S32 bits)
{
    p->len = 3;

    if (bits >= (RK_S32)I32_MPP_MAX) {
        p->bits[0] = 0;
        p->bits[1] = 0;
        p->bits[2] = 0;
        return MPP_NOK;
    }

    p->bits[0] = bits - p->bits[2];
    p->bits[1] = bits + p->bits[1];
    p->bits[2] = bits;

    return MPP_OK;
}

static RK_S32 lin_sxy(RK_S32 *qp, RK_S32 *r, RK_S32 n)
{
    RK_S32 tmp, sum = 0;

    while (n--) {
        tmp = qp[n] * qp[n] * qp[n];
        if (tmp > r[n]) {
            sum += MPP_DIV_SIGN(tmp, DSCY) * r[n];
        } else {
            sum += tmp * MPP_DIV_SIGN(r[n], DSCY);
        }

        if (sum < 0) {
            return I32_MPP_MAX;
        }
    }

    return sum;
}

static RK_S32 lin_sx(RK_S32 *qp, RK_S32 n)
{
    RK_S32 tmp = 0;

    while (n--) {
        tmp += qp[n];
    }

    return tmp;
}

static RK_S32 lin_sy(RK_S32 *qp, RK_S32 *r, RK_S32 n)
{
    RK_S32 sum = 0;

    while (n--) {
        sum += qp[n] * qp[n] * r[n];
        if (sum < 0) {
            return 2147483647 / 64;
        }
    }

    return MPP_DIV_SIGN(sum, DSCY);
}

static RK_S32 lin_nsxx(RK_S32 *qp, RK_S32 n)
{
    RK_S32 tmp = 0;
    RK_S32 sum = 0;
    RK_S32 d = n;

    while (n--) {
        tmp = qp[n];
        tmp *= tmp;
        sum += d * tmp;
    }

    return sum;
}

static void update_model(Vp8eLinReg *p)
{
    RK_S32 a1, a2;

    RK_S32 *qs = p->qs;
    RK_S32 *r = p->bits;
    RK_S32 n = p->len;
    RK_S32 sx = lin_sx(qs, n);
    RK_S32 sy = lin_sy(qs, r, n);

    a1 = lin_sxy(qs, r, n);
    a1 = (a1 < (I32_MPP_MAX / n)) ? (a1 * n) : I32_MPP_MAX;

    if (sy == 0) {
        a1 = 0;
    } else {
        a1 -= (sx < I32_MPP_MAX / sy) ? (sx * sy) : I32_MPP_MAX;
    }

    a2 = (lin_nsxx(qs, n) - (sx * sx));
    if (a2 == 0) {
        if (p->a1 == 0) {
            a1 = 0;
        } else {
            a1 = (p->a1 * 2) / 3;
        }
    } else {
        a1 = axb_div_c(a1, DSCY, a2);
    }

    a1 = MPP_MAX(a1, -4096 * DSCY);
    a1 = MPP_MIN(a1,  4096 * DSCY - 1);

    a2 = MPP_DIV_SIGN(sy * DSCY, n) - MPP_DIV_SIGN(a1 * sx, n);

    if (p->len > 0) {
        p->a1 = a1;
        p->a2 = a2;
    }

}

static RK_S32 get_vir_buffer_bitcnt(Vp8eVirBuf *vb, RK_S32 time_inc)
{
    RK_S32 drift = 0;
    RK_S32 target = 0;

    /* Saturate realBitCnt, this is to prevent overflows caused by much greater
       bitrate setting than is really possible to reach */
    if (vb->real_bit_cnt > BIT_COUNT_MAX)
        vb->real_bit_cnt = BIT_COUNT_MAX;
    if (vb->real_bit_cnt < BIT_COUNT_MIN)
        vb->real_bit_cnt = BIT_COUNT_MIN;

    vb->pic_time_inc    += time_inc;
    vb->virtual_bit_cnt += axb_div_c(vb->bit_rate, time_inc,
                                     vb->time_scale);
    target = vb->virtual_bit_cnt - vb->real_bit_cnt;

    /* Saturate target, prevents rc going totally out of control.
       This situation should never happen. */
    if (target > BIT_COUNT_MAX)
        target = BIT_COUNT_MAX;
    if (target < BIT_COUNT_MIN)
        target = BIT_COUNT_MIN;

    while (vb->pic_time_inc >= vb->time_scale) {
        vb->pic_time_inc    -= vb->time_scale;
        vb->virtual_bit_cnt -= vb->bit_rate;
        vb->real_bit_cnt    -= vb->bit_rate;
    }

    drift = axb_div_c(vb->bit_rate, vb->pic_time_inc, vb->time_scale);
    drift -= vb->virtual_bit_cnt;
    vb->virtual_bit_cnt += drift;

    return target;
}

static MPP_RET skip_pic(Vp8eRc *rc)
{
    Vp8eVirBuf *vb = &rc->virbuf;
    RK_S32 skip_inc_limit = -vb->bit_per_pic / 3;
    RK_S32 skip_dec_limit = vb->bit_per_pic / 3;
    RK_S32 bit_available = vb->virtual_bit_cnt - vb->real_bit_cnt;

    if (((rc->pic_rc_enable == 0) || (vb->skip_frame_target == 0)) &&
        (bit_available < skip_inc_limit))
        vb->skip_frame_target++;

    if ((bit_available > skip_dec_limit) && vb->skip_frame_target > 0)
        vb->skip_frame_target--;

    if (vb->skipped_frames < vb->skip_frame_target) {
        vb->skipped_frames++;
        rc->frame_coded = 0;
    } else {
        vb->skipped_frames = 0;
    }

    return MPP_OK;
}

static RK_S32 new_pic_quant(Vp8eLinReg *p, RK_S32 bits, RK_U8 use_qp_delta_limit)
{
    RK_S32 tmp, diff;
    RK_S32 qp = p->qp_prev;
    RK_S32 qp_best = p->qp_prev;
    RK_S32 diff_best = I32_MPP_MAX;

    if (p->a1 == 0 && p->a2 == 0) {
        return qp;
    }

    if (bits <= 0) {
        if (use_qp_delta_limit)
            qp = MPP_MIN(QINDEX_RANGE - 1, MPP_MAX(0, qp + 4));
        else
            qp = MPP_MIN(QINDEX_RANGE - 1, MPP_MAX(0, qp + 10));

        return qp;
    }

    do {
        tmp  = MPP_DIV_SIGN(p->a1, ac_q_lookup_tbl[qp]);
        tmp += MPP_DIV_SIGN(p->a2, ac_q_lookup_tbl[qp] * ac_q_lookup_tbl[qp]);
        diff = MPP_ABS(tmp - bits);

        if (diff < diff_best) {
            diff_best = diff;
            qp_best   = qp;
            if ((tmp - bits) <= 0) {
                if (qp < 1) {
                    break;
                }
                qp--;
            } else {
                if (qp >= QINDEX_RANGE - 1) {
                    break;
                }
                qp++;
            }
        } else {
            break;
        }
    } while ((qp >= 0) && (qp < QINDEX_RANGE));
    qp = qp_best;

    if (use_qp_delta_limit) {
        tmp = qp - p->qp_prev;
        if (tmp > 4) {
            qp = p->qp_prev + 4;
        } else if (tmp < -4) {
            qp = p->qp_prev - 4;
        }
    }

    return qp;
}

static RK_S32 avg_rc_error(Vp8eLinReg *p)
{
    return MPP_DIV_SIGN(p->bits[2] * 4 + p->bits[1] * 6 + p->bits[0] * 0, 100);
}

static MPP_RET cal_pic_quant(Vp8eRc *rc)
{
    RK_S32 tmp = 0;
    RK_S32 tmp_value = 0;
    RK_S32 tmp_avg_rc_error;
    RK_U8  use_qp_delta_limit = 1;

    if (!rc->pic_rc_enable) {
        rc->qp_hdr = rc->fixed_qp;
        return MPP_OK;
    }

    if (rc->curr_frame_intra) {
        if (rc->gop_len == 1 || rc->gop_len == 2) {
            tmp = new_pic_quant(&rc->lin_reg,
                                axb_div_c(rc->target_pic_size, 256, rc->mb_per_pic),
                                use_qp_delta_limit);
        } else {
            if (rc->gop_qp_sum) {
                tmp = MPP_DIV_SIGN(rc->gop_qp_sum, rc->gop_qp_div);
            }
            rc->gop_qp_sum = 0;
            rc->gop_qp_div = 0;
        }
        if (tmp) {
            rc->qp_hdr = tmp;
        }
    } else if (rc->prev_frame_intra) {
        rc->qp_hdr = rc->qp_hdr_prev;
    } else {
        tmp_avg_rc_error = avg_rc_error(&rc->r_error);
        tmp_value = axb_div_c(rc->target_pic_size - tmp_avg_rc_error,
                              256, rc->mb_per_pic);
        rc->qp_hdr = new_pic_quant(&rc->lin_reg, tmp_value,
                                   use_qp_delta_limit);
    }

    vp8e_rc_dbg_rc("frame_cnt = %d, qp = %d\n", rc->frame_cnt, rc->qp_hdr);

    rc->qp_hdr = MPP_MIN(rc->qp_max, MPP_MAX(rc->qp_min, rc->qp_hdr));
    rc->qp_hdr_prev = rc->qp_hdr;

    if (rc->curr_frame_intra) {
        if (rc->fixed_intra_qp)
            rc->qp_hdr = rc->fixed_intra_qp;
        else if (!rc->prev_frame_intra)
            rc->qp_hdr += rc->intra_qp_delta;

        rc->qp_hdr = MPP_MIN(rc->qp_max, MPP_MAX(rc->qp_min, rc->qp_hdr));
    } else {
        rc->gop_qp_sum += rc->qp_hdr;
        rc->gop_qp_div++;
    }

    return MPP_OK;
}

static void update_tables(Vp8eLinReg *p, RK_S32 qp, RK_S32 bits)
{
    RK_S32 len = 10;
    RK_S32 tmp = p->pos;

    p->qp_prev = qp;
    p->qs[tmp] = ac_q_lookup_tbl[qp];
    p->bits[tmp] = bits;

    if ((++p->pos) >= len) {
        p->pos = 0;
    }

    if (p->len < len) {
        p->len++;
    }
}

MPP_RET vp8e_update_rc_cfg(Vp8eRc *rc, MppEncRcCfg *cfg)
{
    RK_U32 change = cfg->change;
    Vp8eVirBuf *vb = &rc->virbuf;

    if (change & MPP_ENC_RC_CFG_CHANGE_BPS) {
        vp8e_rc_dbg_cfg("bps: %d [%d %d]\n",
                        cfg->bps_target, cfg->bps_min, cfg->bps_max);
        vb->bps_min = cfg->bps_min;
        vb->bps_max = cfg->bps_max;
        vb->bit_rate = cfg->bps_target;
    }

    if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
        vp8e_rc_dbg_cfg("fps: %d / %d\n", cfg->fps_out_num, cfg->fps_out_denorm);
        rc->fps_out_num = cfg->fps_out_num;
        rc->fps_out_denorm = cfg->fps_out_denorm;
        if (rc->fps_out_denorm == 0) {
            mpp_err("denorm can not be 0, change to default 1");
            rc->fps_out_denorm = 1;
        }
        rc->fps_out = rc->fps_out_num / rc->fps_out_denorm;
        if (rc->fps_out == 0) {
            rc->fps_out = 30;
            rc->fps_out_num = 30;
            rc->fps_out_denorm = 1;
            mpp_err("fps out can not be 0, change to default 30");
        }
    }

    if ((change & MPP_ENC_RC_CFG_CHANGE_GOP) &&
        (rc->gop_len != cfg->gop)) {
        rc->gop_len = cfg->gop;
        vp8e_rc_dbg_cfg("gop: %d\n", cfg->gop);
    }
    vb->bit_per_pic = axb_div_c(vb->bit_rate, rc->fps_out_denorm, rc->fps_out_num);

    cfg->change = 0;

    return MPP_OK;
}


MPP_RET vp8e_init_rc(Vp8eRc *rc, MppEncCfgSet *cfg)
{
    RK_S32 max_bps;
    Vp8eVirBuf *vb = &rc->virbuf;

    rc->qp_hdr = -1;
    rc->qp_min = 0;
    rc->qp_max = QINDEX_RANGE;
    rc->pic_skip = 0;
    rc->pic_rc_enable = 1;
    rc->gop_len = cfg->rc.gop;
    rc->intra_qp_delta = 0;
    rc->fixed_intra_qp = 0;
    rc->intra_picture_rate = 30;
    rc->golden_picture_rate = 0;
    rc->altref_picture_rate = 0;
    rc->virbuf.bit_rate = cfg->rc.bps_target;
    rc->fps_out_denorm = cfg->rc.fps_out_denorm;
    rc->fps_out_num = cfg->rc.fps_out_num;
    rc->mb_per_pic = ((cfg->prep.width + 15) / 16) * ((cfg->prep.height + 15) / 16);

    if (rc->qp_max >= QINDEX_RANGE)
        rc->qp_max = QINDEX_RANGE - 1;

    if (rc->qp_min < 0)
        rc->qp_min = 0;

    max_bps = rc->mb_per_pic * 16 * 16 * 6;
    max_bps = axb_div_c(max_bps, rc->fps_out_num,
                        rc->fps_out_denorm);

    if (max_bps < 0)
        max_bps = I32_MPP_MAX;
    vb->bit_rate = MPP_MIN(vb->bit_rate, max_bps);
    vb->bit_per_pic = axb_div_c(vb->bit_rate, rc->fps_out_denorm,
                                rc->fps_out_num);

    if (rc->qp_hdr == -1)
        rc->qp_hdr = initial_qp(vb->bit_per_pic, rc->mb_per_pic * 16 * 16);

    rc->qp_hdr = MPP_MIN(rc->qp_max, MPP_MAX(rc->qp_min, rc->qp_hdr));
    vp8e_rc_dbg_rc("init qp, qp = %d, bitRate = %d, bitPerPic = %d\n",
                   rc->qp_hdr, vb->bit_rate, vb->bit_per_pic);

    rc->qp_hdr_prev       = rc->qp_hdr;
    rc->fixed_qp          = rc->qp_hdr;
    rc->frame_coded       = 1;
    rc->curr_frame_intra  = 1;
    rc->prev_frame_intra  = 0;
    rc->frame_cnt         = 0;
    rc->gop_qp_sum        = 0;
    rc->gop_qp_div        = 0;
    rc->target_pic_size   = 0;
    rc->frame_bit_cnt     = 0;
    rc->time_inc          = 0;

    memset(&rc->lin_reg, 0, sizeof(rc->lin_reg));
    rc->lin_reg.qs[0]    = ac_q_lookup_tbl[QINDEX_RANGE - 1];
    rc->lin_reg.qp_prev  = rc->qp_hdr;

    vb->gop_rem          = rc->gop_len;
    vb->time_scale       = rc->fps_out_num;

    update_rc_error(&rc->r_error, RC_ERROR_RESET);

    return MPP_OK;
}

MPP_RET vp8e_before_pic_rc(Vp8eRc *rc)
{
    RK_S32 tmp = 0;
    Vp8eVirBuf *vb = &rc->virbuf;

    rc->frame_coded = 1;

    if (rc->curr_frame_intra || vb->gop_rem == 1) {
        vb->gop_rem = rc->gop_len;
    } else {
        vb->gop_rem--;
    }

    tmp = get_vir_buffer_bitcnt(&rc->virbuf, (RK_S32)rc->time_inc);

    rc->target_pic_size = vb->bit_per_pic +
                          MPP_DIV_SIGN(tmp, MPP_MAX(vb->gop_rem, 3));
    rc->target_pic_size = MPP_MAX(0, rc->target_pic_size);

    if (rc->pic_skip)
        skip_pic(rc);

    cal_pic_quant(rc);

    return MPP_OK;
}

MPP_RET vp8e_after_pic_rc(Vp8eRc *rc, RK_S32 bitcnt)
{
    Vp8eVirBuf *vb = &rc->virbuf;

    rc->time_inc = 1;
    rc->frame_cnt++;
    rc->frame_bit_cnt = bitcnt;
    rc->prev_frame_intra = rc->curr_frame_intra;
    vb->real_bit_cnt += bitcnt;

    if ((!rc->curr_frame_intra) || (rc->gop_len == 1)) {
        update_tables(&rc->lin_reg, rc->qp_hdr_prev,
                      axb_div_c(bitcnt, 256, rc->mb_per_pic));


        if (vb->gop_rem == rc->gop_len - 1) {
            update_rc_error(&rc->r_error, RC_ERROR_RESET);
        } else {
            update_rc_error(&rc->r_error,
                            MPP_MIN(bitcnt - rc->target_pic_size,
                                    2 * rc->target_pic_size));
        }
        update_model(&rc->lin_reg);
    }

    return MPP_OK;
}
