/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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
    RK_U32 flag_set = 0xffff;
    RK_U32 flag_get = 0;

    mpp_err("mpp log test start\n");

    mpp_log("mpp log flag_set: %08x\n", mpp_get_log_level());

    mpp_log("set flag_set to %08x\n", flag_set);

    mpp_set_log_level(flag_set);

    flag_get = mpp_get_log_level();

    mpp_log("mpp log flag_get: %08x\n", flag_get);

    mpp_assert(flag_set == flag_get);

    mpp_err("mpp log log test done\n");

    return 0;
}
