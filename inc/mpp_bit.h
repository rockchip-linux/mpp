/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef MPP_BIT_H
#define MPP_BIT_H

#define MPP_BIT(n)                      (1UL << (n))
#define MPP_BIT64(n)                    (1ULL << (n))

#define MPP_FLAG_1OR(a)                 ((RK_U32)(a))
#define MPP_FLAG_2OR(a, b)              ((RK_U32)(a) | (RK_U32)(b))
#define MPP_FLAG_3OR(a, b, c)           ((RK_U32)(a) | (RK_U32)(b) | (RK_U32)(c))
#define MPP_FLAG_4OR(a, b, c, d)        ((RK_U32)(a) | (RK_U32)(b) | (RK_U32)(c) | (RK_U32)(d))
#define MPP_FLAG_5OR(a, b, c, d, e)     ((RK_U32)(a) | (RK_U32)(b) | (RK_U32)(c) | (RK_U32)(d) | (RK_U32)(e))
#define MPP_FLAG_6OR(a, b, c, d, e, f)  ((RK_U32)(a) | (RK_U32)(b) | (RK_U32)(c) | (RK_U32)(d) | (RK_U32)(e) | (RK_U32)(f))

#define MPP_FLAG_OR_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define MPP_FLAG_OR(...) MPP_FLAG_OR_HELPER(__VA_ARGS__, MPP_FLAG_6OR, MPP_FLAG_5OR, MPP_FLAG_4OR, MPP_FLAG_3OR, MPP_FLAG_2OR, MPP_FLAG_1OR)(__VA_ARGS__)

#define MPP_BIT32_1OR(a)                (MPP_BIT(a))
#define MPP_BIT32_2OR(a, b)             (MPP_BIT(a) | MPP_BIT(b))
#define MPP_BIT32_3OR(a, b, c)          (MPP_BIT(a) | MPP_BIT(b) | MPP_BIT(c))
#define MPP_BIT32_4OR(a, b, c, d)       (MPP_BIT(a) | MPP_BIT(b) | MPP_BIT(c) | MPP_BIT(d))
#define MPP_BIT32_5OR(a, b, c, d, e)    (MPP_BIT(a) | MPP_BIT(b) | MPP_BIT(c) | MPP_BIT(d) | MPP_BIT(e))
#define MPP_BIT32_6OR(a, b, c, d, e, f) (MPP_BIT(a) | MPP_BIT(b) | MPP_BIT(c) | MPP_BIT(d) | MPP_BIT(e) | MPP_BIT(f))

#define MPP_BIT32_OR_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define MPP_BIT32_OR(...) MPP_BIT32_OR_HELPER(__VA_ARGS__, MPP_BIT32_6OR, MPP_BIT32_5OR, MPP_BIT32_4OR, MPP_BIT32_3OR, MPP_BIT32_2OR, MPP_BIT32_1OR)(__VA_ARGS__)

#define MPP_BIT64_1OR(a)                (MPP_BIT64(a))
#define MPP_BIT64_2OR(a, b)             (MPP_BIT64(a) | MPP_BIT64(b))
#define MPP_BIT64_3OR(a, b, c)          (MPP_BIT64(a) | MPP_BIT64(b) | MPP_BIT64(c))
#define MPP_BIT64_4OR(a, b, c, d)       (MPP_BIT64(a) | MPP_BIT64(b) | MPP_BIT64(c) | MPP_BIT64(d))
#define MPP_BIT64_5OR(a, b, c, d, e)    (MPP_BIT64(a) | MPP_BIT64(b) | MPP_BIT64(c) | MPP_BIT64(d) | MPP_BIT64(e))
#define MPP_BIT64_6OR(a, b, c, d, e, f) (MPP_BIT64(a) | MPP_BIT64(b) | MPP_BIT64(c) | MPP_BIT64(d) | MPP_BIT64(e) | MPP_BIT64(f))

#define MPP_BIT64_OR_HELPER(_1, _2, _3, _4, _5, _6, NAME, ...) NAME
#define MPP_BIT64_OR(...) MPP_BIT64_OR_HELPER(__VA_ARGS__, MPP_BIT64_6OR, MPP_BIT64_5OR, MPP_BIT64_4OR, MPP_BIT64_3OR, MPP_BIT64_2OR, MPP_BIT64_1OR)(__VA_ARGS__)

#endif /* MPP_BIT_H */
