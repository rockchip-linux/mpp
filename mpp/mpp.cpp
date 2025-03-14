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

#include "mpp_dec_cfg_impl.h"

#include "kmpp.h"

#define MPP_TEST_FRAME_SIZE     SZ_1M
#define MPP_TEST_PACKET_SIZE    SZ_512K

static void mpp_notify_by_buffer_group(void *arg, void *group)
{
    Mpp *mpp = (Mpp *)arg;

    mpp->notify((MppBufferGroup) group);
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

Mpp::Mpp(MppCtx ctx)
    : mPktIn(NULL),
      mPktOut(NULL),
      mFrmIn(NULL),
      mFrmOut(NULL),
      mPacketPutCount(0),
      mPacketGetCount(0),
      mFramePutCount(0),
      mFrameGetCount(0),
      mTaskPutCount(0),
      mTaskGetCount(0),
      mPacketGroup(NULL),
      mFrameGroup(NULL),
      mExternalBufferMode(0),
      mUsrInPort(NULL),
      mUsrOutPort(NULL),
      mMppInPort(NULL),
      mMppOutPort(NULL),
      mInputTaskQueue(NULL),
      mOutputTaskQueue(NULL),
      mInputTaskCount(1),
      mOutputTaskCount(1),
      mInputTimeout(MPP_POLL_BUTT),
      mOutputTimeout(MPP_POLL_BUTT),
      mInputTask(NULL),
      mEosTask(NULL),
      mCtx(ctx),
      mDec(NULL),
      mEnc(NULL),
      mEncAyncIo(0),
      mEncAyncProc(0),
      mIoMode(MPP_IO_MODE_DEFAULT),
      mDisableThread(0),
      mDump(NULL),
      mType(MPP_CTX_BUTT),
      mCoding(MPP_VIDEO_CodingUnused),
      mInitDone(0),
      mStatus(0),
      mExtraPacket(NULL)
{
    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    memset(&mDecInitcfg, 0, sizeof(mDecInitcfg));
    mpp_dec_cfg_set_default(&mDecInitcfg);
    mDecInitcfg.base.enable_vproc = MPP_VPROC_MODE_DEINTELACE;
    mDecInitcfg.base.change  |= MPP_DEC_CFG_CHANGE_ENABLE_VPROC;
    mKmpp = NULL;
    mVencInitKcfg = NULL;
    mpp_dump_init(&mDump);
}

MPP_RET Mpp::init(MppCtxType type, MppCodingType coding)
{
    MPP_RET ret = MPP_NOK;

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

    mpp_ops_init(mDump, type, coding);

    mType = type;
    mCoding = coding;

    /* init kmpp venc */
    if (mVencInitKcfg) {
        mKmpp = mpp_calloc(Kmpp, 1);
        if (!mKmpp) {
            mpp_err("failed to alloc kmpp context\n");
            return MPP_NOK;
        }
        mKmpp->mClientFd = -1;
        mpp_get_api(mKmpp);
        mKmpp->mVencInitKcfg = mVencInitKcfg;
        ret = mKmpp->mApi->init(mKmpp, type, coding);
        if (ret) {
            mpp_err("failed to init kmpp ret %d\n", ret);
            return ret;
        }
        mInitDone = 1;

        return ret;
    }

    mpp_task_queue_init(&mInputTaskQueue, this, "input");
    mpp_task_queue_init(&mOutputTaskQueue, this, "output");

    switch (mType) {
    case MPP_CTX_DEC : {
        mPktIn  = new mpp_list(list_wraper_packet);
        mFrmOut = new mpp_list(list_wraper_frame);

        if (mInputTimeout == MPP_POLL_BUTT)
            mInputTimeout = MPP_POLL_NON_BLOCK;

        if (mOutputTimeout == MPP_POLL_BUTT)
            mOutputTimeout = MPP_POLL_NON_BLOCK;

        if (mCoding != MPP_VIDEO_CodingMJPEG) {
            mpp_buffer_group_get_internal(&mPacketGroup, MPP_BUFFER_TYPE_ION | MPP_BUFFER_FLAGS_CACHABLE);
            mpp_buffer_group_limit_config(mPacketGroup, 0, 3);

            mInputTaskCount = 4;
            mOutputTaskCount = 4;
        }

        mpp_task_queue_setup(mInputTaskQueue, mInputTaskCount);
        mpp_task_queue_setup(mOutputTaskQueue, mOutputTaskCount);

        mUsrInPort  = mpp_task_queue_get_port(mInputTaskQueue,  MPP_PORT_INPUT);
        mUsrOutPort = mpp_task_queue_get_port(mOutputTaskQueue, MPP_PORT_OUTPUT);
        mMppInPort  = mpp_task_queue_get_port(mInputTaskQueue,  MPP_PORT_OUTPUT);
        mMppOutPort = mpp_task_queue_get_port(mOutputTaskQueue, MPP_PORT_INPUT);

        mDecInitcfg.base.disable_thread = mDisableThread;
        mDecInitcfg.base.change |= MPP_DEC_CFG_CHANGE_DISABLE_THREAD;

        MppDecInitCfg cfg = {
            coding,
            this,
            &mDecInitcfg,
        };

        ret = mpp_dec_init(&mDec, &cfg);
        if (ret)
            break;
        ret = mpp_dec_start(mDec);
        if (ret)
            break;
        mInitDone = 1;
    } break;
    case MPP_CTX_ENC : {
        mPktOut = new mpp_list(list_wraper_packet);
        mFrmIn  = new mpp_list(list_wraper_frame);

        if (mInputTimeout == MPP_POLL_BUTT)
            mInputTimeout = MPP_POLL_BLOCK;

        if (mOutputTimeout == MPP_POLL_BUTT)
            mOutputTimeout = MPP_POLL_NON_BLOCK;

        mpp_buffer_group_get_internal(&mPacketGroup, MPP_BUFFER_TYPE_ION);
        mpp_buffer_group_get_internal(&mFrameGroup, MPP_BUFFER_TYPE_ION);

        if (mInputTimeout == MPP_POLL_NON_BLOCK) {
            mEncAyncIo = 1;

            mInputTaskCount = check_frm_task_cnt_cap(coding);
            if (mInputTaskCount == 1)
                mInputTimeout = MPP_POLL_BLOCK;
        }
        mOutputTaskCount = 8;

        mpp_task_queue_setup(mInputTaskQueue, mInputTaskCount);
        mpp_task_queue_setup(mOutputTaskQueue, mOutputTaskCount);

        mUsrInPort  = mpp_task_queue_get_port(mInputTaskQueue,  MPP_PORT_INPUT);
        mUsrOutPort = mpp_task_queue_get_port(mOutputTaskQueue, MPP_PORT_OUTPUT);
        mMppInPort  = mpp_task_queue_get_port(mInputTaskQueue,  MPP_PORT_OUTPUT);
        mMppOutPort = mpp_task_queue_get_port(mOutputTaskQueue, MPP_PORT_INPUT);

        MppEncInitCfg cfg = {
            coding,
            mInputTaskCount,
            this,
        };

        ret = mpp_enc_init_v2(&mEnc, &cfg);
        if (ret)
            break;

        if (mInputTimeout == MPP_POLL_NON_BLOCK) {
            mEncAyncProc = 1;
            ret = mpp_enc_start_async(mEnc);
        } else {
            ret = mpp_enc_start_v2(mEnc);
        }

        if (ret)
            break;
        mInitDone = 1;
    } break;
    default : {
        mpp_err("Mpp error type %d\n", mType);
    } break;
    }


    if (!mInitDone) {
        mpp_err("error found on mpp initialization\n");
        clear();
    }

    return ret;
}

Mpp::~Mpp ()
{
    clear();
}

void Mpp::clear()
{
    // MUST: release listener here
    if (mFrameGroup)
        mpp_buffer_group_set_callback((MppBufferGroupImpl *)mFrameGroup,
                                      NULL, NULL);

    if (mType == MPP_CTX_DEC) {
        if (mDec) {
            mpp_dec_stop(mDec);
            mpp_dec_deinit(mDec);
            mDec = NULL;
        }
    } else {
        if (mEnc) {
            mpp_enc_stop_v2(mEnc);
            mpp_enc_deinit_v2(mEnc);
            mEnc = NULL;
        }
    }

    if (mInputTaskQueue) {
        mpp_task_queue_deinit(mInputTaskQueue);
        mInputTaskQueue = NULL;
    }
    if (mOutputTaskQueue) {
        mpp_task_queue_deinit(mOutputTaskQueue);
        mOutputTaskQueue = NULL;
    }

    mUsrInPort  = NULL;
    mUsrOutPort = NULL;
    mMppInPort  = NULL;
    mMppOutPort = NULL;

    if (mExtraPacket) {
        mpp_packet_deinit(&mExtraPacket);
        mExtraPacket = NULL;
    }

    if (mPktIn) {
        delete mPktIn;
        mPktIn = NULL;
    }
    if (mPktOut) {
        delete mPktOut;
        mPktOut = NULL;
    }
    if (mFrmIn) {
        delete mFrmIn;
        mFrmIn = NULL;
    }
    if (mFrmOut) {
        delete mFrmOut;
        mFrmOut = NULL;
    }

    if (mPacketGroup) {
        mpp_buffer_group_put(mPacketGroup);
        mPacketGroup = NULL;
    }

    if (mFrameGroup && !mExternalBufferMode) {
        mpp_buffer_group_put(mFrameGroup);
        mFrameGroup = NULL;
    }

    if (mKmpp) {
        if (mKmpp->mApi && mKmpp->mApi->clear)
            mKmpp->mApi->clear(mKmpp);

        MPP_FREE(mKmpp);
    }

    mpp_dump_deinit(&mDump);
}

MPP_RET Mpp::start()
{
    return MPP_OK;
}

MPP_RET Mpp::stop()
{
    return MPP_OK;
}

MPP_RET Mpp::pause()
{
    return MPP_OK;
}

MPP_RET Mpp::resume()
{
    return MPP_OK;
}

MPP_RET Mpp::put_packet(MppPacket packet)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    MPP_RET ret = MPP_NOK;
    MppPollType timeout = mInputTimeout;
    MppTask task_dequeue = NULL;
    RK_U32 pkt_copy = 0;

    if (mDisableThread) {
        mpp_err_f("no thread decoding case MUST use mpi_decode interface\n");
        return ret;
    }

    if (mExtraPacket) {
        MppPacket extra = mExtraPacket;

        mExtraPacket = NULL;
        put_packet(extra);
    }

    /* non-jpeg mode - reserve extra task for incoming eos packet */
    if (mInputTaskCount > 1) {
        if (!mEosTask) {
            /* handle eos packet on block mode */
            ret = poll(MPP_PORT_INPUT, MPP_POLL_BLOCK);
            if (ret < 0)
                goto RET;

            dequeue(MPP_PORT_INPUT, &mEosTask);
            if (NULL == mEosTask) {
                mpp_err_f("fail to reserve eos task\n", ret);
                ret = MPP_NOK;
                goto RET;
            }
        }

        if (mpp_packet_get_eos(packet)) {
            mpp_assert(mEosTask);
            task_dequeue = mEosTask;
            mEosTask = NULL;
        }
    }

    /* Use reserved task to send eos packet */
    if (mInputTask && !task_dequeue) {
        task_dequeue = mInputTask;
        mInputTask = NULL;
    }

    if (NULL == task_dequeue) {
        ret = poll(MPP_PORT_INPUT, timeout);
        if (ret < 0) {
            ret = MPP_ERR_BUFFER_FULL;
            goto RET;
        }

        /* do not pull here to avoid block wait */
        dequeue(MPP_PORT_INPUT, &task_dequeue);
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
        mInputTask = task_dequeue;
        goto RET;
    }

    mpp_ops_dec_put_pkt(mDump, packet);

    /* enqueue valid task to decoder */
    ret = enqueue(MPP_PORT_INPUT, task_dequeue);
    if (ret) {
        mpp_err_f("enqueue ret %d\n", ret);
        goto RET;
    }

    mPacketPutCount++;

    if (timeout && !pkt_copy)
        poll(MPP_PORT_INPUT, timeout);

RET:
    /* wait enqueued task finished */
    if (NULL == mInputTask) {
        MPP_RET cnt = poll(MPP_PORT_INPUT, MPP_POLL_NON_BLOCK);
        /* reserve one task for eos block mode */
        if (cnt >= 0) {
            dequeue(MPP_PORT_INPUT, &mInputTask);
            mpp_assert(mInputTask);
        }
    }

    return ret;
}

MPP_RET Mpp::get_frame(MppFrame *frame)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    AutoMutex autoFrameLock(mFrmOut->mutex());
    MppFrame frm = NULL;

    if (0 == mFrmOut->list_size()) {
        if (mOutputTimeout) {
            if (mOutputTimeout < 0) {
                /* block wait */
                mFrmOut->wait();
            } else {
                RK_S32 ret = mFrmOut->wait(mOutputTimeout);
                if (ret) {
                    if (ret == ETIMEDOUT)
                        return MPP_ERR_TIMEOUT;
                    else
                        return MPP_NOK;
                }
            }
        }
    }

    if (mFrmOut->list_size()) {
        MppBuffer buffer;

        mFrmOut->del_at_head(&frm, sizeof(frame));
        mFrameGetCount++;
        notify(MPP_OUTPUT_DEQUEUE);

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
        AutoMutex autoPacketLock(mPktIn->mutex());
        if (mPktIn->list_size())
            notify(MPP_INPUT_ENQUEUE);
    }

    *frame = frm;

    // dump output
    mpp_ops_dec_get_frm(mDump, frm);

    return MPP_OK;
}

MPP_RET Mpp::get_frame_noblock(MppFrame *frame)
{
    MppFrame first = NULL;

    if (!mInitDone)
        return MPP_ERR_INIT;

    mFrmOut->lock();
    if (mFrmOut->list_size()) {
        mFrmOut->del_at_head(&first, sizeof(frame));
        mpp_buffer_sync_ro_begin(mpp_frame_get_buffer(first));
        mFrameGetCount++;
    }
    mFrmOut->unlock();
    *frame = first;

    return MPP_OK;
}

MPP_RET Mpp::decode(MppPacket packet, MppFrame *frame)
{
    RK_U32 pkt_done = 0;
    RK_S32 frm_rdy = 0;
    MPP_RET ret = MPP_OK;

    if (!mDec)
        return MPP_NOK;

    if (!mInitDone)
        return MPP_ERR_INIT;

    /*
     * If there is frame to return get the frame first
     * But if the output mode is block then we need to send packet first
     */
    if (!mOutputTimeout) {
        AutoMutex autoFrameLock(mFrmOut->mutex());

        if (mFrmOut->list_size()) {
            MppBuffer buffer;

            mFrmOut->del_at_head(frame, sizeof(*frame));
            buffer = mpp_frame_get_buffer(*frame);
            if (buffer)
                mpp_buffer_sync_ro_begin(buffer);
            mFrameGetCount++;
            return MPP_OK;
        }
    }

    do {
        if (!pkt_done)
            ret = mpp_dec_decode(mDec, packet);

        /* check input packet finished or not */
        if (!packet || !mpp_packet_get_length(packet))
            pkt_done = 1;

        /* always try getting frame */
        {
            AutoMutex autoFrameLock(mFrmOut->mutex());

            if (mFrmOut->list_size()) {
                MppBuffer buffer;

                mFrmOut->del_at_head(frame, sizeof(*frame));
                buffer = mpp_frame_get_buffer(*frame);
                if (buffer)
                    mpp_buffer_sync_ro_begin(buffer);
                mFrameGetCount++;
                frm_rdy = 1;
            }
        }

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

MPP_RET Mpp::put_frame(MppFrame frame)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    mpp_dbg_pts("%p input frame pts %lld\n", this, mpp_frame_get_pts(frame));

    if (mKmpp && mKmpp->mApi && mKmpp->mApi->put_frame)
        return mKmpp->mApi->put_frame(mKmpp, frame);

    if (mInputTimeout == MPP_POLL_NON_BLOCK) {
        set_io_mode(MPP_IO_MODE_NORMAL);
        return put_frame_async(frame);
    }

    MPP_RET ret = MPP_NOK;
    MppStopwatch stopwatch = NULL;

    if (mpp_debug & MPP_DBG_TIMING) {
        mpp_frame_set_stopwatch_enable(frame, 1);
        stopwatch = mpp_frame_get_stopwatch(frame);
    }

    mpp_stopwatch_record(stopwatch, NULL);
    mpp_stopwatch_record(stopwatch, "put frame start");

    if (mInputTask == NULL) {
        mpp_stopwatch_record(stopwatch, "input port user poll");
        /* poll input port for valid task */
        ret = poll(MPP_PORT_INPUT, mInputTimeout);
        if (ret < 0) {
            if (mInputTimeout)
                mpp_log_f("poll on set timeout %d ret %d\n", mInputTimeout, ret);
            goto RET;
        }

        /* dequeue task for setup */
        mpp_stopwatch_record(stopwatch, "input port user dequeue");
        ret = dequeue(MPP_PORT_INPUT, &mInputTask);
        if (ret || NULL == mInputTask) {
            mpp_log_f("dequeue on set ret %d task %p\n", ret, mInputTask);
            goto RET;
        }
    }

    mpp_assert(mInputTask);

    /* setup task */
    ret = mpp_task_meta_set_frame(mInputTask, KEY_INPUT_FRAME, frame);
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
            ret = mpp_task_meta_set_packet(mInputTask, KEY_OUTPUT_PACKET, packet);
            if (ret) {
                mpp_log_f("set output packet to task ret %d\n", ret);
                goto RET;
            }
        }

        mpp_meta_get_buffer(meta, KEY_MOTION_INFO, &md_info_buf);
        if (md_info_buf) {
            ret = mpp_task_meta_set_buffer(mInputTask, KEY_MOTION_INFO, md_info_buf);
            if (ret) {
                mpp_log_f("set output motion dection info ret %d\n", ret);
                goto RET;
            }
        }
    }

    // dump input
    mpp_ops_enc_put_frm(mDump, frame);

    /* enqueue valid task to encoder */
    mpp_stopwatch_record(stopwatch, "input port user enqueue");
    ret = enqueue(MPP_PORT_INPUT, mInputTask);
    if (ret) {
        mpp_log_f("enqueue ret %d\n", ret);
        goto RET;
    }

    mInputTask = NULL;
    /* wait enqueued task finished */
    mpp_stopwatch_record(stopwatch, "input port user poll");
    ret = poll(MPP_PORT_INPUT, mInputTimeout);
    if (ret < 0) {
        if (mInputTimeout)
            mpp_log_f("poll on get timeout %d ret %d\n", mInputTimeout, ret);
        goto RET;
    }

    /* get previous enqueued task back */
    mpp_stopwatch_record(stopwatch, "input port user dequeue");
    ret = dequeue(MPP_PORT_INPUT, &mInputTask);
    if (ret) {
        mpp_log_f("dequeue on get ret %d\n", ret);
        goto RET;
    }

    mpp_assert(mInputTask);
    if (mInputTask) {
        MppFrame frm_out = NULL;

        mpp_task_meta_get_frame(mInputTask, KEY_INPUT_FRAME, &frm_out);
        mpp_assert(frm_out == frame);
    }

RET:
    mpp_stopwatch_record(stopwatch, "put_frame finish");
    mpp_frame_set_stopwatch_enable(frame, 0);
    return ret;
}

MPP_RET Mpp::get_packet(MppPacket *packet)
{
    MppPacket pkt = NULL;

    if (!mInitDone)
        return MPP_ERR_INIT;

    if (mKmpp && mKmpp->mApi && mKmpp->mApi->get_packet)
        return mKmpp->mApi->get_packet(mKmpp, packet);

    if (mInputTimeout == MPP_POLL_NON_BLOCK) {
        set_io_mode(MPP_IO_MODE_NORMAL);
        return get_packet_async(packet);
    }

    MPP_RET ret = MPP_OK;
    MppTask task = NULL;

    ret = poll(MPP_PORT_OUTPUT, mOutputTimeout);
    if (ret < 0) {
        // NOTE: Do not treat poll failure as error. Just clear output
        ret = MPP_OK;
        *packet = NULL;
        goto RET;
    }

    ret = dequeue(MPP_PORT_OUTPUT, &task);
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
        RK_U32 offset = (RK_U32)((char *)impl->pos - (char *)impl->data);

        mpp_buffer_sync_ro_partial_begin(buf, offset, impl->length);

        mpp_dbg_pts("%p output packet pts %lld\n", this, impl->pts);
    }

    // dump output
    mpp_ops_enc_get_pkt(mDump, pkt);

    ret = enqueue(MPP_PORT_OUTPUT, task);
    if (ret)
        mpp_log_f("enqueue on set ret %d\n", ret);
RET:

    return ret;
}

MPP_RET Mpp::put_frame_async(MppFrame frame)
{
    if (NULL == mFrmIn)
        return MPP_NOK;

    if (mFrmIn->trylock())
        return MPP_NOK;

    /* NOTE: the max input queue length is 2 */
    if (mFrmIn->wait_le(10, 1)) {
        mFrmIn->unlock();
        return MPP_NOK;
    }

    mFrmIn->add_at_tail(&frame, sizeof(frame));
    mFramePutCount++;

    notify(MPP_INPUT_ENQUEUE);
    mFrmIn->unlock();

    return MPP_OK;
}

MPP_RET Mpp::get_packet_async(MppPacket *packet)
{
    AutoMutex autoPacketLock(mPktOut->mutex());

    *packet = NULL;
    if (0 == mPktOut->list_size()) {
        if (mOutputTimeout) {
            if (mOutputTimeout < 0) {
                /* block wait */
                mPktOut->wait();
            } else {
                RK_S32 ret = mPktOut->wait(mOutputTimeout);
                if (ret) {
                    if (ret == ETIMEDOUT)
                        return MPP_ERR_TIMEOUT;
                    else
                        return MPP_NOK;
                }
            }
        } else {
            /* NOTE: in non-block mode the sleep is to avoid user's dead loop */
            msleep(1);
        }
    }

    if (mPktOut->list_size()) {
        MppPacket pkt = NULL;
        MppPacketImpl *impl = NULL;
        RK_U32 offset;

        mPktOut->del_at_head(&pkt, sizeof(pkt));
        mPacketGetCount++;
        notify(MPP_OUTPUT_DEQUEUE);

        *packet = pkt;

        impl = (MppPacketImpl *)pkt;
        offset = (RK_U32)((char *)impl->pos - (char *)impl->data);
        mpp_buffer_sync_ro_partial_begin(impl->buffer, offset, impl->length);
    } else {
        AutoMutex autoFrameLock(mFrmIn->mutex());

        if (mFrmIn->list_size())
            notify(MPP_INPUT_ENQUEUE);

        return MPP_NOK;
    }

    return MPP_OK;
}

MPP_RET Mpp::poll(MppPortType type, MppPollType timeout)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    MPP_RET ret = MPP_NOK;
    MppTaskQueue port = NULL;

    set_io_mode(MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mUsrInPort;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mUsrOutPort;
    } break;
    default : {
    } break;
    }

    if (port)
        ret = mpp_port_poll(port, timeout);

    return ret;
}

MPP_RET Mpp::dequeue(MppPortType type, MppTask *task)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    MPP_RET ret = MPP_NOK;
    MppTaskQueue port = NULL;
    RK_U32 notify_flag = 0;

    set_io_mode(MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mUsrInPort;
        notify_flag = MPP_INPUT_DEQUEUE;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mUsrOutPort;
        notify_flag = MPP_OUTPUT_DEQUEUE;
    } break;
    default : {
    } break;
    }

    if (port) {
        ret = mpp_port_dequeue(port, task);
        if (MPP_OK == ret)
            notify(notify_flag);
    }

    return ret;
}

MPP_RET Mpp::enqueue(MppPortType type, MppTask task)
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    MPP_RET ret = MPP_NOK;
    MppTaskQueue port = NULL;
    RK_U32 notify_flag = 0;

    set_io_mode(MPP_IO_MODE_TASK);

    switch (type) {
    case MPP_PORT_INPUT : {
        port = mUsrInPort;
        notify_flag = MPP_INPUT_ENQUEUE;
    } break;
    case MPP_PORT_OUTPUT : {
        port = mUsrOutPort;
        notify_flag = MPP_OUTPUT_ENQUEUE;
    } break;
    default : {
    } break;
    }

    if (port) {
        ret = mpp_port_enqueue(port, task);
        // if enqueue success wait up thread
        if (MPP_OK == ret)
            notify(notify_flag);
    }

    return ret;
}

void Mpp::set_io_mode(MppIoMode mode)
{
    mpp_assert(mode == MPP_IO_MODE_NORMAL || mode == MPP_IO_MODE_TASK);

    if (mIoMode == MPP_IO_MODE_DEFAULT)
        mIoMode = mode;
    else if (mIoMode != mode) {
        static const char *iomode_2str[] = {
            "normal",
            "task queue",
        };

        mpp_assert(mIoMode < MPP_IO_MODE_BUTT);
        mpp_assert(mode < MPP_IO_MODE_BUTT);
        mpp_err("can not reset io mode from %s to %s\n",
                iomode_2str[!!mIoMode], iomode_2str[!!mode]);
    }
}

MPP_RET Mpp::control(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    mpp_ops_ctrl(mDump, cmd);

    if (mKmpp && mKmpp->mApi && mKmpp->mApi->control)
        return mKmpp->mApi->control(mKmpp, cmd, param);

    switch (cmd & CMD_MODULE_ID_MASK) {
    case CMD_MODULE_OSAL : {
        ret = control_osal(cmd, param);
    } break;
    case CMD_MODULE_MPP : {
        mpp_assert(cmd > MPP_CMD_BASE);
        mpp_assert(cmd < MPP_CMD_END);

        ret = control_mpp(cmd, param);
    } break;
    case CMD_MODULE_CODEC : {
        switch (cmd & CMD_CTX_ID_MASK) {
        case CMD_CTX_ID_DEC : {
            mpp_assert(mType == MPP_CTX_DEC || mType == MPP_CTX_BUTT);
            mpp_assert(cmd > MPP_DEC_CMD_BASE);
            mpp_assert(cmd < MPP_DEC_CMD_END);

            ret = control_dec(cmd, param);
        } break;
        case CMD_CTX_ID_ENC : {
            mpp_assert(mType == MPP_CTX_ENC);
            mpp_assert(cmd > MPP_ENC_CMD_BASE);
            mpp_assert(cmd < MPP_ENC_CMD_END);

            ret = control_enc(cmd, param);
        } break;
        case CMD_CTX_ID_ISP : {
            mpp_assert(mType == MPP_CTX_ISP);
            ret = control_isp(cmd, param);
        } break;
        default : {
            mpp_assert(cmd > MPP_CODEC_CMD_BASE);
            mpp_assert(cmd < MPP_CODEC_CMD_END);

            ret = control_codec(cmd, param);
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

MPP_RET Mpp::reset()
{
    if (!mInitDone)
        return MPP_ERR_INIT;

    if (mKmpp && mKmpp->mApi && mKmpp->mApi->reset)
        return mKmpp->mApi->reset(mKmpp);

    mpp_ops_reset(mDump);

    if (mType == MPP_CTX_DEC) {
        /*
         * On mp4 case extra data of sps/pps will be put at the beginning
         * If these packet was reset before they are send to decoder then
         * decoder can not get these important information to continue decoding
         * To avoid this case happen we need to save it on reset beginning
         * then restore it on reset end.
         */
        mPktIn->lock();
        while (mPktIn->list_size()) {
            MppPacket pkt = NULL;
            mPktIn->del_at_head(&pkt, sizeof(pkt));
            mPacketGetCount++;

            RK_U32 flags = mpp_packet_get_flag(pkt);
            if (flags & MPP_PACKET_FLAG_EXTRA_DATA) {
                if (mExtraPacket) {
                    mpp_packet_deinit(&mExtraPacket);
                }
                mExtraPacket = pkt;
            } else {
                mpp_packet_deinit(&pkt);
            }
        }
        mPktIn->flush();
        mPktIn->unlock();

        mpp_dec_reset(mDec);

        mFrmOut->lock();
        mFrmOut->flush();
        mFrmOut->unlock();

        mpp_port_awake(mUsrInPort);
        mpp_port_awake(mUsrOutPort);
    } else {
        mpp_enc_reset_v2(mEnc);
    }

    return MPP_OK;
}

MPP_RET Mpp::control_mpp(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_OK;

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
            mInputTimeout = block;
        else
            mOutputTimeout = block;

        mpp_log("deprecated block control, use timeout control instead\n");
    } break;

    case MPP_SET_DISABLE_THREAD: {
        mDisableThread = 1;
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
            mInputTimeout = timeout;
        else
            mOutputTimeout = timeout;
    } break;
    case MPP_SET_VENC_INIT_KCFG: {
        KmppObj obj = param;

        if (!obj) {
            mpp_err_f("ctrl %d invalid param %p\n", cmd, param);
            return MPP_ERR_VALUE;
        }
        mVencInitKcfg = obj;
    } break;
    case MPP_START : {
        start();
    } break;
    case MPP_STOP : {
        stop();
    } break;

    case MPP_PAUSE : {
        pause();
    } break;
    case MPP_RESUME : {
        resume();
    } break;

    default : {
        ret = MPP_NOK;
    } break;
    }
    return ret;
}

MPP_RET Mpp::control_osal(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    mpp_assert(cmd > MPP_OSAL_CMD_BASE);
    mpp_assert(cmd < MPP_OSAL_CMD_END);

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET Mpp::control_codec(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET Mpp::control_dec(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    switch (cmd) {
    case MPP_DEC_GET_THUMBNAIL_FRAME_INFO:
    case MPP_DEC_SET_FRAME_INFO: {
        ret = mpp_dec_control(mDec, cmd, param);
    } break;
    case MPP_DEC_SET_EXT_BUF_GROUP: {
        /*
         * NOTE: If frame buffer group is configured before decoder init
         * then the buffer limitation maybe not be correctly setup
         * without infomation from InfoChange frame.
         * And the thread signal connection may not be setup here. It
         * may have a bad effect on MPP efficiency.
         */
        if (!mInitDone) {
            mpp_err("WARNING: setup buffer group before decoder init\n");
            break;
        }

        ret = MPP_OK;
        if (!param) {
            /* set to internal mode */
            if (mExternalBufferMode) {
                /* switch from external mode to internal mode */
                mpp_assert(mFrameGroup);
                mpp_buffer_group_set_callback((MppBufferGroupImpl *)mFrameGroup,
                                              NULL, NULL);
                mFrameGroup = NULL;
            } else {
                /* keep internal buffer mode cleanup old buffers */
                if (mFrameGroup)
                    mpp_buffer_group_clear(mFrameGroup);
            }

            mpp_dbg_info("using internal buffer group %p\n", mFrameGroup);
            mExternalBufferMode = 0;
        } else {
            /* set to external mode */
            if (mExternalBufferMode) {
                /* keep external buffer mode */
                if (mFrameGroup != param) {
                    /* switch to new buffer group */
                    mpp_assert(mFrameGroup);
                    mpp_buffer_group_set_callback((MppBufferGroupImpl *)mFrameGroup,
                                                  NULL, NULL);
                } else {
                    /* keep old group the external group user should cleanup its old buffers */
                }
            } else {
                /* switch from intenal mode to external mode */
                if (mFrameGroup)
                    mpp_buffer_group_put(mFrameGroup);
            }

            mpp_dbg_info("using external buffer group %p\n", mFrameGroup);

            mFrameGroup = (MppBufferGroup)param;
            mpp_buffer_group_set_callback((MppBufferGroupImpl *)mFrameGroup,
                                          mpp_notify_by_buffer_group, (void *)this);
            mExternalBufferMode = 1;
            notify(MPP_DEC_NOTIFY_EXT_BUF_GRP_READY);
        }
    } break;
    case MPP_DEC_SET_INFO_CHANGE_READY: {
        mpp_dbg_info("set info change ready\n");

        ret = mpp_dec_control(mDec, cmd, param);
        notify(MPP_DEC_NOTIFY_INFO_CHG_DONE | MPP_DEC_NOTIFY_BUFFER_MATCH);
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
        if (mDec) {
            ret = mpp_dec_control(mDec, cmd, param);
            return ret;
        }

        ret = mpp_dec_set_cfg_by_cmd(&mDecInitcfg, cmd, param);
    } break;
    case MPP_DEC_GET_STREAM_COUNT: {
        AutoMutex autoLock(mPktIn->mutex());
        *((RK_S32 *)param) = mPktIn->list_size();
        ret = MPP_OK;
    } break;
    case MPP_DEC_GET_VPUMEM_USED_COUNT :
    case MPP_DEC_SET_OUTPUT_FORMAT :
    case MPP_DEC_QUERY :
    case MPP_DEC_SET_MAX_USE_BUFFER_SIZE: {
        ret = mpp_dec_control(mDec, cmd, param);
    } break;
    case MPP_DEC_SET_CFG : {
        if (mDec)
            ret = mpp_dec_control(mDec, cmd, param);
        else if (param) {
            MppDecCfgImpl *dec_cfg = (MppDecCfgImpl *)param;

            ret = mpp_dec_set_cfg(&mDecInitcfg, &dec_cfg->cfg);
        }
    } break;
    case MPP_DEC_GET_CFG : {
        if (mDec)
            ret = mpp_dec_control(mDec, cmd, param);
        else if (param) {
            MppDecCfgImpl *dec_cfg = (MppDecCfgImpl *)param;

            memcpy(&dec_cfg->cfg, &mDecInitcfg, sizeof(dec_cfg->cfg));
            ret = MPP_OK;
        }
    } break;
    default : {
    } break;
    }
    return ret;
}

MPP_RET Mpp::control_enc(MpiCmd cmd, MppParam param)
{
    mpp_assert(mEnc);
    return mpp_enc_control_v2(mEnc, cmd, param);
}

MPP_RET Mpp::control_isp(MpiCmd cmd, MppParam param)
{
    MPP_RET ret = MPP_NOK;

    mpp_assert(cmd > MPP_ISP_CMD_BASE);
    mpp_assert(cmd < MPP_ISP_CMD_END);

    (void)cmd;
    (void)param;
    return ret;
}

MPP_RET Mpp::notify(RK_U32 flag)
{
    switch (mType) {
    case MPP_CTX_DEC : {
        return mpp_dec_notify(mDec, flag);
    } break;
    case MPP_CTX_ENC : {
        return mpp_enc_notify_v2(mEnc, flag);
    } break;
    default : {
        mpp_err("unsupport context type %d\n", mType);
    } break;
    }
    return MPP_NOK;
}

MPP_RET Mpp::notify(MppBufferGroup group)
{
    MPP_RET ret = MPP_NOK;

    switch (mType) {
    case MPP_CTX_DEC : {
        if (group == mFrameGroup)
            ret = notify(MPP_DEC_NOTIFY_BUFFER_VALID |
                         MPP_DEC_NOTIFY_BUFFER_MATCH);
    } break;
    default : {
    } break;
    }

    return ret;
}
