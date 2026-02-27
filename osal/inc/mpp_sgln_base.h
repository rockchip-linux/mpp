/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_SGLN_BASE_H
#define MPP_SGLN_BASE_H

#include "rk_type.h"

typedef struct MppSglnInfo_t {
    rk_s32 id;
    const char *name;
    void (*init)(void);
    void (*deinit)(void);
} MppSglnInfo;

typedef struct MppSglnList_t {
    rk_u32 count;
    rk_u32 capacity;
    MppSglnInfo *entries;
} MppSglnList;

typedef struct MppSglnBase_t {
    const char *name;
    MppSglnList *arrays[2];
    rk_u32 array_count;
    rk_u32 max_name_len;
    rk_u32 debug;
} MppSglnBase;

#define MPP_SGLN_STR(x)      #x
#define MPP_SGLN_CAT(p, x)   p##_##x
#define MPP_SGLN_CTOR(p, x)  MPP_SGLN_CAT(p, x)

/* warning: constructor priorities from 0 to 100 are reserved for the implementation */
#define MPP_SGLN_BASE_ID     101
#define MPP_SGLN_MAX_CNT     64

#define MPP_SGLN_DEF(add, id, name, init, deinit) \
    _Static_assert((id) >= 0, "singleton id must be >= 0"); \
    _Static_assert((id) < MPP_SGLN_MAX_CNT, "singleton id out of range"); \
    __attribute__((constructor(MPP_SGLN_BASE_ID + id))) \
    static void MPP_SGLN_CTOR(add, name)(void) { \
        MppSglnInfo info = { id, MPP_SGLN_STR(name), init, deinit, }; \
        add(&info, __FUNCTION__); \
    }

#define MPP_MODULE_DEF(add, name, init, deinit) \
    __attribute__((constructor(MPP_SGLN_BASE_ID + 64))) \
    static void MPP_SGLN_CTOR(add, name)(void) { \
        MppSglnInfo info = { -1, MPP_SGLN_STR(name), init, deinit, }; \
        add(&info, __FUNCTION__); \
    }

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_sgln_base_add(MppSglnBase *base, MppSglnInfo *info, const char *caller);
void mpp_sgln_base_deinit(MppSglnBase *base);
void mpp_sgln_base_run_init(MppSglnBase *base);

#ifdef __cplusplus
}
#endif

#endif /* MPP_SGLN_BASE_H */
