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

#define MODULE_TAG "h264d_test"


#if defined(_WIN32)
#include "vld.h"
#endif
#include <string.h>
#include "mpp_log.h"
#include "mpp_env.h"
#include "mpp_dec.h"
#include "mpp_hal.h"
#include "mpp_mem.h"
#include "mpp_packet.h"
#include "mpp_packet_impl.h"
#include "mpp_buffer.h"
#include "mpp_buffer_impl.h"
#include "mpp_hal.h"

#include "h264d_log.h"
#include "h264d_rwfile.h"
#include "h264d_api.h"
#include "hal_task.h"
#include "hal_h264d_api.h"


static MPP_RET manual_set_env(void)
{
#if defined(_MSC_VER)
    mpp_env_set_u32("h264d_log_help",     1             );
    mpp_env_set_u32("h264d_log_show",     1             );
    mpp_env_set_u32("h264d_log_ctrl",     0x800B        );
    mpp_env_set_u32("h264d_log_level",    5             );
    mpp_env_set_u32("h264d_log_decframe", 0             );
    mpp_env_set_u32("h264d_log_begframe", 0             );
    mpp_env_set_u32("h264d_log_endframe", 0             );
    mpp_env_set_u32("h264d_log_yuv",      0             );
    mpp_env_set_u32("h264d_chg_org",      0             );  //!< 0:VDPU 1: RKVDEC
    mpp_env_set_str("h264d_log_outpath",  "F:/h264_log/allegro_dat" );
    mpp_env_set_str("h264d_log_cmppath",  "F:/h264_log_driver/trunk_dat" );
#endif
    return MPP_OK;
}

static MPP_RET decoder_deinit(MppDec *pApi)
{
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

    return MPP_OK;
}

static MPP_RET decoder_init(MppDec *pApi)
{
    RK_U32 hal_device_id = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    ParserCfg       parser_cfg;
    MppHalCfg       hal_cfg;

    pApi->coding = MPP_VIDEO_CodingAVC;
    // malloc slot
    FUN_CHECK(ret = mpp_buf_slot_init(&pApi->frame_slots));
    MEM_CHECK(ret, pApi->frame_slots);
    ret = mpp_buf_slot_init(&pApi->packet_slots);
    MEM_CHECK(ret, pApi->packet_slots);
    mpp_buf_slot_setup(pApi->packet_slots, 2);
    // init parser part
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.coding = pApi->coding;
    parser_cfg.frame_slots  = pApi->frame_slots;
    parser_cfg.packet_slots = pApi->packet_slots;
    parser_cfg.task_count   = 2;
    FUN_CHECK(ret = parser_init(&pApi->parser, &parser_cfg));
    // init hal part
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pApi->coding;
    hal_cfg.work_mode = HAL_MODE_LIBVPU;
    mpp_env_get_u32("h264d_chg_org", &hal_device_id, 0);
	//if (hal_device_id == 1) {
        hal_cfg.device_id = HAL_RKVDEC;
	//} else {
    //    hal_cfg.device_id = HAL_VDPU;
	//}
    hal_cfg.frame_slots = pApi->frame_slots;
    hal_cfg.packet_slots = pApi->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;
__FAILED:
    decoder_deinit(pApi);

    return ret;
}



int main(int argc, char **argv)
{
    RK_U32 i = 0;
	RK_U32 end_of_flag = 0;
    RK_S32 frame_slot_idx = 0;
    MPP_RET ret = MPP_ERR_UNKNOW;
    MppPacket pkt = NULL;
    MppFrame out_frame = NULL;
    RK_U32 out_yuv_flag = 0;
    MppBuffer dec_pkt_buf = NULL;
	MppBuffer dec_pic_buf = NULL;
    MppBufferGroup  mFrameGroup = NULL;
    MppBufferGroup  mStreamGroup = NULL;

    InputParams *pIn = mpp_calloc(InputParams, 1);
    MppDec *pApi = mpp_calloc(MppDec, 1);
    HalTaskInfo *task = mpp_calloc_size(HalTaskInfo, sizeof(HalTaskInfo));

    if (mFrameGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&mFrameGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h264d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    if (mStreamGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&mStreamGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h264d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    //if (g_debug_file0 == NULL) {
    //  g_debug_file0 = fopen("rk_debugfile_view0.txt", "wb");
    //}
	//if (g_debug_file1 == NULL) {
	//	g_debug_file1 = fopen("rk_debugfile_view1.txt", "wb");
	//}

    MEM_CHECK(ret, pIn && pApi && task);
    mpp_log("== test start == \n");
    // set debug mode
    FUN_CHECK(ret = manual_set_env());
    // input paramters configure
    FUN_CHECK(ret = h264d_configure(pIn, argc, argv));
    // open files
    FUN_CHECK(ret = h264d_open_files(pIn));
    FUN_CHECK(ret = h264d_alloc_frame_buffer(pIn));

    //!< init decoder
    FUN_CHECK(ret = decoder_init(pApi));
	
    //!< initial task
    memset(task, 0, sizeof(HalTaskInfo));
    memset(task->dec.refer, -1, sizeof(task->dec.refer));
    task->dec.input = -1;

    do {
        //!< get one packet
        if (pkt == NULL) {
            if (pIn->is_eof || 
				(pIn->iDecFrmNum && (pIn->iFrmdecoded >= pIn->iDecFrmNum))) {
                mpp_packet_init(&pkt, NULL, 0);
                mpp_packet_set_eos(pkt);
				//FPRINT(g_debug_file0, "[READ_PKT]pIn->iFrmdecoded=%d, strm_pkt_len=%d, is_eof=%d \n", pIn->iFrmdecoded, 0, 1);
            } else {
                FUN_CHECK(ret = h264d_read_one_frame(pIn));
                mpp_packet_init(&pkt, pIn->strm.pbuf, pIn->strm.strmbytes);
				//FPRINT(g_debug_file0, "[READ_PKT]pIn->iFrmdecoded=%d, strm_pkt_len=%d, is_eof=%d \n", 
					//pIn->iFrmdecoded, pIn->strm.strmbytes, 0);
            }
            mpp_log("---- decoder, read_one_frame Frame_no = %d \n", pIn->iFrmdecoded++);
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

            mpp_buf_slot_get_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, &dec_pkt_buf);
            if (NULL == dec_pkt_buf) {
                size_t stream_size = mpp_packet_get_size(task->dec.input_packet);
                mpp_buffer_get(mStreamGroup, &dec_pkt_buf, stream_size);
                if (dec_pkt_buf)
                    mpp_buf_slot_set_prop(pApi->packet_slots, task->dec.input, SLOT_BUFFER, dec_pkt_buf);
            }
            buf = (MppBufferImpl *)dec_pkt_buf;
            memcpy(buf->info.ptr, mpp_packet_get_data(task->dec.input_packet), mpp_packet_get_length(task->dec.input_packet));

            mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_CODEC_READY);
            mpp_buf_slot_set_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
            FUN_CHECK(ret = parser_parse(pApi->parser, &task->dec));
        }
        //!< deinit packet
        if (mpp_packet_get_length(pkt) == 0) {
            if (mpp_packet_get_eos(pkt)) {
                end_of_flag = 1; //!< end of stream
            }
            mpp_packet_deinit(&pkt);
            pkt = NULL;
        }
        //!< run hal module
        if (task->dec.valid) {
            mpp_buf_slot_get_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, &dec_pic_buf);
            if (NULL == dec_pic_buf) {
                RK_U32 size = (RK_U32)mpp_buf_slot_get_size(pApi->frame_slots);
               // mpp_err("run hal module, size = %d", size);
                mpp_buffer_get(mFrameGroup, &dec_pic_buf, size);
                if (dec_pic_buf)
                    mpp_buf_slot_set_prop(pApi->frame_slots, task->dec.output, SLOT_BUFFER, dec_pic_buf);
            }
            FUN_CHECK(ret = mpp_hal_reg_gen(pApi->hal, task));
            FUN_CHECK(ret = mpp_hal_hw_start(pApi->hal, task));
            FUN_CHECK(ret = mpp_hal_hw_wait(pApi->hal, task));
            if (dec_pkt_buf) {
                mpp_buffer_put(dec_pkt_buf);
            }
            mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
            mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);

            //!< write frame out
            mpp_env_get_u32("h264d_log_yuv", &out_yuv_flag, 0);
            //mpp_log("h264d_log_yuv=%d", out_yuv_flag);
            while (MPP_OK == mpp_buf_slot_dequeue(pApi->frame_slots, &frame_slot_idx, QUEUE_DISPLAY)) {
                mpp_buf_slot_get_prop(pApi->frame_slots, frame_slot_idx, SLOT_FRAME, &out_frame);
                //mpp_log("write picture out, slot_idx=%d, yuv_fp_is_null=%d", frame_slot_idx, (pIn->fp_yuv_data == NULL));
                if (out_frame) {
                    RK_U32 stride_w, stride_h;
                    void *ptr = NULL;
                    MppBuffer framebuf;
                    stride_w = mpp_frame_get_hor_stride(out_frame);
                    stride_h = mpp_frame_get_ver_stride(out_frame);
                    framebuf = mpp_frame_get_buffer(out_frame);
                    ptr = mpp_buffer_get_ptr(framebuf);
                    if (out_yuv_flag && pIn->fp_yuv_data) {
                        fwrite(ptr, 1, stride_w * stride_h * 3 / 2, pIn->fp_yuv_data);
                        fflush(pIn->fp_yuv_data);
                    }
                    mpp_frame_deinit(&out_frame);
                    out_frame = NULL;
                }
                mpp_buf_slot_clr_flag(pApi->frame_slots, frame_slot_idx, SLOT_QUEUE_USE);
            }
            //!< clear refrece flag
            for (i = 0; i < MPP_ARRAY_ELEMS(task->dec.refer); i++) {
                if (task->dec.refer[i] >= 0) {
                    mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.refer[i], SLOT_HAL_INPUT);
                }
            }
            //!< reset task
            memset(task, 0, sizeof(HalTaskInfo));
            memset(task->dec.refer, -1, sizeof(task->dec.refer));
            task->dec.input = -1;
        }
    } while (!end_of_flag);

    //FPRINT(g_debug_file1, "[FLUSH] flush begin \n");
    //!< flush dpb and send to display	
	FUN_CHECK(ret = mpp_dec_flush(pApi));	
	while (MPP_OK == mpp_buf_slot_dequeue(pApi->frame_slots, &frame_slot_idx, QUEUE_DISPLAY)) {
		//mpp_log("get_display for index = %d", frame_slot_idx);
		mpp_buf_slot_get_prop(pApi->frame_slots, frame_slot_idx, SLOT_FRAME, &out_frame);
		if (out_frame) {
			mpp_frame_deinit(&out_frame);
		}
		mpp_buf_slot_clr_flag(pApi->frame_slots, frame_slot_idx, SLOT_QUEUE_USE);
	}

    //FPRINT(g_debug_file1, "+++++++ all test return +++++++ \n");
    ret = MPP_OK;
__FAILED:
    decoder_deinit(pApi);
    h264d_free_frame_buffer(pIn);
    //h264d_write_fpga_data(pIn);  //!< for fpga debug
    h264d_close_files(pIn);
    MPP_FREE(pIn);
    MPP_FREE(pApi);
    MPP_FREE(task);

    if (dec_pkt_buf) {
        mpp_buffer_put(dec_pkt_buf);
    }
    if (dec_pic_buf) {
        mpp_buffer_put(dec_pic_buf);
    }
    if (mFrameGroup != NULL) {
        mpp_err("mFrameGroup deInit");
        mpp_buffer_group_put(mFrameGroup);
    }
    if (mStreamGroup != NULL) {
        mpp_err("mStreamGroup deInit");
        mpp_buffer_group_put(mStreamGroup);
    }

    return ret;
}

