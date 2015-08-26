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
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M

void *thread_hal(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *codec    = mpp->mTheadCodec;
    MppThread *hal      = mpp->mThreadHal;
    mpp_list *frames    = mpp->mFrames;
    mpp_list *tasks     = mpp->mTasks;

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (0 == tasks->list_size())
            hal->wait();
        hal->unlock();

        // get_config
        // register genertation
        if (tasks->list_size()) {
            MppHalDecTask *task;
            mpp->mTasks->del_at_head(&task, sizeof(task));
            mpp->mTaskGetCount++;

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

            // for test
            MppBuffer buffer;
            mpp_buffer_get(mpp->mFrameGroup, &buffer, MPP_TEST_FRAME_SIZE);

            MppFrame frame;
            mpp_frame_init(&frame);
            mpp_frame_set_buffer(frame, buffer);
            frames->add_at_tail(&frame, sizeof(frame));
            mpp->mFramePutCount++;
        }
    }

    return NULL;
}

static void *thread_dec(void *data)
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

static void *thread_enc(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *thd_enc  = mpp->mTheadCodec;
    MppThread *thd_hal  = mpp->mThreadHal;
    mpp_list *packets = mpp->mPackets;
    mpp_list *frames  = mpp->mFrames;
    MppFrameImpl frame;
    MppPacket packet;
    size_t size = SZ_1M;
    char *buf = mpp_malloc(char, size);

    while (MPP_THREAD_RUNNING == thd_enc->get_status()) {
        if (frames->list_size()) {
            frames->del_at_head(&frame, sizeof(frame));

            mpp_packet_init(&packet, buf, size);
            packets->add_at_tail(&packet, sizeof(packet));
        }
    }
    mpp_free(buf);
    return NULL;
}

Mpp::Mpp(MppCtxType type, MppCodingType coding)
    : mPackets(NULL),
      mFrames(NULL),
      mTasks(NULL),
      mPacketPutCount(0),
      mPacketGetCount(0),
      mFramePutCount(0),
      mFrameGetCount(0),
      mTaskPutCount(0),
      mTaskGetCount(0),
      mPacketGroup(NULL),
      mFrameGroup(NULL),
      mTheadCodec(NULL),
      mThreadHal(NULL),
      mType(type),
      mCoding(coding),
      mStatus(0),
      mTask(NULL),
      mTaskNum(2)
{
    switch (mType) {
    case MPP_CTX_DEC : {
        mPackets    = new mpp_list((node_destructor)NULL);
        mFrames     = new mpp_list((node_destructor)mpp_frame_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);
        mTheadCodec = new MppThread(thread_dec, this);
        mThreadHal  = new MppThread(thread_hal, this);
        mTask       = mpp_malloc(MppHalDecTask*, mTaskNum);
        mpp_buffer_group_normal_get(&mPacketGroup, MPP_BUFFER_TYPE_NORMAL);
        mpp_buffer_group_limited_get(&mFrameGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_limit_config(mFrameGroup, 4, MPP_TEST_FRAME_SIZE);
    } break;
    case MPP_CTX_ENC : {
        mFrames     = new mpp_list((node_destructor)NULL);
        mPackets    = new mpp_list((node_destructor)mpp_packet_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);
        mTheadCodec = new MppThread(thread_enc, this);
        mThreadHal  = new MppThread(thread_hal, this);
        mTask       = mpp_malloc(MppHalDecTask*, mTaskNum);
        mpp_buffer_group_normal_get(&mPacketGroup, MPP_BUFFER_TYPE_NORMAL);
        mpp_buffer_group_limited_get(&mFrameGroup, MPP_BUFFER_TYPE_ION);
    } break;
    default : {
        mpp_err("Mpp error type %d\n", mType);
    } break;
    }

    if (mFrames && mPackets && mTask &&
        mTheadCodec && mThreadHal &&
        mPacketGroup && mFrameGroup) {
        mTheadCodec->start();
        mThreadHal->start();
    } else
        clear();
}

Mpp::~Mpp ()
{
    clear();
}

void Mpp::clear()
{
    if (mTheadCodec)
        mTheadCodec->stop();
    if (mThreadHal)
        mThreadHal->stop();

    if (mTheadCodec) {
        delete mTheadCodec;
        mTheadCodec = NULL;
    }
    if (mThreadHal) {
        delete mThreadHal;
        mThreadHal = NULL;
    }
    if (mPackets) {
        delete mPackets;
        mPackets = NULL;
    }
    if (mFrames) {
        delete mFrames;
        mFrames = NULL;
    }
    if (mTasks) {
        delete mTasks;
        mTasks = NULL;
    }
    if (mPacketGroup) {
        mpp_buffer_group_put(mPacketGroup);
        mPacketGroup = NULL;
    }
    if (mFrameGroup) {
        mpp_buffer_group_put(mFrameGroup);
        mFrameGroup = NULL;
    }
    if (mTask)
        mpp_free(mTask);
}

MPP_RET Mpp::put_packet(MppPacket packet)
{
    Mutex::Autolock autoLock(&mPacketLock);
    if (mPackets->list_size() < 4) {
        mPackets->add_at_tail(packet, sizeof(MppPacketImpl));
        mPacketPutCount++;
        mTheadCodec->signal();
        return MPP_OK;
    }
    return MPP_NOK;
}

MPP_RET Mpp::get_frame(MppFrame *frame)
{
    Mutex::Autolock autoLock(&mFrameLock);
    if (mFrames->list_size()) {
        mFrames->del_at_tail(frame, sizeof(frame));
        mFrameGetCount++;
    }
    mThreadHal->signal();
    return MPP_OK;
}

MPP_RET Mpp::put_frame(MppFrame frame)
{
    Mutex::Autolock autoLock(&mFrameLock);
    if (mFrames->list_size() < 4) {
        mFrames->add_at_tail(frame, sizeof(MppFrameImpl));
        mTheadCodec->signal();
        mFramePutCount++;
        return MPP_OK;
    }
    return MPP_NOK;
}

MPP_RET Mpp::get_packet(MppPacket *packet)
{
    Mutex::Autolock autoLock(&mPacketLock);
    if (mPackets->list_size()) {
        mPackets->del_at_tail(packet, sizeof(packet));
        mPacketGetCount++;
    }
    return MPP_OK;
}

