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

#define MODULE_TAG "h265e_api"

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

RK_U32 h265e_debug = 0;

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
    h265->ip_qp_delta = 5;
    h265->raw_dealt_qp = 4;
    h265->max_delta_qp = 10;
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
    h265->sao_cfg.slice_sao_chroma_disable = 0;
    h265->sao_cfg.slice_sao_luma_disable = 0;
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
    prep->color = MPP_FRAME_SPC_UNSPECIFIED;
    prep->colorprim = MPP_FRAME_PRI_UNSPECIFIED;
    prep->colortrc = MPP_FRAME_TRC_UNSPECIFIED;
    prep->range = MPP_FRAME_RANGE_UNSPECIFIED;
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
    rc_cfg->qp_max = 51;
    rc_cfg->qp_min = 10;
    rc_cfg->qp_max_i = 51;
    rc_cfg->qp_min_i = 15;
    rc_cfg->qp_delta_ip = 4;
    rc_cfg->qp_delta_vi = 2;

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
    h265e_dpb_proc_cpb(p->dpb, cpb);
    h265e_dpb_get_curr(p->dpb);
    h265e_slice_init(ctx, cpb->curr);
    h265e_dpb_build_list(p->dpb, cpb);

    rc_task->frm  = p->dpb->curr->status;

    h265e_dbg_func("leave\n");
    return MPP_OK;
}

static MPP_RET h265e_proc_hal(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    H265eSyntax_new *syntax = NULL;
    EncFrmStatus *frm = &task->rc_task->frm;
    MppPacket packet = task->packet;
    MppMeta meta = mpp_packet_get_meta(packet);

    if (ctx == NULL) {
        mpp_err_f("invalid NULL ctx\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_meta_set_s32(meta, KEY_TEMPORAL_ID, frm->temporal_id);
    h265e_dbg_func("enter ctx %p \n", ctx);
    syntax = &p->syntax;
    h265e_syntax_fill(ctx);
    task->valid = 1;
    task->syntax.data = syntax;
    h265e_dbg_func("leave ctx %p \n", ctx);
    return MPP_OK;
}

static MPP_RET h265e_proc_enc_skip(void *ctx, HalEncTask *task)
{
    H265eCtx *p = (H265eCtx *)ctx;
    MppPacket pkt = task->packet;
    RK_U8 *ptr = mpp_packet_get_pos(pkt);
    RK_U32 offset = mpp_packet_get_length(pkt);
    RK_U32 len    = mpp_packet_get_size(pkt) - offset;
    RK_U32 new_length = 0;

    h265e_dbg_func("enter\n");
    ptr += offset;
    p->slice->m_sliceQp = task->rc_task->info.quality_target;
    new_length = h265e_code_slice_skip_frame(ctx, p->slice, ptr, len);
    task->length = new_length;
    task->rc_task->info.bit_real = 8 * new_length;
    ///mpp_packet_set_length(pkt, offset + new_length);

    h265e_dbg_func("leave\n");
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
    MPP_RET ret = MPP_OK;
    RK_U32 change = src->change;

    mpp_assert(change);

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
        mpp_log_f("MPP_ENC_SET_PREP_CFG w:h [%d:%d] stride [%d:%d]\n",
                  dst->width, dst->height,
                  dst->hor_stride, dst->ver_stride);
    }
    return MPP_OK;
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

    if (change & MPP_ENC_H265_CFG_SAO_CHANGE) {
        memcpy(&dst->sao_cfg, &src->sao_cfg, sizeof(src->sao_cfg));
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
    h265e_proc_enc_skip,
};
