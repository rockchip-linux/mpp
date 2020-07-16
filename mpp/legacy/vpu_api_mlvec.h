/*
 * Copyright 2020 Rockchip Electronics Co. LTD
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

#ifndef __VPU_API_MLVEC_H__
#define __VPU_API_MLVEC_H__

#include "rk_mpi.h"

#define MLVEC_MAGIC                     'M'
#define MLVEC_VERSION                   '0'

#define VPU_API_ENC_MAX_TID_UPDATED     (0x00000001)
#define VPU_API_ENC_MARK_LTR_UPDATED    (0x00000002)
#define VPU_API_ENC_USE_LTR_UPDATED     (0x00000004)
#define VPU_API_ENC_FRAME_QP_UPDATED    (0x00000008)
#define VPU_API_ENC_BASE_PID_UPDATED    (0x00000010)

typedef struct VpuApiMlvecStaticCfg_t {
    RK_S16 width;
    RK_S16 sar_width;
    RK_S16 height;
    RK_S16 sar_height;
    RK_S32 rc_mode;                 /* 0 - CQP mode; 1 - CBR mode; */
    RK_S32 bitRate;                 /* target bitrate */
    RK_S32 framerate;
    RK_S32 qp;
    RK_S32 enableCabac;
    RK_S32 cabacInitIdc;
    RK_S32 format;
    RK_S32 intraPicRate;
    RK_S32 framerateout;
    RK_S32 profileIdc;
    RK_S32 levelIdc;

    RK_S32 magic;                   /* MLVEC magic word */
    /* static configure */
    RK_S32 max_tid      : 8;        /* max temporal layer id */
    RK_S32 ltr_frames   : 8;        /* max long-term reference frame count */
    RK_S32 hdr_on_idr   : 8;        /* sps/pps header with IDR frame */
    RK_S32 add_prefix   : 8;        /* add prefix before each frame */
    RK_S32 slice_mbs    : 16;       /* macroblock row count for each slice */
    RK_S32 reserved     : 16;
} VpuApiMlvecStaticCfg;

typedef struct VpuApiMlvecDynamicCfg_t {
    /* dynamic configure */
    RK_U32 updated;
    RK_S32 max_tid;
    RK_S32 mark_ltr;
    RK_S32 use_ltr;
    RK_S32 frame_qp;
    RK_S32 base_layer_pid;
} VpuApiMlvecDynamicCfg;

typedef void* VpuApiMlvec;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET vpu_api_mlvec_init(VpuApiMlvec *ctx);
MPP_RET vpu_api_mlvec_deinit(VpuApiMlvec ctx);

/* setup basic mpp info in vpu_api */
MPP_RET vpu_api_mlvec_setup(VpuApiMlvec ctx, MppCtx mpp, MppApi *mpi, MppEncCfg enc_cfg);
/* check the input encoder parameter is mlvec configure or not */
MPP_RET vpu_api_mlvec_check_cfg(void *p);

/* setup mlvec static configure */
MPP_RET vpu_api_mlvec_set_st_cfg(VpuApiMlvec ctx, VpuApiMlvecStaticCfg *cfg);
/* setup mlvec dynamic configure and setup MppMeta in MppFrame */
MPP_RET vpu_api_mlvec_set_dy_cfg(VpuApiMlvec ctx, VpuApiMlvecDynamicCfg *cfg, MppMeta meta);
/* setup mlvec max temporal layer count dynamic configure */
MPP_RET vpu_api_mlvec_set_dy_max_tid(VpuApiMlvec ctx, RK_S32 max_tid);

#ifdef __cplusplus
}
#endif

#endif /* __VPU_API_MLVEC_H__ */
