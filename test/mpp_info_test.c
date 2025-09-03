/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_info_test"

#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_info.h"
#include "mpp_compat.h"

int main()
{
    mpp_env_set_u32("mpp_show_history", 0);

    mpp_log("normal version log:\n");
    show_mpp_version();

    mpp_env_set_u32("mpp_show_history", 1);
    mpp_log("history version log:\n");

    show_mpp_version();
    mpp_env_set_u32("mpp_show_history", 0);

    mpp_compat_show();

    return 0;
}

