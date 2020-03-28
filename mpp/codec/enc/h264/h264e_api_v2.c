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

#define MODULE_TAG "h264e_api_v2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_packet_impl.h"

#include "h264e_debug.h"
#include "h264e_syntax.h"
#include "h264e_sps.h"
#include "h264e_pps.h"
#include "h264e_sei.h"
#include "h264e_dpb.h"
#include "h264e_slice.h"
#include "h264e_api.h"
#include "rc.h"

#include "enc_impl_api.h"

RK_U32 h264e_debug = 0;

typedef struct {
    /* config from mpp_enc */
    MppDeviceId         dev_id;
    MppEncCfgSet        *cfg;
    RK_U32              idr_request;

    /* H.264 high level syntax */
    SynH264eSps         sps;
    SynH264ePps         pps;

    /*
     * H.264 low level syntax
     *
     * NOTE: two dpb is for dpb roll-back and reencode
     */
    H264eDpb            dpb;
    H264eDpb            dpb_bak;
    H264eSlice          slice;
    H264eReorderInfo    reorder;
    H264eMarkingInfo    marking;
    /* H.264 frame status syntax */
    H264eFrmInfo        frms;
    RcSyntax            rc_syn;
    /* header generation */
    MppPacket           hdr_pkt;
    void                *hdr_buf;
    size_t              hdr_size;
    size_t              hdr_len;

    /* rate control config */
    RcCtx               rc_ctx;

    /* output to hal */
    RK_S32              syn_num;
    H264eSyntaxDesc     syntax[H264E_SYN_BUTT];
} H264eCtx;

static void init_h264e_cfg_set(MppEncCfgSet *cfg)
{
    MppEncRcCfg *rc_cfg = &cfg->rc;
    MppEncPrepCfg *prep = &cfg->prep;
    MppEncH264Cfg *h264 = &cfg->codec.h264;

    /*
     * default codec:
     * High Profile
     * frame mode
     * all flag enabled
     */
    memset(h264, 0, sizeof(*h264));
    h264->profile = H264_PROFILE_BASELINE;
    h264->level = H264_LEVEL_3_1;
    h264->qp_init = 26;
    h264->qp_max = 48;
    h264->qp_min = 16;
    h264->qp_max_step = 8;

    /*
     * default prep:
     * 720p
     * YUV420SP
     */
    prep->change = 0;
    prep->width = 1280;
    prep->height = 720;
    prep->hor_stride = 1280;
    prep->ver_stride = 720;
    prep->format = MPP_FMT_YUV420SP;
    prep->rotation = MPP_ENC_ROT_0;
    prep->mirroring = 0;
    prep->denoise = 0;

    /*
     * default rc_cfg:
     * CBR
     * 2Mbps +-25%
     * 30fps
     * gop 60
     */
    rc_cfg->change = 0;
    rc_cfg->rc_mode = MPP_ENC_RC_MODE_CBR;
    rc_cfg->quality = MPP_ENC_RC_QUALITY_MEDIUM;
    rc_cfg->bps_target = 2000 * 1000;
    rc_cfg->bps_max = rc_cfg->bps_target * 5 / 4;
    rc_cfg->bps_min = rc_cfg->bps_target * 3 / 4;
    rc_cfg->fps_in_flex = 0;
    rc_cfg->fps_in_num = 30;
    rc_cfg->fps_in_denorm = 1;
    rc_cfg->fps_out_flex = 0;
    rc_cfg->fps_out_num = 30;
    rc_cfg->fps_out_denorm = 1;
    rc_cfg->gop = 60;
    rc_cfg->skip_cnt = 0;
}

static void h264e_add_syntax(H264eCtx *ctx, H264eSyntaxType type, void *p)
{
    ctx->syntax[ctx->syn_num].type  = type;
    ctx->syntax[ctx->syn_num].p     = p;
    ctx->syn_num++;
}

static MPP_RET h264e_init(void *ctx, EncImplCfg *ctrl_cfg)
{
    MPP_RET ret = MPP_OK;
    H264eCtx *p = (H264eCtx *)ctx;

    mpp_env_get_u32("h264e_debug", &h264e_debug, 0);

    h264e_dbg_func("enter\n");

    p->dev_id = ctrl_cfg->dev_id;
    p->hdr_size = SZ_1K;
    p->hdr_buf = mpp_malloc_size(void, p->hdr_size);
    mpp_assert(p->hdr_buf);
    mpp_packet_init(&p->hdr_pkt, p->hdr_buf, p->hdr_size);
    mpp_assert(p->hdr_pkt);

    p->cfg = ctrl_cfg->cfg;
    p->idr_request = 0;

    h264e_reorder_init(&p->reorder);
    h264e_marking_init(&p->marking);

    h264e_dpb_init(&p->dpb, &p->reorder, &p->marking);
    h264e_slice_init(&p->slice, &p->reorder, &p->marking);

    init_h264e_cfg_set(p->cfg);

    mpp_env_get_u32("h264e_debug", &h264e_debug, 0);

    h264e_dbg_func("leave\n");
    return ret;
}

static MPP_RET h264e_deinit(void *ctx)
{
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter\n");

    if (p->hdr_pkt)
        mpp_packet_deinit(&p->hdr_pkt);

    MPP_FREE(p->hdr_buf);

    h264e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h264e_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_ALL_CFG : {
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        MppEncPrepCfg *src = (MppEncPrepCfg *)param;
        RK_U32 change = src->change;

        mpp_assert(change);
        if (change) {
            MppEncPrepCfg *dst = &p->cfg->prep;
            MppEncPrepCfg bak = p->cfg->prep;

            if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
                dst->format = src->format;

            if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
                dst->rotation = src->rotation;

            if (change & MPP_ENC_PREP_CFG_CHANGE_MIRRORING)
                dst->mirroring = src->mirroring;

            if (change & MPP_ENC_PREP_CFG_CHANGE_DENOISE)
                dst->denoise = src->denoise;

            if (change & MPP_ENC_PREP_CFG_CHANGE_SHARPEN)
                dst->sharpen = src->sharpen;

            if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
                if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
                    dst->width = src->height;
                    dst->height = src->width;
                } else {
                    dst->width = src->width;
                    dst->height = src->height;
                }
                dst->hor_stride = src->hor_stride;
                dst->ver_stride = src->ver_stride;
            }

            dst->change |= change;

            // parameter checking
            if (dst->width > dst->hor_stride || dst->height > dst->ver_stride) {
                mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                        dst->width, dst->height, dst->hor_stride, dst->ver_stride);
                ret = MPP_ERR_VALUE;
            }

            if (ret) {
                mpp_err_f("failed to accept new prep config\n");
                *dst = bak;
                break;
            }
            mpp_log_f("MPP_ENC_SET_PREP_CFG w:h [%d:%d] stride [%d:%d]\n",
                      dst->width, dst->height, dst->hor_stride, dst->ver_stride);
        }
    } break;
    case MPP_ENC_SET_RC_CFG : {
        MppEncRcCfg *src = (MppEncRcCfg *)param;
        RK_U32 change = src->change;

        if (change) {
            MppEncRcCfg *dst = &p->cfg->rc;
            MppEncRcCfg bak = p->cfg->rc;

            if (change & MPP_ENC_RC_CFG_CHANGE_RC_MODE)
                dst->rc_mode = src->rc_mode;

            if (change & MPP_ENC_RC_CFG_CHANGE_QUALITY)
                dst->quality = src->quality;

            if (change & MPP_ENC_RC_CFG_CHANGE_BPS) {
                dst->bps_target = src->bps_target;
                dst->bps_max = src->bps_max;
                dst->bps_min = src->bps_min;
            }

            if (change & MPP_ENC_RC_CFG_CHANGE_FPS_IN) {
                dst->fps_in_flex = src->fps_in_flex;
                dst->fps_in_num = src->fps_in_num;
                dst->fps_in_denorm = src->fps_in_denorm;
            }

            if (change & MPP_ENC_RC_CFG_CHANGE_FPS_OUT) {
                dst->fps_out_flex = src->fps_out_flex;
                dst->fps_out_num = src->fps_out_num;
                dst->fps_out_denorm = src->fps_out_denorm;
            }

            if (change & MPP_ENC_RC_CFG_CHANGE_GOP)
                dst->gop = src->gop;

            if (change & MPP_ENC_RC_CFG_CHANGE_SKIP_CNT)
                dst->skip_cnt = src->skip_cnt;

            // parameter checking
            if (dst->rc_mode >= MPP_ENC_RC_MODE_BUTT) {
                mpp_err("invalid rc mode %d should be RC_MODE_VBR or RC_MODE_CBR\n",
                        src->rc_mode);
                ret = MPP_ERR_VALUE;
            }
            if (dst->quality >= MPP_ENC_RC_QUALITY_BUTT) {
                mpp_err("invalid quality %d should be from QUALITY_WORST to QUALITY_BEST\n",
                        dst->quality);
                ret = MPP_ERR_VALUE;
            }
            if (dst->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
                if ((dst->bps_target >= 100 * SZ_1M || dst->bps_target <= 1 * SZ_1K) ||
                    (dst->bps_max    >= 100 * SZ_1M || dst->bps_max    <= 1 * SZ_1K) ||
                    (dst->bps_min    >= 100 * SZ_1M || dst->bps_min    <= 1 * SZ_1K)) {
                    mpp_err("invalid bit per second %d [%d:%d] out of range 1K~100M\n",
                            dst->bps_target, dst->bps_min, dst->bps_max);
                    ret = MPP_ERR_VALUE;
                }
            }

            dst->change |= change;

            if (ret) {
                mpp_err_f("failed to accept new rc config\n");
                *dst = bak;
                break;
            }

            mpp_log_f("MPP_ENC_SET_RC_CFG bps %d [%d : %d] fps [%d:%d] gop %d\n",
                      dst->bps_target, dst->bps_min, dst->bps_max,
                      dst->fps_in_num, dst->fps_out_num, dst->gop);
        }
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncH264Cfg *src = &((MppEncCodecCfg *)param)->h264;
        MppEncH264Cfg *dst = &p->cfg->codec.h264;
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

        if (change & MPP_ENC_H264_CFG_CHANGE_SCALING_LIST)
            dst->scaling_list_mode = src->scaling_list_mode;

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

        dst->change |= change;
    } break;
    case MPP_ENC_SET_SEI_CFG : {
    } break;
    case MPP_ENC_SET_IDR_FRAME : {
        p->idr_request++;
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG : {
    } break;
    case MPP_ENC_SET_OSD_DATA_CFG : {
    } break;
    case MPP_ENC_SET_SPLIT : {
        MppEncSliceSplit *src = (MppEncSliceSplit *)param;
        MppEncSliceSplit *dst = &p->cfg->split;
        RK_U32 change = src->change;

        if (change & MPP_ENC_SPLIT_CFG_CHANGE_MODE) {
            dst->split_mode = src->split_mode;
            dst->split_arg = src->split_arg;
        }

        if (change & MPP_ENC_SPLIT_CFG_CHANGE_ARG)
            dst->split_arg = src->split_arg;

        dst->change |= change;
    } break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
        break;
    }

    h264e_dbg_func("leave ret %d\n", ret);

    return ret;
}

static MPP_RET h264e_gen_hdr(void *ctx, MppPacket pkt)
{
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter\n");

    h264e_sps_update(&p->sps, p->cfg, p->dev_id);
    h264e_pps_update(&p->pps, p->cfg);

    /*
     * NOTE: When sps/pps is update we need to update dpb and slice info
     */
    h264e_dpb_set_cfg(&p->dpb, p->cfg, &p->sps);

    mpp_packet_reset(p->hdr_pkt);

    h264e_sps_to_packet(&p->sps, p->hdr_pkt);
    h264e_pps_to_packet(&p->pps, p->hdr_pkt);

    p->hdr_len = mpp_packet_get_length(p->hdr_pkt);

    if (pkt) {
        mpp_packet_write(pkt, 0, p->hdr_buf, p->hdr_len);
        mpp_packet_set_length(pkt, p->hdr_len);
    }

    h264e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h264e_start(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter\n");

    /* Backup dpb for reencode */
    h264e_dpb_copy(&p->dpb_bak, &p->dpb);

    h264e_dbg_func("leave\n");
    (void) task;

    return MPP_OK;
}

static MPP_RET h264e_proc_dpb(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    H264eDpb *dpb = &p->dpb;
    H264eFrmInfo *frms = &p->frms;
    H264eDpbFrmCfg frm_cfg;
    EncFrmStatus *frm = &task->rc_task->frm;
    MppFrame frame = task->frame;
    MppMeta meta = mpp_frame_get_meta(frame);
    RK_S32 i;

    h264e_dbg_func("enter\n");

    /*
     * Step 4: Determine current frame type, reference info and temporal id
     *
     * This part is a complete dpb management for current frame.
     * NOTE: reencode may use force pskip flag to change the dpb behave.
     */
    if (p->idr_request) {
        frm_cfg.force_idr = 1;
        p->idr_request--;
    } else
        frm_cfg.force_idr = 0;

    frm_cfg.force_pskip = 0;
    frm_cfg.force_lt_idx = -1;
    frm_cfg.force_ref_lt_idx = -1;
    mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &frm_cfg.force_ref_lt_idx);

    // update dpb
    h264e_dpb_set_curr(dpb, &frm_cfg);
    h264e_dpb_build_list(dpb);
    h264e_dpb_build_marking(dpb);

    // update frame usage
    frms->seq_idx = dpb->curr->seq_idx;
    frms->curr_idx = dpb->curr->slot_idx;
    frms->refr_idx = (dpb->refr) ? dpb->refr->slot_idx : frms->curr_idx;

    // update frame status for rc module
    *frm = dpb->curr->status;
    frm->seq_idx = dpb->curr->seq_idx;

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(frms->usage); i++)
        frms->usage[i] = dpb->frames[i].on_used;

    // update slice info
    h264e_slice_update(&p->slice, p->cfg, &p->sps, dpb->curr);

    // update dpb to after encoding status
    h264e_dpb_curr_ready(dpb);

    /*
     * Step 5: Wait previous frame bit/quality result
     *
     * On normal case encoder will wait previous encoding done and get feedback
     * from hardware then start the new frame encoding.
     * But for asynchronous process rate control module should be able to
     * handle the case that previous encoding is not done.
     */

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h264e_proc_hal(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    MppMeta meta = mpp_frame_get_meta(task->frame);
    MppEncUserData *user_data = NULL;

    h264e_dbg_func("enter\n");

    mpp_meta_get_ptr(meta, KEY_USER_DATA, (void**)&user_data);
    if (user_data) {
        if (user_data->pdata && user_data->len)
            h264e_sei_to_packet(user_data->pdata, user_data->len,
                                H264_SEI_USER_DATA_UNREGISTERED, task->packet);
        else
            mpp_err_f("failed to insert user data %p len %d\n",
                      user_data->pdata, user_data->len);
    }

    p->syn_num = 0;
    h264e_add_syntax(p, H264E_SYN_CFG, p->cfg);
    h264e_add_syntax(p, H264E_SYN_SPS, &p->sps);
    h264e_add_syntax(p, H264E_SYN_PPS, &p->pps);
    h264e_add_syntax(p, H264E_SYN_SLICE, &p->slice);
    h264e_add_syntax(p, H264E_SYN_FRAME, &p->frms);
    h264e_add_syntax(p, H264E_SYN_RC, &p->rc_syn);

    task->valid = 1;
    task->syntax.data   = &p->syntax[0];
    task->syntax.number = p->syn_num;
    task->is_intra = p->slice.idr_flag;

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h264e_update_hal(void *ctx, HalEncTask *task)
{
    (void) ctx;
    (void) task;

    h264e_dbg_func("enter\n");

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   api struct interface
***********************************************************************
*/
const EncImplApi api_h264e = {
    "h264e_control",
    MPP_VIDEO_CodingAVC,
    sizeof(H264eCtx),
    0,
    h264e_init,
    h264e_deinit,
    h264e_proc_cfg,
    h264e_gen_hdr,
    h264e_start,
    h264e_proc_dpb,
    h264e_proc_hal,
    h264e_update_hal,
    NULL,
    NULL,
    NULL,
};
