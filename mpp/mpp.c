/* SPDX-License-Identifier: Apache-2.0 OR MIT */
/*
 * Copyright (c) 2015 Rockchip Electronics Co., Ltd.
 */

#define  MODULE_TAG "mpp"

#include <errno.h>
#include <string.h>

#include "rk_mpi.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_time.h"
#include "mpp_impl.h"
#include "mpp_2str.h"
#include "mpp_debug.h"

#include "mpp.h"
#include "mpp_hal.h"

#include "mpp_task_impl.h"
#include "mpp_buffer_impl.h"
#include "mpp_frame_impl.h"
#include "mpp_packet_impl.h"

#include "kmpp.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M
#define MPP_TEST_PACKET_SIZE    SZ_512K

static void mpp_notify_by_buffer_group(void *arg, void *group)
{
    Mpp *mpp = (Mpp *)arg;
    mpp_notify_group(mpp, (MppBufferGroup)group);
}

static void *list_wraper_packet(void *arg)
{
    MppPacket packet = *(MppPacket*)arg;

    if (mpp_packet_has_meta(packet)) {
        MppMeta meta = mpp_packet_get_meta(packet);
        MppFrame frm = NULL;

        if (MPP_OK == mpp_meta_get_frame(meta, KEY_INPUT_FRAME, &frm)) {
            mpp_assert(frm);
            mpp_frame_deinit(&frm);
        }
    }

    mpp_packet_deinit((MppPacket *)arg);
    return NULL;
}

static void *list_wraper_frame(void *arg)
{
    mpp_frame_deinit((MppFrame *)arg);
    return NULL;
}

static RK_S32 check_frm_task_cnt_cap(MppCodingType coding)
{
    RockchipSocType soc_type = mpp_get_soc_type();

    if (soc_type == ROCKCHIP_SOC_RK3588 || soc_type == ROCKCHIP_SOC_RK3576) {
        if (coding == MPP_VIDEO_CodingAVC || coding == MPP_VIDEO_CodingHEVC)
            return 2;
        if (coding == MPP_VIDEO_CodingMJPEG && soc_type == ROCKCHIP_SOC_RK3588)
            return 4;
    }

    mpp_log("Only rk3588's h264/265/jpeg and rk3576's h264/265 encoder can use frame parallel\n");

    return 1;
}

MPP_RET mpp_ctx_create(Mpp **mpp, MppCtx ctx)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    Mpp *p = mpp_calloc(Mpp, 1);
    if (!p) {
        mpp_err_f("failed to allocate mpp context\n");
        return MPP_ERR_MALLOC;
    }

    /* Initialize all members to default values */
    p->mPktIn = NULL;
    p->mPktOut = NULL;
    p->mFrmIn = NULL;
    p->mFrmOut = NULL;
    p->mPacketPutCount = 0;
    p->mPacketGetCount = 0;
    p->mFramePutCount = 0;
    p->mFrameGetCount = 0;
    p->mTaskPutCount = 0;
    p->mTaskGetCount = 0;
    p->mPacketGroup = NULL;
    p->mFrameGroup = NULL;
    p->mExternalBufferMode = 0;
    p->mUsrInPort = NULL;
    p->mUsrOutPort = NULL;
    p->mMppInPort = NULL;
    p->mMppOutPort = NULL;
    p->mInputTaskQueue = NULL;
    p->mOutputTaskQueue = NULL;
    p->mInputTaskCount = 1;
    p->mOutputTaskCount = 1;
    p->mInputTimeout = MPP_POLL_BUTT;
    p->mOutputTimeout = MPP_POLL_BUTT;
    p->mInputTask = NULL;
    p->mEosTask = NULL;
    p->mCtx = ctx;
    p->mDec = NULL;
    p->mEnc = NULL;
    p->mEncAyncIo = 0;
    p->mEncAyncProc = 0;
    p->mIoMode = MPP_IO_MODE_DEFAULT;
    p->mDisableThread = 0;
    p->mDump = NULL;
    p->mKmpp = NULL;
    p->mVencInitKcfg = NULL;
    p->mType = MPP_CTX_BUTT;
    p->mCoding = MPP_VIDEO_CodingUnused;
    p->mInitDone = 0;
    p->mStatus = 0;
    p->mDecCfg = NULL;
    p->mParserFastMode = 0;
    p->mParserNeedSplit = 0;
    p->mParserInternalPts = 0;
    p->mImmediateOut = 0;
    p->mExtraPacket = NULL;

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    mpp_dec_cfg_init(&p->mDecCfg);
    mpp_dec_cfg_set_u32(p->mDecCfg, "base:enable_vproc", MPP_VPROC_MODE_DEINTELACE);

    mpp_dump_init(&p->mDump);

    *mpp = p;
    return MPP_OK;
}

MPP_RET mpp_ctx_init(Mpp *mpp, MppCtxType type, MppCodingType coding)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp_check_soc_cap(type, coding)) {
        mpp_err("unable to create %s %s for soc %s unsupported\n",
                strof_ctx_type(type), strof_coding_type(coding),
                mpp_get_soc_info()->compatible);
        return MPP_NOK;
    }

    if (mpp_check_support_format(type, coding)) {
        mpp_err("unable to create %s %s for mpp unsupported\n",
                strof_ctx_type(type), strof_coding_type(coding));
        return MPP_NOK;
    }

    mpp_ops_init(mpp->mDump, type, coding);

    mpp->mType = type;
    mpp->mCoding = coding;

    /* init kmpp venc */
    if (mpp->mVencInitKcfg) {
        mpp->mKmpp = mpp_calloc(Kmpp, 1);
        if (!mpp->mKmpp) {
            mpp_err("failed to alloc kmpp context\n");
            return MPP_NOK;
        }
        mpp->mKmpp->mClientFd = -1;
        mpp_get_api(mpp->mKmpp);
        mpp->mKmpp->mVencInitKcfg = mpp->mVencInitKcfg;
        ret = mpp->mKmpp->mApi->init(mpp->mKmpp, type, coding);
        if (ret) {
            mpp_err("failed to init kmpp ret %d\n", ret);
            return ret;
        }
        mpp->mInitDone = 1;
        return ret;
    }

    mpp_task_queue_init(&mpp->mInputTaskQueue, mpp, "input");
    mpp_task_queue_init(&mpp->mOutputTaskQueue, mpp, "output");

    switch (mpp->mType) {
    case MPP_CTX_DEC : {
        mpp->mPktIn = mpp_list_create(list_wraper_packet);
        mpp->mFrmOut = mpp_list_create(list_wraper_frame);

        if (mpp->mInputTimeout == MPP_POLL_BUTT)
            mpp->mInputTimeout = MPP_POLL_NON_BLOCK;

        if (mpp->mOutputTimeout == MPP_POLL_BUTT)
            mpp->mOutputTimeout = MPP_POLL_NON_BLOCK;

        if (mpp->mCoding != MPP_VIDEO_CodingMJPEG) {
            mpp_buffer_group_get_internal(&mpp->mPacketGroup, MPP_BUFFER_TYPE_ION | MPP_BUFFER_FLAGS_CACHABLE);
            mpp_buffer_group_limit_config(mpp->mPacketGroup, 0, 3);

            mpp->mInputTaskCount = 4;
            mpp->mOutputTaskCount = 4;
        }

        mpp_task_queue_setup(mpp->mInputTaskQueue, mpp->mInputTaskCount);
        mpp_task_queue_setup(mpp->mOutputTaskQueue, mpp->mOutputTaskCount);

        mpp->mUsrInPort  = mpp_task_queue_get_port(mpp->mInputTaskQueue, MPP_PORT_INPUT);
        mpp->mUsrOutPort = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_OUTPUT);
        mpp->mMppInPort  = mpp_task_queue_get_port(mpp->mInputTaskQueue, MPP_PORT_OUTPUT);
        mpp->mMppOutPort = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);

        mpp_dec_cfg_set_u32(mpp->mDecCfg, "base:disable_thread", mpp->mDisableThread);

        MppDecInitCfg cfg = {
            coding,
            mpp,
            mpp->mDecCfg,
        };

        ret = mpp_dec_init(&mpp->mDec, &cfg);
        if (ret)
            break;
        ret = mpp_dec_start(mpp->mDec);
        if (ret)
            break;
        mpp->mInitDone = 1;
    } break;
    case MPP_CTX_ENC : {
        mpp->mPktOut = mpp_list_create(list_wraper_packet);
        mpp->mFrmIn = mpp_list_create(list_wraper_frame);

        if (mpp->mInputTimeout == MPP_POLL_BUTT)
            mpp->mInputTimeout = MPP_POLL_BLOCK;

        if (mpp->mOutputTimeout == MPP_POLL_BUTT)
            mpp->mOutputTimeout = MPP_POLL_NON_BLOCK;

        mpp_buffer_group_get_internal(&mpp->mPacketGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_get_internal(&mpp->mFrameGroup, MPP_BUFFER_TYPE_ION);

        if (mpp->mInputTimeout == MPP_POLL_NON_BLOCK) {
            mpp->mEncAyncIo = 1;

            mpp->mInputTaskCount = check_frm_task_cnt_cap(coding);
            if (mpp->mInputTaskCount == 1)
                mpp->mInputTimeout = MPP_POLL_BLOCK;
        }
        mpp->mOutputTaskCount = 8;

        mpp_task_queue_setup(mpp->mInputTaskQueue, mpp->mInputTaskCount);
        mpp_task_queue_setup(mpp->mOutputTaskQueue, mpp->mOutputTaskCount);

        mpp->mUsrInPort = mpp_task_queue_get_port(mpp->mInputTaskQueue, MPP_PORT_INPUT);
        mpp->mUsrOutPort = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_OUTPUT);
        mpp->mMppInPort = mpp_task_queue_get_port(mpp->mInputTaskQueue, MPP_PORT_OUTPUT);
        mpp->mMppOutPort = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);

        MppEncInitCfg cfg = {
            coding,
            mpp->mInputTaskCount,
            mpp,
        };

        ret = mpp_enc_init_v2(&mpp->mEnc, &cfg);
        if (ret)
            break;

        if (mpp->mInputTimeout == MPP_POLL_NON_BLOCK) {
            mpp->mEncAyncProc = 1;
            ret = mpp_enc_start_async(mpp->mEnc);
        } else {
            ret = mpp_enc_start_v2(mpp->mEnc);
        }

        if (ret)
            break;
        mpp->mInitDone = 1;
    } break;
    default : {
        mpp_err("Mpp error type %d\n", mpp->mType);
    } break;
    }

    if (!mpp->mInitDone) {
        mpp_err("error found on mpp initialization\n");
        mpp_clear(mpp);
    }

    return ret;
}

void mpp_clear(Mpp *mpp)
{
    if (!mpp)
        return;

    /* MUST: release listener here */
    if (mpp->mFrameGroup)
        mpp_buffer_group_set_callback((MppBufferGroupImpl *)mpp->mFrameGroup,
                                      NULL, NULL);

    if (mpp->mType == MPP_CTX_DEC) {
        if (mpp->mDec) {
            mpp_dec_stop(mpp->mDec);
            mpp_dec_deinit(mpp->mDec);
            mpp->mDec = NULL;
        }
    } else {
        if (mpp->mEnc) {
            mpp_enc_stop_v2(mpp->mEnc);
            mpp_enc_deinit_v2(mpp->mEnc);
            mpp->mEnc = NULL;
        }
    }

    if (mpp->mInputTaskQueue) {
        mpp_task_queue_deinit(mpp->mInputTaskQueue);
        mpp->mInputTaskQueue = NULL;
    }
    if (mpp->mOutputTaskQueue) {
        mpp_task_queue_deinit(mpp->mOutputTaskQueue);
        mpp->mOutputTaskQueue = NULL;
    }

    mpp->mUsrInPort = NULL;
    mpp->mUsrOutPort = NULL;
    mpp->mMppInPort = NULL;
    mpp->mMppOutPort = NULL;

    if (mpp->mExtraPacket) {
        mpp_packet_deinit(&mpp->mExtraPacket);
        mpp->mExtraPacket = NULL;
    }

    if (mpp->mPktIn) {
        mpp_list_destroy(mpp->mPktIn);
        mpp->mPktIn = NULL;
    }
    if (mpp->mPktOut) {
        mpp_list_destroy(mpp->mPktOut);
        mpp->mPktOut = NULL;
    }
    if (mpp->mFrmIn) {
        mpp_list_destroy(mpp->mFrmIn);
        mpp->mFrmIn = NULL;
    }
    if (mpp->mFrmOut) {
        mpp_list_destroy(mpp->mFrmOut);
        mpp->mFrmOut = NULL;
    }

    if (mpp->mPacketGroup) {
        mpp_buffer_group_put(mpp->mPacketGroup);
        mpp->mPacketGroup = NULL;
    }

    if (mpp->mFrameGroup && !mpp->mExternalBufferMode) {
        mpp_buffer_group_put(mpp->mFrameGroup);
        mpp->mFrameGroup = NULL;
    }

    if (mpp->mKmpp) {
        if (mpp->mKmpp->mApi && mpp->mKmpp->mApi->clear)
            mpp->mKmpp->mApi->clear(mpp->mKmpp);

        MPP_FREE(mpp->mKmpp);
    }

    if (mpp->mDecCfg) {
        mpp_dec_cfg_deinit(mpp->mDecCfg);
        mpp->mDecCfg = NULL;
    }

    mpp_dump_deinit(&mpp->mDump);
}

MPP_RET mpp_ctx_destroy(Mpp *mpp)
{
    if (!mpp)
        return MPP_OK;

    mpp_clear(mpp);
    mpp_free(mpp);

    return MPP_OK;
}

MPP_RET mpp_start(Mpp *mpp)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}

MPP_RET mpp_stop(Mpp *mpp)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}

MPP_RET mpp_pause(Mpp *mpp)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}

MPP_RET mpp_resume(Mpp *mpp)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}

MPP_RET mpp_put_packet(Mpp *mpp, MppPacket packet)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    MPP_RET ret = MPP_NOK;
    MppPollType timeout = mpp->mInputTimeout;
    MppTask task_dequeue = NULL;
    RK_U32 pkt_copy = 0;

    if (mpp->mDisableThread) {
        mpp_err_f("no thread decoding case MUST use mpi_decode interface\n");
        return ret;
    }

    if (mpp->mExtraPacket) {
        MppPacket extra = mpp->mExtraPacket;

        mpp->mExtraPacket = NULL;
        mpp_put_packet(mpp, extra);
    }

    /* non-jpeg mode - reserve extra task for incoming eos packet */
    if (mpp->mInputTaskCount > 1) {
        if (!mpp->mEosTask) {
            /* handle eos packet on block mode */
            ret = mpp_poll(mpp, MPP_PORT_INPUT, MPP_POLL_BLOCK);
            if (ret < 0)
                goto RET;

            mpp_dequeue(mpp, MPP_PORT_INPUT, &mpp->mEosTask);
            if (NULL == mpp->mEosTask) {
                mpp_err_f("fail to reserve eos task\n");
                ret = MPP_NOK;
                goto RET;
            }
        }

        if (mpp_packet_get_eos(packet)) {
            mpp_assert(mpp->mEosTask);
            task_dequeue = mpp->mEosTask;
            mpp->mEosTask = NULL;
        }
    }

    /* Use reserved task to send eos packet */
    if (mpp->mInputTask && !task_dequeue) {
        task_dequeue = mpp->mInputTask;
        mpp->mInputTask = NULL;
    }

    if (NULL == task_dequeue) {
        ret = mpp_poll(mpp, MPP_PORT_INPUT, timeout);
        if (ret < 0) {
            ret = MPP_ERR_BUFFER_FULL;
            goto RET;
        }

        /* do not pull here to avoid block wait */
        mpp_dequeue(mpp, MPP_PORT_INPUT, &task_dequeue);
        if (NULL == task_dequeue) {
            mpp_err_f("fail to get task on poll ret %d\n", ret);
            ret = MPP_NOK;
            goto RET;
        }
    }

    if (NULL == mpp_packet_get_buffer(packet)) {
        /* packet copy path */
        MppPacket pkt_in = NULL;

        mpp_packet_copy_init(&pkt_in, packet);
        mpp_packet_set_length(packet, 0);
        pkt_copy = 1;
        packet = pkt_in;
        ret = MPP_OK;
    } else {
        /* packet zero copy path */
        timeout = MPP_POLL_BLOCK;
        ret = MPP_OK;
    }

    /* setup task */
    ret = mpp_task_meta_set_packet(task_dequeue, KEY_INPUT_PACKET, packet);
    if (ret) {
        mpp_err_f("set input frame to task ret %d\n", ret);
        /* keep current task for next */
        mpp->mInputTask = task_dequeue;
        goto RET;
    }

    mpp_ops_dec_put_pkt(mpp->mDump, packet);

    /* enqueue valid task to decoder */
    ret = mpp_enqueue(mpp, MPP_PORT_INPUT, task_dequeue);
    if (ret) {
        mpp_err_f("enqueue ret %d\n", ret);
        goto RET;
    }

    mpp->mPacketPutCount++;

    if (timeout && !pkt_copy)
        mpp_poll(mpp, MPP_PORT_INPUT, timeout);

RET:
    /* wait enqueued task finished */
    if (NULL == mpp->mInputTask) {
        MPP_RET cnt = mpp_poll(mpp, MPP_PORT_INPUT, MPP_POLL_NON_BLOCK);
        /* reserve one task for eos block mode */
        if (cnt >= 0) {
            mpp_dequeue(mpp, MPP_PORT_INPUT, &mpp->mInputTask);
            mpp_assert(mpp->mInputTask);
        }
    }

    return ret;
}

MPP_RET mpp_get_frame(Mpp *mpp, MppFrame *frame)
{
    MppFrame frm = NULL;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_mutex_cond_lock(&mpp->mFrmOut->cond_lock);

    if (0 == mpp_list_size(mpp->mFrmOut)) {
        if (mpp->mOutputTimeout) {
            if (mpp->mOutputTimeout < 0) {
                /* block wait */
                mpp_list_wait(mpp->mFrmOut);
            } else {
                RK_S32 ret = mpp_list_wait_timed(mpp->mFrmOut, mpp->mOutputTimeout);
                if (ret) {
                    if (ret == ETIMEDOUT) {
                        mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);
                        return MPP_ERR_TIMEOUT;
                    } else {
                        mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);
                        return MPP_NOK;
                    }
                }
            }
        }
    }

    if (mpp_list_size(mpp->mFrmOut)) {
        MppBuffer buffer;

        mpp_list_del_at_head(mpp->mFrmOut, &frm, sizeof(frm));
        mpp->mFrameGetCount++;
        mpp_notify_flag(mpp, MPP_OUTPUT_DEQUEUE);

        buffer = mpp_frame_get_buffer(frm);
        if (buffer)
            mpp_buffer_sync_ro_begin(buffer);
    } else {
        // NOTE: Add signal here is not efficient
        // This is for fix bug of stucking on decoder parser thread
        // When decoder parser thread is block by info change and enter waiting.
        // There is no way to wake up parser thread to continue decoding.
        // The put_packet only signal sem on may be it better to use sem on info
        // change too.
        mpp_mutex_cond_lock(&mpp->mPktIn->cond_lock);
        if (mpp_list_size(mpp->mPktIn))
            mpp_notify_flag(mpp, MPP_INPUT_ENQUEUE);
        mpp_mutex_cond_unlock(&mpp->mPktIn->cond_lock);
    }

    *frame = frm;

    // dump output
    mpp_ops_dec_get_frm(mpp->mDump, frm);
    mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);

    return MPP_OK;
}

MPP_RET mpp_get_frame_noblock(Mpp *mpp, MppFrame *frame)
{
    MppFrame first = NULL;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_mutex_cond_lock(&mpp->mFrmOut->cond_lock);
    if (mpp_list_size(mpp->mFrmOut)) {
        mpp_list_del_at_head(mpp->mFrmOut, &first, sizeof(first));
        mpp_buffer_sync_ro_begin(mpp_frame_get_buffer(first));
        mpp->mFrameGetCount++;
    }
    mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);
    *frame = first;

    return MPP_OK;
}

MPP_RET mpp_decode(Mpp *mpp, MppPacket packet, MppFrame *frame)
{
    RK_U32 pkt_done = 0;
    RK_S32 frm_rdy = 0;
    MPP_RET ret = MPP_OK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mDec)
        return MPP_NOK;

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    /*
     * If there is frame to return get the frame first
     * But if the output mode is block then we need to send packet first
     */
    if (!mpp->mOutputTimeout) {
        mpp_mutex_cond_lock(&mpp->mFrmOut->cond_lock);
        if (mpp_list_size(mpp->mFrmOut)) {
            MppBuffer buffer;

            mpp_list_del_at_head(mpp->mFrmOut, frame, sizeof(*frame));
            buffer = mpp_frame_get_buffer(*frame);
            if (buffer)
                mpp_buffer_sync_ro_begin(buffer);
            mpp->mFrameGetCount++;
            mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);
            return MPP_OK;
        }
        mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);
    }

    do {
        if (!pkt_done)
            ret = mpp_dec_decode(mpp->mDec, packet);

        /* check input packet finished or not */
        if (!packet || !mpp_packet_get_length(packet))
            pkt_done = 1;

        /* always try getting frame */
        mpp_mutex_cond_lock(&mpp->mFrmOut->cond_lock);
        if (mpp_list_size(mpp->mFrmOut)) {
            MppBuffer buffer;

            mpp_list_del_at_head(mpp->mFrmOut, frame, sizeof(*frame));
            buffer = mpp_frame_get_buffer(*frame);
            if (buffer)
                mpp_buffer_sync_ro_begin(buffer);
            mpp->mFrameGetCount++;
            frm_rdy = 1;
        }
        mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);

        /* return on flow error */
        if (ret < 0)
            break;

        /* return on output frame is ready */
        if (frm_rdy) {
            mpp_assert(ret > 0);
            ret = MPP_OK;
            break;
        }

        /* return when packet is send and it is a non-block call */
        if (pkt_done) {
            ret = MPP_OK;
            break;
        }

        /* otherwise continue decoding and getting frame */
    } while (1);

    return ret;
}

MPP_RET mpp_put_frame(Mpp *mpp, MppFrame frame)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_dbg_pts("%p input frame pts %lld\n", mpp, mpp_frame_get_pts(frame));

    if (mpp->mKmpp && mpp->mKmpp->mApi && mpp->mKmpp->mApi->put_frame)
        return mpp->mKmpp->mApi->put_frame(mpp->mKmpp, frame);

    if (mpp->mInputTimeout == MPP_POLL_NON_BLOCK) {
        mpp_set_io_mode(mpp, MPP_IO_MODE_NORMAL);
        return mpp_put_frame_async(mpp, frame);
    }

    MPP_RET ret = MPP_NOK;
    MppStopwatch stopwatch = NULL;

    if (mpp_debug & MPP_DBG_TIMING) {
        mpp_frame_set_stopwatch_enable(frame, 1);
        stopwatch = mpp_frame_get_stopwatch(frame);
    }

    mpp_stopwatch_record(stopwatch, NULL);
    mpp_stopwatch_record(stopwatch, "put frame start");

    if (mpp->mInputTask == NULL) {
        mpp_stopwatch_record(stopwatch, "input port user poll");
        /* poll input port for valid task */
        ret = mpp_poll(mpp, MPP_PORT_INPUT, mpp->mInputTimeout);
        if (ret < 0) {
            if (mpp->mInputTimeout)
                mpp_log_f("poll on set timeout %d ret %d\n", mpp->mInputTimeout, ret);
            goto RET;
        }

        /* dequeue task for setup */
        mpp_stopwatch_record(stopwatch, "input port user dequeue");
        ret = mpp_dequeue(mpp, MPP_PORT_INPUT, &mpp->mInputTask);
        if (ret || NULL == mpp->mInputTask) {
            mpp_log_f("dequeue on set ret %d task %p\n", ret, mpp->mInputTask);
            goto RET;
        }
    }

    mpp_assert(mpp->mInputTask);

    /* setup task */
    ret = mpp_task_meta_set_frame(mpp->mInputTask, KEY_INPUT_FRAME, frame);
    if (ret) {
        mpp_log_f("set input frame to task ret %d\n", ret);
        goto RET;
    }

    if (mpp_frame_has_meta(frame)) {
        MppMeta meta = mpp_frame_get_meta(frame);
        MppPacket packet = NULL;
        MppBuffer md_info_buf = NULL;

        mpp_meta_get_packet(meta, KEY_OUTPUT_PACKET, &packet);
        if (packet) {
            ret = mpp_task_meta_set_packet(mpp->mInputTask, KEY_OUTPUT_PACKET, packet);
            if (ret) {
                mpp_log_f("set output packet to task ret %d\n", ret);
                goto RET;
            }
        }

        mpp_meta_get_buffer(meta, KEY_MOTION_INFO, &md_info_buf);
        if (md_info_buf) {
            ret = mpp_task_meta_set_buffer(mpp->mInputTask, KEY_MOTION_INFO, md_info_buf);
            if (ret) {
                mpp_log_f("set output motion dection info ret %d\n", ret);
                goto RET;
            }
        }
    }

    // dump input
    mpp_ops_enc_put_frm(mpp->mDump, frame);

    /* enqueue valid task to encoder */
    mpp_stopwatch_record(stopwatch, "input port user enqueue");
    ret = mpp_enqueue(mpp, MPP_PORT_INPUT, mpp->mInputTask);
    if (ret) {
        mpp_log_f("enqueue ret %d\n", ret);
        goto RET;
    }

    mpp->mInputTask = NULL;
    /* wait enqueued task finished */
    mpp_stopwatch_record(stopwatch, "input port user poll");
    ret = mpp_poll(mpp, MPP_PORT_INPUT, mpp->mInputTimeout);
    if (ret < 0) {
        if (mpp->mInputTimeout)
            mpp_log_f("poll on get timeout %d ret %d\n", mpp->mInputTimeout, ret);
        goto RET;
    }

    /* get previous enqueued task back */
    mpp_stopwatch_record(stopwatch, "input port user dequeue");
    ret = mpp_dequeue(mpp, MPP_PORT_INPUT, &mpp->mInputTask);
    if (ret) {
        mpp_log_f("dequeue on get ret %d\n", ret);
        goto RET;
    }

    mpp_assert(mpp->mInputTask);
    if (mpp->mInputTask) {
        MppFrame frm_out = NULL;

        mpp_task_meta_get_frame(mpp->mInputTask, KEY_INPUT_FRAME, &frm_out);
        mpp_assert(frm_out == frame);
    }

RET:
    mpp_stopwatch_record(stopwatch, "put_frame finish");
    mpp_frame_set_stopwatch_enable(frame, 0);
    return ret;
}

MPP_RET mpp_get_packet(Mpp *mpp, MppPacket *packet)
{
    MppPacket pkt = NULL;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    if (mpp->mKmpp && mpp->mKmpp->mApi && mpp->mKmpp->mApi->get_packet)
        return mpp->mKmpp->mApi->get_packet(mpp->mKmpp, packet);

    if (mpp->mInputTimeout == MPP_POLL_NON_BLOCK) {
        mpp_set_io_mode(mpp, MPP_IO_MODE_NORMAL);
        return mpp_get_packet_async(mpp, packet);
    }

    MPP_RET ret = MPP_OK;
    MppTask task = NULL;

    ret = mpp_poll(mpp, MPP_PORT_OUTPUT, mpp->mOutputTimeout);
    if (ret < 0) {
        // NOTE: Do not treat poll failure as error. Just clear output
        ret = MPP_OK;
        *packet = NULL;
        goto RET;
    }

    ret = mpp_dequeue(mpp, MPP_PORT_OUTPUT, &task);
    if (ret || NULL == task) {
        mpp_log_f("dequeue on get ret %d task %p\n", ret, task);
        goto RET;
    }

    mpp_assert(task);

    ret = mpp_task_meta_get_packet(task, KEY_OUTPUT_PACKET, packet);
    if (ret) {
        mpp_log_f("get output packet from task ret %d\n", ret);
        goto RET;
    }

    pkt = *packet;
    if (!pkt) {
        mpp_log_f("get invalid task without output packet\n");
    } else {
        MppPacketImpl *impl = (MppPacketImpl *)pkt;
        MppBuffer buf = impl->buffer;

        if (buf) {
            RK_U32 offset = (RK_U32)((char *)impl->pos - (char *)impl->data);

            mpp_buffer_sync_ro_partial_begin(buf, offset, impl->length);
        }

        mpp_dbg_pts("%p output packet pts %lld\n", mpp, impl->pts);
    }

    // dump output
    mpp_ops_enc_get_pkt(mpp->mDump, pkt);

    ret = mpp_enqueue(mpp, MPP_PORT_OUTPUT, task);
    if (ret)
        mpp_log_f("enqueue on set ret %d\n", ret);
RET:

    return ret;
}

MPP_RET mpp_put_frame_async(Mpp *mpp, MppFrame frame)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (NULL == mpp->mFrmIn)
        return MPP_NOK;

    if (mpp_mutex_cond_trylock(&mpp->mFrmIn->cond_lock))
        return MPP_NOK;

    /* NOTE: the max input queue length is 2 */
    if (mpp_list_wait_le(mpp->mFrmIn, 10, 1)) {
        mpp_mutex_cond_unlock(&mpp->mFrmIn->cond_lock);
        return MPP_NOK;
    }

    mpp_list_add_at_tail(mpp->mFrmIn, &frame, sizeof(frame));
    mpp->mFramePutCount++;

    mpp_notify_flag(mpp, MPP_INPUT_ENQUEUE);
    mpp_mutex_cond_unlock(&mpp->mFrmIn->cond_lock);

    return MPP_OK;
}

MPP_RET mpp_get_packet_async(Mpp *mpp, MppPacket *packet)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_mutex_cond_lock(&mpp->mPktOut->cond_lock);
    *packet = NULL;
    if (0 == mpp_list_size(mpp->mPktOut)) {
        if (mpp->mOutputTimeout) {
            if (mpp->mOutputTimeout < 0) {
                /* block wait */
                mpp_list_wait(mpp->mPktOut);
            } else {
                RK_S32 ret = mpp_list_wait_timed(mpp->mPktOut, mpp->mOutputTimeout);

                if (ret) {
                    if (ret == ETIMEDOUT) {
                        mpp_mutex_cond_unlock(&mpp->mPktOut->cond_lock);
                        return MPP_ERR_TIMEOUT;
                    } else {
                        mpp_mutex_cond_unlock(&mpp->mPktOut->cond_lock);
                        return MPP_NOK;
                    }
                }
            }
        } else {
            /* NOTE: in non-block mode the sleep is to avoid user's dead loop */
            msleep(1);
        }
    }

    if (mpp_list_size(mpp->mPktOut)) {
        MppPacket pkt = NULL;
        MppPacketImpl *impl = NULL;
        RK_U32 offset;

        mpp_list_del_at_head(mpp->mPktOut, &pkt, sizeof(pkt));
        mpp->mPacketGetCount++;
        mpp_notify_flag(mpp, MPP_OUTPUT_DEQUEUE);

        *packet = pkt;

        impl = (MppPacketImpl *)pkt;
        if (impl->buffer) {
            offset = (RK_U32)((char *)impl->pos - (char *)impl->data);
            mpp_buffer_sync_ro_partial_begin(impl->buffer, offset, impl->length);
        }
    } else {
        mpp_mutex_cond_lock(&mpp->mFrmIn->cond_lock);
        if (mpp_list_size(mpp->mFrmIn))
            mpp_notify_flag(mpp, MPP_INPUT_ENQUEUE);
        mpp_mutex_cond_unlock(&mpp->mFrmIn->cond_lock);
        mpp_mutex_cond_unlock(&mpp->mPktOut->cond_lock);

        return MPP_NOK;
    }
    mpp_mutex_cond_unlock(&mpp->mPktOut->cond_lock);

    return MPP_OK;
}

MPP_RET mpp_poll(Mpp *mpp, MppPortType type, MppPollType timeout)
{
    MppTaskQueue port = NULL;
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_set_io_mode(mpp, MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mpp->mUsrInPort;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mpp->mUsrOutPort;
    } break;
    default : {
    } break;
    }

    if (port)
        ret = mpp_port_poll(port, timeout);

    return ret;
}

MPP_RET mpp_dequeue(Mpp *mpp, MppPortType type, MppTask *task)
{
    MppTaskQueue port = NULL;
    RK_U32 notify_flag = 0;
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_set_io_mode(mpp, MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mpp->mUsrInPort;
        notify_flag = MPP_INPUT_DEQUEUE;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mpp->mUsrOutPort;
        notify_flag = MPP_OUTPUT_DEQUEUE;
    } break;
    default : {
    } break;
    }

    if (port) {
        ret = mpp_port_dequeue(port, task);
        if (MPP_OK == ret)
            mpp_notify_flag(mpp, notify_flag);
    }

    return ret;
}

MPP_RET mpp_enqueue(Mpp *mpp, MppPortType type, MppTask task)
{
    MppTaskQueue port = NULL;
    RK_U32 notify_flag = 0;
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    mpp_set_io_mode(mpp, MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mpp->mUsrInPort;
        notify_flag = MPP_INPUT_ENQUEUE;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mpp->mUsrOutPort;
        notify_flag = MPP_OUTPUT_ENQUEUE;
    } break;
    default : {
    } break;
    }

    if (port) {
        ret = mpp_port_enqueue(port, task);
        // if enqueue success wait up thread
        if (MPP_OK == ret)
            mpp_notify_flag(mpp, notify_flag);
    }

    return ret;
}

void mpp_set_io_mode(Mpp *mpp, MppIoMode mode)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return;
    }

    mpp_assert(mode == MPP_IO_MODE_NORMAL || mode == MPP_IO_MODE_TASK);

    if (mpp->mIoMode == MPP_IO_MODE_DEFAULT)
        mpp->mIoMode = mode;
    else if (mpp->mIoMode != mode) {
        static const char *iomode_2str[] = {
            "normal",
            "task queue",
        };

        mpp_assert(mpp->mIoMode < MPP_IO_MODE_BUTT);
        mpp_assert(mode < MPP_IO_MODE_BUTT);
        mpp_err("can not reset io mode from %s to %s\n",
                iomode_2str[!!mpp->mIoMode], iomode_2str[!!mode]);
    }
}

MPP_RET mpp_control(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_ops_ctrl(mpp->mDump, cmd);

    if (mpp->mKmpp && mpp->mKmpp->mApi && mpp->mKmpp->mApi->control)
        return mpp->mKmpp->mApi->control(mpp->mKmpp, cmd, param);

    switch (cmd & CMD_MODULE_ID_MASK) {
    case CMD_MODULE_OSAL : {
        ret = mpp_control_osal(mpp, cmd, param);
    } break;
    case CMD_MODULE_MPP : {
        mpp_assert(cmd > MPP_CMD_BASE);
        mpp_assert(cmd < MPP_CMD_END);

        ret = mpp_control_mpp(mpp, cmd, param);
    } break;
    case CMD_MODULE_CODEC : {
        switch (cmd & CMD_CTX_ID_MASK) {
        case CMD_CTX_ID_DEC : {
            mpp_assert(mpp->mType == MPP_CTX_DEC || mpp->mType == MPP_CTX_BUTT);
            mpp_assert(cmd > MPP_DEC_CMD_BASE);
            mpp_assert(cmd < MPP_DEC_CMD_END);

            ret = mpp_control_dec(mpp, cmd, param);
        } break;
        case CMD_CTX_ID_ENC : {
            mpp_assert(mpp->mType == MPP_CTX_ENC);
            mpp_assert(cmd > MPP_ENC_CMD_BASE);
            mpp_assert(cmd < MPP_ENC_CMD_END);

            ret = mpp_control_enc(mpp, cmd, param);
        } break;
        case CMD_CTX_ID_ISP : {
            mpp_assert(mpp->mType == MPP_CTX_ISP);
            ret = mpp_control_isp(mpp, cmd, param);
        } break;
        default : {
            mpp_assert(cmd > MPP_CODEC_CMD_BASE);
            mpp_assert(cmd < MPP_CODEC_CMD_END);

            ret = mpp_control_codec(mpp, cmd, param);
        } break;
        }
    } break;
    default : {
    } break;
    }

    if (ret)
        mpp_err("command %x param %p ret %d\n", cmd, param, ret);

    return ret;
}

MPP_RET mpp_reset(Mpp *mpp)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    if (!mpp->mInitDone)
        return MPP_ERR_INIT;

    if (mpp->mKmpp && mpp->mKmpp->mApi && mpp->mKmpp->mApi->reset)
        return mpp->mKmpp->mApi->reset(mpp->mKmpp);

    mpp_ops_reset(mpp->mDump);

    if (mpp->mType == MPP_CTX_DEC) {
        /*
         * On mp4 case extra data of sps/pps will be put at the beginning
         * If these packet was reset before they are send to decoder then
         * decoder can not get these important information to continue decoding
         * To avoid this case happen we need to save it on reset beginning
         * then restore it on reset end.
         */
        mpp_mutex_cond_lock(&mpp->mPktIn->cond_lock);
        while (mpp_list_size(mpp->mPktIn)) {
            MppPacket pkt = NULL;

            mpp_list_del_at_head(mpp->mPktIn, &pkt, sizeof(pkt));
            mpp->mPacketGetCount++;

            RK_U32 flags = mpp_packet_get_flag(pkt);
            if (flags & MPP_PACKET_FLAG_EXTRA_DATA) {
                if (mpp->mExtraPacket) {
                    mpp_packet_deinit(&mpp->mExtraPacket);
                }
                mpp->mExtraPacket = pkt;
            } else {
                mpp_packet_deinit(&pkt);
            }
        }
        mpp_list_flush(mpp->mPktIn);
        mpp_mutex_cond_unlock(&mpp->mPktIn->cond_lock);

        mpp_dec_reset(mpp->mDec);

        mpp_mutex_cond_lock(&mpp->mFrmOut->cond_lock);
        mpp_list_flush(mpp->mFrmOut);
        mpp_mutex_cond_unlock(&mpp->mFrmOut->cond_lock);

        mpp_port_awake(mpp->mUsrInPort);
        mpp_port_awake(mpp->mUsrOutPort);
    } else {
        mpp_enc_reset_v2(mpp->mEnc);
    }

    return MPP_OK;
}

MPP_RET mpp_control_mpp(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_OK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case MPP_SET_INPUT_BLOCK :
    case MPP_SET_OUTPUT_BLOCK :
    case MPP_SET_INTPUT_BLOCK_TIMEOUT :
    case MPP_SET_OUTPUT_BLOCK_TIMEOUT : {
        MppPollType block = (param) ? *((MppPollType *)param) : MPP_POLL_NON_BLOCK;

        if (block <= MPP_POLL_BUTT || block > MPP_POLL_MAX) {
            mpp_err("invalid output timeout type %d should be in range [%d, %d]\n",
                    block, MPP_POLL_BUTT, MPP_POLL_MAX);
            ret = MPP_ERR_VALUE;
            break;
        }
        if (cmd == MPP_SET_INPUT_BLOCK || cmd == MPP_SET_INTPUT_BLOCK_TIMEOUT)
            mpp->mInputTimeout = block;
        else
            mpp->mOutputTimeout = block;

        mpp_log("deprecated block control, use timeout control instead\n");
    } break;

    case MPP_SET_DISABLE_THREAD: {
        mpp->mDisableThread = 1;
    } break;

    case MPP_SET_INPUT_TIMEOUT:
    case MPP_SET_OUTPUT_TIMEOUT: {
        MppPollType timeout = (param) ? *((MppPollType *)param) : MPP_POLL_NON_BLOCK;

        if (timeout <= MPP_POLL_BUTT || timeout > MPP_POLL_MAX) {
            mpp_err("invalid output timeout type %d should be in range [%d, %d]\n",
                    timeout, MPP_POLL_BUTT, MPP_POLL_MAX);
            ret = MPP_ERR_VALUE;
            break;
        }

        if (cmd == MPP_SET_INPUT_TIMEOUT)
            mpp->mInputTimeout = timeout;
        else
            mpp->mOutputTimeout = timeout;
    } break;
    case MPP_SET_VENC_INIT_KCFG: {
        KmppObj obj = param;

        if (!obj) {
            mpp_err_f("ctrl %d invalid param %p\n", cmd, param);
            return MPP_ERR_VALUE;
        }
        mpp->mVencInitKcfg = obj;
    } break;
    case MPP_START : {
        mpp_start(mpp);
    } break;
    case MPP_STOP : {
        mpp_stop(mpp);
    } break;

    case MPP_PAUSE : {
        mpp_pause(mpp);
    } break;
    case MPP_RESUME : {
        mpp_resume(mpp);
    } break;

    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

MPP_RET mpp_control_osal(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_assert(cmd > MPP_OSAL_CMD_BASE);
    mpp_assert(cmd < MPP_OSAL_CMD_END);

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET mpp_control_codec(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET mpp_control_dec(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd) {
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO:
    case MPP_DEC_SET_FRAME_INFO: {
        ret = mpp_dec_control(mpp->mDec, cmd, param);
    } break;
    case MPP_DEC_SET_EXT_BUF_GROUP: {
        /*
         * NOTE: If frame buffer group is configured before decoder init
         * then the buffer limitation maybe not be correctly setup
         * without infomation from InfoChange frame.
         * And the thread signal connection may not be setup here. It
         * may have a bad effect on MPP efficiency.
         */
        if (!mpp->mInitDone) {
            mpp_err("WARNING: setup buffer group before decoder init\n");
            break;
        }

        ret = MPP_OK;
        if (!param) {
            /* set to internal mode */
            if (mpp->mExternalBufferMode) {
                /* switch from external mode to internal mode */
                mpp_assert(mpp->mFrameGroup);
                mpp_buffer_group_set_callback((MppBufferGroupImpl *)mpp->mFrameGroup,
                                              NULL, NULL);
                mpp->mFrameGroup = NULL;
            } else {
                /* keep internal buffer mode cleanup old buffers */
                if (mpp->mFrameGroup)
                    mpp_buffer_group_clear(mpp->mFrameGroup);
            }

            mpp_dbg_info("using internal buffer group %p\n", mpp->mFrameGroup);
            mpp->mExternalBufferMode = 0;
        } else {
            /* set to external mode */
            if (mpp->mExternalBufferMode) {
                /* keep external buffer mode */
                if (mpp->mFrameGroup != param) {
                    /* switch to new buffer group */
                    mpp_assert(mpp->mFrameGroup);
                    mpp_buffer_group_set_callback((MppBufferGroupImpl *)mpp->mFrameGroup,
                                                  NULL, NULL);
                } else {
                    /* keep old group the external group user should cleanup its old buffers */
                }
            } else {
                /* switch from intenal mode to external mode */
                if (mpp->mFrameGroup)
                    mpp_buffer_group_put(mpp->mFrameGroup);
            }

            mpp_dbg_info("using external buffer group %p\n", mpp->mFrameGroup);

            mpp->mFrameGroup = (MppBufferGroup)param;
            mpp_buffer_group_set_callback((MppBufferGroupImpl *)mpp->mFrameGroup,
                                          mpp_notify_by_buffer_group, (void *)mpp);
            mpp->mExternalBufferMode = 1;
            mpp_notify_flag(mpp, MPP_DEC_NOTIFY_EXT_BUF_GRP_READY);
        }
    } break;
    case MPP_DEC_SET_INFO_CHANGE_READY: {
        mpp_dbg_info("set info change ready\n");

        ret = mpp_dec_control(mpp->mDec, cmd, param);
        mpp_notify_flag(mpp, MPP_DEC_NOTIFY_INFO_CHG_DONE | MPP_DEC_NOTIFY_BUFFER_MATCH);
    } break;
    case MPP_DEC_SET_PRESENT_TIME_ORDER :
    case MPP_DEC_SET_PARSER_SPLIT_MODE :
    case MPP_DEC_SET_PARSER_FAST_MODE :
    case MPP_DEC_SET_IMMEDIATE_OUT :
    case MPP_DEC_SET_DISABLE_ERROR :
    case MPP_DEC_SET_DIS_ERR_CLR_MARK :
    case MPP_DEC_SET_ENABLE_DEINTERLACE :
    case MPP_DEC_SET_ENABLE_FAST_PLAY :
    case MPP_DEC_SET_ENABLE_MVC :
    case MPP_DEC_SET_DISABLE_DPB_CHECK :
    case MPP_DEC_SET_CODEC_MODE : {
        /*
         * These control may be set before mpp_init
         * When this case happen record the config and wait for decoder init
         */
        if (mpp->mDec) {
            ret = mpp_dec_control(mpp->mDec, cmd, param);
            return ret;
        }

        ret = mpp_dec_set_cfg_by_cmd(mpp->mDecCfg, cmd, param);
    } break;
    case MPP_DEC_GET_STREAM_COUNT: {
        mpp_mutex_cond_lock(&mpp->mPktIn->cond_lock);
        *((RK_S32 *)param) = mpp_list_size(mpp->mPktIn);
        ret = MPP_OK;
        mpp_mutex_cond_unlock(&mpp->mPktIn->cond_lock);
    } break;
    case MPP_DEC_GET_VPUMEM_USED_COUNT :
    case MPP_DEC_SET_OUTPUT_FORMAT :
    case MPP_DEC_QUERY :
    case MPP_DEC_SET_MAX_USE_BUFFER_SIZE: {
        ret = mpp_dec_control(mpp->mDec, cmd, param);
    } break;
    case MPP_DEC_SET_CFG : {
        if (mpp->mDec)
            ret = mpp_dec_control(mpp->mDec, cmd, param);
        else if (param) {
            ret = (MPP_RET)kmpp_obj_update(mpp->mDecCfg, param);
        }
    } break;
    case MPP_DEC_GET_CFG : {
        if (mpp->mDec)
            ret = mpp_dec_control(mpp->mDec, cmd, param);
        else if (param) {
            ret = (MPP_RET)kmpp_obj_copy_entry(param, mpp->mDecCfg);
        }
    } break;
    default : {
    } break;
    }
    return ret;
}

MPP_RET mpp_control_enc(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_assert(mpp->mEnc);
    return mpp_enc_control_v2(mpp->mEnc, cmd, param);
}

MPP_RET mpp_control_isp(Mpp *mpp, MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_assert(cmd > MPP_ISP_CMD_BASE);
    mpp_assert(cmd < MPP_ISP_CMD_END);

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET mpp_notify_flag(Mpp *mpp, RK_U32 flag)
{
    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (mpp->mType) {
    case MPP_CTX_DEC : {
        return mpp_dec_notify(mpp->mDec, flag);
    } break;
    case MPP_CTX_ENC : {
        return mpp_enc_notify_v2(mpp->mEnc, flag);
    } break;
    default : {
        mpp_err("unsupport context type %d\n", mpp->mType);
    } break;
    }
    return MPP_NOK;
}

MPP_RET mpp_notify_group(Mpp *mpp, MppBufferGroup group)
{
    MPP_RET ret = MPP_NOK;

    if (!mpp) {
        mpp_err_f("invalid input mpp pointer\n");
        return MPP_ERR_NULL_PTR;
    }

    switch (mpp->mType) {
    case MPP_CTX_DEC : {
        if (group == mpp->mFrameGroup)
            ret = mpp_notify_flag(mpp, MPP_DEC_NOTIFY_BUFFER_VALID | MPP_DEC_NOTIFY_BUFFER_MATCH);
    } break;
    default : {
    } break;
    }

    return ret;
}
