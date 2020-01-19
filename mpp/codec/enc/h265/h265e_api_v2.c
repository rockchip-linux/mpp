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

#define MODULE_TAG "h265e_api_v2"

#include <string.h>

#include "mpp_env.h"

#include "h265e_api.h"
#include "h265e_slice.h"
#include "h265e_codec.h"
#include "h265e_syntax_new.h"
#include "h265e_ps.h"
#include "h265e_header_gen.h"
#include "mpp_mem.h"
#include "rc.h"

extern RK_U32 h265e_debug;

static MPP_RET h265e_init(void *ctx, EncImplCfg *ctrlCfg)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MPP_RET ret = MPP_OK;
    MppEncCodecCfg *codec = NULL;
    MppEncRcCfg *rc_cfg = &ctrlCfg->cfg->rc;
    MppEncPrepCfg *prep = &ctrlCfg->cfg->prep;
    MppEncH265Cfg *h265 = NULL;
    H265eDpbCfg *dpb_cfg = NULL;

    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("h265e_debug", &h265e_debug, 0);
    h265e_dbg_func("enter ctx %p\n", ctx);

    mpp_assert(ctrlCfg->coding == MPP_VIDEO_CodingHEVC);
    p->cfg = ctrlCfg->cfg;
    p->set = ctrlCfg->set;

    memset(&p->syntax, 0, sizeof(p->syntax));
    ctrlCfg->task_count = 1;

    p->extra_info = mpp_calloc(H265eExtraInfo, 1);

    p->param_buf = mpp_calloc_size(void,  H265E_EXTRA_INFO_BUF_SIZE);
    mpp_packet_init(&p->packeted_param, p->param_buf, H265E_EXTRA_INFO_BUF_SIZE);

    h265e_init_extra_info(p->extra_info);
    /* set defualt value of codec */
    codec = &p->cfg->codec;
    h265 = &codec->h265;
    dpb_cfg = &p->dpbcfg;
    h265->intra_qp = 26;
    h265->max_qp = 51;
    h265->min_qp = 10;
    h265->max_i_qp = 51;
    h265->min_i_qp = 10;
    h265->qpmap_mode = 1;
    h265->ip_qp_delta = 3;
    h265->raw_dealt_qp = 2;
    h265->max_delta_qp = 10;
    h265->const_intra_pred = 0;
    h265->gop_delta_qp = 0;
    h265->intra_refresh_mode = 0;
    h265->intra_refresh_arg = 0;
    h265->independ_slice_mode = 0;
    h265->independ_slice_arg = 0;
    h265->depend_slice_mode = 0;
    h265->depend_slice_arg = 0;

    h265->profile = MPP_PROFILE_HEVC_MAIN;
    h265->level = 120;
    h265->const_intra_pred = 0;           /* constraint intra prediction flag */
    h265->ctu_size = 64;
    h265->max_cu_size = 64;
    h265->tmvp_enable = 1;
    h265->amp_enable = 0;
    h265->sao_enable = 1;

    h265->num_ref = 1;

    h265->slice_cfg.split_enable = 0;
    h265->entropy_cfg.cabac_init_flag = 1;
    h265->sao_cfg.slice_sao_chroma_flag = 0;
    h265->sao_cfg.slice_sao_luma_flag = 0;
    h265->dblk_cfg.slice_deblocking_filter_disabled_flag = 0;
    h265->cu_cfg.strong_intra_smoothing_enabled_flag = 1;
    h265->merge_cfg.max_mrg_cnd = 2;
    h265->merge_cfg.merge_left_flag = 1;
    h265->merge_cfg.merge_up_flag = 1;

    dpb_cfg->maxNumReferences = 1;
    dpb_cfg->bBPyramid = 0;
    dpb_cfg->bOpenGOP = 0;
    dpb_cfg->nLongTerm = 0;
    dpb_cfg->gop_len = 60;
    dpb_cfg->vgop_size = 1;
    memset(dpb_cfg->nDeltaPocIdx, -1, sizeof(dpb_cfg->nDeltaPocIdx));
    dpb_cfg->tot_poc_num = dpb_cfg->maxNumReferences;

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

    INIT_LIST_HEAD(&p->rc_list);

    h265e_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

static MPP_RET h265e_deinit(void *ctx)
{
    H265eCtx *p = (H265eCtx *)ctx;

    h265e_dbg_func("enter ctx %p\n", ctx);

    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    if (p->rc_ctx)
        rc_deinit(p->rc_ctx);

    h265e_deinit_extra_info(p->extra_info);

    MPP_FREE(p->extra_info);
    MPP_FREE(p->param_buf);
    if (p->packeted_param)
        mpp_packet_deinit(&p->packeted_param);

    h265e_dpb_deinit(p->dpb);

    h265e_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_gen_hdr(void *ctx, MppPacket pkt)
{
    H265eCtx *p = (H265eCtx *)ctx;

    h265e_dbg_func("enter ctx %p\n", ctx);

    h265e_dpb_set_cfg(&p->dpbcfg, p->cfg);
    h265e_set_extra_info(p);
    h265e_get_extra_info(p, pkt);

    h265e_dbg_func("leave ctx %p\n", ctx);

    return MPP_OK;
}

static void h265_set_rc_cfg(RcCfg *cfg, MppEncRcCfg *rc, MppEncGopRef *ref)
{
    switch (rc->rc_mode) {
    case MPP_ENC_RC_MODE_CBR : {
        cfg->mode = RC_CBR;
    } break;
    case MPP_ENC_RC_MODE_VBR : {
        cfg->mode = RC_VBR;
    } break;
    case MPP_ENC_RC_MODE_FIXQP: {
        cfg->mode = RC_FIXQP;
    } break;
    default : {
        cfg->mode = RC_AVBR;
    } break;
    }

    cfg->fps.fps_in_flex    = rc->fps_in_flex;
    cfg->fps.fps_in_num     = rc->fps_in_num;
    cfg->fps.fps_in_denorm  = rc->fps_in_denorm;
    cfg->fps.fps_out_flex   = rc->fps_out_flex;
    cfg->fps.fps_out_num    = rc->fps_out_num;
    cfg->fps.fps_out_denorm = rc->fps_out_denorm;
    cfg->igop               = rc->gop;
    cfg->max_i_bit_prop     = 20;

    cfg->bps_target     = rc->bps_target;
    cfg->bps_max        = rc->bps_max;
    cfg->bps_min        = rc->bps_min;

    cfg->vgop = (ref->gop_cfg_enable) ? ref->ref_gop_len : 0;

    if (ref->layer_rc_enable) {
        RK_S32 i;

        for (i = 0; i < MAX_TEMPORAL_LAYER; i++) {
            cfg->layer_bit_prop[i] = ref->layer_weight[i];
        }
    } else {
        cfg->layer_bit_prop[0] = 256;
        cfg->layer_bit_prop[1] = 0;
        cfg->layer_bit_prop[2] = 0;
        cfg->layer_bit_prop[3] = 0;
    }
    cfg->stat_times     = rc->stat_times;
    cfg->max_reencode_times = 1;
}


static MPP_RET h265e_start(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;
    MppEncGopRef *ref = &cfg->gop_ref;
    MppEncRcCfg *rc = &cfg->rc;
    MppEncCodecCfg *codec = &p->cfg->codec;

    h265e_dbg_func("enter\n");

    /* Step 1: Check and update config */
    if (rc->change || ref->change) {
        if (p->rc_ctx) {
            rc_deinit(p->rc_ctx);
            p->rc_ctx = NULL;
        }

        RcCfg rc_cfg;

        h265_set_rc_cfg(&rc_cfg, rc, ref);

        rc_init(&p->rc_ctx, MPP_VIDEO_CodingHEVC, NULL);
        mpp_assert(p->rc_ctx);
        rc_update_usr_cfg(p->rc_ctx, &rc_cfg);

        p->rc_ready = 1;
        rc->change = 0;
        ref->change = 0;
    }

    if (codec->change) {
        if (NULL == p->dpb) {

            h265e_dpb_init(&p->dpb, &p->dpbcfg);

        }
        if (rc->gop < 0) {
            p->dpb->idr_gap = 10000;
        } else {
            p->dpb->idr_gap = rc->gop;
        }
        codec->change = 0;
    }

    /*
     * Step 2: Fps conversion
     *
     * Determine current frame which should be encoded or not according to
     * input and output frame rate.
     */

    if (p->rc_ctx)
        task->valid = !rc_frm_check_drop(p->rc_ctx);
    if (!task->valid)
        mpp_log_f("drop one frame\n");

    /*
     * Step 3: Backup dpb for reencode
     */
    //h265e_dpb_copy(&p->dpb_bak, p->dpb);

    return MPP_OK;
}

static MPP_RET h265e_proc_dpb(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MppEncCodecCfg *codec = &p->cfg->codec;
    MppEncH265Cfg *h265 = &codec->h265;
    MppFrame frame = task->frame;
    H265eDpbCfg *dpb_cfg = &p->dpbcfg;
    H265eFrmInfo *frms = &p->frms;
    MppMeta meta = mpp_frame_get_meta(frame);
    RK_S32 mb_wd64, mb_h64;

    h265e_dbg_func("enter\n");

    mb_wd64 = (p->cfg->prep.width + 63) / 64;
    mb_h64 = (p->cfg->prep.height + 63) / 64;

    if (NULL == p->dpb) {

        h265e_dpb_init(&p->dpb, &p->dpbcfg);


    }

    if (p->idr_request) {
        dpb_cfg->force_idr = 1;
        p->idr_request = 0;
    } else
        dpb_cfg->force_idr = 0;

    dpb_cfg->force_pskip = 0;
    dpb_cfg->force_lt_idx = -1;
    dpb_cfg->force_ref_lt_idx = -1;
    mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &dpb_cfg->force_ref_lt_idx);

    if (h265->ref_cfg.num_lt_ref_pic > 0)
        p->dpb->is_long_term = 1;

    h265e_dpb_get_curr(p->dpb);
    p->slice = p->dpb->curr->slice;
    h265e_slice_init(ctx, p->slice);
    h265e_dpb_build_list(p->dpb);

    frms->seq_idx = p->dpb->curr->seq_idx;
    frms->status = p->dpb->curr->status;

    frms->mb_per_frame = mb_wd64 * mb_h64;
    frms->mb_raw = mb_h64;
    frms->mb_wid = mb_wd64 ;

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_proc_rc(void *ctx, HalEncTask *task)
{
    (void)task;
    H265eCtx *p = (H265eCtx *)ctx;
    H265eFrmInfo *frms = &p->frms;
    MppEncRcCfg *rc = &p->cfg->rc;

    h265e_dbg_func("enter\n");
    if (!p->rc_ready && rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        mpp_err_f("not initialize encoding\n");
        return MPP_NOK;
    }
    if (rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        rc_frm_start(p->rc_ctx, &frms->rc_cfg, &frms->status);
    }

    p->frms.status.reencode = 0;

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_proc_hal(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    H265eSyntax_new *syntax = NULL;

    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    h265e_dbg_func("enter ctx %p \n", ctx);
    syntax = &p->syntax;
    h265e_syntax_fill(ctx);
    task->valid = 1;
    task->syntax.data = syntax;

    task->hal_ret.data = &p->frms.rc_cfg;
    task->hal_ret.number = 1;

    h265e_dbg_func("leave ctx %p \n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_update_hal(void *ctx, HalEncTask *task)
{
    (void)ctx;
    MppFrame frame = task->frame;
    RK_U8 *out_ptr = mpp_buffer_get_ptr(task->output);
    MppMeta meta = mpp_frame_get_meta(frame);
    MppEncUserData *user_data = NULL;

    h265e_dbg_func("enter\n");

    if (!task->length || NULL == out_ptr)
        return MPP_OK;

    out_ptr = out_ptr + task->length;
    mpp_meta_get_ptr(meta, KEY_USER_DATA, (void**)&user_data);
    if (user_data && user_data->len)
        h265e_insert_user_data(out_ptr, user_data->pdata, user_data->len);

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_update_rc(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MppEncRcCfg *rc = &p->cfg->rc;
    RcHalCfg *rc_hal_cfg = NULL;

    h265e_dbg_func("enter\n");

    rc_hal_cfg = (RcHalCfg *)task->hal_ret.data;
    if (rc->rc_mode != MPP_ENC_RC_MODE_FIXQP) {
        rc_frm_end(p->rc_ctx, rc_hal_cfg);
        if (rc_hal_cfg->need_reenc) {
            p->frms.status.reencode = 1;
        }
    }
    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_reset(void *ctx)
{
    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    h265e_dbg_func("enter ctx %p\n", ctx);
    h265e_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_flush(void *ctx)
{
    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    h265e_dbg_func("enter ctx %p\n", ctx);
    h265e_dbg_func("leave ctx %p\n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MPP_RET ret = MPP_OK;

    h265e_dbg_func("enter ctx %p cmd %08x\n", ctx, cmd);

    switch (cmd) {
    case MPP_ENC_SET_IDR_FRAME : {
        p->syntax.idr_request = 1;
        p->idr_request = 1;
    } break;
    case MPP_ENC_GET_EXTRA_INFO: {
        MppPacket pkt_out = (MppPacket )param;
        h265e_set_extra_info(p);
        h265e_get_extra_info(p, pkt_out);
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        MppEncPrepCfg *src = (MppEncPrepCfg *)param;
        MppEncPrepCfg *dst = &p->cfg->prep;

        memcpy(dst, src, sizeof(MppEncPrepCfg));
    } break;
    case MPP_ENC_SET_CODEC_CFG: {
        MppEncCodecCfg *cfg = (MppEncCodecCfg *)param;
        MppEncH265Cfg *src = &cfg->h265;
        MppEncH265Cfg *dst = &p->cfg->codec.h265;
        RK_U32 change = cfg->h265.change;

        // TODO: do codec check first
        if (change & MPP_ENC_H265_CFG_PROFILE_LEVEL_TILER_CHANGE) {
            dst->profile = src->profile;
            dst->level = src->level;
        }

        if (change & MPP_ENC_H265_CFG_CU_CHANGE) {
            memcpy(&dst->cu_cfg, &src->cu_cfg, sizeof(src->cu_cfg));
        }
        if (change & MPP_ENC_H265_CFG_DBLK_CHANGE) {
            memcpy(&dst->dblk_cfg, &src->dblk_cfg, sizeof(src->dblk_cfg));
        }
        if (change & MPP_ENC_H265_CFG_SAO_CHANGE) {
            memcpy(&dst->sao_cfg, &src->sao_cfg, sizeof(src->sao_cfg));
        }
        if (change & MPP_ENC_H265_CFG_TRANS_CHANGE) {
            memcpy(&dst->trans_cfg, &src->trans_cfg, sizeof(src->trans_cfg));
        }

        if (change & MPP_ENC_H265_CFG_SLICE_CHANGE) {
            memcpy(&dst->slice_cfg, &src->slice_cfg, sizeof(src->slice_cfg));
        }

        if (change & MPP_ENC_H265_CFG_ENTROPY_CHANGE) {
            memcpy(&dst->entropy_cfg, &src->entropy_cfg, sizeof(src->entropy_cfg));
        }

        if (change & MPP_ENC_H265_CFG_MERGE_CHANGE) {
            memcpy(&dst->merge_cfg, &src->merge_cfg, sizeof(src->merge_cfg));
        }

        if (change & MPP_ENC_H265_CFG_RC_QP_CHANGE) {
            dst->qp_init = src->qp_init;
            dst->max_qp = src->max_qp;
            dst->min_qp = src->min_qp;
            dst->max_i_qp = src->max_i_qp;
            dst->min_i_qp = src->min_i_qp;
        }
        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
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
    case MPP_ENC_SET_SEI_CFG: {

    } break;

    case MPP_ENC_SET_GOPREF: {
        MppEncGopRef *ref = (MppEncGopRef *)param;
        MppEncCfgSet *cfg = p->cfg;
        memcpy(&cfg->gop_ref, ref , sizeof(*ref));
    } break;

    default:
        mpp_err("No correspond %08x found, and can not config!\n", cmd);
        ret = MPP_NOK;
        break;
    }

    h265e_dbg_func("leave ctx %p\n", ctx);
    return ret;
}

const EncImplApi api_h265e = {
    "h265e_control",
    MPP_VIDEO_CodingHEVC,
    sizeof(H265eCtx),
    0,
    h265e_init,
    h265e_deinit,
    h265e_proc_cfg,
    h265e_gen_hdr,
    h265e_start,
    h265e_proc_dpb,
    h265e_proc_rc,
    h265e_proc_hal,
    h265e_update_hal,
    h265e_update_rc,
    h265e_reset,
    h265e_flush,
    NULL,
};
