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

#ifndef __MPP_TIME_H__
#define __MPP_TIME_H__

#include "rk_type.h"
#include "mpp_thread.h"

#if defined(_WIN32) && !defined(__MINGW32CE__)
#include <windows.h>
#define msleep                  Sleep
#define sleep(x)                Sleep((x)*1000)
#else
#include <unistd.h>
#define msleep(x)               usleep((x)*1000)
#endif

typedef void* MppClock;
typedef void* MppTimer;
typedef void* MppStopwatch;

#ifdef __cplusplus
extern "C" {
#endif

RK_S64 mpp_time();
void mpp_time_diff(RK_S64 start, RK_S64 end, RK_S64 limit, const char *fmt);

/*
 * Clock create / destroy / enable / disable function
 * Note when clock is create it is default disabled user need to call enable
 * fucntion with enable = 1 to enable the clock.
 * User can use enable function with enable = 0 to disable the clock.
 */
MppClock mpp_clock_get(const char *name);
void mpp_clock_put(MppClock clock);
void mpp_clock_enable(MppClock clock, RK_U32 enable);

/*
 * Clock basic operation function:
 * start : let clock start timing counter
 * pause : let clock pause and return the diff to start time
 * reset : let clock counter to all zero
 */
RK_S64 mpp_clock_start(MppClock clock);
RK_S64 mpp_clock_pause(MppClock clock);
RK_S64 mpp_clock_reset(MppClock clock);

/*
 * These clock helper function can only be call when clock is paused:
 * mpp_clock_get_sum    - Return clock sum up total value
 * mpp_clock_get_count  - Return clock sum up counter value
 * mpp_clock_get_name   - Return clock name
 */
RK_S64 mpp_clock_get_sum(MppClock clock);
RK_S64 mpp_clock_get_count(MppClock clock);
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
void mpp_timer_set_timing(MppTimer timer, RK_S32 initial, RK_S32 interval);
void mpp_timer_set_enable(MppTimer timer, RK_S32 enable);
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
void mpp_stopwatch_set_show_on_exit(MppStopwatch stopwatch, RK_S32 show_on_exit);
void mpp_stopwatch_record(MppStopwatch stopwatch, const char *event);
void mpp_stopwatch_put(MppStopwatch timer);
RK_S64 mpp_stopwatch_elapsed_time(MppStopwatch stopwatch);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class AutoTiming
{
public:
    AutoTiming(const char *name = __FUNCTION__);
    ~AutoTiming();
private:
    const char  *mName;
    RK_S64      mStart;
    RK_S64      mEnd;

    AutoTiming(const AutoTiming &);
    AutoTiming &operator = (const AutoTiming&);
};

#endif

#define AUTO_TIMER_STRING(name, cnt)        name ## cnt
#define AUTO_TIMER_NAME_STRING(name, cnt)   AUTO_TIMER_STRING(name, cnt)
#define AUTO_TIMER_NAME(name)               AUTO_TIMER_NAME_STRING(name, __COUNTER__)
#define AUTO_TIMING()                       AutoTiming AUTO_TIMER_NAME(auto_timing)(__FUNCTION__)

#endif /*__MPP_TIME_H__*/
