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

#define MODULE_TAG "mpp_lock"

#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_lock.h"
#include "mpp_time.h"

#define LOCK_IDLE   0
#define LOCK_BUSY   1

void mpp_spinlock_init(spinlock_t *lock)
{
    MPP_SYNC_CLR(&lock->lock);
    lock->count = 0;
    lock->time = 0;

    mpp_env_get_u32("mpp_lock_debug", &lock->debug, 0);
}

void mpp_spinlock_lock(spinlock_t *lock)
{
    RK_S64 time = 0;

    if (lock->debug)
        time = mpp_time();

    while (!MPP_BOOL_CAS(&lock->lock, LOCK_IDLE, LOCK_BUSY)) {
        asm("NOP");
        asm("NOP");
    }

    if (lock->debug && time) {
        lock->time += mpp_time() - time;
        lock->count++;
    }
}

void mpp_spinlock_deinit(spinlock_t *lock, const char *name)
{
    if (lock->debug && lock->count) {
        mpp_log("lock %s lock %lld times take time %lld avg %d", name,
                lock->count, lock->time, (RK_S32)(lock->time / lock->count));
    }
}

void mpp_spinlock_unlock(spinlock_t *lock)
{
    MPP_SYNC_CLR(&lock->lock);
}

bool mpp_spinlock_trylock(spinlock_t *lock)
{
    RK_S64 time = 0;
    bool ret;

    if (lock->debug)
        time = mpp_time();

    ret = MPP_BOOL_CAS(&lock->lock, LOCK_IDLE, LOCK_BUSY);

    if (ret && lock->debug && time) {
        lock->time += mpp_time() - time;
        lock->count++;
    }

    return ret;
}
