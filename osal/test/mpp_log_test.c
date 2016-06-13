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

#define MODULE_TAG "mpp_log_test"

#include "mpp_log.h"

int main()
{
    RK_U32 flag_dbg = 0x02;
    RK_U32 flag_set = 0xffff;
    RK_U32 flag_get = 0;

    mpp_err("mpp log test start\n");

    mpp_err_f("log_f test\n");

    mpp_log("mpp log flag_set: %08x\n", mpp_log_get_flag());

    mpp_log("set flag_set to %08x\n", flag_set);

    mpp_log_set_flag(flag_set);

    flag_get = mpp_log_get_flag();

    mpp_log("mpp log flag_get: %08x\n", flag_get);

    mpp_assert(flag_set == flag_get);

    mpp_log("try _mpp_dbg test 0 debug %x, flag %x", flag_get, flag_dbg);
    _mpp_dbg(flag_get, flag_dbg, "mpp_dbg printing debug %x, flag %x", flag_get, flag_dbg);

    flag_dbg = 0;

    mpp_log("try _mpp_dbg test 0 debug %x, flag %x", flag_get, flag_dbg);
    _mpp_dbg(flag_get, flag_dbg, "mpp_dbg printing debug %x, flag %x", flag_get, flag_dbg);

    mpp_err("mpp log log test done\n");

    return 0;
}
