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

#ifndef __AVS2D_DPB_H__
#define __AVS2D_DPB_H__

#include "avs2d_global.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET avs2d_dpb_create(Avs2dCtx_t *p_dec);
MPP_RET avs2d_dpb_destroy(Avs2dCtx_t *p_dec);
MPP_RET avs2d_dpb_insert(Avs2dCtx_t *p_dec, HalDecTask *task);
MPP_RET avs2d_dpb_flush(Avs2dCtx_t *p_dec);
MPP_RET dpb_remove_unused_frame(Avs2dCtx_t *p_dec);
Avs2dFrame_t* get_dpb_frm_by_slot_idx(Avs2dFrameMgr_t *mgr, RK_S32 slot_idx);

#ifdef  __cplusplus
}
#endif

#endif /*__AVS2D_DPB_H__*/
