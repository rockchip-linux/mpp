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

#define  MODULE_TAG "mpp_dec"

#include <string.h>

#include "mpp_mem.h"
#include "mpp_log.h"
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_frame.h"
#include "mpp_buffer_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_frame_impl.h"

typedef union PaserTaskWait_u {
    RK_U32          val;
    struct {
        RK_U32      task_hnd    : 1;
        RK_U32      mpp_pkt_in  : 1;
        RK_U32      dec_pkt_idx : 1;
        RK_U32      dec_pkt_buf : 1;
        RK_U32      prev_task   : 1;
        RK_U32      info_change : 1;
        RK_U32      dec_pic_buf : 1;
    };
} PaserTaskWait;

typedef union DecTaskStatus_u {
    RK_U32          val;
    struct {
        RK_U32      task_hnd_rdy      : 1;
        RK_U32      mpp_pkt_in_rdy    : 1;
        RK_U32      dec_pkt_idx_rdy   : 1;
        RK_U32      dec_pkt_buf_rdy   : 1;
        RK_U32      task_valid_rdy    : 1;
        RK_U32      dec_pkt_copy_rdy  : 1;
        RK_U32      prev_task_rdy     : 1;
        RK_U32      info_task_gen_rdy : 1;
        RK_U32      curr_task_rdy     : 1;
        RK_U32      task_parsed_rdy   : 1;
    };
} DecTaskStatus;

typedef struct DecTask_t {
    HalTaskHnd      hnd;

    DecTaskStatus   status;
    PaserTaskWait   wait;

    RK_S32          hal_pkt_idx_in;
    RK_S32          hal_frm_idx_out;

    MppBuffer       hal_pkt_buf_in;
    MppBuffer       hal_frm_buf_out;

    HalTaskInfo     info;
} DecTask;

static void dec_task_init(DecTask *task)
{
    task->hnd = NULL;
    task->status.val = 0;
    task->wait.val   = 0;
    task->status.prev_task_rdy  = 1;

    task->hal_pkt_idx_in  = -1;
    task->hal_frm_idx_out = -1;

    task->hal_pkt_buf_in  = NULL;
    task->hal_frm_buf_out = NULL;

    hal_task_info_init(&task->info, MPP_CTX_DEC);
}
#if 0
static void dec_task_reset(MppDec *dec, DecTask *task)
{
    task->hnd = NULL;
    task->status.val = 0;
    task->wait.val   = 0;
    task->status.prev_task_rdy  = 1;

    if (task->hal_pkt_idx_in >= 0) {
        mpp_buf_slot_clr_flag(dec->frame_slots, task->hal_pkt_idx_in, SLOT_HAL_INPUT);
        task->hal_pkt_idx_in = -1;
    }

    if (task->hal_frm_idx_out >= 0) {
        mpp_buf_slot_clr_flag(dec->frame_slots, task->hal_frm_idx_out, SLOT_HAL_INPUT);
        task->hal_frm_idx_out = -1;
    }

    if (task->hal_pkt_buf_in) {
        mpp_buffer_put(task->hal_pkt_buf_in);
        task->hal_pkt_buf_in = NULL;
    }

    if (task->hal_frm_buf_out) {
        mpp_buffer_put(task->hal_frm_buf_out);
        task->hal_frm_buf_out = NULL;
    }

    hal_task_info_init(&task->info, MPP_CTX_DEC);
}
#endif
/*
 * return MPP_OK for not wait
 * return MPP_NOK for wait
 */
static MPP_RET check_task_wait(MppDec *dec, DecTask *task)
{
    if (dec->reset_flag) {
        return MPP_OK;
    }

    if (task->wait.task_hnd ||
        task->wait.mpp_pkt_in ||
        task->wait.prev_task ||
        task->wait.info_change ||
        task->wait.dec_pic_buf)
        return MPP_NOK;

    return MPP_OK;
}

static RK_U32 reset_dec_task(Mpp *mpp, DecTask *task)
{
    MppThread *parser   = mpp->mThreadCodec;
    MppDec    *dec      = mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppBufSlots frame_slots  = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    HalDecTask *task_dec = &task->info.dec;

    if (!dec->parser_fast_mode) {
        if (!task->status.prev_task_rdy) {
            HalTaskHnd task_prev = NULL;
            hal_task_get_hnd(tasks, TASK_PROC_DONE, &task_prev);
            if (task_prev) {
                task->status.prev_task_rdy  = 1;
                hal_task_hnd_set_status(task_prev, TASK_IDLE);
                task_prev = NULL;
                task->wait.prev_task = 0;
            } else {
                msleep(5);
                task->wait.prev_task = 1;
                return MPP_NOK;
            }
        }
    } else {
        if (hal_task_check_empty(tasks, TASK_PROCESSING)) {
            msleep(5);
            return MPP_NOK;
        }
    }

    {
        RK_S32 index;
        parser->lock(THREAD_RESET);
        task->status.curr_task_rdy = 0;
        task_dec->valid = 0;
        parser_reset(dec->parser);
        mpp_hal_reset(dec->hal);
        dec->reset_flag = 0;
        if (task->wait.info_change) {
            mpp_log("reset add info change status");
            mpp_buf_slot_reset(frame_slots, task_dec->output);

        }
        if (task->status.task_parsed_rdy) {
            mpp_log("task no send to hal que must clr current frame hal status");
            mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);
            for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
                index = task_dec->refer[i];
                if (index >= 0)
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_HAL_INPUT);
            }
        }
        if (dec->mpp_pkt_in) {
            mpp_packet_deinit(&dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }
        while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
            /* release extra ref in slot's MppBuffer */
            MppBuffer buffer = NULL;
            mpp_buf_slot_get_prop(frame_slots, index, SLOT_BUFFER, &buffer);
            if (buffer)
                mpp_buffer_put(buffer);
            mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        }
        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
            task->status.dec_pkt_copy_rdy = 0;
            task_dec->input = -1;
        }


        task->status.task_parsed_rdy = 0;
        parser->unlock(THREAD_RESET);
        parser->signal(THREAD_RESET);
    }

    dec_task_init(task);

    return MPP_OK;
}

static void mpp_put_frame(Mpp *mpp, MppFrame frame)
{
    mpp_list *list = mpp->mFrames;

    list->lock();
    list->add_at_tail(&frame, sizeof(frame));

    if (mpp_debug & MPP_DBG_PTS)
        mpp_log("output frame pts %lld\n", mpp_frame_get_pts(frame));

    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
}

static void mpp_put_frame_eos(Mpp *mpp)
{
    MppFrame info_frame = NULL;
    mpp_frame_init(&info_frame);
    mpp_assert(NULL == mpp_frame_get_buffer(info_frame));
    mpp_frame_set_eos(info_frame, 1);
    mpp_put_frame((Mpp*)mpp, info_frame);
    return;
}

static void mpp_dec_push_display(Mpp *mpp)
{
    RK_S32 index;
    MppDec *dec = mpp->mDec;
    MppBufSlots frame_slots = dec->frame_slots;
    mpp->mThreadHal->lock(THREAD_QUE_DISPLAY);
    while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
        MppFrame frame = NULL;
        mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
        if (!dec->reset_flag) {
            mpp_put_frame(mpp, frame);
        } else {
            /* release extra ref in slot's MppBuffer */
            MppBuffer buffer = mpp_frame_get_buffer(frame);
            if (buffer)
                mpp_buffer_put(buffer);
        }
        mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
    }
    mpp->mThreadHal->unlock(THREAD_QUE_DISPLAY);
}

static void mpp_dec_push_eos_task(Mpp *mpp, DecTask *task)
{
    hal_task_hnd_set_info(task->hnd, &task->info);
    mpp->mThreadHal->lock();
    hal_task_hnd_set_status(task->hnd, TASK_PROCESSING);
    mpp->mThreadHal->unlock();
    mpp->mThreadHal->signal();
}

static MPP_RET try_proc_dec_task(Mpp *mpp, DecTask *task)
{
    MppDec *dec = mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    size_t stream_size = 0;
    HalDecTask  *task_dec = &task->info.dec;


    /*
     * 1. get task handle from hal for parsing one frame
     */
    if (!task->hnd) {
        hal_task_get_hnd(tasks, TASK_IDLE, &task->hnd);
        if (task->hnd) {
            task->wait.task_hnd = 0;
        } else {
            task->wait.task_hnd = 1;
            return MPP_NOK;
        }
    }


    /*
     * 2. get packet for parser preparing
     */
    if (!dec->mpp_pkt_in && !task->status.curr_task_rdy) {
        mpp_list *packets = mpp->mPackets;
        AutoMutex autoLock(packets->mutex());
        if (packets->list_size()) {
            /*
             * packet will be destroyed outside, here just copy the content
             */
            packets->del_at_head(&dec->mpp_pkt_in, sizeof(dec->mpp_pkt_in));
            mpp->mPacketGetCount++;
            task->wait.mpp_pkt_in = 0;
        } else {
            task->wait.mpp_pkt_in = 1;
            return MPP_NOK;
        }
    }

    /*
     * 3. send packet data to parser for prepare
     *
     *    parser_prepare functioin input / output
     *    input:    input MppPacket data from user
     *    output:   one packet which contains one frame for hardware processing
     *              output information will be stored in task_dec->input_packet
     *              output data can be stored inside of parser.
     *
     *    NOTE:
     *    1. Prepare process is controlled by need_split flag
     *       If need_split flag is zero prepare function is just copy the input
     *       packet to task_dec->input_packet
     *       If need_split flag is non-zero prepare function will call split funciton
     *       of different coding type and find the start and end of one frame. Then
     *       copy data to task_dec->input_packet
     *    2. On need_split mode if one input MppPacket contain multiple frame for
     *       decoding one parser_prepare call will only frame for task. Then input
     *       MppPacket->pos/length will be updated. The input MppPacket will not be
     *       released until it is totally consumed.
     *    3. On spliting frame if one frame contain multiple slices and these multiple
     *       slices have different pts/dts the output frame will use the last pts/dts
     *       as the output frame's pts/dts.
     *
     */
    if (!task->status.curr_task_rdy) {
        RK_S64 p_e, p_s, diff;
        p_s = mpp_time();

        if (mpp_debug & MPP_DBG_PTS)
            mpp_log("input packet pts %lld\n", mpp_packet_get_pts(dec->mpp_pkt_in));

        parser_prepare(dec->parser, dec->mpp_pkt_in, task_dec);
        p_e = mpp_time();
        if (mpp_debug & MPP_DBG_TIMING) {
            diff = (p_e - p_s) / 1000;
            if (diff > 15) {
                mpp_log("waring mpp prepare stream consume %lld big than 15ms ", diff);
            }
        }
        if (0 == mpp_packet_get_length(dec->mpp_pkt_in)) {
            mpp_packet_deinit(&dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }
    }

    task->status.curr_task_rdy = task_dec->valid;
    /*
    * We may find eos in prepare step and there will be no anymore vaild task generated.
    * So here we try push eos task to hal, hal will push all frame to display then
    * push a eos frame to tell all frame decoded
    */
    //  mpp_dec_push_display(mpp);
    if (task_dec->flags.eos && !task_dec->valid) {
        mpp_dec_push_eos_task(mpp, task);
        task->hnd = NULL;
    }

    if (!task->status.curr_task_rdy)
        return MPP_NOK;

    // NOTE: packet in task should be ready now
    mpp_assert(task_dec->input_packet);

    /*
     * 4. look for a unused packet slot index
     */
    if (task_dec->input < 0) {
        mpp_buf_slot_get_unused(packet_slots, &task_dec->input);
    }

    task->wait.dec_pkt_idx = (task_dec->input < 0);
    if (task->wait.dec_pkt_idx)
        return MPP_NOK;

    /*
     * 5. malloc hardware buffer for the packet slot index
     */
    task->hal_pkt_idx_in = task_dec->input;
    stream_size = mpp_packet_get_size(task_dec->input_packet);

    MppBuffer hal_buf_in;
    mpp_buf_slot_get_prop(packet_slots, task->hal_pkt_idx_in, SLOT_BUFFER, &hal_buf_in);
    if (NULL == hal_buf_in) {
        mpp_buffer_get(mpp->mPacketGroup, &hal_buf_in, stream_size);
        if (hal_buf_in) {
            mpp_buf_slot_set_prop(packet_slots, task->hal_pkt_idx_in, SLOT_BUFFER, hal_buf_in);
            mpp_buffer_put(hal_buf_in);
        }
    } else {
        MppBufferImpl *buf = (MppBufferImpl *)hal_buf_in;
        mpp_assert(buf->info.size >= stream_size);
    }

    task->hal_pkt_buf_in = hal_buf_in;
    task->wait.dec_pkt_buf = (NULL == hal_buf_in);
    if (task->wait.dec_pkt_buf)
        return MPP_NOK;

    /*
     * 6. copy prepared stream to hardware buffer
     */
    if (!task->status.dec_pkt_copy_rdy) {
        void *dst = mpp_buffer_get_ptr(task->hal_pkt_buf_in);
        void *src = mpp_packet_get_data(task_dec->input_packet);
        size_t length = mpp_packet_get_length(task_dec->input_packet);
        memcpy(dst, src, length);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        task->status.dec_pkt_copy_rdy = 1;
    }

    /*
     * 7. if not fast mode wait previous task done here
     */
    if (!dec->parser_fast_mode) {
        // wait previous task done
        if (!task->status.prev_task_rdy) {
            HalTaskHnd task_prev = NULL;
            hal_task_get_hnd(tasks, TASK_PROC_DONE, &task_prev);
            if (task_prev) {
                task->status.prev_task_rdy  = 1;
                task->wait.prev_task = 0;
                hal_task_hnd_set_status(task_prev, TASK_IDLE);
                task_prev = NULL;
            } else {
                task->wait.prev_task = 1;
                return MPP_NOK;
            }
        }
    }

    /*
     * 7. look for a unused hardware buffer for output
     */
    if (mpp->mFrameGroup) {
        task->wait.dec_pic_buf = (mpp_buffer_group_unused(mpp->mFrameGroup) < 1);
        if (task->wait.dec_pic_buf)
            return MPP_NOK;
    }

    /*
     * We may find eos in prepare step and there will be no anymore vaild task generated.
     * So here we try push all frames to display to avoid eos no notify to display
     */
    // mpp_dec_push_display(mpp);

    /*
     * 8. send packet data to parser
     *
     *    parser prepare functioin input / output
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
    if (!task->status.task_parsed_rdy) {
        parser_parse(dec->parser, task_dec);
        task->status.task_parsed_rdy = 1;
    }

    /*
     * 9. parse local task and slot to check whether new buffer or info change is needed.
     *
     * a. first detect info change from frame slot
     * b. then detect whether output index has MppBuffer
     */
    if (mpp_buf_slot_is_changed(frame_slots)) {
        if (!task->status.info_task_gen_rdy) {
            task_dec->flags.info_change = 1;
            hal_task_hnd_set_info(task->hnd, &task->info);
            mpp->mThreadHal->lock();
            hal_task_hnd_set_status(task->hnd, TASK_PROCESSING);
            mpp->mThreadHal->unlock();
            mpp->mThreadHal->signal();
            mpp->mTaskPutCount++;
            task->hnd = NULL;
            task->status.info_task_gen_rdy = 1;
            return MPP_NOK;
        }
    }

    task->wait.info_change = mpp_buf_slot_is_changed(frame_slots);
    if (task->wait.info_change) {
        return MPP_NOK;
    } else {
        task->status.info_task_gen_rdy = 0;
        task_dec->flags.info_change = 0;
        // NOTE: check the task must be ready
        mpp_assert(task->hnd);
    }


    if (task_dec->output < 0) {
        /*
         * We may find eos in parser step and there will be no anymore vaild task generated.
         * So here we try push eos task to hal, hal will push all frame to display then
         * push a eos frame to tell all frame decoded
         */
        if (task_dec->flags.eos) {
            mpp_dec_push_eos_task(mpp, task);
        } else {
            hal_task_hnd_set_status(task->hnd, TASK_IDLE);
        }

        mpp->mTaskPutCount++;
        task->hnd = NULL;
        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
            task->status.dec_pkt_copy_rdy = 0;
        }
        task->status.curr_task_rdy  = 0;
        task->status.task_parsed_rdy = 0;
        hal_task_info_init(&task->info, MPP_CTX_DEC);
        return MPP_NOK;
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
    MppBuffer hal_buf_out;
    mpp_buf_slot_get_prop(frame_slots, output, SLOT_BUFFER, &hal_buf_out);
    if (NULL == hal_buf_out) {
        size_t size = mpp_buf_slot_get_size(frame_slots);
        mpp_buffer_get(mpp->mFrameGroup, &hal_buf_out, size);
        if (hal_buf_out)
            mpp_buf_slot_set_prop(frame_slots, output, SLOT_BUFFER, hal_buf_out);

    }

    task->hal_frm_buf_out = hal_buf_out;
    task->wait.dec_pic_buf = (NULL == hal_buf_out);
    if (task->wait.dec_pic_buf)
        return MPP_NOK;

    // register genertation
    mpp_hal_reg_gen(dec->hal, &task->info);

    /*
     * wait previous register set done
     */
    //mpp_hal_hw_wait(dec->hal_ctx, &task_local);

    /*
     * send current register set to hardware
     */
    //mpp_hal_hw_start(dec->hal_ctx, &task_local);

    mpp_hal_hw_start(dec->hal, &task->info);

    /*
     * 6. send dxva output information and buffer information to hal thread
     *    combinate video codec dxva output and buffer information
     */
    hal_task_hnd_set_info(task->hnd, &task->info);
    mpp->mThreadHal->lock();
    hal_task_hnd_set_status(task->hnd, TASK_PROCESSING);
    mpp->mThreadHal->unlock();
    mpp->mThreadHal->signal();

    mpp->mTaskPutCount++;
    task->hnd = NULL;
    task->status.dec_pkt_copy_rdy  = 0;
    task->status.curr_task_rdy  = 0;
    task->status.task_parsed_rdy = 0;
    task->status.prev_task_rdy   = 0;
    hal_task_info_init(&task->info, MPP_CTX_DEC);

    return MPP_OK;
}



void *mpp_dec_parser_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *parser   = mpp->mThreadCodec;
    MppDec    *dec      = mpp->mDec;
    MppBufSlots packet_slots = dec->packet_slots;

    /*
     * parser thread need to wait at cases below:
     * 1. no task slot for output
     * 2. no packet for parsing
     * 3. info change on progress
     * 3. no buffer on analyzing output task
     */
    DecTask task;
    HalDecTask  *task_dec = &task.info.dec;

    dec_task_init(&task);

    while (MPP_THREAD_RUNNING == parser->get_status()) {
        /*
         * wait for stream input
         */
        if (dec->reset_flag) {
            if (reset_dec_task(mpp, &task))
                continue;
        }

        parser->lock();
        if (MPP_THREAD_RUNNING == parser->get_status()) {
            if (check_task_wait(dec, &task))
                parser->wait();
        }
        parser->unlock();


        if (try_proc_dec_task(mpp, &task))
            continue;

    }
    mpp_log("mpp_dec_parser_thread exit");
    if (NULL != task.hnd && task_dec->valid) {
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        mpp_buf_slot_clr_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
    }
    mpp_buffer_group_clear(mpp->mPacketGroup);
    mpp_log("mpp_dec_parser_thread exit ok");
    return NULL;
}

void *mpp_dec_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppThread *hal      = mpp->mThreadHal;
    MppDec    *dec      = mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;

    /*
     * hal thread need to wait at cases below:
     * 1. no task slot for work
     */
    HalTaskHnd  task = NULL;
    HalTaskInfo task_info;
    HalDecTask  *task_dec = &task_info.dec;
    RK_S64 cur_deat = 0;
    RK_U64 dec_no = 0, total_time = 0;
    RK_S64 p_s, p_e;

    p_s = mpp_time();
    while (MPP_THREAD_RUNNING == hal->get_status()) {
        /*
         * hal thread wait for dxva interface intput firt
         */
        hal->lock();
        if (MPP_THREAD_RUNNING == hal->get_status()) {
            if (hal_task_get_hnd(tasks, TASK_PROCESSING, &task))
                hal->wait();
        }
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
                mpp_dec_flush(dec);
                mpp_dec_push_display(mpp);
                mpp_buf_slot_get_prop(frame_slots, task_dec->output, SLOT_FRAME, &info_frame);
                mpp_assert(info_frame);
                mpp_assert(NULL == mpp_frame_get_buffer(info_frame));
                mpp_frame_set_info_change(info_frame, 1);
                mpp_frame_set_errinfo(info_frame, 0);
                mpp_put_frame(mpp, info_frame);

                hal_task_hnd_set_status(task, TASK_IDLE);
                task = NULL;
                mpp->mThreadCodec->signal();
                continue;
            }
            /*
             * check eos task
             * if this task is invalid then eos flag come we will flush display que
             * then push eos frame to tell all frame decoded
             */
            if (task_dec->flags.eos && !task_dec->valid) {
                mpp_dec_push_display(mpp);
                mpp_put_frame_eos(mpp);
                hal_task_hnd_set_status(task, TASK_IDLE);
                mpp->mThreadCodec->signal();
                task = NULL;
                continue;
            }
            mpp_hal_hw_wait(dec->hal, &task_info);
            p_e = mpp_time();
            cur_deat = (p_e - p_s);
            total_time += cur_deat;
            //mpp_log("[Cal_time] dec_no=%lld, time=%d ms, av_time=%lld ms. \n", dec_no, cur_deat, total_time/(dec_no + 1));
            dec_no++;
            p_s = p_e;
            /*
             * when hardware decoding is done:
             * 1. clear decoding flag (mark buffer is ready)
             * 2. use get_display to get a new frame with buffer
             * 3. add frame to output list
             * repeat 2 and 3 until not frame can be output
             */
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);

            // TODO: may have risk here
            hal_task_hnd_set_status(task, TASK_PROC_DONE);
            task = NULL;
            if (dec->parser_fast_mode) {
                hal_task_get_hnd(tasks, TASK_PROC_DONE, &task);
                if (task) {
                    hal_task_hnd_set_status(task, TASK_IDLE);
                }
            }
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
            mpp_dec_push_display(mpp);
            /*
             * check eos task
             * if this task is valid then eos flag come we will flush display que
             * then push eos frame to tell all frame decoded
             */
            if (task_dec->flags.eos) {
                mpp_put_frame_eos(mpp);
            }
        }
    }

    mpp_log("mpp_dec_hal_thread exit ok");
    return NULL;
}

static MPP_RET dec_release_task_in_port(MppPort port)
{
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    MppTask mpp_task;

    do {
        ret = mpp_port_dequeue(port, &mpp_task);
        if (ret || mpp_task == NULL)
            break;

        packet = NULL;
        frame = NULL;
        ret = mpp_task_meta_get_frame(mpp_task, KEY_OUTPUT_FRAME,  &frame);
        if (frame) {
            mpp_frame_deinit(&frame);
            frame = NULL;
        }
        ret = mpp_task_meta_get_packet(mpp_task, KEY_INPUT_PACKET, &packet);
        if (packet) {
            mpp_packet_deinit(&packet);
            packet = NULL;
        }

        mpp_port_enqueue(port, mpp_task);
        mpp_task = NULL;
    } while (1);

    return ret;
}

void *mpp_dec_advanced_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppDec *dec      = mpp->mDec;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    MppThread *thd_dec  = mpp->mThreadCodec;
    DecTask task;   /* decoder task */
    DecTask *pTask = &task;
    dec_task_init(pTask);
    HalDecTask  *task_dec = &pTask->info.dec;

    MppPort input  = mpp_task_queue_get_port(mpp->mInputTaskQueue,  MPP_PORT_OUTPUT);
    MppPort output = mpp_task_queue_get_port(mpp->mOutputTaskQueue, MPP_PORT_INPUT);
    MppTask mpp_task = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;

    while (MPP_THREAD_RUNNING == thd_dec->get_status()) {
        thd_dec->lock();
        ret = mpp_port_dequeue(input, &mpp_task);
        if (ret || NULL == mpp_task) {
            thd_dec->wait();
        }
        thd_dec->unlock();

        if (mpp_task != NULL) {
            mpp_task_meta_get_packet(mpp_task, KEY_INPUT_PACKET, &packet);
            mpp_task_meta_get_frame (mpp_task, KEY_OUTPUT_FRAME,  &frame);

            if (NULL == packet) {
                mpp_port_enqueue(input, mpp_task);
                continue;
            }

            if (mpp_packet_get_buffer(packet)) {
                /*
                 * if there is available buffer in the input packet do decoding
                 */
                MppBuffer input_buffer = mpp_packet_get_buffer(packet);
                MppBuffer output_buffer = mpp_frame_get_buffer(frame);

                parser_prepare(dec->parser, packet, task_dec);

                /*
                 * We may find eos in prepare step and there will be no anymore vaild task generated.
                 * So here we try push eos task to hal, hal will push all frame to display then
                 * push a eos frame to tell all frame decoded
                 */
                if (task_dec->flags.eos && !task_dec->valid) {
                    mpp_frame_init(&frame);
                    mpp_frame_set_eos(frame, 1);
                    goto DEC_OUT;
                }

                /*
                 *  look for a unused packet slot index
                 */
                if (task_dec->input < 0) {
                    mpp_buf_slot_get_unused(packet_slots, &task_dec->input);
                }
                mpp_buf_slot_set_prop(packet_slots, task_dec->input, SLOT_BUFFER, input_buffer);
                mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
                mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);

                ret = parser_parse(dec->parser, task_dec);
                if (ret != MPP_OK) {
                    mpp_err_f("something wrong with parser_parse!\n");
                    mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
                    mpp_frame_init(&frame);
                    goto DEC_OUT;
                }

                if (mpp_buf_slot_is_changed(frame_slots)) {
                    size_t slot_size = mpp_buf_slot_get_size(frame_slots);
                    size_t buffer_size = mpp_buffer_get_size(output_buffer);

                    if (slot_size == buffer_size) {
                        mpp_buf_slot_ready(frame_slots);
                    }

                    mpp_assert(slot_size == buffer_size);
                }

                mpp_buf_slot_set_prop(frame_slots, task_dec->output, SLOT_BUFFER, output_buffer);

                // register genertation
                mpp_hal_reg_gen(dec->hal, &pTask->info);
                mpp_hal_hw_start(dec->hal, &pTask->info);
                mpp_hal_hw_wait(dec->hal, &pTask->info);

                MppFrame tmp = NULL;
                mpp_buf_slot_get_prop(frame_slots, task_dec->output, SLOT_FRAME_PTR, &tmp);
                mpp_frame_set_width(frame, mpp_frame_get_width(tmp));
                mpp_frame_set_height(frame, mpp_frame_get_height(tmp));
                mpp_frame_set_hor_stride(frame, mpp_frame_get_hor_stride(tmp));
                mpp_frame_set_ver_stride(frame, mpp_frame_get_ver_stride(tmp));
                mpp_frame_set_pts(frame, mpp_frame_get_pts(tmp));
                mpp_frame_set_fmt(frame, mpp_frame_get_fmt(tmp));

                mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
                mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);
            } else {
                /*
                 * else init a empty frame for output
                 */
                mpp_log_f("line(%d): Error! Get no buffer from input packet\n", __LINE__);
                mpp_frame_init(&frame);
            }

            /*
             * first clear output packet
             * then enqueue task back to input port
             * final user will release the mpp_frame they had input
             */
        DEC_OUT:
            mpp_task_meta_set_packet(mpp_task, KEY_INPUT_PACKET, packet);
            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            // send finished task to output port
            mpp_port_dequeue(output, &mpp_task);
            mpp_task_meta_set_frame(mpp_task, KEY_OUTPUT_FRAME, frame);

            // setup output task here
            mpp_port_enqueue(output, mpp_task);
            mpp_task = NULL;
            packet = NULL;
            frame = NULL;

            hal_task_info_init(&pTask->info, MPP_CTX_DEC);
        }
    }

    // clear remain task in output port
    dec_release_task_in_port(input);
    dec_release_task_in_port(mpp->mOutputPort);

    return NULL;
}

MPP_RET mpp_dec_init(MppDec **dec, MppDecCfg *cfg)
{
    MPP_RET ret;
    MppCodingType coding;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    Parser parser = NULL;
    MppHal hal = NULL;
    RK_S32 hal_task_count = 0;
    MppDec *p = NULL;
    IOInterruptCB cb = {NULL, NULL};

    if (NULL == dec || NULL == cfg) {
        mpp_err_f("invalid input dec %p cfg %p\n", dec, cfg);
        return MPP_ERR_NULL_PTR;
    }

    *dec = NULL;

    p = mpp_calloc(MppDec, 1);
    if (NULL == p) {
        mpp_err_f("failed to malloc context\n");
        return MPP_ERR_MALLOC;
    }

    coding = cfg->coding;
    hal_task_count = (cfg->fast_mode) ? (3) : (2);

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

        mpp_buf_slot_setup(packet_slots, hal_task_count);
        cb.callBack = mpp_dec_notify;
        cb.opaque = p;
        ParserCfg parser_cfg = {
            coding,
            frame_slots,
            packet_slots,
            hal_task_count,
            cfg->need_split,
            cfg->internal_pts,
            cb,
        };

        ret = parser_init(&parser, &parser_cfg);
        if (ret) {
            mpp_err_f("could not init parser\n");
            break;
        }
        cb.callBack = hal_callback;
        cb.opaque = parser;
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
            cfg->fast_mode,
            cb,
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

        p->mpp                  = cfg->mpp;
        p->parser_need_split    = cfg->need_split;
        p->parser_fast_mode     = cfg->fast_mode;
        p->parser_internal_pts  = cfg->internal_pts;
        *dec = p;
        return MPP_OK;
    } while (0);

    mpp_dec_deinit(p);
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

MPP_RET mpp_dec_notify(void *ctx, void *info)
{
    MppDec *dec  = (MppDec *)ctx;
    MppFrame info_frame = NULL;
    mpp_frame_init(&info_frame);
    mpp_assert(NULL == mpp_frame_get_buffer(info_frame));
    mpp_frame_set_eos(info_frame, 1);
    mpp_put_frame((Mpp*)dec->mpp, info_frame);
    (void)info;
    return MPP_OK;
}

MPP_RET mpp_dec_control(MppDec *dec, MpiCmd cmd, void *param)
{
    if (NULL == dec) {
        mpp_err_f("found NULL input dec %p\n", dec);
        return MPP_ERR_NULL_PTR;
    }
    parser_control(dec->parser, cmd, param);
    mpp_hal_control(dec->hal, cmd, param);

    switch (cmd) {
    case MPP_DEC_SET_FRAME_INFO : {
        MppFrame frame = (MppFrame)param;

        mpp_log("setting default w %4d h %4d h_str %4d v_str %4d\n",
                mpp_frame_get_width(frame),
                mpp_frame_get_height(frame),
                mpp_frame_get_hor_stride(frame),
                mpp_frame_get_ver_stride(frame));

        mpp_slots_set_prop(dec->frame_slots, SLOTS_FRAME_INFO, frame);
    } break;
    case MPP_DEC_GET_VPUMEM_USED_COUNT: {
        RK_S32 *p = (RK_S32 *)param;
        *p = mpp_buf_slot_get_used_size(dec->frame_slots);
    } break;
    default : {
    } break;
    }



    return MPP_OK;
}

