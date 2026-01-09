/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef HWPQ_DEBUG_H
#define HWPQ_DEBUG_H

#include "mpp_log.h"

#define HWPQ_VDPP_FATAL           (0x00000001)
#define HWPQ_VDPP_ERROR           (0x00000002)
#define HWPQ_VDPP_WARNING         (0x00000004)
#define HWPQ_VDPP_INFO            (0x00000008)
#define HWPQ_VDPP_DEBUG           (0x00000010)
#define HWPQ_VDPP_TRACE           (0x00000020)

#define HWPQ_VDPP_DUMP_IN         (0x00000100)
#define HWPQ_VDPP_DUMP_OUT        (0x00000200)

/* log macros */
#define hwpq_log(log_func, log_lv, fmt, ...) \
    do {                                     \
        if (hwpq_vdpp_debug & log_lv)        \
            log_func(fmt, ##__VA_ARGS__);    \
    } while (0)

#define hwpq_logf(fmt, ...) hwpq_log(mpp_logf_f, HWPQ_VDPP_FATAL, fmt, ##__VA_ARGS__)
#define hwpq_loge(fmt, ...) hwpq_log(mpp_loge_f, HWPQ_VDPP_ERROR, fmt, ##__VA_ARGS__)
#define hwpq_logw(fmt, ...) hwpq_log(mpp_logw_f, HWPQ_VDPP_WARNING, fmt, ##__VA_ARGS__)
#define hwpq_logi(fmt, ...) hwpq_log(mpp_logi_f, HWPQ_VDPP_INFO, fmt, ##__VA_ARGS__)
#define hwpq_logd(fmt, ...) hwpq_log(mpp_logi_f, HWPQ_VDPP_DEBUG, fmt, ##__VA_ARGS__)
#define hwpq_logt(fmt, ...) hwpq_log(mpp_logi_f, HWPQ_VDPP_TRACE, fmt, ##__VA_ARGS__)
#define hwpq_logv(fmt, ...) hwpq_log(mpp_logi_f, HWPQ_VDPP_TRACE, fmt, ##__VA_ARGS__)

#define hwpq_enter()        hwpq_logv("--- enter ---");
#define hwpq_leave()        hwpq_logv("--- leave ---");

extern RK_U32 hwpq_vdpp_debug;

#endif // HWPQ_DEBUG_H