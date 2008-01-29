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

typedef void*   MppHalCtx;

typedef struct MppHalCfg_t {
    // input
    MppCtxType      type;
    MppCodingType   coding;

    // output
    HalTaskGroup  syntaxes;
    RK_U32          syntax_count;
} MppHalCfg;


typedef struct {
    RK_U32 ctx_size;

    MPP_RET (*init)(void **ctx, MppHalCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    // parser syntax process function
    MPP_RET (*reg_gen)(void *ctx, MppSyntax *syn);

    // hw operation function
    MPP_RET (*start)(void *ctx, MppHalDecTask task);
    MPP_RET (*wait)(void *ctx, MppHalDecTask task);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*control)(void *ctx, RK_S32 cmd, void *param);
} MppHalApi;

typedef struct {
    MppCodingType   mCoding;

    void            *mHalCtx;

    MppSyntax       mSyn[2];
    MppHalApi       *api;

    HalTaskGroup  syntaxes;
    RK_U32          syntax_count;
} MppHal;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_hal_init(MppHal **ctx, MppHalCfg *cfg);
MPP_RET mpp_hal_deinit(MppHal *ctx);

MPP_RET mpp_hal_reg_gen(MppHal *ctx, MppHalDecTask *task);
MPP_RET mpp_hal_hw_start(MppHal *ctx, MppHalDecTask *task);
MPP_RET mpp_hal_hw_wait(MppHal *ctx, MppHalDecTask *task);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_HAL__*/

