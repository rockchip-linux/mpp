/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "H265d"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_packet_impl.h"
#include "rk_hdr_meta_com.h"

#include "mpp_parser.h"
#include "h265d_debug.h"
#include "h265d_spliter.h"
#include "h265d_dpb.h"
#include "h265d_syntax.h"

#ifdef dump
FILE *fp = NULL;
#endif

MPP_RET h265d_init(void *ctx, ParserCfg *parser_cfg)
{
    H265dCtx *p = (H265dCtx *)ctx;
    H265dPrs *s = (H265dPrs *)p->parser;
    RK_S32 ret;

    mpp_env_get_u32("h265d_debug", &h265d_debug, 0);

    p->parser_cfg = parser_cfg;
    p->cfg = parser_cfg->cfg;

    if (p->cfg->base.split_parse)
        h265d_spliter_init(&p->spliter);

    if (s == NULL) {
        ret = h265d_parser_init(&p->parser, p);
        if (ret < 0)
            return ret;

        s = p->parser;
    }

    if (p->extradata_size > 0 && p->extradata) {
        ret = h265d_extradata(s);
        if (ret < 0)
            return ret;
    }

    mpp_buf_slot_setup(s->slots, 25);

    s->hw_info = parser_cfg->hw_info;

    mpp_slots_set_prop(s->slots, SLOTS_WIDTH_ALIGN, mpp_align_64);

    s->cap_hw_h265_rps = s->hw_info->cap_hw_h265_rps;

#ifdef dump
    fp = fopen("/data/dump1.bin", "wb+");
#endif
    return 0;
}

MPP_RET h265d_deinit(void *ctx)
{
    H265dCtx *p = (H265dCtx *)ctx;

    h265d_parser_deinit(p->parser);
    h265d_spliter_deinit(p->spliter);

    return MPP_OK;
}

MPP_RET h265d_prepare(void *ctx, MppPacket pkt, HalDecTask *task)
{
    H265dCtx *p = (H265dCtx *)ctx;
    H265dPrs *s = (H265dPrs *)p->parser;
    H265dSpliter spl = p->spliter;
    RK_S64 pts = -1, dts = -1;
    RK_U8 *buf = NULL;
    void *pos = NULL;
    RK_S32 length = 0;
    RK_S32 orig_length = 0;
    MPP_RET ret = MPP_OK;

    task->valid = 0;
    s->eos = mpp_packet_get_eos(pkt);

    if (p->cfg->base.split_parse && NULL == spl) {
        h265d_spliter_init(&p->spliter);
        spl = p->spliter;
    }

    h265d_spliter_set_eos(spl, s->eos);

    buf = (RK_U8 *)mpp_packet_get_pos(pkt);
    pts = mpp_packet_get_pts(pkt);
    dts = mpp_packet_get_dts(pkt);
    h265d_dbg_pts("prepare get pts %lld", pts);
    length = (RK_S32)mpp_packet_get_length(pkt);
    orig_length = length;

    if (mpp_packet_get_flag(pkt) & MPP_PACKET_FLAG_EXTRA_DATA) {
        p->extradata_size = length;
        p->extradata = buf;
        s->extra_has_frame = 0;
        s->task = NULL;
        h265d_extradata(s);
        if (!s->extra_has_frame) {
            pos = buf + length;
            mpp_packet_set_pos(pkt, pos);
            return MPP_OK;
        }
    }

    if (p->cfg->base.split_parse && !s->is_nalff) {
        RK_S32 consume = 0;
        RK_U8 *split_out_buf = NULL;
        RK_S32 split_size = 0;

        consume = h265d_spliter_frame(spl, (const RK_U8**)&split_out_buf, &split_size,
                                      (const RK_U8*)buf, length, pts, dts);
        pos = buf + consume;
        mpp_packet_set_pos(pkt, pos);
        if (split_size) {
            buf = split_out_buf;
            length = split_size;
            s->pts = h265d_spliter_get_pts(spl);
            s->dts = h265d_spliter_get_dts(spl);
            s->eos = (s->eos && (mpp_packet_get_length(pkt) < 4)) ? 1 : 0;
            h265d_dbg_pts("split frame get pts %lld", s->pts);
        } else {
            return MPP_FAIL_SPLIT_FRAME;
        }
    } else {
        pos = buf + length;
        s->pts = pts;
        s->dts = dts;
        mpp_packet_set_pos(pkt, pos);
        if (s->eos && !length) {
            task->valid = 0;
            task->flags.eos = 1;
            h265d_flush(ctx);
            return ret;
        }
    }
#ifdef dump
    if (s->nb_frame < 10 && fp != NULL) {
        fwrite(buf, 1, length, fp);
    }
#endif
    ret = (MPP_RET)h265d_split_nal(s, buf, length);

    if (MPP_OK == ret) {
        if (s->is_nalff) {
            RK_S32 consumed = s->consumed_bytes;

            if (consumed > orig_length || consumed < 0)
                consumed = orig_length;
            if (consumed == 0 && orig_length > 0) {
                mpp_loge_f("skip %d bytes with no progress\n", orig_length);
                consumed = orig_length;
            }
            pos = buf + consumed;
            mpp_packet_set_pos(pkt, pos);
        }

        if (MPP_OK == h265d_syntax_fill_slice(s->ctx, task->input)) {
            task->valid = 1;
            task->input_packet = s->input_packet;
        }
    }
    return ret;

}

MPP_RET h265d_parse(void *ctx, HalDecTask *task)
{
    H265dCtx *p = (H265dCtx *)ctx;
    H265dPrs *s = p->parser;
    MPP_RET ret;

    task->valid = 0;
    s->task = task;
    s->ref = NULL;
    ret = h265d_nal_units(s);
    if (ret < 0) {
        if (ret ==  MPP_ERR_STREAM) {
            mpp_log("current stream is no right skip it %p\n", s->ref);
            ret = 0;
        }
        task->flags.parse_err = 1;
    }
    if (s->ref) {
        if (!task->flags.parse_err)
            h265d_parser2_syntax(p);

        s->task->syntax.data = s->hal_pic_private;
        s->task->syntax.number = 1;
        s->task->valid = 1;
        s->ps_need_upate = 0;
        s->rps_need_upate = 0;
    }
    if (s->eos) {
        h265d_flush(ctx);
        s->task->flags.eos = 1;
    }
    s->nb_frame++;
    if (s->is_decoded) {
        h265d_dbg_global("frame %d decoded\n", s->poc);
        s->is_decoded = 0;
    }
    h265d_dpb_output(ctx, 0);
    return MPP_OK;
}

MPP_RET h265d_reset(void *ctx)
{
    H265dCtx *p = (H265dCtx *)ctx;
    H265dPrs *s = (H265dPrs *)p->parser;
    RK_S32 ret = 0;
    RK_U32 i;

    do {
        ret = h265d_dpb_output(ctx, 1);
    } while (ret);

    h265d_dpb_flush(s);

    h265d_spliter_reset(p->spliter);

    /* Clear RPS pointers to prevent dangling references */
    for (i = 0; i < 5; i++) {
        s->rps[i].nb_refs = 0;
        memset(s->rps[i].ref, 0, sizeof(s->rps[i].ref));
        memset(s->rps[i].list, 0, sizeof(s->rps[i].list));
    }

    s->max_ra = INT_MAX;
    s->eos = 0;
    s->first_i_fast_play = 0;
    return MPP_OK;
}

MPP_RET h265d_flush(void *ctx)
{
    RK_S32 ret = 0;
    do {
        ret = h265d_dpb_output(ctx, 1);
    } while (ret);
    return MPP_OK;
}

MPP_RET h265d_sync(void *ctx, RK_S32 frame_id)
{
    H265dCtx *p = (H265dCtx *)ctx;

    if (p && p->spliter)
        h265d_spliter_sync(p->spliter, frame_id);

    return MPP_OK;
}

MPP_RET h265d_control(void *ctx, MpiCmd cmd, void *param)
{
    (void) ctx;
    (void) cmd;
    (void) param;
    return MPP_OK;
}

MPP_RET h265d_callback(void *ctx, void *err_info)
{
    H265dCtx *p = (H265dCtx *)ctx;
    HalDecTask *task_dec = (HalDecTask *)err_info;
    H265dPrs *s = (H265dPrs *)p->parser;

    if (!p->cfg->base.disable_error) {
        MppFrame frame = NULL;
        RK_U32 i = 0;

        if (s->first_nal_type >= 16 && s->first_nal_type <= 23) {
            mpp_log("IS_IRAP frame found error");
            s->max_ra = INT_MAX;
        }
        mpp_buf_slot_get_prop(s->slots, task_dec->output, SLOT_FRAME_PTR, &frame);
        mpp_frame_set_errinfo(frame, MPP_FRAME_ERR_UNKNOW);
        h265d_dbg_ref("set decoded frame error, poc %d, slot %d\n",
                      mpp_frame_get_poc(frame), task_dec->output);

        for (i = 0; i < MPP_ARRAY_ELEMS(s->dpb); i++) {
            H265dFrame *frm = &s->dpb[i];

            if (frm->slot_index == task_dec->output) {
                frm->err_status.has_err = 1;
                h265d_dbg_ref("mark dpb idx %d poc %d slot_idx %d has_err %d err_info %d discard %d\n",
                              i, mpp_frame_get_poc(frm->frame), frm->slot_index, frm->err_status.has_err,
                              mpp_frame_get_errinfo(frm->frame), mpp_frame_get_discard(frm->frame));
            }
        }
    }

    if (!task_dec->flags.parse_err) {
        s->ps_need_upate = 0;
        s->rps_need_upate = 0;
    }

    (void) err_info;

    return MPP_OK;
}

const ParserApi mpp_h265d = {
    .name = "h265d_parse",
    .coding = MPP_VIDEO_CodingHEVC,
    .ctx_size = sizeof(H265dCtx),
    .flag = 0,
    .init = h265d_init,
    .deinit = h265d_deinit,
    .prepare = h265d_prepare,
    .parse = h265d_parse,
    .reset = h265d_reset,
    .flush = h265d_flush,
    .control = h265d_control,
    .callback = h265d_callback,
    .sync = h265d_sync,
};

MPP_PARSER_API_REGISTER(mpp_h265d);
