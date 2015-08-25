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

#define  MODULE_TAG "mpp"

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_time.h"

#include "mpp.h"
#include "mpp_frame_impl.h"
#include "mpp_buffer.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

static void *thread_dec(void *data)
{
    Mpp *mpp = (Mpp*)data;
    mpp_list *packets = mpp->packets;
    mpp_list *frames  = mpp->frames;
    MppPacketImpl packet;
    MppFrame frame;

    while (mpp->thread_running) {
        if (packets->list_size()) {
            /*
             * packet will be destroyed outside, here just copy the content
             */
            packets->del_at_head(&packet, sizeof(packet));

            /*
             * generate a new frame, copy the pointer to list
             */
            mpp_frame_init(&frame);
            frames->add_at_tail(&frame, sizeof(frame));
        }
    }

    return NULL;
}

static void *thread_enc(void *data)
{
    Mpp *mpp = (Mpp*)data;
    mpp_list *packets = mpp->packets;
    mpp_list *frames  = mpp->frames;
    MppFrameImpl frame;
    MppPacket packet;
    size_t size = SZ_1M;
    char *buf = mpp_malloc(char, size);

    while (mpp->thread_running) {
        if (frames->list_size()) {
            frames->del_at_head(&frame, sizeof(frame));

            mpp_packet_init(&packet, buf, size);
            packets->add_at_tail(&packet, sizeof(packet));
        }
    }
    mpp_free(buf);
    return NULL;
}

Mpp::Mpp(MppCtxType type)
    : packets(NULL),
      frames(NULL),
      thread_running(0),
      thread_reset(0),
      status(0)
{
    switch (type) {
    case MPP_CTX_DEC : {
        thread_start(thread_dec);
        packets = new mpp_list((node_destructor)NULL);
        frames  = new mpp_list((node_destructor)mpp_frame_deinit);
    } break;
    case MPP_CTX_ENC : {
        thread_start(thread_enc);
        frames  = new mpp_list((node_destructor)NULL);
        packets = new mpp_list((node_destructor)mpp_packet_deinit);
    } break;
    default : {
        mpp_err("Mpp error type %d\n", type);
    } break;
    }
    if (thread_running) {
    }
}

Mpp::~Mpp ()
{
    if (thread_running)
        thread_stop();
    if (packets)
        delete packets;
    if (frames)
        delete frames;
}

void Mpp::thread_start(MppThread func)
{
    if (!thread_running) {
    	pthread_attr_t attr;
    	pthread_attr_init(&attr);
    	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if (pthread_create(&thread, &attr, func, (void*)this))
            status = MPP_ERR_FATAL_THREAD;
        else
            thread_running = 1;
    	pthread_attr_destroy(&attr);
    }
}

void Mpp::thread_stop()
{
    thread_running = 0;
    void *dummy;
    pthread_join(thread, &dummy);
}

MPP_RET Mpp::put_packet(MppPacket packet)
{
    // TODO: packet data need preprocess or can be write to hardware buffer
    return (MPP_RET)packets->add_at_tail(packet, sizeof(MppPacketImpl));
}

MPP_RET Mpp::get_frame(MppFrame *frame)
{
    MPP_RET ret = MPP_NOK;
    if (frames->list_size()) {
        ret = (MPP_RET)frames->del_at_tail(frame, sizeof(frame));
    }
    return ret;
}

MPP_RET Mpp::put_frame(MppFrame frame)
{
    MPP_RET ret = (MPP_RET)frames->add_at_tail(frame, sizeof(MppFrameImpl));
    return ret;
}

MPP_RET Mpp::get_packet(MppPacket *packet)
{
    MPP_RET ret = MPP_NOK;
    if (packets->list_size()) {
        ret = (MPP_RET)packets->del_at_tail(packet, sizeof(packet));
    }
    return ret;
}

