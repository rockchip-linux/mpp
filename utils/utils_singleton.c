/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#include <stdlib.h>

#include "utils_singleton.h"

static MppSglnInfo utils_info_with_id[MPP_SGLN_MAX_CNT] = {0};

static MppSglnList utils_sgln_arr = {
    .count = 0,
    .capacity = MPP_SGLN_MAX_CNT,
    .entries = utils_info_with_id,
};

static MppSglnBase utils_sgln_base = {
    .name = "utils_sgln_debug",
    .arrays = {&utils_sgln_arr, NULL},
    .array_count = 1,
    .max_name_len = 12,
    .debug = 0,
};

rk_s32 utils_singleton_add(MppSglnInfo *info, const char *caller)
{
    return mpp_sgln_base_add(&utils_sgln_base, info, caller);
}

static void utils_singleton_deinit(void)
{
    mpp_sgln_base_deinit(&utils_sgln_base);
}

__attribute__((constructor(65534)))
static void utils_singleton_init(void)
{
    atexit(utils_singleton_deinit);
    mpp_sgln_base_run_init(&utils_sgln_base);
}
