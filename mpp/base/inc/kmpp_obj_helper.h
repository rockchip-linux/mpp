/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#if defined(KMPP_OBJ_HERLPER_EXIST)
#error "can not include kmpp_obj_helper.h twice"
#endif

#define KMPP_OBJ_HERLPER_EXIST

#if defined(KMPP_OBJ_IMPL_TYPE) && defined(KMPP_OBJ_IMPL_DEF)

/*
 * macro for register structure fiedl to trie
 * type     -> struct base type
 * f1       -> struct:field1        name segment 1 the field1 part
 * f2       -> struct:field1:field2 name segment 2 the field2 part
 * ftype    -> field type as EntryType
 */
#define FIELD_TO_LOCTBL_FLAG1(f1, ftype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS1(f1, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define ENTRY_TO_TRIE1(ftype, type, f1) \
    do { \
        KmppEntry tbl = FIELD_TO_LOCTBL_ACCESS1(f1, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_IMPL_DEF, #f1, &tbl); \
    } while (0);

#define FIELD_TO_LOCTBL_FLAG2(f1, f2, ftype, field_flag, flag_value) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = offsetof(KMPP_OBJ_IMPL_TYPE, field_flag), \
        .flag_value = flag_value, \
    }

#define FIELD_TO_LOCTBL_ACCESS2(f1, f2, ftype) \
    { \
        .data_offset = offsetof(KMPP_OBJ_IMPL_TYPE, f1.f2), \
        .data_size = sizeof(((KMPP_OBJ_IMPL_TYPE *)0)->f1.f2), \
        .data_type = ENTRY_TYPE_##ftype, \
        .flag_offset = 0, \
        .flag_value = 0, \
    }

#define ENTRY_TO_TRIE2(ftype, type, f1, f2) \
    do { \
        KmppEntry tbl = FIELD_TO_LOCTBL_ACCESS2(KMPP_OBJ_IMPL_TYPE, f1, f2, ftype); \
        kmpp_objdef_add_entry(KMPP_OBJ_IMPL_DEF, #f1":"#f2, &tbl); \
    } while (0);

#else

#define FIELD_TO_LOCTBL_FLAG1(f1, ftype, field_flag, flag_value)
#define FIELD_TO_LOCTBL_ACCESS1(f1, ftype)
#define ENTRY_TO_TRIE1(ftype, type, f1)
#define FIELD_TO_LOCTBL_FLAG2(f1, f2, ftype, field_flag, flag_value)
#define FIELD_TO_LOCTBL_ACCESS2(f1, f2, ftype)
#define ENTRY_TO_TRIE2(ftype, type, f1, f2)

#error "use kmpp_obj_helper.h need macro KMPP_OBJ_IMPL_TYPE and KMPP_OBJ_IMPL_DEF as input"

#endif

#undef KMPP_OBJ_HERLPER_EXIST