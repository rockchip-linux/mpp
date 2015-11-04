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

#ifndef __HAL_H264D_RKV_PKT_H__
#define __HAL_H264D_RKV_PKT_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "hal_task.h"

#include "h264d_log.h"
#include "hal_h264d_fifo.h"


/* Number registers for the decoder */
#define DEC_RKV_REGISTERS          78

#define RKV_CABAC_TAB_SIZE        (3680)        /* bytes */
#define RKV_SPSPPS_SIZE           (256*32)      /* bytes */
#define RKV_RPS_SIZE              (128)         /* bytes */
#define RKV_SCALING_LIST_SIZE     (6*16+2*64)   /* bytes */

typedef struct h264d_rkv_packet_t {
    FifoCtx_t   spspps;
    FifoCtx_t   rps;
    FifoCtx_t   scanlist;
    FifoCtx_t   reg;
} H264dRkvPkt_t;


#ifdef  __cplusplus
extern "C" {
#endif
void    rkv_reset_fifo_packet(H264dRkvPkt_t *pkt);
void    rkv_free_fifo_packet (H264dRkvPkt_t *pkt);
MPP_RET rkv_alloc_fifo_packet(H264dLogCtx_t *logctx, H264dRkvPkt_t *pkts);

void    rkv_prepare_spspps_packet  (void *hal, FifoCtx_t *pkt);
void    rkv_prepare_framerps_packet(void *hal, FifoCtx_t *pkt);
void    rkv_prepare_scanlist_packet(void *hal, FifoCtx_t *pkt);
void    rkv_prepare_stream_packet  (void *hal, FifoCtx_t *pkt);
void    rkv_generate_regs          (void *hal, FifoCtx_t *pkt);
void    rkv_fprint_fifo_data       (FILE *fp,  FifoCtx_t *pkt);


#ifdef  __cplusplus
}
#endif



//!<============================
#endif /* __HAL_H264D_RKV_PKT_H__ */



