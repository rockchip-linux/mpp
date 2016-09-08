#ifndef __HAL_H264D_FIFO_H__
#define __HAL_H264D_FIFO_H__

#include "rk_type.h"
#include "mpp_err.h"
#include "h264d_log.h"


typedef struct {
    RK_U32          header;
    RK_U32          buflen;         //!< max buf length, 64bit uint
    RK_U32          index;           //!< current uint position
    RK_U64          *pbuf;          //!< outpacket data
    RK_U64          bvalue;         //!< buffer value, 64 bit
    RK_U8           bitpos;         //!< bit pos in 64bit
    RK_U32          size;           //!< data size,except header
    LogCtx_t        *logctx;        //!< for debug
    FILE            *fp_data;       //!< for fpga
} FifoCtx_t;


#ifdef  __cplusplus
extern "C" {
#endif

void    fifo_packet_reset (FifoCtx_t *pkt);
void    fifo_fwrite_header(FifoCtx_t *pkt, RK_S32 pkt_size);
void    fifo_fwrite_data  (FifoCtx_t *pkt);
void    fifo_write_bits   (FifoCtx_t *pkt, RK_U64 invalue, RK_U8 lbits, const char *name);
void    fifo_flush_bits   (FifoCtx_t *pkt);
void    fifo_align_bits   (FifoCtx_t *pkt, RK_U8 align_bits);
void    fifo_write_bytes  (FifoCtx_t *pkt, void *psrc, RK_U32 size);
void    fifo_fwrite       (FILE *fp,       void *psrc, RK_U32 size);
void    fifo_packet_init  (FifoCtx_t *pkt, void *p_start, RK_S32 size);
MPP_RET fifo_packet_alloc (FifoCtx_t *pkt, RK_S32 header, RK_S32 size);
#ifdef  __cplusplus
}
#endif



//!<============================
#endif //!<__HAL_H264D_FIFO_H__



