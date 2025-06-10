/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_TIME_H__
#define __MPP_TIME_H__

#include <unistd.h>

#include "mpp_thread.h"

#define msleep(x)               usleep((x)*1000)

typedef void* MppClock;
typedef void* MppTimer;
typedef void* MppStopwatch;

#ifdef __cplusplus
extern "C" {
#endif

rk_s64 mpp_time();
void mpp_time_diff(rk_s64 start, rk_s64 end, rk_s64 limit, const char *fmt);

/*
 * Clock create / destroy / enable / disable function
 * Note when clock is create it is default disabled user need to call enable
 * fucntion with enable = 1 to enable the clock.
 * User can use enable function with enable = 0 to disable the clock.
 */
MppClock mpp_clock_get(const char *name);
void mpp_clock_put(MppClock clock);
void mpp_clock_enable(MppClock clock, rk_u32 enable);

/*
 * Clock basic operation function:
 * start : let clock start timing counter
 * pause : let clock pause and return the diff to start time
 * reset : let clock counter to all zero
 */
rk_s64 mpp_clock_start(MppClock clock);
rk_s64 mpp_clock_pause(MppClock clock);
rk_s64 mpp_clock_reset(MppClock clock);

/*
 * These clock helper function can only be call when clock is paused:
 * mpp_clock_get_sum    - Return clock sum up total value
 * mpp_clock_get_count  - Return clock sum up counter value
 * mpp_clock_get_name   - Return clock name
 */
rk_s64 mpp_clock_get_sum(MppClock clock);
rk_s64 mpp_clock_get_count(MppClock clock);
const char *mpp_clock_get_name(MppClock clock);

/*
 * MppTimer is for timer with callback function
 * It will provide the ability to repeat doing something until it is
 * disalble or put.
 *
 * Timer work flow:
 *
 * 1. mpp_timer_get
 * 2. mpp_timer_set_callback
 * 3. mpp_timer_set_timing(initial, interval)
 * 4. mpp_timer_set_enable(initial, 1)
 *    ... running ...
 * 5. mpp_timer_set_enable(initial, 0)
 * 6. mpp_timer_put
 */
MppTimer mpp_timer_get(const char *name);
void mpp_timer_set_callback(MppTimer timer, MppThreadFunc func, void *ctx);
void mpp_timer_set_timing(MppTimer timer, rk_s32 initial, rk_s32 interval);
void mpp_timer_set_enable(MppTimer timer, rk_s32 enable);
void mpp_timer_put(MppTimer timer);

/*
 * MppStopwatch is for timer to record event and time
 *
 * Stopwatch work flow:
 *
 * 1. mpp_stopwatch_get
 * 2. mpp_stopwatch_setup(max_count, show_on_exit)
 * 3. mpp_stopwatch_record(event)
 *    ... running ...
 * 4. mpp_stopwatch_record(event)
 * 5. mpp_stopwatch_put (show events and time)
 */
MppStopwatch mpp_stopwatch_get(const char *name);
void mpp_stopwatch_set_show_on_exit(MppStopwatch stopwatch, rk_s32 show_on_exit);
void mpp_stopwatch_record(MppStopwatch stopwatch, const char *event);
void mpp_stopwatch_put(MppStopwatch timer);
rk_s64 mpp_stopwatch_elapsed_time(MppStopwatch stopwatch);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TIME_H__*/
