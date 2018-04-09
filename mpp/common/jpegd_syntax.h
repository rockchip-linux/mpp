/*
 *
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

#ifndef __JPEGD_SYNTAX__
#define __JPEGD_SYNTAX__

#define JPEGDEC_YUV400                    (0)
#define JPEGDEC_YUV420                    (2)
#define JPEGDEC_YUV422                    (3)
#define JPEGDEC_YUV444                    (4)
#define JPEGDEC_YUV440                    (5)
#define JPEGDEC_YUV411                    (6)

#define JPEGD_STREAM_BUFF_SIZE            (512*1024)
#define MAX_COMPONENTS                    (3)       /* for JFIF: YCbCr */
#define DRI_MARKER_LENGTH                 (4)       /* must be 4 bytes */
#define QUANTIZE_TABLE_LENGTH             (64)
#define MAX_AC_HUFFMAN_TABLE_LENGTH       (162)     /* for baseline */
#define MAX_DC_HUFFMAN_TABLE_LENGTH       (12)      /* for baseline */
#define MAX_HUFFMAN_CODE_BIT_LENGTH       (16)      /* The longest code word is 16 bits */
#define MIN_WIDTH                         (48)      /* 48 Bytes */
#define MIN_HEIGHT                        (48)      /* 48 Bytes */
#define MAX_WIDTH                         (8*1024)  /* 4K Bytes */
#define MAX_HEIGHT                        (8*1024)  /* 4K Bytes */
#define MAX_STREAM_LENGTH                 (MAX_WIDTH * MAX_HEIGHT) /* 16M Bytes */
#define ZERO_PADDING_LENGTH               (4)       /* 4 Bytes */
#define JPEGD_BASELINE_TABLE_SIZE         (QUANTIZE_TABLE_LENGTH * 3 \
                                           + MAX_AC_HUFFMAN_TABLE_LENGTH * 2 \
                                           + MAX_DC_HUFFMAN_TABLE_LENGTH * 2 \
                                           + ZERO_PADDING_LENGTH) /* 544 Bytes */

#define JPEGD_DBG_FUNCTION                (0x00000001)
#define JPEGD_DBG_STARTCODE               (0x00000002)
#define JPEGD_DBG_TABLE                   (0x00000004)
#define JPEGD_DBG_RESULT                  (0x00000008)
#define JPEGD_DBG_IO                      (0x00000010) /* input and output write file */
#define JPEGD_DBG_PARSER_INFO             (0x00000020) /* parser information */
#define JPEGD_DBG_SYNTAX_ERR              (0x00000040) /* syntax error */
#define JPEGD_DBG_HAL_INFO                (0x00000080) /* hal information */

extern RK_U32 jpegd_debug;

#define jpegd_dbg(flag, fmt, ...)         _mpp_dbg(jpegd_debug, flag, fmt, ## __VA_ARGS__)
#define jpegd_dbg_f(flag, fmt, ...)       _mpp_dbg_f(jpegd_debug, flag, fmt, ## __VA_ARGS__)

#define jpegd_dbg_func(fmt, ...)          jpegd_dbg_f(JPEGD_DBG_FUNCTION, fmt, ## __VA_ARGS__)
#define jpegd_dbg_marker(fmt, ...)        jpegd_dbg(JPEGD_DBG_STARTCODE, fmt, ## __VA_ARGS__)
#define jpegd_dbg_table(fmt, ...)         jpegd_dbg(JPEGD_DBG_TABLE, fmt, ## __VA_ARGS__)
#define jpegd_dbg_result(fmt, ...)        jpegd_dbg(JPEGD_DBG_RESULT, fmt, ## __VA_ARGS__)
#define jpegd_dbg_io(fmt, ...)            jpegd_dbg(JPEGD_DBG_IO, fmt, ## __VA_ARGS__)
#define jpegd_dbg_parser(fmt, ...)        jpegd_dbg(JPEGD_DBG_PARSER_INFO, fmt, ## __VA_ARGS__)
#define jpegd_dbg_syntax(fmt, ...)        jpegd_dbg(JPEGD_DBG_SYNTAX_ERR, fmt, ## __VA_ARGS__)
#define jpegd_dbg_hal(fmt, ...)           jpegd_dbg(JPEGD_DBG_HAL_INFO, fmt, ## __VA_ARGS__)


enum huffman_table_type {
    HUFFMAN_TABLE_TYPE_DC = 0,
    HUFFMAN_TABLE_TYPE_AC = 1,
    HUFFMAN_TABLE_TYPE_BUTT = 2,
};

enum huffman_table_id {
    HUFFMAN_TABLE_ID_ZERO = 0,
    HUFFMAN_TABLE_ID_ONE = 1,
    HUFFMAN_TABLE_ID_TWO = 2,
    HUFFMAN_TABLE_ID_THREE = 3,
    HUFFMAN_TABLE_ID_BUTT = 4,
};

enum quantize_table_id {
    QUANTIZE_TABLE_ID_ZERO = 0,
    QUANTIZE_TABLE_ID_ONE = 1,
    QUANTIZE_TABLE_ID_TWO = 2,
    QUANTIZE_TABLE_ID_THREE = 3,
    QUANTIZE_TABLE_ID_BUTT = 4,
};

/* Alternating Current Table */
typedef struct {
    RK_U32         bits[MAX_HUFFMAN_CODE_BIT_LENGTH];
    RK_U32         vals[MAX_AC_HUFFMAN_TABLE_LENGTH];
    RK_U32         actual_length; /* calculate based on actual stream */
} AcTable;

/* Direct Current Table */
typedef struct {
    RK_U32         bits[MAX_HUFFMAN_CODE_BIT_LENGTH];
    RK_U32         vals[MAX_DC_HUFFMAN_TABLE_LENGTH];
    RK_U32         actual_length; /* calculate based on actual stream */
} DcTable;

typedef struct JpegdSyntax {
    /* just 16 bits because quantization parameters are much less than 2^16 */
    RK_U16         quant_matrixes[4][QUANTIZE_TABLE_LENGTH];

    /* only two ac table for baseline */
    AcTable        ac_table[2];

    /* only two dc table for baseline */
    DcTable        dc_table[2];

    /* quantizer scale calculated from quant_matrixes */
    RK_U32         qscale[4];

    /* output format */
    MppFrameFormat output_fmt;

    /* especially for jpeg */
    RK_U32         yuv_mode;
    RK_U8          fill_bottom;
    RK_U8          fill_right;

    /* syntax in SOS */
    RK_U8          scan_start;
    RK_U8          scan_end;
    RK_U8          prev_shift; /* Ah */
    RK_U8          point_transform; /* Al */

    /* 0 - not found; 1 - found */
    RK_U8          dht_found;
    RK_U8          eoi_found;

    /* amount of quantize tables: 1 or 3 */
    RK_U8          qtable_cnt;

    /* length of sos segment */
    RK_U32         sos_len;

    /* decoded bytes by software */
    RK_U32         strm_offset;

    /* hardware decode start address */
    RK_U8          *cur_pos;

    /* length of a jpeg packet */
    RK_U32         pkt_len;

    /* Horizontal pixel density */
    RK_U16         hor_density;

    /* Vertical pixel density */
    RK_U16         ver_density;

    RK_U32         width, height;
    RK_U32         hor_stride, ver_stride;

    /* Number of components in frame */
    RK_U32         nb_components;

    /* Component id */
    RK_U32         component_id[MAX_COMPONENTS];

    /* Horizontal sampling factor */
    RK_U32         h_count[MAX_COMPONENTS];

    /* Vertical sampling factor */
    RK_U32         v_count[MAX_COMPONENTS];

    /* Huffman Table ID used by DC */
    RK_U32         dc_index[MAX_COMPONENTS];

    /* Huffman Table ID used by AC */
    RK_U32         ac_index[MAX_COMPONENTS];

    /* maximum h and v counts */
    RK_U32         h_max, v_max;

    /* quant table index for each component */
    RK_U32         quant_index[MAX_COMPONENTS];

    RK_U32         restart_interval;
} JpegdSyntax;

#endif /*__JPEGD_SYNTAX__*/
