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


#define MODULE_TAG "hal_avsd_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rk_type.h"
#include "vpu.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"

#include "hal_avsd_api.h"
#include "hal_avsd_reg.h"


RK_U32 avsd_hal_debug = 0;

static RK_U32 avsd_ver_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static RK_U32 avsd_hor_align(RK_U32 val)
{
    return MPP_ALIGN(val, 16);
}

static RK_U32 avsd_len_align(RK_U32 val)
{
    return (2 * MPP_ALIGN(val, 16));
}


static void explain_input_buffer(AvsdHalCtx_t *p_hal, HalDecTask *task)
{

    memcpy(&p_hal->syn, task->syntax.data, sizeof(AvsdSyntax_t));

}
static MPP_RET repeat_other_field(AvsdHalCtx_t *p_hal, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdRegs_t *p_regs = (AvsdRegs_t *)p_hal->p_regs;

    //!< update syntax
    p_hal->data_offset = p_regs->sw12.rlc_vlc_base >> 10;
    //p_hal->syn.bitstream_size -= p_hal->data_offset;
    mpp_log_f("stream_size=%d, data_offset=%d\n", p_hal->data_offset);
    //!< re-generate register
    memset(p_hal->p_regs, 0, sizeof(AvsdRegs_t));
    FUN_CHECK(ret = set_defalut_parameters(p_hal));
    FUN_CHECK(ret = set_regs_parameters(p_hal, &task->dec));
    hal_avsd_start((void *)p_hal, task);
    hal_avsd_wait((void *)p_hal, task);

    return ret = MPP_OK;
__FAILED:
    return ret;

}
/*!
***********************************************************************
* \brief
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_init(void *decoder, MppHalCfg *cfg)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = NULL;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    mpp_env_get_u32("avsd_debug", &avsd_hal_debug, 0);

    p_hal = (AvsdHalCtx_t *)decoder;
    memset(p_hal, 0, sizeof(AvsdHalCtx_t));

    p_hal->frame_slots = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;
    //!< callback function to parser module
    p_hal->init_cb = cfg->hal_int_cb;
    //!< VPUClientInit
#ifdef RKPLATFORM
    if (p_hal->vpu_socket <= 0) {
        p_hal->vpu_socket = VPUClientInit(VPU_DEC_AVS);
        if (p_hal->vpu_socket <= 0) {
            mpp_err("p_hal->vpu_socket <= 0\n");
            ret = MPP_ERR_UNKNOW;
            goto __FAILED;
        }
    }
#endif
    //< get buffer group
    if (p_hal->buf_group == NULL) {
        RK_U32 buf_size = 0;
#ifdef RKPLATFORM
        mpp_log_f("mpp_buffer_group_get_internal used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_NORMAL));
#endif
        buf_size = (1920 * 1088) * 2;
        FUN_CHECK(ret = mpp_buffer_get(p_hal->buf_group, &p_hal->mv_buf, buf_size));
    }
    mpp_log_f("p_hal->buf_group=%p \n", p_hal->buf_group);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_HOR_ALIGN, avsd_hor_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_VER_ALIGN, avsd_ver_align);
    mpp_slots_set_prop(p_hal->frame_slots, SLOTS_LEN_ALIGN, avsd_len_align);

    p_hal->p_regs = mpp_calloc_size(RK_U32 , sizeof(AvsdRegs_t));
    MEM_CHECK(ret, p_hal->p_regs);

    //!< initial for control
    p_hal->first_field = 1;
    p_hal->prev_pic_structure = 0; //!< field

    memset(p_hal->pic, 0, sizeof(p_hal->pic));
    p_hal->work_out = -1;
    p_hal->work0 = -1;
    p_hal->work1 = -1;

__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    deinit
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_deinit(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    //!< VPUClientInit
#ifdef RKPLATFORM
    if (p_hal->vpu_socket >= 0) {
        VPUClientRelease(p_hal->vpu_socket);
    }
#endif
    if (p_hal->mv_buf) {
        FUN_CHECK(ret = mpp_buffer_put(p_hal->mv_buf));
    }
    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }
    MPP_FREE(p_hal->p_regs);

__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief
*    generate register
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_gen_regs(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;
    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    mpp_log_f("p_hal->p_regs=%p\n", p_hal->p_regs);
    memset(p_hal->p_regs, 0, sizeof(AvsdRegs_t));
    explain_input_buffer(p_hal, &task->dec);
    p_hal->data_offset = 0;
    FUN_CHECK(ret = set_defalut_parameters(p_hal));
    FUN_CHECK(ret = set_regs_parameters(p_hal, &task->dec));

__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
__FAILED:
    return ret;
}
/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_start(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    if (task->dec.flags.had_error) {
        goto __RETURN;
    }
#ifdef RKPLATFORM
    if (VPUClientSendReg(p_hal->vpu_socket, p_hal->p_regs, AVSD_REGISTERS)) {
        ret = MPP_ERR_VPUHW;
        mpp_err_f("Avs decoder FlushRegs fail. \n");
    }
#endif

__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_wait(void *decoder, HalTaskInfo *task)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");
    INP_CHECK(ret, NULL == decoder);

    if (task->dec.flags.had_error) {
        goto __SKIP_HARD;
    }
#ifdef RKPLATFORM
    {
        RK_S32 wait_ret = -1;
        RK_S32 ret_len = 0;
        VPU_CMD_TYPE ret_cmd = VPU_CMD_BUTT;
        wait_ret = VPUClientWaitResult(p_hal->vpu_socket, p_hal->p_regs, AVSD_REGISTERS, &ret_cmd, &ret_len);
        (void)wait_ret;
        (void)ret_len;
        (void)ret_cmd;
    }
#endif

__SKIP_HARD:
    if (p_hal->init_cb.callBack) {
        IOCallbackCtx m_ctx = { 0 };
        m_ctx.device_id = HAL_VDPU;
        if (!((AvsdRegs_t *)p_hal->p_regs)->sw01.dec_rdy_int) {
            m_ctx.hard_err = 1;
        }
        m_ctx.task = (void *)&task->dec;
        m_ctx.regs = (RK_U32 *)p_hal->p_regs;
        p_hal->init_cb.callBack(p_hal->init_cb.opaque, &m_ctx);
    }
    update_parameters(p_hal);
    memset(&p_hal->p_regs[1], 0, sizeof(RK_U32));
    if (!p_hal->first_field
        && p_hal->syn.pp.pictureStructure == FIELDPICTURE) {
        repeat_other_field(p_hal, task);
    }

    p_hal->frame_no++;
__RETURN:
    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_reset(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdHalCtx_t *p_hal = (AvsdHalCtx_t *)decoder;

    AVSD_HAL_TRACE("In.");

    memset(p_hal->p_regs, 0, sizeof(AvsdRegs_t));

    p_hal->first_field = 1;
    p_hal->prev_pic_structure = 0; //!< field

    memset(p_hal->pic, 0, sizeof(p_hal->pic));
    p_hal->work_out = -1;
    p_hal->work0 = -1;
    p_hal->work1 = -1;


    AVSD_HAL_TRACE("Out.");

    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_flush(void *decoder)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)decoder;
    return ret = MPP_OK;
}
/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_avsd_control(void *decoder, RK_S32 cmd_type, void *param)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    AVSD_HAL_TRACE("In.");

    (void)decoder;
    (void)cmd_type;
    (void)param;

    return ret = MPP_OK;
}


const MppHalApi hal_api_avsd = {
    "avsd_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingAVS,
    sizeof(AvsdHalCtx_t),
    0,
    hal_avsd_init,
    hal_avsd_deinit,
    hal_avsd_gen_regs,
    hal_avsd_start,
    hal_avsd_wait,
    hal_avsd_reset,
    hal_avsd_flush,
    hal_avsd_control,
};

