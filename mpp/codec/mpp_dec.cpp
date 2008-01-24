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

#include "h264d_api.h"
#include "h265d_api.h"

/*
 * all decoder static register here
 */
static const MppDecParser *parsers[] = {
    &api_h264d_parser,
};

#define MPP_TEST_FRAME_SIZE     SZ_1M

void *mpp_dec_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *dec  = mpp->mTheadCodec;
    MppThread *hal  = mpp->mThreadHal;
    mpp_list *packets   = mpp->mPackets;
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

MPP_RET mpp_dec_init(MppDecCtx **ctx, MppCodingType coding)
{
    MppDecCtx *p = mpp_malloc(MppDecCtx, 1);
    if (NULL == p) {
        mpp_err("%s failed to malloc context\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }
    RK_U32 i;
    for (i = 0; i < MPP_ARRAY_ELEMS(parsers); i++) {
        if (coding == parsers[i]->coding) {
            p->coding   = coding;
            p->parser   = parsers[i];
            *ctx = p;
            return MPP_OK;
        }
    }
    mpp_err("%s could not found coding type %d\n", __FUNCTION__, coding);
    *ctx = NULL;
    mpp_free(p);
    return MPP_NOK;
}

MPP_RET mpp_dec_deinit(MppDecCtx *ctx)
{
    if (NULL == ctx) {
        mpp_err("%s found NULL input\n", __FUNCTION__);
        return MPP_ERR_NULL_PTR;
    }
    mpp_free(ctx);
    return MPP_OK;
}

MPP_RET mpp_dec_parse(MppDecCtx *ctx, MppPacket pkt, MppSyntax **syn)
{
    if (NULL == ctx || NULL == pkt || NULL == syn) {
        mpp_err("%s found NULL input ctx %p pkt %p syn %p\n",
                __FUNCTION__, ctx, pkt, syn);
        return MPP_ERR_NULL_PTR;
    }

    return ctx->parser->parse(ctx, pkt, &ctx->syntax[0]);
}

MPP_RET mpp_dec_reset(MppDecCtx *ctx)
{
    if (NULL == ctx) {
        mpp_err("%s found NULL input ctx %p\n",
                __FUNCTION__, ctx);
        return MPP_ERR_NULL_PTR;
    }

    return ctx->parser->reset(ctx);
}

MPP_RET mpp_dec_flush(MppDecCtx *ctx)
{
    if (NULL == ctx) {
        mpp_err("%s found NULL input ctx %p\n",
                __FUNCTION__, ctx);
        return MPP_ERR_NULL_PTR;
    }

    return ctx->parser->flush(ctx);
}

MPP_RET mpp_dec_control(MppDecCtx *ctx, RK_S32 cmd, void *param)
{
    if (NULL == ctx) {
        mpp_err("%s found NULL input ctx %p\n",
                __FUNCTION__, ctx);
        return MPP_ERR_NULL_PTR;
    }

    return ctx->parser->control(ctx, cmd, param);
}

