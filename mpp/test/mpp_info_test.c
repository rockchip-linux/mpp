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
#include "mpp_info.h"

int main()
{
    mpp_log("mpp revision is %d\n", mpp_info_get_revision());
    mpp_log("mpp info all:\n%s\n", mpp_info_get(INFO_ALL));
    mpp_log("mpp info revision: %s\n", mpp_info_get(INFO_REVISION));
    mpp_log("mpp info date    : %s\n", mpp_info_get(INFO_DATE));
    mpp_log("mpp info author  : %s\n", mpp_info_get(INFO_AUTHOR));

    return 0;
}

