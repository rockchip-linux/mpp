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
    RK_U32 flag = 0xffff;

    mpp_err("mpp error log test start\n");

    mpp_log("mpp log flag: %08x\n", mpp_get_log_flag());

    mpp_log("set flag to %08x\n", flag);

    mpp_set_log_flag(flag);

    mpp_log("mpp log flag: %08x\n", mpp_get_log_flag());

    mpp_err("mpp error log test done\n");

    return 0;
}
