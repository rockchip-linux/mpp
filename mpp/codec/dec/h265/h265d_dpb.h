/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_DPB_H
#define H265D_DPB_H

#include "h265d_ctx.h"

#ifdef  __cplusplus
extern "C" {
#endif

void h265d_frame_unref(H265dPrs *p, RK_U32 dpb_idx, H265dFrmRefStatus clear_mask);
void h265d_frame_unref_all(H265dPrs *p);

RK_S32 h265d_dpb_output(void *ctx, RK_S32 flush);
RK_S32 h265d_build_refs(H265dPrs *p, MppFrame *frame, RK_S32 poc);
void h265d_dpb_flush(H265dPrs *p);

RK_S32 h265d_build_rps(H265dPrs *p);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_DPB_H */
