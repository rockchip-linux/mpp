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

#include "mpp_log.h"
#include "mpp_thread.h"

#define MPP_THREAD_DBG_FUNCTION     (0x00000001)

static RK_U32 thread_debug = 0;

#define thread_dbg(flag, fmt, ...)  mpp_dbg(thread_debug, flag, fmt, ## __VA_ARGS__)

MppThread::MppThread(MppThreadFunc func, void *ctx)
    : status(MPP_THREAD_UNINITED)
{
    pthread_mutexattr_t mutex;
    pthread_mutexattr_init(&mutex);
    pthread_mutexattr_settype(&mutex, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&thread_lock, &mutex);
    pthread_mutexattr_destroy(&mutex);

    pthread_cond_init(&condition, NULL);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (0 == pthread_create(&thread, &attr, func, ctx)) {
        status = MPP_THREAD_RUNNING;
        thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %p context %p create success\n",
                   func, ctx);
    }

    pthread_attr_destroy(&attr);

}

MppThread::~MppThread()
{
    status = MPP_THREAD_STOPPING;
    void *dummy;
    pthread_join(thread, &dummy);
    thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %p context %p destroy success\n",
               function, context);
}

void MppThread::lock()
{
    pthread_mutex_lock(&thread_lock);
}

void MppThread::unlock()
{
    pthread_mutex_unlock(&thread_lock);
}

void MppThread::wait()
{
    pthread_cond_wait(&condition, &thread_lock);
}

void MppThread::signal()
{
    pthread_cond_signal(&condition);
}

