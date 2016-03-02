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
#include "mpp_time.h"
#include "mpp_common.h"

#include "mpp.h"
#include "mpp_frame.h"
#include "mpp_buffer_impl.h"
#include "mpp_packet_impl.h"
#include "mpp_frame_impl.h"


#include "vpu_api.h"

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

    if (!dec->fast_mode) {
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
        if (dec->mpp_pkt_in) {
            mpp_packet_deinit(&dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }
        while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
            MppFrame frame = NULL;
            mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
            mpp_frame_deinit(&frame);
            mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        }
        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_get_prop(packet_slots, task_dec->input,  SLOT_BUFFER, &task->hal_pkt_buf_in);
            if (task->hal_pkt_buf_in) {
                mpp_buffer_put(task->hal_pkt_buf_in);
                task->hal_pkt_buf_in = NULL;
            }
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
    mpp->mFramePutCount++;
    list->signal();
    list->unlock();
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
     * 2. get packet to parse
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
    if (!task->status.curr_task_rdy) {
        parser_prepare(dec->parser, dec->mpp_pkt_in, task_dec);
        if (0 == mpp_packet_get_length(dec->mpp_pkt_in)) {
            mpp_packet_deinit(&dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }
    }

    //if (task_dec->flags.eos && task_dec->valid == 0)
    {
        RK_S32 index;
        while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {

            MppFrame frame = NULL;
            mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
            if (!dec->reset_flag) {
                mpp_put_frame(mpp, frame);
                //mpp_log("discard=%d \n",0);
            } else {
                mpp_frame_deinit(&frame);
                //mpp_log("discard=%d \n",1);
            }
            mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        }
    }

    task->status.curr_task_rdy = task_dec->valid;
    if (!task->status.curr_task_rdy)
        return MPP_NOK;

    // NOTE: packet in task should be ready now
    mpp_assert(task_dec->input_packet);

    // copy prepared stream to hardware buffer
    if (task_dec->input < 0) {
        mpp_buf_slot_get_unused(packet_slots, &task_dec->input);
    }

    task->wait.dec_pkt_idx = (task_dec->input < 0);
    if (task->wait.dec_pkt_idx)
        return MPP_NOK;

    task->hal_pkt_idx_in = task_dec->input;
    stream_size = mpp_packet_get_size(task_dec->input_packet);

    MppBuffer hal_buf_in;
    mpp_buf_slot_get_prop(packet_slots, task->hal_pkt_idx_in, SLOT_BUFFER, &hal_buf_in);
    if (NULL == hal_buf_in) {
        mpp_buffer_get(mpp->mPacketGroup, &hal_buf_in, stream_size);
        if (hal_buf_in)
            mpp_buf_slot_set_prop(packet_slots, task->hal_pkt_idx_in, SLOT_BUFFER, hal_buf_in);
    } else {
        MppBufferImpl *buf = (MppBufferImpl *)hal_buf_in;
        mpp_assert(buf->info.size >= stream_size);
    }

    task->hal_pkt_buf_in = hal_buf_in;
    task->wait.dec_pkt_buf = (NULL == hal_buf_in);
    if (task->wait.dec_pkt_buf)
        return MPP_NOK;

    if (!task->status.dec_pkt_copy_rdy) {
        MppBufferImpl *buf = (MppBufferImpl *)task->hal_pkt_buf_in;
        void *src = mpp_packet_get_data(task_dec->input_packet);
        size_t length = mpp_packet_get_length(task_dec->input_packet);
        memcpy(buf->info.ptr, src, length);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        task->status.dec_pkt_copy_rdy = 1;
    }

    if (!dec->fast_mode) {
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
    if (mpp->mFrameGroup) {
        task->wait.dec_pic_buf = (mpp_buffer_group_unused(mpp->mFrameGroup) < 1);
        if (task->wait.dec_pic_buf)
            return MPP_NOK;
    }

   // if (task_dec->flags.eos && task_dec->valid == 0)
   {

        RK_S32 index;
        mpp->mThreadHal->lock(THREAD_QUE_DISPLAY);
        while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
            MppFrame frame = NULL;
            //RK_U32 display;
            mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
            //display = mpp_frame_get_display(frame);
            if (!dec->reset_flag) {
                mpp_put_frame(mpp, frame);
            } else {
                mpp_frame_deinit(&frame);
            }
            mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        }
        mpp->mThreadHal->unlock(THREAD_QUE_DISPLAY);
    }

    if (!task->status.task_parsed_rdy) {
        parser_parse(dec->parser, task_dec);
        task->status.task_parsed_rdy = 1;
    }

    /*
     * 4. parse local task and slot to check whether new buffer or info change is needed.
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

    // send info change task to hal thread
    if (task_dec->output < 0) {
        hal_task_hnd_set_status(task->hnd, TASK_IDLE);
        mpp->mTaskPutCount++;
        task->hnd = NULL;
        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_get_prop(packet_slots, task_dec->input,  SLOT_BUFFER, &task->hal_pkt_buf_in);
            if (task->hal_pkt_buf_in) {
                mpp_buffer_put(task->hal_pkt_buf_in);
                task->hal_pkt_buf_in = NULL;
            }
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
        if (task.hal_pkt_buf_in)
            mpp_buffer_put(task.hal_pkt_buf_in);
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
#ifdef ANDROID
    RK_S32 cur_deat = 0;
    RK_U64 dec_no = 0, total_time = 0;
    static struct timeval tv1, tv2;
    gettimeofday(&tv1, NULL);
#endif
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
            mpp_hal_hw_wait(dec->hal, &task_info);
#ifdef ANDROID
            gettimeofday(&tv2, NULL);
            cur_deat = (tv2.tv_sec - tv1.tv_sec) * 1000 + (tv2.tv_usec - tv1.tv_usec) / 1000;
            total_time += cur_deat;
            //mpp_log("[Cal_time] dec_no=%lld, time=%d ms, av_time=%lld ms. \n", dec_no, cur_deat, total_time/(dec_no + 1));
            dec_no++;
            tv1 = tv2;
#endif
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
            if (dec->fast_mode) {

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
            {
                RK_S32 index;
                hal->lock(THREAD_QUE_DISPLAY);
                while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
                    MppFrame frame = NULL;
                    //mpp_log("put slot index to dispaly %d",index);
                    mpp_buf_slot_get_prop(frame_slots, index, SLOT_FRAME, &frame);
                    if (!dec->reset_flag) {
                        mpp_put_frame(mpp, frame);
                    } else {
                        mpp_frame_deinit(&frame);
                    }
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
                }
                hal->unlock(THREAD_QUE_DISPLAY);
            }
        }
    }

    mpp_log("mpp_dec_hal_thread exit ok");
    return NULL;
}

MPP_RET mpp_dec_init(MppDec *dec, MppCodingType coding)
{
    MPP_RET ret;
    MppBufSlots frame_slots = NULL;
    MppBufSlots packet_slots = NULL;
    Parser parser = NULL;
    MppHal hal = NULL;
    MppDec *p = dec;
    RK_S32 task_count = 2;
    IOInterruptCB cb = {NULL, NULL};
    if (dec->fast_mode) {
        task_count = 3;
    }
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

        mpp_buf_slot_setup(packet_slots, task_count);
        cb.callBack = mpp_dec_notify;
        cb.opaque = dec;
        ParserCfg parser_cfg = {
            coding,
            frame_slots,
            packet_slots,
            task_count,
            0,
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
            dec->fast_mode,
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

    switch (cmd) {
    case MPP_CODEC_SET_FRAME_INFO : {
        VPU_GENERIC *p = (VPU_GENERIC *)param;
        MppFrame frame = NULL;
        mpp_frame_init(&frame);
        mpp_frame_set_width(frame, p->ImgWidth);
        mpp_frame_set_height(frame, p->ImgHeight);
        mpp_frame_set_hor_stride(frame, p->ImgHorStride);
        mpp_frame_set_ver_stride(frame, p->ImgVerStride);
        mpp_log_f("setting default w %4d h %4d\n", p->ImgWidth, p->ImgHeight);
        mpp_slots_set_prop(dec->frame_slots, SLOTS_FRAME_INFO, frame);
        mpp_frame_deinit(&frame);
    } break;
    default : {
    } break;
    }

    parser_control(dec->parser, cmd, param);
    mpp_hal_control(dec->hal, cmd, param);

    return MPP_OK;
}

