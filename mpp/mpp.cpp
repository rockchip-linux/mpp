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
#define MPP_TEST_PACKET_SIZE    SZ_512K

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
      mThreadCodec(NULL),
      mThreadHal(NULL),
      mType(type),
      mCoding(coding),
      mOutputBlock(0),
      mMultiFrame(0),
      mStatus(0),
      mDec(NULL),
      mEnc(NULL)
{
    switch (mType) {
    case MPP_CTX_DEC : {
        mPackets    = new mpp_list((node_destructor)mpp_packet_deinit);
        mFrames     = new mpp_list((node_destructor)mpp_frame_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);

        mpp_dec_init(&mDec, coding);
        mThreadCodec = new MppThread(mpp_dec_parser_thread, this);
        mThreadHal  = new MppThread(mpp_dec_hal_thread, this);

        mpp_buffer_group_get_internal(&mInternalGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_get_internal(&mPacketGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_limit_config(mPacketGroup, 0, 2);

    } break;
    case MPP_CTX_ENC : {
        mFrames     = new mpp_list((node_destructor)NULL);
        mPackets    = new mpp_list((node_destructor)mpp_packet_deinit);
        mTasks      = new mpp_list((node_destructor)NULL);

        mpp_dec_init(&mEnc, coding);
        mThreadCodec = new MppThread(mpp_enc_control_thread, this);
        mThreadHal  = new MppThread(mpp_enc_hal_thread, this);

        mpp_buffer_group_get_internal(&mInternalGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_get_internal(&mPacketGroup, MPP_BUFFER_TYPE_NORMAL);
        mpp_buffer_group_get_external(&mFrameGroup, MPP_BUFFER_TYPE_ION);
    } break;
    default : {
        mpp_err("Mpp error type %d\n", mType);
    } break;
    }

    if (mFrames && mPackets &&
        (mDec || mEnc) &&
        mThreadCodec && mThreadHal &&
        mPacketGroup) {
        mThreadCodec->start();
        mThreadHal->start();
    } else {
        mpp_err("error found on mpp initialization\n");
        clear();
    }

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);
}

Mpp::~Mpp ()
{
    clear();
}

void Mpp::clear()
{
    if (mThreadCodec)
        mThreadCodec->stop();
    if (mThreadHal)
        mThreadHal->stop();

    if (mThreadCodec) {
        delete mThreadCodec;
        mThreadCodec = NULL;
    }
    if (mThreadHal) {
        delete mThreadHal;
        mThreadHal = NULL;
    }
    if (mDec || mEnc) {
        if (mType == MPP_CTX_DEC) {
            mpp_dec_deinit(mDec);
            mDec = NULL;
        }
        else {
            mpp_dec_deinit(mEnc);
            mEnc = NULL;
        }
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
        MppBufferMode mode = mpp_buffer_group_mode(mFrameGroup);
        if (MPP_BUFFER_INTERNAL == mode) {
            mpp_buffer_group_put(mFrameGroup);
        }
        mFrameGroup = NULL;
    }
}

MPP_RET Mpp::put_packet(MppPacket packet)
{
    Mutex::Autolock autoLock(mPackets->mutex());
    RK_U32 eos = mpp_packet_get_eos(packet);
    if (mPackets->list_size() < 4 || eos) {
        MppPacket pkt;
        mpp_packet_copy_init(&pkt, packet);
        mPackets->add_at_tail(&pkt, sizeof(pkt));
        mPacketPutCount++;
        mThreadCodec->signal();
        return MPP_OK;
    }
    return MPP_NOK;
}

MPP_RET Mpp::get_frame(MppFrame *frame)
{
    Mutex::Autolock autoLock(mFrames->mutex());
    MppFrame first = NULL;

    if (0 == mFrames->list_size()) {
        mThreadCodec->signal();
        if (mOutputBlock)
            mFrames->wait();
    }

    if (mFrames->list_size()) {
        mFrames->del_at_head(&first, sizeof(frame));
        mFrameGetCount++;
        mThreadHal->signal();

        if (mMultiFrame) {
            MppFrame prev = first;
            MppFrame next = NULL;
            while (mFrames->list_size()) {
                mFrames->del_at_head(&next, sizeof(frame));
                mFrameGetCount++;
                mThreadHal->signal();
                mpp_frame_set_next(prev, next);
                prev = next;
            }
        }
        *frame = first;
        return MPP_OK;
    }
    return MPP_NOK;
}

MPP_RET Mpp::put_frame(MppFrame frame)
{
    Mutex::Autolock autoLock(mFrames->mutex());
    if (mFrames->list_size() < 4) {
        mFrames->add_at_tail(frame, sizeof(MppFrameImpl));
        mThreadCodec->signal();
        mFramePutCount++;
        return MPP_OK;
    }
    return MPP_NOK;
}

MPP_RET Mpp::get_packet(MppPacket *packet)
{
    Mutex::Autolock autoLock(mPackets->mutex());
    if (0 == mPackets->list_size()) {
        mThreadCodec->signal();
        if (mOutputBlock)
            mPackets->wait();
    }

    if (mPackets->list_size()) {
        mPackets->del_at_head(packet, sizeof(packet));
        mPacketGetCount++;
    }
    return MPP_OK;
}
MPP_RET Mpp::control(MpiCmd cmd, MppParam param)
{
    switch (cmd) {
    case MPP_DEC_SET_EXT_BUF_GROUP: {
        mpp_log("mpi_control group %p", param);
        mFrameGroup = (MppBufferGroup)param;
        break;
    }
    case MPP_SET_OUTPUT_BLOCK: {
        RK_U32 block = *((RK_U32 *)param);
        mOutputBlock = block;
        break;
    }
    case MPP_CODEC_SET_INFO_CHANGE_READY: {
        if (mType == MPP_CTX_DEC) {
            mpp_buf_slot_ready(mDec->frame_slots);
        }
        break;
    }
    case MPP_CODEC_SET_FRAME_INFO: {
        mpp_assert(mType == MPP_CTX_DEC);
        mpp_dec_control(mDec, cmd, param);
        break;
    }
    default : {
    } break;
    }
    return MPP_OK;
}

MPP_RET Mpp::reset()
{
    {
        Mutex::Autolock autoLock(mPackets->mutex());
        mPackets->flush();
    }
    {
        Mutex::Autolock autoLock(mFrames->mutex());
        mFrames->flush();
    }
    mThreadCodec->reset_lock();

    if (mType == MPP_CTX_DEC) {
        mpp_dec_reset(mDec);
        mThreadCodec->signal();
        mThreadCodec->reset_wait();
        mThreadCodec->reset_unlock();
    } else {
        mpp_dec_reset(mEnc);
    }

    return MPP_OK;
}
