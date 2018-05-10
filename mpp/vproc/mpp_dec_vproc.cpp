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

#include "mpp_env.h"
#include "mpp_mem.h"

#include "mpp_dec_vproc.h"

#define vproc_dbg(flag, fmt, ...) \
    do { \
        _mpp_dbg(vproc_debug, flag, fmt, ## __VA_ARGS__); \
    } while (0)

#define vproc_dbg_f(flag, fmt, ...) \
    do { \
        _mpp_dbg_f(vproc_debug, flag, fmt, ## __VA_ARGS__); \
    } while (0)

#define VPROC_DBG_FUNCTION      (0x00000001)

#define vproc_func(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_FUNCTION, fmt, ## __VA_ARGS__);

RK_U32 vproc_debug = 0;

typedef struct MppDecVprocCtxImpl_t {
    Mpp         *mpp;
    MppBufSlots slots;

    MppThread   *thd;
    RK_U32      reset;
    RK_S32      count;
} MppDecVprocCtxImpl;

static void mpp_enqueue_frames(Mpp *mpp, MppFrame frame)
{
    mpp_list *list = mpp->mFrames;

    list->lock();
    list->add_at_tail(&frame, sizeof(frame));

    if (mpp_debug & MPP_DBG_PTS)
        mpp_log("output frame pts %lld\n", mpp_frame_get_pts(frame));

    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
}

static void *dec_vproc_thread(void *data)
{
    MppDecVprocCtxImpl *ctx = (MppDecVprocCtxImpl *)data;
    MppThread *thd = ctx->thd;
    Mpp *mpp = ctx->mpp;
    MppDec *dec = mpp->mDec;
    MppBufSlots slots = dec->frame_slots;

    mpp_dbg(MPP_DBG_INFO, "mpp_dec_post_proc_thread started\n");

    while (1) {
        RK_S32 index = -1;

        {
            AutoMutex autolock(thd->mutex());
            MPP_RET ret = MPP_OK;

            if (MPP_THREAD_RUNNING != thd->get_status())
                break;

            if (ctx->reset) {
                mpp_log_f("reset start\n");
                // on reset just return all index
                do {
                    ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DEINTERLACE);
                    if (MPP_OK == ret && index >= 0) {
                        mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
                        ctx->count--;
                        mpp_log_f("reset index %d\n", index);
                    }
                } while (ret == MPP_OK);
                mpp_log_f("reset done\n");

                thd->lock(THREAD_CONTROL);
                ctx->reset = 0;
                thd->signal(THREAD_CONTROL);
                thd->unlock(THREAD_CONTROL);
            } else {
                ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DEINTERLACE);
                if (ret)
                    thd->wait();
                else
                    ctx->count--;
            }

            if (ret)
                continue;
        }


        // dequeue from deinterlace queue then send to mpp->mFrames
        if (index >= 0) {
            MppFrame frame = NULL;
            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME, &frame);
            if (mpp_frame_get_info_change(frame)) {
                mpp_enqueue_frames(mpp, frame);
                goto VPROC_DONE;
            }

            if (!dec->reset_flag) {
                mpp_enqueue_frames(mpp, frame);
            } else
                mpp_frame_deinit(&frame);

        VPROC_DONE:
            mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
        }
    }
    mpp_dbg(MPP_DBG_INFO, "mpp_dec_post_proc_thread exited\n");

    return NULL;
}

MPP_RET dec_vproc_init(MppDecVprocCtx *ctx, void *mpp)
{
    if (NULL == ctx || NULL == mpp) {
        mpp_err_f("found NULL input ctx %p mpp %p\n", ctx, mpp);
        return MPP_ERR_NULL_PTR;
    }

    vproc_func("in");
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

    *ctx = p;

    vproc_func("out");
    return MPP_OK;
}

MPP_RET dec_vproc_deinit(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_func("in");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd)
        p->thd->stop();

    mpp_free(p);

    vproc_func("out");
    return MPP_OK;
}

MPP_RET dec_vproc_start(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_func("in");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;

    if (p->thd)
        p->thd->start();
    else
        mpp_err("failed to start dec vproc thread\n");

    vproc_func("out");
    return MPP_OK;
}

MPP_RET dec_vproc_stop(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_func("in");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;

    if (p->thd)
        p->thd->stop();
    else
        mpp_err("failed to stop dec vproc thread\n");

    vproc_func("out");
    return MPP_OK;
}

MPP_RET dec_vproc_signal(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_func("in");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd) {
        p->thd->lock();
        p->count++;
        p->thd->signal();
        p->thd->unlock();
    }

    vproc_func("out");
    return MPP_OK;
}

MPP_RET dec_vproc_reset(MppDecVprocCtx ctx)
{
    if (NULL == ctx) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }
    vproc_func("in");

    MppDecVprocCtxImpl *p = (MppDecVprocCtxImpl *)ctx;
    if (p->thd) {
        p->thd->lock();
        p->reset = 1;
        p->thd->signal();
        p->thd->unlock();

        // wait reset finished
        p->thd->lock(THREAD_CONTROL);
        p->thd->wait(THREAD_CONTROL);
        p->thd->unlock(THREAD_CONTROL);

        mpp_assert(p->reset == 0);
    }

    vproc_func("out");
    return MPP_OK;
}
