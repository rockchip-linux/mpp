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

#define  MODULE_TAG "mpp_dec"

#include <string.h>

#include "mpp_env.h"

#include "mpp_buffer_impl.h"
#include "mpp_frame_impl.h"
#include "mpp_dec_cfg_impl.h"

#include "mpp_dec_debug.h"
#include "mpp_dec_vproc.h"
#include "mpp_dec_cb_param.h"
#include "mpp_dec_normal.h"
#include "mpp_dec_no_thread.h"

RK_U32 mpp_dec_debug = 0;

MPP_RET dec_task_info_init(HalTaskInfo *task)
{
    HalDecTask *p = &task->dec;

    p->valid  = 0;
    p->flags.val = 0;
    p->flags.eos = 0;
    p->input_packet = NULL;
    p->output = -1;
    p->input = -1;
    memset(&task->dec.syntax, 0, sizeof(task->dec.syntax));
    memset(task->dec.refer, -1, sizeof(task->dec.refer));

    return MPP_OK;
}

void dec_task_init(DecTask *task)
{
    task->hnd = NULL;
    task->status.val = 0;
    task->wait.val   = 0;
    task->status.prev_task_rdy  = 1;
    INIT_LIST_HEAD(&task->ts_cur.link);

    dec_task_info_init(&task->info);
}

static MPP_RET mpp_dec_update_cfg(MppDecImpl *p)
{
    MppDecCfgSet *cfg = &p->cfg;
    MppDecBaseCfg *base = &cfg->base;
    MppDecStatusCfg *status = &cfg->status;

    if (status->hal_task_count && !status->hal_support_fast_mode) {
        if (!p->parser_fast_mode && base->fast_parse) {
            mpp_err("can not enable fast parse while hal not support\n");
            base->fast_parse = 0;
        }
    }

    p->parser_fast_mode     = base->fast_parse;
    p->enable_deinterlace   = base->enable_vproc;
    p->disable_error        = base->disable_error;
    p->dis_err_clr_mark     = base->dis_err_clr_mark;

    mpp_env_get_u32("enable_deinterlace", &p->enable_deinterlace, base->enable_vproc);

    return MPP_OK;
}

static MPP_RET mpp_dec_check_fbc_cap(MppDecImpl *p)
{
    MppDecBaseCfg *base = &p->cfg.base;

    if (MPP_FRAME_FMT_IS_FBC(base->out_fmt)) {
        RK_U32 fbc = (RK_U32)base->out_fmt & MPP_FRAME_FBC_MASK;
        RK_U32 fmt = base->out_fmt - fbc;

        if (p->hw_info && p->hw_info->cap_fbc)
            fmt |= fbc;

        base->out_fmt = (MppFrameFormat)fmt;
    }

    return MPP_OK;
}

MPP_RET mpp_dec_proc_cfg(MppDecImpl *dec, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;

    mpp_parser_control(dec->parser, cmd, param);

    ret = mpp_hal_control(dec->hal, cmd, param);

    if (ret)
        goto RET;

    switch (cmd) {
    case MPP_DEC_SET_FRAME_INFO : {
        MppFrame frame = (MppFrame)param;

        /* update output frame format */
        dec->cfg.base.out_fmt = mpp_frame_get_fmt(frame);
        mpp_log_f("found MPP_DEC_SET_FRAME_INFO fmt %x\n", dec->cfg.base.out_fmt);

        mpp_slots_set_prop(dec->frame_slots, SLOTS_FRAME_INFO, frame);

        mpp_log("setting default w %4d h %4d h_str %4d v_str %4d\n",
                mpp_frame_get_width(frame),
                mpp_frame_get_height(frame),
                mpp_frame_get_hor_stride(frame),
                mpp_frame_get_ver_stride(frame));

    } break;
    case MPP_DEC_SET_INFO_CHANGE_READY: {
        ret = mpp_buf_slot_ready(dec->frame_slots);
    } break;
    case MPP_DEC_GET_VPUMEM_USED_COUNT: {
        RK_S32 *p = (RK_S32 *)param;
        *p = mpp_slots_get_used_count(dec->frame_slots);
        dec_dbg_func("used count %d\n", *p);
    } break;
    case MPP_DEC_SET_PRESENT_TIME_ORDER :
    case MPP_DEC_SET_PARSER_SPLIT_MODE :
    case MPP_DEC_SET_PARSER_FAST_MODE :
    case MPP_DEC_SET_IMMEDIATE_OUT :
    case MPP_DEC_SET_OUTPUT_FORMAT :
    case MPP_DEC_SET_DISABLE_ERROR :
    case MPP_DEC_SET_DIS_ERR_CLR_MARK :
    case MPP_DEC_SET_ENABLE_DEINTERLACE :
    case MPP_DEC_SET_ENABLE_FAST_PLAY :
    case MPP_DEC_SET_ENABLE_MVC :
    case MPP_DEC_SET_DISABLE_DPB_CHECK: {
        ret = mpp_dec_set_cfg_by_cmd(&dec->cfg, cmd, param);
        mpp_dec_update_cfg(dec);
        mpp_dec_check_fbc_cap(dec);
        dec->cfg.base.change = 0;
    } break;
    case MPP_DEC_QUERY: {
        MppDecQueryCfg *query = (MppDecQueryCfg *)param;
        RK_U32 flag = query->query_flag;

        dec_dbg_func("query %x\n", flag);

        if (flag & MPP_DEC_QUERY_STATUS)
            query->rt_status = dec->parser_status_flag;

        if (flag & MPP_DEC_QUERY_WAIT)
            query->rt_wait = dec->parser_wait_flag;

        if (flag & MPP_DEC_QUERY_FPS)
            query->rt_fps = 0;

        if (flag & MPP_DEC_QUERY_BPS)
            query->rt_bps = 0;

        if (flag & MPP_DEC_QUERY_DEC_IN_PKT)
            query->dec_in_pkt_cnt = dec->dec_in_pkt_count;

        if (flag & MPP_DEC_QUERY_DEC_WORK)
            query->dec_hw_run_cnt = dec->dec_hw_run_count;

        if (flag & MPP_DEC_QUERY_DEC_OUT_FRM)
            query->dec_out_frm_cnt = dec->dec_out_frame_count;
    } break;
    case MPP_DEC_SET_CFG: {
        MppDecCfgImpl *dec_cfg = (MppDecCfgImpl *)param;

        if (dec_cfg) {
            mpp_dec_set_cfg(&dec->cfg, &dec_cfg->cfg);
            mpp_dec_update_cfg(dec);
            mpp_dec_check_fbc_cap(dec);
        }

        dec_dbg_func("set dec cfg\n");
    } break;
    case MPP_DEC_GET_CFG: {
        MppDecCfgImpl *dec_cfg = (MppDecCfgImpl *)param;

        if (dec_cfg)
            memcpy(&dec_cfg->cfg, &dec->cfg, sizeof(dec->cfg));

        dec_dbg_func("get dec cfg\n");
    } break;
    default : {
    } break;
    }

RET:
    return ret;
}

/* Overall mpp_dec output frame function */
void mpp_dec_put_frame(Mpp *mpp, RK_S32 index, HalDecTaskFlag flags)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppBufSlots slots = dec->frame_slots;
    MppFrame frame = NULL;
    RK_U32 eos = flags.eos;
    RK_U32 change = flags.info_change;
    RK_U32 error = flags.parse_err || flags.ref_err;
    RK_U32 refer = flags.used_for_ref;
    RK_U32 fake_frame = 0;

    if (index >= 0) {
        RK_U32 mode = 0;

        mpp_buf_slot_get_prop(slots, index, SLOT_FRAME_PTR, &frame);

        mode = mpp_frame_get_mode(frame);
        if (mode && dec->enable_deinterlace && NULL == dec->vproc) {
            MppDecVprocCfg cfg = { mpp, NULL };
            MPP_RET ret = dec_vproc_init(&dec->vproc, &cfg);
            if (ret) {
                // When iep is failed to open disable deinterlace function to
                // avoid noisy log.
                dec->enable_deinterlace = 0;
                dec->vproc = NULL;
            } else {
                if (dec_vproc_get_version(dec->vproc) == 1 && mode == MPP_FRAME_FLAG_DEINTERLACED) {
                    mpp_frame_set_mode(frame, MPP_FRAME_FLAG_FRAME);
                    /*iep 1 can't no detect DEINTERLACED, direct disable*/
                    dec->cfg.base.enable_vproc &= (~MPP_VPROC_MODE_DETECTION);
                    dec->enable_deinterlace = dec->cfg.base.enable_vproc;
                    if (dec->vproc && !dec->enable_deinterlace) {
                        dec_vproc_deinit(dec->vproc);
                        dec->vproc = NULL;
                    }
                    dec->vproc = NULL;
                } else {
                    /* store current IEP mode */
                    dec_vproc_set_mode(dec->vproc, (MppVprocMode)dec->enable_deinterlace);

                    dec->vproc_tasks = cfg.task_group;
                    dec_vproc_start(dec->vproc);
                }
            }
        }
    } else {
        // when post-process is needed and eos without slot index case
        // we need to create a slot index for it
        mpp_assert(eos);
        mpp_assert(!change);

        if (dec->vproc) {
            HalTaskGroup group = dec->vproc_tasks;
            HalTaskHnd hnd = NULL;
            HalTaskInfo task;
            HalDecVprocTask *vproc_task = &task.dec_vproc;
            MPP_RET ret = MPP_OK;

            do {
                ret = hal_task_get_hnd(group, TASK_IDLE, &hnd);
                if (ret) {
                    if (dec->reset_flag) {
                        return;
                    } else {
                        msleep(10);
                    }
                }
            } while (ret);
            vproc_task->flags.val = 0;
            vproc_task->flags.eos = eos;
            vproc_task->input = index;

            hal_task_hnd_set_info(hnd, &task);
            hal_task_hnd_set_status(hnd, TASK_PROCESSING);
            dec_vproc_signal(dec->vproc);

            return ;
        } else {
            mpp_frame_init(&frame);
            fake_frame = 1;
            index = 0;
        }

        mpp_frame_set_eos(frame, eos);
    }

    mpp_assert(index >= 0);
    mpp_assert(frame);

    if (dec->cfg.base.disable_error && dec->cfg.base.dis_err_clr_mark) {
        mpp_frame_set_errinfo(frame, 0);
        mpp_frame_set_discard(frame, 0);
    }

    if (!change) {
        if (dec->cfg.base.sort_pts) {
            MppPktTs *pkt_ts;

            mpp_spinlock_lock(&dec->ts_lock);
            pkt_ts = list_first_entry_or_null(&dec->ts_link, MppPktTs, link);
            if (pkt_ts)
                list_del_init(&pkt_ts->link);
            mpp_spinlock_unlock(&dec->ts_lock);
            if (pkt_ts) {
                mpp_frame_set_dts(frame, pkt_ts->dts);
                mpp_frame_set_pts(frame, pkt_ts->pts);
                mpp_mem_pool_put(dec->ts_pool, pkt_ts);
            }
        }
    }
    mpp_frame_set_info_change(frame, change);

    if (eos) {
        mpp_frame_set_eos(frame, 1);
        if (error) {
            if (refer)
                mpp_frame_set_errinfo(frame, 1);
            else
                mpp_frame_set_discard(frame, 1);
        }
    }

    dec->dec_out_frame_count++;
    dec_dbg_detail("detail: %p put frm pts %llu fd %d\n", dec,
                   mpp_frame_get_pts(frame),
                   (NULL == mpp_frame_get_buffer(frame)) ? (-1) :
                   mpp_buffer_get_fd(mpp_frame_get_buffer(frame)));

    if (dec->vproc) {
        HalTaskGroup group = dec->vproc_tasks;
        HalTaskHnd hnd = NULL;
        HalTaskInfo task;
        HalDecVprocTask *vproc_task = &task.dec_vproc;
        MPP_RET ret = MPP_OK;

        do {
            ret = hal_task_get_hnd(group, TASK_IDLE, &hnd);
            if (ret) {
                if (dec->reset_flag) {
                    MppBuffer buffer = NULL;
                    mpp_buf_slot_get_prop(slots, index, SLOT_BUFFER, &buffer);
                    if (buffer)
                        mpp_buffer_put(buffer);
                    return;
                } else {
                    msleep(10);
                }
            }
        } while (ret);

        mpp_assert(ret == MPP_OK);

        vproc_task->flags.eos = eos;
        vproc_task->flags.info_change = change;
        vproc_task->input = index;

        if (!change) {
            mpp_buf_slot_set_flag(slots, index, SLOT_QUEUE_USE);
            mpp_buf_slot_enqueue(slots, index, QUEUE_DEINTERLACE);
        }

        hal_task_hnd_set_info(hnd, &task);
        hal_task_hnd_set_status(hnd, TASK_PROCESSING);

        dec_vproc_signal(dec->vproc);
    } else {
        // direct output -> copy a new MppFrame and output
        mpp_list *list = mpp->mFrmOut;
        MppFrame out = NULL;

        mpp_frame_init(&out);
        mpp_frame_copy(out, frame);

        mpp_dbg_pts("output frame pts %lld\n", mpp_frame_get_pts(out));

        list->lock();
        list->add_at_tail(&out, sizeof(out));
        mpp->mFramePutCount++;
        list->signal();
        list->unlock();

        if (fake_frame)
            mpp_frame_deinit(&frame);

        mpp_dec_callback(dec, MPP_DEC_EVENT_ON_FRM_READY, out);
    }
}

RK_S32 mpp_dec_push_display(Mpp *mpp, HalDecTaskFlag flags)
{
    RK_S32 index = -1;
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppBufSlots frame_slots = dec->frame_slots;
    RK_U32 eos = flags.eos;
    HalDecTaskFlag tmp = flags;
    RK_S32 ret = 0;

    tmp.eos = 0;
    /**
     * After info_change is encountered by parser thread, HalDecTaskFlag will
     * have this flag set. Although mpp_dec_flush is called there may be some
     * frames still remaining in display queue and waiting to be output. So
     * these frames shouldn't have info_change set since their resolution and
     * bit depth are the same as before. What's more, the info_change flag has
     * nothing to do with frames being output.
     */
    tmp.info_change = 0;

    if (dec->thread_hal)
        dec->thread_hal->lock(THREAD_OUTPUT);

    while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
        /* deal with current frame */
        if (eos && mpp_slots_is_empty(frame_slots, QUEUE_DISPLAY))
            tmp.eos = 1;

        mpp_dec_put_frame(mpp, index, tmp);
        mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        ret++;
    }

    if (dec->thread_hal)
        dec->thread_hal->unlock(THREAD_OUTPUT);

    return ret;
}

MPP_RET update_dec_hal_info(MppDecImpl *dec, MppFrame frame)
{
    HalInfo hal_info = dec->hal_info;
    MppDevInfoCfg data[DEC_INFO_BUTT];
    RK_S32 size = sizeof(data);
    RK_U64 val = 0;
    RK_S32 i;

    hal_info_set(hal_info, DEC_INFO_WIDTH, CODEC_INFO_FLAG_NUMBER,
                 mpp_frame_get_width(frame));

    hal_info_set(hal_info, DEC_INFO_HEIGHT, CODEC_INFO_FLAG_NUMBER,
                 mpp_frame_get_height(frame));

    val = hal_info_to_string(hal_info, DEC_INFO_FORMAT, &dec->coding);
    hal_info_set(hal_info, DEC_INFO_FORMAT, CODEC_INFO_FLAG_STRING, val);

    hal_info_get(hal_info, data, &size);

    if (size) {
        size /= sizeof(data[0]);
        for (i = 0; i < size; i++)
            mpp_dev_ioctl(dec->dev, MPP_DEV_SET_INFO, &data[i]);
    }

    return MPP_OK;
}

static MppDecModeApi *dec_api[] = {
    &dec_api_normal,
    &dec_api_no_thread,
};

static const char *timing_str[DEC_TIMING_BUTT] = {
    "prs thread",
    "prs wait  ",
    "prs proc  ",
    "prepare   ",
    "parse     ",
    "gen reg   ",
    "hw start  ",

    "hal thread",
    "hal wait  ",
    "hal proc  ",
    "hw wait   ",
};

MPP_RET mpp_dec_set_cfg(MppDecCfgSet *dst, MppDecCfgSet *src)
{
    MppDecBaseCfg *src_base = &src->base;
    MppDecCbCfg *src_cb = &src->cb;

    if (src_base->change) {
        MppDecBaseCfg *dst_base = &dst->base;
        RK_U32 change = src_base->change;

        if (change & MPP_DEC_CFG_CHANGE_TYPE)
            dst_base->type = src_base->type;

        if (change & MPP_DEC_CFG_CHANGE_CODING)
            dst_base->coding = src_base->coding;

        if (change & MPP_DEC_CFG_CHANGE_HW_TYPE)
            dst_base->hw_type = src_base->hw_type;

        if (change & MPP_DEC_CFG_CHANGE_BATCH_MODE)
            dst_base->batch_mode = src_base->batch_mode;

        if (change & MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT)
            dst_base->out_fmt = src_base->out_fmt;

        if (change & MPP_DEC_CFG_CHANGE_FAST_OUT)
            dst_base->fast_out = src_base->fast_out;

        if (change & MPP_DEC_CFG_CHANGE_FAST_PARSE)
            dst_base->fast_parse = src_base->fast_parse;

        if (change & MPP_DEC_CFG_CHANGE_SPLIT_PARSE)
            dst_base->split_parse = src_base->split_parse;

        if (change & MPP_DEC_CFG_CHANGE_INTERNAL_PTS)
            dst_base->internal_pts = src_base->internal_pts;

        if (change & MPP_DEC_CFG_CHANGE_SORT_PTS)
            dst_base->sort_pts = src_base->sort_pts;

        if (change & MPP_DEC_CFG_CHANGE_DISABLE_ERROR)
            dst_base->disable_error = src_base->disable_error;

        if (change & MPP_DEC_CFG_CHANGE_DIS_ERR_CLR_MARK)
            dst_base->dis_err_clr_mark = src_base->dis_err_clr_mark;

        if (change & MPP_DEC_CFG_CHANGE_ENABLE_VPROC)
            dst_base->enable_vproc = src_base->enable_vproc;

        if (change & MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY)
            dst_base->enable_fast_play = src_base->enable_fast_play;

        if (change & MPP_DEC_CFG_CHANGE_ENABLE_HDR_META)
            dst_base->enable_hdr_meta = src_base->enable_hdr_meta;

        if (change & MPP_DEC_CFG_CHANGE_ENABLE_THUMBNAIL)
            dst_base->enable_thumbnail = src_base->enable_thumbnail;

        if (change & MPP_DEC_CFG_CHANGE_ENABLE_MVC)
            dst_base->enable_mvc = src_base->enable_mvc;

        if (change & MPP_DEC_CFG_CHANGE_DISABLE_DPB_CHECK)
            dst_base->disable_dpb_chk = src_base->disable_dpb_chk;

        if (change & MPP_DEC_CFG_CHANGE_DISABLE_THREAD)
            dst_base->disable_thread = src_base->disable_thread;

        if (change & MPP_DEC_CFG_CHANGE_CODEC_MODE)
            dst_base->codec_mode = src_base->codec_mode;

        dst_base->change = change;
        src_base->change = 0;
    }

    if (src_cb->change) {
        MppDecCbCfg *dst_cb = &dst->cb;
        RK_U32 change = src_cb->change;

        if (change & MPP_DEC_CB_CFG_CHANGE_PKT_RDY) {
            dst_cb->pkt_rdy_cb = src_cb->pkt_rdy_cb;
            dst_cb->pkt_rdy_ctx = src_cb->pkt_rdy_ctx;
            dst_cb->pkt_rdy_cmd = src_cb->pkt_rdy_cmd;
        }

        if (change & MPP_DEC_CB_CFG_CHANGE_FRM_RDY) {
            dst_cb->frm_rdy_cb = src_cb->frm_rdy_cb;
            dst_cb->frm_rdy_ctx = src_cb->frm_rdy_ctx;
            dst_cb->frm_rdy_cmd = src_cb->frm_rdy_cmd;
        }

        dst_cb->change = change;
        src_cb->change = 0;
    }

    return MPP_OK;
}

MPP_RET mpp_dec_callback_hal_to_parser(const char *caller, void *ctx,
                                       RK_S32 cmd, void *param)
{
    MppDecImpl *p = (MppDecImpl *)ctx;
    MPP_RET ret = MPP_OK;
    (void) caller;

    mpp_assert(cmd == DEC_PARSER_CALLBACK);

    if (p->parser)
        ret = mpp_parser_callback(p->parser, param);

    return ret;
}

MPP_RET mpp_dec_callback_slot(const char *caller, void *ctx, RK_S32 cmd, void *param)
{
    (void) caller;
    (void) cmd;
    (void) param;
    return mpp_dec_notify((MppDec)ctx, MPP_DEC_NOTIFY_SLOT_VALID);
}

MPP_RET mpp_dec_init(MppDec *dec, MppDecInitCfg *cfg)
{
    RK_S32 i;
    MPP_RET ret;
    MppCodingType coding;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    HalTaskGroup tasks = NULL;
    Parser parser = NULL;
    MppHal hal = NULL;
    Mpp *mpp = (Mpp *)cfg->mpp;
    MppDecImpl *p = NULL;
    MppDecCfgSet *dec_cfg = NULL;
    RK_U32 hal_task_count = 2;
    RK_U32 support_fast_mode = 0;
    SlotHalFbcAdjCfg hal_fbc_adj_cfg;

    mpp_env_get_u32("mpp_dec_debug", &mpp_dec_debug, 0);

    dec_dbg_func("in\n");

    if (NULL == dec || NULL == cfg) {
        mpp_err_f("invalid input dec %p cfg %p\n", dec, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *dec = NULL;

    p = mpp_calloc(MppDecImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    p->mpp = mpp;
    coding = cfg->coding;
    dec_cfg = &p->cfg;

    mpp_assert(cfg->cfg);
    mpp_dec_cfg_set_default(dec_cfg);
    mpp_dec_set_cfg(dec_cfg, cfg->cfg);
    mpp_dec_update_cfg(p);

    p->dec_cb.callBack = mpp_dec_callback_hal_to_parser;
    p->dec_cb.ctx = p;
    p->dec_cb.cmd = DEC_PARSER_CALLBACK;

    do {
        ret = mpp_buf_slot_init(&frame_slots);
        if (ret) {
            mpp_err_f("could not init frame buffer slot\n");
            break;
        }

        MppCbCtx cb_ctx = {
            mpp_dec_callback_slot,
            (void *)p,
            0,
        };

        mpp_buf_slot_set_callback(frame_slots, &cb_ctx);

        ret = mpp_buf_slot_init(&packet_slots);
        if (ret) {
            mpp_err_f("could not init packet buffer slot\n");
            break;
        }

        MppHalCfg hal_cfg = {
            MPP_CTX_DEC,
            coding,
            frame_slots,
            packet_slots,
            dec_cfg,
            &p->dec_cb,
            NULL,
            NULL,
            0,
            &hal_fbc_adj_cfg,
        };

        memset(&hal_fbc_adj_cfg, 0, sizeof(hal_fbc_adj_cfg));

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        if (hal_fbc_adj_cfg.func)
            mpp_slots_set_prop(frame_slots, SLOTS_HAL_FBC_ADJ, &hal_fbc_adj_cfg);

        support_fast_mode = hal_cfg.support_fast_mode;

        if (dec_cfg->base.fast_parse && support_fast_mode) {
            hal_task_count = dec_cfg->status.hal_task_count ?
                             dec_cfg->status.hal_task_count : 3;
        } else {
            dec_cfg->base.fast_parse = 0;
            p->parser_fast_mode = 0;
        }
        dec_cfg->status.hal_support_fast_mode = support_fast_mode;
        dec_cfg->status.hal_task_count = hal_task_count;

        ret = hal_task_group_init(&tasks, TASK_BUTT, hal_task_count, sizeof(HalDecTask));
        if (ret) {
            mpp_err_f("hal_task_group_init failed ret %d\n", ret);
            break;
        }

        mpp_buf_slot_setup(packet_slots, hal_task_count);
        mpp_slots_set_prop(packet_slots, SLOTS_CODING_TYPE, &coding);
        mpp_slots_set_prop(frame_slots, SLOTS_CODING_TYPE, &coding);

        p->hw_info = hal_cfg.hw_info;
        p->dev = hal_cfg.dev;
        /* check fbc cap after hardware info is valid */
        mpp_dec_check_fbc_cap(p);

        ParserCfg parser_cfg = {
            coding,
            frame_slots,
            packet_slots,
            dec_cfg,
            p->hw_info,
        };

        ret = mpp_parser_init(&parser, &parser_cfg);
        if (ret) {
            mpp_err_f("could not init parser\n");
            break;
        }

        ret = hal_info_init(&p->hal_info, MPP_CTX_DEC, coding);
        if (ret) {
            mpp_err_f("could not init hal info\n");
            break;
        }

        p->coding = coding;
        p->parser = parser;
        p->hal    = hal;
        p->tasks  = tasks;
        p->frame_slots  = frame_slots;
        p->packet_slots = packet_slots;

        p->statistics_en = (mpp_dec_debug & MPP_DEC_DBG_TIMING) ? 1 : 0;

        for (i = 0; i < DEC_TIMING_BUTT; i++) {
            p->clocks[i] = mpp_clock_get(timing_str[i]);
            mpp_assert(p->clocks[i]);
            mpp_clock_enable(p->clocks[i], p->statistics_en);
        }

        p->cmd_lock = new MppMutexCond();
        sem_init(&p->parser_reset, 0, 0);
        sem_init(&p->hal_reset, 0, 0);
        sem_init(&p->cmd_start, 0, 0);
        sem_init(&p->cmd_done, 0, 0);

        if (p->cfg.base.disable_thread) {
            DecTask *task = mpp_calloc(DecTask, 1);

            mpp_assert(task);

            p->task_single = task;
            dec_task_info_init(&task->info);

            p->mode = MPP_DEC_MODE_NO_THREAD;
        }

        p->api = dec_api[p->mode];

        // init timestamp for record and sort pts
        mpp_spinlock_init(&p->ts_lock);
        INIT_LIST_HEAD(&p->ts_link);
        p->ts_pool = mpp_mem_pool_init(sizeof(MppPktTs));
        if (!p->ts_pool) {
            mpp_err_f("malloc ts pool failed!\n");
            break;
        }

        *dec = p;
        dec_dbg_func("%p out\n", p);
        return MPP_OK;
    } while (0);

    mpp_dec_deinit(p);
    return MPP_NOK;
}

MPP_RET mpp_dec_deinit(MppDec ctx)
{
    RK_S32 i;
    MppDecImpl *dec = (MppDecImpl *)ctx;

    dec_dbg_func("%p in\n", dec);
    if (NULL == dec) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (dec->statistics_en) {
        mpp_log("%p work %lu wait %lu\n", dec,
                dec->parser_work_count, dec->parser_wait_count);

        for (i = 0; i < DEC_TIMING_BUTT; i++) {
            MppClock timer = dec->clocks[i];
            RK_S32 base = (i < DEC_HAL_TOTAL) ? (DEC_PRS_TOTAL) : (DEC_HAL_TOTAL);
            RK_S64 time = mpp_clock_get_sum(timer);
            RK_S64 total = mpp_clock_get_sum(dec->clocks[base]);

            if (!time || !total)
                continue;

            mpp_log("%p %s - %6.2f %-12lld avg %-12lld\n", dec,
                    mpp_clock_get_name(timer), time * 100.0 / total, time,
                    time / mpp_clock_get_count(timer));
        }
    }

    for (i = 0; i < DEC_TIMING_BUTT; i++) {
        mpp_clock_put(dec->clocks[i]);
        dec->clocks[i] = NULL;
    }

    if (dec->hal_info) {
        hal_info_deinit(dec->hal_info);
        dec->hal_info = NULL;
    }

    if (dec->parser) {
        mpp_parser_deinit(dec->parser);
        dec->parser = NULL;
    }

    if (dec->tasks) {
        hal_task_group_deinit(dec->tasks);
        dec->tasks = NULL;
    }

    if (dec->hal) {
        mpp_hal_deinit(dec->hal);
        dec->hal = NULL;
    }

    if (dec->vproc) {
        dec_vproc_deinit(dec->vproc);
        dec->vproc = NULL;
    }

    if (dec->frame_slots) {
        mpp_buf_slot_deinit(dec->frame_slots);
        dec->frame_slots = NULL;
    }

    if (dec->packet_slots) {
        mpp_buf_slot_deinit(dec->packet_slots);
        dec->packet_slots = NULL;
    }

    if (dec->cmd_lock) {
        delete dec->cmd_lock;
        dec->cmd_lock = NULL;
    }

    sem_destroy(&dec->parser_reset);
    sem_destroy(&dec->hal_reset);
    sem_destroy(&dec->cmd_start);
    sem_destroy(&dec->cmd_done);

    if (dec->ts_pool) {
        mpp_mem_pool_deinit(dec->ts_pool);
        dec->ts_pool = NULL;
    }

    MPP_FREE(dec->task_single);
    mpp_free(dec);
    dec_dbg_func("%p out\n", dec);
    return MPP_OK;
}

MPP_RET mpp_dec_start(MppDec ctx)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;
    MPP_RET ret = MPP_OK;

    dec_dbg_func("%p in\n", dec);

    if (dec->api && dec->api->start)
        ret = dec->api->start(dec);

    dec_dbg_func("%p out ret %d\n", dec, ret);
    return ret;
}

MPP_RET mpp_dec_stop(MppDec ctx)
{
    MPP_RET ret = MPP_OK;
    MppDecImpl *dec = (MppDecImpl *)ctx;

    dec_dbg_func("%p in\n", dec);

    if (dec->thread_parser)
        dec->thread_parser->stop();

    if (dec->thread_hal)
        dec->thread_hal->stop();

    if (dec->thread_parser) {
        delete dec->thread_parser;
        dec->thread_parser = NULL;
    }

    if (dec->thread_hal) {
        delete dec->thread_hal;
        dec->thread_hal = NULL;
    }

    dec_dbg_func("%p out\n", dec);
    return ret;
}

MPP_RET mpp_dec_reset(MppDec ctx)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;
    MPP_RET ret = MPP_OK;

    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    dec_dbg_func("%p in\n", dec);

    if (dec->api && dec->api->reset)
        ret = dec->api->reset(dec);

    dec_dbg_func("%p out ret %d\n", dec, ret);

    return ret;
}

MPP_RET mpp_dec_flush(MppDec ctx)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;

    dec_dbg_func("%p in\n", dec);
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    mpp_parser_flush(dec->parser);
    mpp_hal_flush(dec->hal);

    dec_dbg_func("%p out\n", dec);
    return MPP_OK;
}

MPP_RET mpp_dec_notify(MppDec ctx, RK_U32 flag)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;
    MPP_RET ret = MPP_OK;

    dec_dbg_func("%p in flag %08x\n", dec, flag);

    if (dec->vproc && (flag & MPP_DEC_NOTIFY_BUFFER_MATCH))
        dec_vproc_signal(dec->vproc);

    if (dec->api && dec->api->notify)
        ret = dec->api->notify(dec, flag);

    dec_dbg_func("%p out ret %d\n", dec, ret);

    return ret;
}

MPP_RET mpp_dec_callback(MppDec ctx, MppDecEvent event, void *arg)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;
    MppDecCbCfg *cb = &dec->cfg.cb;
    Mpp *mpp = (Mpp *)dec->mpp;
    MPP_RET ret = MPP_OK;

    switch (event) {
    case MPP_DEC_EVENT_ON_PKT_RELEASE : {
        if (cb->pkt_rdy_cb)
            ret = cb->pkt_rdy_cb(cb->pkt_rdy_ctx, mpp->mCtx, cb->pkt_rdy_cmd, arg);
    } break;
    case MPP_DEC_EVENT_ON_FRM_READY : {
        if (cb->frm_rdy_cb)
            ret = cb->frm_rdy_cb(cb->frm_rdy_ctx, mpp->mCtx, cb->frm_rdy_cmd, arg);
    } break;
    default : {
    } break;
    }

    return ret;
}

MPP_RET mpp_dec_control(MppDec ctx, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    MppDecImpl *dec = (MppDecImpl *)ctx;

    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    dec_dbg_func("%p in %08x %p\n", dec, cmd, param);
    dec_dbg_detail("detail: %p control cmd %08x param %p start\n", dec, cmd, param);

    if (dec->api && dec->api->control)
        ret = dec->api->control(dec, cmd, param);

    dec_dbg_detail("detail: %p control cmd %08x param %p ret %d\n", dec, cmd, param, ret);
    dec_dbg_func("%p out ret %d\n", dec, ret);

    return ret;
}

MPP_RET mpp_dec_set_cfg_by_cmd(MppDecCfgSet *set, MpiCmd cmd, void *param)
{
    MppDecBaseCfg *cfg = &set->base;
    MPP_RET ret = MPP_OK;

    switch (cmd) {
    case MPP_DEC_SET_PRESENT_TIME_ORDER : {
        cfg->sort_pts = (param) ? (*((RK_U32 *)param)) : (1);
        cfg->change |= MPP_DEC_CFG_CHANGE_SORT_PTS;
        dec_dbg_func("sort time order %d\n", cfg->sort_pts);
    } break;
    case MPP_DEC_SET_PARSER_SPLIT_MODE : {
        cfg->split_parse = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_SPLIT_PARSE;
        dec_dbg_func("split parse mode %d\n", cfg->split_parse);
    } break;
    case MPP_DEC_SET_PARSER_FAST_MODE : {
        cfg->fast_parse = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_FAST_PARSE;
        dec_dbg_func("fast parse mode %d\n", cfg->fast_parse);
    } break;
    case MPP_DEC_SET_OUTPUT_FORMAT : {
        cfg->out_fmt = (param) ? (*((MppFrameFormat *)param)) : (MPP_FMT_YUV420SP);
        cfg->change |= MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT;
    } break;
    case MPP_DEC_SET_DISABLE_ERROR: {
        cfg->disable_error = (param) ? (*((RK_U32 *)param)) : (1);
        cfg->change |= MPP_DEC_CFG_CHANGE_DISABLE_ERROR;
        dec_dbg_func("disable error %d\n", cfg->disable_error);
    } break;
    case MPP_DEC_SET_DIS_ERR_CLR_MARK: {
        cfg->dis_err_clr_mark = (param) ? (*((RK_U32 *)param)) : (1);
        cfg->change |= MPP_DEC_CFG_CHANGE_DIS_ERR_CLR_MARK;
        dec_dbg_func("disable error not mark%d\n", cfg->dis_err_clr_mark);
    } break;
    case MPP_DEC_SET_IMMEDIATE_OUT : {
        cfg->fast_out = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_FAST_OUT;
        dec_dbg_func("fast output mode %d\n", cfg->fast_out);
    } break;
    case MPP_DEC_SET_ENABLE_DEINTERLACE: {
        cfg->enable_vproc = (param) ? (*((RK_U32 *)param)) : MPP_VPROC_MODE_DEINTELACE;
        cfg->change |= MPP_DEC_CFG_CHANGE_ENABLE_VPROC;
        dec_dbg_func("enable dec_vproc %x\n", cfg->enable_vproc);
    } break;
    case MPP_DEC_SET_ENABLE_FAST_PLAY : {
        cfg->enable_fast_play = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY;
        dec_dbg_func("disable idr immediately output %d\n", cfg->enable_fast_play);
    } break;
    case MPP_DEC_SET_ENABLE_MVC : {
        cfg->enable_mvc = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_ENABLE_MVC;
        dec_dbg_func("enable MVC decoder %d\n", cfg->enable_mvc);
    } break;
    case MPP_DEC_SET_DISABLE_DPB_CHECK : {
        cfg->disable_dpb_chk = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_DISABLE_DPB_CHECK;
        dec_dbg_func("disable dpb discontinuous check %d\n", cfg->disable_dpb_chk);
    } break;
    case MPP_DEC_SET_CODEC_MODE : {
        cfg->codec_mode = (param) ? (*((RK_U32 *)param)) : (0);
        cfg->change |= MPP_DEC_CFG_CHANGE_CODEC_MODE;
        dec_dbg_func("force use codec device %d\n", cfg->codec_mode);
    } break;
    default : {
        mpp_err_f("unsupported cfg update cmd %x\n", cmd);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}
