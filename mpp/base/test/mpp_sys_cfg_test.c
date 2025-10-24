/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_sys_cfg_test"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "rk_mpp_cfg.h"
#include "mpp_sys_cfg.h"

int main(void)
{
    MPP_RET ret = MPP_OK;
    MppSysCfg cfg;
    RK_S64 end = 0;
    RK_S64 start = 0;
    MppCodingType type = MPP_VIDEO_CodingHEVC;
    RK_U32 width = 4096;
    RK_U32 height = 2304;
    RK_U32 h_stride_by_byte;
    RK_U32 h_stride_by_pixel;
    RK_U32 v_stride;
    RK_U32 size_total;
    RK_U32 size_fbc_hdr;
    RK_U32 size_fbc_bdy;

    mpp_sys_cfg_show();

    mpp_log("mpp_sys_cfg_test start\n");

    start = mpp_time();

    ret = mpp_sys_cfg_get(&cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_get failed\n");
        goto DONE;
    }

    /* set correct parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:enable", 1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:type", type);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_codec", MPP_FMT_YUV420SP);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:fmt_fbc", MPP_FRAME_FBC_AFBC_V1);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:width", width);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:height", height);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:h_stride_by_byte", 0);
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:h_stride_by_pixel", 0);

    /* try get readonly parameter */
    ret = mpp_sys_cfg_set_u32(cfg, "dec_buf_chk:cap_fbc", 1);
    if (!ret) {
        mpp_log("set readonly success, should be a failure\n");
        goto DONE;
    }

    /* get result */
    mpp_sys_cfg_ioctl(cfg);

    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_byte", &h_stride_by_byte);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:h_stride_by_pixel", &h_stride_by_pixel);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:v_stride", &v_stride);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_total", &size_total);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_fbc_hdr", &size_fbc_hdr);
    ret = mpp_sys_cfg_get_u32(cfg, "dec_buf_chk:size_fbc_bdy", &size_fbc_bdy);

    ret = mpp_sys_cfg_put(cfg);
    if (ret) {
        mpp_err("mpp_sys_cfg_put failed\n");
        goto DONE;
    }

    end = mpp_time();
    mpp_log("set u32 time %lld us\n", end - start);

DONE:
    mpp_log("mpp_sys_cfg_test done %s\n", ret ? "failed" : "success");
    return ret;
}
