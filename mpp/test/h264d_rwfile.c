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
#include "h264d_log.h"
#include "h264d_rwfile.h"


#define     MAX_STRING_SIZE      512

#define     MAX_ITEMS_TO_PARSE   32
#define     START_PREFIX_3BYTE   3
#define     MAX_ITEMS_TO_PARSE   32

#define     RKVDEC_REG_HEADER    0x48474552
#define     RKVDEC_PPS_HEADER    0x48535050
#define     RKVDEC_SCL_HEADER    0x534c4353
#define     RKVDEC_RPS_HEADER    0x48535052
#define     RKVDEC_CRC_HEADER    0x48435243
#define     RKVDEC_STM_HEADER    0x4d525453
#define     RKVDEC_ERR_HEADER    0x524f5245


static const RK_U32 IOBUFSIZE   = 16 * 1024 * 1024; //524288
static const RK_U32 STMBUFSIZE  = 16 * 1024 * 1024; //524288


//!< values for nal_unit_type
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

static void display_input_cmd(RK_S32 argc, char *argv[])
{
    RK_S32 nn = 0;
    char *pnamecmd = NULL;
    char strcmd[MAX_STRING_SIZE] = {0};

    strcat(strcmd, "INPUT_CMD:  ");
    pnamecmd = strrchr(argv[0], '/');
    pnamecmd = pnamecmd ? (pnamecmd + 1) : (strrchr(argv[0], '\\') + 1) ;
    strcat(strcmd, pnamecmd);
    for (nn = 1; nn < argc; nn++) {
        strcat(strcmd, " ");
        strcat(strcmd, argv[nn]);
    }
    mpp_log("%s \n", strcmd);
}

static void print_help_message(char *cmd)
{
    mpp_log(
        "##### Options\n"
        "   -h/--help      : prints help message.\n"
        "   --cmppath      :[string] Set golden input  file store directory.\n"
        "   --outpath      :[string] Set driver output file store directory.\n"
        "   -r/--raw       :[number] Set bitstream raw cfg, 0/1: slice long 2:slice short.\n"
        "   -c/--cfg       :[file]   Set configure file< such as, decoder.cfg>.\n"
        "   -i/--input     :[file]   Set input bitstream file.\n"
        "   -n/--num       :[number] Set decoded frames.\n"
        "##### Examples of usage:\n"
        "   %s  -h\n"
        "   %s  -c decoder.cfg\n"
        "   %s  -i input.bin -n 10 ", cmd, cmd, cmd);
}

static RK_U8 *get_config_file_content(char *fname)
{
    FILE *fp_cfg = NULL;
    RK_U8 *pbuf = NULL;
    RK_U32 filesize = 0;

    if (!(fp_cfg = fopen(fname, "r"))) {
        mpp_log("Cannot open configuration file %s.", fname);
        goto __FAILED;
    }

    if (fseek(fp_cfg, 0, SEEK_END)) {
        mpp_log("Cannot fseek in configuration file %s.", fname);
        goto __FAILED;
    }

    filesize = ftell(fp_cfg);

    if (filesize > 150000) {
        mpp_log("\n Unreasonable Filesize %d reported by ftell for configuration file %s.", filesize, fname);
        goto __FAILED;
    }
    if (fseek(fp_cfg, 0, SEEK_SET)) {
        mpp_log("Cannot fseek in configuration file %s.", fname);
        goto __FAILED;
    }

    if (!(pbuf = mpp_malloc_size(RK_U8, filesize + 1))) {
        mpp_log("Cannot malloc content buffer for file %s.", fname);
        goto __FAILED;
    }
    filesize = (long)fread(pbuf, 1, filesize, fp_cfg);
    pbuf[filesize] = '\0';

    FCLOSE(fp_cfg);

    return pbuf;
__FAILED:
    FCLOSE(fp_cfg);

    return NULL;
}

static MPP_RET parse_content(InputParams *p_in, RK_U8 *p)
{
    RK_S32 i = 0, item = 0;
    RK_S32 InString = 0, InItem = 0;
    RK_U8  *bufend  = NULL;
    RK_U8  *items[MAX_ITEMS_TO_PARSE] = { NULL };

    //==== parsing
    bufend = &p[strlen((const char *)p)];
    while (p < bufend) {
        switch (*p) {
        case 13:
            ++p;
            break;
        case '#':                 // Found comment
            *p = '\0';              // Replace '#' with '\0' in case of comment immediately following integer or string
            while (*p != '\n' && p < bufend)  // Skip till EOL or EOF, whichever comes first
                ++p;
            InString = 0;
            InItem = 0;
            break;
        case '\n':
            InItem = 0;
            InString = 0;
            *p++ = '\0';
            break;
        case ' ':
        case '\t':              // Skip whitespace, leave state unchanged
            if (InString)
                p++;
            else {
                // Terminate non-strings once whitespace is found
                *p++ = '\0';
                InItem = 0;
            }
            break;

        case '"':               // Begin/End of String
            *p++ = '\0';
            if (!InString) {
                items[item++] = p;
                InItem = ~InItem;
            } else
                InItem = 0;
            InString = ~InString; // Toggle
            break;

        default:
            if (!InItem) {
                items[item++] = p;
                InItem = ~InItem;
            }
            p++;
        }
    }
    item--;
    //===== read syntax
    for (i = 0; i < item; i += 3) {
        if (!strncmp((const char*)items[i], "InputFile", 9)) {
            strncpy((char *)p_in->infile_name, (const char*)items[i + 2], strlen((const char*)items[i + 2]) + 1);
        } else if (!strncmp((const char*)items[i], "GoldenDataPath", 14)) {
            strncpy((char *)p_in->cmp_path_dir, (const char*)items[i + 2], strlen((const char*)items[i + 2]) + 1);
        } else if (!strncmp((const char*)items[i], "OutputDataPath", 14)) {
            strncpy((char *)p_in->out_path_dir, (const char*)items[i + 2], strlen((const char*)items[i + 2]) + 1);
        } else if (!strncmp((const char*)items[i], "DecodedFrames", 13)) {
            if (!sscanf((const char*)items[i + 2], "%d", &p_in->iDecFrmNum)) {
                goto __FAILED;
            }
        } else if (!strncmp((const char*)items[i], "BitStrmRawCfg", 13)) {
            if (!sscanf((const char*)items[i + 2], "%d", &p_in->raw_cfg)) {
                goto __FAILED;
            }
        }
    }

    return MPP_OK;
__FAILED:
    return MPP_NOK;
}

static MPP_RET parse_command(InputParams *p_in, int ac, char *av[])
{
    RK_S32  CLcount  = 0;
    RK_U8  *content  = NULL;
    char   *pnamecmd = NULL;
    RK_U8  have_cfg_flag = 0;

    CLcount = 1;
    while (CLcount < ac) {
        if (!strncmp(av[1], "-h", 2) || !strncmp(av[1], "--help", 6)) {
            pnamecmd = strrchr(av[0], '/');
            pnamecmd = pnamecmd ? (pnamecmd + 1) : (strrchr(av[0], '\\') + 1) ;
            print_help_message(pnamecmd);
            goto __FAILED;
        } else if (!strncmp(av[CLcount], "-n", 2) || !strncmp(av[1], "--num", 5)) { // decoded frames
            if (!sscanf(av[CLcount + 1], "%d", &p_in->iDecFrmNum)) {
                goto __FAILED;
            }
            CLcount += 2;
        } else if (!strncmp(av[CLcount], "-i", 2) || !strncmp(av[1], "--input", 7)) {
            strncpy(p_in->infile_name, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
            CLcount += 2;
        } else if (!strncmp(av[CLcount], "--cmppath", 9)) { // compare direct path
            strncpy(p_in->cmp_path_dir, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
            CLcount += 2;
        } else if (!strncmp(av[CLcount], "--outpath", 9)) { // compare direct path
            strncpy(p_in->out_path_dir, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
            CLcount += 2;
        } else if (!strncmp(av[1], "-r", 2) || !strncmp(av[CLcount], "--raw", 5)) {
            if (!sscanf(av[CLcount + 1], "%d", &p_in->raw_cfg)) {
                goto __FAILED;
            }
            CLcount += 2;
        } else if (!strncmp(av[1], "-c", 2) || !strncmp(av[CLcount], "--cfg", 5)) { // configure file
            strncpy(p_in->cfgfile_name, av[CLcount + 1], FILE_NAME_SIZE);
            CLcount += 2;
            have_cfg_flag = 1;
        } else {
            mpp_log("Error: %s cannot explain command! \n", av[CLcount]);
            goto __FAILED;
        }
    }
    if (have_cfg_flag) {
        if (!(content = get_config_file_content(p_in->cfgfile_name))) {
            goto __FAILED;
        }
        if (parse_content(p_in, content)) {
            goto __FAILED;
        }
        mpp_free(content);
    }

    return MPP_OK;
__FAILED:

    return MPP_NOK;
}

static FILE *open_file(char *path, char *infile, char *sufix, const char *mode)
{
    char tmp_fname[FILE_NAME_SIZE] = { 0 };

    sprintf(tmp_fname, "%s/%s%s", path, (strrchr(infile, '/') + 1), sufix);

    return fopen(tmp_fname, mode);
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
RK_U32 g_nalu_cnt2 = 0;
static MPP_RET read_next_nalu(InputParams *p_in)
{
    RK_S32 forbidden_bit      = -1;
    RK_S32 nal_reference_idc  = -1;
    RK_S32 nal_unit_type      = -1;
    RK_S32 nalu_header_bytes  = -1;
    RK_U32 first_mb_in_slice  = -1;
    RK_S32 svc_extension_flag = -1;

    GetBitCtx_t *pStrmData = (GetBitCtx_t *)p_in->bitctx;
    memset(pStrmData, 0, sizeof(GetBitCtx_t));
    set_streamdata(pStrmData, p_in->IO.pNALU, 4);
    read_bits( pStrmData, 1, &forbidden_bit);
    ASSERT(forbidden_bit == 0);
    read_bits( pStrmData, 2, &nal_reference_idc);
    read_bits( pStrmData, 5, &nal_unit_type);
    if (g_nalu_cnt2 == 344) {
        g_nalu_cnt2 = g_nalu_cnt2;
    }
    //if (g_debug_file0 == NULL)
    //{
    //  g_debug_file0 = fopen("rk_debugfile_view0.txt", "wb");
    //}
    //FPRINT(g_debug_file0, "g_nalu_cnt = %d, nal_unit_type = %d, nalu_size = %d\n", g_nalu_cnt2++, nal_unit_type, p_in->IO.nalubytes);

    nalu_header_bytes = 1;
    if ((nal_unit_type == NALU_TYPE_PREFIX) || (nal_unit_type == NALU_TYPE_SLC_EXT)) {
        read_bits(pStrmData, 1, &svc_extension_flag);
        if (!svc_extension_flag && nal_unit_type == NALU_TYPE_SLC_EXT) {//!< MVC
            nal_unit_type = NALU_TYPE_SLICE;
        }
        nalu_header_bytes += 3;
    }
    //-- parse slice
    if ( nal_unit_type == NALU_TYPE_SLICE || nal_unit_type == NALU_TYPE_IDR) {
        set_streamdata(pStrmData, (p_in->IO.pNALU + nalu_header_bytes), 4); // reset
        read_ue(pStrmData, &first_mb_in_slice);
        //FPRINT(g_debug_file0, "first_mb_in_slice = %d \n", first_mb_in_slice);
        //if (first_mb_in_slice == 0) {
        //  FPRINT(g_debug_file0, "--- new frame ---- \n");
        //}
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

static void reset_tmpdata_ctx(TempDataCtx_t *tmp)
{
    tmp->header = 0;
    tmp->len    = 0;
    tmp->pps_id = 0;
}


static void read_golden_data(FILE *fp, TempDataCtx_t *tmpctx, RK_U32 frame_no)
{
    RK_U32 header = 0, datasize = 0;

    reset_tmpdata_ctx(tmpctx);
    fread(&header,   1, sizeof(RK_U32), fp);
    fread(&datasize, 1, sizeof(RK_U32), fp);
    while (!feof(fp)) {
        switch (header) {
        case RKVDEC_PPS_HEADER:
            tmpctx->pps_id = datasize >> 16;
            datasize = datasize & 0xffff;
        case RKVDEC_SCL_HEADER:
        case RKVDEC_RPS_HEADER:
        case RKVDEC_STM_HEADER:
            fseek(fp, datasize, SEEK_CUR);
            break;
        case RKVDEC_REG_HEADER:
            fseek(fp, datasize, SEEK_CUR);
            goto __RETURN;
            break;
        case RKVDEC_CRC_HEADER:
            fread(tmpctx->data, sizeof(RK_U8), datasize, fp);
            tmpctx->len = datasize;
            break;
        case RKVDEC_ERR_HEADER:
            break;
        default:
            mpp_log("ERROR: frame_no=%d. \n", frame_no);
            ASSERT(0);
            break;
        }
        fread(&header,   1, sizeof(RK_U32), fp);
        fread(&datasize, 1, sizeof(RK_U32), fp);
    }
__RETURN:
    return;
}

static void write_bytes(FILE *fp_in, TempDataCtx_t *tmpctx, FILE *fp_out)
{
    RK_U32 i = 0;
    RK_U8  temp = 0;
    RK_U32 data_temp = 0;
    data_temp = (tmpctx->pps_id << 16) | tmpctx->len;

    fwrite(&tmpctx->header,   sizeof(RK_U32), 1, fp_out);
    fwrite(&data_temp, sizeof(RK_U32), 1, fp_out);
    while (i < tmpctx->len) {
        fread (&temp, sizeof(RK_U8), 1, fp_in);
        fwrite(&temp, sizeof(RK_U8), 1, fp_out);
        i++;
    }
}


static void write_driver_bytes(FILE *fp_out, TempDataCtx_t *in_tmpctx, FILE *fp_in, RK_U32 frame_no)
{
    RK_U32 header = 0, datasize = 0;
    TempDataCtx_t m_tmpctx = { 0 };

    fread(&header,   1, sizeof(RK_U32), fp_in);
    fread(&datasize, 1, sizeof(RK_U32), fp_in);

    while (!feof(fp_in)) {
        memset(&m_tmpctx, 0, sizeof(TempDataCtx_t));
        switch (header) {
        case RKVDEC_PPS_HEADER:
            m_tmpctx.pps_id = in_tmpctx->pps_id;
        case RKVDEC_SCL_HEADER:
        case RKVDEC_RPS_HEADER:
        case RKVDEC_STM_HEADER:
            m_tmpctx.header = header;
            m_tmpctx.len = datasize;
            write_bytes(fp_in, &m_tmpctx, fp_out);
            break;
        case RKVDEC_REG_HEADER:
            m_tmpctx.header = header;
            m_tmpctx.len = datasize;
            header = RKVDEC_CRC_HEADER;
            fwrite(&header, sizeof(RK_U32), 1, fp_out);
            fwrite(&in_tmpctx->len, sizeof(RK_U32), 1, fp_out);
            fwrite(in_tmpctx->data, sizeof(RK_U8), in_tmpctx->len, fp_out);
            write_bytes(fp_in, &m_tmpctx, fp_out);
            return;
        default:
            mpp_log("ERROR: frame_no=%d. \n", frame_no);
            ASSERT(0);
            break;
        }
        fread(&header,   1, sizeof(RK_U32), fp_in);
        fread(&datasize, 1, sizeof(RK_U32), fp_in);
    }
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
    display_input_cmd(ac, av);
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
    FCLOSE(p_in->fp_bitstream);
    FCLOSE(p_in->fp_golden_data);
    FCLOSE(p_in->fp_driver_data);

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

    FLE_CHECK(ret, p_in->fp_bitstream   = fopen(p_in->infile_name, "rb"));

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
        mpp_free(p_in->IO.pbuf);
        mpp_free(p_in->bitctx);
        mpp_free(p_in->strm.pbuf);
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
MPP_RET h264d_read_one_frame(InputParams *p_in, MppPacket pkt)
{
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
    //if (first_mb_in_slice == 0) {
    //FPRINT(g_debug_file0, "--- new frame ---- \n");
    //}

    //-- set code input context
    ((MppPacketImpl *)pkt)->pos  = p_in->strm.pbuf;
    ((MppPacketImpl *)pkt)->size = p_in->strm.strmbytes;
    if (g_max_slice_data < p_in->strm.strmbytes) {
        g_max_slice_data = p_in->strm.strmbytes;
    }
    if (p_in->is_eof) {
        mpp_packet_set_eos(pkt);
    }
    return MPP_OK;
}
/*!
***********************************************************************
* \brief
*   write fpage data to file
***********************************************************************
*/
MPP_RET h264d_write_fpga_data(InputParams *p_in)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    TempDataCtx_t tmpctx = { 0 };
    FILE *fp_log = NULL;
    RK_U32 frame_no = 0;
    RK_U32 ctrl_value = 0;
    RK_U32 ctrl_debug = 0;
    RK_U32 ctrl_fpga  = 0;
    RK_U32 ctrl_write = 0;
    char *outpath_dir = NULL;
    char *cmppath_dir = NULL;

    mpp_env_get_u32(logenv_name.ctrl, &ctrl_value, 0);
    ctrl_debug = GetBitVal(ctrl_value, LOG_DEBUG);
    ctrl_fpga  = GetBitVal(ctrl_value, LOG_FPGA);
    ctrl_write = GetBitVal(ctrl_value, LOG_WRITE);
    INP_CHECK(ret, ctx, !(ctrl_debug && ctrl_fpga && ctrl_write));
    mpp_env_get_str(logenv_name.outpath,  &outpath_dir,  NULL);
    mpp_env_get_str(logenv_name.cmppath,  &cmppath_dir,  NULL);
    p_in->fp_driver_data = open_file(outpath_dir, p_in->infile_name, ".dat", "wb");
    p_in->fp_golden_data = open_file(cmppath_dir, p_in->infile_name, ".dat",  "rb");
    fp_log = fopen(strcat(outpath_dir, "/h264d_driver_data.dat"), "rb");
    FLE_CHECK(ret, p_in->fp_golden_data);
    FLE_CHECK(ret, p_in->fp_driver_data);
    FLE_CHECK(ret, fp_log);
    tmpctx.data = mpp_calloc_size(RK_U8, 128);
    MEM_CHECK(ret, tmpctx.data); //!< for read golden fpga data
    while (!feof(p_in->fp_golden_data) && !feof(fp_log)) {
        read_golden_data (p_in->fp_golden_data, &tmpctx, frame_no);
        write_driver_bytes(p_in->fp_driver_data, &tmpctx, fp_log, frame_no);
        frame_no++;
    }
    //remove(out_name);
__RETURN:
    ret = MPP_OK;
__FAILED:
    mpp_free(tmpctx.data);
    h264d_close_files(p_in);

    return ret;
}
