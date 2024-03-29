/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "vdpu383_com"

#include <string.h>

#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_common.h"
#include "mpp_compat_impl.h"
#include "mpp_frame_impl.h"

#include "vdpu383_com.h"

static RK_U32 rcb_coeff[RCB_BUF_COUNT] = {
    [RCB_STRMD_ROW]                 = 3,
    [RCB_STRMD_TILE_ROW]            = 3,
    [RCB_INTER_ROW]                 = 6,
    [RCB_INTER_TILE_ROW]            = 6,
    [RCB_INTRA_ROW]                 = 5,
    [RCB_INTRA_TILE_ROW]            = 5,
    [RCB_FILTERD_ROW]               = 60,
    [RCB_FILTERD_PROTECT_ROW]       = 60,
    [RCB_FILTERD_TILE_ROW]          = 60,
    [RCB_FILTERD_TILE_COL]          = 90,
    [RCB_FILTERD_AV1_UP_TILE_COL]   = 0,
};

static RK_S32 update_size_offset(Vdpu383RcbInfo *info, RK_U32 reg_idx,
                                 RK_S32 offset, RK_S32 len, RK_S32 idx)
{
    RK_S32 buf_size = 0;

    buf_size = 2 * MPP_ALIGN(len * rcb_coeff[idx], RCB_ALLINE_SIZE);
    info[idx].reg_idx = reg_idx;
    info[idx].offset = offset;
    info[idx].size = buf_size;

    return buf_size;
}

RK_S32 vdpu383_get_rcb_buf_size(Vdpu383RcbInfo *info, RK_S32 width, RK_S32 height)
{
    RK_S32 offset = 0;

    offset += update_size_offset(info, 140, offset, width, RCB_STRMD_ROW);
    offset += update_size_offset(info, 142, offset, width, RCB_STRMD_TILE_ROW);
    offset += update_size_offset(info, 144, offset, width, RCB_INTER_ROW);
    offset += update_size_offset(info, 146, offset, width, RCB_INTER_TILE_ROW);
    offset += update_size_offset(info, 148, offset, width, RCB_INTRA_ROW);
    offset += update_size_offset(info, 150, offset, width, RCB_INTRA_TILE_ROW);
    offset += update_size_offset(info, 152, offset, width, RCB_FILTERD_ROW);
    offset += update_size_offset(info, 154, offset, width, RCB_FILTERD_PROTECT_ROW);
    offset += update_size_offset(info, 156, offset, width, RCB_FILTERD_TILE_ROW);
    offset += update_size_offset(info, 158, offset, height, RCB_FILTERD_TILE_COL);
    offset += update_size_offset(info, 160, offset, height, RCB_FILTERD_AV1_UP_TILE_COL);

    return offset;
}

void vdpu383_setup_rcb(Vdpu383RegCommonAddr *reg, MppDev dev,
                       MppBuffer buf, Vdpu383RcbInfo *info)
{
    MppDevRegOffsetCfg trans_cfg;
    RK_U32 i;

    reg->reg140_rcb_strmd_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg142_rcb_strmd_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg144_rcb_inter_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg146_rcb_inter_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg148_rcb_intra_row_offset           = mpp_buffer_get_fd(buf);
    reg->reg150_rcb_intra_tile_row_offset      = mpp_buffer_get_fd(buf);
    reg->reg152_rcb_filterd_row_offset         = mpp_buffer_get_fd(buf);
    reg->reg154_rcb_filterd_protect_row_offset = mpp_buffer_get_fd(buf);
    reg->reg156_rcb_filterd_tile_row_offset    = mpp_buffer_get_fd(buf);
    reg->reg158_rcb_filterd_tile_col_offset    = mpp_buffer_get_fd(buf);
    reg->reg160_rcb_filterd_av1_upscale_tile_col_offset = mpp_buffer_get_fd(buf);

    reg->reg141_rcb_strmd_row_len            =  info[RCB_STRMD_ROW].size          ;
    reg->reg143_rcb_strmd_tile_row_len       =  info[RCB_STRMD_TILE_ROW].size     ;
    reg->reg145_rcb_inter_row_len            =  info[RCB_INTER_ROW].size          ;
    reg->reg147_rcb_inter_tile_row_len       =  info[RCB_INTER_TILE_ROW].size     ;
    reg->reg149_rcb_intra_row_len            =  info[RCB_INTRA_ROW].size          ;
    reg->reg151_rcb_intra_tile_row_len       =  info[RCB_INTRA_TILE_ROW].size     ;
    reg->reg153_rcb_filterd_row_len          =  info[RCB_FILTERD_ROW].size        ;
    reg->reg155_rcb_filterd_protect_row_len  =  info[RCB_FILTERD_PROTECT_ROW].size;
    reg->reg157_rcb_filterd_tile_row_len     =  info[RCB_FILTERD_TILE_ROW].size   ;
    reg->reg159_rcb_filterd_tile_col_len     =  info[RCB_FILTERD_TILE_COL].size   ;
    reg->reg161_rcb_filterd_av1_upscale_tile_col_len = info[RCB_FILTERD_AV1_UP_TILE_COL].size;

    for (i = 0; i < RCB_BUF_COUNT; i++) {
        if (info[i].offset) {
            trans_cfg.reg_idx = info[i].reg_idx;
            trans_cfg.offset = info[i].offset;
            mpp_dev_ioctl(dev, MPP_DEV_REG_OFFSET, &trans_cfg);
        }
    }
}

RK_S32 vdpu383_compare_rcb_size(const void *a, const void *b)
{
    RK_S32 val = 0;
    Vdpu383RcbInfo *p0 = (Vdpu383RcbInfo *)a;
    Vdpu383RcbInfo *p1 = (Vdpu383RcbInfo *)b;

    val = (p0->size > p1->size) ? -1 : 1;

    return val;
}

void vdpu383_setup_statistic(Vdpu383CtrlReg *ctrl_regs)
{
    ctrl_regs->reg28.axi_perf_work_e = 1;
    ctrl_regs->reg28.axi_cnt_type = 1;
    ctrl_regs->reg28.rd_latency_id = 11;

    ctrl_regs->reg29.addr_align_type     = 1;
    ctrl_regs->reg29.ar_cnt_id_type      = 0;
    ctrl_regs->reg29.aw_cnt_id_type      = 1;
    ctrl_regs->reg29.ar_count_id         = 17;
    ctrl_regs->reg29.aw_count_id         = 0;
    ctrl_regs->reg29.rd_band_width_mode  = 0;

    /* set hurry */
    ctrl_regs->reg30.axi_wr_qos = 0;
    ctrl_regs->reg30.axi_rd_qos = 0;
}

void vdpu383_afbc_align_calc(MppBufSlots slots, MppFrame frame, RK_U32 expand)
{
    RK_U32 ver_stride = 0;
    RK_U32 img_height = mpp_frame_get_height(frame);
    RK_U32 img_width = mpp_frame_get_width(frame);
    RK_U32 hdr_stride = (*compat_ext_fbc_hdr_256_odd) ?
                        (MPP_ALIGN(img_width, 256) | 256) :
                        (MPP_ALIGN(img_width, 64));

    mpp_slots_set_prop(slots, SLOTS_HOR_ALIGN, mpp_align_64);
    mpp_slots_set_prop(slots, SLOTS_VER_ALIGN, mpp_align_16);

    mpp_frame_set_fbc_hdr_stride(frame, hdr_stride);

    ver_stride = mpp_align_16(img_height);
    if (*compat_ext_fbc_buf_size) {
        ver_stride += expand;
    }
    mpp_frame_set_ver_stride(frame, ver_stride);
}

RK_S32 vdpu383_set_rcbinfo(MppDev dev, Vdpu383RcbInfo *rcb_info)
{
    MppDevRcbInfoCfg rcb_cfg;
    RK_U32 i;

    Vdpu383RcbSetMode_e set_rcb_mode = RCB_SET_BY_PRIORITY_MODE;

    RK_U32 rcb_priority[RCB_BUF_COUNT] = {
        RCB_FILTERD_ROW,
        RCB_INTER_ROW,
        RCB_INTRA_ROW,
        RCB_STRMD_ROW,
        RCB_INTER_TILE_ROW,
        RCB_INTRA_TILE_ROW,
        RCB_STRMD_TILE_ROW,
        RCB_FILTERD_TILE_ROW,
        RCB_FILTERD_TILE_COL,
        RCB_FILTERD_AV1_UP_TILE_COL,
        RCB_FILTERD_PROTECT_ROW,
    };
    /*
     * RCB_SET_BY_SIZE_SORT_MODE: by size sort
     * RCB_SET_BY_PRIORITY_MODE: by priority
     */

    switch (set_rcb_mode) {
    case RCB_SET_BY_SIZE_SORT_MODE : {
        Vdpu383RcbInfo info[RCB_BUF_COUNT];

        memcpy(info, rcb_info, sizeof(info));
        qsort(info, MPP_ARRAY_ELEMS(info),
              sizeof(info[0]), vdpu383_compare_rcb_size);

        for (i = 0; i < MPP_ARRAY_ELEMS(info); i++) {
            rcb_cfg.reg_idx = info[i].reg_idx;
            rcb_cfg.size = info[i].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            } else
                break;
        }
    } break;
    case RCB_SET_BY_PRIORITY_MODE : {
        Vdpu383RcbInfo *info = rcb_info;
        RK_U32 index = 0;

        for (i = 0; i < MPP_ARRAY_ELEMS(rcb_priority); i ++) {
            index = rcb_priority[i];

            rcb_cfg.reg_idx = info[index].reg_idx;
            rcb_cfg.size = info[index].size;
            if (rcb_cfg.size > 0) {
                mpp_dev_ioctl(dev, MPP_DEV_RCB_INFO, &rcb_cfg);
            }
        }
    } break;
    default:
        break;
    }

    return 0;
}

void vdpu383_update_thumbnail_frame_info(MppFrame frame)
{
    RK_U32 down_scale_height = mpp_frame_get_height(frame) >> 1;
    RK_U32 down_scale_width = mpp_frame_get_width(frame) >> 1;
    RK_U32 down_scale_ver = MPP_ALIGN(down_scale_height, 16);
    RK_U32 down_scale_hor = MPP_ALIGN(down_scale_width, 16);
    RK_U32 down_scale_buf_size = 0;

    if (!MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(frame))) {
        down_scale_hor = mpp_align_128_odd_plus_64(down_scale_hor);
        down_scale_ver = mpp_frame_get_ver_stride(frame) >> 1;
    }

    down_scale_buf_size = down_scale_hor * down_scale_ver *  3 / 2;
    /*
     *  no matter what format, scale down image will output as 8bit raster format;
     */
    mpp_frame_set_fmt(frame, MPP_FMT_YUV420SP);
    mpp_frame_set_width(frame, down_scale_width);
    mpp_frame_set_height(frame, down_scale_height);
    mpp_frame_set_hor_stride(frame, down_scale_hor);
    mpp_frame_set_ver_stride(frame, down_scale_ver);
    mpp_frame_set_buf_size(frame, down_scale_buf_size);
}

void vdpu383_setup_down_scale(MppFrame frame, MppDev dev, Vdpu383CtrlReg *com, void* comParas)
{
    RK_U32 down_scale_height = mpp_frame_get_height(frame) >> 1;
    RK_U32 down_scale_width = mpp_frame_get_width(frame) >> 1;
    RK_U32 down_scale_ver = MPP_ALIGN(down_scale_height, 16);
    RK_U32 down_scale_hor = MPP_ALIGN(down_scale_width, 16);

    Vdpu383RegCommParas* paras = (Vdpu383RegCommParas*)comParas;
    MppFrameFormat fmt = mpp_frame_get_fmt(frame);
    MppMeta meta = mpp_frame_get_meta(frame);
    RK_U32 down_scale_y_offset = 0;
    RK_U32 down_scale_uv_offset = 0;
    RK_U32 down_scale_y_virstride = down_scale_ver * down_scale_hor;
    RK_U32 downscale_buf_size;

    if (!MPP_FRAME_FMT_IS_FBC(mpp_frame_get_fmt(frame))) {
        down_scale_hor = mpp_align_128_odd_plus_64(down_scale_hor);
        down_scale_ver = mpp_frame_get_ver_stride(frame) >> 1;
        down_scale_y_virstride = down_scale_ver * down_scale_hor;
    }
    /*
     *  no matter what format, scale down image will output as 8bit raster format;
     *  down_scale image buffer size was already added to the buf_size of mpp_frame,
     *  which was calculated in mpp_buf_slot.cpp: (size = original_size + scaledown_size)
     */
    switch ((fmt & MPP_FRAME_FMT_MASK)) {
    case MPP_FMT_YUV400 : {
        downscale_buf_size = down_scale_y_virstride;
    } break;
    case MPP_FMT_YUV420SP_10BIT :
    case MPP_FMT_YUV420SP : {
        downscale_buf_size = down_scale_y_virstride * 3 / 2;
    } break;
    case MPP_FMT_YUV422SP_10BIT :
    case MPP_FMT_YUV422SP : {
        downscale_buf_size = down_scale_y_virstride * 2;
    } break;
    case MPP_FMT_YUV444SP : {
        downscale_buf_size = down_scale_y_virstride * 3;
    } break;
    default : {
        downscale_buf_size = down_scale_y_virstride * 3 / 2;
    } break;
    }
    downscale_buf_size = MPP_ALIGN(downscale_buf_size, 16);

    down_scale_y_offset = MPP_ALIGN((mpp_frame_get_buf_size(frame) - downscale_buf_size), 16);
    down_scale_uv_offset = down_scale_y_offset + down_scale_y_virstride;

    com->reg9.scale_down_en = 1;
    com->reg9.av1_fgs_en = 0;
    paras->reg71_scl_ref_hor_virstride = down_scale_hor >> 4;
    paras->reg72_scl_ref_raster_uv_hor_virstride = down_scale_hor >> 4;
    if ((fmt & MPP_FRAME_FMT_MASK) == MPP_FMT_YUV444SP)
        paras->reg72_scl_ref_raster_uv_hor_virstride = down_scale_hor >> 3;
    paras->reg73_scl_ref_virstride = down_scale_y_virstride >> 4;
    if (mpp_frame_get_thumbnail_en(frame) == MPP_FRAME_THUMBNAIL_MIXED) {
        mpp_dev_set_reg_offset(dev, 133, down_scale_y_offset);
        mpp_meta_set_s32(meta, KEY_DEC_TBN_Y_OFFSET, down_scale_y_offset);
        mpp_meta_set_s32(meta, KEY_DEC_TBN_UV_OFFSET, down_scale_uv_offset);
    }
}

#ifdef DUMP_VDPU383_DATAS
RK_U32 dump_cur_frame = 0;
char dump_cur_dir[128];
char dump_cur_fname_path[512];

MPP_RET flip_string(char *str)
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

MPP_RET dump_data_to_file(char *fname_path, void *data, RK_U32 data_bit_size,
                          RK_U32 line_bits, RK_U32 big_end)
{
    RK_U8 *buf_p = (RK_U8 *)data;
    RK_U8 cur_data;
    RK_U32 i;
    RK_U32 loop_cnt;
    FILE *dump_fp = NULL;
    char line_tmp[256];
    RK_U32 str_idx = 0;

    dump_fp = fopen(fname_path, "w+");
    if (!dump_fp) {
        mpp_err_f("open file: %s error!\n", fname_path);
        return MPP_NOK;
    }

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

    fclose(dump_fp);

    return MPP_OK;
}
#endif