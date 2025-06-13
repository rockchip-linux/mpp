/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_THREAD_H__
#define __MPP_THREAD_H__

#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "rk_type.h"

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

#if defined(__USE_XOPEN2K) || (defined(__ANDROID__) && __ANDROID_API__ >= 21)
#define COND_USE_CLOCK_MONOTONIC
#endif

#define THREAD_NAME_LEN 16

#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*MppThreadFunc)(void *);

typedef enum MppThreadStatus_e {
    MPP_THREAD_UNINITED,
    MPP_THREAD_READY,
    MPP_THREAD_RUNNING,
    MPP_THREAD_WAITING,
    MPP_THREAD_STOPPING,
} MppThreadStatus;

typedef struct MppMutex_t {
    pthread_mutex_t     lock;
} MppMutex;

typedef struct MppCond_t {
    pthread_cond_t      cond;
    clockid_t           clock_id;
} MppCond;

typedef struct MppMutexCond_t {
    MppMutex            lock;
    MppCond             cond;
} MppMutexCond;

typedef enum MppThreadSignalId_e {
    THREAD_WORK,
    THREAD_INPUT,
    THREAD_OUTPUT,
    THREAD_CONTROL,
    THREAD_SIGNAL_BUTT,
} MppThreadSignalId;

typedef struct MppThread_t {
    pthread_t           thd;
    MppMutexCond        mutex_cond[THREAD_SIGNAL_BUTT];
    MppThreadStatus     thd_status[THREAD_SIGNAL_BUTT];
    MppThreadFunc       func;
    char                name[THREAD_NAME_LEN];
    void                *ctx;
} MppThread;

// Mutex functions
void mpp_mutex_init(MppMutex *mutex);
void mpp_mutex_destroy(MppMutex *mutex);
void mpp_mutex_lock(MppMutex *mutex);
void mpp_mutex_unlock(MppMutex *mutex);
int mpp_mutex_trylock(MppMutex *mutex);

// Condition functions
void mpp_cond_init(MppCond *condition);
void mpp_cond_destroy(MppCond *condition);
rk_s32 mpp_cond_wait(MppCond *condition, MppMutex *mutex);
rk_s32 mpp_cond_timedwait(MppCond *condition, MppMutex *mutex, rk_s64 timeout);
rk_s32 mpp_cond_signal(MppCond *condition);
rk_s32 mpp_cond_broadcast(MppCond *condition);

// Mutex-Condition functions
void mpp_mutex_cond_init(MppMutexCond *mutexCond);
void mpp_mutex_cond_destroy(MppMutexCond *mutexCond);
void mpp_mutex_cond_lock(MppMutexCond *mutexCond);
void mpp_mutex_cond_unlock(MppMutexCond *mutexCond);
int mpp_mutex_cond_trylock(MppMutexCond *mutexCond);
rk_s32 mpp_mutex_cond_wait(MppMutexCond *mutexCond);
rk_s32 mpp_mutex_cond_timedwait(MppMutexCond *mutexCond, rk_s64 timeout);
void mpp_mutex_cond_signal(MppMutexCond *mutexCond);
void mpp_mutex_cond_broadcast(MppMutexCond *mutexCond);

// Thread functions
void mpp_thread_init(MppThread *thread, MppThreadFunc func, void *ctx, const char *name);
void mpp_thread_set_status(MppThread *thread, MppThreadStatus status, MppThreadSignalId id);
MppThreadStatus mpp_thread_get_status(MppThread *thread, MppThreadSignalId id);
void mpp_thread_lock(MppThread *thread, MppThreadSignalId id);
void mpp_thread_unlock(MppThread *thread, MppThreadSignalId id);
void mpp_thread_wait(MppThread *thread, MppThreadSignalId id);
void mpp_thread_signal(MppThread *thread, MppThreadSignalId id);

MppThread *mpp_thread_create(MppThreadFunc func, void *ctx, const char *name);
void mpp_thread_destroy(MppThread *thread);
void mpp_thread_start(MppThread *thread);
void mpp_thread_stop(MppThread *thread);


/*
 * status transaction:
 *                 create
 *                   v
 *           MPP_THREAD_UNINITED
 *                   v
 *                 setup
 *                   v
 * destroy <- MPP_THREAD_READY  <-------------------+
 *                   v                              |
 *                 start                            |
 *                   v                              |
 *           MPP_THREAD_RUNNING -> stop -> MPP_THREAD_STOPPING
 *                   v                              |
 *                 wait                             |
 *                   v                              |
 *           MPP_THREAD_WAITING -> stop ------------+
 *
 */
typedef enum MppSThdStatus_e {
    MPP_STHD_UNINITED,
    MPP_STHD_READY,
    MPP_STHD_RUNNING,
    MPP_STHD_WAITING,
    MPP_STHD_STOPPING,
    MPP_STHD_BUTT,
} MppSThdStatus;

/* MppSThd for Mpp Simple Thread */
typedef void* MppSThd;
typedef void* MppSThdGrp;

typedef struct MppSThdCtx_t {
    MppSThd     thd;
    void        *ctx;
} MppSThdCtx;

typedef void *(*MppSThdFunc)(MppSThdCtx *);

MppSThd mpp_sthd_get(const char *name);
void mpp_sthd_put(MppSThd thd);

MppSThdStatus mpp_sthd_get_status(MppSThd thd);
const char* mpp_sthd_get_name(MppSThd thd);
rk_s32 mpp_sthd_get_idx(MppSThd thd);
rk_s32 mpp_sthd_check(MppSThd thd);

void mpp_sthd_setup(MppSThd thd, MppSThdFunc func, void *ctx);

void mpp_sthd_start(MppSThd thd);
void mpp_sthd_stop(MppSThd thd);
void mpp_sthd_stop_sync(MppSThd thd);

void mpp_sthd_lock(MppSThd thd);
void mpp_sthd_unlock(MppSThd thd);
int  mpp_sthd_trylock(MppSThd thd);

void mpp_sthd_wait(MppSThd thd);
void mpp_sthd_signal(MppSThd thd);
void mpp_sthd_broadcast(MppSThd thd);

/* multi-thread group with same callback and context */
MppSThdGrp mpp_sthd_grp_get(const char *name, rk_s32 count);
void mpp_sthd_grp_put(MppSThdGrp grp);

void mpp_sthd_grp_setup(MppSThdGrp grp, MppSThdFunc func, void *ctx);
MppSThd mpp_sthd_grp_get_each(MppSThdGrp grp, rk_s32 idx);

void mpp_sthd_grp_start(MppSThdGrp grp);
void mpp_sthd_grp_stop(MppSThdGrp grp);
void mpp_sthd_grp_stop_sync(MppSThdGrp grp);

#ifdef __cplusplus
}
#endif

#endif