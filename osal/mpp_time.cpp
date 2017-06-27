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

#include "mpp_log.h"
#include "mpp_time.h"

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>

RK_S64 mpp_time()
{
    if (!(mpp_debug & MPP_DBG_TIMING))
        return 0;

    struct timeb tb;
    ftime(&tb);
    return ((RK_S64)tb.time * 1000 + (RK_S64)tb.millitm) * 1000;
}

#else
#include <sys/time.h>

RK_S64 mpp_time()
{
    if (!(mpp_debug & MPP_DBG_TIMING))
        return 0;

    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (RK_S64)tv_date.tv_sec * 1000000 + (RK_S64)tv_date.tv_usec;
}

#endif

void mpp_time_diff(RK_S64 start, RK_S64 end, RK_S64 limit, char *fmt)
{
    if (!(mpp_debug & MPP_DBG_TIMING))
        return;

    RK_S64 diff = end - start;
    if (diff >= limit)
        mpp_dbg(MPP_DBG_TIMING, "%s timing %lld us\n", fmt, diff);
}

