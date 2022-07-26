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

#define MODULE_TAG "mpp_meta_test"

#include <pthread.h>

#include "mpp_time.h"
#include "mpp_debug.h"
#include "mpp_meta_impl.h"

#define TEST_MAX    200
#define LOOP_MAX    100000

void *meta_test(void *param)
{
    RK_S32 loop_max = LOOP_MAX;
    RK_S64 time_start;
    RK_S64 time_end;
    MppMeta meta = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame;
    MppPacket packet;
    MppBuffer buffer;
    void *ptr;
    RK_S32 val;
    RK_S32 i;

    time_start = mpp_time();

    for (i = 0; i < loop_max; i++) {
        ret |= mpp_meta_get(&meta);
        mpp_assert(meta);

        /* set */
        ret |= mpp_meta_set_frame(meta,  KEY_INPUT_FRAME, NULL);
        ret |= mpp_meta_set_packet(meta, KEY_INPUT_PACKET, NULL);
        ret |= mpp_meta_set_frame(meta,  KEY_OUTPUT_FRAME, NULL);
        ret |= mpp_meta_set_packet(meta, KEY_OUTPUT_PACKET, NULL);

        ret |= mpp_meta_set_buffer(meta, KEY_MOTION_INFO, NULL);
        ret |= mpp_meta_set_buffer(meta, KEY_HDR_INFO, NULL);

        ret |= mpp_meta_set_s32(meta, KEY_INPUT_BLOCK, 0);
        ret |= mpp_meta_set_s32(meta, KEY_OUTPUT_BLOCK, 0);
        ret |= mpp_meta_set_s32(meta, KEY_INPUT_IDR_REQ, 0);
        ret |= mpp_meta_set_s32(meta, KEY_OUTPUT_INTRA, 0);

        ret |= mpp_meta_set_s32(meta, KEY_TEMPORAL_ID, 0);
        ret |= mpp_meta_set_s32(meta, KEY_LONG_REF_IDX, 0);
        ret |= mpp_meta_set_s32(meta, KEY_ENC_AVERAGE_QP, 0);

        ret |= mpp_meta_set_ptr(meta, KEY_ROI_DATA, NULL);
        ret |= mpp_meta_set_ptr(meta, KEY_OSD_DATA, NULL);
        ret |= mpp_meta_set_ptr(meta, KEY_OSD_DATA2, NULL);
        ret |= mpp_meta_set_ptr(meta, KEY_USER_DATA, NULL);
        ret |= mpp_meta_set_ptr(meta, KEY_USER_DATAS, NULL);

        ret |= mpp_meta_set_buffer(meta, KEY_QPMAP0, NULL);
        ret |= mpp_meta_set_ptr(meta, KEY_MV_LIST, NULL);

        ret |= mpp_meta_set_s32(meta, KEY_ENC_MARK_LTR, 0);
        ret |= mpp_meta_set_s32(meta, KEY_ENC_USE_LTR, 0);
        ret |= mpp_meta_set_s32(meta, KEY_ENC_FRAME_QP, 0);
        ret |= mpp_meta_set_s32(meta, KEY_ENC_BASE_LAYER_PID, 0);

        /* get */
        ret |= mpp_meta_get_frame(meta,  KEY_INPUT_FRAME, &frame);
        ret |= mpp_meta_get_packet(meta, KEY_INPUT_PACKET, &packet);
        ret |= mpp_meta_get_frame(meta,  KEY_OUTPUT_FRAME, &frame);
        ret |= mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &packet);

        ret |= mpp_meta_get_buffer(meta, KEY_MOTION_INFO, &buffer);
        ret |= mpp_meta_get_buffer(meta, KEY_HDR_INFO, &buffer);

        ret |= mpp_meta_get_s32(meta, KEY_INPUT_BLOCK, &val);
        ret |= mpp_meta_get_s32(meta, KEY_OUTPUT_BLOCK, &val);
        ret |= mpp_meta_get_s32(meta, KEY_INPUT_IDR_REQ, &val);
        ret |= mpp_meta_get_s32(meta, KEY_OUTPUT_INTRA, &val);

        ret |= mpp_meta_get_s32(meta, KEY_TEMPORAL_ID, &val);
        ret |= mpp_meta_get_s32(meta, KEY_LONG_REF_IDX, &val);
        ret |= mpp_meta_get_s32(meta, KEY_ENC_AVERAGE_QP, &val);

        ret |= mpp_meta_get_ptr(meta, KEY_ROI_DATA, &ptr);
        ret |= mpp_meta_get_ptr(meta, KEY_OSD_DATA, &ptr);
        ret |= mpp_meta_get_ptr(meta, KEY_OSD_DATA2, &ptr);
        ret |= mpp_meta_get_ptr(meta, KEY_USER_DATA, &ptr);
        ret |= mpp_meta_get_ptr(meta, KEY_USER_DATAS, &ptr);

        ret |= mpp_meta_get_buffer(meta, KEY_QPMAP0, &buffer);
        ret |= mpp_meta_get_ptr(meta, KEY_MV_LIST, &ptr);

        ret |= mpp_meta_get_s32(meta, KEY_ENC_MARK_LTR, &val);
        ret |= mpp_meta_get_s32(meta, KEY_ENC_USE_LTR, &val);
        ret |= mpp_meta_get_s32(meta, KEY_ENC_FRAME_QP, &val);
        ret |= mpp_meta_get_s32(meta, KEY_ENC_BASE_LAYER_PID, &val);

        ret |= mpp_meta_put(meta);
    }

    time_end = mpp_time();

    *((RK_S64 *)param) = (time_end - time_start) / loop_max;

    return NULL;
}

int main()
{
    pthread_t thds[TEST_MAX];
    RK_S64 times[TEST_MAX];
    RK_S32 thd_cnt = TEST_MAX;
    RK_S64 avg_time = 0;
    pthread_attr_t attr;
    RK_S32 i;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    mpp_log("mpp_meta_test start\n");

    for (i = 0; i < thd_cnt; i++)
        pthread_create(&thds[i], &attr, meta_test, &times[i]);

    for (i = 0; i < thd_cnt; i++)
        pthread_join(thds[i], NULL);

    for (i = 0; i < thd_cnt; i++)
        avg_time += times[i];

    mpp_log("mpp_meta_test %d threads %d loop config avg %lld us",
            thd_cnt, LOOP_MAX, avg_time / thd_cnt);

    mpp_log("mpp_meta_test done\n");

    return 0;
}
