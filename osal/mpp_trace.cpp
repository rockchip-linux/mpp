/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_trace"

#include <fcntl.h>
#include <stdarg.h>

#include "mpp_common.h"
#include "mpp_trace.h"

#define ATRACE_MESSAGE_LENGTH 256

class MppTraceService
{
private:
    // avoid any unwanted function
    MppTraceService();
    ~MppTraceService();
    MppTraceService(const MppTraceService &);
    MppTraceService &operator=(const MppTraceService &);

    void trace_write(const char *fmt, ...);

    RK_S32 mTraceFd;

public:
    static MppTraceService *get_inst() {
        static MppTraceService inst;
        return &inst;
    }

    void trace_begin(const char* name);
    void trace_end(const char* name);
    void trace_async_begin(const char* name, RK_S32 cookie);
    void trace_async_end(const char* name, RK_S32 cookie);
    void trace_int32(const char* name, RK_S32 val);
    void trace_int64(const char* name, RK_S64 val);
};

MppTraceService::MppTraceService()
{
    static const char *ftrace_paths[] = {
        "/sys/kernel/debug/tracing/trace_marker",
        "/debug/tracing/trace_marker",
        "/debugfs/tracing/trace_marker",
    };

    RK_U32 i;

    for (i = 0; i < MPP_ARRAY_ELEMS(ftrace_paths); i++) {
        if (!access(ftrace_paths[i], F_OK)) {
            mTraceFd = open(ftrace_paths[i], O_WRONLY | O_CLOEXEC);
            if (mTraceFd >= 0)
                break;
        }
    }
}

MppTraceService::~MppTraceService()
{
    if (mTraceFd >= 0) {
        close(mTraceFd);
        mTraceFd = -1;
    }
}

void MppTraceService::trace_write(const char *fmt, ...)
{
    char buf[ATRACE_MESSAGE_LENGTH];
    va_list ap;
    RK_S32 len;

    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);

    write(mTraceFd, buf, len);
}

void MppTraceService::trace_begin(const char* name)
{
    if (mTraceFd < 0)
        return;

    trace_write("B|%d|%s", getpid(), name);
}

void MppTraceService::trace_end(const char* name)
{
    if (mTraceFd < 0)
        return;

    trace_write("E|%d|%s", getpid(), name);
}

void MppTraceService::trace_async_begin(const char* name, RK_S32 cookie)
{
    if (mTraceFd < 0)
        return;

    trace_write("S|%d|%s|%d", getpid(), name, cookie);
}

void MppTraceService::trace_async_end(const char* name, RK_S32 cookie)
{
    if (mTraceFd < 0)
        return;

    trace_write("F|%d|%s|%d", getpid(), name, cookie);
}

void MppTraceService::trace_int32(const char* name, RK_S32 value)
{
    if (mTraceFd < 0)
        return;

    trace_write("C|%d|%s|%d", getpid(), name, value);
}

void MppTraceService::trace_int64(const char* name, RK_S64 value)
{
    if (mTraceFd < 0)
        return;

    trace_write("C|%d|%s|%lld", getpid(), name, value);
}

void mpp_trace_begin(const char* name)
{
    MppTraceService::get_inst()->trace_begin(name);
}

void mpp_trace_end(const char* name)
{
    MppTraceService::get_inst()->trace_end(name);
}

void mpp_trace_async_begin(const char* name, RK_S32 cookie)
{
    MppTraceService::get_inst()->trace_async_begin(name, cookie);
}

void mpp_trace_async_end(const char* name, RK_S32 cookie)
{
    MppTraceService::get_inst()->trace_async_end(name, cookie);
}

void mpp_trace_int32(const char* name, RK_S32 value)
{
    MppTraceService::get_inst()->trace_int32(name, value);
}

void mpp_trace_int64(const char* name, RK_S64 value)
{
    MppTraceService::get_inst()->trace_int64(name, value);
}
