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

#define MODULE_TAG "mpi"

#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpi_impl.h"

RK_U32 mpi_debug = 0;

void get_mpi_debug()
{
    mpp_env_get_u32("mpi_debug", &mpi_debug, 0);
    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);
}

