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

    mpp_err("mpp log test start\n");

    mpp_err_f("log_f test\n");

    mpp_log("log disabled\n");

    mpp_log_disable(1);

    mpp_log("nothing should be show here\n");

    mpp_log_enable(1);

    mpp_log("log enabled\n");

    mpp_err("mpp log test done\n");

    return 0;
}
