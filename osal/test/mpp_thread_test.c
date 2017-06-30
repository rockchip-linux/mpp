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

#define MODULE_TAG "mpp_thread_test"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"

#include "mpp_log.h"
#include "mpp_thread.h"
#include "mpp_time.h"

// TODO: add thread mutex and condition test case

#define MAX_THREAD_NUM      10
#define MAX_LOCK_LOOP       10000

static RK_S32 thread_debug = 0;
#define thread_dbg(fmt, ...)    _mpp_dbg(thread_debug, 1, fmt, ## __VA_ARGS__)

static pthread_mutex_t mutex_0;
static pthread_mutex_t mutex_1;
static pthread_cond_t cond_0;
static pthread_cond_t cond_1;
static RK_S32 flag_0 = 1;
static RK_S32 flag_1 = 1;

void *thread_test(void *pdata)
{
    int idx = *((int*)pdata);
    mpp_log("thread %d is running\n", idx);
    sleep(1);
    mpp_log("thread %d done\n", idx);
    return NULL;
}

void* mutex_performance_test_loop_0(void *arg)
{
    RK_S32 i = 0;

    for (i = 0; i < MAX_LOCK_LOOP; i++) {
        thread_dbg("0 %5d lock\n", i);
        pthread_mutex_lock(&mutex_0);

        thread_dbg("0 %5d wait\n", i);
        if (flag_0) {
            pthread_cond_wait(&cond_0, &mutex_0);
        }

        thread_dbg("0 %5d signal\n", i);
        pthread_mutex_lock(&mutex_1);
        flag_1 = 0;
        pthread_cond_signal(&cond_1);
        pthread_mutex_unlock(&mutex_1);

        thread_dbg("0 %5d unlock\n", i);
        flag_0 = 1;
        pthread_mutex_unlock(&mutex_0);
    }
    (void)arg;
    return NULL;
}

void *mutex_performance_test_loop_1(void *arg)
{
    RK_S32 i = 0;

    flag_0 = 0;

    for (i = 0; i < MAX_LOCK_LOOP; i++) {
        thread_dbg("1 %5d lock\n", i);
        pthread_mutex_lock(&mutex_1);

        thread_dbg("1 %5d wait\n", i);
        if (flag_1) {
            pthread_cond_wait(&cond_1, &mutex_1);
        }

        thread_dbg("1 %5d signal\n", i);
        pthread_mutex_lock(&mutex_0);
        flag_0 = 0;
        pthread_cond_signal(&cond_0);
        pthread_mutex_unlock(&mutex_0);

        thread_dbg("1 %5d unlock\n", i);
        flag_1 = 1;
        pthread_mutex_unlock(&mutex_1);
    }
    (void)arg;
    return NULL;
}

void mutex_performance_test_once(void)
{
    pthread_mutexattr_t attr;
    pthread_mutex_t mutex;

    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&mutex, &attr);
    pthread_mutexattr_destroy(&attr);

    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_destroy(&mutex);
}

int main()
{
    int i;
    int pdata[MAX_THREAD_NUM];
    pthread_t threads[MAX_THREAD_NUM];
    pthread_attr_t attr;
    pthread_mutexattr_t mutex_attr;
    void *dummy;
    RK_S64 time_start, time_end;

    mpp_log("vpu test start\n");
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pdata[i] = i;
        pthread_create(&threads[i], &attr, thread_test, &pdata[i]);
    }

    sleep(2);

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_join(threads[i], &dummy);
    }

    mpp_debug = MPP_DBG_TIMING;
    time_start = mpp_time();

    for (i = 0; i < MAX_LOCK_LOOP; i++)
        mutex_performance_test_once();

    time_end = mpp_time();
    mpp_time_diff(time_start, time_end, 0, "lock unlock test");

    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);

    pthread_mutex_init(&mutex_0, &mutex_attr);
    pthread_mutex_init(&mutex_1, &mutex_attr);
    pthread_cond_init(&cond_0, NULL);
    pthread_cond_init(&cond_1, NULL);
    pthread_mutexattr_destroy(&mutex_attr);

    time_start = mpp_time();
    pthread_create(&threads[0], &attr, mutex_performance_test_loop_0, NULL);
    pthread_create(&threads[1], &attr, mutex_performance_test_loop_1, NULL);

    pthread_join(threads[0], &dummy);
    pthread_join(threads[1], &dummy);
    time_end = mpp_time();
    mpp_time_diff(time_start, time_end, 0, "lock and signal test");

    pthread_cond_destroy(&cond_0);
    pthread_cond_destroy(&cond_1);

    pthread_attr_destroy(&attr);

    mpp_debug = 0;
    mpp_log("vpu test end\n");
    return 0;
}

