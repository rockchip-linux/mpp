/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __IEP2_H__
#define __IEP2_H__

#include <stdint.h>

#include "rk_type.h"

#include "iep2_pd.h"
#include "iep2_ff.h"

#define TILE_W                  16
#define TILE_H                  4
#define MVL                     28
#define MVR                     27

#define TEST_DBG    printf
#define FLOOR(v, r)             (((v) / (r)) * (r))

#define RKCLIP(a, min, max)     ((a < min) ? (min) : ((a > max) ? max : a))
#define RKABS(a)                (((a) >= 0) ? (a) : -(a))
#define RKMIN(a, b)             (((a) < (b)) ? (a) : (b))
#define RKMAX(a, b)             (((a) > (b)) ? (a) : (b))

struct iep2_addr {
    uint32_t y;
    uint32_t cbcr;
    uint32_t cr;
};

struct iep2_params {
    uint32_t src_fmt;
    uint32_t src_yuv_swap;
    uint32_t dst_fmt;
    uint32_t dst_yuv_swap;
    uint32_t tile_cols;
    uint32_t tile_rows;
    uint32_t src_y_stride;
    uint32_t src_uv_stride;
    uint32_t dst_y_stride;

    struct iep2_addr src[3]; // current, next, previous
    struct iep2_addr dst[2]; // top/bottom field reconstructed frame
    uint32_t mv_addr;
    uint32_t md_addr;

    uint32_t dil_mode;
    uint32_t dil_out_mode;
    uint32_t dil_field_order;

    uint32_t md_theta;
    uint32_t md_r;
    uint32_t md_lambda;

    uint32_t dect_resi_thr;
    uint32_t osd_area_num;
    uint32_t osd_gradh_thr;
    uint32_t osd_gradv_thr;

    uint32_t osd_pos_limit_en;
    uint32_t osd_pos_limit_num;

    uint32_t osd_limit_area[2];

    uint32_t osd_line_num;
    uint32_t osd_pec_thr;

    uint32_t osd_x_sta[8];
    uint32_t osd_x_end[8];
    uint32_t osd_y_sta[8];
    uint32_t osd_y_end[8];

    uint32_t me_pena;
    uint32_t mv_bonus;
    uint32_t mv_similar_thr;
    uint32_t mv_similar_num_thr0;
    int32_t me_thr_offset;

    uint32_t mv_left_limit;
    uint32_t mv_right_limit;

    int8_t mv_tru_list[8];
    uint32_t mv_tru_vld[8];

    uint32_t eedi_thr0;

    uint32_t ble_backtoma_num;

    uint32_t comb_cnt_thr;
    uint32_t comb_feature_thr;
    uint32_t comb_t_thr;
    uint32_t comb_osd_vld[8];

    uint32_t mtn_en;
    uint32_t mtn_tab[16];

    uint32_t pd_mode;

    uint32_t roi_en;
    uint32_t roi_layer_num;
    uint32_t roi_mode[8];
    uint32_t xsta[8];
    uint32_t xend[8];
    uint32_t ysta[8];
    uint32_t yend[8];
};

struct iep2_output {
    uint32_t mv_hist[MVL + MVR + 1];
    uint32_t dect_pd_tcnt;
    uint32_t dect_pd_bcnt;
    uint32_t dect_ff_cur_tcnt;
    uint32_t dect_ff_cur_bcnt;
    uint32_t dect_ff_nxt_tcnt;
    uint32_t dect_ff_nxt_bcnt;
    uint32_t dect_ff_ble_tcnt;
    uint32_t dect_ff_ble_bcnt;
    uint32_t dect_ff_nz;
    uint32_t dect_ff_comb_f;
    uint32_t dect_osd_cnt;
    uint32_t out_comb_cnt;
    uint32_t out_osd_comb_cnt;
    uint32_t ff_gradt_tcnt;
    uint32_t ff_gradt_bcnt;
    uint32_t x_sta[8];
    uint32_t x_end[8];
    uint32_t y_sta[8];
    uint32_t y_end[8];
};

struct iep2_api_ctx {
    struct iep2_params params;
    struct iep2_output output;
    struct iep2_ff_info ff_inf;
    struct iep2_pd_info pd_inf;

    MppBufferGroup memGroup;
    MppBuffer mv_buf;
    MppBuffer md_buf;
    int fd;
};

#endif
