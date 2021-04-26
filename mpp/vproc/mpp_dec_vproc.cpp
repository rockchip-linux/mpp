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

#define vproc_dbg_func(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_FUNCTION, fmt, ## __VA_ARGS__);
#define vproc_dbg_status(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_STATUS, fmt, ## __VA_ARGS__);
#define vproc_dbg_reset(fmt, ...)  \
    vproc_dbg_f(VPROC_DBG_RESET, fmt, ## __VA_ARGS__);

RK_U32 vproc_debug = 0;

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

    // slot index for previous frame and current frame
    RK_S32              prev_idx0;
    MppFrame            prev_frm0;
    RK_S32              prev_idx1;
    MppFrame            prev_frm1;
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

    mpp_dbg_pts("output frame pts %lld\n", mpp_frame_get_pts(out));

    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
}

static void dec_vproc_clr_prev0(MppDecVprocCtxImpl *ctx)
{
    if (vproc_debug & VPROC_DBG_STATUS) {
        if (ctx->prev_frm0) {
            MppBuffer buf = mpp_frame_get_buffer(ctx->prev_frm0);
            RK_S32 fd = (buf) ? (mpp_buffer_get_fd(buf)) : (-1);
            mpp_log("clearing prev index %d frm %p fd %d\n", ctx->prev_idx0,
                    ctx->prev_frm0, fd);
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
            mpp_log("clearing prev index %d frm %p fd %d\n", ctx->prev_idx1,
                    ctx->prev_frm1, fd);
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
    MPP_RET ret;

    if (ctx->com_ctx->ver == 1) {
        ctx->dei_cfg.dei_field_order =
            (mode & MPP_FRAME_FLAG_TOP_FIRST) ?
            (IEP_DEI_FLD_ORDER_TOP_FIRST) :
            (IEP_DEI_FLD_ORDER_BOT_FIRST);

        ret = ctx->com_ctx->ops->control(ctx->iep_ctx,
                                         IEP_CMD_SET_DEI_CFG, &ctx->dei_cfg);
        if (ret)
            mpp_log_f("IEP_CMD_SET_DEI_CFG failed %d\n", ret);
    }

    ret = ctx->com_ctx->ops->control(ctx->iep_ctx, IEP_CMD_RUN_SYNC, NULL);
    if (ret)
        mpp_log_f("IEP_CMD_RUN_SYNC failed %d\n", ret);
}

static void dec_vproc_set_dei_v1(MppDecVprocCtxImpl *ctx, MppFrame frm)
{
    MPP_RET ret = MPP_OK;
    IepImg img;

    Mpp *mpp = ctx->mpp;
    MppBufferGroup group = mpp->mFrameGroup;
    RK_U32 mode = mpp_frame_get_mode(frm);
    MppBuffer buf = mpp_frame_get_buffer(frm);
    MppBuffer dst0 = NULL;
    MppBuffer dst1 = NULL;
    int fd = -1;
    size_t buf_size = mpp_buffer_get_size(buf);

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

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I4O2;
        mpp_frame_set_mode(frm, mode);

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
        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I2O1;
        mpp_frame_set_mode(frm, mode);

        // start hardware
        dec_vproc_start_dei(ctx, mode);
        dec_vproc_put_frame(mpp, frm, dst0, -1);
    }
}

static void dec_vproc_set_dei_v2(MppDecVprocCtxImpl *ctx, MppFrame frm)
{
    IepImg img;

    Mpp *mpp = ctx->mpp;
    MppBufferGroup group = mpp->mFrameGroup;
    RK_U32 mode = mpp_frame_get_mode(frm);
    MppBuffer buf = mpp_frame_get_buffer(frm);
    MppBuffer dst0 = NULL;
    MppBuffer dst1 = NULL;
    int fd = -1;
    size_t buf_size = mpp_buffer_get_size(buf);
    iep_com_ops *ops = ctx->com_ctx->ops;

    // setup source IepImg
    dec_vproc_set_img_fmt(&img, frm);

    if (ctx->prev_frm1 && ctx->prev_frm0) {

        struct iep2_api_params params;

        // 5 in 2 out case
        vproc_dbg_status("5 field in and 2 frame out\n");
        RK_S64 prev_pts = mpp_frame_get_pts(ctx->prev_frm1);
        RK_S64 curr_pts = mpp_frame_get_pts(ctx->prev_frm0);
        RK_S64 first_pts = (prev_pts + curr_pts) / 2;

        // setup source frames
        buf = mpp_frame_get_buffer(ctx->prev_frm0);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);

        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);

        buf = mpp_frame_get_buffer(ctx->prev_frm1);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC2);

        // setup dst 0
        dst0 = dec_vproc_get_buffer(group, buf_size);
        mpp_assert(dst0);
        fd = mpp_buffer_get_fd(dst0);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);

        // setup dst 1
        dst1 = dec_vproc_get_buffer(group, buf_size);
        mpp_assert(dst1);
        fd = mpp_buffer_get_fd(dst1);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_DST1);

        params.ptype = IEP2_PARAM_TYPE_MODE;
        params.param.mode.dil_mode = IEP2_DIL_MODE_I5O2;
        params.param.mode.out_mode = IEP2_OUT_MODE_LINE;
        params.param.mode.dil_order = (mode & MPP_FRAME_FLAG_TOP_FIRST) ?
                                      IEP2_FIELD_ORDER_TFF : IEP2_FIELD_ORDER_BFF;

        ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

        params.ptype = IEP2_PARAM_TYPE_COM;
        params.param.com.sfmt = IEP2_FMT_YUV420;
        params.param.com.dfmt = IEP2_FMT_YUV420;
        params.param.com.sswap = IEP2_YUV_SWAP_SP_UV;
        params.param.com.dswap = IEP2_YUV_SWAP_SP_UV;
        params.param.com.width = img.act_w;
        params.param.com.height = img.act_h;

        ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

        mode = mode | MPP_FRAME_FLAG_IEP_DEI_I4O2;
        mpp_frame_set_mode(frm, mode);

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
        struct iep2_api_params params;

        // 2 in 1 out case
        vproc_dbg_status("2 field in and 1 frame out\n");
        buf = mpp_frame_get_buffer(frm);
        fd = mpp_buffer_get_fd(buf);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_SRC);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC1);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_SRC2);

        // setup dst 0
        dst0 = dec_vproc_get_buffer(group, buf_size);
        mpp_assert(dst0);
        fd = mpp_buffer_get_fd(dst0);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DST);
        dec_vproc_set_img(ctx, &img, fd, IEP_CMD_SET_DEI_DST1);

        params.ptype = IEP2_PARAM_TYPE_MODE;
        params.param.mode.dil_mode = IEP2_DIL_MODE_I1O1T;
        params.param.mode.out_mode = IEP2_OUT_MODE_LINE;
        ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

        params.ptype = IEP2_PARAM_TYPE_COM;
        params.param.com.sfmt = IEP2_FMT_YUV420;
        params.param.com.dfmt = IEP2_FMT_YUV420;
        params.param.com.sswap = IEP2_YUV_SWAP_SP_UV;
        params.param.com.dswap = IEP2_YUV_SWAP_SP_UV;
        params.param.com.width = img.act_w;
        params.param.com.height = img.act_h;
        ops->control(ctx->iep_ctx, IEP_CMD_SET_DEI_CFG, &params);

        // start hardware
        dec_vproc_start_dei(ctx, mode);
        dec_vproc_put_frame(mpp, frm, dst0, -1);
    }
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

        {
            AutoMutex autolock(thd->mutex());

            if (MPP_THREAD_RUNNING != thd->get_status())
                break;

            if (hal_task_get_hnd(tasks, TASK_PROCESSING, &task)) {
                {
                    AutoMutex autolock_reset(thd->mutex(THREAD_CONTROL));
                    if (ctx->reset) {
                        vproc_dbg_reset("reset start\n");
                        dec_vproc_clr_prev(ctx);

                        ctx->reset = 0;
                        sem_post(&ctx->reset_sem);
                        vproc_dbg_reset("reset done\n");
                        continue;
                    }
                }

                thd->wait();
                continue;
            }
        }
        // process all task then do reset process
        thd->lock(THREAD_CONTROL);
        thd->unlock(THREAD_CONTROL);

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
                dec_vproc_put_frame(mpp, frm, NULL, -1);
                dec_vproc_clr_prev(ctx);
                mpp_frame_deinit(&frm);

                hal_task_hnd_set_status(task, TASK_IDLE);
                continue;
            }

            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME_PTR, &frm);

            if (change) {
                vproc_dbg_status("info change\n");
                dec_vproc_put_frame(mpp, frm, NULL, -1);
                dec_vproc_clr_prev(ctx);

                hal_task_hnd_set_status(task, TASK_IDLE);
                continue;
            }

            RK_S32 tmp = -1;
            mpp_buf_slot_dequeue(slots, &tmp, QUEUE_DEINTERLACE);
            mpp_assert(tmp == index);

            if (!dec->reset_flag && ctx->iep_ctx) {
                if (ctx->com_ctx->ver == 1) {
                    dec_vproc_set_dei_v1(ctx, frm);
                } else {
                    dec_vproc_set_dei_v2(ctx, frm);
                }
            }


            if (ctx->com_ctx->ver == 1) {
                dec_vproc_clr_prev0(ctx);
                ctx->prev_idx0 = index;
                ctx->prev_frm0 = frm;
            } else {
                dec_vproc_clr_prev1(ctx);
                ctx->prev_idx1 = ctx->prev_idx0;
                ctx->prev_idx0 = index;

                ctx->prev_frm1 = ctx->prev_frm0;
                ctx->prev_frm0 = frm;
            }

            if (eos) {
                dec_vproc_clr_prev(ctx);
                if (ctx->com_ctx->ver == 2) {
                    ctx->prev_idx1 = ctx->prev_idx0;
                    ctx->prev_idx0 = index;
                    dec_vproc_clr_prev(ctx);
                }
            }

            hal_task_hnd_set_status(task, TASK_IDLE);
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

    p->mpp = (Mpp *)cfg->mpp;
    p->slots = ((MppDecImpl *)p->mpp->mDec)->frame_slots;
    p->thd = new MppThread(dec_vproc_thread, p, "mpp_dec_vproc");
    sem_init(&p->reset_sem, 0, 0);
    ret = hal_task_group_init(&p->task_group, 4);
    if (ret) {
        mpp_err_f("create task group failed\n");
        delete p->thd;
        MPP_FREE(p);
        return MPP_ERR_MALLOC;
    }
    cfg->task_group = p->task_group;

    /// TODO, seperate iep1/2 api
    p->com_ctx = get_iep_ctx();
    if (!p->com_ctx) {
        mpp_err("failed to require context\n");
        delete p->thd;

        if (p->task_group) {
            hal_task_group_deinit(p->task_group);
            p->task_group = NULL;
        }

        MPP_FREE(p);

        return MPP_ERR_MALLOC;
    }

    ret = p->com_ctx->ops->init(&p->com_ctx->priv);
    p->iep_ctx = p->com_ctx->priv;
    if (!p->thd || ret) {
        mpp_err("failed to create context\n");
        if (p->thd) {
            delete p->thd;
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
        MppThread *thd = p->thd;

        vproc_dbg_reset("reset contorl start\n");
        // wait reset finished
        thd->lock();
        thd->lock(THREAD_CONTROL);
        p->reset = 1;
        thd->unlock(THREAD_CONTROL);
        thd->signal();
        thd->unlock();

        vproc_dbg_reset("reset contorl wait\n");
        sem_wait(&p->reset_sem);
        vproc_dbg_reset("reset contorl done\n");

        mpp_assert(p->reset == 0);
    }

    vproc_dbg_func("out\n");
    return MPP_OK;
}
