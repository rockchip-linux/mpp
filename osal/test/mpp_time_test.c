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

#define MODULE_TAG "mpp_time_test"

#include "mpp_log.h"
#include "mpp_time.h"

int main()
{
    RK_S64 time_0;
    RK_S64 time_1;
    MppTimer timer;
    RK_S32 i;

    mpp_log("mpp time test start\n");

    time_0 = mpp_time();

    msleep(10);

    time_1 = mpp_time();

    mpp_log("time 0 %lld us\n", time_0);
    mpp_log("time 1 %lld us\n", time_1);
    mpp_log("diff expected 10 ms real %.2f ms\n", (float)(time_1 - time_0) / 1000);

    mpp_log("mpp time test done\n");

    mpp_log("mpp timer test start\n");

    timer = mpp_timer_get("timer loop test");

    mpp_timer_enable(timer, 1);

    for (i = 0; i < 1000; i++) {
        mpp_timer_start(timer);
        msleep(2);
        mpp_timer_pause(timer);
    }

    mpp_log("loop 1000 times timer 2ms start/pause operation:\n");
    mpp_log("total   time  %8.3f ms\n", mpp_timer_get_sum(timer) / 1000.0);
    mpp_log("average time  %8.3f ms\n",
            mpp_timer_get_sum(timer) / mpp_timer_get_count(timer) / 1000.0);

    mpp_timer_reset(timer);

    for (i = 0; i < 10000; i++) {
        mpp_timer_start(timer);
        mpp_time();
        mpp_timer_pause(timer);
    }

    mpp_log("mpp_time cost %.3f ms\n",
            mpp_timer_get_sum(timer) / mpp_timer_get_count(timer) / 1000.0);

    mpp_timer_reset(timer);
    mpp_timer_start(timer);
    msleep(20);
    time_0 = mpp_timer_pause(timer);
    msleep(20);
    time_1 = mpp_timer_pause(timer);

    mpp_log("mpp_time pause 0 at %.3f ms pause 1 at %.3f ms\n",
            time_0 / 1000.0, time_1 / 1000.0);


    mpp_log("mpp time test done\n");

    return 0;
}
