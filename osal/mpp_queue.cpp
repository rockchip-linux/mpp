/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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

#include "mpp_err.h"
#include "mpp_queue.h"

MppQueue::MppQueue(node_destructor func)
    : mpp_list(func), mFlushFlag(0)
{
    sem_init(&mQueuePending, 0, 0);
}

MppQueue::~MppQueue()
{
    sem_destroy (&mQueuePending);
}

RK_S32 MppQueue::push(void *data, RK_S32 size)
{
    RK_S32 ret = 0;

    ret = mpp_list::add_at_tail(data, size);
    mFlushFlag = 0;
    sem_post(&mQueuePending);

    return ret;
}

RK_S32 MppQueue::pull(void *data, RK_S32 size)
{
    if (!mFlushFlag)
        sem_wait(&mQueuePending);
    {
        AutoMutex autoLock(mpp_list::mutex());
        if (!mpp_list::list_size())
            return MPP_NOK;

        return mpp_list::del_at_head(data, size);
    }
    return MPP_ERR_INIT;
}

RK_S32 MppQueue::flush()
{
    if (mFlushFlag)
        return 0;

    mFlushFlag = 1;
    sem_post(&mQueuePending);
    return mpp_list::flush();
}
