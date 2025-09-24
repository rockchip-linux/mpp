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

// #include "av1.h"
#include "hal_av1d_common.h"
#include "vdpu383_av1d.h"
#include "vdpu383_com.h"

#include "av1d_syntax.h"
#include "film_grain_noise_table.h"
#include "av1d_common.h"

#define VDPU383_UNCMPS_HEADER_SIZE            (MPP_ALIGN(5159, 128) / 8) // byte, 5159 bit

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

typedef struct vcpu383_ref_info_t {
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

typedef struct VdpuAv1dRegCtx_t {
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

    FilmGrainMemory fgsmem;

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

static RK_U32 g_default_prob[7400] = {
    0x000052ce, 0x90000000, 0x000003e2, 0x3b000000, 0x0013e5db, 0x00000000, 0x00000000, 0x20000000,
    0x0e2d3544, 0x80000252, 0x11938c99, 0x16000042, 0x0ad1c0cf, 0x95800004, 0x6c0db50d, 0x20000000,
    0x571fc313, 0x02f5c801, 0x7c7b593f, 0x62112a31, 0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0xea6ee10b, 0xd5074f9b, 0x94f16a25, 0x761fc65f, 0x0000000b, 0x00000000, 0x00000000, 0x00000000,
    0x6615eada, 0x85ff113e, 0xfd02628a, 0x8b324070, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0xb393759a, 0x35d09796, 0xf0f9b25f, 0x3d269662, 0x00000006, 0x00000000, 0x00000000, 0x00000000,
    0x172437e2, 0xd280296b, 0xc474391b, 0xc8143c32, 0x00000004, 0x00000000, 0x00000000, 0x00000000,
    0x2e4e61f7, 0x43cc143a, 0xe4cdd1bc, 0x8424ee5a, 0x0000000e, 0x00000000, 0x00000000, 0x00000000,
    0xae67e881, 0xf404162c, 0x04c279cb, 0xd128f459, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x3bb07a8e, 0xb2075d45, 0xc06730e7, 0x65142e2d, 0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x95eab157, 0x31904976, 0x104e58b7, 0x140ea823, 0x00000004, 0x00000000, 0x00000000, 0x00000000,
    0xf15f65b4, 0x625b16b9, 0x5c84e91a, 0x6818843b, 0x0000000a, 0x00000000, 0x00000000, 0x00000000,
    0x3108e8c7, 0x31feb7d9, 0x405e58e4, 0x12134c2a, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x7da27c9a, 0xe0c45e8c, 0x04255055, 0xb406ee10, 0x00000001, 0x00000000, 0x00000000, 0x00000000,
    0xc8e29305, 0x50238423, 0x0404a80e, 0x31000002, 0xed325966, 0x22505897, 0x06340d70, 0x10000000,
    0xb2f66acb, 0xd05cb90b, 0xe8076821, 0x39000002, 0xdb3e1d7d, 0x0e60281e, 0x01a004b8, 0x10000000,
    0xf095ea0a, 0xf718f048, 0xac895935, 0x3e000003, 0xf8dc6548, 0xae43d329, 0x06a03be8, 0x10000000,
    0x485c9479, 0x31e4c3f3, 0x6403c817, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x10000000,
    0x20004000, 0xd0893000, 0x4f7103f7, 0x415e3802, 0x00000060, 0x00000000, 0x00000000, 0x30000000,
    0xc1441200, 0x00000016, 0x6c144120, 0xbd000001, 0x00000008, 0x00000000, 0x00000000, 0x20000000,
    0xc1441200, 0x00000016, 0x6c144120, 0x00000001, 0x16c14412, 0x20000000, 0x016c1441, 0x20000000,
    0x9f757cda, 0xf30acc49, 0x41ed9ecd, 0x91f3da2f, 0x0004dcbe, 0x00000000, 0x00000000, 0x30000000,
    0x99a0ef74, 0xbbc6c189, 0xe01b49f3, 0x05a9f5f1, 0x447d2e17, 0x00013411, 0x00000000, 0x30000000,
    0x510e9734, 0xbace1440, 0xabda974a, 0xb5b865df, 0x00141ae2, 0x00000000, 0x00000000, 0x30000000,
    0x92c86b5c, 0x3e1ce0f8, 0x20ef048a, 0xaa88b5cf, 0x00000017, 0x00000000, 0x00000000, 0x30000000,
    0x5926ecae, 0x7b6081fd, 0x84356a85, 0x688991e8, 0x00000014, 0x00000000, 0x00000000, 0x30000000,
    0x9e717745, 0x9f4e221c, 0x00473c4b, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x30000000,
    0x2500e1b0, 0xc762f090, 0xd57a3352, 0x1600005b, 0x59da0256, 0x2194e46b, 0x3f30eeaa, 0x10000000,
    0x1881d658, 0x94d0caeb, 0xb0e6aa11, 0x0e000043, 0x025ed44c, 0xe154314c, 0x6328dbc1, 0x10000000,
    0x9271b628, 0x629b0759, 0x688cf130, 0x1b00001f, 0x591de956, 0x17d4db8c, 0x6908ed82, 0x10000000,
    0x9097bd1b, 0xa29386ca, 0x007f011c, 0x0a000022, 0x11d2a94d, 0x4d535548, 0x35b88731, 0x10000000,
    0x5f85221d, 0x2be7d105, 0x076de504, 0x00f7c5de, 0x00000000, 0x00000000, 0x00000000, 0x30000000,
    0x10c2225a, 0x82064e92, 0x41916a10, 0xff402133, 0x00000035, 0x00000000, 0x00000000, 0x30000000,
    0xd21fa6ef, 0xf3e98846, 0x48cfa9bf, 0xd729cc60, 0x02c6ea12, 0x00012e63, 0x00000000, 0x00000000,
    0x593db70f, 0x249d49e6, 0x24fdba27, 0x082a3871, 0xb805fd92, 0x0000ef82, 0x00000000, 0x00000000,
    0x1706b2c6, 0xf44fa927, 0x40fd9214, 0x4a2c7e77, 0xf786a813, 0x000107a2, 0x00000000, 0x00000000,
    0x9665b145, 0x34aec9b2, 0xa5209a52, 0xf83e388d, 0x9e0c189c, 0x00024de5, 0x00000000, 0x00000000,
    0x9ebec31c, 0x6620cd1d, 0xfd6c52f3, 0x834a68aa, 0x96c8fda1, 0x00011fc3, 0x00000000, 0x00000000,
    0x1cb75110, 0x16162cf1, 0x0161e2f1, 0x8e4c10aa, 0x89080720, 0x0000a9e2, 0x00000000, 0x00000000,
    0x2af6d8bc, 0x14df8a3a, 0xed29ca5f, 0xd037d886, 0xc5c6b419, 0x00009622, 0x00000000, 0x00000000,
    0xa1e948f5, 0x06d10ff1, 0x899d3354, 0x565360c7, 0xc486379f, 0x00002641, 0x00000000, 0x00000000,
    0x6617d0a6, 0xf7f170c3, 0x258dfb74, 0xbb3822a2, 0xa7c4e598, 0x000024a1, 0x00000000, 0x00000000,
    0x19bfd8d9, 0x85880ba4, 0x3d38c2ab, 0xdb445297, 0x3b87551c, 0x0000a682, 0x00000000, 0x00000000,
    0x10eee8a1, 0x63d94804, 0x1cd9e9e1, 0x2631926b, 0x75059794, 0x000079c1, 0x00000000, 0x00000000,
    0xe709e2ec, 0xb5e10c4b, 0x995ccada, 0x6b4926a5, 0x97897aa0, 0x00011c83, 0x00000000, 0x00000000,
    0x234fdea7, 0x07a830ee, 0x61c823c0, 0xb7651ee0, 0xa2070f9f, 0x000045c1, 0x00000000, 0x00000000,
    0x2434e289, 0xf817b0de, 0x81789ba0, 0x6f4078a8, 0xbec60a9a, 0x000060c1, 0x00000000, 0x00000000,
    0x6574cebb, 0x85418b1e, 0x5d3e7a8d, 0x7f3c4292, 0x67c64f1b, 0x00005662, 0x00000000, 0x00000000,
    0xa5b15a29, 0xc69c4ded, 0xa58a6b36, 0x4d53a4bc, 0x7949d0a5, 0x0000e8a3, 0x00000000, 0x00000000,
    0x334be7d9, 0x232e8696, 0xd8c5f190, 0x75227c58, 0xed847e90, 0x00005541, 0x00000000, 0x00000000,
    0xa9155622, 0x37971165, 0xfdcfe3b6, 0xba57dadd, 0x6c4768a2, 0x00004082, 0x00000000, 0x00000000,
    0xecbf5c16, 0x87922fe2, 0x15a8eb80, 0x3f352eaf, 0xfbc54418, 0x00002621, 0x00000000, 0x00000000,
    0x23e0cec6, 0x670670e0, 0xd1a46b6a, 0x1c58aacb, 0xcec6001e, 0x00002ce1, 0x00000000, 0x00000000,
    0x2501dff5, 0x17a5f1d1, 0x71c96bbe, 0x7767e8e0, 0x9286699d, 0x00003541, 0x00000000, 0x00000000,
    0x2cfedefd, 0x4762b159, 0x09c03b9a, 0xe357bcd6, 0xa087211f, 0x00005dc2, 0x00000000, 0x00000000,
    0x2e5461de, 0xf7dcb6b5, 0xfdecf3e4, 0x896d62f3, 0x3404a597, 0x00001e01, 0x00000000, 0x00000000,
    0x2bec5d75, 0x488574c6, 0x71cf4bef, 0x574e84d5, 0xbe457f9b, 0x00002361, 0x00000000, 0x00000000,
    0x654ccede, 0xd7c7d065, 0xcd727b57, 0x95392e98, 0x9484fd19, 0x00002461, 0x00000000, 0x00000000,
    0x25635aa5, 0x483d9146, 0x05740baa, 0xdc42a8a5, 0x9646159c, 0x00003d61, 0x00000000, 0x00000000,
    0x2e9ddf5c, 0xa7160ecc, 0xe98bb346, 0x9e354a9e, 0x31459a18, 0x00003f62, 0x00000000, 0x00000000,
    0x28ecd5ad, 0x682ad32f, 0xf5cc4bd8, 0xd94e3ed7, 0xd746679f, 0x000023c1, 0x00000000, 0x00000000,
    0x6fd0623e, 0x6ade765f, 0x25b8fc3e, 0x041f309a, 0xf182c28e, 0x000015e0, 0x00000000, 0x00000000,
    0x762c777c, 0x34e1189c, 0x004fb969, 0x03000000, 0x67f50c77, 0x6a248837, 0x00004c31, 0x10000000,
    0x6a7b713c, 0x568cd29f, 0x002e525d, 0x1b000000, 0x356a136e, 0xab67a1d1, 0x0000889a, 0x10000000,
    0xeaa8f937, 0x766f91d4, 0x007b4a72, 0x98000000, 0xceec2075, 0xba0761d3, 0x0000470a, 0x10000000,
    0xea547740, 0xf61170e8, 0x00785a7d, 0xeb000000, 0xd56ba271, 0xd0475e93, 0x0000420a, 0x10000000,
    0x90d42799, 0xe3786737, 0x44c341a7, 0xac25da59, 0xc8c54a91, 0x000064e1, 0x00000000, 0x00000000,
    0xcb7fdad7, 0x22cb45a8, 0x949f8161, 0x3926a04e, 0xe6065f91, 0x00007982, 0x00000000, 0x00000000,
    0x6ca2d98b, 0xb1fc0402, 0xc87ca0fa, 0x62170437, 0xf7c3ea0b, 0x00005c60, 0x00000000, 0x00000000,
    0x64dbcab9, 0xa3a1b229, 0x7cdc09bf, 0x5f2be86a, 0x61443610, 0x00002261, 0x00000000, 0x00000000,
    0xaccad9dc, 0x6b23d64d, 0x28b299cd, 0xad24184b, 0x00034191, 0x000032a1, 0x00000000, 0x00000000,
    0xe575d1cf, 0x894d12a4, 0x2c6f1c53, 0xf119dc36, 0x4bc3238b, 0x00001b41, 0x00000000, 0x00000000,
    0x23e0c8a1, 0xa7c96fc8, 0xe5d48bac, 0xdb141041, 0x6a81d189, 0x00000ca0, 0x00000000, 0x00000000,
    0xa6894dab, 0x7863515f, 0x4213e42c, 0xad1c7103, 0xc9c4268d, 0x00002660, 0x00000000, 0x00000000,
    0xa551d106, 0xe886f273, 0x16149437, 0xe580df06, 0xf444a610, 0x00002c01, 0x00000000, 0x00000000,
    0x59403594, 0x25c98be7, 0x0d612ad6, 0x7a4e82aa, 0x5547a1a5, 0x00004a22, 0x00000000, 0x00000000,
    0x999fb73d, 0xc5a3ab84, 0x255642c0, 0x194b62a5, 0x56cb29a4, 0x00004d81, 0x00000000, 0x00000000,
    0xdae23d7e, 0x6611ac78, 0x6d6c8af9, 0x2e54dab2, 0x14cd6c28, 0x000076a6, 0x00000000, 0x00000000,
    0x6520d0a4, 0x18ae1164, 0x6a29ac56, 0x69898514, 0x56613bc4, 0x0007e450, 0x00000000, 0x00000000,
    0x2a1c5759, 0x19907367, 0x36450ca0, 0x8e85b117, 0x7f58c541, 0x16c5092b, 0x00000002, 0x00000000,
    0xd95e6e4c, 0x46212c69, 0xc56d8b03, 0xad586ab4, 0xe2d0ae29, 0xce02cd07, 0x00000000, 0x00000000,
    0xf57eeb67, 0xf61a4c4b, 0xad814b05, 0xf25682b8, 0x5012cf2a, 0x7c93f028, 0x00000001, 0x00000000,
    0xb200e5ac, 0x89171884, 0x1a31fc73, 0x3d7d3715, 0x5095f838, 0x18a4946a, 0x00000002, 0x00000000,
    0x35846c85, 0x6d099a36, 0x7d852b55, 0x755226aa, 0x370f58a8, 0x8f834aa7, 0x00000001, 0x00000000,
    0x728beb06, 0x6c63f8ec, 0x996c35ae, 0x23560eb1, 0x358f9a2a, 0x5d830d67, 0x00000001, 0x00000000,
    0x364fed30, 0x3c6c58f7, 0xdee8f5dc, 0xbd5e6cc4, 0xcb52b62e, 0xe9440f28, 0x00000001, 0x00000000,
    0xf2d66630, 0x7b94f7ab, 0xe6dba5bd, 0x8d6ceb67, 0x57146035, 0xf2744c89, 0x00000001, 0x00000000,
    0xf182e892, 0x9ba37866, 0x9ed59dbd, 0x2eae9f67, 0x67541136, 0xa7b3e889, 0x00000001, 0x00000000,
    0xa9ef565c, 0x69ac73be, 0xa25644bf, 0x528ac924, 0xb8d78b43, 0xee348daa, 0x00000001, 0x00000000,
    0x297b56e6, 0xe9abb3be, 0x6e516cbb, 0xe288fb21, 0xd618bd41, 0xb1246449, 0x00000001, 0x00000000,
    0x6b1ed88d, 0x69a0f3a8, 0x4e54fcbb, 0xec898b23, 0x01999242, 0xd884842c, 0x00000001, 0x00000000,
    0xb610f3b8, 0x7c5618ca, 0x1f0fb625, 0xd8c10186, 0x5b6df05f, 0xe88adcf6, 0x00000000, 0x00000000,
    0x3bdafa76, 0x470372f9, 0x688a496a, 0x00000007, 0x00000000, 0x00000000, 0x00000000, 0x10000000,
    0xd788e22b, 0xf023e155, 0x30028006, 0x44009001, 0x0f002000, 0x03400700, 0x00b00180, 0x00000000,
    0x51e6c7e3, 0x90c80486, 0x640ba825, 0x94018604, 0x19003c80, 0x05c00c00, 0x015002c0, 0x00000000,
    0xd44a52f4, 0x30b00438, 0xe007a81a, 0x5f00ee02, 0x15c02d80, 0x04f00a60, 0x011c0258, 0x00000000,
    0x82ab1692, 0xc018a079, 0x10024004, 0x3c008001, 0x0d001c00, 0x02c00600, 0x00900140, 0x00000000,
    0x0d1b3ca0, 0x710e03c6, 0x081bd85b, 0x76037e0a, 0x3f009801, 0x0c001a00, 0x02480578, 0x00000000,
    0x55a9466e, 0x026066d3, 0xf04d10ea, 0x70073a1b, 0x3e00b182, 0x092015c0, 0x01b00380, 0x00000000,
    0xf2d4edf3, 0x9c28da3e, 0x22da74ef, 0x2c90473e, 0x00145cce, 0x00080010, 0x00000000, 0x30000000,
    0x20004000, 0xe8001000, 0xeebc04e1, 0x007296c3, 0x00200040, 0x00000000, 0x00000000, 0x30000000,
    0x670c5d0b, 0x01914f31, 0x02a00700, 0x000000d2, 0x00000040, 0x00000000, 0x00000004, 0x20000000,
    0x83801000, 0x903800e3, 0x7003800d, 0x06001600, 0x00000080, 0x00000000, 0x00000000, 0x00000000,
    0x83801000, 0x903800e3, 0x7003800d, 0x06001600, 0x00000080, 0x00000000, 0x00000000, 0x00000000,
    0x0a001400, 0x07400f00, 0x01800360, 0x004000a0, 0xc0058010, 0x00010002, 0x00000000, 0x30000000,
    0x1d003c00, 0x06000d80, 0x01000280, 0x00160040, 0x0004000b, 0x00000000, 0x00000000, 0x30000000,
    0x10004000, 0x00000600, 0x01680500, 0x00000087, 0x00100040, 0x00000006, 0x87016805, 0x20000000,
    0x18003000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x30000000,
    0x1e006000, 0x00000b40, 0x01e00600, 0x000000b4, 0x00000040, 0x00000000, 0x00000004, 0x20000000,
    0x2a007000, 0x00000d20, 0x00000400, 0x00000000, 0x00000040, 0x00000000, 0x00000000, 0x20000000,
    0x83801000, 0x903800e3, 0x7003800d, 0x06001600, 0x00000080, 0x00000000, 0x00000000, 0x00000000,
    0x83801000, 0x903800e3, 0x7003800d, 0x06001600, 0x00000080, 0x00000000, 0x00000000, 0x00000000,
    0x0a001400, 0x07400f00, 0x01800360, 0x004000a0, 0xc0058010, 0x00010002, 0x00000000, 0x30000000,
    0x1d003c00, 0x06000d80, 0x01000280, 0x00160040, 0x0004000b, 0x00000000, 0x00000000, 0x30000000,
    0x10004000, 0x00000600, 0x01680500, 0x00000087, 0x00100040, 0x00000006, 0x87016805, 0x20000000,
    0x18003000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x30000000,
    0x1e006000, 0x00000b40, 0x01e00600, 0x000000b4, 0x00000040, 0x00000000, 0x00000004, 0x20000000,
    0x000016f9, 0x30000000, 0x00000143, 0xe3000000, 0x00000009, 0x00000000, 0x00000000, 0x20000000,
    0x2a5b78ad, 0x7000054f, 0xb2c7a765, 0x72000063, 0xaea977ef, 0x00000006, 0x00000000, 0x20000000,
    0x0f41b1bc, 0x428ec5ec, 0x2067e0c8, 0x0080005c, 0x00200040, 0x00080010, 0x00000000, 0x30000000,
    0x20004000, 0x08001000, 0x02000400, 0x65ae7700, 0xfa2dd5db, 0x0008f52e, 0x00000000, 0x30000000,
    0x4d93c4da, 0x0537a9b6, 0xe036d0a5, 0x00010412, 0xe4200040, 0x0005cb68, 0x00000000, 0x30000000,
    0x4b80a238, 0x2991a91e, 0xd2ceb533, 0xe4bfaf71, 0x00200067, 0x00080010, 0x00000000, 0x30000000,
    0x20004000, 0x08001000, 0xae8ea400, 0x00800187, 0x00000040, 0x00000000, 0x00000000, 0x30000000,
    0x0fa4621d, 0xe0000000, 0x00fa1ed7, 0xf1000000, 0x000e246a, 0x42500000, 0x0000ba33, 0x20000000,
    0x11e56bfd, 0xa0000000, 0x0108652a, 0x6c000000, 0x00072219, 0x07800000, 0x00009313, 0x20000000,
    0x0b992b51, 0x40000000, 0x004098cb, 0x1e000000, 0x0004a18f, 0x76200000, 0x00002cc8, 0x20000000,
    0x00690105, 0x50000000, 0x01555d55, 0x55000000, 0x001555d5, 0xf8100000, 0x00002b10, 0x20000000,
    0x03e518c1, 0x30000000, 0x0024b0ed, 0xd2000000, 0x00030e8b, 0x00000000, 0x00000000, 0x20000000,
    0x7754767a, 0x3bc5398e, 0xe66db550, 0x7972a509, 0x70545330, 0x39837348, 0x18f467b1, 0x00000000,
    0xf99f7cda, 0xecb5da22, 0x9719ce3c, 0x3d8e0986, 0xfb1e14c0, 0x6c96844d, 0x310ccf8a, 0x00000000,
    0x78b37525, 0x1c7a7b6d, 0xc7057e1a, 0xed8fd778, 0x0e165639, 0xcf029648, 0x116045f8, 0x00000000,
    0x78ebf96c, 0xac879a75, 0x8e9fb5ba, 0x257a111e, 0xa7170fb5, 0x7a140d29, 0x1d8c7eb1, 0x00000000,
    0xf92efb8a, 0x7c6dd9de, 0x06faee06, 0xf7959173, 0x4f9f5343, 0x807691ee, 0x3248d322, 0x00000000,
    0x77cef54a, 0xcc13ba68, 0x7adc45ce, 0x8a91a961, 0x1d97e5bc, 0x2d835189, 0x152063f1, 0x00000000,
    0xf84a7941, 0xfc319a0a, 0xaa7e4598, 0x626c0109, 0x9954f52f, 0x6943a428, 0x1b707d11, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0xfe12ff66, 0x8effbe1e, 0x67be0f7d, 0x694c0fde, 0x99d09223, 0x58537587, 0x1d047969, 0x00000000,
    0xfd6b7b91, 0x8eb6be8a, 0xbbaa7f57, 0x695bd1d4, 0x634f0fa6, 0xa771c165, 0x0d0c3880, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x38007800, 0x0c001a00, 0x02800580, 0x00800120, 0x00180038, 0x8004000a, 0x20008001, 0x00000000,
    0x134a9811, 0x23fdcd9a, 0xf13b7322, 0x739b7ee2, 0xe2a61ae4, 0x000dbb96, 0x00000000, 0x30000000,
    0x00180341, 0x00000000, 0x00018ea4, 0x5a000000, 0x003a437e, 0x1ac00000, 0x00000501, 0x20000000,
    0x0197861a, 0x20000000, 0x00143ed0, 0xfe000000, 0x003dc07c, 0xe6700000, 0x0000deda, 0x20000000,
    0x0016035a, 0x80000000, 0x0001c6fe, 0xcf000000, 0x003b9cfe, 0x4f500000, 0x00000421, 0x20000000,
    0x017b86d2, 0xd0000000, 0x00151f46, 0xa7000000, 0x003e28fd, 0x58700000, 0x00016354, 0x20000000,
    0x516d105b, 0xc420aba4, 0x6684db8e, 0x03aaf2da, 0x770aead5, 0x000a7dec, 0x00000000, 0x30000000,
    0x1a4a184b, 0x6252321c, 0xee3e02bd, 0x2850fe47, 0x80190041, 0x0004200c, 0x00000000, 0x30000000,
    0x05125010, 0x00000000, 0x00512501, 0x0b000000, 0x0003c037, 0x00000000, 0x00000000, 0x20000000,
    0x225a4d46, 0x60000000, 0x0225a4d4, 0x12000000, 0x000dff21, 0x00000000, 0x00000000, 0x20000000,
    0xf04efa01, 0x59c636c2, 0x009a7a45, 0xcc000000, 0x84f97cfd, 0x921ab156, 0x00009213, 0x10000000,
    0x2996e96a, 0xa0000000, 0x02996e96, 0x5d000000, 0x00138cbe, 0x00000000, 0x00000000, 0x20000000,
    0x39a8fd60, 0x9a265c69, 0x012d4b42, 0x61000000, 0xe064e1eb, 0x41973090, 0x00004ea2, 0x10000000,
    0xf9528444, 0x506b1ec4, 0xc7e1674d, 0xf3e5700e, 0x85c1717c, 0x000f8f1c, 0x00000000, 0x30000000,
    0xf4236eb9, 0x3aea1833, 0x006c71ba, 0x32000000, 0xc337b7fa, 0xfb8acb56, 0x00009e93, 0x10000000,
    0xb1ab81cb, 0x103efe92, 0xfbd07f05, 0x7fc1ec04, 0x040099ff, 0x0000000b, 0x00000000, 0x30000000,
    0xba757e49, 0x6cb5fc8f, 0x0119b397, 0x5a000000, 0x4277bbfb, 0x0c7a4d5b, 0x0001063b, 0x10000000,
    0xe69c60f0, 0x95834e46, 0x006e39c4, 0x1d000000, 0x2f69b1e4, 0x38e67510, 0x000092ba, 0x10000000,
    0x77947beb, 0xcb0a378e, 0x006b4b72, 0x36000000, 0x5d78b57f, 0xd19c6fdb, 0x00011183, 0x10000000,
    0xe71de194, 0x55fd8f14, 0x00847210, 0xb1000000, 0x2ea4885f, 0xe115788e, 0x00008459, 0x10000000,
    0x77947e41, 0x6ba99957, 0x01167417, 0xeb000000, 0xde7777fe, 0xfafba6fa, 0x0001480b, 0x10000000,
    0x5a8a4e4b, 0x13df2aa6, 0x0052c147, 0x11000000, 0x7962ec5a, 0xbc35b88f, 0x000098e9, 0x10000000,
    0x756272af, 0x986235a6, 0x0110c331, 0xb2000000, 0xcfa52978, 0x3267f1d1, 0x00009e72, 0x10000000,
    0x9761c5a4, 0x7430cad4, 0x0075c966, 0xf7000000, 0x8218fadd, 0x59d18ae5, 0x00000f80, 0x10000000,
    0x2ed27eba, 0x88a331b6, 0x00a5134b, 0x1c000000, 0x7831347e, 0x5298f2b8, 0x00013e0b, 0x10000000,
    0xe195e931, 0x82d7a8f8, 0x002160b6, 0xb6000000, 0x6c6a026e, 0xaf9582ae, 0x00006851, 0x10000000,
    0x620a7b9a, 0x07b0307a, 0x004dcae6, 0x71000000, 0xdff6857d, 0x9add1cfa, 0x0000aff2, 0x10000000,
    0xed847364, 0x572af15b, 0x00a72280, 0x48000000, 0x376ea571, 0x82174893, 0x0000adca, 0x10000000,
    0xf3637aed, 0xdb76d752, 0x00e33c5f, 0xc9000000, 0x9435a8fe, 0x115ca37a, 0x00013b84, 0x10000000,
    0xef7ef660, 0xb5701370, 0x0083a9c4, 0x0b000000, 0xcff562fb, 0xb3c6bdb5, 0x0000d7ba, 0x10000000,
    0xb042fc8d, 0x7a80d7ec, 0x015e9bc7, 0x1b000000, 0xe6b1327d, 0xed0af2f6, 0x00007453, 0x10000000,
    0xe0000fda, 0xd2cc95b1, 0x0072f848, 0xb7bbef00, 0x0002430d, 0x00000000, 0x00000000, 0x30000000,
    0x71897f92, 0x7b82b845, 0x0126fc15, 0x95000000, 0xdcf06d7e, 0x2f3ac9f6, 0x00010334, 0x10000000,
    0x0473131b, 0x40000000, 0x00dcfd2f, 0x70000000, 0x0004f366, 0x61600000, 0x000092c2, 0x20000000,
    0x3110ff67, 0x5b261814, 0x018f2443, 0x49000000, 0x18f3a272, 0x48588335, 0x00011eb3, 0x10000000,
    0x00bf8355, 0x70000000, 0x007139d5, 0x01000000, 0x000ab1d0, 0x48700000, 0x00009646, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x0e61b14f, 0x00000000, 0x00234887, 0x1c000000, 0xa8c9391c, 0xafa00002, 0x56215d55, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x0b95e3b0, 0x100002e0, 0x0d02d352, 0x58000045, 0x8181d785, 0x16e00000, 0x2cc4aeca, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x94475960, 0x3000056c, 0x4cd1de8f, 0x72000036, 0x54d4bc38, 0x90500004, 0x0d283300, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x0b5a9f35, 0x3136c418, 0xdd83a5e2, 0x61248687, 0x4fca29ef, 0x0000d4a3, 0x00000000, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x11c33268, 0x2194c5e9, 0xb41b5846, 0x0002ac09, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0xce3ca63c, 0x315bc4de, 0x956305f2, 0x2f1e6c7b, 0x1f867bf7, 0x00009242, 0x00000000, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x5697350b, 0xc20686b1, 0x9c16a039, 0x0001f407, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x663334cc, 0xcce666e6, 0x0003334c, 0x00000000, 0x20000000,
    0x8e60a5a4, 0x721da5a9, 0x000000a8, 0xed000000, 0xc4de5362, 0x2b23e8cb, 0x00000001, 0x10000000,
    0xe666e666, 0x63334ccc, 0xce666e66, 0x993334cc, 0xc326f97b, 0x000294a9, 0x00000000, 0x20000000,
    0x0f716764, 0x11f205b0, 0x00000083, 0x1c000000, 0x535348b6, 0xc2c29aa7, 0x00000000, 0x10000000,
    0x32cbff6f, 0x62fab44d, 0x8b480fe9, 0x7f7d5749, 0xd96249fe, 0x0001036d, 0x00000000, 0x20000000,
    0x026105da, 0xf058a0f0, 0x00000019, 0x37000000, 0xfcd004a9, 0x8ac1fea5, 0x00000000, 0x10000000,
    0xba4f7fe7, 0x42602968, 0x6f7457fc, 0xdb449348, 0xd4797e7f, 0x00058711, 0x00000000, 0x20000000,
    0x1ff56375, 0x83f90cd3, 0x0000010f, 0xa7000000, 0xc04f0c6e, 0x7391de45, 0x00000000, 0x10000000,
    0x33a87f63, 0x977c3345, 0x5b2e0ff8, 0xd11a5936, 0xb8352fff, 0x000580b3, 0x00000000, 0x20000000,
    0x57e83cc6, 0x22f0493a, 0x000000c7, 0x2e000000, 0x57038b88, 0x23108121, 0x00000000, 0x10000000,
    0xb4227ef3, 0xd4b6b501, 0x7f508ffa, 0x50796944, 0x42752678, 0x0006f1b4, 0x00000000, 0x20000000,
    0x0ec8a5bf, 0xd27b8628, 0x004c90ef, 0xda000000, 0x7e21d4e4, 0xc214eced, 0x00008369, 0x10000000,
    0x75246e96, 0xeb42787c, 0xfe31dcbe, 0x0061aced, 0xa093602c, 0x3b138fc8, 0x21647869, 0x00000000,
    0x92276278, 0x72b7a71b, 0x004060f0, 0x90000000, 0x1dd44db7, 0x229335a8, 0x00005519, 0x10000000,
    0xbaf9f993, 0x1d363b55, 0x9af29618, 0xf688075b, 0x02d8243a, 0x6143c9ca, 0x1f8c7a41, 0x00000000,
    0x82928622, 0x506ec112, 0x000bc828, 0x09000000, 0x8b12c02d, 0xfdf2d5c7, 0x000046b0, 0x10000000,
    0xfb45fcfe, 0x59b2bae5, 0x1db08c23, 0x1d43ceaf, 0x2fc84918, 0x00000002, 0x00000000, 0x00000000,
    0x22ae666f, 0x1558ee6b, 0x007f19c6, 0x88000000, 0x34cca4f2, 0xa521fc05, 0x00002970, 0x10000000,
    0x00004000, 0x90000000, 0x000006fb, 0x32000000, 0x00000078, 0xd1400000, 0x00000007, 0x20000000,
    0x1c36c1ef, 0x03f0cace, 0x0060316a, 0xf4000000, 0x42430e06, 0x31108141, 0x00000dd8, 0x10000000,
    0x3df8fcb8, 0xed9c3e11, 0x97eb0fe8, 0x00dd13e2, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x515aab47, 0xe322076d, 0x347b2948, 0x14000028, 0x9421f3e5, 0x1215734e, 0x37b0bbfa, 0x10000000,
    0xfefc7e70, 0x2ef95f0b, 0x00000667, 0x2e000000, 0xb77f35ff, 0x28fe605e, 0x00000006, 0x10000000,
    0x92b969ed, 0x230d478b, 0x3069812b, 0x6b00001d, 0x2793e734, 0x47c34ec8, 0x244074c9, 0x10000000,
    0xff077eb7, 0xff211eec, 0x030d9733, 0xb1000000, 0x937e937e, 0xdcdd515e, 0x000269dd, 0x10000000,
    0x02ed06cc, 0x9084e13c, 0xe0133834, 0x3e000005, 0xd292842c, 0x3ff32f47, 0x22747661, 0x10000000,
    0x3f0f7f25, 0x0efabee3, 0x5b43a71a, 0x8d00013b, 0xc6bea2fe, 0xaf9da45e, 0xee9e7775, 0x10000000,
    0x22d5e919, 0xd5e88edc, 0x9cc6aa37, 0xa9000036, 0xc0cd9772, 0xdd925545, 0x11f84a30, 0x10000000,
    0x7edc7eca, 0x8e3f7e28, 0xa2f2ee7e, 0x1a85b94e, 0x12bc637c, 0xc43d439d, 0x9a359283, 0x10003822,
    0x18c639d3, 0xe44b4a4a, 0x28887195, 0x2a000025, 0x3902cf86, 0x3af08821, 0x06f81730, 0x10000000,
    0x7e14fd7f, 0x6d595c61, 0x722c9580, 0x334918da, 0x0000001a, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x7f2dfe77, 0x8f353f44, 0x7f39873a, 0xd573032f, 0x000d8d27, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x36557351, 0x08e3b68a, 0xb7973789, 0x008d9386, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x347e0397, 0xf54a342c, 0x64a57b0b, 0xefc43404, 0x45c57033, 0x00013bd6, 0x00000000, 0x30000000,
    0x37a7f5b4, 0xfa3dd897, 0x00000304, 0x06000000, 0x42b9caf9, 0x592a7479, 0x00000003, 0x10000000,
    0xbcf984c4, 0x17de161b, 0x9cc9e376, 0x50d5ca03, 0x000566b9, 0x00080010, 0x00000000, 0x30000000,
    0xb5a4724f, 0xc9415626, 0x0125b3ae, 0xe5000000, 0x3d3aba79, 0x68eaac3b, 0x00014184, 0x10000000,
    0x75788afb, 0x147e4e62, 0xecd91286, 0x18e27c08, 0x000765c4, 0x00080010, 0x00000000, 0x30000000,
    0x31706b83, 0xf842936c, 0xa114231b, 0xfa000050, 0x44f93877, 0x8be90d1a, 0x5e017c93, 0x10000000,
    0xbc733a00, 0x53a8f8e3, 0x4025f555, 0xf4f55402, 0xdb848142, 0x000d7e9f, 0x00000000, 0x30000000,
    0x39ac7628, 0xfbab5a30, 0x39db54ed, 0x654330c8, 0xbd77ef77, 0xf54ac1b8, 0x6fdd1bfa, 0x10002b06,
    0xbfc5e75c, 0x0ef1fe66, 0x66aaac00, 0x00800027, 0x00200040, 0x00080010, 0x00000000, 0x30000000,
    0x33696c19, 0xf97ad63d, 0x25cdf416, 0x2c33deb9, 0x0000000e, 0x00000000, 0x00000000, 0x00000000,
    0x2000028b, 0x008af000, 0x020c0400, 0x806d0134, 0x2026c044, 0x0000000f, 0x00000000, 0x30000000,
    0xbc1e78b7, 0xbdeddd23, 0x59fb356d, 0x7e2b12b2, 0x0001ec10, 0x00000000, 0x00000000, 0x00000000,
    0xde5cbdbf, 0x0800188e, 0x02000400, 0x00800100, 0xc4a41340, 0x0004324b, 0x00000000, 0x40000000,
    0x82dd2787, 0x98c26090, 0x1c487960, 0xf875b57e, 0xe4fa1924, 0xa009c1d7, 0x8fc37627, 0x80000001,
    0x84a3b36b, 0x4965a128, 0x084ec197, 0x406e857c, 0xe6788b20, 0xa3783355, 0x95d37f5f, 0x80000001,
    0xc5e9b95e, 0x49d980de, 0x6847199f, 0x835b056c, 0x36f3ae16, 0x66159c31, 0x6727395f, 0x80000001,
    0x05d928ae, 0x08cfa15d, 0xbc6f11c8, 0x1a949f97, 0x5ebbf637, 0x000b72da, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x0740ba53, 0x40a54318, 0x881bd078, 0x3e1dd87a, 0x926e8f70, 0x00000013, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x0a2fc288, 0xea86c21d, 0x147c6a30, 0x0b939da3, 0xe577c4b3, 0x4ef937b6, 0x62d73617, 0x80000001,
    0xc4572f45, 0x59a1a0d1, 0xa452519f, 0x366d7780, 0x7675bc9f, 0x4fc6e713, 0x519f1ed7, 0x80000001,
    0x7985f960, 0x18f91b3e, 0x5938e321, 0x3e4a9d06, 0xfba83297, 0xc28455ec, 0xc3761745, 0x80000000,
    0xd9b7cf08, 0x5bbf08c4, 0x999e8c53, 0xb6a7839f, 0x48b6bd43, 0x1259e0d7, 0x5b3b1b5f, 0x80000001,
    0x1d95d5ac, 0x4be98a9d, 0x15b30c79, 0x80a3fb9a, 0x55b593c1, 0x42694816, 0x7bcb48e7, 0x80000001,
    0x2748e464, 0x6d3d0f5f, 0xb23bc573, 0xb4d901d7, 0x5fe84c63, 0xa28589ce, 0x817d787c, 0x80000000,
    0xdebfb04f, 0x29b6f374, 0x0200048f, 0x00800100, 0xa0a3dd40, 0x0004144b, 0x00000000, 0x40000000,
    0x40ba980e, 0xe6ec401b, 0xf014588d, 0x0236c93d, 0xb1b1148b, 0x4473ef4d, 0x3f170357, 0x80000001,
    0x41e5a121, 0x77d82052, 0xbc2180e2, 0xcb47fd56, 0xd2b35e0f, 0x6914bdef, 0x5f6b32f7, 0x80000001,
    0x82a41c8d, 0x07e4608c, 0x003cf127, 0xb94d6f54, 0x1d327e93, 0x5bf51ab0, 0x5b5f2b27, 0x80000001,
    0x01601e0c, 0x073cc02f, 0x201720a5, 0x3e3ccf44, 0x0ab1518d, 0x3554220e, 0x2fb6ed9f, 0x80000001,
    0xc246a740, 0x38b2005d, 0xf42cc922, 0xe8527d67, 0x8a33b012, 0x4a251f70, 0x41eb0bb7, 0x80000001,
    0x4c49e9ab, 0x20852239, 0xc013104c, 0x4809fa30, 0x53ec546e, 0x00000012, 0x00000000, 0x50000000,
    0x442eb069, 0xc936009f, 0xa03ac956, 0xf857956d, 0x62b46795, 0x5955b031, 0x4f271e3f, 0x80000001,
    0x8289225e, 0xa79d6061, 0x141770bd, 0xf3393f38, 0xc82e758a, 0xd8835b6c, 0xf7ea9ca6, 0x80000000,
    0x41ea213f, 0x27c20043, 0x281bc8cf, 0xae3c4545, 0xb8b08a0b, 0x2c93c08d, 0x2deaedbf, 0x80000001,
    0x39577938, 0x28f45b12, 0x5d34f31b, 0xab4d1b0c, 0x16285d17, 0xc9d45fad, 0xc36a1905, 0x80000000,
    0xdb12d1d0, 0x7bf7e96e, 0x71b46c7c, 0x46ac0ba4, 0xb6b742c6, 0x1eaa2617, 0x631726b7, 0x80000001,
    0xd414c582, 0x7a6c263a, 0x8528cb70, 0xd8881575, 0x733207b1, 0x18579053, 0x640b2427, 0x80000001,
    0x2762e4a9, 0xfd398f5a, 0xea393d6f, 0x4ad585d3, 0xebe12be1, 0x85541b2a, 0x4658eb9b, 0x80000000,
    0x9e6b229f, 0xe75c3035, 0xf211f49f, 0x00800143, 0x25eb5ec0, 0x000659b0, 0x00000000, 0x40000000,
    0x40388bf1, 0xe5a9e008, 0x440af855, 0x132b852d, 0xd36ec207, 0xf522844a, 0xf5ca9a2e, 0x80000000,
    0x00e21590, 0x76bea030, 0x4015a890, 0x85448747, 0x3b31a70f, 0x2b53f72e, 0x2386df0f, 0x80000001,
    0x88b8aba3, 0xfa9c620b, 0xb4c4a2a3, 0x5873ad80, 0x3ef5c326, 0x89f6f393, 0x802b5d47, 0x80000001,
    0x405f0fdc, 0x762d0009, 0x1008f05f, 0xa728c52a, 0x03ad9b05, 0xc7921d2a, 0xcfea60ee, 0x80000000,
    0x40dc1289, 0x26e4a02c, 0xf01970a5, 0x6b3b8d41, 0xe7b0d70c, 0x25c3e7ad, 0x2682e10f, 0x80000001,
    0x675e7906, 0xd08ee20e, 0x5009601a, 0x91074e19, 0x9d2b926a, 0x00000012, 0x00000000, 0x50000000,
    0x417c1439, 0xb7a06045, 0x8c2750f4, 0x44447b4c, 0x56b0f78f, 0x3fd42dce, 0x411b060f, 0x80000001,
    0x808f1194, 0x566a201c, 0x700b606b, 0xd42b0927, 0xe4ed3d86, 0xcf4286ca, 0xe83283f6, 0x80000000,
    0x813d976a, 0x4749802c, 0x581f48c8, 0x0d411549, 0x3671450e, 0x44f40eae, 0x3f870477, 0x80000001,
    0xbaee7ae1, 0xca825c42, 0x9598bbfe, 0x356c433f, 0x4e6b61a7, 0x03859aef, 0xe2de4a56, 0x80000000,
    0x20b559be, 0x7cb44ca9, 0x76138d15, 0x30b83db2, 0xbd787dce, 0x415acfb8, 0x75fb41df, 0x80000001,
    0x920641b0, 0x29f0e562, 0x81049322, 0x737d7d67, 0x1d305e2c, 0xfd66cfd2, 0x54bf0c76, 0x80000001,
    0xe9976734, 0x9d7690cc, 0x4e5d45a4, 0x45dca5db, 0xb5a5f3e6, 0x496577ed, 0x3cb8d14b, 0x80000000,
    0x601c94f9, 0xf6700e5a, 0x61c002f0, 0x9bb0b4ce, 0x6cbb72dd, 0x000d709c, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xc75115ce, 0x8cbe82e8, 0x1215f4ee, 0x42cfb7de, 0xb37dcd5e, 0xe45dc9bc, 0xdccbd57f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x01580c39, 0x083ce037, 0xd04d7146, 0xd9636379, 0xf976739c, 0x74069772, 0x5bfb294f, 0x80000001,
    0x3b0e7905, 0x008a41e4, 0xec02b03b, 0xeb018e09, 0xef32246b, 0x00000017, 0x00000000, 0x50000000,
    0xc11d91fb, 0x55e0203e, 0x2c258092, 0xd43f4320, 0xbdb60093, 0x8ee74e73, 0x7eef55ff, 0x80000001,
    0x00160614, 0xa4990005, 0xcc06f83e, 0xad289d1a, 0x98af6a06, 0x1c42af0b, 0x1272c177, 0x80000001,
    0x03279a1d, 0x596a4053, 0x5085e9fe, 0x3a8c119a, 0x68396332, 0xa0698357, 0x91cb78f7, 0x80000001,
    0x3db5fda8, 0xac411e4c, 0xe1de1cc1, 0xd7c321bd, 0xcefa4ed5, 0x6f2c637a, 0x9fe377e7, 0x80000001,
    0xf6aef4b5, 0x9dc159ff, 0x02cffe23, 0x11ca87c9, 0x067953dc, 0x5d7b729a, 0x8c9f56e7, 0x80000001,
    0xd714c9eb, 0x2a9a27ca, 0x394c7b9d, 0xa7878571, 0x29f17132, 0x2d677a13, 0x732f383f, 0x80000001,
    0xb83077b1, 0xaf18da93, 0xbf5b2f26, 0xe5f0e7f0, 0x2f5c4c74, 0x21425568, 0x66dd3b8c, 0x80000000,
    0x1555a492, 0x88000e00, 0x62777bd3, 0x86510af1, 0x0030002c, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x524936db, 0x0d556492, 0xeeaaae00, 0x4f89d9c4, 0x6bfaf2ac, 0xf96a1b18, 0xf35bf1b7, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00930a2d, 0x07100015, 0x34180090, 0x3130c54d, 0x1134448c, 0x746599b1, 0x745f2e8f, 0x80000001,
    0xb943f943, 0x706821a0, 0x9004d813, 0xa700ee05, 0xb87e14fd, 0x0000001e, 0x00000000, 0x50000000,
    0x41278716, 0x6785a031, 0xa834e0c8, 0xee508168, 0x00300014, 0x92558e50, 0x50030497, 0x80000001,
    0x00110640, 0xf5054005, 0x7c094852, 0xd637cb37, 0x07725e0b, 0x3d94b5cf, 0x2d66ea4f, 0x80000001,
    0x85f928f6, 0x2bd880da, 0xc8d402e4, 0x5d9cafc5, 0x0e3cd639, 0xd27aae79, 0xc967ba27, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc173a0d7, 0x76e1e02e, 0xb019c8a6, 0x48523960, 0xfcf54494, 0x58c607f1, 0x4c5b184f, 0x80000001,
    0xd401b583, 0x080012ef, 0x02000400, 0x00800100, 0xa1e0d0c0, 0x00032a49, 0x00000000, 0x40000000,
    0xc1951f9a, 0xe6e7602d, 0xb01d48b2, 0x355e196d, 0xf8b6ef99, 0x53a72a53, 0x4b17192f, 0x80000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x42672600, 0xc8948049, 0x7027c10d, 0x9e4cbd63, 0x88b33d90, 0x22e4866f, 0x1472cd47, 0x80000001,
    0xc1cf1ca7, 0x76d2a04a, 0x902a68d9, 0xd876a188, 0x32b95823, 0x0008a737, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x055b2c8b, 0x602b2234, 0x08049845, 0x6206ee31, 0xbfdfe267, 0x0000000a, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xc3e73117, 0x98cbc08b, 0x182f3125, 0x6b624d7d, 0x0f747218, 0xf54544d1, 0xe84e960e, 0x80000000,
    0xc1bea084, 0x483ee03c, 0xf82368f0, 0xce4d4f64, 0x6eb34090, 0xfee46eaf, 0xf2f29ece, 0x80000000,
    0xb65d7636, 0x14c118f3, 0xb03940f7, 0xe92f60ce, 0x91e2650b, 0x3702c4e9, 0x8e79b27d, 0x80000000,
    0x57a94b22, 0x4b5c07ff, 0x057e4c12, 0xf99dd193, 0x2575863d, 0xefb92796, 0x46befa76, 0x80000001,
    0xd21ec2d4, 0x0a11a535, 0x0102c32c, 0x6f7cf568, 0x0ef088ab, 0xac06b0d2, 0x225ec236, 0x80000001,
    0xa1d45d4b, 0xbc878c39, 0xe5dfc4dc, 0x28c12fbd, 0x219948d3, 0x6361c386, 0x3bf0d5bb, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc0a5182c, 0x75d18016, 0xb40d6060, 0xf137393d, 0xa9ef068a, 0xea22d40b, 0xf22e827e, 0x80000000,
    0x9791af37, 0x08aaf11b, 0x0200045b, 0x00800100, 0xd021ce40, 0x0003906a, 0x00000000, 0x40000000,
    0x00a9952a, 0xf5908012, 0xfc0f806f, 0x633a9539, 0x90ee040b, 0xc662fe0b, 0xdb525ece, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x82141f3a, 0x37c84055, 0x58358105, 0x904da95a, 0x90f2d993, 0x34e4bc2f, 0x2c32e977, 0x80000001,
    0xc0909529, 0x758d400d, 0xf80be861, 0x6e3c5341, 0xbf70940b, 0xdc23a32d, 0xd8fe6d76, 0x80000000,
    0x40cb1443, 0xd6baa020, 0x0418b0a7, 0x6442914f, 0xb271d00e, 0x0654390e, 0x035eaf87, 0x80000001,
    0x09aa4ec0, 0x7043e20a, 0xf40d887a, 0x6b02b216, 0x8edf3ee8, 0x0000000a, 0x00000000, 0x50000000,
    0x41d49adb, 0x57e7e046, 0xcc2b6905, 0xea4e4960, 0x4eb2a811, 0x18447f8f, 0x106ac417, 0x80000001,
    0x00f31ad0, 0x865ee01c, 0xb40f607b, 0x3532fb37, 0xdeaf4209, 0xb0c2deab, 0xc03648ae, 0x80000000,
    0x01121c12, 0xa6e3a019, 0xf8110897, 0xcd33b73b, 0x076f8e08, 0xde42e9ec, 0xe1627be6, 0x80000000,
    0xb732770d, 0x864e198e, 0xf45be16d, 0x883a6ee7, 0x8c241d0f, 0x562328ea, 0x9969c945, 0x80000000,
    0x5a67d076, 0xdbbda95a, 0x91a3e454, 0xfea4919b, 0xed366341, 0xfd59a096, 0x53fb0f8e, 0x80000001,
    0x515fc077, 0x69c544fc, 0x44f44306, 0x377a6d62, 0xb72fe32a, 0xbab680b1, 0x2b2acf7e, 0x80000001,
    0x6310df19, 0x5cb1acd5, 0x59f3d500, 0xb7c375c0, 0x89993a54, 0x45520006, 0x3b0cd013, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x01069b27, 0x96e8c00c, 0x4c0dd857, 0x7a22c525, 0x16a81083, 0xaa70ad26, 0x43795d65, 0x80000000,
    0xd760a269, 0x2816ad6c, 0x5a4603a2, 0x00800144, 0xb52ac540, 0x0005c56f, 0x00000000, 0x40000000,
    0x004291e4, 0x353a800b, 0x28014019, 0xf0103ce5, 0xd0244600, 0x01a06023, 0x1dbcda05, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x84099975, 0x99c240e9, 0x649b2a31, 0x305e9f71, 0x2c33901b, 0x65f58550, 0x56eb1fbf, 0x80000001,
    0x0149a190, 0xf7130023, 0x8c1928c0, 0x2046d360, 0x71f0978c, 0x85427c8c, 0x8979f4ce, 0x80000000,
    0x40e50c3b, 0x7711002b, 0x801df8b2, 0x0957eb61, 0x03b58917, 0x40560ad2, 0x2f7afab7, 0x80000001,
    0x1163b657, 0x002a814f, 0x840c5840, 0xd701e40f, 0xc0641169, 0x0000000c, 0x00000000, 0x50000000,
    0x00951014, 0xd76fc017, 0x7011787c, 0xe22dfd26, 0xb2eb8008, 0x3bc26469, 0xae2dffa6, 0x80000000,
    0x802887e9, 0x44c44005, 0x8808b844, 0x7026991d, 0xbbec2106, 0x79e22809, 0xb736227e, 0x80000000,
    0x40668af7, 0xd70e400c, 0x7c1998b3, 0x53407d4e, 0xdb70920d, 0x05e37f2c, 0x0d1ab517, 0x80000001,
    0x39907979, 0x9940bb2d, 0xc1307352, 0xb06ab944, 0x31ab1f25, 0x0d056c2f, 0xe18e45b6, 0x80000000,
    0x22415a40, 0x3ca34dc2, 0x9e1e850b, 0xefba79b4, 0x01b8e94f, 0x424af6d9, 0x6d57371f, 0x80000001,
    0x12fc425d, 0x39d44570, 0xf101631a, 0x3476ef58, 0x1b6dffa9, 0xc2a63551, 0x3b3adde6, 0x80000001,
    0x27fd652c, 0xed6cd041, 0x526de5cd, 0x18d2ebd1, 0xfa59745f, 0x60b1c565, 0x3b20d153, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x574f9e9c, 0x67dc5441, 0x0a27f323, 0xac9452ec, 0xfcf4970f, 0x000ab556, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x86301f53, 0x3b60c1bd, 0x01437b5a, 0xc0b781c8, 0x29fd0fc7, 0xd5bd58fc, 0xd543d127, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x807605bc, 0x38cb6036, 0x2c197096, 0x87a549bb, 0xf73ede37, 0x961c6cdb, 0x2352c23f, 0x80000001,
    0xd492ca49, 0x402ce0a9, 0x54978163, 0x63015c36, 0xd9724df6, 0x00000016, 0x00000000, 0x50000000,
    0x40f30ac8, 0x469a802c, 0x501fe0a6, 0x8d434551, 0x6134f70c, 0x7be69ef1, 0x7be3605f, 0x80000001,
    0x001a0390, 0x04c38005, 0x08062045, 0x542fc12a, 0xf3301908, 0xf633426c, 0xfab2a9ae, 0x80000000,
    0xc409224d, 0x5961c0a2, 0xf46d8a00, 0x5d8e499e, 0x4939e0ad, 0x67d85ef5, 0x4d77214f, 0x80000001,
    0x7b5f7bc2, 0xab41dcc7, 0x858123c9, 0x1d94fd7a, 0x3476653d, 0xfdfa2f57, 0x5b1f14a6, 0x80000001,
    0x392ef6f7, 0x8e895bd3, 0x3315c682, 0x74c5d3da, 0xb47a1359, 0x014aaab7, 0x5486e8bf, 0x80000001,
    0xda834d5e, 0x1afc0866, 0xd14c7ba4, 0x1288b579, 0xacef0b2f, 0xcfa67a91, 0x31aed2e6, 0x80000001,
    0xb531f6c4, 0x6f1ad821, 0xff522f22, 0xdbe949e8, 0x4de1e9ef, 0x2b537dab, 0x60fd5204, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20004000, 0x08001000, 0x02000400, 0x00800100, 0x00300040, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x5555d555, 0x55557555, 0x55555d55, 0x00555755, 0x00200060, 0x00000008, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xfb04f7b3, 0xfba1daf4, 0xf7e81fdf, 0x00ce7be2, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x515aab47, 0xe322076d, 0x347b2948, 0x14000028, 0x9421f3e5, 0x1215734e, 0x37b0bbfa, 0x10000000,
    0x7d8ffc23, 0xfdf93e07, 0x0000055d, 0xc7000000, 0xed7f23fe, 0xe8ee937e, 0x00000005, 0x10000000,
    0x92b969ed, 0x230d478b, 0x3069812b, 0x6b00001d, 0x2793e734, 0x47c34ec8, 0x244074c9, 0x10000000,
    0xfd2d7b14, 0x4e301dcc, 0x025e3e60, 0x6f000000, 0xfefed1fe, 0x92bebfbe, 0x00027576, 0x10000000,
    0x02ed06cc, 0x9084e13c, 0xe0133834, 0x3e000005, 0xd292842c, 0x3ff32f47, 0x22747661, 0x10000000,
    0x3e2dfd53, 0x2eacde8c, 0x2af476d6, 0x270000d3, 0x65bf507f, 0xb73ee03f, 0xef02c87e, 0x10000000,
    0x22d5e919, 0xd5e88edc, 0x9cc6aa37, 0xa9000036, 0xc0cd9772, 0xdd925545, 0x11f84a30, 0x10000000,
    0x7be1fa58, 0xfcf13bf2, 0xa261a5b8, 0x7149f8e7, 0x4afe02fe, 0x734e845e, 0x9cb61a35, 0x10002ac4,
    0x18c639d3, 0xe44b4a4a, 0x28887195, 0x2a000025, 0x3902cf86, 0x3af08821, 0x06f81730, 0x10000000,
    0x7b8d7b32, 0x1c341b15, 0xea20751a, 0xb33ff0cd, 0x0000000f, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xfe267d48, 0x6d375ced, 0x1662fda1, 0xa45540e9, 0x00093f1b, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x6d73622b, 0xf6ab9213, 0x7b800763, 0x007b3f75, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x7137095d, 0x75de3327, 0xccb22ae1, 0xead5120b, 0x0008c439, 0x00080010, 0x00000000, 0x30000000,
    0x2bc85f36, 0x26cb3265, 0x000001a6, 0xee000000, 0x02f7f275, 0x15695358, 0x00000002, 0x10000000,
    0xfc6a03da, 0xa767958f, 0xc4a412b8, 0x5add7a05, 0x0008a3c0, 0x00080010, 0x00000000, 0x30000000,
    0x2b465e5f, 0xa6dcd1b8, 0x0075027a, 0x7d000000, 0x4579e4f8, 0x83bb433a, 0x0000b124, 0x10000000,
    0x6bdc8363, 0x83e34e60, 0x6097b250, 0x74df1207, 0x0008a5c2, 0x00080010, 0x00000000, 0x30000000,
    0xea206093, 0x466eb0dd, 0x98a78227, 0xfa00001c, 0xb477e076, 0x9f58ad78, 0x27817703, 0x10000000,
    0x7df5979a, 0x65f25492, 0xf8d47378, 0xbfd4620c, 0x5fc6992c, 0x000520ff, 0x00000000, 0x30000000,
    0x701666fe, 0x88a413ac, 0xc5360b6d, 0x761a7e62, 0x66b9a679, 0x883b0a3a, 0x7c2d1832, 0x10000e48,
    0x7fa21828, 0x6407375c, 0x80444a66, 0x00800002, 0x00200040, 0x00080010, 0x00000000, 0x30000000,
    0x6c82e39f, 0xc699d08d, 0x4916d2ae, 0xbd222862, 0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x20004000, 0x00a49000, 0x020c0400, 0x806d0134, 0x2026c044, 0x0000000f, 0x00000000, 0x30000000,
    0xb8fcf590, 0xb7a896e3, 0x85092273, 0x27250a61, 0x0003b211, 0x00000000, 0x00000000, 0x00000000,
    0xd880bbc1, 0x080014e8, 0x02000400, 0x00800100, 0x28e2b6c0, 0x0003ea6b, 0x00000000, 0x40000000,
    0x816f9c6a, 0x57cf805a, 0xd440112c, 0x2472176c, 0xacfac5a5, 0xa14a3d78, 0x8ffb7847, 0x80000001,
    0x82b226a1, 0x789220a3, 0x2c38a92a, 0x6e68176d, 0x6af9471e, 0xaa888216, 0x99f38507, 0x80000001,
    0x43a3b09a, 0x19158081, 0x803fd973, 0x275eb367, 0x65f37818, 0x46a5a3b1, 0x55d7208f, 0x80000001,
    0xc2911844, 0x57a300a6, 0xf0532166, 0xe788217d, 0x703c48b0, 0x000b607a, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x45a03b68, 0x10986306, 0xa816406e, 0x67159455, 0x7268d968, 0x00000010, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x464bb958, 0xd99f4127, 0x505439b0, 0x157c8b74, 0x17310d29, 0x99b73973, 0x1e22bda6, 0x80000001,
    0x82b923f1, 0x88afc080, 0x9838f14e, 0x7261ff6c, 0xc8b2ff19, 0x0b45f931, 0x350ef0df, 0x80000001,
    0xb8eff8c9, 0x58ef1ac5, 0x6945cb33, 0x73433cf9, 0x1326ce14, 0xb3d3eeec, 0xbb0e0885, 0x80000000,
    0xd85fcc9a, 0x2ba267f9, 0x9d92e442, 0x41a5d79d, 0x2ef697c2, 0x16d9c317, 0x59f71cbf, 0x80000001,
    0x1a5dd0da, 0x2ba0e8ee, 0x358bb43d, 0xbc9c8d92, 0x57f480bc, 0x12f89b95, 0x5cc71c97, 0x80000001,
    0x2733e470, 0xdd4b0f41, 0xda3c2579, 0x81d371d1, 0x5a67a85f, 0x386592ce, 0x69394004, 0x80000000,
    0x57a53032, 0xf7b6d0fe, 0x0200046b, 0x00800100, 0x5020c0c0, 0x000392ea, 0x00000000, 0x40000000,
    0x80601272, 0xf5a8c012, 0x480ec05a, 0x5233712d, 0x8c72098b, 0x4ef46aae, 0x40770757, 0x80000001,
    0x80ee1b0c, 0x56adc02e, 0x1414f885, 0xf5403d45, 0x4a73db0e, 0x6dc515d0, 0x5d1b3187, 0x80000001,
    0xc1e59f1a, 0x9766004d, 0x242ba0e8, 0x9d4af353, 0x44b34392, 0x53451990, 0x49eb1587, 0x80000001,
    0x807813c2, 0xc5e7c014, 0x0c0fa069, 0xc53d0b3e, 0x7872f60d, 0x46a4b8ef, 0x3626fb9f, 0x80000001,
    0x81181a7c, 0x0810603d, 0x0426d0f1, 0x1458eb71, 0x6f74e415, 0x56358111, 0x447311af, 0x80000001,
    0x458a4479, 0xe054216b, 0xc00a7027, 0x8506081d, 0xbca528e5, 0x0000000e, 0x00000000, 0x50000000,
    0x41c8a598, 0x58274051, 0x082d190b, 0x6b601b78, 0x5f35c298, 0x5c360e32, 0x4b471acf, 0x80000001,
    0x80d617f4, 0x663d4022, 0x140d586b, 0x95340f31, 0xefef3a09, 0xd933584c, 0xf1a295fe, 0x80000000,
    0xc0c89d4e, 0x76b24020, 0x5412888a, 0x913b0942, 0x43b1b88b, 0x2bb3f00e, 0x241ae1a7, 0x80000001,
    0x38a2f894, 0x484a9a8e, 0x1d139aca, 0x9242aaf9, 0x13a6db93, 0xa8a3e2cc, 0xb66a00ad, 0x80000000,
    0x98f64d06, 0x0bcdc871, 0xbda2745f, 0x7aa973a1, 0x85771b44, 0x1a89f8b7, 0x5e7f20ef, 0x80000001,
    0xd2e54357, 0x3a4e45b3, 0x151d3b5d, 0x72866174, 0x33b1c7b0, 0xf6875a93, 0x4d43042e, 0x80000001,
    0xe633e348, 0x3d170eae, 0x82258551, 0xd1cf4dcd, 0xc52137dc, 0x48a4a90b, 0x3ea4d3db, 0x80000000,
    0x96f62877, 0x3781b0b7, 0x25889c39, 0x008000cf, 0xbd2589c0, 0x0004b14c, 0x00000000, 0x40000000,
    0x40288a4b, 0xa4bd2007, 0x1007e835, 0x6424db1c, 0xdbee4186, 0xf5b297ea, 0xf24293d6, 0x80000000,
    0x807811c8, 0x05d1e015, 0xb00e105e, 0xc9365338, 0xa3f1398b, 0x24b3d4ad, 0x1dced61f, 0x80000001,
    0x431b20a6, 0xe8f8c078, 0x8c4e9978, 0x95673d7c, 0x6136b11d, 0x8136d3f3, 0x70274a97, 0x80000001,
    0x00268cb2, 0x9509a005, 0xc004d832, 0x2c1f9d17, 0x932d1084, 0xcbc1fa89, 0xcf4a5e96, 0x80000000,
    0x005d0de8, 0x76338010, 0x74118077, 0x653c0744, 0xaab26e8c, 0x3534306e, 0x2526e38f, 0x80000001,
    0x8ec975ac, 0x1027a0b6, 0x68024808, 0x7d01f409, 0xeee2d162, 0x0000000d, 0x00000000, 0x50000000,
    0xc0979521, 0xa6846018, 0xd4129086, 0x753c9d45, 0xbd326d0c, 0x44143ece, 0x3362f73f, 0x80000001,
    0x402e0eb6, 0x75354008, 0xe0060039, 0x53252f1e, 0xd46e5e85, 0xdaa25caa, 0xdb1273d6, 0x80000000,
    0x40ca1768, 0x16da801a, 0x0c1a28af, 0x4c462d54, 0x7cf35f0f, 0x41c4854f, 0x2e56f217, 0x80000001,
    0xb9d479c2, 0xd889fb6f, 0x9518dae0, 0xa0533316, 0x0669ce9a, 0xe5f4dc8e, 0xd15a2fbd, 0x80000000,
    0x9c1951bc, 0xdc222a46, 0x49d49cac, 0x86b0c3aa, 0x3477dd49, 0x2dca7898, 0x6cb734b7, 0x80000001,
    0x9035be2a, 0xa9b80495, 0x58f2c2ff, 0xee7b5d64, 0xffb03b2a, 0xe726b311, 0x45faf70e, 0x80000001,
    0x283465af, 0x9d4fcfe8, 0x4a42557e, 0x44d2cdd1, 0x56d873df, 0xdb229ce7, 0x2b3ca412, 0x80000000,
    0x58531a79, 0x76644eaf, 0x69e32b72, 0xf276e6ab, 0x53b4dbc2, 0x00085275, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x580f49f3, 0x2d1cc818, 0x99a40487, 0x56afcbc3, 0xf63ae646, 0xbd5a9bd8, 0xb563a28f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00e28d36, 0x779f0024, 0xf82c00eb, 0xfc553569, 0x99347294, 0x51e52f90, 0x43c70cef, 0x80000001,
    0x7b97fbec, 0x0069a1f7, 0x30025823, 0xfa013208, 0x57b2a4f3, 0x00000016, 0x00000000, 0x50000000,
    0x409f0dc2, 0x76df001a, 0xc81db0b1, 0x70496f5b, 0xe373d590, 0x5d04b56f, 0x491f1747, 0x80000001,
    0x001d07df, 0x156f4005, 0xdc084850, 0x6a2e3535, 0xe3300507, 0x1cb2d4cb, 0x0ddebf1f, 0x80000001,
    0xc2311b27, 0xe94e803b, 0x804a7991, 0xda6ecf8d, 0x3a36d69e, 0x8df69693, 0x7d835bdf, 0x80000001,
    0x3bd3fbdf, 0x5b1bdcdd, 0xb1606bd5, 0xbb9db58c, 0x68f4263f, 0xc168ffb5, 0x3bd6e096, 0x80000001,
    0x6711e0f6, 0x5cadf043, 0xaa04f50a, 0x88b0d3aa, 0x2d77d1c8, 0x389a61b8, 0x6d033d5f, 0x80000001,
    0x94ee468d, 0xca470695, 0xcd282365, 0xd383616d, 0x91f0bf2f, 0x0d971bd2, 0x5ea31adf, 0x80000001,
    0x705b6f28, 0xae319524, 0x8ece5e4c, 0xe9e605e5, 0xa79b5dec, 0xb1027047, 0x4d5cff63, 0x80000000,
    0xa9ec1813, 0xe7ea0faa, 0x6920e2e7, 0xd496c6c9, 0x00300025, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x5c934c96, 0x7eaf0b8d, 0x8eee6e61, 0x3bdf7fea, 0x51bddae5, 0xeacdb29d, 0xe707e01f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x82a01204, 0x67bda0d0, 0x104ad939, 0xc15c0571, 0xd9746118, 0x6445ac90, 0x4beb1b0f, 0x80000001,
    0xfc6b7e22, 0x3061a1b8, 0xbc01601c, 0xf500f01d, 0xf13a84fb, 0x0000001b, 0x00000000, 0x50000000,
    0x80918876, 0xd638c018, 0xb02580af, 0x37476950, 0x61730212, 0x4084c86f, 0x3436f547, 0x80000001,
    0x001603e7, 0x95254005, 0xd80c5875, 0xdd3c1340, 0xa031b10b, 0x0a339eed, 0xfc62ab2f, 0x80000000,
    0x4496a6d6, 0xeaeca09e, 0x28b612af, 0x2f9f13b6, 0x163b38bc, 0xb35ad3f9, 0xa2178bdf, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x41b61d2e, 0xe74de045, 0xf02740d6, 0x2d595f68, 0x5936ad18, 0x6ee6bc13, 0x4bb32457, 0x80000001,
    0x95aeb091, 0x08001187, 0x02000400, 0x00800100, 0x4a2196c0, 0x00036d6a, 0x00000000, 0x40000000,
    0x41a81c80, 0x573b6038, 0x502380d3, 0xe05f1f70, 0x97f6dc18, 0x49b69fd3, 0x2e12f7c7, 0x80000001,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0175a3f5, 0xa8126029, 0x701a08cf, 0x52448358, 0xe232dd8d, 0x0a94140e, 0xf91eaa8f, 0x80000000,
    0x81629899, 0x96fcc030, 0xc01ad8ba, 0x9c67f17c, 0x48f88e1a, 0x00079955, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x030a181e, 0x50174186, 0xf404183a, 0xf9046421, 0x341ae2de, 0x00000008, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x423026da, 0xf7d58047, 0xac1a38c3, 0xcb4c0f5f, 0x9432458f, 0xcfd4060e, 0xb00a4e16, 0x80000000,
    0x80f01937, 0x177d4021, 0x501508a7, 0x113fc552, 0x0c31f28c, 0xd8a3a96e, 0xd2366e66, 0x80000000,
    0xb459744f, 0x9490176e, 0x643e30f1, 0xae27dcbb, 0x299fd309, 0xf4922ea8, 0x79958964, 0x80000000,
    0x95dcc756, 0xfb066738, 0xb15dfbd3, 0x0598178d, 0x5834b4ba, 0xd6889555, 0x3382e1ee, 0x80000001,
    0xce39ba65, 0xb93b83b5, 0x18c512a6, 0x176f8554, 0x71ee83a4, 0x6d05cbf0, 0xfeea8b96, 0x80000000,
    0x5fccdb3e, 0x2c520b10, 0x21bcfcad, 0x13b51db1, 0x5996cf4b, 0xec417885, 0x2ad8a7b2, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x808a1940, 0xc608e011, 0x380bf856, 0x582b552a, 0x432e7908, 0xf762f86b, 0xf76a937e, 0x80000000,
    0x94fa299b, 0x382acdc9, 0x0200041f, 0x00800100, 0x47e2e6c0, 0x0004092b, 0x00000000, 0x40000000,
    0x40cd995f, 0x65e8000e, 0x900a385f, 0x4a2b1522, 0x9dacb907, 0xbd42aeaa, 0xd2ea59f6, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc1d1a0ac, 0xf7b3603c, 0x2c1ec0d0, 0x2342ad50, 0x03b2d08e, 0x25e4448f, 0x12a6ccd7, 0x80000001,
    0x009a979b, 0xe5c0200a, 0xc4078852, 0xa5317d33, 0x61b00407, 0xd3d2f14c, 0xc92661ce, 0x80000000,
    0xc09b11cc, 0x46ebc018, 0xb414e099, 0xce415150, 0xc232978d, 0x1a24356e, 0x05eabc37, 0x80000001,
    0x83841959, 0xf0222196, 0x1c072041, 0x41015c0d, 0x24da3ade, 0x00000008, 0x00000000, 0x50000000,
    0x013c1b1f, 0x378be026, 0x0c1758b6, 0xe040d351, 0xa2f26e8c, 0x1484156e, 0x01eeb2e7, 0x80000001,
    0x80861528, 0x45d1c00f, 0xec095851, 0x63278b24, 0xc32e7e06, 0xac32684a, 0xb4f635be, 0x80000000,
    0x80949955, 0x066e200d, 0x980a3064, 0x532d6d32, 0xe52ff507, 0xda52decb, 0xd272673e, 0x80000000,
    0xf5867572, 0xc4e4f852, 0xd4485111, 0x1e2becc3, 0xd760f40b, 0x1c028168, 0x86f5a355, 0x80000000,
    0x5862cb31, 0x2b656884, 0x51839c1a, 0xf49f7b97, 0x8636103e, 0xf6d94d96, 0x48b70136, 0x80000001,
    0xce84ba8b, 0x494803e3, 0x84ca3ab3, 0xbb702354, 0x94ee55a4, 0x80e5e650, 0x0f06a20e, 0x80000001,
    0x20e25c5e, 0x3c580b9c, 0x19c234af, 0xc6b541b0, 0x11d749cb, 0xe0b1df86, 0x2d10a8a2, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc0649afa, 0xe5d8c008, 0x9406482c, 0x0210bef2, 0x05260e82, 0xa3b0a1c5, 0x3c293fe5, 0x80000000,
    0xd4182164, 0xc6c38ead, 0x91c0a34b, 0x008000d9, 0xd7e4bc40, 0x0004da6c, 0x00000000, 0x40000000,
    0x002394d5, 0xa3d4c005, 0xdc01b017, 0xe809e0b0, 0xf7dffc00, 0x25407502, 0x39b0f665, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x02731d91, 0x38c94066, 0x5c43895f, 0xa458856f, 0x2c74d296, 0x3b2550d1, 0x2976ea8f, 0x80000001,
    0x40ef217f, 0x06c46008, 0xd0066868, 0x552e4f40, 0xac300a85, 0x3c81ccca, 0x7515e406, 0x80000000,
    0x40858be0, 0xb77d2012, 0xfc1b08bf, 0x744dad64, 0xedb47c11, 0x29a53990, 0x196ed297, 0x80000001,
    0x89f63cdd, 0x501b4107, 0x1802d01e, 0xbf019a0d, 0xd21ee166, 0x0000000a, 0x00000000, 0x50000000,
    0xc080903a, 0x268b800f, 0xdc0fc885, 0x3e32e532, 0xff2ee709, 0xee63170b, 0xf4aa954e, 0x80000000,
    0x003089ac, 0xd55d8006, 0x58085848, 0xc5269720, 0x326d3a85, 0xce62332a, 0xd61e670e, 0x80000000,
    0x80fd1693, 0xf7818015, 0x141558b0, 0xa83d774e, 0x9ef1c38b, 0x16a3868d, 0x04aab57f, 0x80000001,
    0xb89c78a5, 0x46d81a72, 0xe07881bc, 0x823fecf6, 0x70659812, 0x72b3a3eb, 0xa3e1d935, 0x80000000,
    0x1e21d472, 0xcc824b45, 0xb5e6d4d5, 0xd5b1ddac, 0x7037dcc9, 0x268a99f8, 0x694b30df, 0x80000001,
    0xcfde3d4c, 0x2979c481, 0x58e91ae3, 0x5973a757, 0xb72e6527, 0x9b461430, 0x1d7eb786, 0x80000001,
    0x23abdf3e, 0xdcb54d55, 0x3dec84ec, 0x15c069bd, 0xd354cc53, 0xfad13ce4, 0x2ce4a912, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x16119827, 0x07572c0c, 0xde23eb4b, 0xbb5072c2, 0x11afd50e, 0x000839d3, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xcb8b2ffa, 0x3c3682fe, 0x111a4b87, 0xa6b7b1ca, 0xeabc0e4c, 0x8f7bfb7a, 0x848763ff, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x007c069c, 0xb7272016, 0xf01cb0aa, 0xb04c8f5f, 0x40f6d614, 0x192696f2, 0xedb2931f, 0x80000000,
    0xccd0acbb, 0x10ed838c, 0x9c3008d8, 0x840d2230, 0x7da8456b, 0x0000000f, 0x00000000, 0x50000000,
    0x005d8a94, 0x95bda00c, 0x9414407c, 0x973e1945, 0xc0b2188d, 0x10741b8e, 0x0142b177, 0x80000001,
    0x0012832e, 0x4452e005, 0x8008b841, 0xca24c90d, 0x88298185, 0x7cf20ca9, 0xbdae3aa6, 0x80000000,
    0x41c71655, 0x5831803b, 0x70432947, 0xa1582970, 0x37b3bf97, 0x3dc4610f, 0x1952dbaf, 0x80000001,
    0xb93f7968, 0xb9ad3adc, 0x10c392a9, 0xcc82a967, 0x0533adb0, 0xbc18bd55, 0x3baadbde, 0x80000001,
    0x6102d6cd, 0xeb8dcd86, 0x3d7bfc21, 0xbe9a6395, 0x84369abc, 0x06f933b6, 0x52231a47, 0x80000001,
    0xd0bbc019, 0x392ee4b3, 0xa0d78ab4, 0xe66e054b, 0xecadb224, 0x8065c50f, 0x0f8aa1b6, 0x80000001,
    0x2fedeedb, 0xfe39f4c7, 0xc2b4264d, 0x4dca83c7, 0x85d58659, 0x41413a24, 0x3220c15b, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20004000, 0x08001000, 0x02000400, 0x00800100, 0x00300040, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x5555d555, 0x55557555, 0x55555d55, 0x00555755, 0x00200060, 0x00000008, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xf66ff050, 0x48b11753, 0xabdd7fd3, 0x00b171d6, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x515aab47, 0xe322076d, 0x347b2948, 0x14000028, 0x9421f3e5, 0x1215734e, 0x37b0bbfa, 0x10000000,
    0x3a22f62d, 0x7c03dba7, 0x00000415, 0xc2000000, 0x353e65fd, 0xcb8d83fe, 0x00000004, 0x10000000,
    0x92b969ed, 0x230d478b, 0x3069812b, 0x6b00001d, 0x2793e734, 0x47c34ec8, 0x244074c9, 0x10000000,
    0x7a9676ba, 0x0c729b86, 0x01983500, 0x0f000000, 0xb77e6b7e, 0x4a4e891e, 0x0002173e, 0x10000000,
    0x02ed06cc, 0x9084e13c, 0xe0133834, 0x3e000005, 0xd292842c, 0x3ff32f47, 0x22747661, 0x10000000,
    0x7c9b7aaa, 0x5d8d1d88, 0xf6125db7, 0x9e00007e, 0x143ee97e, 0x389ea71f, 0xabb6495e, 0x10000000,
    0x22d5e919, 0xd5e88edc, 0x9cc6aa37, 0xa9000036, 0xc0cd9772, 0xdd925545, 0x11f84a30, 0x10000000,
    0xb85873ef, 0x6b619a1a, 0x6992bc84, 0xc41c106d, 0x983b657b, 0x325d7fdc, 0x6965ca4d, 0x10001732,
    0x18c639d3, 0xe44b4a4a, 0x28887195, 0x2a000025, 0x3902cf86, 0x3af08821, 0x06f81730, 0x10000000,
    0x385075c0, 0x5b4559ac, 0x75d61496, 0x9a220c94, 0x00000006, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xf884f520, 0x1adcf91f, 0x35b32460, 0xd32f8c96, 0x00035d8d, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xe615547d, 0x847e4e2f, 0x1f62c738, 0x0065ab5f, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x2e4a0c52, 0x7675d361, 0x38feabab, 0x66dfd221, 0x000de3c1, 0x00080010, 0x00000000, 0x30000000,
    0xe028cb64, 0xf3bdec1d, 0x000000d9, 0x2a000000, 0xd4754172, 0xbd47d115, 0x00000001, 0x10000000,
    0xf9b1032b, 0x1726b51b, 0x38c2c315, 0xf5e82c0c, 0x000edec6, 0x00080010, 0x00000000, 0x30000000,
    0x9e8d4536, 0x43d94b17, 0x003fd120, 0x3e000000, 0x0173b770, 0xd228af56, 0x000085aa, 0x10000000,
    0xab1e0195, 0x641b6d5b, 0x5c8b4208, 0xb6e38009, 0x000ba0c2, 0x00080010, 0x00000000, 0x30000000,
    0xe0454ac5, 0x44126c4c, 0x2449892d, 0x7d00000e, 0x93ecafe7, 0x2ac4b091, 0x07c04719, 0x10000000,
    0x78840833, 0xa6c6f49c, 0x79277b2b, 0xe1d7f816, 0xfd886636, 0x0005c15c, 0x00000000, 0x30000000,
    0x250f5306, 0x75ddceee, 0x30a8720d, 0x550ba633, 0xd92d3b66, 0xe5c557d0, 0x0e783c98, 0x10000078,
    0xb9c89063, 0xb350ee9d, 0xf49a417c, 0x00800005, 0x00200040, 0x00080010, 0x00000000, 0x30000000,
    0x2327d111, 0x253b8cdf, 0x70d96216, 0x8e1bba55, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x20004000, 0x019fb000, 0x020c0400, 0x806d0134, 0x2026c044, 0x0000000f, 0x00000000, 0x30000000,
    0x67a75a97, 0x35de4ead, 0x9d2a8a9d, 0x0d38ea7f, 0x0001641b, 0x00000000, 0x00000000, 0x00000000,
    0xd7f035d9, 0x08001176, 0x02000400, 0x00800100, 0x4c607b40, 0x00038b6a, 0x00000000, 0x40000000,
    0x01001431, 0x57c4e04d, 0xc43e513f, 0x3c711d6a, 0x46fa0a23, 0x81b91d37, 0x67174757, 0x80000001,
    0x0189196c, 0xd87b6070, 0xc8342124, 0x0869df67, 0x98f9a19f, 0xa3887fd6, 0x8ad778f7, 0x80000001,
    0x41ed23b3, 0x5897603c, 0x482d3939, 0x0a5a1f61, 0x7df37615, 0x33b57951, 0x3a6b012f, 0x80000001,
    0x01788eb3, 0x275e605e, 0x303b893d, 0x4d786970, 0xa93b4ea5, 0x0009be58, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x43e53184, 0x505d8206, 0x1814c866, 0x400ed440, 0xdaa04edd, 0x0000000b, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xc3e5aef7, 0xb89400a8, 0x603b995a, 0xbd5cef34, 0xc668019b, 0x2d54874d, 0xb03dddbd, 0x80000000,
    0x017b9efd, 0xb7f06044, 0xcc25290f, 0xe657ed54, 0x71b09b94, 0xaf453610, 0x074aa986, 0x80000001,
    0xb70af732, 0x48949945, 0xcd2bbb0c, 0xdd3906e1, 0xdb64d110, 0x8bc361ca, 0xa855e815, 0x80000000,
    0x1431440d, 0x0b4e4649, 0xe16563ea, 0xf5a01b98, 0xcc369abd, 0x15c962b6, 0x4d930bff, 0x80000001,
    0x14fec822, 0x5ab52643, 0xd52c1392, 0xba850177, 0xbe31482e, 0xadb6f212, 0x1d7abd0e, 0x80000001,
    0x656162b5, 0x7cfe4dd9, 0x4a0bed36, 0x0cc32fc1, 0x6d629c54, 0x8e64180b, 0x41dce7f3, 0x80000000,
    0x5608b53d, 0xf5ca4d7c, 0x02000356, 0x00800100, 0x25205540, 0x00036aca, 0x00000000, 0x40000000,
    0x404a8b4e, 0x15716011, 0x8411b86a, 0xc03eab3c, 0xfd73bb8e, 0x3524e48f, 0x1d7ede17, 0x80000001,
    0x40a00fa0, 0xf660e023, 0x30175889, 0xc447b74b, 0x51b53211, 0x66458751, 0x4d032097, 0x80000001,
    0xc0f51776, 0xc7064021, 0xd41da8c4, 0xcd489b55, 0xce73950f, 0x35049e6f, 0x230ee317, 0x80000001,
    0x00478bed, 0xd5c4400e, 0x7c0f286d, 0x7a471350, 0x4034420f, 0x29d4d590, 0x0f2acc2f, 0x80000001,
    0x410015aa, 0x18808034, 0x2c286911, 0x4f5ecb76, 0xef353c96, 0x4895a131, 0x3416ff17, 0x80000001,
    0x8473b97f, 0xd03c4109, 0x08093825, 0x18049013, 0x5d1e395a, 0x0000000b, 0x00000000, 0x50000000,
    0xc1441ed5, 0x788fe03b, 0x8c295919, 0xcc5f4d79, 0xecf56496, 0x44e5a691, 0x3166fb5f, 0x80000001,
    0x005211d7, 0xa5b9e011, 0x6c09c052, 0xf5333731, 0x11efe088, 0xcbe3496d, 0xd9ca76be, 0x80000000,
    0x406195bc, 0x8652c011, 0x100db879, 0xa73bc148, 0x63f2568a, 0x1583c62e, 0x047ab9b7, 0x80000001,
    0x7714f70b, 0xd841b95d, 0x7117a2cd, 0xca37fce0, 0x8924058f, 0x6533344a, 0x9d01cf7d, 0x80000000,
    0x5664487b, 0x1b80674c, 0x1580cc23, 0xf5a4259c, 0x0776aac0, 0x11399ed7, 0x54df159f, 0x80000001,
    0x50213ea8, 0xb9b8647c, 0x90e862f5, 0x6f785f62, 0x692f9aa8, 0x99d640b1, 0x1702b22e, 0x80000001,
    0x63675f97, 0xbcb4cd02, 0x71f01cf9, 0x06bf8bbc, 0xe05fb652, 0xf4540caa, 0x3070af92, 0x80000000,
    0xd6013287, 0xf61aaeec, 0x6926fb84, 0x008000b0, 0xb0a3cd40, 0x00043ceb, 0x00000000, 0x40000000,
    0x40268656, 0x44b7c007, 0xa40a2041, 0x192f1d28, 0xfd30aa09, 0x0ec3662c, 0xf8aea467, 0x80000000,
    0x40590ae3, 0xd601e012, 0xa412d070, 0xdd47cb4c, 0xb8347211, 0x429538f0, 0x2eaaf31f, 0x80000001,
    0x41b999d7, 0x48a8403c, 0xc8371133, 0x8a5f5977, 0x4135fc18, 0x5b45fbd2, 0x45af1397, 0x80000001,
    0x001606da, 0x34766005, 0x94046029, 0x55245b19, 0x41af4f85, 0xe222954b, 0xd616709e, 0x80000000,
    0xc0438ad9, 0x164aa00b, 0xec11b87e, 0x9b431151, 0x7533760d, 0x2c04644f, 0x13cacf3f, 0x80000001,
    0x072dcdca, 0x501a407c, 0x38013806, 0xb8016e07, 0x6bdd8ed9, 0x0000000b, 0x00000000, 0x50000000,
    0x004c10cf, 0x264ec00e, 0x64106081, 0xd9445154, 0x9fb3b50d, 0x35447c4f, 0x1d4eddef, 0x80000001,
    0x00198ab9, 0x04cc8005, 0xd804702e, 0x0f25d120, 0x2c6f4705, 0xd1225ecb, 0xc8c65d7e, 0x80000000,
    0xc0841461, 0xa70f4010, 0x7414e0a9, 0x66490960, 0x8533d90e, 0x27a4528f, 0x0ddac8f7, 0x80000001,
    0x3849783e, 0xd8161a46, 0x9cf4f29a, 0x3f444efd, 0x18e70a14, 0x9d93dbac, 0xb161f8e5, 0x80000000,
    0xda7eceb3, 0x3c01e971, 0x59c0f48d, 0x88adf7a7, 0xf477ba47, 0x289a4577, 0x66cb2d17, 0x80000001,
    0xce53bb04, 0x695f43c3, 0xecd372c1, 0x66738f59, 0x0faf2e26, 0xa1961af1, 0x1bf2b82e, 0x80000001,
    0xa52b61fa, 0x2ce86e08, 0x6e0d252a, 0xacc4dbc2, 0x38936ed5, 0x9301bdc5, 0x1ffc84f2, 0x80000000,
    0x95cca4f0, 0xe6be6fdc, 0x25b05398, 0x487bce85, 0x8fae583d, 0x00070911, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x0734ad1d, 0x2a8b6120, 0x38834a32, 0x3971178e, 0x06b67321, 0x89e6aab3, 0x78b753e7, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x80790afe, 0xa8204013, 0xe82378ec, 0x28591f72, 0x8df56e95, 0x48159d31, 0x3256f8d7, 0x80000001,
    0xda4df507, 0xe012408c, 0x5801301a, 0x9801300f, 0xd4e9fb6e, 0x00000010, 0x00000000, 0x50000000,
    0x80538c9d, 0xf7266009, 0x2811b894, 0xe443cd5d, 0xec735e0c, 0x4664114e, 0x2c96f19f, 0x80000001,
    0x001a8740, 0x35e8c005, 0x8c087857, 0x3b2fe13c, 0xdaf04307, 0xf792accb, 0xe5f288d6, 0x80000000,
    0x413c17e5, 0x28db401e, 0xd029091a, 0xfd59b377, 0x07750894, 0x58d54111, 0x422b0f67, 0x80000001,
    0x3938f93e, 0x3a3cbaee, 0x953a5b84, 0x2f72ed4d, 0x98ad9b29, 0x28d61db0, 0xec1e5f9e, 0x80000000,
    0x614d5905, 0x2c22ed0a, 0x75c3cc9a, 0xb0aaefa4, 0x86f707c4, 0x1149e457, 0x576718e7, 0x80000001,
    0x9160c0d7, 0x69bd84f5, 0x40f23301, 0x3a789960, 0x4e2f5929, 0xc5364a91, 0x330ad9f6, 0x80000001,
    0x29e5e7a7, 0x8d7df0fa, 0x7e58bda4, 0x63d2e5d1, 0x649694df, 0x3a518705, 0x331cc173, 0x80000000,
    0x9f4f968a, 0x56c2ce94, 0x2db9a33c, 0x7e84c0f1, 0x00300027, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x973f494e, 0x0ce72731, 0x1999b453, 0x6fa35dba, 0xe57a67bd, 0xac09e157, 0x9c27849f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x01020b2c, 0xc77e602e, 0x7c2548e1, 0x7a52a965, 0xb8748513, 0x502532b0, 0x2fd6f13f, 0x80000001,
    0xf9657c59, 0xe06b61ee, 0xd0006012, 0xf101fe1d, 0x2ef1c7f5, 0x00000015, 0x00000000, 0x50000000,
    0xc0640a19, 0xd6f4000d, 0x681678a0, 0x273fc554, 0x42f2698c, 0x24c3ae2e, 0x0e52c3d7, 0x80000001,
    0x001003ef, 0x05b9e005, 0xa006f05a, 0x422f293b, 0xc4304606, 0xf102756b, 0xe0928306, 0x80000000,
    0x42fc9f70, 0x49dae04d, 0xcc5781cf, 0x8471758f, 0xa7762fa0, 0x62764a72, 0x4aaf1bc7, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x01a9961d, 0x372f0054, 0xa030e8f2, 0x74588d60, 0x63f6e617, 0x6c067a53, 0x3ca32777, 0x80000001,
    0x9a063186, 0x08000f1e, 0x02000400, 0x00800100, 0x12de0740, 0x00026128, 0x00000000, 0x40000000,
    0xc13116b0, 0xb6e78026, 0x1c1708ac, 0xb850ad60, 0xb6b55091, 0x35d57e91, 0xf4aeaf8f, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00e39b75, 0x578f0017, 0x841118a0, 0x383da54e, 0xa6726d8b, 0xf413d86e, 0xe5a68f66, 0x80000000,
    0x00a413d0, 0x16380013, 0x100a5064, 0xe6594778, 0x8474f110, 0x00055bb1, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x03449586, 0xd01220f1, 0xc807803e, 0xc8020c14, 0xbc14ec55, 0x00000005, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x40e09b72, 0x46736018, 0xec09d064, 0xdd31e72e, 0x09ac4587, 0xc072f3cb, 0x5cdd99f5, 0x80000000,
    0xc07b9318, 0x46b4400f, 0xb00b106e, 0xd0322f3b, 0x856fd987, 0x9d62ecac, 0xad5a35fe, 0x80000000,
    0x32a971ed, 0x856d1618, 0xd050413c, 0xd125eab2, 0x421e2888, 0xb971d767, 0x693d63dc, 0x80000000,
    0xd24241ef, 0x3a5ae573, 0x8d161354, 0x7c8c497e, 0x3d735032, 0xd5d7eb54, 0x2a4ed4d6, 0x80000001,
    0x0a11318b, 0x18844249, 0x68830a0f, 0x1667e147, 0xd36c099f, 0x1f73e82d, 0xc27e3916, 0x80000000,
    0x9df7d86c, 0x8bcf8a2f, 0x099a7c55, 0xe5a657a7, 0xeb10e63f, 0x2d30a4c2, 0x1264600a, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc0869232, 0x75774018, 0xa4178872, 0xc93e1d38, 0xa1f3378f, 0x1bf4bfcf, 0xf8feb4df, 0x80000000,
    0xd7a42fc8, 0x26a9abdb, 0x0200045f, 0x00800100, 0x5763cfc0, 0x0003d2cb, 0x00000000, 0x40000000,
    0x006c11fd, 0x3535600a, 0x60098051, 0x41390132, 0xc131bc8a, 0xb873908d, 0xb8964b4e, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc0cd9aa6, 0xc743c016, 0x88102090, 0x883b9549, 0xe0b2e00b, 0xf7a3ff0e, 0xe992937e, 0x80000000,
    0x00449110, 0xf5350006, 0x2c05283c, 0x82306b2c, 0x9af0dd87, 0xcf72f9ac, 0xb43a4996, 0x80000000,
    0xc06e8ecb, 0x16e0e00f, 0x6410888b, 0x9a43eb55, 0x3473708d, 0x11543e6f, 0xfc5ab907, 0x80000000,
    0x43c31614, 0x801540c7, 0xf406002a, 0xf800d00a, 0xd594d0d4, 0x00000005, 0x00000000, 0x50000000,
    0xc096970a, 0x16b3e012, 0xb80d2879, 0xc03b0b47, 0x67728c0a, 0xeee3cece, 0xd7427dae, 0x80000000,
    0x00310c76, 0x94b12006, 0xb404d82f, 0x44200710, 0xec6dd704, 0x8001f249, 0x99220ebe, 0x80000000,
    0x40489110, 0xc5aa8005, 0xf0060844, 0x322a092b, 0xbcafeb06, 0xc372a44b, 0xb97e48ce, 0x80000000,
    0xb382f339, 0x7527d698, 0xc8531934, 0x832a30bb, 0xc11f010b, 0xc4120c67, 0x6d8d6724, 0x80000000,
    0xd59646be, 0x8b27a70d, 0xe95e5be4, 0xc79a0992, 0xbe756aba, 0xd0c8a6d5, 0x34aae1b6, 0x80000001,
    0x0a99b252, 0xd860e270, 0x9c939233, 0x95633f44, 0x296d2d9e, 0x2374712e, 0xd4e250be, 0x80000000,
    0x5e47d8ca, 0xdbcea9f7, 0xcd84cc59, 0x6ca72ba7, 0x1e94fac4, 0x5c617dc5, 0x182c7132, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x803a117d, 0x564ee005, 0x38062829, 0xe22e7d18, 0xefabdf0d, 0x62920009, 0x92a1e0c6, 0x80000000,
    0xd23b29e7, 0x95f6cc77, 0xd98a7ae2, 0x0080010c, 0xaa257fc0, 0x0004ab4c, 0x00000000, 0x40000000,
    0xc0520eed, 0xf37ce005, 0xe803d01d, 0x552000da, 0x1eac6f85, 0x532212a9, 0xa63622de, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc1879a1f, 0x882e2024, 0x481ea8eb, 0xea46ed5b, 0x79f2da0d, 0x0b43f36e, 0xfabaa70f, 0x80000000,
    0x4081a283, 0x96452007, 0x0404484f, 0x6334b346, 0x60b24708, 0x8922b18c, 0x822dca36, 0x80000000,
    0x80578b13, 0x375f400c, 0xbc132898, 0x1347a15d, 0x2533ce8f, 0x07850f30, 0x05bebb5f, 0x80000001,
    0xc4b11c7c, 0x100a2095, 0x1c027812, 0x1f010c11, 0xe419895c, 0x00000007, 0x00000000, 0x50000000,
    0x80679127, 0x968b4007, 0xc408f063, 0x8a2bb32f, 0xb7efdf06, 0xda12bf2b, 0xcee66396, 0x80000000,
    0x001f0692, 0x24d76005, 0x6405c836, 0x4523cd19, 0x24adde05, 0xa2f2288a, 0xaa522546, 0x80000000,
    0x006511c0, 0xe6ac8006, 0x4008d865, 0x6130093c, 0x32f0e187, 0xe2a2df6c, 0xd74a6e9e, 0x80000000,
    0x360af5ef, 0x450cb8c3, 0x3c4438ff, 0x452734b9, 0x74e1100a, 0xfec268c8, 0x801d8e1c, 0x80000000,
    0xd9164cb8, 0x7beee8b0, 0x39a1ac60, 0x2faa41a0, 0x5e76db45, 0x1b89e357, 0x5a93187f, 0x80000001,
    0x4d0ab813, 0x688fe33f, 0xd4a9425b, 0x38601139, 0x2b6b191d, 0x043491ee, 0xd39a3c06, 0x80000000,
    0x9f37d818, 0x5bf16b0b, 0x95aabc88, 0x35a8c1a1, 0xc6d2c144, 0xa750dd43, 0x1f84885a, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xcf59182d, 0xc6550dcc, 0x9acb0b05, 0x6f14150b, 0x13e7e69f, 0x00062a8f, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x02959538, 0x09ef8068, 0x8083c20c, 0xd0727b88, 0xfef4f122, 0x1e564851, 0x0f8abd47, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x402702ea, 0xb6edc005, 0x3c13708d, 0x29497150, 0x70b4f990, 0xc6f52e4f, 0xfc0e9a66, 0x80000000,
    0x8727a72f, 0x2032c1e9, 0x000468ac, 0x26018423, 0x1d17b3d8, 0x00000007, 0x00000000, 0x50000000,
    0x001d0583, 0xa57a0005, 0xf408404f, 0x9427c726, 0x6bae5285, 0xbba23e8a, 0xcf8a5dfe, 0x80000000,
    0x000e0194, 0xd3df2005, 0xe006c835, 0x301f2af3, 0x6de7ae04, 0x7eb18bc8, 0xa0ce1f1e, 0x80000000,
    0x004c071f, 0x76e0c008, 0x1c10188d, 0x053a114c, 0x0131738a, 0xe7e33b8d, 0xd8ea682e, 0x80000000,
    0x750274ca, 0x06347831, 0xf48491c6, 0xdf4d4511, 0x82a6a399, 0x9ca434ac, 0xb195f7fd, 0x80000000,
    0xd924cb3f, 0x0b312951, 0x415e23dd, 0x5498718e, 0x39f6cc38, 0xef78f156, 0x3d0f08b6, 0x80000001,
    0xcc64b61b, 0xf8378326, 0xf49cb239, 0x7d5f3737, 0xbe6a5b1d, 0x0fd4960d, 0xd70e3ede, 0x80000000,
    0x200c5b10, 0x4ca72bba, 0x8dbc1cd7, 0x66ad79a7, 0x1d8de445, 0x7d606d42, 0x1a30726a, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20004000, 0x08001000, 0x02000400, 0x00800100, 0x00300040, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x5555d555, 0x55557555, 0x55555d55, 0x00555755, 0x00200060, 0x00000008, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0xae8165cc, 0xa5317199, 0x47bf37b3, 0x00879db5, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0x515aab47, 0xe322076d, 0x347b2948, 0x14000028, 0x9421f3e5, 0x1215734e, 0x37b0bbfa, 0x10000000,
    0xb4d9edf7, 0x195e17c0, 0x0000025a, 0x7c000000, 0x697cad7b, 0xfc5b78dc, 0x00000002, 0x10000000,
    0x92b969ed, 0x230d478b, 0x3069812b, 0x6b00001d, 0x2793e734, 0x47c34ec8, 0x244074c9, 0x10000000,
    0x3145e75d, 0x78035439, 0x00993a7e, 0xf7000000, 0x1f7b787a, 0x8b7c215c, 0x000108ec, 0x10000000,
    0x02ed06cc, 0x9084e13c, 0xe0133834, 0x3e000005, 0xd292842c, 0x3ff32f47, 0x22747661, 0x10000000,
    0xf675f270, 0xb9e7f8af, 0xf501437a, 0x8a000030, 0xce7c9efc, 0x32fbdd7c, 0x44854c94, 0x10000000,
    0x22d5e919, 0xd5e88edc, 0x9cc6aa37, 0xa9000036, 0xc0cd9772, 0xdd925545, 0x11f84a30, 0x10000000,
    0x321beb1c, 0x68399475, 0x08bb3a9a, 0x6b07e626, 0xd7774376, 0xfa6bb6f9, 0x3ae1320b, 0x1000098a,
    0x18c639d3, 0xe44b4a4a, 0x28887195, 0x2a000025, 0x3902cf86, 0x3af08821, 0x06f81730, 0x10000000,
    0x70bfe8d9, 0x18df7555, 0x610dcb41, 0xc40cb443, 0x00000002, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xefb965d6, 0x684dd451, 0x59165312, 0x961ae453, 0x00016706, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x157d3389, 0xf17e6691, 0xdeaf8e37, 0x0023bcd2, 0x00000000, 0x00000000, 0x00000000, 0x20000000,
    0xf2db96f9, 0xb77bd5e1, 0xc1419453, 0xabeace39, 0x00176dd1, 0x00080010, 0x00000000, 0x30000000,
    0x909329ba, 0x1176c59f, 0x00000040, 0xed000000, 0x996ab7e1, 0x4742da4b, 0x00000000, 0x10000000,
    0x3c020361, 0xa8dcd8a6, 0x0d0cc40c, 0x46ea3c1b, 0x0018a2d4, 0x00080010, 0x00000000, 0x30000000,
    0x0dca216c, 0x5155a465, 0x0016785b, 0xea000000, 0x0be7d35d, 0xaaa315cd, 0x00000990, 0x10000000,
    0xaf890102, 0x6434cf16, 0xd47c5a38, 0xc3e52009, 0x0013a94c, 0x00080010, 0x00000000, 0x30000000,
    0xcd192107, 0xf14ca447, 0x9c1b3060, 0x33000006, 0x58a60f5c, 0x36a1364b, 0x00e00628, 0x10000000,
    0xbbfc0449, 0x57c59487, 0x10fbab73, 0x36dba014, 0x3c8fc041, 0x0008d97e, 0x00000000, 0x30000000,
    0x0ff5a5ba, 0x91e4457f, 0x803988a2, 0xf2054415, 0x699d81d8, 0x5b4247e7, 0x05b416d0, 0x10000092,
    0x2f8784cd, 0x42600be8, 0x1c55b133, 0x00800005, 0x00200040, 0x00080010, 0x00000000, 0x30000000,
    0x92fead9b, 0x727ec6dc, 0x1c5f60e5, 0x2c0f382a, 0x00000003, 0x00000000, 0x00000000, 0x00000000,
    0x20004000, 0x03a11000, 0x020c0400, 0x806d0134, 0x2026c044, 0x0000000f, 0x00000000, 0x30000000,
    0xd4332fa7, 0xb2e2a707, 0x48787926, 0x9117c833, 0x0000630b, 0x00000000, 0x00000000, 0x00000000,
    0x9765b12f, 0x08000c36, 0x02000400, 0x00800100, 0x47dc3ac0, 0x00028c28, 0x00000000, 0x40000000,
    0x00958cf1, 0xb7386025, 0x5c2b211a, 0x8062fd55, 0x72f8b099, 0x6ba6e3f4, 0x0a76d43f, 0x80000001,
    0xc116110f, 0xf88ac04d, 0xb037e134, 0xf472616d, 0xfffb8aa0, 0xb348a2d7, 0x84df83e7, 0x80000001,
    0x80fda026, 0x7952a017, 0xc81ee927, 0x115e7d79, 0xab74e594, 0x1b954071, 0x0a36cab7, 0x80000001,
    0x409706a4, 0xc613e021, 0xb418e8c7, 0x1161c357, 0xd5ba8a96, 0x000785f5, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x4311281f, 0xa0222107, 0x300d7848, 0x6a061425, 0x345fd464, 0x0000000a, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x81b122b0, 0x96e98038, 0x2c1868c0, 0xbf3dd902, 0xec21358d, 0x24928b29, 0x5228fc2b, 0x80000000,
    0x80af1883, 0x2801001b, 0x781530d6, 0xd64f0f5a, 0x3bb0080e, 0xa1b4222f, 0xd4f67356, 0x80000000,
    0xf4177515, 0x27a0f6f8, 0x0cbe824e, 0x9c27c0bc, 0x1ce0b009, 0x27b20fc8, 0x781d8c1d, 0x80000000,
    0x50fe3e2a, 0x9b0c84d1, 0xf92bdb9f, 0xa59a9596, 0x67b6e5b8, 0x1088d1d6, 0x3feb00df, 0x80000001,
    0x4bcab5ad, 0x58cda29e, 0x88818233, 0x595dad4e, 0x59ea869c, 0xc364304e, 0xbdde2b95, 0x80000000,
    0x9fe9dbb6, 0xac318a6e, 0x698f946f, 0xdf9f4d9a, 0xeb15a7bb, 0x88b13204, 0x178c7462, 0x80000000,
    0x56dfb0f2, 0x1517ed4a, 0x02000314, 0x00800100, 0xc4dc4f40, 0x000261c7, 0x00000000, 0x40000000,
    0x004d8734, 0x267bc00f, 0x8014a8a0, 0xa4448550, 0x7772c78d, 0xed23c6ae, 0xd3ca758e, 0x80000000,
    0x408f8a27, 0x17988024, 0x2425f0d8, 0xfe598365, 0xffb73c96, 0x5f660d92, 0x369f0d27, 0x80000001,
    0x40db1bba, 0x785a2013, 0xa81830dd, 0x1a4a1165, 0x5033768e, 0x0ed41e0f, 0xf5c6a82f, 0x80000000,
    0x8046886f, 0xa68ac00b, 0x9c0f0888, 0x1f477b5c, 0xc6f36a8d, 0xddb3dfce, 0xc5da6266, 0x80000000,
    0xc092111f, 0x3886c017, 0x5c19f0eb, 0x5e572374, 0xc3745111, 0x1dc4dfd0, 0x03e2bf2f, 0x80000001,
    0x435eac2f, 0x7023a0cc, 0x3c067026, 0xb6045214, 0x609ee661, 0x0000000a, 0x00000000, 0x50000000,
    0x40981570, 0x88612019, 0xf417e0e1, 0xbc531371, 0xc233d40f, 0x0314458f, 0xebe29c7f, 0x80000000,
    0x80450ddc, 0xc667c00c, 0x18099060, 0x95327134, 0x9eadfa07, 0x7fe2968b, 0xa8b6269e, 0x80000000,
    0xc0569456, 0x476b200a, 0xf40b8888, 0x9b3df556, 0xc0b21609, 0xe813588d, 0xd1d27386, 0x80000000,
    0xb48e74cd, 0xd7a3978c, 0x50d05a57, 0x3c2956c1, 0xf2dfa18a, 0xe3321687, 0x6ed97494, 0x80000000,
    0x52c9c34f, 0xcb62e5a1, 0x5155ebe9, 0x939e5d97, 0x5db6423c, 0xf7491156, 0x460b016e, 0x80000001,
    0xcac0b3e7, 0x1873c277, 0x0090ba3b, 0xa35f013e, 0x696bf59b, 0x0b04a46e, 0xccba360e, 0x80000000,
    0x9d155807, 0xcbd669b7, 0xd583ac41, 0x81a5219d, 0x1c580740, 0x5ed1b346, 0x18a47052, 0x80000000,
    0x97e8b21b, 0xa5052b08, 0x55457339, 0x008000c2, 0xd2de93c0, 0x0002c6e8, 0x00000000, 0x40000000,
    0x80290460, 0xa5c4c006, 0xf00d7868, 0x1d373d3d, 0x2231bc0a, 0xef232f0d, 0xd94280de, 0x80000000,
    0xc07207c1, 0x876f8014, 0x402260ca, 0x61584961, 0xebf6c617, 0x57263552, 0x334b0097, 0x80000001,
    0xc1b3223f, 0x98e7e023, 0xbc29292c, 0x865d5d7e, 0xabf5b115, 0x45957f51, 0x24a6ebaf, 0x80000001,
    0x001684a3, 0xe5906005, 0xe0079849, 0x5b2fff38, 0x27b17c87, 0xdac2d74c, 0xc5c65fae, 0x80000000,
    0x40430a12, 0x6774e007, 0x2c0f909b, 0x9e469f60, 0x31739c0c, 0x0f74110f, 0xf4b2a25f, 0x80000000,
    0x4440b1f4, 0xb0156089, 0x1401f00b, 0x3c03c20f, 0xc4defde1, 0x0000000a, 0x00000000, 0x50000000,
    0x805c9066, 0xf771e00a, 0x5c1090a2, 0x3e47e761, 0x10f3648d, 0x0bd410af, 0xf602a6f7, 0x80000000,
    0x001a881f, 0x057ac005, 0x5404f03a, 0x1d28872c, 0xc0ee9805, 0xa5422d6a, 0xa78a28c6, 0x80000000,
    0x00611290, 0xe7782009, 0x400e9899, 0xe0440360, 0x77b31b0b, 0x05e3c48e, 0xea8695ff, 0x80000000,
    0xb71bf731, 0x871a1960, 0x98b29a0c, 0x522f54d2, 0x8c62d40c, 0x41a2ba09, 0x8cb9afcd, 0x80000000,
    0x163148c5, 0x6bd30725, 0x45960456, 0xeea921a4, 0x96374cc3, 0x273a0477, 0x5dbb2817, 0x80000001,
    0x4bbb357d, 0x78ab42ce, 0x78a5fa5b, 0x5064df45, 0x202c951f, 0x40151ecf, 0xeaa26546, 0x80000000,
    0x217fddae, 0x4c6d6be5, 0x4dcd6cc5, 0x4bb5b1b0, 0x7093c8cb, 0x55013fc4, 0x16d46b6a, 0x80000000,
    0x54552c3e, 0x7617adee, 0x19669321, 0xc46870b5, 0xa7e83f56, 0x0004f10d, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x02fd27ac, 0x696e004e, 0xfc34895e, 0x78634582, 0x39f5e818, 0x5b85dc92, 0x3e7f0d2f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x40638a4a, 0x285d4009, 0x0819b8d1, 0xc6552f71, 0xa674f891, 0x2b64ed90, 0x0e2ecea7, 0x80000001,
    0x04809fb9, 0xc0092059, 0x6c018012, 0x9801ea17, 0xb2e41068, 0x0000000c, 0x00000000, 0x50000000,
    0x405b108e, 0x67a1a008, 0xa00ee09a, 0x0b443360, 0xbdb3768c, 0x2283e5ce, 0x08c2c07f, 0x80000001,
    0x0015059b, 0x15eec005, 0x6c070050, 0x002b5f36, 0xc6af2806, 0xc1023eaa, 0xbbd2448e, 0x80000000,
    0xc0d41666, 0x9842c00e, 0xa81818d1, 0x574e116b, 0x90b3fc8f, 0x27b44bef, 0x0a5ac64f, 0x80000001,
    0xb703f74c, 0xb8083918, 0xb88c41f6, 0xe04a270a, 0x1c272515, 0x9483c52c, 0xa8a1ec4d, 0x80000000,
    0x9987ce33, 0xfbcf88c0, 0x9d843443, 0x61a4379f, 0xd4768e3f, 0x11396f16, 0x4a8f0f0f, 0x80000001,
    0xce823be0, 0xe922a3ba, 0xa0c29a9f, 0xa66c534f, 0x0aad9ea2, 0x77a58990, 0x06e295e6, 0x80000001,
    0x643a6130, 0x4cb0ed45, 0xd1ec3500, 0x24bce5b9, 0xc61297d0, 0xdc60e3a3, 0x234097e2, 0x80000000,
    0xd4d71a30, 0x55fa6cd8, 0xbdf8b3b4, 0x676ba6bf, 0x00300043, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x8b43ba60, 0x4b0482a5, 0x108bea6f, 0x938383a1, 0x51f87a29, 0x7347ca15, 0x61b73a67, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x8061087e, 0x07c7800c, 0xb01c68c9, 0x3c4fe56a, 0xaaf3af91, 0x2d04818f, 0x12c2cacf, 0x80000001,
    0x4f37cfba, 0x602fe167, 0xd4006008, 0xd701b824, 0xafaad0ef, 0x00000010, 0x00000000, 0x50000000,
    0xc04b8d39, 0x76c6a005, 0x600b787a, 0x21355549, 0xfcf16a08, 0x0bb2fcac, 0xf6329d17, 0x80000000,
    0x001004b4, 0x55cac005, 0xe4054048, 0xcf283731, 0x7ceee004, 0xc291ff8a, 0xb95a471e, 0x80000000,
    0x81781b10, 0x489f601a, 0x5c26a919, 0x4c52536d, 0xb8341f11, 0x30a4892f, 0x1a9ed96f, 0x80000001,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x015292b2, 0x46b3602e, 0x8c18e0b5, 0xe4520d5d, 0x9ff44d12, 0x90058092, 0x14032807, 0x80000001,
    0x9744ac58, 0x08000923, 0x02000400, 0x00800100, 0xb41627c0, 0x00012984, 0x00000000, 0x40000000,
    0x808591da, 0xf57c400c, 0x6c06384d, 0xbf3aff52, 0x3b34ec8a, 0xdb733b31, 0xdb6e492e, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x0098164f, 0x4806a00b, 0xc40e589c, 0xe845035e, 0xf033e98b, 0x1fb43acf, 0xda22abe7, 0x80000000,
    0xc05e0ff0, 0x14fa0006, 0xf403582b, 0x4c530759, 0x496db711, 0x00024932, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x8250131b, 0x50050095, 0x94028026, 0x98008408, 0xd014d55b, 0x00000004, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00479268, 0x55122005, 0x1c03182b, 0xfb2c4d0f, 0x20dc9883, 0xaab2b946, 0xaaae0006, 0x80000000,
    0x00480e9e, 0xd69cc005, 0xb005a84e, 0x1d2daf39, 0xed323b86, 0x1d1281ac, 0x94b2165e, 0x80000000,
    0x72c471a1, 0x04a0545f, 0xcc2288a3, 0xa4167082, 0x821a6983, 0xc6e10fe5, 0x6371947c, 0x80000000,
    0x8d893972, 0x7a2983a5, 0x68f8c325, 0x0c8ad187, 0x02b4dd2e, 0xcde81e35, 0xf882e406, 0x80000000,
    0x486a25dd, 0x68f8016f, 0x0c497a39, 0x5b456d27, 0x99a33311, 0xaab26669, 0x55560006, 0x80000000,
    0xda4fd2c6, 0xfc0ee89a, 0xad6434c0, 0xdfaaabaa, 0x31ca8cb2, 0xeb806842, 0x0cf04189, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x80fb8f5a, 0x26616026, 0xc81a80a0, 0xe350ed50, 0x27f59c12, 0xc474c0d2, 0xe9766d7e, 0x80000000,
    0x57d8b163, 0xb51d0acd, 0x020003c4, 0x00800100, 0xfd9d9840, 0x00024607, 0x00000000, 0x40000000,
    0x809494a4, 0xa5b58008, 0x7406784d, 0x15328339, 0xc9b00b86, 0x863238aa, 0x5979a196, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x40b517ad, 0x77b3200e, 0x2c0f1098, 0xb23bbb4d, 0x9d337a89, 0x56f37fed, 0xc3a27ab6, 0x80000000,
    0xc0621397, 0xa5d40005, 0xf8039837, 0x462a4930, 0xff30ed04, 0x3d716bca, 0x70a5c296, 0x80000000,
    0x40578dbc, 0x47aa0009, 0x8c0ed090, 0x4b436359, 0xebb33c0d, 0xcbf44a2e, 0xc62e6476, 0x80000000,
    0x0328945a, 0xf01000bb, 0x38060829, 0x4e006208, 0x661868de, 0x00000006, 0x00000000, 0x50000000,
    0x407d1217, 0x0720a00d, 0x4c0b487c, 0x0539494d, 0xe6b1d609, 0xfe23380d, 0xb83a5ebe, 0x80000000,
    0x00150646, 0x24c2e005, 0xcc021822, 0xd21b9f0b, 0xe8edaa82, 0x4f619b09, 0x7939d986, 0x80000000,
    0x002b0b22, 0x55eae005, 0xc404b040, 0xa3270736, 0xc2709404, 0xb742358a, 0x8dba2f56, 0x80000000,
    0xb1727333, 0x94ca94bf, 0xf03fb107, 0x0f1dee9c, 0x4b1bee87, 0xc4e18e86, 0x6f3d78bc, 0x80000000,
    0x527ec23f, 0x6abe257b, 0x45187b63, 0x4f90a387, 0x5ab407b6, 0xe7e7c1d4, 0x17ced0c6, 0x80000001,
    0x4babae97, 0xc8d66317, 0xe880320d, 0xc547af4e, 0x95ae9311, 0xf934d4f1, 0xbf2658c5, 0x80000000,
    0xdda45453, 0xabf3aa0b, 0xc1000384, 0x78a32f89, 0xadcf4645, 0xbb60caa3, 0x09a44329, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0xc0f59478, 0xd7b82015, 0x28150063, 0xc647af5c, 0x00300012, 0xd174aab0, 0xba31d17d, 0x80000000,
    0x4f292720, 0x45770a71, 0xd55a33ab, 0x008000d3, 0xcba11a40, 0x00031109, 0x00000000, 0x40000000,
    0x417f13bd, 0xd8a3e026, 0x002220cc, 0x00400150, 0xb6fb6d90, 0x38e4924d, 0x71c9c726, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x80f797e6, 0x7801a010, 0x441208af, 0x4343dd53, 0xcb328a0c, 0xc723990d, 0xd36267fe, 0x80000000,
    0xc115907d, 0x35d9801b, 0x101088bb, 0xd4584796, 0x74745d08, 0x00045d31, 0x5555555c, 0x80000000,
    0x00760a63, 0xa7d2a00c, 0xa81020a2, 0x00481561, 0x3434b08e, 0xef647f90, 0xf1f2ba96, 0x80000000,
    0x446a9897, 0xc005a054, 0x7004e823, 0x6701e012, 0x785c1fe1, 0x00000008, 0x00000000, 0x50000000,
    0x00470ccc, 0x66494005, 0xc8079855, 0x6b2c8335, 0x482fb307, 0x65429e6b, 0x8b35ea0e, 0x80000000,
    0x001082dd, 0x24ca6005, 0xf004b03a, 0xe420c919, 0x906bc203, 0x4a4191e8, 0x6f25bffe, 0x80000000,
    0x00330b76, 0x9670e005, 0x8c063059, 0x6e30e740, 0xb7306486, 0x9d32782b, 0x97b60ed6, 0x80000000,
    0x72057290, 0x54293654, 0x683040e8, 0x5f24cca6, 0xd01fc709, 0xd8822367, 0x679d6a4c, 0x80000000,
    0x1735cb26, 0x6be6a760, 0x4980e435, 0x049fbf99, 0xd3758bc0, 0xdfb8e315, 0x3e8ef816, 0x80000001,
    0x486da860, 0x06b241de, 0x2063a1bb, 0x334c091f, 0x8b679892, 0xf662fccb, 0x666984d4, 0x80000000,
    0x1ba2d616, 0x9b06a73e, 0x8937cc06, 0x918db36e, 0x288f3b29, 0xf1f058e2, 0x10b85851, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x0f459fb8, 0xb453ebc7, 0xc6d552fe, 0xcb12c76c, 0x6764b1b4, 0x0004a8cc, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x40f48fa9, 0x0914a018, 0xc425792d, 0x2b4d0b67, 0x88b2948e, 0xca03db8e, 0xd2366506, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x001f042f, 0xf7478005, 0x880d9883, 0xb33e293c, 0x5333ee0d, 0xc4f3edcd, 0xe76658a6, 0x80000000,
    0x027790be, 0xc008609a, 0xa4055040, 0x3a00700f, 0x0fd6cf54, 0x00000006, 0x00000000, 0x50000000,
    0x001a04c0, 0x754d8005, 0xe804883e, 0xb323af1d, 0x8ead7584, 0x53620809, 0x86dde546, 0x80000000,
    0x000800e1, 0xf4244003, 0xa8033029, 0xe81bbf06, 0x55ac1082, 0x4ef15f48, 0x6f31c3fe, 0x80000000,
    0x0035869b, 0x56870005, 0xc007e862, 0xc02e2d38, 0x6bafa706, 0xaaf20a2a, 0xa39a1f9e, 0x80000000,
    0x719af114, 0x35b4d61e, 0xc860a96d, 0x68205ab3, 0x2a9c1406, 0x5ea12366, 0x58a13414, 0x80000000,
    0x51f23de6, 0x1aa065d8, 0x8d02cb5a, 0xaa90ed8c, 0xc3f51132, 0x83679954, 0x0b56a19e, 0x80000001,
    0xc6d82b5c, 0x87112115, 0xc454316a, 0x7f4134ff, 0x83a3fa8d, 0xfae24709, 0x818d9514, 0x80000000,
    0x19f24eb4, 0x7b76c7ad, 0xd13773f5, 0xdc906197, 0xac49e431, 0xf7b06661, 0x0f505731, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20004000, 0x08001000, 0x02000400, 0x00800100, 0x00300040, 0x00040010, 0x00000000, 0x40000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x5555d555, 0x55557555, 0x55555d55, 0x00555755, 0x00200060, 0x00000008, 0x00000000, 0x50000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
    0x20006000, 0x0c000800, 0x01000400, 0x00800180, 0x00300020, 0x00040010, 0x80020006, 0x80000000,
};


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

    BUF_CHECK(ret, mpp_buffer_get(p_hal->buf_group, &reg_ctx->cdf_rd_def_base, 200 * MPP_ALIGN(sizeof(g_default_prob), 2048)));
    mpp_buffer_attach_dev(reg_ctx->cdf_rd_def_base, p_hal->dev);
    cdf_ptr = mpp_buffer_get_ptr(reg_ctx->cdf_rd_def_base);
    memcpy(cdf_ptr, g_default_prob, sizeof(g_default_prob));
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
