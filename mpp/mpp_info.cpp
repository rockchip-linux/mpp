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

#define MODULE_TAG "mpp_info"

#include <stdlib.h>
#include <string.h>

#include "rk_log.h"
#include "rk_env.h"
#include "mpp_info.h"

#include "svn_info.h"

mpp_info *mpp_info::singleton = NULL;

/* 
 * To avoid string | grep author getting multiple results
 * use commit to replace author
 */
static char mpp_version_revision[]  = SVN_VERSION;
static char mpp_version_commit[]    = SVN_AUTHOR;
static char mpp_version_date[]      = SVN_DATE;
static char mpp_version_one_line[]  = SVN_ONE_LINE;

mpp_info *mpp_info::getInstance()
{
    if (singleton == NULL) {
        singleton = new mpp_info();
    }
    return singleton;
}

static RK_CHIP_TYPE chip_version(void)
{
    RK_CHIP_TYPE type = NONE;
    char *value = NULL;
    RK_S32 ret = rk_get_env_str("ro.product.board", &value, NULL);

    if (0 == ret) {
        if (strstr(value, "rk29")) {
            rk_log("rk29 board found in board property");
            type = RK29;
        } else if (strstr(value, "rk30")) {
            rk_log("rk30 board found in board property");
            type = RK30;
        }
    }
    if (NONE == type) {
        ret = rk_get_env_str("ro.board.platform", &value, NULL);
        if (0 == ret) {
            if (strstr(value, "rk29")) {
                rk_log("rk29 board found in platform property");
                type = RK29;
            } else if (strstr(value, "rk30")) {
                rk_log("rk30 board found in platform property");
                type = RK30;
            }
        }
    }

    if (NONE == type) {
        rk_log("can not found matched chip type");
    }
    return type;
}

mpp_info::mpp_info()
{
    chip_type  = chip_version();

    mpp_info_revision   = mpp_version_revision;
    mpp_info_commit     = mpp_version_commit;
    mpp_info_date       = mpp_version_date;
    mpp_revision        = atoi(SVN_VERSION);

    show_mpp_info();
}

void mpp_info::show_mpp_info()
{
    rk_log("%s\n", mpp_version_one_line);
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
RK_CHIP_TYPE get_chip_type()
{
    return chip_version();
}
#ifdef __cplusplus
}
#endif


