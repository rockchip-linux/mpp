/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_frame"

#include "kmpp_frame_impl.h"

#define KMPP_OBJ_NAME               kmpp_frame
#define KMPP_OBJ_INTF_TYPE          KmppFrame
#define KMPP_OBJ_IMPL_TYPE          KmppFrameImpl
#define KMPP_OBJ_ENTRY_TABLE        KMPP_FRAME_ENTRY_TABLE
#define KMPP_OBJ_STRUCT_TABLE       KMPP_FRAME_STRUCT_TABLE
#define KMPP_OBJ_FUNC_INIT          kmpp_frame_impl_init
#define KMPP_OBJ_FUNC_DEINIT        kmpp_frame_impl_deinit
#define KMPP_OBJ_FUNC_EXPORT_ENABLE
#define KMPP_OBJ_SHARE_ENABLE
#include "kmpp_obj_helper.h"
