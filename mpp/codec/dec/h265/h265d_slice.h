/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd.
 */

#ifndef H265D_SLICE_H
#define H265D_SLICE_H

#include "h265d_pps.h"
#include "h265d_sps.h"

typedef struct H265dSlice_t {
    RK_U32 pps_id;

    RK_U32 slice_segment_addr;

    SliceType slice_type;

    RK_S32 pic_order_cnt_lsb;

    RK_U32 first_slice_in_pic_flag;
    RK_U32 dependent_slice_segment_flag;

    RK_S32 short_term_ref_pic_set_sps_flag;
    RK_S32 short_term_ref_pic_set_size;
    H265dStRps st_rps_slice;
    const H265dStRps *st_rps_using;
    H265dLtRps lt_rps;

    RK_S32 slice_qp;
} H265dSlice;

#endif /* H265D_SLICE_H */
