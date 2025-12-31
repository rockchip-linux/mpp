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

#include "vdpu_com.h"
#include "vdpu383_com.h"

void vdpu383_init_ctrl_regs(Vdpu383RegSet *regs, MppCodingType codec_t)
{
    Vdpu383CtrlReg *ctrl_regs = &regs->ctrl_regs;

    switch (codec_t) {
    case MPP_VIDEO_CodingAVC : {
        ctrl_regs->reg8_dec_mode = 1;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xfffedfff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0x0ffbf9ff;
    } break;
    case MPP_VIDEO_CodingHEVC : {
        ctrl_regs->reg8_dec_mode = 0;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0x3ff3f9ff;
    } break;
    case MPP_VIDEO_CodingAVS2 : {
        ctrl_regs->reg8_dec_mode = 3;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffff;
        ctrl_regs->reg21_cabac_error_en_highbits = 0x3fffffff;
    } break;
    case MPP_VIDEO_CodingVP9 : {
        ctrl_regs->reg8_dec_mode = 2;
        ctrl_regs->reg20_cabac_error_en_lowbits = 0xffffffdf;
        ctrl_regs->reg21_cabac_error_en_highbits = 0x3fffffff;
    } break;
    case MPP_VIDEO_CodingAV1 : {
        ctrl_regs->reg8_dec_mode = 4;
        ctrl_regs->reg20_cabac_error_en_lowbits  = 0xffffffdf;
        ctrl_regs->reg21_cabac_error_en_highbits = 0x3fffffff;
    } break;
    default : {
        mpp_err("not support codec type %d\n", codec_t);
    } break;
    }

    ctrl_regs->reg9.buf_empty_en = 0;

    ctrl_regs->reg10.strmd_auto_gating_e      = 1;
    ctrl_regs->reg10.inter_auto_gating_e      = 1;
    ctrl_regs->reg10.intra_auto_gating_e      = 1;
    ctrl_regs->reg10.transd_auto_gating_e     = 1;
    ctrl_regs->reg10.recon_auto_gating_e      = 1;
    ctrl_regs->reg10.filterd_auto_gating_e    = 1;
    ctrl_regs->reg10.bus_auto_gating_e        = 1;
    ctrl_regs->reg10.ctrl_auto_gating_e       = 1;
    ctrl_regs->reg10.rcb_auto_gating_e        = 1;
    ctrl_regs->reg10.err_prc_auto_gating_e    = 1;

    ctrl_regs->reg13_core_timeout_threshold = 0xffffff;

    ctrl_regs->reg16.error_proc_disable = 1;
    ctrl_regs->reg16.error_spread_disable = 0;
    ctrl_regs->reg16.roi_error_ctu_cal_en = 0;

    return;
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
