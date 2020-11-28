/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_dec_cfg_test"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "rk_vdec_cfg.h"
#include "mpp_dec_cfg_impl.h"

int main()
{
    MPP_RET ret = MPP_OK;
    MppDecCfg cfg;
    RK_S64 end = 0;
    RK_S64 start = 0;

    mpp_dec_cfg_show();

    mpp_log("mpp_dec_cfg_test start\n");

    ret = mpp_dec_cfg_init(&cfg);
    if (ret) {
        mpp_err("mpp_dec_cfg_init failed\n");
        goto DONE;
    }

    RK_S32 fast_out = 1;

    MppDecCfgImpl *impl = (MppDecCfgImpl *)cfg;

    mpp_log("before set: fast_out %d\n", impl->cfg.base.fast_out);

    start = mpp_time();
    ret = mpp_dec_cfg_set_u32(cfg, "base:fast_out", fast_out);
    end = mpp_time();
    mpp_log("set u32 time %lld us\n", end - start);

    mpp_log("after  get: fast_out %d\n", fast_out);

    ret = mpp_dec_cfg_deinit(cfg);
    if (ret) {
        mpp_err("mpp_dec_cfg_deinit failed\n");
        goto DONE;
    }

DONE:
    mpp_log("mpp_dec_cfg_test done %s\n", ret ? "failed" : "success");
    return ret;
}
