/*
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
#include "avsd_parse.h"
#include "hal_avsd_api.h"


#define AVSD_TEST_ERROR       (0x00000001)
#define AVSD_TEST_ASSERT      (0x00000002)
#define AVSD_TEST_WARNNING    (0x00000004)
#define AVSD_TEST_TRACE       (0x00000008)


#define AVSD_TEST_LOG(level, fmt, ...)\
do {\
if (level & avsd_test_debug)\
        { mpp_log(fmt, ## __VA_ARGS__); }\
} while (0)


typedef struct ParserImpl_t {
    ParserCfg           cfg;

    const ParserApi     *api;
    void                *ctx;
} ParserImpl;


static RK_U32 avsd_test_debug = 0;

typedef struct inp_par_t {
    FILE  *fp_in;
    FILE  *fp_out;
    FILE  *fp_chk;

    RK_U8 *pktbuf;
    RK_U32 pktsize;
    RK_U32 pktlen;

    RK_U32 nalu_find;
    RK_U32 frame_find;
    RK_U32 prefix;

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

    RK_U32         display_no;
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
        pctx->m_dec_pkt_buf = NULL;
    }
    if (pctx->m_dec_pic_buf) {
        mpp_buffer_put(pctx->m_dec_pic_buf);
        pctx->m_dec_pic_buf = NULL;
    }
    if (pctx->mFrameGroup) {
        mpp_err("mFrameGroup deInit");
        mpp_buffer_group_put(pctx->mFrameGroup);
        pctx->mFrameGroup = NULL;
    }
    if (pctx->mStreamGroup) {
        mpp_err("mStreamGroup deInit");
        mpp_buffer_group_put(pctx->mStreamGroup);
        pctx->mStreamGroup = NULL;
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
#ifdef RKPLATFORM
        mpp_log_f("mFrameGroup used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mFrameGroup, MPP_BUFFER_TYPE_NORMAL));
#endif
    }
    if (pctx->mStreamGroup == NULL) {
#ifdef RKPLATFORM
        mpp_log_f("mStreamGroup used ion In");
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_ION));
#else
        FUN_CHECK(ret = mpp_buffer_group_get_internal(&pctx->mStreamGroup, MPP_BUFFER_TYPE_NORMAL));
#endif
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
    hal_cfg.device_id = HAL_RKVDEC;
    hal_cfg.frame_slots = pApi->frame_slots;
    hal_cfg.packet_slots = pApi->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    hal_cfg.hal_int_cb.opaque = ((ParserImpl *)(pApi->parser))->ctx;
    hal_cfg.hal_int_cb.callBack = api_avsd_parser.callback;
    FUN_CHECK(ret = mpp_hal_init(&pApi->hal, &hal_cfg));
    pApi->tasks = hal_cfg.tasks;

    return MPP_OK;

__FAILED:
    decoder_deinit(pctx);

    return ret;
}

static MPP_RET avsd_flush_frames(MppDec *pApi, AvsdTestCtx_t *pctx, FILE *fp_out, FILE *fp_chk)
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
            if (fp_out) {
                fwrite(ptr, 1, stride_w * stride_h * 3 / 2, fp_out);
                fflush(fp_out);
            }
            mpp_frame_deinit(&out_frame);
            out_frame = NULL;
        }
        mpp_buf_slot_clr_flag(pApi->frame_slots, slot_idx, SLOT_QUEUE_USE);
        pctx->display_no++;
    }
    (void)fp_chk;
    return MPP_OK;
}


static MPP_RET avsd_input_deinit(InputParams *inp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;

    MPP_FREE(inp->pktbuf);
    MPP_FCLOSE(inp->fp_in);
    MPP_FCLOSE(inp->fp_out);
    MPP_FCLOSE(inp->fp_chk);

    return ret = MPP_OK;
}

static MPP_RET avsd_input_init(InputParams *inp, RK_S32 ac, char *av[])
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_S32 ac_cnt = 1;

    while (ac_cnt < ac) {
        if (!strncmp(av[ac_cnt], "-h", 2)) {
            mpp_log("Options:");
            mpp_log("   -h     : prints help message.");
            mpp_log("   -i     :[file]   Set input AVS+ bitstream file.");
            mpp_log("   -o     :[file]   Set output YUV file.");
            mpp_log("   -c     :[file]   Set input check file.");
            mpp_log("   -n     :[number] Set decoded frames.");
            ac_cnt += 1;
        } else if (!strncmp(av[ac_cnt], "-i", 2)) {
            AVSD_TEST_LOG(AVSD_TEST_TRACE, "Input AVS+ bitstream : %s", av[ac_cnt + 1]);
            if ((inp->fp_in = fopen(av[ac_cnt + 1], "rb")) == NULL) {
                mpp_err_f("error, open file %s ", av[ac_cnt + 1]);
            }
            ac_cnt += 2;
        } else if (!strncmp(av[ac_cnt], "-n", 2)) {
            if (!sscanf(av[ac_cnt + 1], "%d", &inp->dec_num)) {
                goto __FAILED;
            }
            ac_cnt += 2;
        } else if (!strncmp(av[ac_cnt], "-o", 2)) {
            AVSD_TEST_LOG(AVSD_TEST_TRACE, "Output decoded YUV   : %s", av[ac_cnt + 1]);
            if ((inp->fp_out = fopen(av[ac_cnt + 1], "wb")) == NULL) {
                mpp_err_f("error, open file %s ", av[ac_cnt + 1]);
            }
            ac_cnt += 2;
        } else if (!strncmp(av[ac_cnt], "-c", 2)) {
            AVSD_TEST_LOG(AVSD_TEST_TRACE, "Input check data  : %s", av[ac_cnt + 1]);
            if ((inp->fp_chk = fopen(av[ac_cnt + 1], "rb")) == NULL) {
                mpp_err_f("error, open file %s ", av[ac_cnt + 1]);
            }
            ac_cnt += 2;
        } else {
            mpp_err("error, %s cannot explain command! \n", av[ac_cnt]);
            goto __FAILED;
        }
    }
    //!< malloc packet buffer
    inp->pktsize = 2 * 1024 * 1024;
    MEM_CHECK(ret, inp->pktbuf = mpp_malloc(RK_U8, inp->pktsize));
    inp->pktlen = 0;

    return MPP_OK;
__FAILED:
    avsd_deinit(inp);

    return ret;
}

static MPP_RET avsd_read_data(InputParams *inp)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    RK_U8  curdata = 0;

    while (!inp->is_eof) {
        //!< copy header
        if (!inp->nalu_find && (inp->prefix & 0xFFFFFF00) == 0x00000100) {
            RK_U8 *p_data = (RK_U8 *)&inp->prefix;
            inp->nalu_find = 1;
            inp->pktbuf[inp->pktlen++] = p_data[3];
            inp->pktbuf[inp->pktlen++] = p_data[2];
            inp->pktbuf[inp->pktlen++] = p_data[1];
            inp->pktbuf[inp->pktlen++] = p_data[0];
        }
        //!< find frame end
        if ((inp->prefix == I_PICUTRE_START_CODE) || (inp->prefix == PB_PICUTRE_START_CODE)) {
            if (inp->frame_find) {
                inp->pktlen -= sizeof(inp->prefix);
                inp->nalu_find = 0;
                inp->frame_find = 0;
                break;
            }
            inp->frame_find = 1;
        }
        //!< read one byte
        fread(&curdata, sizeof(RK_U8), 1, inp->fp_in);
        inp->prefix = (inp->prefix << 8) | curdata;
        inp->is_eof = feof(inp->fp_in);
        //!< copy data
        if (!inp->is_eof && inp->nalu_find) {
            inp->pktbuf[inp->pktlen++] = curdata;
        }
    }
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
            } else {
                RK_U32 frame_num = 0;
                while (frame_num < 1) {
                    FUN_CHECK(ret = avsd_read_data(inp));
                    frame_num++;
                }
                //if (inp->fp_out) {
                //    fwrite(inp->pktbuf, 1, inp->pktlen, inp->fp_out);
                //    fflush(inp->fp_out);
                //}
                mpp_packet_init(&pkt, inp->pktbuf, inp->pktlen);
                inp->pktlen = 0;
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
                if (task->dec.valid) {
                    mpp_buf_slot_clr_flag(pApi->packet_slots, task->dec.input, SLOT_HAL_INPUT);
                    mpp_buf_slot_clr_flag(pApi->frame_slots, task->dec.output, SLOT_HAL_OUTPUT);
                    task->dec.valid = 0;
                }
                end_of_flag = 1; //!< end of stream
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
                mpp_log_f("get_slot_size=%d \n", size);
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
            avsd_flush_frames(pApi, pctx, NULL/*inp->fp_out*/, inp->fp_chk);
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
    avsd_flush_frames(pApi, pctx, NULL/*inp->fp_out*/, inp->fp_chk);

    ret = MPP_OK;
__FAILED:
    return ret;
}


int main(int argc, char **argv)
{
    MPP_RET ret = MPP_ERR_UNKNOW;
    AvsdTestCtx_t m_decoder;
    AvsdTestCtx_t *p_dec = &m_decoder;

#if defined(_MSC_VER)
    mpp_env_set_u32("avsd_test_debug", 0xFF);
#endif
    mpp_env_get_u32("avsd_test_debug", &avsd_test_debug, 0);
    memset(p_dec, 0, sizeof(AvsdTestCtx_t));
    // read file
    FUN_CHECK(ret = avsd_input_init(&p_dec->m_in, argc, argv));
    // init
    decoder_init(p_dec);
    // decode
    ret = decoder_single_test(p_dec);
    if (ret != MPP_OK) {
        AVSD_TEST_LOG(AVSD_TEST_TRACE, "[AVSD_TEST] Single-thread test error.");
        goto __FAILED;
    }

    ret = MPP_OK;
__FAILED:
    decoder_deinit(p_dec);
    avsd_input_deinit(&p_dec->m_in);

    AVSD_TEST_LOG(AVSD_TEST_TRACE, "[AVSD_TEST] decoder_deinit over.");

    return ret;

}

