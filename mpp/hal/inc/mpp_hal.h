/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __MPP_HAL__
#define __MPP_HAL__

#include "rk_mpi.h"
#include "hal_task.h"
#include "mpp_buf_slot.h"

typedef void*   MppHalCtx;

typedef struct MppHalCfg_t {
    // input
    MppCtxType      type;
    MppCodingType   coding;
    MppBufSlots     slots;

    // output
    HalTaskGroup    tasks;
    RK_U32          task_count;
} MppHalCfg;

typedef struct MppHalApi_t {
    char            *name;
    MppCtxType      type;
    MppCodingType   coding;
    RK_U32          ctx_size;
    RK_U32          flag;

    MPP_RET (*init)(void *ctx, MppHalCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    // task preprocess function
    MPP_RET (*reg_gen)(void *ctx, HalTask *syn);

    // hw operation function
    MPP_RET (*start)(void *ctx, HalTask *task);
    MPP_RET (*wait)(void *ctx, HalTask *task);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*control)(void *ctx, RK_S32 cmd, void *param);
} MppHalApi;

typedef void* MppHal;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_hal_init(MppHal *ctx, MppHalCfg *cfg);
MPP_RET mpp_hal_deinit(MppHal ctx);

MPP_RET mpp_hal_reg_gen(MppHal ctx, HalTask *task);
MPP_RET mpp_hal_hw_start(MppHal ctx, HalTask *task);
MPP_RET mpp_hal_hw_wait(MppHal ctx, HalTask *task);

MPP_RET mpp_hal_reset(MppHal ctx);
MPP_RET mpp_hal_flush(MppHal ctx);
MPP_RET mpp_hal_control(MppHal ctx, RK_S32 cmd, void *param);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_HAL__*/

