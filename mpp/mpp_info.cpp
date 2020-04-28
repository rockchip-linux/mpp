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

#define MODULE_TAG "mpp_info"

#include <stdlib.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_info.h"

#include "version.h"

/*
 * To avoid string | grep author getting multiple results
 * use commit to replace author
 */
static const char mpp_version[] = MPP_VERSION;
static const RK_S32 mpp_history_cnt = MPP_VER_HIST_CNT;
static const char *mpp_history[]  = {
    MPP_VER_HIST_0,
    MPP_VER_HIST_1,
    MPP_VER_HIST_2,
    MPP_VER_HIST_3,
    MPP_VER_HIST_4,
    MPP_VER_HIST_5,
    MPP_VER_HIST_6,
    MPP_VER_HIST_7,
    MPP_VER_HIST_8,
    MPP_VER_HIST_9,
};

void show_mpp_version(void)
{
    RK_U32 show_history = 0;

    mpp_env_get_u32("mpp_show_history", &show_history, 0);

    if (show_history) {
        RK_S32 i;

        mpp_log("mpp version history %d:\n", mpp_history_cnt);
        for (i = 0; i < mpp_history_cnt; i++)
            mpp_log("%s\n", mpp_history[i]);
    } else
        mpp_log("mpp version: %s\n", mpp_version);
}

const char *get_mpp_version(void)
{
    return mpp_version;
}
