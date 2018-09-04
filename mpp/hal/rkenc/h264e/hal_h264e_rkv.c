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

#define MODULE_TAG "hal_h264e_rkv"

#include <string.h>
#include <math.h>
#include <limits.h>

#include "mpp_device.h"
#include "mpp_common.h"
#include "mpp_mem.h"

#include "h264_syntax.h"
#include "hal_h264e_rkv.h"
#include "hal_h264e_rkv_dpb.h"
#include "hal_h264e_rkv_stream.h"
#include "hal_h264e_rkv_utils.h"
#include "hal_h264e_rkv_nal.h"

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

static H264eRkvMbRcMcfg mb_rc_m_cfg[H264E_MB_RC_M_NUM] = {
    /* aq_prop, aq_strength, mb_num, qp_range */
    {16,        1,           0,      1}, // mode = 0
    {8,         1,           1,      2}, // mode = 1
    {4,         1,           1,      4}, // mode = 2
    {8,         2,           1,     10}, // mode = 3
    {8,        1.8,          1,     10}, // mode = 4
    {0,         0,           1,     15}, // mode = 5
    {16,       2.8,          1,     20}, // mode = 6
};


static H264eRkvMbRcQRcfg mb_rc_qr_cfg[9] = {
    /*qp min offset to qp hdr, qp_range  */
    {8,  0,                      10}, //qp 0 - 5
    {8,  0,                      10}, //qp 6 - 11
    {6,  0,                      8 }, //qp 12 - 17
    {6,  0,                      8 }, //qp 18 - 23
    {4,  0,                      6 }, //qp 24 - 29
    {4,  0,                      5 }, //qp 30 - 35
    {0,  4,                      6 }, //qp 36 - 41
    {0,  2,                      8 }, //qp 42 - 45
    {0,  2,                      5 }, //qp 46 - 51
};

static H264eRkvMbRcQcfg mb_rc_q_cfg[MPP_ENC_RC_QUALITY_BUTT] = {
    /* qp_min, qp_max */
    {31,       51}, // worst
    {28,       46}, // worse
    {24,       42}, // medium
    {20,       39}, // better
    {16,       35}, // best
    {0,         0}, // cqp
};

static double QP2Qstep( double qp_avg )
{
    RK_S32 i;
    double Qstep;
    RK_S32 QP = qp_avg * 4.0;

    Qstep = 0.625 * pow(1.0293, QP % 24);
    for (i = 0; i < (QP / 24); i++)
        Qstep *= 2;

    return round(Qstep * 4);
}

static MPP_RET h264e_rkv_free_buffers(H264eHalContext *ctx)
{
    RK_S32 k = 0;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    h264e_hal_enter();
    for (k = 0; k < 2; k++) {
        if (buffers->hw_pp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_pp_buf[k])) {
                h264e_hal_err("hw_pp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < 2; k++) {
        if (buffers->hw_dsp_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_dsp_buf[k])) {
                h264e_hal_err("hw_dsp_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }
    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_mei_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_mei_buf[k])) {
                h264e_hal_err("hw_mei_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
        if (buffers->hw_roi_buf[k]) {
            if (MPP_OK != mpp_buffer_put(buffers->hw_roi_buf[k])) {
                h264e_hal_err("hw_roi_buf[%d] put failed", k);
                return MPP_NOK;
            }
        }
    }

    {
        RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
        for (k = 0; k < num_buf; k++) {
            if (buffers->hw_rec_buf[k]) {
                if (MPP_OK != mpp_buffer_put(buffers->hw_rec_buf[k])) {
                    h264e_hal_err("hw_rec_buf[%d] put failed", k);
                    return MPP_NOK;
                }
            }
        }
    }

    h264e_hal_leave();

    return MPP_OK;
}

static MPP_RET
h264e_rkv_allocate_buffers(H264eHalContext *ctx, H264eHwCfg *hw_cfg)
{
    RK_S32 k = 0;
    MPP_RET ret = MPP_OK;
    h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 num_mbs_oneframe;
    RK_U32 frame_size;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvFrame *frame_buf = dpb_ctx->frame_buf;
    H264eRkvCsp input_fmt = H264E_RKV_CSP_YUV420P;
    h264e_hal_enter();

    if (ctx->alloc_flg) {
        MppEncCfgSet *cfg = ctx->cfg;
        MppEncPrepCfg *prep = &cfg->prep;

        num_mbs_oneframe = (prep->width + 15) / 16 * ((prep->height + 15) / 16);
        frame_size = MPP_ALIGN(prep->width, 16) * MPP_ALIGN(prep->height, 16);

        h264e_rkv_set_format(hw_cfg, prep);
    } else {
        num_mbs_oneframe = (hw_cfg->width + 15) / 16 * ((hw_cfg->height + 15) / 16);
        frame_size = MPP_ALIGN(hw_cfg->width, 16) * MPP_ALIGN(hw_cfg->height, 16);
    }

    input_fmt = (H264eRkvCsp)hw_cfg->input_format;
    switch (input_fmt) {
    case H264E_RKV_CSP_YUV420P:
    case H264E_RKV_CSP_YUV420SP: {
        frame_size = frame_size * 3 / 2;
    } break;
    case H264E_RKV_CSP_YUV422P:
    case H264E_RKV_CSP_YUV422SP:
    case H264E_RKV_CSP_YUYV422:
    case H264E_RKV_CSP_UYVY422:
    case H264E_RKV_CSP_BGR565: {
        frame_size *= 2;
    } break;
    case H264E_RKV_CSP_BGR888: {
        frame_size *= 3;
    } break;
    case H264E_RKV_CSP_BGRA8888: {
        frame_size *= 4;
    } break;
    default: {
        h264e_hal_err("invalid src color space: %d\n", hw_cfg->input_format);
        return MPP_NOK;
    }
    }

    if (frame_size > ctx->frame_size) {
        h264e_rkv_free_buffers(ctx);

        if (hw_cfg->preproc_en) {
            for (k = 0; k < 2; k++) {
                ret = mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_PP],
                                     &buffers->hw_pp_buf[k], frame_size);
                if (ret) {
                    h264e_hal_err("hw_pp_buf[%d] get failed", k);
                    return ret;
                }
            }
        }

        for (k = 0; k < 2; k++) {
            ret = mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_DSP],
                                 &buffers->hw_dsp_buf[k], frame_size / 16);
            if (ret) {
                h264e_hal_err("hw_dsp_buf[%d] get failed", k);
                return ret;
            }
        }

#if 0 //default setting
        RK_U32 num_mei_oneframe = (syn->width + 255) / 256 * ((syn->height + 15) / 16);
        for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
            if (MPP_OK != mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_MEI], &buffers->hw_mei_buf[k], num_mei_oneframe * 16 * 4)) {
                h264e_hal_err("hw_mei_buf[%d] get failed", k);
                return MPP_ERR_MALLOC;
            } else {
                h264e_hal_dbg(H264E_DBG_DPB, "hw_mei_buf[%d] %p done, fd %d", k, buffers->hw_mei_buf[k], mpp_buffer_get_fd(buffers->hw_mei_buf[k]));
            }
        }
#endif

        if (hw_cfg->roi_en) {
            for (k = 0; k < RKV_H264E_LINKTABLE_FRAME_NUM; k++) {
                ret = mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_ROI],
                                     &buffers->hw_roi_buf[k], num_mbs_oneframe * 1);
                if (ret) {
                    h264e_hal_err("hw_roi_buf[%d] get failed", k);
                    return ret;
                }
            }
        }

        {
            RK_S32 num_buf = MPP_ARRAY_ELEMS(buffers->hw_rec_buf);
            for (k = 0; k < num_buf; k++) {
                ret = mpp_buffer_get(buffers->hw_buf_grp[H264E_HAL_RKV_BUF_GRP_REC],
                                     &buffers->hw_rec_buf[k], frame_size);
                if (ret) {
                    h264e_hal_err("hw_rec_buf[%d] get failed", k);
                    return ret;
                }
                frame_buf[k].hw_buf = buffers->hw_rec_buf[k];
            }
        }

        ctx->frame_size = frame_size;
    }

    h264e_hal_leave();
    return ret;
}

static void h264e_rkv_adjust_param(H264eHalContext *ctx)
{
    H264eHalParam *par = &ctx->param;
    (void)par;
}

static void h264e_rkv_reference_deinit( H264eRkvDpbCtx *dpb_ctx)
{
    H264eRkvDpbCtx *d_ctx = (H264eRkvDpbCtx *)dpb_ctx;

    h264e_hal_enter();

    MPP_FREE(d_ctx->frames.unused);

    h264e_hal_leave();
}

static void h264e_rkv_reference_init( void *dpb,  H264eHalParam *par)
{
    //RK_S32 k = 0;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)dpb;
    H264eRefParam *ref_cfg = &par->ref;

    h264e_hal_enter();
    memset(dpb_ctx, 0, sizeof(H264eRkvDpbCtx));

    dpb_ctx->frames.unused = mpp_calloc(H264eRkvFrame *, H264E_REF_MAX + 1);
    //for(k=0; k<RKV_H264E_REF_MAX+1; k++) {
    //    dpb_ctx->frames.reference[k] = &dpb_ctx->frame_buf[k];
    //    h264e_hal_dbg(H264E_DBG_DPB, "dpb_ctx->frames.reference[%d]: %p", k, dpb_ctx->frames.reference[k]);
    //}
    dpb_ctx->i_long_term_reference_flag = ref_cfg->i_long_term_en;
    dpb_ctx->i_tmp_idr_pic_id = 0;

    h264e_hal_leave();
}

MPP_RET hal_h264e_rkv_init(void *hal, MppHalCfg *cfg)
{
    RK_U32 k = 0;
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvDpbCtx *dpb_ctx = NULL;
    h264e_hal_rkv_buffers *buffers = NULL;

    h264e_hal_enter();

    ctx->ioctl_input    = mpp_calloc(H264eRkvIoctlInput, 1);
    ctx->ioctl_output   = mpp_calloc(H264eRkvIoctlOutput, 1);
    ctx->regs           = mpp_calloc(H264eRkvRegSet, RKV_H264E_LINKTABLE_FRAME_NUM);
    ctx->buffers        = mpp_calloc(h264e_hal_rkv_buffers, 1);
    ctx->extra_info     = mpp_calloc(H264eRkvExtraInfo, 1);
    ctx->dpb_ctx        = mpp_calloc(H264eRkvDpbCtx, 1);
    ctx->param_buf      = mpp_calloc_size(void,  H264E_EXTRA_INFO_BUF_SIZE);
    mpp_packet_init(&ctx->packeted_param, ctx->param_buf, H264E_EXTRA_INFO_BUF_SIZE);
    h264e_rkv_init_extra_info(ctx->extra_info);
    h264e_rkv_reference_init(ctx->dpb_ctx, &ctx->param);

    ctx->int_cb = cfg->hal_int_cb;
    ctx->frame_cnt = 0;
    ctx->frame_cnt_gen_ready = 0;
    ctx->frame_cnt_send_ready = 0;
    ctx->num_frames_to_send = 1;
    ctx->osd_plt_type = H264E_OSD_PLT_TYPE_NONE;
    ctx->hw_cfg.roi_en = 1;

    /* support multi-refs */
    dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    dpb_ctx->i_frame_cnt = 0;
    dpb_ctx->i_frame_num = 0;

    MppDevCfg dev_cfg = {
        .type = MPP_CTX_ENC,            /* type */
        .coding = MPP_VIDEO_CodingAVC,  /* coding */
        .platform = 0,                  /* platform */
        .pp_enable = 0,                 /* pp_enable */
    };
    ret = mpp_device_init(&ctx->dev_ctx, &dev_cfg);
    if (ret != MPP_OK) {
        h264e_hal_err("mpp_device_init failed. ret: %d\n", ret);
        return ret;
    }

    buffers = (h264e_hal_rkv_buffers *)ctx->buffers;
    for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
        if (MPP_OK != mpp_buffer_group_get_internal(&buffers->hw_buf_grp[k], MPP_BUFFER_TYPE_ION)) {
            h264e_hal_err("buf group[%d] get failed", k);
            return MPP_ERR_MALLOC;
        }
    }

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_deinit(void *hal)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    MPP_FREE(ctx->regs);
    MPP_FREE(ctx->ioctl_input);
    MPP_FREE(ctx->ioctl_output);
    MPP_FREE(ctx->param_buf);

    if (ctx->buffers) {
        RK_U32 k = 0;
        h264e_hal_rkv_buffers *buffers = (h264e_hal_rkv_buffers *)ctx->buffers;

        h264e_rkv_free_buffers(ctx);
        for (k = 0; k < H264E_HAL_RKV_BUF_GRP_BUTT; k++) {
            if (buffers->hw_buf_grp[k]) {
                if (MPP_OK != mpp_buffer_group_put(buffers->hw_buf_grp[k])) {
                    h264e_hal_err("buf group[%d] put failed", k);
                    return MPP_NOK;
                }
            }
        }

        MPP_FREE(ctx->buffers);
    }

    if (ctx->extra_info) {
        h264e_rkv_deinit_extra_info(ctx->extra_info);
        MPP_FREE(ctx->extra_info);
    }

    if (ctx->packeted_param) {
        mpp_packet_deinit(&ctx->packeted_param);
        ctx->packeted_param = NULL;
    }

    if (ctx->dpb_ctx) {
        h264e_rkv_reference_deinit(ctx->dpb_ctx);
        MPP_FREE(ctx->dpb_ctx);
    }

    if (ctx->qp_p) {
        mpp_data_deinit(ctx->qp_p);
        ctx->qp_p = NULL;
    }

    if (ctx->sse_p) {
        mpp_data_deinit(ctx->sse_p);
        ctx->qp_p = NULL;
    }

    if (ctx->intra_qs) {
        mpp_linreg_deinit(ctx->intra_qs);
        ctx->intra_qs = NULL;
    }

    if (ctx->inter_qs) {
        mpp_linreg_deinit(ctx->inter_qs);
        ctx->inter_qs = NULL;
    }

    if (ctx->dev_ctx) {
        ret = mpp_device_deinit(ctx->dev_ctx);
        if (ret)
            h264e_hal_err("mpp_device_deinit failed. ret: %d\n", ret);
    }

    h264e_hal_leave();
    return MPP_OK;
}

static MPP_RET
h264e_rkv_set_ioctl_extra_info(H264eRkvIoctlExtraInfo *extra_info,
                               H264eHwCfg *syn, H264eRkvRegSet *regs)
{
    H264eRkvIoctlExtraInfoElem *info = NULL;
    RK_U32 hor_stride = regs->swreg23.src_ystrid + 1;
    RK_U32 ver_stride = syn->ver_stride ? syn->ver_stride : syn->height;
    RK_U32 frame_size = hor_stride * ver_stride;
    RK_U32 u_offset = 0, v_offset = 0;
    H264eRkvCsp input_fmt = syn->input_format;

    switch (input_fmt) {
    case H264E_RKV_CSP_YUV420P: {
        u_offset = frame_size;
        v_offset = frame_size * 5 / 4;
    } break;
    case H264E_RKV_CSP_YUV420SP:
    case H264E_RKV_CSP_YUV422SP: {
        u_offset = frame_size;
        v_offset = frame_size;
    } break;
    case H264E_RKV_CSP_YUV422P: {
        u_offset = frame_size;
        v_offset = frame_size * 3 / 2;
    } break;
    case H264E_RKV_CSP_YUYV422:
    case H264E_RKV_CSP_UYVY422: {
        u_offset = 0;
        v_offset = 0;
    } break;
    default: {
        h264e_hal_err("unknown color space: %d\n", input_fmt);
        u_offset = frame_size;
        v_offset = frame_size * 5 / 4;
    }
    }

    extra_info->magic = 0;
    extra_info->cnt = 2;

    /* input cb addr */
    info = &extra_info->elem[0];
    info->reg_idx = 71;
    info->offset  = u_offset;

    /* input cr addr */
    info = &extra_info->elem[1];
    info->reg_idx = 72;
    info->offset  = v_offset;

    return MPP_OK;
}

static void h264e_rkv_set_mb_rc(H264eHalContext *ctx)
{
    RK_S32 m = 0;
    RK_S32 q = 0;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncRcCfg *rc = &cfg->rc;
    H264eHwCfg *hw = &ctx->hw_cfg;
    H264eMbRcCtx *mb_rc = &ctx->mb_rc;

    if (rc->rc_mode == MPP_ENC_RC_MODE_VBR) {
        m = H264E_MB_RC_ONLY_QUALITY;
        q = rc->quality;
        if (q == MPP_ENC_RC_QUALITY_AQ_ONLY) {
            m = H264E_MB_RC_ONLY_AQ;
        } else if (q != MPP_ENC_RC_QUALITY_CQP) {
            /* better quality for intra frame */
            if (hw->frame_type == H264E_RKV_FRAME_I)
                q++;
            q = H264E_HAL_CLIP3(q, MPP_ENC_RC_QUALITY_WORST, MPP_ENC_RC_QUALITY_BEST);
        }
    } else { // CBR
        m = H264E_MB_RC_ONLY_BITRATE;
        if (hw->frame_type == H264E_RKV_FRAME_I)
            m--;
        m = H264E_HAL_CLIP3(m, H264E_MB_RC_ONLY_QUALITY, H264E_MB_RC_ONLY_BITRATE);
        q = -1;
    }

    mb_rc->mode = m;
    mb_rc->quality = q;
}

static MPP_RET h264e_rkv_set_rc_regs(H264eHalContext *ctx, H264eRkvRegSet *regs,
                                     H264eHwCfg *syn, RcSyntax *rc_syn)
{
    RK_U32 num_mbs_oneframe =
        (syn->width + 15) / 16 * ((syn->height + 15) / 16);
    RK_U32 mb_target_size_mul_16 = (rc_syn->bit_target << 4) / num_mbs_oneframe;
    RK_U32 mb_target_size = mb_target_size_mul_16 >> 4;
    MppEncRcCfg *rc = &ctx->cfg->rc;
    H264eMbRcCtx *mb_rc = &ctx->mb_rc;
    H264eRkvMbRcMcfg m_cfg;

    h264e_rkv_set_mb_rc(ctx);

    if (rc_syn->gop_mode == MPP_GOP_ALL_INTRA) {
        m_cfg = mb_rc_m_cfg[H264E_MB_RC_BALANCE];
    } else if ((ctx->frame_cnt == 0) || (ctx->frame_cnt == 1)) {
        /* The first and second frame(I and P frame), will be discarded.
         * just for getting real qp for target bits
         */
        m_cfg = mb_rc_m_cfg[H264E_MB_RC_WIDE_RANGE];
    } else {
        m_cfg = mb_rc_m_cfg[mb_rc->mode];
    }

    /* (VBR) if focus on quality, qp range is limited more precisely */
    if (rc->rc_mode == MPP_ENC_RC_MODE_VBR) {
        if (rc->quality == MPP_ENC_RC_QUALITY_AQ_ONLY) {
            m_cfg = mb_rc_m_cfg[mb_rc->mode];
            syn->qp = ctx->hw_cfg.qp_prev;
            if (rc_syn->aq_prop_offset) {
                syn->qp -= rc_syn->aq_prop_offset;
                syn->qp = mpp_clip(syn->qp, syn->qp_min, 33);
                if (syn->qp == 33) {
                    syn->qp_min = 24;
                }
            }
        } else if (rc->quality == MPP_ENC_RC_QUALITY_CQP) {
            syn->qp_min = syn->qp;
            syn->qp_max = syn->qp;
        } else {
            H264eRkvMbRcQcfg q_cfg = mb_rc_q_cfg[mb_rc->quality];
            syn->qp_min = q_cfg.qp_min;
            syn->qp_max = q_cfg.qp_max;
        }
    } else {
        if (ctx->frame_cnt > 1 && ctx->hw_cfg.frame_type != H264E_RKV_FRAME_I) {
            H264eRkvMbRcQRcfg qr_cfg = mb_rc_qr_cfg[syn->qp / 6];
            if (syn->qp > 45) {
                qr_cfg = mb_rc_qr_cfg[48 / 6];
            }
            if (qr_cfg.qp_min_offset)
                syn->qp_min =  (RK_U32)MPP_MAX(syn->qp_min, (RK_S32)(syn->qp - qr_cfg.qp_min_offset));
            else
                syn->qp_min =  (RK_U32)MPP_MAX(syn->qp_min, (RK_S32)(syn->qp - qr_cfg.qp_range));

            if (qr_cfg.qp_max_offset)
                syn->qp_max =  (RK_U32)MPP_MIN(syn->qp_max, (RK_S32)(syn->qp + qr_cfg.qp_max_offset));
            else
                syn->qp_max =  (RK_U32)MPP_MIN(syn->qp_max, (RK_S32)(syn->qp + qr_cfg.qp_range));

            m_cfg.qp_range = qr_cfg.qp_range;
            ctx->qp_scale = 1;
            if (rc_syn->aq_prop_offset) {
                m_cfg.aq_prop = mpp_clip((m_cfg.aq_prop + rc_syn->aq_prop_offset), 0, m_cfg.aq_prop);
            }
        } else if (ctx->hw_cfg.frame_type == H264E_RKV_FRAME_I) {
            syn->qp_min =  (RK_U32)MPP_MAX(10, syn->qp_min);
            syn->qp_max =  (RK_U32)MPP_MIN(37, syn->qp_max);
        }
    }

    regs->swreg10.pic_qp        = syn->qp;
    regs->swreg46.rc_en         = 1; //0: disable mb rc
    regs->swreg46.rc_mode       = m_cfg.aq_prop < 16; //0:frame/slice rc; 1:mbrc

    regs->swreg46.aqmode_en     = m_cfg.aq_prop && m_cfg.aq_strength;
    regs->swreg46.aq_strg       = (RK_U32)(m_cfg.aq_strength * 1.0397 * 256);
    regs->swreg54.rc_qp_mod     = 2; //sw_quality_flag;
    regs->swreg54.rc_fact0      = m_cfg.aq_prop;
    regs->swreg54.rc_fact1      = 16 - m_cfg.aq_prop;
    regs->swreg54.rc_max_qp     = syn->qp_max;
    regs->swreg54.rc_min_qp     = syn->qp_min;

    if (regs->swreg46.aqmode_en) {
        regs->swreg54.rc_max_qp = (RK_U32)MPP_MIN(syn->qp_max,
                                                  (RK_S32)(syn->qp + m_cfg.qp_range * ctx->qp_scale));
        regs->swreg54.rc_min_qp = (RK_U32)MPP_MAX(syn->qp_min,
                                                  (RK_S32)(syn->qp - m_cfg.qp_range * ctx->qp_scale));
        if (regs->swreg54.rc_max_qp < regs->swreg54.rc_min_qp)
            MPP_SWAP(RK_U32, regs->swreg54.rc_max_qp, regs->swreg54.rc_min_qp);
    }
    if (regs->swreg46.rc_mode) { //checkpoint rc open
        RK_U32 target = mb_target_size * m_cfg.mb_num;

        regs->swreg54.rc_qp_range    = m_cfg.qp_range * ctx->qp_scale;

        regs->swreg46.rc_ctu_num     = m_cfg.mb_num;
        regs->swreg55.ctu_ebits      = mb_target_size_mul_16;
        regs->swreg47.bits_error0    = (RK_S32)((pow(0.88, 4) - 1) * (double)target);
        regs->swreg47.bits_error1    = (RK_S32)((pow(0.88, 3) - 1) * (double)target);
        regs->swreg48.bits_error2    = (RK_S32)((pow(0.88, 2) - 1) * (double)target);
        regs->swreg48.bits_error3    = (RK_S32)((pow(0.88, 1) - 1) * (double)target);
        regs->swreg49.bits_error4    = (RK_S32)((pow(1.12, 2) - 1) * (double)target);
        regs->swreg49.bits_error5    = (RK_S32)((pow(1.12, 3) - 1) * (double)target);
        regs->swreg50.bits_error6    = (RK_S32)((pow(1.12, 4) - 1) * (double)target);
        regs->swreg50.bits_error7    = (RK_S32)((pow(2, 4) - 1) * (double)target);
        regs->swreg51.bits_error8    = (RK_S32)((pow(2, 5) - 1) * (double)target);

        regs->swreg52.qp_adjust0    = -4;
        regs->swreg52.qp_adjust1    = -3;
        regs->swreg52.qp_adjust2    = -2;
        regs->swreg52.qp_adjust3    = -1;
        regs->swreg52.qp_adjust4    =  1;
        regs->swreg52.qp_adjust5    =  2;
        regs->swreg53.qp_adjust6    =  3;
        regs->swreg53.qp_adjust7    =  4;
        regs->swreg53.qp_adjust8    =  8;
    }
    regs->swreg62.sli_beta_ofst     = 0;
    regs->swreg62.sli_alph_ofst     = 0;

    return MPP_OK;
}

static MPP_RET
h264e_rkv_set_osd_regs(H264eHalContext *ctx, H264eRkvRegSet *regs)
{
#define H264E_DEFAULT_OSD_INV_THR 15 //TODO: open interface later
    MppEncOSDData *osd_data = &ctx->osd_data;
    RK_U32 num = osd_data->num_region;

    if (num && osd_data->buf) { // enable OSD
        RK_S32 buf_fd = mpp_buffer_get_fd(osd_data->buf);

        if (buf_fd >= 0) {
            RK_U32 k = 0;
            MppEncOSDRegion *region = osd_data->region;

            regs->swreg65.osd_clk_sel    = 1;
            regs->swreg65.osd_plt_type   = ctx->osd_plt_type == H264E_OSD_PLT_TYPE_NONE ?
                                           H264E_OSD_PLT_TYPE_DEFAULT :
                                           ctx->osd_plt_type;

            for (k = 0; k < num; k++) {
                regs->swreg65.osd_en |= region[k].enable << k;
                regs->swreg65.osd_inv |= region[k].inverse << k;

                if (region[k].enable) {
                    regs->swreg67_osd_pos[k].lt_pos_x = region[k].start_mb_x;
                    regs->swreg67_osd_pos[k].lt_pos_y = region[k].start_mb_y;
                    regs->swreg67_osd_pos[k].rd_pos_x =
                        region[k].start_mb_x + region[k].num_mb_x - 1;
                    regs->swreg67_osd_pos[k].rd_pos_y =
                        region[k].start_mb_y + region[k].num_mb_y - 1;

                    regs->swreg68_indx_addr_i[k] =
                        buf_fd | (region[k].buf_offset << 10);
                }
            }

            if (region[0].inverse)
                regs->swreg66.osd_inv_r0 = H264E_DEFAULT_OSD_INV_THR;
            if (region[1].inverse)
                regs->swreg66.osd_inv_r1 = H264E_DEFAULT_OSD_INV_THR;
            if (region[2].inverse)
                regs->swreg66.osd_inv_r2 = H264E_DEFAULT_OSD_INV_THR;
            if (region[3].inverse)
                regs->swreg66.osd_inv_r3 = H264E_DEFAULT_OSD_INV_THR;
            if (region[4].inverse)
                regs->swreg66.osd_inv_r4 = H264E_DEFAULT_OSD_INV_THR;
            if (region[5].inverse)
                regs->swreg66.osd_inv_r5 = H264E_DEFAULT_OSD_INV_THR;
            if (region[6].inverse)
                regs->swreg66.osd_inv_r6 = H264E_DEFAULT_OSD_INV_THR;
            if (region[7].inverse)
                regs->swreg66.osd_inv_r7 = H264E_DEFAULT_OSD_INV_THR;
        }
    }

    return MPP_OK;
}

static MPP_RET
h264e_rkv_set_roi_regs(H264eHalContext *ctx, H264eRkvRegSet *regs)
{
    MppEncROICfg *cfg = &ctx->roi_data;
    h264e_hal_rkv_buffers *bufs = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U8 *roi_base;

    if (cfg->number && cfg->regions) {
        regs->swreg10.roi_enc = 1;
        regs->swreg29_ctuc_addr = mpp_buffer_get_fd(bufs->hw_roi_buf[0]);

        roi_base = (RK_U8 *)mpp_buffer_get_ptr(bufs->hw_roi_buf[0]);
        rkv_config_roi_area(ctx, roi_base);
    }

    return MPP_OK;
}

static MPP_RET h264e_rkv_set_pp_regs(H264eRkvRegSet *regs, H264eHwCfg *syn,
                                     MppEncPrepCfg *prep_cfg, MppBuffer hw_buf_w,
                                     MppBuffer hw_buf_r)
{
    RK_S32 k = 0;
    RK_S32 stridey = 0;
    RK_S32 stridec = 0;

    regs->swreg14.src_cfmt = syn->input_format;
    regs->swreg19.src_rot = prep_cfg->rotation;

    for (k = 0; k < 5; k++)
        regs->swreg21_scr_stbl[k] = 0;

    for (k = 0; k < 40; k++)
        regs->swreg22_h3d_tbl[k]  = h264e_h3d_tbl[k];

    if (syn->hor_stride) {
        stridey = syn->hor_stride - 1;
    } else {
        stridey = syn->width - 1;
        if (regs->swreg14.src_cfmt == 0 )
            stridey = (stridey + 1) * 4 - 1;
        else if (regs->swreg14.src_cfmt == 1 )
            stridey = (stridey + 1) * 3 - 1;
        else if ( regs->swreg14.src_cfmt == 2 || regs->swreg14.src_cfmt == 8
                  || regs->swreg14.src_cfmt == 9 )
            stridey = (stridey + 1) * 2 - 1;
    }
    stridec = (regs->swreg14.src_cfmt == 4 || regs->swreg14.src_cfmt == 6)
              ? stridey : ((stridey + 1) / 2 - 1);
    regs->swreg23.src_ystrid    = stridey;
    regs->swreg23.src_cstrid    = stridec;

    regs->swreg19.src_shp_y    = prep_cfg->sharpen.enable_y;
    regs->swreg19.src_shp_c    = prep_cfg->sharpen.enable_uv;
    regs->swreg19.src_shp_div  = prep_cfg->sharpen.div;
    regs->swreg19.src_shp_thld = prep_cfg->sharpen.threshold;
    regs->swreg21_scr_stbl[0]  = prep_cfg->sharpen.coef[0];
    regs->swreg21_scr_stbl[1]  = prep_cfg->sharpen.coef[1];
    regs->swreg21_scr_stbl[2]  = prep_cfg->sharpen.coef[2];
    regs->swreg21_scr_stbl[3]  = prep_cfg->sharpen.coef[3];
    regs->swreg21_scr_stbl[4]  = prep_cfg->sharpen.coef[4];

    (void)hw_buf_w;
    (void)hw_buf_r;

    return MPP_OK;
}

static RK_S32
h264e_rkv_find_best_qp(MppLinReg *ctx, MppEncH264Cfg *codec,
                       RK_S32 qp_start, RK_S64 bits)
{
    RK_S32 qp = qp_start;
    RK_S32 qp_best = qp_start;
    RK_S32 qp_min = codec->qp_min;
    RK_S32 qp_max = codec->qp_max;
    RK_S64 diff_best = INT_MAX;

    if (ctx->a == 0 && ctx->b == 0)
        return qp_best;


    h264e_hal_dbg(H264E_DBG_DETAIL, "RC: qp est target bit %lld\n", bits);
    if (bits <= 0) {
        qp_best = mpp_clip(qp_best + codec->qp_max_step, qp_min, qp_max);
    } else {
        do {
            RK_S64 est_bits = mpp_quadreg_calc(ctx, QP2Qstep(qp));
            RK_S64 diff = est_bits - bits;
            h264e_hal_dbg(H264E_DBG_DETAIL,
                          "RC: qp est qp %d qstep %f bit %lld diff %lld best %lld\n",
                          qp, QP2Qstep(qp), bits, diff, diff_best);
            if (MPP_ABS(diff) < MPP_ABS(diff_best)) {
                diff_best = MPP_ABS(diff);
                qp_best = qp;
                if (diff > 0)
                    qp++;
                else
                    qp--;
            } else
                break;
        } while (qp <= qp_max && qp >= qp_min);
        qp_best = mpp_clip(qp_best, qp_min, qp_max);
    }

    return qp_best;
}

static MPP_RET
h264e_rkv_update_hw_cfg(H264eHalContext *ctx, HalEncTask *task,
                        H264eHwCfg *hw_cfg)
{
    RK_S32 i;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    RcSyntax *rc_syn = (RcSyntax *)task->syntax.data;

    /* preprocess setup */
    if (prep->change) {
        RK_U32 change = prep->change;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
            hw_cfg->width   = prep->width;
            hw_cfg->height  = prep->height;
            hw_cfg->hor_stride = prep->hor_stride;
            hw_cfg->ver_stride = prep->ver_stride;
        }

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT) {
            hw_cfg->input_format = prep->format;
            h264e_rkv_set_format(hw_cfg, prep);
            switch (prep->color) {
            case MPP_FRAME_SPC_RGB : {
                /* BT.601 */
                /* Y  = 0.2989 R + 0.5866 G + 0.1145 B
                 * Cb = 0.5647 (B - Y) + 128
                 * Cr = 0.7132 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            case MPP_FRAME_SPC_BT709 : {
                /* BT.709 */
                /* Y  = 0.2126 R + 0.7152 G + 0.0722 B
                 * Cb = 0.5389 (B - Y) + 128
                 * Cr = 0.6350 (R - Y) + 128
                 */
                hw_cfg->color_conversion_coeff_a = 13933;
                hw_cfg->color_conversion_coeff_b = 46871;
                hw_cfg->color_conversion_coeff_c = 4732;
                hw_cfg->color_conversion_coeff_e = 35317;
                hw_cfg->color_conversion_coeff_f = 41615;
            } break;
            default : {
                hw_cfg->color_conversion_coeff_a = 19589;
                hw_cfg->color_conversion_coeff_b = 38443;
                hw_cfg->color_conversion_coeff_c = 7504;
                hw_cfg->color_conversion_coeff_e = 37008;
                hw_cfg->color_conversion_coeff_f = 46740;
            } break;
            }
        }
    }

    if (codec->change) {
        // TODO: setup sps / pps here
        hw_cfg->idr_pic_id = !ctx->idr_pic_id;
        hw_cfg->filter_disable = codec->deblock_disable;
        hw_cfg->slice_alpha_offset = codec->deblock_offset_alpha;
        hw_cfg->slice_beta_offset = codec->deblock_offset_beta;
        hw_cfg->inter4x4_disabled = (codec->profile >= 31) ? (1) : (0);
        hw_cfg->cabac_init_idc = codec->cabac_init_idc;
        hw_cfg->qp = codec->qp_init;

        if (hw_cfg->qp_prev <= 0)
            hw_cfg->qp_prev = hw_cfg->qp;
    }

    /* init qp calculate, if outside doesn't set init qp.
     * mpp will use bpp to estimates one.
     */
    if (hw_cfg->qp <= 0) {
        RK_S32 qp_tbl[2][13] = {
            {
                26, 36, 48, 63, 85, 110, 152, 208, 313, 427, 936,
                1472, 0x7fffffff
            },
            {42, 39, 36, 33, 30, 27, 24, 21, 18, 15, 12, 9, 6}
        };
        RK_S32 pels = ctx->cfg->prep.width * ctx->cfg->prep.height;
        RK_S32 bits_per_pic = axb_div_c(rc->bps_target,
                                        rc->fps_out_denorm,
                                        rc->fps_out_num);

        if (pels) {
            RK_S32 upscale = 8000;
            if (bits_per_pic > 1000000)
                hw_cfg->qp = codec->qp_min;
            else {
                RK_S32 j = -1;

                pels >>= 8;
                bits_per_pic >>= 5;

                bits_per_pic *= pels + 250;
                bits_per_pic /= 350 + (3 * pels) / 4;
                bits_per_pic = axb_div_c(bits_per_pic, upscale, pels << 6);

                while (qp_tbl[0][++j] < bits_per_pic);

                hw_cfg->qp = qp_tbl[1][j];
                hw_cfg->qp_prev = hw_cfg->qp;
            }
        }
    }

    if (NULL == ctx->intra_qs)
        mpp_linreg_init(&ctx->intra_qs, MPP_MIN(rc->gop, 15), 4);
    if (NULL == ctx->inter_qs) {
        if (rc_syn->gop_mode == MPP_GOP_ALL_INTRA)
            mpp_linreg_init(&ctx->inter_qs, 10, 4);
        else
            mpp_linreg_init(&ctx->inter_qs, MPP_MIN(rc->gop, 15), 4);
    }
    if (NULL == ctx->qp_p)
        mpp_data_init(&ctx->qp_p, MPP_MIN(rc->gop, 10));
    if (NULL == ctx->sse_p)
        mpp_data_init(&ctx->sse_p, MPP_MIN(rc->gop, 10));
    mpp_assert(ctx->intra_qs);
    mpp_assert(ctx->inter_qs);
    mpp_assert(ctx->qp_p);
    mpp_assert(ctx->sse_p);

    /* frame type and rate control setup */
    {
        RK_S32 prev_frame_type = hw_cfg->frame_type;
        RK_S32 is_cqp = rc->rc_mode == MPP_ENC_RC_MODE_VBR &&
                        rc->quality == MPP_ENC_RC_QUALITY_CQP;

        if (rc_syn->type == INTRA_FRAME && rc_syn->gop_mode != MPP_GOP_ALL_INTRA) {
            hw_cfg->frame_type = H264E_RKV_FRAME_I;
            hw_cfg->coding_type = RKVENC_CODING_TYPE_IDR;
            hw_cfg->frame_num = 0;

            if (!is_cqp) {
                if (ctx->frame_cnt > 0) {
                    hw_cfg->qp = mpp_data_avg(ctx->qp_p, -1, 1, 1);
                    if (hw_cfg->qp >= 42)
                        hw_cfg->qp -= 4;
                    else if (hw_cfg->qp >= 36)
                        hw_cfg->qp -= 3;
                    else if (hw_cfg->qp >= 30)
                        hw_cfg->qp -= 2;
                    else if (hw_cfg->qp >= 24)
                        hw_cfg->qp -= 1;
                }

                /*
                 * Previous frame is inter then intra frame can not
                 * have a big qp step between these two frames
                 */
                if (prev_frame_type == H264E_RKV_FRAME_P)
                    hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 2,
                                          hw_cfg->qp_prev + 2);
            }
        } else {
            if (rc_syn->gop_mode == MPP_GOP_ALL_INTRA) {
                hw_cfg->frame_type = H264E_RKV_FRAME_I;
                hw_cfg->coding_type = RKVENC_CODING_TYPE_IDR;
            } else {
                hw_cfg->frame_type = H264E_RKV_FRAME_P;
                hw_cfg->coding_type = RKVENC_CODING_TYPE_P;
            }

            if (!is_cqp) {
                RK_S32 sse = mpp_data_avg(ctx->sse_p, 1, 1, 1) + 1;
                hw_cfg->qp = h264e_rkv_find_best_qp(ctx->inter_qs, codec,
                                                    hw_cfg->qp_prev,
                                                    (RK_S64)rc_syn->bit_target * 1024 / sse);
                hw_cfg->qp_min = h264e_rkv_find_best_qp(ctx->inter_qs, codec,
                                                        hw_cfg->qp_prev,
                                                        (RK_S64)rc_syn->bit_max * 1024 / sse);
                hw_cfg->qp_max = h264e_rkv_find_best_qp(ctx->inter_qs, codec,
                                                        hw_cfg->qp_prev,
                                                        (RK_S64)rc_syn->bit_min * 1024 / sse);

                /*
                 * Previous frame is intra then inter frame can not
                 * have a big qp step between these two frames
                 */
                if (prev_frame_type == H264E_RKV_FRAME_I)
                    hw_cfg->qp = mpp_clip(hw_cfg->qp, hw_cfg->qp_prev - 4,
                                          hw_cfg->qp_prev + 4);
            }
        }
    }

    /* limit QP by qp_step */
    if (ctx->frame_cnt > 1) {

        hw_cfg->qp_min = codec->qp_min;
        hw_cfg->qp_max = codec->qp_max;

        hw_cfg->qp = mpp_clip(hw_cfg->qp,
                              hw_cfg->qp_prev - codec->qp_max_step,
                              hw_cfg->qp_prev + codec->qp_max_step);
    } else {
        hw_cfg->qp_min = codec->qp_min;
        hw_cfg->qp_max = codec->qp_max;
    }

    hw_cfg->mad_qp_delta = 0;
    hw_cfg->mad_threshold = 6;
    hw_cfg->keyframe_max_interval = rc->gop ? rc->gop : 1;

    /* limit QP by codec global config */
    hw_cfg->qp_min = MPP_MAX(codec->qp_min, hw_cfg->qp_min);
    hw_cfg->qp_max = MPP_MIN(codec->qp_max, hw_cfg->qp_max);

    /* disable mb rate control first */
    hw_cfg->cp_distance_mbs = 0;
    for (i = 0; i < 10; i++)
        hw_cfg->cp_target[i] = 0;

    for (i = 0; i < 9; i++)
        hw_cfg->target_error[i] = 0;

    for (i = 0; i < 9; i++)
        hw_cfg->delta_qp[i] = 0;

    /* slice mode setup */
    hw_cfg->slice_size_mb_rows = (prep->width + 15) >> 4;

    /* input and preprocess config */
    hw_cfg->input_luma_addr = mpp_buffer_get_fd(task->input);
    hw_cfg->input_cb_addr = hw_cfg->input_luma_addr;
    hw_cfg->input_cr_addr = hw_cfg->input_cb_addr;
    hw_cfg->output_strm_addr = mpp_buffer_get_fd(task->output);
    hw_cfg->output_strm_limit_size = mpp_buffer_get_size(task->output);

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_gen_regs(void *hal, HalTaskInfo *task)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvRegSet *regs = NULL;
    H264eRkvIoctlRegInfo *ioctl_reg_info = NULL;
    H264eHwCfg *syn = &ctx->hw_cfg;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    H264eRkvDpbCtx *dpb_ctx = (H264eRkvDpbCtx *)ctx->dpb_ctx;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    H264eSps *sps = &extra_info->sps;
    H264ePps *pps = &extra_info->pps;
    HalEncTask *enc_task = &task->enc;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RcSyntax *rc_syn = (RcSyntax *)enc_task->syntax.data;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncPrepCfg *prep = &cfg->prep;

    RK_S32 pic_width_align16 = 0;
    RK_S32 pic_height_align16 = 0;
    RK_S32 pic_width_in_blk64 = 0;
    h264e_hal_rkv_buffers *bufs = (h264e_hal_rkv_buffers *)ctx->buffers;
    RK_U32 buf2_idx = ctx->frame_cnt % 2;
    MppBuffer mv_info_buf = task->enc.mv_info;

    ctx->enc_mode = RKV_H264E_ENC_MODE;

    h264e_hal_enter();

    enc_task->flags.err = 0;

    h264e_rkv_update_hw_cfg(ctx, &task->enc, syn);

    pic_width_align16 = (syn->width + 15) & (~15);
    pic_height_align16 = (syn->height + 15) & (~15);
    pic_width_in_blk64 = (syn->width + 63) / 64;

    h264e_rkv_adjust_param(ctx); //TODO: future expansion

    h264e_hal_dbg(H264E_DBG_SIMPLE,
                  "frame %d | type %d | start gen regs",
                  ctx->frame_cnt, syn->frame_type);

    if ((!ctx->alloc_flg) && ((prep->change & MPP_ENC_PREP_CFG_CHANGE_INPUT) ||
                              (prep->change & MPP_ENC_PREP_CFG_CHANGE_FORMAT))) {
        if (MPP_OK != h264e_rkv_allocate_buffers(ctx, syn)) {
            h264e_hal_err("h264e_rkv_allocate_buffers failed, free buffers and return\n");
            enc_task->flags.err |= HAL_ENC_TASK_ERR_ALLOC;
            h264e_rkv_free_buffers(ctx);
            return MPP_ERR_MALLOC;
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
        ioctl_info->reg_info[idx].reg_num = sizeof(H264eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[idx];
    } else {
        ctx->num_frames_to_send = 1;
        ioctl_info->frame_num = ctx->num_frames_to_send;
        ioctl_info->enc_mode = ctx->enc_mode;
        regs = &reg_list[0];
        ioctl_info->reg_info[0].reg_num = sizeof(H264eRkvRegSet) / 4;
        ioctl_reg_info = &ioctl_info->reg_info[0];
    }

    if (MPP_OK != h264e_rkv_reference_frame_set(ctx, syn)) {
        h264e_hal_err("h264e_rkv_reference_frame_set failed, multi-ref error");
    }

    memset(regs, 0, sizeof(H264eRkvRegSet));

    regs->swreg01.rkvenc_ver      = 0x1;

    regs->swreg02.lkt_num = 0;
    regs->swreg02.rkvenc_cmd = ctx->enc_mode;
    //regs->swreg02.rkvenc_cmd = syn->link_table_en ? 0x2 : 0x1;
    regs->swreg02.enc_cke = 0;

    regs->swreg03.safe_clr = 0x0;

    regs->swreg04.lkt_addr = 0x10000000;

    regs->swreg05.ofe_fnsh    = 1;
    regs->swreg05.lkt_fnsh    = 1;
    regs->swreg05.clr_fnsh    = 1;
    regs->swreg05.ose_fnsh    = 1;
    regs->swreg05.bs_ovflr    = 1;
    regs->swreg05.brsp_ful    = 1;
    regs->swreg05.brsp_err    = 1;
    regs->swreg05.rrsp_err    = 1;
    regs->swreg05.tmt_err     = 1;

    regs->swreg09.pic_wd8_m1  = pic_width_align16 / 8 - 1;
    regs->swreg09.pic_wfill   = (syn->width & 0xf)
                                ? (16 - (syn->width & 0xf)) : 0;
    regs->swreg09.pic_hd8_m1  = pic_height_align16 / 8 - 1;
    regs->swreg09.pic_hfill   = (syn->height & 0xf)
                                ? (16 - (syn->height & 0xf)) : 0;

    regs->swreg10.enc_stnd       = 0; //H264
    regs->swreg10.cur_frm_ref    = 1; //current frame will be refered
    regs->swreg10.bs_scp         = 1;
    regs->swreg10.slice_int      = 0;
    regs->swreg10.node_int       = 0;

    regs->swreg11.ppln_enc_lmt     = 0xf;
    regs->swreg11.rfp_load_thrd    = 0;

    regs->swreg12.src_bus_ordr     = 0x0;
    regs->swreg12.cmvw_bus_ordr    = 0x0;
    regs->swreg12.dspw_bus_ordr    = 0x0;
    regs->swreg12.rfpw_bus_ordr    = 0x0;
    regs->swreg12.src_bus_edin     = 0x0;
    regs->swreg12.meiw_bus_edin    = 0x0;
    regs->swreg12.bsw_bus_edin     = 0x7;
    regs->swreg12.lktr_bus_edin    = 0x0;
    regs->swreg12.ctur_bus_edin    = 0x0;
    regs->swreg12.lktw_bus_edin    = 0x0;
    regs->swreg12.rfp_map_dcol     = 0x0;
    regs->swreg12.rfp_map_sctr     = 0x0;

    regs->swreg13.axi_brsp_cke      = 0x7f;
    regs->swreg13.cime_dspw_orsd    = 0x0;

    h264e_rkv_set_pp_regs(regs, syn, &ctx->cfg->prep,
                          bufs->hw_pp_buf[buf2_idx], bufs->hw_pp_buf[1 - buf2_idx]);
    h264e_rkv_set_ioctl_extra_info(&ioctl_reg_info->extra_info, syn, regs);

    regs->swreg24_adr_srcy     = syn->input_luma_addr;
    regs->swreg25_adr_srcu     = syn->input_cb_addr;
    regs->swreg26_adr_srcv     = syn->input_cr_addr;

    regs->swreg30_rfpw_addr    = mpp_buffer_get_fd(dpb_ctx->fdec->hw_buf);
    if (dpb_ctx->fref[0][0])
        regs->swreg31_rfpr_addr = mpp_buffer_get_fd(dpb_ctx->fref[0][0]->hw_buf);

    if (bufs->hw_dsp_buf[buf2_idx])
        regs->swreg34_dspw_addr = mpp_buffer_get_fd(bufs->hw_dsp_buf[buf2_idx]);
    if (bufs->hw_dsp_buf[1 - buf2_idx])
        regs->swreg35_dspr_addr =
            mpp_buffer_get_fd(bufs->hw_dsp_buf[1 - buf2_idx]);

    if (mv_info_buf) {
        regs->swreg10.mei_stor    = 1;
        regs->swreg36_meiw_addr   = mpp_buffer_get_fd(mv_info_buf);
    } else {
        regs->swreg10.mei_stor    = 0;
        regs->swreg36_meiw_addr   = 0;
    }

    regs->swreg38_bsbb_addr    = syn->output_strm_addr;
    /* TODO: stream size relative with syntax */
    regs->swreg37_bsbt_addr = regs->swreg38_bsbb_addr | (syn->output_strm_limit_size << 10);
    regs->swreg39_bsbr_addr    = regs->swreg38_bsbb_addr;
    regs->swreg40_bsbw_addr    = regs->swreg38_bsbb_addr;

    regs->swreg41.sli_cut         = 0;
    regs->swreg41.sli_cut_mode    = 0;
    regs->swreg41.sli_cut_bmod    = 1;
    regs->swreg41.sli_max_num     = 0;
    regs->swreg41.sli_out_mode    = 0;
    regs->swreg41.sli_cut_cnum    = 0;

    regs->swreg42.sli_cut_byte    = 0;

    {
        RK_U32 cime_wid_4p = 0, cime_hei_4p = 0;
        if (sps->i_level_idc == 10 || sps->i_level_idc == 9) { //9 is level 1b;
            cime_wid_4p = 44;
            cime_hei_4p = 12;
        } else if ( sps->i_level_idc == 11 || sps->i_level_idc == 12
                    || sps->i_level_idc == 13 || sps->i_level_idc == 20 ) {
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

        if (cime_wid_4p / 4 > (RK_U32)(((regs->swreg09.pic_wd8_m1 + 1)
                                        * 8 + 63) / 64 * 64 / 128 * 4))
            cime_wid_4p = ((regs->swreg09.pic_wd8_m1 + 1)
                           * 8 + 63) / 64 * 64 / 128 * 4 * 4;

        regs->swreg43.cime_srch_h    = cime_wid_4p / 4;
        regs->swreg43.cime_srch_v    = cime_hei_4p / 4;
    }

    regs->swreg43.rime_srch_h    = 7;
    regs->swreg43.rime_srch_v    = 5;
    regs->swreg43.dlt_frm_num    = 0x0;

    regs->swreg44.pmv_mdst_h    = 5;
    regs->swreg44.pmv_mdst_v    = 5;
    regs->swreg44.mv_limit      = (sps->i_level_idc > 20)
                                  ? 2 : ((sps->i_level_idc >= 11) ? 1 : 0);
    regs->swreg44.mv_num        = 3;

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
        RK_U32 i_swin_all_16_w = ((regs->swreg43.cime_srch_h * 4 + 15)
                                  / 16 * 2 + 1);
        if (i_swin_all_4_h < regs->swreg45.cime_rama_h)
            regs->swreg45.cime_rama_max = (i_swin_all_4_h - 1)
                                          * pic_width_in_blk64 + i_swin_all_16_w;
        else
            regs->swreg45.cime_rama_max = (regs->swreg45.cime_rama_h - 1)
                                          * pic_width_in_blk64 + i_swin_all_16_w;
    }

    regs->swreg45.cach_l1_dtmr     = 0x3;
    regs->swreg45.cach_l2_tag      = 0x0;
    if (pic_width_align16 <= 512)
        regs->swreg45.cach_l2_tag  = 0x0;
    else if (pic_width_align16 <= 1024)
        regs->swreg45.cach_l2_tag  = 0x1;
    else if (pic_width_align16 <= 2048)
        regs->swreg45.cach_l2_tag  = 0x2;
    else if (pic_width_align16 <= 4096)
        regs->swreg45.cach_l2_tag  = 0x3;

    h264e_rkv_set_rc_regs(ctx, regs, syn, rc_syn);

    if (ctx->sei_mode != MPP_ENC_SEI_MODE_DISABLE) {
        extra_info->nal_num = 0;
        h264e_rkv_stream_reset(&extra_info->stream);
        h264e_rkv_nal_start(extra_info, H264E_NAL_SEI, H264E_NAL_PRIORITY_DISPOSABLE);
        h264e_rkv_sei_encode(ctx, rc_syn);
        h264e_rkv_nal_end(extra_info);
#ifdef SEI_ADD_NAL_HEADER
        h264e_rkv_encapsulate_nals(extra_info);
#endif
    }

    regs->swreg56.rect_size = (sps->i_profile_idc == H264_PROFILE_BASELINE
                               && sps->i_level_idc <= 30);
    regs->swreg56.inter_4x4 = 1;
    regs->swreg56.arb_sel   = 0;
    regs->swreg56.vlc_lmt   = (sps->i_profile_idc < H264_PROFILE_HIGH
                               && !pps->b_cabac);
    regs->swreg56.rdo_mark  = 0;
    {
        RK_U32 i_nal_type = 0, i_nal_ref_idc = 0;

        /* TODO: extend syn->frame_coding_type definition */
        if (syn->coding_type == RKVENC_CODING_TYPE_IDR ) {
            /* reset ref pictures */
            i_nal_type    = H264E_NAL_SLICE_IDR;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGHEST;
        } else if (syn->coding_type == RKVENC_CODING_TYPE_I ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if (syn->coding_type == RKVENC_CODING_TYPE_P ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH; /* Not completely true but for now it is (as all I/P are kept as ref)*/
        } else if (syn->coding_type == RKVENC_CODING_TYPE_BREF ) {
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_HIGH;
        } else { /* B frame */
            i_nal_type    = H264E_NAL_SLICE;
            i_nal_ref_idc = H264E_NAL_PRIORITY_DISPOSABLE;
        }
        if (sps->keyframe_max_interval == 1)
            i_nal_ref_idc = H264E_NAL_PRIORITY_LOW;

        regs->swreg57.nal_ref_idc      = i_nal_ref_idc;
        regs->swreg57.nal_unit_type    = i_nal_type;
    }

    regs->swreg58.max_fnum    = sps->i_log2_max_frame_num - 4;
    regs->swreg58.drct_8x8    = 1;
    regs->swreg58.mpoc_lm4    = sps->i_log2_max_poc_lsb - 4;

    regs->swreg59.etpy_mode       = pps->b_cabac;
    regs->swreg59.trns_8x8        = pps->b_transform_8x8_mode;
    regs->swreg59.csip_flg        = pps->b_constrained_intra_pred;
    regs->swreg59.num_ref0_idx    = pps->i_num_ref_idx_l0_default_active - 1;
    regs->swreg59.num_ref1_idx    = pps->i_num_ref_idx_l1_default_active - 1;
    regs->swreg59.pic_init_qp     = pps->i_pic_init_qp - H264_QP_BD_OFFSET;
    regs->swreg59.cb_ofst         = pps->i_chroma_qp_index_offset;
    regs->swreg59.cr_ofst         = pps->i_second_chroma_qp_index_offset;
    regs->swreg59.wght_pred       = 0x0;
    regs->swreg59.dbf_cp_flg      = 1;

    regs->swreg60.sli_type        = syn->frame_type;
    regs->swreg60.pps_id          = pps->i_id;
    regs->swreg60.drct_smvp       = 0x0;
    regs->swreg60.num_ref_ovrd    = 0;
    regs->swreg60.cbc_init_idc    = syn->cabac_init_idc;

    regs->swreg60.frm_num         = dpb_ctx->i_frame_num;

    regs->swreg61.idr_pid    = dpb_ctx->i_idr_pic_id;
    regs->swreg61.poc_lsb    = dpb_ctx->fdec->i_poc
                               & ((1 << sps->i_log2_max_poc_lsb) - 1);

    regs->swreg62.rodr_pic_idx      = dpb_ctx->ref_pic_list_order[0][0].idc;
    regs->swreg62.ref_list0_rodr    = dpb_ctx->b_ref_pic_list_reordering[0];
    regs->swreg62.dis_dblk_idc      = 0;
    regs->swreg62.rodr_pic_num      = dpb_ctx->ref_pic_list_order[0][0].arg;

    {
        RK_S32 mmco4_pre = dpb_ctx->i_mmco_command_count > 0
                           && (dpb_ctx->mmco[0].memory_management_control_operation == 4);
        regs->swreg63.nopp_flg     = 0x0;
        regs->swreg63.ltrf_flg     = dpb_ctx->i_long_term_reference_flag;
        regs->swreg63.arpm_flg     = dpb_ctx->i_mmco_command_count;
        regs->swreg63.mmco4_pre    = mmco4_pre;
        regs->swreg63.mmco_0       = dpb_ctx->i_mmco_command_count > mmco4_pre
                                     ? dpb_ctx->mmco[mmco4_pre].memory_management_control_operation : 0;
        regs->swreg63.dopn_m1_0    = dpb_ctx->i_mmco_command_count > mmco4_pre
                                     ? dpb_ctx->mmco[mmco4_pre].i_difference_of_pic_nums - 1 : 0;

        regs->swreg64.mmco_1       = dpb_ctx->i_mmco_command_count
                                     > (mmco4_pre + 1)
                                     ? dpb_ctx->mmco[(mmco4_pre + 1)].memory_management_control_operation : 0;
        regs->swreg64.dopn_m1_1    = dpb_ctx->i_mmco_command_count
                                     > (mmco4_pre + 1)
                                     ? dpb_ctx->mmco[(mmco4_pre + 1)].i_difference_of_pic_nums - 1 : 0;
    }

    h264e_rkv_set_osd_regs(ctx, regs);

    /* ROI configure */
    h264e_rkv_set_roi_regs(ctx, regs);

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

    regs->swreg76.bsbw_addr    = 0x0;
    regs->swreg76.Bsbw_ovfl    = 0x0;

    h264e_rkv_reference_frame_update(ctx);
    dpb_ctx->i_frame_cnt++;
    if (dpb_ctx->i_nal_ref_idc != H264E_NAL_PRIORITY_DISPOSABLE)
        dpb_ctx->i_frame_num ++;

    ctx->frame_cnt_gen_ready++;
    ctx->frame_cnt++;
    extra_info->sei.frame_cnt++;
    hw_cfg->frame_num++;

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_OK;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    RK_U32 length = 0, k = 0;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    HalEncTask *enc_task = &task->enc;

    h264e_hal_enter();
    if (enc_task->flags.err) {
        h264e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h264e_hal_dbg(H264E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), start hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    }

    h264e_hal_dbg(H264E_DBG_DETAIL,
                  "memcpy %d frames' regs from reg list to reg info",
                  ioctl_info->frame_num);
    for (k = 0; k < ioctl_info->frame_num; k++) {
        RK_U32 i;
        RK_U32 *regs = (RK_U32*)&reg_list[k];
        memcpy(&ioctl_info->reg_info[k].regs, &reg_list[k],
               sizeof(H264eRkvRegSet));
        for (i = 0; i < sizeof(H264eRkvRegSet) / 4; i++) {
            h264e_hal_dbg(H264E_DBG_REG, "set reg[%03d]: %08x\n", i, regs[i]);
        }
    }

    length = (sizeof(ioctl_info->enc_mode) + sizeof(ioctl_info->frame_num) +
              sizeof(ioctl_info->reg_info[0]) * ioctl_info->frame_num) >> 2;

    ctx->frame_cnt_send_ready ++;

    h264e_hal_dbg(H264E_DBG_DETAIL, "vpu client is sending %d regs", length);
    if (mpp_device_send_reg(ctx->dev_ctx, (RK_U32 *)ioctl_info, length)) {
        h264e_hal_err("mpp_device_send_reg Failed!!!");
        return  MPP_ERR_VPUHW;
    } else {
        h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_send_reg successfully!");
    }

    h264e_hal_leave();

    return ret;
}

static MPP_RET h264e_rkv_set_feedback(H264eHalContext *ctx,
                                      H264eRkvIoctlOutput *out, HalEncTask *enc_task)
{
    RK_U32 k = 0;
    H264eRkvIoctlOutputElem *elem = NULL;
    H264eRkvExtraInfo *extra_info = (H264eRkvExtraInfo *)ctx->extra_info;
    h264e_feedback *fb = &ctx->feedback;

    h264e_hal_enter();
    for (k = 0; k < out->frame_num; k++) {
        elem = &out->elem[k];
        fb->qp_sum = elem->swreg71.qp_sum;
        fb->out_hw_strm_size =
            fb->out_strm_size = elem->swreg69.bs_lgth;
        fb->sse_sum = elem->swreg70.sse_l32 +
                      ((RK_S64)(elem->swreg71.sse_h8 & 0xff) << 32);

        fb->hw_status = elem->hw_status;
        h264e_hal_dbg(H264E_DBG_DETAIL, "hw_status: 0x%08x", elem->hw_status);
        if (elem->hw_status & RKV_H264E_INT_LINKTABLE_FINISH)
            h264e_hal_err("RKV_H264E_INT_LINKTABLE_FINISH");

        if (elem->hw_status & RKV_H264E_INT_ONE_FRAME_FINISH)
            h264e_hal_dbg(H264E_DBG_DETAIL, "RKV_H264E_INT_ONE_FRAME_FINISH");

        if (elem->hw_status & RKV_H264E_INT_ONE_SLICE_FINISH)
            h264e_hal_err("RKV_H264E_INT_ONE_SLICE_FINISH");

        if (elem->hw_status & RKV_H264E_INT_SAFE_CLEAR_FINISH)
            h264e_hal_err("RKV_H264E_INT_SAFE_CLEAR_FINISH");

        if (elem->hw_status & RKV_H264E_INT_BIT_STREAM_OVERFLOW)
            h264e_hal_err("RKV_H264E_INT_BIT_STREAM_OVERFLOW");

        if (elem->hw_status & RKV_H264E_INT_BUS_WRITE_FULL)
            h264e_hal_err("RKV_H264E_INT_BUS_WRITE_FULL");

        if (elem->hw_status & RKV_H264E_INT_BUS_WRITE_ERROR)
            h264e_hal_err("RKV_H264E_INT_BUS_WRITE_ERROR");

        if (elem->hw_status & RKV_H264E_INT_BUS_READ_ERROR)
            h264e_hal_err("RKV_H264E_INT_BUS_READ_ERROR");

        if (elem->hw_status & RKV_H264E_INT_TIMEOUT_ERROR)
            h264e_hal_err("RKV_H264E_INT_TIMEOUT_ERROR");
    }

    if (ctx->sei_mode != MPP_ENC_SEI_MODE_DISABLE) {
        H264eRkvNal *nal = &extra_info->nal[0];
        mpp_buffer_write(enc_task->output, fb->out_strm_size,
                         nal->p_payload, nal->i_payload);
        fb->out_strm_size += nal->i_payload;
    }

    h264e_hal_leave();

    return MPP_OK;
}

/* this function must be called after first mpp_device_wait_reg,
 * since it depend on h264e_feedback structure, and this structure will
 * be filled after mpp_device_wait_reg called
 */
static MPP_RET h264e_rkv_resend(H264eHalContext *ctx, RK_S32 mb_rc)
{
    unsigned int k = 0;
    H264eRkvIoctlInput *ioctl_info = (H264eRkvIoctlInput *)ctx->ioctl_input;
    H264eRkvRegSet *reg_list = (H264eRkvRegSet *)ctx->regs;
    RK_S32 length;
    MppEncPrepCfg *prep = &ctx->cfg->prep;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16)
                    * MPP_ALIGN(prep->height, 16) / 16 / 16;
    H264eRkvIoctlOutput *reg_out = (H264eRkvIoctlOutput *)ctx->ioctl_output;
    h264e_feedback *fb = &ctx->feedback;
    RK_S32 hw_ret = 0;

    reg_list->swreg10.pic_qp        = fb->qp_sum / num_mb;
    reg_list->swreg46.rc_en         = mb_rc; //0: disable mb rc
    reg_list->swreg46.rc_mode       = mb_rc ? 1 : 0; //0:frame/slice rc; 1:mbrc
    reg_list->swreg46.aqmode_en     = mb_rc;

    for (k = 0; k < ioctl_info->frame_num; k++)
        memcpy(&ioctl_info->reg_info[k].regs,
               &reg_list[k], sizeof(H264eRkvRegSet));

    length = (sizeof(ioctl_info->enc_mode) +
              sizeof(ioctl_info->frame_num) +
              sizeof(ioctl_info->reg_info[0]) *
              ioctl_info->frame_num) >> 2;

    hw_ret = mpp_device_send_reg(ctx->dev_ctx, (RK_U32 *)ioctl_info, length);
    if (hw_ret) {
        h264e_hal_err("mpp_device_send_reg Failed!!!");
        return MPP_ERR_VPUHW;
    } else {
        h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_send_reg successfully!");
    }

    hw_ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)reg_out, length);
    if (hw_ret) {
        h264e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_wait(void *hal, HalTaskInfo *task)
{
    RK_S32 hw_ret = 0;
    H264eHalContext *ctx = (H264eHalContext *)hal;
    H264eRkvIoctlOutput *reg_out = (H264eRkvIoctlOutput *)ctx->ioctl_output;
    RK_S32 length = (sizeof(reg_out->frame_num)
                     + sizeof(reg_out->elem[0])
                     * ctx->num_frames_to_send) >> 2;
    IOInterruptCB int_cb = ctx->int_cb;
    h264e_feedback *fb = &ctx->feedback;
    HalEncTask *enc_task = &task->enc;
    MppEncCfgSet *cfg = ctx->cfg;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncH264Cfg *codec = &cfg->codec.h264;
    H264eHwCfg *hw_cfg = &ctx->hw_cfg;
    RK_S32 num_mb = MPP_ALIGN(prep->width, 16)
                    * MPP_ALIGN(prep->height, 16) / 16 / 16;
    RcSyntax *rc_syn = (RcSyntax *)task->enc.syntax.data;
    struct list_head *rc_head = rc_syn->rc_head;
    RK_U32 frame_cnt = ctx->frame_cnt;

    h264e_hal_enter();

    if (enc_task->flags.err) {
        h264e_hal_err("enc_task->flags.err %08x, return early",
                      enc_task->flags.err);
        return MPP_NOK;
    }

    if (ctx->frame_cnt_gen_ready != ctx->num_frames_to_send) {
        h264e_hal_dbg(H264E_DBG_DETAIL,
                      "frame_cnt_gen_ready(%d) != num_frames_to_send(%d), wait hardware later",
                      ctx->frame_cnt_gen_ready, ctx->num_frames_to_send);
        return MPP_OK;
    } else {
        ctx->frame_cnt_gen_ready = 0;
        if (ctx->enc_mode == 3) {
            /* TODO: remove later */
            h264e_hal_dbg(H264E_DBG_DETAIL, "only for test enc_mode 3 ...");
            if (ctx->frame_cnt_send_ready != RKV_H264E_LINKTABLE_FRAME_NUM) {
                h264e_hal_dbg(H264E_DBG_DETAIL,
                              "frame_cnt_send_ready(%d) != RKV_H264E_LINKTABLE_FRAME_NUM(%d), wait hardware later",
                              ctx->frame_cnt_send_ready, RKV_H264E_LINKTABLE_FRAME_NUM);
                return MPP_OK;
            } else {
                ctx->frame_cnt_send_ready = 0;
            }
        }
    }

    h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_wait_reg expect length %d\n",
                  length);

    hw_ret = mpp_device_wait_reg(ctx->dev_ctx, (RK_U32 *)reg_out, length);

    h264e_hal_dbg(H264E_DBG_DETAIL, "mpp_device_wait_reg: ret %d\n", hw_ret);

    if (hw_ret != MPP_OK) {
        h264e_hal_err("hardware returns error:%d", hw_ret);
        return MPP_ERR_VPUHW;
    }

    h264e_rkv_set_feedback(ctx, reg_out, enc_task);

    /* we need re-encode */
    if ((frame_cnt == 1) || (frame_cnt == 2)) {
        if (fb->hw_status & RKV_H264E_INT_BIT_STREAM_OVERFLOW) {
            RK_S32 new_qp = fb->qp_sum / num_mb + 3;
            h264e_hal_dbg(H264E_DBG_DETAIL,
                          "re-encode for first frame overflow ...\n");
            fb->qp_sum = new_qp * num_mb;
            h264e_rkv_resend(ctx, 1);
        } else {
            /* The first and second frame, that is the first I and P frame,
             * is re-encoded for getting appropriate QP for target bits.
             */
            h264e_rkv_resend(ctx, 1);
        }
        h264e_rkv_set_feedback(ctx, reg_out, enc_task);
    } else if ((RK_S32)frame_cnt < rc->fps_out_num / rc->fps_out_denorm &&
               rc_syn->type == INTER_P_FRAME &&
               rc_syn->bit_target > fb->out_hw_strm_size * 8 * 1.5) {
        /* re-encode frame if it meets all the conditions below:
         * 1. gop is the first gop
         * 2. type is p frame
         * 3. target_bits is larger than 1.5 * real_bits
         * and the qp_init will decrease 3 when re-encode.
         *
         * TODO: maybe use sse to calculate a proper value instead of 3
         * or add a much more suitable condition to start this re-encode
         * process.
         */
        RK_S32 new_qp = fb->qp_sum / num_mb - 3;
        fb->qp_sum = new_qp * num_mb;

        h264e_rkv_resend(ctx, 1);
        h264e_rkv_set_feedback(ctx, reg_out, enc_task);
    } else if (fb->hw_status & RKV_H264E_INT_BIT_STREAM_OVERFLOW) {
        RK_S32 new_qp = fb->qp_sum / num_mb + 3;
        h264e_hal_dbg(H264E_DBG_DETAIL,
                      "re-encode for overflow ...\n");
        fb->qp_sum = new_qp * num_mb;
        h264e_rkv_resend(ctx, 1);
        h264e_rkv_set_feedback(ctx, reg_out, enc_task);
    }

    task->enc.length = fb->out_strm_size;
    h264e_hal_dbg(H264E_DBG_DETAIL, "output stream size %d\n",
                  fb->out_strm_size);
    if (int_cb.callBack) {
        RcSyntax *syn = (RcSyntax *)task->enc.syntax.data;
        RcHalResult result;
        double avg_qp = 0.0;
        RK_S32 avg_sse = 1;
        RK_S32 wlen = 15;
        RK_S32 prev_sse = 1;

        avg_qp = fb->qp_sum * 1.0 / num_mb;
        RK_S32 real_qp = (RK_S32)avg_qp;
        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_REAL_QP, &real_qp);

        if (syn->type == INTER_P_FRAME || syn->gop_mode == MPP_GOP_ALL_INTRA) {
            avg_sse = (RK_S32)sqrt((double)(fb->sse_sum));
            prev_sse = mpp_data_avg(ctx->sse_p, 1, 1, 1);

            if (avg_sse < 1)
                avg_sse = 1;
            if (prev_sse < 1)
                prev_sse = 1;

            if (avg_sse > prev_sse)
                wlen = wlen * prev_sse / avg_sse;
            else
                wlen = wlen * avg_sse / prev_sse;

            mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_WIN_LEN, &wlen);
        }

        mpp_assert(avg_qp >= 0);
        mpp_assert(avg_qp <= 51);

        result.bits = fb->out_hw_strm_size * 8;
        result.type = syn->type;
        fb->result = &result;

        h264e_hal_dbg(H264E_DBG_RC, "target bits %d real bits %d "
                      "target qp %d real qp %0.2f\n",
                      rc_syn->bit_target, result.bits,
                      hw_cfg->qp, avg_qp);

        if (syn->type == INTER_P_FRAME || syn->gop_mode == MPP_GOP_ALL_INTRA) {
            mpp_save_regdata(ctx->inter_qs, QP2Qstep(avg_qp),
                             result.bits * 1024 / avg_sse);
            mpp_quadreg_update(ctx->inter_qs, wlen);
        }
        if (rc->quality == MPP_ENC_RC_QUALITY_AQ_ONLY) {
            hw_cfg->qp_prev = hw_cfg->qp;
        } else {
            hw_cfg->qp_prev = avg_qp;
        }

        if (syn->type == INTER_P_FRAME || syn->gop_mode == MPP_GOP_ALL_INTRA) {
            mpp_data_update(ctx->qp_p, avg_qp);
            mpp_data_update(ctx->sse_p, avg_sse);
        }

        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_REAL_BITS, &result.bits);
        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_QP_SUM, &fb->qp_sum);
        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_SSE_SUM, &fb->sse_sum);

        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_QP_MIN, &hw_cfg->qp_min);
        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_QP_MAX, &hw_cfg->qp_max);
        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_SET_QP, &hw_cfg->qp);

        mpp_rc_param_ops(rc_head, frame_cnt, RC_RECORD_LIN_REG, ctx->inter_qs);

        int_cb.callBack(int_cb.opaque, fb);
    }

    codec->change = 0;
    prep->change = 0;
    rc->change = 0;
    ctx->osd_data.buf = NULL;
    ctx->osd_data.num_region = 0;

    h264e_hal_leave();

    return MPP_OK;
}

MPP_RET hal_h264e_rkv_reset(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_flush(void *hal)
{
    (void)hal;
    h264e_hal_enter();

    h264e_hal_leave();
    return MPP_OK;
}

MPP_RET hal_h264e_rkv_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264eHalContext *ctx = (H264eHalContext *)hal;
    h264e_hal_enter();

    h264e_hal_dbg(H264E_DBG_DETAIL, "h264e_rkv_control cmd 0x%x, info %p",
                  cmd_type, param);
    switch (cmd_type) {
    case MPP_ENC_SET_EXTRA_INFO: {
        break;
    }
    case MPP_ENC_GET_EXTRA_INFO: {
        MppPacket *pkt_out = (MppPacket *)param;
        h264e_rkv_set_extra_info(ctx);
        h264e_rkv_get_extra_info(ctx, pkt_out);
        break;
    }
    case MPP_ENC_SET_OSD_PLT_CFG: {
        h264e_rkv_set_osd_plt(ctx, param);
        break;
    }
    case MPP_ENC_SET_OSD_DATA_CFG: {
        h264e_rkv_set_osd_data(ctx, param);
        break;
    }
    case MPP_ENC_SET_ROI_CFG : {
        h264e_rkv_set_roi_data(ctx, param);
        break;
    }
    case MPP_ENC_SET_SEI_CFG: {
        ctx->sei_mode = *((MppEncSeiMode *)param);
        break;
    }
    case MPP_ENC_GET_SEI_DATA: {
        h264e_rkv_get_extra_info(ctx, (MppPacket *)param);
        break;
    }
    case MPP_ENC_SET_PREP_CFG: {
        //LKSTODO: check cfg
        break;
    }
    case MPP_ENC_SET_RC_CFG: {
        // TODO: do rate control check here
    } break;
    case MPP_ENC_SET_CODEC_CFG: {
        MppEncH264Cfg *src = &ctx->set->codec.h264;
        MppEncH264Cfg *dst = &ctx->cfg->codec.h264;
        RK_U32 change = src->change;

        // TODO: do codec check first

        if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
            dst->stream_type = src->stream_type;
        if (change & MPP_ENC_H264_CFG_CHANGE_PROFILE) {
            dst->profile = src->profile;
            dst->level = src->level;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) {
            dst->entropy_coding_mode = src->entropy_coding_mode;
            dst->cabac_init_idc = src->cabac_init_idc;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8)
            dst->transform8x8_mode = src->transform8x8_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA)
            dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
        if (change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) {
            dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
            dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) {
            dst->deblock_disable = src->deblock_disable;
            dst->deblock_offset_alpha = src->deblock_offset_alpha;
            dst->deblock_offset_beta = src->deblock_offset_beta;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
            dst->use_longterm = src->use_longterm;
        if (change & MPP_ENC_H264_CFG_CHANGE_QP_LIMIT) {
            dst->qp_init = src->qp_init;
            dst->qp_max = src->qp_max;
            dst->qp_min = src->qp_min;
            dst->qp_max_step = src->qp_max_step;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) {
            dst->intra_refresh_mode = src->intra_refresh_mode;
            dst->intra_refresh_arg = src->intra_refresh_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SLICE_MODE) {
            dst->slice_mode = src->slice_mode;
            dst->slice_arg = src->slice_arg;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_VUI) {
            dst->vui = src->vui;
        }
        if (change & MPP_ENC_H264_CFG_CHANGE_SEI) {
            dst->sei = src->sei;
        }

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    } break;
    case MPP_ENC_PRE_ALLOC_BUFF: {
        /* allocate buffers before encoding, so that we can save some time
         * when encoding the first frame.
         */
        H264eHwCfg *hw_cfg = &ctx->hw_cfg;
        ctx->alloc_flg = 1;
        if (MPP_OK != h264e_rkv_allocate_buffers(ctx, hw_cfg)) {
            mpp_err_f("allocate buffers failed\n");
            h264e_rkv_free_buffers(ctx);
            return MPP_ERR_MALLOC;
        }
    } break;
    case MPP_ENC_SET_QP_RANGE: {
        RK_U32 scale_min = 1, scale_max = 2;
        ctx->qp_scale = *((RK_U32 *)param);
        ctx->qp_scale = mpp_clip(ctx->qp_scale, scale_min, scale_max);
    } break;
    default : {
        h264e_hal_err("unrecognizable cmd type %x", cmd_type);
    } break;
    }

    h264e_hal_leave();
    return MPP_OK;
}

