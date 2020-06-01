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
#include "mpp_mem.h"

#include "rc.h"
#include "mpp_enc_cfg_impl.h"

#include "h265e_api.h"
#include "h265e_slice.h"
#include "h265e_codec.h"
#include "h265e_syntax_new.h"
#include "h265e_ps.h"
#include "h265e_header_gen.h"

extern RK_U32 h265e_debug;

static MPP_RET h265e_init(void *ctx, EncImplCfg *ctrlCfg)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MPP_RET ret = MPP_OK;
    MppEncCodecCfg *codec = NULL;
    MppEncRcCfg *rc_cfg = &ctrlCfg->cfg->rc;
    MppEncPrepCfg *prep = &ctrlCfg->cfg->prep;
    MppEncH265Cfg *h265 = NULL;

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
    rc_cfg->max_reenc_times = 1;

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

    h265e_set_extra_info(p);
    h265e_get_extra_info(p, pkt);

    if (NULL == p->dpb)
        h265e_dpb_init(&p->dpb);

    h265e_dbg_func("leave ctx %p\n", ctx);

    return MPP_OK;
}

static MPP_RET h265e_start(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    (void) p;
    h265e_dbg_func("enter\n");

    /*
     * Step 2: Fps conversion
     *
     * Determine current frame which should be encoded or not according to
     * input and output frame rate.
     */

    if (!task->valid)
        mpp_log_f("drop one frame\n");

    /*
     * Step 3: Backup dpb for reencode
     */
    //h265e_dpb_copy(&p->dpb_bak, p->dpb);
    h265e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h265e_proc_dpb(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    EncRcTask    *rc_task = task->rc_task;
    EncCpbStatus *cpb = &task->rc_task->cpb;
    h265e_dbg_func("enter\n");

    h265e_dpb_bakup(p->dpb, &p->dpb_bak);
    h265e_dpb_get_curr(p->dpb);
    p->slice = p->dpb->curr->slice;
    h265e_slice_init(ctx, p->slice, cpb->curr);
    h265e_dpb_build_list(p->dpb, cpb);

    rc_task->frm  = p->dpb->curr->status;

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
    h265e_dbg_func("leave ctx %p \n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_add_sei(MppPacket pkt, RK_S32 *length, RK_U8 uuid[16],
                             const void *data, RK_S32 size)
{
    RK_U8 *ptr = mpp_packet_get_pos(pkt);
    RK_U32 offset = mpp_packet_get_length(pkt);
    RK_U32 new_length = 0;

    ptr += offset;
    new_length = h265e_data_to_sei(ptr, uuid, data, size);
    *length = new_length;

    mpp_packet_set_length(pkt, offset + new_length);

    return MPP_OK;
}

static MPP_RET h265e_proc_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    memcpy(dst, src, sizeof(MppEncPrepCfg));
    return MPP_OK;
}

static MPP_RET h265e_proc_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change) {
        MppEncRcCfg bak = *dst;

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

            if ((dst->bps_target > dst->bps_max || dst->bps_target < dst->bps_min) ||
                (dst->bps_max    < dst->bps_min)) {
                mpp_err("invalid bit rate config %d [%d:%d] for size relationship\n",
                        dst->bps_target, dst->bps_min, dst->bps_max);
                ret = MPP_ERR_VALUE;
            }
        }

        dst->change |= change;

        if (ret) {
            mpp_err_f("failed to accept new rc config\n");
            *dst = bak;
        } else {
            mpp_log_f("MPP_ENC_SET_RC_CFG bps %d [%d : %d] fps [%d:%d] gop %d\n",
                      dst->bps_target, dst->bps_min, dst->bps_max,
                      dst->fps_in_num, dst->fps_out_num, dst->gop);
        }
    }

    return ret;
}

static MPP_RET h265e_proc_h265_cfg(MppEncH265Cfg *dst, MppEncH265Cfg *src)
{
    RK_U32 change = src->change;

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

    if (change & MPP_ENC_H265_CFG_CHANGE_VUI) {
        memcpy(&dst->vui, &src->vui, sizeof(src->vui));
    }

    if (change & MPP_ENC_H265_CFG_RC_QP_CHANGE) {
        dst->qp_init = src->qp_init;
        dst->max_qp = src->max_qp;
        dst->min_qp = src->min_qp;
        dst->max_i_qp = src->max_i_qp;
        dst->min_i_qp = src->min_i_qp;
        dst->ip_qp_delta = src->ip_qp_delta;
    }

    /*
     * NOTE: use OR here for avoiding overwrite on multiple config
     * When next encoding is trigger the change flag will be clear
     */
    dst->change |= change;

    return MPP_OK;
}

static MPP_RET h265e_proc_split_cfg(MppEncH265SliceCfg *dst, MppEncSliceSplit *src)
{
    if (src->split_mode > MPP_ENC_SPLIT_NONE) {
        dst->split_enable = 1;
        dst->split_mode = 0;
        if (src->split_mode == MPP_ENC_SPLIT_BY_CTU)
            dst->split_mode = 1;
        dst->slice_size =  src->split_arg;
    } else {
        dst->split_enable = 0;
    }

    return MPP_OK;
}

static MPP_RET h265e_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;
    MPP_RET ret = MPP_OK;

    h265e_dbg_func("enter ctx %p cmd %08x\n", ctx, cmd);

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
        MppEncCfgSet *src = &impl->cfg;

        if (src->prep.change) {
            ret |= h265e_proc_prep_cfg(&cfg->prep, &src->prep);
            src->prep.change = 0;
        }
        if (src->rc.change) {
            ret |= h265e_proc_rc_cfg(&cfg->rc, &src->rc);
            src->rc.change = 0;
        }
        if (src->codec.h265.change) {
            ret |= h265e_proc_h265_cfg(&cfg->codec.h265, &src->codec.h265);
            src->codec.h265.change = 0;
        }
        if (src->split.change) {
            ret |= h265e_proc_split_cfg(&cfg->codec.h265.slice_cfg, &src->split);
            src->split.change = 0;
        }
    } break;
    case MPP_ENC_GET_EXTRA_INFO: {
        MppPacket pkt_out = (MppPacket )param;
        h265e_set_extra_info(p);
        h265e_get_extra_info(p, pkt_out);
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        ret = h265e_proc_prep_cfg(&cfg->prep, param);
    } break;
    case MPP_ENC_SET_CODEC_CFG: {
        MppEncCodecCfg *codec = (MppEncCodecCfg *)param;

        ret = h265e_proc_h265_cfg(&cfg->codec.h265, &codec->h265);
    } break;
    case MPP_ENC_SET_RC_CFG : {
        ret = h265e_proc_rc_cfg(&cfg->rc, param);
    } break;
    case MPP_ENC_SET_SEI_CFG: {
    } break;
    case MPP_ENC_SET_SPLIT : {
        MppEncSliceSplit *src = (MppEncSliceSplit *)param;
        MppEncH265SliceCfg *slice_cfg = &cfg->codec.h265.slice_cfg;

        if (src->split_mode > MPP_ENC_SPLIT_NONE) {
            slice_cfg->split_enable = 1;
            slice_cfg->split_mode = 0;
            if (src->split_mode == MPP_ENC_SPLIT_BY_CTU)
                slice_cfg->split_mode = 1;
            slice_cfg->slice_size =  src->split_arg;
        } else {
            slice_cfg->split_enable = 0;
        }
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
    h265e_proc_hal,
    h265e_add_sei,
    NULL,
    NULL,
    NULL,
};
