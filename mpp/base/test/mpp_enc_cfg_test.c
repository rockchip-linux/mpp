/*
 * Copyright 2015 Rockchip Electronics Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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

    MppEncCfgImpl *impl = (MppEncCfgImpl *)cfg;

    mpp_log("before set: rc mode %d bps_target %d\n",
            impl->cfg.rc.rc_mode, impl->cfg.rc.bps_target);

    start = mpp_time();
    ret = mpp_enc_cfg_set_u32(cfg, "rc:mode", rc_mode);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:mode", rc_mode);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:bps", 400000);
    ret = mpp_enc_cfg_set_s32(cfg, "rc:bps_target", bps_target);
    end = mpp_time();
    mpp_log("set s32 time %lld us\n", end - start);

    mpp_log("after  set: rc mode %d bps_target %d\n",
            impl->cfg.rc.rc_mode, impl->cfg.rc.bps_target);

    rc_mode = 0;
    bps_target = 0;

    mpp_log("before get: rc mode %d bps_target %d\n", rc_mode, bps_target);

    ret = mpp_enc_cfg_get_s32(cfg, "rc:mode", &rc_mode);
    ret = mpp_enc_cfg_get_s32(cfg, "rc:bps_target", &bps_target);

    mpp_log("after  get: rc mode %d bps_target %d\n", rc_mode, bps_target);

    ret = mpp_enc_cfg_deinit(cfg);
    if (ret) {
        mpp_err("mpp_enc_cfg_deinit failed\n");
        goto DONE;
    }

DONE:
    mpp_log("mpp_enc_cfg_test done %s\n", ret ? "failed" : "success");
    return ret;
}
