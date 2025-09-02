/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_rt_test"

#include <stdio.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>

#ifdef __GLIBC__
#include <gnu/libc-version.h>
#endif

#ifdef __ANDROID__
#include <android/api-level.h>
#include <sys/system_properties.h>
#endif

#if defined(__USE_XOPEN2K) || (defined(__ANDROID__) && __ANDROID_API__ >= 21)
#define COND_USE_CLOCK_MONOTONIC
#endif

#include "mpp_log.h"
#include "mpp_runtime.h"

clockid_t clock_id = CLOCK_REALTIME;

rk_s32 allocator_check()
{
    if (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_ION))
        mpp_logi("mpp found ion buffer is valid\n");
    else
        mpp_logi("mpp found ion buffer is invalid\n");

    if (mpp_rt_allcator_is_valid(MPP_BUFFER_TYPE_DRM))
        mpp_logi("mpp found drm buffer is valid\n");
    else
        mpp_logi("mpp found drm buffer is invalid\n");

    return MPP_OK;
}

#if defined(__ANDROID__)
// Android OS
int os_check()
{
    char version_str[PROP_VALUE_MAX];
    char sdk_str[PROP_VALUE_MAX];
    rk_u32 sdk_api_level = 0;

    mpp_logi("Compiled for Android System.\n");

    __system_property_get("ro.build.version.sdk", sdk_str);
    __system_property_get("ro.build.version.release", version_str);
    sdk_api_level = atoi(sdk_str);

    mpp_logi("Running on Android %s, api level %d, program compiled with api level %d.\n",
             version_str, sdk_api_level, __ANDROID_API__);

    if (sdk_api_level < __ANDROID_API__)
        mpp_logi("Warning!!! Target is compiled for a higher api level than current system.\n");

    // check libc
#if defined(__BIONIC__)
    mpp_logi("Using bionic libc.\n");
#else
    mpp_logi("Unknown libc.\n");
#endif // __BIONIC__

// POSIX C compatibility test
    if (__ANDROID_API__ >= 21) {
        clock_id = CLOCK_MONOTONIC;
        mpp_logi("pthread_condattr_setclock is defined.\n");

        if (__ANDROID_API__ >= 30)
            mpp_logi("pthread_cond_clockwait is defined.\n");
        else
            mpp_logi("pthread_cond_clockwait is not defined.\n");
    } else {
        mpp_logi("pthread_condattr_setclock is not defined.\n");
    }

    return 0;
}

#else

// Linux OS
int os_check()
{
#if defined(linux) && !defined(__ANDROID__)
    mpp_logi("Compiled for Linux.\n");
#else
    mpp_logi("Compiled for Unknown OS.\n");
#endif // OS check

    // check libc
#if defined(__GLIBC__)
    mpp_logi("Compiled with glibc %d-%d, running with glibc %s.\n", __GLIBC__, __GLIBC_MINOR__, gnu_get_libc_version());
#elif defined(__UCLIBC__)
    mpp_logi("Using uClibc.\n");
#if defined(__UCLIBC_MAJOR__) && defined(__UCLIBC_MINOR__) && defined(__UCLIBC_SUBLEVEL__)
    mpp_logi("Compiled with uClibc version: %d.%d.%d.\n",
             __UCLIBC_MAJOR__, __UCLIBC_MINOR__, __UCLIBC_SUBLEVEL__);
#else
    mpp_logi("uClibc version macros not available.\n");
#endif
    mpp_logi("pthread_cond_clockwait is not defined.\n");
#elif defined(__MUSL__)
    mpp_logi("Using musl libc.\n");
#else
    mpp_logi("Unknown libc.\n");
#endif // LIBC check

// POSIX C compatibility test
#if defined(__USE_XOPEN2K)
    clock_id = CLOCK_MONOTONIC;
    mpp_logi("XPG6 is supported.\n");
    mpp_logi("pthread_condattr_setclock is defined.\n");
#else
    mpp_logi("XPG6 is not supported.\n");
    mpp_logi("pthread_condattr_setclock is not defined.\n");
#endif // __USE_XOPEN2K

    return 0;
}
#endif // USE_XOPEN2K

#ifdef COND_USE_CLOCK_MONOTONIC
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond;
rk_u32 condition = 0;

void *wait_thread()
{
    int ret;
    struct timespec ts, ts_s, ts_e;
    char time_str[64];
    struct tm *tm_info;

    pthread_mutex_lock(&mutex);

    clock_gettime(CLOCK_MONOTONIC, &ts);
    clock_gettime(CLOCK_REALTIME, &ts_s);

    ts.tv_sec += 1; // timeout after 1 sec
    tm_info = localtime(&ts_s.tv_sec);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    mpp_logi("Current time is: %s.%08ld.\n", time_str, ts_s.tv_nsec);
    mpp_logi("Thread is waiting at monotonic time: %ld sec %ld nsec, will be timeout after 1 sec.\n",
             ts.tv_sec, ts.tv_nsec);

    while (!condition) {
        ret = pthread_cond_timedwait(&cond, &mutex, &ts);

        if (ret == ETIMEDOUT) {
            clock_gettime(CLOCK_REALTIME, &ts_e);
            tm_info = localtime(&ts_e.tv_sec);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

            mpp_logi("Timeout expired at %s.%09ld.\n", time_str, ts_e.tv_nsec);

            if ((ts_e.tv_sec - ts_s.tv_sec) != 1)
                mpp_loge("Error! Timeout expired too soon or too late.\n");
            else
                mpp_logi("pthrad_cond_timedwait is reliable when using CLOCK_MONOTONIC\n");

            pthread_mutex_unlock(&mutex);
            return NULL;
        }
    }

    mpp_logi("Thread is waked up.");
    pthread_mutex_unlock(&mutex);
    return NULL;
}

int check_pthread_clock()
{
    pthread_condattr_t cond_attr;
    pthread_t thread;

    pthread_condattr_init(&cond_attr);
    if (pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC)) {
        mpp_loge("pthread is not supported with CLOCK_MONOTONIC\n");
        pthread_condattr_destroy(&cond_attr);
        pthread_mutex_destroy(&mutex);
        return -1;
    }
    pthread_cond_init(&cond, &cond_attr);

    pthread_create(&thread, NULL, wait_thread, NULL);
    pthread_join(thread, NULL);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    pthread_condattr_destroy(&cond_attr);

    return 0;
}
#endif // COND_USE_CLOCK_MONOTONIC

int main()
{
    allocator_check();
    // system compatibility test
#if defined(__LP64__)
    mpp_logi("This is 64-bit program.\n");
#else
    mpp_logi("This is 32-bit program.\n");
#endif // __LP64__

    os_check();

#ifdef COND_USE_CLOCK_MONOTONIC
    if (check_pthread_clock())
        mpp_loge("Warning! pthread_cond_timedwait will not be reliable!\n");
#else
    mpp_logi("pthread clock compatibility checking is skipped.\n");
    mpp_loge("Warning! pthread_cond_timedwait will not be reliable!\n");
#endif // COND_USE_CLOCK_MONOTONIC

    return 0;
}
