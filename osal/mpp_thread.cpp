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

#define MODULE_TAG "mpp_thread"

#include <string.h>

#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_thread.h"

#define MPP_THREAD_DBG_FUNCTION     (0x00000001)

static RK_U32 thread_debug = 0;

#define thread_dbg(flag, fmt, ...)  _mpp_dbg(thread_debug, flag, fmt, ## __VA_ARGS__)

MppThread::MppThread(MppThreadFunc func, void *ctx, const char *name)
    : mFunction(func),
      mContext(ctx)
{
    mStatus[THREAD_WORK]    = MPP_THREAD_UNINITED;
    mStatus[THREAD_INPUT]   = MPP_THREAD_RUNNING;
    mStatus[THREAD_OUTPUT]  = MPP_THREAD_RUNNING;
    mStatus[THREAD_CONTROL] = MPP_THREAD_RUNNING;

    if (name)
        strncpy(mName, name, sizeof(mName));
    else
        snprintf(mName, sizeof(mName), "mpp_thread");
}

MppThreadStatus MppThread::get_status(MppThreadSignal id)
{
    return mStatus[id];
}

void MppThread::set_status(MppThreadStatus status, MppThreadSignal id)
{
    mStatus[id] = status;
}

void MppThread::dump_status()
{
    mpp_log("thread %s status: %d %d %d %d\n", mName,
            mStatus[THREAD_WORK], mStatus[THREAD_INPUT], mStatus[THREAD_OUTPUT],
            mStatus[THREAD_CONTROL]);
}

void MppThread::start()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (MPP_THREAD_UNINITED == get_status()) {
        // NOTE: set status here first to avoid unexpected loop quit racing condition
        set_status(MPP_THREAD_RUNNING);
        if (0 == pthread_create(&mThread, &attr, mFunction, mContext)) {
#ifndef ARMLINUX
            RK_S32 ret = pthread_setname_np(mThread, mName);
            if (ret)
                mpp_err("thread %p setname %s failed\n", mFunction, mName);
#endif

            thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %s %p context %p create success\n",
                       mName, mFunction, mContext);
        } else
            set_status(MPP_THREAD_UNINITED);
    }
    pthread_attr_destroy(&attr);
}

void MppThread::stop()
{
    if (MPP_THREAD_UNINITED != get_status()) {
        lock();
        set_status(MPP_THREAD_STOPPING);
        thread_dbg(MPP_THREAD_DBG_FUNCTION,
                   "MPP_THREAD_STOPPING status set mThread %p", this);
        signal();
        unlock();
        void *dummy;
        pthread_join(mThread, &dummy);
        thread_dbg(MPP_THREAD_DBG_FUNCTION,
                   "thread %s %p context %p destroy success\n",
                   mName, mFunction, mContext);

        set_status(MPP_THREAD_UNINITED);
    }
}

#if defined(_WIN32) && !defined(__MINGW32CE__)
//
// Usage: SetThreadName ((DWORD)-1, "MainThread");
//
#include <windows.h>
const DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType; // Must be 0x1000.
    LPCSTR szName; // Pointer to name (in user addr space).
    DWORD dwThreadID; // Thread ID (-1=caller thread).
    DWORD dwFlags; // Reserved for future use, must be zero.
} THREADNAME_INFO;
#pragma pack(pop)

void SetThreadName(DWORD dwThreadID, const char* threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;
#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), (ULONG_PTR*)&info);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
}


#ifndef ARMLINUX
/*
 * add pthread_setname_np for windows
 */
int pthread_setname_np(pthread_t thread, const char *name)
{
    DWORD dwThreadID = pthread_getw32threadid_np(thread);
    SetThreadName(dwThreadID, name);
    return 0;
}
#endif

#endif

