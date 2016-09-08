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

#ifdef __cplusplus
extern "C" {
#endif

RK_S64 mpp_time();
void mpp_time_diff(RK_S64 start, RK_S64 end, RK_S64 limit, char *fmt);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_TIME_H__*/

