/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_singleton"

#include <stdio.h>
#include <stdlib.h>

#include "mpp_env.h"
#include "mpp_singleton.h"

#define sgln_dbg(fmt, ...) \
    do { \
        if (sgln_debug) \
            printf(MODULE_TAG ": " fmt, ##__VA_ARGS__); \
    } while (0)

static MppSingletonInfo sgln_info[MPP_SGLN_MAX_CNT] = {0};
static rk_u64 sgln_mask = 0;
static rk_u32 sgln_debug = 0;

rk_s32 mpp_singleton_add(MppSingletonInfo *info, const char *caller)
{
    mpp_env_get_u32("mpp_sgln_debug", &sgln_debug, 0);

    if (!info) {
        sgln_dbg("can not add NULL info at %s\n", caller);
        return rk_nok;
    }

    if (info->id >= MPP_SGLN_MAX_CNT) {
        sgln_dbg("id %d larger than max %d at %s\n", info->id, MPP_SGLN_MAX_CNT, caller);
        return rk_nok;
    }

    if (sgln_mask & (1 << info->id)) {
        sgln_dbg("info %d has been registered at %s\n", info->id, caller);
        return rk_nok;
    }

    sgln_info[info->id] = *info;
    sgln_mask |= (1 << info->id);

    sgln_dbg("info %2d %-12s registered at %s\n", info->id, info->name, caller);

    return rk_ok;
}

static void mpp_singleton_deinit(void)
{
    rk_s32 i;

    sgln_dbg("deinit enter\n");

    /* NOTE: revert deinit order */
    for (i = MPP_SGLN_MAX_CNT - 1; i >= 0; i--) {
        if (sgln_mask & (1 << i)) {
            MppSingletonInfo *info = &sgln_info[i];

            if (info->deinit) {
                sgln_dbg("info %2d %-12s deinit start\n", info->id, info->name);
                info->deinit();
                sgln_dbg("info %2d %-12s deinit finish\n", info->id, info->name);
            }
        }
    }

    sgln_dbg("deinit leave\n");
}

__attribute__((constructor(65535))) static void mpp_singleton_init(void)
{
    rk_s32 i;

    sgln_dbg("init enter\n");

    /* NOTE: call atexit first to avoid init crash but not deinit */
    atexit(mpp_singleton_deinit);

    for (i = 0; i < MPP_SGLN_MAX_CNT; i++) {
        if (sgln_mask & (1 << i)) {
            MppSingletonInfo *info = &sgln_info[i];

            if (info->init) {
                sgln_dbg("info %2d %-12s init start\n", info->id, info->name);
                info->init();
                sgln_dbg("info %2d %-12s init finish\n", info->id, info->name);
            }
        }
    }

    sgln_dbg("init leave\n");
}
