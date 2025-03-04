/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_H__
#define __KMPP_OBJ_H__

#include <linux/types.h>
#include "mpp_trie.h"

typedef enum EntryType_e {
    /* commaon fix size value */
    ENTRY_TYPE_FIX      = 0x0,
    ENTRY_TYPE_s32      = (ENTRY_TYPE_FIX + 0),
    ENTRY_TYPE_u32      = (ENTRY_TYPE_FIX + 1),
    ENTRY_TYPE_s64      = (ENTRY_TYPE_FIX + 2),
    ENTRY_TYPE_u64      = (ENTRY_TYPE_FIX + 3),
    /* value only structure */
    ENTRY_TYPE_st       = (ENTRY_TYPE_FIX + 4),

    /* kernel and userspace share data */
    ENTRY_TYPE_SHARE    = 0x6,
    /* share memory between kernel and userspace */
    ENTRY_TYPE_shm      = (ENTRY_TYPE_SHARE + 0),

    /* kernel access only data */
    ENTRY_TYPE_KERNEL   = 0x8,
    /* kenrel object poineter */
    ENTRY_TYPE_kobj     = (ENTRY_TYPE_KERNEL + 0),
    /* kenrel normal data poineter */
    ENTRY_TYPE_kptr     = (ENTRY_TYPE_KERNEL + 1),
    /* kernel function poineter */
    ENTRY_TYPE_kfp      = (ENTRY_TYPE_KERNEL + 2),

    /* userspace access only data */
    ENTRY_TYPE_USER     = 0xc,
    /* userspace object poineter */
    ENTRY_TYPE_uobj     = (ENTRY_TYPE_USER + 0),
    /* userspace normal data poineter */
    ENTRY_TYPE_uptr     = (ENTRY_TYPE_USER + 1),
    /* userspace function poineter */
    ENTRY_TYPE_ufp      = (ENTRY_TYPE_USER + 2),

    ENTRY_TYPE_BUTT     = 0xf,
} EntryType;

/* location table */
typedef union MppLocTbl_u {
    rk_u64              val;
    struct {
        rk_u16          data_offset;
        rk_u16          data_size       : 12;
        EntryType       data_type       : 4;
        rk_u16          flag_offset;
        rk_u16          flag_value;
    };
} KmppLocTbl;

/* MUST be the same to the KmppObjShm in rk-mpp-kobj.h */
typedef struct KmppShmPtr_t {
    /* uaddr - the userspace base address for userspace access */
    union {
        rk_u64 uaddr;
        void *uptr;
    };
    /* kaddr - the kernel base address for kernel access */
    union {
        rk_u64 kaddr;
        void *kptr;
    };
    /* DO NOT access reserved data only used by kernel */
} KmppShmPtr;

/* KmppObjDef - mpp object name size and access table trie definition */
typedef void* KmppObjDef;
/* KmppObj - mpp object for string name access and function access */
typedef void* KmppObj;

typedef void (*KmppObjPreset)(void *obj);
typedef rk_s32 (*KmppObjDump)(void *obj);

#ifdef __cplusplus
extern "C" {
#endif

/* query objdef from /dev/kmpp_objs */
rk_s32 kmpp_objdef_get(KmppObjDef *def, const char *name);
rk_s32 kmpp_objdef_put(KmppObjDef def);

rk_s32 kmpp_objdef_dump(KmppObjDef def);

/* mpp objcet internal element set / get function */
const char *kmpp_objdef_get_name(KmppObjDef def);
rk_s32 kmpp_objdef_get_entry_size(KmppObjDef def);
MppTrie kmpp_objdef_get_trie(KmppObjDef def);

/* import kernel object ref */
rk_s32 kmpp_obj_get(KmppObj *obj, KmppObjDef def);
rk_s32 kmpp_obj_get_by_name(KmppObj *obj, const char *name);
rk_s32 kmpp_obj_get_by_sptr(KmppObj *obj, KmppShmPtr *sptr);
rk_s32 kmpp_obj_put(KmppObj obj);
rk_s32 kmpp_obj_check(KmppObj obj, const char *caller);
rk_s32 kmpp_obj_ioctl(KmppObj obj, rk_s32 cmd, KmppObj in, KmppObj out);

/* KmppShmPtr is the kernel share object userspace base address for kernel ioctl */
KmppShmPtr *kmpp_obj_to_shm(KmppObj obj);
/* KmppShmPtr size defined the copy size for kernel ioctl */
rk_s32 kmpp_obj_to_shm_size(KmppObj obj);
const char *kmpp_obj_get_name(KmppObj obj);
/*
 * entry is the userspace address for kernel share object body
 * entry = KmppShmPtr->uaddr + entry_offset
 */
void *kmpp_obj_get_entry(KmppObj obj);
/* offset is the entry offset from kernel share object body */
rk_s32 kmpp_obj_get_offset(KmppObj obj, const char *name);

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
rk_s32 kmpp_obj_tbl_set_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 val);
rk_s32 kmpp_obj_tbl_get_s32(KmppObj obj, KmppLocTbl *tbl, rk_s32 *val);
rk_s32 kmpp_obj_tbl_set_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 val);
rk_s32 kmpp_obj_tbl_get_u32(KmppObj obj, KmppLocTbl *tbl, rk_u32 *val);
rk_s32 kmpp_obj_tbl_set_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 val);
rk_s32 kmpp_obj_tbl_get_s64(KmppObj obj, KmppLocTbl *tbl, rk_s64 *val);
rk_s32 kmpp_obj_tbl_set_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 val);
rk_s32 kmpp_obj_tbl_get_u64(KmppObj obj, KmppLocTbl *tbl, rk_u64 *val);
rk_s32 kmpp_obj_tbl_set_st(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_st(KmppObj obj, KmppLocTbl *tbl, void *val);

/* userspace access only function */
rk_s32 kmpp_obj_set_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_obj(KmppObj obj, const char *name, KmppObj *val);
rk_s32 kmpp_obj_set_ptr(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_ptr(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_set_fp(KmppObj obj, const char *name, void *val);
rk_s32 kmpp_obj_get_fp(KmppObj obj, const char *name, void **val);
rk_s32 kmpp_obj_tbl_set_obj(KmppObj obj, KmppLocTbl *tbl, KmppObj val);
rk_s32 kmpp_obj_tbl_get_obj(KmppObj obj, KmppLocTbl *tbl, KmppObj *val);
rk_s32 kmpp_obj_tbl_set_ptr(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_ptr(KmppObj obj, KmppLocTbl *tbl, void **val);
rk_s32 kmpp_obj_tbl_set_fp(KmppObj obj, KmppLocTbl *tbl, void *val);
rk_s32 kmpp_obj_tbl_get_fp(KmppObj obj, KmppLocTbl *tbl, void **val);

/* share access function */
rk_s32 kmpp_obj_set_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_get_shm(KmppObj obj, const char *name, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_set_shm(KmppObj obj, KmppLocTbl *tbl, KmppShmPtr *val);
rk_s32 kmpp_obj_tbl_get_shm(KmppObj obj, KmppLocTbl *tbl, KmppShmPtr *val);

/* helper for get share object from a share memory element */
rk_s32 kmpp_obj_set_shm_obj(KmppObj obj, const char *name, KmppObj val);
rk_s32 kmpp_obj_get_shm_obj(KmppObj obj, const char *name, KmppObj *val);

/* run a callback function */
rk_s32 kmpp_obj_run(KmppObj obj, const char *name);
/* dump by userspace */
rk_s32 kmpp_obj_udump_f(KmppObj obj, const char *caller);
/* dump by kernel */
rk_s32 kmpp_obj_kdump_f(KmppObj obj, const char *caller);

#define kmpp_obj_udump(obj) kmpp_obj_udump_f(obj, __FUNCTION__)
#define kmpp_obj_kdump(obj) kmpp_obj_kdump_f(obj, __FUNCTION__)

#ifdef __cplusplus
}
#endif

#endif /* __KMPP_OBJ_H__ */
