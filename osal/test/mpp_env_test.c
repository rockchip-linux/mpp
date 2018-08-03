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

