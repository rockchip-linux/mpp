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

/*
 * File         : mpp_thread.h
 * Description  : thread library for different OS
 * Author       : herman.chen@rock-chips.com
 * Date         : 9:47 2015/7/27
 */

#ifndef __MPP_THREAD_H__
#define __MPP_THREAD_H__

#if defined(_WIN32) && !defined(__MINGW32CE__)

/*
 * NOTE: POSIX Threads for Win32
 * Downloaded from http://www.sourceware.org/pthreads-win32/
 */
#include "semaphore.h"
#include "pthread.h"
#pragma comment(lib, "pthreadVC2.lib")

#else

#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#if defined(ANDROID)
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

#endif

typedef void *(*MppThreadFunc)(void *);

typedef enum {
    MPP_THREAD_UNINITED,
    MPP_THREAD_RUNNING,
    MPP_THREAD_WAITING,
    MPP_THREAD_STOPPING,
} MppThreadStatus;

#ifdef __cplusplus
class MppThread {
public:
    MppThread(MppThreadFunc func, void *ctx);
    ~MppThread();

    MppThreadStatus get_status();
    void set_status(MppThreadStatus status);

    void start();
    void stop();
    void lock();
    void unlock();
    void wait();
    void signal();

private:
    pthread_t       mThread;
    pthread_mutex_t mLock;
    pthread_cond_t  mCondition;

    MppThreadStatus mStatus;
    MppThreadFunc   mFunction;
    void            *mContext;

    MppThread();
    MppThread(const MppThread &);
    MppThread &operator=(const MppThread &);
};
#endif

#endif /*__MPP_THREAD_H__*/
