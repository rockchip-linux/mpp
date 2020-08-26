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

#define MODULE_TAG "mpp_eventfd_test"

#include "mpp_log.h"
#include "mpp_thread.h"
#include "mpp_eventfd.h"

static RK_S32 fd = -1;
static RK_U64 wr_val = 0xfedcba;

void *thread_wait(void *arg)
{
    (void) arg;
    RK_U64 rd_val = 0xabcdef;
    RK_S32 ret = mpp_eventfd_read(fd, &rd_val, -1);

    mpp_log("eventfd timed read 0x%llx ret %d", rd_val, ret);

    return NULL;
}

int main()
{
    RK_S32 ret = 0;
    pthread_t td;

    mpp_log("eventfd test start\n");

    fd = mpp_eventfd_get(0);

    mpp_log("eventfd get fd %d", fd);

    pthread_create(&td, NULL, thread_wait, NULL);

    ret = mpp_eventfd_write(fd, wr_val);
    mpp_log("eventfd write 0x%llx ret %d", wr_val, ret);

    pthread_join(td, NULL);

    mpp_eventfd_put(fd);

    mpp_log("eventfd test done\n");

    return 0;
}

