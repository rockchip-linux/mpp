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


#define MODULE_TAG "hal_h264d_api"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef RKPLATFORM
#include <dlfcn.h>
#include <unistd.h>
#endif

#include "rk_type.h"
#include "mpp_device.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_platform.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"

#include "hal_h264d_global.h"
#include "hal_h264d_api.h"

#include "hal_h264d_rkv_reg.h"
#include "hal_h264d_vdpu2.h"
#include "hal_h264d_vdpu1.h"

RK_U32 rkv_h264d_hal_debug = 0;

static void explain_input_buffer(void *hal, HalDecTask *task)
{
    RK_U32 i = 0;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    DXVA2_DecodeBufferDesc *pdes = (DXVA2_DecodeBufferDesc *)task->syntax.data;
    for (i = 0; i < task->syntax.number; i++) {
        switch (pdes[i].CompressedBufferType) {
        case DXVA2_PictureParametersBufferType:
            p_hal->pp = (DXVA_PicParams_H264_MVC *)pdes[i].pvPVPState;
            break;
        case DXVA2_InverseQuantizationMatrixBufferType:
            p_hal->qm = (DXVA_Qmatrix_H264 *)pdes[i].pvPVPState;
            break;
        case DXVA2_SliceControlBufferType:
            p_hal->slice_num = pdes[i].DataSize / sizeof(DXVA_Slice_H264_Long);
            p_hal->slice_long = (DXVA_Slice_H264_Long *)pdes[i].pvPVPState;
            break;
        case DXVA2_BitStreamDateBufferType:
            p_hal->bitstream = (RK_U8 *)pdes[i].pvPVPState;
            p_hal->strm_len = pdes[i].DataSize;
            break;
        default:
            break;
        }
    }
}

static MPP_RET init_hard_platform(H264dHalCtx_t *p_hal, RK_U32 hard_mode)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U32 hard_platform = 0;
    MppHalApi *p_api = &p_hal->hal_api;
    if ((p_hal->vcodec_type & HAVE_RKVDEC)
        && (hard_mode != VDPU1_MODE)
        && (hard_mode != VDPU2_MODE)) {
        hard_mode = RKVDEC_MODE;
    } else if (p_hal->vcodec_type & HAVE_VPU1) {
        hard_mode = VDPU1_MODE;
    } else if (p_hal->vcodec_type & HAVE_VPU2) {
        hard_mode = VDPU2_MODE;
    }
    H264D_DBG(H264D_DBG_HARD_MODE, "set_mode=%d, hw_spt=%08x, use_mode=%d\n",
              hard_mode, p_hal->vcodec_type, hard_mode);
    p_hal->hard_mode = hard_mode;
    switch (hard_mode) {
    case RKVDEC_MODE:
        p_api->init    = rkv_h264d_init;
        p_api->deinit  = rkv_h264d_deinit;
        p_api->reg_gen = rkv_h264d_gen_regs;
        p_api->start   = rkv_h264d_start;
        p_api->wait    = rkv_h264d_wait;
        p_api->reset   = rkv_h264d_reset;
        p_api->flush   = rkv_h264d_flush;
        p_api->control = rkv_h264d_control;
        hard_platform  = HAVE_RKVDEC;
        break;
    case VDPU1_MODE:
        p_api->init    = vdpu1_h264d_init;
        p_api->deinit  = vdpu1_h264d_deinit;
        p_api->reg_gen = vdpu1_h264d_gen_regs;
        p_api->start   = vdpu1_h264d_start;
        p_api->wait    = vdpu1_h264d_wait;
        p_api->reset   = vdpu1_h264d_reset;
        p_api->flush   = vdpu1_h264d_flush;
        p_api->control = vdpu1_h264d_control;
        hard_platform  = HAVE_VPU1;
        break;
    case VDPU2_MODE:
        p_api->init    = vdpu2_h264d_init;
        p_api->deinit  = vdpu2_h264d_deinit;
        p_api->reg_gen = vdpu2_h264d_gen_regs;
        p_api->start   = vdpu2_h264d_start;
        p_api->wait    = vdpu2_h264d_wait;
        p_api->reset   = vdpu2_h264d_reset;
        p_api->flush   = vdpu2_h264d_flush;
        p_api->control = vdpu2_h264d_control;
        hard_platform  = HAVE_VPU2;
        break;
    default:
        mpp_err_f("hard mode error, value=%d\n", p_hal->hard_mode);
        ret = MPP_NOK;
        goto __FAILED;
        break;
    }

    //!< mpp_device_init
#ifdef RKPLATFORM
    if (p_hal->vpu_socket <= 0) {
        mpp_device_control(&p_hal->dev_ctx, MPP_DEV_SET_HARD_PLATFORM, &hard_platform);
        p_hal->vpu_socket = mpp_device_init(&p_hal->dev_ctx, MPP_CTX_DEC, MPP_VIDEO_CodingAVC);
        if (p_hal->vpu_socket <= 0) {
            mpp_err("p_hal->vpu_socket <= 0\n");
            ret = MPP_ERR_UNKNOW;
            goto __FAILED;
        }
    }
#endif

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
MPP_RET hal_h264d_init(void *hal, MppHalCfg *cfg)
{
    MppHalApi *p_api = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    INP_CHECK(ret, NULL == p_hal);
    memset(p_hal, 0, sizeof(H264dHalCtx_t));

    p_api = &p_hal->hal_api;

    p_hal->frame_slots  = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;

    //!< callback function to parser module
    p_hal->init_cb = cfg->hal_int_cb;
    mpp_env_get_u32("rkv_h264d_debug", &rkv_h264d_hal_debug, 0);
    //< get buffer group
#ifdef RKPLATFORM
    FUN_CHECK(ret = mpp_buffer_group_get_internal
                    (&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
#else
    FUN_CHECK(ret = mpp_buffer_group_get_internal
                    (&p_hal->buf_group, MPP_BUFFER_TYPE_NORMAL));
#endif
    mpp_env_get_u32("use_mpp_mode", &p_hal->hard_mode, RKVDEC_MODE);
    p_hal->vcodec_type = mpp_get_vcodec_type();
    mpp_assert(p_hal->vcodec_type & (HAVE_RKVDEC | HAVE_VPU1 | HAVE_VPU2));

    //!< init hard platform
    FUN_CHECK(ret = init_hard_platform(p_hal, p_hal->hard_mode));

    //!< run init funtion
    FUN_CHECK(ret = p_api->init(hal, cfg));

__RETURN:
    return MPP_OK;
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
MPP_RET hal_h264d_deinit(void *hal)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    FUN_CHECK(ret = p_hal->hal_api.deinit(hal));
    //!< mpp_device_init
#ifdef RKPLATFORM
    if (p_hal->vpu_socket >= 0) {
        mpp_device_deinit(p_hal->vpu_socket);
    }
#endif
    if (p_hal->buf_group) {
        FUN_CHECK(ret = mpp_buffer_group_put(p_hal->buf_group));
    }

    return MPP_OK;
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
MPP_RET hal_h264d_gen_regs(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    explain_input_buffer(hal, &task->dec);
    return p_hal->hal_api.reg_gen(hal, task);
}

/*!
***********************************************************************
* \brief h
*    start hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_start(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.start(hal, task);
}

/*!
***********************************************************************
* \brief
*    wait hard
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_wait(void *hal, HalTaskInfo *task)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.wait(hal, task);
}

/*!
***********************************************************************
* \brief
*    reset
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_reset(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.reset(hal);
}

/*!
***********************************************************************
* \brief
*    flush
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_flush(void *hal)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    return p_hal->hal_api.flush(hal);
}

/*!
***********************************************************************
* \brief
*    control
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_control(void *hal, RK_S32 cmd_type, void *param)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

    if (cmd_type == MPP_DEC_CHANGE_HARD_MODE) {
        RK_U32 hard_mode = *((RK_U32 *)param);
        p_hal->hal_api.deinit(hal);
        init_hard_platform(p_hal, hard_mode);
        p_hal->hal_api.init(hal, NULL);
    }
    return p_hal->hal_api.control(hal, cmd_type, param);
}


const MppHalApi hal_api_h264d = {
    .name = "h264d_rkdec",
    .type = MPP_CTX_DEC,
    .coding = MPP_VIDEO_CodingAVC,
    .ctx_size = sizeof(H264dHalCtx_t),
    .flag = 0,
    .init = hal_h264d_init,
    .deinit = hal_h264d_deinit,
    .reg_gen = hal_h264d_gen_regs,
    .start = hal_h264d_start,
    .wait = hal_h264d_wait,
    .reset = hal_h264d_reset,
    .flush = hal_h264d_flush,
    .control = hal_h264d_control,
};

