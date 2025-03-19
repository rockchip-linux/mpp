/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_trace"

#include <fcntl.h>
#include <stdarg.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_singleton.h"

#include "mpp_trace.h"

#define ATRACE_MESSAGE_LENGTH 256

#define get_srv_trace() \
    ({ \
        MppTraceSrv *__tmp; \
        if (!srv_trace) { \
            mpp_trace_srv_init(); \
        } \
        if (srv_trace) { \
            __tmp = srv_trace; \
        } else { \
            mpp_err("mpp trace srv not init at %s : %s\n", __FUNCTION__); \
            __tmp = NULL; \
        } \
        __tmp; \
    })

typedef struct  MppTraceSrv_t {
    const char  *name;
    rk_s32      fd;
} MppTraceSrv;

static MppTraceSrv *srv_trace = NULL;

static void mpp_trace_srv_init()
{
    static const char *ftrace_paths[] = {
        "/sys/kernel/debug/tracing/trace_marker",
        "/debug/tracing/trace_marker",
        "/debugfs/tracing/trace_marker",
    };
    MppTraceSrv *srv = srv_trace;
    rk_u32 i;

    if (srv)
        return;

    srv = mpp_calloc(MppTraceSrv, 1);
    if (!srv) {
        mpp_err_f("failed to allocate trace service\n");
        return;
    }

    srv_trace = srv;
    srv->fd = -1;

    for (i = 0; i < MPP_ARRAY_ELEMS(ftrace_paths); i++) {
        if (!access(ftrace_paths[i], F_OK)) {
            rk_s32 fd = open(ftrace_paths[i], O_WRONLY | O_CLOEXEC);

            if (fd >= 0) {
                srv->fd = fd;
                srv->name = ftrace_paths[i];
                break;
            }
        }
    }
}

static void mpp_trace_srv_deinit()
{
    MppTraceSrv *srv = srv_trace;

    if (srv) {
        if (srv->fd >= 0) {
            close(srv->fd);
            srv->fd = -1;
        }
        mpp_free(srv);
    }

    srv_trace = NULL;
}

static void mpp_trace_write(rk_s32 fd, const char *fmt, ...)
{
    char buf[ATRACE_MESSAGE_LENGTH];
    va_list ap;
    rk_s32 len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    (void)!write(fd, buf, len);
}

void mpp_trace_begin(const char* name)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "B|%d|%s", getpid(), name);
}

void mpp_trace_end(const char* name)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "E|%d|%s", getpid(), name);
}

void mpp_trace_async_begin(const char* name, rk_s32 cookie)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "S|%d|%s|%d", getpid(), name, cookie);
}

void mpp_trace_async_end(const char* name, rk_s32 cookie)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "F|%d|%s|%d", getpid(), name, cookie);
}

void mpp_trace_int32(const char* name, rk_s32 value)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "C|%d|%s|%d", getpid(), name, value);
}

void mpp_trace_int64(const char* name, RK_S64 value)
{
    MppTraceSrv *srv = get_srv_trace();

    if (srv && srv->fd >= 0)
        mpp_trace_write(srv->fd, "C|%d|%s|%lld", getpid(), name, value);
}

MPP_SINGLETON(MPP_SGLN_TRACE, mpp_trace, mpp_trace_srv_init, mpp_trace_srv_deinit)
