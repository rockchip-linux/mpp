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

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_dec.h"
#include "mpp_packet_impl.h"
#include "mpp_frame_impl.h"

#include "h264d_api.h"
#include "h265d_api.h"

// for test and demo
#include "dummy_dec_api.h"

/*
 * all decoder static register here
 */
static const MppDecParser *parsers[] = {
    &api_h264d_parser,
    &dummy_dec_parser,
};

#define MPP_TEST_FRAME_SIZE     SZ_1M

static MPP_RET mpp_dec_parse(MppDec *dec, MppPacket pkt, HalDecTask *task)
{
    return dec->parser_api->parse(dec->parser_ctx, pkt, task);
}

void *mpp_dec_parser_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *parser   = mpp->mThreadCodec;
    MppDec    *dec      = mpp->mDec;
    MppBufSlots slots   = dec->slots;
    HalTaskGroup tasks  = dec->tasks;
    MppPacketImpl packet;

    /*
     * parser thread need to wait at cases below:
     * 1. no task slot for output
     * 2. no packet for parsing
     * 3. info change on progress
     * 3. no buffer on analyzing output task
     */
    RK_U32 wait_on_packet   = 0;
    RK_U32 wait_on_task     = 0;
    RK_U32 wait_on_change   = 0;
    RK_U32 wait_on_buffer   = 0;

    RK_U32 packet_ready     = 0;
    RK_U32 task_ready       = 0;

    HalTask     task_local;
    HalDecTask  *task_dec = &task_local.dec;
    memset(&task_local, 0, sizeof(task_local));

    while (MPP_THREAD_RUNNING == parser->get_status()) {
        /*
         * wait for stream input
         */
        parser->lock();
        if (wait_on_task || wait_on_packet || wait_on_change || wait_on_buffer)
            parser->wait();
        parser->unlock();

        /*
         * 1. get task handle from hal for parsing one frame
         */
        wait_on_task = (MPP_OK != hal_task_can_put(tasks));
        if (wait_on_task)
            continue;

        /*
         * 2. get packet to parse
         */
        if (!packet_ready) {
            mpp_list *packets = mpp->mPackets;
            Mutex::Autolock autoLock(packets->mutex());
            if (packets->list_size()) {
                /*
                 * packet will be destroyed outside, here just copy the content
                 */
                packets->del_at_head(&packet, sizeof(packet));
                mpp->mPacketGetCount++;
                packet_ready = 1;
                wait_on_packet = 0;
            } else {
                wait_on_packet = 1;
                continue;
            }
        }

        /*
         * 3. send packet data to parser
         *
         *    parser functioin input / output
         *    input:    packet data
         *    output:   dec task output information (with dxva output slot)
         *              buffer slot usage informatioin
         *
         *    NOTE:
         *    1. dpb slot will be set internally in parser process.
         *    2. parse function need to set valid flag when one frame is ready.
         *    3. if packet size is zero then next packet is needed.
         *
         */
        if (!task_ready) {
            mpp_dec_parse(dec, (MppPacket)&packet, task_dec);
            if (0 == packet.size) {
                mpp_packet_reset(&packet);
                packet_ready = 0;
            }
        }

        task_ready = task_dec->valid;
        if (!task_ready)
            continue;

        /*
         * 4. parse local task and slot to check whether new buffer or info change is needed.
         *
         * a. first detect info change
         * b. then detect whether output index has MppBuffer
         */
        wait_on_change = mpp_buf_slot_is_changed(slots);
        if (wait_on_change)
            continue;

        /*
         * 5. do buffer operation according to usage information
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
        RK_U32 output;
        mpp_buf_slot_get_hw_dst(slots, &output);
        if (NULL == mpp_buf_slot_get_buffer(slots, output)) {
            MppBuffer buffer = NULL;
            RK_U32 size = mpp_buf_slot_get_size(slots);
            mpp_buffer_get(mpp->mFrameGroup, &buffer, size);
            if (buffer)
                mpp_buf_slot_set_buffer(slots, output, buffer);
        }

        wait_on_buffer = (NULL == mpp_buf_slot_get_buffer(slots, output));
        if (wait_on_buffer)
            continue;

        /*
         * 6. send dxva output information and buffer information to hal thread
         *    combinate video codec dxva output and buffer information
         */
        hal_task_put(tasks, &task_local);
        mpp->mTaskPutCount++;

        task_ready      = 0;
        mpp->mThreadHal->signal();
    }

    return NULL;
}

void *mpp_dec_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    MppDec    *dec      = mpp->mDec;
    MppBufSlots slots   = dec->slots;
    HalTaskGroup tasks  = dec->tasks;
    mpp_list *frames    = mpp->mFrames;

    /*
     * hal thread need to wait at cases below:
     * 1. no task slot for work
     */
    RK_U32 wait_on_task = 0;

    HalTask     task_local;
    HalDecTask  *task_dec = &task_local.dec;
    memset(&task_local, 0, sizeof(task_local));

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (wait_on_task)
            hal->wait();
        hal->unlock();

        // get hw task first
        wait_on_task = (MPP_OK != hal_task_can_get(tasks));
        if (wait_on_task)
            continue;

        mpp->mTaskGetCount++;

        hal_task_get(tasks, &task_local);

        // register genertation
        mpp_hal_reg_gen(dec->hal_ctx, &task_local);
        mpp->mThreadCodec->signal();

        /*
         * wait previous register set done
         */
        //mpp_hal_hw_wait(dec->hal_ctx, &task_local);

        /*
         * send current register set to hardware
         */
        //mpp_hal_hw_start(dec->hal_ctx, &task_local);

        mpp_hal_hw_start(dec->hal_ctx, &task_local);
        mpp_hal_hw_wait(dec->hal_ctx, &task_local);

        /*
         * when hardware decoding is done:
         * 1. clear decoding flag
         * 2. use get_display to get a new frame with buffer
         * 3. add frame to output list
         */
        mpp_buf_slot_clr_hw_dst(slots, task_dec->output);
        for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
            RK_S32 index = task_dec->refer[i];
            if (index >= 0)
                mpp_buf_slot_dec_hw_ref(slots, index);
        }

        MppFrame frame = NULL;
        while (MPP_OK == mpp_buf_slot_get_display(slots, &frame)) {
            frames->lock();
            frames->add_at_tail(&frame, sizeof(frame));
            mpp->mFramePutCount++;
            frames->signal();
            frames->unlock();
        }

        /*
         * mark previous buffer is complete
         */
        // change dpb slot status
        // signal()
        // mark frame in output queue
        // wait up output thread to get a output frame
    }

    return NULL;
}

MPP_RET mpp_dec_init(MppDec **dec, MppCodingType coding)
{
    MppDec *p = mpp_calloc(MppDec, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    mpp_buf_slot_init(&p->slots);
    if (NULL == p->slots) {
        mpp_err_f("could not init buffer slot\n");
        *dec = NULL;
        mpp_free(p);
        return MPP_ERR_UNKNOW;
    }

    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(parsers); i++) {
        if (coding == parsers[i]->coding) {
            p->coding       = coding;
            p->parser_api   = parsers[i];
            p->parser_ctx   = mpp_malloc_size(void, parsers[i]->ctx_size);
            if (NULL == p->parser_ctx) {
                mpp_err_f("failed to alloc decoder context\n");
                break;
            }

            // init parser first to get task count
            MppParserInitCfg parser_cfg = {
                p->slots,
                2,
            };
            p->parser_api->init(p->parser_ctx, &parser_cfg);

            // then init hal with task count from parser
            MppHalCfg hal_cfg = {
                MPP_CTX_DEC,
                coding,
                p->slots,
                NULL,
                parser_cfg.task_count,
            };
            mpp_hal_init(&p->hal_ctx, &hal_cfg);
            p->tasks = hal_cfg.tasks;

            *dec = p;
            return MPP_OK;
        }
    }

    mpp_err_f("could not found coding type %d\n", coding);

    // TODO: need to add error handle here
    *dec = NULL;
    mpp_free(p);
    return MPP_NOK;
}

MPP_RET mpp_dec_deinit(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    dec->parser_api->deinit(dec->parser_ctx);

    mpp_free(dec->parser_ctx);

    if (dec->hal_ctx)
        mpp_hal_deinit(dec->hal_ctx);

    if (dec->slots)
        mpp_buf_slot_deinit(dec->slots);

    mpp_free(dec);
    return MPP_OK;
}

MPP_RET mpp_dec_reset(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    return dec->parser_api->reset(dec->parser_ctx);
}

MPP_RET mpp_dec_flush(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    return dec->parser_api->flush(dec->parser_ctx);
}

MPP_RET mpp_dec_control(MppDec *dec, RK_S32 cmd, void *param)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    return dec->parser_api->control(dec->parser_ctx, cmd, param);
}

