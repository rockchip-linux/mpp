#ifndef __HAL_H264D_PACKET_H__
#define __HAL_H264D_PACKET_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "hal_task.h"

#include "h264d_log.h"
#include "hal_h264d_fifo.h"

typedef struct h264_fifo_packet_t
{
	FifoCtx_t   spspps;
	FifoCtx_t   rps;
	FifoCtx_t   strm;
	FifoCtx_t   scanlist;
	FifoCtx_t   reg;
}H264_FifoPkt_t;


#ifdef	__cplusplus
extern "C" {
#endif
	void    reset_fifo_packet(H264_FifoPkt_t *pkt);
	void    free_fifo_packet (H264_FifoPkt_t *pkt);
	MPP_RET alloc_fifo_packet(H264dLogCtx_t *logctx, H264_FifoPkt_t *pkts);

	void    explain_input_buffer   (void *hal, HalDecTask *task);
	void    prepare_spspps_packet  (void *hal, FifoCtx_t *pkt);
	void    prepare_framerps_packet(void *hal, FifoCtx_t *pkt);
	void    prepare_scanlist_packet(void *hal, FifoCtx_t *pkt);
	void    prepare_stream_packet  (void *hal, FifoCtx_t *pkt);
	void    generate_regs          (void *hal, FifoCtx_t *pkt);

#ifdef	__cplusplus
}
#endif



//!<============================
#endif // __HAL_H264D_PACKET_H__



