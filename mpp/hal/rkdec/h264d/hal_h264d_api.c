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
#include "vpu.h"
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
#include "hal_h264d_vdpu_reg.h"
#include "hal_h264d_vdpu1_reg.h"

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


/*!
***********************************************************************
* \brief
*    VPUClientGetIOMMUStatus
***********************************************************************
*/
//extern "C"
#ifndef RKPLATFORM
RK_S32 VPUClientGetIOMMUStatus()
{
    return 0;
}
#endif
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
    VPU_CLIENT_TYPE vpu_client = VPU_DEC_RKV;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    VpuHardMode hard_mode = MODE_NULL;

    INP_CHECK(ret, NULL == p_hal);
    memset(p_hal, 0, sizeof(H264dHalCtx_t));

    p_api = &p_hal->hal_api;

    p_hal->frame_slots  = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;

    //!< choose hard mode
    {
        RK_U32 mode = 0;
        RK_U32 hw_flag = 0;
        mpp_env_get_u32("use_mpp_mode", &mode, 0);
        hw_flag = mpp_get_vcodec_hw_flag();
        mpp_assert(hw_flag & (HAVE_RKVDEC | HAVE_VPU1 | HAVE_VPU2));
        if ((mode <= RKVDEC_MODE) && (hw_flag & HAVE_RKVDEC)) {
            hard_mode = RKVDEC_MODE;
        } else if (hw_flag & HAVE_VPU1) {
            hard_mode = VDPU1_MODE;
        } else if (hw_flag & HAVE_VPU2) {
            hard_mode = VDPU2_MODE;
        }
        H264D_DBG(H264D_DBG_HARD_MODE, "set_mode=%d, hw_spt=%08x, use_mode=%d\n",
                  mode, hw_flag, hard_mode);
    }
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
        cfg->device_id = HAL_RKVDEC;
        vpu_client     = VPU_DEC_RKV;
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
        cfg->device_id = HAL_VDPU;
        vpu_client     = VPU_DEC;
        break;
    case VDPU2_MODE:
        p_api->init    = vdpu_h264d_init;
        p_api->deinit  = vdpu_h264d_deinit;
        p_api->reg_gen = vdpu_h264d_gen_regs;
        p_api->start   = vdpu_h264d_start;
        p_api->wait    = vdpu_h264d_wait;
        p_api->reset   = vdpu_h264d_reset;
        p_api->flush   = vdpu_h264d_flush;
        p_api->control = vdpu_h264d_control;
        cfg->device_id = HAL_VDPU;
        vpu_client     = VPU_DEC;
        break;
    default:
        mpp_err_f("hard mode error, value=%d\n", hard_mode);
        mpp_assert(0);
        break;
    }
    //!< callback function to parser module
    p_hal->init_cb = cfg->hal_int_cb;
    mpp_env_get_u32("rkv_h264d_debug", &rkv_h264d_hal_debug, 0);
    //!< VPUClientInit
#ifdef RKPLATFORM
    if (p_hal->vpu_socket <= 0) {
        p_hal->vpu_socket = VPUClientInit(vpu_client);
        if (p_hal->vpu_socket <= 0) {
            mpp_err("p_hal->vpu_socket <= 0\n");
            ret = MPP_ERR_UNKNOW;
            goto __FAILED;
        }
    }
#endif
    //< get buffer group
    if (p_hal->buf_group == NULL) {
#ifdef RKPLATFORM
        mpp_log_f("mpp_buffer_group_get_internal used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&p_hal->buf_group, MPP_BUFFER_TYPE_NORMAL));
#endif
    }

    //!< run init funtion
    FUN_CHECK(ret = p_api->init(hal, cfg));
    (void)vpu_client;
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
    //!< VPUClientInit
#ifdef RKPLATFORM
    if (p_hal->vpu_socket >= 0) {
        VPUClientRelease(p_hal->vpu_socket);
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

    return p_hal->hal_api.control(hal, cmd_type, param);
}


const MppHalApi hal_api_h264d = {
    "h264d_rkdec",
    MPP_CTX_DEC,
    MPP_VIDEO_CodingAVC,
    sizeof(H264dHalCtx_t),
    0,
    hal_h264d_init,
    hal_h264d_deinit,
    hal_h264d_gen_regs,
    hal_h264d_start,
    hal_h264d_wait,
    hal_h264d_reset,
    hal_h264d_flush,
    hal_h264d_control,
};

