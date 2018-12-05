/*
 * Copyright 2018 Rockchip Electronics Co. LTD
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

#include "mpp_frame_impl.h"
#include "mpp_dec_vproc.h"
#include "iep_api.h"

#define vproc_dbg(flag, fmt, ...) \
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

#define vproc_dbg_func(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_FUNCTION, fmt, ## __VA_ARGS__);
#define vproc_dbg_status(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_STATUS, fmt, ## __VA_ARGS__);
#define vproc_dbg_reset(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_RESET, fmt, ## __VA_ARGS__);

RK_U32 vproc_debug = 0;

typedef struct MppDecVprocCtxImpl_t {
    Mpp                 *mpp;
    MppBufSlots         slots;

    MppThread           *thd;
    RK_U32              reset;
    RK_S32              count;

    IepCtx              iep_ctx;
    IepCmdParamDeiCfg   dei_cfg;

    // slot index for previous frame and current frame
    RK_S32              prev_idx;
    MppFrame            prev_frm;
} MppDecVprocCtxImpl;

static void dec_vproc_put_frame(Mpp *mpp, MppFrame frame, MppBuffer buf, RK_S64 pts)
{
    mpp_list *list = mpp->mFrames;
    MppFrame out = NULL;
    MppFrameImpl *impl = NULL;

    mpp_frame_init(&out);
    mpp_frame_copy(out, frame);
    impl = (MppFrameImpl *)out;
    if (pts >= 0)
        impl->pts = pts;
    if (buf)
        impl->buffer = buf;

    list->lock();
    list->add_at_tail(&out, sizeof(out));

    if (mpp_debug & MPP_DBG_PTS)
        mpp_log("output frame pts %lld\n", mpp_frame_get_pts(out));

    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
}

static void dec_vproc_clr_prev(MppDecVprocCtxImpl *ctx)
{
    if (vproc_debug & VPROC_DBG_STATUS) {
        if (ctx->prev_frm) {
            MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm);
            RK_S32 fd = (buf) ? (mpp_buffer_get_fd(buf)) : (-1);
            mpp_log("clearing prev index %d frm %p fd %d\n", ctx->prev_idx,
                    ctx->prev_frm, fd);
        } else
            mpp_log("clearing nothing\n");
    }

    if (ctx->prev_frm) {
        MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm);
        if (buf)
            mpp_buffer_put(buf);
    }
    if (ctx->prev_idx >= 0)
        mpp_buf_slot_clr_flag(ctx->slots, ctx->prev_idx, SLOT_QUEUE_USE);

    ctx->prev_idx = -1;
    ctx->prev_frm = NULL;
}

static void dec_vproc_reset_queue(MppDecVprocCtxImpl *ctx)
{
    MppThread *thd = ctx->thd;
    Mpp *mpp = ctx->mpp;
    MppDec *dec = mpp->mDec;
    MppBufSlots slots = dec->frame_slots;
    RK_S32 index = -1;
    MPP_RET ret = MPP_OK;

    vproc_dbg_reset("reset start\n");
    dec_vproc_clr_prev(ctx);

    vproc_dbg_reset("reset loop start\n");
    // on reset just return all index
    do {
        ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DEINTERLACE);
        if (MPP_OK == ret && index >= 0) {
            MppFrame frame = NULL;

            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME, &frame);
            if (frame)
                mpp_frame_deinit(&frame);

            mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
            ctx->count--;
            vproc_dbg_status("reset index %d\n", index);
        }
    } while (ret == MPP_OK);
    mpp_assert(ctx->count == 0);

    vproc_dbg_reset("reset loop done\n");
    thd->lock(THREAD_CONTROL);
    ctx->reset = 0;
    vproc_dbg_reset("reset signal\n");
    thd->signal(THREAD_CONTROL);
    thd->unlock(THREAD_CONTROL);
    vproc_dbg_reset("reset done\n");
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

    MPP_RET ret = iep_control(ctx->iep_ctx, cmd, img);
    if (ret)
        mpp_log_f("control %08x failed %d\n", cmd, ret);
}

static MppBuffer dec_vproc_get_buffer(MppBufferGroup group, size_t size)
{
    MppBuffer buf = NULL;

    do {
        mpp_buffer_get(group, &buf, size);
        if (NULL == buf)
            usleep(2000);
        else
            break;
    } while (1);

    return buf;
}

// start deinterlace hardware
static void dec_vproc_start_dei(MppDecVprocCtxImpl *ctx, RK_U32 mode)
{
    ctx->dei_cfg.dei_field_order =
        (mode & MPP_FRAME_FLAG_TOP_FIRST) ?
        (IEP_DEI_FLD_ORDER_TOP_FIRST) :
        (IEP_DEI_FLD_ORDER_BOT_FIRST);

    MPP_RET ret = iep_control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &ctx->dei_cfg);
    if (ret)
        mpp_log_f("IEP_CMD_SET_DEI_CFG failed %d\n", ret);

    ret = iep_control(ctx->iep_ctx, IEP_CMD_RUN_SYNC, NULL);
    if (ret)
        mpp_log_f("IEP_CMD_RUN_SYNC failed %d\n", ret);
}

static void *dec_vproc_thread(void *data)
{
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)data;
    MppThread *thd = ctx->thd;
    Mpp *mpp = ctx->mpp;
    MppDec *dec = mpp->mDec;
    MppBufSlots slots = dec->frame_slots;
    IepImg img;

    mpp_dbg(MPP_DBG_INFO, "mpp_dec_post_proc_thread started\n");

    while (1) {
        RK_S32 index = -1;
        MPP_RET ret = MPP_OK;

        {
            AutoMutex autolock(thd->mutex());

            if (MPP_THREAD_RUNNING != thd->get_status())
                break;

            if (ctx->reset) {
                dec_vproc_reset_queue(ctx);
                continue;
            } else {
                ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DEINTERLACE);
                if (ret) {
                    thd->wait();
                    continue;
                } else {
                    ctx->count--;
                }
            }
        }

        // dequeue from deinterlace queue then send to mpp->mFrames
        if (index >= 0) {
            MppFrame frm = NULL;

            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME_PTR, &frm);

            if (mpp_frame_get_info_change(frm)) {
                vproc_dbg_status("info change\n");
                dec_vproc_put_frame(mpp, frm, NULL, -1);
                dec_vproc_clr_prev(ctx);
                mpp_buf_slot_clr_flag(ctx->slots, index, SLOT_QUEUE_USE);
                continue;
            }

            RK_U32 eos = mpp_frame_get_eos(frm);
            if (eos && NULL == mpp_frame_get_buffer(frm)) {
                vproc_dbg_status("eos\n");
                dec_vproc_put_frame(mpp, frm, NULL, -1);
                dec_vproc_clr_prev(ctx);
                mpp_buf_slot_clr_flag(ctx->slots, index, SLOT_QUEUE_USE);
                continue;
            }

            if (!dec->reset_flag && ctx->iep_ctx) {
                MppBufferGroup group = mpp->mFrameGroup;
                RK_U32 mode = mpp_frame_get_mode(frm);
                MppBuffer buf = mpp_frame_get_buffer(frm);
                MppBuffer dst0 = NULL;
                MppBuffer dst1 = NULL;
                int fd = -1;
                size_t buf_size = mpp_buffer_get_size(buf);

                // setup source IepImg
                dec_vproc_set_img_fmt(&img, frm);

                ret = iep_control(ctx->iep_ctx, IEP_CMD_INIT, NULL);
                if (ret)
                    mpp_log_f("IEP_CMD_INIT failed %d\n", ret);

                // setup destination IepImg with new buffer
                // NOTE: when deinterlace is enabled parser thread will reserve
                //       more buffer than normal case
                if (ctx->prev_frm) {
                    // 4 in 2 out case
                    vproc_dbg_status("4 field in and 2 frame out\n");
                    RK_S64 prev_pts = mpp_frame_get_pts(ctx->prev_frm);
                    RK_S64 curr_pts = mpp_frame_get_pts(frm);
                    RK_S64 first_pts = (prev_pts + curr_pts) / 2;

                    buf = mpp_frame_get_buffer(ctx->prev_frm);
                    fd = mpp_buffer_get_fd(buf);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);

                    // setup dst 0
                    dst0 = dec_vproc_get_buffer(group, buf_size);
                    mpp_assert(dst0);
                    fd = mpp_buffer_get_fd(dst0);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);

                    buf = mpp_frame_get_buffer(frm);
                    fd = mpp_buffer_get_fd(buf);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);

                    // setup dst 1
                    dst1 = dec_vproc_get_buffer(group, buf_size);
                    mpp_assert(dst1);
                    fd = mpp_buffer_get_fd(dst1);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_DST1);

                    ctx->dei_cfg.dei_mode = IEP_DEI_MODE_I4O2;

                    // start hardware
                    dec_vproc_start_dei(ctx, mode);

                    // NOTE: we need to process pts here
                    if (mode & MPP_FRAME_FLAG_TOP_FIRST) {
                        dec_vproc_put_frame(mpp, frm, dst0, first_pts);
                        dec_vproc_put_frame(mpp, frm, dst1, curr_pts);
                    } else {
                        dec_vproc_put_frame(mpp, frm, dst1, first_pts);
                        dec_vproc_put_frame(mpp, frm, dst0, curr_pts);
                    }
                } else {
                    // 2 in 1 out case
                    vproc_dbg_status("2 field in and 1 frame out\n");
                    buf = mpp_frame_get_buffer(frm);
                    fd = mpp_buffer_get_fd(buf);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);

                    // setup dst 0
                    dst0 = dec_vproc_get_buffer(group, buf_size);
                    mpp_assert(dst0);
                    fd = mpp_buffer_get_fd(dst0);
                    dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);

                    ctx->dei_cfg.dei_mode = IEP_DEI_MODE_I2O1;

                    // start hardware
                    dec_vproc_start_dei(ctx, mode);
                    dec_vproc_put_frame(mpp, frm, dst0, -1);
                }
            }

            dec_vproc_clr_prev(ctx);
            ctx->prev_idx = index;
            ctx->prev_frm = frm;

            if (eos)
                dec_vproc_clr_prev(ctx);
        }
    }
    mpp_dbg(MPP_DBG_INFO, "mpp_dec_post_proc_thread exited\n");

    return NULL;
}

MPP_RET dec_vproc_init(MppDecVprocCtx *ctx, void *mpp)
{
    MPP_RET ret = MPP_OK;
    if (NULL == ctx || NULL == mpp) {
        mpp_err_f("found NULL input ctx %p mpp %p\n", ctx, mpp);
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

    p->mpp = (Mpp *)mpp;
    p->slots = p->mpp->mDec->frame_slots;
    p->thd = new MppThread(dec_vproc_thread, p, "mpp_dec_vproc");
    ret = iep_init(&p->iep_ctx);
    if (!p->thd || ret) {
        mpp_err("failed to create context\n");
        if (p->thd) {
            delete p->thd;
            p->thd = NULL;
        }

        if (p->iep_ctx) {
            iep_deinit(p->iep_ctx);
            p->iep_ctx = NULL;
        }

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

        p->prev_idx = -1;
        p->prev_frm = NULL;
    }

    *ctx = p;

    vproc_dbg_func("out\n");
    return ret;
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
        p->thd->stop();
        delete p->thd;
        p->thd = NULL;
    }

    if (p->iep_ctx) {
        iep_deinit(p->iep_ctx);
        p->iep_ctx = NULL;
    }

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
        p->thd->start();
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
        p->thd->stop();
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
        p->thd->lock();
        p->count++;
        p->thd->signal();
        p->thd->unlock();
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
        // wait reset finished
        p->thd->lock(THREAD_CONTROL);

        p->thd->lock();
        p->reset = 1;
        p->thd->signal();
        p->thd->unlock();

        vproc_dbg_reset("reset contorl wait\n");
        p->thd->wait(THREAD_CONTROL);
        vproc_dbg_reset("reset contorl done\n");
        p->thd->unlock(THREAD_CONTROL);

        mpp_assert(p->reset == 0);
    }

    vproc_dbg_func("out\n");
    return MPP_OK;
}
