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

#ifndef _H264D_FILL_H_
#define _H264D_FILL_H_

#include "rk_type.h"
#include "mpp_err.h"
#include "dxva_syntax.h"
#include "h264d_syntax.h"
#include "h264d_global.h"



#ifdef  __cplusplus
extern "C" {
#endif

void fill_picparams(H264dVideoCtx_t *p_Vid, DXVA_PicParams_H264_MVC *pp);
void fill_scanlist(H264dVideoCtx_t *p_Vid, DXVA_Qmatrix_H264 *qm);
void commit_buffer(H264dDxvaCtx_t *dxva);
MPP_RET fill_slice_syntax(H264_SLICE_t *currSlice, H264dDxvaCtx_t *dxva_ctx);
#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _RKV_H264_DECODER_FILL_H_ */

