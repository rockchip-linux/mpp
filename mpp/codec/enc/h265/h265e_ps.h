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

#ifndef __H265E_PS_H__
#define __H265E_PS_H__

#include "h265e_slice.h"
#include "h265e_codec.h"

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET h265e_set_sps(H265eCtx *ctx, H265eSps *sps, H265eVps *vps);
MPP_RET h265e_set_pps(H265eCtx *ctx, H265ePps *pps, H265eSps *sps);
MPP_RET h265e_set_vps(H265eCtx *ctx, H265eVps *vps);

#ifdef __cplusplus
}
#endif

#endif
