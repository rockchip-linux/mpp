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

#define  MODULE_TAG "mpp_enc"

#include "string.h"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp_packet_impl.h"

#include "mpp.h"
#include "mpp_enc_impl.h"
#include "mpp_hal.h"
#include "hal_h264e_api.h"

#define MPP_ENC_DBG_FUNCTION            (0x00000001)
#define MPP_ENC_DBG_CONTROL             (0x00000002)
#define MPP_ENC_DBG_STATUS              (0x00000010)
#define MPP_ENC_DBG_DETAIL              (0x00000020)
#define MPP_ENC_DBG_RESET               (0x00000040)
#define MPP_ENC_DBG_NOTIFY              (0x00000080)

RK_U32 mpp_enc_debug = 0;

#define mpp_enc_dbg(flag, fmt, ...)     _mpp_dbg(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)
#define mpp_enc_dbg_f(flag, fmt, ...)   _mpp_dbg_f(mpp_enc_debug, flag, fmt, ## __VA_ARGS__)

#define enc_dbg_func(fmt, ...)          mpp_enc_dbg_f(MPP_ENC_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define enc_dbg_ctrl(fmt, ...)          mpp_enc_dbg_f(MPP_ENC_DBG_CONTROL, fmt, ## __VA_ARGS__)
#define enc_dbg_status(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_STATUS, fmt, ## __VA_ARGS__)
#define enc_dbg_detail(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_DETAIL, fmt, ## __VA_ARGS__)
#define enc_dbg_notify(fmt, ...)        mpp_enc_dbg_f(MPP_ENC_DBG_NOTIFY, fmt, ## __VA_ARGS__)

typedef struct MppEncImpl_t {
    MppCodingType       coding;
    EncImpl             impl;
    MppHal              hal;

    MppThread           *thread_enc;
    void                *mpp;

    // common resource
    MppBufSlots         frame_slots;
    MppBufSlots         packet_slots;
    HalTaskGroup        tasks;

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

void *mpp_enc_control_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEncImpl *enc = (MppEncImpl *)mpp->mEnc;
    MppHal hal = enc->hal;
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

            {
                AutoMutex auto_lock(&enc->lock);
                ret = enc_impl_proc_hal(enc->impl, hal_task);
                if (ret) {
                    mpp_err("mpp %p enc_impl_proc_hal failed return %d", mpp, ret);
                    goto TASK_END;
                }
            }

            enc_dbg_detail("mpp_hal_reg_gen  hal %p task %p\n", hal, task_info);
            ret = mpp_hal_reg_gen(hal, task_info);
            if (ret) {
                mpp_err("mpp %p hal_reg_gen failed return %d", mpp, ret);
                goto TASK_END;
            }
            enc_dbg_detail("mpp_hal_hw_start hal %p task %p\n", hal, task_info);
            ret = mpp_hal_hw_start(hal, task_info);
            if (ret) {
                mpp_err("mpp %p hal_hw_start failed return %d", mpp, ret);
                goto TASK_END;
            }
            enc_dbg_detail("mpp_hal_hw_wait  hal %p task %p\n", hal, task_info);
            ret = mpp_hal_hw_wait(hal, task_info);
            if (ret) {
                mpp_err("mpp %p hal_hw_wait failed return %d", mpp, ret);
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

MPP_RET mpp_enc_init(MppEnc *enc, MppEncCfg *cfg)
{
    MPP_RET ret;
    MppCodingType coding = cfg->coding;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    EncImpl impl = NULL;
    MppHal hal = NULL;
    MppEncImpl *p = NULL;
    RK_S32 task_count = 2;
    IOInterruptCB cb = {NULL, NULL};

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

    do {
        ret = mpp_buf_slot_init(&frame_slots);
        if (ret) {
            mpp_err_f("could not init frame buffer slot\n");
            break;
        }

        ret = mpp_buf_slot_init(&packet_slots);
        if (ret) {
            mpp_err_f("could not init packet buffer slot\n");
            break;
        }

        mpp_buf_slot_setup(packet_slots, task_count);

        EncImplCfg ctrl_cfg = {
            coding,
            &p->cfg,
            &p->set,
            task_count,
        };

        ret = enc_impl_init(&impl, &ctrl_cfg);
        if (ret) {
            mpp_err_f("could not init impl\n");
            break;
        }
        cb.callBack = hal_enc_callback;
        cb.opaque = impl;
        // then init hal with task count from impl
        MppHalCfg hal_cfg = {
            MPP_CTX_ENC,
            coding,
            HAL_MODE_LIBVPU,
            DEV_VEPU,
            frame_slots,
            packet_slots,
            &p->cfg,
            &p->set,
            NULL,
            1/*ctrl_cfg.task_count*/,  // TODO
            0,
            cb,
        };

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        p->coding       = coding;
        p->impl         = impl;
        p->hal          = hal;
        p->mpp          = cfg->mpp;
        p->tasks        = hal_cfg.tasks;
        p->frame_slots  = frame_slots;
        p->packet_slots = packet_slots;

        sem_init(&p->enc_reset, 0, 0);

        *enc = p;
        return MPP_OK;
    } while (0);

    mpp_enc_deinit(p);
    return MPP_NOK;

}

MPP_RET mpp_enc_deinit(MppEnc ctx)
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

    if (enc->hal) {
        mpp_hal_deinit(enc->hal);
        enc->hal = NULL;
    }

    if (enc->frame_slots) {
        mpp_buf_slot_deinit(enc->frame_slots);
        enc->frame_slots = NULL;
    }

    if (enc->packet_slots) {
        mpp_buf_slot_deinit(enc->packet_slots);
        enc->packet_slots = NULL;
    }

    sem_destroy(&enc->enc_reset);

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_start(MppEnc ctx)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in\n", enc);

    enc->thread_enc = new MppThread(mpp_enc_control_thread,
                                    enc->mpp, "mpp_enc_ctrl");
    enc->thread_enc->start();

    enc_dbg_func("%p out\n", enc);

    return MPP_OK;
}

MPP_RET mpp_enc_stop(MppEnc ctx)
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

MPP_RET mpp_enc_reset(MppEnc ctx)
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
    mpp_enc_notify(enc, MPP_ENC_RESET);
    thd->unlock(THREAD_CONTROL);
    sem_wait(&enc->enc_reset);
    mpp_assert(enc->reset_flag == 0);

    return MPP_OK;
}

MPP_RET mpp_enc_notify(MppEnc ctx, RK_U32 flag)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    enc_dbg_func("%p in flag %08x\n", enc, flag);
    MppThread *thd  = enc->thread_enc;

    thd->lock();
    {
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
void mpp_enc_update_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src);
void mpp_enc_update_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src);

MPP_RET mpp_enc_control(MppEnc ctx, MpiCmd cmd, void *param)
{
    MppEncImpl *enc = (MppEncImpl *)ctx;

    if (NULL == enc || (NULL == param && cmd != MPP_ENC_SET_IDR_FRAME)) {
        mpp_err_f("found NULL input enc %p cmd %x param %d\n", enc, cmd, param);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    AutoMutex auto_lock(&enc->lock);

    switch (cmd) {
    case MPP_ENC_SET_ALL_CFG : {
        enc_dbg_ctrl("set all config\n");
        memcpy(&enc->set, param, sizeof(enc->set));

        ret = enc_impl_proc_cfg(enc->impl, MPP_ENC_SET_RC_CFG, param);
        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_RC_CFG, &enc->set.rc);

        if (!ret)
            mpp_enc_update_rc_cfg(&enc->cfg.rc, &enc->set.rc);

        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_PREP_CFG, &enc->set.prep);

        if (!ret)
            ret = mpp_hal_control(enc->hal, MPP_ENC_SET_CODEC_CFG, &enc->set.codec);
    } break;
    case MPP_ENC_GET_ALL_CFG : {
        MppEncCfgSet *p = (MppEncCfgSet *)param;

        enc_dbg_ctrl("get all config\n");
        memcpy(p, &enc->cfg, sizeof(*p));
    } break;
    case MPP_ENC_SET_PREP_CFG : {
        enc_dbg_ctrl("set prep config\n");
        memcpy(&enc->set.prep, param, sizeof(enc->set.prep));

        ret = mpp_hal_control(enc->hal, cmd, param);
        if (!ret)
            mpp_enc_update_prep_cfg(&enc->cfg.prep, &enc->set.prep);
    } break;
    case MPP_ENC_GET_PREP_CFG : {
        MppEncPrepCfg *p = (MppEncPrepCfg *)param;

        enc_dbg_ctrl("get prep config\n");
        memcpy(p, &enc->cfg.prep, sizeof(*p));
    } break;
    case MPP_ENC_SET_RC_CFG : {
        enc_dbg_ctrl("set rc config\n");
        memcpy(&enc->set.rc, param, sizeof(enc->set.rc));

        ret = enc_impl_proc_cfg(enc->impl, cmd, param);
        if (!ret)
            ret = mpp_hal_control(enc->hal, cmd, param);

        if (!ret)
            mpp_enc_update_rc_cfg(&enc->cfg.rc, &enc->set.rc);
    } break;
    case MPP_ENC_GET_RC_CFG : {
        MppEncRcCfg *p = (MppEncRcCfg *)param;

        enc_dbg_ctrl("get rc config\n");
        memcpy(p, &enc->cfg.rc, sizeof(*p));
    } break;
    case MPP_ENC_SET_CODEC_CFG : {
        enc_dbg_ctrl("set codec config\n");
        memcpy(&enc->set.codec, param, sizeof(enc->set.codec));

        ret = mpp_hal_control(enc->hal, cmd, param);
        /* NOTE: codec information will be update by encoder hal */
    } break;
    case MPP_ENC_GET_CODEC_CFG : {
        MppEncCodecCfg *p = (MppEncCodecCfg *)param;

        enc_dbg_ctrl("get codec config\n");
        memcpy(p, &enc->cfg.codec, sizeof(*p));
    } break;

    case MPP_ENC_SET_IDR_FRAME : {
        enc_dbg_ctrl("idr request\n");
        ret = enc_impl_proc_cfg(enc->impl, cmd, param);
    } break;
    case MPP_ENC_GET_EXTRA_INFO : {
        enc_dbg_ctrl("get extra info\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_OSD_PLT_CFG : {
        enc_dbg_ctrl("set osd plt\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_OSD_DATA_CFG : {
        enc_dbg_ctrl("set osd data\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_ROI_CFG : {
        enc_dbg_ctrl("set roi data\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_SEI_CFG : {
        enc_dbg_ctrl("set sei\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_GET_SEI_DATA : {
        enc_dbg_ctrl("get sei\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_PRE_ALLOC_BUFF : {
        enc_dbg_ctrl("pre alloc buff\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_QP_RANGE : {
        enc_dbg_ctrl("set qp range\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    case MPP_ENC_SET_CTU_QP: {
        enc_dbg_ctrl("set ctu qp\n");
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    default : {
        mpp_log_f("unsupported cmd id %08x param %p\n", cmd, param);
        ret = MPP_NOK;
    } break;
    }

    return ret;
}

void mpp_enc_update_prep_cfg(MppEncPrepCfg *dst, MppEncPrepCfg *src)
{
    RK_U32 change = src->change;

    if (change) {

        if (change & MPP_ENC_PREP_CFG_CHANGE_FORMAT)
            dst->format = src->format;

        if (change & MPP_ENC_PREP_CFG_CHANGE_ROTATION)
            dst->rotation = src->rotation;

        if (change & MPP_ENC_PREP_CFG_CHANGE_MIRRORING)
            dst->mirroring = src->mirroring;

        if (change & MPP_ENC_PREP_CFG_CHANGE_DENOISE)
            dst->denoise = src->denoise;

        if (change & MPP_ENC_PREP_CFG_CHANGE_SHARPEN)
            dst->sharpen = src->sharpen;

        if (change & MPP_ENC_PREP_CFG_CHANGE_INPUT) {
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

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    }
}

void mpp_enc_update_rc_cfg(MppEncRcCfg *dst, MppEncRcCfg *src)
{
    RK_U32 change = src->change;
    if (change) {
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

        if (change & MPP_ENC_RC_CFG_CHANGE_SKIP_CNT)
            dst->skip_cnt = src->skip_cnt;

        /*
         * NOTE: use OR here for avoiding overwrite on multiple config
         * When next encoding is trigger the change flag will be clear
         */
        dst->change |= change;
        src->change = 0;
    }
}

