/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#ifndef __KMPP_OBJ_MACRO_H__
#define __KMPP_OBJ_MACRO_H__

#define TO_STR(x)                   #x

/* concat by underscore */
#define CONCAT_US1(a)               a
#define CONCAT_US2(a,b)             a##_##b
#define CONCAT_US3(a,b,c)           a##_##b##_##c
#define CONCAT_US4(a,b,c,d)         a##_##b##_##c##_##d
#define CONCAT_US5(a,b,c,d,e)       a##_##b##_##c##_##d##_##e
#define CONCAT_US6(a,b,c,d,e,f)     a##_##b##_##c##_##d##_##e##_##f

#define CONCAT_US_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_US(...)  CONCAT_US_HELPER(__VA_ARGS__, CONCAT_US6, CONCAT_US5, CONCAT_US4, CONCAT_US3, CONCAT_US2, CONCAT_US1)(__VA_ARGS__)

/* concat by dot */
#define CONCAT_DOT1(a)              a
#define CONCAT_DOT2(a,b)            a.b
#define CONCAT_DOT3(a,b,c)          a.b.c
#define CONCAT_DOT4(a,b,c,d)        a.b.c.d
#define CONCAT_DOT5(a,b,c,d,e)      a.b.c.d.e
#define CONCAT_DOT6(a,b,c,d,e,f)    a.b.c.d.e.f

#define CONCAT_DOT_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_DOT(...) CONCAT_DOT_HELPER(__VA_ARGS__, CONCAT_DOT6, CONCAT_DOT5, CONCAT_DOT4, CONCAT_DOT3, CONCAT_DOT2, CONCAT_DOT1)(__VA_ARGS__)

/* concat to string connect with colon */
#define CONCAT_STR1(a)              TO_STR(a)
#define CONCAT_STR2(a,b)            TO_STR(a:b)
#define CONCAT_STR3(a,b,c)          TO_STR(a:b:c)
#define CONCAT_STR4(a,b,c,d)        TO_STR(a:b:c:d)
#define CONCAT_STR5(a,b,c,d,e)      TO_STR(a:b:c:d:e)
#define CONCAT_STR6(a,b,c,d,e,f)    TO_STR(a:b:c:d:e:f)

#define CONCAT_STR_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define CONCAT_STR(...) CONCAT_STR_HELPER(__VA_ARGS__, CONCAT_STR6, CONCAT_STR5, CONCAT_STR4, CONCAT_STR3, CONCAT_STR2, CONCAT_STR1)(__VA_ARGS__)

/* objdef struct name */
#define KMPP_OBJ_DEF(x)             CONCAT_US(x, def)
#define KMPP_OBJ_DEF_NAME(x)        TO_STR(x)

/*
 * element update flag bits usage:
 * bit 0  - 7   record / replay operation index bit
 * bit 8  - 9   record / replay operation bit
 * bit 10 - 11  update flag update operation invalid / start / update / hold
 */
#define ELEM_FLAG_OP_SHIFT  8
#define ELEM_FLAG_IDX_MASK  ((1 << ELEM_FLAG_OP_SHIFT) - 1)

typedef enum ElemFlagType_e {
    /* element without update flag (not available) */
    ELEM_FLAG_NONE          = (1 << ELEM_FLAG_OP_SHIFT),
    /* element update flag will align to new 32bit */
    ELEM_FLAG_START         = (2 << ELEM_FLAG_OP_SHIFT),
    /* element flag align up to 64bit */
    ELEM_FLAG_START64       = (3 << ELEM_FLAG_OP_SHIFT),
    /* element flag increase by one */
    ELEM_FLAG_UPDATE        = (4 << ELEM_FLAG_OP_SHIFT),
    /* element flag equal to previous one */
    ELEM_FLAG_HOLD          = (5 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_OP_MASK       = (7 << ELEM_FLAG_OP_SHIFT),

    /* index for record element update flag */
    ELEM_FLAG_RECORD        = (8 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_RECORD_0      = (ELEM_FLAG_RECORD + 0),
    ELEM_FLAG_RECORD_1      = (ELEM_FLAG_RECORD + 1),
    ELEM_FLAG_RECORD_2      = (ELEM_FLAG_RECORD + 2),
    ELEM_FLAG_RECORD_3      = (ELEM_FLAG_RECORD + 3),
    ELEM_FLAG_RECORD_4      = (ELEM_FLAG_RECORD + 4),
    ELEM_FLAG_RECORD_5      = (ELEM_FLAG_RECORD + 5),
    ELEM_FLAG_RECORD_6      = (ELEM_FLAG_RECORD + 6),
    ELEM_FLAG_RECORD_7      = (ELEM_FLAG_RECORD + 7),
    ELEM_FLAG_RECORD_8      = (ELEM_FLAG_RECORD + 8),
    ELEM_FLAG_RECORD_9      = (ELEM_FLAG_RECORD + 9),
    ELEM_FLAG_RECORD_BUT,
    ELEM_FLAG_RECORD_MAX    = (ELEM_FLAG_RECORD_BUT - ELEM_FLAG_RECORD),

    /* index for replay element update flag */
    ELEM_FLAG_REPLAY        = (16 << ELEM_FLAG_OP_SHIFT),
    ELEM_FLAG_REPLAY_0      = (ELEM_FLAG_REPLAY + 0),
    ELEM_FLAG_REPLAY_1      = (ELEM_FLAG_REPLAY + 1),
    ELEM_FLAG_REPLAY_2      = (ELEM_FLAG_REPLAY + 2),
    ELEM_FLAG_REPLAY_3      = (ELEM_FLAG_REPLAY + 3),
    ELEM_FLAG_REPLAY_4      = (ELEM_FLAG_REPLAY + 4),
    ELEM_FLAG_REPLAY_5      = (ELEM_FLAG_REPLAY + 5),
    ELEM_FLAG_REPLAY_6      = (ELEM_FLAG_REPLAY + 6),
    ELEM_FLAG_REPLAY_7      = (ELEM_FLAG_REPLAY + 7),
    ELEM_FLAG_REPLAY_8      = (ELEM_FLAG_REPLAY + 8),
    ELEM_FLAG_REPLAY_9      = (ELEM_FLAG_REPLAY + 9),
    ELEM_FLAG_REPLAY_BUT,
} ElemFlagType;

/* macro for register structure update flag type to offset */
#define FLAG_TYPE_TO_OFFSET(flag) \
    ({ \
        rk_u16 __offset; \
        rk_u32 __flag = flag; \
        rk_s32 __flag_op = __flag & ELEM_FLAG_OP_MASK; \
        switch (__flag_op) { \
        case ELEM_FLAG_NONE     :   __offset = 0; break; \
        case ELEM_FLAG_START    :   __flag_base = ((__flag_base + 31) & (~31)); __offset = __flag_base; break; \
        case ELEM_FLAG_START64  :   __flag_base = ((__flag_base + 63) & (~63)); __offset = __flag_base; break; \
        case ELEM_FLAG_UPDATE   :   __offset = ++__flag_base; break; \
        case ELEM_FLAG_HOLD     :   __offset = __flag_base; break; \
        default                 :   __offset = 0; break; \
        }; \
        if (__flag & (ELEM_FLAG_RECORD | ELEM_FLAG_REPLAY)) { \
            rk_s32 __flag_idx = __flag & ELEM_FLAG_IDX_MASK; \
            if (__flag & ELEM_FLAG_RECORD) { \
                __flag_record[__flag_idx] = __offset; \
            } else { \
                __offset = __flag_record[__flag_idx]; \
            } \
        } \
        __offset; \
    })

#define ENTRY_NOTHING(prefix, ftype, type, name, flag, ...)

#endif /* __KMPP_OBJ_MACRO_H__ */
