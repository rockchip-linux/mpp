/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys_cfg"

#include <string.h>

#include "rk_mpp_cfg.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_list.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_singleton.h"

#include "mpp_cfg.h"
#include "mpp_soc.h"
#include "mpp_trie.h"
#include "mpp_sys_cfg.h"
#include "mpp_mem_pool.h"
#include "mpp_compat_impl.h"

#include "mpp_cfg.h"
#include "mpp_cfg_io.h"
#include "kmpp_obj.h"

#define SYS_CFG_DBG_FUNC                (0x00000001)
#define SYS_CFG_DBG_INFO                (0x00000002)
#define SYS_CFG_DBG_SET                 (0x00000004)
#define SYS_CFG_DBG_GET                 (0x00000008)
#define SYS_CFG_DBG_DEC_BUF             (0x00000010)

#define sys_cfg_dbg(flag, fmt, ...)     mpp_dbg_f(mpp_sys_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define sys_cfg_dbg_func(fmt, ...)      sys_cfg_dbg(SYS_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_info(fmt, ...)      sys_cfg_dbg(SYS_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_set(fmt, ...)       sys_cfg_dbg(SYS_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_get(fmt, ...)       sys_cfg_dbg(SYS_CFG_DBG_GET, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_dec_buf(fmt, ...)   sys_cfg_dbg(SYS_CFG_DBG_DEC_BUF, fmt, ## __VA_ARGS__)

#define MPP_SYS_CFG_ENTRY_TABLE(prefix, ENTRY, STRCT, EHOOK, SHOOK, ALIAS) \
    CFG_DEF_START() \
    STRUCT_START(dec_buf_chk) \
    ENTRY(prefix, u32, RK_U32,          enable,             FLAG_BASE(0),   dec_buf_chk, enable) \
    ENTRY(prefix, u32, MppCodingType,   type,               FLAG_INCR,      dec_buf_chk, type) \
    ENTRY(prefix, u32, MppFrameFormat,  fmt_codec,          FLAG_INCR,      dec_buf_chk, fmt_codec) \
    ENTRY(prefix, u32, RK_U32,          fmt_fbc,            FLAG_INCR,      dec_buf_chk, fmt_fbc) \
    ENTRY(prefix, u32, RK_U32,          fmt_hdr,            FLAG_INCR,      dec_buf_chk, fmt_hdr) \
    ENTRY(prefix, u32, RK_U32,          width,              FLAG_INCR,      dec_buf_chk, width) \
    ENTRY(prefix, u32, RK_U32,          height,             FLAG_INCR,      dec_buf_chk, height) \
    ENTRY(prefix, u32, RK_U32,          crop_top,           FLAG_INCR,      dec_buf_chk, crop_top) \
    ENTRY(prefix, u32, RK_U32,          crop_bottom,        FLAG_INCR,      dec_buf_chk, crop_bottom) \
    ENTRY(prefix, u32, RK_U32,          crop_left,          FLAG_INCR,      dec_buf_chk, crop_left) \
    ENTRY(prefix, u32, RK_U32,          crop_right,         FLAG_INCR,      dec_buf_chk, crop_right) \
    ENTRY(prefix, u32, RK_U32,          unit_size,          FLAG_INCR,      dec_buf_chk, unit_size) \
    ENTRY(prefix, u32, RK_U32,          has_metadata,       FLAG_INCR,      dec_buf_chk, has_metadata) \
    ENTRY(prefix, u32, RK_U32,          has_thumbnail,      FLAG_INCR,      dec_buf_chk, has_thumbnail) \
    ENTRY(prefix, u32, RK_U32,          h_stride_by_byte,   FLAG_INCR,      dec_buf_chk, h_stride_by_byte) \
    ENTRY(prefix, u32, RK_U32,          v_stride,           FLAG_INCR,      dec_buf_chk, v_stride) \
    /* read-only config */ \
    ENTRY(prefix, u32, RK_U32,          cap_fbc,            FLAG_NONE,      dec_buf_chk, cap_fbc) \
    ENTRY(prefix, u32, RK_U32,          cap_tile,           FLAG_NONE,      dec_buf_chk, cap_tile) \
    ENTRY(prefix, u32, RK_U32,          h_stride_by_pixel,  FLAG_NONE,      dec_buf_chk, h_stride_by_pixel) \
    ENTRY(prefix, u32, RK_U32,          offset_y,           FLAG_NONE,      dec_buf_chk, offset_y) \
    ENTRY(prefix, u32, RK_U32,          size_total,         FLAG_NONE,      dec_buf_chk, size_total) \
    ENTRY(prefix, u32, RK_U32,          size_fbc_hdr,       FLAG_NONE,      dec_buf_chk, size_fbc_hdr) \
    ENTRY(prefix, u32, RK_U32,          size_fbc_bdy,       FLAG_NONE,      dec_buf_chk, size_fbc_bdy) \
    ENTRY(prefix, u32, RK_U32,          size_metadata,      FLAG_NONE,      dec_buf_chk, size_metadata) \
    ENTRY(prefix, u32, RK_U32,          size_thumbnail,     FLAG_NONE,      dec_buf_chk, size_thumbnail) \
    STRUCT_END(dec_buf_chk) \
    CFG_DEF_END()

#define KMPP_OBJ_NAME               mpp_sys_cfg
#define KMPP_OBJ_INTF_TYPE          MppSysCfg
#define KMPP_OBJ_IMPL_TYPE          MppSysCfgSet
#define KMPP_OBJ_SGLN_ID            MPP_SGLN_SYS_CFG
#define KMPP_OBJ_ENTRY_TABLE        MPP_SYS_CFG_ENTRY_TABLE
#define KMPP_OBJ_ACCESS_DISABLE
#define KMPP_OBJ_HIERARCHY_ENABLE
#include "kmpp_obj_helper.h"

typedef enum SysCfgAlignType_e {
    SYS_CFG_ALIGN_8,
    SYS_CFG_ALIGN_16,
    SYS_CFG_ALIGN_32,
    SYS_CFG_ALIGN_64,
    SYS_CFG_ALIGN_128,
    SYS_CFG_ALIGN_256,
    SYS_CFG_ALIGN_256_ODD,
    SYS_CFG_ALIGN_128_ODD_PLUS_64,
    SYS_CFG_ALIGN_LEN_DEFAULT,
    SYS_CFG_ALIGN_LEN_420,
    SYS_CFG_ALIGN_LEN_422,
    SYS_CFG_ALIGN_LEN_444,
    SYS_CFG_ALIGN_LEN_422_AVC,
    SYS_CFG_ALIGN_LEN_420_AV1,
    SYS_CFG_ALIGN_LEN_422_AV1,
    SYS_CFG_ALIGN_BUTT,
} SysCfgAlignType;

static RK_U32 mpp_sys_cfg_align(SysCfgAlignType type, RK_U32 val)
{
    switch (type) {
    case SYS_CFG_ALIGN_8: { return MPP_ALIGN(val, 8);};
    case SYS_CFG_ALIGN_16: { return MPP_ALIGN(val, 16);};
    case SYS_CFG_ALIGN_32: { return MPP_ALIGN(val, 32);};
    case SYS_CFG_ALIGN_64: { return MPP_ALIGN(val, 64);};
    case SYS_CFG_ALIGN_128: { return MPP_ALIGN(val, 128);};
    case SYS_CFG_ALIGN_256: { return MPP_ALIGN(val, 256);};
    case SYS_CFG_ALIGN_256_ODD: {return MPP_ALIGN(val, 256) | 256;};
    case SYS_CFG_ALIGN_128_ODD_PLUS_64: {
        val = MPP_ALIGN(val, 64);
        if (((val - 64) % 256 == 128))
            return val;
        else
            return ((MPP_ALIGN(val, 128) | 128) + 64);
    };
    case SYS_CFG_ALIGN_LEN_DEFAULT: { return (9 * MPP_ALIGN(val, 16) / 5);};
    case SYS_CFG_ALIGN_LEN_420:
    case SYS_CFG_ALIGN_LEN_422: { return (2 * MPP_ALIGN(val, 16));};
    case SYS_CFG_ALIGN_LEN_444: { return (3 * MPP_ALIGN(val, 16));};
    case SYS_CFG_ALIGN_LEN_422_AVC: { return ((5 * MPP_ALIGN(val, 16)) / 2);};
    case SYS_CFG_ALIGN_LEN_420_AV1: { return (2 * MPP_ALIGN(val, 128));};
    case SYS_CFG_ALIGN_LEN_422_AV1: { return ((5 * MPP_ALIGN(val, 64)) / 2);};
    default: {
        mpp_err("Specifying the align type is necessary");
        return MPP_NOK;
    };
    }
}

/* Based on drm_gem_framebuffer_helper.c drm_gem_afbc_min_size() */
static RK_S32 get_afbc_min_size(RK_S32 width, RK_S32 height, RK_S32 bpp)
{
#define AFBC_HEADER_SIZE 16
#define AFBC_HDR_ALIGN 64
#define AFBC_SUPERBLOCK_PIXELS 256
#define AFBC_SUPERBLOCK_ALIGNMENT 128

    RK_S32 n_blocks, hdr_alignment, size;

    /* AFBC_FORMAT_MOD_BLOCK_SIZE_16x16 and !AFBC_FORMAT_MOD_TILED */
    width = MPP_ALIGN(width, 16);
    height = MPP_ALIGN(height, 16);
    hdr_alignment = AFBC_HDR_ALIGN;

    n_blocks = (width * height) / AFBC_SUPERBLOCK_PIXELS;

    size = MPP_ALIGN(n_blocks * AFBC_HEADER_SIZE, hdr_alignment);
    size += n_blocks * MPP_ALIGN(bpp * AFBC_SUPERBLOCK_PIXELS / 8,
                                 AFBC_SUPERBLOCK_ALIGNMENT);
    return size;
}

/*
 * in:  fmt_fbc,type,width,h_stride
 * out: stride_w
 *
 * in:  fmt_fbc,type,height,v_stride
 * out: stride_h
 *
 * in:  fmt_fbc,type,fmt_codec,width,h_stride
 * out: h_stride_by_byte
 *
 * in:  fmt_fbc,type,fmt_codec,width,height,h_stride,v_stride
 * out: buffer_size
 */
MPP_RET mpp_sys_dec_buf_chk_proc(MppSysDecBufChkCfg *cfg)
{
    MppCodingType type = cfg->type;
    MppFrameFormat fmt = (MppFrameFormat)(((RK_U32)cfg->fmt_codec & MPP_FRAME_FMT_MASK) |
                                          (cfg->fmt_fbc & MPP_FRAME_FBC_MASK) |
                                          (cfg->fmt_hdr & MPP_FRAME_HDR_MASK));
    MppFrameFormat fmt_raw = cfg->fmt_codec;

    RK_U32 aligned_pixel = 0;
    RK_U32 aligned_pixel_byte = 0;
    RK_U32 aligned_byte = 0;
    RK_U32 aligned_height = 0;
    RK_U32 size_total = 0;
    RK_U32 size_total_old = 0;
    RK_U32 depth = MPP_FRAME_FMT_IS_YUV_10BIT(fmt) ? 10 : 8;
    RockchipSocType soc_type = mpp_get_soc_type();

    if (type == MPP_VIDEO_CodingUnused) {
        mpp_err("The coding type is invalid");
        return MPP_NOK;
    }

    /* use codec stride */
    if (cfg->h_stride_by_byte)
        aligned_pixel = cfg->h_stride_by_byte * 8 / depth;
    if (cfg->v_stride)
        aligned_height = cfg->v_stride;

    sys_cfg_dbg_dec_buf("org pixel wxh: [%d %d]\n", cfg->width, cfg->height);
    sys_cfg_dbg_dec_buf("outside stride wxh: [%d %d]\n",
                        cfg->h_stride_by_byte, cfg->v_stride);
    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        RK_U32 ext_pix = 0;
        /* fbc case */
        switch (type) {
        case MPP_VIDEO_CodingHEVC : {
            if (((soc_type == ROCKCHIP_SOC_RK3538) ||
                 (soc_type == ROCKCHIP_SOC_RK3572)) &&
                MPP_FRAME_FMT_IS_AFBC(fmt)) {
                ext_pix = ((aligned_height == cfg->height) ||
                           (aligned_height - cfg->height < 8)) ?
                          8 : 0;
            }
            sys_cfg_dbg_dec_buf("height padding: %d\n", ext_pix);
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = ((aligned_height != 0) ? aligned_height :
                              cfg->height) + ext_pix;
            aligned_height = MPP_ALIGN(aligned_height, 16);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN((aligned_height != 0) ? aligned_height :
                                       cfg->height, 16);
        } break;
        /*
         * avc aligned to ctu
         * p_Vid->width = p_Vid->PicWidthInMbs * 16
         * p_Vid->height = p_Vid->FrameHeightInMbs * 16
         */
        case MPP_VIDEO_CodingAVC : {
            if (((soc_type == ROCKCHIP_SOC_RK3538) ||
                 (soc_type == ROCKCHIP_SOC_RK3572)) &&
                MPP_FRAME_FMT_IS_AFBC(fmt)) {
                ext_pix = ((aligned_height == cfg->height) ||
                           (aligned_height - cfg->height < 16)) ?
                          8 : 0;
            }
            sys_cfg_dbg_dec_buf("height padding: %d\n", ext_pix);
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = ((aligned_height != 0) ? aligned_height :
                              cfg->height) + ext_pix;
            aligned_height = MPP_ALIGN(aligned_height, 16);
        } break;
        case MPP_VIDEO_CodingAVSPLUS :
        case MPP_VIDEO_CodingAVS :
        case MPP_VIDEO_CodingAVS2 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN((aligned_height != 0) ? aligned_height :
                                       cfg->height, 16);
        } break;
        case MPP_VIDEO_CodingVP9 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN((aligned_height != 0) ? aligned_height :
                                       cfg->height, 64);
        } break;
        default : {
            aligned_pixel = MPP_ALIGN(cfg->width, 16);
            aligned_height = MPP_ALIGN((aligned_height != 0) ? aligned_height :
                                       cfg->height, 16);
        } break;
        }
        sys_cfg_dbg_dec_buf("spec aligned pixel wxh: [%d %d]\n", aligned_pixel, aligned_height);

        /*fbc stride default 64 align*/
        if (*compat_ext_fbc_hdr_256_odd)
            aligned_pixel_byte = (MPP_ALIGN(aligned_pixel, 256) | 256) * depth >> 3;
        else
            aligned_pixel_byte = MPP_ALIGN(aligned_pixel, 64) * depth >> 3;
        sys_cfg_dbg_dec_buf("need 256 odd align: %d\n", *compat_ext_fbc_hdr_256_odd);

        switch (type) {
        case MPP_VIDEO_CodingAVC :
        case MPP_VIDEO_CodingAVSPLUS :
        case MPP_VIDEO_CodingAVS :
        case MPP_VIDEO_CodingAVS2 : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 64);
        } break;
        case MPP_VIDEO_CodingHEVC : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 64);
        } break;
        case MPP_VIDEO_CodingVP9 : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 64);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 16);
        } break;
        default : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 16);
        } break;
        }
        sys_cfg_dbg_dec_buf("dec hw aligned hor_byte: [%d]\n", aligned_byte);

        cfg->h_stride_by_byte = aligned_byte;
        cfg->h_stride_by_pixel = aligned_pixel;
        cfg->v_stride = aligned_height;

        switch ((fmt_raw & MPP_FRAME_FMT_MASK)) {
        case MPP_FMT_YUV420SP_10BIT : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 15);
        } break;
        case MPP_FMT_YUV422SP_10BIT : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 20);
        } break;
        case MPP_FMT_YUV420SP : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 12);
        } break;
        case MPP_FMT_YUV422SP : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 16);
        } break;
        case MPP_FMT_YUV444SP : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 24);
        } break;
        case MPP_FMT_YUV444SP_10BIT : {
            size_total = get_afbc_min_size(aligned_pixel, aligned_height, 30);
        } break;
        default : {
            size_total = aligned_byte * aligned_height * 3 / 2;
            mpp_err("dec out fmt 0x%x is no support", fmt_raw & MPP_FRAME_FMT_MASK);
        } break;
        }
        sys_cfg_dbg_dec_buf("res aligned_pixel %d\n", aligned_pixel);
        sys_cfg_dbg_dec_buf("res aligned_byte %d\n", aligned_byte);
        sys_cfg_dbg_dec_buf("res aligned_height %d\n", aligned_height);
        sys_cfg_dbg_dec_buf("res GPU aligned size_total: [%d]\n", size_total);

        cfg->size_total = size_total;
    } else {
        /* tile case */
        /* raster case */
        aligned_pixel = cfg->width;
        switch (type) {
        case MPP_VIDEO_CodingHEVC : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(cfg->height, 8);
        } break;
        /*
         * avc aligned to ctu
         * p_Vid->width = p_Vid->PicWidthInMbs * 16
         * p_Vid->height = p_Vid->FrameHeightInMbs * 16
         */
        case MPP_VIDEO_CodingAVC : {
            aligned_pixel = MPP_ALIGN(cfg->width, 16);
            aligned_height = MPP_ALIGN(cfg->height, 16);
        } break;
        case MPP_VIDEO_CodingVP9 : {
            if (soc_type == ROCKCHIP_SOC_RK3399)
                aligned_height = MPP_ALIGN(cfg->height, 64);
            else if (soc_type == ROCKCHIP_SOC_RK3588)
                aligned_height = MPP_ALIGN(cfg->height, 16);
            else
                aligned_height = MPP_ALIGN(cfg->height, 8);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            aligned_height = MPP_ALIGN(cfg->height, 8);
        } break;
        case MPP_VIDEO_CodingVP8 :
        case MPP_VIDEO_CodingH263 :
        case MPP_VIDEO_CodingMPEG2 :
        case MPP_VIDEO_CodingMPEG4 : {
            aligned_height = MPP_ALIGN(cfg->height, 16);
        } break;
        case MPP_VIDEO_CodingAVS2 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(cfg->height, 8);
        } break;
        default : {
            aligned_height = MPP_ALIGN(cfg->height, 8);
        } break;
        }
        sys_cfg_dbg_dec_buf("spec aligned pixel wxh: [%d %d]\n", aligned_pixel, aligned_height);

        aligned_pixel_byte = (cfg->h_stride_by_byte != 0) ? cfg->h_stride_by_byte :
                             aligned_pixel * depth / 8;
        aligned_height = (cfg->v_stride != 0) ? cfg->v_stride : aligned_height;

        switch (type) {
        case MPP_VIDEO_CodingHEVC : {
            aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_64, aligned_pixel_byte);
        } break;
        case MPP_VIDEO_CodingVP9 : {
            if (soc_type == ROCKCHIP_SOC_RK3576)
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_128_ODD_PLUS_64,
                                                 aligned_pixel_byte);
            else
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_256_ODD, aligned_pixel_byte);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            if (soc_type == ROCKCHIP_SOC_RK3588)
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_16, aligned_pixel_byte);
            else
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_128, aligned_pixel_byte);
        } break;
        default : {
            aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_16, aligned_pixel_byte);
        } break;
        }
        sys_cfg_dbg_dec_buf("dec hw aligned hor_byte: [%d %d]\n", aligned_byte);

        /*
         * NOTE: rk3576 use 128 odd plus 64 for all non jpeg format
         * all the other socs use 256 odd on larger than 1080p
         */
        if ((aligned_byte > 1920 || soc_type == ROCKCHIP_SOC_RK3576)
            && type != MPP_VIDEO_CodingMJPEG) {
            rk_s32 update = 0;

            switch (soc_type) {
            case ROCKCHIP_SOC_RK3399 :
            case ROCKCHIP_SOC_RK3568 :
            case ROCKCHIP_SOC_RK3562 :
            case ROCKCHIP_SOC_RK3528 :
            case ROCKCHIP_SOC_RK3588 : {
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_256_ODD, aligned_byte);
                update = 1;
            } break;
            case ROCKCHIP_SOC_RK3576 : {
                aligned_byte = mpp_sys_cfg_align(SYS_CFG_ALIGN_128_ODD_PLUS_64, aligned_byte);
                update = 1;
            } break;
            default : {
            } break;
            }

            /*
             * recalc aligned_pixel here
             * NOTE: no RGB format here in fact
             */
            if (update) {
                switch (fmt & MPP_FRAME_FMT_MASK) {
                case MPP_FMT_YUV420SP_10BIT:
                case MPP_FMT_YUV422SP_10BIT:
                case MPP_FMT_YUV444SP_10BIT: {
                    aligned_pixel = aligned_byte * 8 / 10;
                } break;
                case MPP_FMT_YUV422_YVYU:
                case MPP_FMT_YUV422_YUYV:
                case MPP_FMT_RGB565:
                case MPP_FMT_BGR565: {
                    aligned_pixel = aligned_byte / 2;
                } break;
                case MPP_FMT_RGB888:
                case MPP_FMT_BGR888: {
                    aligned_pixel = aligned_byte / 3;
                } break;
                default : {
                    aligned_pixel = aligned_byte;
                } break;
                }
            }
        }
        sys_cfg_dbg_dec_buf("dec hw performance aligned hor_byte: [%d]\n", aligned_pixel);

        cfg->h_stride_by_byte = aligned_byte;
        cfg->h_stride_by_pixel = aligned_pixel;
        cfg->v_stride = aligned_height;

        size_total = aligned_byte * aligned_height;
        size_total_old = size_total;
        sys_cfg_dbg_dec_buf("fmt_raw %x\n", fmt_raw);
        sys_cfg_dbg_dec_buf("res aligned_pixel %d\n", aligned_pixel);
        sys_cfg_dbg_dec_buf("res aligned_byte %d\n", aligned_byte);
        sys_cfg_dbg_dec_buf("res aligned_height %d\n", aligned_height);

        switch (fmt_raw) {
        case MPP_FMT_YUV420SP :
        case MPP_FMT_YUV420SP_10BIT :
        case MPP_FMT_YUV420P :
        case MPP_FMT_YUV420SP_VU : {
            SysCfgAlignType align_type = SYS_CFG_ALIGN_LEN_DEFAULT;

            /* hevc and vp9 - SYS_CFG_ALIGN_LEN_DEFAULT */
            if (type == MPP_VIDEO_CodingAV1)
                align_type = SYS_CFG_ALIGN_LEN_420_AV1;
            else if (type == MPP_VIDEO_CodingAVC)
                align_type = SYS_CFG_ALIGN_LEN_420;

            size_total = mpp_sys_cfg_align(align_type, size_total);
        } break;
        case MPP_FMT_YUV422SP :
        case MPP_FMT_YUV422SP_10BIT :
        case MPP_FMT_YUV422P :
        case MPP_FMT_YUV422SP_VU :
        case MPP_FMT_YUV422_YUYV :
        case MPP_FMT_YUV422_YVYU :
        case MPP_FMT_YUV422_UYVY :
        case MPP_FMT_YUV422_VYUY :
        case MPP_FMT_YUV440SP :
        case MPP_FMT_YUV411SP : {
            SysCfgAlignType align_type;

            if (type == MPP_VIDEO_CodingAVC)
                align_type = SYS_CFG_ALIGN_LEN_422_AVC;
            else if (type == MPP_VIDEO_CodingAV1)
                align_type = SYS_CFG_ALIGN_LEN_422_AV1;
            else
                align_type = SYS_CFG_ALIGN_LEN_422;

            size_total = mpp_sys_cfg_align(align_type, size_total);
        } break;
        case MPP_FMT_YUV400 : {
            /* do nothing */
        } break;
        case MPP_FMT_YUV444SP :
        case MPP_FMT_YUV444P :
        case MPP_FMT_YUV444SP_10BIT : {
            size_total = mpp_sys_cfg_align(SYS_CFG_ALIGN_LEN_444, size_total);
        } break;
        default : {
            size_total = size_total * 3 / 2;
        }
        }
        sys_cfg_dbg_dec_buf("res size total %d -> %d\n", size_total_old, size_total);

        cfg->size_total = size_total;
    }

    return MPP_OK;
}

MPP_RET mpp_sys_cfg_ioctl(MppSysCfg cfg)
{
    MppSysCfgSet *p = (MppSysCfgSet *)kmpp_obj_to_entry(cfg);

    if (!p) {
        mpp_loge_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    if (p->dec_buf_chk.enable) {
        mpp_sys_dec_buf_chk_proc(&p->dec_buf_chk);
        p->dec_buf_chk.enable = 0;
    }

    return MPP_OK;
}

#define MPP_CFG_SET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppSysCfg cfg, const char *name, in_type val) \
    { \
        KmppEntry *entry = NULL; \
        if (NULL == cfg || NULL == name) { \
            mpp_loge_f("invalid input cfg %p name %p\n", cfg, name); \
            return rk_nok; \
        } \
        kmpp_objdef_get_entry(mpp_sys_cfg_def, name, &entry); \
        if (!entry) \
            return rk_nok; \
        if (!entry->tbl.flag_offset) { \
            mpp_loge_f("can not set readonly cfg %s\n", name); \
            return rk_nok; \
        } \
        return kmpp_obj_tbl_set_##cfg_type(cfg, entry, val); \
    }

MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_s32, RK_S32, s32);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_u32, RK_U32, u32);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_s64, RK_S64, s64);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_u64, RK_U64, u64);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_ptr, void *, ptr);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_st,  void *, st);

#define MPP_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppSysCfg cfg, const char *name, in_type *val) \
    { \
        if (NULL == cfg || NULL == name) { \
            mpp_loge_f("invalid input cfg %p name %p\n", cfg, name); \
            return rk_nok; \
        } \
        return kmpp_obj_get_##cfg_type(cfg, name, val); \
    }

MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_s32, RK_S32, s32);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_u32, RK_U32, u32);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_s64, RK_S64, s64);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_u64, RK_U64, u64);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_ptr, void *, ptr);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_st,  void  , st);

void mpp_sys_cfg_show(void)
{
    MppTrie *trie = kmpp_objdef_get_trie(mpp_sys_cfg_def);
    MppTrieInfo *root;

    if (!trie)
        return ;

    root = mpp_trie_get_info_first(trie);

    mpp_logi("dumping valid configure string start\n");

    if (root) {
        MppTrieInfo *node = root;
        rk_s32 len = mpp_trie_get_name_max(trie);

        mpp_logi("%-*s %-6s | %6s | %4s | %4s\n", len, "name", "type", "offset", "size", "flag (hex)");

        do {
            if (mpp_trie_info_is_self(node))
                continue;

            if (node->ctx_len == sizeof(KmppEntry)) {
                KmppEntry *entry = (KmppEntry *)mpp_trie_info_ctx(node);

                mpp_logi("%-*s %-6s | %-6d | %-4d | %-4x %s\n", len, mpp_trie_info_name(node),
                         strof_elem_type(entry->tbl.elem_type), entry->tbl.elem_offset,
                         entry->tbl.elem_size, entry->tbl.flag_offset,
                         entry->tbl.flag_offset ? "wr" : "ro");
            } else {
                mpp_logi("%-*s size - %d\n", len, mpp_trie_info_name(node), node->ctx_len);
            }
        } while ((node = mpp_trie_get_info_next(trie, node)));
    }

    mpp_logi("dumping valid configure string done\n");

    mpp_logi("sys cfg size %d count %d with trie node %d size %d\n",
             sizeof(MppSysCfgSet), mpp_trie_get_info_count(trie),
             mpp_trie_get_node_count(trie), mpp_trie_get_buf_size(trie));
}
