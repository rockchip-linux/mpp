/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_PARSE_CFG_H__
#define __MPP_PARSE_CFG_H__

struct rc_event {
    union {
        int msec;
        int frm;
        int idx;
    };
    float fps;
    int bps;
};

struct rc_test_config {
    int idx_type;
    int loop;
    struct rc_event event[128];
    int event_cnt;
};

int mpp_parse_config(char *cfg_url, struct rc_test_config *ea);

#endif
