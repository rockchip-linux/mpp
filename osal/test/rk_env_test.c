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

#define LOG_TAG     "rk_env_test"
#include "rk_env.h"
#include "rk_log.h"

const char env_debug[] = "test_env_debug";
const char env_string[] = "test_env_string";
char env_test_string[] = "just for debug";

int main()
{
    RK_U32 env_debug_u32 = 0x100;
    char *env_string_str = env_test_string;

    rk_set_env_u32(env_debug, env_debug_u32);
    rk_set_env_str(env_string, env_string_str);
    rk_log("set env: %s to %u\n", env_debug, env_debug_u32);
    rk_log("set env: %s to %s\n", env_string, env_string_str);

    env_debug_u32 = 0;
    env_string_str = NULL;
    rk_log("clear local value to zero\n");

    rk_get_env_u32(env_debug, &env_debug_u32, 0);
    rk_get_env_str(env_string, &env_string_str, NULL);

    rk_log("get env: %s is %u\n", env_debug, env_debug_u32);
    rk_log("get env: %s is %s\n", env_string, env_string_str);

    return 0;
}

