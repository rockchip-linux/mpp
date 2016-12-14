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

#define MODULE_TAG "h264d_rwfile"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_mem.h"
#include "mpp_env.h"
#include "h264d_rwfile.h"


#define     MAX_STRING_SIZE      512

#define     MAX_ITEMS_TO_PARSE   32
#define     START_PREFIX_3BYTE   3
#define     MAX_ITEMS_TO_PARSE   32




static const RK_U32 IOBUFSIZE   = 16 * 1024 * 1024; //524288
static const RK_U32 STMBUFSIZE  = 16 * 1024 * 1024; //524288

RK_U32 rkv_h264d_test_debug = 0;
//!< values for nalu_type
typedef enum {
    NALU_TYPE_NULL = 0,
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,   // Access Unit Delimiter
    NALU_TYPE_EOSEQ = 10,  // end of sequence
    NALU_TYPE_EOSTREAM = 11,  // end of stream
    NALU_TYPE_FILL = 12,
    NALU_TYPE_SPSEXT = 13,
    NALU_TYPE_PREFIX = 14,  // prefix
    NALU_TYPE_SUB_SPS = 15,
    NALU_TYPE_SLICE_AUX = 19,
    NALU_TYPE_SLC_EXT = 20,  // slice extensive
    NALU_TYPE_VDRD = 24   // View and Dependency Representation Delimiter NAL Unit

} Nalu_type;


typedef struct {
    RK_U8 *data_;
    RK_U32 bytes_left_;
    RK_S64 curr_byte_;
    RK_S32 num_remaining_bits_in_curr_byte_;
    RK_S64 prev_two_bytes_;
    RK_S64 emulation_prevention_bytes_;
    RK_S32 used_bits;
    RK_U8  *buf;
    RK_S32 buf_len;
} GetBitCtx_t;

typedef struct {
    RK_U32 header;
    RK_U8  *data;
    RK_U32 len;
    RK_S32 pps_id;
} TempDataCtx_t;



static MPP_RET parse_command(InputParams *p_in, int ac, char *av[])
{
    RK_S32  CLcount  = 0;

    CLcount = 1;
    while (CLcount < ac) {
        if (!strncmp(av[1], "-h", 2)) {
            mpp_log("   -h  : prints help message.");
            mpp_log("   -i  :[file]   Set input bitstream file.");
            mpp_log("   -o  :[file]   Set input bitstream file.");
            mpp_log("   -n  :[number] Set decoded frames.");
            goto __RETURN;
        } else if (!strncmp(av[CLcount], "-n", 2)) { // decoded frames
            if (!sscanf(av[CLcount + 1], "%d", &p_in->iDecFrmNum)) {
                goto __FAILED;
            }
            CLcount += 2;
        } else if (!strncmp(av[CLcount], "-i", 2)) {
            strncpy(p_in->infile_name, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
            CLcount += 2;
        } else if (!strncmp(av[CLcount], "-o", 2)) {
            strncpy(p_in->infile_name, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
            CLcount += 2;
        } else {
            mpp_err("error, cannot explain command: %s.\n", av[CLcount]);
            goto __FAILED;
        }
    }
__RETURN:
    return MPP_OK;
__FAILED:

    return MPP_NOK;
}


static MPP_RET update_curr_byte(GetBitCtx_t *pStrmData)
{
    if (pStrmData->bytes_left_ < 1)
        return MPP_NOK;

    // Emulation prevention three-byte detection.
    // If a sequence of 0x000003 is found, skip (ignore) the last byte (0x03).
    if (*pStrmData->data_ == 0x03 && (pStrmData->prev_two_bytes_ & 0xffff) == 0) {
        // Detected 0x000003, skip last byte.
        ++pStrmData->data_;
        --pStrmData->bytes_left_;
        ++pStrmData->emulation_prevention_bytes_;
        // Need another full three bytes before we can detect the sequence again.
        pStrmData->prev_two_bytes_ = 0xffff;

        if (pStrmData->bytes_left_ < 1)
            return MPP_NOK;
    }

    // Load a new byte and advance pointers.
    pStrmData->curr_byte_ = *pStrmData->data_++ & 0xff;
    --pStrmData->bytes_left_;
    pStrmData->num_remaining_bits_in_curr_byte_ = 8;

    pStrmData->prev_two_bytes_ = (pStrmData->prev_two_bytes_ << 8) | pStrmData->curr_byte_;

    return MPP_OK;
}

// Read |num_bits| (1 to 31 inclusive) from the stream and return them
// in |out|, with first bit in the stream as MSB in |out| at position
// (|num_bits| - 1).
static MPP_RET read_bits(GetBitCtx_t *pStrmData, RK_S32 num_bits, RK_S32 *out)
{
    RK_S32 bits_left = num_bits;
    *out = 0;
    ASSERT(num_bits <= 31);

    while (pStrmData->num_remaining_bits_in_curr_byte_ < bits_left) {
        // Take all that's left in current byte, shift to make space for the rest.
        *out |= (pStrmData->curr_byte_ << (bits_left - pStrmData->num_remaining_bits_in_curr_byte_));
        bits_left -= pStrmData->num_remaining_bits_in_curr_byte_;

        if (update_curr_byte(pStrmData)) {
            return MPP_NOK;
        }
    }

    *out |= (pStrmData->curr_byte_ >> (pStrmData->num_remaining_bits_in_curr_byte_ - bits_left));
    *out &= ((1 << num_bits) - 1);
    pStrmData->num_remaining_bits_in_curr_byte_ -= bits_left;
    pStrmData->used_bits += num_bits;

    return MPP_OK;
}

static MPP_RET read_ue(GetBitCtx_t *pStrmData, RK_U32 *val)
{
    RK_S32 num_bits = -1;
    RK_S32 bit;
    RK_S32 rest;
    // Count the number of contiguous zero bits.
    do {
        if (read_bits(pStrmData, 1, &bit)) {
            return MPP_NOK;
        }
        num_bits++;
    } while (bit == 0);

    if (num_bits > 31) {
        return MPP_NOK;
    }
    // Calculate exp-Golomb code value of size num_bits.
    *val = (1 << num_bits) - 1;

    if (num_bits > 0) {
        if (read_bits(pStrmData, num_bits, &rest)) {
            return MPP_NOK;
        }
        *val += rest;
    }

    return MPP_OK;
}

static void set_streamdata(GetBitCtx_t *pStrmData, RK_U8 *data, RK_S32 size)
{
    memset(pStrmData, 0, sizeof(GetBitCtx_t));
    pStrmData->data_ = data;
    pStrmData->bytes_left_ = size;
    pStrmData->num_remaining_bits_in_curr_byte_ = 0;
    // Initially set to 0xffff to accept all initial two-byte sequences.
    pStrmData->prev_two_bytes_ = 0xffff;
    pStrmData->emulation_prevention_bytes_ = 0;
    // add
    pStrmData->buf = data;
    pStrmData->buf_len = size;
    pStrmData->used_bits = 0;
}


static void write_nalu_prefix(InputParams *p_in)
{
    if (p_in->IO.pfxbytes > START_PREFIX_3BYTE) {
        p_in->strm.pbuf[p_in->strm.strmbytes++] = 0x00;
    }
    p_in->strm.pbuf[p_in->strm.strmbytes++] = 0x00;
    p_in->strm.pbuf[p_in->strm.strmbytes++] = 0x00;
    p_in->strm.pbuf[p_in->strm.strmbytes++] = 0x01;
}

static RK_U8 read_one_byte(InputParams *p_in)
{
    RK_U8 data = 0;

    if (0 == fread(&data, 1, 1, p_in->fp_bitstream)) {
        p_in->is_eof = 1;
    }

    return data;
}

static void find_next_nalu(InputParams *p_in)
{
    RK_U8 startcode_had_find = 0;
    //-- first read three bytes
    if (p_in->is_fist_nalu) {
        p_in->IO.pbuf[p_in->IO.offset]     = read_one_byte(p_in);
        p_in->IO.pbuf[p_in->IO.offset + 1] = read_one_byte(p_in);
        p_in->IO.pbuf[p_in->IO.offset + 2] = read_one_byte(p_in);
        p_in->is_fist_nalu = 0;
    }
    do {
        if (startcode_had_find) {
            p_in->IO.nalubytes++;
        }
        //--- find 0x000001
        if ((p_in->IO.pbuf[p_in->IO.offset] == 0x00)
            && (p_in->IO.pbuf[p_in->IO.offset + 1] == 0x00)
            && (p_in->IO.pbuf[p_in->IO.offset + 2] == 0x01)) {
            if (startcode_had_find) { //-- end_code
                p_in->IO.nalubytes -= START_PREFIX_3BYTE;
                if (p_in->IO.pbuf[p_in->IO.offset - 1] == 0x00) {
                    p_in->IO.pbuf[0] = 0x00;
                    p_in->IO.nalubytes--;
                } else {
                    p_in->IO.pbuf[0] = 0xFF;
                }
                p_in->IO.pbuf[1] = 0x00;
                p_in->IO.pbuf[2] = 0x00;
                p_in->IO.pbuf[3] = 0x01;
                p_in->IO.offset = 1;
                break;
            } else { //-- find strart_code
                startcode_had_find  = 1;
                p_in->IO.nalubytes = 0;
                p_in->IO.pfxbytes = START_PREFIX_3BYTE;
                p_in->IO.pNALU = &p_in->IO.pbuf[p_in->IO.offset + START_PREFIX_3BYTE];
                if (p_in->IO.offset && (p_in->IO.pbuf[p_in->IO.offset - 1] == 0x00)) {
                    p_in->IO.pfxbytes++;
                }
            }
        }
        p_in->IO.offset++;
        p_in->IO.pbuf[p_in->IO.offset + 2] = read_one_byte(p_in);
    } while (!p_in->is_eof);
}

static MPP_RET read_next_nalu(InputParams *p_in)
{
    RK_S32 forbidden_bit      = -1;
    RK_S32 nal_reference_idc  = -1;
    RK_S32 nalu_type      = -1;
    RK_S32 nalu_header_bytes  = -1;
    RK_U32 first_mb_in_slice  = -1;
    RK_S32 svc_extension_flag = -1;

    GetBitCtx_t *pStrmData = (GetBitCtx_t *)p_in->bitctx;
    memset(pStrmData, 0, sizeof(GetBitCtx_t));
    set_streamdata(pStrmData, p_in->IO.pNALU, 4);
    read_bits( pStrmData, 1, &forbidden_bit);
    ASSERT(forbidden_bit == 0);
    read_bits( pStrmData, 2, &nal_reference_idc);
    read_bits( pStrmData, 5, &nalu_type);

    nalu_header_bytes = 1;
    if ((nalu_type == NALU_TYPE_PREFIX) || (nalu_type == NALU_TYPE_SLC_EXT)) {
        read_bits(pStrmData, 1, &svc_extension_flag);
        if (!svc_extension_flag && nalu_type == NALU_TYPE_SLC_EXT) {//!< MVC
            nalu_type = NALU_TYPE_SLICE;
        }
        nalu_header_bytes += 3;
    }
    //-- parse slice
    if ( nalu_type == NALU_TYPE_SLICE || nalu_type == NALU_TYPE_IDR) {
        set_streamdata(pStrmData, (p_in->IO.pNALU + nalu_header_bytes), 4); // reset
        read_ue(pStrmData, &first_mb_in_slice);
        if (!p_in->is_fist_frame && (first_mb_in_slice == 0)) {
            p_in->is_new_frame = 1;
        }
        p_in->is_fist_frame = 0;
    }
    if (!p_in->is_new_frame) {
        write_nalu_prefix(p_in);
        memcpy(&p_in->strm.pbuf[p_in->strm.strmbytes], p_in->IO.pNALU, p_in->IO.nalubytes);
        p_in->strm.strmbytes += p_in->IO.nalubytes;
    }

    return MPP_OK;
}


/*!
***********************************************************************
* \brief
*   configure function to analyze command or configure file
***********************************************************************
*/
MPP_RET h264d_configure(InputParams *p_in, RK_S32 ac, char *av[])
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    VAL_CHECK(ret, ac > 1);

    FUN_CHECK (ret = parse_command(p_in, ac, av));

    return MPP_OK;
__FAILED:
    return ret;
}


/*!
***********************************************************************
* \brief
*   close file
***********************************************************************
*/
MPP_RET h264d_close_files(InputParams *p_in)
{
    MPP_FCLOSE(p_in->fp_bitstream);
    MPP_FCLOSE(p_in->fp_yuv_data);

    return MPP_OK;
}

/*!
***********************************************************************
* \brief
*   open file
***********************************************************************
*/
MPP_RET h264d_open_files(InputParams *p_in)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    FLE_CHECK(ret, p_in->fp_bitstream = fopen(p_in->infile_name, "rb"));
    p_in->fp_yuv_data = fopen(p_in->infile_name, "wb");
    return MPP_OK;
__FAILED:
    h264d_close_files(p_in);

    return ret;
}

/*!
***********************************************************************
* \brief
*   free frame buffer
***********************************************************************
*/

MPP_RET h264d_free_frame_buffer(InputParams *p_in)
{
    if (p_in) {
        MPP_FREE(p_in->IO.pbuf);
        MPP_FREE(p_in->bitctx);
        MPP_FREE(p_in->strm.pbuf);
    }

    return MPP_NOK;
}
/*!
***********************************************************************
* \brief
*   alloc frame buffer
***********************************************************************
*/
MPP_RET h264d_alloc_frame_buffer(InputParams *p_in)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    p_in->IO.pbuf   = mpp_malloc_size(RK_U8, IOBUFSIZE);
    p_in->strm.pbuf = mpp_malloc_size(RK_U8, STMBUFSIZE);
    p_in->bitctx    = mpp_malloc_size(void, sizeof(GetBitCtx_t));
    MEM_CHECK(ret, p_in->IO.pbuf && p_in->strm.pbuf && p_in->bitctx);
    p_in->is_fist_nalu  = 1;
    p_in->is_fist_frame = 1;

    return MPP_OK;
__FAILED:
    return ret;
}


/*!
***********************************************************************
* \brief
*   read one frame
***********************************************************************
*/
MPP_RET h264d_read_one_frame(InputParams *p_in)
{
    if (0) {
        p_in->strm.strmbytes = 0;
        p_in->is_new_frame = 0;
        //-- copy first nalu
        if (!p_in->is_fist_frame) {
            write_nalu_prefix(p_in);
            memcpy(&p_in->strm.pbuf[p_in->strm.strmbytes], p_in->IO.pNALU, p_in->IO.nalubytes);
            p_in->strm.strmbytes += p_in->IO.nalubytes;
        }
        //-- read one nalu and copy to stream buffer
        do {
            find_next_nalu(p_in);
            read_next_nalu(p_in);
        } while (!p_in->is_new_frame && !p_in->is_eof);
    } else {
        RK_U32 read_len = 512;
        p_in->strm.strmbytes = fread(p_in->strm.pbuf, 1, read_len, p_in->fp_bitstream);
        p_in->is_eof = p_in->strm.strmbytes ? 0 : 1;
    }

    return MPP_OK;
}
