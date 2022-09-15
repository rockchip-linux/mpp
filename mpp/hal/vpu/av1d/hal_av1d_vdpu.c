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

#define MODULE_TAG "hal_av1d_vdpu"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_bitput.h"
#include "mpp_hal.h"
#include "mpp_dec_cb_param.h"
#include "mpp_device.h"
#include "hal_bufs.h"

#include "hal_av1d_vdpu_reg.h"
#include "hal_av1d_common.h"
#include "av1d_syntax.h"
#include "film_grain_noise_table.h"
#include "av1d_common.h"

#define VDPU_FAST_REG_SET_CNT    3
#define AV1_MAX_TILES 128
#define AV1_TILE_INFO_SIZE AV1_MAX_TILES * 16
#define GM_GLOBAL_MODELS_PER_FRAME 7
#define GLOBAL_MODEL_TOTAL_SIZE (6 * 4 + 4 * 2)
#define GLOBAL_MODEL_SIZE GM_GLOBAL_MODELS_PER_FRAME * GLOBAL_MODEL_TOTAL_SIZE
#define MaxTiles 128

#define DUMP_AV1_DATAS 0

typedef enum AV1D_FILT_TYPE_E {
    DB_DATA_COL,
    DB_CTRL_COL,
    CDEF_COL,
    SR_COL,
    LR_COL,
    RFC_COL,
    FILT_TYPE_BUT,
} Av1dFiltType_e;

typedef struct filt_info_t {
    RK_U32 size;
    RK_U32 offset;
} filtInfo;

typedef struct av1d_rkv_buf_t {
    RK_U32              valid;
    VdpuAv1dRegSet  *regs;
} av1dVdpuBuf;

typedef struct VdpuAv1dRegCtx_t {
    av1dVdpuBuf     reg_buf[VDPU_FAST_REG_SET_CNT];
    MppBuffer       prob_tbl_base;
    MppBuffer       prob_tbl_out_base;
    MppBuffer       tile_info;
    MppBuffer       film_grain_mem;
    MppBuffer       global_model;
    MppBuffer       filter_mem;
    MppBuffer       buf_tmp;
    MppBuffer       tile_buf;
    filtInfo        filt_info[FILT_TYPE_BUT];

    AV1CDFs         *cdfs;
    MvCDFs          *cdfs_ndvc;
    AV1CDFs         default_cdfs;
    MvCDFs          default_cdfs_ndvc;
    AV1CDFs         cdfs_last[NUM_REF_FRAMES];
    MvCDFs          cdfs_last_ndvc[NUM_REF_FRAMES];
    RK_U32          refresh_frame_flags;

    RK_S32          width;
    RK_S32          height;
    RK_S32          hor_stride;
    RK_S32          ver_stride;
    RK_U32          luma_size ;
    RK_U32          chroma_size;

    FilmGrainMemory  fgsmem;

    RK_S8            prev_out_buffer_i;
    RK_U8            fbc_en;
    RK_U8            resolution_change;
    RK_U8            tile_transpose;
    RK_U32           ref_frame_sign_bias[AV1_REF_LIST_SIZE];

    VdpuAv1dRegSet  *regs;
    HalBufs          tile_out_bufs;
    RK_U32           tile_out_count;
    size_t           tile_out_size;
} VdpuAv1dRegCtx;

static RK_U32 rkv_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 8);
}

static RK_U32 rkv_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 8);
}

static RK_U32 rkv_len_align(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 128));
}

static RK_U32 rkv_len_align_422(RK_U32 val)
{
    return ((5 * MPP_ALIGN(val, 64)) / 2);
}

static MPP_RET hal_av1d_alloc_res(void *hal)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;
    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(VdpuAv1dRegCtx)));
    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(VdpuAv1dRegSet, 1);
        memset(reg_ctx->reg_buf[i].regs, 0, sizeof(VdpuAv1dRegSet));
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
    }

    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->prob_tbl_base, MPP_ALIGN(sizeof(AV1CDFs), 2048)));
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->prob_tbl_out_base, MPP_ALIGN(sizeof(AV1CDFs), 2048)));
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->tile_info, AV1_TILE_INFO_SIZE));
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->film_grain_mem, MPP_ALIGN(sizeof(AV1FilmGrainMemory), 2048)));
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->global_model, MPP_ALIGN(GLOBAL_MODEL_SIZE, 2048)));
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->tile_buf, MPP_ALIGN(32 * MaxTiles, 4096)));

__RETURN:
    return ret;
__FAILED:
    return ret;
}

static void vdpu_av1d_filtermem_release(VdpuAv1dRegCtx *ctx)
{
    BUF_PUT(ctx->filter_mem);
}

static MPP_RET vdpu_av1d_filtermem_alloc(Av1dHalCtx *p_hal, VdpuAv1dRegCtx *ctx, DXVA_PicParams_AV1 *dxva)
{
    RK_U32 size = 0;
    RK_U32 pic_height = MPP_ALIGN(dxva->height, 64);
    RK_U32 height_in_sb = pic_height / 64;
    RK_U32 stripe_num = ((pic_height + 8) + 63) / 64;
    RK_U32 max_bit_depth = 10;
    RK_U32 num_tile_cols = 1 << dxva->tile_cols_log2;//dxva->tiles.cols;
    filtInfo *filt_info = ctx->filt_info;

    /* db tile col data buffer */
    // asic_buff->db_data_col_offset = 0;
    // asic_buff->db_data_col_tsize = NEXT_MULTIPLE(pic_height * 12 * max_bit_depth / 8, 128);
    // size = asic_buff->db_data_col_tsize * num_tile_cols;
    // asic_buff->db_ctrl_col_offset = size;

    filt_info[DB_DATA_COL].offset = 0;
    filt_info[DB_DATA_COL].size = MPP_ALIGN(pic_height * 12 * max_bit_depth / 8, 128);
    size += filt_info[DB_DATA_COL].size * num_tile_cols;


    /* db tile col ctrl buffer */
    filt_info[DB_CTRL_COL].offset = size;
    filt_info[DB_CTRL_COL].size = MPP_ALIGN(pic_height * 2 * 16 / 4, 128);
    size += filt_info[DB_CTRL_COL].size * num_tile_cols;

    // size += asic_buff->db_ctrl_col_tsize * num_tile_cols;
    // asic_buff->cdef_col_offset = size;

    /* cdef tile col buffer */
    filt_info[CDEF_COL].offset = size;
    filt_info[CDEF_COL].size = MPP_ALIGN(height_in_sb * 44 * max_bit_depth * 16 / 8, 128);
    size += filt_info[CDEF_COL].size * num_tile_cols;
    // asic_buff->cdef_col_tsize = NEXT_MULTIPLE(height_in_sb * 44 * max_bit_depth * 16 / 8, 128);
    // size += asic_buff->cdef_col_tsize * num_tile_cols;
    // asic_buff->sr_col_offset = size;

    /* sr tile col buffer */
    filt_info[SR_COL].offset = size;
    filt_info[SR_COL].size = MPP_ALIGN(height_in_sb * (3040 + 1280), 128);
    size += filt_info[SR_COL].size * num_tile_cols;
    // asic_buff->sr_col_tsize = NEXT_MULTIPLE(height_in_sb * (3040 + 1280), 128);
    // size += asic_buff->sr_col_tsize * num_tile_cols;
    // asic_buff->lr_col_offset = size;

    /* lr tile col buffer */
    filt_info[LR_COL].offset = size;
    filt_info[LR_COL].size = MPP_ALIGN(stripe_num * 1536 * max_bit_depth / 8, 128);
    size += filt_info[LR_COL].size * num_tile_cols;
    // asic_buff->lr_col_tsize = NEXT_MULTIPLE(stripe_num * 1536 * max_bit_depth / 8, 128);
    // size += asic_buff->lr_col_tsize * num_tile_cols;
    // if (dec_cont->use_multicore) {
    //     asic_buff->rfc_col_offset = size;
    //     asic_buff->rfc_col_size = NEXT_MULTIPLE(asic_buff->height, 8) / 8 * 16 * 2;
    //     size += asic_buff->rfc_col_size * num_tile_cols;
    // }
    if (!mpp_buffer_get(p_hal->buf_group, &ctx->filter_mem, MPP_ALIGN(size, SZ_4K)))
        return MPP_NOK;
    return MPP_OK;
}

static void hal_av1d_release_res(void *hal)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    RK_U32 i = 0;
    RK_U32 loop = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    for (i = 0; i < loop; i++)
        MPP_FREE(reg_ctx->reg_buf[i].regs);

    BUF_PUT(reg_ctx->prob_tbl_base);
    BUF_PUT(reg_ctx->prob_tbl_out_base);
    BUF_PUT(reg_ctx->tile_info);
    BUF_PUT(reg_ctx->film_grain_mem);
    BUF_PUT(reg_ctx->global_model);
    BUF_PUT(reg_ctx->buf_tmp);
    BUF_PUT(reg_ctx->tile_buf);
    vdpu_av1d_filtermem_release(reg_ctx);
    hal_bufs_deinit(reg_ctx->tile_out_bufs);

    MPP_FREE(p_hal->reg_ctx);
}

MPP_RET vdpu_av1d_deinit(void *hal)
{
    hal_av1d_release_res(hal);

    return MPP_OK;
}

MPP_RET vdpu_av1d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    INP_CHECK(ret, NULL == p_hal);

    FUN_CHECK(hal_av1d_alloc_res(hal));

    {
        VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;

        reg_ctx->cdfs = &reg_ctx->default_cdfs;
        reg_ctx->cdfs_ndvc = &reg_ctx->default_cdfs_ndvc;
        reg_ctx->tile_transpose = 1;
    }

    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, rkv_hor_align);
    mpp_slots_set_prop(p_hal->slots, SLOTS_VER_ALIGN, rkv_ver_align);
    mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align);

    (void)cfg;
__RETURN:
    return MPP_OK;
__FAILED:
    vdpu_av1d_deinit(hal);

    return ret;
}

static void set_ref_width(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg33.sw_ref0_width = val;
    } else if (i == 1) {
        regs->swreg34.sw_ref1_width = val;
    } else if (i == 2) {
        regs->swreg35.sw_ref2_width = val;
    } else if (i == 3) {
        regs->swreg43.sw_ref3_width = val;
    } else if (i == 4) {
        regs->swreg44.sw_ref4_width = val;
    } else if (i == 5) {
        regs->swreg45.sw_ref5_width = val;
    } else if (i == 6) {
        regs->swreg46.sw_ref6_width = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_height(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg33.sw_ref0_height = val;
    } else if (i == 1) {
        regs->swreg34.sw_ref1_height = val;
    } else if (i == 2) {
        regs->swreg35.sw_ref2_height = val;
    } else if (i == 3) {
        regs->swreg43.sw_ref3_height = val;
    } else if (i == 4) {
        regs->swreg44.sw_ref4_height = val;
    } else if (i == 5) {
        regs->swreg45.sw_ref5_height = val;
    } else if (i == 6) {
        regs->swreg46.sw_ref6_height = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_hor_scale(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg36.sw_ref0_hor_scale = val;
    } else if (i == 1) {
        regs->swreg37.sw_ref1_hor_scale = val;
    } else if (i == 2) {
        regs->swreg38.sw_ref2_hor_scale = val;
    } else if (i == 3) {
        regs->swreg39.sw_ref3_hor_scale = val;
    } else if (i == 4) {
        regs->swreg40.sw_ref4_hor_scale = val;
    } else if (i == 5) {
        regs->swreg41.sw_ref5_hor_scale = val;
    } else if (i == 6) {
        regs->swreg42.sw_ref6_hor_scale = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_ver_scale(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg36.sw_ref0_ver_scale = val;
    } else if (i == 1) {
        regs->swreg37.sw_ref1_ver_scale = val;
    } else if (i == 2) {
        regs->swreg38.sw_ref2_ver_scale = val;
    } else if (i == 3) {
        regs->swreg39.sw_ref3_ver_scale = val;
    } else if (i == 4) {
        regs->swreg40.sw_ref4_ver_scale = val;
    } else if (i == 5) {
        regs->swreg41.sw_ref5_ver_scale = val;
    } else if (i == 6) {
        regs->swreg42.sw_ref6_ver_scale = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_lum_base(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val, HalBufs bufs)
{
    HalBuf *tile_out_buf;
    tile_out_buf = hal_bufs_get_buf(bufs, val);
    // MppBuffer framebuf = NULL;
    // mpp_buf_slot_get_prop(slots, val, SLOT_BUFFER, &framebuf);
    if (tile_out_buf == NULL) {
        mpp_err_f("get slots frame buff fail");
    }
    val =  mpp_buffer_get_fd(tile_out_buf->buf[0]);
    if (i == 0) {
        regs->addr_cfg.swreg67.sw_refer0_ybase_lsb = val;
    } else if (i == 1) {
        regs->addr_cfg.swreg69.sw_refer1_ybase_lsb = val;
    } else if (i == 2) {
        regs->addr_cfg.swreg71.sw_refer2_ybase_lsb = val;
    } else if (i == 3) {
        regs->addr_cfg.swreg73.sw_refer3_ybase_lsb = val;
    } else if (i == 4) {
        regs->addr_cfg.swreg75.sw_refer4_ybase_lsb = val;
    } else if (i == 5) {
        regs->addr_cfg.swreg77.sw_refer5_ybase_lsb = val;
    } else if (i == 6) {
        regs->addr_cfg.swreg79.sw_refer6_ybase_lsb = val;
    } else {
        mpp_err( "Error: trying to set invalid reference index.");
    }
}

static void set_ref_lum_base_msb(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->addr_cfg.swreg66.sw_refer0_ybase_msb = val;
    } else if (i == 1) {
        regs->addr_cfg.swreg68.sw_refer1_ybase_msb = val;
    } else if (i == 2) {
        regs->addr_cfg.swreg70.sw_refer2_ybase_msb = val;
    } else if (i == 3) {
        regs->addr_cfg.swreg72.sw_refer3_ybase_msb = val;
    } else if (i == 4) {
        regs->addr_cfg.swreg74.sw_refer4_ybase_msb = val;
    } else if (i == 5) {
        regs->addr_cfg.swreg76.sw_refer5_ybase_msb = val;
    } else if (i == 6) {
        regs->addr_cfg.swreg78.sw_refer6_ybase_msb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_cb_base(Av1dHalCtx *p_hal, RK_S32 i, RK_S32 val, HalBufs bufs, RK_U32 offset)
{
    VdpuAv1dRegCtx *ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    HalBuf *tile_out_buf;
    tile_out_buf = hal_bufs_get_buf(bufs, val);
    // MppBuffer framebuf = NULL;

    // mpp_buf_slot_get_prop(slots, val, SLOT_BUFFER, &framebuf);
    if (tile_out_buf == NULL) {
        mpp_err_f("get slots frame buff fail");
    }
    val =  mpp_buffer_get_fd(tile_out_buf->buf[0]);

    if (i == 0) {
        mpp_dev_set_reg_offset(p_hal->dev, 101, offset);
        regs->addr_cfg.swreg101.sw_refer0_cbase_lsb  = val;
    } else if (i == 1) {
        mpp_dev_set_reg_offset(p_hal->dev, 103, offset);
        regs->addr_cfg.swreg103.sw_refer1_cbase_lsb  = val;
    } else if (i == 2) {
        mpp_dev_set_reg_offset(p_hal->dev, 105, offset);
        regs->addr_cfg.swreg105.sw_refer2_cbase_lsb  = val;
    } else if (i == 3) {
        mpp_dev_set_reg_offset(p_hal->dev, 107, offset);
        regs->addr_cfg.swreg107.sw_refer3_cbase_lsb  = val;
    } else if (i == 4) {
        mpp_dev_set_reg_offset(p_hal->dev, 109, offset);
        regs->addr_cfg.swreg109.sw_refer4_cbase_lsb  = val;
    } else if (i == 5) {
        mpp_dev_set_reg_offset(p_hal->dev, 111, offset);
        regs->addr_cfg.swreg111.sw_refer5_cbase_lsb  = val;
    } else if (i == 6) {
        mpp_dev_set_reg_offset(p_hal->dev, 113, offset);
        regs->addr_cfg.swreg113.sw_refer6_cbase_lsb  = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_cb_base_msb(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->addr_cfg.swreg100.sw_refer0_cbase_msb = val;
    } else if (i == 1) {
        regs->addr_cfg.swreg102.sw_refer1_cbase_msb = val;
    } else if (i == 2) {
        regs->addr_cfg.swreg104.sw_refer2_cbase_msb = val;
    } else if (i == 3) {
        regs->addr_cfg.swreg106.sw_refer3_cbase_msb = val;
    } else if (i == 4) {
        regs->addr_cfg.swreg108.sw_refer4_cbase_msb = val;
    } else if (i == 5) {
        regs->addr_cfg.swreg110.sw_refer5_cbase_msb = val;
    } else if (i == 6) {
        regs->addr_cfg.swreg112.sw_refer6_cbase_msb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}


static void set_ref_dbase(Av1dHalCtx *p_hal, RK_S32 i, RK_S32 val,  HalBufs bufs, RK_U32 offset)
{
    VdpuAv1dRegCtx *ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    HalBuf *tile_out_buf;
    tile_out_buf = hal_bufs_get_buf(bufs, val);
    // MppBuffer framebuf = NULL;

    // mpp_buf_slot_get_prop(slots, val, SLOT_BUFFER, &framebuf);
    if (tile_out_buf == NULL) {
        mpp_err_f("get slots frame buff fail");
    }
    val =  mpp_buffer_get_fd(tile_out_buf->buf[0]);
    if (i == 0) {
        mpp_dev_set_reg_offset(p_hal->dev, 135, offset);
        regs->addr_cfg.swreg135.sw_refer0_dbase_lsb = val;
    } else if (i == 1) {
        mpp_dev_set_reg_offset(p_hal->dev, 137, offset);
        regs->addr_cfg.swreg137.sw_refer1_dbase_lsb = val;
    } else if (i == 2) {
        mpp_dev_set_reg_offset(p_hal->dev, 139, offset);
        regs->addr_cfg.swreg139.sw_refer2_dbase_lsb = val;
    } else if (i == 3) {
        mpp_dev_set_reg_offset(p_hal->dev, 141, offset);
        regs->addr_cfg.swreg141.sw_refer3_dbase_lsb = val;
    } else if (i == 4) {
        mpp_dev_set_reg_offset(p_hal->dev, 143, offset);
        regs->addr_cfg.swreg143.sw_refer4_dbase_lsb = val;
    } else if (i == 5) {
        mpp_dev_set_reg_offset(p_hal->dev, 145, offset);
        regs->addr_cfg.swreg145.sw_refer5_dbase_lsb = val;
    } else if (i == 6) {
        mpp_dev_set_reg_offset(p_hal->dev, 147, offset);
        regs->addr_cfg.swreg147.sw_refer6_dbase_lsb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_dbase_msb(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->addr_cfg.swreg134.sw_refer0_dbase_msb = val;
    } else if (i == 1) {
        regs->addr_cfg.swreg136.sw_refer1_dbase_msb = val;
    } else if (i == 2) {
        regs->addr_cfg.swreg138.sw_refer2_dbase_msb = val;
    } else if (i == 3) {
        regs->addr_cfg.swreg140.sw_refer3_dbase_msb = val;
    } else if (i == 4) {
        regs->addr_cfg.swreg142.sw_refer4_dbase_msb = val;
    } else if (i == 5) {
        regs->addr_cfg.swreg144.sw_refer5_dbase_msb = val;
    } else if (i == 6) {
        regs->addr_cfg.swreg146.sw_refer6_dbase_msb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_ty_base(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val, HalBufs bufs)
{
    // MppBuffer framebuf = NULL;
    // mpp_buf_slot_get_prop(slots, val, SLOT_BUFFER, &framebuf);
    HalBuf *tile_out_buf;
    tile_out_buf = hal_bufs_get_buf(bufs, val);

    if (tile_out_buf == NULL) {
        mpp_err_f("get slots frame buff fail");
    }
    val =  mpp_buffer_get_fd(tile_out_buf->buf[0]);

    if (i == 0) {
        regs->swreg192.sw_refer0_tybase_lsb = val;
    } else if (i == 1) {
        regs->swreg194.sw_refer1_tybase_lsb = val;
    } else if (i == 2) {
        regs->swreg196.sw_refer2_tybase_lsb = val;
    } else if (i == 3) {
        regs->swreg198.sw_refer3_tybase_lsb = val;
    } else if (i == 4) {
        regs->swreg200.sw_refer4_tybase_lsb = val;
    } else if (i == 5) {
        regs->swreg202.sw_refer5_tybase_lsb = val;
    } else if (i == 6) {
        regs->swreg204.sw_refer6_tybase_lsb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_ty_base_msb(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg191.sw_refer0_tybase_msb = val;
    } else if (i == 1) {
        regs->swreg193.sw_refer1_tybase_msb = val;
    } else if (i == 2) {
        regs->swreg195.sw_refer2_tybase_msb = val;
    } else if (i == 3) {
        regs->swreg197.sw_refer3_tybase_msb = val;
    } else if (i == 4) {
        regs->swreg199.sw_refer4_tybase_msb = val;
    } else if (i == 5) {
        regs->swreg201.sw_refer5_tybase_msb = val;
    } else if (i == 6) {
        regs->swreg203.sw_refer6_tybase_msb = val;
    } else {
        mpp_err(" trying to set invalid reference index.");
    }
}

static void set_ref_tc_base(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val, HalBufs bufs)
{
    // MppBuffer framebuf = NULL;
    // mpp_buf_slot_get_prop(slots, val, SLOT_BUFFER, &framebuf);
    HalBuf *tile_out_buf;
    tile_out_buf = hal_bufs_get_buf(bufs, val);

    if (tile_out_buf == NULL) {
        mpp_err_f("get slots frame buff fail");
    }
    val =  mpp_buffer_get_fd(tile_out_buf->buf[0]);

    if (i == 0) {
        regs->swreg226.sw_refer0_tcbase_lsb = val;
    } else if (i == 1) {
        regs->swreg228.sw_refer1_tcbase_lsb = val;
    } else if (i == 2) {
        regs->swreg230.sw_refer2_tcbase_lsb = val;
    } else if (i == 3) {
        regs->swreg232.sw_refer3_tcbase_lsb = val;
    } else if (i == 4) {
        regs->swreg234.sw_refer4_tcbase_lsb = val;
    } else if (i == 5) {
        regs->swreg236.sw_refer5_tcbase_lsb = val;
    } else if (i == 6) {
        regs->swreg238.sw_refer6_tcbase_lsb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}


static void set_ref_tc_base_msb(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg225.sw_refer0_tcbase_msb = val;
    } else if (i == 1) {
        regs->swreg227.sw_refer1_tcbase_msb = val;
    } else if (i == 2) {
        regs->swreg229.sw_refer2_tcbase_msb = val;
    } else if (i == 3) {
        regs->swreg231.sw_refer3_tcbase_msb = val;
    } else if (i == 4) {
        regs->swreg233.sw_refer4_tcbase_msb = val;
    } else if (i == 5) {
        regs->swreg235.sw_refer5_tcbase_msb = val;
    } else if (i == 6) {
        regs->swreg237.sw_refer6_tcbase_msb = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

static void set_ref_sign_bias(VdpuAv1dRegSet *regs, RK_S32 i, RK_S32 val)
{
    if (i == 0) {
        regs->swreg59.sw_ref0_sign_bias = val;
    } else if (i == 1) {
        regs->swreg59.sw_ref1_sign_bias = val;
    } else if (i == 2) {
        regs->swreg59.sw_ref2_sign_bias = val;
    } else if (i == 3) {
        regs->swreg59.sw_ref3_sign_bias = val;
    } else if (i == 4) {
        regs->swreg9.sw_ref4_sign_bias = val;
    } else if (i == 5) {
        regs->swreg9.sw_ref5_sign_bias = val;
    } else if (i == 6) {
        regs->swreg9.sw_ref6_sign_bias = val;
    } else {
        mpp_err("Error: trying to set invalid reference index.");
    }
}

#define MAX_FRAME_DISTANCE 31
#define MAX_ACTIVE_REFS AV1_ACTIVE_REFS_EX

RK_S32 GetRelativeDist(DXVA_PicParams_AV1 *dxva, RK_S32 a, RK_S32 b)
{
    if (!dxva->order_hint_bits) return 0;
    const RK_S32 bits = dxva->order_hint_bits - 1;

    RK_S32 diff = a - b;
    RK_S32 m = 1 << bits;
    diff = (diff & (m - 1)) - (diff & m);
    return diff;
}

#define POPULATE_REF_OFFSET(index)                                               \
 {                                                                               \
    RK_S32 ref_offset[MAX_REF_FRAMES_EX - 1];                                    \
    RK_S32 idx = refs_selected[(index) - 1];                                     \
    ref_offset[0] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].lst_frame_offset);     \
    ref_offset[1] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].lst2_frame_offset);    \
    ref_offset[2] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].lst3_frame_offset);    \
    ref_offset[3] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].gld_frame_offset);     \
    ref_offset[4] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].bwd_frame_offset);     \
    ref_offset[5] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].alt2_frame_offset);    \
    ref_offset[6] = GetRelativeDist(dxva, dxva->frame_refs[idx].order_hint,      \
                                    dxva->frame_refs[idx].alt_frame_offset);     \
    if(index == 1) {                                                             \
        regs->swreg20.sw_mf1_last_offset = ref_offset[0];                        \
        regs->swreg21.sw_mf1_last2_offset = ref_offset[1];                       \
        regs->swreg22.sw_mf1_last3_offset = ref_offset[2];                       \
        regs->swreg23.sw_mf1_golden_offset = ref_offset[3];                      \
        regs->swreg24.sw_mf1_bwdref_offset = ref_offset[4];                      \
        regs->swreg25.sw_mf1_altref2_offset = ref_offset[5];                     \
        regs->swreg26.sw_mf1_altref_offset = ref_offset[6];                      \
    }else if(index == 2) {                                                       \
        regs->swreg27.sw_mf2_last_offset = ref_offset[0];                        \
        regs->swreg47.sw_mf2_last2_offset = ref_offset[1];                       \
        regs->swreg47.sw_mf2_last3_offset = ref_offset[2];                       \
        regs->swreg47.sw_mf2_golden_offset = ref_offset[3];                      \
        regs->swreg48.sw_mf2_bwdref_offset = ref_offset[4];                      \
        regs->swreg48.sw_mf2_altref2_offset = ref_offset[5];                     \
        regs->swreg48.sw_mf2_altref_offset = ref_offset[6];                      \
    }else {                                                                      \
        regs->swreg184.sw_mf3_last_offset = ref_offset[0];                       \
        regs->swreg185.sw_mf3_last2_offset = ref_offset[1];                      \
        regs->swreg186.sw_mf3_last3_offset = ref_offset[2];                      \
        regs->swreg187.sw_mf3_golden_offset = ref_offset[3];                     \
        regs->swreg188.sw_mf3_bwdref_offset = ref_offset[4];                     \
        regs->swreg257.sw_mf3_altref2_offset = ref_offset[5];                    \
        regs->swreg262.sw_mf3_altref_offset = ref_offset[6];                     \
    }                                                                            \
}


void set_frame_sign_bias(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    RK_U32 i = 0;
    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    if (!dxva->order_hint_bits || dxva->format.frame_type == AV1_FRAME_INTRA_ONLY ||
        dxva->format.frame_type == AV1_FRAME_KEY) {
        for (i = 0; i < AV1_REF_LIST_SIZE; i++) {
            reg_ctx->ref_frame_sign_bias[i] = 0;
        }
        return;
    }

    // Identify the nearest forward and backward references.
    for (i = 0; i < AV1_ACTIVE_REFS_EX; i++) {
        if (dxva->frame_refs[i].Index >= 0) {
            RK_S32 ref_frame_offset = dxva->frame_refs[i].order_hint;
            RK_S32 rel_off = GetRelativeDist(dxva, ref_frame_offset, dxva->order_hint);
            reg_ctx->ref_frame_sign_bias[i + 1] = (rel_off <= 0) ? 0 : 1;
        }
    }
}

void vdpu_av1d_set_prob(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    const int mv_cdf_offset = offsetof(AV1CDFs, mv_cdf);
    void* prob_base = mpp_buffer_get_ptr(reg_ctx->prob_tbl_base);
    VdpuAv1dRegSet *regs = reg_ctx->regs;

    memcpy(prob_base, dxva->cdfs, sizeof(AV1CDFs));
    if (dxva->format.frame_type == AV1_FRAME_INTRA_ONLY ||
        dxva->format.frame_type == AV1_FRAME_KEY) {
        // Overwrite MV context area with intrabc MV context
        memcpy(prob_base + mv_cdf_offset, dxva->cdfs_ndvc, sizeof(MvCDFs));
    }

    regs->addr_cfg.swreg171.sw_prob_tab_out_base_lsb    = mpp_buffer_get_fd(reg_ctx->prob_tbl_out_base);
    regs->addr_cfg.swreg173.sw_prob_tab_base_lsb        = mpp_buffer_get_fd(reg_ctx->prob_tbl_base);
}

void vdpu_av1d_set_reference_frames(Av1dHalCtx *p_hal, VdpuAv1dRegCtx *ctx, DXVA_PicParams_AV1 *dxva)
{
    RK_U32 tmp1, tmp2, i;
    RK_U32 cur_height, cur_width;
    RK_U8  max_ref_frames = MAX_REF_FRAMES_EX;
    RK_U8 prev_valid = 0;

    VdpuAv1dRegSet *regs = ctx->regs;
    RK_S32 ref_count[AV1DEC_MAX_PIC_BUFFERS] = {0};

    RK_U32 ref_scale_e = 0;
    RK_U32 y_stride = ctx->luma_size;
    RK_U32 uv_stride = y_stride / 2;
    RK_U32 mv_offset = ctx->luma_size + ctx->chroma_size + 64;

    if (!dxva->coding.intrabc) {
        for (i = 0; i < AV1_REF_LIST_SIZE - 1; i++) {
            if (dxva->frame_refs[i].Index >= 0)
                ref_count[dxva->frame_refs[i].Index]++;
        }

        for (i = 0; i < AV1DEC_MAX_PIC_BUFFERS; i++) {
            if (ref_count[i])
                regs->swreg4.sw_ref_frames++;
        }
    } else
        regs->swreg4.sw_ref_frames = 1;

    cur_width = dxva->width;
    cur_height = dxva->height;

    set_frame_sign_bias(p_hal, dxva);

    for (i = LAST_FRAME; i < max_ref_frames; i++) {
        RK_U32 ref = i - 1;
        RK_S32 idx = 0;
        if (dxva->coding.intrabc) {
            idx = dxva->CurrPicTextureIndex;
            tmp1 = cur_width;
            tmp2 = cur_height;
        } else {
            tmp1 =  dxva->frame_refs[ref].width;
            tmp2 =  dxva->frame_refs[ref].height;
            if (dxva->frame_refs[ref].Index > 0) {
                idx = dxva->frame_refs[ref].Index;
            }
        }

        set_ref_width(regs, ref, tmp1);
        set_ref_height(regs, ref, tmp2);
        tmp1 = ((tmp1 << AV1_REF_SCALE_SHIFT) + cur_width / 2) / cur_width;
        tmp2 = ((tmp2 << AV1_REF_SCALE_SHIFT) + cur_height / 2) / cur_height;

        set_ref_hor_scale(regs, ref, tmp1);
        set_ref_ver_scale(regs, ref, tmp2);
        if (tmp1 != (1 << AV1_REF_SCALE_SHIFT) ||
            tmp2 != (1 << AV1_REF_SCALE_SHIFT)) {
            ref_scale_e = 1;
        }

        if (idx == ctx->prev_out_buffer_i) {
            prev_valid = 1;
        }

        set_ref_lum_base(regs, ref, idx, ctx->tile_out_bufs);
        set_ref_cb_base(p_hal,  ref, idx, ctx->tile_out_bufs, y_stride);
        set_ref_dbase  (p_hal,  ref, idx, ctx->tile_out_bufs, mv_offset);

        set_ref_lum_base_msb(regs, ref, 0);
        set_ref_cb_base_msb(regs,  ref, 0);
        set_ref_dbase_msb  (regs,  ref, 0);

        if (0) {
            set_ref_ty_base(regs,  ref, idx, ctx->tile_out_bufs);
            set_ref_tc_base(regs,  ref, idx, ctx->tile_out_bufs);
            set_ref_ty_base_msb(regs,  ref, 0);
            set_ref_tc_base_msb(regs,  ref, 0);
        }
        set_ref_sign_bias(regs, ref, ctx->ref_frame_sign_bias[i]);
    }

    regs->swreg184.sw_ref0_gm_mode                 = dxva->frame_refs[0].wmtype;
    regs->swreg185.sw_ref1_gm_mode                 = dxva->frame_refs[1].wmtype;
    regs->swreg186.sw_ref2_gm_mode                 = dxva->frame_refs[2].wmtype;
    regs->swreg187.sw_ref3_gm_mode                 = dxva->frame_refs[3].wmtype;
    regs->swreg188.sw_ref4_gm_mode                 = dxva->frame_refs[4].wmtype;
    regs->swreg257.sw_ref5_gm_mode                 = dxva->frame_refs[5].wmtype;
    regs->swreg262.sw_ref6_gm_mode                 = dxva->frame_refs[6].wmtype;


    if (dxva->coding.intrabc) {
        ctx->prev_out_buffer_i = dxva->CurrPicTextureIndex;
    } else if (!prev_valid) {
        ctx->prev_out_buffer_i = dxva->frame_refs[0].Index;  // LAST
    }

    {
        RK_S32 gld_buf_idx = GOLDEN_FRAME_EX - LAST_FRAME;
        RK_S32 alt_buf_idx = ALTREF_FRAME_EX - LAST_FRAME;
        RK_S32 lst_buf_idx = LAST_FRAME - LAST_FRAME;
        RK_S32 bwd_buf_idx = BWDREF_FRAME_EX - LAST_FRAME;
        RK_S32 alt2_buf_idx = ALTREF2_FRAME_EX - LAST_FRAME;
        RK_S32 lst2_buf_idx = LAST2_FRAME_EX - LAST_FRAME;

        RK_S32 cur_frame_offset = dxva->order_hint;
        RK_S32 alt_frame_offset = 0;
        RK_S32 gld_frame_offset = 0;
        RK_S32 bwd_frame_offset = 0;
        RK_S32 alt2_frame_offset = 0;
        RK_S32 refs_selected[3] = {0, 0, 0};
        RK_S32 cur_mi_cols = (dxva->width + 7) >> 3;
        RK_S32 cur_mi_rows = (dxva->height + 7) >> 3;
        RK_U8 mf_types[3] = {0, 0, 0};
        RK_S32 ref_stamp = 2;
        RK_S32 ref_ind = 0;
        RK_S32 rf;

        if (dxva->frame_refs[alt_buf_idx].Index >= 0)
            alt_frame_offset = dxva->frame_refs[alt_buf_idx].order_hint;
        if (dxva->frame_refs[gld_buf_idx].Index >= 0)
            gld_frame_offset = dxva->frame_refs[gld_buf_idx].order_hint;
        if (dxva->frame_refs[bwd_buf_idx].Index >= 0)
            bwd_frame_offset = dxva->frame_refs[bwd_buf_idx].order_hint;
        if (dxva->frame_refs[alt2_buf_idx].Index >= 0)
            alt2_frame_offset = dxva->frame_refs[alt2_buf_idx].order_hint;

        AV1D_DBG(AV1D_DBG_LOG, "frame_offset[%d %d %d %d] lst_idx %d alt_off %d\n",
                 alt_frame_offset,
                 gld_frame_offset,
                 bwd_frame_offset,
                 alt2_frame_offset,
                 dxva->frame_refs[lst_buf_idx].Index,
                 dxva->frame_refs[lst_buf_idx].alt_frame_offset);

        if (dxva->frame_refs[lst_buf_idx].Index >= 0) {
            const RK_S32 alt_frame_offset_in_lst =
                dxva->frame_refs[lst_buf_idx].alt_frame_offset;

            const RK_S32 is_lst_overlay = (alt_frame_offset_in_lst == gld_frame_offset);
            if (!is_lst_overlay) {
                RK_S32 lst_mi_cols =
                    (dxva->frame_refs[lst_buf_idx].width + 7) >> 3;
                RK_S32 lst_mi_rows =
                    (dxva->frame_refs[lst_buf_idx].height + 7) >> 3;
                // TODO(stan): what's the difference btw key_frame and intra_only?
                RK_S32 lst_intra_only =
                    dxva->frame_refs[lst_buf_idx].intra_only ||
                    dxva->frame_refs[lst_buf_idx].is_intra_frame;
                if (lst_mi_cols == cur_mi_cols && lst_mi_rows == cur_mi_rows &&
                    !lst_intra_only) {
                    mf_types[ref_ind] = LAST_FRAME;
                    refs_selected[ref_ind++] = lst_buf_idx;
                }
            }
            ref_stamp--;
        }

        if (GetRelativeDist(dxva, bwd_frame_offset, cur_frame_offset) > 0) {
            RK_S32 bwd_mi_cols =
                (dxva->frame_refs[bwd_buf_idx].width + 7) >> 3;
            RK_S32 bwd_mi_rows =
                (dxva->frame_refs[bwd_buf_idx].height + 7) >> 3;
            RK_S32 bwd_intra_only = dxva->frame_refs[bwd_buf_idx].intra_only ||
                                    dxva->frame_refs[bwd_buf_idx].is_intra_frame;
            if (bwd_mi_cols == cur_mi_cols && bwd_mi_rows == cur_mi_rows &&
                !bwd_intra_only) {
                mf_types[ref_ind] = BWDREF_FRAME_EX;
                refs_selected[ref_ind++] = bwd_buf_idx;
                ref_stamp--;
            }
        }

        if (GetRelativeDist(dxva, alt2_frame_offset, cur_frame_offset) > 0) {
            RK_S32 alt2_mi_cols =
                (dxva->frame_refs[alt2_buf_idx].width + 7) >> 3;
            RK_S32 alt2_mi_rows =
                (dxva->frame_refs[alt2_buf_idx].height + 7) >> 3;
            RK_S32 alt2_intra_only =
                dxva->frame_refs[alt2_buf_idx].intra_only ||
                dxva->frame_refs[alt2_buf_idx].is_intra_frame;
            if (alt2_mi_cols == cur_mi_cols && alt2_mi_rows == cur_mi_rows &&
                !alt2_intra_only) {
                mf_types[ref_ind] = ALTREF2_FRAME_EX;
                refs_selected[ref_ind++] = alt2_buf_idx;
                ref_stamp--;
            }
        }

        if (GetRelativeDist(dxva, alt_frame_offset, cur_frame_offset) > 0 &&
            ref_stamp >= 0) {
            RK_S32 alt_mi_cols =
                (dxva->frame_refs[alt_buf_idx].width + 7) >> 3;
            RK_S32 alt_mi_rows =
                (dxva->frame_refs[alt_buf_idx].height + 7) >> 3;
            RK_S32 alt_intra_only = dxva->frame_refs[alt_buf_idx].intra_only ||
                                    dxva->frame_refs[alt_buf_idx].is_intra_frame;
            if (alt_mi_cols == cur_mi_cols && alt_mi_rows == cur_mi_rows &&
                !alt_intra_only) {
                mf_types[ref_ind] = ALTREF_FRAME_EX;
                refs_selected[ref_ind++] = alt_buf_idx;
                ref_stamp--;
            }
        }

        if (ref_stamp >= 0 && dxva->frame_refs[lst2_buf_idx].Index >= 0) {
            RK_S32 lst2_mi_cols =
                (dxva->frame_refs[lst2_buf_idx].width + 7) >> 3;
            RK_S32 lst2_mi_rows =
                (dxva->frame_refs[lst2_buf_idx].height + 7) >> 3;
            RK_S32 lst2_intra_only =
                dxva->frame_refs[lst2_buf_idx].intra_only ||
                dxva->frame_refs[lst2_buf_idx].is_intra_frame;
            if (lst2_mi_cols == cur_mi_cols && lst2_mi_rows == cur_mi_rows &&
                !lst2_intra_only) {
                mf_types[ref_ind] = LAST2_FRAME_EX;
                refs_selected[ref_ind++] = lst2_buf_idx;
                ref_stamp--;
            }
        }

        RK_S32 cur_offset[MAX_REF_FRAMES_EX - 1];
        RK_S32 cur_roffset[MAX_REF_FRAMES_EX - 1];
        for ( rf = 0; rf < MAX_REF_FRAMES_EX - 1; ++rf) {
            RK_S32 buf_idx = dxva->frame_refs[rf].Index;
            if (buf_idx >= 0) {
                cur_offset[rf] =
                    GetRelativeDist(dxva, cur_frame_offset,
                                    dxva->frame_refs[rf].order_hint);
                cur_roffset[rf] =
                    GetRelativeDist(dxva, dxva->frame_refs[rf].order_hint,
                                    cur_frame_offset);
                AV1D_DBG(AV1D_DBG_LOG, "buf_idx[%d]=%d offset[%d : %d] hin %d\n", rf, buf_idx, cur_offset[rf], cur_roffset[rf], dxva->frame_refs[rf].order_hint);
            } else {
                cur_offset[rf] = 0;
                cur_roffset[rf] = 0;
            }
        }

        regs->swreg11.sw_use_temporal0_mvs = 0;
        regs->swreg11.sw_use_temporal1_mvs = 0;
        regs->swreg11.sw_use_temporal2_mvs = 0;
        regs->swreg11.sw_use_temporal3_mvs = 0;

        if (dxva->coding.use_ref_frame_mvs && ref_ind > 0 &&
            cur_offset[mf_types[0] - LAST_FRAME] <= MAX_FRAME_DISTANCE &&
            cur_offset[mf_types[0] - LAST_FRAME] >= -MAX_FRAME_DISTANCE) {
            regs->swreg11.sw_use_temporal0_mvs = 1;
            POPULATE_REF_OFFSET(1)
        }

        if (dxva->coding.use_ref_frame_mvs && ref_ind > 1 &&
            cur_offset[mf_types[1] - LAST_FRAME] <= MAX_FRAME_DISTANCE &&
            cur_offset[mf_types[1] - LAST_FRAME] >= -MAX_FRAME_DISTANCE) {
            regs->swreg11.sw_use_temporal1_mvs = 1;
            POPULATE_REF_OFFSET(2)
        }

        if (dxva->coding.use_ref_frame_mvs && ref_ind > 2 &&
            cur_offset[mf_types[2] - LAST_FRAME] <= MAX_FRAME_DISTANCE &&
            cur_offset[mf_types[2] - LAST_FRAME] >= -MAX_FRAME_DISTANCE) {
            regs->swreg11.sw_use_temporal2_mvs = 1;
            POPULATE_REF_OFFSET(3)
        }

        // Pass one additional frame that will contain the segment information
        if (dxva->segmentation.enabled &&
            dxva->primary_ref_frame < ALLOWED_REFS_PER_FRAME_EX) {
            // Primary ref frame is zero based
            RK_S32 prim_buf_idx = dxva->frame_refs[dxva->primary_ref_frame].Index;
            if (prim_buf_idx >= 0) {
                MppBuffer buffer = NULL;
                y_stride = ctx->luma_size ;
                uv_stride = y_stride / 2;
                mv_offset = y_stride + uv_stride + 64;
                mpp_buf_slot_get_prop(p_hal->slots, dxva->RefFrameMapTextureIndex[dxva->primary_ref_frame], SLOT_BUFFER, &buffer);
                regs->addr_cfg.swreg80.sw_segment_read_base_msb = 0;
                regs->addr_cfg.swreg81.sw_segment_read_base_lsb = mpp_buffer_get_fd(buffer);
                mpp_dev_set_reg_offset(p_hal->dev, 81, mv_offset);
                regs->swreg11.sw_use_temporal3_mvs = 1;
            }
        }
        if (dxva->primary_ref_frame < ALLOWED_REFS_PER_FRAME_EX) {
            RK_S32 prim_buf_idx = dxva->primary_ref_frame;
            ctx->resolution_change =
                cur_mi_cols !=
                (RK_S32)((dxva->frame_refs[prim_buf_idx].width + 7) >>
                         3) ||
                cur_mi_rows !=
                (RK_S32)((dxva->frame_refs[prim_buf_idx].height + 7) >>
                         3);
        }

        regs->swreg184.sw_cur_last_offset              = cur_offset[0];
        regs->swreg185.sw_cur_last2_offset             = cur_offset[1];
        regs->swreg186.sw_cur_last3_offset             = cur_offset[2];
        regs->swreg187.sw_cur_golden_offset            = cur_offset[3];
        regs->swreg188.sw_cur_bwdref_offset            = cur_offset[4];
        regs->swreg257.sw_cur_altref2_offset           = cur_offset[5];
        regs->swreg262.sw_cur_altref_offset            = cur_offset[6];

        regs->swreg184.sw_cur_last_roffset             = cur_roffset[0];
        regs->swreg185.sw_cur_last2_roffset            = cur_roffset[1];
        regs->swreg186.sw_cur_last3_roffset            = cur_roffset[2];
        regs->swreg187.sw_cur_golden_roffset           = cur_roffset[3];
        regs->swreg188.sw_cur_bwdref_roffset           = cur_roffset[4];
        regs->swreg257.sw_cur_altref2_roffset          = cur_roffset[5];
        regs->swreg262.sw_cur_altref_roffset           = cur_roffset[6];

        /* Index start from 0 */
        regs->swreg9.sw_mf1_type  = mf_types[0] - LAST_FRAME;
        regs->swreg9.sw_mf2_type  = mf_types[1] - LAST_FRAME;
        regs->swreg9.sw_mf3_type  = mf_types[2] - LAST_FRAME;
        AV1D_DBG(AV1D_DBG_LOG, "mf_types[%d %d %d]\n", mf_types[0], mf_types[1], mf_types[2]);
    }
    regs->swreg5.sw_ref_scaling_enable = ref_scale_e;
}
#undef MAX_FRAME_DISTANCE

void vdpu_av1d_superres_params(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    // Compute and store scaling paramers needed for superres
#define SUPERRES_SCALE_BITS 3
#define SCALE_NUMERATOR 8
#define SUPERRES_SCALE_DENOMINATOR_MIN (SCALE_NUMERATOR + 1)

#define RS_SUBPEL_BITS 6
#define RS_SUBPEL_MASK ((1 << RS_SUBPEL_BITS) - 1)
#define RS_SCALE_SUBPEL_BITS 14
#define RS_SCALE_SUBPEL_MASK ((1 << RS_SCALE_SUBPEL_BITS) - 1)
#define RS_SCALE_EXTRA_BITS (RS_SCALE_SUBPEL_BITS - RS_SUBPEL_BITS)
#define RS_SCALE_EXTRA_OFF (1 << (RS_SCALE_EXTRA_BITS - 1))
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_U8 superres_scale_denominator = SCALE_NUMERATOR;
    RK_U32 superres_luma_step = 0;
    RK_U32 superres_chroma_step = 0;
    RK_U32 superres_luma_step_invra = 0;
    RK_U32 superres_chroma_step_invra = 0;
    RK_U32 superres_init_luma_subpel_x = 0;
    RK_U32 superres_init_chroma_subpel_x = 0;
    RK_U32 superres_is_scaled = 1;
    RK_U32 width = 0;
    if (dxva->coding.superres) {
        superres_scale_denominator = regs->swreg9.sw_scale_denom_minus9 + 9;
    }

    if (superres_scale_denominator > SCALE_NUMERATOR) {
        width = (dxva->upscaled_width * SCALE_NUMERATOR +
                 (superres_scale_denominator / 2)) /
                superres_scale_denominator;
        RK_U32 min_w = MPP_MIN(16, dxva->upscaled_width);
        if (width < min_w) width = min_w;
        if (width == dxva->upscaled_width) {
            superres_is_scaled = 0;
            superres_luma_step = RS_SCALE_SUBPEL_BITS;
            superres_chroma_step = RS_SCALE_SUBPEL_BITS;
            superres_luma_step_invra = RS_SCALE_SUBPEL_BITS;
            superres_chroma_step_invra = RS_SCALE_SUBPEL_BITS;
            superres_init_luma_subpel_x = 0;
            superres_init_chroma_subpel_x = 0;
            goto end;
        }
        {
            RK_S32 upscaledLumaPlaneW = dxva->upscaled_width;
            RK_S32 downscaledLumaPlaneW = width;

            RK_S32 downscaledChromaPlaneW = (downscaledLumaPlaneW + 1) >> 1;
            RK_S32 upscaledChromaPlaneW = (upscaledLumaPlaneW + 1) >> 1;

            RK_S32 stepLumaX = ((downscaledLumaPlaneW << RS_SCALE_SUBPEL_BITS) +
                                (upscaledLumaPlaneW / 2)) /
                               upscaledLumaPlaneW;
            RK_S32 stepChromaX = ((downscaledChromaPlaneW << RS_SCALE_SUBPEL_BITS) +
                                  (upscaledChromaPlaneW / 2)) /
                                 upscaledChromaPlaneW;
            RK_S32 errLuma = (upscaledLumaPlaneW * stepLumaX) -
                             (downscaledLumaPlaneW << RS_SCALE_SUBPEL_BITS);
            RK_S32  errChroma = (upscaledChromaPlaneW * stepChromaX) -
                                (downscaledChromaPlaneW << RS_SCALE_SUBPEL_BITS);
            RK_S32 initialLumaSubpelX =
                ((-((upscaledLumaPlaneW - downscaledLumaPlaneW)
                    << (RS_SCALE_SUBPEL_BITS - 1)) +
                  upscaledLumaPlaneW / 2) /
                 upscaledLumaPlaneW +
                 (1 << (RS_SCALE_EXTRA_BITS - 1)) - errLuma / 2) &
                RS_SCALE_SUBPEL_MASK;
            RK_S32 initialChromaSubpelX =
                ((-((upscaledChromaPlaneW - downscaledChromaPlaneW)
                    << (RS_SCALE_SUBPEL_BITS - 1)) +
                  upscaledChromaPlaneW / 2) /
                 upscaledChromaPlaneW +
                 (1 << (RS_SCALE_EXTRA_BITS - 1)) - errChroma / 2) &
                RS_SCALE_SUBPEL_MASK;

            superres_luma_step = stepLumaX;
            superres_chroma_step = stepChromaX;
            superres_luma_step_invra =
                ((upscaledLumaPlaneW << RS_SCALE_SUBPEL_BITS) +
                 (downscaledLumaPlaneW / 2)) /
                downscaledLumaPlaneW;
            superres_chroma_step_invra =
                ((upscaledChromaPlaneW << RS_SCALE_SUBPEL_BITS) +
                 (downscaledChromaPlaneW / 2)) /
                downscaledChromaPlaneW;
            superres_init_luma_subpel_x = initialLumaSubpelX;
            superres_init_chroma_subpel_x = initialChromaSubpelX;
        }
    } else {
        superres_luma_step = RS_SCALE_SUBPEL_BITS;
        superres_chroma_step = RS_SCALE_SUBPEL_BITS;
        superres_luma_step_invra = RS_SCALE_SUBPEL_BITS;
        superres_chroma_step_invra = RS_SCALE_SUBPEL_BITS;
        superres_init_luma_subpel_x = 0;
        superres_init_chroma_subpel_x = 0;
        superres_is_scaled = 0;
    }
end:
    regs->swreg51.sw_superres_luma_step = superres_luma_step;
    regs->swreg51.sw_superres_chroma_step = superres_chroma_step;
    regs->swreg298.sw_superres_luma_step_invra = superres_luma_step_invra;
    regs->swreg298.sw_superres_chroma_step_invra = superres_chroma_step_invra;
    regs->swreg52.sw_superres_init_luma_subpel_x = superres_init_luma_subpel_x;
    regs->swreg52.sw_superres_init_chroma_subpel_x = superres_init_chroma_subpel_x;
    regs->swreg5.sw_superres_is_scaled = superres_is_scaled;

    regs->addr_cfg.swreg89.sw_superres_colbuf_base_lsb = mpp_buffer_get_fd(ctx->filter_mem);
    mpp_dev_set_reg_offset(p_hal->dev, 89, ctx->filt_info[SR_COL].offset);
}


void vdpu_av1d_set_picture_dimensions(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    /* Write dimensions for the current picture
       (This is needed when scaling is used) */
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;

    regs->swreg4.sw_pic_width_in_cbs    = MPP_ALIGN(dxva->width, 8) >> 3;
    regs->swreg4.sw_pic_height_in_cbs   = MPP_ALIGN(dxva->height, 8) >> 3;
    regs->swreg12.sw_pic_width_pad    = MPP_ALIGN(dxva->width, 8) - dxva->width;
    regs->swreg12.sw_pic_height_pad   = MPP_ALIGN(dxva->height, 8) - dxva->height;

    regs->swreg8.sw_superres_pic_width = dxva->upscaled_width;
    regs->swreg9.sw_scale_denom_minus9 = dxva->superres_denom;

    vdpu_av1d_superres_params(p_hal, dxva);
}

void vdpu_av1d_set_segmentation(VdpuAv1dRegCtx *ctx, DXVA_PicParams_AV1 *dxva)
{
    RK_U32 segval[MAX_MB_SEGMENTS][SEG_AV1_LVL_MAX];
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_U8 s, i, j;
    RK_U8 segsign = 0;
    RK_U8 preskip_segid = 0;
    RK_U8 last_active_seg = 0;
    /* Segmentation */
    regs->swreg13.sw_segment_temp_upd_e   = dxva->segmentation.update_data;
    regs->swreg13.sw_segment_upd_e        = dxva->segmentation.update_map;
    regs->swreg13.sw_segment_e            = dxva->segmentation.enabled;

    //dec->error_resilient || dec->resolution_change;
    regs->swreg5.sw_error_resilient     =  dxva->coding.error_resilient_mode;

    if ((!dxva->format.frame_type ||  dxva->format.frame_type == AV1_FRAME_INTRA_ONLY)
        || regs->swreg5.sw_error_resilient) {
        regs->swreg11.sw_use_temporal3_mvs = 0;
    }

    regs->swreg14.sw_filt_level0   = dxva->loop_filter.filter_level[0];
    regs->swreg15.sw_filt_level1   = dxva->loop_filter.filter_level[1];
    regs->swreg16.sw_filt_level2   = dxva->loop_filter.filter_level_u;
    regs->swreg17.sw_filt_level3   = dxva->loop_filter.filter_level_v;

    /* Set filter level and QP for every segment ID. Initialize all
    * segments with default QP and filter level. */
    for (s = 0; s < MAX_MB_SEGMENTS; s++) {
        segval[s][SEG_AV1_LVL_ALT_Q] = 0;
        segval[s][SEG_AV1_LVL_ALT_LF_Y_V] = 0;
        segval[s][SEG_AV1_LVL_ALT_LF_Y_H] = 0;
        segval[s][SEG_AV1_LVL_ALT_LF_U] = 0;
        segval[s][SEG_AV1_LVL_ALT_LF_V] = 0;
        segval[s][SEG_AV1_LVL_REF_FRAME] = 0; /* segment ref_frame disabled */
        segval[s][SEG_AV1_LVL_SKIP] = 0;      /* segment skip disabled */
        segval[s][SEG_AV1_LVL_GLOBALMV] = 0;  /* global motion */
    }
    /* If a feature is enabled for a segment, overwrite the default. */
    if (dxva->segmentation.enabled) {
        RK_S32 (*segdata)[SEG_AV1_LVL_MAX] = dxva->segmentation.feature_data;

        for (s = 0; s < MAX_MB_SEGMENTS; s++) {
            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_ALT_Q)) {
                segval[s][SEG_AV1_LVL_ALT_Q] =
                    MPP_CLIP3(0, 255, MPP_ABS(segdata[s][SEG_AV1_LVL_ALT_Q]));
                segsign |= (segdata[s][SEG_AV1_LVL_ALT_Q] < 0) << s;
            }

            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_ALT_LF_Y_V))
                segval[s][SEG_AV1_LVL_ALT_LF_Y_V] =
                    MPP_CLIP3(-63, 63, segdata[s][SEG_AV1_LVL_ALT_LF_Y_V]);

            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_ALT_LF_Y_H))
                segval[s][SEG_AV1_LVL_ALT_LF_Y_H] =
                    MPP_CLIP3(-63, 63, segdata[s][SEG_AV1_LVL_ALT_LF_Y_H]);

            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_ALT_LF_U))
                segval[s][SEG_AV1_LVL_ALT_LF_U] =
                    MPP_CLIP3(-63, 63, segdata[s][SEG_AV1_LVL_ALT_LF_U]);

            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_ALT_LF_V))
                segval[s][SEG_AV1_LVL_ALT_LF_V] =
                    MPP_CLIP3(-63, 63, segdata[s][SEG_AV1_LVL_ALT_LF_V]);

            if (dxva->format.frame_type &&
                dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_REF_FRAME))
                segval[s][SEG_AV1_LVL_REF_FRAME] =
                    segdata[s][SEG_AV1_LVL_REF_FRAME] + 1;

            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_SKIP))
                segval[s][SEG_AV1_LVL_SKIP] = 1;
            if (dxva->segmentation.feature_mask[s] & (1 << SEG_AV1_LVL_GLOBALMV))
                segval[s][SEG_AV1_LVL_GLOBALMV] = 1;
        }
    }

    for (i = 0; i < MAX_MB_SEGMENTS; i++) {
        for (j = 0; j < SEG_AV1_LVL_MAX; j++) {
            if (dxva->segmentation.feature_mask[i] & (1 << j)) {
                preskip_segid |= j >= SEG_AV1_LVL_REF_FRAME;
                last_active_seg = MPP_MAX(i, last_active_seg);
            }
        }
    }

    regs->swreg9.sw_last_active_seg = last_active_seg;
    regs->swreg5.sw_preskip_segid   = preskip_segid;

    regs->swreg12.sw_seg_quant_sign = segsign;
    /* Write QP, filter level, ref frame and skip for every segment */
    regs->swreg14.sw_quant_seg0 = segval[0][SEG_AV1_LVL_ALT_Q];
    regs->swreg14.sw_filt_level_delta0_seg0 = segval[0][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg20.sw_filt_level_delta1_seg0 = segval[0][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg20.sw_filt_level_delta2_seg0 = segval[0][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg20.sw_filt_level_delta3_seg0 = segval[0][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg14.sw_refpic_seg0 = segval[0][SEG_AV1_LVL_REF_FRAME];
    regs->swreg14.sw_skip_seg0 = segval[0][SEG_AV1_LVL_SKIP];
    regs->swreg20.sw_global_mv_seg0 = segval[0][SEG_AV1_LVL_GLOBALMV];

    regs->swreg15.sw_quant_seg1 = segval[1][SEG_AV1_LVL_ALT_Q];
    regs->swreg15.sw_filt_level_delta0_seg1 = segval[1][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg21.sw_filt_level_delta1_seg1 = segval[1][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg21.sw_filt_level_delta2_seg1 = segval[1][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg21.sw_filt_level_delta3_seg1 = segval[1][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg15.sw_refpic_seg1 = segval[1][SEG_AV1_LVL_REF_FRAME];
    regs->swreg15.sw_skip_seg1 = segval[1][SEG_AV1_LVL_SKIP];
    regs->swreg21.sw_global_mv_seg1 = segval[1][SEG_AV1_LVL_GLOBALMV];

    regs->swreg16.sw_quant_seg2 = segval[2][SEG_AV1_LVL_ALT_Q];
    regs->swreg16.sw_filt_level_delta0_seg2 = segval[2][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg22.sw_filt_level_delta1_seg2 = segval[2][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg22.sw_filt_level_delta2_seg2 = segval[2][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg22.sw_filt_level_delta3_seg2 = segval[2][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg16.sw_refpic_seg2 = segval[2][SEG_AV1_LVL_REF_FRAME];
    regs->swreg16.sw_skip_seg2 = segval[2][SEG_AV1_LVL_SKIP];
    regs->swreg22.sw_global_mv_seg2 = segval[2][SEG_AV1_LVL_GLOBALMV];

    regs->swreg17.sw_quant_seg3 = segval[3][SEG_AV1_LVL_ALT_Q];
    regs->swreg17.sw_filt_level_delta0_seg3 = segval[3][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg23.sw_filt_level_delta1_seg3 = segval[3][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg23.sw_filt_level_delta2_seg3 = segval[3][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg23.sw_filt_level_delta3_seg3 = segval[3][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg17.sw_refpic_seg3 = segval[3][SEG_AV1_LVL_REF_FRAME];
    regs->swreg17.sw_skip_seg3 = segval[3][SEG_AV1_LVL_SKIP];
    regs->swreg23.sw_global_mv_seg3 = segval[3][SEG_AV1_LVL_GLOBALMV];

    regs->swreg18.sw_quant_seg4 = segval[4][SEG_AV1_LVL_ALT_Q];
    regs->swreg18.sw_filt_level_delta0_seg4 = segval[4][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg24.sw_filt_level_delta1_seg4 = segval[4][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg24.sw_filt_level_delta2_seg4 = segval[4][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg24.sw_filt_level_delta3_seg4 = segval[4][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg18.sw_refpic_seg4 = segval[4][SEG_AV1_LVL_REF_FRAME];
    regs->swreg18.sw_skip_seg4 = segval[4][SEG_AV1_LVL_SKIP];
    regs->swreg24.sw_global_mv_seg4 = segval[4][SEG_AV1_LVL_GLOBALMV];

    regs->swreg19.sw_quant_seg5 = segval[5][SEG_AV1_LVL_ALT_Q];
    regs->swreg19.sw_filt_level_delta0_seg5 = segval[5][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg25.sw_filt_level_delta1_seg5 = segval[5][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg25.sw_filt_level_delta2_seg5 = segval[5][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg25.sw_filt_level_delta3_seg5 = segval[5][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg19.sw_refpic_seg5 = segval[5][SEG_AV1_LVL_REF_FRAME];
    regs->swreg19.sw_skip_seg5 = segval[5][SEG_AV1_LVL_SKIP];
    regs->swreg25.sw_global_mv_seg5 = segval[5][SEG_AV1_LVL_GLOBALMV];

    regs->swreg31.sw_quant_seg6 = segval[6][SEG_AV1_LVL_ALT_Q];
    regs->swreg31.sw_filt_level_delta0_seg6 = segval[6][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg26.sw_filt_level_delta1_seg6 = segval[6][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg26.sw_filt_level_delta2_seg6 = segval[6][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg26.sw_filt_level_delta3_seg6 = segval[6][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg31.sw_refpic_seg6 = segval[6][SEG_AV1_LVL_REF_FRAME];
    regs->swreg31.sw_skip_seg6 = segval[6][SEG_AV1_LVL_SKIP];
    regs->swreg26.sw_global_mv_seg6 = segval[6][SEG_AV1_LVL_GLOBALMV];

    regs->swreg32.sw_quant_seg7 = segval[7][SEG_AV1_LVL_ALT_Q];
    regs->swreg32.sw_filt_level_delta0_seg7 = segval[7][SEG_AV1_LVL_ALT_LF_Y_V];
    regs->swreg27.sw_filt_level_delta1_seg7 = segval[7][SEG_AV1_LVL_ALT_LF_Y_H];
    regs->swreg27.sw_filt_level_delta2_seg7 = segval[7][SEG_AV1_LVL_ALT_LF_U];
    regs->swreg27.sw_filt_level_delta3_seg7 = segval[7][SEG_AV1_LVL_ALT_LF_V];
    regs->swreg32.sw_refpic_seg7 = segval[7][SEG_AV1_LVL_REF_FRAME];
    regs->swreg32.sw_skip_seg7 = segval[7][SEG_AV1_LVL_SKIP];
    regs->swreg27.sw_global_mv_seg7 = segval[7][SEG_AV1_LVL_GLOBALMV];
}

void vdpu_av1d_set_loopfilter(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    regs->swreg3.sw_filtering_dis      = (dxva->loop_filter.filter_level[0] == 0) && (dxva->loop_filter.filter_level[1] == 0);
    regs->swreg5.sw_filt_level_base_gt32    = dxva->loop_filter.filter_level[0] > 32;
    regs->swreg30.sw_filt_sharpness         = dxva->loop_filter.sharpness_level;
    if (dxva->loop_filter.mode_ref_delta_enabled) {
        regs->swreg59.sw_filt_ref_adj_0 = dxva->loop_filter.ref_deltas[0];
        regs->swreg59.sw_filt_ref_adj_1 = dxva->loop_filter.ref_deltas[1];
        regs->swreg59.sw_filt_ref_adj_2 = dxva->loop_filter.ref_deltas[2];
        regs->swreg59.sw_filt_ref_adj_3 = dxva->loop_filter.ref_deltas[3];
        regs->swreg30.sw_filt_ref_adj_4 = dxva->loop_filter.ref_deltas[4];
        regs->swreg30.sw_filt_ref_adj_5 = dxva->loop_filter.ref_deltas[5];
        regs->swreg49.sw_filt_ref_adj_7 = dxva->loop_filter.ref_deltas[6];
        regs->swreg49.sw_filt_ref_adj_6 = dxva->loop_filter.ref_deltas[7];
        regs->swreg30.sw_filt_mb_adj_0  = dxva->loop_filter.mode_deltas[0];
        regs->swreg30.sw_filt_mb_adj_1  = dxva->loop_filter.mode_deltas[1];
    } else {
        regs->swreg59.sw_filt_ref_adj_0 = 0;
        regs->swreg59.sw_filt_ref_adj_1 = 0;
        regs->swreg59.sw_filt_ref_adj_2 = 0;
        regs->swreg59.sw_filt_ref_adj_3 = 0;
        regs->swreg30.sw_filt_ref_adj_4 = 0;
        regs->swreg30.sw_filt_ref_adj_5 = 0;
        regs->swreg49.sw_filt_ref_adj_7 = 0;
        regs->swreg49.sw_filt_ref_adj_6 = 0;
        regs->swreg30.sw_filt_mb_adj_0  = 0;
        regs->swreg30.sw_filt_mb_adj_1  = 0;
    }

    regs->addr_cfg.swreg179.sw_dec_vert_filt_base_lsb = mpp_buffer_get_fd(ctx->filter_mem);
    regs->addr_cfg.swreg183.sw_dec_bsd_ctrl_base_lsb = mpp_buffer_get_fd(ctx->filter_mem);
    mpp_dev_set_reg_offset(p_hal->dev, 183, ctx->filt_info[DB_CTRL_COL].offset);
}

void vdpu_av1d_set_global_model(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_U8 *dst = (RK_U8 *) mpp_buffer_get_ptr(ctx->global_model);
    RK_S32 ref_frame, i;
    for (ref_frame = 0; ref_frame < GM_GLOBAL_MODELS_PER_FRAME; ++ref_frame) {
        mpp_assert(dxva->frame_refs[ref_frame].wmtype <= 3);

        /* In DDR wmmat order is 0, 1, 3, 2, 4, 5 */
        for (i = 0; i < 6; ++i) {
            if (i == 2)
                *(RK_S32 *)(dst) = dxva->frame_refs[ref_frame].wmmat[3];
            else if (i == 3)
                *(RK_S32 *)(dst) = dxva->frame_refs[ref_frame].wmmat[2];
            else
                *(RK_S32 *)(dst) = dxva->frame_refs[ref_frame].wmmat[i];
            dst += 4;
        }

        *(RK_S16 *)(dst) = dxva->frame_refs[ref_frame].alpha;//-32768;
        dst += 2;
        *(RK_S16 *)(dst) = dxva->frame_refs[ref_frame].beta;//-32768;
        dst += 2;
        *(RK_S16 *)(dst) = dxva->frame_refs[ref_frame].gamma;//-32768;
        dst += 2;
        *(RK_S16 *)(dst) = dxva->frame_refs[ref_frame].delta;//-32768;
        dst += 2;
    }

    regs->addr_cfg.swreg82.sw_global_model_base_msb = 0;
    regs->addr_cfg.swreg83.sw_global_model_base_lsb = mpp_buffer_get_fd(ctx->global_model);
}

void vdpu_av1d_set_tile_info_regs(VdpuAv1dRegCtx *ctx, DXVA_PicParams_AV1 *dxva)
{
    int transpose = ctx->tile_transpose;
    VdpuAv1dRegSet *regs = ctx->regs;
    size_t context_update_tile_id =  dxva->tiles.context_update_id;
    size_t context_update_y = context_update_tile_id / dxva->tiles.cols;
    size_t context_update_x = context_update_tile_id % dxva->tiles.cols;

    regs->swreg11.sw_multicore_expect_context_update = (0 == context_update_x);
    if (transpose) {
        context_update_tile_id =
            context_update_x * dxva->tiles.rows + context_update_y;
    }
    regs->swreg10.sw_tile_enable = (dxva->tiles.cols > 1) || (dxva->tiles.rows > 1);
    regs->swreg10.sw_num_tile_cols_8k       = dxva->tiles.cols;
    regs->swreg10.sw_num_tile_rows_8k_av1   = dxva->tiles.rows;
    regs->swreg9.sw_context_update_tile_id  = context_update_tile_id;
    regs->swreg10.sw_tile_transpose         = transpose;
    regs->swreg11.sw_dec_tile_size_mag      = dxva->tiles.tile_sz_mag;
    if (regs->swreg10.sw_tile_enable) AV1D_DBG(AV1D_DBG_LOG, "NOTICE: tile enabled.\n");

    regs->addr_cfg.swreg167.sw_tile_base_lsb = mpp_buffer_get_fd(ctx->tile_info);//
    regs->addr_cfg.swreg166.sw_tile_base_msb = 0;
}

static int check_tile_width(DXVA_PicParams_AV1 *dxva, RK_S32 width, RK_S32 leftmost)
{
    RK_S32 valid = 1;
    if (!leftmost && dxva->coding.use_128x128_superblock == 0 && dxva->coding.superres && width == 1) {
        AV1D_DBG(AV1D_DBG_LOG, "WARNING: Superres used and tile width == 64\n");
        valid = 0;
    }

    const RK_S32 sb_size_log2 = dxva->coding.use_128x128_superblock ? 7 : 6;
    RK_S32 tile_width_pixels = (width << sb_size_log2);
    if (dxva->coding.superres) {
        tile_width_pixels =
            (tile_width_pixels * (9 + dxva->superres_denom) + 4) / 8;
    }
    if (tile_width_pixels > 4096) {
        if (dxva->coding.superres)
            AV1D_LOG("WARNING: Tile width after superres > 4096\n");
        else
            AV1D_LOG("WARNING: Tile width > 4096\n");
        valid = 0;
    }
    return valid;
}

void vdpu_av1d_set_tile_info_mem(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegCtx *ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;

    RK_S32 transpose = ctx->tile_transpose;
    RK_S32 tmp = dxva->frame_tag_size + dxva->offset_to_dct_parts;
    RK_U32 stream_len =  p_hal->strm_len - tmp;
    RK_U8  *p1 = (RK_U8*)mpp_buffer_get_ptr(ctx->tile_info);
    RK_S32 size0 = transpose ?  dxva->tiles.cols : dxva->tiles.rows;
    RK_S32 size1 = transpose ? dxva->tiles.rows  :  dxva->tiles.cols;
    RK_S32 tile0, tile1;
    RK_U32 not_valid_tile_dimension = 0;

    // Write tile dimensions
    for (tile0 = 0; tile0 < size0; tile0++) {
        for (tile1 = 0; tile1 < size1; tile1++) {
            RK_S32 tile_y = transpose ? tile1 : tile0;
            RK_S32 tile_x = transpose ? tile0 : tile1;
            RK_S32 tile_id = transpose ? tile1 * size0 + tile0 : tile0 * size1 + tile1;
            RK_U32 start, end;

            RK_U32 y0 = dxva->tiles.heights[tile_y];
            RK_U32 y1 = dxva->tiles.heights[tile_y + 1];
            RK_U32 x0 = dxva->tiles.widths[tile_x];
            RK_U32 x1 = dxva->tiles.widths[tile_x + 1];

            RK_U8 leftmost = (tile_x == dxva->tiles.cols - 1);
            if (!not_valid_tile_dimension)
                not_valid_tile_dimension = !check_tile_width(dxva, x1 - x0, leftmost);
            if ((x0 << (dxva->coding.use_128x128_superblock ? 7 : 6)) >= dxva->width ||
                (y0 << (dxva->coding.use_128x128_superblock ? 7 : 6)) >= dxva->height)
                not_valid_tile_dimension = 1;

            // tile size in SB units (width,height)
            *p1++ = x1 - x0;
            *p1++ = 0;
            *p1++ = 0;
            *p1++ = 0;
            *p1++ = y1 - y0;
            *p1++ = 0;
            *p1++ = 0;
            *p1++ = 0;

            // tile start position (offset from sw_stream0_base)
            start = dxva->tiles.tile_offset_start[tile_id];
            *p1++ = start & 255;
            *p1++ = (start >> 8) & 255;
            *p1++ = (start >> 16) & 255;
            *p1++ = (start >> 24) & 255;
            if (!not_valid_tile_dimension) {
                if ((start + 1) > stream_len)
                    not_valid_tile_dimension = 1;
            }

            // # of bytes in tile data
            end = dxva->tiles.tile_offset_end[tile_id];
            *p1++ = end & 255;
            *p1++ = (end >> 8) & 255;
            *p1++ = (end >> 16) & 255;
            *p1++ = (end >> 24) & 255;
            if (!not_valid_tile_dimension) {
                if (end > stream_len)
                    not_valid_tile_dimension = 1;
            }
            AV1D_DBG(AV1D_DBG_LOG, "tile_info[%d][%d]: start=%08x end=%08x x0:x1=%d:%d y0:y1=%d:%d\n",
                     tile0, tile1, start, end, x0, x1, y0, y1);
        }
    }
}

void vdpu_av1d_set_cdef(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    RK_U32 luma_pri_strength = 0;
    RK_U16 luma_sec_strength = 0;
    RK_U32 chroma_pri_strength = 0;
    RK_U16 chroma_sec_strength = 0;
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_S32 i;

    /* CDEF */
    regs->swreg7.sw_cdef_bits           = dxva->cdef.bits;
    regs->swreg7.sw_cdef_damping        = dxva->cdef.damping;

    for (i = 0; i < 8; i++) {
        if (i == (1 << (dxva->cdef.bits))) break;
        luma_pri_strength |= dxva->cdef.y_strengths[i].primary << (i * 4);
        luma_sec_strength |= dxva->cdef.y_strengths[i].secondary << (i * 2);
        chroma_pri_strength |= dxva->cdef.uv_strengths[i].primary << (i * 4);
        chroma_sec_strength |= dxva->cdef.uv_strengths[i].secondary << (i * 2);
    }

    regs->swreg263.sw_cdef_luma_primary_strength = luma_pri_strength;
    regs->swreg53.sw_cdef_luma_secondary_strength = luma_sec_strength;
    regs->swreg264.sw_cdef_chroma_primary_strength = chroma_pri_strength;
    regs->swreg53.sw_cdef_chroma_secondary_strength = chroma_sec_strength;

    // tile column buffer; repurpose some encoder specific base
    regs->addr_cfg.swreg85.sw_cdef_colbuf_base_lsb = mpp_buffer_get_fd(ctx->filter_mem);
    mpp_dev_set_reg_offset(p_hal->dev, 85, ctx->filt_info[CDEF_COL].offset);
}

void vdpu_av1d_set_lr(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegCtx *ctx = p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_U16 lr_type = 0;
    RK_U16 lr_unit_size = 0;
    RK_S32 i = 0;

    for (i = 0; i < 3; i++) {
        lr_type |= dxva->loop_filter.frame_restoration_type[i] << (i * 2);
        lr_unit_size |= dxva->loop_filter.log2_restoration_unit_size[i] << (i * 2);
    }
    regs->swreg18.sw_lr_type = lr_type;
    regs->swreg19.sw_lr_unit_size = lr_unit_size;
    regs->addr_cfg.swreg91.sw_lr_colbuf_base_lsb = mpp_buffer_get_fd(ctx->filter_mem);
    mpp_dev_set_reg_offset(p_hal->dev, 91, ctx->filt_info[LR_COL].offset);
}

void init_scaling_function(RK_U8 scaling_points[][2], RK_U8 num_points,
                           RK_U8 scaling_lut[])
{
    RK_S32 i, point;

    if (num_points == 0) {
        memset(scaling_lut, 0, 256);
        return;
    }

    for (i = 0; i < scaling_points[0][0]; i++)
        scaling_lut[i] = scaling_points[0][1];

    for (point = 0; point < num_points - 1; point++) {
        RK_S32 x ;
        RK_S32 delta_y = scaling_points[point + 1][1] - scaling_points[point][1];
        RK_S32 delta_x = scaling_points[point + 1][0] - scaling_points[point][0];
        RK_S64 delta =
            delta_x ? delta_y * ((65536 + (delta_x >> 1)) / delta_x) : 0;
        for (x = 0; x < delta_x; x++) {
            scaling_lut[scaling_points[point][0] + x] =
                scaling_points[point][1] + (RK_S32)((x * delta + 32768) >> 16);
        }
    }

    for (i = scaling_points[num_points - 1][0]; i < 256; i++)
        scaling_lut[i] = scaling_points[num_points - 1][1];
}

void vdpu_av1d_set_fgs(VdpuAv1dRegCtx *ctx, DXVA_PicParams_AV1 *dxva)
{
    VdpuAv1dRegSet *regs = ctx->regs;
    RK_S32 ar_coeffs_y[24];
    RK_S32 ar_coeffs_cb[25];
    RK_S32 ar_coeffs_cr[25];
    RK_S32 luma_grain_block[73][82];
    RK_S32 cb_grain_block[38][44];
    RK_S32 cr_grain_block[38][44];
    RK_S32 ar_coeff_lag;
    RK_S32 ar_coeff_shift;
    RK_S32 grain_scale_shift;
    RK_S32 bitdepth;
    RK_S32 grain_center;
    RK_S32 grain_min;
    RK_S32 grain_max;
    RK_S32 i, j;
    RK_U8 *ptr = mpp_buffer_get_ptr(ctx->film_grain_mem);
    if (!dxva->film_grain.apply_grain) {
        regs->swreg7.sw_apply_grain = 0;
        // store reset params
        //   asic_buff->fg_params[asic_buff->out_buffer_i] = dec->fg_params;
        return;
    }
    /*   struct Av1FilmGrainParams *fg_params = &dec->fg_params;
       if (!dec->update_parameters) {
           RK_S32 active_ref = dec->film_grain_params_ref_idx;
           RK_S32 index_ref = Av1BufferQueueGetRef(dec_cont->bq, active_ref);
           u16 random_seed = fg_params->random_seed;
           *fg_params = asic_buff->fg_params[index_ref];
           fg_params->random_seed = random_seed;
       }
       asic_buff->fg_params[asic_buff->out_buffer_i] = *fg_params;*/

    // film grain applied on secondary output
    //  sw_ctrl->sw_apply_grain = dec_cont->pp_enabled ? 1 : 0;
    regs->swreg7.sw_num_y_points_b = dxva->film_grain.num_y_points > 0;
    regs->swreg7.sw_num_cb_points_b = dxva->film_grain.num_cb_points > 0;
    regs->swreg7.sw_num_cr_points_b = dxva->film_grain.num_cr_points > 0;
    regs->swreg8.sw_scaling_shift =  dxva->film_grain.scaling_shift_minus8 + 8;
    if (! dxva->film_grain.chroma_scaling_from_luma) {
        regs->swreg28.sw_cb_mult = dxva->film_grain.cb_mult - 128;
        regs->swreg28.sw_cb_luma_mult = dxva->film_grain.cb_luma_mult - 128;
        regs->swreg28.sw_cb_offset = dxva->film_grain.cb_offset - 256;
        regs->swreg29.sw_cr_mult = dxva->film_grain.cr_mult - 128;
        regs->swreg29.sw_cr_luma_mult = dxva->film_grain.cr_luma_mult - 128;
        regs->swreg29.sw_cr_offset = dxva->film_grain.cr_offset - 256;
    } else {
        regs->swreg28.sw_cb_mult = 0;
        regs->swreg28.sw_cb_luma_mult = 64;
        regs->swreg28.sw_cb_offset = 0;
        regs->swreg29.sw_cr_mult = 0;
        regs->swreg29.sw_cr_luma_mult = 64;
        regs->swreg29.sw_cr_offset = 0;
    }
    regs->swreg7.sw_overlap_flag = dxva->film_grain.overlap_flag;
    regs->swreg7.sw_clip_to_restricted_range = dxva->film_grain.clip_to_restricted_range;
    regs->swreg7.sw_chroma_scaling_from_luma = dxva->film_grain.chroma_scaling_from_luma;
    regs->swreg7.sw_random_seed = dxva->film_grain.grain_seed;

    init_scaling_function(dxva->film_grain.scaling_points_y, dxva->film_grain.num_y_points,
                          ctx->fgsmem.scaling_lut_y);

    if (dxva->film_grain.chroma_scaling_from_luma) {
        memcpy(ctx->fgsmem.scaling_lut_cb, ctx->fgsmem.scaling_lut_y,
               sizeof(*ctx->fgsmem.scaling_lut_y) * 256);
        memcpy(ctx->fgsmem.scaling_lut_cr, ctx->fgsmem.scaling_lut_y,
               sizeof(*ctx->fgsmem.scaling_lut_y) * 256);
    } else {
        init_scaling_function(dxva->film_grain.scaling_points_cb,
                              dxva->film_grain.num_cb_points, ctx->fgsmem.scaling_lut_cb);
        init_scaling_function(dxva->film_grain.scaling_points_cr,
                              dxva->film_grain.num_cr_points, ctx->fgsmem.scaling_lut_cr);
    }


    for (i = 0; i < 25; i++) {
        if (i < 24) {
            ar_coeffs_y[i] = dxva->film_grain.ar_coeffs_y[i] - 128;
        }
        ar_coeffs_cb[i] = dxva->film_grain.ar_coeffs_cb[i] - 128;
        ar_coeffs_cr[i] = dxva->film_grain.ar_coeffs_cr[i] - 128;
    }

    ar_coeff_lag = dxva->film_grain.ar_coeff_lag;
    ar_coeff_shift = dxva->film_grain.ar_coeff_shift_minus6 + 6;
    grain_scale_shift = dxva->film_grain.grain_scale_shift;
    bitdepth =  dxva->bitdepth;
    grain_center = 128 << (bitdepth - 8);
    grain_min = 0 - grain_center;
    grain_max = (256 << (bitdepth - 8)) - 1 - grain_center;

    GenerateLumaGrainBlock(luma_grain_block, bitdepth, dxva->film_grain.num_y_points,
                           grain_scale_shift, ar_coeff_lag, ar_coeffs_y,
                           ar_coeff_shift, grain_min, grain_max,
                           dxva->film_grain.grain_seed);

    GenerateChromaGrainBlock(
        luma_grain_block, cb_grain_block, cr_grain_block, bitdepth,
        dxva->film_grain.num_y_points, dxva->film_grain.num_cb_points,
        dxva->film_grain.num_cr_points, grain_scale_shift, ar_coeff_lag, ar_coeffs_cb,
        ar_coeffs_cr, ar_coeff_shift, grain_min, grain_max,
        dxva->film_grain.chroma_scaling_from_luma, dxva->film_grain.grain_seed);

    for (i = 0; i < 64; i++) {
        for (j = 0; j < 64; j++) {
            ctx->fgsmem.cropped_luma_grain_block[i * 64 + j] =
                luma_grain_block[i + 9][j + 9];
        }
    }

    for (i = 0; i < 32; i++) {
        for (j = 0; j < 32; j++) {
            ctx->fgsmem.cropped_chroma_grain_block[i * 64 + 2 * j] =
                cb_grain_block[i + 6][j + 6];
            ctx->fgsmem.cropped_chroma_grain_block[i * 64 + 2 * j + 1] =
                cr_grain_block[i + 6][j + 6];
        }
    }

    memcpy(ptr, &ctx->fgsmem, sizeof(FilmGrainMemory));

    regs->addr_cfg.swreg94.sw_filmgrain_base_msb = 0;
    regs->addr_cfg.swreg95.sw_filmgrain_base_lsb = mpp_buffer_get_fd(ctx->film_grain_mem);

    if (regs->swreg7.sw_apply_grain) AV1D_DBG(AV1D_DBG_LOG, "NOTICE: filmgrain enabled.\n");
}

MPP_RET vdpu_av1d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    VdpuAv1dRegCtx *ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    VdpuAv1dRegSet *regs;
    DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
    MppFrame mframe;
    MppBuffer buffer = NULL;
    MppBuffer streambuf = NULL;
    RK_U32 height = dxva->height;
    RK_U32 width = dxva->width;
    RK_U32 hor_stride;
    RK_U32 ver_stride;
    HalBuf *tile_out_buf;
    // RK_U32 bit_depth = dxva->bitdepth;
    ctx->width = width;
    ctx->height = height;
    INP_CHECK(ret, NULL == p_hal);

    ctx->refresh_frame_flags = dxva->refresh_frame_flags;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }

    if (p_hal->fast_mode) {
        RK_U32 i = 0;

        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                ctx->regs = ctx->reg_buf[i].regs;
                ctx->reg_buf[i].valid = 1;
                break;
            }
        }
    }

    regs = ctx->regs;
    memset(regs, 0, sizeof(*regs));

    if (!ctx->tile_out_bufs) {
        RK_U32 out_w = MPP_ALIGN(width * dxva->bitdepth, 16 * 8) / 8;
        RK_U32 num_sbs = ((width + 63) / 64 + 1) * ((height + 63) / 64  + 1);
        RK_U32 dir_mvs_size = MPP_ALIGN(num_sbs * 24 * 128 / 8, 16);
        RK_U32 out_h = MPP_ALIGN(height, 16);
        RK_U32 luma_size = out_w * out_h;
        RK_U32 chroma_size = luma_size / 2;

        ctx->hor_stride = out_w;
        ctx->luma_size = luma_size;
        ctx->chroma_size = chroma_size;
        ctx->tile_out_size = luma_size + chroma_size + dir_mvs_size + 512;

        if (ctx->tile_out_bufs) {
            hal_bufs_deinit(ctx->tile_out_bufs);
            ctx->tile_out_bufs = NULL;
        }
        hal_bufs_init(&ctx->tile_out_bufs);
        if (!ctx->tile_out_bufs) {
            mpp_err_f("tile out bufs init fail\n");
            goto __RETURN;
        }
        ctx->tile_out_count = mpp_buf_slot_get_count(p_hal->slots);
        hal_bufs_setup(ctx->tile_out_bufs, ctx->tile_out_count, 1, &ctx->tile_out_size);
    }

    if (!ctx->filter_mem) {
        ret = vdpu_av1d_filtermem_alloc(p_hal, ctx, dxva);
        if (!ret) {
            mpp_err("filt buffer get fail\n");
            vdpu_av1d_filtermem_release(ctx);
        }
    }
    if (!ctx->buf_tmp) {
        RK_U32 size = height * width * 2;

        mpp_buffer_get(p_hal->buf_group, &ctx->buf_tmp, size);
    }

    mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
    mpp_buf_slot_get_prop(p_hal ->slots, task->dec.output, SLOT_BUFFER, &buffer);
    mpp_buf_slot_get_prop(p_hal ->packet_slots, task->dec.input, SLOT_BUFFER, &streambuf);
    tile_out_buf = hal_bufs_get_buf(ctx->tile_out_bufs, task->dec.output);
    hor_stride = mpp_frame_get_hor_stride(mframe);
    ver_stride = mpp_frame_get_ver_stride(mframe);

    ctx->ver_stride = ver_stride;

    p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);

    ctx->fbc_en = !!MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe));

    AV1D_DBG(AV1D_DBG_LOG, "bitdepth %d fmt %d [%d : %d] wxh [%d : %d] uxv [%d : %d]\n",
             dxva->bitdepth, mpp_frame_get_fmt(mframe),
             dxva->format.subsampling_x, dxva->format.subsampling_y,
             ctx->width, ctx->height,
             ctx->hor_stride, ctx->ver_stride);

    regs->swreg1.sw_dec_abort_e     = 0;
    regs->swreg1.sw_dec_e           = 1;
    regs->swreg1.sw_dec_tile_int_e  = 0;
    regs->swreg2.sw_dec_clk_gate_e = 1;

    regs->swreg3.sw_dec_mode           = 17; // av1 mode
    regs->swreg3.sw_skip_mode          = dxva->coding.skip_mode;
    regs->swreg3.sw_dec_out_ec_byte_word = 0; // word align
    regs->swreg3.sw_write_mvs_e        = 1;
    regs->swreg3.sw_dec_out_ec_bypass  = 1;

    regs->swreg5.sw_tempor_mvp_e        = dxva->coding.use_ref_frame_mvs;
    regs->swreg5.sw_delta_lf_res_log    = dxva->loop_filter.delta_lf_res;
    regs->swreg5.sw_delta_lf_multi      = dxva->loop_filter.delta_lf_multi;
    regs->swreg5.sw_delta_lf_present    = dxva->loop_filter.delta_lf_present;
    regs->swreg5.sw_disable_cdf_update  = dxva->coding.disable_cdf_update;
    regs->swreg5.sw_allow_warp          = dxva->coding.warped_motion;
    regs->swreg5.sw_show_frame          = dxva->format.show_frame;
    regs->swreg5.sw_switchable_motion_mode  = dxva->coding.switchable_motion_mode;
    regs->swreg5.sw_enable_cdef         = dxva->coding.cdef_en;
    regs->swreg5.sw_allow_masked_compound   = dxva->coding.masked_compound;
    regs->swreg5.sw_allow_interintra    = dxva->coding.interintra_compound;
    regs->swreg5.sw_enable_intra_edge_filter = dxva->coding.intra_edge_filter;
    regs->swreg5.sw_allow_filter_intra  = dxva->coding.filter_intra;
    regs->swreg5.sw_enable_jnt_comp     = dxva->coding.jnt_comp;
    regs->swreg5.sw_enable_dual_filter  = dxva->coding.dual_filter;
    regs->swreg5.sw_reduced_tx_set_used = dxva->coding.reduced_tx_set;
    regs->swreg5.sw_allow_screen_content_tools = dxva->coding.screen_content_tools;
    regs->swreg5.sw_allow_intrabc       = dxva->coding.intrabc;

    regs->swreg5.sw_force_interger_mv   = dxva->coding.integer_mv;

    vdpu_av1d_set_global_model(p_hal, dxva);
    vdpu_av1d_set_tile_info_mem(p_hal, dxva);

    if ((dxva->format.frame_type && (dxva->format.frame_type != AV1_FRAME_INTRA_ONLY))
        || dxva->coding.intrabc) {
        vdpu_av1d_set_reference_frames(p_hal, ctx, dxva);
    }
    vdpu_av1d_set_segmentation(ctx, dxva);
    vdpu_av1d_set_loopfilter(p_hal, dxva);
    vdpu_av1d_set_picture_dimensions(p_hal, dxva);
    vdpu_av1d_set_cdef(p_hal, dxva);
    vdpu_av1d_set_lr(p_hal, dxva);
    vdpu_av1d_set_fgs(ctx, dxva);
    vdpu_av1d_set_prob(p_hal, dxva);
    vdpu_av1d_set_tile_info_regs(ctx, dxva);

#if DUMP_AV1_DATAS/* dump buffer */
    {
        char name[128];
        char *path = "/data/video";
        static int g_frame_num = 0;
        FILE *fp;
        RK_U32 i;
        RK_U32 *data;
        RK_U32 size;

        data = mpp_buffer_get_ptr(ctx->global_model);
        size = MPP_ALIGN(GLOBAL_MODEL_SIZE, 2048);
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/global_mode_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        data = mpp_buffer_get_ptr(ctx->tile_info);
        size = AV1_TILE_INFO_SIZE;
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/tile_info_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        data = mpp_buffer_get_ptr(streambuf);// + (dxva->frame_tag_size & (~0xf));
        size = MPP_ALIGN(p_hal->strm_len, 128);//mpp_buffer_get_size(streambuf);
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/stream_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        data = mpp_buffer_get_ptr(ctx->film_grain_mem);
        size = MPP_ALIGN(sizeof(AV1FilmGrainMemory), 2048);
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/film_grain_mem_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        data = mpp_buffer_get_ptr(ctx->prob_tbl_base);
        size = MPP_ALIGN(sizeof(AV1CDFs), 2048);
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/prob_tbl_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        data = mpp_buffer_get_ptr(ctx->prob_tbl_out_base);
        size = MPP_ALIGN(sizeof(AV1CDFs), 2048);
        memset(name, 0, sizeof(name));
        sprintf(name, "%s/prob_tbl_out_%d.txt", path, g_frame_num);
        fp = fopen(name, "wb");
        for ( i = 0; i < size / 4; i++)
            fprintf(fp, "%08x\n", data[i]);
        fflush(fp);
        fclose(fp);

        g_frame_num ++;
    }
#endif

    regs->swreg7.sw_blackwhite_e    = dxva->format.mono_chrome;
    regs->swreg7.sw_clip_to_restricted_range  = dxva->film_grain.clip_to_restricted_range;
    regs->swreg7.sw_delta_q_res_log     = dxva->quantization.delta_q_res;
    regs->swreg7.sw_delta_q_present     = dxva->quantization.delta_q_present;

    regs->swreg8.sw_idr_pic_e           = !dxva->format.frame_type;
    regs->swreg8.sw_quant_base_qindex   = dxva->quantization.base_qindex;
    regs->swreg8.sw_bit_depth_y_minus8  = dxva->bitdepth - 8;
    regs->swreg8.sw_bit_depth_c_minus8  = dxva->bitdepth - 8;

    regs->swreg11.sw_mcomp_filt_type    = dxva->interp_filter;
    regs->swreg11.sw_high_prec_mv_e     = dxva->coding.high_precision_mv;
    regs->swreg11.sw_comp_pred_mode     =  dxva->coding.reference_mode ? 2 : 0;
    regs->swreg11.sw_transform_mode     = dxva->coding.tx_mode;
    regs->swreg12.sw_max_cb_size        = dxva->coding.use_128x128_superblock ? 7 : 6;;
    regs->swreg12.sw_min_cb_size        = 3;

    /* unused in comdel */
    regs->swreg12.sw_av1_comp_pred_fixed_ref    = 0;
    regs->swreg13.sw_comp_pred_var_ref0_av1     = 0;
    regs->swreg13.sw_comp_pred_var_ref1_av1     = 0;
    regs->swreg14.sw_filt_level_seg0            = 0;
    regs->swreg15.sw_filt_level_seg1            = 0;
    regs->swreg16.sw_filt_level_seg2            = 0;
    regs->swreg17.sw_filt_level_seg3            = 0;
    regs->swreg18.sw_filt_level_seg4            = 0;
    regs->swreg19.sw_filt_level_seg5            = 0;
    regs->swreg31.sw_filt_level_seg6            = 0;
    regs->swreg32.sw_filt_level_seg7            = 0;


    regs->swreg13.sw_qp_delta_y_dc_av1          = dxva->quantization.y_dc_delta_q;
    regs->swreg13.sw_qp_delta_ch_dc_av1         = dxva->quantization.u_dc_delta_q;
    regs->swreg13.sw_qp_delta_ch_ac_av1         = dxva->quantization.u_ac_delta_q;
    regs->swreg47.sw_qmlevel_y                  = dxva->quantization.qm_y;
    regs->swreg48.sw_qmlevel_u                  = dxva->quantization.qm_u;
    regs->swreg49.sw_qmlevel_v                  = dxva->quantization.qm_v;

    regs->swreg13.sw_lossless_e                 = dxva->coded_lossless;
    regs->swreg28.sw_quant_delta_v_dc           = dxva->quantization.v_dc_delta_q;
    regs->swreg29.sw_quant_delta_v_ac           = dxva->quantization.v_ac_delta_q;

    regs->swreg31.sw_skip_ref0              = dxva->skip_ref0 ? dxva->skip_ref0 : 1;
    regs->swreg32.sw_skip_ref1              = dxva->skip_ref1 ? dxva->skip_ref1 : 1;

    /*input out put buf cfg*/
    {
        // RK_U32 out_w = MPP_ALIGN(4 * width * bit_depth, 128) / 8;
        // RK_U32 out_h = height / 4;
        // RK_U32 y_stride = out_w * out_h;
        // RK_U32 uv_stride = y_stride / 2;

        RK_U32 y_stride = ctx->luma_size;
        RK_U32 uv_stride = y_stride / 2;
        RK_U32 mv_offset = y_stride + uv_stride + 64;
        RK_U32 offset = (dxva->frame_tag_size & (~0xf));

        regs->addr_cfg.swreg65.sw_dec_out_ybase_lsb = mpp_buffer_get_fd(tile_out_buf->buf[0]);//mpp_buffer_get_fd(buffer);
        regs->addr_cfg.swreg99.sw_dec_out_cbase_lsb = mpp_buffer_get_fd(tile_out_buf->buf[0]);
        mpp_dev_set_reg_offset(p_hal->dev, 99, y_stride);
        regs->addr_cfg.swreg133.sw_dec_out_dbase_lsb = mpp_buffer_get_fd(tile_out_buf->buf[0]);
        mpp_dev_set_reg_offset(p_hal->dev, 133, mv_offset);

        /*  if (ctx->fbc_en) {
              regs->swreg190.sw_dec_out_tybase_lsb = 0;// TODO:
              regs->swreg224.sw_dec_out_tcbase_lsb = 0;// TODO:
          }*/

        regs->swreg258.sw_strm_buffer_len = MPP_ALIGN(p_hal->strm_len, 128);//
        regs->swreg5.sw_strm_start_bit    =  (dxva->frame_tag_size & 0xf) * 8; // bit start to decode
        regs->swreg6.sw_stream_len  = MPP_ALIGN(p_hal->strm_len, 128);//p_hal->strm_len - offset;
        regs->swreg259.sw_strm_start_offset = 0;
        regs->addr_cfg.swreg168.sw_stream_base_msb = 0;
        regs->addr_cfg.swreg169.sw_stream_base_lsb = mpp_buffer_get_fd(streambuf);
        mpp_dev_set_reg_offset(p_hal->dev, 169, offset);

        AV1D_DBG(AV1D_DBG_LOG, "stream len %d\n", p_hal->strm_len);
        AV1D_DBG(AV1D_DBG_LOG, "stream offset %d\n", offset);
        AV1D_DBG(AV1D_DBG_LOG, "stream tag_size %d\n", dxva->frame_tag_size);
        AV1D_DBG(AV1D_DBG_LOG, "stream start_bit %d\n", regs->swreg5.sw_strm_start_bit);
    }
    regs->swreg314.sw_dec_alignment = 64;

    regs->addr_cfg.swreg175.sw_mc_sync_curr_base_lsb = mpp_buffer_get_fd(ctx->tile_buf);
    regs->addr_cfg.swreg177.sw_mc_sync_left_base_lsb = mpp_buffer_get_fd(ctx->tile_buf);

    regs->swreg55.sw_apf_disable = 0;
    regs->swreg55.sw_apf_threshold = 8;
    regs->swreg58.sw_dec_buswidth = 2;
    regs->swreg58.sw_dec_max_burst = 16;
    regs->swreg266.sw_error_conceal_e                     = 0;
    regs->swreg265.sw_axi_rd_ostd_threshold               = 64;
    regs->swreg265.sw_axi_wr_ostd_threshold               = 64;

    regs->swreg318.sw_ext_timeout_cycles                  = 0xfffffff;
    regs->swreg318.sw_ext_timeout_override_e              = 1;
    regs->swreg319.sw_timeout_cycles                      = 0xfffffff;
    regs->swreg319.sw_timeout_override_e                  = 1;

    /* pp cfg */
    regs->vdpu_av1d_pp_cfg.swreg320.sw_pp_out_e = 1;
    regs->vdpu_av1d_pp_cfg.swreg322.sw_pp_in_format = 0;
    regs->vdpu_av1d_pp_cfg.swreg394.sw_pp0_dup_hor = 1;
    regs->vdpu_av1d_pp_cfg.swreg394.sw_pp0_dup_ver = 1;
    regs->vdpu_av1d_pp_cfg.swreg331.sw_pp_in_height = height / 2;
    regs->vdpu_av1d_pp_cfg.swreg331.sw_pp_in_width = width / 2;
    regs->vdpu_av1d_pp_cfg.swreg332.sw_pp_out_height = height;
    regs->vdpu_av1d_pp_cfg.swreg332.sw_pp_out_width = width;
    regs->vdpu_av1d_pp_cfg.swreg329.sw_pp_out_y_stride = hor_stride;
    regs->vdpu_av1d_pp_cfg.swreg329.sw_pp_out_c_stride = hor_stride;

    // regs->vdpu_av1d_pp_cfg.swreg337.sw_pp_in_y_stride = hor_stride;
    // regs->vdpu_av1d_pp_cfg.swreg337.sw_pp_in_c_stride = hor_stride;
    if (ctx->fbc_en) {
        regs->swreg58.sw_dec_axi_wd_id_e = 1;
        regs->swreg58.sw_dec_axi_rd_id_e = 1;
        regs->vdpu_av1d_pp_cfg.swreg320.sw_pp_out_tile_e = 1;
        regs->vdpu_av1d_pp_cfg.swreg321.sw_pp_tile_size = 2;

        RK_U32 vir_left = 0, vir_right = 0, vir_top = 0, vir_bottom = 0;
        vir_left = 0;
        if (((vir_left + width) % 16))
            vir_right = 16 - ((vir_left + width) % 16);
        else
            vir_right = 0;

        vir_top = 8;

        if (((vir_top + height) % 16))
            vir_bottom = 16 - ((vir_top + height) % 16);
        else
            vir_bottom = 0;

        regs->vdpu_av1d_pp_cfg.swreg503.sw_pp0_virtual_top = vir_top;
        regs->vdpu_av1d_pp_cfg.swreg503.sw_pp0_virtual_left = vir_left;
        regs->vdpu_av1d_pp_cfg.swreg503.sw_pp0_virtual_bottom = vir_bottom;
        regs->vdpu_av1d_pp_cfg.swreg503.sw_pp0_virtual_right = vir_right;
        regs->vdpu_av1d_pp_cfg.swreg326.sw_pp_out_lu_base_lsb = mpp_buffer_get_fd(buffer);
        regs->vdpu_av1d_pp_cfg.swreg328.sw_pp_out_ch_base_lsb = mpp_buffer_get_fd(buffer);
        regs->vdpu_av1d_pp_cfg.swreg505.sw_pp0_afbc_tile_base_lsb = mpp_buffer_get_fd(buffer);
    } else {
        RK_U32 out_w = hor_stride;
        RK_U32 out_h = ver_stride;
        RK_U32 y_stride = out_w * out_h;
        regs->vdpu_av1d_pp_cfg.swreg322.sw_pp_out_format = 3;
        regs->vdpu_av1d_pp_cfg.swreg326.sw_pp_out_lu_base_lsb = mpp_buffer_get_fd(buffer);
        regs->vdpu_av1d_pp_cfg.swreg328.sw_pp_out_ch_base_lsb = mpp_buffer_get_fd(buffer);
        mpp_dev_set_reg_offset(p_hal->dev, 328, y_stride);
    }

    /*RK_U32 i = 0;
    for (i = 0; i < sizeof(VdpuAv1dRegSet) / sizeof(RK_U32); i++)
                mpp_log("regs[%04d]=%08X\n", i, ((RK_U32 *)regs)[i]);*/


__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu_av1d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    INP_CHECK(ret, NULL == p_hal);
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }

    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    VdpuAv1dRegSet *regs = p_hal->fast_mode ?
                           reg_ctx->reg_buf[task->dec.reg_index].regs :
                           reg_ctx->regs;
    MppDev dev = p_hal->dev;
#if DUMP_AV1_DATAS
    {
        RK_U32 i = 0;
        RK_U32 *p = (RK_U32*)regs;
        char fname[128];
        FILE *fp_in = NULL;
        static RK_U32 g_frame_no = 0;

        sprintf(fname, "/data/video/reg_%d_in.txt", g_frame_no++);
        fp_in = fopen(fname, "wb");
        for (i = 0; i < sizeof(*regs) / 4; i ++ ) {
            fprintf(fp_in, "reg[%3d] = %08x\n", i, *p);
            AV1D_LOG("reg[%3d] = %08x", i, *p);
            p++;
        }
        fflush(fp_in);
        fclose(fp_in);
    }
#endif
    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = regs;
        wr_cfg.size = sizeof(*regs);
        wr_cfg.offset = 0;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg      = regs;
        rd_cfg.size     = sizeof(*regs);
        rd_cfg.offset   = 0;

        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }
        /* send request to hardware */
        ret = mpp_dev_ioctl(dev, MPP_DEV_CMD_SEND, NULL);
        if (ret) {
            mpp_err_f("send cmd failed %d\n", ret);
            break;
        }
    } while (0);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu_av1d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);
    VdpuAv1dRegCtx *reg_ctx = (VdpuAv1dRegCtx *)p_hal->reg_ctx;
    VdpuAv1dRegSet *p_regs = p_hal->fast_mode ?
                             reg_ctx->reg_buf[task->dec.reg_index].regs :
                             reg_ctx->regs;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __SKIP_HARD;
    }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);
#if DUMP_AV1_DATAS
    {
        char fname[128];
        FILE *fp_in = NULL;
        static RK_U32 g_frame_no = 0;
        RK_U32 *p = (RK_U32*)p_regs;
        RK_U32 i;

        sprintf(fname, "/data/video/reg_%d_out.txt", g_frame_no++);
        fp_in = fopen(fname, "wb");
        for (i = 0; i < sizeof(*p_regs) / 4; i ++ ) {
            fprintf(fp_in, "reg[%3d] = %08x\n", i, *p);
            AV1D_LOG("reg[%3d] = %08x", i, *p);
            p++;
        }
        fflush(fp_in);
        fclose(fp_in);
    }
#endif

__SKIP_HARD:
    if (p_hal->dec_cb) {
        DecCbHalDone m_ctx;
        RK_U32 *prob_out = (RK_U32*)mpp_buffer_get_ptr(reg_ctx->prob_tbl_out_base);

        m_ctx.task = mpp_buffer_get_ptr(reg_ctx->prob_tbl_out_base);//(void *)&task->dec;
        m_ctx.regs = (RK_U32 *)prob_out;
        if (!p_regs->swreg1.sw_dec_rdy_int/* decode err */)
            m_ctx.hard_err = 1;
        else
            m_ctx.hard_err = 0;

        mpp_callback(p_hal->dec_cb, &m_ctx);
    }
    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu_av1d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);


__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu_av1d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu_av1d_control(void *hal, MpiCmd cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO: {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);
        RK_U32 imgwidth = mpp_frame_get_width((MppFrame)param);
        RK_U32 imgheight = mpp_frame_get_height((MppFrame)param);

        AV1D_DBG(AV1D_DBG_LOG, "control info: fmt %d, w %d, h %d\n", fmt, imgwidth, imgheight);
        if ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV422SP) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
        }
        break;
    }
    case MPP_DEC_SET_OUTPUT_FORMAT: {

    } break;
    default:
        break;
    }

__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_av1d_vdpu = {
    .name       = "av1d_vdpu",
    .type       = MPP_CTX_DEC,
    .coding     = MPP_VIDEO_CodingAV1,
    .ctx_size   = sizeof(VdpuAv1dRegCtx),
    .flag       = 0,
    .init       = vdpu_av1d_init,
    .deinit     = vdpu_av1d_deinit,
    .reg_gen    = vdpu_av1d_gen_regs,
    .start      = vdpu_av1d_start,
    .wait       = vdpu_av1d_wait,
    .reset      = vdpu_av1d_reset,
    .flush      = vdpu_av1d_flush,
    .control    = vdpu_av1d_control,
};
