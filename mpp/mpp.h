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

#ifndef __MPP_H__
#define __MPP_H__

#include "mpp_list.h"
#include "mpp_dec.h"
#include "mpp_enc.h"

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

    MPP_RET reset();
    MPP_RET control(MpiCmd cmd, MppParam param);

    mpp_list        *mPackets;
    mpp_list        *mFrames;
    mpp_list        *mTasks;

    RK_U32          mPacketPutCount;
    RK_U32          mPacketGetCount;
    RK_U32          mFramePutCount;
    RK_U32          mFrameGetCount;
    RK_U32          mTaskPutCount;
    RK_U32          mTaskGetCount;

    /*
     * internal buffer group
     *      - for all kind of table like cabac / scaling list
     *        fixed to ion buffer group
     * packet buffer group
     *      - packets in I/O, can be ion buffer or normal buffer
     * frame buffer group
     *      - frames in I/O, normally should be a ion buffer group
     */
    MppBufferGroup  mInternalGroup;
    MppBufferGroup  mPacketGroup;
    MppBufferGroup  mFrameGroup;

    MppThread       *mThreadCodec;
    MppThread       *mThreadHal;

    MppDec          *mDec;
    MppEnc          *mEnc;

    RK_U32          mFastMode;
    RK_U32          mInitDone;

private:
    void clear();

    MppCtxType      mType;
    MppCodingType   mCoding;

    RK_U32          mPacketBlock;
    RK_U32          mOutputBlock;
    RK_U32          mMultiFrame;

    RK_U32          mStatus;

    Mpp(const Mpp &);
    Mpp &operator=(const Mpp &);
};

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif /*__MPP_H__*/
