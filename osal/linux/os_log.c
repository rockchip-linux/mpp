/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#if defined(linux) && !defined(__ANDROID__)

#define MODULE_TAG "os_log"

#include <stdio.h>
#include <stdarg.h>
#include <syslog.h>

#include "os_log.h"
#include "os_env.h"
#include "mpp_singleton.h"

#define LINE_SZ 1024

void syslog_wrapper_init()
{
    int option = LOG_PID;
    RK_U32 syslog_perror = 1;
    RK_U32 syslog_cons = 0;

    os_get_env_u32("mpp_syslog_perror", &syslog_perror, 1);
    if (syslog_perror)
        option |= LOG_PERROR;

    os_get_env_u32("mpp_syslog_cons", &syslog_cons, 0);
    if (syslog_cons)
        option |= LOG_CONS;

    openlog("mpp", option, LOG_USER);
}

void syslog_wrapper_deinit()
{
    closelog();
}

void os_log_trace(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_NOTICE, line, list);
}

void os_log_debug(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_DEBUG, line, list);
}

void os_log_info(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_INFO, line, list);
}

void os_log_warn(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_WARNING, line, list);
}

void os_log_error(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_ERR, line, list);
}

void os_log_fatal(const char* tag, const char* msg, va_list list)
{
    char line[LINE_SZ] = {0};
    snprintf(line, sizeof(line) - 1, "%s: %s", tag, msg);
    vsyslog(LOG_CRIT, line, list);
}

MPP_SINGLETON(MPP_SGLN_OS_LOG, os_log, syslog_wrapper_init, syslog_wrapper_deinit)

#endif
