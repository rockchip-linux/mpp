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

#ifndef _H264D_SPS_H_
#define _H264D_SPS_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_global.h"


#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET process_sps   (H264_SLICE_t  *currSlice);
void    recycle_subsps(H264_subSPS_t *subset_sps);
MPP_RET process_subsps(H264_SLICE_t  *currSlice);
MPP_RET activate_sps(H264dVideoCtx_t *p_Vid, H264_SPS_t *sps, H264_subSPS_t *subset_sps);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H264D_SPS_H_ */

