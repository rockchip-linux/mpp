/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_enc_cfg_test"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "rk_venc_cfg.h"
#include "mpp_enc_cfg_impl.h"

int main()
{
    MPP_RET ret = MPP_OK;
    MppEncCfg cfg;
    RK_S64 end = 0;
    RK_S64 start = 0;

    mpp_enc_cfg_show();

    mpp_log("mpp_enc_cfg_test start\n");

    ret = mpp_enc_cfg_init(&cfg);
    if (ret) {
        mpp_err("mpp_enc_cfg_init failed\n");
        goto DONE;
    }

    RK_S32 rc_mode = 1;
    RK_S32 bps_target = 400000;
    RK_S32 aq_thrd_i[16] = {
        0,  0,  0,  0,
        3,  3,  5,  5,
        8,  8,  8,  15,
        15, 20, 25, 35
    };

    RK_S32 aq_thrd_i_ret[16] = {
        -1, -1, -1, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1,
        -1, -1, -1, -1,
    };

    MppEncCfgSet *impl = (MppEncCfgSet *)kmpp_obj_to_entry(cfg);

    mpp_log("before set: rc mode %d bps_target %d\n",
            impl->rc.rc_mode, impl->rc.bps_target);

    start = mpp_time();
    ret = mpp_enc_cfg_set_u32(cfg, "rc:mode", rc_mode);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:bps", 400000);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bps_target);
    end = mpp_time();
    mpp_log("set s32 time %lld us\n", end - start);

    mpp_log("after  set: rc mode %d bps_target %d\n",
            impl->rc.rc_mode, impl->rc.bps_target);

    rc_mode = 0;
    bps_target = 0;

    mpp_log("before get: rc mode %d bps_target %d\n", rc_mode, bps_target);

    ret = mpp_enc_cfg_get_s32(cfg, "rc:mode", &rc_mode);
    ret = mpp_enc_cfg_get_s32(cfg, "rc:bps_target", &bps_target);
    mpp_log("after  get: rc mode %d bps_target %d\n", rc_mode, bps_target);

    mpp_log("before set: rc aq_thrd_i: %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d\n",
            aq_thrd_i_ret[0], aq_thrd_i_ret[1], aq_thrd_i_ret[2], aq_thrd_i_ret[3],
            aq_thrd_i_ret[4], aq_thrd_i_ret[5], aq_thrd_i_ret[6], aq_thrd_i_ret[7],
            aq_thrd_i_ret[8], aq_thrd_i_ret[9], aq_thrd_i_ret[10], aq_thrd_i_ret[11],
            aq_thrd_i_ret[12], aq_thrd_i_ret[13], aq_thrd_i_ret[14], aq_thrd_i_ret[15]);

    ret = mpp_enc_cfg_set_st(cfg, "hw:aq_step_i", aq_thrd_i);
    ret = mpp_enc_cfg_get_st(cfg, "hw:aq_step_i", aq_thrd_i_ret);

    mpp_log("after  get: rc aq_thrd_i: %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d %2d\n",
            aq_thrd_i_ret[0], aq_thrd_i_ret[1], aq_thrd_i_ret[2], aq_thrd_i_ret[3],
            aq_thrd_i_ret[4], aq_thrd_i_ret[5], aq_thrd_i_ret[6], aq_thrd_i_ret[7],
            aq_thrd_i_ret[8], aq_thrd_i_ret[9], aq_thrd_i_ret[10], aq_thrd_i_ret[11],
            aq_thrd_i_ret[12], aq_thrd_i_ret[13], aq_thrd_i_ret[14], aq_thrd_i_ret[15]);

    ret = mpp_enc_cfg_deinit(cfg);
    if (ret) {
        mpp_err("mpp_enc_cfg_deinit failed\n");
        goto DONE;
    }

DONE:
    mpp_log("mpp_enc_cfg_test done %s\n", ret ? "failed" : "success");
    return ret;
}
