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

#include <errno.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"
#include "mpp_thread.h"

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

static const char *clock_name = "mpp_clock";

MPP_RET check_is_mpp_clock(void *clock)
{
    if (clock && ((MppClockImpl*)clock)->check == clock_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", clock);
    mpp_abort();
    return MPP_NOK;
}

MppClock mpp_clock_get(const char *name)
{
    MppClockImpl *impl = mpp_calloc(MppClockImpl, 1);
    if (impl) {
        impl->check = clock_name;
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

typedef struct MppTimerImpl_t {
    const char          *check;
    char                name[16];

    RK_S32              enabled;
    RK_S32              initial;
    RK_S32              interval;
    RK_S32              timer_fd;
    RK_S32              epoll_fd;

    MppThread           *thd;
    MppThreadFunc       func;
    void                *ctx;
} MppTimerImpl;

static const char *timer_name = "mpp_timer";

MPP_RET check_is_mpp_timer(void *timer)
{
    if (timer && ((MppTimerImpl*)timer)->check == timer_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", timer);
    mpp_abort();
    return MPP_NOK;
}

static void *mpp_timer_thread(void *ctx)
{
    struct itimerspec ts;
    RK_S32 ret = 0;
    MppTimerImpl *impl = (MppTimerImpl *)ctx;
    MppThread *thd = impl->thd;
    RK_S32 timer_fd = impl->timer_fd;

    // first expire time
    ts.it_value.tv_sec = impl->initial / 1000;
    ts.it_value.tv_nsec = (impl->initial % 1000) * 1000;

    // last expire time
    ts.it_interval.tv_sec = impl->interval / 1000;
    ts.it_interval.tv_nsec = (impl->interval % 1000) * 1000;

    ret = timerfd_settime(timer_fd, 0, &ts, NULL);
    if (ret < 0) {
        mpp_err("timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
        return NULL;
    }

    mpp_log("timer %p start looping\n", impl);

    while (1) {
        if (MPP_THREAD_RUNNING != thd->get_status())
            break;

        struct epoll_event events;

        memset(&events, 0, sizeof(events));

        /* wait epoll event */
        RK_S32 fd_cnt = epoll_wait(impl->epoll_fd, &events, 1, 500);
        if (fd_cnt && (events.events & EPOLLIN) && (events.data.fd == timer_fd)) {
            RK_U64 exp = 0;

            ssize_t cnt = read(timer_fd, &exp, sizeof(exp));
            mpp_assert(cnt == sizeof(exp));
            impl->func(impl->ctx);
        }
    }

    return NULL;
}

MppTimer mpp_timer_get(const char *name)
{
    RK_S32 timer_fd = -1;
    RK_S32 epoll_fd = -1;
    MppTimerImpl *impl = NULL;

    do {
        struct epoll_event event;

        impl = mpp_calloc(MppTimerImpl, 1);
        if (NULL == impl) {
            mpp_err_f("malloc failed\n");
            break;
        }

        timer_fd = timerfd_create(CLOCK_REALTIME, 0);
        if (timer_fd < 0)
            break;

        epoll_fd = epoll_create(1);
        if (epoll_fd < 0)
            break;

        memset(&event, 0, sizeof(event));
        event.data.fd = timer_fd;
        event.events = EPOLLIN | EPOLLET;

        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &event) < 0)
            break;

        impl->timer_fd = timer_fd;
        impl->epoll_fd = epoll_fd;
        /* default 1 second (1000ms) looper */
        impl->initial  = 1000;
        impl->interval = 1000;
        impl->check = timer_name;
        snprintf(impl->name, sizeof(impl->name), name, NULL);

        return impl;
    } while (0);

    mpp_err_f("failed to create timer\n");

    if (impl) {
        mpp_free(impl);
        impl = NULL;
    }

    if (timer_fd >= 0) {
        close(timer_fd);
        timer_fd = -1;
    }

    if (epoll_fd >= 0) {
        close(epoll_fd);
        epoll_fd = -1;
    }

    return NULL;
}

void mpp_timer_set_callback(MppTimer timer, MppThreadFunc func, void *ctx)
{
    if (NULL == timer || check_is_mpp_timer(timer)) {
        mpp_err_f("invalid timer %p\n", timer);
        return ;
    }

    if (NULL == func) {
        mpp_err_f("invalid NULL callback\n");
        return ;
    }

    MppTimerImpl *impl = (MppTimerImpl *)timer;
    impl->func = func;
    impl->ctx = ctx;
}

void mpp_timer_set_timing(MppTimer timer, RK_S32 initial, RK_S32 interval)
{
    if (NULL == timer || check_is_mpp_timer(timer)) {
        mpp_err_f("invalid timer %p\n", timer);
        return ;
    }

    MppTimerImpl *impl = (MppTimerImpl *)timer;
    impl->initial = initial;
    impl->interval = interval;
}

void mpp_timer_set_enable(MppTimer timer, RK_S32 enable)
{
    if (NULL == timer || check_is_mpp_timer(timer)) {
        mpp_err_f("invalid timer %p\n", timer);
        return ;
    }

    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (NULL == impl->func || impl->initial < 0 || impl->interval < 0) {
        mpp_err_f("invalid func %p initial %d interval %d\n",
                  impl->func, impl->initial, impl->interval);
        return ;
    }

    if (enable) {
        if (!impl->enabled && NULL == impl->thd) {
            MppThread *thd = new MppThread(mpp_timer_thread, impl, impl->name);
            if (thd) {
                impl->thd = thd;
                impl->enabled = 1;
                thd->start();
            }
        }
    } else {
        if (impl->enabled && impl->thd) {
            impl->thd->stop();
            impl->enabled = 0;
        }
    }
}

void mpp_timer_put(MppTimer timer)
{
    if (NULL == timer || check_is_mpp_timer(timer)) {
        mpp_err_f("invalid timer %p\n", timer);
        return ;
    }

    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (impl->enabled)
        mpp_timer_set_enable(timer, 0);

    if (impl->timer_fd >= 0) {
        close(impl->timer_fd);
        impl->timer_fd = -1;
    }

    if (impl->epoll_fd >= 0) {
        close(impl->epoll_fd);
        impl->epoll_fd = -1;
    }

    if (impl->thd) {
        delete impl->thd;
        impl->thd = NULL;
    }

    if (impl) {
        mpp_free(impl);
        impl = NULL;
    }
}

AutoTiming::AutoTiming(const char *name)
{
    mStart = mpp_time();
    mName = name;
}

AutoTiming::~AutoTiming()
{
    mEnd = mpp_time();
    mpp_log("%s timing %lld us\n", mName, mEnd - mStart);
}

#define STOPWATCH_TRACE_STR_LEN 64

typedef struct MppStopwatchNode_t {
    char                event[STOPWATCH_TRACE_STR_LEN];
    RK_S64              time;
} MppStopwatchNode;

typedef struct MppStopwatchImpl_t {
    const char          *check;
    char                name[STOPWATCH_TRACE_STR_LEN];

    RK_S32              max_count;
    RK_S32              filled_count;
    RK_S32              show_on_exit;
    RK_S32              log_len;
    RK_S64              time_elipsed;

    MppStopwatchNode    *nodes;
} MppStopwatchImpl;

static const char *stopwatch_name = "mpp_stopwatch";

MPP_RET check_is_mpp_stopwatch(void *stopwatch)
{
    if (stopwatch && ((MppStopwatchImpl*)stopwatch)->check == stopwatch_name)
        return MPP_OK;

    mpp_err_f("pointer %p failed on check\n", stopwatch);
    mpp_abort();
    return MPP_NOK;
}

MppStopwatch mpp_stopwatch_get(const char *name)
{
    MppStopwatchImpl *impl = mpp_calloc(MppStopwatchImpl, 1);
    MppStopwatchNode *nodes = mpp_calloc(MppStopwatchNode, 8);

    if (impl && nodes) {
        impl->check = stopwatch_name;
        snprintf(impl->name, sizeof(impl->name) - 1, name, NULL);
        impl->nodes = nodes;
        impl->max_count = 8;
    } else {
        mpp_err_f("malloc failed\n");
        MPP_FREE(impl);
        MPP_FREE(nodes);
    }

    return impl;
}

void mpp_stopwatch_set_show_on_exit(MppStopwatch stopwatch, RK_S32 show_on_exit)
{
    if (NULL == stopwatch || check_is_mpp_stopwatch(stopwatch)) {
        mpp_err_f("invalid stopwatch %p\n", stopwatch);
        return ;
    }

    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;
    impl->show_on_exit = show_on_exit;
}

void mpp_stopwatch_record(MppStopwatch stopwatch, const char *event)
{
    /* do not print noisy log */
    if (NULL == stopwatch)
        return ;

    if (check_is_mpp_stopwatch(stopwatch)) {
        mpp_err_f("invalid stopwatch %p on %s\n", stopwatch, event);
        return ;
    }

    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;
    if (impl->filled_count >= impl->max_count) {
        RK_S32 max_count = impl->max_count * 2;
        MppStopwatchNode *nodes = mpp_realloc(impl->nodes, MppStopwatchNode,
                                              max_count);
        if (nodes) {
            impl->nodes = nodes;
            impl->max_count = max_count;
        }
    }

    if (impl->filled_count < impl->max_count) {
        MppStopwatchNode *node = impl->nodes + impl->filled_count;

        node->time = mpp_time();
        if (event) {
            RK_S32 len = snprintf(node->event, sizeof(node->event) - 1,
                                  "%s", event);
            if (len > impl->log_len)
                impl->log_len = len;
        }
        impl->filled_count++;
    }
}

void mpp_stopwatch_put(MppStopwatch stopwatch)
{
    if (NULL == stopwatch || check_is_mpp_stopwatch(stopwatch)) {
        mpp_err_f("invalid stopwatch %p\n", stopwatch);
        return ;
    }

    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;
    if (impl->show_on_exit && impl->nodes && impl->filled_count) {
        MppStopwatchNode *node = impl->nodes;
        RK_S64 last_time = node->time;
        RK_S32 i;
        char fmt[32];

        snprintf(fmt, sizeof(fmt) - 1, "%%s %%-%ds: %%6.2f\n", impl->log_len);
        node++;

        for (i = 1; i < impl->filled_count; i++) {
            mpp_log(fmt, impl->name, node->event,
                    (float)(node->time - last_time) / 1000);
            last_time = node->time;
            node++;
        }
    }
    MPP_FREE(impl->nodes);
    MPP_FREE(impl);
}

RK_S64 mpp_stopwatch_elapsed_time(MppStopwatch stopwatch)
{
    if (NULL == stopwatch || check_is_mpp_stopwatch(stopwatch)) {
        mpp_err_f("invalid stopwatch %p\n", stopwatch);
        return 0;
    }

    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;
    if (impl->filled_count < 2)
        return 0;

    RK_S64 base_time = impl->nodes[0].time;
    RK_S64 curr_time = impl->nodes[impl->filled_count - 1].time;
    RK_S64 elapsed_time = curr_time - base_time;
    return elapsed_time;
}
