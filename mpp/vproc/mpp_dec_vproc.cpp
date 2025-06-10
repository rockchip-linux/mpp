/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_dec_vproc"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_dec_impl.h"

#include "mpp_frame_impl.h"
#include "mpp_dec_vproc.h"
#include "iep_api.h"
#include "iep2_api.h"

#define DUMP_FILE 1

#define dec_vproc_dbg(flag, fmt, ...) \
    do { \
        _mpp_dbg(vproc_debug, flag, fmt, ## __VA_ARGS__); \
    } while (0)

#define vproc_dbg_f(flag, fmt, ...) \
    do { \
        _mpp_dbg_f(vproc_debug, flag, fmt, ## __VA_ARGS__); \
    } while (0)

#define VPROC_DBG_FUNCTION      (0x00000001)
#define VPROC_DBG_STATUS        (0x00000002)
#define VPROC_DBG_RESET         (0x00000004)
#define VPROC_DBG_DUMP_IN       (0x00000010)
#define VPROC_DBG_DUMP_OUT      (0x00000020)
#define VPROC_DBG_IN            (0x00000040)
#define VPROC_DBG_OUT           (0x00000080)

#define vproc_dbg_func(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_FUNCTION, fmt, ## __VA_ARGS__);
#define vproc_dbg_status(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_STATUS, fmt, ## __VA_ARGS__);
#define vproc_dbg_reset(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_RESET, fmt, ## __VA_ARGS__);
#define vproc_dbg_in(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_IN, fmt, ## __VA_ARGS__);
#define vproc_dbg_out(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_OUT, fmt, ## __VA_ARGS__);

RK_U32 vproc_debug = 0;

typedef union VprocTaskStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_rdy      : 1;
        RK_U32      buf_rdy   : 1;
    };
} VprocTaskStatus;

typedef union VprocTaskWait_u {
    RK_U32          val;
    struct {
        RK_U32      task_in      : 1;
        RK_U32      task_buf_in  : 1;
    };
} VprocTaskWait;

typedef struct MppDecVprocCtxImpl_t {
    Mpp                 *mpp;
    HalTaskGroup        task_group;
    MppBufSlots         slots;

    MppThread           *thd;
    RK_U32              reset;
    sem_t               reset_sem;

    IepCtx              iep_ctx;
    iep_com_ctx         *com_ctx;
    IepCmdParamDeiCfg   dei_cfg;
    iep2_api_info       dei_info;

    VprocTaskStatus     task_status;
    VprocTaskWait       task_wait;

    // slot index for previous frame and current frame
    RK_S32              prev_idx0;
    MppFrame            prev_frm0;
    RK_S32              prev_idx1;
    MppFrame            prev_frm1;
    enum IEP2_FF_MODE   pre_ff_mode;
    RK_U32              pd_mode;
    MppBuffer           out_buf0;
    MppBuffer           out_buf1;
    MppVprocMode        vproc_mode;

    MPP_RET (*set_dei)(MppDecVprocCtx *vproc_ctx, MppFrame frm);
    MPP_RET (*start_dei)(MppDecVprocCtx *vproc_ctx, RK_U32 mode);
    MPP_RET (*update_ref)(MppDecVprocCtx *vproc_ctx, MppFrame frm, RK_U32 index);
} MppDecVprocCtxImpl;

static void dec_vproc_put_frame(Mpp *mpp, MppFrame frame, MppBuffer buf, RK_S64 pts, RK_U32 err)
{
    MppList *list = mpp->mFrmOut;
    MppFrame out = NULL;
    MppFrameImpl *impl = NULL;

    mpp_frame_init(&out);
    mpp_frame_copy(out, frame);
    impl = (MppFrameImpl *)out;
    if (pts >= 0)
        impl->pts = pts;
    if (buf)
        impl->buffer = buf;

    impl->errinfo |= err;

    mpp_mutex_cond_lock(&list->cond_lock);
    mpp_list_add_at_tail(list, &out, sizeof(out));

    mpp->mFramePutCount++;
    vproc_dbg_out("Output frame[%d]:poc %d, pts %lld, err 0x%x, dis %x, buf ptr %p\n",
                  mpp->mFramePutCount, mpp_frame_get_poc(out), mpp_frame_get_pts(out),
                  mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame),
                  mpp_buffer_get_ptr(impl->buffer));
    mpp_mutex_cond_signal(&list->cond_lock);
    mpp_mutex_cond_unlock(&list->cond_lock);

    if (mpp->mDec)
        mpp_dec_callback(mpp->mDec, MPP_DEC_EVENT_ON_FRM_READY, out);
}

static void dec_vproc_clr_prev0(MppDecVprocCtxImpl *ctx)
{
    if (vproc_debug & VPROC_DBG_STATUS) {
        if (ctx->prev_frm0) {
            MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm0);
            RK_S32 fd = (buf) ? (mpp_buffer_get_fd(buf)) : (-1);
            mpp_log("clearing prev index %d frm %p fd %d, poc%d\n", ctx->prev_idx0,
                    ctx->prev_frm0, fd, mpp_frame_get_poc(ctx->prev_frm0));
        } else
            mpp_log("clearing nothing\n");
    }

    if (ctx->prev_frm0) {
        MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm0);
        if (buf)
            mpp_buffer_put(buf);
    }
    if (ctx->prev_idx0 >= 0)
        mpp_buf_slot_clr_flag(ctx->slots, ctx->prev_idx0, SLOT_QUEUE_USE);

    ctx->prev_idx0 = -1;
    ctx->prev_frm0 = NULL;
}

static void dec_vproc_clr_prev1(MppDecVprocCtxImpl *ctx)
{
    if (vproc_debug & VPROC_DBG_STATUS) {
        if (ctx->prev_frm1) {
            MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm1);
            RK_S32 fd = (buf) ? (mpp_buffer_get_fd(buf)) : (-1);
            mpp_log("clearing prev index %d frm %p fd %d, poc %d\n", ctx->prev_idx1,
                    ctx->prev_frm1, fd, mpp_frame_get_poc(ctx->prev_frm1));
        } else
            mpp_log("clearing nothing\n");
    }
    if (ctx->prev_frm1) {
        MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm1);
        if (buf)
            mpp_buffer_put(buf);
    }
    if (ctx->prev_idx1 >= 0)
        mpp_buf_slot_clr_flag(ctx->slots, ctx->prev_idx1, SLOT_QUEUE_USE);

    ctx->prev_idx1 = -1;
    ctx->prev_frm1 = NULL;
}

static void dec_vproc_clr_prev(MppDecVprocCtxImpl *ctx)
{
    dec_vproc_clr_prev0(ctx);
    dec_vproc_clr_prev1(ctx);
    if (ctx->out_buf0) {
        mpp_buffer_put(ctx->out_buf0);
        ctx->out_buf0 = NULL;
    }
    if (ctx->out_buf1) {
        mpp_buffer_put(ctx->out_buf1);
        ctx->out_buf1 = NULL;
    }
}

static void dec_vproc_set_img_fmt(IepImg *img, MppFrame frm)
{
    memset(img, 0, sizeof(*img));
    img->act_w = mpp_frame_get_width(frm);
    img->act_h = mpp_frame_get_height(frm);
    img->vir_w = mpp_frame_get_hor_stride(frm);
    img->vir_h = mpp_frame_get_ver_stride(frm);
    img->format = IEP_FORMAT_YCbCr_420_SP;
}

static void dec_vproc_set_img(MppDecVprocCtxImpl *ctx, IepImg *img, RK_S32 fd, IepCmd cmd)
{
    RK_S32 y_size = img->vir_w * img->vir_h;
    img->mem_addr = fd;
    img->uv_addr = fd + (y_size << 10);
    img->v_addr = fd + ((y_size + y_size / 4) << 10);

    MPP_RET ret = ctx->com_ctx->ops->control(ctx->iep_ctx, cmd, img);
    if (ret)
        mpp_log_f("control %08x failed %d\n", cmd, ret);
}

static MPP_RET dec_vproc_start_dei_v1(MppDecVprocCtx *vproc_ctx, RK_U32 mode)
{
    MPP_RET ret = MPP_OK;
    MppDecVprocCtxImpl *impl = (MppDecVprocCtxImpl *) vproc_ctx;
    impl->dei_cfg.dei_field_order = (mode & MPP_FRAME_FLAG_TOP_FIRST) ?
                                    (IEP_DEI_FLD_ORDER_TOP_FIRST) :
                                    (IEP_DEI_FLD_ORDER_BOT_FIRST);
    ret = impl->com_ctx->ops->control(impl->iep_ctx,
                                      IEP_CMD_SET_DEI_CFG, &impl->dei_cfg);
    if (ret)
        mpp_err_f("IEP_CMD_SET_DEI_CFG failed %d\n", ret);

    ret = impl->com_ctx->ops->control(impl->iep_ctx, IEP_CMD_RUN_SYNC, &impl->dei_info);
    if (ret)
        mpp_err_f("IEP_CMD_RUN_SYNC failed %d\n", ret);

    return ret;
}

static MPP_RET dec_vproc_start_dei_v2(MppDecVprocCtx *vproc_ctx, RK_U32 mode)
{
    MPP_RET ret = MPP_OK;
    MppDecVprocCtxImpl *impl = (MppDecVprocCtxImpl *) vproc_ctx;
    (void)mode;

    ret = impl->com_ctx->ops->control(impl->iep_ctx, IEP_CMD_RUN_SYNC, &impl->dei_info);
    if (ret)
        mpp_log_f("IEP_CMD_RUN_SYNC failed %d\n", ret);

    return ret;
}

static MPP_RET dec_vproc_set_dei_v1(MppDecVprocCtx *vproc_ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    IepImg img;
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)vproc_ctx;

    Mpp *mpp = ctx->mpp;
    RK_U32 mode = mpp_frame_get_mode(frm);
    MppBuffer buf = mpp_frame_get_buffer(frm);
    MppBuffer dst0 = ctx->out_buf0;
    MppBuffer dst1 = ctx->out_buf1;
    int fd = -1;
    RK_U32 frame_err = 0;

    // setup source IepImg
    dec_vproc_set_img_fmt(&img, frm);

    ret = ctx->com_ctx->ops->control(ctx->iep_ctx, IEP_CMD_INIT, NULL);
    if (ret)
        mpp_log_f("IEP_CMD_INIT failed %d\n", ret);

    IepCap_t *cap = NULL;
    ret = ctx->com_ctx->ops->control(ctx->iep_ctx, IEP_CMD_QUERY_CAP, &cap);
    if (ret)
        mpp_log_f("IEP_CMD_QUERY_CAP failed %d\n", ret);

    // setup destination IepImg with new buffer
    // NOTE: when deinterlace is enabled parser thread will reserve
    //       more buffer than normal case
    if (ctx->prev_frm0 && cap && cap->i4_deinterlace_supported) {
        // 4 in 2 out case
        vproc_dbg_status("4 field in and 2 frame out\n");
        RK_S64 prev_pts = mpp_frame_get_pts(ctx->prev_frm0);
        RK_S64 curr_pts = mpp_frame_get_pts(frm);
        RK_S64 first_pts = (prev_pts + curr_pts) / 2;

        buf = mpp_frame_get_buffer(ctx->prev_frm0);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);
        frame_err = mpp_frame_get_errinfo(ctx->prev_frm0) ||
                    mpp_frame_get_discard(ctx->prev_frm0);
        // setup dst 0
        mpp_assert(dst0);
        fd = mpp_buffer_get_fd(dst0);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);

        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);
        frame_err |= mpp_frame_get_errinfo(frm) ||
                     mpp_frame_get_discard(frm);
        // setup dst 1
        mpp_assert(dst1);
        fd = mpp_buffer_get_fd(dst1);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_DST1);

        ctx->dei_cfg.dei_mode = IEP_DEI_MODE_I4O2;

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I4O2;
        mpp_frame_set_mode(frm, mode);

        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);

        // NOTE: we need to process pts here
        if (mode & MPP_FRAME_FLAG_TOP_FIRST) {
            dec_vproc_put_frame(mpp, frm, dst0, first_pts, frame_err);
            dec_vproc_put_frame(mpp, frm, dst1, curr_pts, frame_err);
        } else {
            dec_vproc_put_frame(mpp, frm, dst1, first_pts, frame_err);
            dec_vproc_put_frame(mpp, frm, dst0, curr_pts, frame_err);
        }
        ctx->out_buf0 = NULL;
        ctx->out_buf1 = NULL;
    } else {
        // 2 in 1 out case
        vproc_dbg_status("2 field in and 1 frame out\n");
        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);
        frame_err = mpp_frame_get_errinfo(frm) ||
                    mpp_frame_get_discard(frm);

        // setup dst 0
        mpp_assert(dst0);
        fd = mpp_buffer_get_fd(dst0);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);

        ctx->dei_cfg.dei_mode = IEP_DEI_MODE_I2O1;
        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I2O1;
        mpp_frame_set_mode(frm, mode);

        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);
        dec_vproc_put_frame(mpp, frm, dst0, -1, frame_err);
        ctx->out_buf0 = NULL;
    }

    return ret;
}

#if DUMP_FILE
static void dump_mppbuffer(MppBuffer buf, const char *fname, int stride, int height)
{
    char title[256];
    void *ptr = mpp_buffer_get_ptr(buf);
    sprintf(title, "%s.%dx%d.yuv", fname, stride, height);
    FILE *dump = fopen(title, "ab+");

    if (dump) {
        fwrite(ptr, 1, stride * height * 3 / 2, dump);
        fclose(dump);
    }
}
#else
#define dump_mppbuffer(...)
#endif

static MPP_RET dec_vproc_config_dei_v2(MppDecVprocCtxImpl *ctx, MppFrame frm,
                                       enum IEP2_DIL_MODE dil_mode)
{
    MPP_RET ret = MPP_OK;
    MppBuffer buf = NULL;
    RK_S32 fd = -1;
    IepImg img;
    struct iep2_api_params params;
    iep_com_ops *ops = ctx->com_ctx->ops;

    if (!frm) {
        mpp_err("found NULL pointer frm\n");
        ret = MPP_ERR_NULL_PTR;
        return ret;
    }

    /* default alloc 2 out buffer for IEP */
    if (!ctx->out_buf0 || !ctx->out_buf1) {
        mpp_err("found NULL pointer out_buf0 %p out_buf1 %p\n", ctx->out_buf0, ctx->out_buf1);
        ret = MPP_ERR_NULL_PTR;
        return ret;
    }

    // setup source IepImg
    dec_vproc_set_img_fmt(&img, frm);

    if (vproc_debug & VPROC_DBG_DUMP_IN)
        dump_mppbuffer(buf, "/data/dump/dump_in.yuv", img.vir_w, img.vir_h);

    vproc_dbg_status("set dil_mode %d\n", dil_mode);
    // TODO: check the input frame
    switch (dil_mode) {
    case IEP2_DIL_MODE_I5O2:
    case IEP2_DIL_MODE_I5O1T:
    case IEP2_DIL_MODE_I5O1B:
    case IEP2_DIL_MODE_DECT: {
        // require 3 frames
        buf = mpp_frame_get_buffer(ctx->prev_frm0);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);

        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);

        buf = mpp_frame_get_buffer(ctx->prev_frm1);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC2);
    } break;
    case IEP2_DIL_MODE_PD: {
        // require 2 frame
        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);

        if (ctx->prev_frm0) {
            buf = mpp_frame_get_buffer(ctx->prev_frm0);
            fd = mpp_buffer_get_fd(buf);
        } else if (ctx->prev_frm1) {
            buf = mpp_frame_get_buffer(ctx->prev_frm1);
            fd = mpp_buffer_get_fd(buf);
        }

        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC2);
    } break;
    case IEP2_DIL_MODE_I2O2:
    case IEP2_DIL_MODE_I1O1T:
    case IEP2_DIL_MODE_I1O1B:
    case IEP2_DIL_MODE_BYPASS:
    default: {
        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC2);
    } break;
    }

    // setup output
    fd = mpp_buffer_get_fd(ctx->out_buf0);
    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);
    fd = mpp_buffer_get_fd(ctx->out_buf1);
    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_DST1);

    memset(&params, 0, sizeof(params));
    params.ptype = IEP2_PARAM_TYPE_MODE;
    params.param.mode.dil_mode = dil_mode;
    params.param.mode.out_mode = IEP2_OUT_MODE_LINE;
    {
        RK_U32 mode = mpp_frame_get_mode(frm);
        RK_U32 fo_from_syntax = (mode & MPP_FRAME_FLAG_TOP_FIRST) ? 1 : 0;

        /* refer to syntax */
        if ((mode & MPP_FRAME_FLAG_TOP_FIRST) && (mode & MPP_FRAME_FLAG_BOT_FIRST))
            params.param.mode.dil_order = IEP2_FIELD_ORDER_UND;
        else if (fo_from_syntax)
            params.param.mode.dil_order = IEP2_FIELD_ORDER_TFF;
        else
            params.param.mode.dil_order = IEP2_FIELD_ORDER_BFF;

        /* refer to IEP */
        if (ctx->pre_ff_mode == IEP2_FF_MODE_FIELD) {
            RK_U32 fo_from_iep = (ctx->dei_info.dil_order == IEP2_FIELD_ORDER_UND) ?
                                 fo_from_syntax : (ctx->dei_info.dil_order == IEP2_FIELD_ORDER_TFF);
            RK_U32 is_tff = 0;

            if (fo_from_iep != fo_from_syntax) {
                if (ctx->dei_info.dil_order_confidence_ratio > 30)
                    is_tff = fo_from_iep;
                else
                    is_tff = fo_from_syntax;
            } else {
                is_tff = fo_from_syntax;
            }

            if (is_tff)
                params.param.mode.dil_order = IEP2_FIELD_ORDER_TFF;
            else
                params.param.mode.dil_order = IEP2_FIELD_ORDER_BFF;

            vproc_dbg_status("Config field order: is TFF %d, syn %d vs iep %d\n",
                             is_tff, fo_from_syntax, fo_from_iep);
        }
    }
    ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

    memset(&params, 0, sizeof(params));
    params.ptype = IEP2_PARAM_TYPE_COM;
    params.param.com.sfmt = IEP2_FMT_YUV420;
    params.param.com.dfmt = IEP2_FMT_YUV420;
    params.param.com.sswap = IEP2_YUV_SWAP_SP_UV;
    params.param.com.dswap = IEP2_YUV_SWAP_SP_UV;
    params.param.com.width = img.act_w;
    params.param.com.height = img.act_h;
    params.param.com.hor_stride = img.vir_w;//img.act_w;
    ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

    return MPP_OK;
}

MPP_RET dec_vproc_output_dei_v2(MppDecVprocCtxImpl *ctx, MppFrame frm, RK_U32 is_frm)
{
    MPP_RET ret = MPP_OK;
    Mpp *mpp = ctx->mpp;
    RK_U32 hor_stride = mpp_frame_get_hor_stride(frm);
    RK_U32 ver_stride = mpp_frame_get_ver_stride(frm);
    RK_U32 mode = mpp_frame_get_mode(frm);
    RK_U32 dei_mode = mode & MPP_FRAME_FLAG_IEP_DEI_MASK;
    MppBuffer dst0 = ctx->out_buf0;
    MppBuffer dst1 = ctx->out_buf1;
    RK_U32 frame_err = 0;

    vproc_dbg_status("is_frm %d frm %p, dei_mode %d field0 %p field1 %p",
                     is_frm, ctx->prev_frm1, dei_mode, dst0, dst1);
    if (is_frm) {
        if (ctx->prev_frm1) {
            vproc_dbg_out("output frame prev1 poc %d\n", mpp_frame_get_poc(ctx->prev_frm1));
            dec_vproc_put_frame(mpp,  ctx->prev_frm1, NULL, -1, 0);
            if (ctx->prev_idx1 >= 0)
                mpp_buf_slot_clr_flag(ctx->slots, ctx->prev_idx1, SLOT_QUEUE_USE);
            ctx->prev_idx1 = -1;
            ctx->prev_frm1 = NULL;
        }
        return ret;
    }

    switch (dei_mode) {
    case MPP_FRAME_FLAG_IEP_DEI_I4O2: {
        RK_S64 prev_pts = mpp_frame_get_pts(ctx->prev_frm1);
        RK_S64 curr_pts = mpp_frame_get_pts(ctx->prev_frm0);
        RK_S64 first_pts = (prev_pts + curr_pts) / 2;

        frame_err |= mpp_frame_get_errinfo(ctx->prev_frm0) + mpp_frame_get_discard(ctx->prev_frm0);
        frame_err |= mpp_frame_get_errinfo(ctx->prev_frm1) + mpp_frame_get_discard(ctx->prev_frm1);

        if (ctx->pd_mode) {
            // NOTE: we need to process pts here if PD mode
            if (ctx->dei_info.pd_flag != PD_COMP_FLAG_NON &&
                ctx->dei_info.pd_types != PD_TYPES_UNKNOWN) {
                vproc_dbg_out("output at pd mode, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, dst0, first_pts, frame_err);
                if (vproc_debug & VPROC_DBG_DUMP_OUT)
                    dump_mppbuffer(dst0, "/data/dump/dump_output.yuv", hor_stride, ver_stride);
                ctx->out_buf0 = NULL;
            }
        } else {
            RK_U32 fo_from_syntax = (mode & MPP_FRAME_FLAG_TOP_FIRST) ? 1 : 0;
            RK_U32 fo_from_iep = (ctx->dei_info.dil_order == IEP2_FIELD_ORDER_UND) ?
                                 fo_from_syntax : (ctx->dei_info.dil_order == IEP2_FIELD_ORDER_TFF);
            RK_U32 is_tff = 0;

            if (fo_from_iep != fo_from_syntax) {
                if (ctx->dei_info.dil_order_confidence_ratio > 30)
                    is_tff = fo_from_iep;
                else
                    is_tff = fo_from_syntax;
            } else {
                is_tff = fo_from_syntax;
            }

            vproc_dbg_status("Output field order: is TFF %d, syn %d vs iep %d\n",
                             is_tff, fo_from_syntax, fo_from_iep);

            if (is_tff) {
                vproc_dbg_out("output at I4O2 for tff, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, dst0, first_pts, frame_err);
                if (vproc_debug & VPROC_DBG_DUMP_OUT)
                    dump_mppbuffer(dst0, "/data/dump/dump_output.yuv", hor_stride, ver_stride);
                vproc_dbg_out("output at I4O2 for bff, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, dst1, curr_pts, frame_err);
                if (vproc_debug & VPROC_DBG_DUMP_OUT)
                    dump_mppbuffer(dst1, "/data/dump/dump_output.yuv", hor_stride, ver_stride);
            } else {
                vproc_dbg_out("output at I4O2 for bff, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, dst1, first_pts, frame_err);
                if (vproc_debug & VPROC_DBG_DUMP_OUT)
                    dump_mppbuffer(dst1, "/data/dump/dump_output.yuv", hor_stride, mpp_frame_get_height(frm));
                vproc_dbg_out("output at I4O2 for tff, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, dst0, curr_pts, frame_err);
                if (vproc_debug & VPROC_DBG_DUMP_OUT)
                    dump_mppbuffer(dst0, "/data/dump/dump_output.yuv", hor_stride, mpp_frame_get_height(frm));
            }

            ctx->out_buf0 = NULL;
            ctx->out_buf1 = NULL;
        }
    } break;
    case MPP_FRAME_FLAG_IEP_DEI_I2O1:
    case MPP_FRAME_FLAG_IEP_DEI_I4O1: {
        vproc_dbg_out("output at I2O1, frame poc %d\n", mpp_frame_get_poc(frm));
        dec_vproc_put_frame(mpp, frm, dst0, -1, frame_err);
        if (vproc_debug & VPROC_DBG_DUMP_OUT)
            dump_mppbuffer(dst0, "/data/dump/dump_output.yuv", hor_stride, mpp_frame_get_height(frm));
        ctx->out_buf0 = NULL;
    }
    default:
        break;
    }

    return ret;
}

static MPP_RET dec_vproc_dei_v2_deinterlace(MppDecVprocCtxImpl *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    RK_U32 mode = mpp_frame_get_mode(frm);
    enum IEP2_DIL_MODE dil_mode = IEP2_DIL_MODE_DISABLE;

    /* refer to syntax */
    if (((mode & MPP_FRAME_FLAG_PAIRED_FIELD) == MPP_FRAME_FLAG_FRAME) &&
        !(mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK))
        return dec_vproc_output_dei_v2(ctx, frm, 1);

    if (ctx->prev_frm1 && ctx->prev_frm0) {
        // 5 in 2 out case
        vproc_dbg_status("5 field in and 2 frame out\n");

        if (!ctx->pd_mode)
            dil_mode = IEP2_DIL_MODE_I5O2;
        else
            dil_mode = IEP2_DIL_MODE_PD;

        dec_vproc_config_dei_v2(ctx, frm, dil_mode);

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I4O2;
        mpp_frame_set_mode(frm, mode);
        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);

        dec_vproc_output_dei_v2(ctx, frm, 0);

        if (ctx->dei_info.pd_types == PD_TYPES_UNKNOWN)
            ctx->pd_mode = 0;
        else
            ctx->pd_mode = 1;

    } else if (ctx->prev_frm0 && ! ctx->prev_frm1) {
        vproc_dbg_status("Wait for next frame to turn into I5O2");

        if (ctx->out_buf0) {
            mpp_buffer_put(ctx->out_buf0);
            ctx->out_buf0 = NULL;
        }

        if (ctx->out_buf1) {
            mpp_buffer_put(ctx->out_buf1);
            ctx->out_buf1 = NULL;
        }
    } else {
        // 2 in 1 out case
        vproc_dbg_status("2 field in and 1 frame out\n");
        dil_mode = IEP2_DIL_MODE_I1O1T;

        dec_vproc_config_dei_v2(ctx, frm, dil_mode);

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I2O1;
        mpp_frame_set_mode(frm, mode);
        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);

        dec_vproc_output_dei_v2(ctx, frm, 0);
    }

    return ret;
}

static MPP_RET dec_vproc_dei_v2_detection(MppDecVprocCtxImpl *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    RK_U32 mode = mpp_frame_get_mode(frm);
    enum IEP2_DIL_MODE dil_mode = IEP2_DIL_MODE_DISABLE;
    RK_U32 is_frame = 0;

    if (ctx->pre_ff_mode == IEP2_FF_MODE_UND) {
        if (((mode & MPP_FRAME_FLAG_PAIRED_FIELD) == MPP_FRAME_FLAG_FRAME) &&
            !(mode & MPP_FRAME_FLAG_FIELD_ORDER_MASK))
            is_frame = 1;
        else
            is_frame = 0;
    } else {
        is_frame = ctx->pre_ff_mode == IEP2_FF_MODE_FRAME ? 1 : 0;
    }

    /* TODO: diff detection strategy */
    if (ctx->prev_frm1 && ctx->prev_frm0) {
        // 5 in 2 out case
        vproc_dbg_status("5 field in and 2 frame out\n");

        if (!ctx->pd_mode)
            dil_mode = IEP2_DIL_MODE_I5O2;
        else
            dil_mode = IEP2_DIL_MODE_PD;

        dec_vproc_config_dei_v2(ctx, frm, dil_mode);

        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);

        if (ctx->dei_info.frm_mode == IEP2_FF_MODE_FRAME) {
            is_frame = 1;
        } else if (ctx->dei_info.pd_types == PD_TYPES_UNKNOWN) {
            ctx->pd_mode = 0;
            is_frame = 0;
        } else {
            ctx->pd_mode = 1;
            is_frame = 0;
        }

        if (!is_frame) {
            mode = mode | MPP_FRAME_FLAG_IEP_DEI_I4O2;
            mpp_frame_set_mode(frm, mode);
        }
    } else if (ctx->prev_frm0 && ! ctx->prev_frm1) {
        vproc_dbg_status("Wait for next frame to turn into I5O2");

        if (ctx->out_buf0) {
            mpp_buffer_put(ctx->out_buf0);
            ctx->out_buf0 = NULL;
        }

        if (ctx->out_buf1) {
            mpp_buffer_put(ctx->out_buf1);
            ctx->out_buf1 = NULL;
        }
    } else {
        // 2 in 1 out case
        vproc_dbg_status("2 field in and 1 frame out\n");
        dil_mode = IEP2_DIL_MODE_I1O1T;

        dec_vproc_config_dei_v2(ctx, frm, dil_mode);

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I2O1;
        mpp_frame_set_mode(frm, mode);
        // start hardware
        ctx->start_dei((MppDecVprocCtx *)ctx, mode);
    }

    ret = dec_vproc_output_dei_v2(ctx, frm, is_frame);
    ctx->pre_ff_mode = ctx->dei_info.frm_mode;

    return ret;
}

static MPP_RET dec_vproc_set_dei_v2(MppDecVprocCtx *vproc_ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)vproc_ctx;
    /*     RK_U32 mode = mpp_frame_get_mode(frm);
        enum IEP2_DIL_MODE dil_mode = IEP2_DIL_MODE_DISABLE; */
    MppVprocMode vproc_mode = ctx->vproc_mode;

    switch (vproc_mode) {
    case MPP_VPROC_MODE_DETECTION: {
        dec_vproc_dei_v2_detection(ctx, frm);
    } break;
    case MPP_VPROC_MODE_DEINTELACE: {
        dec_vproc_dei_v2_deinterlace(ctx, frm);
    } break;
    default: {
        mpp_err("warning: vproc mode unknown!\n");
        ret = MPP_NOK;
    } break;
    }

    return ret;
}

MPP_RET dec_vproc_update_ref_v1(MppDecVprocCtx *vproc_ctx, MppFrame frm, RK_U32 index)
{
    MPP_RET ret = MPP_OK;
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)vproc_ctx;
    dec_vproc_clr_prev0(ctx);
    ctx->prev_idx0 = index;
    ctx->prev_frm0 = frm;

    return ret;
}

MPP_RET dec_vproc_update_ref_v2(MppDecVprocCtx *vproc_ctx, MppFrame frm, RK_U32 index)
{
    MPP_RET ret = MPP_OK;
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)vproc_ctx;

    dec_vproc_clr_prev1(ctx);

    ctx->prev_idx1 = ctx->prev_idx0;
    ctx->prev_idx0 = index;
    ctx->prev_frm1 = ctx->prev_frm0;
    ctx->prev_frm0 = frm;
    return ret;
}

static MPP_RET dec_vproc_update_ref(MppDecVprocCtxImpl *ctx, MppFrame frm, RK_U32 index, RK_U32 eos)
{
    MPP_RET ret = MPP_OK;
    Mpp *mpp = ctx->mpp;

    ret = ctx->update_ref((MppDecVprocCtx *)ctx, frm, index);

    if (eos) {
        mpp_frame_init(&frm);
        mpp_frame_set_eos(frm, eos);
        vproc_dbg_out("output at update ref, frame poc %d\n", mpp_frame_get_poc(frm));
        dec_vproc_put_frame(mpp, frm, NULL, -1, 0);
        dec_vproc_clr_prev(ctx);
        mpp_frame_deinit(&frm);
    }
    return ret;
}

static void *dec_vproc_thread(void *data)
{
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)data;
    HalTaskGroup tasks = ctx->task_group;
    MppThread *thd = ctx->thd;
    Mpp *mpp = ctx->mpp;
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppBufSlots slots = dec->frame_slots;

    HalTaskHnd task = NULL;
    HalTaskInfo task_info;
    HalDecVprocTask *task_vproc = &task_info.dec_vproc;

    mpp_dbg_info("mpp_dec_post_proc_thread started\n");

    while (1) {
        MPP_RET ret = MPP_OK;

        mpp_thread_lock(thd, THREAD_WORK);
        if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd, THREAD_WORK)) {
            mpp_thread_unlock(thd, THREAD_WORK);
            break;
        }

        if (ctx->task_wait.val && !ctx->reset) {
            vproc_dbg_status("vproc thread wait %d", ctx->task_wait.val);
            mpp_thread_wait(thd, THREAD_WORK);
        }
        mpp_thread_unlock(thd, THREAD_WORK);

        if (!ctx->task_status.task_rdy) {
            if (hal_task_get_hnd(tasks, TASK_PROCESSING, &task)) {
                if (ctx->reset) {
                    /* reset only on all task finished */
                    vproc_dbg_reset("reset start\n");

                    dec_vproc_clr_prev(ctx);

                    mpp_thread_lock(thd, THREAD_CONTROL);
                    ctx->reset = 0;
                    mpp_thread_unlock(thd, THREAD_CONTROL);
                    sem_post(&ctx->reset_sem);
                    ctx->task_wait.val = 0;

                    vproc_dbg_reset("reset done\n");
                    continue;
                }

                ctx->task_wait.task_in = 1;
                continue;
            }
            ctx->task_status.task_rdy = 1;
            ctx->task_wait.task_in = 0;
        }

        if (task) {
            ret = hal_task_hnd_get_info(task, &task_info);

            mpp_assert(ret == MPP_OK);

            RK_S32 index = task_vproc->input;
            RK_U32 eos = task_vproc->flags.eos;
            RK_U32 change = task_vproc->flags.info_change;
            MppFrame frm = NULL;

            if (eos && index < 0) {
                vproc_dbg_status("eos signal\n");

                mpp_frame_init(&frm);
                mpp_frame_set_eos(frm, eos);
                vproc_dbg_out("output at eos, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, NULL, -1, 0);
                dec_vproc_clr_prev(ctx);
                mpp_frame_deinit(&frm);

                hal_task_hnd_set_status(task, TASK_IDLE);
                ctx->task_status.task_rdy = 0;
                continue;
            }

            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME_PTR, &frm);

            if (change) {
                vproc_dbg_status("info change\n");
                vproc_dbg_out("output at info change, frame poc %d\n", mpp_frame_get_poc(frm));
                dec_vproc_put_frame(mpp, frm, NULL, -1, 0);
                dec_vproc_clr_prev(ctx);

                if (ctx->com_ctx->ops->reset)
                    ctx->com_ctx->ops->reset(ctx->iep_ctx);

                hal_task_hnd_set_status(task, TASK_IDLE);
                ctx->task_status.task_rdy = 0;
                continue;
            }
            vproc_dbg_status("vproc get buf in");
            if (!ctx->task_status.buf_rdy && !ctx->reset) {
                MppBuffer buf = mpp_frame_get_buffer(frm);
                size_t buf_size = mpp_buffer_get_size(buf);
                if (!ctx->out_buf0) {
                    mpp_buffer_get(mpp->mFrameGroup, &ctx->out_buf0, buf_size);
                    vproc_dbg_out("get out buf0 ptr %p\n", mpp_buffer_get_ptr(ctx->out_buf0));
                    if (NULL == ctx->out_buf0) {
                        ctx->task_wait.task_buf_in = 1;
                        continue;
                    }
                }
                if (!ctx->out_buf1) {
                    mpp_buffer_get(mpp->mFrameGroup, &ctx->out_buf1, buf_size);
                    vproc_dbg_out("get out buf1 ptr %p\n", mpp_buffer_get_ptr(ctx->out_buf1));
                    if (NULL == ctx->out_buf1) {
                        ctx->task_wait.task_buf_in = 1;
                        continue;
                    }
                }
                ctx->task_status.buf_rdy = 1;
            }

            RK_S32 tmp = -1;
            mpp_buf_slot_dequeue(slots, &tmp, QUEUE_DEINTERLACE);
            mpp_assert(tmp == index);

            vproc_dbg_status("vproc get buf ready & start process ");
            if (!ctx->reset && ctx->iep_ctx) {
                vproc_dbg_in("processing frame poc %d, mode 0x%x, err %x vs %x, buf slot %x, ptr %p\n",
                             mpp_frame_get_poc(frm), mpp_frame_get_mode(frm), mpp_frame_get_errinfo(frm),
                             mpp_frame_get_discard(frm), index, mpp_buffer_get_ptr(mpp_frame_get_buffer(frm)));
                ctx->set_dei((MppDecVprocCtx *)ctx, frm);
            }

            dec_vproc_update_ref(ctx, frm, index, eos);
            hal_task_hnd_set_status(task, TASK_IDLE);
            ctx->task_status.val = 0;
            ctx->task_wait.val = 0;

            vproc_dbg_status("vproc task done");
        }
    }
    mpp_dbg_info("mpp_dec_post_proc_thread exited\n");

    return NULL;
}

MPP_RET dec_vproc_init(MppDecVprocCtx *ctx, MppDecVprocCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    if (NULL == ctx || NULL == cfg || NULL == cfg->mpp) {
        mpp_err_f("found NULL input ctx %p mpp %p\n", ctx, cfg->mpp);
        return MPP_ERR_NULL_PTR;
    }

    vproc_dbg_func("in\n");
    mpp_env_get_u32("vproc_debug", &vproc_debug, 0);

    *ctx = NULL;

    MppDecVprocCtxImpl *p = mpp_calloc(MppDecVprocCtxImpl, 1);
    if (NULL == p) {
        mpp_err_f("malloc failed\n");
        return MPP_ERR_MALLOC;
    }

    p->pre_ff_mode = IEP2_FF_MODE_UND;
    p->mpp = (Mpp *)cfg->mpp;
    p->slots = ((MppDecImpl *)p->mpp->mDec)->frame_slots;
    p->thd = mpp_thread_create(dec_vproc_thread, p, "mpp_dec_vproc");
    sem_init(&p->reset_sem, 0, 0);
    ret = hal_task_group_init(&p->task_group, TASK_BUTT, 4, sizeof(HalDecVprocTask));
    if (ret) {
        mpp_err_f("create task group failed\n");
        mpp_thread_destroy(p->thd);
        MPP_FREE(p);
        return MPP_ERR_MALLOC;
    }
    cfg->task_group = p->task_group;

    p->com_ctx = get_iep_ctx();
    if (!p->com_ctx) {
        mpp_err("failed to require context\n");
        mpp_thread_destroy(p->thd);

        if (p->task_group) {
            hal_task_group_deinit(p->task_group);
            p->task_group = NULL;
        }

        MPP_FREE(p);

        return MPP_ERR_MALLOC;
    }

    if (p->com_ctx->ver == 1) {
        p->start_dei = dec_vproc_start_dei_v1;
        p->set_dei = dec_vproc_set_dei_v1;
        p->update_ref = dec_vproc_update_ref_v1;
    } else {
        p->start_dei = dec_vproc_start_dei_v2;
        p->set_dei = dec_vproc_set_dei_v2;
        p->update_ref = dec_vproc_update_ref_v2;
    }

    ret = p->com_ctx->ops->init(&p->com_ctx->priv);
    p->iep_ctx = p->com_ctx->priv;
    if (!p->thd || ret) {
        mpp_err("failed to create context\n");
        if (p->thd) {
            mpp_thread_destroy(p->thd);
            p->thd = NULL;
        }

        if (p->iep_ctx)
            p->com_ctx->ops->deinit(p->iep_ctx);

        if (p->task_group) {
            hal_task_group_deinit(p->task_group);
            p->task_group = NULL;
        }

        put_iep_ctx(p->com_ctx);

        MPP_FREE(p);
    } else {
        p->dei_cfg.dei_mode = IEP_DEI_MODE_I2O1;
        p->dei_cfg.dei_field_order = IEP_DEI_FLD_ORDER_BOT_FIRST;
        /*
         * We need to turn off this switch to prevent some areas
         * of the video from flickering.
         */
        p->dei_cfg.dei_high_freq_en = 0;
        p->dei_cfg.dei_high_freq_fct = 64;
        p->dei_cfg.dei_ei_mode = 0;
        p->dei_cfg.dei_ei_smooth = 1;
        p->dei_cfg.dei_ei_sel = 0;
        p->dei_cfg.dei_ei_radius = 2;

        p->prev_idx0 = -1;
        p->prev_frm0 = NULL;
        p->prev_idx1 = -1;
        p->prev_frm1 = NULL;
    }

    *ctx = p;

    vproc_dbg_func("out\n");
    return ret;
}

RK_U32 dec_vproc_get_version(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return 0;
    }

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    return p->com_ctx->ver;
}

MPP_RET dec_vproc_set_mode(MppDecVprocCtx ctx, MppVprocMode mode)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    p->vproc_mode = mode;
    return MPP_OK;
}

MPP_RET dec_vproc_deinit(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_dbg_func("in\n");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd) {
        mpp_thread_destroy(p->thd);
        p->thd = NULL;
    }

    if (p->iep_ctx)
        p->com_ctx->ops->deinit(p->iep_ctx);

    if (p->task_group) {
        hal_task_group_deinit(p->task_group);
        p->task_group = NULL;
    }

    if (p->com_ctx) {
        put_iep_ctx(p->com_ctx);
        p->com_ctx = NULL;
    }

    sem_destroy(&p->reset_sem);
    mpp_free(p);

    vproc_dbg_func("out\n");
    return MPP_OK;
}

MPP_RET dec_vproc_start(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_dbg_func("in\n");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;

    if (p->thd)
        mpp_thread_start(p->thd);
    else
        mpp_err("failed to start dec vproc thread\n");

    vproc_dbg_func("out\n");
    return MPP_OK;
}

MPP_RET dec_vproc_stop(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_dbg_func("in\n");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;

    if (p->thd)
        mpp_thread_stop(p->thd);
    else
        mpp_err("failed to stop dec vproc thread\n");

    vproc_dbg_func("out\n");
    return MPP_OK;
}

MPP_RET dec_vproc_signal(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_dbg_func("in\n");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd) {
        mpp_thread_lock(p->thd, THREAD_WORK);
        mpp_thread_signal(p->thd, THREAD_WORK);
        mpp_thread_unlock(p->thd, THREAD_WORK);
    }

    vproc_dbg_func("out\n");
    return MPP_OK;
}

MPP_RET dec_vproc_reset(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_dbg_func("in\n");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd) {
        MppThread *thd = p->thd;

        vproc_dbg_reset("reset contorl start\n");
        // wait reset finished
        mpp_thread_lock(thd, THREAD_WORK);
        mpp_thread_lock(thd, THREAD_CONTROL);
        p->reset = 1;
        mpp_thread_signal(thd, THREAD_WORK);
        mpp_thread_unlock(thd, THREAD_CONTROL);
        mpp_thread_unlock(thd, THREAD_WORK);

        vproc_dbg_reset("reset contorl wait\n");
        sem_wait(&p->reset_sem);
        vproc_dbg_reset("reset contorl done\n");

        mpp_assert(p->reset == 0);
    }

    vproc_dbg_func("out\n");
    return MPP_OK;
}
