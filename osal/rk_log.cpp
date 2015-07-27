/*
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

#include <stdio.h>
#include <stdarg.h>
#include "rk_log.h"

#ifdef ANDROID
#include <utils/Log.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

static RK_U32 mpp_log_flag = 0;

void rk_log(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef ANDROID
    ALOGI(fmt, args);
#else
    vfprintf(stdout, fmt, args);
#endif
    va_end(args);
    return ;
}

void rk_err(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
#ifdef ANDROID
    ALOGE(fmt, args);
#else
    vfprintf(stderr, fmt, args);
#endif
    va_end(args);
    return ;
}

void rk_set_log_flag(RK_U32 flag)
{
    mpp_log_flag = flag;
    return ;
}

RK_U32 rk_get_log_flag()
{
    return mpp_log_flag;
}


#ifdef __cplusplus
}
#endif
