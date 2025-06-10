/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_time"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <sys/timerfd.h>
#include <sys/epoll.h>

#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"

rk_s64 mpp_time()
{
    struct timespec time = {0, 0};

    clock_gettime(CLOCK_MONOTONIC, &time);
    return (rk_s64)time.tv_sec * 1000000 + (rk_s64)time.tv_nsec / 1000;
}

void mpp_time_diff(rk_s64 start, rk_s64 end, rk_s64 limit, const char *fmt)
{
    rk_s64 diff = end - start;

    if (diff >= limit)
        mpp_dbg(MPP_DBG_TIMING, "%s timing %lld us\n", fmt, diff);
}

typedef struct MppClockImpl_t {
    const char *check;
    char    name[16];
    rk_u32  enable;
    rk_s64  base;
    rk_s64  time;
    rk_s64  sum;
    rk_s64  count;
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
        snprintf(impl->name, sizeof(impl->name) - 1, name, NULL);
    } else
        mpp_err_f("malloc failed\n");

    return impl;
}

void mpp_clock_put(MppClock clock)
{
    if (check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
        return;
    }

    mpp_free(clock);
}

void mpp_clock_enable(MppClock clock, rk_u32 enable)
{
    if (check_is_mpp_clock(clock)) {
        mpp_err_f("invalid clock %p\n", clock);
    } else {
        MppClockImpl *p = (MppClockImpl *)clock;

        p->enable = (enable) ? (1) : (0);
    }
}

rk_s64 mpp_clock_start(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
        return 0;
    }

    if (!p->enable)
        return 0;

    p->base = mpp_time();
    p->time = 0;
    return p->base;
}

rk_s64 mpp_clock_pause(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;
    rk_s64 time;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
        return 0;
    }

    if (!p->enable)
        return 0;

    time = mpp_time();

    if (!p->time) {
        // first pause after start
        p->sum += time - p->base;
        p->count++;
    }

    p->time = time;

    return p->time - p->base;
}

rk_s64 mpp_clock_reset(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
    } else {
        p->base = 0;
        p->time = 0;
        p->sum = 0;
        p->count = 0;
    }

    return 0;
}

rk_s64 mpp_clock_get_sum(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
        return 0;
    }

    return (p->enable) ? (p->sum) : (0);
}

rk_s64 mpp_clock_get_count(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
        return 0;
    }

    return (p->enable) ? (p->count) : (0);
}

const char *mpp_clock_get_name(MppClock clock)
{
    MppClockImpl *p = (MppClockImpl *)clock;

    if (check_is_mpp_clock(p)) {
        mpp_err_f("invalid clock %p\n", p);
        return NULL;
    }

    return p->name;
}

typedef struct MppTimerImpl_t {
    const char          *check;
    char                name[16];

    rk_s32              enabled;
    rk_s32              initial;
    rk_s32              interval;
    rk_s32              timer_fd;
    rk_s32              epoll_fd;

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
    MppTimerImpl *impl = (MppTimerImpl *)ctx;
    MppThread *thd = impl->thd;
    rk_s32 timer_fd = impl->timer_fd;
    rk_s32 ret = 0;

    // first expire time
    ts.it_value.tv_sec = impl->initial / 1000;
    ts.it_value.tv_nsec = (impl->initial % 1000) * 1000;

    // last expire time
    ts.it_interval.tv_sec = impl->interval / 1000;
    ts.it_interval.tv_nsec = (impl->interval % 1000) * 1000 * 1000;

    ret = timerfd_settime(timer_fd, 0, &ts, NULL);
    if (ret < 0) {
        mpp_err("timerfd_settime error, Error:[%d:%s]", errno, strerror(errno));
        return NULL;
    }

    while (1) {
        struct epoll_event events;
        rk_s32 fd_cnt;

        if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd, THREAD_WORK))
            break;

        memset(&events, 0, sizeof(events));

        /* wait epoll event */
        fd_cnt = epoll_wait(impl->epoll_fd, &events, 1, 500);
        if (fd_cnt && (events.events & EPOLLIN) && (events.data.fd == timer_fd)) {
            rk_u64 exp = 0;
            ssize_t cnt = read(timer_fd, &exp, sizeof(exp));

            mpp_assert(cnt == sizeof(exp));
            impl->func(impl->ctx);
        }
    }

    return NULL;
}

MppTimer mpp_timer_get(const char *name)
{
    MppTimerImpl *impl = NULL;
    rk_s32 timer_fd = -1;
    rk_s32 epoll_fd = -1;

    do {
        struct epoll_event event;

        impl = mpp_calloc(MppTimerImpl, 1);
        if (!impl) {
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
        snprintf(impl->name, sizeof(impl->name) - 1, name, NULL);

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
    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (check_is_mpp_timer(impl)) {
        mpp_err_f("invalid timer %p\n", impl);
        return;
    }

    if (!func) {
        mpp_err_f("invalid NULL callback\n");
        return;
    }

    impl->func = func;
    impl->ctx = ctx;
}

void mpp_timer_set_timing(MppTimer timer, rk_s32 initial, rk_s32 interval)
{
    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (check_is_mpp_timer(impl)) {
        mpp_err_f("invalid timer %p\n", impl);
        return;
    }

    impl->initial = initial;
    impl->interval = interval;
}

void mpp_timer_set_enable(MppTimer timer, rk_s32 enable)
{
    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (check_is_mpp_timer(impl)) {
        mpp_err_f("invalid timer %p\n", impl);
        return;
    }

    if (!impl->func || impl->initial < 0 || impl->interval < 0) {
        mpp_err_f("invalid func %p initial %d interval %d\n",
                  impl->func, impl->initial, impl->interval);
        return;
    }

    if (enable) {
        if (!impl->enabled && !impl->thd) {
            MppThread *thd = mpp_thread_create(mpp_timer_thread, impl, impl->name);

            if (thd) {
                impl->thd = thd;
                impl->enabled = 1;
                mpp_thread_start(impl->thd);
            }
        }
    } else {
        if (impl->enabled && impl->thd) {
            mpp_thread_stop(impl->thd);
            impl->enabled = 0;
        }
    }
}

void mpp_timer_put(MppTimer timer)
{
    MppTimerImpl *impl = (MppTimerImpl *)timer;

    if (check_is_mpp_timer(impl)) {
        mpp_err_f("invalid timer %p\n", impl);
        return;
    }

    if (impl->enabled)
        mpp_timer_set_enable(impl, 0);

    if (impl->timer_fd >= 0) {
        close(impl->timer_fd);
        impl->timer_fd = -1;
    }

    if (impl->epoll_fd >= 0) {
        close(impl->epoll_fd);
        impl->epoll_fd = -1;
    }

    if (impl->thd) {
        mpp_thread_destroy(impl->thd);
        impl->thd = NULL;
    }

    if (impl) {
        mpp_free(impl);
        impl = NULL;
    }
}

#define STOPWATCH_TRACE_STR_LEN 64

typedef struct MppStopwatchNode_t {
    char                event[STOPWATCH_TRACE_STR_LEN];
    rk_s64              time;
} MppStopwatchNode;

typedef struct MppStopwatchImpl_t {
    const char          *check;
    char                name[STOPWATCH_TRACE_STR_LEN];

    rk_s32              max_count;
    rk_s32              filled_count;
    rk_s32              show_on_exit;
    rk_s32              log_len;
    rk_s64              time_elipsed;

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

void mpp_stopwatch_set_show_on_exit(MppStopwatch stopwatch, rk_s32 show_on_exit)
{
    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;

    if (check_is_mpp_stopwatch(impl)) {
        mpp_err_f("invalid stopwatch %p\n", impl);
        return;
    }

    impl->show_on_exit = show_on_exit;
}

void mpp_stopwatch_record(MppStopwatch stopwatch, const char *event)
{
    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;

    /* do not print noisy log */
    if (!impl)
        return;

    if (check_is_mpp_stopwatch(impl)) {
        mpp_err_f("invalid stopwatch %p on %s\n", impl, event);
        return;
    }

    if (impl->filled_count >= impl->max_count) {
        rk_s32 max_count = impl->max_count * 2;
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
            rk_s32 len = snprintf(node->event, sizeof(node->event) - 1, "%s", event);

            if (len > impl->log_len)
                impl->log_len = len;
        }
        impl->filled_count++;
    }
}

void mpp_stopwatch_put(MppStopwatch stopwatch)
{
    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;

    if (check_is_mpp_stopwatch(impl)) {
        mpp_err_f("invalid stopwatch %p\n", impl);
        return;
    }

    if (impl->show_on_exit && impl->nodes && impl->filled_count) {
        MppStopwatchNode *node = impl->nodes;
        rk_s64 last_time = node->time;
        rk_s32 i;
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

rk_s64 mpp_stopwatch_elapsed_time(MppStopwatch stopwatch)
{
    MppStopwatchImpl *impl = (MppStopwatchImpl *)stopwatch;

    if (check_is_mpp_stopwatch(impl)) {
        mpp_err_f("invalid stopwatch %p\n", impl);
        return 0;
    }

    if (impl->filled_count < 2)
        return 0;

    rk_s64 base_time = impl->nodes[0].time;
    rk_s64 curr_time = impl->nodes[impl->filled_count - 1].time;
    rk_s64 elapsed_time = curr_time - base_time;
    return elapsed_time;
}
