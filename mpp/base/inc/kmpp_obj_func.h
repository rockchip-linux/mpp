/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef KMPP_OBJ_FUNC_PREFIX
#error "KMPP_OBJ_FUNC_PREFIX must be defined on using kmpp_idef_begin.h"
#endif

#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE must be defined on using kmpp_idef_begin.h"
#endif

#ifndef KMPP_OBJ_ENTRY_TABLE
#error "KMPP_OBJ_ENTRY_TABLE must be defined on using kmpp_idef_begin.h"
#endif

#define KMPP_OBJ_FUNC(x, y, z)  x##y##z

#define ENTRY_TO_DECLARE(ftype, type, f1) \
    rk_s32 KMPP_OBJ_FUNC(KMPP_OBJ_FUNC_PREFIX, _set_, f1)(KMPP_OBJ_INTF_TYPE p, type  val); \
    rk_s32 KMPP_OBJ_FUNC(KMPP_OBJ_FUNC_PREFIX, _get_, f1)(KMPP_OBJ_INTF_TYPE p, type* val);

KMPP_OBJ_ENTRY_TABLE(ENTRY_TO_DECLARE)

#undef KMPP_OBJ_FUNC_PREFIX
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_ENTRY_TABLE

#undef ENTRY_TO_DECLARE
#undef KMPP_OBJ_FUNC
