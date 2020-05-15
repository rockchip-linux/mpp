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

#define MODULE_TAG "mpp_trie_test"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp_trie.h"

typedef void *(*TestProc)(void *);

typedef struct TestAction_t {
    const char          *name;
    void                *ctx;
    TestProc            proc;
} TestAction;

void *print_opt(void *ctx)
{
    RK_U8 **str = (RK_U8 **)ctx;

    if (str && *str)
        mpp_log("get option %s\n", *str);

    return NULL;
}

TestAction test_acts[] = {
    { "rc:rc_mode",     &test_acts[0],  print_opt},
    { "rc:bps_target",  &test_acts[1],  print_opt},
    { "rc:bps_max",     &test_acts[2],  print_opt},
    { "rc:bps_min",     &test_acts[3],  print_opt},
};

const char *test_str[] = {
    "rc:rc_mode",
    "rc:bps_target",
    "rc:bps_max",
};

int main()
{
    MppTrie ac = NULL;
    void *info = NULL;
    RK_U32 i;
    RK_S64 end = 0;
    RK_S64 start = 0;

    mpp_trie_init(&ac, 100);

    start = mpp_time();
    mpp_trie_add_info(ac, &test_acts[0].name);
    mpp_trie_add_info(ac, &test_acts[1].name);
    mpp_trie_add_info(ac, &test_acts[2].name);
    mpp_trie_add_info(ac, &test_acts[3].name);
    end = mpp_time();
    mpp_log("add act time %lld us\n", end - start);

    for (i = 0; i < MPP_ARRAY_ELEMS(test_str); i++) {
        start = mpp_time();
        info = mpp_trie_get_info(ac, test_str[i]);
        end = mpp_time();
        if (info) {
            TestAction *act = (TestAction *)info;

            if (act && act->proc) {
                act->proc(act->ctx);
                mpp_log("search time %lld us\n", end - start);
            }
        }
    }

    mpp_trie_deinit(ac);

    return 0;
}
