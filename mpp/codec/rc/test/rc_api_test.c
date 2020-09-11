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

#define MODULE_TAG "mpp_api_test"

#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_rc_api.h"

#define MAX_QUERY_COUNT     16

const RcImplApi test_h264_api = {
    "test_h264e_rc",
    MPP_VIDEO_CodingAVC,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

const RcImplApi test_h265_api = {
    "test_h265e_rc",
    MPP_VIDEO_CodingHEVC,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
};

int main()
{
    RcApiQueryAll query;
    RcApiBrief briefs[MAX_QUERY_COUNT];
    RK_S32 i;

    mpp_log("rc api test start\n");

    query.brief = briefs;
    query.max_count = MAX_QUERY_COUNT;
    query.count = 0;

    rc_brief_get_all(&query);

    mpp_log("default rc api query result:\n");
    for (i = 0; i < query.count; i++)
        mpp_log("rc api %s type %x\n", briefs[i].name, briefs[i].type);

    mpp_log("add test rc api\n");

    rc_api_add(&test_h264_api);
    rc_api_add(&test_h265_api);

    rc_brief_get_all(&query);

    mpp_log("rc api query result after adding\n");
    for (i = 0; i < query.count; i++)
        mpp_log("rc api %s type %x\n", briefs[i].name, briefs[i].type);

    mpp_log("mpp rc api test done\n");

    return 0;
}

