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

#ifndef _H264D_PPS_H_
#define _H264D_PPS_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_global.h"


#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET process_pps(H264_SLICE_t *currSlice);
MPP_RET activate_pps(H264dVideoCtx_t *p_Vid, H264_PPS_t *pps);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H264D_PPS_H_ */

