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

#ifndef __H264E_SLICE_H__
#define __H264E_SLICE_H__

#include "mpp_enc_cfg.h"

#include "h264e_syntax_new.h"
#include "h264e_dpb.h"

#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET h264e_reorder_init(H264eReorderInfo *reorder);
MPP_RET h264e_marking_init(H264eMarkingInfo *marking);

/*
 * h264e_slice_update is called only on cfg is update.
 * When cfg has no changes just use slice next to setup
 */
void h264e_slice_init(H264eSlice *slice, H264eReorderInfo *reorder,
                      H264eMarkingInfo *marking);
RK_S32 h264e_slice_update(H264eSlice *slice, MppEncCfgSet *cfg,
                          SynH264eSps *sps, H264eDpbFrm *frm);

/* reorder context for dpb */
MPP_RET h264e_reorder_wr_op(H264eReorderInfo *info, H264eRplmo *op);
MPP_RET h264e_reorder_rd_op(H264eReorderInfo *info, H264eRplmo *op);

/* mmco context for dpb */
MPP_RET h264e_marking_wr_op(H264eMarkingInfo *info, H264eMmco *op);
MPP_RET h264e_marking_rd_op(H264eMarkingInfo *info, H264eMmco *op);


RK_S32 h264e_slice_read(H264eSlice *slice, void *p, RK_S32 size);
RK_S32 h264e_slice_write(H264eSlice *slice, void *p, RK_U32 size);
RK_S32 h264e_slice_move(RK_U8 *dst, RK_U8 *src, RK_S32 dst_bit, RK_S32 src_bit,
                        RK_S32 src_size);

#ifdef __cplusplus
}
#endif

#endif /* __H264E_SLICE_H__ */
