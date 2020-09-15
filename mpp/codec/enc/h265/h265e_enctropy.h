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

#ifndef __H265E_ENCTROPY_H__
#define __H265E_ENCTROPY_H__

#include "mpp_bitwrite.h"

typedef struct H265eContextModel {
    RK_U8 m_state;  ///< internal state variable
    RK_U8 bBinsCoded;
} H265eContextModel_t;

typedef struct H265eCabacCtx {
    MppWriteCtx *m_bitIf;
    RK_U32      m_low;
    RK_U32      m_range;
    RK_U32      m_bufferedByte;
    RK_S32      m_numBufferedBytes;
    RK_S32      m_bitsLeft;
    RK_U64      m_fracBits;
    RK_U8       m_bIsCounter;
} H265eCabacCtx;

#ifdef __cplusplus
extern "C" {
#endif

void h265e_cabac_init(H265eCabacCtx *cabac_ctx, MppWriteCtx * bitIf);
void h265e_reset_enctropy(void *slice_ctx);
void h265e_cabac_resetBits(H265eCabacCtx *cabac_ctx);
void h265e_cabac_encodeBin(H265eCabacCtx *cabac_ctx, H265eContextModel_t *ctxModel, RK_U32 binValue);
void h265e_cabac_encodeBinTrm(H265eCabacCtx *cabac_ctx, RK_U32 binValue);
void h265e_cabac_start(H265eCabacCtx *cabac_ctx);
void h265e_cabac_finish(H265eCabacCtx *cabac_ctx);
void h265e_cabac_flush(H265eCabacCtx *cabac_ctx);

#ifdef __cplusplus
}
#endif

#endif
