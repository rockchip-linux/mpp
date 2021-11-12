/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __HAL_VP9D_COM_H__
#define __HAL_VP9D_COM_H__

#include "rk_type.h"
#include "mpp_err.h"

typedef RK_U8 vp9_prob;

#define PARTITION_CONTEXTS              16
#define PARTITION_TYPES                 4
#define MAX_SEGMENTS                    8
#define SEG_TREE_PROBS                  (MAX_SEGMENTS-1)
#define PREDICTION_PROBS                3
#define SKIP_CONTEXTS                   3
#define TX_SIZE_CONTEXTS                2
#define TX_SIZES                        4
#define INTRA_INTER_CONTEXTS            4
#define PLANE_TYPES                     2
#define COEF_BANDS                      6
#define COEFF_CONTEXTS                  6
#define UNCONSTRAINED_NODES             3
#define INTRA_MODES                     10
#define INTER_PROB_SIZE_ALIGN_TO_128    151
#define INTRA_PROB_SIZE_ALIGN_TO_128    149
#define BLOCK_SIZE_GROUPS               4
#define COMP_INTER_CONTEXTS             5
#define REF_CONTEXTS                    5
#define INTER_MODE_CONTEXTS             7
#define SWITCHABLE_FILTERS              3   // number of switchable filters
#define SWITCHABLE_FILTER_CONTEXTS      (SWITCHABLE_FILTERS + 1)
#define INTER_MODES                     4
#define MV_JOINTS                       4
#define MV_CLASSES                      11
#define CLASS0_BITS                     1  /* bits at integer precision for class 0 */
#define CLASS0_SIZE                     (1 << CLASS0_BITS)
#define MV_OFFSET_BITS                  (MV_CLASSES + CLASS0_BITS - 2)
#define MV_FP_SIZE                      4

#define PROB_SIZE                       4864
#define COUNT_SIZE                      13208

/*
 * MAX_SEGMAP_SIZE calculate(e.g. 4096x2304):
 *      nCtuX*nCtuY*8*8/2
 *      MaxnCtuX = 4096/64
 *      MaxnCtuY = 2304/64
 * for support 8k resolusion, segmap_size(8k) = segmap_size(4k) * 4
 */
#define MAX_SEGMAP_SIZE                 (73728 * 4)

#define VP9_DUMP 0

//!< memory malloc check
#define MEM_CHECK(ret, val, ...)\
do{\
    if (!(val)) {\
        ret = MPP_ERR_MALLOC; \
        mpp_err("malloc buffer error(%d).\n", __LINE__); \
        mpp_assert(0); goto __FAILED; \
}} while (0)


extern const vp9_prob vp9_kf_y_mode_prob[INTRA_MODES][INTRA_MODES][INTRA_MODES - 1];
extern const vp9_prob vp9_kf_uv_mode_prob[INTRA_MODES][INTRA_MODES - 1];
extern const vp9_prob vp9_kf_partition_probs[PARTITION_CONTEXTS][PARTITION_TYPES - 1];

#ifdef __cplusplus
extern "C" {
#endif

RK_U32 vp9_ver_align(RK_U32 val);
RK_U32 vp9_hor_align(RK_U32 val);

MPP_RET hal_vp9d_output_probe(void *buf, void *dxva);
MPP_RET hal_vp9d_prob_flag_delta(void *buf, void *dxva);
void hal_vp9d_update_counts(void *buf, void *dxva);
MPP_RET hal_vp9d_prob_default(void *buf, void *dxva);

#ifdef __cplusplus
}
#endif

#endif
