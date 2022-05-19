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
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_packet_impl.h"
#include "mpp_enc_refs.h"

#include "h264e_debug.h"
#include "h264e_syntax.h"
#include "h264e_sps.h"
#include "h264e_pps.h"
#include "h264e_sei.h"
#include "h264e_dpb.h"
#include "h264e_slice.h"
#include "h264e_api_v2.h"
#include "rc.h"
#include "mpp_rc.h"

#include "enc_impl_api.h"
#include "mpp_enc_cfg_impl.h"

RK_U32 h264e_debug = 0;

typedef struct {
    /* config from mpp_enc */
    MppClientType       type;
    MppEncCfgSet        *cfg;
    MppEncRefs          refs;
    RK_U32              idr_request;

    /* H.264 high level syntax */
    H264eSps            sps;
    H264ePps            pps;

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
    /* header generation */
    MppPacket           hdr_pkt;
    void                *hdr_buf;
    size_t              hdr_size;
    size_t              hdr_len;
    RK_S32              sps_offset;
    RK_S32              sps_len;
    RK_S32              pps_offset;
    RK_S32              pps_len;
    RK_S32              prefix_offset;
    RK_S32              prefix_len;
    H264ePrefixNal      prefix;

    /* rate control config */
    RcCtx               rc_ctx;

    /* output to hal */
    RK_S32              syn_num;
    H264eSyntaxDesc     syntax[H264E_SYN_BUTT];
} H264eCtx;

static void init_h264e_cfg_set(MppEncCfgSet *cfg, MppClientType type)
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

    switch (type) {
    case VPU_CLIENT_VEPU1 :
    case VPU_CLIENT_VEPU2 : {
        h264->poc_type = 2;
        h264->log2_max_poc_lsb = 12;
        h264->log2_max_frame_num = 12;
    } break;
    case VPU_CLIENT_RKVENC : {
        h264->poc_type = 0;
        h264->log2_max_poc_lsb = 12;
        h264->log2_max_frame_num = 12;
        h264->chroma_cb_qp_offset = -6;
        h264->chroma_cr_qp_offset = -6;
    } break;
    default : {
        h264->poc_type = 0;
        h264->log2_max_poc_lsb = 12;
        h264->log2_max_frame_num = 12;
    } break;
    }

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
    prep->color = MPP_FRAME_SPC_UNSPECIFIED;
    prep->colorprim = MPP_FRAME_PRI_UNSPECIFIED;
    prep->colortrc = MPP_FRAME_TRC_UNSPECIFIED;
    prep->range = MPP_FRAME_RANGE_UNSPECIFIED;
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
    rc_cfg->max_i_prop = 30;
    rc_cfg->min_i_prop = 10;
    rc_cfg->init_ip_ratio = 160;
    rc_cfg->qp_init = 26;
    rc_cfg->qp_max = 48;
    rc_cfg->qp_min = 8;
    /* default max/min intra qp is not set */
    rc_cfg->qp_max_i = 0;
    rc_cfg->qp_min_i = 0;
    rc_cfg->qp_delta_ip = 2;
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

    p->type = ctrl_cfg->type;
    p->hdr_size = SZ_1K;
    p->hdr_buf = mpp_malloc_size(void, p->hdr_size);
    mpp_assert(p->hdr_buf);
    mpp_packet_init(&p->hdr_pkt, p->hdr_buf, p->hdr_size);
    mpp_assert(p->hdr_pkt);

    p->cfg = ctrl_cfg->cfg;
    p->refs = ctrl_cfg->refs;
    p->idr_request = 0;

    h264e_reorder_init(&p->reorder);
    h264e_marking_init(&p->marking);

    h264e_dpb_init(&p->dpb, &p->reorder, &p->marking);
    h264e_slice_init(&p->slice, &p->reorder, &p->marking);

    init_h264e_cfg_set(p->cfg, p->type);

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

static MPP_RET h264e_proc_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    mpp_assert(change);
    if (change) {
        MppEncPrepCfg bak = *dst;

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
            dst->format = src->format;

        if (change & MPP_ENC_PREP_CFG_CHANGE_COLOR_RANGE)
            dst->range = src->range;

        if (change & MPP_ENC_PREP_CFG_CHANGE_COLOR_SPACE)
            dst->color = src->color;

        if (change & MPP_ENC_PREP_CFG_CHANGE_COLOR_PRIME)
            dst->colorprim = src->colorprim;

        if (change & MPP_ENC_PREP_CFG_CHANGE_COLOR_TRC)
            dst->colortrc = src->colortrc;

        if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
            dst->rotation = src->rotation;

        if (change & MPP_ENC_PREP_CFG_CHANGE_MIRRORING)
            dst->mirroring = src->mirroring;

        if (change & MPP_ENC_PREP_CFG_CHANGE_DENOISE)
            dst->denoise = src->denoise;

        if (change & MPP_ENC_PREP_CFG_CHANGE_SHARPEN)
            dst->sharpen = src->sharpen;

        if ((change & MPP_ENC_PREP_CFG_CHANGE_INPUT) ||
            (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)) {
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
        if (dst->rotation == MPP_ENC_ROT_90 || dst->rotation == MPP_ENC_ROT_270) {
            if (dst->height > dst->hor_stride || dst->width > dst->ver_stride) {
                mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                        dst->width, dst->height, dst->hor_stride, dst->ver_stride);
                ret = MPP_ERR_VALUE;
            }
        } else {
            if (dst->width > dst->hor_stride || dst->height > dst->ver_stride) {
                mpp_err("invalid size w:h [%d:%d] stride [%d:%d]\n",
                        dst->width, dst->height, dst->hor_stride, dst->ver_stride);
                ret = MPP_ERR_VALUE;
            }
        }

        if (MPP_FRAME_FMT_IS_FBC(dst->format) && (dst->mirroring || dst->rotation)) {
            mpp_err("invalid cfg fbc data no support mirror %d or rotaion",
                    dst->mirroring, dst->rotation);
            ret = MPP_ERR_VALUE;
        }

        if (dst->range >= MPP_FRAME_RANGE_NB ||
            dst->color >= MPP_FRAME_SPC_NB ||
            dst->colorprim >= MPP_FRAME_PRI_NB ||
            dst->colortrc >= MPP_FRAME_TRC_NB) {
            mpp_err("invalid color range %d colorspace %d primaries %d transfer characteristic %d\n",
                    dst->range, dst->color, dst->colorprim, dst->colortrc);
            ret = MPP_ERR_VALUE;
        }

        if (ret) {
            mpp_err_f("failed to accept new prep config\n");
            *dst = bak;
        } else {
            mpp_log("MPP_ENC_SET_PREP_CFG w:h [%d:%d] stride [%d:%d]\n",
                    dst->width, dst->height,
                    dst->hor_stride, dst->ver_stride);
        }
    }

    src->change = 0;
    return ret;
}

static MPP_RET h264e_proc_h264_cfg(MppEncH264Cfg *dst, MppEncH264Cfg *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    // TODO: do codec check first
    if (change & MPP_ENC_H264_CFG_STREAM_TYPE)
        dst->stream_type = src->stream_type;
    if ((change & MPP_ENC_H264_CFG_CHANGE_PROFILE) &&
        ((dst->profile != src->profile) || (dst->level != src->level))) {
        dst->profile = src->profile;
        dst->level = src->level;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_PROFILE;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_POC_TYPE) &&
        (dst->poc_type != src->poc_type)) {
        dst->poc_type = src->poc_type;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_POC_TYPE;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_MAX_POC_LSB) &&
        (dst->log2_max_poc_lsb != src->log2_max_poc_lsb)) {
        dst->log2_max_poc_lsb = src->log2_max_poc_lsb;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_MAX_POC_LSB;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_MAX_FRM_NUM) &&
        (dst->log2_max_frame_num != src->log2_max_frame_num)) {
        dst->log2_max_frame_num = src->log2_max_frame_num;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_MAX_FRM_NUM;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_GAPS_IN_FRM_NUM) &&
        (dst->gaps_not_allowed != src->gaps_not_allowed)) {
        dst->gaps_not_allowed = src->gaps_not_allowed;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_GAPS_IN_FRM_NUM;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_ENTROPY) &&
        ((dst->entropy_coding_mode != src->entropy_coding_mode) ||
         (dst->cabac_init_idc != src->cabac_init_idc))) {
        dst->entropy_coding_mode = src->entropy_coding_mode;
        dst->cabac_init_idc = src->cabac_init_idc;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_ENTROPY;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_TRANS_8x8) &&
        (dst->transform8x8_mode != src->transform8x8_mode)) {
        dst->transform8x8_mode = src->transform8x8_mode;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_TRANS_8x8;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_CONST_INTRA) &&
        (dst->constrained_intra_pred_mode != src->constrained_intra_pred_mode)) {
        dst->constrained_intra_pred_mode = src->constrained_intra_pred_mode;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_CONST_INTRA;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_CHROMA_QP) &&
        (dst->chroma_cb_qp_offset != src->chroma_cb_qp_offset ||
         dst->chroma_cr_qp_offset != src->chroma_cr_qp_offset)) {
        dst->chroma_cb_qp_offset = src->chroma_cb_qp_offset;
        dst->chroma_cr_qp_offset = src->chroma_cr_qp_offset;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_CHROMA_QP;
    }
    if ((change & MPP_ENC_H264_CFG_CHANGE_DEBLOCKING) &&
        ((dst->deblock_disable != src->deblock_disable) ||
         (dst->deblock_offset_alpha != src->deblock_offset_alpha) ||
         (dst->deblock_offset_beta != src->deblock_offset_beta))) {
        dst->deblock_disable = src->deblock_disable;
        dst->deblock_offset_alpha = src->deblock_offset_alpha;
        dst->deblock_offset_beta = src->deblock_offset_beta;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_DEBLOCKING;
    }
    if (change & MPP_ENC_H264_CFG_CHANGE_LONG_TERM)
        dst->use_longterm = src->use_longterm;

    if ((change & MPP_ENC_H264_CFG_CHANGE_SCALING_LIST) &&
        (dst->scaling_list_mode != src->scaling_list_mode)) {
        dst->scaling_list_mode = src->scaling_list_mode;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_SCALING_LIST;
    }

    if ((change & MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH) &&
        ((dst->intra_refresh_mode != src->intra_refresh_mode) ||
         (dst->intra_refresh_arg != src->intra_refresh_arg))) {
        dst->intra_refresh_mode = src->intra_refresh_mode;
        dst->intra_refresh_arg = src->intra_refresh_arg;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_INTRA_REFRESH;
    }

    if ((change & MPP_ENC_H264_CFG_CHANGE_MAX_LTR) &&
        (dst->max_ltr_frames != src->max_ltr_frames)) {
        dst->max_ltr_frames = src->max_ltr_frames;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_MAX_LTR;
    }

    if ((change & MPP_ENC_H264_CFG_CHANGE_MAX_TID) &&
        (dst->max_tid != src->max_tid)) {
        dst->max_tid = src->max_tid;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_MAX_TID;
    }

    if ((change & MPP_ENC_H264_CFG_CHANGE_ADD_PREFIX) &&
        (dst->prefix_mode != src->prefix_mode)) {
        dst->prefix_mode = src->prefix_mode;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_ADD_PREFIX;
    }

    if ((change & MPP_ENC_H264_CFG_CHANGE_BASE_LAYER_PID) &&
        (dst->base_layer_pid != src->base_layer_pid)) {
        dst->base_layer_pid = src->base_layer_pid;
        dst->change |= MPP_ENC_H264_CFG_CHANGE_BASE_LAYER_PID;
    }

    src->change = 0;
    return ret;
}

static MPP_RET h264e_proc_split_cfg(MppEncSliceSplit *dst, MppEncSliceSplit *src)
{
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_MODE) {
        dst->split_mode = src->split_mode;
        dst->split_arg = src->split_arg;
    }

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_ARG)
        dst->split_arg = src->split_arg;

    if (change & MPP_ENC_SPLIT_CFG_CHANGE_OUTPUT)
        dst->split_out = src->split_out;

    dst->change |= change;
    src->change = 0;

    return ret;
}

static void h264e_check_cfg(MppEncCfgSet *cfg)
{
    MppEncRcCfg *rc = &cfg->rc;
    MppEncH264Cfg *h264 = &cfg->codec.h264;

    if (rc->drop_mode == MPP_ENC_RC_DROP_FRM_PSKIP &&
        rc->drop_gap == 0 &&
        h264->poc_type == 2) {
        mpp_err("poc type 2 is ocnflict with successive non-reference pskip mode\n");
        mpp_err("set drop gap to 1\n");
        rc->drop_gap = 1;
    }
}

static MPP_RET h264e_proc_cfg(void *ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    H264eCtx *p = (H264eCtx *)ctx;
    MppEncCfgSet *cfg = p->cfg;

    h264e_dbg_func("enter ctx %p cmd %x param %p\n", ctx, cmd, param);

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncCfgImpl *impl = (MppEncCfgImpl *)param;
        MppEncCfgSet *src = &impl->cfg;

        if (src->prep.change)
            ret |= h264e_proc_prep_cfg(&cfg->prep, &src->prep);

        if (src->codec.h264.change)
            ret |= h264e_proc_h264_cfg(&cfg->codec.h264, &src->codec.h264);

        if (src->split.change)
            ret |= h264e_proc_split_cfg(&cfg->split, &src->split);
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        ret = h264e_proc_prep_cfg(&cfg->prep, param);
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        MppEncCodecCfg *codec = (MppEncCodecCfg *)param;
        ret = h264e_proc_h264_cfg(&cfg->codec.h264, &codec->h264);
    } break;
    case MPP_ENC_SET_SEI_CFG : {
    } break;
    case MPP_ENC_SET_SPLIT : {
        ret = h264e_proc_split_cfg(&cfg->split, param);
    } break;
    default:
        mpp_err("No correspond cmd found, and can not config!");
        ret = MPP_NOK;
        break;
    }

    h264e_check_cfg(cfg);

    h264e_dbg_func("leave ret %d\n", ret);

    return ret;
}

static MPP_RET h264e_gen_hdr(void *ctx, MppPacket pkt)
{
    H264eCtx *p = (H264eCtx *)ctx;

    h264e_dbg_func("enter\n");

    h264e_sps_update(&p->sps, p->cfg);
    h264e_pps_update(&p->pps, p->cfg);

    /*
     * NOTE: When sps/pps is update we need to update dpb and slice info
     */
    h264e_dpb_setup(&p->dpb, p->cfg, &p->sps);

    mpp_packet_reset(p->hdr_pkt);
    h264e_sps_to_packet(&p->sps, p->hdr_pkt, &p->sps_offset, &p->sps_len);
    h264e_pps_to_packet(&p->pps, p->hdr_pkt, &p->pps_offset, &p->pps_len);
    p->hdr_len = mpp_packet_get_length(p->hdr_pkt);

    if (pkt) {
        mpp_packet_write(pkt, 0, p->hdr_buf, p->hdr_len);
        mpp_packet_set_length(pkt, p->hdr_len);

        mpp_packet_add_segment_info(pkt, H264_NALU_TYPE_SPS,
                                    p->sps_offset, p->sps_len);
        mpp_packet_add_segment_info(pkt, H264_NALU_TYPE_PPS,
                                    p->pps_offset, p->pps_len);
    }

    h264e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h264e_start(void *ctx, HalEncTask *task)
{
    h264e_dbg_func("enter\n");

    if (mpp_frame_has_meta(task->frame)) {
        MppEncRefFrmUsrCfg *frm_cfg = task->frm_cfg;
        EncRcForceCfg *rc_force = &task->rc_task->force;
        MppMeta meta = mpp_frame_get_meta(task->frame);
        RK_S32 force_lt_idx = -1;
        RK_S32 force_use_lt_idx = -1;
        RK_S32 force_frame_qp = -1;
        RK_S32 base_layer_pid = -1;

        mpp_meta_get_s32(meta, KEY_ENC_MARK_LTR, &force_lt_idx);
        mpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &force_use_lt_idx);
        mpp_meta_get_s32(meta, KEY_ENC_FRAME_QP, &force_frame_qp);
        mpp_meta_get_s32(meta, KEY_ENC_BASE_LAYER_PID, &base_layer_pid);

        if (force_lt_idx >= 0) {
            frm_cfg->force_flag |= ENC_FORCE_LT_REF_IDX;
            frm_cfg->force_lt_idx = force_lt_idx;
        }

        if (force_use_lt_idx >= 0) {
            frm_cfg->force_flag |= ENC_FORCE_REF_MODE;
            frm_cfg->force_ref_mode = REF_TO_LT_REF_IDX;
            frm_cfg->force_ref_arg = force_use_lt_idx;
        }

        if (force_frame_qp >= 0) {
            rc_force->force_flag = ENC_RC_FORCE_QP;
            rc_force->force_qp = force_frame_qp;
        } else {
            rc_force->force_flag &= (~ENC_RC_FORCE_QP);
            rc_force->force_qp = -1;
        }

        if (base_layer_pid >= 0) {
            H264eCtx *p = (H264eCtx *)ctx;
            MppEncH264Cfg *h264 = &p->cfg->codec.h264;

            h264->base_layer_pid = base_layer_pid;
        }
    }

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h264e_proc_dpb(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    H264eDpb *dpb = &p->dpb;
    H264eFrmInfo *frms = &p->frms;
    EncCpbStatus *cpb = &task->rc_task->cpb;
    EncFrmStatus *frm = &task->rc_task->frm;
    H264eDpbFrm *curr = NULL;
    H264eDpbFrm *refr = NULL;
    RK_S32 i;

    h264e_dbg_func("enter\n");

    // update dpb
    h264e_dpb_proc(dpb, cpb);

    curr = dpb->curr;
    refr = dpb->refr;

    // update slice info
    h264e_slice_update(&p->slice, p->cfg, &p->sps, &p->pps, dpb->curr);

    // update frame usage
    frms->seq_idx = curr->seq_idx;
    frms->curr_idx = curr->slot_idx;
    frms->refr_idx = (refr) ? refr->slot_idx : curr->slot_idx;

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(frms->usage); i++)
        frms->usage[i] = dpb->frames[i].on_used;

    // update dpb to after encoding status
    h264e_dpb_check(dpb, cpb);

    frm->val = curr->status.val;

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h264e_proc_hal(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    MppEncH264Cfg *h264 = &p->cfg->codec.h264;

    h264e_dbg_func("enter\n");

    p->syn_num = 0;
    h264e_add_syntax(p, H264E_SYN_CFG, p->cfg);
    h264e_add_syntax(p, H264E_SYN_SPS, &p->sps);
    h264e_add_syntax(p, H264E_SYN_PPS, &p->pps);
    h264e_add_syntax(p, H264E_SYN_DPB, &p->dpb);
    h264e_add_syntax(p, H264E_SYN_SLICE, &p->slice);
    h264e_add_syntax(p, H264E_SYN_FRAME, &p->frms);

    /* check max temporal layer id */
    {
        MppEncCpbInfo *cpb_info = mpp_enc_ref_cfg_get_cpb_info(p->cfg->ref_cfg);
        RK_S32 cpb_max_tid = cpb_info->max_st_tid;
        RK_S32 cfg_max_tid = h264->max_tid;

        if (cpb_max_tid != cfg_max_tid) {
            mpp_log("max tid is update to match cpb %d -> %d\n",
                    cfg_max_tid, cpb_max_tid);
            h264->max_tid = cpb_max_tid;
        }
    }

    /* NOTE: prefix nal is added after SEI packet and before hw_stream */
    if (h264->prefix_mode || h264->max_tid) {
        H264ePrefixNal *prefix = &p->prefix;
        H264eSlice *slice = &p->slice;
        EncFrmStatus *frm = &task->rc_task->frm;
        MppPacket packet = task->packet;
        MppMeta meta = mpp_packet_get_meta(packet);

        prefix->idr_flag = slice->idr_flag;
        prefix->nal_ref_idc = slice->nal_reference_idc;
        prefix->priority_id = h264->base_layer_pid + frm->temporal_id;
        prefix->no_inter_layer_pred_flag = 1;
        prefix->dependency_id = 0;
        prefix->quality_id = 0;
        prefix->temporal_id = frm->temporal_id;
        prefix->use_ref_base_pic_flag = 0;
        prefix->discardable_flag = 0;
        prefix->output_flag = 1;

        h264e_add_syntax(p, H264E_SYN_PREFIX, &p->prefix);
        mpp_meta_set_s32(meta, KEY_TEMPORAL_ID, frm->temporal_id);
        if (!frm->is_non_ref && frm->is_lt_ref)
            mpp_meta_set_s32(meta, KEY_LONG_REF_IDX, frm->lt_idx);
    } else
        h264e_add_syntax(p, H264E_SYN_PREFIX, NULL);

    task->valid = 1;
    task->syntax.data   = &p->syntax[0];
    task->syntax.number = p->syn_num;
    h264->change = 0;

    h264e_dbg_func("leave\n");

    return MPP_OK;
}

static MPP_RET h264e_sw_enc(void *ctx, HalEncTask *task)
{
    H264eCtx *p = (H264eCtx *)ctx;
    MppEncH264Cfg *h264 = &p->cfg->codec.h264;
    EncRcTaskInfo *rc_info = &task->rc_task->info;
    MppPacket packet = task->packet;
    void *pos = mpp_packet_get_pos(packet);
    void *data = mpp_packet_get_data(packet);
    size_t size = mpp_packet_get_size(packet);
    size_t length = mpp_packet_get_length(packet);
    void *base = pos + length;
    RK_S32 buf_size = (data + size) - (pos + length);
    RK_S32 slice_len = 0;
    RK_S32 final_len = 0;

    if (h264->prefix_mode || h264->max_tid) {
        /* add prefix first */
        RK_S32 prefix_bit = h264e_slice_write_prefix_nal_unit_svc(&p->prefix, base, buf_size);

        prefix_bit = (prefix_bit + 7) / 8;

        base += prefix_bit;
        buf_size -= prefix_bit;
        final_len += prefix_bit;
    }

    /* write slice header */
    slice_len = h264e_slice_write_pskip(&p->slice, base, buf_size);
    slice_len = (slice_len + 7) / 8;
    final_len += slice_len;

    task->length += final_len;

    rc_info->bit_real = task->length;
    rc_info->quality_real = rc_info->quality_target;

    return MPP_OK;
}

MPP_RET h264e_add_sei(MppPacket pkt, RK_S32 *length, RK_U8 uuid[16],
                      const void *data, RK_S32 size)
{
    return h264e_sei_to_packet(pkt, length, H264_SEI_USER_DATA_UNREGISTERED,
                               uuid, data, size);
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
    h264e_add_sei,
    h264e_sw_enc,
};
