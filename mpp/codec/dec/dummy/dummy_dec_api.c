/*
*
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
#define MODULE_TAG "dummy_dec_api"

#include <string.h>

#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "dummy_dec_api.h"

#define DUMMY_DEC_FRAME_WIDTH       1280
#define DUMMY_DEC_FRAME_HEIGHT      720

#define DUMMY_DEC_FRAME_NEW_WIDTH   1920
#define DUMMY_DEC_FRAME_NEW_HEIGHT  1088

#define DUMMY_DEC_FRAME_SIZE        SZ_1M
#define DUMMY_DEC_FRAME_COUNT       16
#define DUMMY_DEC_REF_COUNT         2

typedef struct DummyDec_t {
    MppBufSlots     frame_slots;
    MppBufSlots     packet_slots;
    RK_S32          task_count;
    void            *stream;
    size_t          stream_size;
    MppPacket       task_pkt;

    RK_S64          task_pts;
    RK_U32          task_eos;

    RK_U32          slots_inited;
    RK_U32          frame_count;
    RK_S32          prev_index;
    RK_S32          slot_index[DUMMY_DEC_REF_COUNT];
} DummyDec;

MPP_RET dummy_dec_init(void *dec, ParserCfg *cfg)
{
    DummyDec *p;
    RK_S32 i;
    void *stream;
    size_t stream_size = SZ_512K;
    MppPacket task_pkt;

    if (NULL == dec) {
        mpp_err_f("found NULL intput dec %p cfg %p\n", dec, cfg);
        return MPP_ERR_NULL_PTR;
    }

    stream = mpp_malloc_size(void, stream_size);
    if (NULL == stream) {
        mpp_err_f("failed to malloc stream buffer size %d\n", stream_size);
        return MPP_ERR_MALLOC;
    }

    mpp_packet_init(&task_pkt, stream, stream_size);
    if (NULL == task_pkt) {
        mpp_err_f("failed to create mpp_packet for task\n");
        return MPP_ERR_UNKNOW;
    }

    p = (DummyDec *)dec;
    p->frame_slots  = cfg->frame_slots;
    p->packet_slots = cfg->packet_slots;
    p->task_count   = cfg->task_count = 2;
    p->stream       = stream;
    p->stream_size  = stream_size;
    p->task_pkt     = task_pkt;
    for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
        p->slot_index[i] = -1;
    }
    return MPP_OK;
}

MPP_RET dummy_dec_deinit(void *dec)
{
    DummyDec *p;
    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (DummyDec *)dec;
    if (p->task_pkt)
        mpp_packet_deinit(&p->task_pkt);
    if (p->stream)
        mpp_free(p->stream);
    return MPP_OK;
}

MPP_RET dummy_dec_reset(void *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}


MPP_RET dummy_dec_flush(void *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }
    return MPP_OK;
}


MPP_RET dummy_dec_control(void *dec, RK_S32 cmd_type, void *param)
{
    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }
    (void)cmd_type;
    (void)param;
    return MPP_OK;
}

MPP_RET dummy_dec_prepare(void *dec, MppPacket pkt, HalDecTask *task)
{
    DummyDec *p;
    RK_U8 *data;
    size_t length;

    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (DummyDec *)dec;

    /*********************************************************************
     * do packet prepare here
     * including connet nals into a big buffer, setup packet for task copy
     * pts/eos record
     *********************************************************************/
    p->task_pts = mpp_packet_get_pts(pkt);
    p->task_eos = mpp_packet_get_eos(pkt);

    // set pos to indicate that buffer is done
    data    = mpp_packet_get_data(pkt);
    length  = mpp_packet_get_length(pkt);
    if (length > p->stream_size) {
        p->stream = mpp_realloc(p->stream, RK_U8, length);
        mpp_packet_set_data(p->task_pkt, p->stream);
        p->stream_size = length;
    }
    if (p->stream) {
        memcpy(p->stream, data, length);
        mpp_packet_set_length(p->task_pkt, length);
    } else {
        mpp_err("failed to found task buffer for hardware\n");
        return MPP_ERR_UNKNOW;
    }
    mpp_packet_set_pos(pkt, data + length);

    /*
     * this step will enable the task and goto parse stage
     */
    task->input_packet = p->task_pkt;
    task->flags.eos = p->task_eos;
    task->valid = 1;
    return MPP_OK;
}
MPP_RET dummy_dec_parse(void *dec, HalDecTask *task)
{
    DummyDec *p;
    RK_S32 output;
    MppFrame frame = NULL;
    RK_U32 frame_count;
    MppBufSlots slots;
    RK_S32 i;
    RK_U32 width, height;

    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }
    p = (DummyDec *)dec;

    slots = p->frame_slots;
    frame_count = p->frame_count;

    width = DUMMY_DEC_FRAME_WIDTH;
    height = DUMMY_DEC_FRAME_HEIGHT;

    mpp_frame_init(&frame);

    if (!p->slots_inited) {
        mpp_buf_slot_setup(slots, DUMMY_DEC_FRAME_COUNT);
        p->slots_inited = 1;
    } else if (frame_count >= 2) {
        // do info change test
        width = DUMMY_DEC_FRAME_NEW_WIDTH;
        height = DUMMY_DEC_FRAME_NEW_HEIGHT;
    }

    mpp_frame_set_width(frame, width);
    mpp_frame_set_height(frame, height);
    mpp_frame_set_hor_stride(frame, MPP_ALIGN(width, 16));
    mpp_frame_set_ver_stride(frame, MPP_ALIGN(height, 16));

    if (task->prev_status) {
        /*
         * if previous task has error happened mark the previous frame to be error
         */
        mpp_err("previous task error found\n");
    }

    /*
     * set slots information
     * 1. output index MUST be set
     * 2. get unused index for output if needed
     * 3. set output index as hal_input
     * 4. set frame information to output index
     * 5. if one frame can be display, it SHOULD be enqueued to display queue
     */
    mpp_buf_slot_get_unused(slots, &output);
    mpp_buf_slot_set_flag(slots, output, SLOT_HAL_OUTPUT);
    task->output = output;

    mpp_frame_set_pts(frame, p->task_pts);
    mpp_buf_slot_set_prop(slots, output, SLOT_FRAME, frame);
    mpp_frame_deinit(&frame);
    mpp_assert(NULL == frame);

    /*
     * setup output task
     * 1. valid flag MUST be set if need hardware to run once
     * 2. set output slot index
     * 3. set reference slot index
     */
    memset(&task->refer, -1, sizeof(task->refer));
    for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
        RK_S32 index = p->slot_index[i];
        if (index >= 0) {
            task->refer[i] = index;
            mpp_buf_slot_set_flag(slots, index, SLOT_HAL_INPUT);
            mpp_buf_slot_set_flag(slots, index, SLOT_CODEC_USE);
        }
    }

    /*
     * update dpb status assuming that hw has decoded the frame
     */
    mpp_buf_slot_set_flag(slots, output, SLOT_QUEUE_USE);
    mpp_buf_slot_enqueue(slots, output, QUEUE_DISPLAY);

    // add new reference buffer
    if (p->task_eos) {
        for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
            mpp_buf_slot_clr_flag(slots, p->slot_index[i], SLOT_CODEC_USE);
            p->slot_index[i] = -1;
        }
    } else {
        // clear unreference buffer
        RK_U32 replace_index = frame_count & 1;
        if (p->slot_index[replace_index] >= 0)
            mpp_buf_slot_clr_flag(slots, p->slot_index[replace_index], SLOT_CODEC_USE);

        p->slot_index[replace_index] = output;
        mpp_buf_slot_set_flag(slots, output, SLOT_CODEC_USE);
    }

    p->frame_count = ++frame_count;

    return MPP_OK;
}

MPP_RET dummy_dec_callback(void *dec, void *err_info)
{
    (void)dec;
    (void)err_info;
    return MPP_OK;
}
const ParserApi dummy_dec_parser = {
    .name = "dummy_dec_parser",
    .coding = MPP_VIDEO_CodingUnused,
    .ctx_size = sizeof(DummyDec),
    .flag = 0,
    .init = dummy_dec_init,
    .deinit = dummy_dec_deinit,
    .prepare = dummy_dec_prepare,
    .parse = dummy_dec_parse,
    .reset = dummy_dec_reset,
    .flush = dummy_dec_flush,
    .control = dummy_dec_control,
    .callback = dummy_dec_callback,
};

