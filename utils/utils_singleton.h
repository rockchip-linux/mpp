/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef UTILS_SINGLETON_H
#define UTILS_SINGLETON_H

#include "mpp_sgln_base.h"

#define UTILS_SGLN_MAX_CNT 64

typedef enum UtilsSingletonId_e {
    UTILS_SGLN_BASE           = 0,
    /* utils module */
    UTILS_SGLN_ENC_ARGS       = UTILS_SGLN_BASE,
} UtilsSingletonId;

#define UTILS_SINGLETON(id, name, init, deinit) \
    MPP_SGLN_DEF(utils_singleton_add, id, name, init, deinit)

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 utils_singleton_add(MppSglnInfo *info, const char *caller);

#ifdef __cplusplus
}
#endif

#endif /* UTILS_SINGLETON_H */
