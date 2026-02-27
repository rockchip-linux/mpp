/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include <stdlib.h>

#include "mpp_singleton.h"

static MppSglnInfo mpp_info_with_id[MPP_SGLN_MAX_CNT] = {0};
static MppSglnInfo mpp_info_without_id[MPP_SGLN_NO_ID_MAX_CNT] = {0};

static MppSglnList mpp_arr_with_id = {
    .count = 0,
    .capacity = MPP_SGLN_MAX_CNT,
    .entries = mpp_info_with_id,
};

static MppSglnList mpp_arr_without_id = {
    .count = 0,
    .capacity = MPP_SGLN_NO_ID_MAX_CNT,
    .entries = mpp_info_without_id,
};

static MppSglnBase mpp_sgln_base = {
    .name = "mpp_sgln_debug",
    .arrays = {&mpp_arr_with_id, &mpp_arr_without_id},
    .array_count = 2,
    .max_name_len = 12,
    .debug = 0,
};

rk_s32 mpp_singleton_add(MppSglnInfo *info, const char *caller)
{
    return mpp_sgln_base_add(&mpp_sgln_base, info, caller);
}

static void mpp_singleton_deinit(void)
{
    mpp_sgln_base_deinit(&mpp_sgln_base);
}

__attribute__((constructor(65535)))
static void mpp_singleton_init(void)
{
    atexit(mpp_singleton_deinit);
    mpp_sgln_base_run_init(&mpp_sgln_base);
}
