/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "kmpp_obj_macro.h"

#ifndef KMPP_OBJ_NAME
#error "KMPP_OBJ_NAME must be defined on using kmpp_obj_func.h"
#endif

#ifndef KMPP_OBJ_INTF_TYPE
#error "KMPP_OBJ_INTF_TYPE must be defined on using kmpp_obj_func.h"
#endif

#include "rk_type.h"

/* always define object common function */
#define KMPP_OBJ_FUNC_DEFINE(prefix) \
    rk_s32 CONCAT_US(prefix, size)(void); \
    rk_s32 CONCAT_US(prefix, get)(KMPP_OBJ_INTF_TYPE *p); \
    rk_s32 CONCAT_US(prefix, assign)(KMPP_OBJ_INTF_TYPE *p, void *buf, rk_s32 size); \
    rk_s32 CONCAT_US(prefix, put)(KMPP_OBJ_INTF_TYPE p); \
    rk_s32 CONCAT_US(prefix, dump)(KMPP_OBJ_INTF_TYPE p, const char *caller); \
    KmppObjDef CONCAT_US(prefix, objdef)(void);

#ifdef __cplusplus
extern "C" {
#endif

KMPP_OBJ_FUNC_DEFINE(KMPP_OBJ_NAME)

#ifdef __cplusplus
}
#endif

#undef KMPP_OBJ_FUNC_DEFINE

/* entry and hook access functions */
#ifdef KMPP_OBJ_ENTRY_TABLE
/* disable all hierarchy macro in header */
#define CFG_DEF_START(...)
#define CFG_DEF_END(...)
#define STRUCT_START(...)
#define STRUCT_END(...)

#define ENTRY_DECLARE(prefix, ftype, type, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p, type val); \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p);

#define STRCT_DECLARE(prefix, ftype, type, name, flag, ...) \
    rk_s32 CONCAT_US(prefix, set, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 CONCAT_US(prefix, get, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p, type* val); \
    rk_s32 CONCAT_US(prefix, test, __VA_ARGS__)(KMPP_OBJ_INTF_TYPE p);

#define ALIAS_DECLARE(prefix, ftype, type, name, flag, ...)

#ifdef __cplusplus
extern "C" {
#endif

KMPP_OBJ_ENTRY_TABLE(KMPP_OBJ_NAME, ENTRY_DECLARE, STRCT_DECLARE,
                     ENTRY_DECLARE, STRCT_DECLARE, ALIAS_DECLARE)

#ifdef __cplusplus
}
#endif

#undef ENTRY_DECLARE
#undef ENTRY_TO_ALIAS
#undef STRCT_DECLARE

#undef CFG_DEF_START
#undef CFG_DEF_END
#undef STRUCT_START
#undef STRUCT_END

#endif /* KMPP_OBJ_ENTRY_TABLE */

#ifdef KMPP_OBJ_FUNC_IOCTL

#define IOCTL_CTX(prefix, func, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx);

#define IOCTL_IN_(prefix, func, in_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, in_type in);

#define IOCTL_OUT(prefix, func, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, out_type *out);

#define IOCTL_IO_(prefix, func, in_type, out_type, ...) \
    rk_s32 CONCAT_US(prefix, func)(KMPP_OBJ_INTF_TYPE ctx, in_type in, out_type *out);

#ifdef __cplusplus
extern "C" {
#endif

KMPP_OBJ_FUNC_IOCTL(KMPP_OBJ_NAME, IOCTL_CTX, IOCTL_IN_, IOCTL_OUT, IOCTL_IO_)

#ifdef __cplusplus
}
#endif

#undef IOCTL_CTX
#undef IOCTL_IN_
#undef IOCTL_OUT
#undef IOCTL_IO_

#endif /* KMPP_OBJ_FUNC_IOCTL */

#undef KMPP_OBJ_NAME
#undef KMPP_OBJ_INTF_TYPE
#undef KMPP_OBJ_ENTRY_TABLE
#undef KMPP_OBJ_FUNC_IOCTL
