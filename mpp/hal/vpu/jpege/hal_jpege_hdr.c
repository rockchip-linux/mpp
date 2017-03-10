/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mpp_log.h"
#include "mpp_mem.h"

#include "hal_jpege_hdr.h"

#define MAX_NUMBER_OF_COMPONENTS 3

/* JPEG markers, table B.1 */
enum {
    SOI = 0xFFD8,   /* Start of Image                    */
    DQT = 0xFFDB,   /* Define Quantization Table(s)      */
    SOF0 = 0xFFC0,  /* Start of Frame                    */
    DRI = 0xFFDD,   /* Define Restart Interval           */
    RST0 = 0xFFD0,  /* Restart marker 0                  */
    RST1 = 0xFFD1,  /* Restart marker 1                  */
    RST2 = 0xFFD2,  /* Restart marker 2                  */
    RST3 = 0xFFD3,  /* Restart marker 3                  */
    RST4 = 0xFFD4,  /* Restart marker 4                  */
    RST5 = 0xFFD5,  /* Restart marker 5                  */
    RST6 = 0xFFD6,  /* Restart marker 6                  */
    RST7 = 0xFFD7,  /* Restart marker 7                  */
    DHT = 0xFFC4,   /* Define Huffman Table(s)           */
    SOS = 0xFFDA,   /* Start of Scan                     */
    EOI = 0xFFD9,   /* End of Image                      */
    APP0 = 0xFFE0,  /* APP0 Marker                       */
    COM = 0xFFFE    /* Comment marker                    */
};

enum {
    JPEGE_NO_UNITS = 0,
    JPEGE_DOTS_PER_INCH = 1,
    JPEGE_DOTS_PER_CM = 2
};

enum {
    JPEGE_SINGLE_MARKER,
    JPEGE_MULTI_MARKER
};

static const RK_U8 zigzag[64] = {
    0, 1, 8, 16, 9, 2, 3, 10,
    17, 24, 32, 25, 18, 11, 4, 5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13, 6, 7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

/* Mjpeg quantization tables levels 0-10 */
static const RK_U8 qtable_y[11][64] = {
    {
        80, 56, 50, 80, 120, 200, 248, 248,
        60, 60, 72, 96, 136, 248, 248, 248,
        72, 68, 80, 120, 200, 248, 248, 248,
        72, 88, 112, 152, 248, 248, 248, 248,
        92, 112, 192, 248, 248, 248, 248, 248,
        120, 176, 248, 248, 248, 248, 248, 248,
        152, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248
    },

    {
        40, 28, 25, 40, 60, 100, 128, 160,
        30, 30, 36, 48, 68, 152, 152, 144,
        36, 34, 40, 60, 100, 144, 176, 144,
        36, 44, 56, 76, 128, 224, 200, 160,
        46, 56, 96, 144, 176, 248, 248, 200,
        60, 88, 144, 160, 208, 248, 248, 232,
        124, 160, 200, 224, 248, 248, 248, 248,
        184, 232, 240, 248, 248, 248, 248, 248
    },

    {
        27, 18, 17, 27, 40, 68, 88, 104,
        20, 20, 23, 32, 44, 96, 100, 92,
        23, 22, 27, 40, 68, 96, 116, 96,
        23, 28, 38, 48, 88, 144, 136, 104,
        30, 38, 62, 96, 116, 184, 176, 128,
        40, 58, 92, 108, 136, 176, 192, 160,
        84, 108, 136, 144, 176, 208, 200, 168,
        120, 160, 160, 168, 192, 168, 176, 168
    },

    {
        20, 14, 13, 20, 30, 50, 64, 76,
        15, 15, 18, 24, 34, 76, 76, 72,
        18, 16, 20, 30, 50, 72, 88, 72,
        18, 21, 28, 36, 64, 112, 100, 80,
        23, 28, 46, 72, 88, 136, 136, 96,
        30, 44, 72, 80, 104, 136, 144, 116,
        62, 80, 100, 112, 136, 152, 152, 128,
        92, 116, 120, 124, 144, 128, 136, 124
    },

    {
        16, 11, 10, 16, 24, 40, 52, 62,
        12, 12, 14, 19, 26, 58, 60, 56,
        14, 13, 16, 24, 40, 58, 72, 56,
        14, 17, 22, 29, 52, 88, 80, 62,
        18, 22, 38, 56, 68, 112, 104, 80,
        24, 36, 56, 64, 84, 104, 116, 92,
        50, 64, 80, 88, 104, 124, 120, 104,
        72, 92, 96, 100, 124, 100, 104, 100
    },

    {
        13,  9,  8, 13, 19, 32, 42, 50,
        10, 10, 11, 15, 21, 46, 48, 44,
        11, 10, 13, 19, 32, 46, 56, 46,
        11, 14, 18, 23, 42, 72, 64, 50,
        14, 18, 30, 46, 54, 88, 84, 62,
        19, 28, 44, 52, 68, 84, 92, 76,
        40, 52, 62, 72, 84, 100, 96, 84,
        58, 76, 76, 80, 100, 80, 84, 80
    },

    {
        10,  7,  6, 10, 14, 24, 31, 38,
        7,  7,  8, 11, 16, 36, 36, 34,
        8,  8, 10, 14, 24, 34, 42, 34,
        8, 10, 13, 17, 32, 52, 48, 38,
        11, 13, 22, 34, 42, 68, 62, 46,
        14, 21, 34, 38, 50, 62, 68, 56,
        29, 38, 48, 52, 62, 76, 72, 62,
        44, 56, 58, 60, 68, 60, 62, 60
    },

    {
        6,  4,  4,  6, 10, 16, 20, 24,
        5,  5,  6,  8, 10, 23, 24, 22,
        6,  5,  6, 10, 16, 23, 28, 22,
        6,  7,  9, 12, 20, 36, 32, 25,
        7,  9, 15, 22, 27, 44, 42, 31,
        10, 14, 22, 26, 32, 42, 46, 38,
        20, 26, 31, 36, 42, 48, 48, 40,
        29, 38, 38, 40, 46, 40, 42, 40
    },

    {
        3,  2,  2,  3,  5,  8, 10, 12,
        2,  2,  3,  4,  5, 12, 12, 11,
        3,  3,  3,  5,  8, 11, 14, 11,
        3,  3,  4,  6, 10, 17, 16, 12,
        4,  4,  7, 11, 14, 22, 21, 15,
        5,  7, 11, 13, 16, 21, 23, 18,
        10, 13, 16, 17, 21, 24, 24, 20,
        14, 18, 19, 20, 22, 20, 21, 20
    },

    {
        1,  1,  1,  1,  2,  3,  3,  4,
        1,  1,  1,  1,  2,  4,  4,  4,
        1,  1,  1,  2,  3,  4,  5,  4,
        1,  1,  1,  2,  3,  6,  5,  4,
        1,  1,  2,  4,  5,  7,  7,  5,
        2,  2,  4,  4,  5,  7,  8,  6,
        3,  4,  5,  6,  7,  8,  8,  7,
        5,  6,  6,  7,  7,  7,  7,  7
    },

    {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1
    }
};

static const RK_U8 qtable_c[11][64] = {
    {
        88, 92, 120, 240, 248, 248, 248, 248,
        92, 108, 136, 248, 248, 248, 248, 248,
        120, 136, 248, 248, 248, 248, 248, 248,
        240, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248
    },

    {
        44, 46, 60, 120, 248, 248, 248, 248,
        46, 54, 68, 168, 248, 248, 248, 248,
        60, 66, 144, 248, 248, 248, 248, 248,
        120, 168, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248,
        248, 248, 248, 248, 248, 248, 248, 248
    },

    {
        28, 30, 40, 80, 168, 168, 168, 168,
        30, 36, 44, 112, 168, 168, 168, 168,
        40, 44, 96, 168, 168, 168, 168, 168,
        80, 112, 168, 168, 168, 168, 168, 168,
        168, 168, 168, 168, 168, 168, 168, 168,
        168, 168, 168, 168, 168, 168, 168, 168,
        168, 168, 168, 168, 168, 168, 168, 168,
        168, 168, 168, 168, 168, 168, 168, 168
    },

    {
        21, 23, 30, 60, 124, 124, 124, 124,
        23, 26, 34, 84, 124, 124, 124, 124,
        30, 34, 72, 124, 124, 124, 124, 124,
        60, 84, 124, 124, 124, 124, 124, 124,
        124, 124, 124, 124, 124, 124, 124, 124,
        124, 124, 124, 124, 124, 124, 124, 124,
        124, 124, 124, 124, 124, 124, 124, 124,
        124, 124, 124, 124, 124, 124, 124, 124
    },

    {
        17, 18, 24, 48, 100, 100, 100, 100,
        18, 21, 26, 68, 100, 100, 100, 100,
        24, 26, 56, 100, 100, 100, 100, 100,
        48, 68, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100,
        100, 100, 100, 100, 100, 100, 100, 100
    },

    {
        14, 14, 19, 38, 80, 80, 80, 80,
        14, 17, 21, 54, 80, 80, 80, 80,
        19, 21, 46, 80, 80, 80, 80, 80,
        38, 54, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80,
        80, 80, 80, 80, 80, 80, 80, 80
    },

    {
        10, 11, 14, 28, 60, 60, 60, 60,
        11, 13, 16, 40, 60, 60, 60, 60,
        14, 16, 34, 60, 60, 60, 60, 60,
        28, 40, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60,
        60, 60, 60, 60, 60, 60, 60, 60
    },

    {
        7,  7, 10, 19, 40, 40, 40, 40,
        7,  8, 10, 26, 40, 40, 40, 40,
        10, 10, 22, 40, 40, 40, 40, 40,
        19, 26, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40,
        40, 40, 40, 40, 40, 40, 40, 40
    },

    {
        3,  4,  5,  9, 20, 20, 20, 20,
        4,  4,  5, 13, 20, 20, 20, 20,
        5,  5, 11, 20, 20, 20, 20, 20,
        9, 13, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20,
        20, 20, 20, 20, 20, 20, 20, 20
    },

    {
        1,  1,  2,  3,  7,  7,  7,  7,
        1,  1,  2,  4,  7,  7,  7,  7,
        2,  2,  4,  7,  7,  7,  7,  7,
        3,  4,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7,
        7,  7,  7,  7,  7,  7,  7,  7
    },

    {
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1
    }
};

typedef struct {
    RK_U32 Ci[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Hi[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Vi[MAX_NUMBER_OF_COMPONENTS];
    RK_U32 Tqi[MAX_NUMBER_OF_COMPONENTS];
} JpegeColorInfo;

static const JpegeColorInfo color_info[2] = {
    {
        /* YUV420 */
        {   1,  2,  3,  },
        {   2,  1,  1,  },
        {   2,  1,  1,  },
        {   0,  1,  1,  },
    },
    {
        /* YUV422 */
        {   1,  2,  3,  },
        {   2,  1,  1,  },
        {   1,  1,  1,  },
        {   0,  1,  1,  },
    },
};

typedef struct {
    RK_U32  val_y;
    RK_U32  val_c;
} JpegeHuffmanTable;

static const JpegeHuffmanTable dc_li[16] = {
    {0x00, 0x00},
    {0x01, 0x03},
    {0x05, 0x01},
    {0x01, 0x01},
    {0x01, 0x01},
    {0x01, 0x01},
    {0x01, 0x01},
    {0x01, 0x01},
    {0x01, 0x01},
    {0x00, 0x01},
    {0x00, 0x01},
    {0x00, 0x00},
    {0x00, 0x00},
    {0x00, 0x00},
    {0x00, 0x00},
    {0x00, 0x00}
};

static const JpegeHuffmanTable dc_vij[12] = {
    {0x00, 0x00},
    {0x01, 0x01},
    {0x02, 0x02},
    {0x03, 0x03},
    {0x04, 0x04},
    {0x05, 0x05},
    {0x06, 0x06},
    {0x07, 0x07},
    {0x08, 0x08},
    {0x09, 0x09},
    {0x0A, 0x0A},
    {0x0B, 0x0B},
};

static const JpegeHuffmanTable ac_li[16] = {
    {0x00, 0x00},
    {0x02, 0x02},
    {0x01, 0x01},
    {0x03, 0x02},
    {0x03, 0x04},
    {0x02, 0x04},
    {0x04, 0x03},
    {0x03, 0x04},
    {0x05, 0x07},
    {0x05, 0x05},
    {0x04, 0x04},
    {0x04, 0x04},
    {0x00, 0x00},
    {0x00, 0x01},
    {0x01, 0x02},
    {0x7D, 0x77}
};

static const JpegeHuffmanTable ac_vij[162] = {
    {0x01, 0x00},
    {0x02, 0x01},
    {0x03, 0x02},
    {0x00, 0x03},
    {0x04, 0x11},
    {0x11, 0x04},
    {0x05, 0x05},
    {0x12, 0x21},
    {0x21, 0x31},
    {0x31, 0x06},
    {0x41, 0x12},
    {0x06, 0x41},
    {0x13, 0x51},
    {0x51, 0x07},
    {0x61, 0x61},
    {0x07, 0x71},
    {0x22, 0x13},
    {0x71, 0x22},
    {0x14, 0x32},
    {0x32, 0x81},
    {0x81, 0x08},
    {0x91, 0x14},
    {0xA1, 0x42},
    {0x08, 0x91},
    {0x23, 0xA1},
    {0x42, 0xB1},
    {0xB1, 0xC1},
    {0xC1, 0x09},
    {0x15, 0x23},
    {0x52, 0x33},
    {0xD1, 0x52},
    {0xF0, 0xF0},
    {0x24, 0x15},
    {0x33, 0x62},
    {0x62, 0x72},
    {0x72, 0xD1},
    {0x82, 0x0A},
    {0x09, 0x16},
    {0x0A, 0x24},
    {0x16, 0x34},
    {0x17, 0xE1},
    {0x18, 0x25},
    {0x19, 0xF1},
    {0x1A, 0x17},
    {0x25, 0x18},
    {0x26, 0x19},
    {0x27, 0x1A},
    {0x28, 0x26},
    {0x29, 0x27},
    {0x2A, 0x28},
    {0x34, 0x29},
    {0x35, 0x2A},
    {0x36, 0x35},
    {0x37, 0x36},
    {0x38, 0x37},
    {0x39, 0x38},
    {0x3A, 0x39},
    {0x43, 0x3A},
    {0x44, 0x43},
    {0x45, 0x44},
    {0x46, 0x45},
    {0x47, 0x46},
    {0x48, 0x47},
    {0x49, 0x48},
    {0x4A, 0x49},
    {0x53, 0x4A},
    {0x54, 0x53},
    {0x55, 0x54},
    {0x56, 0x55},
    {0x57, 0x56},
    {0x58, 0x57},
    {0x59, 0x58},
    {0x5A, 0x59},
    {0x63, 0x5A},
    {0x64, 0x63},
    {0x65, 0x64},
    {0x66, 0x65},
    {0x67, 0x66},
    {0x68, 0x67},
    {0x69, 0x68},
    {0x6A, 0x69},
    {0x73, 0x6A},
    {0x74, 0x73},
    {0x75, 0x74},
    {0x76, 0x75},
    {0x77, 0x76},
    {0x78, 0x77},
    {0x79, 0x78},
    {0x7A, 0x79},
    {0x83, 0x7A},
    {0x84, 0x82},
    {0x85, 0x83},
    {0x86, 0x84},
    {0x87, 0x85},
    {0x88, 0x86},
    {0x89, 0x87},
    {0x8A, 0x88},
    {0x92, 0x89},
    {0x93, 0x8A},
    {0x94, 0x92},
    {0x95, 0x93},
    {0x96, 0x94},
    {0x97, 0x95},
    {0x98, 0x96},
    {0x99, 0x97},
    {0x9A, 0x98},
    {0xA2, 0x99},
    {0xA3, 0x9A},
    {0xA4, 0xA2},
    {0xA5, 0xA3},
    {0xA6, 0xA4},
    {0xA7, 0xA5},
    {0xA8, 0xA6},
    {0xA9, 0xA7},
    {0xAA, 0xA8},
    {0xB2, 0xA9},
    {0xB3, 0xAA},
    {0xB4, 0xB2},
    {0xB5, 0xB3},
    {0xB6, 0xB4},
    {0xB7, 0xB5},
    {0xB8, 0xB6},
    {0xB9, 0xB7},
    {0xBA, 0xB8},
    {0xC2, 0xB9},
    {0xC3, 0xBA},
    {0xC4, 0xC2},
    {0xC5, 0xC3},
    {0xC6, 0xC4},
    {0xC7, 0xC5},
    {0xC8, 0xC6},
    {0xC9, 0xC7},
    {0xCA, 0xC8},
    {0xD2, 0xC9},
    {0xD3, 0xCA},
    {0xD4, 0xD2},
    {0xD5, 0xD3},
    {0xD6, 0xD4},
    {0xD7, 0xD5},
    {0xD8, 0xD6},
    {0xD9, 0xD7},
    {0xDA, 0xD8},
    {0xE1, 0xD9},
    {0xE2, 0xDA},
    {0xE3, 0xE2},
    {0xE4, 0xE3},
    {0xE5, 0xE4},
    {0xE6, 0xE5},
    {0xE7, 0xE6},
    {0xE8, 0xE7},
    {0xE9, 0xE8},
    {0xEA, 0xE9},
    {0xF1, 0xEA},
    {0xF2, 0xF2},
    {0xF3, 0xF3},
    {0xF4, 0xF4},
    {0xF5, 0xF5},
    {0xF6, 0xF6},
    {0xF7, 0xF7},
    {0xF8, 0xF8},
    {0xF9, 0xF9},
    {0xFA, 0xFA}
};

typedef struct {
    RK_U8 *buffer;          /* Pointer to first byte of stream */
    RK_U8 *stream;          /* Pointer to next byte of stream */
    RK_U32 size;            /* Byte size of stream buffer */
    RK_U32 byteCnt;         /* Byte counter */
    RK_U32 bitCnt;          /* Bit counter */
    RK_U32 byteBuffer;      /* Byte buffer */
    RK_U32 bufferedBits;    /* Amount of bits in byte buffer, [0-7] */
} JpegeBitsImpl;

void jpege_bits_init(JpegeBits *ctx)
{
    JpegeBitsImpl *impl = mpp_malloc(JpegeBitsImpl, 1);
    *ctx = impl;
}

void jpege_bits_deinit(JpegeBits ctx)
{
    if (ctx)
        mpp_free(ctx);
}

void jpege_bits_setup(JpegeBits ctx, RK_U8 *buf, RK_S32 size)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;

    impl->buffer = buf;
    impl->stream = buf;
    impl->size = size;
    impl->byteCnt = 0;
    impl->bitCnt = 0;
    impl->byteBuffer = 0;
    impl->bufferedBits = 0;
    buf[0] = 0;
    buf[1] = 0;
}

void jpege_bits_put(JpegeBits ctx, RK_U32 value, RK_S32 number)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;

    RK_U32 bits;
    RK_U32 byteBuffer = impl->byteBuffer;
    RK_U8 *stream = impl->stream;

    /* Debug: value is too big */
    mpp_assert(value < ((RK_U32) 1 << number));
    mpp_assert(number < 25);

    bits = number + impl->bufferedBits;

    value <<= (32 - bits);
    byteBuffer = (((RK_U32) stream[0]) << 24) | value;

    while (bits > 7) {
        *stream = (RK_U8) (byteBuffer >> 24);
        bits -= 8;
        byteBuffer <<= 8;
        stream++;
        impl->byteCnt++;
    }

    stream[0] = (RK_U8) (byteBuffer >> 24);
    impl->stream = stream;
    impl->bitCnt += number;
    impl->byteBuffer = byteBuffer;
    impl->bufferedBits = (RK_U8) bits;
}

void jpege_bits_align_byte(JpegeBits ctx)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;

    if (impl->bitCnt & 7)
        jpege_bits_put(ctx, 0, 8 - (impl->bitCnt & 7));
}

RK_U8 *jpege_bits_get_buf(JpegeBits ctx)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;
    return impl->buffer;
}

RK_S32  jpege_bits_get_bitpos(JpegeBits ctx)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;
    return impl->bitCnt;
}

RK_S32 jpege_bits_get_bytepos(JpegeBits ctx)
{
    JpegeBitsImpl *impl = (JpegeBitsImpl *)ctx;
    return impl->byteCnt;
}

static void write_jpeg_app0_header(JpegeBits *bits, JpegeSyntax *syntax)
{
    /* APP0 */
    jpege_bits_put(bits, APP0, 16);

    /* Length */
    jpege_bits_put(bits, 0x0010, 16);

    /* "JFIF" ID */
    /* Ident1 */
    jpege_bits_put(bits, 0x4A46, 16);
    /* Ident2 */
    jpege_bits_put(bits, 0x4946, 16);
    /* Ident3 */
    jpege_bits_put(bits, 0x00, 8);
    /* Version */
    jpege_bits_put(bits, 0x0102, 16);

    if (syntax->density_x && syntax->density_y) {
        /* Units */
        jpege_bits_put(bits, syntax->units_type, 8);
        /* Xdensity */
        jpege_bits_put(bits, syntax->density_x, 16);
        /* Ydensity */
        jpege_bits_put(bits, syntax->density_y, 16);
    } else {
        /* Units */
        jpege_bits_put(bits, 0, 8);
        /* Xdensity */
        jpege_bits_put(bits, 1, 16);
        /* Ydensity */
        jpege_bits_put(bits, 1, 16);
    }

    /* XThumbnail */
    jpege_bits_put(bits, 0x00, 8);
    /* YThumbnail */
    jpege_bits_put(bits, 0x00, 8);

    /* Do NOT write thumbnail */
}

static void write_jpeg_comment_header(JpegeBits *bits, JpegeSyntax *syntax)
{
    RK_U32 i;
    RK_U8 *data = syntax->comment_data;
    RK_U32 length = syntax->comment_length;

    /* COM */
    jpege_bits_put(bits, COM, 16);

    /* Lc */
    jpege_bits_put(bits, 2 + length, 16);

    for (i = 0; i < length; i++) {
        /* COM data */
        jpege_bits_put(bits, data[i], 8);
    }
}

static void write_jpeg_dqt_header(JpegeBits *bits, const RK_U8 *qtables[2])
{
    RK_S32 i;
    const RK_U8 *qtable = qtables[0];

    /* DQT */
    jpege_bits_put(bits, DQT, 16);
    /* Lq */
    jpege_bits_put(bits, 2 + 65, 16);
    /* Pq */
    jpege_bits_put(bits, 0, 4);
    /* Tq */
    jpege_bits_put(bits, 0, 4);

    for (i = 0; i < 64; i++) {
        /* Qk table 0 */
        jpege_bits_put(bits, qtable[zigzag[i]], 8);
    }

    /* DQT */
    jpege_bits_put(bits, DQT, 16);
    /* Lq */
    jpege_bits_put(bits, 2 + 65, 16);
    /* Pq */
    jpege_bits_put(bits, 0, 4);
    /* Tq */
    jpege_bits_put(bits, 1, 4);

    qtable = qtables[1];

    for (i = 0; i < 64; i++) {
        /* Qk table 1 */
        jpege_bits_put(bits, qtable[zigzag[i]], 8);
    }
}

static void write_jpeg_SOFO_header(JpegeBits *bits, JpegeSyntax *syntax)
{
    RK_S32 i;
    RK_U32 width = syntax->width;
    RK_U32 height = syntax->height;
    const JpegeColorInfo *info = &color_info[0];

    /* SOF0 */
    jpege_bits_put(bits, SOF0, 16);

    /* Lf */
    jpege_bits_put(bits, (8 + 3 * 3), 16);
    /* P */
    jpege_bits_put(bits, 8, 8);
    /* Y */
    jpege_bits_put(bits, height, 16);
    /* X */
    jpege_bits_put(bits, width, 16);
    /* Nf */
    jpege_bits_put(bits, 3, 8);

    /* NOTE: only output 420 bits */
    for (i = 0; i < 3; i++) {
        /* Ci */
        jpege_bits_put(bits, info->Ci[i], 8);
        /* Hi */
        jpege_bits_put(bits, info->Hi[i], 4);
        /* Vi */
        jpege_bits_put(bits, info->Vi[i], 4);
        /* Tqi */
        jpege_bits_put(bits, info->Tqi[i], 8);
    }
}

static void write_jpeg_dht_header(JpegeBits *bits)
{
    RK_S32 i;

    /* DHT  */
    jpege_bits_put(bits, DHT, 16);

    /* Huffman table for luminance DC components */
    /* Lh  */
    jpege_bits_put(bits, 2 + ((17 * 1) + ((1 * 12))), 16);
    /* TC */
    jpege_bits_put(bits, 0, 4);
    /* TH */
    jpege_bits_put(bits, 0, 4);

    for (i = 0; i < 16; i++) {
        /* Dc_Li */
        jpege_bits_put(bits, dc_li[i].val_y, 8);
    }

    for (i = 0; i < 12; i++) {
        /* Dc_Vij */
        jpege_bits_put(bits, dc_vij[i].val_y, 8);
    }

    /* DHT  */
    jpege_bits_put(bits, DHT, 16);

    /* Huffman table for luminance AC components */
    /* Lh */
    jpege_bits_put(bits, 2 + ((17 * 1) + ((1 * 162))), 16);
    /* TC */
    jpege_bits_put(bits, 1, 4);
    /* TH */
    jpege_bits_put(bits, 0, 4);

    for (i = 0; i < 16; i++) {
        /* Ac_Li */
        jpege_bits_put(bits, ac_li[i].val_y, 8);
    }

    for (i = 0; i < 162; i++) {
        /* Ac_Vij */
        jpege_bits_put(bits, ac_vij[i].val_y, 8);
    }

    /* Huffman table for chrominance DC components */
    /* DHT  */
    jpege_bits_put(bits, DHT, 16);
    /* Lh */
    jpege_bits_put(bits, 2 + ((17 * 1) + ((1 * 12))), 16);
    /* TC */
    jpege_bits_put(bits, 0, 4);
    /* TH */
    jpege_bits_put(bits, 1, 4);

    for (i = 0; i < 16; i++) {
        /* Dc_Li */
        jpege_bits_put(bits, dc_li[i].val_c, 8);
    }

    for (i = 0; i < 12; i++) {
        /* Dc_Vij */
        jpege_bits_put(bits, dc_vij[i].val_c, 8);
    }

    /* Huffman table for chrominance AC components */
    /* DHT  */
    jpege_bits_put(bits, DHT, 16);
    /* Lh */
    jpege_bits_put(bits, 2 + ((17 * 1) + ((1 * 162))), 16);
    /* TC */
    jpege_bits_put(bits, 1, 4);
    /* TH */
    jpege_bits_put(bits, 1, 4);

    for (i = 0; i < 16; i++) {
        /* Ac_Li */
        jpege_bits_put(bits, ac_li[i].val_c, 8);
    }

    for (i = 0; i < 162; i++) {
        /* Ac_Vij */
        jpege_bits_put(bits, ac_vij[i].val_c, 8);
    }
}

static void write_jpeg_sos_header(JpegeBits *bits)
{
    RK_U32 i;
    RK_U32 Ns = 3;
    RK_U32 Ls = (6 + (2 * Ns));

    /* SOS  */
    jpege_bits_put(bits, SOS, 16);
    /* Ls  */
    jpege_bits_put(bits, Ls, 16);
    /* Ns  */
    jpege_bits_put(bits, Ns, 8);

    for (i = 0; i < Ns; i++) {
        /* Csj */
        jpege_bits_put(bits, i + 1, 8);

        if (i == 0) {
            /* Tdj */
            jpege_bits_put(bits, 0, 4);
            /* Taj */
            jpege_bits_put(bits, 0, 4);
        } else {
            /* Tdj */
            jpege_bits_put(bits, 1, 4);
            /* Taj */
            jpege_bits_put(bits, 1, 4);
        }
    }

    /* Ss */
    jpege_bits_put(bits, 0, 8);
    /* Se */
    jpege_bits_put(bits, 63, 8);
    /* Ah */
    jpege_bits_put(bits, 0, 4);
    /* Al */
    jpege_bits_put(bits, 0, 4);
}

MPP_RET write_jpeg_header(JpegeBits *bits, JpegeSyntax *syntax, const RK_U8 *qtables[2])
{
    /* SOI */
    jpege_bits_put(bits, SOI, 16);

    /* APP0 header */
    write_jpeg_app0_header(bits, syntax);

    /* Com header */
    if (syntax->comment_length)
        write_jpeg_comment_header(bits, syntax);

    /* Quant header */
    if (syntax->qtable_y)
        qtables[0] = syntax->qtable_y;
    else
        qtables[0] = qtable_y[syntax->quality];

    if (syntax->qtable_c)
        qtables[1] = syntax->qtable_c;
    else
        qtables[1] = qtable_c[syntax->quality];

    write_jpeg_dqt_header(bits, qtables);

    /* Frame header */
    write_jpeg_SOFO_header(bits, syntax);

    /* Do NOT have Restart interval */

    /* Huffman header */
    write_jpeg_dht_header(bits);

    /* Scan header */
    write_jpeg_sos_header(bits);

    jpege_bits_align_byte(bits);
    return MPP_OK;
}
