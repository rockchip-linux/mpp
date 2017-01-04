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

typedef struct JpegeSyntax_t {
    RK_U32          width;
    RK_U32          height;
    RK_U32          hor_stride;
    RK_U32          ver_stride;
    MppFrameFormat  format;

    /* For quantization table */
    RK_U32          quality;
    RK_U8           *qtable_y;
    RK_U8           *qtable_c;

    /*
     * For color conversion
     *
     * 0 = bt601
     * 1 = bt709
     * 2 = user defined
     */
    RK_U32          color_conversion_type;
    RK_U32          coeffA;
    RK_U32          coeffB;
    RK_U32          coeffC;
    RK_U32          coeffE;
    RK_U32          coeffF;

    /* For slice encoding mode */
    RK_U32          slice_enable;
    RK_U32          slice_size_mb_rows;
    RK_U32          restart_interval;

    /*
     * For unit type and density
     *
     * units_type   0 - no unit
     *              1 - dots per inch
     *              2 - dots per cm
     *
     * X/Y density  specify the pixel aspect ratio
     */
    RK_U32          units_type;
    RK_U32          density_x;
    RK_U32          density_y;

    /* For comment header */
    RK_U32          comment_length;
    RK_U8           *comment_data;
} JpegeSyntax;

typedef struct JpegeFeedback_t {
    RK_U32 hw_status;       /* zero -> correct; non-zero -> error */
    RK_U32 stream_length;
} JpegeFeedback;

#endif
