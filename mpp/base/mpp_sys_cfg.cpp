/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys_cfg"

#include <string.h>

#include "rk_mpp_cfg.h"

#include "mpp_env.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_common.h"
#include "mpp_thread.h"
#include "mpp_list.h"

#include "mpp_cfg.h"
#include "mpp_trie.h"
#include "mpp_sys_cfg.h"
#include "mpp_soc.h"
#include "mpp_mem_pool.h"

#define MPP_SYS_CFG_DBG_FUNC            (0x00000001)
#define MPP_SYS_CFG_DBG_INFO            (0x00000002)
#define MPP_SYS_CFG_DBG_SET             (0x00000004)
#define MPP_SYS_CFG_DBG_GET             (0x00000008)

#define mpp_sys_cfg_dbg(flag, fmt, ...) _mpp_dbg_f(mpp_sys_cfg_debug, flag, fmt, ## __VA_ARGS__)

#define mpp_sys_cfg_dbg_func(fmt, ...)  mpp_sys_cfg_dbg(MPP_SYS_CFG_DBG_FUNC, fmt, ## __VA_ARGS__)
#define mpp_sys_cfg_dbg_info(fmt, ...)  mpp_sys_cfg_dbg(MPP_SYS_CFG_DBG_INFO, fmt, ## __VA_ARGS__)
#define mpp_sys_cfg_dbg_set(fmt, ...)   mpp_sys_cfg_dbg(MPP_SYS_CFG_DBG_SET, fmt, ## __VA_ARGS__)
#define mpp_sys_cfg_dbg_get(fmt, ...)   mpp_sys_cfg_dbg(MPP_SYS_CFG_DBG_GET, fmt, ## __VA_ARGS__)

#define SYS_CFG_CNT 3

RK_U32 mpp_sys_cfg_debug = 0;
static MppMemPool mpp_sys_cfg_pool = mpp_mem_pool_init_f(MODULE_TAG, sizeof(MppSysCfgSet));

class MppSysCfgService
{
private:
    MppSysCfgService();
    ~MppSysCfgService();
    MppSysCfgService(const MppSysCfgService &);
    MppSysCfgService &operator=(const MppSysCfgService &);

    MppCfgInfoHead mHead;
    MppTrie mTrie;

public:
    static MppSysCfgService *get_ins() {
        static MppSysCfgService instance;

        return &instance;
    }

    MppSysCfgSet *get_cfg();
    MPP_RET put_cfg(MppSysCfgSet *);

    MppTrieInfo *get_info(const char *name);
    MppTrieInfo *get_info_first();
    MppTrieInfo *get_info_next(MppTrieInfo *info);

    RK_S32 get_node_count() { return mHead.node_count; };
    RK_S32 get_info_count() { return mHead.info_count; };
    RK_S32 get_info_size() { return mHead.info_size; };
};

#define EXPAND_AS_TRIE(base, name, cfg_type, in_type, flag, field_change, field_data) \
    do { \
        MppCfgInfo tmp = { \
            CFG_FUNC_TYPE_##cfg_type, \
            flag > 0 ? 1 : 0, \
            (RK_U32)((long)&(((MppSysCfgSet *)0)->field_change.change)), \
            flag, \
            (RK_U32)((long)&(((MppSysCfgSet *)0)->field_change.field_data)), \
            sizeof((((MppSysCfgSet *)0)->field_change.field_data)), \
        }; \
        mpp_trie_add_info(mTrie, #base":"#name, &tmp, sizeof(tmp)); \
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
    /* read-only config */ \
    ENTRY(dec_buf_chk, cap_fbc,     U32, RK_U32,            0,                                              dec_buf_chk, cap_fbc) \
    ENTRY(dec_buf_chk, cap_tile,    U32, RK_U32,            0,                                              dec_buf_chk, cap_tile) \
    ENTRY(dec_buf_chk, h_stride_by_byte,    U32, RK_U32,    0,                                              dec_buf_chk, h_stride_by_byte) \
    ENTRY(dec_buf_chk, h_stride_by_pixel,   U32, RK_U32,    0,                                              dec_buf_chk, h_stride_by_pixel) \
    ENTRY(dec_buf_chk, v_stride,    U32, RK_U32,            0,                                              dec_buf_chk, v_stride) \
    ENTRY(dec_buf_chk, offset_y,    U32, RK_U32,            0,                                              dec_buf_chk, offset_y) \
    ENTRY(dec_buf_chk, size_total,  U32, RK_U32,            0,                                              dec_buf_chk, size_total) \
    ENTRY(dec_buf_chk, size_fbc_hdr, U32, RK_U32,           0,                                              dec_buf_chk, size_fbc_hdr) \
    ENTRY(dec_buf_chk, size_fbc_bdy, U32, RK_U32,           0,                                              dec_buf_chk, size_fbc_bdy) \
    ENTRY(dec_buf_chk, size_metadata,   U32, RK_U32,        0,                                              dec_buf_chk, size_metadata) \
    ENTRY(dec_buf_chk, size_thumbnail,  U32, RK_U32,        0,                                              dec_buf_chk, size_thumbnail)

MppSysCfgService::MppSysCfgService() :
    mTrie(NULL)
{
    MPP_RET ret = mpp_trie_init(&mTrie, "MppSysCfg");
    if (ret) {
        mpp_err_f("failed to init dec cfg set trie\n");
        return ;
    }

    ENTRY_TABLE(EXPAND_AS_TRIE)

    mpp_trie_add_info(mTrie, NULL, NULL, 0);

    mHead.node_count = mpp_trie_get_node_count(mTrie);
    mHead.info_count = mpp_trie_get_info_count(mTrie);
    mHead.info_size = mpp_trie_get_buf_size(mTrie);

    mpp_sys_cfg_dbg_func("node cnt: %d\n", mHead.node_count);
}

MppSysCfgService::~MppSysCfgService()
{
    if (mTrie) {
        mpp_trie_deinit(mTrie);
        mTrie = NULL;
    }
}

MppSysCfgSet *MppSysCfgService::get_cfg()
{
    MppSysCfgSet *node;

    node = (MppSysCfgSet*)mpp_mem_pool_get(mpp_sys_cfg_pool);
    node->dec_buf_chk.type = MPP_VIDEO_CodingUnused;

    return node;
}

MPP_RET MppSysCfgService::put_cfg(MppSysCfgSet *node)
{
    mpp_mem_pool_put(mpp_sys_cfg_pool, node);

    return MPP_OK;
}

MppTrieInfo *MppSysCfgService::get_info(const char *name)
{
    return mpp_trie_get_info(mTrie, name);
}

MppTrieInfo *MppSysCfgService::get_info_first()
{
    return mpp_trie_get_info_first(mTrie);
}

MppTrieInfo *MppSysCfgService::get_info_next(MppTrieInfo *info)
{
    return mpp_trie_get_info_next(mTrie, info);
}

MPP_RET mpp_sys_cfg_get(MppSysCfg *cfg)
{
    if (NULL == cfg) {
        mpp_err_f("invalid NULL input config\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_env_get_u32("mpp_sys_cfg_debug", &mpp_sys_cfg_debug, 0);

    *cfg = MppSysCfgService::get_ins()->get_cfg();

    return *cfg ? MPP_OK : MPP_NOK;
}

MPP_RET mpp_sys_cfg_put(MppSysCfg cfg)
{
    return MppSysCfgService::get_ins()->put_cfg((MppSysCfgSet *)cfg);
}
MPP_RET mpp_sys_dec_buf_chk_proc(MppSysDecBufChkCfg *cfg)
{
    MppCodingType type = cfg->type;
    MppFrameFormat fmt = (MppFrameFormat)(((RK_U32)cfg->fmt_codec & MPP_FRAME_FMT_MASK) |
                                          (cfg->fmt_fbc & MPP_FRAME_FBC_MASK) |
                                          (cfg->fmt_hdr & MPP_FRAME_HDR_MASK));
    MppFrameFormat fmt_raw = cfg->fmt_codec;

    if (MPP_FRAME_FMT_IS_FBC(fmt)) {
        /* fbc case */
    } else if (MPP_FRAME_FMT_IS_TILE(fmt)) {
        /* tile case */
    } else {
        /* raster case */
        RK_U32 aligned_pixel;
        RK_U32 aligned_pixel_byte;
        RK_U32 aligned_byte;
        RK_U32 aligned_height;
        RK_U32 size_total;

        switch (type) {
        case MPP_VIDEO_CodingHEVC :
        case MPP_VIDEO_CodingVP9 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 64);
            aligned_height = MPP_ALIGN(cfg->height, 64);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            aligned_pixel = MPP_ALIGN(cfg->width, 128);
            aligned_height = MPP_ALIGN(cfg->height, 128);
        } break;
        default : {
            aligned_pixel = MPP_ALIGN(cfg->width, 16);
            aligned_height = MPP_ALIGN(cfg->height, 16);
        } break;
        }

        if (MPP_FRAME_FMT_IS_YUV_10BIT(fmt))
            aligned_pixel_byte = aligned_pixel * 10 / 8;
        else
            aligned_pixel_byte = aligned_pixel;

        switch (type) {
        case MPP_VIDEO_CodingHEVC :
        case MPP_VIDEO_CodingVP9 : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 64);
        } break;
        case MPP_VIDEO_CodingAV1 : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 128);
        } break;
        default : {
            aligned_byte = MPP_ALIGN(aligned_pixel_byte, 16);
        } break;
        }

        if (aligned_byte > 1920 && type != MPP_VIDEO_CodingMJPEG) {
            RockchipSocType soc_type = mpp_get_soc_type();

            switch (soc_type) {
            case ROCKCHIP_SOC_RK3568 :
            case ROCKCHIP_SOC_RK3562 :
            case ROCKCHIP_SOC_RK3528 :
            case ROCKCHIP_SOC_RK3588 : {
                aligned_byte = mpp_align_256_odd(aligned_byte);
            } break;
            case ROCKCHIP_SOC_RK3576 : {
                aligned_byte = mpp_align_128_odd_plus_64(aligned_byte);
            } break;
            default : {
            } break;
            }
        }

        cfg->h_stride_by_byte = aligned_byte;
        cfg->h_stride_by_pixel = aligned_pixel;
        cfg->v_stride = aligned_height;

        switch (fmt_raw) {
        case MPP_FMT_YUV420SP :
        case MPP_FMT_YUV420SP_10BIT :
        case MPP_FMT_YUV420P :
        case MPP_FMT_YUV420SP_VU : {
            size_total = aligned_byte * aligned_height * 3 / 2;
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
            size_total = aligned_byte * aligned_height * 2;
        } break;
        case MPP_FMT_YUV400 : {
            size_total = aligned_byte * aligned_height;
        } break;
        case MPP_FMT_YUV444SP :
        case MPP_FMT_YUV444P :
        case MPP_FMT_YUV444SP_10BIT : {
            size_total = aligned_byte * aligned_height * 3;
        } break;
        default : {
            size_total = aligned_byte * aligned_height * 3 / 2;
        }
        }

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
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppSysCfgSet *p = (MppSysCfgSet *)cfg; \
        MppTrieInfo *node = MppSysCfgService::get_ins()->get_info(name); \
        MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        if (!info->flag_value) { \
            mpp_log_f("can not set readonly cfg %s\n", mpp_trie_info_name(node)); \
            return MPP_NOK; \
        } \
        mpp_sys_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), \
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
        if (NULL == cfg || NULL == name) { \
            mpp_err_f("invalid input cfg %p name %p\n", cfg, name); \
            return MPP_ERR_NULL_PTR; \
        } \
        MppSysCfgSet *p = (MppSysCfgSet *)cfg; \
        MppTrieInfo *node = MppSysCfgService::get_ins()->get_info(name); \
        MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node); \
        if (CHECK_CFG_INFO(info, name, CFG_FUNC_TYPE_##cfg_type)) { \
            return MPP_NOK; \
        } \
        mpp_sys_cfg_dbg_set("name %s type %s\n", mpp_trie_info_name(node), \
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
    MppSysCfgService *srv = MppSysCfgService::get_ins();
    MppTrieInfo *root = srv->get_info_first();

    mpp_log("dumping valid configure string start\n");

    if (root) {
        MppTrieInfo *node = root;

        do {
            MppCfgInfo *info = (MppCfgInfo *)mpp_trie_info_ctx(node);

            mpp_log("%-25s type %s\n", mpp_trie_info_name(node),
                    strof_cfg_type(info->data_type));

            node = srv->get_info_next(node);
            if (!node)
                break;
        } while (1);
    }
    mpp_log("dumping valid configure string done\n");

    mpp_log("total cfg count %d with %d node size %d\n",
            srv->get_info_count(), srv->get_node_count(), srv->get_info_size());
}
