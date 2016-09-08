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

#ifndef _H264D_DPB_H_
#define _H264D_DPB_H_


#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_global.h"


#ifdef  __cplusplus
extern "C" {
#endif

void    update_ref_list(H264_DpbBuf_t *p_Dpb);
void    update_ltref_list(H264_DpbBuf_t *p_Dpb);
void    free_storable_picture(H264_DecCtx_t *p_Dec, H264_StorePic_t *p);
void    free_frame_store(H264_DecCtx_t *p_Dec, H264_FrameStore_t *f);

MPP_RET idr_memory_management(H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p);
MPP_RET insert_picture_in_dpb(H264dVideoCtx_t *p_Vid, H264_FrameStore_t *fs,    H264_StorePic_t *p, RK_U8 combine_flag);
MPP_RET store_picture_in_dpb (H264_DpbBuf_t *p_Dpb, H264_StorePic_t *p);

MPP_RET init_dpb    (H264dVideoCtx_t *p_Vid, H264_DpbBuf_t *p_Dpb, RK_S32 type);
MPP_RET flush_dpb   (H264_DpbBuf_t   *p_Dpb, RK_S32 type);
MPP_RET output_dpb  (H264_DecCtx_t *p_Dec, H264_DpbBuf_t *p_Dpb);

void    free_dpb    (H264_DpbBuf_t   *p_Dpb);
MPP_RET exit_picture(H264dVideoCtx_t *p_Vid, H264_StorePic_t **dec_pic);

RK_U32  get_filed_dpb_combine_flag(H264_FrameStore_t *p_last, H264_StorePic_t *p);
H264_StorePic_t *alloc_storable_picture(H264dVideoCtx_t *p_Vid, RK_S32 structure);

#ifdef  __cplusplus
}
#endif

//========================================
#endif /* end of _H264D_DPB_H_ */

