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
