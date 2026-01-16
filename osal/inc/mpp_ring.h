/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_RING_H
#define MPP_RING_H

#include "rk_type.h"

typedef void* MppRing;

#ifdef __cplusplus
extern "C" {
#endif

rk_s32 mpp_ring_get(MppRing *ring, rk_s32 size, const char *name);
rk_s32 mpp_ring_put(MppRing ring);

void *mpp_ring_get_ptr(MppRing ring);
/* resize ring buffer to new size */
void *mpp_ring_resize(MppRing ring, rk_s32 new_size);

#ifdef __cplusplus
}
#endif

#endif /* MPP_RING_H */
