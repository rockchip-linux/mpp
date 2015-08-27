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
    MppBuffer       dst;
    // current task reference buffers
    MppBuffer       refer[MAX_REF_SIZE];

    void            *pointers[1];
} MppHalDecTask;

#ifdef __cplusplus
extern "C" {
#endif

void HalTaskSetDstBuffer();
void HalTaskSetDpb();
void HalTaskSetDXVA();

void *mpp_hal_thread(void *data);

#ifdef __cplusplus
}
#endif

#endif /*__HAL_TASK__*/

