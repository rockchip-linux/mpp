/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "mpp_env_test"

#include "mpp_env.h"
#include "mpp_log.h"

const char env_debug[] = "test_env_debug";
const char env_string[] = "test_env_string";
char env_test_string[] = "just for debug";

int main()
{
    RK_U32 env_debug_u32 = 0x100;
    const char *env_str_out = NULL;

    mpp_env_set_u32(env_debug, env_debug_u32);
    mpp_env_set_str(env_string, env_test_string);
    mpp_log("set env: %s to %u\n", env_debug, env_debug_u32);
    mpp_log("set env: %s to %s\n", env_string, env_test_string);

    env_debug_u32 = 0;
    mpp_log("start reading env:\n");

    mpp_env_get_u32(env_debug, &env_debug_u32, 0);
    mpp_env_get_str(env_string, &env_str_out, NULL);

    mpp_log("get env: %s is %u\n", env_debug, env_debug_u32);
    mpp_log("get env: %s is %s\n", env_string, env_str_out);

    return 0;
}

