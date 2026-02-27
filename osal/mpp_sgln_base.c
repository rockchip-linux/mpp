/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#include <stdlib.h>

#include "mpp_common.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_sgln_base.h"

#define SGLN_LOG(base, fmt, ...) \
    do { if ((base)->debug) printf("%s: " fmt, (base)->name, ##__VA_ARGS__); } while (0)

#define SGLN_ERR(base, fmt, ...) \
    printf("%s: " fmt, (base)->name, ##__VA_ARGS__)

rk_s32 mpp_sgln_base_add(MppSglnBase *base, MppSglnInfo *info, const char *caller)
{
    MppSglnList *arr;
    rk_s32 id;

    mpp_env_get_u32(base->name, &base->debug, 0);

    if (!info) {
        SGLN_ERR(base, "can not add NULL entry at %s\n", caller);
        return rk_nok;
    }

    id = info->id;

    arr = (id < 0 && base->array_count > 1) ? base->arrays[1] : base->arrays[0];

    if (id >= 0) {
        if (id >= (rk_s32)arr->capacity) {
            SGLN_ERR(base, "id %d larger than max %d at %s\n", id, arr->capacity, caller);
            return rk_nok;
        }
        if (arr->entries[id].name) {
            SGLN_ERR(base, "entry %d has been registered at %s\n", id, caller);
            return rk_nok;
        }
        arr->count = MPP_MAX(arr->count, (rk_u32)id + 1);
    } else {
        if (arr->count >= arr->capacity) {
            SGLN_ERR(base, "no-id list full at %s\n", caller);
            abort();
            return rk_nok;
        }
        info->id = arr->count++;
    }

    arr->entries[info->id] = *info;

    {
        rk_u32 len = strlen(info->name);

        if (len > base->max_name_len)
            base->max_name_len = len;
    }

    SGLN_LOG(base, "entry %2d %-*s registered at %s\n",
             info->id, base->max_name_len, info->name, caller);

    return rk_ok;
}

void mpp_sgln_base_deinit(MppSglnBase *base)
{
    rk_s32 j;

    SGLN_LOG(base, "deinit enter\n");

    for (j = base->array_count - 1; j >= 0; j--) {
        MppSglnList *arr = base->arrays[j];
        rk_s32 i;

        for (i = (rk_s32)arr->count - 1; i >= 0; i--) {
            MppSglnInfo *info = &arr->entries[i];

            if (info->name && info->deinit) {
                SGLN_LOG(base, "entry %2d %-*s deinit start\n",
                         info->id, base->max_name_len, info->name);
                info->deinit();
                SGLN_LOG(base, "entry %2d %-*s deinit finish\n",
                         info->id, base->max_name_len, info->name);
            }
        }
    }

    SGLN_LOG(base, "deinit leave\n");
}

void mpp_sgln_base_run_init(MppSglnBase *base)
{
    rk_s64 sum = 0;
    rk_s32 j;

    SGLN_LOG(base, "init enter\n");

    for (j = 0; j < (rk_s32)base->array_count; j++) {
        MppSglnList *arr = base->arrays[j];
        rk_s32 i;

        for (i = 0; i < (rk_s32)arr->count; i++) {
            MppSglnInfo *info = &arr->entries[i];

            if (info->name && info->init) {
                rk_s64 time = mpp_time();
                SGLN_LOG(base, "entry %2d %-*s init start\n",
                         info->id, base->max_name_len, info->name);
                info->init();
                time = mpp_time() - time;
                sum += time;
                SGLN_LOG(base, "entry %2d %-*s init finish %lld us\n",
                         info->id, base->max_name_len, info->name, time);
            }
        }
    }

    SGLN_LOG(base, "init leave total %lld us\n", sum);
}
