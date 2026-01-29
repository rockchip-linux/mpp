/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_SPLITER_H
#define H265D_SPLITER_H

#include "rk_type.h"

typedef void* H265dSpliter;

#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 h265d_spliter_init(H265dSpliter *ctx);
RK_S32 h265d_spliter_deinit(H265dSpliter ctx);

RK_S32 h265d_spliter_reset(H265dSpliter ctx);

void h265d_spliter_set_eos(H265dSpliter ctx, RK_S32 eos);
RK_S64 h265d_spliter_get_pts(H265dSpliter ctx);
RK_S64 h265d_spliter_get_dts(H265dSpliter ctx);
void h265d_spliter_sync(H265dSpliter ctx, RK_S32 frame_id);

RK_S32 h265d_spliter_frame(H265dSpliter ctx, const RK_U8 **poutbuf, RK_S32 *poutbuf_size,
                           const RK_U8 *buf, RK_S32 buf_size, RK_S64 pts, RK_S64 dts);

#ifdef  __cplusplus
}
#endif

#endif /* H265D_SPLITER_H */
