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

static void *thread_hal(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *thd_dec  = mpp->thd_codec;
    MppThread *thd_hal  = mpp->thd_hal;
    mpp_list *packets   = mpp->packets;
    mpp_list *frames    = mpp->frames;

    while (MPP_THREAD_RUNNING == thd_hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        // get_config
        // register genertation

        /*
         * wait previous register set done
         */
        // hal->get_regs;

        /*
         * send current register set to hardware
         */
        // hal->put_regs;

        /*
         * mark previous buffer is complete
         */
        // signal()
        // mark frame in output queue
        // wait up output thread to get a output frame

        msleep(10);
    }

    return NULL;
}

static void *thread_dec(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *thd_dec  = mpp->thd_codec;
    MppThread *thd_hal  = mpp->thd_hal;
    mpp_list *packets   = mpp->packets;
    mpp_list *frames    = mpp->frames;
    MppPacketImpl packet;
    MppFrame frame;

    while (MPP_THREAD_RUNNING == thd_dec->get_status()) {
        if (packets->list_size()) {
            /*
             * packet will be destroyed outside, here just copy the content
             */
            packets->del_at_head(&packet, sizeof(packet));

            /*
             * 1. send packet data to parser
             *
             *    parser functioin input / output
             *    input:    packet data
             *              dxva output slot
             *    output:   dxva output slot
             *              buffer usage informatioin
             */

            // decoder->parser->parse;

            /*
             * 2. do buffer operation according to usage information
             *
             *    possible case:
             *    a. normal case
             *       - wait and alloc a normal frame buffer
             *    b. field mode case
             *       - two field may reuse a same buffer, no need to alloc
             *    c. info change case
             *       - need buffer in different side, need to send a info change
             *         frame to hal loop.
             */

            // mpp->get_buffer

            /*
             * 3. send dxva output information and buffer information to hal thread
             *    combinate video codec dxva output and buffer information
             */

            // hal->wait_prev_done;
            // hal->send_config;


            // for test
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

    while (mpp->thread_codec_running) {
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
      thd_codec(NULL),
      thd_hal(NULL),
      thread_codec_running(0),
      thread_codec_reset(0),
      status(0)
{
    switch (type) {
    case MPP_CTX_DEC : {
        packets = new mpp_list((node_destructor)NULL);
        frames  = new mpp_list((node_destructor)mpp_frame_deinit);
        thd_codec = new MppThread(thread_dec, this);
        thd_hal   = new MppThread(thread_hal, this);
    } break;
    case MPP_CTX_ENC : {
        frames  = new mpp_list((node_destructor)NULL);
        packets = new mpp_list((node_destructor)mpp_packet_deinit);
        thd_codec = new MppThread(thread_enc, this);
        thd_hal   = new MppThread(thread_hal, this);
    } break;
    default : {
        mpp_err("Mpp error type %d\n", type);
    } break;
    }

    if (packets && frames && thd_codec && thd_hal) {
        thd_codec->start();
        thd_hal->start();
    } else {
        if (thd_codec)
            thd_codec->stop();
        if (thd_hal)
            thd_hal->stop();

        if (thd_codec) {
            delete thd_codec;
            thd_codec = NULL;
        }
        if (thd_hal) {
            delete thd_hal;
            thd_hal = NULL;
        }
        if (packets) {
            delete packets;
            packets = NULL;
        }
        if (frames) {
            delete frames;
            frames = NULL;
        }
    }
}

Mpp::~Mpp ()
{
    if (thd_codec)
        thd_codec->stop();
    if (thd_hal)
        thd_hal->stop();

    if (thd_codec)
        delete thd_codec;
    if (thd_hal)
        delete thd_hal;
    if (packets)
        delete packets;
    if (frames)
        delete frames;
}

MPP_RET Mpp::put_packet(MppPacket packet)
{
    // TODO: packet data need preprocess or can be write to hardware buffer
    return (MPP_RET)packets->add_at_tail(packet, sizeof(MppPacketImpl));
}

MPP_RET Mpp::get_frame(MppFrame *frame)
{
    if (frames->list_size()) {
        frames->del_at_tail(frame, sizeof(frame));
    }
    return MPP_OK;
}

MPP_RET Mpp::put_frame(MppFrame frame)
{
    MPP_RET ret = (MPP_RET)frames->add_at_tail(frame, sizeof(MppFrameImpl));
    return ret;
}

MPP_RET Mpp::get_packet(MppPacket *packet)
{
    if (packets->list_size()) {
        packets->del_at_tail(packet, sizeof(packet));
    }
    return MPP_OK;
}

