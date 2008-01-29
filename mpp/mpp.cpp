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
#include "mpp_env.h"
#include "mpp_time.h"

#include "mpp.h"
#include "mpp_dec.h"
#include "mpp_enc.h"
#include "mpp_hal.h"
#include "mpp_frame_impl.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M

RK_U32 mpp_debug = 0;

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
      mInternalGroup(NULL),
      mPacketGroup(NULL),
      mFrameGroup(NULL),
      mTheadCodec(NULL),
      mThreadHal(NULL),
      mType(type),
      mCoding(coding),
      mStatus(0),
      mDec(NULL),
      mEnc(NULL),
      mTask(NULL),
      mTaskNum(2)
{
    switch (mType) {
    case MPP_CTX_DEC : {
        mPackets    = new mpp_list((node_destructor)NULL);
        mFrames     = new mpp_list((node_destructor)mpp_frame_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);

        mpp_dec_init(&mDec, coding);
        mTheadCodec = new MppThread(mpp_dec_parser_thread, this);
        mThreadHal  = new MppThread(mpp_dec_hal_thread, this);

        mTask       = mpp_malloc(HalDecTask*, mTaskNum);
        mpp_buffer_group_normal_get(&mInternalGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_normal_get(&mPacketGroup, MPP_BUFFER_TYPE_NORMAL);
        mpp_buffer_group_limited_get(&mFrameGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_limit_config(mFrameGroup, 4, MPP_TEST_FRAME_SIZE);
    } break;
    case MPP_CTX_ENC : {
        mFrames     = new mpp_list((node_destructor)NULL);
        mPackets    = new mpp_list((node_destructor)mpp_packet_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);

        mpp_dec_init(&mDec, coding);
        mTheadCodec = new MppThread(mpp_enc_control_thread, this);
        mThreadHal  = new MppThread(mpp_dec_hal_thread, this);

        mTask       = mpp_malloc(HalDecTask*, mTaskNum);
        mpp_buffer_group_normal_get(&mInternalGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_normal_get(&mPacketGroup, MPP_BUFFER_TYPE_NORMAL);
        mpp_buffer_group_limited_get(&mFrameGroup, MPP_BUFFER_TYPE_ION);
    } break;
    default : {
        mpp_err("Mpp error type %d\n", mType);
    } break;
    }

    if (mFrames && mPackets && mTask &&
        (mDec || mEnc) &&
        mTheadCodec && mThreadHal &&
        mPacketGroup && mFrameGroup) {
        mTheadCodec->start();
        mThreadHal->start();
    } else
        clear();

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);
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
    if (mDec) {
        if (mType == MPP_CTX_DEC)
            mpp_dec_deinit(mDec);
        else
            mpp_dec_deinit(mDec);
        mDec = NULL;
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
    if (mInternalGroup) {
        mpp_buffer_group_put(mInternalGroup);
        mInternalGroup = NULL;
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

