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

#endif /* __KMPP_OBJ_MACRO_H__ */
