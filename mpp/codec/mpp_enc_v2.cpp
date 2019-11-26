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

#define  MODULE_TAG "mpp_enc_v2"

#include <string.h>

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_packet_impl.h"

#include "mpp.h"
#include "mpp_enc_debug.h"
#include "mpp_enc_impl.h"
#include "mpp_enc_hal.h"
#include "hal_h264e_api_v2.h"

RK_U32 mpp_enc_debug = 0;

typedef struct MppEncImpl_t {
    MppCodingType       coding;
    EncImpl             impl;
    MppEncHal           enc_hal;

    MppThread           *thread_enc;
    void                *mpp;

    // internal status and protection
    Mutex               lock;
    RK_U32              reset_flag;
    sem_t               enc_reset;

    RK_U32              wait_count;
    RK_U32              work_count;
    RK_U32              status_flag;
    RK_U32              notify_flag;

    /* Encoder configure set */
    MppEncCfgSet        cfg;
    MppEncCfgSet        set;

    /* control process */
    MpiCmd              cmd;
    void                *param;
    sem_t               enc_ctrl;

    // legacy support for MPP_ENC_GET_EXTRA_INFO
    MppPacket           hdr_pkt;

    RK_U32              cmd_send;
    RK_U32              cmd_recv;
    RK_U32              hdr_need_updated;
} MppEncImpl;

typedef union EncTaskWait_u {
    RK_U32          val;
    struct {
        RK_U32      enc_frm_in      : 1;   // 0x0001 MPP_ENC_NOTIFY_FRAME_ENQUEUE
        RK_U32      reserv0002      : 1;   // 0x0002
        RK_U32      reserv0004      : 1;   // 0x0004
        RK_U32      enc_pkt_out     : 1;   // 0x0008 MPP_ENC_NOTIFY_PACKET_ENQUEUE

        RK_U32      reserv0010      : 1;   // 0x0010
        RK_U32      reserv0020      : 1;   // 0x0020
        RK_U32      reserv0040      : 1;   // 0x0040
        RK_U32      reserv0080      : 1;   // 0x0080

        RK_U32      reserv0100      : 1;   // 0x0100
        RK_U32      reserv0200      : 1;   // 0x0200
        RK_U32      reserv0400      : 1;   // 0x0400
        RK_U32      reserv0800      : 1;   // 0x0800

        RK_U32      reserv1000      : 1;   // 0x1000
        RK_U32      reserv2000      : 1;   // 0x2000
        RK_U32      reserv4000      : 1;   // 0x4000
        RK_U32      reserv8000      : 1;   // 0x8000
    };
} EncTaskWait;

typedef union EncTaskStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_in_rdy     : 1;
        RK_U32      task_out_rdy    : 1;
    };
} EncTaskStatus;

typedef struct EncTask_t {
    EncTaskStatus   status;
    EncTaskWait     wait;
    HalTaskInfo     info;
} EncTask;

static void reset_hal_enc_task(HalEncTask *task)
{
    memset(task, 0, sizeof(*task));
}

static MPP_RET release_task_in_port(MppPort port)
{
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    MppTask mpp_task;

    do {
        ret = mpp_port_dequeue(port, &mpp_task);
        if (ret)
            break;

        if (mpp_task) {
            packet = NULL;
            frame = NULL;
            ret = mpp_task_meta_get_frame(mpp_task, KEY_INPUT_FRAME,  &frame);
            if (frame) {
                mpp_frame_deinit(&frame);
                frame = NULL;
            }
            ret = mpp_task_meta_get_packet(mpp_task, KEY_OUTPUT_PACKET, &packet);
            if (packet) {
                mpp_packet_deinit(&packet);
                packet = NULL;
            }

            mpp_port_enqueue(port, mpp_task);
            mpp_task = NULL;
        } else
            break;
    } while (1);

    return ret;
}

static MPP_RET check_enc_task_wait(MppEncImpl *enc, EncTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 notify = enc->notify_flag;
    RK_U32 last_wait = enc->status_flag;
    RK_U32 curr_wait = task->wait.val;
    RK_U32 wait_chg  = last_wait & (~curr_wait);

    do {
        if (enc->reset_flag)
            break;

        // NOTE: User control should always be processed
        if (notify & MPP_ENC_CONTROL)
            break;

        // NOTE: When condition is not fulfilled check nofify flag again
        if (!curr_wait || (curr_wait & notify))
            break;

        ret = MPP_NOK;
    } while (0);

    enc_dbg_status("%p %08x -> %08x [%08x] notify %08x -> %s\n", enc,
                   last_wait, curr_wait, wait_chg, notify, (ret) ? ("wait") : ("work"));

    enc->status_flag = task->wait.val;
    enc->notify_flag = 0;

    if (ret) {
        enc->wait_count++;
    } else {
        enc->work_count++;
    }

    return ret;
}

#define RUN_ENC_HAL_FUNC(func, hal, task, mpp, ret)             \
    ret = func(hal, task);                                      \
    if (ret) {                                                  \
        mpp_err("mpp %p ##func failed return %d", mpp, ret);    \
        goto TASK_END;                                          \
    }

void *mpp_enc_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    EncImpl impl = enc->impl;
    MppEncHal hal = enc->enc_hal;
    MppThread *thd_enc  = enc->thread_enc;
    EncTask task;
    HalTaskInfo *task_info = &task.info;
    HalEncTask *hal_task = &task_info->enc;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask task_in = NULL;
    MppTask task_out = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;
    MppBuffer mv_info = NULL;

    memset(&task, 0, sizeof(task));

    while (1) {
        {
            AutoMutex autolock(thd_enc->mutex());
            if (MPP_THREAD_RUNNING != thd_enc->get_status())
                break;

            if (check_enc_task_wait(enc, &task))
                thd_enc->wait();
        }

        // process control first
        if (enc->cmd_send != enc->cmd_recv) {
            if (enc->cmd == MPP_ENC_GET_HDR_SYNC ||
                enc->cmd == MPP_ENC_GET_EXTRA_INFO) {
                MppPacket pkt = NULL;

                /*
                 * NOTE: get stream header should use user's MppPacket
                 * If we provide internal MppPacket to external user
                 * we do not known when the buffer usage is finished.
                 * So encoder always write its header to external buffer
                 * which is provided by user.
                 */
                if (enc->cmd == MPP_ENC_GET_EXTRA_INFO) {
                    mpp_err("Please use MPP_ENC_GET_HDR_SYNC instead of unsafe MPP_ENC_GET_EXTRA_INFO\n");

                    if (NULL == enc->hdr_pkt) {
                        size_t size = SZ_1K;
                        void *ptr = mpp_calloc_size(void, size);

                        mpp_packet_init(&enc->hdr_pkt, ptr, size);
                    }
                    pkt = enc->hdr_pkt;
                    *(MppPacket *)enc->param = pkt;
                } else
                    pkt = (MppPacket)enc->param;

                enc_impl_gen_hdr(impl, pkt);

                enc->hdr_need_updated = 0;
            } else {
                enc_impl_proc_cfg(impl, enc->cmd, enc->param);
                if (MPP_ENC_SET_CODEC_CFG == enc->cmd ||
                    MPP_ENC_SET_PREP_CFG == enc->cmd)
                    enc->hdr_need_updated = 1;
            }

            sem_post(&enc->enc_ctrl);
            enc->cmd_recv++;
            continue;
        }

        if (enc->reset_flag) {
            {
                AutoMutex autolock(thd_enc->mutex());
                enc->status_flag = 0;
            }

            AutoMutex autolock(thd_enc->mutex(THREAD_CONTROL));
            enc->reset_flag = 0;
            sem_post(&enc->enc_reset);
            continue;
        }

        // 1. check task in
        if (!task.status.task_in_rdy) {
            ret = mpp_port_poll(input, MPP_POLL_NON_BLOCK);
            if (ret) {
                task.wait.enc_frm_in = 1;
                continue;
            }

            task.status.task_in_rdy = 1;
            task.wait.enc_frm_in = 0;
        }
        enc_dbg_detail("task in ready\n");

        // 2. get task out
        if (!task.status.task_out_rdy) {
            ret = mpp_port_poll(output, MPP_POLL_NON_BLOCK);
            if (ret) {
                task.wait.enc_pkt_out = 1;
                continue;
            }

            task.status.task_out_rdy = 1;
            task.wait.enc_pkt_out = 0;
        }
        enc_dbg_detail("task out ready\n");

        ret = mpp_port_dequeue(input, &task_in);
        mpp_assert(task_in);

        mpp_task_meta_get_frame (task_in, KEY_INPUT_FRAME,  &frame);
        mpp_task_meta_get_packet(task_in, KEY_OUTPUT_PACKET, &packet);
        mpp_task_meta_get_buffer(task_in, KEY_MOTION_INFO, &mv_info);

        if (NULL == frame) {
            mpp_port_enqueue(input, task_in);
            continue;
        }

        reset_hal_enc_task(hal_task);

        if (enc->hdr_need_updated) {
            enc_impl_gen_hdr(impl, NULL);
            enc->hdr_need_updated = 0;
        }

        if (mpp_frame_get_buffer(frame)) {
            /*
             * if there is available buffer in the input frame do encoding
             */
            if (NULL == packet) {
                RK_U32 width  = enc->cfg.prep.width;
                RK_U32 height = enc->cfg.prep.height;
                RK_U32 size = width * height;
                MppBuffer buffer = NULL;

                mpp_assert(size);
                mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
                mpp_packet_init_with_buffer(&packet, buffer);
                mpp_buffer_put(buffer);
            }
            mpp_assert(packet);

            mpp_packet_set_pts(packet, mpp_frame_get_pts(frame));

            hal_task->frame  = frame;
            hal_task->input  = mpp_frame_get_buffer(frame);
            hal_task->packet = packet;
            hal_task->output = mpp_packet_get_buffer(packet);
            hal_task->mv_info = mv_info;

            ret = enc_impl_start(impl);
            if (ret) {
                enc_dbg_detail("mpp %p enc_impl_start drop one frame", mpp);
                goto TASK_END;
            }

            ret = enc_impl_proc_dpb(impl, hal_task);
            if (ret) {
                mpp_err("mpp %p enc_impl_proc_dpb failed return %d", mpp, ret);
                goto TASK_END;
            }

            ret = enc_impl_proc_rc(impl, hal_task);
            if (ret) {
                mpp_err("mpp %p enc_impl_proc_rc failed return %d", mpp, ret);
                goto TASK_END;
            }

            ret = enc_impl_proc_hal(impl, hal_task);
            if (ret) {
                mpp_err("mpp %p enc_impl_proc_hal failed return %d", mpp, ret);
                goto TASK_END;
            }

            RUN_ENC_HAL_FUNC(mpp_enc_hal_get_task, hal, hal_task, mpp, ret);
            RUN_ENC_HAL_FUNC(mpp_enc_hal_gen_regs, hal, hal_task, mpp, ret);
            RUN_ENC_HAL_FUNC(mpp_enc_hal_start, hal, hal_task, mpp, ret);
            RUN_ENC_HAL_FUNC(mpp_enc_hal_wait,  hal, hal_task, mpp, ret);
            RUN_ENC_HAL_FUNC(mpp_enc_hal_ret_task, hal, hal_task, mpp, ret);

            ret = enc_impl_update_hal(impl, hal_task);
            if (ret) {
                mpp_err("mpp %p enc_impl_proc_hal failed return %d", mpp, ret);
                goto TASK_END;
            }

            ret = enc_impl_update_rc(impl, hal_task);
            if (ret) {
                mpp_err("mpp %p enc_impl_proc_rc failed return %d", mpp, ret);
                goto TASK_END;
            }

        TASK_END:
            mpp_packet_set_length(packet, hal_task->length);
        } else {
            /*
             * else init a empty packet for output
             */
            mpp_packet_new(&packet);
        }

        if (mpp_frame_get_eos(frame))
            mpp_packet_set_eos(packet);

        /*
         * first clear output packet
         * then enqueue task back to input port
         * final user will release the mpp_frame they had input
         */
        mpp_task_meta_set_frame(task_in, KEY_INPUT_FRAME, frame);
        mpp_port_enqueue(input, task_in);

        // send finished task to output port
        ret = mpp_port_dequeue(output, &task_out);

        /*
         * task_in may be null if output port is awaken by Mpp::clear()
         */
        if (task_out) {
            //set motion info buffer to output task
            if (mv_info)
                mpp_task_meta_set_buffer(task_out, KEY_MOTION_INFO, mv_info);

            mpp_task_meta_set_packet(task_out, KEY_OUTPUT_PACKET, packet);

            {
                RK_S32 is_intra = hal_task->is_intra;
                RK_U32 flag = mpp_packet_get_flag(packet);

                mpp_task_meta_set_s32(task_out, KEY_OUTPUT_INTRA, is_intra);
                if (is_intra) {
                    mpp_packet_set_flag(packet, flag | MPP_PACKET_FLAG_INTRA);
                }
            }

            // setup output task here
            mpp_port_enqueue(output, task_out);
        } else {
            mpp_packet_deinit(&packet);
        }

        task_in = NULL;
        task_out = NULL;
        packet = NULL;
        frame = NULL;

        task.status.val = 0;
    }

    // clear remain task in output port
    release_task_in_port(input);
    release_task_in_port(mpp->mOutputPort);

    return NULL;
}

MPP_RET mpp_enc_init_v2(MppEnc *enc, MppEncCfg *cfg)
{
    MPP_RET ret;
    MppCodingType coding = cfg->coding;
    EncImpl impl = NULL;
    MppEncImpl *p = NULL;

    mpp_env_get_u32("mpp_enc_debug", &mpp_enc_debug, 0);

    if (NULL == enc) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    *enc = NULL;

    p = mpp_calloc(MppEncImpl, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    // H.264 encoder use mpp_enc_hal path
    // create hal first
    MppEncHal enc_hal = NULL;
    MppEncHalCfg enc_hal_cfg = {
        coding,
        &p->cfg,
        &p->set,
        HAL_MODE_LIBVPU,
        DEV_VEPU,
    };
    // then create enc_impl
    EncImplCfg ctrl_cfg = {
        coding,
        DEV_VEPU,
        &p->cfg,
        &p->set,
        2,
    };

    ret = mpp_enc_hal_init(&enc_hal, &enc_hal_cfg);
    if (ret) {
        mpp_err_f("could not init enc hal\n");
        goto ERR_RET;
    }

    ctrl_cfg.dev_id = enc_hal_cfg.device_id;
    ctrl_cfg.task_count = -1;

    ret = enc_impl_init(&impl, &ctrl_cfg);
    if (ret) {
        mpp_err_f("could not init impl\n");
        goto ERR_RET;
    }

    p->coding   = coding;
    p->impl     = impl;
    p->enc_hal  = enc_hal;
    p->mpp      = cfg->mpp;

    sem_init(&p->enc_reset, 0, 0);
    sem_init(&p->enc_ctrl, 0, 0);

    *enc = p;
    return ret;
ERR_RET:
    mpp_enc_deinit_v2(p);
    return ret;
}

MPP_RET mpp_enc_deinit_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (enc->impl) {
        enc_impl_deinit(enc->impl);
        enc->impl = NULL;
    }

    if (enc->enc_hal) {
        mpp_enc_hal_deinit(enc->enc_hal);
        enc->enc_hal = NULL;
    }

    if (enc->hdr_pkt)
        mpp_packet_deinit(&enc->hdr_pkt);

    sem_destroy(&enc->enc_reset);
    sem_destroy(&enc->enc_ctrl);

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_start_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    enc->thread_enc = new MppThread(mpp_enc_thread,
                                    enc->mpp, "mpp_enc");
    enc->thread_enc->start();

    enc_dbg_func("%p out\n", enc);

    return MPP_OK;
}

MPP_RET mpp_enc_stop_v2(MppEnc ctx)
{
    MPP_RET ret = MPP_OK;
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    if (enc->thread_enc) {
        enc->thread_enc->stop();
        delete enc->thread_enc;
        enc->thread_enc = NULL;
    }

    enc_dbg_func("%p out\n", enc);
    return ret;

}

MPP_RET mpp_enc_reset_v2(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);
    if (NULL == enc) {
        mpp_err_f("found NULL input enc\n");
        return MPP_ERR_NULL_PTR;
    }

    MppThread *thd = enc->thread_enc;

    thd->lock(THREAD_CONTROL);
    enc->reset_flag = 1;
    mpp_enc_notify_v2(enc, MPP_ENC_RESET);
    thd->unlock(THREAD_CONTROL);
    sem_wait(&enc->enc_reset);
    mpp_assert(enc->reset_flag == 0);

    return MPP_OK;
}

MPP_RET mpp_enc_notify_v2(MppEnc ctx, RK_U32 flag)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in flag %08x\n", enc, flag);
    MppThread *thd  = enc->thread_enc;

    thd->lock();
    if (flag == MPP_ENC_CONTROL) {
        enc->notify_flag |= flag;
        enc_dbg_notify("%p status %08x notify control signal\n", enc,
                       enc->status_flag);
        thd->signal();
    } else {
        RK_U32 old_flag = enc->notify_flag;

        enc->notify_flag |= flag;
        if ((old_flag != enc->notify_flag) &&
            (enc->notify_flag & enc->status_flag)) {
            enc_dbg_notify("%p status %08x notify %08x signal\n", enc,
                           enc->status_flag, enc->notify_flag);
            thd->signal();
        }
    }
    thd->unlock();
    enc_dbg_func("%p out\n", enc);
    return MPP_OK;
}

/*
 * preprocess config and rate-control config is common config then they will
 * be done in mpp_enc layer
 *
 * codec related config will be set in each hal component
 */
MPP_RET mpp_enc_control_v2(MppEnc ctx, MpiCmd cmd, void *param)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc || (NULL == param && cmd != MPP_ENC_SET_IDR_FRAME)) {
        mpp_err_f("found NULL input enc %p cmd %x param %d\n", enc, cmd, param);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    AutoMutex auto_lock(&enc->lock);

    enc_dbg_ctrl("sending cmd %d param %p\n", cmd, param);

    switch (cmd) {
    case MPP_ENC_GET_ALL_CFG : {
        enc_dbg_ctrl("get all config\n");
        memcpy(param, &enc->cfg, sizeof(enc->cfg));
    } break;
    case MPP_ENC_GET_PREP_CFG : {
        enc_dbg_ctrl("get prep config\n");
        memcpy(param, &enc->cfg.prep, sizeof(enc->cfg.prep));
    } break;
    case MPP_ENC_GET_RC_CFG : {
        enc_dbg_ctrl("get rc config\n");
        memcpy(param, &enc->cfg.rc, sizeof(enc->cfg.rc));
    } break;
    default : {
        // Cmd which is not get configure will handle by enc_impl
        enc->cmd    = cmd;
        enc->param  = param;

        enc->cmd_send++;
        mpp_enc_notify_v2(ctx, MPP_ENC_CONTROL);
        sem_wait(&enc->enc_ctrl);
    } break;
    }

    enc_dbg_ctrl("sending cmd %d done\n", cmd);
    return ret;
}
