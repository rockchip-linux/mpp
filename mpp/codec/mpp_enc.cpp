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

#include "mpp_log.h"
#include "mpp_mem.h"

#include "mpp.h"
#include "mpp_enc.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

void *mpp_enc_control_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *thd_enc = mpp->mThreadCodec;
    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask task = NULL;
    MPP_RET ret = MPP_OK;

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        thd_enc->lock();
        ret = mpp_port_dequeue(input, &task);
        if (ret || NULL == task)
            thd_enc->wait();
        thd_enc->unlock();

        if (task) {
            MppFrame frame = NULL;
            MppPacket packet = NULL;
            // task process here


            // enqueue task back to input input
            mpp_task_meta_get_frame (task, MPP_META_KEY_INPUT_FRM,  &frame);
            mpp_task_meta_get_packet(task, MPP_META_KEY_OUTPUT_PKT, &packet);

            mpp_port_enqueue(input, task);
            task = NULL;

            // send finished task to output port
            mpp_port_dequeue(output, &task);

            mpp_task_meta_set_frame(task,  MPP_META_KEY_INPUT_FRM,  frame);
            mpp_task_meta_set_packet(task, MPP_META_KEY_OUTPUT_PKT, packet);

            // setup output task here
            mpp_port_enqueue(output, task);
            task = NULL;
        }
    }
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
    MppEnc *p = mpp_calloc(MppEnc, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_NOK;
    MppHal hal = NULL;

    MppHalCfg hal_cfg = {
        MPP_CTX_ENC,
        coding,
        HAL_MODE_LIBVPU,
        HAL_RKVDEC,
        NULL,
        NULL,
        NULL,
        2,
        0,
        NULL,
    };

    ret = mpp_hal_init(&hal, &hal_cfg);
    if (ret) {
        mpp_err_f("could not init hal\n");
    }

    p->coding = coding;

    *enc = p;

    return MPP_OK;
}

MPP_RET mpp_enc_deinit(MppEnc *enc)
{
    if (NULL == enc) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = mpp_hal_deinit(enc->hal);
    if (ret) {
        mpp_err_f("mpp enc hal deinit failed\n");
    }
    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_reset(MppEnc *enc)
{
    (void)enc;
    return MPP_OK;
}


