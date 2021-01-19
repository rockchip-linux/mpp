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

/*
 * add pthread_setname_np for windows
 */
int pthread_setname_np(pthread_t thread, const char *name);

#else

#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>
#include <sys/time.h>

#ifndef PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP
#define PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP PTHREAD_RECURSIVE_MUTEX_INITIALIZER
#endif

#endif

#define THREAD_NAME_LEN 16

typedef void *(*MppThreadFunc)(void *);

typedef enum {
    MPP_THREAD_UNINITED,
    MPP_THREAD_RUNNING,
    MPP_THREAD_WAITING,
    MPP_THREAD_STOPPING,
} MppThreadStatus;

#ifdef __cplusplus

#include "mpp_log.h"

class Mutex;
class Condition;

/*
 * for shorter type name and function name
 */
class Mutex
{
public:
    Mutex();
    ~Mutex();

    void lock();
    void unlock();
    int  trylock();

    class Autolock
    {
    public:
        inline Autolock(Mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline Autolock(Mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~Autolock() { mLock.unlock(); }
    private:
        Mutex& mLock;
    };

private:
    friend class Condition;

    pthread_mutex_t mMutex;

    Mutex(const Mutex &);
    Mutex &operator = (const Mutex&);
};

inline Mutex::Mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&mMutex, &attr);
    pthread_mutexattr_destroy(&attr);
}
inline Mutex::~Mutex()
{
    pthread_mutex_destroy(&mMutex);
}
inline void Mutex::lock()
{
    pthread_mutex_lock(&mMutex);
}
inline void Mutex::unlock()
{
    pthread_mutex_unlock(&mMutex);
}
inline int Mutex::trylock()
{
    return pthread_mutex_trylock(&mMutex);
}

typedef Mutex::Autolock AutoMutex;


/*
 * for shorter type name and function name
 */
class Condition
{
public:
    Condition();
    Condition(int type);
    ~Condition();
    RK_S32 wait(Mutex& mutex);
    RK_S32 wait(Mutex* mutex);
    RK_S32 timedwait(Mutex& mutex, RK_S64 timeout);
    RK_S32 timedwait(Mutex* mutex, RK_S64 timeout);
    RK_S32 signal();

private:
    pthread_cond_t mCond;
};

inline Condition::Condition()
{
    pthread_cond_init(&mCond, NULL);
}
inline Condition::~Condition()
{
    pthread_cond_destroy(&mCond);
}
inline RK_S32 Condition::wait(Mutex& mutex)
{
    return pthread_cond_wait(&mCond, &mutex.mMutex);
}
inline RK_S32 Condition::wait(Mutex* mutex)
{
    return pthread_cond_wait(&mCond, &mutex->mMutex);
}
inline RK_S32 Condition::timedwait(Mutex& mutex, RK_S64 timeout)
{
    return timedwait(&mutex, timeout);
}
inline RK_S32 Condition::timedwait(Mutex* mutex, RK_S64 timeout)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME_COARSE, &ts);

    ts.tv_sec += timeout / 1000;
    ts.tv_nsec += (timeout % 1000) * 1000000;
    /* Prevent the out of range at nanoseconds field */
    ts.tv_sec += ts.tv_nsec / 1000000000;
    ts.tv_nsec %= 1000000000;

    return pthread_cond_timedwait(&mCond, &mutex->mMutex, &ts);
}
inline RK_S32 Condition::signal()
{
    return pthread_cond_signal(&mCond);
}

class MppMutexCond
{
public:
    MppMutexCond() {};
    ~MppMutexCond() {};

    void    lock()      { mLock.lock(); }
    void    unlock()    { mLock.unlock(); }
    void    trylock()   { mLock.trylock(); }
    void    wait()      { mCondition.wait(mLock); }
    RK_S32  wait(RK_S64 timeout) { return mCondition.timedwait(mLock, timeout); }
    void    signal()    { mCondition.signal(); }
    Mutex   *mutex()    { return &mLock; }

private:
    Mutex           mLock;
    Condition       mCondition;
};

// Thread lock / signal is distinguished by its source
typedef enum MppThreadSignal_e {
    THREAD_WORK,        // for working loop
    THREAD_INPUT,       // for thread input
    THREAD_OUTPUT,      // for thread output
    THREAD_CONTROL,     // for thread async control (reset)
    THREAD_SIGNAL_BUTT,
} MppThreadSignal;

#define THREAD_NORMAL       0
#define THRE       0

class MppThread
{
public:
    MppThread(MppThreadFunc func, void *ctx, const char *name = NULL);
    ~MppThread() {};

    MppThreadStatus get_status(MppThreadSignal id = THREAD_WORK);
    void set_status(MppThreadStatus status, MppThreadSignal id = THREAD_WORK);
    void dump_status();

    void start();
    void stop();

    void lock(MppThreadSignal id = THREAD_WORK) {
        mpp_assert(id < THREAD_SIGNAL_BUTT);
        mMutexCond[id].lock();
    }

    void unlock(MppThreadSignal id = THREAD_WORK) {
        mpp_assert(id < THREAD_SIGNAL_BUTT);
        mMutexCond[id].unlock();
    }

    void wait(MppThreadSignal id = THREAD_WORK) {
        mpp_assert(id < THREAD_SIGNAL_BUTT);
        MppThreadStatus status = mStatus[id];

        mStatus[id] = MPP_THREAD_WAITING;
        mMutexCond[id].wait();

        // check the status is not changed then restore status
        if (mStatus[id] == MPP_THREAD_WAITING)
            mStatus[id] = status;
    }

    void signal(MppThreadSignal id = THREAD_WORK) {
        mpp_assert(id < THREAD_SIGNAL_BUTT);
        mMutexCond[id].signal();
    }

    Mutex *mutex(MppThreadSignal id = THREAD_WORK) {
        mpp_assert(id < THREAD_SIGNAL_BUTT);
        return mMutexCond[id].mutex();
    }

private:
    pthread_t       mThread;
    MppMutexCond    mMutexCond[THREAD_SIGNAL_BUTT];
    MppThreadStatus mStatus[THREAD_SIGNAL_BUTT];

    MppThreadFunc   mFunction;
    char            mName[THREAD_NAME_LEN];
    void            *mContext;

    MppThread();
    MppThread(const MppThread &);
    MppThread &operator=(const MppThread &);
};

#endif

#endif /*__MPP_THREAD_H__*/
