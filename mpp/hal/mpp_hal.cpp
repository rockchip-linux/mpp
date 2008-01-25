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

#define  MODULE_TAG "mpp_hal"

#include "mpp_mem.h"
#include "mpp_log.h"

#include "mpp.h"
#include "mpp_hal.h"
#include "mpp_frame_impl.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M

void *mpp_hal_thread(void *data)
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
            MppHalDecTask *task;
            mpp->mTasks->del_at_head(&task, sizeof(task));
            mpp->mTaskGetCount++;

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
            mpp_buffer_get(mpp->mFrameGroup, &buffer, MPP_TEST_FRAME_SIZE);

            MppFrame frame;
            mpp_frame_init(&frame);
            mpp_frame_set_buffer(frame, buffer);
            frames->add_at_tail(&frame, sizeof(frame));
            mpp->mFramePutCount++;
        }
    }

    return NULL;
}

MPP_RET mpp_hal_init(MppHal **ctx, MppHalCfg *cfg)
{
    *ctx = mpp_malloc(MppHal, 1);
    (void)cfg;
    return MPP_OK;
}

MPP_RET mpp_hal_deinit(MppHal *ctx)
{
    mpp_free(ctx);
    return MPP_OK;
}

MPP_RET mpp_hal_reg_gen(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_start(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}

MPP_RET mpp_hal_hw_wait(MppHal *ctx, MppHalDecTask *task)
{
    (void)ctx;
    (void)task;
    return MPP_OK;
}


