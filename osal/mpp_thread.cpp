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

#define MODULE_TAG "mpp_thread"

#include <string.h>

#include "mpp_log.h"
#include "mpp_thread.h"

#define MPP_THREAD_DBG_FUNCTION     (0x00000001)

static RK_U32 thread_debug = 0;

#define thread_dbg(flag, fmt, ...)  _mpp_dbg(thread_debug, flag, fmt, ## __VA_ARGS__)

MppThread::MppThread(MppThreadFunc func, void *ctx, const char *name)
    : mStatus(MPP_THREAD_UNINITED),
      mFunction(func),
      mContext(ctx)
{
    if (name)
        strncpy(mName, name, sizeof(mName));
    else
        snprintf(mName, sizeof(mName), "mpp_thread");
}

MppThreadStatus MppThread::get_status()
{
    return mStatus;
}
void MppThread::set_status(MppThreadStatus status)
{
    mStatus = status;
}

void MppThread::start()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (MPP_THREAD_UNINITED == mStatus) {
        // NOTE: set status here first to avoid unexpected loop quit racing condition
        mStatus = MPP_THREAD_RUNNING;
        if (0 == pthread_create(&mThread, &attr, mFunction, mContext)) {
            RK_S32 ret = pthread_setname_np(mThread, mName);
            if (ret)
                mpp_err("thread %p setname %s failed\n", mFunction, mName);

            thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %s %p context %p create success\n",
                       mName, mFunction, mContext);
        } else
            mStatus = MPP_THREAD_UNINITED;
    }
    pthread_attr_destroy(&attr);
}

void MppThread::stop()
{
    if (MPP_THREAD_UNINITED != mStatus) {
        lock();
        mStatus = MPP_THREAD_STOPPING;
        mpp_log("MPP_THREAD_STOPPING status set mThread %p", this);
        signal();
        unlock();
        void *dummy;
        pthread_join(mThread, &dummy);
        thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %s %p context %p destroy success\n",
                   mName, mFunction, mContext);

        mStatus = MPP_THREAD_UNINITED;
    }
}

