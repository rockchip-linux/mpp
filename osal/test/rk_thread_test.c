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

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <signal.h>

#include "rk_type.h"
#include "rk_log.h"
#include "rk_thread.h"

#define MAX_THREAD_NUM      10
void *thread_test(void *pdata)
{
    (void)pdata;
    for (;;) {
        int fd = open("/dev/vpu_service", O_RDWR);
        if (fd < 0) {
            rk_log("failed to open /dev/vpu_service ret %d\n", fd);
            return NULL;
        }
        close(fd);
    }
    return NULL;
}

int main()
{
    int i;

    rk_log("vpu test start\n");
    pthread_t threads[MAX_THREAD_NUM];
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_create(&threads[i], &attr, thread_test, NULL);
    }
    pthread_attr_destroy(&attr);
    for (i = 0; i < 2; i++)
        sleep(1);

    void *dummy;

    for (i = 0; i < MAX_THREAD_NUM; i++) {
        pthread_join(threads[i], &dummy);
    }

    rk_log("vpu test end\n");
    return 0;
}

