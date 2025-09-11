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

#include "mpp_buffer_impl.h"

#include "mpp_dec_debug.h"
#include "mpp_dec_vproc.h"
#include "mpp_dec_normal.h"
#include "rk_hdr_meta_com.h"

static RK_S32 ts_cmp(void *priv, const struct list_head *a, const struct list_head *b)
{
    MppPktTs *ts1, *ts2;
    (void)priv;

    ts1 = container_of(a, MppPktTs, link);
    ts2 = container_of(b, MppPktTs, link);

    return ts1->pts - ts2->pts;
}

/*
 * return MPP_OK for not wait
 * return MPP_NOK for wait
 */
static MPP_RET check_task_wait(MppDecImpl *dec, DecTask *task)
{
    MPP_RET ret = MPP_OK;
    RK_U32 notify = dec->parser_notify_flag;
    RK_U32 last_wait = dec->parser_wait_flag;
    RK_U32 curr_wait = task->wait.val;
    RK_U32 wait_chg  = last_wait & (~curr_wait);
    RK_U32 keep_notify = 0;

    do {
        if (dec->reset_flag)
            break;

        // NOTE: User control should always be processed
        if (notify & MPP_DEC_CONTROL) {
            keep_notify = notify & (~MPP_DEC_CONTROL);
            break;
        }

        // NOTE: When condition is not fulfilled check nofify flag again
        if (!curr_wait || (curr_wait & notify))
            break;

        // wait for condition
        ret = MPP_NOK;
    } while (0);

    dec_dbg_status("%p %08x -> %08x [%08x] notify %08x -> %s\n", dec,
                   last_wait, curr_wait, wait_chg, notify, (ret) ? ("wait") : ("work"));

    dec->parser_status_flag = task->status.val;
    dec->parser_wait_flag = task->wait.val;
    dec->parser_notify_flag = keep_notify;

    if (ret) {
        dec->parser_wait_count++;
    } else {
        dec->parser_work_count++;
    }

    return ret;
}

static MPP_RET dec_release_task_in_port(MppPort port)
{
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame frame = NULL;
    MppTask mpp_task;

    do {
        ret = mpp_port_poll(port, MPP_POLL_NON_BLOCK);
        if (ret < 0)
            break;

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
        if (packet && NULL == mpp_packet_get_buffer(packet)) {
            mpp_packet_deinit(&packet);
            packet = NULL;
        }

        mpp_port_enqueue(port, mpp_task);
        mpp_task = NULL;
    } while (1);

    return ret;
}

static void dec_release_input_packet(MppDecImpl *dec, RK_S32 force)
{
    if (dec->mpp_pkt_in) {
        if (force || 0 == mpp_packet_get_length(dec->mpp_pkt_in)) {
            mpp_packet_deinit(&dec->mpp_pkt_in);

            mpp_dec_callback(dec, MPP_DEC_EVENT_ON_PKT_RELEASE, dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }
    }
}

static RK_U32 reset_parser_thread(Mpp *mpp, DecTask *task)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppThread *hal = dec->thread_hal;
    HalTaskGroup tasks  = dec->tasks;
    MppBufSlots frame_slots  = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    HalDecTask *task_dec = &task->info.dec;

    dec_dbg_reset("reset: parser reset start\n");
    dec_dbg_reset("reset: parser wait hal proc reset start\n");

    dec_release_task_in_port(mpp->mMppInPort);

    mpp_assert(hal);

    mpp_thread_lock(hal, THREAD_WORK);
    dec->hal_reset_post++;
    mpp_thread_signal(hal, THREAD_WORK);
    mpp_thread_unlock(hal, THREAD_WORK);

    sem_wait(&dec->hal_reset);

    dec_dbg_reset("reset: parser check hal proc task empty start\n");

    if (hal_task_check_empty(tasks, TASK_PROCESSING)) {
        mpp_err_f("found task not processed put %d get %d\n",
                  mpp->mTaskPutCount, mpp->mTaskGetCount);
        mpp_abort();
    }

    dec_dbg_reset("reset: parser check hal proc task empty done\n");

    // do parser reset process
    {
        RK_S32 index;
        task->status.curr_task_rdy = 0;
        task->status.prev_task_rdy = 1;
        task_dec->valid = 0;
        mpp_parser_reset(dec->parser);
        mpp_hal_reset(dec->hal);
        if (dec->vproc) {
            dec_dbg_reset("reset: vproc reset start\n");
            dec_vproc_reset(dec->vproc);
            dec_dbg_reset("reset: vproc reset done\n");
        }

        if (task->status.task_parsed_rdy) {
            mpp_log("task no send to hal que must clr current frame hal status\n");
            if (task_dec->output >= 0)
                mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);

            for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
                index = task_dec->refer[i];
                if (index >= 0)
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_HAL_INPUT);
            }
            task->status.task_parsed_rdy = 0;
        }

        dec_release_input_packet(dec, 1);

        while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
            /* release extra ref in slot's MppBuffer */
            MppBuffer buffer = NULL;
            mpp_buf_slot_get_prop(frame_slots, index, SLOT_BUFFER, &buffer);
            if (buffer)
                mpp_buffer_put(buffer);
            mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
        }

        if (dec->cfg->base.sort_pts) {
            // flush
            MppPktTs *ts, *pos;

            mpp_spinlock_lock(&dec->ts_lock);
            list_for_each_entry_safe(ts, pos, &dec->ts_link, MppPktTs, link) {
                list_del_init(&ts->link);
                mpp_mem_pool_put_f(dec->ts_pool, ts);
            }
            mpp_spinlock_unlock(&dec->ts_lock);
        }

        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
            task->status.dec_pkt_copy_rdy = 0;
            task_dec->input = -1;
        }

        // wait hal thread reset ready
        if (task->wait.info_change)
            mpp_log("reset at info change\n");

        task->status.task_parsed_rdy = 0;
        // IMPORTANT: clear flag in MppDec context
        dec->parser_status_flag = 0;
        dec->parser_wait_flag = 0;
    }

    dec_task_init(task);

    dec_dbg_reset("reset: parser reset all done\n");

    return MPP_OK;
}

static void mpp_dec_put_task(Mpp *mpp, DecTask *task)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;

    hal_task_hnd_set_info(task->hnd, &task->info);
    mpp_thread_lock(dec->thread_hal, THREAD_WORK);
    hal_task_hnd_set_status(task->hnd, TASK_PROCESSING);
    mpp->mTaskPutCount++;
    mpp_thread_signal(dec->thread_hal, THREAD_WORK);
    mpp_thread_unlock(dec->thread_hal, THREAD_WORK);
    task->hnd = NULL;
}

static void reset_hal_thread(Mpp *mpp)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    HalTaskGroup tasks = dec->tasks;
    MppBufSlots frame_slots = dec->frame_slots;
    HalDecTaskFlag flag;
    RK_S32 index = -1;
    HalTaskHnd  task = NULL;

    /* when hal thread reset output all frames */
    flag.val = 0;
    mpp_dec_flush(dec);

    mpp_thread_lock(dec->thread_hal, THREAD_OUTPUT);
    while (MPP_OK == mpp_buf_slot_dequeue(frame_slots, &index, QUEUE_DISPLAY)) {
        mpp_dec_put_frame(mpp, index, flag);
        mpp_buf_slot_clr_flag(frame_slots, index, SLOT_QUEUE_USE);
    }

    // Need to set processed task to idle status
    while (MPP_OK == hal_task_get_hnd(tasks, TASK_PROC_DONE, &task)) {
        if (task) {
            hal_task_hnd_set_status(task, TASK_IDLE);
            task = NULL;
        }
    }

    mpp_thread_unlock(dec->thread_hal, THREAD_OUTPUT);
}

static MPP_RET try_get_input_packet(Mpp *mpp, DecTask *task)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppPort input = mpp->mMppInPort;
    MppTask mpp_task = NULL;
    MppPacket packet = NULL;
    MPP_RET ret = MPP_OK;

    ret = mpp_port_poll(input, MPP_POLL_NON_BLOCK);
    if (ret < 0) {
        task->wait.dec_pkt_in = 1;
        return MPP_NOK;
    }

    ret = mpp_port_dequeue(input, &mpp_task);
    mpp_assert(ret == MPP_OK && mpp_task);
    mpp_task_meta_get_packet(mpp_task, KEY_INPUT_PACKET, &packet);
    mpp_assert(packet);

    /* when it is copy buffer return packet right here */
    if (NULL == mpp_packet_get_buffer(packet))
        mpp_port_enqueue(input, mpp_task);

    dec->mpp_pkt_in = packet;
    mpp->mPacketGetCount++;
    dec->dec_in_pkt_count++;

    task->status.mpp_pkt_in_rdy = 1;
    task->wait.dec_pkt_in = 0;

    return MPP_OK;
}

static MPP_RET try_proc_dec_task(Mpp *mpp, DecTask *task)
{
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    HalTaskGroup tasks  = dec->tasks;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    HalDecTask  *task_dec = &task->info.dec;
    MppBuffer hal_buf_in = NULL;
    MppBuffer hal_buf_out = NULL;
    size_t stream_size = 0;
    RK_S32 output = 0;

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
        if (try_get_input_packet(mpp, task))
            return MPP_NOK;

        mpp_assert(dec->mpp_pkt_in);

        dec_dbg_detail("detail: %p get pkt pts %llu len %d\n", dec,
                       mpp_packet_get_pts(dec->mpp_pkt_in),
                       mpp_packet_get_length(dec->mpp_pkt_in));
    }

    /*
     * 3. send packet data to parser for prepare
     *
     *    mpp_parser_prepare functioin input / output
     *    input:    input MppPacket data from user
     *    output:   one packet which contains one frame for hardware processing
     *              output information will be stored in task_dec->input_packet
     *              output data can be stored inside of parser.
     *
     *    NOTE:
     *    1. Prepare process is controlled by need_split flag
     *       If need_split flag is zero prepare function is just copy the input
     *       packet to task_dec->input_packet
     *       If need_split flag is non-zero prepare function will call split function
     *       of different coding type and find the start and end of one frame. Then
     *       copy data to task_dec->input_packet
     *    2. On need_split mode if one input MppPacket contain multiple frame for
     *       decoding one mpp_parser_prepare call will only frame for task. Then input
     *       MppPacket->pos/length will be updated. The input MppPacket will not be
     *       released until it is totally consumed.
     *    3. On spliting frame if one frame contain multiple slices and these multiple
     *       slices have different pts/dts the output frame will use the last pts/dts
     *       as the output frame's pts/dts.
     *
     */
    if (!task->status.curr_task_rdy) {
        mpp_dbg_pts("input packet pts %lld\n", mpp_packet_get_pts(dec->mpp_pkt_in));

        mpp_clock_start(dec->clocks[DEC_PRS_PREPARE]);
        mpp_parser_prepare(dec->parser, dec->mpp_pkt_in, task_dec);
        mpp_clock_pause(dec->clocks[DEC_PRS_PREPARE]);
        if (dec->cfg->base.sort_pts && task_dec->valid) {
            task->ts_cur.pts = mpp_packet_get_pts(dec->mpp_pkt_in);
            task->ts_cur.dts = mpp_packet_get_dts(dec->mpp_pkt_in);
        }
        dec_release_input_packet(dec, 0);
    }

    task->status.curr_task_rdy = task_dec->valid;
    /*
    * We may find eos in prepare step and there will be no anymore vaild task generated.
    * So here we try push eos task to hal, hal will push all frame to display then
    * push a eos frame to tell all frame decoded
    */
    if (task_dec->flags.eos && !task_dec->valid)
        mpp_dec_put_task(mpp, task);

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
    stream_size = mpp_packet_get_size(task_dec->input_packet);

    mpp_buf_slot_get_prop(packet_slots, task_dec->input, SLOT_BUFFER, &hal_buf_in);
    if (NULL == hal_buf_in) {
        mpp_buffer_get(mpp->mPacketGroup, &hal_buf_in, stream_size);
        if (hal_buf_in) {
            mpp_buf_slot_set_prop(packet_slots, task_dec->input, SLOT_BUFFER, hal_buf_in);
            mpp_buffer_attach_dev(hal_buf_in, dec->dev);
            mpp_buffer_put(hal_buf_in);
        }
    } else {
        MppBufferImpl *buf = (MppBufferImpl *)hal_buf_in;
        mpp_assert(buf->info.size >= stream_size);
    }

    task->wait.dec_pkt_buf = (NULL == hal_buf_in);
    if (task->wait.dec_pkt_buf)
        return MPP_NOK;

    /*
     * 6. copy prepared stream to hardware buffer
     */
    if (!task->status.dec_pkt_copy_rdy) {
        void *src = mpp_packet_get_data(task_dec->input_packet);
        size_t length = mpp_packet_get_length(task_dec->input_packet);

        mpp_buffer_write(hal_buf_in, 0, src, length);
        mpp_buffer_sync_partial_end(hal_buf_in, 0, length);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        task->status.dec_pkt_copy_rdy = 1;
    }

    /* 7.1 if not fast mode wait previous task done here */
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

    // for vp9 only wait all task is processed
    if (task->wait.dec_all_done) {
        if (!hal_task_check_empty(dec->tasks, TASK_PROCESSING))
            task->wait.dec_all_done = 0;
        else
            return MPP_NOK;
    }

    dec_dbg_detail("detail: %p check prev task pass\n", dec);

    /* too many frame delay in dispaly queue */
    if (mpp->mFrmOut) {
        task->wait.dis_que_full = (mpp_list_size(mpp->mFrmOut) > 4) ? 1 : 0;
        if (task->wait.dis_que_full)
            return MPP_ERR_DISPLAY_FULL;
    }
    dec_dbg_detail("detail: %p check mframes pass\n", dec);

    /* 7.3 wait for a unused slot index for decoder parse operation */
    task->wait.dec_slot_idx = (mpp_slots_get_unused_count(frame_slots)) ? (0) : (1);
    if (task->wait.dec_slot_idx)
        return MPP_ERR_BUFFER_FULL;

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
     *    4. detect whether output index has MppBuffer and task valid
     */
    if (!task->status.task_parsed_rdy) {
        mpp_clock_start(dec->clocks[DEC_PRS_PARSE]);
        mpp_parser_parse(dec->parser, task_dec);
        mpp_clock_pause(dec->clocks[DEC_PRS_PARSE]);
        task->status.task_parsed_rdy = 1;
    }

    if (task_dec->output < 0 || !task_dec->valid) {
        /*
         * We may meet an eos in parser step and there will be no anymore vaild
         * task generated. So here we try push eos task to hal, hal will push
         * all frame(s) to display, a frame of them with a eos flag will be
         * used to inform that all frame have decoded
         */
        if (task_dec->flags.eos) {
            mpp_dec_put_task(mpp, task);
        } else {
            hal_task_hnd_set_status(task->hnd, TASK_IDLE);
            task->hnd = NULL;
        }

        if (task->status.dec_pkt_copy_rdy) {
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
            task->status.dec_pkt_copy_rdy = 0;
        }
        task->status.curr_task_rdy  = 0;
        task->status.task_parsed_rdy = 0;
        dec_task_info_init(&task->info);
        return MPP_NOK;
    }
    dec_dbg_detail("detail: %p check output index pass\n", dec);

    /*
     * 9. parse local task and slot to check whether new buffer or info change is needed.
     *
     * detect info change from frame slot
     */
    if (mpp_buf_slot_is_changed(frame_slots)) {
        if (!task->status.info_task_gen_rdy) {
            RK_U32 eos = task_dec->flags.eos;

            // NOTE: info change should not go with eos flag
            task_dec->flags.info_change = 1;
            task_dec->flags.eos = 0;
            mpp_dec_put_task(mpp, task);
            task_dec->flags.eos = eos;

            task->status.info_task_gen_rdy = 1;
            return MPP_ERR_STREAM;
        }
        dec->info_updated = 0;
    }

    task->wait.info_change = mpp_buf_slot_is_changed(frame_slots);
    if (task->wait.info_change) {
        return MPP_ERR_STREAM;
    } else {
        task->status.info_task_gen_rdy = 0;
        task_dec->flags.info_change = 0;
        // NOTE: check the task must be ready
        mpp_assert(task->hnd);
    }

    /* 10. whether the frame buffer group is internal or external */
    if (NULL == mpp->mFrameGroup) {
        mpp_log("mpp_dec use internal frame buffer group\n");
        mpp_buffer_group_get_internal(&mpp->mFrameGroup, MPP_BUFFER_TYPE_ION | MPP_BUFFER_FLAGS_CACHABLE);
    }

    /* 10.1 look for a unused hardware buffer for output */
    if (mpp->mFrameGroup) {
        RK_S32 unused = mpp_buffer_group_unused(mpp->mFrameGroup);

        // NOTE: When dec post-process is enabled reserve 2 buffer for it.
        task->wait.dec_pic_unusd = (dec->vproc) ? (unused < 3) : (unused < 1);
        if (task->wait.dec_pic_unusd)
            return MPP_ERR_BUFFER_FULL;
    }
    dec_dbg_detail("detail: %p check frame group count pass\n", dec);

    /*
     * 11. do buffer operation according to usage information
     *
     *    possible case:
     *    a. normal case
     *       - wait and alloc(or fetch) a normal frame buffer
     *    b. field mode case
     *       - two field may reuse a same buffer, no need to alloc
     *    c. info change case
     *       - need buffer in different side, need to send a info change
     *         frame to hal loop.
     */
    output = task_dec->output;
    mpp_buf_slot_get_prop(frame_slots, output, SLOT_BUFFER, &hal_buf_out);
    if (NULL == hal_buf_out) {
        MppFrame mframe = NULL;
        mpp_buf_slot_get_prop(frame_slots, output,
                              SLOT_FRAME_PTR, &mframe);
        size_t size = mpp_buf_slot_get_size(frame_slots);

        if (mframe && mpp_frame_get_thumbnail_en(mframe) == MPP_FRAME_THUMBNAIL_ONLY) {
            // only for 8K thumbnail downscale to 4K 8bit mode
            RK_U32 downscale_width = mpp_frame_get_width(mframe) / 2;
            RK_U32 downscale_height = mpp_frame_get_height(mframe) / 2;

            size = downscale_width * downscale_height * 3 / 2;
        }
        mpp_buffer_get(mpp->mFrameGroup, &hal_buf_out, size);
        if (hal_buf_out)
            mpp_buf_slot_set_prop(frame_slots, output, SLOT_BUFFER,
                                  hal_buf_out);
    }

    dec_dbg_detail("detail: %p check output buffer %p\n", dec, hal_buf_out);

    {
        MppFrame mframe = NULL;

        mpp_buf_slot_get_prop(frame_slots, output, SLOT_FRAME_PTR, &mframe);

        if (MPP_FRAME_FMT_IS_HDR(mpp_frame_get_fmt(mframe)) &&
            dec->cfg->base.enable_hdr_meta) {
            fill_hdr_meta_to_frame(mframe, dec->coding);
        }
    }

    // update codec info
    if (!dec->info_updated && dec->dev) {
        MppFrame frame = NULL;

        mpp_buf_slot_get_prop(frame_slots, output, SLOT_FRAME_PTR, &frame);
        update_dec_hal_info(dec, frame);
        dec->info_updated = 1;
    }

    task->wait.dec_pic_match = (NULL == hal_buf_out);
    if (task->wait.dec_pic_match)
        return MPP_NOK;

    if (dec->cfg->base.sort_pts) {
        MppFrame frame = NULL;
        MppPktTs *pkt_ts = (MppPktTs *)mpp_mem_pool_get_f(dec->ts_pool);

        mpp_assert(pkt_ts);
        mpp_buf_slot_get_prop(frame_slots, output, SLOT_FRAME_PTR, &frame);
        pkt_ts->pts = task->ts_cur.pts;
        pkt_ts->dts = task->ts_cur.dts;
        INIT_LIST_HEAD(&pkt_ts->link);
        if (frame && mpp_frame_get_pts(frame) == pkt_ts->pts) {
            mpp_spinlock_lock(&dec->ts_lock);
            list_add_tail(&pkt_ts->link, &dec->ts_link);
            list_sort(NULL, &dec->ts_link, ts_cmp);
            mpp_spinlock_unlock(&dec->ts_lock);
        }
    }

    /* generating registers table */
    mpp_clock_start(dec->clocks[DEC_HAL_GEN_REG]);
    mpp_hal_reg_gen(dec->hal, &task->info);
    mpp_clock_pause(dec->clocks[DEC_HAL_GEN_REG]);

    /* send current register set to hardware */
    mpp_clock_start(dec->clocks[DEC_HW_START]);
    mpp_hal_hw_start(dec->hal, &task->info);
    mpp_clock_pause(dec->clocks[DEC_HW_START]);

    /*
     * 12. send dxva output information and buffer information to hal thread
     *    combinate video codec dxva output and buffer information
     */
    mpp_dec_put_task(mpp, task);

    task->wait.dec_all_done = (dec->parser_fast_mode &&
                               task_dec->flags.wait_done) ? 1 : 0;

    task->status.dec_pkt_copy_rdy  = 0;
    task->status.curr_task_rdy  = 0;
    task->status.task_parsed_rdy = 0;
    task->status.prev_task_rdy   = 0;
    dec_task_info_init(&task->info);

    dec_dbg_detail("detail: %p one task ready\n", dec);

    return MPP_OK;
}

void *mpp_dec_parser_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppThread *parser = dec->thread_parser;
    MppBufSlots packet_slots = dec->packet_slots;

    DecTask task;
    HalDecTask  *task_dec = &task.info.dec;

    dec_task_init(&task);

    mpp_clock_start(dec->clocks[DEC_PRS_TOTAL]);

    while (1) {
        mpp_thread_lock(parser, THREAD_WORK);
        if (MPP_THREAD_RUNNING != mpp_thread_get_status(parser, THREAD_WORK)) {
            mpp_thread_unlock(parser, THREAD_WORK);
            break;
        }

        /*
         * parser thread need to wait at cases below:
         * 1. no task slot for output
         * 2. no packet for parsing
         * 3. info change on progress
         * 3. no buffer on analyzing output task
         */
        if (check_task_wait(dec, &task)) {
            mpp_clock_start(dec->clocks[DEC_PRS_WAIT]);
            mpp_thread_wait(parser, THREAD_WORK);
            mpp_clock_pause(dec->clocks[DEC_PRS_WAIT]);
        }
        mpp_thread_unlock(parser, THREAD_WORK);

        // process user control
        if (dec->cmd_send != dec->cmd_recv) {
            dec_dbg_detail("ctrl proc %d cmd %08x\n", dec->cmd_recv, dec->cmd);
            sem_wait(&dec->cmd_start);
            *dec->cmd_ret = mpp_dec_proc_cfg(dec, dec->cmd, dec->param);
            dec->cmd_recv++;
            dec_dbg_detail("ctrl proc %d done send %d\n", dec->cmd_recv,
                           dec->cmd_send);
            mpp_assert(dec->cmd_send == dec->cmd_send);
            dec->param = NULL;
            dec->cmd = (MpiCmd)0;
            dec->cmd_ret = NULL;
            sem_post(&dec->cmd_done);
            continue;
        }

        if (dec->reset_flag) {
            reset_parser_thread(mpp, &task);

            mpp_thread_lock(parser, THREAD_CONTROL);
            dec->reset_flag = 0;
            sem_post(&dec->parser_reset);
            mpp_thread_unlock(parser, THREAD_CONTROL);
            continue;
        }

        // NOTE: ignore return value here is to fast response to reset.
        // Otherwise we can loop all dec task until it is failed.
        mpp_clock_start(dec->clocks[DEC_PRS_PROC]);
        try_proc_dec_task(mpp, &task);
        mpp_clock_pause(dec->clocks[DEC_PRS_PROC]);
    }

    mpp_clock_pause(dec->clocks[DEC_PRS_TOTAL]);

    mpp_dbg_info("mpp_dec_parser_thread is going to exit\n");
    if (task.hnd && task_dec->valid) {
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        mpp_buf_slot_clr_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
    }
    mpp_buffer_group_clear(mpp->mPacketGroup);
    dec_release_task_in_port(mpp->mMppInPort);
    mpp_dbg_info("mpp_dec_parser_thread exited\n");
    return NULL;
}

void *mpp_dec_hal_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppThread *hal = dec->thread_hal;
    HalTaskGroup tasks = dec->tasks;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;

    HalTaskHnd  task = NULL;
    HalTaskInfo task_info;
    HalDecTask  *task_dec = &task_info.dec;

    mpp_clock_start(dec->clocks[DEC_HAL_TOTAL]);

    while (1) {
        /* hal thread wait for dxva interface intput first */
        mpp_thread_lock(hal, THREAD_WORK);
        if (MPP_THREAD_RUNNING != mpp_thread_get_status(hal, THREAD_WORK)) {
            mpp_thread_unlock(hal, THREAD_WORK);
            break;
        }

        if (hal_task_get_hnd(tasks, TASK_PROCESSING, &task)) {
            // process all task then do reset process
            if (dec->hal_reset_post != dec->hal_reset_done) {
                dec_dbg_reset("reset: hal reset start\n");
                reset_hal_thread(mpp);
                dec_dbg_reset("reset: hal reset done\n");
                dec->hal_reset_done++;
                sem_post(&dec->hal_reset);
                mpp_thread_unlock(hal, THREAD_WORK);
                continue;
            }

            mpp_dec_notify(dec, MPP_DEC_NOTIFY_TASK_ALL_DONE);
            mpp_clock_start(dec->clocks[DEC_HAL_WAIT]);
            mpp_thread_wait(hal, THREAD_WORK);
            mpp_clock_pause(dec->clocks[DEC_HAL_WAIT]);
            mpp_thread_unlock(hal, THREAD_WORK);
            continue;
        }
        mpp_thread_unlock(hal, THREAD_WORK);

        if (task) {
            RK_U32 notify_flag = MPP_DEC_NOTIFY_TASK_HND_VALID;

            mpp_clock_start(dec->clocks[DEC_HAL_PROC]);
            mpp->mTaskGetCount++;

            hal_task_hnd_get_info(task, &task_info);

            /*
             * check info change flag
             * if this is a frame with that flag, only output an empty
             * MppFrame without any image data for info change.
             */
            if (task_dec->flags.info_change) {
                mpp_dec_flush(dec);
                mpp_dec_push_display(mpp, task_dec->flags);
                mpp_dec_put_frame(mpp, task_dec->output, task_dec->flags);

                hal_task_hnd_set_status(task, TASK_IDLE);
                task = NULL;
                mpp_dec_notify(dec, notify_flag);
                mpp_clock_pause(dec->clocks[DEC_HAL_PROC]);
                continue;
            }
            /*
             * check eos task
             * if this task is invalid while eos flag is set, we will
             * flush display queue then push the eos frame to info that
             * all frames have decoded.
             */
            if (task_dec->flags.eos &&
                (!task_dec->valid || task_dec->output < 0)) {
                mpp_dec_push_display(mpp, task_dec->flags);
                /*
                 * Use -1 as invalid buffer slot index.
                 * Reason: the last task maybe is a empty task with eos flag
                 * only but this task may go through vproc process also. We need
                 * create a buffer slot index for it.
                 */
                mpp_dec_put_frame(mpp, -1, task_dec->flags);

                hal_task_hnd_set_status(task, TASK_IDLE);
                task = NULL;
                mpp_dec_notify(dec, notify_flag);
                mpp_clock_pause(dec->clocks[DEC_HAL_PROC]);
                continue;
            }

            mpp_clock_start(dec->clocks[DEC_HW_WAIT]);
            mpp_hal_hw_wait(dec->hal, &task_info);
            mpp_clock_pause(dec->clocks[DEC_HW_WAIT]);
            dec->dec_hw_run_count++;

            /*
             * when hardware decoding is done:
             * 1. clear decoding flag (mark buffer is ready)
             * 2. use get_display to get a new frame with buffer
             * 3. add frame to output list
             * repeat 2 and 3 until not frame can be output
             */
            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,
                                  SLOT_HAL_INPUT);

            hal_task_hnd_set_status(task, (dec->parser_fast_mode) ?
                                    (TASK_IDLE) : (TASK_PROC_DONE));

            if (dec->parser_fast_mode)
                notify_flag |= MPP_DEC_NOTIFY_TASK_HND_VALID;
            else
                notify_flag |= MPP_DEC_NOTIFY_TASK_PREV_DONE;

            task = NULL;

            if (task_dec->output >= 0)
                mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);

            for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
                RK_S32 index = task_dec->refer[i];
                if (index >= 0)
                    mpp_buf_slot_clr_flag(frame_slots, index, SLOT_HAL_INPUT);
            }
            if (task_dec->flags.eos)
                mpp_dec_flush(dec);
            mpp_dec_push_display(mpp, task_dec->flags);

            mpp_dec_notify(dec, notify_flag);
            mpp_clock_pause(dec->clocks[DEC_HAL_PROC]);
        }
    }

    mpp_clock_pause(dec->clocks[DEC_HAL_TOTAL]);

    mpp_assert(mpp->mTaskPutCount == mpp->mTaskGetCount);
    mpp_dbg_info("mpp_dec_hal_thread exited\n");
    return NULL;
}

void *mpp_dec_advanced_thread(void *data)
{
    Mpp *mpp = (Mpp*)data;
    MppDecImpl *dec = (MppDecImpl *)mpp->mDec;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    MppThread *thd_dec  = dec->thread_parser;
    DecTask task;   /* decoder task */
    DecTask *pTask = &task;
    dec_task_init(pTask);
    HalDecTask  *task_dec = &pTask->info.dec;
    MppPort input  = mpp->mMppInPort;
    MppPort output = mpp->mMppOutPort;
    MppTask mpp_task = NULL;
    MPP_RET ret = MPP_OK;
    MppFrame frame = NULL;
    MppPacket packet = NULL;

    while (1) {
        mpp_thread_lock(thd_dec, THREAD_WORK);
        if (MPP_THREAD_RUNNING != mpp_thread_get_status(thd_dec, THREAD_WORK)) {
            mpp_thread_unlock(thd_dec, THREAD_WORK);
            break;
        }

        if (check_task_wait(dec, &task))
            mpp_thread_wait(thd_dec, THREAD_WORK);
        mpp_thread_unlock(thd_dec, THREAD_WORK);

        // process user control
        if (dec->cmd_send != dec->cmd_recv) {
            dec_dbg_detail("ctrl proc %d cmd %08x\n", dec->cmd_recv, dec->cmd);
            sem_wait(&dec->cmd_start);
            *dec->cmd_ret = mpp_dec_proc_cfg(dec, dec->cmd, dec->param);
            dec->cmd_recv++;
            dec_dbg_detail("ctrl proc %d done send %d\n", dec->cmd_recv,
                           dec->cmd_send);
            mpp_assert(dec->cmd_send == dec->cmd_send);
            dec->param = NULL;
            dec->cmd = (MpiCmd)0;
            dec->cmd_ret = NULL;
            sem_post(&dec->cmd_done);
            continue;
        }

        // 1. check task in
        dec_dbg_detail("mpp_pkt_in_rdy %d\n", task.status.mpp_pkt_in_rdy);
        if (!task.status.mpp_pkt_in_rdy) {
            ret = mpp_port_poll(input, MPP_POLL_NON_BLOCK);
            if (ret < 0) {
                task.wait.dec_pkt_in = 1;
                continue;
            }

            dec_dbg_detail("poll ready\n");

            task.status.mpp_pkt_in_rdy = 1;
            task.wait.dec_pkt_in = 0;

            ret = mpp_port_dequeue(input, &mpp_task);
            mpp_assert(ret == MPP_OK);
        }
        dec_dbg_detail("task in ready\n");

        mpp_assert(mpp_task);

        mpp_task_meta_get_packet(mpp_task, KEY_INPUT_PACKET, &packet);
        mpp_task_meta_get_frame (mpp_task, KEY_OUTPUT_FRAME,  &frame);

        if (!frame && packet) {
            MppMeta meta = mpp_packet_get_meta(packet);

            if (meta) {
                mpp_meta_get_frame(meta, KEY_OUTPUT_FRAME, &frame);
                if (frame)
                    task.status.mpp_in_frm_at_pkt = 1;
            }
        }

        if (NULL == packet || NULL == frame) {
            mpp_port_enqueue(input, mpp_task);
            task.status.mpp_pkt_in_rdy = 0;
            continue;
        }

        if (mpp_packet_get_buffer(packet)) {
            /*
             * if there is available buffer in the input packet do decoding
             */
            MppBuffer input_buffer = mpp_packet_get_buffer(packet);
            MppBuffer output_buffer = mpp_frame_get_buffer(frame);

            mpp_parser_prepare(dec->parser, packet, task_dec);

            /*
             * We may find eos in prepare step and there will be no anymore vaild task generated.
             * So here we try push eos task to hal, hal will push all frame to display then
             * push a eos frame to tell all frame decoded
             */
            if (task_dec->flags.eos && !task_dec->valid) {
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

            ret = mpp_parser_parse(dec->parser, task_dec);
            if (ret != MPP_OK) {
                mpp_err_f("something wrong with mpp_parser_parse!\n");
                mpp_frame_set_errinfo(frame, 1); /* 0 - OK; 1 - error */
                mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
                goto DEC_OUT;
            }

            dec_dbg_detail("slot change %d\n", mpp_buf_slot_is_changed(frame_slots));
            if (mpp_buf_slot_is_changed(frame_slots)) {
                size_t slot_size = mpp_buf_slot_get_size(frame_slots);
                size_t buffer_size = mpp_buffer_get_size(output_buffer);

                dec_dbg_detail("change size required %d vs input %d\n", slot_size, buffer_size);
                if (slot_size == buffer_size) {
                    mpp_buf_slot_ready(frame_slots);
                }

                mpp_assert(slot_size <= buffer_size);

                if (slot_size > buffer_size) {
                    mpp_err_f("required buffer size %d is larger than input buffer size %d\n",
                              slot_size, buffer_size);
                }
            }

            mpp_buf_slot_set_prop(frame_slots, task_dec->output, SLOT_BUFFER, output_buffer);
            // update codec info
            if (!dec->info_updated && dec->dev) {
                MppFrame tmp = NULL;

                mpp_buf_slot_get_prop(frame_slots, task_dec->output, SLOT_FRAME_PTR, &tmp);
                update_dec_hal_info(dec, tmp);
                dec->info_updated = 1;
            }
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
            mpp_frame_set_hor_stride_pixel(frame, mpp_frame_get_hor_stride_pixel(tmp));
            mpp_frame_set_pts(frame, mpp_frame_get_pts(tmp));
            mpp_frame_set_fmt(frame, mpp_frame_get_fmt(tmp));
            mpp_frame_set_errinfo(frame, mpp_frame_get_errinfo(tmp));
            mpp_frame_set_buf_size(frame, mpp_frame_get_buf_size(tmp));

            mpp_buf_slot_clr_flag(packet_slots, task_dec->input,  SLOT_HAL_INPUT);
            mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);
        } else {
            /*
             * else init a empty frame for output
             */
            mpp_log_f("line(%d): Error! Get no buffer from input packet\n", __LINE__);
            mpp_frame_init(&frame);
            mpp_frame_set_errinfo(frame, 1);
        }

        /*
         * first clear output packet
         * then enqueue task back to input port
         * final user will release the mpp_frame they had input
         */
    DEC_OUT:
        if (task.status.mpp_in_frm_at_pkt) {
            MppList *list = mpp->mFrmOut;
            MppMeta meta = mpp_frame_get_meta(frame);

            if (meta)
                mpp_meta_set_packet(meta, KEY_INPUT_PACKET, packet);

            mpp_dbg_pts("output frame pts %lld\n", mpp_frame_get_pts(frame));

            mpp_mutex_cond_lock(&list->cond_lock);
            mpp_list_add_at_tail(list, &frame, sizeof(frame));
            mpp->mFramePutCount++;
            mpp_list_signal(list);
            mpp_mutex_cond_unlock(&list->cond_lock);

            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            task.status.mpp_in_frm_at_pkt = 0;
        } else {
            mpp_task_meta_set_packet(mpp_task, KEY_INPUT_PACKET, packet);
            mpp_port_enqueue(input, mpp_task);
            mpp_task = NULL;

            // send finished task to output port
            mpp_port_poll(output, MPP_POLL_BLOCK);
            mpp_port_dequeue(output, &mpp_task);
            mpp_task_meta_set_frame(mpp_task, KEY_OUTPUT_FRAME, frame);
            mpp_buffer_sync_ro_begin(mpp_frame_get_buffer(frame));

            // setup output task here
            mpp_port_enqueue(output, mpp_task);
            mpp_task = NULL;
        }

        packet = NULL;
        frame = NULL;

        dec_task_info_init(&pTask->info);

        task.status.mpp_pkt_in_rdy = 0;
    }

    // clear remain task in output port
    dec_release_task_in_port(input);
    dec_release_task_in_port(mpp->mUsrInPort);
    dec_release_task_in_port(mpp->mUsrOutPort);

    return NULL;
}

MPP_RET mpp_dec_start_normal(MppDecImpl *dec)
{
    if (dec->coding != MPP_VIDEO_CodingMJPEG) {
        dec->thread_parser = mpp_thread_create(mpp_dec_parser_thread,
                                               dec->mpp, "mpp_dec_parser");
        mpp_thread_start(dec->thread_parser);
        dec->thread_hal = mpp_thread_create(mpp_dec_hal_thread,
                                            dec->mpp, "mpp_dec_hal");

        mpp_thread_start(dec->thread_hal);
    } else {
        dec->thread_parser = mpp_thread_create(mpp_dec_advanced_thread,
                                               dec->mpp, "mpp_dec_parser");
        mpp_thread_start(dec->thread_parser);
    }

    return MPP_OK;
}

MPP_RET mpp_dec_reset_normal(MppDecImpl *dec)
{
    MppThread *parser = dec->thread_parser;

    if (dec->coding != MPP_VIDEO_CodingMJPEG) {
        // set reset flag
        mpp_thread_lock(parser, THREAD_CONTROL);
        dec->reset_flag = 1;
        // signal parser thread to reset
        mpp_dec_notify(dec, MPP_DEC_RESET);
        mpp_thread_unlock(parser, THREAD_CONTROL);
        sem_wait(&dec->parser_reset);
    }

    dec->dec_in_pkt_count = 0;
    dec->dec_hw_run_count = 0;
    dec->dec_out_frame_count = 0;
    dec->info_updated = 0;

    return MPP_OK;
}

MPP_RET mpp_dec_notify_normal(MppDecImpl *dec, RK_U32 flag)
{
    MppThread *thd_dec  = dec->thread_parser;
    RK_U32 notify = 0;

    if (!thd_dec)
        return MPP_NOK;

    mpp_thread_lock(thd_dec, THREAD_WORK);
    if (flag == MPP_DEC_CONTROL) {
        dec->parser_notify_flag |= flag;
        notify = 1;
    } else {
        RK_U32 old_flag = dec->parser_notify_flag;

        dec->parser_notify_flag |= flag;
        if ((old_flag != dec->parser_notify_flag) &&
            (dec->parser_notify_flag & dec->parser_wait_flag))
            notify = 1;
    }

    if (notify) {
        dec_dbg_notify("%p status %08x notify control signal\n", dec,
                       dec->parser_wait_flag, dec->parser_notify_flag);
        mpp_thread_signal(thd_dec, THREAD_WORK);
    }
    mpp_thread_unlock(thd_dec, THREAD_WORK);

    return MPP_OK;
}

MPP_RET mpp_dec_control_normal(MppDecImpl *dec, MpiCmd cmd, void *param)
{
    MPP_RET ret = MPP_OK;
    mpp_mutex_cond_lock(&dec->cmd_lock);

    dec->cmd = cmd;
    dec->param = param;
    dec->cmd_ret = &ret;
    dec->cmd_send++;

    dec_dbg_detail("detail: %p control cmd %08x param %p start disable_thread %d \n",
                   dec, cmd, param, dec->cfg->base.disable_thread);

    mpp_dec_notify_normal(dec, MPP_DEC_CONTROL);
    sem_post(&dec->cmd_start);
    sem_wait(&dec->cmd_done);
    mpp_mutex_cond_unlock(&dec->cmd_lock);

    return ret;
}

MppDecModeApi dec_api_normal = {
    mpp_dec_start_normal,
    NULL,
    mpp_dec_reset_normal,
    mpp_dec_notify_normal,
    mpp_dec_control_normal,
};
