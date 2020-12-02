/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __HAL_JPEGE_BASE_H__
#define __HAL_JPEGE_BASE_H__

#include "mpp_device.h"
#include "mpp_hal.h"

#include "vepu_common.h"

#include "jpege_syntax.h"
#include "hal_jpege_hdr.h"

#define QUANTIZE_TABLE_SIZE 64

typedef struct HalJpegeRc_t {
    /* For quantization table */
    RK_S32              q_factor;
    RK_U8               *qtable_y;
    RK_U8               *qtable_c;
    RK_S32              last_quality;
} HalJpegeRc;

typedef struct hal_jpege_ctx_s {
    MppDev              dev;
    JpegeBits           bits;
    /* NOTE: regs should reserve space for extra_info */
    void                *regs;
    RK_U32              reg_size;

    MppEncCfgSet        *cfg;
    JpegeSyntax         syntax;
    JpegeFeedback       feedback;

    HalJpegeRc          hal_rc;
    RK_S32              hal_start_pos;
    VepuStrideCfg       stride_cfg;
} HalJpegeCtx;

extern const RK_U32 qp_reorder_table[QUANTIZE_TABLE_SIZE];
extern const RK_U8 jpege_luma_quantizer[QUANTIZE_TABLE_SIZE];
extern const RK_U8 jpege_chroma_quantizer[QUANTIZE_TABLE_SIZE];

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET hal_jpege_vepu_rc(HalJpegeCtx *ctx, HalEncTask *task);

#ifdef __cplusplus
}
#endif

#endif /* __HAL_JPEGE_BASE_H__ */
