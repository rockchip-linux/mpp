/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "hal_av1d_vdpu383"

#include <string.h>

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_bitput.h"
#include "mpp_buffer_impl.h"

#include "hal_av1d_common.h"
#include "hal_av1d_com.h"
#include "vdpu383_av1d.h"
#include "vdpu383_com.h"

#include "av1d_syntax.h"
#include "film_grain_noise_table.h"
#include "av1d_syntax.h"

#define VDPU383_UNCMPS_HEADER_SIZE            (MPP_ALIGN(5160, 128) / 8 + 16) // byte, 5160 bit, reverse 128 bits

// bits len
#define VDPU383_RCB_STRMD_ROW_LEN             (MPP_ALIGN(dxva->width, 8) / 8 * 100)
#define VDPU383_RCB_STRMD_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 8) / 8 * 100)
#define VDPU383_RCB_INTER_ROW_LEN             (MPP_ALIGN(dxva->width, 64) / 64 * 2752)
#define VDPU383_RCB_INTER_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 64) / 64 * 2752)
#define VDPU383_RCB_INTRA_ROW_LEN             (MPP_ALIGN(dxva->width, 512) * 12 * 3)
#define VDPU383_RCB_INTRA_TILE_ROW_LEN        (MPP_ALIGN(dxva->width, 512) * 12 * 3)
#define VDPU383_RCB_FILTERD_ROW_LEN           (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_PROTECT_ROW_LEN   (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_TILE_ROW_LEN      (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 6 * 3))
#define VDPU383_RCB_FILTERD_TILE_COL_LEN      (MPP_ALIGN(dxva->width, 64) * (16 + 1) * (14 + 7 * 3 + (14 + 13 * 3) + (9 + 7 * 3)))
#define VDPU383_RCB_FILTERD_AV1_UP_TL_COL_LEN (MPP_ALIGN(dxva->width, 64) * 10 * 22)

#define VDPU383_UNCMPS_HEADER_OFFSET_BASE            (0)

#define VDPU383_INFO_ELEM_SIZE (VDPU383_UNCMPS_HEADER_SIZE)

#define VDPU383_UNCMPS_HEADER_OFFSET(idx)            (VDPU383_INFO_ELEM_SIZE * idx + VDPU383_UNCMPS_HEADER_OFFSET_BASE)

#define VDPU383_INFO_BUF_SIZE(cnt) (VDPU383_INFO_ELEM_SIZE * cnt)

#define NON_COEF_CDF_SIZE (434 * 16) // byte
#define COEF_CDF_SIZE (354 * 16) // byte
#define ALL_CDF_SIZE (NON_COEF_CDF_SIZE + COEF_CDF_SIZE * 4)

#define SET_REF_HOR_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg83_ref0_hor_virstride = value; break;\
        case 1: regs.reg86_ref1_hor_virstride = value; break;\
        case 2: regs.reg89_ref2_hor_virstride = value; break;\
        case 3: regs.reg92_ref3_hor_virstride = value; break;\
        case 4: regs.reg95_ref4_hor_virstride = value; break;\
        case 5: regs.reg98_ref5_hor_virstride = value; break;\
        case 6: regs.reg101_ref6_hor_virstride = value; break;\
        case 7: regs.reg104_ref7_hor_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_RASTER_UV_HOR_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg84_ref0_raster_uv_hor_virstride = value; break;\
        case 1: regs.reg87_ref1_raster_uv_hor_virstride = value; break;\
        case 2: regs.reg90_ref2_raster_uv_hor_virstride = value; break;\
        case 3: regs.reg93_ref3_raster_uv_hor_virstride = value; break;\
        case 4: regs.reg96_ref4_raster_uv_hor_virstride = value; break;\
        case 5: regs.reg99_ref5_raster_uv_hor_virstride = value; break;\
        case 6: regs.reg102_ref6_raster_uv_hor_virstride = value; break;\
        case 7: regs.reg105_ref7_raster_uv_hor_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_VIRSTRIDE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg85_ref0_virstride = value; break;\
        case 1: regs.reg88_ref1_virstride = value; break;\
        case 2: regs.reg91_ref2_virstride = value; break;\
        case 3: regs.reg94_ref3_virstride = value; break;\
        case 4: regs.reg97_ref4_virstride = value; break;\
        case 5: regs.reg100_ref5_virstride = value; break;\
        case 6: regs.reg103_ref6_virstride = value; break;\
        case 7: regs.reg106_ref7_virstride = value; break;\
        default: break;}\
    }while(0)

#define SET_REF_BASE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg170_av1_last_base      = value; break; \
        case 1: regs.reg171_av1golden_base     = value; break; \
        case 2: regs.reg172_av1alfter_base     = value; break; \
        case 3: regs.reg173_refer3_base        = value; break; \
        case 4: regs.reg174_refer4_base        = value; break; \
        case 5: regs.reg175_refer5_base        = value; break; \
        case 6: regs.reg176_refer6_base        = value; break; \
        case 7: regs.reg177_refer7_base        = value; break; \
        default: break;}\
    }while(0)

#define SET_FBC_PAYLOAD_REF_BASE(regs, ref_index, value)\
    do{ \
        switch(ref_index){\
        case 0: regs.reg195_payload_st_ref0_base    = value; break; \
        case 1: regs.reg196_payload_st_ref1_base    = value; break; \
        case 2: regs.reg197_payload_st_ref2_base    = value; break; \
        case 3: regs.reg198_payload_st_ref3_base    = value; break; \
        case 4: regs.reg199_payload_st_ref4_base    = value; break; \
        case 5: regs.reg200_payload_st_ref5_base    = value; break; \
        case 6: regs.reg201_payload_st_ref6_base    = value; break; \
        case 7: regs.reg202_payload_st_ref7_base    = value; break; \
        default: break;}\
    }while(0)

#define VDPU_FAST_REG_SET_CNT    3

#define OFFSET_CTRL_REGS          (8 * sizeof(RK_U32))
#define OFFSET_COMMON_ADDR_REGS   (128 * sizeof(RK_U32))
#define OFFSET_RCB_PARAS_REGS     (140 * sizeof(RK_U32))
#define OFFSET_AV1D_PARAS_REGS    (64 * sizeof(RK_U32))
#define OFFSET_AV1D_ADDR_REGS     (168 * sizeof(RK_U32))
#define OFFSET_INTERRUPT_REGS     (15 * sizeof(RK_U32))

typedef struct av1d_rkv_buf_t {
    RK_U32              valid;
    Vdpu383Av1dRegSet  *regs;
} av1dVdpu383Buf;

typedef struct vdpu383_ref_info_t {
    RK_U32 dpb_idx;
    RK_U32 seg_idx;
    RK_U32 colmv_exist_flag;
    RK_U32 cdf_valid;
    RK_U32 coeff_idx;
    RK_U32 mi_rows;
    RK_U32 mi_cols;
    RK_U32 seg_en;
    RK_U32 seg_up_map;
    RK_U32 cdf_update_flag;
} vdpu383RefInfo;

typedef struct Vdpu383Av1dRegCtx_t {
    Vdpu383Av1dRegSet  *regs;
    RK_U32             offset_uncomps;

    av1dVdpu383Buf     reg_buf[VDPU_FAST_REG_SET_CNT];
    MppBuffer          bufs;
    RK_S32             bufs_fd;
    void               *bufs_ptr;
    RK_U32             uncmps_offset[VDPU_FAST_REG_SET_CNT];

    Vdpu383RcbInfo  rcb_buf_info[RCB_BUF_COUNT];
    RK_U32          rcb_buf_size;
    MppBuffer       rcb_bufs[VDPU_FAST_REG_SET_CNT];

    HalBufs         colmv_bufs;
    RK_U32          colmv_count;
    RK_U32          colmv_size;

    vdpu383RefInfo  ref_info_tbl[NUM_REF_FRAMES];

    MppBuffer       cdf_rd_def_base;
    HalBufs         cdf_segid_bufs;
    RK_U32          cdf_segid_count;
    RK_U32          cdf_segid_size;
    RK_U32          cdf_coeff_cdf_idxs[NUM_REF_FRAMES];
    // RK_U32          cdfs_last[NUM_REF_FRAMES];

    MppBuffer       tile_info;
    MppBuffer       film_grain_mem;
    MppBuffer       global_model;
    MppBuffer       filter_mem;
    MppBuffer       tile_buf;

    AV1CDFs         *cdfs;
    MvCDFs          *cdfs_ndvc;
    AV1CDFs         default_cdfs;
    MvCDFs          default_cdfs_ndvc;
    AV1CDFs         cdfs_last[NUM_REF_FRAMES];
    MvCDFs          cdfs_last_ndvc[NUM_REF_FRAMES];
    RK_U32          refresh_frame_flags;

    RK_U32          width;
    RK_U32          height;
    RK_S32          hor_stride;
    RK_S32          ver_stride;
    RK_U32          luma_size ;
    RK_U32          chroma_size;

    AV1FilmGrainMemory fgsmem;

    RK_S8           prev_out_buffer_i;
    RK_U8           fbc_en;
    RK_U8           resolution_change;
    RK_U8           tile_transpose;
    RK_U32          ref_frame_sign_bias[AV1_REF_LIST_SIZE];

    RK_U32          tile_out_count;
    size_t          tile_out_size;

    RK_U32          num_tile_cols;
    /* uncompress header data */
    RK_U8           header_data[VDPU383_UNCMPS_HEADER_SIZE];
    HalBufs         origin_bufs;
} Vdpu383Av1dRegCtx;

// #define DUMP_AV1D_VDPU383_DATAS

#ifdef DUMP_AV1D_VDPU383_DATAS
static RK_U32 dump_cur_frame = 0;
static char dump_cur_dir[128];
static char dump_cur_fname_path[512];

static MPP_RET flip_string(char *str)
{
    RK_U32 len = strlen(str);
    RK_U32 i, j;

    for (i = 0, j = len - 1; i <= j; i++, j--) {
        // swapping characters
        char c = str[i];
        str[i] = str[j];
        str[j] = c;
    }
    return MPP_OK;
}

static MPP_RET dump_data_to_file(char *fname_path, void *data, RK_U32 data_bit_size,
                                 RK_U32 line_bits, RK_U32 big_end, RK_U32 append)
{
    RK_U8 *buf_p = (RK_U8 *)data;
    RK_U8 cur_data;
    RK_U32 i;
    RK_U32 loop_cnt;
    FILE *dump_fp = NULL;
    char line_tmp[256];
    RK_U32 str_idx = 0;

    if (append)
        dump_fp = fopen(fname_path, "aw+");
    else
        dump_fp = fopen(fname_path, "w+");
    if (!dump_fp) {
        mpp_err_f("open file: %s error!\n", fname_path);
        return MPP_NOK;
    }
    if (append)
        fseek(dump_fp, 0, SEEK_END);

    if ((data_bit_size % 4 != 0) || (line_bits % 8 != 0)) {
        mpp_err_f("line bits not align to 4!\n");
        return MPP_NOK;
    }

    loop_cnt = data_bit_size / 8;
    for (i = 0; i < loop_cnt; i++) {
        cur_data = buf_p[i];

        sprintf(&line_tmp[str_idx++], "%0x", cur_data & 0xf);
        if ((i * 8 + 4) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
        sprintf(&line_tmp[str_idx++], "%0x", (cur_data >> 4) & 0xf);
        if ((i * 8 + 8) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
    }

    // last line
    if (data_bit_size % 4) {
        cur_data = buf_p[i];
        sprintf(&line_tmp[str_idx++], "%0x", cur_data & 0xf);
        if ((i * 8 + 8) % line_bits == 0) {
            line_tmp[str_idx++] = '\0';
            str_idx = 0;
            if (!big_end)
                flip_string(line_tmp);
            fprintf(dump_fp, "%s\n", line_tmp);
        }
    }
    if (data_bit_size % line_bits) {
        loop_cnt = (line_bits - (data_bit_size % line_bits)) / 4;
        for (i = 0; i < loop_cnt; i++)
            sprintf(&line_tmp[str_idx++], "%0x", 0);
        line_tmp[str_idx++] = '\0';
        str_idx = 0;
        if (!big_end)
            flip_string(line_tmp);
        fprintf(dump_fp, "%s\n", line_tmp);
    }

    fflush(dump_fp);
    fclose(dump_fp);

    return MPP_OK;
}

static MPP_RET dump_reg(RK_U32 *reg_s, RK_U32 count, RK_U32 log_start_idx)
{
    RK_U32 loop;
    for (loop = 0; loop < count; loop++) {
        mpp_log("reg[%03d]: 0%08x", log_start_idx + loop, reg_s[loop]);
    }

    return MPP_OK;
}
#endif

static RK_U32 rkv_ver_align(RK_U32 val)
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

static MPP_RET vdpu383_setup_scale_origin_bufs(Av1dHalCtx *p_hal, MppFrame mframe)
{
    Vdpu383Av1dRegCtx *ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    /* for 8K FrameBuf scale mode */
    size_t origin_buf_size = 0;

    origin_buf_size = mpp_frame_get_buf_size(mframe);

    if (!origin_buf_size) {
        mpp_err_f("origin_bufs get buf size failed\n");
        return MPP_NOK;
    }
    if (ctx->origin_bufs) {
        hal_bufs_deinit(ctx->origin_bufs);
        ctx->origin_bufs = NULL;
    }
    hal_bufs_init(&ctx->origin_bufs);
    if (!ctx->origin_bufs) {
        mpp_err_f("origin_bufs init fail\n");
        return MPP_ERR_NOMEM;
    }
    hal_bufs_setup(ctx->origin_bufs, 16, 1, &origin_buf_size);

    return MPP_OK;
}


static MPP_RET hal_av1d_alloc_res(void *hal)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i = 0;
    void *cdf_ptr;
    INP_CHECK(ret, NULL == p_hal);

    MEM_CHECK(ret, p_hal->reg_ctx = mpp_calloc_size(void, sizeof(Vdpu383Av1dRegCtx)));
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;

    //!< malloc buffers
    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->bufs, MPP_ALIGN(VDPU383_INFO_BUF_SIZE(max_cnt), SZ_2K)));
    mpp_buffer_attach_dev(reg_ctx->bufs, p_hal->dev);
    reg_ctx->bufs_fd = mpp_buffer_get_fd(reg_ctx->bufs);
    reg_ctx->bufs_ptr = mpp_buffer_get_ptr(reg_ctx->bufs);


    for (i = 0; i < max_cnt; i++) {
        reg_ctx->reg_buf[i].regs = mpp_calloc(Vdpu383Av1dRegSet, 1);
        memset(reg_ctx->reg_buf[i].regs, 0, sizeof(Vdpu383Av1dRegSet));
        reg_ctx->uncmps_offset[i] = VDPU383_UNCMPS_HEADER_OFFSET(i);
    }

    if (!p_hal->fast_mode) {
        reg_ctx->regs = reg_ctx->reg_buf[0].regs;
        reg_ctx->offset_uncomps = reg_ctx->uncmps_offset[0];
    }

    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->cdf_rd_def_base,
                                  MPP_ALIGN(sizeof(g_av1d_default_prob), SZ_2K)));
    mpp_buffer_attach_dev(reg_ctx->cdf_rd_def_base, p_hal->dev);
    cdf_ptr = mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base);
    memcpy(cdf_ptr, g_av1d_default_prob, sizeof(g_av1d_default_prob));
    mpp_buffer_sync_end(reg_ctx->cdf_rd_def_base);

__RETURN:
    return ret;
__FAILED:
    return ret;
}

static void vdpu_av1d_filtermem_release(Vdpu383Av1dRegCtx *ctx)
{
    BUF_PUT(ctx->filter_mem);
}

static void hal_av1d_release_res(void *hal)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    RK_U32 i = 0;
    RK_U32 max_cnt = p_hal->fast_mode ? MPP_ARRAY_ELEMS(reg_ctx->reg_buf) : 1;

    for (i = 0; i < max_cnt; i++)
        MPP_FREE(reg_ctx->reg_buf[i].regs);

    BUF_PUT(reg_ctx->cdf_rd_def_base);
    BUF_PUT(reg_ctx->bufs);
    for (i = 0; i < max_cnt; i++)
        BUF_PUT(reg_ctx->rcb_bufs[i]);

    vdpu_av1d_filtermem_release(reg_ctx);
    if (reg_ctx->cdf_segid_bufs) {
        hal_bufs_deinit(reg_ctx->cdf_segid_bufs);
        reg_ctx->cdf_segid_bufs = NULL;
    }
    if (reg_ctx->colmv_bufs) {
        hal_bufs_deinit(reg_ctx->colmv_bufs);
        reg_ctx->colmv_bufs = NULL;
    }
    if (reg_ctx->origin_bufs) {
        hal_bufs_deinit(reg_ctx->origin_bufs);
        reg_ctx->origin_bufs = NULL;
    }

    MPP_FREE(p_hal->reg_ctx);
}

MPP_RET vdpu383_av1d_deinit(void *hal)
{
    hal_av1d_release_res(hal);

    return MPP_OK;
}

MPP_RET vdpu383_av1d_init(void *hal, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_OK;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    INP_CHECK(ret, NULL == p_hal);
    (void) cfg;

    FUN_CHECK(hal_av1d_alloc_res(hal));

    mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
    mpp_slots_set_prop(p_hal->slots, SLOTS_VER_ALIGN, rkv_ver_align);
    mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align);

__RETURN:
    return MPP_OK;
__FAILED:
    vdpu383_av1d_deinit(hal);

    return ret;
}

#define MAX_FRAME_DISTANCE 31
#define MAX_ACTIVE_REFS AV1_ACTIVE_REFS_EX

static RK_S32 GetRelativeDist(DXVA_PicParams_AV1 *dxva, RK_S32 a, RK_S32 b)
{
    if (!dxva->order_hint_bits) return 0;
    const RK_S32 bits = dxva->order_hint_bits - 1;

    RK_S32 diff = a - b;
    RK_S32 m = 1 << bits;
    diff = (diff & (m - 1)) - (diff & m);
    return diff;
}

#undef MAX_FRAME_DISTANCE

static RK_U32 mpp_clip_uintp2(RK_S32 a, RK_S32 p)
{
    if (a & ~((1 << p) - 1)) return -a >> 31 & ((1 << p) - 1);
    else                   return  a;
}

static MPP_RET prepare_uncompress_header(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva,
                                         RK_U64 *data, RK_U32 len)
{
    BitputCtx_t bp;
    RK_S32 i, j;
    (void) p_hal;

    mpp_set_bitput_ctx(&bp, data, len);

    /* sequence header */
    mpp_put_bits(&bp, dxva->coding.current_operating_point, 12);
    mpp_put_bits(&bp, dxva->coding.use_128x128_superblock, 1);
    mpp_put_bits(&bp, dxva->coding.filter_intra, 1);
    mpp_put_bits(&bp, dxva->coding.intra_edge_filter, 1);
    mpp_put_bits(&bp, dxva->coding.interintra_compound, 1);
    mpp_put_bits(&bp, dxva->coding.masked_compound, 1);
    mpp_put_bits(&bp, dxva->coding.dual_filter, 1);
    mpp_put_bits(&bp, dxva->enable_order_hint, 1);
    mpp_put_bits(&bp, dxva->coding.jnt_comp, 1);
    mpp_put_bits(&bp, dxva->coding.enable_ref_frame_mvs, 1);
    {
        RK_S32 order_hint_bits_minus_1 = dxva->order_hint_bits ? (dxva->order_hint_bits - 1) : 0;

        mpp_put_bits(&bp, order_hint_bits_minus_1, 3);
    }
    {
        RK_U32 skip_loop_filter = 0; // TODO: control by user
        RK_U32 enable_cdef = !skip_loop_filter && !dxva->coded_lossless;
        RK_U32 enable_cdef_y = dxva->cdef.y_strengths[0].primary || dxva->cdef.y_strengths[0].secondary;
        RK_U32 enable_cdef_uv = dxva->cdef.uv_strengths[0].primary || dxva->cdef.uv_strengths[0].secondary;

        enable_cdef = enable_cdef && (dxva->cdef.bits || enable_cdef_y || enable_cdef_uv);
        mpp_put_bits(&bp, enable_cdef, 1);
    }
    mpp_put_bits(&bp, (dxva->bitdepth > 8) ? (dxva->bitdepth - 8) : 0, 3);
    {
        RK_U32 yuv_format = 0;

        if (dxva->format.mono_chrome)
            yuv_format = 0;  // 400
        else if (dxva->format.subsampling_y == 1 && dxva->format.subsampling_y == 1)
            yuv_format = 1;  // 420
        else if (dxva->format.subsampling_x == 1)
            yuv_format = 2;  // 422
        else
            yuv_format = 3;  // 444

        mpp_put_bits(&bp, yuv_format, 2);
    }

    mpp_put_bits(&bp, dxva->film_grain.matrix_coefficients, 8);
    mpp_put_bits(&bp, dxva->coding.film_grain_en, 1);

    /* frame uncompresss header */
    {
        RK_U32 frame_is_intra = dxva->format.frame_type == KEY_FRAME ||
                                dxva->format.frame_type == INTRA_ONLY_FRAME;

        mpp_put_bits(&bp, frame_is_intra, 1);
        mpp_put_bits(&bp, dxva->coding.disable_cdf_update, 1);
        mpp_put_bits(&bp, dxva->coding.screen_content_tools, 1);
        mpp_put_bits(&bp, dxva->coding.integer_mv || frame_is_intra, 1);
        mpp_put_bits(&bp, dxva->order_hint, 8);
        mpp_put_bits(&bp, dxva->width, 16);
        mpp_put_bits(&bp, dxva->height, 16);
        mpp_put_bits(&bp, dxva->coding.superres, 1);
        mpp_put_bits(&bp, dxva->superres_denom, 5);
        mpp_put_bits(&bp, dxva->upscaled_width, 16);
        mpp_put_bits(&bp, dxva->coding.high_precision_mv, 1);
        mpp_put_bits(&bp, dxva->coding.intrabc, 1);
    }

    for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++)
        mpp_put_bits(&bp, dxva->ref_frame_valued ? dxva->ref_frame_idx[i] : (RK_U32) - 1, 3);

    mpp_put_bits(&bp, dxva->interp_filter, 3);
    mpp_put_bits(&bp, dxva->coding.switchable_motion_mode, 1);
    mpp_put_bits(&bp, dxva->coding.use_ref_frame_mvs, 1);

    {
        RK_U32 mapped_idx = 0;

        for (i = 0; i < NUM_REF_FRAMES; i++) {
            mpp_put_bits(&bp, dxva->frame_refs[i].order_hint, 8);
        }
        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++) {
            mapped_idx = dxva->ref_frame_idx[i];
            mpp_put_bits(&bp, dxva->ref_order_hint[mapped_idx], 8);
        }
    }

    for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i) {
        if (!dxva->order_hint_bits) {
            dxva->ref_frame_sign_bias[i] = 0;
        } else {
            if (dxva->frame_refs[i].Index >= 0) {
                RK_S32 ref_frame_offset = dxva->frame_refs[dxva->ref_frame_idx[i]].order_hint;
                RK_S32 rel_off = GetRelativeDist(dxva, ref_frame_offset, dxva->order_hint);

                dxva->ref_frame_sign_bias[i] = (rel_off <= 0) ? 0 : 1;
            }
        }
        mpp_put_bits(&bp, dxva->ref_frame_sign_bias[i], 1);
    }

    mpp_put_bits(&bp, dxva->coding.disable_frame_end_update_cdf, 1);

    /* quantization params */
    mpp_put_bits(&bp, dxva->quantization.base_qindex, 8);
    mpp_put_bits(&bp, dxva->quantization.y_dc_delta_q, 7);
    mpp_put_bits(&bp, dxva->quantization.u_dc_delta_q, 7);
    mpp_put_bits(&bp, dxva->quantization.u_ac_delta_q, 7);
    mpp_put_bits(&bp, dxva->quantization.v_dc_delta_q, 7);
    mpp_put_bits(&bp, dxva->quantization.v_ac_delta_q, 7);
    mpp_put_bits(&bp, dxva->quantization.using_qmatrix, 1);

    /* segmentation params */
    mpp_put_bits(&bp, dxva->segmentation.enabled, 1);
    mpp_put_bits(&bp, dxva->segmentation.update_map, 1);
    mpp_put_bits(&bp, dxva->segmentation.temporal_update, 1);

    {
        RK_U32 mi_rows = MPP_ALIGN(dxva->width, 8) >> MI_SIZE_LOG2;
        RK_U32 mi_cols = MPP_ALIGN(dxva->height, 8) >> MI_SIZE_LOG2;
        /* index 0: AV1_REF_FRAME_LAST - AV1_REF_FRAME_LAST */
        RK_U32 prev_mi_rows = MPP_ALIGN(dxva->frame_refs[0].width, 8) >> MI_SIZE_LOG2;
        RK_U32 prev_mi_cols = MPP_ALIGN(dxva->frame_refs[0].height, 8) >> MI_SIZE_LOG2;
        RK_U32 use_prev_segmentation_ids  = dxva->segmentation.enabled && dxva->primary_ref_frame &&
                                            (mi_rows == prev_mi_rows) &&
                                            (mi_cols == prev_mi_cols);

        mpp_put_bits(&bp, use_prev_segmentation_ids, 1);
    }

    /* Segmentation data update */
    for (i = 0; i < MAX_SEGMENTS; i++)
        mpp_put_bits(&bp, dxva->segmentation.feature_mask[i], 8);

    for (i = 0; i < MAX_SEGMENTS; i++) {
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][0], 9);
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][1], 7);
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][2], 7);
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][3], 7);
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][4], 7);
        mpp_put_bits(&bp, dxva->segmentation.feature_data[i][5], 3);
    }
    mpp_put_bits(&bp, dxva->segmentation.last_active, 3);
    mpp_put_bits(&bp, dxva->segmentation.preskip, 1);
    mpp_put_bits(&bp, dxva->quantization.delta_q_present, 1);
    if (dxva->quantization.delta_q_present)
        mpp_put_bits(&bp, dxva->quantization.delta_q_res, 2);
    else
        mpp_put_bits(&bp, 1 << dxva->quantization.delta_q_res, 2);

    /* delta lf params */
    mpp_put_bits(&bp, dxva->loop_filter.delta_lf_present, 1);
    mpp_put_bits(&bp, 1 << dxva->loop_filter.delta_lf_res, 2);
    mpp_put_bits(&bp, dxva->loop_filter.delta_lf_multi, 1);
    mpp_put_bits(&bp, dxva->coded_lossless, 1);
    for (i = 0; i < MAX_SEGMENTS; ++i) {
        RK_S32 qindex, lossless;

        if (dxva->segmentation.feature_mask[i] & 0x1) {
            qindex = (dxva->quantization.base_qindex + dxva->segmentation.feature_data[i][SEG_LVL_ALT_Q]);
        } else {
            qindex = dxva->quantization.base_qindex;
        }
        qindex = mpp_clip_uintp2(qindex, 8);
        lossless = qindex == 0 && dxva->quantization.y_dc_delta_q == 0 &&
                   dxva->quantization.u_dc_delta_q == 0 &&
                   dxva->quantization.v_dc_delta_q == 0 &&
                   dxva->quantization.u_ac_delta_q == 0 &&
                   dxva->quantization.v_ac_delta_q == 0;

        mpp_put_bits(&bp, lossless, 1);
    }
    mpp_put_bits(&bp, dxva->all_lossless, 1);

    /* segmentation dequant */
    mpp_put_bits(&bp, dxva->quantization.qm_y, 4);
    mpp_put_bits(&bp, dxva->quantization.qm_u, 4);
    mpp_put_bits(&bp, dxva->quantization.qm_v, 4);
    mpp_put_bits(&bp, dxva->loop_filter.filter_level[0], 6);
    mpp_put_bits(&bp, dxva->loop_filter.filter_level[1], 6);
    mpp_put_bits(&bp, dxva->loop_filter.filter_level_u, 6);
    mpp_put_bits(&bp, dxva->loop_filter.filter_level_v, 6);
    mpp_put_bits(&bp, dxva->loop_filter.sharpness_level, 3);
    mpp_put_bits(&bp, dxva->loop_filter.mode_ref_delta_enabled, 1);

    for (i = 0; i < NUM_REF_FRAMES; i++)
        mpp_put_bits(&bp, dxva->loop_filter.ref_deltas[i], 7);

    for (i = 0; i < MAX_MODE_LF_DELTAS; i++)
        mpp_put_bits(&bp, dxva->loop_filter.mode_deltas[i], 7);

    /* cdef params */
    mpp_put_bits(&bp, dxva->cdef.damping + 3, 3);
    mpp_put_bits(&bp, dxva->cdef.bits, 2);

    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, dxva->cdef.y_strengths[i].primary, 4);

    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, dxva->cdef.uv_strengths[i].primary, 4);

    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, dxva->cdef.y_strengths[i].secondary, 2);

    for (i = 0; i < 8; i++)
        mpp_put_bits(&bp, dxva->cdef.uv_strengths[i].secondary, 2);

    {
        RK_U32 uses_lr = 0;

        for (i = 0; i < (dxva->format.mono_chrome ? 1 : 3); i++)
            uses_lr |= (dxva->loop_filter.frame_restoration_type[i] != AV1_RESTORE_NONE) ? 1 : 0;
        mpp_put_bits(&bp, uses_lr, 1);
    }
    for (i = 0; i < 3; ++i)
        mpp_put_bits(&bp, dxva->loop_filter.frame_restoration_type[i], 2);
    for (i = 0; i < 2; ++i) // 0:32x32, 1:64x64, 2:128x128, 3:256x256
        mpp_put_bits(&bp, dxva->loop_filter.log2_restoration_unit_size[i], 2);

    mpp_put_bits(&bp, dxva->coding.tx_mode, 2);
    mpp_put_bits(&bp, dxva->coding.reference_mode, 1);
    mpp_put_bits(&bp, dxva->skip_ref0, 3);
    mpp_put_bits(&bp, dxva->skip_ref1, 3);
    mpp_put_bits(&bp, dxva->coding.skip_mode, 1);
    mpp_put_bits(&bp, dxva->coding.warped_motion, 1);
    mpp_put_bits(&bp, dxva->coding.reduced_tx_set, 1);

    /* gm_type and gm_params */
    for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i)
        mpp_put_bits(&bp, dxva->frame_refs[i].wmtype, 2);

    for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i)
        for (j = 0; j < 6; j++)
            mpp_put_bits(&bp, dxva->frame_refs[i].wmmat_val[j], 17);

    /* film_grain_params */
    {
        mpp_put_bits(&bp, dxva->film_grain.apply_grain, 1);
        mpp_put_bits(&bp, dxva->film_grain.grain_seed, 16);
        mpp_put_bits(&bp, dxva->film_grain.update_grain, 1);
        mpp_put_bits(&bp, dxva->film_grain.num_y_points, 4);

        for (i = 0; i < 14; i++)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_y[i][0], 8);

        for (i = 0; i < 14; i++)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_y[i][1], 8);

        mpp_put_bits(&bp, dxva->film_grain.chroma_scaling_from_luma, 1);
        mpp_put_bits(&bp, dxva->film_grain.num_cb_points, 4);

        for (i = 0; i < 10; ++i)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_cb[i][0], 8);

        for (i = 0; i < 10; ++i)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_cb[i][1], 8);

        mpp_put_bits(&bp, dxva->film_grain.num_cr_points, 4);
        for (i = 0; i < 10; ++i)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_cr[i][0], 8);

        for (i = 0; i < 10; ++i)
            mpp_put_bits(&bp, dxva->film_grain.scaling_points_cr[i][1], 8);

        mpp_put_bits(&bp, dxva->film_grain.scaling_shift_minus8, 2);
        mpp_put_bits(&bp, dxva->film_grain.ar_coeff_lag, 2);
        for (i = 0; i < 24; ++i)
            mpp_put_bits(&bp, dxva->film_grain.ar_coeffs_y[i], 8);

        for (i = 0; i < 25; ++i)
            mpp_put_bits(&bp, dxva->film_grain.ar_coeffs_cb[i], 8);

        for (i = 0; i < 25; ++i)
            mpp_put_bits(&bp, dxva->film_grain.ar_coeffs_cr[i], 8);

        mpp_put_bits(&bp, dxva->film_grain.ar_coeff_shift_minus6, 2);
        mpp_put_bits(&bp, dxva->film_grain.grain_scale_shift, 2);
        mpp_put_bits(&bp, dxva->film_grain.cb_mult, 8);
        mpp_put_bits(&bp, dxva->film_grain.cb_luma_mult, 8);
        mpp_put_bits(&bp, dxva->film_grain.cb_offset, 9);
        mpp_put_bits(&bp, dxva->film_grain.cr_mult, 8);
        mpp_put_bits(&bp, dxva->film_grain.cr_luma_mult, 8);
        mpp_put_bits(&bp, dxva->film_grain.cr_offset, 9);
        mpp_put_bits(&bp, dxva->film_grain.overlap_flag, 1);
        mpp_put_bits(&bp, dxva->film_grain.clip_to_restricted_range, 1);
    }

    /* ref frame info */
    for (i = 0; i < NUM_REF_FRAMES; ++i)
        mpp_put_bits(&bp, dxva->frame_ref_state[i].upscaled_width, 16);

    for (i = 0; i < NUM_REF_FRAMES; ++i)
        mpp_put_bits(&bp, dxva->frame_ref_state[i].frame_height, 16);

    for (i = 0; i < NUM_REF_FRAMES; ++i)
        mpp_put_bits(&bp, dxva->frame_ref_state[i].frame_width, 16);

    for (i = 0; i < NUM_REF_FRAMES; ++i)
        mpp_put_bits(&bp, dxva->frame_ref_state[i].frame_type, 2);

    for (i = 0; i < NUM_REF_FRAMES; ++i) {
        mpp_put_bits(&bp, dxva->frame_refs[i].lst_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].lst2_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].lst3_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].gld_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].bwd_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].alt2_frame_offset, 8);
        mpp_put_bits(&bp, dxva->frame_refs[i].alt_frame_offset, 8);
    }

    {
        RK_U32 mapped_idx = 0;
        RK_U32 mapped_frame_width[8] = {0};
        RK_U32 mapped_frame_height[8] = {0};

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++) {
            mapped_idx = dxva->ref_frame_idx[i];
            mapped_frame_width[mapped_idx] = dxva->frame_ref_state[mapped_idx].frame_width;
            mapped_frame_height[mapped_idx] = dxva->frame_ref_state[mapped_idx].frame_height;
        }
        for (i = 0; i <= ALLOWED_REFS_PER_FRAME_EX; ++i) {
            RK_U32 hor_scale, ver_scale;

            if (dxva->coding.intrabc) {
                hor_scale = dxva->width;
                ver_scale = dxva->height;
            } else {
                hor_scale = mapped_frame_width[i];
                ver_scale = mapped_frame_height[i];
            }
            hor_scale = ((hor_scale << AV1_REF_SCALE_SHIFT) + dxva->width / 2) / dxva->width;
            ver_scale = ((ver_scale << AV1_REF_SCALE_SHIFT) + dxva->height / 2) / dxva->height;

            mpp_put_bits(&bp, hor_scale, 16);
            mpp_put_bits(&bp, ver_scale, 16);
        }
    }

    mpp_put_bits(&bp, (dxva->frame_header_size + 7) >> 3, 10);
    /* tile info */
    mpp_put_bits(&bp, dxva->tiles.cols, 7);
    mpp_put_bits(&bp, dxva->tiles.rows, 7);
    mpp_put_bits(&bp, dxva->tiles.context_update_id, 12);
    mpp_put_bits(&bp, dxva->tiles.tile_sz_mag + 1, 3);
    mpp_put_bits(&bp, dxva->tiles.cols * dxva->tiles.rows, 13);
    mpp_put_bits(&bp, dxva->tile_cols_log2 + dxva->tile_rows_log2, 12);

    for (i = 0; i < 64; i++)
        mpp_put_bits(&bp, dxva->tiles.widths[i], 7);

    for (i = 0; i < 64; i++)
        mpp_put_bits(&bp, dxva->tiles.heights[i], 10);

    mpp_put_align(&bp, 128, 0);

    return MPP_OK;
}

static RK_S32 update_size_offset(Vdpu383RcbInfo *info, RK_U32 reg_idx,
                                 RK_S32 offset, RK_S32 len, RK_S32 rcb_buf_idx)
{
    RK_S32 buf_size = 0;

    buf_size = MPP_RCB_BYTES(len);
    info[rcb_buf_idx].reg_idx = reg_idx;
    info[rcb_buf_idx].offset = offset;
    info[rcb_buf_idx].size = buf_size;

    return buf_size;
}

#define VDPU383_SET_BUF_PROTECT_VAL(buf_p, buf_size, val, set_byte_cnt) \
do { \
    RK_U8 *but_p_tmp = (RK_U8 *)buf_p; \
    memset(&but_p_tmp[0], val, set_byte_cnt); \
    memset(&but_p_tmp[buf_size - set_byte_cnt], val, set_byte_cnt); \
} while (0)

#define VDPU383_DUMP_BUF_PROTECT_VAL(buf_p, buf_size, dump_byte_cnt) \
do { \
    RK_U32 prot_loop; \
    RK_U8 *buf_p_tmp = (RK_U8 *)buf_p; \
    printf("======> protect buffer begin\n"); \
    for (prot_loop = 0; prot_loop < dump_byte_cnt; prot_loop++) { \
        if (prot_loop % 32 == 0 && prot_loop != 0) \
            printf("\n"); \
        printf("0x%02x ", buf_p_tmp[prot_loop]); \
    } \
    printf("\n"); \
    printf("======> protect buffer end\n"); \
    for (prot_loop = 0; prot_loop < dump_byte_cnt; prot_loop++) { \
        if (prot_loop % 32 == 0 && prot_loop != 0) \
            printf("\n"); \
        printf("0x%02x ", buf_p_tmp[buf_size - 1 - prot_loop]); \
    } \
    printf("\n"); \
} while (0)

#if 0
static void rcb_buf_set_edge(Vdpu383Av1dRegCtx *reg_ctx, MppBuffer buf)
{
    RK_U32 loop;
    RK_U8 *buf_p = mpp_buffer_get_ptr(buf);

    for (loop = 0; loop < RCB_BUF_COUNT; loop++) {
        VDPU383_SET_BUF_PROTECT_VAL(&buf_p[reg_ctx->rcb_buf_info[loop].offset],
                                    reg_ctx->rcb_buf_info[loop].size, 0xaa, 128);
    }
}

static void rcb_buf_dump_edge(Vdpu383Av1dRegCtx *reg_ctx, MppBuffer buf)
{
    RK_U8 *buf_p = mpp_buffer_get_ptr(buf);
    RK_U32 loop;

    for (loop = 0; loop < RCB_BUF_COUNT; loop++) {
        // reg_ctx->rcb_buf_info[loop].reg;
        // reg_ctx->rcb_buf_info[loop].offset;
        // reg_ctx->rcb_buf_info[loop].size;

        printf("======> reg idx:%d\n", reg_ctx->rcb_buf_info[loop].reg);
        VDPU383_DUMP_BUF_PROTECT_VAL(&buf_p[reg_ctx->rcb_buf_info[loop].offset],
                                     reg_ctx->rcb_buf_info[loop].size, 128);
    }
}
#endif

static void av1d_refine_rcb_size(Vdpu383RcbInfo *rcb_info,
                                 RK_S32 width, RK_S32 height, void* data)
{
    RK_U32 rcb_bits = 0;
    DXVA_PicParams_AV1 *pic_param = (DXVA_PicParams_AV1*)data;
    RK_U32 tile_row_num = pic_param->tiles.rows;
    RK_U32 tile_col_num = pic_param->tiles.cols;
    RK_U32 bit_depth = pic_param->bitdepth;
    RK_U32 sb_size = pic_param->coding.use_128x128_superblock ? 128 : 64;
    RK_U32 ext_row_align_size = tile_row_num * 64 * 8;
    RK_U32 ext_col_align_size = tile_col_num * 64 * 8;
    RK_U32 filterd_row_append = 8192;

    width = MPP_ALIGN(width, sb_size);
    height = MPP_ALIGN(height, sb_size);
    /* RCB_STRMD_ROW && RCB_STRMD_TILE_ROW*/
    rcb_bits = ((width + 7) / 8) * 100;
    rcb_info[RCB_STRMD_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_info[RCB_STRMD_TILE_ROW].size = 0;

    /* RCB_INTER_ROW && RCB_INTER_TILE_ROW*/
    rcb_bits = ((width + 63) / 64) * 2752;
    rcb_info[RCB_INTER_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTER_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTER_TILE_ROW].size = 0;

    /* RCB_INTRA_ROW && RCB_INTRA_TILE_ROW*/
    rcb_bits = MPP_ALIGN(width, 512) * (bit_depth + 2);
    rcb_bits = rcb_bits * 3; //TODO:
    rcb_info[RCB_INTRA_ROW].size = MPP_RCB_BYTES(rcb_bits);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_INTRA_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_INTRA_TILE_ROW].size = 0;

    /* RCB_FILTERD_ROW && RCB_FILTERD_TILE_ROW*/
    if (width > 4096)
        filterd_row_append = 27648;
    rcb_bits = (RK_U32)(MPP_ALIGN(width, 64) * (32 * bit_depth + 10));
    rcb_info[RCB_FILTERD_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_info[RCB_FILTERD_PROTECT_ROW].size = filterd_row_append + MPP_RCB_BYTES(rcb_bits / 2);
    rcb_bits += ext_row_align_size;
    if (tile_row_num > 1)
        rcb_info[RCB_FILTERD_TILE_ROW].size = MPP_RCB_BYTES(rcb_bits);
    else
        rcb_info[RCB_FILTERD_TILE_ROW].size = 0;

    /* RCB_FILTERD_TILE_COL */
    if (tile_col_num > 1) {
        rcb_bits = (MPP_ALIGN(height, 64) * (101 * bit_depth + 32)) + ext_col_align_size;
        rcb_info[RCB_FILTERD_TILE_COL].size = MPP_RCB_BYTES(rcb_bits);
    } else {
        rcb_info[RCB_FILTERD_TILE_COL].size = 0;
    }

}

static void vdpu383_av1d_rcb_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    RK_U32 offset = 0;
    RK_U32 max_cnt = p_hal->fast_mode ? VDPU_FAST_REG_SET_CNT : 1;
    RK_U32 i;

    offset += update_size_offset(reg_ctx->rcb_buf_info, 140, offset, VDPU383_RCB_STRMD_ROW_LEN,             RCB_STRMD_ROW          );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 142, offset, VDPU383_RCB_STRMD_TILE_ROW_LEN,        RCB_STRMD_TILE_ROW     );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 144, offset, VDPU383_RCB_INTER_ROW_LEN,             RCB_INTER_ROW          );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 146, offset, VDPU383_RCB_INTER_TILE_ROW_LEN,        RCB_INTER_TILE_ROW     );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 148, offset, VDPU383_RCB_INTRA_ROW_LEN,             RCB_INTRA_ROW          );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 150, offset, VDPU383_RCB_INTRA_TILE_ROW_LEN,        RCB_INTRA_TILE_ROW     );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 152, offset, VDPU383_RCB_FILTERD_ROW_LEN,           RCB_FILTERD_ROW        );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 154, offset, VDPU383_RCB_FILTERD_PROTECT_ROW_LEN,   RCB_FILTERD_PROTECT_ROW);
    offset += update_size_offset(reg_ctx->rcb_buf_info, 156, offset, VDPU383_RCB_FILTERD_TILE_ROW_LEN,      RCB_FILTERD_TILE_ROW   );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 158, offset, VDPU383_RCB_FILTERD_TILE_COL_LEN,      RCB_FILTERD_TILE_COL   );
    offset += update_size_offset(reg_ctx->rcb_buf_info, 160, offset, VDPU383_RCB_FILTERD_AV1_UP_TL_COL_LEN, RCB_FILTERD_AV1_UP_TILE_COL);
    reg_ctx->rcb_buf_size = offset;

    av1d_refine_rcb_size(reg_ctx->rcb_buf_info, dxva->width, dxva->height, dxva);

    for (i = 0; i < max_cnt; i++) {
        MppBuffer rcb_buf = reg_ctx->rcb_bufs[i];

        if (rcb_buf) {
            mpp_buffer_put(rcb_buf);
            reg_ctx->rcb_bufs[i] = NULL;
        }
        mpp_buffer_get(p_hal->buf_group, &rcb_buf, reg_ctx->rcb_buf_size);
        reg_ctx->rcb_bufs[i] = rcb_buf;

        // rcb_buf_set_edge(reg_ctx, rcb_buf);
        // rcb_buf_dump_edge(reg_ctx, rcb_buf);
    }

    return;
}

static void vdpu383_av1d_rcb_reg_cfg(Av1dHalCtx *p_hal, MppBuffer buf)
{
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs = reg_ctx->regs;
    RK_U32 fd = mpp_buffer_get_fd(buf);
    RK_U32 i;

    regs->common_addr.reg140_rcb_strmd_row_offset           = fd;
    regs->common_addr.reg142_rcb_strmd_tile_row_offset      = fd;
    regs->common_addr.reg144_rcb_inter_row_offset           = fd;
    regs->common_addr.reg146_rcb_inter_tile_row_offset      = fd;
    regs->common_addr.reg148_rcb_intra_row_offset           = fd;
    regs->common_addr.reg150_rcb_intra_tile_row_offset      = fd;
    regs->common_addr.reg152_rcb_filterd_row_offset         = fd;
    regs->common_addr.reg154_rcb_filterd_protect_row_offset = fd;
    regs->common_addr.reg156_rcb_filterd_tile_row_offset    = fd;
    regs->common_addr.reg158_rcb_filterd_tile_col_offset    = fd;
    regs->common_addr.reg160_rcb_filterd_av1_upscale_tile_col_offset = fd;

    regs->common_addr.reg141_rcb_strmd_row_len            = reg_ctx->rcb_buf_info[RCB_STRMD_ROW].size;
    regs->common_addr.reg143_rcb_strmd_tile_row_len       = reg_ctx->rcb_buf_info[RCB_STRMD_TILE_ROW].size;
    regs->common_addr.reg145_rcb_inter_row_len            = reg_ctx->rcb_buf_info[RCB_INTER_ROW].size;
    regs->common_addr.reg147_rcb_inter_tile_row_len       = reg_ctx->rcb_buf_info[RCB_INTER_TILE_ROW].size;
    regs->common_addr.reg149_rcb_intra_row_len            = reg_ctx->rcb_buf_info[RCB_INTRA_ROW].size;
    regs->common_addr.reg151_rcb_intra_tile_row_len       = reg_ctx->rcb_buf_info[RCB_INTRA_TILE_ROW].size;
    regs->common_addr.reg153_rcb_filterd_row_len          = reg_ctx->rcb_buf_info[RCB_FILTERD_ROW].size;
    regs->common_addr.reg155_rcb_filterd_protect_row_len  = reg_ctx->rcb_buf_info[RCB_FILTERD_PROTECT_ROW].size;
    regs->common_addr.reg157_rcb_filterd_tile_row_len     = reg_ctx->rcb_buf_info[RCB_FILTERD_TILE_ROW].size;
    regs->common_addr.reg159_rcb_filterd_tile_col_len     = reg_ctx->rcb_buf_info[RCB_FILTERD_TILE_COL].size;
    regs->common_addr.reg161_rcb_filterd_av1_upscale_tile_col_len  = reg_ctx->rcb_buf_info[RCB_FILTERD_AV1_UP_TILE_COL].size;

    for (i = 0; i < RCB_BUF_COUNT; i++)
        mpp_dev_set_reg_offset(p_hal->dev, reg_ctx->rcb_buf_info[i].reg_idx, reg_ctx->rcb_buf_info[i].offset);
}

static MPP_RET vdpu383_av1d_colmv_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    size_t mv_size;

    /* the worst case is the frame is error with whole frame */
    mv_size = MPP_ALIGN(dxva->width, 64) / 64 * MPP_ALIGN(dxva->height, 64) / 64 * 1024;
    if (reg_ctx->colmv_bufs == NULL || reg_ctx->colmv_size < mv_size) {
        if (reg_ctx->colmv_bufs) {
            hal_bufs_deinit(reg_ctx->colmv_bufs);
            reg_ctx->colmv_bufs = NULL;
        }

        hal_bufs_init(&reg_ctx->colmv_bufs);
        if (reg_ctx->colmv_bufs == NULL) {
            mpp_err_f("colmv bufs init fail");
            goto __RETURN;
        }
        reg_ctx->colmv_size = mv_size;
        reg_ctx->colmv_count = mpp_buf_slot_get_count(p_hal->slots);
        hal_bufs_setup(reg_ctx->colmv_bufs, reg_ctx->colmv_count, 1, &mv_size);
    }

__RETURN:
    return ret;
}

static MPP_RET vdpu383_av1d_cdf_setup(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    size_t size = 0;
    size_t segid_size = (MPP_ALIGN(dxva->width, 128) / 128) * \
                        (MPP_ALIGN(dxva->height, 128) / 128) * \
                        32 * 16;
    size = ALL_CDF_SIZE + segid_size;

    /* the worst case is the frame is error with whole frame */
    if (reg_ctx->cdf_segid_bufs == NULL || reg_ctx->cdf_segid_size < size) {
        if (reg_ctx->cdf_segid_bufs) {
            hal_bufs_deinit(reg_ctx->cdf_segid_bufs);
            reg_ctx->cdf_segid_bufs = NULL;
        }

        hal_bufs_init(&reg_ctx->cdf_segid_bufs);
        if (reg_ctx->cdf_segid_bufs == NULL) {
            mpp_err_f("cdf bufs init fail");
            goto __RETURN;
        }

        reg_ctx->cdf_segid_size = size;
        reg_ctx->cdf_segid_count = mpp_buf_slot_get_count(p_hal->slots);
        hal_bufs_setup(reg_ctx->cdf_segid_bufs, reg_ctx->cdf_segid_count, 1, &size);
    }

__RETURN:
    return ret;
}

/*
 * cdf buf structure:
 *
 *      base_addr0 +--------------------------+
 *      434x128bit |   def_non_coeff_cdf      |
 *  base_addr0+433 +--------------------------+
 *
 *      base_addr1 +--------------------------+
 *      354x128bit |     def_coeff_cdf_0      |
 *                 |   (base_q_idx <= 20)     |
 *                 +--------------------------+
 *      354x128bit |     def_coeff_cdf_1      |
 *                 | (20 < base_q_idx <= 60)  |
 *                 +--------------------------+
 *      354x128bit |     def_coeff_cdf_2      |
 *                 | (60 < base_q_idx <= 120) |
 *                 +--------------------------+
 *      354x128bit |     def_coeff_cdf_3      |
 *                 |   (base_q_idx > 120)     |
 * base_addr1+1415 +--------------------------+
 */

static void vdpu383_av1d_set_cdf(Av1dHalCtx *p_hal, DXVA_PicParams_AV1 *dxva)
{
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs = reg_ctx->regs;
    RK_U32 coeff_cdf_idx = 0;
    RK_U32 mapped_idx = 0;
    HalBuf *cdf_buf = NULL;
    RK_U32 i = 0;
    MppBuffer buf_tmp = NULL;

    /* use para in decoder */
#ifdef DUMP_AV1D_VDPU383_DATAS
    {
        char *cur_fname = "cabac_cdf_in.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
    }
#endif
    if (dxva->format.frame_type == AV1_FRAME_KEY)
        for (i = 0; i < NUM_REF_FRAMES; i++)
            reg_ctx->ref_info_tbl[i].cdf_valid = 0;
    /* def coeff cdf idx */
    coeff_cdf_idx = dxva->quantization.base_qindex <= 20 ? 0 :
                    dxva->quantization.base_qindex <= 60  ? 1 :
                    dxva->quantization.base_qindex <= 120 ? 2 : 3;

    if (dxva->format.frame_type == AV1_FRAME_KEY ||
        dxva->primary_ref_frame == 7) { /* AV1_PRIMARY_REF_NONE */
        regs->av1d_addrs.reg184_av1_noncoef_rd_base = mpp_buffer_get_fd(reg_ctx->cdf_rd_def_base);
        regs->av1d_addrs.reg178_av1_coef_rd_base = mpp_buffer_get_fd(reg_ctx->cdf_rd_def_base);
#ifdef DUMP_AV1D_VDPU383_DATAS
        {
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base),
                              8 * NON_COEF_CDF_SIZE, 128, 0, 0);
            dump_data_to_file(dump_cur_fname_path, (RK_U8 *)mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base)
                              + NON_COEF_CDF_SIZE + COEF_CDF_SIZE * coeff_cdf_idx,
                              8 * COEF_CDF_SIZE, 128, 0, 1);
        }
#endif
    } else {
        mapped_idx = dxva->ref_frame_idx[dxva->primary_ref_frame];

        coeff_cdf_idx = reg_ctx->ref_info_tbl[mapped_idx].coeff_idx;
        if (!dxva->coding.disable_frame_end_update_cdf &&
            reg_ctx->ref_info_tbl[mapped_idx].cdf_valid &&
            dxva->frame_refs[mapped_idx].Index != (CHAR)0xff &&
            dxva->frame_refs[mapped_idx].Index != 0x7f) {
            cdf_buf = hal_bufs_get_buf(reg_ctx->cdf_segid_bufs, dxva->frame_refs[mapped_idx].Index);
            buf_tmp = cdf_buf->buf[0];
        } else {
            buf_tmp = reg_ctx->cdf_rd_def_base;
        }
        regs->av1d_addrs.reg184_av1_noncoef_rd_base = mpp_buffer_get_fd(buf_tmp);
        regs->av1d_addrs.reg178_av1_coef_rd_base = mpp_buffer_get_fd(buf_tmp);
        regs->av1d_addrs.reg181_av1_rd_segid_base = mpp_buffer_get_fd(buf_tmp);
#ifdef DUMP_AV1D_VDPU383_DATAS
        {
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(buf_tmp),
                              8 * NON_COEF_CDF_SIZE, 128, 0, 0);
            dump_data_to_file(dump_cur_fname_path, (RK_U8 *)mpp_buffer_get_ptr(buf_tmp)
                              + NON_COEF_CDF_SIZE + COEF_CDF_SIZE * coeff_cdf_idx,
                              8 * COEF_CDF_SIZE, 128, 0, 1);
        }
#endif
    }
    cdf_buf = hal_bufs_get_buf(reg_ctx->cdf_segid_bufs, dxva->CurrPic.Index7Bits);
    regs->av1d_addrs.reg185_av1_noncoef_wr_base = mpp_buffer_get_fd(cdf_buf->buf[0]);
    regs->av1d_addrs.reg179_av1_coef_wr_base = mpp_buffer_get_fd(cdf_buf->buf[0]);
    regs->av1d_addrs.reg182_av1_wr_segid_base = mpp_buffer_get_fd(cdf_buf->buf[0]);

    /* byte, 434 x 128 bit = 434 x 16 byte */
    mpp_dev_set_reg_offset(p_hal->dev, 178, NON_COEF_CDF_SIZE + COEF_CDF_SIZE * coeff_cdf_idx);
    mpp_dev_set_reg_offset(p_hal->dev, 179, NON_COEF_CDF_SIZE);
    mpp_dev_set_reg_offset(p_hal->dev, 181, ALL_CDF_SIZE);
    mpp_dev_set_reg_offset(p_hal->dev, 182, ALL_CDF_SIZE);

    /* update params sync with "update buffer" */
    for (i = 0; i < NUM_REF_FRAMES; i++) {
        if (dxva->refresh_frame_flags & (1 << i)) {
            if (dxva->coding.disable_frame_end_update_cdf) {
                if (dxva->show_existing_frame && dxva->format.frame_type == AV1_FRAME_KEY)
                    reg_ctx->ref_info_tbl[i].coeff_idx
                        = reg_ctx->ref_info_tbl[dxva->frame_to_show_map_idx].coeff_idx;
                else
                    reg_ctx->ref_info_tbl[i].coeff_idx = coeff_cdf_idx;
            } else {
                reg_ctx->ref_info_tbl[i].cdf_valid = 1;
                reg_ctx->ref_info_tbl[i].coeff_idx = 0;
            }
        }
    }

#ifdef DUMP_AV1D_VDPU383_DATAS
    {
        char *cur_fname = "cdf_rd_def.dat";
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base),
                          (NON_COEF_CDF_SIZE + COEF_CDF_SIZE * 4) * 8, 128, 0, 0);
    }
#endif

}

MPP_RET vdpu383_av1d_gen_regs(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    Vdpu383Av1dRegCtx *ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs;
    DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
    RK_U32 i = 0;
    HalBuf *origin_buf = NULL;
    MppFrame mframe;

    INP_CHECK(ret, NULL == p_hal);

    ctx->refresh_frame_flags = dxva->refresh_frame_flags;

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        mpp_err_f("parse err %d ref err %d\n",
                  task->dec.flags.parse_err, task->dec.flags.ref_err);
        goto __RETURN;
    }

    mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
    if (mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY &&
        MPP_MAX(dxva->width, dxva->height) > 4096 && ctx->origin_bufs == NULL) {
        vdpu383_setup_scale_origin_bufs(p_hal, mframe);
    }

    if (p_hal->fast_mode) {
        for (i = 0; i <  MPP_ARRAY_ELEMS(ctx->reg_buf); i++) {
            if (!ctx->reg_buf[i].valid) {
                task->dec.reg_index = i;
                ctx->regs = ctx->reg_buf[i].regs;
                ctx->reg_buf[i].valid = 1;
                ctx->offset_uncomps = ctx->uncmps_offset[i];
                break;
            }
        }
    }
    regs = ctx->regs;
    memset(regs, 0, sizeof(*regs));
    p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);

#ifdef DUMP_AV1D_VDPU383_DATAS
    {
        memset(dump_cur_dir, 0, sizeof(dump_cur_dir));
        sprintf(dump_cur_dir, "av1/Frame%04d", dump_cur_frame);
        if (access(dump_cur_dir, 0)) {
            if (mkdir(dump_cur_dir))
                mpp_err_f("error: mkdir %s\n", dump_cur_dir);
        }
        dump_cur_frame++;
    }
#endif

    /* set reg -> ctrl reg */
    {
        regs->ctrl_regs.reg8_dec_mode          = 4; // av1
        regs->ctrl_regs.reg9.fbc_e             = 0;
        regs->ctrl_regs.reg9.buf_empty_en      = 0;

        regs->ctrl_regs.reg10.strmd_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.inter_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.intra_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.transd_auto_gating_e   = 1;
        regs->ctrl_regs.reg10.recon_auto_gating_e    = 1;
        regs->ctrl_regs.reg10.filterd_auto_gating_e  = 1;
        regs->ctrl_regs.reg10.bus_auto_gating_e      = 1;
        regs->ctrl_regs.reg10.ctrl_auto_gating_e     = 1;
        regs->ctrl_regs.reg10.rcb_auto_gating_e      = 1;
        regs->ctrl_regs.reg10.err_prc_auto_gating_e  = 1;

        // regs->ctrl_regs.reg11.dec_timeout_dis        = 1;

        regs->ctrl_regs.reg13_core_timeout_threshold  = 0x3fffff;

        regs->ctrl_regs.reg16.error_proc_disable     = 1;
        regs->ctrl_regs.reg16.error_spread_disable   = 0;
        regs->ctrl_regs.reg16.roi_error_ctu_cal_en   = 0;

        regs->ctrl_regs.reg20_cabac_error_en_lowbits  = 0xffffffdf;
        regs->ctrl_regs.reg21_cabac_error_en_highbits = 0x3fffffff;

        regs->ctrl_regs.reg28.axi_perf_work_e = 1;
        regs->ctrl_regs.reg28.axi_cnt_type    = 1;
        regs->ctrl_regs.reg28.rd_latency_id   = 11;

        regs->ctrl_regs.reg29.addr_align_type     = 1;
        regs->ctrl_regs.reg29.ar_cnt_id_type      = 0;
        regs->ctrl_regs.reg29.aw_cnt_id_type      = 1;
        regs->ctrl_regs.reg29.ar_count_id         = 17;
        regs->ctrl_regs.reg29.aw_count_id         = 0;
        regs->ctrl_regs.reg29.rd_band_width_mode  = 0;

        regs->ctrl_regs.reg30.axi_wr_qos = 0;
        regs->ctrl_regs.reg30.axi_rd_qos = 0;
    }

    /* set reg -> pkt data */
    {
        MppBuffer mbuffer = NULL;

        /* uncompress header data */
        prepare_uncompress_header(p_hal, dxva, (RK_U64 *)ctx->header_data, sizeof(ctx->header_data) / 8);
        memcpy((char *)ctx->bufs_ptr, (void *)ctx->header_data, sizeof(ctx->header_data));
        regs->av1d_paras.reg67_global_len = VDPU383_UNCMPS_HEADER_SIZE / 16; // 128 bit as unit
        regs->common_addr.reg131_gbl_base = ctx->bufs_fd;
        // mpp_dev_set_reg_offset(p_hal->dev, 131, ctx->offset_uncomps);
#ifdef DUMP_AV1D_VDPU383_DATAS
        {
            char *cur_fname = "global_cfg.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, ctx->bufs_ptr,
                              8 * regs->av1d_paras.reg67_global_len * 16, 64, 0, 0);
        }
#endif
        // input strm
        p_hal->strm_len = (RK_S32)mpp_packet_get_length(task->dec.input_packet);
        regs->av1d_paras.reg66_stream_len = MPP_ALIGN(p_hal->strm_len + 15, 128);
        mpp_buf_slot_get_prop(p_hal->packet_slots, task->dec.input, SLOT_BUFFER, &mbuffer);
        regs->common_addr.reg128_strm_base = mpp_buffer_get_fd(mbuffer);
        regs->av1d_paras.reg65_strm_start_bit = (ctx->offset_uncomps & 0xf) * 8; // bit start to decode
        mpp_dev_set_reg_offset(p_hal->dev, 128, ctx->offset_uncomps & 0xfffffff0);
        /* error */
        regs->av1d_addrs.reg169_error_ref_base = mpp_buffer_get_fd(mbuffer);
#ifdef DUMP_AV1D_VDPU383_DATAS
        {
            char *cur_fname = "stream_in.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer)
                              + ctx->offset_uncomps,
                              8 * p_hal->strm_len, 128, 0, 0);
        }
        {
            char *cur_fname = "stream_in_no_offset.dat";
            memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
            sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
            dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                              8 * p_hal->strm_len, 128, 0, 0);
        }
#endif
    }

    /* set reg -> rcb */
    vdpu383_av1d_rcb_setup(p_hal, dxva);
    vdpu383_av1d_rcb_reg_cfg(p_hal, p_hal->fast_mode ? ctx->rcb_bufs[task->dec.reg_index] : ctx->rcb_bufs[0]);

    /* set reg -> para (stride, len) */
    {
        RK_U32 hor_virstride = 0;
        RK_U32 ver_virstride = 0;
        RK_U32 y_virstride = 0;
        RK_U32 uv_virstride = 0;
        RK_U32 mapped_idx;

        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_FRAME_PTR, &mframe);
        if (mframe) {
            hor_virstride = mpp_frame_get_hor_stride(mframe);
            ver_virstride = mpp_frame_get_ver_stride(mframe);
            y_virstride = hor_virstride * ver_virstride;
            uv_virstride = hor_virstride * ver_virstride / 2;

            if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
                RK_U32 fbd_offset;
                RK_U32 fbc_hdr_stride = mpp_frame_get_fbc_hdr_stride(mframe);
                RK_U32 h = MPP_ALIGN(mpp_frame_get_height(mframe), 64);

                regs->ctrl_regs.reg9.fbc_e = 1;
                regs->av1d_paras.reg68_hor_virstride = fbc_hdr_stride / 64;
                fbd_offset = regs->av1d_paras.reg68_hor_virstride * h * 4;
                regs->av1d_addrs.reg193_fbc_payload_offset = fbd_offset;
            } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                regs->ctrl_regs.reg9.tile_e = 1;
                regs->av1d_paras.reg68_hor_virstride = MPP_ALIGN(hor_virstride * 6, 16) >> 4;
                regs->av1d_paras.reg70_y_virstride = (y_virstride + uv_virstride) >> 4;
            } else {
                regs->ctrl_regs.reg9.fbc_e = 0;
                regs->av1d_paras.reg68_hor_virstride = hor_virstride >> 4;
                regs->av1d_paras.reg69_raster_uv_hor_virstride = hor_virstride >> 4;
                regs->av1d_paras.reg70_y_virstride = y_virstride >> 4;
            }
            /* error */
            regs->av1d_paras.reg80_error_ref_hor_virstride = regs->av1d_paras.reg68_hor_virstride;
            regs->av1d_paras.reg81_error_ref_raster_uv_hor_virstride = regs->av1d_paras.reg69_raster_uv_hor_virstride;
            regs->av1d_paras.reg82_error_ref_virstride = regs->av1d_paras.reg70_y_virstride;
        }

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; ++i) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_FRAME_PTR, &mframe);
                if (mframe) {
                    hor_virstride = mpp_frame_get_hor_stride(mframe);
                    ver_virstride = mpp_frame_get_ver_stride(mframe);
                    y_virstride = hor_virstride * ver_virstride;
                    if (MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(mframe))) {
                        hor_virstride = mpp_frame_get_fbc_hdr_stride(mframe) / 4;
                    } else if (MPP_FRAME_FMT_IS_TILE(mpp_frame_get_fmt(mframe))) {
                        hor_virstride = MPP_ALIGN(hor_virstride * 6, 16);
                        y_virstride += y_virstride / 2;
                    }
                    SET_REF_HOR_VIRSTRIDE(regs->av1d_paras, mapped_idx, hor_virstride >> 4);
                    SET_REF_RASTER_UV_HOR_VIRSTRIDE(regs->av1d_paras, mapped_idx, hor_virstride >> 4);
                    SET_REF_VIRSTRIDE(regs->av1d_paras, mapped_idx, y_virstride >> 4);
                }
            }
        }
    }

    /* set reg -> para (ref, fbc, colmv) */
    {
        MppBuffer mbuffer = NULL;
        RK_U32 mapped_idx;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &mbuffer);
        regs->av1d_addrs.reg168_decout_base = mpp_buffer_get_fd(mbuffer);
        regs->av1d_addrs.reg192_payload_st_cur_base = mpp_buffer_get_fd(mbuffer);
        // VDPU383_SET_BUF_PROTECT_VAL(mpp_buffer_get_ptr(mbuffer), mpp_buffer_get_size(mbuffer), 0xbb, 128);

        for (i = 0; i < ALLOWED_REFS_PER_FRAME_EX; i++) {
            mapped_idx = dxva->ref_frame_idx[i];
            if (dxva->frame_refs[mapped_idx].Index != (CHAR)0xff && dxva->frame_refs[mapped_idx].Index != 0x7f) {
                mpp_buf_slot_get_prop(p_hal->slots, dxva->frame_refs[mapped_idx].Index, SLOT_BUFFER, &mbuffer);
                if (ctx->origin_bufs && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
                    origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->frame_refs[mapped_idx].Index);
                    mbuffer = origin_buf->buf[0];
                }
                if (mbuffer) {
                    SET_REF_BASE(regs->av1d_addrs, mapped_idx, mpp_buffer_get_fd(mbuffer));
                    SET_FBC_PAYLOAD_REF_BASE(regs->av1d_addrs, mapped_idx, mpp_buffer_get_fd(mbuffer));
                }
            }
        }

        HalBuf *mv_buf = NULL;
        vdpu383_av1d_colmv_setup(p_hal, dxva);
        mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->CurrPic.Index7Bits);
        regs->av1d_addrs.reg216_colmv_cur_base = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_AV1D_VDPU383_DATAS
        memset(mpp_buffer_get_ptr(mv_buf->buf[0]), 0, mpp_buffer_get_size(mv_buf->buf[0]));
#endif
        for (i = 0; i < NUM_REF_FRAMES; i++) {
            if (dxva->frame_refs[i].Index != (CHAR)0xff && dxva->frame_refs[i].Index != 0x7f) {
                mv_buf = hal_bufs_get_buf(ctx->colmv_bufs, dxva->frame_refs[i].Index);
                regs->av1d_addrs.reg217_232_colmv_ref_base[i] = mpp_buffer_get_fd(mv_buf->buf[0]);
#ifdef DUMP_AV1D_VDPU383_DATAS
                {
                    char *cur_fname = "colmv_ref_frame";
                    memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
                    sprintf(dump_cur_fname_path, "%s/%s%d.dat", dump_cur_dir, cur_fname, i);
                    dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                                      8 * mpp_buffer_get_size(mv_buf->buf[0]), 64, 0, 0);
                }
#endif
            }
        }
    }

    {
        vdpu383_av1d_cdf_setup(p_hal, dxva);
        vdpu383_av1d_set_cdf(p_hal, dxva);
    }
    mpp_buffer_sync_end(ctx->bufs);

    {
        //scale down config
        MppBuffer mbuffer = NULL;
        RK_S32 fd = -1;
        MppFrameThumbnailMode thumbnail_mode;

        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits, SLOT_BUFFER, &mbuffer);
        mpp_buf_slot_get_prop(p_hal->slots, dxva->CurrPic.Index7Bits,
                              SLOT_FRAME_PTR, &mframe);
        thumbnail_mode = mpp_frame_get_thumbnail_en(mframe);
        fd = mpp_buffer_get_fd(mbuffer);

        switch (thumbnail_mode) {
        case MPP_FRAME_THUMBNAIL_ONLY:
            regs->common_addr.reg133_scale_down_base = fd;
            origin_buf = hal_bufs_get_buf(ctx->origin_bufs, dxva->CurrPic.Index7Bits);
            fd = mpp_buffer_get_fd(origin_buf->buf[0]);
            regs->av1d_addrs.reg168_decout_base = fd;
            regs->av1d_addrs.reg192_payload_st_cur_base = fd;
            regs->av1d_addrs.reg169_error_ref_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->av1d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_MIXED:
            regs->common_addr.reg133_scale_down_base = fd;
            vdpu383_setup_down_scale(mframe, p_hal->dev, &regs->ctrl_regs,
                                     (void *)&regs->av1d_paras);
            break;
        case MPP_FRAME_THUMBNAIL_NONE:
        default:
            regs->ctrl_regs.reg9.scale_down_en = 0;
            break;
        }
    }

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_start(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    INP_CHECK(ret, NULL == p_hal);
    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __RETURN;
    }

    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *regs = p_hal->fast_mode ?
                              reg_ctx->reg_buf[task->dec.reg_index].regs :
                              reg_ctx->regs;
    MppDev dev = p_hal->dev;
    do {
        MppDevRegWrCfg wr_cfg;
        MppDevRegRdCfg rd_cfg;

        wr_cfg.reg = &regs->ctrl_regs;
        wr_cfg.size = sizeof(regs->ctrl_regs);
        wr_cfg.offset = OFFSET_CTRL_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->common_addr;
        wr_cfg.size = sizeof(regs->common_addr);
        wr_cfg.offset = OFFSET_COMMON_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->av1d_paras;
        wr_cfg.size = sizeof(regs->av1d_paras);
        wr_cfg.offset = OFFSET_AV1D_PARAS_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        wr_cfg.reg = &regs->av1d_addrs;
        wr_cfg.size = sizeof(regs->av1d_addrs);
        wr_cfg.offset = OFFSET_AV1D_ADDR_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_WR, &wr_cfg);
        if (ret) {
            mpp_err_f("set register write failed %d\n", ret);
            break;
        }

        rd_cfg.reg = &regs->ctrl_regs.reg15;
        rd_cfg.size = sizeof(regs->ctrl_regs.reg15);
        rd_cfg.offset = OFFSET_INTERRUPT_REGS;
        ret = mpp_dev_ioctl(dev, MPP_DEV_REG_RD, &rd_cfg);
        if (ret) {
            mpp_err_f("set register read failed %d\n", ret);
            break;
        }

        /* rcb info for sram */
        vdpu383_set_rcbinfo(dev, (Vdpu383RcbInfo*)reg_ctx->rcb_buf_info);

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

MPP_RET vdpu383_av1d_wait(void *hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);
    Vdpu383Av1dRegCtx *reg_ctx = (Vdpu383Av1dRegCtx *)p_hal->reg_ctx;
    Vdpu383Av1dRegSet *p_regs = p_hal->fast_mode ?
                                reg_ctx->reg_buf[task->dec.reg_index].regs :
                                reg_ctx->regs;
#ifdef DUMP_AV1D_VDPU383_DATAS
    {
        char *cur_fname = "colmv_cur_frame.dat";
        DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
        HalBuf *mv_buf = NULL;
        mv_buf = hal_bufs_get_buf(reg_ctx->colmv_bufs, dxva->CurrPic.Index7Bits);
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mv_buf->buf[0]),
                          8 * mpp_buffer_get_size(mv_buf->buf[0]), 64, 0, 0);
    }
    {
        char *cur_fname = "decout.dat";
        MppBuffer mbuffer = NULL;
        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &mbuffer);
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(mbuffer),
                          8 * mpp_buffer_get_size(mbuffer), 128, 0, 0);
    }
#endif

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err) {
        goto __SKIP_HARD;
    }

    // rcb_buf_dump_edge(reg_ctx, p_hal->fast_mode ? reg_ctx->rcb_bufs[task->dec.reg_index] : reg_ctx->rcb_bufs[0]);
    // {
    //     MppBuffer mbuffer = NULL;
    //     mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_BUFFER, &mbuffer);
    //     VDPU383_DUMP_BUF_PROTECT_VAL(mpp_buffer_get_ptr(mbuffer), mpp_buffer_get_size(mbuffer), 128);
    // }

    ret = mpp_dev_ioctl(p_hal->dev, MPP_DEV_CMD_POLL, NULL);
    if (ret)
        mpp_err_f("poll cmd failed %d\n", ret);
#ifdef DUMP_AV1D_VDPU383_DATAS
    {
        char *cur_fname = "cabac_cdf_out.dat";
        HalBuf *cdf_buf = NULL;
        DXVA_PicParams_AV1 *dxva = (DXVA_PicParams_AV1*)task->dec.syntax.data;
        memset(dump_cur_fname_path, 0, sizeof(dump_cur_fname_path));
        sprintf(dump_cur_fname_path, "%s/%s", dump_cur_dir, cur_fname);
        cdf_buf = hal_bufs_get_buf(reg_ctx->cdf_segid_bufs, dxva->CurrPic.Index7Bits);
        dump_data_to_file(dump_cur_fname_path, (void *)mpp_buffer_get_ptr(cdf_buf->buf[0]),
                          (NON_COEF_CDF_SIZE + COEF_CDF_SIZE) * 8, 128, 0, 0);
    }
#endif

    if (task->dec.flags.parse_err ||
        task->dec.flags.ref_err ||
        (!p_regs->ctrl_regs.reg15.rkvdec_frame_rdy_sta) ||
        p_regs->ctrl_regs.reg15.rkvdec_strm_error_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_core_timeout_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_ip_timeout_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_bus_error_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_buffer_empty_sta ||
        p_regs->ctrl_regs.reg15.rkvdec_colmv_ref_error_sta) {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(p_hal->slots, task->dec.output, SLOT_FRAME_PTR, &mframe);
        mpp_frame_set_errinfo(mframe, 1);
    }

__SKIP_HARD:
    if (p_hal->fast_mode)
        reg_ctx->reg_buf[task->dec.reg_index].valid = 0;

    (void)task;
__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_reset(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);


__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_flush(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;

    INP_CHECK(ret, NULL == p_hal);

__RETURN:
    return ret = MPP_OK;
}

MPP_RET vdpu383_av1d_control(void *hal, MpiCmd cmd_type, void *param)
{
    Av1dHalCtx *p_hal = (Av1dHalCtx *)hal;
    MPP_RET ret = MPP_ERR_UNKNOW;

    INP_CHECK(ret, NULL == p_hal);

    switch ((MpiCmd)cmd_type) {
    case MPP_DEC_SET_FRAME_INFO : {
        MppFrameFormat fmt = mpp_frame_get_fmt((MppFrame)param);
        RK_U32 imgwidth = mpp_frame_get_width((MppFrame)param);
        RK_U32 imgheight = mpp_frame_get_height((MppFrame)param);

        AV1D_DBG(AV1D_DBG_LOG, "control info: fmt %d, w %d, h %d\n", fmt, imgwidth, imgheight);
        if ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV422SP) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_LEN_ALIGN, rkv_len_align_422);
        }
        if (MPP_FRAME_FMT_IS_FBC(fmt)) {
            vdpu383_afbc_align_calc(p_hal->slots, (MppFrame)param, 16);
        } else if (imgwidth > 1920 || imgheight > 1088) {
            mpp_slots_set_prop(p_hal->slots, SLOTS_HOR_ALIGN, mpp_align_128_odd_plus_64);
        }
        break;
    }
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO: {
        vdpu383_update_thumbnail_frame_info((MppFrame)param);
    } break;
    case MPP_DEC_SET_OUTPUT_FORMAT : {
    } break;
    default : {
    } break;
    }

__RETURN:
    return ret = MPP_OK;
}

const MppHalApi hal_av1d_vdpu383 = {
    .name       = "av1d_vdpu383",
    .type       = MPP_CTX_DEC,
    .coding     = MPP_VIDEO_CodingAV1,
    .ctx_size   = sizeof(Vdpu383Av1dRegCtx),
    .flag       = 0,
    .init       = vdpu383_av1d_init,
    .deinit     = vdpu383_av1d_deinit,
    .reg_gen    = vdpu383_av1d_gen_regs,
    .start      = vdpu383_av1d_start,
    .wait       = vdpu383_av1d_wait,
    .reset      = vdpu383_av1d_reset,
    .flush      = vdpu383_av1d_flush,
    .control    = vdpu383_av1d_control,
};
