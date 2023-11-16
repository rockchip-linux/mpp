/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2023 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_DMABUF_H__
#define __MPP_DMABUF_H__

#include "rk_type.h"
#include "mpp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_dmabuf_sync_begin(RK_S32 fd, RK_S32 ro, const char *caller);
MPP_RET mpp_dmabuf_sync_end(RK_S32 fd, RK_S32 ro, const char *caller);
MPP_RET mpp_dmabuf_sync_partial_begin(RK_S32 fd, RK_S32 ro, RK_U32 offset, RK_U32 length, const char *caller);
MPP_RET mpp_dmabuf_sync_partial_end(RK_S32 fd, RK_S32 ro, RK_U32 offset, RK_U32 length, const char *caller);
MPP_RET mpp_dmabuf_set_name(RK_S32 fd, const char *name, const char *caller);

RK_U32 mpp_dmabuf_sync_partial_support(void);

#ifdef __cplusplus
}
#endif

#endif /* __MPP_DMABUF_H__ */