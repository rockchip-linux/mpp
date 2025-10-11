/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#ifndef __MPP_H__
#define __MPP_H__

#include "mpp_queue.h"
#include "mpp_task_impl.h"

#include "mpp_dec.h"
#include "mpp_enc.h"
#include "mpp_impl.h"
#include "kmpp_obj.h"
#include "kmpp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MPP_DBG_FUNCTION                    (0x00000001)
#define MPP_DBG_PACKET                      (0x00000002)
#define MPP_DBG_FRAME                       (0x00000004)
#define MPP_DBG_BUFFER                      (0x00000008)

/*
 * mpp notify event flags
 * When event happens mpp will signal deocder / encoder with different flag.
 * These event will wake up the codec thread or hal thread
 */
#define MPP_INPUT_ENQUEUE                   (0x00000001)
#define MPP_OUTPUT_DEQUEUE                  (0x00000002)
#define MPP_INPUT_DEQUEUE                   (0x00000004)
#define MPP_OUTPUT_ENQUEUE                  (0x00000008)
#define MPP_RESET                           (0xFFFFFFFF)

/* mpp dec event flags */
#define MPP_DEC_NOTIFY_PACKET_ENQUEUE       (MPP_INPUT_ENQUEUE)
#define MPP_DEC_NOTIFY_FRAME_DEQUEUE        (MPP_OUTPUT_DEQUEUE)
#define MPP_DEC_NOTIFY_EXT_BUF_GRP_READY    (0x00000010)
#define MPP_DEC_NOTIFY_INFO_CHG_DONE        (0x00000020)
#define MPP_DEC_NOTIFY_BUFFER_VALID         (0x00000040)
#define MPP_DEC_NOTIFY_TASK_ALL_DONE        (0x00000080)
#define MPP_DEC_NOTIFY_TASK_HND_VALID       (0x00000100)
#define MPP_DEC_NOTIFY_TASK_PREV_DONE       (0x00000200)
#define MPP_DEC_NOTIFY_BUFFER_MATCH         (0x00000400)
#define MPP_DEC_NOTIFY_SLOT_VALID           (0x00004000)
#define MPP_DEC_CONTROL                     (0x00010000)
#define MPP_DEC_RESET                       (MPP_RESET)

/* mpp enc event flags */
#define MPP_ENC_NOTIFY_FRAME_ENQUEUE        (MPP_INPUT_ENQUEUE)
#define MPP_ENC_NOTIFY_PACKET_DEQUEUE       (MPP_OUTPUT_DEQUEUE)
#define MPP_ENC_NOTIFY_FRAME_DEQUEUE        (MPP_INPUT_DEQUEUE)
#define MPP_ENC_NOTIFY_PACKET_ENQUEUE       (MPP_OUTPUT_ENQUEUE)
#define MPP_ENC_CONTROL                     (0x00000010)
#define MPP_ENC_RESET                       (MPP_RESET)

typedef enum MppIOMode_e {
    MPP_IO_MODE_DEFAULT                     = -1,
    MPP_IO_MODE_NORMAL                      = 0,
    MPP_IO_MODE_TASK                        = 1,
    MPP_IO_MODE_BUTT,
} MppIoMode;

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

typedef struct Mpp {
    /* Public members that were previously public in C++ class */
    MppList        *mPktIn;
    MppList        *mPktOut;
    MppList        *mFrmIn;
    MppList        *mFrmOut;
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
    RK_U32          mExternalBufferMode;

    /*
     * Mpp task queue for advance task mode
     */
    /*
     * Task data flow:
     *                  |
     *     user         |          mpp
     *           mInputTaskQueue
     * mUsrInPort  ->   |   -> mMppInPort
     *                  |          |
     *                  |          v
     *                  |       process
     *                  |          |
     *                  |          v
     * mUsrOutPort <-   |   <- mMppOutPort
     *           mOutputTaskQueue
     */
    MppPort         mUsrInPort;
    MppPort         mUsrOutPort;
    MppPort         mMppInPort;
    MppPort         mMppOutPort;
    MppTaskQueue    mInputTaskQueue;
    MppTaskQueue    mOutputTaskQueue;
    RK_S32          mInputTaskCount;
    RK_S32          mOutputTaskCount;

    MppPollType     mInputTimeout;
    MppPollType     mOutputTimeout;

    MppTask         mInputTask;
    MppTask         mEosTask;

    MppCtx          mCtx;
    MppDec          mDec;
    MppEnc          mEnc;

    RK_U32          mEncAyncIo;
    RK_U32          mEncAyncProc;
    MppIoMode       mIoMode;
    RK_U32          mDisableThread;

    /* dump info for debug */
    MppDump         mDump;

    /* kmpp infos */
    Kmpp            *mKmpp;
    KmppObj         mVencInitKcfg;

    MppCtxType      mType;
    MppCodingType   mCoding;

    RK_U32          mInitDone;

    RK_U32          mStatus;

    /* decoder paramter before init */
    MppDecCfg       mDecCfg;
    RK_U32          mParserFastMode;
    RK_U32          mParserNeedSplit;
    RK_U32          mParserInternalPts;     /* for MPEG2/MPEG4 */
    RK_U32          mImmediateOut;
    /* backup extra packet for seek */
    MppPacket       mExtraPacket;
} Mpp;

MPP_RET mpp_ctx_create(Mpp **mpp, MppCtx ctx);
MPP_RET mpp_ctx_destroy(Mpp *mpp);
MPP_RET mpp_ctx_init(Mpp *mpp, MppCtxType type, MppCodingType coding);
void    mpp_clear(Mpp *mpp);

/* Control functions */
MPP_RET mpp_start(Mpp *mpp);
MPP_RET mpp_stop(Mpp *mpp);
MPP_RET mpp_pause(Mpp *mpp);
MPP_RET mpp_resume(Mpp *mpp);

/* Data processing functions */
MPP_RET mpp_put_packet(Mpp *mpp, MppPacket packet);
MPP_RET mpp_get_frame(Mpp *mpp, MppFrame *frame);
MPP_RET mpp_get_frame_noblock(Mpp *mpp, MppFrame *frame);

MPP_RET mpp_put_frame(Mpp *mpp, MppFrame frame);
MPP_RET mpp_get_packet(Mpp *mpp, MppPacket *packet);

/* Task queue functions */
MPP_RET mpp_poll(Mpp *mpp, MppPortType type, MppPollType timeout);
MPP_RET mpp_dequeue(Mpp *mpp, MppPortType type, MppTask *task);
MPP_RET mpp_enqueue(Mpp *mpp, MppPortType type, MppTask task);

/* Decode function */
MPP_RET mpp_decode(Mpp *mpp, MppPacket packet, MppFrame *frame);

/* System functions */
MPP_RET mpp_reset(Mpp *mpp);
MPP_RET mpp_control(Mpp *mpp, MpiCmd cmd, MppParam param);

/* Notification functions */
MPP_RET mpp_notify_flag(Mpp *mpp, RK_U32 flag);
MPP_RET mpp_notify_group(Mpp *mpp, MppBufferGroup group);

MPP_RET mpp_control_mpp(Mpp *mpp, MpiCmd cmd, MppParam param);
MPP_RET mpp_control_osal(Mpp *mpp, MpiCmd cmd, MppParam param);
MPP_RET mpp_control_codec(Mpp *mpp, MpiCmd cmd, MppParam param);
MPP_RET mpp_control_dec(Mpp *mpp, MpiCmd cmd, MppParam param);
MPP_RET mpp_control_enc(Mpp *mpp, MpiCmd cmd, MppParam param);
MPP_RET mpp_control_isp(Mpp *mpp, MpiCmd cmd, MppParam param);

/* for special encoder async io mode */
MPP_RET mpp_put_frame_async(Mpp *mpp, MppFrame frame);
MPP_RET mpp_get_packet_async(Mpp *mpp, MppPacket *packet);

void mpp_set_io_mode(Mpp *mpp, MppIoMode mode);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_H__*/
