/*
 * Copyright 2021 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MPP_LOCK_H__
#define __MPP_LOCK_H__

#include <stdbool.h>

#include "rk_type.h"

#define MPP_FETCH_ADD           __sync_fetch_and_add
#define MPP_FETCH_SUB           __sync_fetch_and_sub
#define MPP_FETCH_OR            __sync_fetch_and_or
#define MPP_FETCH_AND           __sync_fetch_and_and
#define MPP_FETCH_XOR           __sync_fetch_and_xor
#define MPP_FETCH_NAND          __sync_fetch_and_nand

#define MPP_ADD_FETCH           __sync_add_and_fetch
#define MPP_SUB_FETCH           __sync_sub_and_fetch
#define MPP_OR_FETCH            __sync_or_and_fetch
#define MPP_AND_FETCH           __sync_and_and_fetch
#define MPP_XOR_FETCH           __sync_xor_and_fetch
#define MPP_NAND_FETCH          __sync_nand_and_fetch

#define MPP_BOOL_CAS            __sync_bool_compare_and_swap
#define MPP_VAL_CAS             __sync_val_compare_and_swap

#define MPP_SYNC                __sync_synchronize
#define MPP_SYNC_TEST_SET       __sync_lock_test_and_set
#define MPP_SYNC_CLR            __sync_lock_release

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    RK_U32  lock;
} spinlock_t;

void mpp_spinlock_init(spinlock_t *lock);
void mpp_spinlock_lock(spinlock_t *lock);
void mpp_spinlock_unlock(spinlock_t *lock);
bool mpp_spinlock_trylock(spinlock_t *lock);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_LOCK_H__*/
