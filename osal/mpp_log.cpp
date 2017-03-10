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

RK_U32 mpp_debug = 0;
static RK_U32 mpp_log_flag = 0;

// TODO: add log timing information and switch flag
static const char *msg_log_warning = "log message is long\n";
static const char *msg_log_nothing = "\n";

static void __mpp_log(mpp_log_callback func, const char *tag, const char *fmt,
                      const char *fname, va_list args)
{
    char msg[MPP_LOG_MAX_LEN + 1];
    char *tmp = msg;
    const char *buf = fmt;
    size_t len_fmt  = strnlen(fmt, MPP_LOG_MAX_LEN);
    size_t len_name = (fname) ? (strnlen(fname, MPP_LOG_MAX_LEN)) : (0);
    size_t buf_left = MPP_LOG_MAX_LEN;
    size_t len_all  = len_fmt + len_name;

    if (NULL == tag)
        tag = MODULE_TAG;

    if (len_name) {
        buf = msg;
        buf_left -= snprintf(msg, buf_left, "%s ", fname);
        tmp += len_name + 1;
    }

    if (len_all == 0) {
        buf = msg_log_nothing;
    } else if (len_all >= MPP_LOG_MAX_LEN) {
        buf_left -= snprintf(tmp, buf_left, "%s", msg_log_warning);
        buf = msg;
    } else {
        snprintf(tmp, buf_left, "%s", fmt);
        if (fmt[len_fmt - 1] != '\n') {
            tmp[len_fmt]    = '\n';
            tmp[len_fmt + 1]  = '\0';
        }
        buf = msg;
    }

    func(tag, buf, args);
}

void _mpp_log(const char *tag, const char *fmt, const char *fname, ...)
{
    va_list args;
    va_start(args, fname);
    __mpp_log(os_log, tag, fmt, fname, args);
    va_end(args);
}

void _mpp_err(const char *tag, const char *fmt, const char *fname, ...)
{
    va_list args;
    va_start(args, fname);
    __mpp_log(os_err, tag, fmt, fname, args);
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
