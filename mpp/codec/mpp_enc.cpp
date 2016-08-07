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

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "hal_h264e_api.h"

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
            ret = mpp_task_meta_get_frame(mpp_task, MPP_META_KEY_INPUT_FRM,  &frame);
            if (frame) {
                mpp_frame_deinit(&frame);
                frame = NULL;
            }
            ret = mpp_task_meta_get_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, &packet);
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

void *mpp_enc_control_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppEnc *enc = mpp->mEnc;
    MppThread *thd_enc  = mpp->mThreadCodec;
    HalTaskInfo task_info;
    HalEncTask *enc_task = &task_info.enc;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask mpp_task = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;

    memset(&task_info, 0, sizeof(HalTaskInfo));

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        thd_enc->lock();
        ret = mpp_port_dequeue(input, &mpp_task);
        if (ret || NULL == mpp_task) {
            thd_enc->wait();
        }
        thd_enc->unlock();

        if (mpp_task != NULL) {
            mpp_task_meta_get_frame (mpp_task, MPP_META_KEY_INPUT_FRM,  &frame);
            mpp_task_meta_get_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, &packet);

            if (NULL == frame) {
                mpp_port_enqueue(input, mpp_task);
                continue;
            }

            reset_hal_enc_task(enc_task);

            if (mpp_frame_get_buffer(frame)) {
                /*
                 * if there is available buffer in the input frame do encoding
                 */
                if (NULL == packet) {
                    RK_U32 width  = enc->mpp_cfg.width;
                    RK_U32 height = enc->mpp_cfg.height;
                    RK_U32 size = width * height;
                    MppBuffer buffer = NULL;

                    mpp_buffer_get(mpp->mPacketGroup, &buffer, size);
                    mpp_log("create buffer size %d fd %d\n", size, mpp_buffer_get_fd(buffer));
                    mpp_packet_init_with_buffer(&packet, buffer);
                    mpp_buffer_put(buffer);
                }
                mpp_assert(packet);

                mpp_packet_set_pts(packet, mpp_frame_get_pts(frame));

                enc_task->input  = mpp_frame_get_buffer(frame);
                enc_task->output = mpp_packet_get_buffer(packet);
                controller_encode(mpp->mEnc->controller, enc_task);

                mpp_hal_reg_gen((mpp->mEnc->hal), &task_info);
                mpp_hal_hw_start((mpp->mEnc->hal), &task_info);
                mpp_hal_hw_wait((mpp->mEnc->hal), &task_info);

                RK_U32 outputStreamSize = 0;
                controller_config(mpp->mEnc->controller, GET_OUTPUT_STREAM_SIZE, (void*)&outputStreamSize);

                mpp_packet_set_length(packet, outputStreamSize);
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
            mpp_task_meta_set_frame(mpp_task, MPP_META_KEY_INPUT_FRM, frame);
            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            // send finished task to output port
            mpp_port_dequeue(output, &mpp_task);
            mpp_task_meta_set_packet(mpp_task, MPP_META_KEY_OUTPUT_PKT, packet);

            {
                RK_S32 is_intra = enc_task->is_intra;
                RK_U32 flag = mpp_packet_get_flag(packet);

                mpp_task_meta_set_s32(mpp_task, MPP_META_KEY_OUTPUT_INTRA, is_intra);
                if (is_intra) {
                    mpp_packet_set_flag(packet, flag | MPP_PACKET_FLAG_INTRA);
                }
            }

            // setup output task here
            mpp_port_enqueue(output, mpp_task);
            mpp_task = NULL;
            packet = NULL;
            frame = NULL;
        }
    }

    // clear remain task in output port
    release_task_in_port(input);
    release_task_in_port(mpp->mOutputPort);

    return NULL;
}

void *mpp_enc_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    mpp_list *frames    = mpp->mFrames;
    mpp_list *tasks     = mpp->mTasks;

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (0 == tasks->list_size())
            hal->wait();
        hal->unlock();

        // get_config
        // register genertation
        if (tasks->list_size()) {
            HalDecTask *task;
            tasks->lock();
            tasks->del_at_head(&task, sizeof(task));
            mpp->mTaskGetCount++;
            tasks->unlock();

            // hal->mpp_hal_reg_gen(current);

            /*
             * wait previous register set done
             */
            // hal->mpp_hal_hw_wait(previous);

            /*
             * send current register set to hardware
             */
            // hal->mpp_hal_hw_start(current);

            /*
             * mark previous buffer is complete
             */
            // change dpb slot status
            // signal()
            // mark frame in output queue
            // wait up output thread to get a output frame

            // for test
            MppBuffer buffer;
            mpp_buffer_get(mpp->mFrameGroup, &buffer, SZ_1M);

            MppFrame frame;
            mpp_frame_init(&frame);
            mpp_frame_set_buffer(frame, buffer);
            frames->lock();
            frames->add_at_tail(&frame, sizeof(frame));
            mpp->mFramePutCount++;
            frames->signal();
            frames->unlock();
        }
    }

    return NULL;
}

MPP_RET mpp_enc_init(MppEnc **enc, MppCodingType coding)
{
    MPP_RET ret;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    Controller controller = NULL;
    MppHal hal = NULL;
    MppEnc *p = NULL;
    RK_S32 task_count = 2;
    IOInterruptCB cb = {NULL, NULL};

    if (NULL == enc) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    *enc = NULL;

    p = mpp_calloc(MppEnc, 1);
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
        cb.callBack = mpp_enc_notify;
        cb.opaque = p;

        ControllerCfg controller_cfg = {
            coding,
            task_count,
            cb,
        };

        ret = controller_init(&controller, &controller_cfg);
        if (ret) {
            mpp_err_f("could not init parser\n");
            break;
        }
        cb.callBack = hal_enc_callback;
        cb.opaque = controller;
        // then init hal with task count from parser
        MppHalCfg hal_cfg = {
            MPP_CTX_ENC,
            coding,
            HAL_MODE_LIBVPU,
            HAL_VEPU,
            frame_slots,
            packet_slots,
            NULL,
            1/*controller_cfg.task_count*/,  // TODO
            0,
            cb,
        };

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        p->coding       = coding;
        p->controller   = controller;
        p->hal          = hal;
        p->tasks        = hal_cfg.tasks;
        p->frame_slots  = frame_slots;
        p->packet_slots = packet_slots;
        p->mpp_cfg.size = sizeof(p->mpp_cfg);
        *enc = p;
        return MPP_OK;
    } while (0);

    mpp_enc_deinit(p);
    return MPP_NOK;

}

MPP_RET mpp_enc_deinit(MppEnc *enc)
{
    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (enc->controller) {
        controller_deinit(enc->controller);
        enc->controller = NULL;
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

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_reset(MppEnc *enc)
{
    (void)enc;
    return MPP_OK;
}

MPP_RET mpp_enc_notify(void *ctx, void *info)
{
    // TODO
    (void)ctx;
    (void)info;
    return MPP_OK;
}

MPP_RET mpp_enc_control(MppEnc *enc, MpiCmd cmd, void *param)
{
    if (NULL == enc) {
        mpp_err_f("found NULL input enc %p\n", enc);
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_NOK;

    switch (cmd) {
    case MPP_ENC_SET_CFG : {
        MppEncConfig *mpp_cfg = &enc->mpp_cfg;
        void *extra_info_cfg = NULL;

        *mpp_cfg = *((MppEncConfig *)param);

        /* before set config to controller check it first */
        ret = controller_config(enc->controller, CHK_ENC_CFG, (void *)mpp_cfg);
        if (ret) {
            mpp_err("config check failed ret %d\n", ret);
            break;
        }
        controller_config(enc->controller, SET_ENC_CFG,     (void *)mpp_cfg);
        controller_config(enc->controller, SET_ENC_RC_CFG,  (void *)mpp_cfg);
        controller_config(enc->controller, GET_ENC_EXTRA_INFO,  (void *)&extra_info_cfg);

        ret = mpp_hal_control(enc->hal, MPP_ENC_SET_EXTRA_INFO, extra_info_cfg);

    } break;
    case MPP_ENC_GET_CFG : {
        MppEncConfig *mpp_cfg = (MppEncConfig *)param;

        mpp_assert(mpp_cfg->size == sizeof(enc->mpp_cfg));

        *mpp_cfg = enc->mpp_cfg;
        ret = MPP_OK;
    } break;
    case MPP_ENC_GET_EXTRA_INFO : {
        ret = mpp_hal_control(enc->hal, cmd, param);
    } break;
    default : {
    } break;
    }

    return ret;
}

