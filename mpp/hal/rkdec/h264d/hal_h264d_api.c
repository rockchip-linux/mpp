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
#include <dlfcn.h>
#include <unistd.h>

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_platform.h"
#include "mpp_common.h"
#include "osal_2str.h"

#include "dxva_syntax.h"
#include "h264d_syntax.h"

#include "hal_h264d_global.h"
#include "hal_h264d_api.h"

#include "hal_h264d_rkv_reg.h"
#include "hal_h264d_vdpu34x.h"
#include "hal_h264d_vdpu2.h"
#include "hal_h264d_vdpu1.h"

RK_U32 hal_h264d_debug = 0;

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
*    init
***********************************************************************
*/
//extern "C"
MPP_RET hal_h264d_init(void *hal, MppHalCfg *cfg)
{
    MppHalApi *p_api = NULL;
    MPP_RET ret = MPP_ERR_UNKNOW;
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;
    MppClientType client_type = VPU_CLIENT_BUTT;

    INP_CHECK(ret, NULL == p_hal);
    memset(p_hal, 0, sizeof(H264dHalCtx_t));


    //!< choose hard mode
    {
        RK_S32 hw_type = -1;
        RK_U32 mode = MODE_NULL;
        RK_U32 vcodec_type = mpp_get_vcodec_type();

        // check codec_type
        if (!(vcodec_type & (HAVE_RKVDEC | HAVE_VDPU1 | HAVE_VDPU2))) {
            mpp_err_f("can not found H.264 decoder hardware on platform %x\n", vcodec_type);
            return ret;
        }
        mpp_env_get_u32("use_mpp_mode", &mode, MODE_NULL);
        if (MODE_NULL == mode) {
            MppDecBaseCfg *base = &cfg->cfg->base;

            if (mpp_check_soc_cap(base->type, base->coding))
                hw_type = base->hw_type;

            if (hw_type > 0) {
                if (vcodec_type & (1 << hw_type)) {
                    mpp_log("init with %s hw\n", strof_client_type(hw_type));
                    client_type = hw_type;
                } else
                    mpp_err_f("invalid hw_type %d with vcodec_type %08x\n",
                              hw_type, vcodec_type);
            }
        }

        if (client_type == VPU_CLIENT_BUTT) {
            if ((mode <= RKVDEC_MODE) && (vcodec_type & HAVE_RKVDEC)) {
                client_type = VPU_CLIENT_RKVDEC;
            } else if (vcodec_type & HAVE_VDPU1) {
                client_type = VPU_CLIENT_VDPU1;
            } else if (vcodec_type & HAVE_VDPU2) {
                client_type = VPU_CLIENT_VDPU2;
            }
        }
        H264D_DBG(H264D_DBG_HARD_MODE, "client_type %d\n", client_type);
    }

    p_api = &p_hal->hal_api;
    switch (client_type) {
    case VPU_CLIENT_RKVDEC : {
        RK_U32 hw_id = mpp_get_client_hw_id(client_type);

        if (hw_id == HWID_VDPU34X || hw_id == HWID_VDPU38X) {
            p_api->init    = vdpu34x_h264d_init;
            p_api->deinit  = vdpu34x_h264d_deinit;
            p_api->reg_gen = vdpu34x_h264d_gen_regs;
            p_api->start   = vdpu34x_h264d_start;
            p_api->wait    = vdpu34x_h264d_wait;
            p_api->reset   = vdpu34x_h264d_reset;
            p_api->flush   = vdpu34x_h264d_flush;
            p_api->control = vdpu34x_h264d_control;
        } else {
            p_api->init    = rkv_h264d_init;
            p_api->deinit  = rkv_h264d_deinit;
            p_api->reg_gen = rkv_h264d_gen_regs;
            p_api->start   = rkv_h264d_start;
            p_api->wait    = rkv_h264d_wait;
            p_api->reset   = rkv_h264d_reset;
            p_api->flush   = rkv_h264d_flush;
            p_api->control = rkv_h264d_control;
        }

        cfg->support_fast_mode = 1;
    } break;
    case VPU_CLIENT_VDPU1 : {
        p_api->init    = vdpu1_h264d_init;
        p_api->deinit  = vdpu1_h264d_deinit;
        p_api->reg_gen = vdpu1_h264d_gen_regs;
        p_api->start   = vdpu1_h264d_start;
        p_api->wait    = vdpu1_h264d_wait;
        p_api->reset   = vdpu1_h264d_reset;
        p_api->flush   = vdpu1_h264d_flush;
        p_api->control = vdpu1_h264d_control;
    } break;
    case VPU_CLIENT_VDPU2 : {
        p_api->init    = vdpu2_h264d_init;
        p_api->deinit  = vdpu2_h264d_deinit;
        p_api->reg_gen = vdpu2_h264d_gen_regs;
        p_api->start   = vdpu2_h264d_start;
        p_api->wait    = vdpu2_h264d_wait;
        p_api->reset   = vdpu2_h264d_reset;
        p_api->flush   = vdpu2_h264d_flush;
        p_api->control = vdpu2_h264d_control;
    } break;
    default : {
        mpp_err_f("client_type error, value=%d\n", client_type);
        mpp_assert(0);
    } break;
    }

    mpp_env_get_u32("hal_h264d_debug", &hal_h264d_debug, 0);

    ret = mpp_dev_init(&cfg->dev, client_type);
    if (ret) {
        mpp_err("mpp_dev_init failed ret: %d\n", ret);
        goto __FAILED;
    }

    //!< callback function to parser module
    p_hal->dec_cb       = cfg->dec_cb;
    p_hal->cfg          = cfg->cfg;
    p_hal->dev          = cfg->dev;
    p_hal->frame_slots  = cfg->frame_slots;
    p_hal->packet_slots = cfg->packet_slots;
    p_hal->fast_mode    = cfg->cfg->base.fast_parse;

    //< get buffer group
    if (p_hal->buf_group == NULL) {
        FUN_CHECK(ret = mpp_buffer_group_get_internal
                        (&p_hal->buf_group, MPP_BUFFER_TYPE_ION));
    }

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

    if (p_hal->dev) {
        mpp_dev_deinit(p_hal->dev);
        p_hal->dev = NULL;
    }

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
MPP_RET hal_h264d_control(void *hal, MpiCmd cmd_type, void *param)
{
    H264dHalCtx_t *p_hal = (H264dHalCtx_t *)hal;

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

