/*
*
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


#ifndef _H264D_SCALIST_H_
#define _H264D_SCALIST_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_global.h"


#ifdef  __cplusplus
extern "C" {
#endif
RK_U32  is_prext_profile(RK_U32 profile_idc);
MPP_RET get_max_dec_frame_buf_size(H264_SPS_t *sps);
MPP_RET parse_scalingList(BitReadCtx_t *p_bitctx, RK_S32 size, RK_S32 *scaling_list, RK_S32 *use_default);
MPP_RET parse_sps_scalinglists(BitReadCtx_t *p_bitctx, H264_SPS_t *sps);
MPP_RET prepare_init_scanlist(H264_SLICE_t *currSlice);
#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H264D_SCALIST_H_ */

