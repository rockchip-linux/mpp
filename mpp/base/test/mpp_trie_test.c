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

typedef struct TestCase_t {
    const char          *name;
    MPP_RET             ret;
} TestCase;

void *print_opt(void *ctx)
{
    RK_U8 **str = (RK_U8 **)ctx;

    if (str && *str)
        mpp_log("get option %s\n", *str);

    return NULL;
}

TestAction test_info[] = {
    { "rc:mode",        &test_info[0],  print_opt},
    { "rc:bps_target",  &test_info[1],  print_opt},
    { "rc:bps_max",     &test_info[2],  print_opt},
    { "rc:bps_min",     &test_info[3],  print_opt},
    /* test valid info end in the middle */
    { "rc:bps",         &test_info[4],  print_opt},
};

TestCase test_case[] = {
    { "rc:mode",                    MPP_OK, },
    { "rc:bps_target",              MPP_OK, },
    { "rc:bps_max",                 MPP_OK, },
    { "rc:bps",                     MPP_OK, },
    { "this is an error string",    MPP_NOK, },
    { "",                           MPP_NOK, },
};

int main()
{
    MppTrie trie = NULL;
    void *info = NULL;
    RK_S32 i;
    RK_S64 end = 0;
    RK_S64 start = 0;
    RK_S32 info_cnt = MPP_ARRAY_ELEMS(test_info);
    RK_S32 node_cnt = 100;
    RK_S32 ret = MPP_OK;

    mpp_log("mpp_trie_test start\n");

    mpp_trie_init(&trie, node_cnt, info_cnt);

    start = mpp_time();
    for (i = 0; i < info_cnt; i++)
        mpp_trie_add_info(trie, &test_info[i].name);
    end = mpp_time();
    mpp_log("add act time %lld us\n", end - start);

    ret = mpp_trie_shrink(trie, sizeof(TestAction));
    if (ret) {
        mpp_loge("mpp_trie_shrink failed\n");
        goto DONE;
    }

    for (i = 0; i < (RK_S32)MPP_ARRAY_ELEMS(test_case); i++) {
        start = mpp_time();
        info = mpp_trie_get_info(trie, test_case[i].name);
        end = mpp_time();

        if (info) {
            TestAction *act = (TestAction *)info;

            if (act && act->proc) {
                act->proc(act->ctx);
                mpp_log("search time %lld us\n", end - start);
            }
        } else {
            mpp_loge("search %s failed\n", test_case[i]);
        }

        ret |= ((info && !test_case[i].ret) ||
                (!info && test_case[i].ret)) ? MPP_OK : MPP_NOK;
    }

    mpp_trie_deinit(trie);

DONE:
    mpp_log("mpp_trie_test ret %s\n", ret ? "failed" : "success");

    return ret;
}
