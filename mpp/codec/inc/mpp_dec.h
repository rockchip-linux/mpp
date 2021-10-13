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

#ifndef __MPP_DEC_H__
#define __MPP_DEC_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "rk_mpi_cmd.h"
#include "mpp_dec_cfg.h"

typedef enum MppDecEvent_e {
    MPP_DEC_EVENT_ON_PKT_RELEASE,
    MPP_DEC_EVENT_ON_FRM_READY,

    MPP_DEC_EVENT_BUTT,
} MppDecEvent;

typedef void* MppDec;

typedef struct {
    MppCodingType       coding;
    void                *mpp;

    MppDecCfgSet        *cfg;
} MppDecInitCfg;

#ifdef __cplusplus
extern "C" {
#endif

MPP_RET mpp_dec_init(MppDec *ctx, MppDecInitCfg *cfg);
MPP_RET mpp_dec_deinit(MppDec ctx);

MPP_RET mpp_dec_start(MppDec ctx);
MPP_RET mpp_dec_stop(MppDec ctx);

MPP_RET mpp_dec_reset(MppDec ctx);
MPP_RET mpp_dec_flush(MppDec ctx);
MPP_RET mpp_dec_control(MppDec ctx, MpiCmd cmd, void *param);
MPP_RET mpp_dec_notify(MppDec ctx, RK_U32 flag);
MPP_RET mpp_dec_callback(MppDec ctx, MppDecEvent event, void *arg);

/* update init cfg before init */
MPP_RET mpp_dec_set_cfg_by_cmd(MppDecCfgSet *set, MpiCmd cmd, void *param);
MPP_RET mpp_dec_set_cfg(MppDecCfgSet *dst, MppDecCfgSet *src);

#ifdef __cplusplus
}
#endif

#endif /*__MPP_DEC_H__*/
