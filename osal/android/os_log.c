/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#if defined(__ANDROID__)
#include <android/log.h>

void os_log_trace(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_VERBOSE, tag, msg, list);
}

void os_log_debug(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_DEBUG, tag, msg, list);
}

void os_log_info(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_INFO, tag, msg, list);
}

void os_log_warn(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_WARN, tag, msg, list);
}

void os_log_error(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_ERROR, tag, msg, list);
}

void os_log_fatal(const char* tag, const char* msg, va_list list)
{
    __android_log_vprint(ANDROID_LOG_FATAL, tag, msg, list);
}

#endif
