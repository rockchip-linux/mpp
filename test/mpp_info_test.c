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

#define MODULE_TAG "mpp_info_test"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_info.h"

int main()
{
    mpp_env_set_u32("mpp_show_history", 0);

    mpp_log("normal version log:\n");
    show_mpp_version();

    mpp_env_set_u32("mpp_show_history", 1);
    mpp_log("history version log:\n");

    show_mpp_version();
    mpp_env_set_u32("mpp_show_history", 0);

    return 0;
}

