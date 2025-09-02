/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

/*
 * all os log function will provide two interface
 * os_log and os_err
 * os_log for general message
 * os_err for error message
 */

#ifndef __OS_LOG_H__
#define __OS_LOG_H__

typedef void (*os_log_callback)(const char*, const char*, va_list);

#ifdef __cplusplus
extern "C" {
#endif

void os_log_trace(const char* tag, const char* msg, va_list list);
void os_log_debug(const char* tag, const char* msg, va_list list);
void os_log_info (const char* tag, const char* msg, va_list list);
void os_log_warn (const char* tag, const char* msg, va_list list);
void os_log_error(const char* tag, const char* msg, va_list list);
void os_log_fatal(const char* tag, const char* msg, va_list list);

#ifdef __cplusplus
}
#endif

#endif /*__OS_LOG_H__*/

