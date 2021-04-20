/*
 * Copyright 2022 Rockchip Electronics Co. LTD
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

#define  MODULE_TAG "mpp_dec_nt"

#include <string.h>

#include "mpp_buffer_impl.h"

#include "mpp_dec_debug.h"
#include "mpp_dec_vproc.h"
#include "mpp_dec_no_thread.h"

MPP_RET mpp_dec_decode(MppDec ctx, MppPacket packet, MppFrame *frame)
{
    MppDecImpl *dec = (MppDecImpl *)ctx;
    Mpp *mpp = (Mpp *)dec->mpp;
    DecTask *task = (DecTask *)dec->task_single;
    MppBufSlots frame_slots = dec->frame_slots;
    MppBufSlots packet_slots = dec->packet_slots;
    HalDecTask  *task_dec = &task->info.dec;
    MppMutexCond *cmd_lock = dec->cmd_lock;
    MppBuffer hal_buf_in = NULL;
    MppBuffer hal_buf_out = NULL;
    size_t stream_size = 0;
    RK_S32 output = 0;

    AutoMutex auto_lock(cmd_lock->mutex());

    if (dec->mpp_pkt_in == NULL && !task->status.curr_task_rdy)
        dec->mpp_pkt_in = packet;

    if (!task->status.curr_task_rdy) {
        if (dec->mpp_pkt_in == NULL)
            return MPP_OK;

        mpp_parser_prepare(dec->parser, dec->mpp_pkt_in, task_dec);

        if (0 == mpp_packet_get_length(dec->mpp_pkt_in)) {
            // mpp_packet_deinit(&dec->mpp_pkt_in);
            dec->mpp_pkt_in = NULL;
        }

        if (!task_dec->valid) {
            if (task_dec->flags.eos) {
                mpp_dec_flush(dec);
                mpp_dec_push_display(mpp, task_dec->flags);
                /*
                * Use -1 as invalid buffer slot index.
                * Reason: the last task maybe is a empty task with eos flag
                * only but this task may go through vproc process also. We need
                * create a buffer slot index for it.
                */
                mpp_dec_put_frame(mpp, -1, task_dec->flags);
            }
            return MPP_OK;
        }
    }

    task->status.curr_task_rdy = task_dec->valid;

    // NOTE: packet in task should be ready now
    mpp_assert(task_dec->input_packet);

    /*
     * 4. look for a unused packet slot index
     */
    if (task_dec->input < 0) {
        mpp_buf_slot_get_unused(packet_slots, &task_dec->input);
    }

    /*
     * 5. malloc hardware buffer for the packet slot index
     */
    task->hal_pkt_idx_in = task_dec->input;
    stream_size = mpp_packet_get_size(task_dec->input_packet);

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
    if (!task->status.dec_pkt_copy_rdy) {
        void *dst = mpp_buffer_get_ptr(task->hal_pkt_buf_in);
        void *src = mpp_packet_get_data(task_dec->input_packet);
        size_t length = mpp_packet_get_length(task_dec->input_packet);
        memcpy(dst, src, length);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);
        task->status.dec_pkt_copy_rdy = 1;
    }

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
            mpp_dec_flush(dec);
            mpp_dec_push_display(mpp, task_dec->flags);
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
            mpp_dec_flush(dec);
            mpp_dec_push_display(mpp, task_dec->flags);
            mpp_dec_put_frame(mpp, task_dec->output, task_dec->flags);
            task_dec->flags.eos = eos;
            task->status.info_task_gen_rdy = 1;
            return MPP_OK;
        }
        dec->info_updated = 0;
    }

    task->wait.info_change = mpp_buf_slot_is_changed(frame_slots);
    if (task->wait.info_change) {
        return MPP_OK;
    } else {
        task->status.info_task_gen_rdy = 0;
        task_dec->flags.info_change = 0;
        // NOTE: check the task must be ready
        // mpp_assert(task->hnd);
    }

    /* 10. whether the frame buffer group is internal or external */
    if (NULL == mpp->mFrameGroup) {
        mpp_log("mpp_dec use internal frame buffer group\n");
        mpp_buffer_group_get_internal(&mpp->mFrameGroup, MPP_BUFFER_TYPE_ION);
    }

    /* 10.1 look for a unused hardware buffer for output */
    if (mpp->mFrameGroup) {
        RK_S32 unused = mpp_buffer_group_unused(mpp->mFrameGroup);

        // NOTE: When dec post-process is enabled reserve 2 buffer for it.
        task->wait.dec_pic_unusd = (dec->vproc) ? (unused < 3) : (unused < 1);
        if (task->wait.dec_pic_unusd)
            cmd_lock->wait();
    }
    dec_dbg_detail("detail: %p check frame group count pass\n", dec);

    output = task_dec->output;
    mpp_buf_slot_get_prop(frame_slots, output, SLOT_BUFFER, &hal_buf_out);
    if (NULL == hal_buf_out) {
        size_t size = mpp_buf_slot_get_size(frame_slots);
        mpp_buffer_get(mpp->mFrameGroup, &hal_buf_out, size);
        if (hal_buf_out)
            mpp_buf_slot_set_prop(frame_slots, output, SLOT_BUFFER,
                                  hal_buf_out);
    }
    if (!dec->info_updated && dec->dev) {
        MppFrame slot_frm = NULL;

        mpp_buf_slot_get_prop(frame_slots, output, SLOT_FRAME_PTR, &slot_frm);
        update_dec_hal_info(dec, slot_frm);
        dec->info_updated = 1;
    }

    task->hal_frm_buf_out = hal_buf_out;
    task->wait.dec_pic_match = (NULL == hal_buf_out);
    if (task->wait.dec_pic_match)
        return MPP_NOK;

    mpp_hal_reg_gen(dec->hal, &task->info);
    mpp_hal_hw_start(dec->hal, &task->info);
    mpp_hal_hw_wait(dec->hal, &task->info);
    dec->dec_hw_run_count++;
    /*
     * when hardware decoding is done:
     * 1. clear decoding flag (mark buffer is ready)
     * 2. use get_display to get a new frame with buffer
     * 3. add frame to output list
     * repeat 2 and 3 until not frame can be output
     */
    mpp_buf_slot_clr_flag(packet_slots, task_dec->input, SLOT_HAL_INPUT);

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

    (void)frame;

    task->status.dec_pkt_copy_rdy  = 0;
    task->status.curr_task_rdy  = 0;
    task->status.task_parsed_rdy = 0;
    task->status.prev_task_rdy   = 0;
    dec_task_info_init(&task->info);
    return MPP_OK;
}

MPP_RET mpp_dec_reset_no_thread(MppDecImpl *dec)
{
    DecTask *task = (DecTask *)dec->task_single;
    MppBufSlots frame_slots  = dec->frame_slots;
    MppMutexCond *cmd_lock = dec->cmd_lock;
    HalDecTask *task_dec = &task->info.dec;
    RK_S32 index;

    AutoMutex auto_lock(cmd_lock->mutex());

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

    // wait hal thread reset ready
    if (task->wait.info_change) {
        mpp_log("reset at info change status\n");
        mpp_buf_slot_reset(frame_slots, task_dec->output);
    }

    if (task->status.task_parsed_rdy) {
        mpp_log("task no send to hal que must clr current frame hal status\n");
        mpp_buf_slot_clr_flag(frame_slots, task_dec->output, SLOT_HAL_OUTPUT);
        for (RK_U32 i = 0; i < MPP_ARRAY_ELEMS(task_dec->refer); i++) {
            index = task_dec->refer[i];
            if (index >= 0)
                mpp_buf_slot_clr_flag(frame_slots, index, SLOT_HAL_INPUT);
        }
        task->status.task_parsed_rdy = 0;
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
        mpp_buf_slot_clr_flag(dec->packet_slots, task_dec->input,  SLOT_HAL_INPUT);
        task->status.dec_pkt_copy_rdy = 0;
        task_dec->input = -1;
    }

    task->status.task_parsed_rdy = 0;
    dec_task_init(task);

    dec_dbg_reset("reset: parser reset all done\n");

    dec->dec_in_pkt_count = 0;
    dec->dec_hw_run_count = 0;
    dec->dec_out_frame_count = 0;
    dec->info_updated = 0;

    cmd_lock->signal();

    return MPP_OK;
}

MPP_RET mpp_dec_notify_no_thread(MppDecImpl *dec, RK_U32 flag)
{
    // Only notify buffer group control
    if (flag == (MPP_DEC_NOTIFY_BUFFER_VALID | MPP_DEC_NOTIFY_BUFFER_MATCH)) {
        dec->cmd_lock->signal();
        return MPP_OK;
    }

    return MPP_OK;
}

MPP_RET mpp_dec_control_no_thread(MppDecImpl *dec, MpiCmd cmd, void *param)
{
    // cmd_lock is used to sync all async operations
    AutoMutex auto_lock(dec->cmd_lock->mutex());

    dec->cmd_send++;
    return mpp_dec_proc_cfg(dec, cmd, param);
}

MppDecModeApi dec_api_no_thread = {
    NULL,
    NULL,
    mpp_dec_reset_no_thread,
    mpp_dec_notify_no_thread,
    mpp_dec_control_no_thread,
};
