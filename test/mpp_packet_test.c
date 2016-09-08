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

#define MODULE_TAG "mpp_packet_test"

#include <stdlib.h>

#include "mpp_log.h"
#include "mpp_packet.h"

#define MPP_PACKET_TEST_SIZE    1024

int main()
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppPacket packet = NULL;
    void *data = NULL;
    size_t size = MPP_PACKET_TEST_SIZE;

    mpp_log("mpp_packet_test start\n");

    data = malloc(size);
    if (NULL == data) {
        mpp_err("mpp_packet_test malloc failed\n");
        goto MPP_PACKET_failed;
    }

    ret = mpp_packet_init(&packet, data, size);
    if (MPP_OK != ret) {
        mpp_err("mpp_packet_test mpp_packet_init failed\n");
        goto MPP_PACKET_failed;
    }
    mpp_packet_set_eos(packet);
    if (MPP_OK != ret) {
        mpp_err("mpp_packet_test mpp_packet_set_eos failed\n");
        goto MPP_PACKET_failed;
    }
    mpp_packet_deinit(&packet);

    free(data);
    mpp_log("mpp_packet_test success\n");
    return ret;

MPP_PACKET_failed:
    if (packet)
        mpp_packet_deinit(&packet);

    if (data)
        free(data);

    mpp_log("mpp_packet_test failed\n");
    return ret;
}

