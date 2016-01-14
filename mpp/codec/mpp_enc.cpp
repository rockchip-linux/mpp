/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
    MppThread *thd_enc  = mpp->mThreadCodec;
    mpp_list *packets = mpp->mPackets;
    mpp_list *frames  = mpp->mFrames;
    MppFrameImpl frame;
    MppPacket packet;
    size_t size = SZ_1M;
    char *buf = mpp_malloc(char, size);

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        AutoMutex auto_lock(frames->mutex());
        if (frames->list_size()) {
            frames->del_at_head(&frame, sizeof(frame));

            mpp_packet_init(&packet, buf, size);
            packets->lock();
            packets->add_at_tail(&packet, sizeof(packet));
            packets->signal();
            packets->unlock();
        }
    }
    mpp_free(buf);
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

    mpp_free(enc);
    return MPP_OK;
}

MPP_RET mpp_enc_reset(MppEnc *enc)
{
    (void)enc;
    return MPP_OK;
}


