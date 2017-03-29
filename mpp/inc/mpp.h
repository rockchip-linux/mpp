/*
 * Copyright 2015 Rockchip Electronics Co. LTD
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

#ifndef __MPP_H__
#define __MPP_H__

#include "mpp_list.h"
#include "mpp_dec.h"
#include "mpp_enc.h"
#include "mpp_task.h"

#define MPP_DBG_FUNCTION                (0x00000001)
#define MPP_DBG_PACKET                  (0x00000002)
#define MPP_DBG_FRAME                   (0x00000004)
#define MPP_DBG_BUFFER                  (0x00000008)

/*
 * mpp hierarchy
 *
 * mpp layer create mpp_dec or mpp_dec instance
 * mpp_dec create its parser and hal module
 * mpp_enc create its control and hal module
 *
 *                                  +-------+
 *                                  |       |
 *                    +-------------+  mpp  +-------------+
 *                    |             |       |             |
 *                    |             +-------+             |
 *                    |                                   |
 *                    |                                   |
 *                    |                                   |
 *              +-----+-----+                       +-----+-----+
 *              |           |                       |           |
 *          +---+  mpp_dec  +--+                 +--+  mpp_enc  +---+
 *          |   |           |  |                 |  |           |   |
 *          |   +-----------+  |                 |  +-----------+   |
 *          |                  |                 |                  |
 *          |                  |                 |                  |
 *          |                  |                 |                  |
 *  +-------v------+     +-----v-----+     +-----v-----+     +------v-------+
 *  |              |     |           |     |           |     |              |
 *  |  dec_parser  |     |  dec_hal  |     |  enc_hal  |     |  enc_control |
 *  |              |     |           |     |           |     |              |
 *  +--------------+     +-----------+     +-----------+     +--------------+
 */

#ifdef __cplusplus

class Mpp
{
public:
    Mpp();
    ~Mpp();
    MPP_RET init(MppCtxType type, MppCodingType coding);
    MPP_RET put_packet(MppPacket packet);
    MPP_RET get_frame(MppFrame *frame);

    MPP_RET put_frame(MppFrame frame);
    MPP_RET get_packet(MppPacket *packet);

    MPP_RET poll(MppPortType type, MppPollType timeout);
    MPP_RET dequeue(MppPortType type, MppTask *task);
    MPP_RET enqueue(MppPortType type, MppTask task);

    MPP_RET reset();
    MPP_RET control(MpiCmd cmd, MppParam param);

    mpp_list        *mPackets;
    mpp_list        *mFrames;
    mpp_list        *mTasks;

    /* counters for debug */
    RK_U32          mPacketPutCount;
    RK_U32          mPacketGetCount;
    RK_U32          mFramePutCount;
    RK_U32          mFrameGetCount;
    RK_U32          mTaskPutCount;
    RK_U32          mTaskGetCount;

    /*
     * packet buffer group
     *      - packets in I/O, can be ion buffer or normal buffer
     * frame buffer group
     *      - frames in I/O, normally should be a ion buffer group
     */
    MppBufferGroup  mPacketGroup;
    MppBufferGroup  mFrameGroup;
    RK_U32          mExternalFrameGroup;

    /*
     * Mpp task queue for advance task mode
     */
    Mutex           mPortLock;
    MppPort         mInputPort;
    MppPort         mOutputPort;

    MppTaskQueue    mInputTaskQueue;
    MppTaskQueue    mOutputTaskQueue;

    MppPollType     mInputBlock;
    MppPollType     mOutputBlock;
    RK_S64          mOutputBlockTimeout;
    /*
     * There are two threads for each decoder/encoder: codec thread and hal thread
     *
     * codec thread generate protocol syntax structure and send to hardware
     * hal thread wait hardware return and do corresponding process
     *
     * Two threads work parallelly so that all decoder/encoder will share this
     * acceleration mechanism
     */
    MppThread       *mThreadCodec;
    MppThread       *mThreadHal;

    MppDec          *mDec;
    MppEnc          *mEnc;

private:
    void clear();

    MppCtxType      mType;
    MppCodingType   mCoding;

    RK_U32          mInitDone;
    RK_U32          mMultiFrame;

    RK_U32          mStatus;

    /* decoder paramter before init */
    RK_U32          mParserFastMode;
    RK_U32          mParserNeedSplit;
    RK_U32          mParserInternalPts;     /* for MPEG2/MPEG4 */

    MPP_RET control_mpp(MpiCmd cmd, MppParam param);
    MPP_RET control_osal(MpiCmd cmd, MppParam param);
    MPP_RET control_codec(MpiCmd cmd, MppParam param);
    MPP_RET control_dec(MpiCmd cmd, MppParam param);
    MPP_RET control_enc(MpiCmd cmd, MppParam param);
    MPP_RET control_isp(MpiCmd cmd, MppParam param);

    Mpp(const Mpp &);
    Mpp &operator=(const Mpp &);
};


extern "C" {
}
#endif

#endif /*__MPP_H__*/
