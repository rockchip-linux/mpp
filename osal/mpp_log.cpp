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

#define MODULE_TAG "mpp_log"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "os_log.h"

#define MPP_LOG_MAX_LEN     256

typedef void (*mpp_log_callback)(const char*, const char*, va_list);


#ifdef __cplusplus
extern "C" {
#endif

static RK_U32 mpp_log_flag = 0;

// TODO: add log timing information and switch flag
static const char *msg_log_warning = "log message is long\n";
static const char *msg_log_nothing = "\n";

void __mpp_log(mpp_log_callback func, const char *tag, const char *fmt, va_list args)
{
    const char *buf = fmt;
    size_t len = strnlen(fmt, MPP_LOG_MAX_LEN);

    if (NULL == tag)
        tag = MODULE_TAG;

    if (len == 0) {
        buf = msg_log_nothing;
    } else if (len == MPP_LOG_MAX_LEN) {
        buf = msg_log_warning;
    } else if (fmt[len - 1] != '\n') {
        char msg[MPP_LOG_MAX_LEN + 1];
        snprintf(msg, sizeof(msg), "%s", fmt);
        msg[len]    = '\n';
        msg[len + 1]  = '\0';
        buf = msg;
    }

    func(tag, buf, args);
}

void _mpp_log(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __mpp_log(os_log, tag, fmt, args);
    va_end(args);
}

void _mpp_dbg(RK_U32 debug, RK_U32 flag, const char *tag, const char *fmt, ...)
{
    if (debug & flag) {
        va_list args;
        va_start(args, fmt);
        __mpp_log(os_log, tag, fmt, args);
        va_end(args);
    }
}

void _mpp_err(const char *tag, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    __mpp_log(os_err, tag, fmt, args);
    va_end(args);
}

void mpp_log_set_flag(RK_U32 flag)
{
    mpp_log_flag = flag;
    return ;
}

RK_U32 mpp_log_get_flag()
{
    return mpp_log_flag;
}

#ifdef __cplusplus
}
#endif
