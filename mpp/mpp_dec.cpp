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

#define  MODULE_TAG "mpp_dec"

#include "mpp_log.h"
#include "mpp_time.h"

#include "mpp.h"
#include "mpp_dec.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M

void *mpp_dec_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *dec  = mpp->mTheadCodec;
    MppThread *hal  = mpp->mThreadHal;
    mpp_list *packets   = mpp->mPackets;
    mpp_list *frames    = mpp->mFrames;
    MppPacketImpl packet;

    while (MPP_THREAD_RUNNING == dec->get_status()) {
        RK_U32 packet_ready = 0;
        /*
         * wait for stream input
         */
        dec->lock();
        if (0 == packets->list_size())
            dec->wait();
        dec->unlock();

        if (packets->list_size()) {
            mpp->mPacketLock.lock();
            /*
             * packet will be destroyed outside, here just copy the content
             */
            packets->del_at_head(&packet, sizeof(packet));
            mpp->mPacketGetCount++;
            packet_ready = 1;
            mpp->mPacketLock.unlock();
        }

        if (packet_ready) {
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

            //MppBuffer buffer;
            //mpp_buffer_get(mpp->mFrameGroup, &buffer, MPP_TEST_FRAME_SIZE);


            /*
             * 3. send dxva output information and buffer information to hal thread
             *    combinate video codec dxva output and buffer information
             */

            // hal->wait_prev_done;
            // hal->send_config;
            mpp->mTasks->add_at_tail(&mpp->mTask[0], sizeof(mpp->mTask[0]));
            mpp->mTaskPutCount++;
            hal->signal();
        }
    }

    return NULL;
}

