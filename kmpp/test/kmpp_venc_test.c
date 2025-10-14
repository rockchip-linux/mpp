/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_venc_test"

#include "mpp_debug.h"
#include "kmpp_frame.h"
#include "kmpp_packet.h"
#include "kmpp_venc.h"
#include "rk_venc_kcfg.h"

int main()
{
    KmppVenc venc = NULL;
    MppVencKcfg cfg = NULL;
    KmppFrame frm = NULL;
    KmppPacket pkt = NULL;
    rk_s32 ret = rk_ok;

    mpp_log(MODULE_TAG " start\n");

    ret = kmpp_venc_get(&venc);
    if (ret) {
        mpp_err(MODULE_TAG " get venc failed\n");
        return ret;
    }

    mpp_venc_kcfg_init(&cfg, MPP_VENC_KCFG_TYPE_INIT);
    mpp_venc_kcfg_set_u32(cfg, "type", MPP_CTX_ENC);
    mpp_venc_kcfg_set_u32(cfg, "coding", MPP_VIDEO_CodingAVC);
    mpp_venc_kcfg_set_s32(cfg, "chan_id", 0);
    mpp_venc_kcfg_set_u32(cfg, "buf_size", 4096);
    mpp_venc_kcfg_set_u32(cfg, "max_width", 1920);
    mpp_venc_kcfg_set_u32(cfg, "max_height", 1080);
    mpp_venc_kcfg_set_u32(cfg, "max_lt_cnt", 0);
    mpp_venc_kcfg_set_s32(cfg, "input_timeout", -1);

    ret = kmpp_venc_init(venc, cfg);
    if (ret) {
        mpp_err(MODULE_TAG " init venc failed\n");
        return ret;
    }

    mpp_venc_kcfg_deinit(cfg);
    cfg = NULL;

    mpp_venc_kcfg_init(&cfg, MPP_VENC_KCFG_TYPE_ST_CFG);

    ret = kmpp_venc_get_cfg(venc, cfg);
    if (ret) {
        mpp_err(MODULE_TAG " get venc cfg failed\n");
        return ret;
    }

    ret = kmpp_venc_set_cfg(venc, cfg);
    if (ret) {
        mpp_err(MODULE_TAG " set venc cfg failed\n");
        return ret;
    }

    mpp_venc_kcfg_deinit(cfg);
    cfg = NULL;

    ret = kmpp_venc_start(venc);
    if (ret) {
        mpp_err(MODULE_TAG " start venc failed\n");
        return ret;
    }

    kmpp_frame_get(&frm);

    ret = kmpp_venc_put_frm(venc, frm);
    if (ret) {
        mpp_err(MODULE_TAG " put frm failed\n");
        return ret;
    }

    kmpp_frame_put(frm);
    frm = NULL;

    ret = kmpp_venc_get_pkt(venc, &pkt);
    if (ret) {
        mpp_err(MODULE_TAG " get pkt failed\n");
        return ret;
    }

    if (pkt) {
        ret = kmpp_venc_put_pkt(venc, pkt);
        if (ret) {
            mpp_err(MODULE_TAG " put pkt failed\n");
            return ret;
        }
    }

    ret = kmpp_venc_stop(venc);
    if (ret) {
        mpp_err(MODULE_TAG " stop venc failed\n");
        return ret;
    }

    ret = kmpp_venc_deinit(venc);
    if (ret) {
        mpp_err(MODULE_TAG " deinit venc failed\n");
        return ret;
    }

    ret = kmpp_venc_put(venc);

    mpp_log(MODULE_TAG " %s\n", ret ? "failed" : "success");
    return ret;
}
