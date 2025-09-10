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

#ifndef __JPEGE_SYNTAX_H__
#define __JPEGE_SYNTAX_H__

#include "mpp_frame.h"
#include "rk_venc_cmd.h"

#define MAX_NUMBER_OF_COMPONENTS 3
#define DCT_SIZE 8

typedef enum EntroyTblClass_t {
    TABLE_DC = 0,
    TABLE_AC = 1,
} EntroyTblClass;

typedef union JPEGCompInfo_t {
    struct {
        RK_U32 component_id     : 8;
        RK_U32 h_sample_factor  : 8;
        RK_U32 v_sample_factor  : 8;
        RK_U32 tbl_selector     : 8;
    };
    RK_U32 val;
} JPEGCompInfo;

typedef struct JpegeSyntax_t {
    RK_U32              width;
    RK_U32              height;
    RK_U32              hor_stride;
    RK_U32              ver_stride;
    RK_U32              mcu_hor_cnt;
    RK_U32              mcu_ver_cnt;
    RK_U32              mcu_cnt;
    RK_U32              mcu_width;
    RK_U32              mcu_height;
    MppFrameFormat      format;
    MppFrameChromaFormat format_out;
    MppFrameColorSpace  color;
    MppEncRotationCfg   rotation;
    RK_S32              mirroring;
    RK_U32              offset_x;
    RK_U32              offset_y;

    /* For quantization table */
    RK_S32              q_mode;
    RK_S32              quant;
    RK_S32              q_factor;
    RK_S32              qf_min;
    RK_S32              qf_max;
    RK_U8               *qtable_y;
    RK_U8               *qtable_u;
    RK_U8               *qtable_v;

    /*
     * For color conversion
     *
     * 0 = bt601
     * 1 = bt709
     * 2 = user defined
     */
    RK_U32              color_conversion_type;
    RK_U32              coeffA;
    RK_U32              coeffB;
    RK_U32              coeffC;
    RK_U32              coeffE;
    RK_U32              coeffF;

    /* For slice encoding mode */
    RK_U32              slice_enable;
    RK_U32              slice_size_mb_rows;

    /*
     * For unit type and density
     *
     * units_type   0 - no unit
     *              1 - dots per inch
     *              2 - dots per cm
     *
     * X/Y density  specify the pixel aspect ratio
     */
    RK_U32              units_type;
    RK_U32              density_x;
    RK_U32              density_y;

    /* For comment header */
    RK_U32              comment_length;
    RK_U8               *comment_data;

    /* For jpeg low delay slice encoding */
    RK_U32              low_delay;
    RK_U32              part_rows;
    RK_U32              restart_ri;

    RK_U32              nb_components;
    JPEGCompInfo        comp_info[MAX_NUMBER_OF_COMPONENTS];
} JpegeSyntax;

typedef struct JpegeFeedback_t {
    RK_U32 hw_status;       /* zero -> correct; non-zero -> error */
    RK_U32 stream_length;
} JpegeFeedback;

#endif
