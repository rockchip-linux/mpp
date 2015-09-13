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
#include <string.h>
#include <stdlib.h>
#include "mpp_err.h"
#include "h264d_bitread.h"


static MPP_RET update_currbyte(BitReadCtx_t *bitctx)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	VAL_CHECK(ret, bitctx->bytes_left_ > 0);
    // Emulation prevention three-byte detection.
    // If a sequence of 0x000003 is found, skip (ignore) the last byte (0x03).
    if (*bitctx->data_ == 0x03 && (bitctx->prev_two_bytes_ & 0xffff) == 0) {
        // Detected 0x000003, skip last byte.
        ++bitctx->data_;
        --bitctx->bytes_left_;
        ++bitctx->emulation_prevention_bytes_;
        // Need another full three bytes before we can detect the sequence again.
        bitctx->prev_two_bytes_ = 0xffff;
		VAL_CHECK(ret, bitctx->bytes_left_ > 0);
    }
    // Load a new byte and advance pointers.
    bitctx->curr_byte_ = *bitctx->data_++ & 0xff;
    --bitctx->bytes_left_;
    bitctx->num_remaining_bits_in_curr_byte_ = 8;
    bitctx->prev_two_bytes_ = (bitctx->prev_two_bytes_ << 8) | bitctx->curr_byte_;

    return ret = MPP_OK;
__FAILED:
	return ret;
}

/*!
***********************************************************************
* \brief
*   Read |num_bits| (1 to 31 inclusive) from the stream and return them
*   in |out|, with first bit in the stream as MSB in |out| at position
*   (|num_bits| - 1)
***********************************************************************
*/

MPP_RET read_bits(BitReadCtx_t *bitctx, RK_S32 num_bits, RK_S32 *out)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_S32 bits_left = num_bits;

    *out = 0;
	VAL_CHECK(ret, num_bits < 32);
    while (bitctx->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        *out |= (bitctx->curr_byte_ << (bits_left - bitctx->num_remaining_bits_in_curr_byte_));
        bits_left -= bitctx->num_remaining_bits_in_curr_byte_;

        FUN_CHECK(ret = update_currbyte(bitctx));
    }

    *out |= (bitctx->curr_byte_ >> (bitctx->num_remaining_bits_in_curr_byte_ - bits_left));
    *out &= ((1 << num_bits) - 1);
    bitctx->num_remaining_bits_in_curr_byte_ -= bits_left;
    bitctx->used_bits += num_bits;

    return ret = MPP_OK;
__FAILED:
    return ret = MPP_ERR_READ_BIT;
}
/*!
***********************************************************************
* \brief
*   read one bit
***********************************************************************
*/
MPP_RET read_one_bit(BitReadCtx_t *bitctx, RK_S32 *out)
{
    return read_bits(bitctx, 1, out);
}
/*!
***********************************************************************
* \brief
*   read ue
***********************************************************************
*/
MPP_RET read_ue(BitReadCtx_t *bitctx, RK_U32 *val)
{
    RK_S32 num_bits = -1;
    RK_S32 bit;
    RK_S32 rest;
	MPP_RET ret = MPP_ERR_UNKNOW;
    // Count the number of contiguous zero bits.
    do {
        FUN_CHECK(ret = read_bits(bitctx, 1, &bit));
        num_bits++;
    } while (bit == 0);

    VAL_CHECK(ret, num_bits < 32);
    // Calculate exp-Golomb code value of size num_bits.
    *val = (1 << num_bits) - 1;

    if (num_bits > 0) {
        FUN_CHECK(ret = read_bits(bitctx, num_bits, &rest));
        *val += rest;
    }

    return ret = MPP_OK;
__FAILED:
    return ret = MPP_ERR_READ_BIT;
}
/*!
***********************************************************************
* \brief
*   read se
***********************************************************************
*/
MPP_RET read_se(BitReadCtx_t *bitctx, RK_S32 *val)
{
	RK_U32 ue;
    MPP_RET ret = MPP_ERR_UNKNOW;

    FUN_CHECK(ret = read_ue(bitctx, &ue));

    if (ue % 2 == 0) { // odd
        *val = -(RK_S32)(ue >> 1);
    } else {
        *val = (RK_S32)((ue >> 1) + 1);
    }

    return ret = MPP_OK;
__FAILED:
    return ret = MPP_ERR_READ_BIT;
}

/*!
***********************************************************************
* \brief
*   check has more rbsp data in read pps syntax
***********************************************************************
*/
RET_tpye has_more_rbsp_data(BitReadCtx_t * bitctx)
{
    // Make sure we have more bits, if we are at 0 bits in current byte
    // and updating current byte fails, we don't have more data anyway.
    if (bitctx->num_remaining_bits_in_curr_byte_ == 0 && update_currbyte(bitctx))
        return RET_FALSE;
    // On last byte?
    if (bitctx->bytes_left_)
        return RET_TURE;
    // Last byte, look for stop bit;
    // We have more RBSP data if the last non-zero bit we find is not the
    // first available bit.
    return (RET_tpye)((bitctx->curr_byte_ & ((1 << (bitctx->num_remaining_bits_in_curr_byte_ - 1)) - 1)) != 0);
}

/*!
***********************************************************************
* \brief
*   set bitread context
***********************************************************************
*/
void set_bitread_ctx(BitReadCtx_t *bitctx, RK_U8 *data, RK_S32 size)
{
    memset(bitctx, 0, sizeof(BitReadCtx_t));
    bitctx->data_ = data;
    bitctx->bytes_left_ = size;
    bitctx->num_remaining_bits_in_curr_byte_ = 0;
    // Initially set to 0xffff to accept all initial two-byte sequences.
    bitctx->prev_two_bytes_ = 0xffff;
    bitctx->emulation_prevention_bytes_ = 0;
    // add
    bitctx->buf = data;
    bitctx->buf_len = size;
    bitctx->used_bits = 0;
}
/*!
***********************************************************************
* \brief
*   set bitread log context
***********************************************************************
*/
void set_bitread_logctx(BitReadCtx_t *bitctx, LogCtx_t *p_ctx)
{
    bitctx->ctx = p_ctx;
}

/*!
***********************************************************************
* \brief
*   get current data value, used in read sei syntax
***********************************************************************
*/
RK_U8 get_currdata_value(BitReadCtx_t *bitctx)
{
    return (*bitctx->data_);
}




