/*
 *
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

#define MODULE_TAG "rk_thread_test"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "rk_log.h"
#include "rk_thread.h"

// TODO: add thread mutex and condition test case

#define MAX_THREAD_NUM      10
void *thread_test(void *pdata)
{
    int idx = *((int*)pdata);
    rk_log("thread %d is running\n", idx);
    sleep(1);
    rk_log("thread %d done\n", idx);
    return NULL;
}

int main()
{
    int i;
    int pdata[MAX_THREAD_NUM];
    pthread_t threads[MAX_THREAD_NUM];

    rk_log("vpu test start\n");
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pdata[i] = i;
        pthread_create(&threads[i], &attr, thread_test, &pdata[i]);
    }
    pthread_attr_destroy(&attr);
    sleep(2);

    void *dummy;

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_join(threads[i], &dummy);
    }

    rk_log("vpu test end\n");
    return 0;
}

