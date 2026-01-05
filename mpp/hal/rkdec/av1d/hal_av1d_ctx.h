/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#ifndef HAL_AV1D_CTX_H
#define HAL_AV1D_CTX_H

#include "av1d_common.h"
#include "vdpu_com.h"
#include "hal_bufs.h"

typedef struct Av1dVdpu38xBuf_t {
    RK_U32              valid;
    void                *regs;
} Av1dVdpu38xBuf;

typedef struct Vdpu38xRefInfo_t {
    RK_U32              dpb_idx;
    RK_U32              seg_idx;
    RK_U32              colmv_exist_flag;
    RK_U32              cdf_valid;
    RK_U32              coeff_idx;
    RK_U32              mi_rows;
    RK_U32              mi_cols;
    RK_U32              seg_en;
    RK_U32              seg_up_map;
    RK_U32              cdf_update_flag;
} Vdpu38xRefInfo;

typedef struct Vdpu38xAv1dRegCtx_t {
    void                *regs;
    RK_U32              offset_uncomps;

    Av1dVdpu38xBuf      reg_buf[VDPU_FAST_REG_SET_CNT];
    MppBuffer           bufs;
    RK_S32              bufs_fd;
    void                *bufs_ptr;
    RK_U32              uncmps_offset[VDPU_FAST_REG_SET_CNT];

    void                *rcb_ctx;
    VdpuRcbInfo         rcb_buf_info[RCB_BUF_CNT];
    RK_U32              rcb_buf_size;
    MppBuffer           rcb_bufs[VDPU_FAST_REG_SET_CNT];

    HalBufs             colmv_bufs;
    RK_U32              colmv_count;
    RK_U32              colmv_size;

    Vdpu38xRefInfo      ref_info_tbl[AV1_NUM_REF_FRAMES];

    MppBuffer           cdf_rd_def_base;
    HalBufs             cdf_segid_bufs;
    RK_U32              cdf_segid_count;
    RK_U32              cdf_segid_size;
    RK_U32              cdf_coeff_cdf_idxs[AV1_NUM_REF_FRAMES];

    MppBuffer           tile_info;
    MppBuffer           film_grain_mem;
    MppBuffer           global_model;
    MppBuffer           filter_mem;
    MppBuffer           tile_buf;

    AV1CDFs             *cdfs;
    Av1MvCDFs           *cdfs_ndvc;
    AV1CDFs             default_cdfs;
    Av1MvCDFs           default_cdfs_ndvc;
    AV1CDFs             cdfs_last[AV1_NUM_REF_FRAMES];
    Av1MvCDFs           cdfs_last_ndvc[AV1_NUM_REF_FRAMES];
    RK_U32              refresh_frame_flags;

    RK_U32              width;
    RK_U32              height;
    RK_S32              hor_stride;
    RK_S32              ver_stride;
    RK_U32              luma_size ;
    RK_U32              chroma_size;

    AV1FilmGrainMemory  fgsmem;

    RK_S8               prev_out_buffer_i;
    RK_U8               fbc_en;
    RK_U8               resolution_change;
    RK_U8               tile_transpose;
    RK_U32              ref_frame_sign_bias[AV1_REF_LIST_SIZE];

    RK_U32              tile_out_count;
    size_t              tile_out_size;

    RK_U32              num_tile_cols;
    /* uncompress header data */
    HalBufs             origin_bufs;
    RK_U8               header_data[0];
} Vdpu38xAv1dRegCtx;

#endif /* HAL_AV1D_CTX_H */
