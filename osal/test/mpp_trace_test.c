/*
 * Copyright 2021 Rockchip Electronics Co. LTD
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

#define MODULE_TAG "mpp_trace_test"

#include "mpp_log.h"

#include "mpp_trace.h"

int main(void)
{
    mpp_log("mpp trace test start\n");

    mpp_trace_begin("mpp_trace_test");
    mpp_trace_end("mpp_trace_test");

    mpp_trace_async_begin("mpp_trace_test async", 10);
    mpp_trace_async_end("mpp_trace_test async", 10);

    mpp_trace_int32("mpp_trace_test int32", 256);
    mpp_trace_int64("mpp_trace_test int64", 100000000);

    mpp_log("mpp trace test done\n");

    return 0;
}
