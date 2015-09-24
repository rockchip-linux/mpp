#include <stdlib.h>
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "rk_type.h"
#include <string.h>

static RK_S32 rk_UpdateCurrByte(GetBitCxt_t *pBitCtx)
{
    if (pBitCtx->bytes_left_ < 1)
        return  MPP_ERR_STREAM;

    // Emulation prevention three-byte detection.
    // If a sequence of 0x000003 is found, skip (ignore) the last byte (0x03).
    if (*pBitCtx->data_ == 0x03 && (pBitCtx->prev_two_bytes_ & 0xffff) == 0) {
        // Detected 0x000003, skip last byte.
        ++pBitCtx->data_;
        --pBitCtx->bytes_left_;
        ++pBitCtx->emulation_prevention_bytes_;
        // Need another full three bytes before we can detect the sequence again.
        pBitCtx->prev_two_bytes_ = 0xffff;

        if (pBitCtx->bytes_left_ < 1)
            return  MPP_ERR_STREAM;
    }

    // Load a new byte and advance pointers.
    pBitCtx->curr_byte_ = *pBitCtx->data_++ & 0xff;
    --pBitCtx->bytes_left_;
    pBitCtx->num_remaining_bits_in_curr_byte_ = 8;

    pBitCtx->prev_two_bytes_ = (pBitCtx->prev_two_bytes_ << 8) | pBitCtx->curr_byte_;

    return MPP_OK;
}


// Read |num_bits| (1 to 31 inclusive) from the stream and return them
// in |out|, with first bit in the stream as MSB in |out| at position
// (|num_bits| - 1).
RK_S32 mpp_ReadBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits, RK_S32 *out)
{
    RK_S32 bits_left = num_bits;
    *out = 0;
    if (num_bits > 31) {
        //  rk_err("%s num bit exceed 32",__function__);
        return  MPP_ERR_STREAM;

    }

    while (pBitCtx->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        *out |= (pBitCtx->curr_byte_ << (bits_left - pBitCtx->num_remaining_bits_in_curr_byte_));
        bits_left -= pBitCtx->num_remaining_bits_in_curr_byte_;

        if (rk_UpdateCurrByte(pBitCtx)) {
            return  MPP_ERR_STREAM;
        }
    }

    *out |= (pBitCtx->curr_byte_ >> (pBitCtx->num_remaining_bits_in_curr_byte_ - bits_left));
    *out &= ((1 << num_bits) - 1);
    pBitCtx->num_remaining_bits_in_curr_byte_ -= bits_left;
    pBitCtx->UsedBits += num_bits;

    return MPP_OK;
}

RK_S32 mpp_ReadLongBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits, RK_U32 *out)
{
    RK_S32 val = 0, val1 = 0;
    if (mpp_ReadBits(pBitCtx, 16, &val)) {
        return  MPP_ERR_STREAM;
    }
    if (mpp_ReadBits(pBitCtx, (num_bits - 16), &val1)) {
        return  MPP_ERR_STREAM;
    }
    val = val << 16;
    *out = (RK_U32)(val | val1);
    return MPP_OK;
}

RK_S32 mpp_SkipBits(GetBitCxt_t *pBitCtx, RK_S32 num_bits)
{
    RK_S32 bits_left = num_bits;

    while (pBitCtx->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        bits_left -= pBitCtx->num_remaining_bits_in_curr_byte_;

        if (rk_UpdateCurrByte(pBitCtx)) {
            return  MPP_ERR_STREAM;
        }
    }
    pBitCtx->num_remaining_bits_in_curr_byte_ -= bits_left;
    pBitCtx->UsedBits += num_bits;
    return MPP_OK;
}


RK_S32 mpp_ReadUE(GetBitCxt_t *pBitCtx, RK_U32 *val)
{
    RK_S32 num_bits = -1;
    RK_S32 bit;
    RK_S32 rest;
    // Count the number of contiguous zero bits.
    do {
        if (mpp_ReadBits(pBitCtx, 1, &bit)) {
            return  MPP_ERR_STREAM;
        }
        num_bits++;
    } while (bit == 0);

    if (num_bits > 31) {
        return  MPP_ERR_STREAM;
    }
    // Calculate exp-Golomb code value of size num_bits.
    *val = (1 << num_bits) - 1;

    if (num_bits > 0) {
        if (mpp_ReadBits(pBitCtx, num_bits, &rest)) {
            return  MPP_ERR_STREAM;
        }
        *val += rest;
    }

    return MPP_OK;
}

RK_S32 mpp_ReadSE(GetBitCxt_t *pBitCtx, RK_S32 *val)
{
    RK_U32 ue;
    if (mpp_ReadUE(pBitCtx, &ue)) {
        return  MPP_ERR_STREAM;
    }
    if (ue % 2 == 0) { // odd
        *val = -(RK_S32)(ue >> 1);
    } else {
        *val = (RK_S32)((ue >> 1) + 1);
    }
    return MPP_OK;
}


RK_S32 mpp_has_more_rbsp_data(GetBitCxt_t *pBitCtx)
{
    // Make sure we have more bits, if we are at 0 bits in current byte
    // and updating current byte fails, we don't have more data anyway.
    if (pBitCtx->num_remaining_bits_in_curr_byte_ == 0 && !rk_UpdateCurrByte(pBitCtx))
        return 0;

    // On last byte?
    if (pBitCtx->bytes_left_)
        return 0;

    // Last byte, look for stop bit;
    // We have more RBSP data if the last non-zero bit we find is not the
    // first available bit.
    return (pBitCtx->curr_byte_ &
            ((1 << (pBitCtx->num_remaining_bits_in_curr_byte_ - 1)) - 1)) != 0;
}

void mpp_Init_Bits(GetBitCxt_t *pBitCtx, RK_U8 *data, RK_S32 size)
{
    memset(pBitCtx, 0, sizeof(GetBitCxt_t));
    pBitCtx->data_ = data;
    pBitCtx->bytes_left_ = size;
    pBitCtx->num_remaining_bits_in_curr_byte_ = 0;
    // Initially set to 0xffff to accept all initial two-byte sequences.
    pBitCtx->prev_two_bytes_ = 0xffff;
    pBitCtx->emulation_prevention_bytes_ = 0;
    // add
    pBitCtx->buf = data;
    pBitCtx->buf_len = size;
    pBitCtx->UsedBits = 0;
}

#if 0
void mpp_Set_Bits_LogContex(GetBitCxt_t *pBitCtx, RK_LOG_CONTEX_t *p_ctx)
{
    //pBitCtx->ctx = p_ctx;
}
#endif

void mpp_Reset_UseBits(GetBitCxt_t *pBitCtx)
{
    pBitCtx->UsedBits = 0;
}

RK_U8 mpp_get_curdata_value(GetBitCxt_t *pBitCtx)
{

    return (*pBitCtx->data_);
}
