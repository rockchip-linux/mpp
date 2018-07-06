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

// TODO: add usleep function on windows

#if defined(_WIN32) && !defined(__MINGW32CE__)
#include <windows.h>
#define msleep                  Sleep
#define sleep(x)                Sleep((x)*1000)
#else
#include <unistd.h>
#define msleep(x)               usleep((x)*1000)
#endif

typedef void* MppTimer;

#ifdef __cplusplus
extern "C" {
#endif

RK_S64 mpp_time();
void mpp_time_diff(RK_S64 start, RK_S64 end, RK_S64 limit, const char *fmt);

/*
 * Timer create / destroy / enable / disable function
 * Note when timer is create it is default disabled user need to call enable
 * fucntion with enable = 1 to enable the timer.
 * User can use enable function with enable = 0 to disable the timer.
 */
MppTimer mpp_timer_get(const char *name);
void mpp_timer_put(MppTimer timer);
void mpp_timer_enable(MppTimer timer, RK_U32 enable);

/*
 * Timer basic operation function:
 * start : let timer start timing counter
 * pause : let timer pause and return the diff to start time
 * reset : let timer counter to all zero
 */
RK_S64 mpp_timer_start(MppTimer timer);
RK_S64 mpp_timer_pause(MppTimer timer);
RK_S64 mpp_timer_reset(MppTimer timer);

/*
 * These timer helper function can only be call when timer is paused:
 * mpp_timer_get_sum    - Return timer sum up total value
 * mpp_timer_get_count  - Return timer sum up counter value
 * mpp_timer_get_name   - Return timer name
 */
RK_S64 mpp_timer_get_sum(MppTimer timer);
RK_S64 mpp_timer_get_count(MppTimer timer);
const char *mpp_timer_get_name(MppTimer timer);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TIME_H__*/

