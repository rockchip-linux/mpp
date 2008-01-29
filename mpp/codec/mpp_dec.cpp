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

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_dec.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buf_slot.h"

#include "h264d_api.h"
#include "h265d_api.h"

/*
 * all decoder static register here
 */
static const MppDecParser *parsers[] = {
    &api_h264d_parser,
};

#define MPP_TEST_FRAME_SIZE     SZ_1M

static MPP_RET mpp_dec_parse(MppDec *dec, MppPacket pkt, HalTask *task)
{
    return dec->parser_api->parse(dec->parser_ctx, pkt, &task->dec);
}

void *mpp_dec_parser_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *parser   = mpp->mTheadCodec;
    MppThread *hal      = mpp->mThreadHal;
    MppDec    *dec      = mpp->mDec;
    mpp_list  *packets  = mpp->mPackets;
    MppPacketImpl packet;
    HalTask local_task;
    HalTaskHnd syntax       = NULL;
    RK_U32 packet_ready     = 0;
    RK_U32 packet_parsed    = 0;
    RK_U32 syntax_ready     = 0;
    RK_U32 slot_ready       = 0;

    while (MPP_THREAD_RUNNING == parser->get_status()) {
        /*
         * wait for stream input
         */
        parser->lock();
        if (!packet_ready && (0 == packets->list_size()))
            parser->wait();
        parser->unlock();

        if (!packet_ready) {
            if (packets->list_size()) {
                mpp->mPacketLock.lock();
                /*
                 * packet will be destroyed outside, here just copy the content
                 */
                packets->del_at_head(&packet, sizeof(packet));
                mpp->mPacketGetCount++;
                mpp->mPacketLock.unlock();
                packet_ready = 1;
            }
        }

        if (!packet_ready)
            continue;

        /*
         * 1. send packet data to parser
         *
         *    parser functioin input / output
         *    input:    packet data
         *              dxva output slot
         *    output:   dxva output slot
         *              buffer usage informatioin
         */
        if (!packet_parsed) {
            mpp_dec_parse(dec, (MppPacket)&packet, &local_task);
            packet_parsed = 1;
        }

        if (!syntax_ready) {
            hal_task_get_hnd(dec->tasks, 0, &syntax);
            if (syntax) {
                hal_task_set_info(syntax, &local_task);
                syntax_ready = 1;
            }
        }

        if (!syntax_ready)
            continue;

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

        hal_task_set_used(syntax, 1);
        mpp->mTaskPutCount++;

        hal->signal();
        packet_ready    = 0;
        syntax_ready    = 0;
        packet_parsed   = 0;
        slot_ready      = 0;
    }

    return NULL;
}

void *mpp_dec_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    MppDec    *dec      = mpp->mDec;
    mpp_list *frames    = mpp->mFrames;
    HalTaskHnd  syntax  = NULL;
    HalTask local_task;

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (0 == hal_task_get_hnd(dec->tasks, 1, &syntax))
            hal->wait();
        hal->unlock();

        // get_config
        // register genertation
        if (NULL == syntax)
            hal_task_get_hnd(dec->tasks, 1, &syntax);

        if (NULL == syntax)
            continue;

        mpp->mTaskGetCount++;

        hal_task_get_info(dec->tasks, &local_task);
        // hal->mpp_hal_reg_gen(current);
        hal_task_set_used(syntax, 0);

        /*
         * wait previous register set done
         */
        // hal->mpp_hal_hw_wait(previous);

        /*
         * send current register set to hardware
         */
        // hal->mpp_hal_hw_start(current);

        /*
         * mark previous buffer is complete
         */
        // change dpb slot status
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

            // init hal first to get the syntax group
            MppHalCfg hal_cfg = {
                MPP_CTX_DEC,
                MPP_VIDEO_CodingAVC,
                NULL,
                0,
            };
            mpp_hal_init(&p->hal_ctx, &hal_cfg);
            p->tasks = hal_cfg.tasks;

            // use syntax and dpb slot to init parser
            MppParserInitCfg parser_cfg = {
                p->slots,
            };
            p->parser_api->init(p->parser_ctx, &parser_cfg);

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

    if (dec->parser_ctx)
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

