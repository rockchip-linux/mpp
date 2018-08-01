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
static char mpp_version_revision[]  = MPP_VERSION;
static char mpp_version_commit[]    = MPP_AUTHOR;
static char mpp_version_date[]      = MPP_DATE;
static char mpp_version_one_line[]  = MPP_ONE_LINE;
static char mpp_version_number[]    = MPP_VER_NUM;

static RK_CHIP_TYPE chip_version(void)
{
    RK_CHIP_TYPE type = NONE;
    const char *value = NULL;
    RK_S32 ret = mpp_env_get_str("ro.product.board", &value, NULL);

    if (0 == ret) {
        if (strstr(value, "rk29")) {
            mpp_log("rk29 board found in board property");
            type = RK29;
        } else if (strstr(value, "rk30")) {
            mpp_log("rk30 board found in board property");
            type = RK30;
        }
    }
    if (NONE == type) {
        ret = mpp_env_get_str("ro.board.platform", &value, NULL);
        if (0 == ret) {
            if (strstr(value, "rk29")) {
                mpp_log("rk29 board found in platform property");
                type = RK29;
            } else if (strstr(value, "rk30")) {
                mpp_log("rk30 board found in platform property");
                type = RK30;
            }
        }
    }

    if (NONE == type) {
        mpp_log("can not found matched chip type");
    }
    return type;
}

const char *mpp_info_get(MPP_INFO_TYPE type)
{
    switch (type) {
    case INFO_ALL : {
        return mpp_version_one_line;
    } break;
    case INFO_REVISION : {
        return mpp_version_revision;
    } break;
    case INFO_DATE : {
        return mpp_version_date;
    } break;
    case INFO_AUTHOR : {
        return mpp_version_commit;
    } break;
    default : {
        mpp_err_f("invalid info type %d\n", type);
    } break;
    }
    return NULL;
}

RK_CHIP_TYPE get_chip_type()
{
    return chip_version();
}

int mpp_info_get_revision()
{
    return atoi(mpp_version_number);
}

