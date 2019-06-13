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

#define MODULE_TAG "mpp_buffer_test"

#include <string.h>

#if defined(_WIN32)
#include "vld.h"
#endif
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_common.h"
#include "mpp_buffer.h"
#include "mpp_allocator.h"

#define MPP_BUFFER_TEST_DEBUG_FLAG      (0xf)
#define MPP_BUFFER_TEST_SIZE            (SZ_1K*4)
#define MPP_BUFFER_TEST_COMMIT_COUNT    10
#define MPP_BUFFER_TEST_NORMAL_COUNT    10

int main()
{
    MPP_RET ret = MPP_OK;
    MppAllocator allocator = NULL;
    MppAllocatorApi *api = NULL;
    MppBufferInfo commit;
    MppBufferGroup group = NULL;
    MppBuffer commit_buffer[MPP_BUFFER_TEST_COMMIT_COUNT];
    void *commit_ptr[MPP_BUFFER_TEST_COMMIT_COUNT];
    MppBuffer normal_buffer[MPP_BUFFER_TEST_NORMAL_COUNT];
    MppBuffer legacy_buffer = NULL;
    size_t size = MPP_BUFFER_TEST_SIZE;
    RK_S32 count = MPP_BUFFER_TEST_COMMIT_COUNT;
    RK_S32 i;
    RK_U32 debug = 0;

    mpp_env_set_u32("mpp_buffer_debug", MPP_BUFFER_TEST_DEBUG_FLAG);
    mpp_env_get_u32("mpp_buffer_debug", &debug, 0);

    mpp_log("mpp_buffer_test start with debug 0x%x\n", debug);

    memset(commit_ptr,    0, sizeof(commit_ptr));
    memset(commit_buffer, 0, sizeof(commit_buffer));
    memset(normal_buffer, 0, sizeof(normal_buffer));

    // create group with external type
    ret = mpp_buffer_group_get_external(&group, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        mpp_err("mpp_buffer_test mpp_buffer_group_get failed\n");
        goto MPP_BUFFER_failed;
    }

    mpp_log("mpp_buffer_test commit mode with unused status start\n");

    commit.type = MPP_BUFFER_TYPE_ION;
    commit.size = size;

    // create misc ion buffer and commit to external group
    for (i = 0; i < count; i++) {
        // can be change to different buffer allocator here
        ret = mpp_buffer_get(NULL, &normal_buffer[i], size);
        if (ret || NULL == normal_buffer[i]) {
            mpp_err("mpp_buffer_test get misc buffer failed ret %d\n", ret);
            goto MPP_BUFFER_failed;
        }

        // NOTE: setup fd / index in necessary
        commit.ptr = mpp_buffer_get_ptr(normal_buffer[i]);
        commit.fd = mpp_buffer_get_fd(normal_buffer[i]);
        commit.index = i;

        ret = mpp_buffer_commit(group, &commit);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_commit failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < count; i++) {
        ret = mpp_buffer_get(group, &commit_buffer[i], size);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_get commit mode failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < count; i++) {
        if (commit_buffer[i]) {
            ret = mpp_buffer_put(commit_buffer[i]);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_test mpp_buffer_put commit mode failed\n");
                goto MPP_BUFFER_failed;
            }
            commit_buffer[i] = NULL;
        }
    }

    for (i = 0; i < count; i++) {
        if (normal_buffer[i]) {
            mpp_buffer_put(normal_buffer[i]);
            normal_buffer[i] = NULL;
        }
    }

    mpp_buffer_group_put(group);

    mpp_log("mpp_buffer_test commit mode with unused status success\n");


    mpp_log("mpp_buffer_test commit mode with used status start\n");

    ret = mpp_allocator_get(&allocator, &api, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        mpp_err("mpp_buffer_test mpp_allocator_get ion failed\n");
        goto MPP_BUFFER_failed;
    }

    commit.type = MPP_BUFFER_TYPE_ION;
    commit.size = size;

    for (i = 0; i < count; i++) {
        ret = api->alloc(allocator, &commit);
        if (ret) {
            mpp_err("mpp_buffer_test mpp_allocator_alloc failed\n");
            goto MPP_BUFFER_failed;
        }

        mpp_log("allocator get ptr %p with fd %d\n", commit.ptr, commit.fd);

        /*
         * NOTE: commit buffer info will be directly return within new MppBuffer
         *       This mode allow input group is NULL
         */
        ret = mpp_buffer_import(&commit_buffer[i], &commit);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_commit failed\n");
            goto MPP_BUFFER_failed;
        }

        /*
         * test imported buffer
         */
        {
            void *ptr = mpp_buffer_get_ptr(commit_buffer[i]);
            if (NULL == ptr) {
                mpp_err("mpp_buffer_test mpp_buffer_get_ptr failed\n");
                goto MPP_BUFFER_failed;
            }

            mpp_log("get ptr %p from fd %d\n", ptr, mpp_buffer_get_fd(commit_buffer[i]));

            memset(ptr, 0, mpp_buffer_get_size(commit_buffer[i]));
        }
    }

    for (i = 0; i < count; i++) {
        if (commit_buffer[i]) {
            ret = mpp_buffer_info_get(commit_buffer[i], &commit);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_test mpp_buffer_info_get failed\n");
                goto MPP_BUFFER_failed;
            }

            ret = mpp_buffer_put(commit_buffer[i]);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_test mpp_buffer_put commit mode failed\n");
                goto MPP_BUFFER_failed;
            }

            commit_buffer[i] = NULL;

            /* NOTE: buffer info from allocator need to be free directly */
            ret = api->free(allocator, &commit);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_test api->free failed\n");
                goto MPP_BUFFER_failed;
            }
        }
    }

    ret = mpp_allocator_put(&allocator);
    if (MPP_OK != ret) {
        mpp_err("mpp_buffer_test mpp_allocator_put failed\n");
        goto MPP_BUFFER_failed;
    }

    mpp_log("mpp_buffer_test commit mode with used status success\n");


    mpp_log("mpp_buffer_test normal mode start\n");

    ret = mpp_buffer_group_get_internal(&group, MPP_BUFFER_TYPE_ION);
    if (MPP_OK != ret) {
        mpp_err("mpp_buffer_test mpp_buffer_group_get failed\n");
        goto MPP_BUFFER_failed;
    }

    count = MPP_BUFFER_TEST_NORMAL_COUNT;

    mpp_buffer_group_limit_config(group, 0, count);

    for (i = 0; i < count; i++) {
        ret = mpp_buffer_get(group, &normal_buffer[i], (i + 1) * SZ_1K);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_get mode normal failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < count; i++) {
        if (normal_buffer[i]) {
            ret = mpp_buffer_put(normal_buffer[i]);
            if (MPP_OK != ret) {
                mpp_err("mpp_buffer_test mpp_buffer_get mode normal failed\n");
                goto MPP_BUFFER_failed;
            }
            normal_buffer[i] = NULL;
        }
    }

    mpp_log("mpp_buffer_test normal mode success\n");

    if (group) {
        mpp_buffer_group_put(group);
        group = NULL;
    }

    mpp_log("mpp_buffer_test success\n");

    ret = mpp_buffer_get(NULL, &legacy_buffer, MPP_BUFFER_TEST_SIZE);
    if (MPP_OK != ret) {
        mpp_log("mpp_buffer_test mpp_buffer_get legacy buffer failed\n");
        goto MPP_BUFFER_failed;
    }

    ret = mpp_buffer_put(legacy_buffer);
    if (MPP_OK != ret) {
        mpp_log("mpp_buffer_test mpp_buffer_put legacy buffer failed\n");
        goto MPP_BUFFER_failed;
    }

    mpp_env_set_u32("mpp_buffer_debug", 0);

    return ret;

MPP_BUFFER_failed:
    for (i = 0; i < MPP_BUFFER_TEST_COMMIT_COUNT; i++) {
        if (commit_buffer[i])
            mpp_buffer_put(commit_buffer[i]);
    }

    for (i = 0; i < MPP_BUFFER_TEST_COMMIT_COUNT; i++) {
        if (commit_ptr[i]) {
            free(commit_ptr[i]);
            commit_ptr[i] = NULL;
        }
    }
    for (i = 0; i < MPP_BUFFER_TEST_NORMAL_COUNT; i++) {
        if (normal_buffer[i])
            mpp_buffer_put(normal_buffer[i]);
    }

    if (group) {
        mpp_buffer_group_put(group);
        group = NULL;
    }

    if (legacy_buffer) {
        mpp_buffer_put(legacy_buffer);
        legacy_buffer = NULL;
    }

    if (allocator) {
        mpp_allocator_put(&allocator);
    }
    mpp_assert(NULL == allocator);

    mpp_log("mpp_buffer_test failed\n");

    mpp_env_set_u32("mpp_buffer_debug", 0);

    return ret;
}

