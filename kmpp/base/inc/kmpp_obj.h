/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 *
 * kmpp_obj.h - Kernel MPP object management
 *
 * Memory layout (two modes):
 *
 * 1. Normal mode (flex_entry = 0): single contiguous allocation from pool
 *
 *    +----------+----------+----------+----------+----------+
 *    | KmppObj  |  priv[]  | entry[]  | extra[]  | flags[]  |
 *    +----------+----------+----------+----------+----------+
 *    | obj_size |priv_size |entry_size|extra_size|flag_size |
 *                          |<---------- buf_size ---------->|
 *
 * 2. Split mode (flex_entry = 1): impl and entry are separate allocations
 *    Entry buffer can be resized via kmpp_obj_resize() to append VLA data.
 *
 *    Pool allocation (fixed):       Entry buffer (resizable):
 *    +----------+----------+       +----------+----------+----------+----------+
 *    | KmppObj  |  priv[]  |       | entry[]  | extra[]  | flags[]  |  vla[]   |
 *    +----------+----------+       +----------+----------+----------+----------+
 *    | obj_size |priv_size |       |entry_size|extra_size|flag_size | vla_size |
 *                                  |<------------ entry_buf_size ------------->|
 *
 *    - obj_size   : sizeof(KmppObjImpl) (fixed)
 *    - priv_size  : private data size, from KMPP_OBJ_PRIV_SIZE (compile-time)
 *    - entry_size : struct size from KMPP_OBJ_IMPL_TYPE (compile-time)
 *    - extra_size : extra bytes between entry and flags, from KMPP_OBJ_EXTRA_SIZE (compile-time)
 *    - flag_size  : update flags bitmap, auto-computed from flag_max_pos (registration-time)
 *    - vla_size   : variable-length array data, appended by kmpp_obj_resize() (runtime)
 *
 *    kmpp_obj_resize(obj, vla_size):
 *      - Computes buf_size = entry_size + extra_size + flag_size + vla_size internally
 *      - Skips realloc if buf_size <= current entry_buf_size
 *      - Calls def->resize callback to update VLA offsets after successful resize
 */

#ifndef KMPP_OBJ_H
#define KMPP_OBJ_H

#include "mpp_internal.h"

/* objdef flags for kmpp_objdef_get() / kmpp_objdef_register() */
/* keep cfg tree for cfg<->struct conversion */
#define KMPP_OBJDEF_HIERARCHY             (1 << 0)
/* split entry allocation (userspace-registered defs) */
#define KMPP_OBJDEF_FLEX_ENTRY            (1 << 1)
/* put-to-cache instead of deinit */
#define KMPP_OBJDEF_CACHED                (1 << 2)
/* disable mismatch log */
#define KMPP_OBJDEF_MISMATCH_LOG_DISABLE  (1 << 3)

typedef rk_s32 (*KmppObjInit)(void *entry, KmppObj obj, const char *caller);
typedef rk_s32 (*KmppObjDeinit)(void *entry, KmppObj obj, const char *caller);
typedef rk_s32 (*KmppObjPreset)(void *entry, KmppObj obj, const char *val, const char *caller);
typedef rk_s32 (*KmppObjDump)(void *entry);
typedef rk_s32 (*KmppObjResizeCb)(void *entry, KmppObj obj, const char *caller);

#ifdef __cplusplus
extern "C" {
#endif

/* userspace objdef register */
rk_s32 kmpp_objdef_register(KmppObjDef *def, rk_s32 priv_size, rk_s32 size,
                            const char *name, rk_u32 flags);
/* kernel objdef query from /dev/kmpp_objs */
rk_s32 kmpp_objdef_get(KmppObjDef *def, rk_s32 priv_size, const char *name, rk_u32 flags);
/* find kernel objdef by name */
rk_s32 kmpp_objdef_find(KmppObjDef *def, const char *name);
/* kernel objdef from /dev/kmpp_objs reduce refcnt */
rk_s32 kmpp_objdef_put(KmppObjDef def);

/* userspace objdef add MppCfgObj root */
rk_s32 kmpp_objdef_add_cfg_root(KmppObjDef def, MppCfgObj root);
/* userspace objdef get MppCfgObj root */
MppCfgObj kmpp_objdef_get_cfg_root(KmppObjDef def);
/* userspace objdef add KmppEntry table */
rk_s32 kmpp_objdef_add_entry(KmppObjDef def, rk_s32 subroot,
                             const char *name, KmppEntry *tbl);
/* userspace object init function register default object is all zero */
rk_s32 kmpp_objdef_add_init(KmppObjDef def, KmppObjInit init);
/* userspace object deinit function register */
rk_s32 kmpp_objdef_add_deinit(KmppObjDef def, KmppObjDeinit deinit);
/* userspace object preset function register */
rk_s32 kmpp_objdef_add_preset(KmppObjDef def, KmppObjPreset preset);
/* userspace object dump function register */
rk_s32 kmpp_objdef_add_dump(KmppObjDef def, KmppObjDump dump);
/* userspace object resize callback: called after successful entry realloc */
rk_s32 kmpp_objdef_add_resize(KmppObjDef def, KmppObjResizeCb resize);
/* cached objdef callbacks: cache_init on get-hit, cache_deinit on put-return */
rk_s32 kmpp_objdef_add_cache_init(KmppObjDef def, KmppObjInit cache_init);
rk_s32 kmpp_objdef_add_cache_deinit(KmppObjDef def, KmppObjDeinit cache_deinit);

rk_s32 kmpp_objdef_get_entry(KmppObjDef def, const char *name, KmppEntry **tbl);
rk_s32 kmpp_objdef_get_offset(KmppObjDef def, const char *name);
rk_s32 kmpp_objdef_get_cmd(KmppObjDef def, const char *name);
rk_s32 kmpp_objdef_dump(KmppObjDef def);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
rk_s32 kmpp_objdef_get_buf_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def, const char *caller);
rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const char *name, const char *caller);
rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppShmPtr *sptr, const char *caller);
/* release object and impl head */
rk_s32 kmpp_obj_put(KmppObj obj, const char *caller);
/* resize object: append vla_size bytes after entry+flags (skip if buffer already large enough) */
rk_s32 kmpp_obj_resize(KmppObj obj, rk_s32 vla_size, const char *caller);
/* release impl head only */
rk_s32 kmpp_obj_impl_put(KmppObj obj, const char *caller);
/* setup object to a preset value by string args input */
rk_s32 kmpp_obj_preset(KmppObj obj, const char *arg, const char *caller);
/* check object is valid or not */
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);
/* run object's ioctl to kernel with input and output object */
rk_s32 kmpp_obj_ioctl(KmppObj ctx, rk_s32 cmd, KmppObj in, KmppObj *out, const char *caller);

#define kmpp_obj_get_f(obj, def)                kmpp_obj_get(obj, def, __FUNCTION__)
#define kmpp_obj_get_by_name_f(obj, name)       kmpp_obj_get_by_name(obj, name, __FUNCTION__)
#define kmpp_obj_get_by_sptr_f(obj, sptr)       kmpp_obj_get_by_sptr(obj, sptr, __FUNCTION__)
#define kmpp_obj_put_f(obj)                     kmpp_obj_put(obj, __FUNCTION__)
#define kmpp_obj_resize_f(obj, extra)           kmpp_obj_resize(obj, extra, __FUNCTION__)
#define kmpp_obj_impl_put_f(obj)                kmpp_obj_impl_put(obj, __FUNCTION__)
#define kmpp_obj_preset_f(obj, arg)             kmpp_obj_preset(obj, arg, __FUNCTION__)
#define kmpp_obj_check_f(obj)                   kmpp_obj_check(obj, __FUNCTION__)
#define kmpp_obj_ioctl_f(ctx, cmd, in, out)     kmpp_obj_ioctl(ctx, cmd, in, out, __FUNCTION__)

/* safe check whether a pointer refers to a valid KmppObj (plain struct safe) */
rk_s32 kmpp_obj_is_obj(KmppObj obj);
/* check a object is kobject or not */
rk_s32 kmpp_obj_is_kobj(KmppObj obj);
/* object to its objdef */
KmppObjDef kmpp_obj_to_objdef(KmppObj obj);
/* object implement element update flags access */
void *kmpp_obj_to_flags(KmppObj obj);
rk_s32 kmpp_obj_to_flags_size(KmppObj obj);
rk_s32 kmpp_obj_to_entry_buf_size(KmppObj obj);
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
void *kmpp_obj_to_entry_flex(KmppObj obj);
rk_s32 kmpp_obj_to_entry_flex_size(KmppObj obj);
/* offset is the entry offset from kernel share object body */
rk_s32 kmpp_obj_to_offset(KmppObj obj, const char *name);

/* value access function */
rk_s32 kmpp_obj_set_s8(KmppObj obj, const char *name, rk_s8 val);
rk_s32 kmpp_obj_get_s8(KmppObj obj, const char *name, rk_s8 *val);
rk_s32 kmpp_obj_set_u8(KmppObj obj, const char *name, rk_u8 val);
rk_s32 kmpp_obj_get_u8(KmppObj obj, const char *name, rk_u8 *val);
rk_s32 kmpp_obj_set_s16(KmppObj obj, const char *name, rk_s16 val);
rk_s32 kmpp_obj_get_s16(KmppObj obj, const char *name, rk_s16 *val);
rk_s32 kmpp_obj_set_u16(KmppObj obj, const char *name, rk_u16 val);
rk_s32 kmpp_obj_get_u16(KmppObj obj, const char *name, rk_u16 *val);
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
rk_s32 kmpp_obj_tbl_set_s8(KmppObj obj, KmppEntry *tbl, rk_s8 val);
rk_s32 kmpp_obj_tbl_get_s8(KmppObj obj, KmppEntry *tbl, rk_s8 *val);
rk_s32 kmpp_obj_tbl_set_u8(KmppObj obj, KmppEntry *tbl, rk_u8 val);
rk_s32 kmpp_obj_tbl_get_u8(KmppObj obj, KmppEntry *tbl, rk_u8 *val);
rk_s32 kmpp_obj_tbl_set_s16(KmppObj obj, KmppEntry *tbl, rk_s16 val);
rk_s32 kmpp_obj_tbl_get_s16(KmppObj obj, KmppEntry *tbl, rk_s16 *val);
rk_s32 kmpp_obj_tbl_set_u16(KmppObj obj, KmppEntry *tbl, rk_u16 val);
rk_s32 kmpp_obj_tbl_get_u16(KmppObj obj, KmppEntry *tbl, rk_u16 *val);
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

/*
 * KmppObjPos — lightweight value type for navigating VLA structures.
 *
 * Usage:
 *   KmppObjPos pos;
 *   kmpp_obj_pos_init(&pos);
 *   kmpp_obj_pos_seek(obj, &pos, "st_cfg", 1);     // navigate to st_cfg[1]
 *   kmpp_obj_pos_set_s32(obj, &pos, "temporal_id", 1);
 *   kmpp_obj_pos_seek(obj, &pos, NULL, 2);         // switch to st_cfg[2]
 *   kmpp_obj_pos_set_s32(obj, &pos, "repeat", 1);
 *   kmpp_obj_pos_init(&pos);                        // reset to root
 */
typedef struct KmppObjPos_t {
    /* VLA definition (set by seek with name) */
    rk_s32      vla_base;   /* byte offset from entry base to VLA array start */
    rk_u32      subroot;    /* trie subroot node_idx for field lookup */
    rk_u32      elem_size;  /* current VLA element size for index arithmetic */
    /* Current position (derived from definition + idx) */
    rk_s32      offset;     /* byte offset from entry base to current element */
} KmppObjPos;

void    kmpp_obj_pos_init(KmppObjPos *pos);
void    kmpp_obj_pos_dump(KmppObj obj, const KmppObjPos *pos, const char *tag);
rk_s32  kmpp_obj_pos_seek(KmppObj obj, KmppObjPos *pos, const char *name, rk_s32 idx);
rk_s32  kmpp_obj_pos_set_s32(KmppObj obj, const KmppObjPos *pos, const char *name, rk_s32 val);
rk_s32  kmpp_obj_pos_get_s32(KmppObj obj, const KmppObjPos *pos, const char *name, rk_s32 *val);
rk_s32  kmpp_obj_pos_set_u32(KmppObj obj, const KmppObjPos *pos, const char *name, rk_u32 val);
rk_s32  kmpp_obj_pos_get_u32(KmppObj obj, const KmppObjPos *pos, const char *name, rk_u32 *val);
rk_s32  kmpp_obj_pos_set_s64(KmppObj obj, const KmppObjPos *pos, const char *name, rk_s64 val);
rk_s32  kmpp_obj_pos_get_s64(KmppObj obj, const KmppObjPos *pos, const char *name, rk_s64 *val);
rk_s32  kmpp_obj_pos_set_u64(KmppObj obj, const KmppObjPos *pos, const char *name, rk_u64 val);
rk_s32  kmpp_obj_pos_get_u64(KmppObj obj, const KmppObjPos *pos, const char *name, rk_u64 *val);

/* update flag check function */
rk_s32 kmpp_obj_test(KmppObj obj, const char *name);
rk_s32 kmpp_obj_tbl_test(KmppObj obj, KmppEntry *tbl);

/* update entry value marked by update flag from src to dst (clear src flag) */
rk_s32 kmpp_obj_update(KmppObj dst, KmppObj src);
/* update entry value marked by update flag from src to dst (keep src flag) */
rk_s32 kmpp_obj_update_entry(void *entry, KmppObj src);

/* copy entry value and update flag from src to dst (keep src flag) */
rk_s32 kmpp_obj_copy(KmppObj dst, KmppObj src);
/* copy entry value from src to dst only (keep src flag) */
rk_s32 kmpp_obj_copy_entry(KmppObj dst, KmppObj src);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj obj, const char *name);
/* dump by userspace */
rk_s32 kmpp_obj_udump_f(KmppObj obj, const char *caller);
/* dump by kernel */
rk_s32 kmpp_obj_kdump_f(KmppObj obj, const char *caller);

#define kmpp_obj_udump(obj) kmpp_obj_udump_f(obj, __FUNCTION__)
#define kmpp_obj_kdump(obj) kmpp_obj_kdump_f(obj, __FUNCTION__)

rk_s32 kmpp_shm_get(KmppShm *shm, rk_s32 size, const char *caller);
rk_s32 kmpp_shm_put(KmppShm shm, const char *caller);

void *kmpp_shm_to_entry(KmppShm shm, const char *caller);

#define kmpp_shm_get_f(shm, size)   kmpp_shm_get(shm, size, __FUNCTION__)
#define kmpp_shm_put_f(shm)         kmpp_shm_put(shm, __FUNCTION__)
#define kmpp_shm_to_entry_f(shm)    kmpp_shm_to_entry(shm, __FUNCTION__)

const char *strof_elem_type(ElemType type);

#ifdef __cplusplus
}
#endif

#endif /* KMPP_OBJ_H */
