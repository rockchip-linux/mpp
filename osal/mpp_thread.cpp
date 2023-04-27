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
#include "mpp_mem.h"
#include "mpp_lock.h"
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
        strncpy(mName, name, sizeof(mName) - 1);
    else
        snprintf(mName, sizeof(mName) - 1, "mpp_thread");
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

typedef struct MppSThdImpl_t {
    char            *name;
    MppSThdFunc     func;
    MppSThdStatus   status;
    RK_S32          idx;
    pthread_t       thd;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    MppSThdCtx      ctx;
} MppSThdImpl;

typedef struct MppSThdGrpImpl_t {
    char            name[THREAD_NAME_LEN];
    RK_S32          count;
    MppSThdStatus   status;
    pthread_mutex_t lock;
    MppSThdImpl     thds[];
} MppSThdGrpImpl;

static const char *state2str(MppSThdStatus state)
{
    static const char *strof_sthd_status[] = {
        "uninited",
        "ready",
        "running",
        "waiting",
        "stopping",
        "invalid"
    };

    return state < MPP_STHD_BUTT ? strof_sthd_status[state] : strof_sthd_status[MPP_STHD_BUTT];
}

static RK_S32 check_sthd(const char *name, MppSThdImpl *thd)
{
    if (!thd) {
        mpp_err("MppSThd NULL found at %s\n", name);
        return MPP_NOK;
    }

    if (thd->ctx.thd != thd) {
        mpp_err("MppSThd check %p:%p mismatch at %s\n", thd->ctx.thd, thd, name);
        return MPP_NOK;
    }

    return MPP_OK;
}

#define CHECK_STHD(thd) check_sthd(__FUNCTION__, (MppSThdImpl *)(thd))

static void mpp_sthd_init(MppSThdImpl *thd, RK_S32 idx)
{
    pthread_mutexattr_t attr;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&thd->lock, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_cond_init(&thd->cond, NULL);
    thd->ctx.thd = thd;
    thd->idx = idx;
}

static void mpp_sthd_deinit(MppSThdImpl *thd)
{
    mpp_assert(thd->ctx.thd == thd);
    mpp_assert(thd->status < MPP_STHD_RUNNING);

    pthread_mutex_lock(&thd->lock);
    thd->status = MPP_STHD_UNINITED;
    thd->ctx.thd = NULL;
    pthread_mutex_unlock(&thd->lock);

    pthread_cond_destroy(&thd->cond);
    pthread_mutex_destroy(&thd->lock);
}

static MPP_RET mpp_sthd_create(MppSThdImpl *thd)
{
    pthread_attr_t attr;
    MPP_RET ret = MPP_NOK;

    mpp_assert(thd->ctx.thd == thd);
    mpp_assert(thd->status < MPP_STHD_RUNNING);

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    // NOTE: set status to running first
    thd->status = MPP_STHD_RUNNING;
    if (0 == pthread_create(&thd->thd, &attr, (MppThreadFunc)thd->func, &thd->ctx)) {
        ret = (MPP_RET)pthread_setname_np(thd->thd, thd->name);
        if (ret)
            mpp_err("%s %p setname failed\n", thd->thd, thd->func);

        thread_dbg(MPP_THREAD_DBG_FUNCTION, "thread %s %p context %p create success\n",
                   thd->name, thd->func, thd->ctx.ctx);
        ret = MPP_OK;
    } else {
        thd->status = MPP_STHD_READY;
    }

    pthread_attr_destroy(&attr);

    return ret;
}

MppSThd mpp_sthd_get(const char *name)
{
    RK_S32 size = MPP_ALIGN(sizeof(MppSThdImpl), 8) + THREAD_NAME_LEN;
    MppSThdImpl *thd = mpp_calloc_size(MppSThdImpl, size);

    if (!thd) {
        mpp_err_f("failed to create simple thread\n");
        return NULL;
    }

    thd->name = (char *)(thd + 1);
    if (!name)
        name = "mpp_sthd";

    snprintf(thd->name, THREAD_NAME_LEN - 1, "%s", name);

    mpp_sthd_init(thd, -1);

    return thd;
}

void mpp_sthd_put(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    mpp_assert(impl);
    mpp_assert(impl->ctx.thd == impl);
    mpp_assert(impl->status == MPP_STHD_UNINITED || impl->status == MPP_STHD_READY);

    mpp_sthd_deinit(impl);

    mpp_free(impl);
}

MppSThdStatus mpp_sthd_get_status(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    return impl->status;
}

const char* mpp_sthd_get_name(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    return impl->name;
}

RK_S32 mpp_sthd_get_idx(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    return impl->idx;
}

RK_S32 mpp_sthd_check(MppSThd thd)
{
    return CHECK_STHD(thd);
}

void mpp_sthd_setup(MppSThd thd, MppSThdFunc func, void *ctx)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;
    MppSThdStatus status;

    CHECK_STHD(impl);

    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_UNINITED :
    case MPP_STHD_READY : {
        impl->func = func;
        impl->ctx.ctx = ctx;
        impl->status = func ? MPP_STHD_READY : MPP_STHD_UNINITED;
    } break;
    default : {
        mpp_err("%s can NOT setup on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);

    CHECK_STHD(impl);
}

void mpp_sthd_start(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;
    MppSThdStatus status;

    CHECK_STHD(impl);

    /* we can only change callback function on uninit */
    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_READY : {
        mpp_sthd_create(impl);
    } break;
    default : {
        mpp_err("%s can NOT start on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);

    CHECK_STHD(impl);
}

void mpp_sthd_stop(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;
    MppSThdStatus status;

    CHECK_STHD(impl);

    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_RUNNING :
    case MPP_STHD_WAITING : {
        status = MPP_STHD_STOPPING;
        pthread_cond_signal(&impl->cond);
    } break;
    default : {
        mpp_err("%s can NOT stop on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);

    CHECK_STHD(impl);
}

void mpp_sthd_stop_sync(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;
    MppSThdStatus status;

    CHECK_STHD(impl);

    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_STOPPING : {
        void *dummy;

        pthread_join(impl->thd, &dummy);
        impl->status = MPP_STHD_READY;
    } break;
    default : {
        mpp_err("%s can NOT stop on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);

    CHECK_STHD(impl);
}

void mpp_sthd_lock(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    pthread_mutex_lock(&impl->lock);
}

void mpp_sthd_unlock(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    pthread_mutex_unlock(&impl->lock);
}

int mpp_sthd_trylock(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    return pthread_mutex_trylock(&impl->lock);
}

void mpp_sthd_wait(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    if (impl->status == MPP_STHD_RUNNING)
        impl->status = MPP_STHD_WAITING;

    pthread_cond_wait(&impl->cond, &impl->lock);

    if (impl->status == MPP_STHD_WAITING)
        impl->status = MPP_STHD_RUNNING;
}

void mpp_sthd_signal(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    pthread_cond_signal(&impl->cond);
}

void mpp_sthd_broadcast(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    pthread_cond_broadcast(&impl->cond);
}

MppSThdGrp mpp_sthd_grp_get(const char *name, RK_S32 count)
{
    MppSThdGrpImpl *grp = NULL;

    if (count > 0) {
        RK_S32 elem_size = MPP_ALIGN(sizeof(MppSThdImpl), 8);
        RK_S32 total_size = MPP_ALIGN(sizeof(MppSThdGrpImpl), 8) + count * elem_size;

        grp = mpp_calloc_size(MppSThdGrpImpl, total_size);
        if (grp) {
            pthread_mutexattr_t attr;
            RK_S32 i;

            if (!name)
                name = "mpp_sthd_grp";

            snprintf(grp->name, THREAD_NAME_LEN - 1, "%s", name);

            grp->count = count;
            for (i = 0; i < count; i++) {
                MppSThdImpl *thd = &grp->thds[i];

                thd->name = grp->name;
                mpp_sthd_init(thd, i);
            }

            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&grp->lock, &attr);
            pthread_mutexattr_destroy(&attr);
        }
    }

    if (!grp)
        mpp_err_f("can NOT create %d threads group\n", count);

    return grp;
}

void mpp_sthd_grp_put(MppSThdGrp grp)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    RK_S32 i;

    mpp_assert(impl);
    mpp_assert(impl->status == MPP_STHD_UNINITED || impl->status == MPP_STHD_READY);

    for (i = 0; i < impl->count; i++) {
        MppSThdImpl *thd = &impl->thds[i];

        mpp_sthd_deinit(thd);
    }

    mpp_free(impl);
}

void mpp_sthd_grp_setup(MppSThdGrp grp, MppSThdFunc func, void *ctx)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    MppSThdStatus status;

    mpp_assert(impl);

    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_UNINITED :
    case MPP_STHD_READY : {
        MppSThdStatus next = func ? MPP_STHD_READY : MPP_STHD_UNINITED;
        RK_S32 i;

        for (i = 0; i < impl->count; i++) {
            MppSThdImpl *thd = &impl->thds[i];

            thd->func = func;
            thd->ctx.ctx = ctx;
            thd->status = next;
        }
        impl->status = next;
    } break;
    default : {
        mpp_err("%s can NOT setup on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);
}

void mpp_sthd_grp_start(MppSThdGrp grp)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    MppSThdStatus status;

    mpp_assert(impl);

    /* we can only change callback function on uninit */
    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_READY : {
        RK_S32 i;

        for (i = 0; i < impl->count; i++)
            mpp_sthd_start(&impl->thds[i]);

        impl->status = MPP_STHD_RUNNING;
    } break;
    default : {
        mpp_err("%s can NOT start on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);
}

void mpp_sthd_grp_stop(MppSThdGrp grp)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    MppSThdStatus status;

    mpp_assert(impl);

    /* we can only change callback function on uninit */
    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_RUNNING :
    case MPP_STHD_WAITING : {
        RK_S32 i;

        impl->status = MPP_STHD_STOPPING;

        for (i = 0; i < impl->count; i++) {
            MppSThdImpl *thd = &impl->thds[i];

            pthread_mutex_lock(&thd->lock);
            thd->status = MPP_STHD_STOPPING;
            pthread_cond_signal(&thd->cond);
            pthread_mutex_unlock(&thd->lock);
        }
    } break;
    default : {
        mpp_err("%s can NOT stop on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);
}

void mpp_sthd_grp_stop_sync(MppSThdGrp grp)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    MppSThdStatus status;

    mpp_assert(impl);

    /* we can only change callback function on uninit */
    pthread_mutex_lock(&impl->lock);
    status = impl->status;
    switch (status) {
    case MPP_STHD_STOPPING : {
        void *dummy;
        RK_S32 i;

        status = MPP_STHD_STOPPING;
        for (i = 0; i < impl->count; i++) {
            MppSThdImpl *thd = &impl->thds[i];

            pthread_join(thd->thd, &dummy);
            thd->status = MPP_STHD_READY;
        }
        impl->status = MPP_STHD_READY;
    } break;
    default : {
        mpp_err("%s can NOT stop sync on %s\n", impl->name, state2str(status));
    } break;
    }
    pthread_mutex_unlock(&impl->lock);
}

MppSThd mpp_sthd_grp_get_each(MppSThdGrp grp, RK_S32 idx)
{
    MppSThdGrpImpl *impl = (MppSThdGrpImpl *)grp;
    MppSThd ret = NULL;

    mpp_assert(impl);
    mpp_assert(idx >= 0 && idx < impl->count);

    pthread_mutex_lock(&impl->lock);
    ret = &impl->thds[idx];
    pthread_mutex_unlock(&impl->lock);

    return ret;
}