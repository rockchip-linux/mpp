/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2024 Rockchip Electronics Co., Ltd.
 */

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "rk_mpp_cfg.h"
#include "mpp_sys_cfg_st.h"

int main(void)
{
    MPP_RET ret = MPP_OK;
    MppCodingType type = MPP_VIDEO_CodingHEVC;
    RK_U32 width = 4096;
    RK_U32 height = 2304;

    MppSysCfgStHStrd h_stride_cfg;
    MppSysCfgStHByteStrd byte_stride_cfg;
    MppSysCfgStVStrd v_stride_cfg;
    MppSysCfgStSize size_cfg;


    mpp_log("mpp_sys_cfg_st_test h_stride start\n");
    h_stride_cfg.type = type;
    h_stride_cfg.fmt_fbc = MPP_FRAME_FBC_AFBC_V1;
    h_stride_cfg.width = width;
    ret = mpp_sys_cfg_st_get_h_stride(&h_stride_cfg);
    mpp_log("mpp_sys_cfg_st_test result: h_stride_by_pixel(%d)",
            h_stride_cfg.h_stride_by_pixel);
    mpp_log("mpp_sys_cfg_st_test h_stride done %s\n", ret ? "failed" : "success");


    mpp_log("mpp_sys_cfg_st_test byte_stride start\n");
    byte_stride_cfg.type = type;
    byte_stride_cfg.fmt_codec = MPP_FMT_YUV420SP;
    byte_stride_cfg.fmt_fbc = MPP_FRAME_FBC_AFBC_V1;
    byte_stride_cfg.width = width;
    ret = mpp_sys_cfg_st_get_byte_stride(&byte_stride_cfg);
    mpp_log("mpp_sys_cfg_st_test result: h_stride_by_byte(%d)",
            byte_stride_cfg.h_stride_by_byte);
    mpp_log("mpp_sys_cfg_st_test byte_stride done %s\n", ret ? "failed" : "success");


    mpp_log("mpp_sys_cfg_st_test v_stride_cfg start\n");
    v_stride_cfg.type = type;
    v_stride_cfg.fmt_fbc = MPP_FRAME_FBC_AFBC_V1;
    v_stride_cfg.height = height;
    ret = mpp_sys_cfg_st_get_v_stride(&v_stride_cfg);
    mpp_log("mpp_sys_cfg_st_test result: v_stride(%d)",
            v_stride_cfg.v_stride);
    mpp_log("mpp_sys_cfg_st_test v_stride_cfg done %s\n", ret ? "failed" : "success");


    mpp_log("mpp_sys_cfg_st_test size_cfg start\n");
    size_cfg.type = type;
    size_cfg.fmt_codec = MPP_FMT_YUV420SP;
    size_cfg.fmt_fbc = MPP_FRAME_FBC_AFBC_V1;
    size_cfg.width = width;
    size_cfg.height = height;
    ret = mpp_sys_cfg_st_get_size(&size_cfg);
    mpp_log("mpp_sys_cfg_st_test result: h_stride_by_pixel(%d)",
            size_cfg.h_stride_by_pixel);
    mpp_log("mpp_sys_cfg_st_test result: h_stride_by_byte(%d)",
            size_cfg.h_stride_by_byte);
    mpp_log("mpp_sys_cfg_st_test result: v_stride(%d)",
            size_cfg.v_stride);
    mpp_log("mpp_sys_cfg_st_test result: size_total(%d)",
            size_cfg.size_total);
    mpp_log("mpp_sys_cfg_st_test result: size_fbc_hdr(%d)",
            size_cfg.size_fbc_hdr);
    mpp_log("mpp_sys_cfg_st_test result: size_fbc_bdy(%d)",
            size_cfg.size_fbc_bdy);
    mpp_log("mpp_sys_cfg_st_test size_cfg done %s\n", ret ? "failed" : "success");


    return ret;
}
