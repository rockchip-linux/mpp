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

#ifndef __MPP_DEC_CFG_H__
#define __MPP_DEC_CFG_H__

#include "mpp_frame.h"
#include "rk_vdec_cmd.h"

typedef enum MppDecCfgChange_e {
    MPP_DEC_CFG_CHANGE_TYPE             = (1 << 0),
    MPP_DEC_CFG_CHANGE_CODING           = (1 << 1),
    MPP_DEC_CFG_CHANGE_HW_TYPE          = (1 << 2),
    MPP_DEC_CFG_CHANGE_BATCH_MODE       = (1 << 3),

    MPP_DEC_CFG_CHANGE_OUTPUT_FORMAT    = (1 << 8),
    MPP_DEC_CFG_CHANGE_FAST_OUT         = (1 << 9),
    MPP_DEC_CFG_CHANGE_FAST_PARSE       = (1 << 10),
    MPP_DEC_CFG_CHANGE_SPLIT_PARSE      = (1 << 11),
    MPP_DEC_CFG_CHANGE_INTERNAL_PTS     = (1 << 12),
    MPP_DEC_CFG_CHANGE_SORT_PTS         = (1 << 13),
    MPP_DEC_CFG_CHANGE_DISABLE_ERROR    = (1 << 14),
    MPP_DEC_CFG_CHANGE_ENABLE_VPROC     = (1 << 15),
    MPP_DEC_CFG_CHANGE_ENABLE_FAST_PLAY = (1 << 16),

    MPP_DEC_CFG_CHANGE_ALL              = (0xFFFFFFFF),
} MppDecCfgChange;

typedef struct MppDecBaseCfg_t {
    RK_U64              change;

    MppCtxType          type;
    MppCodingType       coding;
    RK_S32              hw_type;
    RK_U32              batch_mode;

    MppFrameFormat      out_fmt;
    RK_U32              fast_out;
    RK_U32              fast_parse;
    RK_U32              split_parse;
    RK_U32              internal_pts;
    RK_U32              sort_pts;
    RK_U32              disable_error;
    RK_U32              enable_vproc;
    RK_U32              enable_fast_play;
} MppDecBaseCfg;

typedef enum MppDecCbCfgChange_e {
    MPP_DEC_CB_CFG_CHANGE_PKT_RDY       = (1 << 0),
    MPP_DEC_CB_CFG_CHANGE_FRM_RDY       = (1 << 1),

    MPP_DEC_CB_CFG_CHANGE_ALL           = (0xFFFFFFFF),
} MppDecCbCfgChange;

typedef struct MppDecCbCfg_t {
    RK_U64              change;

    /* notify packet process done and can accept new packet */
    MppExtCbFunc        pkt_rdy_cb;
    MppExtCbCtx         pkt_rdy_ctx;
    RK_S32              pkt_rdy_cmd;

    /* notify frame ready for output */
    MppExtCbFunc        frm_rdy_cb;
    MppExtCbCtx         frm_rdy_ctx;
    RK_S32              frm_rdy_cmd;
} MppDecCbCfg;

typedef struct MppDecStatusCfg_t {
    RK_U32              hal_support_fast_mode;
    RK_U32              hal_task_count;
    RK_U32              vproc_task_count;
} MppDecStatusCfg;

typedef struct MppDecCfgSet_t {
    MppDecBaseCfg       base;
    MppDecStatusCfg     status;
    MppDecCbCfg         cb;
} MppDecCfgSet;

/*
 * MppDecCfgApi is the function set for configure MppDecCfgSet by name
 */
typedef struct MppDecCfgApi_t {
    const char          *name;
    RK_S32              type_set;
    RK_S32              type_get;
    void                *api_set;
    void                *api_get;
} MppDecCfgApi;

#endif /* __MPP_DEC_CFG_H__ */
