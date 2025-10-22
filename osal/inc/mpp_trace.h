/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_TRACE_H
#define MPP_TRACE_H

#include "rk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

void mpp_trace_begin(const char* name);
void mpp_trace_end(const char* name);
void mpp_trace_async_begin(const char* name, rk_s32 cookie);
void mpp_trace_async_end(const char* name, rk_s32 cookie);
void mpp_trace_int32(const char* name, rk_s32 value);
void mpp_trace_int64(const char* name, rk_s64 value);

#ifdef __cplusplus
}
#endif

#endif /* MPP_TRACE_H */