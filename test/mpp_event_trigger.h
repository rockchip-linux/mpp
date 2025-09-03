/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2017 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_EVENT_TRIGGER_H__
#define __MPP_EVENT_TRIGGER_H__

typedef void (*event_trigger)(void *parent, void *event);

struct ievent {
    int idx;
    void *event;
};

struct event_packet {
    int cnt;
    int loop;
    struct ievent e[128];
};

struct event_ctx {
    int (*notify)(void *param);
};

struct event_ctx* event_ctx_create(struct event_packet *e,
                                   event_trigger trigger, void *parent);
void event_ctx_release(struct event_ctx *ictx);

#endif
