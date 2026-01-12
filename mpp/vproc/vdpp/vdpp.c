/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpp"

#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>

#include "mpp_common.h"
#include "mpp_env.h"
#include "mpp_service.h"
#include "mpp_platform.h"

#include "vdpp.h"

#define VDPP_MODE_MIN_WIDTH   (128)
#define VDPP_MODE_MIN_HEIGHT  (128)

#define VDPP_DEBUG_ENV_NAME "vdpp_debug"

// default log level: warning
RK_U32 vdpp_debug = VDPP_LOG_FATAL | VDPP_LOG_ERROR | VDPP_LOG_WARNING;

/**
 * thresholds for different scaling coefs:
 * | coef index | ratio    |
 * | ---------- | -------- |
 * | 0          | >= 2.667 |
 * | 1          | >= 2.000 |
 * | 2          | >= 1.500 |
 * | 3          | >  1.000 |
 * | 4          | =  1.000 |
 * | 5          | >= 0.833 |
 * | 6          | >= 0.700 |
 * | 7          | >= 0.500 |
 * | 8          | >= 0.330 |
 * | 9          | >= 0.250 |
 * | 10         | <  0.250 |
 */
static const RK_U16 vdpp_scale_threshold[] = {
    2667, 2000, 1500, 1000, 833, 700, 500, 330, 250,
};

/* default ZME coefs */
static const RK_S16 g_zme_tap8_coeff[11][17][8] = {
//>=2.667
    {
        {   4, -12,  20, 488,  20, -12,   4,   0},
        {   4,  -8,   8, 484,  36, -16,   4,   0},
        {   4,  -4,  -4, 476,  52, -20,   8,   0},
        {   0,   0, -16, 480,  68, -28,   8,   0},
        {   0,   4, -24, 472,  84, -32,   8,   0},
        {   0,   4, -36, 468, 100, -36,  12,   0},
        {   0,   8, -44, 456, 120, -40,  12,   0},
        {   0,  12, -52, 448, 136, -44,  12,   0},
        {   0,  12, -56, 436, 156, -48,  16,  -4},
        {  -4,  16, -60, 424, 176, -52,  16,  -4},
        {  -4,  16, -64, 412, 196, -56,  16,  -4},
        {  -4,  16, -68, 400, 216, -60,  16,  -4},
        {  -4,  20, -72, 380, 236, -64,  20,  -4},
        {  -4,  20, -72, 364, 256, -68,  20,  -4},
        {  -4,  20, -72, 348, 272, -68,  20,  -4},
        {  -4,  20, -72, 332, 292, -72,  20,  -4},
        {  -4,  20, -72, 312, 312, -72,  20,  -4},
    },
//>=2
    {
        {   8, -24,  44, 456,  44, -24,   8,   0},
        {   8, -20,  28, 460,  56, -28,   8,   0},
        {   8, -16,  16, 452,  72, -32,  12,   0},
        {   4, -12,   8, 448,  88, -36,  12,   0},
        {   4,  -8,  -4, 444, 104, -40,  12,   0},
        {   4,  -8, -16, 444, 120, -44,  12,   0},
        {   4,  -4, -24, 432, 136, -48,  16,   0},
        {   4,   0, -32, 428, 152, -52,  16,  -4},
        {   0,   4, -40, 424, 168, -56,  16,  -4},
        {   0,   4, -44, 412, 188, -60,  16,  -4},
        {   0,   8, -52, 400, 204, -60,  16,  -4},
        {   0,   8, -56, 388, 224, -64,  16,  -4},
        {   0,  12, -60, 372, 240, -64,  16,  -4},
        {   0,  12, -64, 356, 264, -68,  16,  -4},
        {   0,  12, -64, 340, 280, -68,  16,  -4},
        {   0,  16, -68, 324, 296, -68,  16,  -4},
        {   0,  16, -68, 308, 308, -68,  16,   0},
    },
//>=1.5
    {
        {  12, -32,  64, 424,  64, -32,  12,   0},
        {   8, -32,  52, 432,  76, -36,  12,   0},
        {   8, -28,  40, 432,  88, -40,  12,   0},
        {   8, -24,  28, 428, 104, -44,  12,   0},
        {   8, -20,  16, 424, 120, -48,  12,   0},
        {   8, -16,   8, 416, 132, -48,  12,   0},
        {   4, -16,  -4, 420, 148, -52,  12,   0},
        {   4, -12, -12, 412, 164, -56,  12,   0},
        {   4,  -8, -20, 400, 180, -56,  12,   0},
        {   4,  -4, -28, 388, 196, -56,  12,   0},
        {   4,  -4, -32, 380, 212, -60,  12,   0},
        {   4,   0, -40, 368, 228, -60,  12,   0},
        {   4,   0, -44, 356, 244, -60,  12,   0},
        {   0,   4, -48, 344, 260, -60,  12,   0},
        {   0,   4, -52, 332, 276, -60,  12,   0},
        {   0,   8, -56, 320, 292, -60,   8,   0},
        {   0,   8, -56, 304, 304, -56,   8,   0},
    },
//>1
    {
        {  12, -40,  84, 400,  84, -40,  12,   0},
        {  12, -40,  72, 404,  96, -44,  12,   0},
        {  12, -36,  60, 404, 108, -48,  12,   0},
        {   8, -32,  48, 404, 120, -48,  12,   0},
        {   8, -32,  36, 404, 136, -52,  12,   0},
        {   8, -28,  28, 396, 148, -52,  12,   0},
        {   8, -24,  16, 392, 160, -52,  12,   0},
        {   8, -20,   8, 384, 176, -56,  12,   0},
        {   8, -20,   0, 384, 188, -56,   8,   0},
        {   8, -16,  -8, 372, 204, -56,   8,   0},
        {   8, -12, -16, 364, 216, -56,   8,   0},
        {   4, -12, -20, 356, 232, -56,   8,   0},
        {   4,  -8, -28, 348, 244, -56,   8,   0},
        {   4,  -8, -32, 332, 264, -52,   4,   0},
        {   4,  -4, -36, 324, 272, -52,   4,   0},
        {   4,   0, -40, 312, 280, -48,   0,   4},
        {   4,   0, -44, 296, 296, -44,   0,   4},
    },
//==1
    {
        { 0,  0,  0,   511, 0,   0,   0,  0 },
        { -1, 3,  -12, 511, 14,  -4,  1,  0 },
        { -2, 6,  -23, 509, 28,  -8,  2,  0 },
        { -2, 9,  -33, 503, 44,  -12, 3,  0 },
        { -3, 11, -41, 496, 61,  -16, 4,  0 },
        { -3, 13, -48, 488, 79,  -21, 5,  -1 },
        { -3, 14, -54, 477, 98,  -25, 7,  -2 },
        { -4, 16, -59, 465, 118, -30, 8,  -2 },
        { -4, 17, -63, 451, 138, -35, 9,  -1 },
        { -4, 18, -66, 437, 158, -39, 10, -2 },
        { -4, 18, -68, 421, 180, -44, 11, -2 },
        { -4, 18, -69, 404, 201, -48, 13, -3 },
        { -4, 18, -70, 386, 222, -52, 14, -2 },
        { -4, 18, -70, 368, 244, -56, 15, -3 },
        { -4, 18, -69, 348, 265, -59, 16, -3 },
        { -4, 18, -67, 329, 286, -63, 16, -3 },
        { -3, 17, -65, 307, 307, -65, 17, -3 },
    },
//>=0.833
    {
        { -16, 0,   145, 254, 145, 0,  -16, 0 },
        { -16, -2,  140, 253, 151, 3,  -17, 0 },
        { -15, -5,  135, 253, 157, 5,  -18, 0 },
        { -14, -7,  129, 252, 162, 8,  -18, 0 },
        { -13, -9,  123, 252, 167, 11, -19, 0 },
        { -13, -11, 118, 250, 172, 15, -19, 0 },
        { -12, -12, 112, 250, 177, 18, -20, -1 },
        { -11, -14, 107, 247, 183, 21, -20, -1 },
        { -10, -15, 101, 245, 188, 25, -21, -1 },
        { -9,  -16, 96,  243, 192, 29, -21, -2 },
        { -8,  -18, 90,  242, 197, 33, -22, -2 },
        { -8,  -19, 85,  239, 202, 37, -22, -2 },
        { -7,  -19, 80,  236, 206, 41, -22, -3 },
        { -7,  -20, 75,  233, 210, 46, -22, -3 },
        { -6,  -21, 69,  230, 215, 50, -22, -3 },
        { -5,  -21, 65,  226, 219, 55, -22, -5 },
        { -5,  -21, 60,  222, 222, 60, -21, -5 },
    },
//>=0.7
    {
        { -16, 0,   145, 254, 145, 0,  -16, 0 },
        { -16, -2,  140, 253, 151, 3,  -17, 0 },
        { -15, -5,  135, 253, 157, 5,  -18, 0 },
        { -14, -7,  129, 252, 162, 8,  -18, 0 },
        { -13, -9,  123, 252, 167, 11, -19, 0 },
        { -13, -11, 118, 250, 172, 15, -19, 0 },
        { -12, -12, 112, 250, 177, 18, -20, -1 },
        { -11, -14, 107, 247, 183, 21, -20, -1 },
        { -10, -15, 101, 245, 188, 25, -21, -1 },
        { -9,  -16, 96,  243, 192, 29, -21, -2 },
        { -8,  -18, 90,  242, 197, 33, -22, -2 },
        { -8,  -19, 85,  239, 202, 37, -22, -2 },
        { -7,  -19, 80,  236, 206, 41, -22, -3 },
        { -7,  -20, 75,  233, 210, 46, -22, -3 },
        { -6,  -21, 69,  230, 215, 50, -22, -3 },
        { -5,  -21, 65,  226, 219, 55, -22, -5 },
        { -5,  -21, 60,  222, 222, 60, -21, -5 },
    },
//>=0.5
    {
        { -16, 0,   145, 254, 145, 0,  -16, 0 },
        { -16, -2,  140, 253, 151, 3,  -17, 0 },
        { -15, -5,  135, 253, 157, 5,  -18, 0 },
        { -14, -7,  129, 252, 162, 8,  -18, 0 },
        { -13, -9,  123, 252, 167, 11, -19, 0 },
        { -13, -11, 118, 250, 172, 15, -19, 0 },
        { -12, -12, 112, 250, 177, 18, -20, -1 },
        { -11, -14, 107, 247, 183, 21, -20, -1 },
        { -10, -15, 101, 245, 188, 25, -21, -1 },
        { -9,  -16, 96,  243, 192, 29, -21, -2 },
        { -8,  -18, 90,  242, 197, 33, -22, -2 },
        { -8,  -19, 85,  239, 202, 37, -22, -2 },
        { -7,  -19, 80,  236, 206, 41, -22, -3 },
        { -7,  -20, 75,  233, 210, 46, -22, -3 },
        { -6,  -21, 69,  230, 215, 50, -22, -3 },
        { -5,  -21, 65,  226, 219, 55, -22, -5 },
        { -5,  -21, 60,  222, 222, 60, -21, -5 },
    },
//>=0.33
    {
        { -18, 18,  144, 226, 144, 19, -17, -4 },
        { -17, 16,  139, 226, 148, 21, -17, -4 },
        { -17, 13,  135, 227, 153, 24, -18, -5 },
        { -17, 11,  131, 226, 157, 27, -18, -5 },
        { -17, 9,   126, 225, 161, 30, -17, -5 },
        { -16, 6,   122, 225, 165, 33, -17, -6 },
        { -16, 4,   118, 224, 169, 37, -17, -7 },
        { -16, 2,   113, 224, 173, 40, -17, -7 },
        { -15, 0,   109, 222, 177, 43, -17, -7 },
        { -15, -1,  104, 220, 181, 47, -16, -8 },
        { -14, -3,  100, 218, 185, 51, -16, -9 },
        { -14, -5,  96,  217, 188, 54, -15, -9 },
        { -14, -6,  91,  214, 192, 58, -14, -9 },
        { -13, -7,  87,  212, 195, 62, -14, -10 },
        { -13, -9,  83,  210, 198, 66, -13, -10 },
        { -12, -10, 79,  207, 201, 70, -12, -11 },
        { -12, -11, 74,  205, 205, 74, -11, -12 },
    },
//>=0.25
    {
        { 14, 66, 113, 133, 113, 66, 14, -7 },
        { 12, 65, 112, 133, 114, 68, 15, -7 },
        { 11, 63, 111, 132, 115, 70, 17, -7 },
        { 10, 62, 110, 132, 116, 71, 18, -7 },
        { 8,  60, 108, 132, 118, 73, 20, -7 },
        { 7,  58, 107, 132, 119, 75, 21, -7 },
        { 6,  56, 106, 132, 120, 76, 23, -7 },
        { 5,  55, 105, 131, 121, 78, 24, -7 },
        { 4,  53, 103, 131, 122, 80, 26, -7 },
        { 3,  51, 102, 131, 122, 81, 28, -6 },
        { 2,  50, 101, 130, 123, 83, 29, -6 },
        { 1,  48, 99,  131, 124, 84, 31, -6 },
        { 0,  46, 98,  129, 125, 86, 33, -5 },
        { -1, 45, 97,  128, 126, 88, 34, -5 },
        { -2, 43, 95,  130, 126, 89, 36, -5 },
        { -3, 41, 94,  128, 127, 91, 38, -4 },
        { -3, 39, 92,  128, 128, 92, 39, -3 },
    },
//others
    {
        { 39, 69, 93, 102, 93, 69, 39, 8 },
        { 38, 68, 92, 102, 93, 70, 40, 9 },
        { 37, 67, 91, 102, 93, 71, 41, 10 },
        { 36, 66, 91, 101, 94, 71, 42, 11 },
        { 35, 65, 90, 102, 94, 72, 43, 11 },
        { 34, 64, 89, 102, 94, 73, 44, 12 },
        { 33, 63, 88, 101, 95, 74, 45, 13 },
        { 32, 62, 88, 100, 95, 75, 46, 14 },
        { 31, 62, 87, 100, 95, 75, 47, 15 },
        { 30, 61, 86, 99,  96, 76, 48, 16 },
        { 29, 60, 86, 98,  96, 77, 49, 17 },
        { 28, 59, 85, 98,  96, 78, 50, 18 },
        { 27, 58, 84, 99,  97, 78, 50, 19 },
        { 26, 57, 83, 99,  97, 79, 51, 20 },
        { 25, 56, 83, 98,  97, 80, 52, 21 },
        { 24, 55, 82, 97,  98, 81, 53, 22 },
        { 23, 54, 81, 98,  98, 81, 54, 23 },
    }
};

static const RK_S16 g_zme_tap6_coeff[11][17][8] = {
//>=2.667
    {
        { -12,  20, 492,  20, -12,   4,   0,   0},
        {  -8,   8, 488,  36, -16,   4,   0,   0},
        {  -4,  -4, 488,  48, -20,   4,   0,   0},
        {   0, -16, 484,  64, -24,   4,   0,   0},
        {   0, -24, 476,  80, -28,   8,   0,   0},
        {   4, -32, 464, 100, -32,   8,   0,   0},
        {   8, -40, 456, 116, -36,   8,   0,   0},
        {   8, -48, 448, 136, -40,   8,   0,   0},
        {  12, -52, 436, 152, -44,   8,   0,   0},
        {  12, -60, 424, 172, -48,  12,   0,   0},
        {  12, -64, 412, 192, -52,  12,   0,   0},
        {  16, -64, 392, 212, -56,  12,   0,   0},
        {  16, -68, 380, 232, -60,  12,   0,   0},
        {  16, -68, 360, 248, -60,  16,   0,   0},
        {  16, -68, 344, 268, -64,  16,   0,   0},
        {  16, -68, 328, 288, -68,  16,   0,   0},
        {  16, -68, 308, 308, -68,  16,   0,   0},
    },
//>=2
    {
        { -20,  40, 468,  40, -20,   4,   0,   0},
        { -16,  28, 464,  56, -24,   4,   0,   0},
        { -16,  16, 464,  68, -28,   8,   0,   0},
        { -12,   4, 460,  84, -32,   8,   0,   0},
        {  -8,  -4, 452, 100, -36,   8,   0,   0},
        {  -4, -12, 444, 116, -40,   8,   0,   0},
        {  -4, -24, 440, 136, -44,   8,   0,   0},
        {   0, -32, 432, 152, -48,   8,   0,   0},
        {   0, -36, 416, 168, -48,  12,   0,   0},
        {   4, -44, 408, 184, -52,  12,   0,   0},
        {   4, -48, 400, 200, -56,  12,   0,   0},
        {   8, -52, 380, 220, -56,  12,   0,   0},
        {   8, -56, 372, 236, -60,  12,   0,   0},
        {   8, -60, 356, 256, -60,  12,   0,   0},
        {  12, -60, 340, 268, -60,  12,   0,   0},
        {  12, -60, 324, 288, -64,  12,   0,   0},
        {  12, -64, 308, 308, -64,  12,   0,   0},
    },
//>=1.5
    {
        { -28,  60, 440,  60, -28,   8,   0,   0},
        { -28,  48, 440,  76, -32,   8,   0,   0},
        { -24,  36, 440,  88, -36,   8,   0,   0},
        { -20,  28, 432, 104, -40,   8,   0,   0},
        { -16,  16, 428, 116, -40,   8,   0,   0},
        { -16,   4, 428, 132, -44,   8,   0,   0},
        { -12,  -4, 420, 148, -48,   8,   0,   0},
        {  -8, -12, 408, 164, -48,   8,   0,   0},
        {  -8, -20, 404, 180, -52,   8,   0,   0},
        {  -4, -24, 388, 196, -52,   8,   0,   0},
        {  -4, -32, 384, 212, -56,   8,   0,   0},
        {   0, -36, 372, 224, -56,   8,   0,   0},
        {   0, -40, 360, 240, -56,   8,   0,   0},
        {   4, -44, 344, 256, -56,   8,   0,   0},
        {   4, -48, 332, 272, -56,   8,   0,   0},
        {   4, -52, 316, 292, -56,   8,   0,   0},
        {   8, -52, 300, 300, -52,   8,   0,   0},
    },
//>1
    {
        { -36,  80, 420,  80, -36,   4,   0,   0},
        { -32,  68, 412,  92, -36,   8,   0,   0},
        { -28,  56, 412, 104, -40,   8,   0,   0},
        { -28,  44, 412, 116, -40,   8,   0,   0},
        { -24,  36, 404, 132, -44,   8,   0,   0},
        { -24,  24, 404, 144, -44,   8,   0,   0},
        { -20,  16, 396, 160, -48,   8,   0,   0},
        { -16,   8, 388, 172, -48,   8,   0,   0},
        { -16,   0, 380, 188, -48,   8,   0,   0},
        { -12,  -8, 376, 200, -48,   4,   0,   0},
        { -12, -12, 364, 216, -48,   4,   0,   0},
        {  -8, -20, 356, 228, -48,   4,   0,   0},
        {  -8, -24, 344, 244, -48,   4,   0,   0},
        {  -4, -32, 332, 260, -48,   4,   0,   0},
        {  -4, -36, 320, 272, -44,   4,   0,   0},
        {   0, -40, 308, 288, -44,   0,   0,   0},
        {   0, -40, 296, 296, -40,   0,   0,   0},
    },
//==1
    {
        {  0,   0, 511,   0,   0,  0, 0, 0 },
        { 3,   -12,  511,  13,   -3,   0, 0, 0 },
        { 6,   -22,  507,  28,   -7,   0, 0, 0 },
        { 8,   -32,  502,  44,   -11,  1, 0, 0 },
        { 10,  -40,  495,  61,   -15,  1, 0, 0 },
        { 11,  -47,  486,  79,   -19,  2, 0, 0 },
        { 12,  -53,  476,  98,   -24,  3, 0, 0 },
        { 13,  -58,  464,  117,  -28,  4, 0, 0 },
        { 14,  -62,  451,  137,  -33,  5, 0, 0 },
        { 15,  -65,  437,  157,  -38,  6, 0, 0 },
        { 15,  -67,  420,  179,  -42,  7, 0, 0 },
        { 15,  -68,  404,  200,  -46,  7, 0, 0 },
        { 14,  -68,  386,  221,  -50,  9, 0, 0 },
        { 14,  -68,  367,  243,  -54,  10, 0, 0 },
        { 14,  -67,  348,  264,  -58,  11, 0, 0 },
        { 13,  -66,  328,  286,  -61,  12, 0, 0 },
        { 13,  -63,  306,  306,  -63,  13, 0, 0 },
    },
//>=0.833
    {
        { -31, 104, 362, 104, -31, 4,  0, 0 },
        { -30, 94,  362, 114, -32, 4,  0, 0 },
        { -29, 84,  361, 125, -32, 3,  0, 0 },
        { -28, 75,  359, 136, -33, 3,  0, 0 },
        { -27, 66,  356, 147, -33, 3,  0, 0 },
        { -25, 57,  353, 158, -33, 2,  0, 0 },
        { -24, 49,  349, 169, -33, 2,  0, 0 },
        { -22, 41,  344, 180, -32, 1,  0, 0 },
        { -20, 33,  339, 191, -31, 0,  0, 0 },
        { -19, 26,  333, 203, -30, -1, 0, 0 },
        { -17, 19,  327, 214, -29, -2, 0, 0 },
        { -16, 13,  320, 225, -27, -3, 0, 0 },
        { -14, 7,   312, 236, -25, -4, 0, 0 },
        { -13, 1,   305, 246, -22, -5, 0, 0 },
        { -11, -4,  295, 257, -19, -6, 0, 0 },
        { -10, -8,  286, 267, -16, -7, 0, 0 },
        { -9,  -12, 277, 277, -12, -9, 0, 0 },
    },
//>=0.7
    {
        { -31, 104, 362, 104, -31, 4,  0, 0 },
        { -30, 94,  362, 114, -32, 4,  0, 0 },
        { -29, 84,  361, 125, -32, 3,  0, 0 },
        { -28, 75,  359, 136, -33, 3,  0, 0 },
        { -27, 66,  356, 147, -33, 3,  0, 0 },
        { -25, 57,  353, 158, -33, 2,  0, 0 },
        { -24, 49,  349, 169, -33, 2,  0, 0 },
        { -22, 41,  344, 180, -32, 1,  0, 0 },
        { -20, 33,  339, 191, -31, 0,  0, 0 },
        { -19, 26,  333, 203, -30, -1, 0, 0 },
        { -17, 19,  327, 214, -29, -2, 0, 0 },
        { -16, 13,  320, 225, -27, -3, 0, 0 },
        { -14, 7,   312, 236, -25, -4, 0, 0 },
        { -13, 1,   305, 246, -22, -5, 0, 0 },
        { -11, -4,  295, 257, -19, -6, 0, 0 },
        { -10, -8,  286, 267, -16, -7, 0, 0 },
        { -9,  -12, 277, 277, -12, -9, 0, 0 },
    },
//>=0.5
    {
        { -20, 130, 297, 130, -20, -5,  0, 0 },
        { -21, 122, 298, 138, -19, -6,  0, 0 },
        { -22, 115, 297, 146, -17, -7,  0, 0 },
        { -22, 108, 296, 153, -16, -7,  0, 0 },
        { -23, 101, 295, 161, -14, -8,  0, 0 },
        { -23, 93,  294, 169, -12, -9,  0, 0 },
        { -24, 87,  292, 177, -10, -10, 0, 0 },
        { -24, 80,  289, 185, -7,  -11, 0, 0 },
        { -24, 73,  286, 193, -4,  -12, 0, 0 },
        { -23, 66,  283, 200, -1,  -13, 0, 0 },
        { -23, 60,  279, 208, 2,   -14, 0, 0 },
        { -23, 54,  276, 215, 5,   -15, 0, 0 },
        { -22, 48,  271, 222, 9,   -16, 0, 0 },
        { -21, 42,  266, 229, 13,  -17, 0, 0 },
        { -21, 37,  261, 236, 17,  -18, 0, 0 },
        { -21, 32,  255, 242, 22,  -18, 0, 0 },
        { -20, 27,  249, 249, 27,  -20, 0, 0 },
    },
//>=0.33
    {
        { 16, 136, 217, 136, 16, -9,  0, 0 },
        { 13, 132, 217, 141, 18, -9,  0, 0 },
        { 11, 128, 217, 145, 21, -10, 0, 0 },
        { 9,  124, 216, 149, 24, -10, 0, 0 },
        { 7,  119, 216, 153, 27, -10, 0, 0 },
        { 5,  115, 216, 157, 30, -11, 0, 0 },
        { 3,  111, 215, 161, 33, -11, 0, 0 },
        { 1,  107, 214, 165, 36, -11, 0, 0 },
        { 0,  102, 213, 169, 39, -11, 0, 0 },
        { -2, 98,  211, 173, 43, -11, 0, 0 },
        { -3, 94,  209, 177, 46, -11, 0, 0 },
        { -4, 90,  207, 180, 50, -11, 0, 0 },
        { -5, 85,  206, 184, 53, -11, 0, 0 },
        { -6, 81,  203, 187, 57, -10, 0, 0 },
        { -7, 77,  201, 190, 61, -10, 0, 0 },
        { -8, 73,  198, 193, 65, -9,  0, 0 },
        { -9, 69,  196, 196, 69, -9,  0, 0 },
    },
//>=0.25
    {
        { 66, 115, 138, 115, 66, 12, 0, 0 },
        { 64, 114, 136, 116, 68, 14, 0, 0 },
        { 63, 113, 134, 117, 70, 15, 0, 0 },
        { 61, 111, 135, 118, 71, 16, 0, 0 },
        { 59, 110, 133, 119, 73, 18, 0, 0 },
        { 57, 108, 134, 120, 74, 19, 0, 0 },
        { 55, 107, 133, 121, 76, 20, 0, 0 },
        { 53, 105, 133, 121, 78, 22, 0, 0 },
        { 51, 104, 133, 122, 79, 23, 0, 0 },
        { 49, 102, 132, 123, 81, 25, 0, 0 },
        { 47, 101, 132, 124, 82, 26, 0, 0 },
        { 45, 99,  131, 125, 84, 28, 0, 0 },
        { 44, 98,  130, 125, 85, 30, 0, 0 },
        { 42, 96,  130, 126, 87, 31, 0, 0 },
        { 40, 95,  128, 127, 89, 33, 0, 0 },
        { 38, 93,  129, 127, 90, 35, 0, 0 },
        { 36, 92,  128, 128, 92, 36, 0, 0 },
    },
//others
    {
        { 80, 105, 116, 105, 80, 26, 0, 0 },
        { 79, 104, 115, 105, 81, 28, 0, 0 },
        { 77, 103, 116, 106, 81, 29, 0, 0 },
        { 76, 102, 115, 106, 82, 31, 0, 0 },
        { 74, 101, 115, 106, 83, 33, 0, 0 },
        { 73, 100, 114, 106, 84, 35, 0, 0 },
        { 71, 99,  114, 107, 84, 37, 0, 0 },
        { 70, 98,  113, 107, 85, 39, 0, 0 },
        { 68, 98,  113, 107, 86, 40, 0, 0 },
        { 67, 97,  112, 108, 86, 42, 0, 0 },
        { 65, 96,  112, 108, 87, 44, 0, 0 },
        { 63, 95,  112, 108, 88, 46, 0, 0 },
        { 62, 94,  112, 108, 88, 48, 0, 0 },
        { 60, 93,  111, 109, 89, 50, 0, 0 },
        { 58, 93,  111, 109, 90, 51, 0, 0 },
        { 57, 92,  110, 110, 90, 53, 0, 0 },
        { 55, 91,  110, 110, 91, 55, 0, 0 },
    }
};

static MPP_RET vdpp_params_to_reg(Vdpp1Params* src_params, Vdpp1ApiCtx *ctx)
{
    struct vdpp_reg *dst_reg = &ctx->reg;
    struct dmsr_params *dmsr_params = &src_params->dmsr_params;
    struct zme_params *zme_params = &src_params->zme_params;

    memset(dst_reg, 0, sizeof(*dst_reg));

    /* 1. set reg::common */
    dst_reg->common.reg0.sw_vdpp_frm_en = 1;

    /* 0x0004(reg1), TODO: add debug function */
    dst_reg->common.reg1.sw_vdpp_src_fmt = VDPP_FMT_YUV420; // vep only support yuv420
    dst_reg->common.reg1.sw_vdpp_src_yuv_swap = src_params->src_yuv_swap;
    dst_reg->common.reg1.sw_vdpp_dst_fmt = src_params->dst_fmt;
    dst_reg->common.reg1.sw_vdpp_dst_yuv_swap = src_params->dst_yuv_swap;
    dst_reg->common.reg1.sw_vdpp_dbmsr_en = dmsr_params->dmsr_enable;

    /* 0x0008(reg2) */
    dst_reg->common.reg2.sw_vdpp_working_mode = VDPP_WORK_MODE_VEP;

    /* 0x000C ~ 0x001C(reg3 ~ reg7), skip */
    dst_reg->common.reg4.sw_vdpp_clk_on = 1;
    dst_reg->common.reg4.sw_md_clk_on = 1;
    dst_reg->common.reg4.sw_dect_clk_on = 1;
    dst_reg->common.reg4.sw_me_clk_on = 1;
    dst_reg->common.reg4.sw_mc_clk_on = 1;
    dst_reg->common.reg4.sw_eedi_clk_on = 1;
    dst_reg->common.reg4.sw_ble_clk_on = 1;
    dst_reg->common.reg4.sw_out_clk_on = 1;
    dst_reg->common.reg4.sw_ctrl_clk_on = 1;
    dst_reg->common.reg4.sw_ram_clk_on = 1;
    dst_reg->common.reg4.sw_dma_clk_on = 1;
    dst_reg->common.reg4.sw_reg_clk_on = 1;

    /* 0x0020(reg8) */
    dst_reg->common.reg8.sw_vdpp_frm_done_en = 1;
    dst_reg->common.reg8.sw_vdpp_osd_max_en = 1;
    dst_reg->common.reg8.sw_vdpp_bus_error_en = 1;
    dst_reg->common.reg8.sw_vdpp_timeout_int_en = 1;
    dst_reg->common.reg8.sw_vdpp_config_error_en = 1;
    /* 0x0024 ~ 0x002C(reg9 ~ reg11), skip */
    {
        RK_U32 src_right_redundant = src_params->src_width % 16 == 0 ? 0 : 16 - src_params->src_width % 16;
        RK_U32 src_down_redundant  = src_params->src_height % 8 == 0 ? 0 : 8 - src_params->src_height % 8;
        RK_U32 dst_right_redundant = src_params->dst_width % 16 == 0 ? 0 : 16 - src_params->dst_width % 16;
        /* 0x0030(reg12) */
        dst_reg->common.reg12.sw_vdpp_src_vir_y_stride = (src_params->src_width + src_right_redundant + 3) / 4;

        /* 0x0034(reg13) */
        dst_reg->common.reg13.sw_vdpp_dst_vir_y_stride = (src_params->dst_width + dst_right_redundant + 3) / 4;

        /* 0x0038(reg14) */
        dst_reg->common.reg14.sw_vdpp_src_pic_width = src_params->src_width + src_right_redundant - 1;
        dst_reg->common.reg14.sw_vdpp_src_right_redundant = src_right_redundant;
        dst_reg->common.reg14.sw_vdpp_src_pic_height = src_params->src_height + src_down_redundant - 1;
        dst_reg->common.reg14.sw_vdpp_src_down_redundant = src_down_redundant;

        /* 0x003C(reg15) */
        dst_reg->common.reg15.sw_vdpp_dst_pic_width = src_params->dst_width + dst_right_redundant - 1;
        dst_reg->common.reg15.sw_vdpp_dst_right_redundant = dst_right_redundant;
        dst_reg->common.reg15.sw_vdpp_dst_pic_height = src_params->dst_height - 1;
    }
    /* 0x0040 ~ 0x005C(reg16 ~ reg23), skip */
    dst_reg->common.reg20.sw_vdpp_timeout_en = 1;
    dst_reg->common.reg20.sw_vdpp_timeout_cnt = 0x8FFFFFF;

    /* 0x0060(reg24) */
    dst_reg->common.reg24.sw_vdpp_src_addr_y = src_params->src.y;

    /* 0x0064(reg25) */
    dst_reg->common.reg25.sw_vdpp_src_addr_uv = src_params->src.cbcr;

    /* 0x0068(reg26) */
    dst_reg->common.reg26.sw_vdpp_dst_addr_y = src_params->dst.y;

    /* 0x006C(reg27) */
    dst_reg->common.reg27.sw_vdpp_dst_addr_uv = src_params->dst.cbcr;

    set_dmsr_to_vdpp_reg(dmsr_params, &dst_reg->dmsr);

    zme_params->src_width = src_params->src_width;
    zme_params->src_height = src_params->src_height;
    zme_params->dst_width = src_params->dst_width;
    zme_params->dst_height = src_params->dst_height;
    zme_params->dst_fmt = src_params->dst_fmt;
    set_zme_to_vdpp_reg(zme_params, &dst_reg->zme);

    return MPP_OK;
}

void vdpp_set_default_dmsr_param(DmsrParams* param)
{
    param->dmsr_enable = 1;
    param->dmsr_str_pri_y = 10;
    param->dmsr_str_sec_y = 4;
    param->dmsr_dumping_y = 6;
    param->dmsr_wgt_pri_gain_even_1 = 12;
    param->dmsr_wgt_pri_gain_even_2 = 12;
    param->dmsr_wgt_pri_gain_odd_1 = 8;
    param->dmsr_wgt_pri_gain_odd_2 = 16;
    param->dmsr_wgt_sec_gain = 5;
    param->dmsr_blk_flat_th = 20;
    param->dmsr_contrast_to_conf_map_x0 = 1680;
    param->dmsr_contrast_to_conf_map_x1 = 6720;
    param->dmsr_contrast_to_conf_map_y0 = 0;
    param->dmsr_contrast_to_conf_map_y1 = 65535;
    param->dmsr_diff_core_th0 = 1;
    param->dmsr_diff_core_th1 = 5;
    param->dmsr_diff_core_wgt0 = 16;
    param->dmsr_diff_core_wgt1 = 16;
    param->dmsr_diff_core_wgt2 = 16;
    param->dmsr_edge_th_low_arr[0] = 30;
    param->dmsr_edge_th_low_arr[1] = 10;
    param->dmsr_edge_th_low_arr[2] = 0;
    param->dmsr_edge_th_low_arr[3] = 0;
    param->dmsr_edge_th_low_arr[4] = 0;
    param->dmsr_edge_th_low_arr[5] = 0;
    param->dmsr_edge_th_low_arr[6] = 0;
    param->dmsr_edge_th_high_arr[0] = 60;
    param->dmsr_edge_th_high_arr[1] = 40;
    param->dmsr_edge_th_high_arr[2] = 20;
    param->dmsr_edge_th_high_arr[3] = 10;
    param->dmsr_edge_th_high_arr[4] = 10;
    param->dmsr_edge_th_high_arr[5] = 10;
    param->dmsr_edge_th_high_arr[6] = 10;
}

void vdpp_set_default_zme_param(ZmeParams *param)
{
    param->zme_bypass_en = 1;
    param->zme_dering_enable = 1;
    param->zme_dering_sen_0 = 16;
    param->zme_dering_sen_1 = 4;
    param->zme_dering_blend_alpha = 16;
    param->zme_dering_blend_beta = 13;
    param->zme_tap8_coeff = g_zme_tap8_coeff;
    param->zme_tap6_coeff = g_zme_tap6_coeff;
}

static MPP_RET vdpp_set_default_param(Vdpp1Params *param)
{
    /* src_fmt only NV12 supported */
    param->src_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->dst_fmt = VDPP_FMT_YUV444;
    param->dst_yuv_swap = VDPP_YUV_SWAP_SP_UV;
    param->src_width = 1920;
    param->src_height = 1080;
    param->dst_width = 1920;
    param->dst_height = 1080;

    vdpp_set_default_dmsr_param(&param->dmsr_params);
    vdpp_set_default_zme_param(&param->zme_params);

    return MPP_OK;
}

void set_dmsr_to_vdpp_reg(const DmsrParams *dmsr_param, RegDmsr *dmsr)
{
    /* 0x0080(reg0) */
    dmsr->reg0.sw_dmsr_edge_low_thre_0 = dmsr_param->dmsr_edge_th_low_arr[0];
    dmsr->reg0.sw_dmsr_edge_high_thre_0 = dmsr_param->dmsr_edge_th_high_arr[0];

    /* 0x0084(reg1) */
    dmsr->reg1.sw_dmsr_edge_low_thre_1 = dmsr_param->dmsr_edge_th_low_arr[1];
    dmsr->reg1.sw_dmsr_edge_high_thre_1 = dmsr_param->dmsr_edge_th_high_arr[1];

    /* 0x0088(reg2) */
    dmsr->reg2.sw_dmsr_edge_low_thre_2 = dmsr_param->dmsr_edge_th_low_arr[2];
    dmsr->reg2.sw_dmsr_edge_high_thre_2 = dmsr_param->dmsr_edge_th_high_arr[2];

    /* 0x008C(reg3) */
    dmsr->reg3.sw_dmsr_edge_low_thre_3 = dmsr_param->dmsr_edge_th_low_arr[3];
    dmsr->reg3.sw_dmsr_edge_high_thre_3 = dmsr_param->dmsr_edge_th_high_arr[3];

    /* 0x0090(reg4) */
    dmsr->reg4.sw_dmsr_edge_low_thre_4 = dmsr_param->dmsr_edge_th_low_arr[4];
    dmsr->reg4.sw_dmsr_edge_high_thre_4 = dmsr_param->dmsr_edge_th_high_arr[4];

    /* 0x0094(reg5) */
    dmsr->reg5.sw_dmsr_edge_low_thre_5 = dmsr_param->dmsr_edge_th_low_arr[5];
    dmsr->reg5.sw_dmsr_edge_high_thre_5 = dmsr_param->dmsr_edge_th_high_arr[5];

    /* 0x0098(reg6) */
    dmsr->reg6.sw_dmsr_edge_low_thre_6 = dmsr_param->dmsr_edge_th_low_arr[6];
    dmsr->reg6.sw_dmsr_edge_high_thre_6 = dmsr_param->dmsr_edge_th_high_arr[6];
    {
        RK_U16 adj_mapping_k[7];
        RK_U16 tmp_diff;
        RK_U16 i;
        RK_U16 contrast2conf_mapping_k;
        RK_U32 tmp_diff_y, tmp_diff_x;

        for (i = 0; i < 7; i++) {
            tmp_diff = dmsr_param->dmsr_edge_th_high_arr[i] - dmsr_param->dmsr_edge_th_low_arr[i];
            adj_mapping_k[i] = (65535 / MPP_MAX(1, tmp_diff));
        }
        tmp_diff_y = dmsr_param->dmsr_contrast_to_conf_map_y1 - dmsr_param->dmsr_contrast_to_conf_map_y0;
        tmp_diff_x = MPP_MAX(dmsr_param->dmsr_contrast_to_conf_map_x1 - dmsr_param->dmsr_contrast_to_conf_map_x0, 1);

        contrast2conf_mapping_k = mpp_clip(256 * tmp_diff_y / tmp_diff_x, 0, 65535);
        /* 0x009C(reg7) */
        dmsr->reg7.sw_dmsr_edge_k_0 = adj_mapping_k[0];
        dmsr->reg7.sw_dmsr_edge_k_1 = adj_mapping_k[1];

        /* 0x00A0(reg8) */
        dmsr->reg8.sw_dmsr_edge_k_2 = adj_mapping_k[2];
        dmsr->reg8.sw_dmsr_edge_k_3 = adj_mapping_k[3];

        /* 0x00A4(reg9) */
        dmsr->reg9.sw_dmsr_edge_k_4 = adj_mapping_k[4];
        dmsr->reg9.sw_dmsr_edge_k_5 = adj_mapping_k[5];

        /* 0x00A8(reg10) */
        dmsr->reg10.sw_dmsr_edge_k_6 = adj_mapping_k[6];
        dmsr->reg10.sw_dmsr_dir_contrast_conf_f = contrast2conf_mapping_k;
    }
    /* 0x00AC(reg11) */
    dmsr->reg11.sw_dmsr_dir_contrast_conf_x0 = dmsr_param->dmsr_contrast_to_conf_map_x0;
    dmsr->reg11.sw_dmsr_dir_contrast_conf_x1 = dmsr_param->dmsr_contrast_to_conf_map_x1;

    /* 0x00B0(reg12) */
    dmsr->reg12.sw_dmsr_dir_contrast_conf_y0 = dmsr_param->dmsr_contrast_to_conf_map_y0;
    dmsr->reg12.sw_dmsr_dir_contrast_conf_y1 = dmsr_param->dmsr_contrast_to_conf_map_y1;

    /* 0x00B4(reg13) */
    dmsr->reg13.sw_dmsr_var_th = dmsr_param->dmsr_blk_flat_th;

    /* 0x00B8(reg14) */
    dmsr->reg14.sw_dmsr_diff_coring_th0 = dmsr_param->dmsr_diff_core_th0;
    dmsr->reg14.sw_dmsr_diff_coring_th1 = dmsr_param->dmsr_diff_core_th1;

    /* 0x00BC(reg15) */
    dmsr->reg15.sw_dmsr_diff_coring_wgt0 = dmsr_param->dmsr_diff_core_wgt0;
    dmsr->reg15.sw_dmsr_diff_coring_wgt1 = dmsr_param->dmsr_diff_core_wgt1;
    dmsr->reg15.sw_dmsr_diff_coring_wgt2 = dmsr_param->dmsr_diff_core_wgt2;
    {
        RK_U16 diff_coring_y0 = dmsr_param->dmsr_diff_core_th0 * dmsr_param->dmsr_diff_core_wgt0;
        RK_U16 diff_coring_y1 = (dmsr_param->dmsr_diff_core_th1 - dmsr_param->dmsr_diff_core_th0) *
                                dmsr_param->dmsr_diff_core_wgt1 + diff_coring_y0;

        /* 0x00C0(reg16) */
        dmsr->reg16.sw_dmsr_diff_coring_y0 = diff_coring_y0;
        dmsr->reg16.sw_dmsr_diff_coring_y1 = diff_coring_y1;
    }
    /* 0x00C4(reg17) */
    dmsr->reg17.sw_dmsr_wgt_pri_gain_1_odd = dmsr_param->dmsr_wgt_pri_gain_odd_1;
    dmsr->reg17.sw_dmsr_wgt_pri_gain_1_even = dmsr_param->dmsr_wgt_pri_gain_even_1;
    dmsr->reg17.sw_dmsr_wgt_pri_gain_2_odd = dmsr_param->dmsr_wgt_pri_gain_odd_2;
    dmsr->reg17.sw_dmsr_wgt_pri_gain_2_even = dmsr_param->dmsr_wgt_pri_gain_even_2;

    /* 0x00C8(reg18) */
    dmsr->reg18.sw_dmsr_wgt_sec_gain_1 = dmsr_param->dmsr_wgt_sec_gain;
    dmsr->reg18.sw_dmsr_wgt_sec_gain_2 = dmsr_param->dmsr_wgt_sec_gain * 2;

    /* 0x00CC(reg19) */
    dmsr->reg19.sw_dmsr_strength_pri = dmsr_param->dmsr_str_pri_y;
    dmsr->reg19.sw_dmsr_strength_sec = dmsr_param->dmsr_str_sec_y;
    dmsr->reg19.sw_dmsr_dump = dmsr_param->dmsr_dumping_y;
}

void calc_scale_factor(const ZmeParams *zme_params, VdppZmeScaleInfo *scale_info, RK_U8 bypass_en)
{
    RK_U16 act_width = scale_info->act_width;
    RK_U16 dsp_width = scale_info->dsp_width;

    RK_U16 act_height = scale_info->act_height;
    RK_U16 dsp_height = scale_info->dsp_height;

    RK_U8 xsd_en = 0;
    RK_U8 xsu_en = 0;
    RK_U8 xscl_mode = scale_info->xscl_mode;
    RK_U16 xscl_factor;
    RK_U8 xscl_offset = 0;

    RK_U8 ysd_en = 0;
    RK_U8 ysu_en = 0;
    RK_U8 yscl_mode = scale_info->yscl_mode;
    RK_U16 yscl_factor;
    RK_U8 yscl_offset = 0;

    RK_U8 xavg_en = 0;
    RK_U8 xgt_en = 0;
    RK_U8 xgt_mode = 0;

    RK_U8 yavg_en = 0;
    RK_U8 ygt_en = 0;
    RK_U8 ygt_mode = 0;

    RK_U32 f_xscl_factor_t;
    RK_U32 f_yscl_factor_t;
    RK_U32 f_xscl_factor_t1;
    RK_U32 f_yscl_factor_t1;

    if (act_width >= dsp_width * 14) {
        act_width = act_width / 4;
        xgt_en = 1;
        xgt_mode = 3;
    } else if (act_width >= dsp_width * 7) {
        act_width = act_width / 2;
        xgt_en = 1;
        xgt_mode = 1;
    }

    if (act_width > dsp_width) {
        xsd_en = 1;
        xsu_en = 0;
        xscl_factor = GET_SCALE_FACTOR_DN(act_width, dsp_width);
    } else if (act_width < dsp_width) {
        xsd_en = 0;
        xsu_en = 1;
        xscl_factor = GET_SCALE_FACTOR_UP(act_width, dsp_width);
    } else {
        xsd_en = 0;
        xsu_en = 0;
        xscl_factor = 1 << 12;
    }

    if (yscl_mode <= SCL_BIL) {
        if (act_height > dsp_height * 4) {
            ygt_en = 1;
            ygt_mode = 1;
            act_height = act_height / 4;
        } else if (act_height > dsp_height * 2) {
            ygt_en = 1;
            ygt_mode = 0;
            act_height = act_height / 2;
        } else {
            ygt_en = 0;
            ygt_mode = 0;
        }
    }

    if (yscl_mode == SCL_MPH) {
        if (act_height >= dsp_height * 6) {
            ygt_en = 1;
            ygt_mode = 3;
        }
    }

    if (act_height > dsp_height) {
        ysd_en = 1;
        ysu_en = 0;
        yscl_factor = GET_SCALE_FACTOR_DN(act_height, dsp_height);
    } else if (act_height < dsp_height) {
        ysd_en = 0;
        ysu_en = 1;
        yscl_factor = GET_SCALE_FACTOR_UP(act_height, dsp_height);
    } else {
        ysd_en = 0;
        ysu_en = 0;
        yscl_factor = 1 << 12;
    }

    if (xsu_en == 1) {
        f_xscl_factor_t = (1 << 16) * act_width / dsp_width;
        f_xscl_factor_t1 = 1000 * (1 << 16) / f_xscl_factor_t;
    } else {
        f_xscl_factor_t = (1 << 12) * act_width / dsp_width;
        f_xscl_factor_t1 = 1000 * (1 << 12) / f_xscl_factor_t;
    }

    if (ysu_en == 1) {
        f_yscl_factor_t = (1 << 16) * act_height / dsp_height;
        f_yscl_factor_t1 = 1000 * (1 << 16) / f_yscl_factor_t;
    } else {
        f_yscl_factor_t = (1 << 12) * act_height / dsp_height;
        f_yscl_factor_t1 = 1000 * (1 << 12) / f_yscl_factor_t;
    }

    if (f_xscl_factor_t1 >= vdpp_scale_threshold[0]) { // >= 2.667
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[0];
        scale_info->xscl_coe_idx = 0;
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[1]) { // >= 2.000
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[1];
        scale_info->xscl_coe_idx = 1;
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[2]) { // >= 1.500
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[2];
        scale_info->xscl_coe_idx = 2;
    } else if (f_xscl_factor_t1 > vdpp_scale_threshold[3]) { // >  1.000
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[3];
        scale_info->xscl_coe_idx = 3;
    } else if (f_xscl_factor_t1 == vdpp_scale_threshold[3]) { // =  1.000
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[4];
        scale_info->xscl_coe_idx = 10;                        // should be 10, not 4 !!
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[4]) { // >= 0.833
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[5];
        scale_info->xscl_coe_idx = 4;                         // continue indexing from 4
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[5]) { // >= 0.700
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[6];
        scale_info->xscl_coe_idx = 5;
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[6]) { // >= 0.500
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[7];
        scale_info->xscl_coe_idx = 6;
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[7]) { // >= 0.330
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[8];
        scale_info->xscl_coe_idx = 7;
    } else if (f_xscl_factor_t1 >= vdpp_scale_threshold[8]) { // >= 0.250
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[9];
        scale_info->xscl_coe_idx = 8;
    } else { // <  0.250
        scale_info->xscl_zme_coe = zme_params->zme_tap8_coeff[10];
        scale_info->xscl_coe_idx = 9;
    }

    if (f_yscl_factor_t1 >= vdpp_scale_threshold[0]) { // >= 2.667
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[0];
        scale_info->yscl_coe_idx = 0;
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[1]) { // >= 2.000
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[1];
        scale_info->yscl_coe_idx = 1;
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[2]) { // >= 1.500
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[2];
        scale_info->yscl_coe_idx = 2;
    } else if (f_yscl_factor_t1 > vdpp_scale_threshold[3]) { // >  1.000
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[3];
        scale_info->yscl_coe_idx = 3;
    } else if (f_yscl_factor_t1 == vdpp_scale_threshold[3]) { // =  1.000
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[4];
        scale_info->yscl_coe_idx = 10;                        // should be 10, not 4 !!
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[4]) { // >= 0.833
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[5];
        scale_info->yscl_coe_idx = 4;                         // continue indexing from 4
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[5]) { // >= 0.700
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[6];
        scale_info->yscl_coe_idx = 5;
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[6]) { // >= 0.500
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[7];
        scale_info->yscl_coe_idx = 6;
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[7]) { // >= 0.330
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[8];
        scale_info->yscl_coe_idx = 7;
    } else if (f_yscl_factor_t1 >= vdpp_scale_threshold[8]) { // >= 0.250
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[9];
        scale_info->yscl_coe_idx = 8;
    } else { // <  0.250
        scale_info->yscl_zme_coe = zme_params->zme_tap6_coeff[10];
        scale_info->yscl_coe_idx = 9;
    }

    scale_info->xsd_en = xsd_en;
    scale_info->xsu_en = xsu_en;
    scale_info->xscl_mode = xscl_mode;
    scale_info->xscl_factor = xscl_factor;
    scale_info->xscl_offset = xscl_offset;

    scale_info->ysd_en = ysd_en;
    scale_info->ysu_en = ysu_en;
    scale_info->yscl_mode = yscl_mode;
    scale_info->yscl_factor = yscl_factor;
    scale_info->yscl_offset = yscl_offset;

    scale_info->xavg_en = xavg_en;
    scale_info->xgt_en = xgt_en;
    scale_info->xgt_mode = xgt_mode;

    scale_info->yavg_en = yavg_en;
    scale_info->ygt_en = ygt_en;
    scale_info->ygt_mode = ygt_mode;

    if (bypass_en) {
        scale_info->xsd_bypass = !xsd_en;
        scale_info->xsu_bypass = !xsu_en;
        scale_info->ys_bypass = !(ysd_en || ysu_en);
    } else {
        scale_info->xsd_bypass = 0;
        scale_info->xsu_bypass = 0;
        scale_info->ys_bypass = 0;
    }
}

void set_zme_to_vdpp_reg(const ZmeParams *zme_params, RegZme *zme)
{
    /* 0x00D0(reg20), debug settings, skip */
    /* 3. set reg::zme */
    /* 3.1 set reg::zme::common */
    enum ZME_FMT zme_format_in = FMT_YCbCr420_888; /* only support yuv420 as input */
    VdppZmeScaleInfo yrgb_scl_info = {0};
    VdppZmeScaleInfo cbcr_scl_info = {0};

    yrgb_scl_info.act_width = zme_params->src_width;
    yrgb_scl_info.act_height = zme_params->src_height;
    yrgb_scl_info.dsp_width = zme_params->dst_width;
    yrgb_scl_info.dsp_height = zme_params->dst_height;
    yrgb_scl_info.xscl_mode = SCL_MPH;
    yrgb_scl_info.yscl_mode = SCL_MPH;
    yrgb_scl_info.dering_en = zme_params->zme_dering_enable;
    calc_scale_factor(zme_params, &yrgb_scl_info, zme_params->zme_bypass_en);

    cbcr_scl_info.act_width = zme_params->src_width / 2;
    cbcr_scl_info.act_height = zme_params->src_height / 2;
    if (zme_params->dst_fmt == VDPP_FMT_YUV444) {
        if (!zme_params->yuv_out_diff) {
            cbcr_scl_info.dsp_width = zme_params->dst_width;
            cbcr_scl_info.dsp_height = zme_params->dst_height;
        } else {
            cbcr_scl_info.dsp_width = zme_params->dst_c_width;
            cbcr_scl_info.dsp_height = zme_params->dst_c_height;
        }
    } else if (zme_params->dst_fmt == VDPP_FMT_YUV420) {
        if (!zme_params->yuv_out_diff) {
            cbcr_scl_info.dsp_width = zme_params->dst_width / 2;
            cbcr_scl_info.dsp_height = zme_params->dst_height / 2;
        } else {
            /** @note: half the chroma size by user! */
            cbcr_scl_info.dsp_width = zme_params->dst_c_width;
            cbcr_scl_info.dsp_height = zme_params->dst_c_height;
        }
    } else {
        /* not supported */
        mpp_loge_f("invalid output vdpp format %d!\n", zme_params->dst_fmt);
    }
    cbcr_scl_info.xscl_mode = SCL_MPH;
    cbcr_scl_info.yscl_mode = SCL_MPH;
    cbcr_scl_info.dering_en = zme_params->zme_dering_enable;
    calc_scale_factor(zme_params, &cbcr_scl_info, zme_params->zme_bypass_en);

    /* 0x0800(reg0) */
    zme->common.reg0.bypass_en = 0;
    zme->common.reg0.align_en = 0;
    zme->common.reg0.format_in = FMT_YCbCr420_888;
    if (zme_params->dst_fmt == VDPP_FMT_YUV444)
        zme->common.reg0.format_out = FMT_YCbCr444_888;
    else
        zme->common.reg0.format_out = FMT_YCbCr420_888;
    zme->common.reg0.auto_gating_en = 1;

    /* 0x0804 ~ 0x0808(reg1 ~ reg2), skip */

    /* 0x080C(reg3), RESERVED */
    // zme->common.reg3.vir_width = zme_params->src_width;
    // zme->common.reg3.vir_height = zme_params->src_height;

    /* 0x0810(reg4) */
    zme->common.reg4.yrgb_xsd_en = yrgb_scl_info.xsd_en;
    zme->common.reg4.yrgb_xsu_en = yrgb_scl_info.xsu_en;
    zme->common.reg4.yrgb_scl_mode = yrgb_scl_info.xscl_mode;
    zme->common.reg4.yrgb_ysd_en = yrgb_scl_info.ysd_en;
    zme->common.reg4.yrgb_ysu_en = yrgb_scl_info.ysu_en;
    zme->common.reg4.yrgb_yscl_mode = yrgb_scl_info.yscl_mode;
    zme->common.reg4.yrgb_dering_en = yrgb_scl_info.dering_en;
    zme->common.reg4.yrgb_gt_en = yrgb_scl_info.ygt_en;
    zme->common.reg4.yrgb_gt_mode = yrgb_scl_info.ygt_mode;
    zme->common.reg4.yrgb_xgt_en = yrgb_scl_info.xgt_en;
    zme->common.reg4.yrgb_xgt_mode = yrgb_scl_info.xgt_mode;
    zme->common.reg4.yrgb_xsd_bypass = yrgb_scl_info.xsd_bypass;
    zme->common.reg4.yrgb_ys_bypass = yrgb_scl_info.ys_bypass;
    zme->common.reg4.yrgb_xsu_bypass = yrgb_scl_info.xsu_bypass;

    /* 0x0814(reg5), RESERVED */
    // zme->common.reg5.yrgb_src_width = yrgb_scl_info.act_width - 1;
    // zme->common.reg5.yrgb_src_height = yrgb_scl_info.act_height - 1;

    /* 0x0818(reg6), RESERVED */
    // zme->common.reg6.yrgb_dst_width = yrgb_scl_info.dsp_width - 1;
    // zme->common.reg6.yrgb_dst_height = yrgb_scl_info.dsp_height - 1;

    /* 0x081C(reg7) */
    zme->common.reg7.yrgb_dering_sen0 = zme_params->zme_dering_sen_0;
    zme->common.reg7.yrgb_dering_sen1 = zme_params->zme_dering_sen_1;
    zme->common.reg7.yrgb_dering_alpha = zme_params->zme_dering_blend_alpha;
    zme->common.reg7.yrgb_dering_delta = zme_params->zme_dering_blend_beta;

    /* 0x0820(reg8) */
    zme->common.reg8.yrgb_xscl_factor = yrgb_scl_info.xscl_factor;
    zme->common.reg8.yrgb_xscl_offset = yrgb_scl_info.xscl_offset;

    /* 0x0824(reg9) */
    zme->common.reg9.yrgb_yscl_factor = yrgb_scl_info.yscl_factor;
    zme->common.reg9.yrgb_yscl_offset = yrgb_scl_info.yscl_offset;

    /* 0x0828 ~ 0x082C(reg10 ~ reg11), skip */

    /* 0x0830(reg12) */
    zme->common.reg12.cbcr_xsd_en = cbcr_scl_info.xsd_en;
    zme->common.reg12.cbcr_xsu_en = cbcr_scl_info.xsu_en;
    zme->common.reg12.cbcr_scl_mode = cbcr_scl_info.xscl_mode;
    zme->common.reg12.cbcr_ysd_en = cbcr_scl_info.ysd_en;
    zme->common.reg12.cbcr_ysu_en = cbcr_scl_info.ysu_en;
    zme->common.reg12.cbcr_yscl_mode = cbcr_scl_info.yscl_mode;
    zme->common.reg12.cbcr_dering_en = cbcr_scl_info.dering_en;
    zme->common.reg12.cbcr_gt_en = cbcr_scl_info.ygt_en;
    zme->common.reg12.cbcr_gt_mode = cbcr_scl_info.ygt_mode;
    zme->common.reg12.cbcr_xgt_en = cbcr_scl_info.xgt_en;
    zme->common.reg12.cbcr_xgt_mode = cbcr_scl_info.xgt_mode;
    zme->common.reg12.cbcr_xsd_bypass = cbcr_scl_info.xsd_bypass;
    zme->common.reg12.cbcr_ys_bypass = cbcr_scl_info.ys_bypass;
    zme->common.reg12.cbcr_xsu_bypass = cbcr_scl_info.xsu_bypass;

    /* 0x0834(reg13), RESERVED */
    // zme->common.reg13.cbcr_src_width = cbcr_scl_info.act_width - 1;
    // zme->common.reg13.cbcr_src_height = cbcr_scl_info.act_height - 1;

    /* 0x0838(reg14), RESERVED */
    // zme->common.reg14.cbcr_dst_width = cbcr_scl_info.dsp_width - 1;
    // zme->common.reg14.cbcr_dst_height = cbcr_scl_info.dsp_height - 1;

    /* 0x083C(reg15) */
    zme->common.reg15.cbcr_dering_sen0 = zme_params->zme_dering_sen_0;
    zme->common.reg15.cbcr_dering_sen1 = zme_params->zme_dering_sen_1;
    zme->common.reg15.cbcr_dering_alpha = zme_params->zme_dering_blend_alpha;
    zme->common.reg15.cbcr_dering_delta = zme_params->zme_dering_blend_beta;

    /* 0x0840(reg16) */
    zme->common.reg16.cbcr_xscl_factor = cbcr_scl_info.xscl_factor;
    zme->common.reg16.cbcr_xscl_offset = cbcr_scl_info.xscl_offset;

    /* 0x0844(reg17) */
    zme->common.reg17.cbcr_yscl_factor = cbcr_scl_info.yscl_factor;
    zme->common.reg17.cbcr_yscl_offset = cbcr_scl_info.yscl_offset;

    /* 3.2 set reg::zme::coef */
#define VDPP_SET_ZME_COEF(index, row, col)                                                             \
    do {                                                                                               \
        zme->yrgb_hor_coe.reg##index.yrgb_hor_coe##row##_##col = yrgb_scl_info.xscl_zme_coe[row][col]; \
        zme->yrgb_ver_coe.reg##index.yrgb_ver_coe##row##_##col = yrgb_scl_info.yscl_zme_coe[row][col]; \
        zme->cbcr_hor_coe.reg##index.cbcr_hor_coe##row##_##col = cbcr_scl_info.xscl_zme_coe[row][col]; \
        zme->cbcr_ver_coe.reg##index.cbcr_ver_coe##row##_##col = cbcr_scl_info.yscl_zme_coe[row][col]; \
    } while (0)

    /* 0x0000(reg0) */
    VDPP_SET_ZME_COEF(0, 0, 0);
    VDPP_SET_ZME_COEF(0, 0, 1);

    /* 0x0004(reg1) */
    VDPP_SET_ZME_COEF(1, 0, 2);
    VDPP_SET_ZME_COEF(1, 0, 3);

    /* 0x0008(reg2) */
    VDPP_SET_ZME_COEF(2, 0, 4);
    VDPP_SET_ZME_COEF(2, 0, 5);

    /* 0x000c(reg3) */
    VDPP_SET_ZME_COEF(3, 0, 6);
    VDPP_SET_ZME_COEF(3, 0, 7);

    /* 0x0010(reg4) */
    VDPP_SET_ZME_COEF(4, 1, 0);
    VDPP_SET_ZME_COEF(4, 1, 1);

    /* 0x0014(reg5) */
    VDPP_SET_ZME_COEF(5, 1, 2);
    VDPP_SET_ZME_COEF(5, 1, 3);

    /* 0x0018(reg6) */
    VDPP_SET_ZME_COEF(6, 1, 4);
    VDPP_SET_ZME_COEF(6, 1, 5);

    /* 0x001c(reg7) */
    VDPP_SET_ZME_COEF(7, 1, 6);
    VDPP_SET_ZME_COEF(7, 1, 7);

    /* 0x0020(reg8) */
    VDPP_SET_ZME_COEF(8, 2, 0);
    VDPP_SET_ZME_COEF(8, 2, 1);

    /* 0x0024(reg9) */
    VDPP_SET_ZME_COEF(9, 2, 2);
    VDPP_SET_ZME_COEF(9, 2, 3);

    /* 0x0028(reg10) */
    VDPP_SET_ZME_COEF(10, 2, 4);
    VDPP_SET_ZME_COEF(10, 2, 5);

    /* 0x002c(reg11) */
    VDPP_SET_ZME_COEF(11, 2, 6);
    VDPP_SET_ZME_COEF(11, 2, 7);

    /* 0x0030(reg12) */
    VDPP_SET_ZME_COEF(12, 3, 0);
    VDPP_SET_ZME_COEF(12, 3, 1);

    /* 0x0034(reg13) */
    VDPP_SET_ZME_COEF(13, 3, 2);
    VDPP_SET_ZME_COEF(13, 3, 3);

    /* 0x0038(reg14) */
    VDPP_SET_ZME_COEF(14, 3, 4);
    VDPP_SET_ZME_COEF(14, 3, 5);

    /* 0x003c(reg15) */
    VDPP_SET_ZME_COEF(15, 3, 6);
    VDPP_SET_ZME_COEF(15, 3, 7);

    /* 0x0040(reg16) */
    VDPP_SET_ZME_COEF(16, 4, 0);
    VDPP_SET_ZME_COEF(16, 4, 1);

    /* 0x0044(reg17) */
    VDPP_SET_ZME_COEF(17, 4, 2);
    VDPP_SET_ZME_COEF(17, 4, 3);

    /* 0x0048(reg18) */
    VDPP_SET_ZME_COEF(18, 4, 4);
    VDPP_SET_ZME_COEF(18, 4, 5);

    /* 0x004c(reg19) */
    VDPP_SET_ZME_COEF(19, 4, 6);
    VDPP_SET_ZME_COEF(19, 4, 7);

    /* 0x0050(reg20) */
    VDPP_SET_ZME_COEF(20, 5, 0);
    VDPP_SET_ZME_COEF(20, 5, 1);

    /* 0x0054(reg21) */
    VDPP_SET_ZME_COEF(21, 5, 2);
    VDPP_SET_ZME_COEF(21, 5, 3);

    /* 0x0058(reg22) */
    VDPP_SET_ZME_COEF(22, 5, 4);
    VDPP_SET_ZME_COEF(22, 5, 5);

    /* 0x005c(reg23) */
    VDPP_SET_ZME_COEF(23, 5, 6);
    VDPP_SET_ZME_COEF(23, 5, 7);

    /* 0x0060(reg24) */
    VDPP_SET_ZME_COEF(24, 6, 0);
    VDPP_SET_ZME_COEF(24, 6, 1);

    /* 0x0064(reg25) */
    VDPP_SET_ZME_COEF(25, 6, 2);
    VDPP_SET_ZME_COEF(25, 6, 3);

    /* 0x0068(reg26) */
    VDPP_SET_ZME_COEF(26, 6, 4);
    VDPP_SET_ZME_COEF(26, 6, 5);

    /* 0x006c(reg27) */
    VDPP_SET_ZME_COEF(27, 6, 6);
    VDPP_SET_ZME_COEF(27, 6, 7);

    /* 0x0070(reg28) */
    VDPP_SET_ZME_COEF(28, 7, 0);
    VDPP_SET_ZME_COEF(28, 7, 1);

    /* 0x0074(reg29) */
    VDPP_SET_ZME_COEF(29, 7, 2);
    VDPP_SET_ZME_COEF(29, 7, 3);

    /* 0x0078(reg30) */
    VDPP_SET_ZME_COEF(30, 7, 4);
    VDPP_SET_ZME_COEF(30, 7, 5);

    /* 0x007c(reg31) */
    VDPP_SET_ZME_COEF(31, 7, 6);
    VDPP_SET_ZME_COEF(31, 7, 7);

    /* 0x0080(reg32) */
    VDPP_SET_ZME_COEF(32, 8, 0);
    VDPP_SET_ZME_COEF(32, 8, 1);

    /* 0x0084(reg33) */
    VDPP_SET_ZME_COEF(33, 8, 2);
    VDPP_SET_ZME_COEF(33, 8, 3);

    /* 0x0088(reg34) */
    VDPP_SET_ZME_COEF(34, 8, 4);
    VDPP_SET_ZME_COEF(34, 8, 5);

    /* 0x008c(reg35) */
    VDPP_SET_ZME_COEF(35, 8, 6);
    VDPP_SET_ZME_COEF(35, 8, 7);

    /* 0x0090(reg36) */
    VDPP_SET_ZME_COEF(36, 9, 0);
    VDPP_SET_ZME_COEF(36, 9, 1);

    /* 0x0094(reg37) */
    VDPP_SET_ZME_COEF(37, 9, 2);
    VDPP_SET_ZME_COEF(37, 9, 3);

    /* 0x0098(reg38) */
    VDPP_SET_ZME_COEF(38, 9, 4);
    VDPP_SET_ZME_COEF(38, 9, 5);

    /* 0x009c(reg39) */
    VDPP_SET_ZME_COEF(39, 9, 6);
    VDPP_SET_ZME_COEF(39, 9, 7);

    /* 0x00a0(reg40) */
    VDPP_SET_ZME_COEF(40, 10, 0);
    VDPP_SET_ZME_COEF(40, 10, 1);

    /* 0x00a4(reg41) */
    VDPP_SET_ZME_COEF(41, 10, 2);
    VDPP_SET_ZME_COEF(41, 10, 3);

    /* 0x00a8(reg42) */
    VDPP_SET_ZME_COEF(42, 10, 4);
    VDPP_SET_ZME_COEF(42, 10, 5);

    /* 0x00ac(reg43) */
    VDPP_SET_ZME_COEF(43, 10, 6);
    VDPP_SET_ZME_COEF(43, 10, 7);

    /* 0x00b0(reg44) */
    VDPP_SET_ZME_COEF(44, 11, 0);
    VDPP_SET_ZME_COEF(44, 11, 1);

    /* 0x00b4(reg45) */
    VDPP_SET_ZME_COEF(45, 11, 2);
    VDPP_SET_ZME_COEF(45, 11, 3);

    /* 0x00b8(reg46) */
    VDPP_SET_ZME_COEF(46, 11, 4);
    VDPP_SET_ZME_COEF(46, 11, 5);

    /* 0x00bc(reg47) */
    VDPP_SET_ZME_COEF(47, 11, 6);
    VDPP_SET_ZME_COEF(47, 11, 7);

    /* 0x00c0(reg48) */
    VDPP_SET_ZME_COEF(48, 12, 0);
    VDPP_SET_ZME_COEF(48, 12, 1);

    /* 0x00c4(reg49) */
    VDPP_SET_ZME_COEF(49, 12, 2);
    VDPP_SET_ZME_COEF(49, 12, 3);

    /* 0x00c8(reg50) */
    VDPP_SET_ZME_COEF(50, 12, 4);
    VDPP_SET_ZME_COEF(50, 12, 5);

    /* 0x00cc(reg51) */
    VDPP_SET_ZME_COEF(51, 12, 6);
    VDPP_SET_ZME_COEF(51, 12, 7);

    /* 0x00d0(reg52) */
    VDPP_SET_ZME_COEF(52, 13, 0);
    VDPP_SET_ZME_COEF(52, 13, 1);

    /* 0x00d4(reg53) */
    VDPP_SET_ZME_COEF(53, 13, 2);
    VDPP_SET_ZME_COEF(53, 13, 3);

    /* 0x00d8(reg54) */
    VDPP_SET_ZME_COEF(54, 13, 4);
    VDPP_SET_ZME_COEF(54, 13, 5);

    /* 0x00dc(reg55) */
    VDPP_SET_ZME_COEF(55, 13, 6);
    VDPP_SET_ZME_COEF(55, 13, 7);

    /* 0x00e0(reg56) */
    VDPP_SET_ZME_COEF(56, 14, 0);
    VDPP_SET_ZME_COEF(56, 14, 1);

    /* 0x00e4(reg57) */
    VDPP_SET_ZME_COEF(57, 14, 2);
    VDPP_SET_ZME_COEF(57, 14, 3);

    /* 0x00e8(reg58) */
    VDPP_SET_ZME_COEF(58, 14, 4);
    VDPP_SET_ZME_COEF(58, 14, 5);

    /* 0x00ec(reg59) */
    VDPP_SET_ZME_COEF(59, 14, 6);
    VDPP_SET_ZME_COEF(59, 14, 7);

    /* 0x00f0(reg60) */
    VDPP_SET_ZME_COEF(60, 15, 0);
    VDPP_SET_ZME_COEF(60, 15, 1);

    /* 0x00f4(reg61) */
    VDPP_SET_ZME_COEF(61, 15, 2);
    VDPP_SET_ZME_COEF(61, 15, 3);

    /* 0x00f8(reg62) */
    VDPP_SET_ZME_COEF(62, 15, 4);
    VDPP_SET_ZME_COEF(62, 15, 5);

    /* 0x00fc(reg63) */
    VDPP_SET_ZME_COEF(63, 15, 6);
    VDPP_SET_ZME_COEF(63, 15, 7);

    /* 0x0100(reg64) */
    VDPP_SET_ZME_COEF(64, 16, 0);
    VDPP_SET_ZME_COEF(64, 16, 1);

    /* 0x0104(reg65) */
    VDPP_SET_ZME_COEF(65, 16, 2);
    VDPP_SET_ZME_COEF(65, 16, 3);

    /* 0x0108(reg66) */
    VDPP_SET_ZME_COEF(66, 16, 4);
    VDPP_SET_ZME_COEF(66, 16, 5);

    /* 0x010c(reg67) */
    VDPP_SET_ZME_COEF(67, 16, 6);
    VDPP_SET_ZME_COEF(67, 16, 7);

#undef VDPP_SET_ZME_COEF
}

MPP_RET vdpp_init(VdppCtx *ictx)
{
    MPP_RET ret;
    MppReqV1 mpp_req;
    RK_U32 client_data = VDPP_CLIENT_TYPE;
    Vdpp1ApiCtx *ctx = NULL;

    if (NULL == *ictx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", *ictx);
        return MPP_ERR_NULL_PTR;
    }

    // default log level: warning
    mpp_env_get_u32(VDPP_DEBUG_ENV_NAME, &vdpp_debug,
                    (VDPP_LOG_FATAL | VDPP_LOG_ERROR | VDPP_LOG_WARNING));
    vdpp_logi("get env '%s' flag: %#x", VDPP_DEBUG_ENV_NAME, vdpp_debug);

    vdpp_enter();

    ctx = *ictx;
    ctx->fd = open("/dev/mpp_service", O_RDWR | O_CLOEXEC);
    if (ctx->fd < 0) {
        vdpp_loge("can NOT find device /dev/vdpp\n");
        return MPP_NOK;
    }

    mpp_req.cmd = MPP_CMD_INIT_CLIENT_TYPE;
    mpp_req.flag = 0;
    mpp_req.size = sizeof(client_data);
    mpp_req.data_ptr = REQ_DATA_PTR(&client_data);

    /* set default parameters */
    memset(&ctx->params, 0, sizeof(Vdpp1Params));
    vdpp_set_default_param(&ctx->params);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        vdpp_loge("ioctl set_client failed!\n");
        return MPP_NOK;
    }

    vdpp_leave();
    return MPP_OK;
}

MPP_RET vdpp_deinit(VdppCtx ictx)
{
    Vdpp1ApiCtx *ctx = NULL;

    if (NULL == ictx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ictx);
        return MPP_ERR_NULL_PTR;
    }

    ctx = ictx;

    if (ctx->fd >= 0) {
        close(ctx->fd);
        ctx->fd = -1;
    }

    return MPP_OK;
}

static MPP_RET vdpp_set_param(Vdpp1ApiCtx *ctx, VdppApiContent *param, VdppParamType type)
{
    switch (type) {
    case VDPP_PARAM_TYPE_COM: {
        ctx->params.src_yuv_swap = param->com.sswap;
        ctx->params.dst_fmt = param->com.dfmt;
        ctx->params.dst_yuv_swap = param->com.dswap;
        ctx->params.src_width = param->com.src_width;
        ctx->params.src_height = param->com.src_height;
        ctx->params.dst_width = param->com.dst_width;
        ctx->params.dst_height = param->com.dst_height;
    } break;
    case VDPP_PARAM_TYPE_DMSR: {
        memcpy(&ctx->params.dmsr_params, &param->dmsr, sizeof(struct dmsr_params));
    } break;
    case VDPP_PARAM_TYPE_ZME_COM: {
        ctx->params.zme_params.zme_bypass_en = param->zme.bypass_enable;
        ctx->params.zme_params.zme_dering_enable = param->zme.dering_enable;
        ctx->params.zme_params.zme_dering_sen_0 = param->zme.dering_sen_0;
        ctx->params.zme_params.zme_dering_sen_1 = param->zme.dering_sen_1;
        ctx->params.zme_params.zme_dering_blend_alpha = param->zme.dering_blend_alpha;
        ctx->params.zme_params.zme_dering_blend_beta = param->zme.dering_blend_beta;
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
    } break;
    case VDPP_PARAM_TYPE_ZME_COEFF: {
        if (param->zme.tap8_coeff != NULL)
            ctx->params.zme_params.zme_tap8_coeff = param->zme.tap8_coeff;
        if (param->zme.tap6_coeff != NULL)
            ctx->params.zme_params.zme_tap6_coeff = param->zme.tap6_coeff;
    } break;
    default: {
        vdpp_loge("unknown VDPP_PARAM_TYPE %#x !\n", type);
    } break;
    }

    return MPP_OK;
}

static RK_S32 check_cap(Vdpp1Params *params)
{
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;
    RK_U32 vep_mode_check = 0;

    if (NULL == params) {
        vdpp_loge("found null pointer params\n");
        return VDPP_CAP_UNSUPPORTED;
    }

    if ((params->src_height < VDPP_MODE_MIN_HEIGHT) ||
        (params->src_width < VDPP_MODE_MIN_WIDTH)) {
        vdpp_logd("vep unsupported src img_w=%d img_h=%d\n",
                  params->src_width, params->src_height);
        vep_mode_check++;
    }

    if ((params->dst_height < VDPP_MODE_MIN_HEIGHT) ||
        (params->dst_width < VDPP_MODE_MIN_WIDTH)) {
        vdpp_logd("vep unsupported dst img_w=%d img_h=%d\n",
                  params->dst_width, params->dst_height);
        vep_mode_check++;
    }

    if ((params->src_width & 1) || (params->src_height & 1) ||
        (params->dst_width & 1) || (params->dst_height & 1)) {
        vdpp_logd("vep only support img_w/h_vld 2pix align\n");
        vdpp_logd("vep unsupported img_w_i=%d img_h_i=%d img_w_o=%d img_h_o=%d\n",
                  params->src_width, params->src_height, params->dst_width, params->dst_height);
        vep_mode_check++;
    }

    if (!vep_mode_check) {
        ret_cap |= VDPP_CAP_VEP;
        vdpp_logd("vdpp support mode: VDPP_CAP_VEP\n");
    }

    return ret_cap;
}

static MPP_RET vdpp_start(Vdpp1ApiCtx *ctx)
{
    MPP_RET ret;
    VdppRegOffsetInfo reg_off[2];
    MppReqV1 mpp_req[9];
    RK_U32 req_cnt = 0;
    struct vdpp_reg *reg = NULL;
    RK_S32 ret_cap = VDPP_CAP_UNSUPPORTED;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    vdpp_enter();

    ret_cap = check_cap(&ctx->params);
    if (!(ret_cap & VDPP_CAP_VEP)) {
        vdpp_loge("found incompat work mode %s, cap %d\n",
                  working_mode_name[VDPP_WORK_MODE_VEP], ret_cap);
        return MPP_NOK;
    }

    reg = &ctx->reg;

    memset(reg_off, 0, sizeof(reg_off));
    memset(mpp_req, 0, sizeof(mpp_req));
    memset(reg, 0, sizeof(*reg));

    vdpp_params_to_reg(&ctx->params, ctx);

    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.yrgb_hor_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_YRGB_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.yrgb_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.yrgb_ver_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_YRGB_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.yrgb_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.cbcr_hor_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_CBCR_HOR_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.cbcr_hor_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.cbcr_ver_coe);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_CBCR_VER_COE;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.cbcr_ver_coe);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->zme.common);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_ZME_COMMON;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->zme.common);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->dmsr);
    mpp_req[req_cnt].offset = VDPP_REG_OFF_DMSR;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->dmsr);

    /* set reg offset */
    reg_off[0].reg_idx = 25;
    reg_off[0].offset = ctx->params.src.cbcr_offset;
    reg_off[1].reg_idx = 27;
    reg_off[1].offset = ctx->params.dst.cbcr_offset;

    vdpp_logv("set reg: cbcr_offset for src/dst=%d/%d\n", reg_off[0].offset, reg_off[1].offset);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_ADDR_OFFSET;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_REG_OFFSET_ALONE;
    mpp_req[req_cnt].size = sizeof(reg_off);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(reg_off);

    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_WRITE;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG;
    mpp_req[req_cnt].size = sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    /* read output register for common */
    req_cnt++;
    mpp_req[req_cnt].cmd = MPP_CMD_SET_REG_READ;
    mpp_req[req_cnt].flag = MPP_FLAGS_MULTI_MSG | MPP_FLAGS_LAST_MSG;
    mpp_req[req_cnt].size = sizeof(reg->common);
    mpp_req[req_cnt].offset = 0;
    mpp_req[req_cnt].data_ptr = REQ_DATA_PTR(&reg->common);

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req[0]);

    if (ret) {
        vdpp_loge("ioctl SET_REG failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
        ret = errno;
    }

    vdpp_leave();
    return ret;
}

static MPP_RET vdpp_wait(Vdpp1ApiCtx *ctx)
{
    MPP_RET ret;
    MppReqV1 mpp_req;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    memset(&mpp_req, 0, sizeof(mpp_req));
    mpp_req.cmd = MPP_CMD_POLL_HW_FINISH;
    mpp_req.flag |= MPP_FLAGS_LAST_MSG;

    ret = (RK_S32)ioctl(ctx->fd, MPP_IOC_CFG_V1, &mpp_req);
    if (ret) {
        vdpp_loge("ioctl POLL_HW_FINISH failed ret %d errno %d %s\n",
                  ret, errno, strerror(errno));
    }

    return ret;
}

static MPP_RET vdpp_done(Vdpp1ApiCtx *ctx)
{
    struct vdpp_reg *reg = NULL;

    if (NULL == ctx) {
        vdpp_loge("found NULL input vdpp ctx %p\n", ctx);
        return MPP_ERR_NULL_PTR;
    }

    vdpp_enter();

    reg = &ctx->reg;

    vdpp_logd("ro_frm_done_sts=%d\n", reg->common.reg10.ro_frm_done_sts);
    vdpp_logd("ro_osd_max_sts=%d\n", reg->common.reg10.ro_osd_max_sts);
    vdpp_logd("ro_bus_error_sts=%d\n", reg->common.reg10.ro_bus_error_sts);
    vdpp_logd("ro_timeout_sts=%d\n", reg->common.reg10.ro_timeout_sts);
    vdpp_logd("ro_config_error_sts=%d\n", reg->common.reg10.ro_config_error_sts);

    if (reg->common.reg8.sw_vdpp_frm_done_en &&
        !reg->common.reg10.ro_frm_done_sts) {
        vdpp_loge("run vdpp failed!!\n");
        return MPP_NOK;
    }

    if (reg->common.reg10.ro_timeout_sts) {
        vdpp_loge("run vdpp timeout!!\n");
        return MPP_NOK;
    }

    vdpp_logi("run vdpp success! %d\n", reg->common.reg10);
    vdpp_leave();
    return MPP_OK;
}

MPP_RET vdpp_control(VdppCtx ictx, VdppCmd cmd, void *iparam)
{
    Vdpp1ApiCtx *ctx = ictx;
    MPP_RET ret = MPP_OK;

    if ((NULL == iparam && VDPP_CMD_RUN_SYNC != cmd) ||
        (NULL == ictx)) {
        vdpp_loge("found NULL iparam %p cmd %d ctx %p\n", iparam, cmd, ictx);
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case VDPP_CMD_SET_COM_CFG:
    case VDPP_CMD_SET_DMSR_CFG:
    case VDPP_CMD_SET_ZME_COM_CFG:
    case VDPP_CMD_SET_ZME_COEFF_CFG: {
        VdppApiParams *api_param = (VdppApiParams *)iparam;

        ret = vdpp_set_param(ctx, &api_param->param, api_param->ptype);
        if (ret) {
            vdpp_loge("set vdpp parameter failed, type %d\n", api_param->ptype);
        }
    } break;
    case VDPP_CMD_SET_SRC: {
        set_addr(&ctx->params.src, (VdppImg *)iparam);
    } break;
    case VDPP_CMD_SET_DST: {
        set_addr(&ctx->params.dst, (VdppImg *)iparam);
    } break;
    case VDPP_CMD_RUN_SYNC: {
        ret = vdpp_start(ctx);
        if (ret) {
            vdpp_loge("run vdpp failed!!\n");
            return MPP_NOK;
        }

        ret |= vdpp_wait(ctx);
        ret |= vdpp_done(ctx);
    } break;
    default: {
        vdpp_loge("unsupported control cmd %#x for vdpp1!\n", cmd);
        ret = MPP_ERR_VALUE;
    } break;
    }

    return ret;
}

RK_S32 vdpp_check_cap(VdppCtx ictx)
{
    Vdpp1ApiCtx *ctx = ictx;

    if (NULL == ictx) {
        vdpp_loge("found NULL ctx %p!\n", ictx);
        return VDPP_CAP_UNSUPPORTED;
    }

    return check_cap(&ctx->params);
}
