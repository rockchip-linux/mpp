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

#define MODULE_TAG "mpp_time"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#if _WIN32
#include <sys/types.h>
#include <sys/timeb.h>

RK_S64 mpp_time()
{
    struct timeb tb;
    ftime(&tb);
    return ((RK_S64)tb.time * 1000 + (RK_S64)tb.millitm) * 1000;
}

#else
#include <time.h>

RK_S64 mpp_time()
{
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    return (RK_S64)time.tv_sec * 1000000 + (RK_S64)time.tv_nsec / 1000;
}

#endif

void mpp_time_diff(RK_S64 start, RK_S64 end, RK_S64 limit, const char *fmt)
{
    RK_S64 diff = end - start;
    if (diff >= limit)
        mpp_dbg(MPP_DBG_TIMING, "%s timing %lld us\n", fmt, diff);
}

typedef struct MppClockImpl_t {
    const char *check;
    char    name[16];
    RK_U32  enable;
    RK_S64  base;
    RK_S64  time;
    RK_S64  sum;
    RK_S64  count;
} MppClockImpl;

static const char *module_name = MODULE_TAG;

MPP_RET check_is_mpp_clock(void *clock)
{
    if (clock && ((MppClockImpl*)clock)->check == module_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", clock);
    mpp_abort();
    return MPP_NOK;
}

MppClock mpp_clock_get(const char *name)
{
    MppClockImpl *impl = mpp_calloc(MppClockImpl, 1);
    if (impl) {
        impl->check = module_name;
        snprintf(impl->name, sizeof(impl->name), name, NULL);
    } else
        mpp_err_f("malloc failed\n");

    return impl;
}

void mpp_clock_put(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return ;
    }

    mpp_free(clock);
}

void mpp_clock_enable(MppClock clock, RK_U32 enable)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
    } else {
        MppClockImpl *p = (MppClockImpl *)clock;
        p->enable = (enable) ? (1) : (0);
    }
}

RK_S64 mpp_clock_start(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return 0;
    }

    MppClockImpl *p = (MppClockImpl *)clock;

    if (!p->enable)
        return 0;

    p->base = mpp_time();
    p->time = 0;
    return p->base;
}

RK_S64 mpp_clock_pause(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return 0;
    }

    MppClockImpl *p = (MppClockImpl *)clock;

    if (!p->enable)
        return 0;

    RK_S64 time = mpp_time();

    if (!p->time) {
        // first pause after start
        p->sum += time - p->base;
        p->count++;
    }
    p->time = time;
    return p->time - p->base;
}

RK_S64 mpp_clock_reset(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
    } else {
        MppClockImpl *p = (MppClockImpl *)clock;

        p->base = 0;
        p->time = 0;
        p->sum = 0;
        p->count = 0;
    }

    return 0;
}

RK_S64 mpp_clock_get_sum(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return 0;
    }

    MppClockImpl *p = (MppClockImpl *)clock;
    return (p->enable) ? (p->sum) : (0);
}

RK_S64 mpp_clock_get_count(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return 0;
    }

    MppClockImpl *p = (MppClockImpl *)clock;
    return (p->enable) ? (p->count) : (0);
}

const char *mpp_clock_get_name(MppClock clock)
{
    if (NULL == clock || check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return NULL;
    }

    MppClockImpl *p = (MppClockImpl *)clock;
    return p->name;
}
