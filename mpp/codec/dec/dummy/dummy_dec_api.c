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

#include "dummy_dec_api.h"

#define DUMMY_DEC_FRAME_SIZE        SZ_1M
#define DUMMY_DEC_FRAME_COUNT       16
#define DUMMY_DEC_REF_COUNT         2

typedef struct DummyDec_t {
    MppBufSlots     slots;
    RK_U32          task_count;

    RK_U32          slots_inited;
    RK_U32          frame_count;
    RK_S32          prev_index;
    RK_S32          slot_index[DUMMY_DEC_REF_COUNT];
} DummyDec;

MPP_RET dummy_dec_init(void *dec, ParserCfg *cfg)
{
    DummyDec *p;
    RK_S32 i;
    if (NULL == dec) {
        mpp_err_f("found NULL intput dec %p cfg %p\n", dec, cfg);
        return MPP_ERR_NULL_PTR;
    }

    p = (DummyDec *)dec;
    p->slots        = cfg->slots;
    p->task_count   = cfg->task_count = 2;
    p->slots_inited = 0;
    p->frame_count  = 0;
    for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
        p->slot_index[i] = -1;
    }
    return MPP_OK;
}

MPP_RET dummy_dec_deinit(void *dec)
{
    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }
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


MPP_RET dummy_dec_parse(void *dec, MppPacket pkt, HalDecTask *task)
{
    DummyDec *p;
    RK_U32 output;
    MppFrame frame;
    RK_U32 frame_count;
    MppBufSlots slots;
    RK_S32 i;

    if (NULL == dec) {
        mpp_err_f("found NULL intput\n");
        return MPP_ERR_NULL_PTR;
    }

    p = (DummyDec *)dec;
    // do packet decoding here

    slots = p->slots;
    frame_count = p->frame_count;

    // set packet size
    mpp_packet_set_size(pkt, 0);

    /*
     * set slots information
     * 1. output index MUST be set
     * 2. get unused index for output if needed
     * 3. set output index as decoding
     * 4. set pts to output index
     * 5. if one frame can be display, it SHOULD be set display
     */
    if (!p->slots_inited) {
        mpp_buf_slot_setup(slots, DUMMY_DEC_FRAME_COUNT, DUMMY_DEC_FRAME_SIZE, 0);
        p->slots_inited = 1;
    }

    mpp_frame_init(&frame);
    mpp_frame_set_pts(frame, mpp_packet_get_pts(pkt));
    mpp_buf_slot_get_unused(slots, &output);
    mpp_buf_slot_set_hw_dst(slots, output, frame);
    mpp_frame_deinit(&frame);
    mpp_assert(NULL == frame);

    /*
     * setup output task
     * 1. valid flag MUST be set if need hardware to run once
     * 2. set output slot index
     * 3. set reference slot index
     */
    task->valid     = 1;
    task->output    = output;

    memset(&task->refer, -1, sizeof(task->refer));
    for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
        RK_S32 index = p->slot_index[i];
        if (index >= 0) {
            task->refer[i] = index;
            mpp_buf_slot_inc_hw_ref(slots, index);
            mpp_buf_slot_set_dpb_ref(slots, index);
        }
    }

    /*
     * update dpb status assuming that hw has decoded the frame
     */
    mpp_buf_slot_set_display(slots, output);

    // add new reference buffer
    if (mpp_packet_get_eos(pkt)) {
        for (i = 0; i < DUMMY_DEC_REF_COUNT; i++) {
            mpp_buf_slot_clr_dpb_ref(slots, p->slot_index[i]);
            p->slot_index[i] = -1;
        }
    } else {
        // clear unreference buffer
        if (p->slot_index[frame_count] >= 0)
            mpp_buf_slot_clr_dpb_ref(slots, p->slot_index[frame_count]);

        p->slot_index[frame_count] = output;
        mpp_buf_slot_set_dpb_ref(slots, output);
    }

    frame_count++;
    if (frame_count >= DUMMY_DEC_REF_COUNT)
        frame_count = 0;
    p->frame_count = frame_count;

    return MPP_OK;
}

const ParserApi dummy_dec_parser = {
    "dummy_dec_parser",
    MPP_VIDEO_CodingUnused,
    sizeof(DummyDec),
    0,
    dummy_dec_init,
    dummy_dec_deinit,
    NULL,
    dummy_dec_parse,
    dummy_dec_reset,
    dummy_dec_flush,
    dummy_dec_control,
};
