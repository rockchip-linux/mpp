/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys_cfg_st"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "rk_mpp_cfg.h"
#include "mpp_sys_cfg.h"
#include "mpp_sys_cfg_st.h"

MPP_RET mpp_sys_cfg_st_get_h_stride(MppSysCfgStHStrd *h_stride_cfg)
{
    MPP_RET ret = MPP_OK;
    MppSysCfg cfg;

    ret = mpp_sys_cfg_get(&cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_get failed\n");
        goto DONE;
    }

    /* set correct parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:enable", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", h_stride_cfg->type);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_fbc", h_stride_cfg->fmt_fbc);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:width", h_stride_cfg->width);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:h_stride_by_byte", h_stride_cfg->h_stride_by_byte);

    /* get result */
    mpp_sys_cfg_ioctl(cfg);

    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_pixel",
                              &h_stride_cfg->h_stride_by_pixel);

    ret = mpp_sys_cfg_put(cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_put failed\n");
        goto DONE;
    }

DONE:
    mpp_err_f("sys cfg get paras %s\n", ret ? "failed" : "success");
    return ret;
}

MPP_RET mpp_sys_cfg_st_get_byte_stride(MppSysCfgStHByteStrd *byte_stride_cfg)
{
    MPP_RET ret = MPP_OK;
    MppSysCfg cfg;

    ret = mpp_sys_cfg_get(&cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_get failed\n");
        goto DONE;
    }

    /* set correct parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:enable", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", byte_stride_cfg->type);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_codec", byte_stride_cfg->fmt_codec);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_fbc", byte_stride_cfg->fmt_fbc);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:width", byte_stride_cfg->width);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:h_stride_by_byte", byte_stride_cfg->h_stride_by_byte);

    /* get result */
    mpp_sys_cfg_ioctl(cfg);

    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_byte",
                              &byte_stride_cfg->h_stride_by_byte);

    ret = mpp_sys_cfg_put(cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_put failed\n");
        goto DONE;
    }

DONE:
    mpp_err_f("sys cfg get paras %s\n", ret ? "failed" : "success");
    return ret;
}

MPP_RET mpp_sys_cfg_st_get_v_stride(MppSysCfgStVStrd *v_stride_cfg)
{
    MPP_RET ret = MPP_OK;
    MppSysCfg cfg;

    ret = mpp_sys_cfg_get(&cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_get failed\n");
        goto DONE;
    }

    /* set correct parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:enable", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", v_stride_cfg->type);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_fbc", v_stride_cfg->fmt_fbc);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:height", v_stride_cfg->height);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:v_stride", v_stride_cfg->v_stride);

    /* get result */
    mpp_sys_cfg_ioctl(cfg);

    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:v_stride", &v_stride_cfg->v_stride);

    ret = mpp_sys_cfg_put(cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_put failed\n");
        goto DONE;
    }

DONE:
    mpp_err_f("sys cfg get paras %s\n", ret ? "failed" : "success");
    return ret;
}

MPP_RET mpp_sys_cfg_st_get_size(MppSysCfgStSize *size_cfg)
{
    MPP_RET ret = MPP_OK;
    MppSysCfg cfg;

    ret = mpp_sys_cfg_get(&cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_get failed\n");
        goto DONE;
    }

    /* set correct parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:enable", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", size_cfg->type);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_codec", size_cfg->fmt_codec);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_fbc", size_cfg->fmt_fbc);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:width", size_cfg->width);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:height", size_cfg->height);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:h_stride_by_byte", size_cfg->h_stride_by_byte);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:v_stride", size_cfg->v_stride);

    /* get result */
    mpp_sys_cfg_ioctl(cfg);

    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_byte", &size_cfg->h_stride_by_byte);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_pixel", &size_cfg->h_stride_by_pixel);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:v_stride", &size_cfg->v_stride);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_total", &size_cfg->size_total);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_fbc_hdr", &size_cfg->size_fbc_hdr);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_fbc_bdy", &size_cfg->size_fbc_bdy);

    ret = mpp_sys_cfg_put(cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_put failed\n");
        goto DONE;
    }

DONE:
    mpp_err_f("sys cfg get paras %s\n", ret ? "failed" : "success");
    return ret;
}
