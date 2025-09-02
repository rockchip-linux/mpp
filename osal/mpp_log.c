/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_log"

#include <stdio.h>
#include <string.h>

#include "mpp_env.h"
#include "mpp_debug.h"
#include "mpp_common.h"

#include "os_log.h"

#define MPP_LOG_MAX_LEN     256

#ifdef __cplusplus
extern "C" {
#endif

RK_U32 mpp_debug = 0;

// TODO: add log timing information and switch flag
static const char *msg_log_warning = "log message is long\n";
static const char *msg_log_nothing = "\n";
static int mpp_log_level = MPP_LOG_INFO;
static MppLogCb mpp_log_ext_cb = NULL;
static void *mpp_log_ext_ctx = NULL;

static os_log_callback log_func[] = {
    NULL,           /* MPP_LOG_DEFAULT */
    os_log_fatal,   /* MPP_LOG_FATAL   */
    os_log_error,   /* MPP_LOG_ERROR   */
    os_log_warn,    /* MPP_LOG_WARN   */
    os_log_info,    /* MPP_LOG_INFO    */
    os_log_debug,   /* MPP_LOG_DEBUG   */
    os_log_trace,   /* MPP_LOG_VERBOSE */
    os_log_info,    /* MPP_LOG_DEFAULT */
};

static void __mpp_log(os_log_callback func, const char *tag, const char *fmt,
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

    mpp_logw("warning: use new logx function\n");

    va_start(args, fname);
    __mpp_log(os_log_info, tag, fmt, fname, args);
    va_end(args);
}

void _mpp_err(const char *tag, const char *fmt, const char *fname, ...)
{
    va_list args;

    mpp_logw("warning: use new logx function\n");

    va_start(args, fname);
    __mpp_log(os_log_error, tag, fmt, fname, args);
    va_end(args);
}

void _mpp_log_l(int level, const char *tag, const char *fmt, const char *fname, ...)
{
    va_list args;
    int log_level;

    if (mpp_log_ext_cb) {
        va_start(args, fname);
        mpp_log_ext_cb(mpp_log_ext_ctx, level, tag, fmt, fname, args);
        va_end(args);
        return;
    }

    if (level <= MPP_LOG_UNKNOWN || level >= MPP_LOG_SILENT)
        return;

    log_level = mpp_log_level;
    if (log_level >= MPP_LOG_SILENT)
        return;

    if (level > log_level)
        return;

    va_start(args, fname);
    __mpp_log(log_func[level], tag, fmt, fname, args);
    va_end(args);
}

void mpp_llog(int level, const char *tag, const char *fmt, const char *fname, ...)
{
    va_list args;
    char *log_buf = NULL;
    int log_size = SZ_1K;
    int log_base = 0;
    int log_len = 0;
    int log_level;
    int i;

    if (mpp_log_ext_cb) {
        va_start(args, fname);
        mpp_log_ext_cb(mpp_log_ext_ctx, level, tag, fmt, fname, args);
        va_end(args);
        return;
    }

    if (level <= MPP_LOG_UNKNOWN || level >= MPP_LOG_SILENT)
        return;

    log_level = mpp_log_level;
    if (log_level >= MPP_LOG_SILENT)
        return;

    if (level > log_level)
        return;

    if (NULL == tag)
        tag = MODULE_TAG;


    va_start(args, fname);

    /* get log len and create buffer */
    do {
        log_buf = (char *)malloc(log_size);
        if (!log_buf)
            break;

        log_len = vsnprintf(log_buf, log_size - 1, fmt, args);
        if (log_len < log_size - 1)
            break;

        free(log_buf);
        log_size <<= 1;
    } while (1);

    va_end(args);

    for (i = 0; i < log_len; i++) {
        if (log_buf[i] == '\n') {
            /* skip empty line */
            if (log_base < i) {
                log_buf[i] = '\0';
                log_func[level](tag, log_buf + log_base, args);
                /* restore \n */
                log_buf[i] = '\n';
            }

            log_base = i + 1;
        }
    }

    if (log_buf)
        free(log_buf);
}

void mpp_set_log_level(int level)
{
    if (level <= MPP_LOG_UNKNOWN || level > MPP_LOG_SILENT) {
        mpp_logw("log level should in range [%d : %d] invalid intput %d\n",
                 MPP_LOG_FATAL, MPP_LOG_SILENT, level);
        level = MPP_LOG_INFO;
    }

    mpp_log_level = level;
}

int mpp_get_log_level(void)
{
    int level;

    mpp_env_get_u32("mpp_log_level", (RK_U32 *)&level, mpp_log_level);

    if (level <= MPP_LOG_UNKNOWN || level > MPP_LOG_SILENT)
        level = MPP_LOG_INFO;

    mpp_log_level = level;

    return level;
}

int mpp_set_log_callback(void *ctx, MppLogCb cb)
{
    mpp_log_ext_cb = cb;
    mpp_log_ext_ctx = ctx;

    return 0;
}

#ifdef __cplusplus
}
#endif
