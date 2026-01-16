/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2025 Rockchip Electronics Co., Ltd.
 */

#define MODULE_TAG "kmpp_vdec_test"

#include <stdio.h>

#include "mpp_mem.h"
#include "mpp_debug.h"

#include "kmpp_frame.h"
#include "kmpp_packet.h"
#include "kmpp_vdec.h"
#include "rk_vdec_kcfg.h"

int main(int argc, char **argv)
{
    KmppVdec vdec = NULL;
    char *path = argv[1];
    MppVdecKcfg cfg;
    rk_s32 ret = rk_ok;

    mpp_log(MODULE_TAG " start path %s\n", path);

    ret = kmpp_vdec_get(&vdec);
    if (ret) {
        mpp_err(MODULE_TAG " get vdec failed\n");
        return ret;
    }

    mpp_vdec_kcfg_init(&cfg, MPP_VDEC_KCFG_TYPE_INIT);
    mpp_vdec_kcfg_set_u32(cfg, "type", MPP_CTX_ENC);
    mpp_vdec_kcfg_set_u32(cfg, "coding", MPP_VIDEO_CodingAVC);

    ret = kmpp_vdec_init(vdec, cfg);
    if (ret) {
        mpp_err(MODULE_TAG " init vdec failed\n");
        return ret;
    }
    mpp_vdec_kcfg_deinit(cfg);
    cfg = NULL;

    ret = kmpp_vdec_start(vdec);
    if (ret) {
        mpp_err(MODULE_TAG " start vdec failed\n");
        return ret;
    }

    if (path) {
        FILE *fp = fopen(path, "rb");
        char *buf = NULL;
        rk_s32 size = 0;

        if (fp) {
            fseek(fp, 0L, SEEK_END);
            size = ftell(fp);
            fseek(fp, 0L, SEEK_SET);

            buf = mpp_calloc(char, size);

            if (buf) {
                KmppPacket packet = NULL;
                KmppFrame frame = NULL;
                KmppShmPtr sptr;
                rk_s32 rd;

                rd = fread(buf, 1, size, fp);

                kmpp_packet_get(&packet);

                sptr.uptr = buf;
                sptr.kaddr = 0;
                kmpp_packet_set_data(packet, &sptr);
                kmpp_packet_set_size(packet, size);
                kmpp_packet_set_pos(packet, &sptr);
                kmpp_packet_set_length(packet, size);

                kmpp_vdec_put_pkt(vdec, packet);

                kmpp_vdec_get_frm(vdec, &frame);

                kmpp_packet_put(packet);
                kmpp_frame_put(frame);
            }

            MPP_FREE(buf);
            fclose(fp);
        }
    }

    ret = kmpp_vdec_stop(vdec);
    if (ret) {
        mpp_err(MODULE_TAG " stop vdec failed\n");
        return ret;
    }

    ret = kmpp_vdec_deinit(vdec);
    if (ret) {
        mpp_err(MODULE_TAG " deinit vdec failed\n");
        return ret;
    }

    ret = kmpp_vdec_put(vdec);

    mpp_log(MODULE_TAG " %s\n", ret ? "failed" : "success");
    return ret;
}
