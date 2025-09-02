/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_eventfd_test"

#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_eventfd.h"

#define WR_VAL      1
#define RD_VAL      10

static RK_S32 data_fd = -1;
static RK_S32 sync_fd = -1;
static RK_U64 wr_val_base = WR_VAL;
static RK_U64 rd_val_base = RD_VAL;
static RK_U64 wr_val = WR_VAL;
static RK_U64 rd_val = RD_VAL;
static RK_S32 wr_ret = -1;
static RK_S32 rd_ret = -1;
static RK_S64 wr_timeout = -1;
static RK_S64 rd_timeout = -1;

void reset_test()
{
    wr_val = wr_val_base;
    rd_val = rd_val_base;
    wr_ret = -1;
    rd_ret = -1;
    rd_val_base++;

    mpp_eventfd_write(data_fd, 0);
    mpp_eventfd_read(data_fd, NULL, 0);

    mpp_log("eventfd test on writing %lld -> %lld\n", wr_val, rd_val);
}

void *thread_read(void *arg)
{
    (void) arg;
    mpp_eventfd_write(sync_fd, 1);

    rd_ret = mpp_eventfd_read(data_fd, &rd_val, rd_timeout);
    mpp_log("eventfd read %d timeout %d ret %lld val %lld\n", data_fd, rd_timeout, rd_ret, rd_val);

    return NULL;
}

int main()
{
    pthread_t td;

    mpp_log("eventfd test start\n");

    data_fd = mpp_eventfd_get(0);
    sync_fd = mpp_eventfd_get(0);

    mpp_log("eventfd get data_fd %d sync_fd %d\n", data_fd, sync_fd);

    reset_test();
    rd_timeout = -1;
    wr_timeout = 0;

    pthread_create(&td, NULL, thread_read, NULL);
    mpp_eventfd_read(sync_fd, NULL, -1);
    mpp_log("eventfd write %d val %lld\n", data_fd, wr_val);
    wr_ret = mpp_eventfd_write(data_fd, wr_val);
    pthread_join(td, NULL);
    mpp_log("eventfd block mode     test wr: %lld ret %d rd: %lld ret %d - %s\n",
            wr_val, wr_ret, rd_val, rd_ret,
            (wr_val == rd_val) ? "success" : "failed");

    reset_test();
    rd_timeout = 0;
    wr_timeout = 100;

    pthread_create(&td, NULL, thread_read, NULL);
    mpp_eventfd_read(sync_fd, NULL, -1);
    msleep(wr_timeout);
    mpp_log("eventfd write %d val %lld\n", data_fd, wr_val);
    wr_ret = mpp_eventfd_write(data_fd, wr_val);
    pthread_join(td, NULL);
    mpp_log("eventfd non-block mode test wr: %lld ret %d rd: %lld ret %d - %s\n",
            wr_val, wr_ret, rd_val, rd_ret,
            (wr_val != rd_val) ? "success" : "failed");

    reset_test();
    rd_timeout = 100;
    wr_timeout = 1;

    pthread_create(&td, NULL, thread_read, NULL);
    mpp_eventfd_read(sync_fd, NULL, -1);
    msleep(wr_timeout);
    mpp_log("eventfd write %d val %lld\n", data_fd, wr_val);
    wr_ret = mpp_eventfd_write(data_fd, wr_val);
    pthread_join(td, NULL);
    mpp_log("eventfd timeout   mode test wr: %lld ret %d rd: %lld ret %d - %s\n",
            wr_val, wr_ret, rd_val, rd_ret,
            (wr_val == rd_val) ? "success" : "failed");

    reset_test();
    rd_timeout = 1;
    wr_timeout = 100;

    pthread_create(&td, NULL, thread_read, NULL);
    mpp_eventfd_read(sync_fd, NULL, -1);
    msleep(wr_timeout);
    mpp_log("eventfd write %d val %lld\n", data_fd, wr_val);
    wr_ret = mpp_eventfd_write(data_fd, wr_val);
    pthread_join(td, NULL);
    mpp_log("eventfd timeout   mode test wr: %lld ret %d rd: %lld ret %d - %s\n",
            wr_val, wr_ret, rd_val, rd_ret,
            (wr_val != rd_val) ? "success" : "failed");

    mpp_eventfd_put(data_fd);
    mpp_eventfd_put(sync_fd);

    mpp_log("eventfd test done\n");

    return 0;
}

