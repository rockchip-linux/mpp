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

#define SYS_CFG_DBG_FUNC                (0x00000001)
#define SYS_CFG_DBG_INFO                (0x00000002)
#define SYS_CFG_DBG_SET                 (0x00000004)
#define SYS_CFG_DBG_GET                 (0x00000008)
#define SYS_CFG_DBG_DEC_BUF             (0x00000010)

#define sys_cfg_dbg(flag, fmt, ...)     _mpp_dbg_f(mpp_sys_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define sys_cfg_dbg_func(fmt, ...)      sys_cfg_dbg(SYS_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_info(fmt, ...)      sys_cfg_dbg(SYS_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_set(fmt, ...)       sys_cfg_dbg(SYS_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_get(fmt, ...)       sys_cfg_dbg(SYS_CFG_DBG_GET, fmt, ## __VA_ARGS__)
#define sys_cfg_dbg_dec_buf(fmt, ...)   sys_cfg_dbg(SYS_CFG_DBG_DEC_BUF, fmt, ## __VA_ARGS__)

#define get_srv_sys_cfg_f() \
    ({ \
        MppSysCfgSrv *__tmp; \
        if (srv_sys_cfg) { \
            __tmp = srv_sys_cfg; \
        } else { \
            mpp_sys_cfg_srv_init(); \
            __tmp = srv_sys_cfg; \
            if (!__tmp) \
                mpp_err("mpp sys cfg srv not init at %s\n", __FUNCTION__); \
        } \
        __tmp; \
    })

typedef struct MppSysCfgSrv_t {
    MppTrie trie;
    MppMemPool pool;
} MppSysCfgSrv;

static MppSysCfgSrv *srv_sys_cfg = NULL;
static RK_U32 mpp_sys_cfg_debug = 0;

#define EXPAND_AS_TRIE(base, name, cfg_type, in_type, flag, field_change, field_data) \
    do { \
        MppCfgInfo tmp = { \
            CFG_FUNC_TYPE_##cfg_type, \
            (RK_U32)((long)&(((MppSysCfgSet *)0)->field_change.change)), \
            flag, \
            (RK_U32)((long)&(((MppSysCfgSet *)0)->field_change.field_data)), \
            sizeof((((MppSysCfgSet *)0)->field_change.field_data)), \
        }; \
        mpp_trie_add_info(srv->trie, #base":"#name, &tmp, sizeof(tmp)); \
    } while (0);

#define ENTRY_TABLE(ENTRY)  \
    ENTRY(dec_buf_chk, enable,      U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_ENABLE,          dec_buf_chk, enable) \
    ENTRY(dec_buf_chk, type,        U32, MppCodingType,     MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_TYPE,            dec_buf_chk, type) \
    ENTRY(dec_buf_chk, fmt_codec,   U32, MppFrameFormat,    MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_CODEC,       dec_buf_chk, fmt_codec) \
    ENTRY(dec_buf_chk, fmt_fbc,     U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_FBC,         dec_buf_chk, fmt_fbc) \
    ENTRY(dec_buf_chk, fmt_hdr,     U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FMT_HDR,         dec_buf_chk, fmt_hdr) \
    ENTRY(dec_buf_chk, width,       U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_WIDTH,           dec_buf_chk, width) \
    ENTRY(dec_buf_chk, height,      U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_HEIGHT,          dec_buf_chk, height) \
    ENTRY(dec_buf_chk, crop_top,    U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_TOP,        dec_buf_chk, crop_top) \
    ENTRY(dec_buf_chk, crop_bottom, U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_BOTTOM,     dec_buf_chk, crop_bottom) \
    ENTRY(dec_buf_chk, crop_left,   U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_LEFT,       dec_buf_chk, crop_left) \
    ENTRY(dec_buf_chk, crop_right,  U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_RIGHT,      dec_buf_chk, crop_right) \
    ENTRY(dec_buf_chk, unit_size,   U32, RK_U32,            MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_CROP_RIGHT,      dec_buf_chk, unit_size) \
    ENTRY(dec_buf_chk, has_metadata,    U32, RK_U32,        MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FLAG_METADATA,   dec_buf_chk, has_metadata) \
    ENTRY(dec_buf_chk, has_thumbnail,   U32, RK_U32,        MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_FLAG_THUMBNAIL,  dec_buf_chk, has_thumbnail) \
    ENTRY(dec_buf_chk, h_stride_by_byte,  U32, RK_U32,      MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_H_STRIDE_BYTE,   dec_buf_chk, h_stride_by_byte) \
    ENTRY(dec_buf_chk, v_stride,          U32, RK_U32,      MPP_SYS_DEC_BUF_CHK_CFG_CHANGE_V_STRIDE,        dec_buf_chk, v_stride) \
    /* read-only config */ \
    ENTRY(dec_buf_chk, cap_fbc,     U32, RK_U32,            0,                                              dec_buf_chk, cap_fbc) \
    ENTRY(dec_buf_chk, cap_tile,    U32, RK_U32,            0,                                              dec_buf_chk, cap_tile) \
    ENTRY(dec_buf_chk, h_stride_by_pixel,   U32, RK_U32,    0,                                              dec_buf_chk, h_stride_by_pixel) \
    ENTRY(dec_buf_chk, offset_y,    U32, RK_U32,            0,                                              dec_buf_chk, offset_y) \
    ENTRY(dec_buf_chk, size_total,  U32, RK_U32,            0,                                              dec_buf_chk, size_total) \
    ENTRY(dec_buf_chk, size_fbc_hdr, U32, RK_U32,           0,                                              dec_buf_chk, size_fbc_hdr) \
    ENTRY(dec_buf_chk, size_fbc_bdy, U32, RK_U32,           0,                                              dec_buf_chk, size_fbc_bdy) \
    ENTRY(dec_buf_chk, size_metadata,   U32, RK_U32,        0,                                              dec_buf_chk, size_metadata) \
    ENTRY(dec_buf_chk, size_thumbnail,  U32, RK_U32,        0,                                              dec_buf_chk, size_thumbnail)

static void mpp_sys_cfg_srv_init()
{
    MppSysCfgSrv *srv = srv_sys_cfg;

    mpp_env_get_u32("mpp_sys_cfg_debug", &mpp_sys_cfg_debug, mpp_sys_cfg_debug);

    if (srv)
        return ;

    srv = mpp_calloc(MppSysCfgSrv, 1);
    if (!srv) {
        mpp_err_f("failed to allocate sys cfg set service\n");
        return ;
    }

    mpp_trie_init(&srv->trie, "MppSysCfg");
    srv->pool = mpp_mem_pool_init_f(MODULE_TAG, sizeof(MppSysCfgSet));
    if (!srv->trie || !srv->pool) {
        mpp_err_f("failed to init sys cfg set service\n");
        if (srv->trie) {
            mpp_trie_deinit(srv->trie);
            srv->trie = NULL;
        }
        if (srv->pool) {
            mpp_mem_pool_deinit_f(srv->pool);
            srv->pool = NULL;
        }
        MPP_FREE(srv);
        return ;
    }

    srv_sys_cfg = srv;

    ENTRY_TABLE(EXPAND_AS_TRIE)
    mpp_trie_add_info(srv->trie, NULL, NULL, 0);

    sys_cfg_dbg_func("info cnt %d node cnt %d size %d\n",
                     mpp_trie_get_info_count(srv->trie),
                     mpp_trie_get_node_count(srv->trie),
                     mpp_trie_get_buf_size(srv->trie));
}

static void mpp_sys_cfg_srv_deinit()
{
    MppSysCfgSrv *srv = srv_sys_cfg;

    if (!srv)
        return ;

    if (srv->trie) {
        mpp_trie_deinit(srv->trie);
        srv->trie = NULL;
    }
    if (srv->pool) {
        mpp_mem_pool_deinit_f(srv->pool);
        srv->pool = NULL;
    }

    MPP_FREE(srv_sys_cfg);
}

MPP_SINGLETON(MPP_SGLN_SYS_CFG, mpp_sys_cfg, mpp_sys_cfg_srv_init, mpp_sys_cfg_srv_deinit)

static MppSysCfgSet *srv_get_cfg(MppSysCfgSrv *srv)
{
    MppSysCfgSet *node = (MppSysCfgSet*)mpp_mem_pool_get_f(srv->pool);

    node->dec_buf_chk.type = MPP_VIDEO_CodingUnused;

    return node;
}

static MPP_RET srv_put_cfg(MppSysCfgSrv *srv, MppSysCfgSet *node)
{
    mpp_mem_pool_put_f(srv->pool, node);

    return MPP_OK;
}

MPP_RET mpp_sys_cfg_get(MppSysCfg *cfg)
{
    MppSysCfgSrv *srv = get_srv_sys_cfg_f();

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    *cfg = NULL;

    if (!srv)
        return MPP_NOK;

    mpp_env_get_u32("mpp_sys_cfg_debug", &mpp_sys_cfg_debug, mpp_sys_cfg_debug);

    *cfg = srv_get_cfg(srv);

    return *cfg ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_sys_cfg_put(MppSysCfg cfg)
{
    MppSysCfgSrv *srv = get_srv_sys_cfg_f();

    if (!srv)
        return MPP_NOK;

    return srv_put_cfg(srv, (MppSysCfgSet *)cfg);
}

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
        /* fbc case */
        switch (type) {
        case MPP_VIDEO_CodingHEVC :
        case MPP_VIDEO_CodingAV1 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(aligned_height ? aligned_height : cfg->height, 16);
        } break;
        case MPP_VIDEO_CodingAVC :
        case MPP_VIDEO_CodingAVSPLUS :
        case MPP_VIDEO_CodingAVS :
        case MPP_VIDEO_CodingAVS2 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(aligned_height ? aligned_height : cfg->height, 16);
        } break;
        case MPP_VIDEO_CodingVP9 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(aligned_height ? aligned_height : cfg->height, 64);
        } break;
        default : {
            aligned_pixel = MPP_ALIGN(cfg->width, 16);
            aligned_height = MPP_ALIGN(aligned_height ? aligned_height : cfg->height, 16);
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
        RockchipSocType soc_type = mpp_get_soc_type();

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

        aligned_pixel_byte = cfg->h_stride_by_byte ? cfg->h_stride_by_byte :
                             aligned_pixel * depth / 8;
        aligned_height = cfg->v_stride ? cfg->v_stride : aligned_height;

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
    MppSysCfgSet *p = (MppSysCfgSet *)cfg;

    if (!cfg) {
        mpp_err_f("invalid NULL input config\n");
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
        MppSysCfgSrv *srv = get_srv_sys_cfg_f(); \
        MppSysCfgSet *p; \
        MppTrieInfo *node; \
        MppCfgInfo *info; \
        if (!srv) \
            return MPP_NOK; \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        p = (MppSysCfgSet *)cfg; \
        node = mpp_trie_get_info(srv->trie, name); \
        info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        if (!info->flag_value) { \
            mpp_log_f("can not set readonly cfg %s\n", mpp_trie_info_name(node)); \
            return MPP_NOK; \
        } \
        sys_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), \
                        strof_cfg_type(info->data_type)); \
        MPP_RET ret = MPP_CFG_SET_##cfg_type(info, p, val); \
        return ret; \
    }

MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_s32, RK_S32, S32);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_u32, RK_U32, U32);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_s64, RK_S64, S64);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_u64, RK_U64, U64);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_ptr, void *, Ptr);
MPP_CFG_SET_ACCESS(mpp_sys_cfg_set_st,  void *, St);

#define MPP_CFG_GET_ACCESS(func_name, in_type, cfg_type) \
    MPP_RET func_name(MppSysCfg cfg, const char *name, in_type *val) \
    { \
        MppSysCfgSrv *srv = get_srv_sys_cfg_f(); \
        MppSysCfgSet *p; \
        MppTrieInfo *node; \
        MppCfgInfo *info; \
        if (!srv) \
            return MPP_NOK; \
        if (!cfg || !name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        p = (MppSysCfgSet *)cfg; \
        node = mpp_trie_get_info(srv->trie, name); \
        info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        sys_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), \
                            strof_cfg_type(info->data_type)); \
        MPP_RET ret = MPP_CFG_GET_##cfg_type(info, p, val); \
        return ret; \
    }

MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_s32, RK_S32, S32);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_u32, RK_U32, U32);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_s64, RK_S64, S64);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_u64, RK_U64, U64);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_ptr, void *, Ptr);
MPP_CFG_GET_ACCESS(mpp_sys_cfg_get_st,  void  , St);

void mpp_sys_cfg_show(void)
{
    MppSysCfgSrv *srv = get_srv_sys_cfg_f();
    MppTrieInfo *root;
    MppTrie trie;

    if (!srv)
        return ;

    trie = srv->trie;
    root = mpp_trie_get_info_first(trie);

    mpp_log("dumping valid configure string start\n");

    if (root) {
        MppTrieInfo *node = root;

        do {
            MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node);

            mpp_log("%-25s type %s\n", mpp_trie_info_name(node),
                    strof_cfg_type(info->data_type));

            node = mpp_trie_get_info_next(trie, node);
            if (!node)
                break;
        } while (1);
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("total cfg count %d with %d node size %d\n",
            mpp_trie_get_info_count(trie),
            mpp_trie_get_node_count(trie),
            mpp_trie_get_buf_size(trie));
}
