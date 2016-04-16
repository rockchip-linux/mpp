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

#define MODULE_TAG "avsd_test"

#if defined(_WIN32)
#include "vld.h"
#endif
#include <string.h>

#include "vpu_api.h"
#include "mpp_env.h"
#include "mpp_log.h"
#include "mpp_dec.h"
#include "mpp_hal.h"
#include "mpp_mem.h"
#include "mpp_common.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer.h"
#include "mpp_buffer_impl.h"
#include "mpp_hal.h"
#include "mpp_thread.h"

#include "hal_task.h"
#include "avsd_api.h"

#include "hal_avsd_api.h"



#ifdef AVS_TEST__
int main(int argc, char **argv)
{
	return avsd_test_main(argc, argv);
}

#else

#define AVSD_TEST_ERROR       (0x00000001)
#define AVSD_TEST_ASSERT      (0x00000002)
#define AVSD_TEST_WARNNING    (0x00000004)
#define AVSD_TEST_TRACE       (0x00000008)

#define AVSD_TEST_DUMPYUV     (0x00000010)


#define AVSD_TEST_LOG(level, fmt, ...)\
do {\
if (level & rkv_avsd_test_debug)\
		{ mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


#define INP_CHECK(ret, val, ...)\
do{\
	if ((val)) {\
		ret = MPP_ERR_INIT; \
		AVSD_TEST_LOG(AVSD_TEST_WARNNING, "input empty(%d).\n", __LINE__); \
		goto __RETURN; \
}} while (0)

#define MEM_CHECK(ret, val, ...)\
do{\
	if (!(val)) {\
		ret = MPP_ERR_MALLOC; \
		AVSD_TEST_LOG(AVSD_TEST_ERROR, "malloc buffer error(%d).\n", __LINE__); \
		goto __FAILED; \
}} while (0)


#define FUN_CHECK(val)\
do{\
	if ((val) < 0) {\
		AVSD_TEST_LOG(AVSD_TEST_WARNNING, "Function error(%d).\n", __LINE__); \
		goto __FAILED; \
}} while (0)

static RK_U32 rkv_avsd_test_debug = 0;

typedef struct inp_par_t {
	FILE *fp_in;
	FILE *fp_out;

	RK_U8 *pbuf;
	RK_U32 bufsize;
	RK_U32 len;

	RK_U32 output_dec_pic;   // output_dec_pic

	RK_U32 dec_num; // to be decoded
	RK_U32 dec_no;  // current decoded number

	RK_U32 is_eof;
} InputParams;


typedef struct avsd_test_ctx_t {
	MppDec         m_api;
	InputParams    m_in;
	HalTaskInfo    m_task;

	MppBuffer      m_dec_pkt_buf;
	MppBuffer      m_dec_pic_buf;
	MppBufferGroup mFrameGroup;
	MppBufferGroup mStreamGroup;
} AvsdTestCtx_t;















static MPP_RET decoder_deinit(AvsdTestCtx_t *pctx)
{
	MppDec *pApi = &pctx->m_api;

	if (pApi->parser) {
		parser_deinit(pApi->parser);
		pApi->parser = NULL;
	}
	if (pApi->hal) {
		mpp_hal_deinit(pApi->hal);
		pApi->hal = NULL;
	}
	if (pApi->frame_slots) {
		mpp_buf_slot_deinit(pApi->frame_slots);
		pApi->frame_slots = NULL;
	}
	if (pApi->packet_slots) {
		mpp_buf_slot_deinit(pApi->packet_slots);
		pApi->packet_slots = NULL;
	}
	if (pctx->m_dec_pkt_buf) {
		mpp_buffer_put(pctx->m_dec_pkt_buf);
	}
	if (pctx->m_dec_pic_buf) {
		mpp_buffer_put(pctx->m_dec_pic_buf);
	}
	if (pctx->mFrameGroup != NULL) {
		mpp_err("mFrameGroup deInit");
		mpp_buffer_group_put(pctx->mFrameGroup);
	}
	if (pctx->mStreamGroup != NULL) {
		mpp_err("mStreamGroup deInit");
		mpp_buffer_group_put(pctx->mStreamGroup);
	}
	return MPP_OK;
}

static MPP_RET decoder_init(AvsdTestCtx_t *pctx)
{

	MPP_RET ret = MPP_ERR_UNKNOW;
	ParserCfg parser_cfg;
	MppHalCfg hal_cfg;
	MppDec *pApi = &pctx->m_api;

	if (pctx->mFrameGroup == NULL) {
		ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_NORMAL);
		if (MPP_OK != ret) {
			mpp_err("h264d mpp_buffer_group_get failed\n");
			goto __FAILED;
		}
	}
	if (pctx->mStreamGroup == NULL) {
		ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_NORMAL);
		if (MPP_OK != ret) {
			mpp_err("h264d mpp_buffer_group_get failed\n");
			goto __FAILED;
		}
	}
	// codec
	pApi->coding = MPP_VIDEO_CodingAVS;
	// malloc slot
	FUN_CHECK(ret = mpp_buf_slot_init(&pApi->frame_slots));
	MEM_CHECK(ret, pApi->frame_slots);
	ret = mpp_buf_slot_init(&pApi->packet_slots);
	MEM_CHECK(ret, pApi->packet_slots);
	mpp_buf_slot_setup(pApi->packet_slots, 2);
	// init parser part
	memset(&parser_cfg, 0, sizeof(parser_cfg));
	parser_cfg.coding = pApi->coding;
	parser_cfg.frame_slots = pApi->frame_slots;
	parser_cfg.packet_slots = pApi->packet_slots;
	parser_cfg.task_count = 2;
	FUN_CHECK(ret = parser_init(&pApi->parser, &parser_cfg));
	// init hal part
	memset(&hal_cfg, 0, sizeof(hal_cfg));
	hal_cfg.type = MPP_CTX_DEC;
	hal_cfg.coding = pApi->coding;
	hal_cfg.work_mode = HAL_MODE_LIBVPU;
	hal_cfg.device_id = HAL_VDPU;
	hal_cfg.frame_slots = pApi->frame_slots;
	hal_cfg.packet_slots = pApi->packet_slots;
	hal_cfg.task_count = parser_cfg.task_count;
	FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
	pApi->tasks = hal_cfg.tasks;

	return MPP_OK;

__FAILED:
	decoder_deinit(pctx);

	return ret;
}

static MPP_RET avsd_flush_frames(MppDec *pApi, FILE *fp)
{
	RK_S32 slot_idx = 0;
	MppFrame out_frame = NULL;

	while (MPP_OK == mpp_buf_slot_dequeue(pApi->frame_slots, &slot_idx, QUEUE_DISPLAY)) {
		mpp_buf_slot_get_prop(pApi->frame_slots, slot_idx, SLOT_FRAME, &out_frame);
		if (out_frame) {
			RK_U32 stride_w, stride_h;
			void *ptr = NULL;
			MppBuffer framebuf;
			stride_w = mpp_frame_get_hor_stride(out_frame);
			stride_h = mpp_frame_get_ver_stride(out_frame);
			framebuf = mpp_frame_get_buffer(out_frame);
			ptr = mpp_buffer_get_ptr(framebuf);
			if (fp) {
				fwrite(ptr, 1, stride_w * stride_h * 3 / 2, fp);
				fflush(fp);
			}
			mpp_frame_deinit(&out_frame);
			out_frame = NULL;
		}
		mpp_buf_slot_clr_flag(pApi->frame_slots, slot_idx, SLOT_QUEUE_USE);
	}

	return MPP_OK;
}


static MPP_RET avsd_input_deinit(InputParams *inp)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	MPP_FCLOSE(inp->fp_in);
	MPP_FCLOSE(inp->fp_out);

	return ret = MPP_OK;
}

static MPP_RET avsd_input_init(InputParams *inp, RK_S32 ac, char *av[])
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	char infile_name[128];    //!< Telenor AVS input
	char outfile_name[128];   //!< Decoded YUV 4:2:0 output
	RK_S32 CLcount = 1;

	inp->output_dec_pic = 0;
	while (CLcount < ac) {
		if (!strncmp(av[CLcount], "-h", 2))	{			
				mpp_log("Options:");
				mpp_log("   -h     : prints help message.");
				mpp_log("   -i     :[file]   Set input AVS+ bitstream file.");
				mpp_log("   -o     :[file]   Set output YUV file.");
				mpp_log("   -n     :[number] Set decoded frames.");			
				CLcount += 1;
		}
		else if (!strncmp(av[CLcount], "-i", 2)) {
			strncpy(infile_name, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
			CLcount += 2;
		}
		else if (!strncmp(av[CLcount], "-n", 2)) {
			if (!sscanf(av[CLcount + 1], "%d", &inp->dec_num)) {
				goto __FAILED;
			}
			CLcount += 2;
		}
		else if (!strncmp(av[CLcount], "-o", 2)) {
			if (rkv_avsd_test_debug & AVSD_TEST_DUMPYUV) {
				inp->output_dec_pic = 1;
				strncpy(outfile_name, av[CLcount + 1], strlen((const char*)av[CLcount + 1]) + 1);
			}			
			CLcount += 2;
		}
		else {
			mpp_err("error, %s cannot explain command! \n", av[CLcount]);
			goto __FAILED;
		}
	}
	if ((inp->fp_in = fopen(infile_name, "rb")) == 0) {
		mpp_err("error, open file %s ", infile_name);
		goto __FAILED;
	}
	if (inp->output_dec_pic) {
		if ((inp->fp_out = fopen(outfile_name, "wb")) == 0) {
			mpp_err("error, open file %s ", outfile_name);
			goto __FAILED;
		}
	}
	//!< malloc read buffer
	inp->bufsize = 512; 
	MEM_CHECK(ret, inp->pbuf = mpp_malloc(RK_U8, inp->bufsize));

	AVSD_TEST_LOG(AVSD_TEST_TRACE, "------------------------------------------------------------");
	AVSD_TEST_LOG(AVSD_TEST_TRACE, "Input AVS+ bitstream : %s", infile_name);
	AVSD_TEST_LOG(AVSD_TEST_TRACE, "Output decoded YUV   : %s", outfile_name);
	AVSD_TEST_LOG(AVSD_TEST_TRACE, "------------------------------------------------------------");
	AVSD_TEST_LOG(AVSD_TEST_TRACE, "Frame   TR    QP   SnrY    SnrU    SnrV   Time(ms)   FRM/FLD");

	return MPP_OK;
__FAILED:
	avsd_deinit(inp);

	return ret;
}


static MPP_RET avsd_read_data(InputParams *inp)
{
	MPP_RET ret = MPP_ERR_UNKNOW;

	inp->len = (RK_U32)fread(inp->pbuf, sizeof(RK_U8), inp->bufsize, inp->fp_in);
	inp->is_eof = feof(inp->fp_in);

	return ret = MPP_OK;
}

static MPP_RET decoder_single_test(AvsdTestCtx_t *pctx)
{
	MppPacket pkt = NULL;
	RK_U32 end_of_flag = 0;
	MPP_RET ret = MPP_ERR_UNKNOW;

	MppDec *pApi = &pctx->m_api;
	InputParams *inp = &pctx->m_in;
	HalTaskInfo *task = &pctx->m_task;
	//!< initial task
	memset(task, 0, sizeof(HalTaskInfo));
	memset(task->dec.refer, -1, sizeof(task->dec.refer));
	task->dec.input = -1;
	do {
		//!< get one packet
		if (pkt == NULL) {
			if (inp->is_eof ||
				(inp->dec_num && (inp->dec_no >= inp->dec_num))) {
				mpp_packet_init(&pkt, NULL, 0);
				mpp_packet_set_eos(pkt);
			}
			else {
				FUN_CHECK(ret = avsd_read_data(inp));
				mpp_packet_init(&pkt, inp->pbuf, inp->len);
			}
		}
		//!< prepare
		FUN_CHECK(ret = parser_prepare(pApi->parser, pkt, &task->dec));

		//!< parse
		if (task->dec.valid) {
			MppBufferImpl *buf = NULL;

			task->dec.valid = 0;
			if (task->dec.input < 0) {
				mpp_buf_slot_get_unused(pApi->packet_slots, &task->dec.input);
			}
			mpp_buf_slot_get_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, &pctx->m_dec_pkt_buf);
			if (NULL == pctx->m_dec_pkt_buf) {
				size_t stream_size = mpp_packet_get_size(task->dec.input_packet);
				mpp_buffer_get(pctx->mStreamGroup, &pctx->m_dec_pkt_buf, stream_size);
				if (pctx->m_dec_pkt_buf)
					mpp_buf_slot_set_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, pctx->m_dec_pkt_buf);
			}
			buf = (MppBufferImpl *)pctx->m_dec_pkt_buf;
			memcpy(buf->info.ptr, mpp_packet_get_data(task->dec.input_packet), mpp_packet_get_length(task->dec.input_packet));

			mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_CODEC_READY);
			mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
			FUN_CHECK(ret = parser_parse(pApi->parser, &task->dec));

			AVSD_TEST_LOG(AVSD_TEST_TRACE, "[AVSD_TEST] ---- decoder, read_one_frame Frame_no = %d", inp->dec_no);
			inp->dec_no++;
		}
		//!< deinit packet
		if (mpp_packet_get_length(pkt) == 0) {
			if (mpp_packet_get_eos(pkt)) {
				end_of_flag = 1; //!< end of stream
				task->dec.valid = 0;
				mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
				mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
			}
			mpp_packet_deinit(&pkt);
			pkt = NULL;
		}
		//!< run hal module
		if (task->dec.valid) {
			if (mpp_buf_slot_is_changed(pApi->frame_slots)) {
				mpp_buf_slot_ready(pApi->frame_slots);
			}
			mpp_buf_slot_get_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, &pctx->m_dec_pic_buf);
			if (NULL == pctx->m_dec_pic_buf) {
				RK_U32 size = (RK_U32)mpp_buf_slot_get_size(pApi->frame_slots);
				mpp_buffer_get(pctx->mFrameGroup, &pctx->m_dec_pic_buf, size);
				if (pctx->m_dec_pic_buf)
					mpp_buf_slot_set_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, pctx->m_dec_pic_buf);
			}
			FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal, task));

			FUN_CHECK(ret = mpp_hal_hw_start(pApi->hal, task));
			FUN_CHECK(ret = mpp_hal_hw_wait(pApi->hal, task));
			if (pctx->m_dec_pkt_buf) {
				mpp_buffer_put(pctx->m_dec_pkt_buf);
				pctx->m_dec_pkt_buf = NULL;
			}
			mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
			mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
			//!< write frame out
			avsd_flush_frames(pApi, inp->fp_out);
			//!< clear refrece flag
			{
				RK_U32 i = 0;
				for (i = 0; i < MPP_ARRAY_ELEMS(task->dec.refer); i++) {
					if (task->dec.refer[i] >= 0) {
						mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.refer[i], SLOT_HAL_INPUT);
					}
				}
			}
			//!< reset task
			memset(task, 0, sizeof(HalTaskInfo));
			memset(task->dec.refer, -1, sizeof(task->dec.refer));
			task->dec.input = -1;

		}
	} while (!end_of_flag);
	//!< flush dpb and send to display
	FUN_CHECK(ret = mpp_dec_flush(pApi));
	avsd_flush_frames(pApi, inp->fp_out);

	ret = MPP_OK;
__FAILED:
	return ret;
}


int main(int argc, char **argv)
{
	MPP_RET ret = MPP_ERR_UNKNOW;
	AvsdTestCtx_t *pctx = NULL;

#if defined(_MSC_VER)
	mpp_env_set_u32("rkv_avsd_test_debug", 0xFF);
#endif
	mpp_env_get_u32("rkv_avsd_test_debug", &rkv_avsd_test_debug, 0x0F);
	MEM_CHECK(ret, pctx = mpp_calloc(AvsdTestCtx_t, 1));
	// read file
	FUN_CHECK(ret = avsd_input_init(&pctx->m_in, argc, argv));
	// init
	decoder_init(pctx);
	// decode
	ret = decoder_single_test(pctx);
	if (ret != MPP_OK) {
		AVSD_TEST_LOG(AVSD_TEST_TRACE, "[AVSD_TEST] Single-thread test error.");
		goto __FAILED;
	}
	
	ret = MPP_OK;
__FAILED:
	decoder_deinit(pctx);
	avsd_input_deinit(&pctx->m_in);
	MPP_FREE(pctx);
	AVSD_TEST_LOG(AVSD_TEST_TRACE, "[AVSD_TEST] decoder_deinit over.");

	return ret;
}

#endif