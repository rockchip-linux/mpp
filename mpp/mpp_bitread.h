#ifndef __RK_BIT_READ_H__
#define __RK_BIT_READ_H__
#include <stdio.h>
#include <assert.h>
#include "rk_type.h"
#include "mpp_log.h"
#include "mpp_common.h"
#include "mpp_err.h"

#define READ_BITS(pBitCtx, num_bits, out)                     \
    do {                                                      \
        RK_S32 _out;                                          \
        RK_S32 _ret = 0;                                      \
        _ret = mpp_ReadBits(pBitCtx, num_bits, &_out);        \
        if (!_ret) { *out = _out;          }                  \
        else     { return  MPP_ERR_STREAM; }                  \
    } while (0)

#define READ_SKIPBITS(pBitCtx, num_bits)                      \
    do {                                                      \
        RK_S32 _ret = 0;                                      \
        _ret = mpp_SkipBits(pBitCtx, num_bits);               \
        if (_ret) {return  MPP_ERR_STREAM; }                  \
    } while (0)


#define READ_BIT1(pBitCtx, out)                               \
    do {                                                      \
        RK_S32 _out;                                          \
        RK_S32 _ret = 0;                                      \
        _ret = mpp_ReadBits(pBitCtx, 1, &_out);               \
        if (!_ret) { *out = _out;          }                  \
        else     { return  MPP_ERR_STREAM;}                   \
    } while (0)


#define READ_UE(pBitCtx, out)                                 \
    do {                                                      \
        RK_U32 _out;                                          \
        RK_S32 _ret = 0;                                      \
        _ret = mpp_ReadUE(pBitCtx, &_out);                    \
        if (!_ret) { *out = _out;          }                  \
        else     { return  MPP_ERR_STREAM; }                  \
    } while (0)


#define READ_SE(pBitCtx, out)                                \
    do {                                                     \
        RK_S32 _out;                                         \
        RK_S32 _ret = 0;                                     \
        _ret = mpp_ReadSE(pBitCtx, &_out);                   \
        if (!_ret) { *out = _out;          }                 \
        else     { return  MPP_ERR_STREAM; }                 \
    } while (0)


#define CHECK_RANGE(pBitCtx,val, _min, _max)                 \
    do {                                                     \
        if ((val) < (_min) || (val) > (_max)) {              \
            mpp_log("%d[%d,%d]", val, _min, _max);            \
            return  MPP_ERR_STREAM;                          \
        }                                                    \
    } while (0)


#define CHECK_ERROR(pBitCtx,val)                             \
    do {                                                     \
        if (!(val)) {                                        \
            mpp_log("value false");                           \
            mpp_assert(0);                                    \
            return  MPP_ERR_STREAM;                          \
        }                                                    \
    } while (0)


//=====================================================================

typedef struct {
    // Pointer to the next unread (not in curr_byte_) byte in the stream.
    RK_U8 *data_;
    // Bytes left in the stream (without the curr_byte_).
    RK_U32 bytes_left_;
    // Contents of the current byte; first unread bit starting at position
    // 8 - num_remaining_bits_in_curr_byte_ from MSB.
    RK_S64 curr_byte_;
    // Number of bits remaining in curr_byte_
    RK_S32 num_remaining_bits_in_curr_byte_;
    // Used in emulation prevention three byte detection (see spec).
    // Initially set to 0xffff to accept all initial two-byte sequences.
    RK_S64 prev_two_bytes_;
    // Number of emulation preventation bytes (0x000003) we met.
    RK_S64 emulation_prevention_bytes_;
    // file to debug
    // count PPS SPS SEI read bits
    RK_S32 UsedBits;
    RK_U8  *buf;
    RK_S32 buf_len;
} GetBitCxt_t;


#ifdef  __cplusplus
extern "C" {
#endif

RK_S32 mpp_ReadBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits, RK_S32 *out);
RK_S32 mpp_ReadLongBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits, RK_U32 *out);
RK_S32 mpp_SkipBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits);
RK_S32 mpp_ReadUE(GetBitCxt_t *pBitCtx, RK_U32* val);
RK_S32 mpp_ReadSE(GetBitCxt_t *pBitCtx, RK_S32* val);
void   mpp_Init_Bits(GetBitCxt_t *pBitCtx, RK_U8 *data, RK_S32 size);
RK_S32 mpp_has_more_rbsp_data(GetBitCxt_t * pBitCtx);
//void   mpp_Set_Bits_LogContex(GetBitCxt_t *pBitCtx, RK_LOG_CONTEX_t *p_ctx);
void   mpp_Reset_UseBits(GetBitCxt_t *pBitCtx);

#ifdef  __cplusplus
}
#endif


#endif /* __RK_BIT_READ_H__ */
