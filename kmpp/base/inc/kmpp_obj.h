/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include "mpp_internal.h"

typedef rk_s32 (*KmppObjInit)(void *entry, KmppObj obj, const char *caller);
typedef rk_s32 (*KmppObjDeinit)(void *entry, KmppObj obj, const char *caller);
typedef rk_s32 (*KmppObjDump)(void *entry);

#ifdef __cplusplus
extern "C" {
#endif

/* userspace objdef register */
rk_s32 kmpp_objdef_register(KmppObjDef *def, rk_s32 size, const char *name);
/* userspace objdef add MppCfgObj root */
rk_s32 kmpp_objdef_add_cfg_root(KmppObjDef def, MppCfgObj root);
/* userspace objdef add KmppEntry table */
rk_s32 kmpp_objdef_add_entry(KmppObjDef def, const char *name, KmppEntry *tbl);
/* userspace object init function register default object is all zero */
rk_s32 kmpp_objdef_add_init(KmppObjDef def, KmppObjInit init);
/* userspace object deinit function register */
rk_s32 kmpp_objdef_add_deinit(KmppObjDef def, KmppObjDeinit deinit);
/* userspace object dump function register */
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);

rk_s32 kmpp_objdef_set_prop(KmppObjDef def, const char *op, rk_s32 value);

/* kernel objdef query from /dev/kmpp_objs */
rk_s32 kmpp_objdef_get(KmppObjDef *def, const char *name);
/* kernel objdef from /dev/kmpp_objs reduce refcnt */
rk_s32 kmpp_objdef_put(KmppObjDef def);

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const char *name, KmppEntry **tbl);
rk_s32 kmpp_objdef_get_offset(KmppObjDef def, const char *name);
rk_s32 kmpp_objdef_dump(KmppObjDef def);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
rk_s32 kmpp_objdef_get_flag_base(KmppObjDef def);
rk_s32 kmpp_objdef_get_flag_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def, const char *caller);
rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const char *name, const char *caller);
rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppShmPtr *sptr, const char *caller);
/* release object and impl head */
rk_s32 kmpp_obj_put(KmppObj obj, const char *caller);
/* release impl head only */
rk_s32 kmpp_obj_impl_put(KmppObj obj, const char *caller);
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);
rk_s32 kmpp_obj_ioctl(KmppObj obj, rk_s32 cmd, KmppObj in, KmppObj out, const char *caller);

#define kmpp_obj_get_f(obj, def)                kmpp_obj_get(obj, def, __FUNCTION__)
#define kmpp_obj_get_by_name_f(obj, name)       kmpp_obj_get_by_name(obj, name, __FUNCTION__)
#define kmpp_obj_get_by_sptr_f(obj, sptr)       kmpp_obj_get_by_sptr(obj, sptr, __FUNCTION__)
#define kmpp_obj_put_f(obj)                     kmpp_obj_put(obj, __FUNCTION__)
#define kmpp_obj_impl_put_f(obj)                kmpp_obj_impl_put(obj, __FUNCTION__)
#define kmpp_obj_check_f(obj)                   kmpp_obj_check(obj, __FUNCTION__)
#define kmpp_obj_ioctl_f(obj, cmd, in, out)     kmpp_obj_ioctl(obj, cmd, in, out, __FUNCTION__)

/* check a object is kobject or not */
rk_s32 kmpp_obj_is_kobj(KmppObj obj);
/* KmppShmPtr is the kernel share object userspace base address for kernel ioctl */
KmppShmPtr *kmpp_obj_to_shm(KmppObj obj);
/* KmppShmPtr size defined the copy size for kernel ioctl */
rk_s32 kmpp_obj_to_shm_size(KmppObj obj);
const char *kmpp_obj_get_name(KmppObj obj);
/*
 * priv is the private data in userspace KmppObjImpl struct for kobject transaction
 * priv = KmppObjImpl->priv
 */
void *kmpp_obj_to_priv(KmppObj obj);
/*
 * entry is the userspace address for kernel share object body
 * entry = KmppShmPtr->uaddr + entry_offset
 */
void *kmpp_obj_to_entry(KmppObj obj);
/* offset is the entry offset from kernel share object body */
rk_s32 kmpp_obj_to_offset(KmppObj obj, const char *name);

/* value access function */
rk_s32 kmpp_obj_set_s32(KmppObj obj, const char *name, rk_s32 val);
rk_s32 kmpp_obj_get_s32(KmppObj obj, const char *name, rk_s32 *val);
rk_s32 kmpp_obj_set_u32(KmppObj obj, const char *name, rk_u32 val);
rk_s32 kmpp_obj_get_u32(KmppObj obj, const char *name, rk_u32 *val);
rk_s32 kmpp_obj_set_s64(KmppObj obj, const char *name, rk_s64 val);
rk_s32 kmpp_obj_get_s64(KmppObj obj, const char *name, rk_s64 *val);
rk_s32 kmpp_obj_set_u64(KmppObj obj, const char *name, rk_u64 val);
rk_s32 kmpp_obj_get_u64(KmppObj obj, const char *name, rk_u64 *val);
rk_s32 kmpp_obj_set_st(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_st(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_tbl_set_s32(KmppObj obj, KmppEntry *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj obj, KmppEntry *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj obj, KmppEntry *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj obj, KmppEntry *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj obj, KmppEntry *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj obj, KmppEntry *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj obj, KmppEntry *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj obj, KmppEntry *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppEntry *tbl, void *val);

/* userspace access only function */
rk_s32 kmpp_obj_set_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_obj(KmppObj obj, const char *name, KmppObj *val);
rk_s32 kmpp_obj_set_ptr(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_tbl_set_obj(KmppObj obj, KmppEntry *tbl, KmppObj val);
rk_s32 kmpp_obj_tbl_get_obj(KmppObj obj, KmppEntry *tbl, KmppObj *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj obj, KmppEntry *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj obj, KmppEntry *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj obj, KmppEntry *tbl, void **val);

/* share access function */
rk_s32 kmpp_obj_set_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_get_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_set_shm(KmppObj obj, KmppEntry *tbl, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_get_shm(KmppObj obj, KmppEntry *tbl, KmppShmPtr *val);

/* helper for get share object from a share memory element */
rk_s32 kmpp_obj_set_shm_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_shm_obj(KmppObj obj, const char *name, KmppObj *val);

/* update flag check function */
rk_s32 kmpp_obj_test(KmppObj obj, const char *name);
rk_s32 kmpp_obj_tbl_test(KmppObj obj, KmppEntry *tbl);
rk_s32 kmpp_obj_update(KmppObj dst, KmppObj src);
rk_s32 kmpp_obj_update_entry(void *entry, KmppObj src);

/* copy entry value from src to dst */
rk_s32 kmpp_obj_copy_entry(KmppObj dst, KmppObj src);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj obj, const char *name);
/* dump by userspace */
rk_s32 kmpp_obj_udump_f(KmppObj obj, const char *caller);
/* dump by kernel */
rk_s32 kmpp_obj_kdump_f(KmppObj obj, const char *caller);

#define kmpp_obj_udump(obj) kmpp_obj_udump_f(obj, __FUNCTION__)
#define kmpp_obj_kdump(obj) kmpp_obj_kdump_f(obj, __FUNCTION__)

const char *strof_elem_type(ElemType type);

#ifdef __cplusplus
}
#endif

#endif /* __KMPP_OBJ_H__ */
