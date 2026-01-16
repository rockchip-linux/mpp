/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp_singleton"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_common.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_singleton.h"

#define sgln_dbg(fmt, ...) \
    do { \
        if (sgln_debug) \
            printf(MODULE_TAG ": " fmt, ##__VA_ARGS__); \
    } while (0)

#define sgln_err(fmt, ...) \
        printf(MODULE_TAG ": " fmt, ##__VA_ARGS__);

typedef struct MppSingletonCtxImpl_t {
    rk_u32 max_id;
    rk_u32 max_info;
    MppSingletonInfo *info;
} MppSingletonCtx;

static MppSingletonInfo info_with_id[MPP_SGLN_MAX_CNT] = {0};
static MppSingletonCtx module_with_id = {
    .max_id = 0,
    .max_info = MPP_SGLN_MAX_CNT,
    .info = info_with_id,
};

static MppSingletonInfo info_without_id[MPP_SGLN_NO_ID_MAX_CNT] = {0};
static MppSingletonCtx module_without_id = {
    .max_id = 0,
    .max_info = MPP_SGLN_NO_ID_MAX_CNT,
    .info = info_without_id,
};

static MppSingletonCtx *info_list[] = {
    &module_with_id,
    &module_without_id,
};

static rk_u32 sgln_debug = 0;
static rk_u32 max_name_len = 12;

rk_s32 mpp_singleton_add(MppSingletonInfo *info, const char *caller)
{
    MppSingletonCtx *impl;
    rk_s32 max_info;
    rk_s32 id;

    mpp_env_get_u32("mpp_sgln_debug", &sgln_debug, 0);

    if (!info) {
        sgln_err("can not add NULL info at %s\n", caller);
        return rk_nok;
    }

    id = info->id;
    impl = id >= 0 ? &module_with_id : &module_without_id;
    max_info = impl->max_info;

    if (id >= 0) {
        if (id >= max_info) {
            sgln_err("id %d larger than max %d at %s\n", id, max_info, caller);
            return rk_nok;
        }

        impl->max_id = MPP_MAX(impl->max_id, info->id);
        if (impl->info[info->id].name) {
            sgln_err("info %d has been registered at %s\n", info->id, caller);
            return rk_nok;
        }
    } else {
        if (impl->max_id >= max_info) {
            sgln_err("id %d larger than max %d at %s\n", id, max_info, caller);
            abort();
            return rk_nok;
        }
        id = impl->max_id++;
        info->id = id;
    }

    impl->info[info->id] = *info;

    {
        rk_u32 name_len = strlen(info->name);

        if (name_len > max_name_len)
            max_name_len = name_len;
    }

    sgln_dbg("info %2d %-*s registered at %s\n", info->id,
             max_name_len, info->name, caller);

    return rk_ok;
}

static void mpp_singleton_deinit(void)
{
    MppSingletonCtx *impl;
    rk_s32 j;

    sgln_dbg("deinit enter\n");

    for (j = MPP_ARRAY_ELEMS(info_list) - 1; j >= 0; j--) {
        MppSingletonCtx *impl = info_list[j];
        rk_s32 i;

        /* NOTE: revert deinit order */
        for (i = impl->max_id; i >= 0; i--) {
            MppSingletonInfo *info = &impl->info[i];

            if (info->deinit) {
                sgln_dbg("info %2d %-*s deinit start\n", info->id,
                         max_name_len, info->name);

                info->deinit();

                sgln_dbg("info %2d %-*s deinit finish\n", info->id,
                         max_name_len, info->name);
            }
        }
    }

    sgln_dbg("deinit leave\n");
}

__attribute__((constructor(65535))) static void mpp_singleton_init(void)
{
    rk_s64 sum = 0;
    rk_s32 j;

    sgln_dbg("init enter\n");

    /* NOTE: call atexit first to avoid init crash but not deinit */
    atexit(mpp_singleton_deinit);

    for (j = 0; j < MPP_ARRAY_ELEMS(info_list); j++) {
        MppSingletonCtx *impl = info_list[j];
        rk_s32 i;

        for (i = 0; i <= impl->max_id; i++) {
            MppSingletonInfo *info = &impl->info[i];

            if (info->init) {
                rk_s64 time;

                sgln_dbg("info %2d %-*s init start\n", info->id,
                         max_name_len, info->name);

                time = mpp_time();
                info->init();
                time = mpp_time() - time;
                sum += time;

                sgln_dbg("info %2d %-*s init finish %lld us\n", info->id,
                         max_name_len, info->name, time);
            }
        }
    }

    sgln_dbg("init leave total %lld us\n", sum);
}
