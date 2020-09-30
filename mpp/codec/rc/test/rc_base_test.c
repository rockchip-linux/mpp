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

#define MODULE_TAG "mpp_rc_test"

#include <stdlib.h>

#include "mpp_log.h"
#include "rc_base.h"

int main()
{
    MPP_RET ret = MPP_OK;
    MppDataV2 *data_2 = NULL;
    MppDataV2 *data_5 = NULL;
    MppDataV2 *data_8 = NULL;
    MppDataV2 *data_30 = NULL;
    RK_S32 val = 0;
    RK_S32 i;

    mpp_log("mpp rc test start\n");

    mpp_data_init_v2(&data_2, 2, 0);
    mpp_data_init_v2(&data_5, 5, 0);
    mpp_data_init_v2(&data_8, 8, 0);
    mpp_data_init_v2(&data_30, 30, 0);

    for (i = 0; i < 50; i++) {
        val = i;

        mpp_data_update_v2(data_2, val);
        mpp_data_update_v2(data_5, val);
        mpp_data_update_v2(data_8, val);
        mpp_data_update_v2(data_30, val);

        mpp_log("sum %4d %4d %4d %4d mean %2d %2d %2d %2d ratio sum %2d %2d %2d %2d\n",
                mpp_data_sum_v2(data_2), mpp_data_sum_v2(data_5),
                mpp_data_sum_v2(data_8), mpp_data_sum_v2(data_30),
                mpp_data_mean_v2(data_2), mpp_data_mean_v2(data_5),
                mpp_data_mean_v2(data_8), mpp_data_mean_v2(data_30),
                mpp_data_sum_with_ratio_v2(data_2, data_2->size, 3, 4),
                mpp_data_sum_with_ratio_v2(data_5, data_5->size, 3, 4),
                mpp_data_sum_with_ratio_v2(data_8, data_8->size, 3, 4),
                mpp_data_sum_with_ratio_v2(data_30, data_30->size, 3, 4));
    }

    mpp_data_deinit_v2(data_5);
    mpp_data_deinit_v2(data_8);
    mpp_data_deinit_v2(data_30);
    mpp_data_deinit_v2(data_2);

    mpp_log("mpp rc test success\n");

    return ret;
}

