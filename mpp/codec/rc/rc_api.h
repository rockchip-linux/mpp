/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2016 Rockchip Electronics Co., Ltd.
 */

#ifndef __RC_IMPL_H__
#define __RC_IMPL_H__

#include "mpp_rc_api.h"

#ifdef __cplusplus
extern "C" {
#endif

RcImplApi *rc_api_get(MppCodingType type, const char *name);

#ifdef __cplusplus
}
#endif

#endif /* __RC_IMPL_H__ */
