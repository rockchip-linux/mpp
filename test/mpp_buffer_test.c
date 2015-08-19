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

#define MODULE_TAG "mpp_buffer_test"

#include <string.h>

#include "mpp_log.h"
#include "mpp_buffer.h"

#define MPP_BUFFER_TEST_SIZE    1024
#define MPP_BUFFER_TEST_COUNT   10

int main()
{
    MPP_RET ret = MPP_OK;
    MppBufferCommit commit;
    MppBufferGroup group = NULL;
    MppBuffer buffer[MPP_BUFFER_TEST_COUNT];
    void *ptr[MPP_BUFFER_TEST_COUNT];
    size_t size = MPP_BUFFER_TEST_SIZE;
    RK_S32 i;

    mpp_log("mpp_buffer_test start\n");

    memset(ptr,    0, sizeof(ptr));
    memset(buffer, 0, sizeof(buffer));

    ret = mpp_buffer_group_get(&group, MPP_BUFFER_TYPE_NORMAL);
    if (MPP_OK != ret) {
        mpp_err("mpp_buffer_test mpp_buffer_group_get failed\n");
        goto MPP_BUFFER_failed;
    }

    commit.type = MPP_BUFFER_TYPE_NORMAL;
    commit.size = size;

    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        ptr[i] = malloc(size);
        if (NULL == ptr[i]) {
            mpp_err("mpp_buffer_test malloc failed\n");
            goto MPP_BUFFER_failed;
        }

        commit.data.ptr = ptr[i];

        ret = mpp_buffer_commit(group, &commit);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_commit failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        ret = mpp_buffer_get(group, &buffer[i], size);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_get failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        ret = mpp_buffer_put(&buffer[i]);
        if (MPP_OK != ret) {
            mpp_err("mpp_buffer_test mpp_buffer_put failed\n");
            goto MPP_BUFFER_failed;
        }
    }

    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        if (ptr[i]) {
            free(ptr[i]);
            ptr[i] = NULL;
        }
    }

    if (group)
        mpp_buffer_group_put(&group);

    mpp_log("mpp_buffer_test success\n");
    return ret;

MPP_BUFFER_failed:
    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        mpp_buffer_put(&buffer[i]);
    }

    for (i = 0; i < MPP_BUFFER_TEST_COUNT; i++) {
        if (ptr[i]) {
            free(ptr[i]);
            ptr[i] = NULL;
        }
    }

    if (group)
        mpp_buffer_group_put(group);

    mpp_log("mpp_buffer_test failed\n");
    return ret;
}

