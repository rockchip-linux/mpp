/*
 * Copyright 2010 Rockchip Electronics S.LSI Co. LTD
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

#ifndef __AVSD_PARSE_H__
#define __AVSD_PARSE_H__


#include "rk_type.h"
#include "parser_api.h"

#include "mpp_err.h"
#include "mpp_packet.h"
#include "mpp_buf_slot.h"

#include "hal_task.h"

#define MAX_STREM_IN_SIZE   (2*1024*1024)
#define MAX_BITSTREAM_SIZE  (2*1024*1024)

//!< NALU type
#define SLICE_START_CODE_MIN    0x00
#define SLICE_START_CODE_MAX    0xAF

#define I_PICTURE_START_CODE    0xB3
#define PB_PICTURE_START_CODE   0xB6

#define USER_DATA_START_CODE    0xB2
#define SEQUENCE_HEADER_CODE    0xB0
#define EXTENSION_START_CODE    0xB5

#define SEQUENCE_END_CODE       0xB1 //!< no in ffmpeg
#define VIDEO_EDIT_CODE         0xB7 //!< no in ffmpeg


#define FPRINT(fp, ...)  do{ if (fp) { fprintf(fp, ## __VA_ARGS__); fflush(fp);} }while(0)


//!< input parameter
typedef struct avsd_input_ctx_t {
	FILE *fp_log;

	ParserCfg   init;
	MppPacket   in_pkt;
	HalDecTask *in_task;

	RK_S64 in_pts;
	RK_S64 in_dts;

	RK_U8  has_get_eos;
	RK_U64 pkt_no;
	struct avsd_cur_ctx_t   *p_cur;
	struct avsd_video_ctx_t *p_vid;
	struct avs_dec_ctx_t    *p_dec;
} AvsdInputCtx_t;

//!< current stream
typedef struct avsd_cur_strm_t {
	RK_U8     *p_start;       //!< store read nalu data
	RK_U32    len;

	RK_U32    prefixdata;

	RK_U8 got_frame_flag;
	RK_U8 got_nalu_flag;
} AvsdCurStream_t;

typedef struct avsd_nalu_t
{
	RK_U8  header;
	RK_U8 *pdata;
	RK_U32 size;
	RK_U32 length;
	RK_U8  start_pos;
	RK_U8  eof; //!< end of frame stream
}AvsdNalu_t;

//!< current parameters
typedef struct avsd_cur_ctx_t {
	struct avsd_cur_strm_t   m_strm;
	struct avsd_nalu_t      *cur_nalu; //!< current nalu

	struct avsd_input_ctx_t *p_inp;
	struct avsd_video_ctx_t *p_vid;
	struct avs_dec_ctx_t    *p_dec;
} AvsdCurCtx_t;


//!< decoder parameters
typedef struct avsd_video_ctx_t {

	struct img_par          *img;

	struct avsd_input_ctx_t *p_inp;
	struct avsd_cur_ctx_t   *p_cur;
	struct avs_dec_ctx_t    *p_dec;
}AvsdVideoCtx_t;


typedef struct avsd_bitstream_t {
	RK_U8 *pbuf;
	RK_U32 size;
	RK_U32 len;
	RK_U32 offset; //!< start from the offset byte
}AvsdBitstream_t;


typedef struct avsd_memory_t {

	struct avsd_bitstream_t    bitstream;
} AvsdMemory_t;


//!< decoder parameters
typedef struct avs_dec_ctx_t {
	MppBufSlots              frame_slots;
	MppBufSlots              packet_slots;

	MppPacket                task_pkt;
	struct avsd_memory_t     *mem; //!< resotre slice data to decoder
	struct avsd_bitstream_t  *bitstream;

	struct avsd_input_ctx_t *p_inp;
	struct avsd_cur_ctx_t   *p_cur;
	struct avsd_video_ctx_t *p_vid;
	//!< use in libavs.so
	void                    *libdec;
	RK_U32                  dec_no;
	//mpp_list

} Avs_DecCtx_t;



#ifdef  __cplusplus
extern "C" {
#endif

MPP_RET avsd_parse_prepare(AvsdInputCtx_t *p_inp, AvsdCurCtx_t *p_cur);

//!< used in libavs.so
MPP_RET lib_avsd_free(void *decoder);
void   *lib_avsd_malloc(void *decoder);
RK_S32 lib_prepare_one_frame(void *decoder, MppPacket pkt, HalDecTask *task);
RK_S32 lib_parse_one_frame(void *decoder, HalDecTask *task);
RK_S32 lib_init_one_frame(void *decoder, HalDecTask *task);
RK_S32 lib_decode_one_frame(void *decoder, HalDecTask *task);
RK_S32 lib_flush(void *decoder);
RK_S32 lib_reset(void *decoder);

RK_S32 nv12_copy_buffer(void *p_dec, RK_U8 *des);



#ifdef  __cplusplus
}
#endif

#endif /*__AVSD_PARSE_H__*/
