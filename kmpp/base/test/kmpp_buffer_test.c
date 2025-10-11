/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_buffer_test"

#include "mpp_debug.h"

#include "kmpp_obj.h"
#include "kmpp_buffer.h"

int main()
{
    KmppShmPtr    sptr;
    KmppBufGrp    grp = NULL;
    KmppBufGrpCfg grp_cfg = NULL;
    KmppBuffer    buf = NULL;
    KmppBufCfg    buf_cfg = NULL;
    rk_s32        used = 0;
    rk_s32        unused = 0;
    rk_u32        size = 1920 * 1080 * 3 / 2;
    rk_s32        ret = rk_ok;

    mpp_logi(MODULE_TAG " start\n");

    /* KmppBufGrp object ready */
    ret = kmpp_buf_grp_get(&grp);
    mpp_logi("object %s ready\n", kmpp_obj_get_name(grp));

    /* get KmppBufGrpCfg from KmppBufGrp to config */
    grp_cfg = kmpp_buf_grp_to_cfg(grp);
    mpp_logi("object %s ready\n", kmpp_obj_get_name(grp_cfg));

    /* write parameters to KmppBufGrpCfg */
    ret |= kmpp_buf_grp_cfg_set_flag(grp_cfg, 0);
    ret |= kmpp_buf_grp_cfg_set_count(grp_cfg, 10);
    ret |= kmpp_buf_grp_cfg_set_size(grp_cfg, size);
    ret |= kmpp_buf_grp_cfg_set_fd(grp_cfg, -1);

    sptr.uptr = "rk dma heap";
    sptr.kaddr = 0;
    ret |= kmpp_buf_grp_cfg_set_allocator(grp_cfg, &sptr);

    sptr.uptr = "test";
    sptr.kaddr = 0;
    ret |= kmpp_buf_grp_cfg_set_name(grp_cfg, &sptr);
    if (ret)
        mpp_loge("set buf grp cfg failed\n");

    /* enable KmppBufGrpCfg by ioctl */
    ret = kmpp_buf_grp_setup(grp);
    if (ret)
        mpp_loge("setup buf grp cfg failed\n");

    /* get KmppBuffer for buffer allocation */
    ret = kmpp_buffer_get(&buf);
    mpp_logi("object %s ready\n", kmpp_obj_get_name(buf));

    /* get KmppBufCfg to setup */
    buf_cfg = kmpp_buffer_to_cfg(buf);
    mpp_logi("object %s ready\n", kmpp_obj_get_name(buf_cfg));

    /* setup buffer config parameters */
    ret = kmpp_buf_cfg_set_group(buf_cfg, kmpp_obj_to_shm(grp));
    if (ret)
        mpp_loge("set kmpp_buf_cfg_set_group failed\n");

    /* set buffer size */
    kmpp_buf_cfg_set_size(buf_cfg, size);

    /* enable KmppBufCfg by ioctl */
    kmpp_buffer_setup(buf);
    if (ret)
        mpp_loge("setup buf grp cfg failed\n");

    /* get buffer group from buf_cfg */
    ret = kmpp_buf_cfg_get_group(buf_cfg, &sptr);
    if (ret)
        mpp_loge("get buf cfg grp failed");

    /* get buffer share pointer and access */
    ret = kmpp_buf_cfg_get_sptr(buf_cfg, &sptr);
    if (ret || !sptr.uptr)
        mpp_loge("get buf cfg sptr failed");

    ret = kmpp_buf_grp_cfg_get_used(grp_cfg, &used);
    ret |= kmpp_buf_grp_cfg_get_unused(grp_cfg, &unused);
    if (ret)
        mpp_loge("get buf grp cfg used failed");

    mpp_logi("buf grp cfg used %d unused %d after buffer_setup\n", used, unused);

    mpp_assert(used == 1);

    if (sptr.uptr) {
        rk_u32 *val = sptr.uptr;

        memset(sptr.uptr, 0xff, size);

        *val = 0x12345678;
        if (*val != 0x12345678)
            mpp_loge("buf cfg sptr access failed");
        else
            mpp_logi("buf sptr [u:k] %p:%llx access success", sptr.uptr, sptr.kaddr);
    }

    if (buf) {
        kmpp_buffer_put(buf);
        buf = NULL;
    }

    ret = kmpp_buf_grp_cfg_get_used(grp_cfg, &used);
    ret |= kmpp_buf_grp_cfg_get_unused(grp_cfg, &unused);
    if (ret)
        mpp_loge("get buf grp cfg used failed");

    mpp_logi("buf grp cfg used %d unused %d after buffer_put\n", used, unused);

    if (grp) {
        kmpp_buf_grp_put(grp);
        grp = NULL;
    }

    mpp_logi(MODULE_TAG " %s\n", ret ? "failed" : "success");

    return ret;
}
