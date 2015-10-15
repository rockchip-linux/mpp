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
#include "mpp_buffer_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_frame_impl.h"

static void mpp_put_frame(Mpp *mpp, MppFrame frame)
{
    mpp_list *list = mpp->mFrames;
    list->lock();
    list->add_at_tail(&frame, sizeof(frame));
    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
}

void *mpp_dec_parser_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *parser   = mpp->mThreadCodec;
    MppDec    *dec      = mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppPacket packet    = NULL;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    RK_S32 stream_index = -1;
    size_t stream_size = 0;

    /*
     * parser thread need to wait at cases below:
     * 1. no task slot for output
     * 2. no packet for parsing
     * 3. info change on progress
     * 3. no buffer on analyzing output task
     */
    RK_U32 wait_on_task_hnd = 0;
    RK_U32 wait_on_packet   = 0;
    RK_U32 wait_on_input_index = 0;
    RK_U32 wait_on_input_buffer = 0;
    RK_U32 wait_on_prev     = 0;
    RK_U32 wait_on_change   = 0;
    RK_U32 wait_on_frame_buffer = 0;

    RK_U32 pkt_buf_copyied  = 0;
    RK_U32 prev_task_done   = 1;
    RK_U32 info_task_done   = 0;
    RK_U32 curr_task_ready  = 0;
    RK_U32 curr_task_parsed = 0;

    HalTaskHnd  task = NULL;
    HalTaskInfo task_local;
    HalDecTask  *task_dec = &task_local.dec;
    MppBuffer buffer = NULL;

    hal_task_info_init(&task_local, MPP_CTX_DEC);

    while (MPP_THREAD_RUNNING == parser->get_status()) {
        /*
         * wait for stream input
         */
        if (dec->reset_flag && !wait_on_frame_buffer) {
            if (!prev_task_done) {
                HalTaskHnd task_prev = NULL;
                hal_task_get_hnd(tasks, TASK_PROC_DONE, &task_prev);
                if (task_prev) {
                    prev_task_done  = 1;
                    hal_task_hnd_set_status(task_prev, TASK_IDLE);
                    task_prev = NULL;
                    wait_on_prev = 0;
                } else {
                    usleep(5000);
                    wait_on_prev = 1;
                    continue;
                }
            }

            {
                RK_S32 index;
                parser->reset_lock();
                curr_task_ready = 0;
                task_dec->valid = 0;
                parser_reset(dec->parser);
                dec->reset_flag = 0;
                if (packet) {
                    mpp_free(mpp_packet_get_data(packet));
                    mpp_packet_deinit(&packet);
                    packet = NULL;
                }
                while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
                    MppFrame frame;
                    mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
                    mpp_frame_deinit(&frame);
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
                }
                if (pkt_buf_copyied) {
                    mpp_buf_slot_get_prop(packet_slots, task_dec->input,  SLOT_BUFFER, &buffer);
                    if (buffer) {
                        mpp_buffer_put(buffer);
                        buffer = NULL;
                    }
                    mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
                    pkt_buf_copyied = 0;
                    task_dec->input = -1;
                }

                curr_task_parsed = 0;
                parser->reset_unlock();
                parser->reset_signal();
            }
        }
        parser->lock();
        if ((wait_on_task_hnd || wait_on_packet ||
             wait_on_prev || wait_on_change || wait_on_frame_buffer) && !dec->reset_flag)
            parser->wait();
        parser->unlock();

        /*
         * 1. get task handle from hal for parsing one frame
         */
        if (!task) {
            hal_task_get_hnd(tasks, TASK_IDLE, &task);
            if (task) {
                wait_on_task_hnd = 0;
            } else {
                wait_on_task_hnd = 1;
                continue;
            }
        }

        /*
         * 2. get packet to parse
         */
        if (!packet && !curr_task_ready) {
            mpp_list *packets = mpp->mPackets;
            Mutex::Autolock autoLock(packets->mutex());
            if (packets->list_size()) {
                /*
                 * packet will be destroyed outside, here just copy the content
                 */
                packets->del_at_head(&packet, sizeof(packet));
                mpp->mPacketGetCount++;
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
        if (!curr_task_ready) {
            parser_prepare(dec->parser, packet, task_dec);
            if (0 == mpp_packet_get_length(packet)) {
                mpp_free(mpp_packet_get_data(packet));
                mpp_packet_deinit(&packet);
                packet = NULL;
            }
        }

        curr_task_ready = task_dec->valid;
        if (!curr_task_ready)
            continue;

        // NOTE: packet in task should be ready now
        mpp_assert(task_dec->input_packet);

        // copy prepared stream to hardware buffer
        if (task_dec->input < 0) {
            mpp_buf_slot_get_unused(packet_slots, &task_dec->input);
        }

        wait_on_input_index = (task_dec->input < 0);
        if (wait_on_input_index)
            continue;

        stream_index = task_dec->input;
        stream_size = mpp_packet_get_size(task_dec->input_packet);
        mpp_buf_slot_get_prop(packet_slots, stream_index, SLOT_BUFFER, &buffer);
        if (NULL == buffer) {
            mpp_buffer_get(mpp->mPacketGroup, &buffer, stream_size);
            if (buffer)
                mpp_buf_slot_set_prop(packet_slots, stream_index, SLOT_BUFFER, buffer);
        } else {
            MppBufferImpl *buf = (MppBufferImpl *)buffer;
            mpp_assert(buf->info.size >= stream_size);
        }

        wait_on_input_buffer = (NULL == buffer);
        if (wait_on_input_buffer)
            continue;

        if (!pkt_buf_copyied) {
            MppBufferImpl *buf = (MppBufferImpl *)buffer;
            void *src = mpp_packet_get_data(task_dec->input_packet);
            size_t length = mpp_packet_get_length(task_dec->input_packet);
            memcpy(buf->info.ptr, src, length);
            mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
            mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
            pkt_buf_copyied = 1;
        }

        // wait previous task done
        if (!prev_task_done) {
            HalTaskHnd task_prev = NULL;
            hal_task_get_hnd(tasks, TASK_PROC_DONE, &task_prev);
            if (task_prev) {
                prev_task_done  = 1;
                wait_on_prev = 0;
                hal_task_hnd_set_status(task_prev, TASK_IDLE);
                task_prev = NULL;
            } else {
                wait_on_prev = 1;
                continue;
            }
        }

        if (!curr_task_parsed) {
            parser_parse(dec->parser, task_dec);
            curr_task_parsed = 1;
        }

        /*
         * 4. parse local task and slot to check whether new buffer or info change is needed.
         *
         * a. first detect info change from frame slot
         * b. then detect whether output index has MppBuffer
         */
        if (mpp_buf_slot_is_changed(frame_slots)) {
            if (!info_task_done) {
                task_dec->flags.info_change = 1;
                hal_task_hnd_set_info(task, &task_local);
                hal_task_hnd_set_status(task, TASK_PROCESSING);
                mpp->mThreadHal->signal();
                mpp->mTaskPutCount++;
                task = NULL;
                info_task_done = 1;
            }
        }
        wait_on_change = mpp_buf_slot_is_changed(frame_slots);
        if (wait_on_change)
            continue;

        info_task_done = 0;
        task_dec->flags.info_change = 0;
        if(task_dec->output < 0){
            hal_task_hnd_set_status(task, TASK_IDLE);
            mpp->mTaskPutCount++;
            task = NULL;
            if (pkt_buf_copyied) {
                mpp_buf_slot_get_prop(packet_slots, task_dec->input,  SLOT_BUFFER, &buffer);
                if (buffer) {
                  mpp_buffer_put(buffer);
                  buffer = NULL;
                }
                mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
                pkt_buf_copyied = 0;
            }
            curr_task_ready  = 0;
            curr_task_parsed = 0;
            hal_task_info_init(&task_local, MPP_CTX_DEC);
            continue;
        }
        /*
         * 5. chekc frame buffer group is internal or external
         */
        if (NULL == mpp->mFrameGroup) {
            mpp_log("mpp_dec use internal frame buffer group\n");
            mpp_buffer_group_get_internal(&mpp->mFrameGroup, MPP_BUFFER_TYPE_ION);
        }

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
        RK_S32 output = task_dec->output;
        buffer = NULL;
        mpp_buf_slot_get_prop(frame_slots, output, SLOT_BUFFER, &buffer);
        if (NULL == buffer) {
            RK_U32 size = mpp_buf_slot_get_size(frame_slots);
            mpp_buffer_get(mpp->mFrameGroup, &buffer, size);
            if (buffer)
                mpp_buf_slot_set_prop(frame_slots, output, SLOT_BUFFER, buffer);
        }

        wait_on_frame_buffer = (NULL == buffer);
        if (wait_on_frame_buffer)
            continue;

        // register genertation
        mpp_hal_reg_gen(dec->hal, &task_local);

        /*
         * wait previous register set done
         */
        //mpp_hal_hw_wait(dec->hal_ctx, &task_local);

        /*
         * send current register set to hardware
         */
        //mpp_hal_hw_start(dec->hal_ctx, &task_local);

        mpp_hal_hw_start(dec->hal, &task_local);

        /*
         * 6. send dxva output information and buffer information to hal thread
         *    combinate video codec dxva output and buffer information
         */
        hal_task_hnd_set_info(task, &task_local);
        hal_task_hnd_set_status(task, TASK_PROCESSING);
        mpp->mThreadHal->signal();

        mpp->mTaskPutCount++;
        task = NULL;
        pkt_buf_copyied  = 0;
        curr_task_ready  = 0;
        curr_task_parsed = 0;
        prev_task_done   = 0;
        hal_task_info_init(&task_local, MPP_CTX_DEC);
    }

    if (NULL != task && task_dec->valid) {
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        mpp_buf_slot_clr_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        if (buffer)
            mpp_buffer_put(buffer);
    }
    mpp_buffer_group_clear(mpp->mPacketGroup);
    return NULL;
}

void *mpp_dec_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    MppDec    *dec      = mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppBuffer buffer    = NULL;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;

    /*
     * hal thread need to wait at cases below:
     * 1. no task slot for work
     */
    HalTaskHnd  task = NULL;
    HalTaskInfo task_info;
    HalDecTask  *task_dec = &task_info.dec;

    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (hal_task_get_hnd(tasks, TASK_PROCESSING, &task))
            hal->wait();
        hal->unlock();

        if (task) {
            mpp->mTaskGetCount++;

            hal_task_hnd_get_info(task, &task_info);
            /*
             * check info change flag
             * if this is a info change frame, only output the mpp_frame for info change.
             */
            if (task_dec->flags.info_change) {
                MppFrame info_frame = NULL;
                mpp_buf_slot_get_prop(frame_slots, task_dec->output, SLOT_FRAME, &info_frame);
                mpp_assert(info_frame);
                mpp_assert(NULL == mpp_frame_get_buffer(info_frame));
                mpp_frame_set_info_change(info_frame, 1);
                mpp_put_frame(mpp, info_frame);
                hal_task_hnd_set_status(task, TASK_IDLE);
                task = NULL;
                continue;
                mpp->mThreadCodec->signal();
            }

            mpp_hal_hw_wait(dec->hal, &task_info);



            /*
             * when hardware decoding is done:
             * 1. clear decoding flag (mark buffer is ready)
             * 2. use get_display to get a new frame with buffer
             * 3. add frame to output list
             * repeat 2 and 3 until not frame can be output
             */
            mpp_buf_slot_get_prop(packet_slots, task_dec->input,  SLOT_BUFFER, &buffer);
            if (buffer) {
                mpp_buffer_put(buffer);
                buffer = NULL;
            }
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);

            // TODO: may have risk here
            hal_task_hnd_set_status(task, TASK_PROC_DONE);
            task = NULL;
            mpp->mThreadCodec->signal();

            mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);
            for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
                RK_S32 index = task_dec->refer[i];
                if (index >= 0)
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_HAL_INPUT);
            }

            if (task_dec->flags.eos) {
                mpp_dec_flush(dec);
            }

            RK_S32 index;
            while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
                MppFrame frame;
                mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
                if (!dec->reset_flag) {
                    mpp_put_frame(mpp, frame);
                } else {
                    mpp_frame_deinit(&frame);
                }
                mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
            }
        }
    }

    return NULL;
}

MPP_RET mpp_dec_init(MppDec **dec, MppCodingType coding)
{
    MPP_RET ret;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    Parser parser = NULL;
    MppHal hal = NULL;

    MppDec *p = mpp_calloc(MppDec, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_NULL_PTR;
    }

    do {
        ret = mpp_buf_slot_init(&frame_slots);
        if (ret) {
            mpp_err_f("could not init frame buffer slot\n");
            break;
        }

        ret = mpp_buf_slot_init(&packet_slots);
        if (ret) {
            mpp_err_f("could not init packet buffer slot\n");
            break;
        }

        mpp_buf_slot_setup(packet_slots, 2, SZ_512K, 0);

        ParserCfg parser_cfg = {
            coding,
            frame_slots,
            packet_slots,
            2,
        };

        ret = parser_init(&parser, &parser_cfg);
        if (ret) {
            mpp_err_f("could not init parser\n");
            break;
        }

        // then init hal with task count from parser
        MppHalCfg hal_cfg = {
            MPP_CTX_DEC,
            coding,
            HAL_MODE_LIBVPU,
            HAL_RKVDEC,
            frame_slots,
            packet_slots,
            NULL,
            parser_cfg.task_count,
        };

        ret = mpp_hal_init(&hal, &hal_cfg);
        if (ret) {
            mpp_err_f("could not init hal\n");
            break;
        }

        p->coding = coding;
        p->parser = parser;
        p->hal    = hal;
        p->tasks  = hal_cfg.tasks;
        p->frame_slots  = frame_slots;
        p->packet_slots = packet_slots;
        *dec = p;
        return MPP_OK;
    } while (0);

    mpp_dec_deinit(p);
    *dec = NULL;
    return MPP_NOK;
}

MPP_RET mpp_dec_deinit(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input\n");
        return MPP_ERR_NULL_PTR;
    }

    if (dec->parser) {
        parser_deinit(dec->parser);
        dec->parser = NULL;
    }

    if (dec->hal) {
        mpp_hal_deinit(dec->hal);
        dec->hal = NULL;
    }

    if (dec->frame_slots) {
        mpp_buf_slot_deinit(dec->frame_slots);
        dec->frame_slots = NULL;
    }

    if (dec->packet_slots) {
        mpp_buf_slot_deinit(dec->packet_slots);
        dec->packet_slots = NULL;
    }

    mpp_free(dec);
    return MPP_OK;
}

MPP_RET mpp_dec_reset(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }
    dec->reset_flag = 1;
    // parser_reset(dec->parser);
    //  mpp_hal_reset(dec->hal);

    return MPP_OK;
}

MPP_RET mpp_dec_flush(MppDec *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    parser_flush(dec->parser);
    mpp_hal_flush(dec->hal);

    return MPP_OK;
}

MPP_RET mpp_dec_control(MppDec *dec, RK_S32 cmd, void *param)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }

    parser_control(dec->parser, cmd, param);
    mpp_hal_control(dec->hal, cmd, param);

    return MPP_OK;
}

