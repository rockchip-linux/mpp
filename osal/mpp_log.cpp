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

#include "os_log.h"

#define MPP_LOG_MAX_LEN     256

#ifdef __cplusplus
extern "C" {
#endif

static RK_U32 mpp_log_flag = 0;
static void (*mpp_log_callback)(const char*, const char*, va_list) = os_log;
static void (*mpp_err_callback)(const char*, const char*, va_list) = os_err;

// TODO: add log timing information and switch flag
static const char *msg_log_warning = "log message is long\n";
static const char *msg_log_nothing = "\n";

void _mpp_log(const char *tag, const char *fmt, ...)
{
    va_list args;
    const char *buf = fmt;
    size_t len = strnlen(fmt, MPP_LOG_MAX_LEN);

    if (NULL == tag)
        tag = MODULE_TAG;

    if (len == 0) {
        buf = msg_log_nothing;
    } else if (len == MPP_LOG_MAX_LEN) {
        buf = msg_log_warning;
    } else if (fmt[len - 1] != '\n') {
        char msg[MPP_LOG_MAX_LEN+1];
        snprintf(msg, sizeof(msg), "%s", fmt);
        msg[len]    = '\n';
        msg[len + 1]  = '\0';
        buf = msg;
    }

    va_start(args, fmt);
    mpp_log_callback(tag, buf, args);
    va_end(args);
}

void _mpp_err(const char *tag, const char *fmt, ...)
{
    va_list args;
    const char *buf = fmt;
    size_t len = strnlen(fmt, MPP_LOG_MAX_LEN);

    if (NULL == tag)
        tag = MODULE_TAG;

    if (len == 0) {
        buf = msg_log_nothing;
    } else if (len == MPP_LOG_MAX_LEN) {
        buf = msg_log_warning;
    } else if (fmt[len - 1] != '\n') {
        char msg[MPP_LOG_MAX_LEN+1];
        snprintf(msg, sizeof(msg), "%s", fmt);
        msg[len]    = '\n';
        msg[len + 1]  = '\0';
        buf = msg;
    }

    va_start(args, fmt);
    mpp_err_callback(tag, buf, args);
    va_end(args);
}

void mpp_set_log_flag(RK_U32 flag)
{
    mpp_log_flag = flag;
    return ;
}

RK_U32 mpp_get_log_flag()
{
    return mpp_log_flag;
}

#ifdef __cplusplus
}
#endif
