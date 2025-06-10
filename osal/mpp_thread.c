/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_thread"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_lock.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"

#define THREAD_DBG_FUNC             (0x00000001)

static rk_u32 thread_debug = 0;

#define thread_dbg(flag, fmt, ...)  _mpp_dbg(thread_debug, flag, fmt, ## __VA_ARGS__)

MppThread *mpp_thread_create(MppThreadFunc func, void *ctx, const char *name)
{
    MppThread *thread = mpp_malloc(MppThread, 1);

    if (thread) {
        thread->func = func;
        thread->m_ctx = ctx;

        thread->thd_status[THREAD_WORK] = MPP_THREAD_UNINITED;
        thread->thd_status[THREAD_INPUT] = MPP_THREAD_RUNNING;
        thread->thd_status[THREAD_OUTPUT] = MPP_THREAD_RUNNING;
        thread->thd_status[THREAD_CONTROL] = MPP_THREAD_RUNNING;

        if (name) {
            strncpy(thread->name, name, THREAD_NAME_LEN - 1);
            thread->name[THREAD_NAME_LEN - 1] = '\0';
        } else {
            snprintf(thread->name, THREAD_NAME_LEN, "MppThread");
        }
        for (int i = 0; i < THREAD_SIGNAL_BUTT; i++) {
            mpp_mutex_cond_init(&thread->mutex_cond[i]);
        }
    }

    return thread;
}

void mpp_thread_dump_status(MppThread *thread)
{
    mpp_log("thread %s status: %d %d %d %d\n", thread->name,
            thread->thd_status[THREAD_WORK], thread->thd_status[THREAD_INPUT],
            thread->thd_status[THREAD_OUTPUT], thread->thd_status[THREAD_CONTROL]);
}

void mpp_thread_start(MppThread *thread)
{
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    if (mpp_thread_get_status(thread, THREAD_WORK) == MPP_THREAD_UNINITED) {
        mpp_thread_set_status(thread, MPP_THREAD_RUNNING, THREAD_WORK);
        if (0 == pthread_create(&thread->thd, &attr, thread->func, thread->m_ctx)) {
#ifndef __linux__
            int ret = pthread_setname_np(thread->thd, thread->name);
            if (ret) {
                mpp_err("thread %p setname %s failed\n", thread->func, thread->name);
            }
#endif
            thread_dbg(THREAD_DBG_FUNC, "thread %s %p context %p create success\n",
                       thread->name, thread->func, thread->m_ctx);
        } else {
            mpp_thread_set_status(thread, MPP_THREAD_UNINITED, THREAD_WORK);
        }
    }

    pthread_attr_destroy(&attr);
}

void mpp_thread_stop(MppThread *thread)
{
    if (mpp_thread_get_status(thread, THREAD_WORK) != MPP_THREAD_UNINITED) {
        void *dummy;

        mpp_thread_lock(thread, THREAD_WORK);
        mpp_thread_set_status(thread, MPP_THREAD_STOPPING, THREAD_WORK);

        thread_dbg(THREAD_DBG_FUNC, "MPP_THREAD_STOPPING status set thd %p\n", (void *)thread);
        mpp_thread_signal(thread, THREAD_WORK);
        mpp_thread_unlock(thread, THREAD_WORK);

        pthread_join(thread->thd, &dummy);
        thread_dbg(THREAD_DBG_FUNC, "thread %s %p context %p destroy success\n", thread->name, thread->func, thread->m_ctx);

        mpp_thread_set_status(thread, MPP_THREAD_UNINITED, THREAD_WORK);
    }
}

void mpp_thread_destroy(MppThread *thread)
{
    if (thread) {
        mpp_thread_stop(thread);
        mpp_free(thread);
    }
}

void mpp_mutex_init(MppMutex *mutex)
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mutex->m_lock, &attr);
    pthread_mutexattr_destroy(&attr);
}

void mpp_mutex_destroy(MppMutex *mutex)
{
    pthread_mutex_destroy(&mutex->m_lock);
}

void mpp_mutex_lock(MppMutex *mutex)
{
    pthread_mutex_lock(&mutex->m_lock);
}

void mpp_mutex_unlock(MppMutex *mutex)
{
    pthread_mutex_unlock(&mutex->m_lock);
}

int mpp_mutex_trylock(MppMutex *mutex)
{
    return pthread_mutex_trylock(&mutex->m_lock);
}

// MppCond functions
void mpp_cond_init(MppCond *condition)
{
    pthread_cond_init(&condition->m_cond, NULL);
}

void mpp_cond_destroy(MppCond *condition)
{
    pthread_cond_destroy(&condition->m_cond);
}

rk_s32 mpp_cond_wait(MppCond *condition, MppMutex *mutex)
{
    return pthread_cond_wait(&condition->m_cond, &mutex->m_lock);
}

rk_s32 mpp_cond_timedwait(MppCond *condition, MppMutex *mutex, rk_s64 timeout)
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    return pthread_cond_timedwait(&condition->m_cond, &mutex->m_lock, &ts);
}

rk_s32 mpp_cond_signal(MppCond *condition)
{
    return pthread_cond_signal(&condition->m_cond);
}

rk_s32 mpp_cond_broadcast(MppCond *condition)
{
    return pthread_cond_broadcast(&condition->m_cond);
}

// MppMutexCond functions
void mpp_mutex_cond_init(MppMutexCond *mutexCond)
{
    mpp_mutex_init(&mutexCond->m_lock);
    mpp_cond_init(&mutexCond->m_cond);
}

void mpp_mutex_cond_destroy(MppMutexCond *mutexCond)
{
    mpp_mutex_destroy(&mutexCond->m_lock);
    mpp_cond_destroy(&mutexCond->m_cond);
}

void mpp_mutex_cond_lock(MppMutexCond *mutexCond)
{
    mpp_mutex_lock(&mutexCond->m_lock);
}

void mpp_mutex_cond_unlock(MppMutexCond *mutexCond)
{
    mpp_mutex_unlock(&mutexCond->m_lock);
}

int mpp_mutex_cond_trylock(MppMutexCond *mutexCond)
{
    return mpp_mutex_trylock(&mutexCond->m_lock);
}

rk_s32 mpp_mutex_cond_wait(MppMutexCond *mutexCond)
{
    return mpp_cond_wait(&mutexCond->m_cond, &mutexCond->m_lock);
}

rk_s32 mpp_mutex_cond_timedwait(MppMutexCond *mutexCond, rk_s64 timeout)
{
    return mpp_cond_timedwait(&mutexCond->m_cond, &mutexCond->m_lock, timeout);
}

void mpp_mutex_cond_signal(MppMutexCond *mutexCond)
{
    mpp_cond_signal(&mutexCond->m_cond);
}

void mpp_mutex_cond_broadcast(MppMutexCond *mutexCond)
{
    mpp_cond_broadcast(&mutexCond->m_cond);
}

// MppThread functions
void mpp_thread_init(MppThread *thread, MppThreadFunc func, void *ctx, const char *name)
{
    thread->func = func;
    thread->m_ctx = ctx;
    if (name) {
        strncpy(thread->name, name, THREAD_NAME_LEN - 1);
        thread->name[THREAD_NAME_LEN - 1] = '\0';
    }
    for (int i = 0; i < THREAD_SIGNAL_BUTT; i++) {
        mpp_mutex_cond_init(&thread->mutex_cond[i]);
        thread->thd_status[i] = MPP_THREAD_UNINITED;
    }
}

void mpp_thread_set_status(MppThread *thread, MppThreadStatus status, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    thread->thd_status[id] = status;
}

MppThreadStatus mpp_thread_get_status(MppThread *thread, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    return thread->thd_status[id];
}

void mpp_thread_lock(MppThread *thread, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    mpp_mutex_cond_lock(&thread->mutex_cond[id]);
}

void mpp_thread_unlock(MppThread *thread, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    mpp_mutex_cond_unlock(&thread->mutex_cond[id]);
}

void mpp_thread_wait(MppThread *thread, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    MppThreadStatus status = thread->thd_status[id];
    thread->thd_status[id] = MPP_THREAD_WAITING;
    mpp_mutex_cond_wait(&thread->mutex_cond[id]);

    if (thread->thd_status[id] == MPP_THREAD_WAITING)
        thread->thd_status[id] = status;
}

void mpp_thread_signal(MppThread *thread, MppThreadSignalId id)
{
    assert(id < THREAD_SIGNAL_BUTT);
    mpp_mutex_cond_signal(&thread->mutex_cond[id]);
}

typedef struct MppSThdImpl_t {
    char            *name;
    MppSThdFunc     func;
    MppSThdStatus   status;
    rk_s32          idx;
    pthread_t       thd;
    pthread_mutex_t lock;
    pthread_cond_t  cond;
    MppSThdCtx      ctx;
} MppSThdImpl;

typedef struct MppSThdGrpImpl_t {
    char            name[THREAD_NAME_LEN];
    rk_s32          count;
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

static rk_s32 check_sthd(const char *name, MppSThdImpl *thd)
{
    if (!thd) {
        mpp_err("mpp_sthd NULL found at %s\n", name);
        return MPP_NOK;
    }

    if (thd->ctx.thd != thd) {
        mpp_err("mpp_sthd check %p:%p mismatch at %s\n", thd->ctx.thd, thd, name);
        return MPP_NOK;
    }

    return MPP_OK;
}

#define CHECK_STHD(thd) check_sthd(__FUNCTION__, (MppSThdImpl *)(thd))

static void mpp_sthd_init(MppSThdImpl *thd, rk_s32 idx)
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

        thread_dbg(THREAD_DBG_FUNC, "thread %s %p context %p create success\n",
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
    rk_s32 size = MPP_ALIGN(sizeof(MppSThdImpl), 8) + THREAD_NAME_LEN;
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

rk_s32 mpp_sthd_get_idx(MppSThd thd)
{
    MppSThdImpl *impl = (MppSThdImpl *)thd;

    CHECK_STHD(impl);

    return impl->idx;
}

rk_s32 mpp_sthd_check(MppSThd thd)
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

MppSThdGrp mpp_sthd_grp_get(const char *name, rk_s32 count)
{
    MppSThdGrpImpl *grp = NULL;

    if (count > 0) {
        rk_s32 elem_size = MPP_ALIGN(sizeof(MppSThdImpl), 8);
        rk_s32 total_size = MPP_ALIGN(sizeof(MppSThdGrpImpl), 8) + count * elem_size;

        grp = mpp_calloc_size(MppSThdGrpImpl, total_size);
        if (grp) {
            pthread_mutexattr_t attr;
            rk_s32 i;

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
    rk_s32 i;

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
        rk_s32 i;

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
        rk_s32 i;

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
        rk_s32 i;

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
        rk_s32 i;

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

MppSThd mpp_sthd_grp_get_each(MppSThdGrp grp, rk_s32 idx)
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