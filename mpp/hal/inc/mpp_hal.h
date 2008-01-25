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

#ifndef __HAL_TASK__
#define __HAL_TASK__

#include "rk_type.h"
#include "h264d_syntax.h"
#include "mpp_dec.h"

#define MAX_REF_SIZE    17

/*
 *  HalTask memory layout:
 *
 *  +----^----+ +----------------------+ +----^----+
 *       |      |     context type     |      |
 *       |      +----------------------+      |
 *       +      |      coding type     |      |
 *     header   +----------------------+      |
 *       +      |         size         |      |
 *       |      +----------------------+      |
 *       |      |     pointer count    |      |
 *  +----v----+ +----------------------+      |
 *              |                      |      |
 *              |       pointers       |      |
 *              |                      |      +
 *              +----------------------+    size
 *              |                      |      +
 *              |        data_0        |      |
 *              |                      |      |
 *              +----------------------+      |
 *              |                      |      |
 *              |        data_1        |      |
 *              |                      |      |
 *              +----------------------+      |
 *              |                      |      |
 *              |                      |      |
 *              |        data_2        |      |
 *              |                      |      |
 *              |                      |      |
 *              +----------------------+ +----v----+
 */
typedef struct {
    MppCtxType      type;
    MppCodingType   coding;
    RK_U32          size;
    RK_U32          count;

    // current tesk output buffer
    RK_S32          index_dst;
    // current task reference buffers
    MppBuffer       refer[MAX_REF_SIZE];

    MppBufSlots     slots;
    H264D_Syntax_t  mSyn;
} MppHalDecTask;

typedef void*   MppHalCtx;

typedef struct MppHalCfg_t {
    MppCtxType      type;
    MppCodingType   coding;
} MppHalCfg;


typedef struct {
    RK_U32 ctx_size;

    MPP_RET (*init)(void **ctx, MppHalCfg *cfg);
    MPP_RET (*deinit)(void *ctx);

    // to parser / mpp
    MPP_RET (*reg_gen)(void *ctx, MppSyntax *syn);

    // hw
    MPP_RET (*start)(void *ctx, MppHalDecTask task);
    MPP_RET (*wait)(void *ctx, MppHalDecTask task);

    MPP_RET (*reset)(void *ctx);
    MPP_RET (*flush)(void *ctx);
    MPP_RET (*control)(void *ctx, RK_S32 cmd, void *param);
} MppHalApi;

typedef struct {
    MppCodingType   mCoding;

    void            *mHalCtx;

    MppSyntax       *mSyn[2];
    MppHalApi       *api;
} MppHal;

#ifdef __cplusplus
extern "C" {
#endif

void *mpp_hal_thread(void *data);

MPP_RET mpp_hal_init(MppHal **ctx, MppHalCfg *cfg);
MPP_RET mpp_hal_deinit(MppHal *ctx);

MPP_RET mpp_hal_reg_gen(MppHal *ctx, MppHalDecTask *task);
MPP_RET mpp_hal_hw_start(MppHal *ctx, MppHalDecTask *task);
MPP_RET mpp_hal_hw_wait(MppHal *ctx, MppHalDecTask *task);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_TASK__*/

