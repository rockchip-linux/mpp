/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_meta_test"

#include <pthread.h>

#include "mpp_time.h"
#include "mpp_debug.h"
#include "kmpp_meta_impl.h"

#define TEST_MAX    1
#define LOOP_MAX    10

void *meta_test(void *param)
{
    RK_S32 loop_max = LOOP_MAX;
    RK_S64 time_start;
    RK_S64 time_end;
    MppMeta meta[LOOP_MAX];
    MPP_RET ret = MPP_OK;
    KmppShmPtr frame;
    KmppShmPtr packet;
    KmppShmPtr buffer;
    KmppShmPtr sptr;
    void *ptr;
    RK_S32 val;
    RK_S32 i;

    time_start = mpp_time();

    frame.uaddr = 0;
    frame.kaddr = 0;
    packet.uaddr = 0;
    packet.kaddr = 0;
    buffer.uaddr = 0;
    buffer.kaddr = 0;
    sptr.uaddr = 0;
    sptr.kaddr = 0;

    for (i = 0; i < loop_max; i++) {
        ret |= kmpp_meta_get_f(&meta[i]);
        mpp_assert(meta[i]);
    }

    for (i = 0; i < loop_max; i++) {
        /* set */
        ret |= kmpp_meta_set_shm(meta[i], KEY_INPUT_FRAME, &frame);
        ret |= kmpp_meta_set_shm(meta[i], KEY_INPUT_PACKET, &packet);
        ret |= kmpp_meta_set_shm(meta[i], KEY_OUTPUT_FRAME, &frame);
        ret |= kmpp_meta_set_shm(meta[i], KEY_OUTPUT_PACKET, &packet);

        ret |= kmpp_meta_set_shm(meta[i], KEY_MOTION_INFO, &sptr);
        ret |= kmpp_meta_set_shm(meta[i], KEY_HDR_INFO, &sptr);

        ret |= kmpp_meta_set_s32(meta[i], KEY_INPUT_BLOCK, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_OUTPUT_BLOCK, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_INPUT_IDR_REQ, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_OUTPUT_INTRA, 0);

        ret |= kmpp_meta_set_s32(meta[i], KEY_TEMPORAL_ID, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_LONG_REF_IDX, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_ENC_AVERAGE_QP, 0);

        //ret |= kmpp_meta_set_shm(meta[i], KEY_ROI_DATA, NULL);
        ret |= kmpp_meta_set_shm(meta[i], KEY_OSD_DATA, NULL);
        ret |= kmpp_meta_set_shm(meta[i], KEY_OSD_DATA2, NULL);
        ret |= kmpp_meta_set_shm(meta[i], KEY_USER_DATA, NULL);
        ret |= kmpp_meta_set_shm(meta[i], KEY_USER_DATAS, NULL);

        ret |= kmpp_meta_set_shm(meta[i], KEY_QPMAP0, NULL);
        ret |= kmpp_meta_set_shm(meta[i], KEY_NPU_SOBJ_FLAG, NULL);
        ret |= kmpp_meta_set_ptr(meta[i], KEY_NPU_UOBJ_FLAG, NULL);

        ret |= kmpp_meta_set_s32(meta[i], KEY_ENC_MARK_LTR, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_ENC_USE_LTR, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_ENC_FRAME_QP, 0);
        ret |= kmpp_meta_set_s32(meta[i], KEY_ENC_BASE_LAYER_PID, 0);

        /* get */
        ret |= kmpp_meta_get_shm(meta[i], KEY_INPUT_FRAME, &frame);
        ret |= kmpp_meta_get_shm(meta[i], KEY_INPUT_PACKET, &packet);
        ret |= kmpp_meta_get_shm(meta[i], KEY_OUTPUT_FRAME, &frame);
        ret |= kmpp_meta_get_shm(meta[i], KEY_OUTPUT_PACKET, &packet);

        ret |= kmpp_meta_get_shm(meta[i], KEY_MOTION_INFO, &buffer);
        ret |= kmpp_meta_get_shm(meta[i], KEY_HDR_INFO, &buffer);

        ret |= kmpp_meta_get_s32(meta[i], KEY_INPUT_BLOCK, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_OUTPUT_BLOCK, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_INPUT_IDR_REQ, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_OUTPUT_INTRA, &val);

        ret |= kmpp_meta_get_s32(meta[i], KEY_TEMPORAL_ID, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_LONG_REF_IDX, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_ENC_AVERAGE_QP, &val);

        //ret |= kmpp_meta_get_shm(meta[i], KEY_ROI_DATA, &sptr);
        ret |= kmpp_meta_get_shm(meta[i], KEY_OSD_DATA, &sptr);
        ret |= kmpp_meta_get_shm(meta[i], KEY_OSD_DATA2, &sptr);
        ret |= kmpp_meta_get_shm(meta[i], KEY_USER_DATA, &sptr);
        ret |= kmpp_meta_get_shm(meta[i], KEY_USER_DATAS, &sptr);

        ret |= kmpp_meta_get_shm(meta[i], KEY_QPMAP0, &buffer);
        ret |= kmpp_meta_get_shm(meta[i], KEY_NPU_SOBJ_FLAG, &sptr);
        ret |= kmpp_meta_get_ptr(meta[i], KEY_NPU_UOBJ_FLAG, &ptr);

        ret |= kmpp_meta_get_s32(meta[i], KEY_ENC_MARK_LTR, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_ENC_USE_LTR, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_ENC_FRAME_QP, &val);
        ret |= kmpp_meta_get_s32(meta[i], KEY_ENC_BASE_LAYER_PID, &val);
    }

    for (i = 0; i < loop_max; i++) {
        ret |= kmpp_meta_put_f(meta[i]);
    }

    time_end = mpp_time();

    if (ret)
        mpp_log("meta setting and getting, ret %d\n", ret);

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

    mpp_log(MODULE_TAG " start\n");

    for (i = 0; i < thd_cnt; i++)
        pthread_create(&thds[i], &attr, meta_test, &times[i]);

    for (i = 0; i < thd_cnt; i++)
        pthread_join(thds[i], NULL);

    for (i = 0; i < thd_cnt; i++)
        avg_time += times[i];

    mpp_log(MODULE_TAG " %d threads %d loop config avg %lld us",
            thd_cnt, LOOP_MAX, avg_time / thd_cnt);

    mpp_log(MODULE_TAG " done\n");

    return 0;
}
