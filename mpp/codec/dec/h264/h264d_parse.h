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

#ifndef __H264D_PARSE_H__
#define __H264D_PARSE_H__
#include "rk_type.h"
#include "mpp_err.h"

#include "h264d_global.h"



#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET open_stream_file(H264dInputCtx_t *p_Inp, char *path);
MPP_RET fwrite_stream_to_file(H264dInputCtx_t *p_Inp, RK_U8 *pdata, RK_U32 len);
MPP_RET close_stream_file(H264dInputCtx_t *p_Inp);
MPP_RET parse_loop(H264_DecCtx_t *p_Dec);
MPP_RET parse_prepare(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur);
MPP_RET parse_prepare_fast(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur);
MPP_RET parse_prepare_avcC_header(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur);
MPP_RET parse_prepare_avcC_data(H264dInputCtx_t *p_Inp, H264dCurCtx_t *p_Cur);

#ifdef  __cplusplus
}
#endif


#endif /* __H264D_PARSE_H__ */
