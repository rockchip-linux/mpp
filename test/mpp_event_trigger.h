/*
 * Copyright 2017 Rockchip Electronics Co. LTD
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
