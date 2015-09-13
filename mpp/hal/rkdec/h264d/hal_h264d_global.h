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


#ifndef __HAL_H264D_GLOBAL_H__
#define __HAL_H264D_GLOBAL_H__

#include "mpp_hal.h"
#include "dxva_syntax.h"
#include "h264d_syntax.h"

#include "h264d_log.h"
#include "hal_h264d_reg.h"
#include "hal_h264d_fifo.h"
#include "hal_h264d_packet.h"


typedef struct h264d_hal_mem_t {
    H264_REGS_t              regs;    //!< for register
    H264_MMU_t               mmu_regs;
    H264_CACHE_t             cache_regs;
    H264_FifoPkt_t           pkts;
} H264dHalMem_t;

typedef struct h264d_hal_ctx_t {
    H264dHalMem_t            *mem;
    H264_REGS_t              *regs;
    H264_MMU_t               *mmu_regs;
    H264_CACHE_t             *cache_regs;
    H264_FifoPkt_t           *pkts;
    RK_U8                    spt_BitstrmRaw;
    RK_U8                    set_BitstrmRaw;

    DXVA_PicParams_H264_MVC  *pp;
    DXVA_Qmatrix_H264        *qm;
    RK_U32                   slice_num;
    DXVA_Slice_H264_Short    *slice_short;  //!<  MAX_SLICES
    DXVA_Slice_H264_Long     *slice_long;   //!<  MAX_SLICES
    RK_U8                    *bitstream;
    RK_U32                   strm_len;
    RK_U32                   g_framecnt;
    H264dLogCtx_t            logctx;           //!< debug log file
    LogCtx_t                 logctxbuf[LOG_MAX];

} H264dHalCtx_t;



#endif /*__HAL_H264D_REG_H__*/
