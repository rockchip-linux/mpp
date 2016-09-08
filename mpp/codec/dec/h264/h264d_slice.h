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

#ifndef __H264D_SLICE_H__
#define __H264D_SLICE_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_global.h"




#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET  process_slice(H264_SLICE_t *currSlice);

MPP_RET  reset_cur_slice(H264dCurCtx_t *p_Cur, H264_SLICE_t *p);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of __H264D_SLICE_H__ */

