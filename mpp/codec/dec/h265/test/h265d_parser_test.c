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
/*
* @file       h265d_parser_test.c
* @brief
* @author      csy(csy@rock-chips.com)

* @version     1.0.0
* @history
*   2015.7.15 : Create
*/

#define MODULE_TAG "h265_test"

#include <string.h>

#include "mpp_env.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_dec.h"
#include "mpp_frame.h"

#include "h265d_api.h"
#include "hal_h265d_api.h"

#include "utils.h"
#include "openHevcWrapper.h"
#include "h265d_api.h"
#include "hal_h265d_api.h"

RK_U32 h265T_debug = 0;

#define H265T_ANSYN         (0x00000001)
#define H265T_DBG_LOG       (0x00000002)


typedef enum VPU_API_DEMO_RET {
    PARSER_DEMO_OK = 0,
    PARSER_DEMO_PARSE_HELP_OK  = 1,

    PARSER_DEMO_ERROR_BASE     = -100,
    ERROR_INVALID_PARAM     = PARSER_DEMO_ERROR_BASE - 1,
    ERROR_INVALID_STREAM    = PARSER_DEMO_ERROR_BASE - 2,
    ERROR_IO                = PARSER_DEMO_ERROR_BASE - 3,
    ERROR_MEMORY            = PARSER_DEMO_ERROR_BASE - 4,
    ERROR_INIT_VPU          = PARSER_DEMO_ERROR_BASE - 5,

    ERROR_VPU_DECODE        = PARSER_DEMO_ERROR_BASE - 90,
} PARSER_API_DEMO_RET;


typedef struct parserDemoCmdContext {
    RK_U32  width;
    RK_U32  height;
    RK_U8   input_file[200];
    RK_U8   output_file[200];
    RK_U8   have_input;
    RK_U8   have_output;
    RK_U8   disable_debug;
    RK_U32  record_frames;
    RK_S64  record_start_ms;
} ParserDemoCmdContext_t;

static OptionInfo parserCmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"vframes",         "number",               "set the number of video frames to record"},
    {"ss",              "time_off",             "set the start time offset, use Ms as the unit."},
    {"d",               "disable",              "disable the debug output info."},
};

static void show_usage()
{
    mpp_log("usage: parserDemo [options] input_file, \n\n");

    mpp_log("Getting help:\n");
    mpp_log("-help  --print options of vpu api demo\n");
}

static RK_S32 show_help()
{
    mpp_log("usage: parserDemo [options] input_file, \n\n");
    show_options(parserCmd);
    return 0;
}

static RK_S32 parse_options(int argc, char **argv, ParserDemoCmdContext_t* cmdCxt)
{
    const char *opt;
    RK_S32 optindex, handleoptions = 1, ret = 0;

    if ((argc < 2) || (cmdCxt == NULL)) {
        mpp_log("parser demo, input parameter invalid\n");
        show_usage();
        return  MPP_ERR_STREAM;
    }

    /* parse options */
    optindex = 1;
    while (optindex < argc) {
        opt = (const char*)argv[optindex++];

        if (handleoptions && opt[0] == '-' && opt[1] != '\0') {
            if (opt[1] == '-') {
                if (opt[2] != '\0') {
                    opt++;
                } else {
                    handleoptions = 0;
                    continue;
                }
            }

            opt++;

            switch (*opt) {
            case 'i':
                if (argv[optindex]) {
                    memcpy(cmdCxt->input_file, argv[optindex], strlen(argv[optindex]));
                    cmdCxt->input_file[strlen(argv[optindex])] = '\0';
                    cmdCxt->have_input = 1;
                } else {
                    mpp_log("input file is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 'o':
                if (argv[optindex]) {
                    memcpy(cmdCxt->output_file, argv[optindex], strlen(argv[optindex]));
                    cmdCxt->output_file[strlen(argv[optindex])] = '\0';
                    cmdCxt->have_output = 1;
                    break;
                } else {
                    mpp_log("out file is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
            case 'd':
                cmdCxt->disable_debug = 1;
                break;
            case 'w':
                if (argv[optindex]) {
                    cmdCxt->width = atoi(argv[optindex]);
                    break;
                } else {
                    mpp_log("input width is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
            case 'h':
                if ((*(opt + 1) != '\0') && !strncmp(opt, "help", 4)) {
                    show_help();
                    ret = PARSER_DEMO_PARSE_HELP_OK;
                    goto PARSE_OPINIONS_OUT;
                } else if (argv[optindex]) {
                    cmdCxt->height = atoi(argv[optindex]);
                } else {
                    mpp_log("input height is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
                break;

            default:
                if ((*(opt + 1) != '\0') && argv[optindex]) {
                    if (!strncmp(opt, "vframes", 7)) {
                        cmdCxt->record_frames = atoi(argv[optindex]);
                    } else if (!strncmp(opt, "ss", 2)) {
                        cmdCxt->record_start_ms = atoi(argv[optindex]);
                    } else {
                        ret = -1;
                        goto PARSE_OPINIONS_OUT;
                    }
                } else {
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            }

            optindex += ret;
        }
    }

PARSE_OPINIONS_OUT:
    if (ret < 0) {
        mpp_log("vpu api demo, input parameter invalid\n");
        show_usage();
        return  MPP_ERR_STREAM;
    }
    return ret;
}


RK_S32 find_start_code (RK_U8 *Buf, RK_S32 zeros_in_startcode)
{
    RK_S32 i;
    for (i = 0; i < zeros_in_startcode; i++)
        if (Buf[i] != 0)
            return 0;
    return Buf[i];
}

RK_S32 get_next_nal(FILE* inpf, unsigned char* Buf)
{
    RK_S32 pos = 0;
    RK_S32 StartCodeFound = 0;
    RK_S32 info2 = 0;
    RK_S32 info3 = 0;

    while (!feof(inpf) && (Buf[pos++] = fgetc(inpf)) == 0);

    while (pos < 3) Buf[pos++] = fgetc(inpf);
    while (!StartCodeFound) {
        if (feof (inpf)) {
            return pos - 1;
        }
        Buf[pos++] = fgetc(inpf);
        info3 = find_start_code(&Buf[pos - 4], 3);
        if (info3 != 1)
            info2 = find_start_code(&Buf[pos - 3], 2);
        StartCodeFound = (info2 == 1 || info3 == 1);
    }
    fseek(inpf, - 4 + info2, SEEK_CUR);
    return pos - 4 + info2;
}


static RK_S32 poll_task(void *hal, MppBufSlots slots, MppBufSlots packet_slots, HalDecTask *dec)
{
    HalTaskInfo syn;
    RK_U32 i;
    MppBuffer   dec_pkt_buf;
    syn.dec = *dec;
    hal_h265d_wait(hal, &syn);
    mpp_err("dec->output = %d", dec->output);
    mpp_buf_slot_clr_flag(slots, dec->output, SLOT_HAL_OUTPUT);

    for (i = 0; i < MPP_ARRAY_ELEMS(dec->refer); i++) {
        RK_S32 id;
        id = dec->refer[i];
        if (id >= 0)
            mpp_buf_slot_clr_flag(slots, id, SLOT_HAL_INPUT);
    }

    mpp_buf_slot_get_prop(packet_slots, dec->input,  SLOT_BUFFER, &dec_pkt_buf);
    mpp_buf_slot_clr_flag(packet_slots, dec->input, SLOT_HAL_INPUT);
    mpp_buffer_put(dec_pkt_buf);
    return MPP_OK;
}


RK_S32 hevc_parser_test(ParserDemoCmdContext_t *cmd)
{
    FILE* pInFile = NULL;
    RK_U8 * buf = NULL;
    RK_S32 nal_len = 0;
    void *mpp_codex_ctx = NULL;
    void *hal = NULL;
    MppBufSlots         slots;
    MppBufSlots         packet_slots;
    ParserCfg parser_cfg;
    MppHalCfg hal_cfg;
    MppBufferGroup  mFrameGroup = NULL;
    MppBufferGroup  mStreamGroup = NULL;
    MppPacket rkpkt = NULL;
    MppFrame frame = NULL;
    HalDecTask *curtask = NULL;
    HalDecTask *pretask = NULL;

    FILE * fp = NULL;
    RK_U32 wait_task = 0;
    MPP_RET ret = MPP_OK;

    if (fp == NULL) {
        fp = fopen("data/dump.yuv", "wb");
    }
    if (cmd->width == 0 || cmd->height == 0) {
        cmd->width = 4096;
        cmd->height = 2160;
    }

    curtask = mpp_calloc(HalDecTask, 1);
    pretask = mpp_calloc(HalDecTask, 1);

    mpp_codex_ctx = mpp_calloc_size(void, api_h265d_parser.ctx_size);
    mpp_err("api_h265d_parser.ctx_size = %d", api_h265d_parser.ctx_size);
    hal = mpp_calloc_size(void, hal_api_h265d.ctx_size);
    if (cmd->have_input) {
        mpp_log("input bitstream w: %d, h: %d, path: %s\n",
                cmd->width, cmd->height,
                cmd->input_file);

        mpp_log("fopen in \n");
        pInFile = fopen((const char*)cmd->input_file, "rb");
        if (pInFile == NULL) {
            mpp_log("input file not exsist\n");
            return ERROR_INVALID_PARAM;
        }

        mpp_log("fopen out \n");
    } else {
        mpp_log("please set input bitstream file\n");
        return -1;
    }
    mpp_log("mallc in void * value %d \n", sizeof(void *));
    buf = mpp_malloc(RK_U8, 2048000);
    mpp_buf_slot_init(&slots);

    mpp_env_get_u32("h265t_debug", &h265T_debug, 0);
    mpp_buf_slot_init(&packet_slots);
    mpp_buf_slot_setup(packet_slots, 2);
    if (NULL == slots) {
        mpp_err("could not init buffer slot\n");
        return MPP_ERR_UNKNOW;
    }
    if (mFrameGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&mFrameGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
        //mpp_buffer_group_limit_config(mFrameGroup,cmd->width*cmd->height*2,20);
    }

    if (mStreamGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&mStreamGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }
#ifndef RKPLATFORM
#ifdef COMPARE
    {
        void *openHevcHandle = NULL;
        openHevcHandle = libOpenHevcInit(1, 1);
        libOpenHevcSetCheckMD5(openHevcHandle, 0);
        libOpenHevcSetTemporalLayer_id(openHevcHandle, 7);
        libOpenHevcSetActiveDecoders(openHevcHandle, 0);
        libOpenHevcSetDebugMode(openHevcHandle, 0);
        libOpenHevcStartDecoder(openHevcHandle);
        if (!openHevcHandle) {
            fprintf(stderr, "could not open OpenHevc\n");
            exit(1);
        }
    }
#endif
#endif
    parser_cfg.frame_slots = slots;
    parser_cfg.packet_slots = packet_slots;
    parser_cfg.need_split = 1;
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.frame_slots = slots;
    hal_cfg.packet_slots = packet_slots;
    h265d_init(mpp_codex_ctx, &parser_cfg);
    hal_h265d_init(hal, &hal_cfg);
    mpp_log("mallc out \n");
    if (buf == NULL) {
        mpp_log("malloc fail for input buf");
    }
    //buf[0] = 0;
    memset(curtask, 0, sizeof(HalDecTask));
    memset(&curtask->refer, -1, sizeof(curtask->refer));
    curtask->input = -1;
    while (!feof(pInFile)) {
        RK_S32 index;
        RK_U8 *tmpbuf = buf;
        RK_U8 *pos = NULL;

        nal_len = get_next_nal(pInFile, buf);
        mpp_err("get nal len from file %d", nal_len);
        do {
            mpp_packet_init(&rkpkt, tmpbuf, nal_len);
            if (-1 == curtask->input) {
                if (MPP_OK == mpp_buf_slot_get_unused(packet_slots, &index) ) {
                    MppBuffer buffer = NULL;
                    curtask->input = index;

                    mpp_buf_slot_get_prop(packet_slots, index, SLOT_BUFFER, &buffer);
                    if (NULL == buffer) {
                        RK_U32 size = (RK_U32)mpp_buf_slot_get_size(packet_slots);
                        if (size == 0) {
                            size = 2 * 1024 * 1024;
                        }
                        mpp_buffer_get(mStreamGroup, &buffer, size);
                        if (buffer != NULL)
                            mpp_err("mpp_buf_slot_set_prop %p", buffer);
                        mpp_buf_slot_set_prop(packet_slots, index, SLOT_BUFFER, buffer);
                    }
                }
            }
            h265d_prepare(mpp_codex_ctx, rkpkt, curtask);
            pos = (RK_U8 *)mpp_packet_get_pos(rkpkt);
            if (pos < (tmpbuf + nal_len)) {
                tmpbuf = pos;
                nal_len = (RK_S32)((tmpbuf + nal_len) - pos);
                mpp_err("nal_len = %d", nal_len);
            } else {
                nal_len = 0;
            }
            if (curtask->valid) {
                if (h265T_debug & H265T_ANSYN) {
                    if (wait_task) {
                        poll_task(hal, slots, packet_slots, pretask);
                        wait_task = 0;
                    }
                }
                curtask->valid = -1;
#ifndef RKPLATFORM
#ifdef COMPARE
                mpp_err("hevc_decode_frame in \n");
                void *sliceInfo = NULL;
                RK_U8 *split_out_buf = NULL;
                RK_S32 split_size = 0;
                h265d_get_stream(mpp_codex_ctx, &split_out_buf, &split_size);
                libOpenHevcDecode(openHevcHandle, split_out_buf, split_size, 0);
                sliceInfo = libOpenHevcGetSliceInfo(openHevcHandle);
                mpp_err("open hevc out \n");
                h265d_set_compare_info(mpp_codex_ctx, sliceInfo);
#endif
#endif
                h265d_parse(mpp_codex_ctx, curtask);
            }
            if (curtask->valid) {
                HalTaskInfo syn;
                MppBuffer buffer = NULL;
                syn.dec = *curtask;
                index = curtask->output;

                if (mpp_buf_slot_is_changed(slots)) {
                    mpp_buf_slot_ready(slots);
                }

                mpp_buf_slot_get_prop(slots, index, SLOT_BUFFER, &buffer);
                if (NULL == buffer) {
                    RK_U32 size = (RK_U32)mpp_buf_slot_get_size(slots);
                    if (size == 0) {
                        size = cmd->width * cmd->height * 2;
                    }
                    mpp_buffer_get(mFrameGroup, &buffer, size);
                    if (buffer)
                        mpp_buf_slot_set_prop(slots, index, SLOT_BUFFER, buffer);
                }

                hal_h265d_gen_regs(hal, &syn);

                hal_h265d_start(hal, &syn);

                if (h265T_debug & H265T_ANSYN) {
                    HalDecTask *task = NULL;
                    task = curtask;
                    curtask = pretask;
                    pretask = task;
                    wait_task = 1;
                } else {
                    poll_task(hal, slots, packet_slots, curtask);
                }
                memset(curtask, 0, sizeof(HalDecTask));
                memset(&curtask->refer, -1, sizeof(curtask->refer));
                curtask->input = -1;

            }

            do {
                ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DISPLAY);
                if (ret == MPP_OK) {
                    mpp_buf_slot_get_prop(slots, index, SLOT_FRAME, &frame);
                    if (frame) {
#ifdef DUMP
                        RK_U32 stride_w, stride_h;
                        void *ptr = NULL;
                        MppBuffer framebuf;
                        stride_w = mpp_frame_get_hor_stride(frame);
                        stride_h = mpp_frame_get_ver_stride(frame);
                        framebuf = mpp_frame_get_buffer(frame);
                        ptr = mpp_buffer_get_ptr(framebuf);
                        if (fp & ptr != NULL) {
                            fwrite(ptr, 1, stride_w * stride_h * 3 / 2, fp);
                            fflush(fp);
                        }
#endif
                        mpp_frame_deinit(&frame);
                        frame = NULL;
                    }
                    mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
                }

            } while (ret == MPP_OK);
            mpp_packet_deinit(&rkpkt);
        } while ( nal_len );
    }

    if (h265T_debug & H265T_ANSYN) {
        if (wait_task) {
            poll_task(hal, slots, packet_slots, pretask);
            wait_task = 0;
        }
    }

    if (-1 != curtask->input) {
        MppBuffer buffer = NULL;
        mpp_buf_slot_set_flag(packet_slots, curtask->input, SLOT_CODEC_READY);
        mpp_buf_slot_set_flag(packet_slots, curtask->input, SLOT_HAL_INPUT);
        mpp_buf_slot_get_prop(packet_slots, curtask->input, SLOT_BUFFER, &buffer);
        mpp_buf_slot_clr_flag(packet_slots, curtask->input, SLOT_HAL_INPUT);
        mpp_err("mpp_buf_slot free for last packet %p", buffer);
        mpp_buffer_put(buffer);
    }

    h265d_flush((void*)mpp_codex_ctx);
    do {
        RK_S32 index;
        ret = mpp_buf_slot_dequeue(slots, &index, QUEUE_DISPLAY);
        if (ret == MPP_OK) {
            mpp_log("get_display for index = %d", index);
            mpp_buf_slot_get_prop(slots, index, SLOT_FRAME, &frame);
            if (frame) {
                mpp_frame_deinit(&frame);
                frame = NULL;
            }
            mpp_buf_slot_clr_flag(slots, index, SLOT_QUEUE_USE);
        }

    } while (ret == MPP_OK);

    if (mpp_codex_ctx != NULL) {
        h265d_deinit(mpp_codex_ctx);
        mpp_codex_ctx = NULL;
    }

    if (hal != NULL) {
        mpp_err("hal_h265d_deinit in");
        hal_h265d_deinit(hal);
    }
    if (slots != NULL) {
        mpp_err("frame slots deInit");
        mpp_buf_slot_deinit(slots);
    }

    if (packet_slots != NULL) {
        mpp_err("packet slots deInit");
        mpp_buf_slot_deinit(packet_slots);
    }
    if (mFrameGroup != NULL) {
        mpp_err("mFrameGroup deInit");
        mpp_buffer_group_put(mFrameGroup);
    }

    if (mStreamGroup != NULL) {
        mpp_err("mStreamGroup deInit");
        mpp_buffer_group_put(mStreamGroup);
    }
#ifndef RKPLATFORM
#ifdef COMPARE
    if (openHevcHandle != NULL) {
        libOpenHevcClose(openHevcHandle);
        openHevcHandle = NULL;
    }
#endif
#endif
    if (fp) {
        fclose(fp);
    }

    mpp_free(buf);
    mpp_free(mpp_codex_ctx);
    mpp_free(hal);
    return 0;
}

int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    ParserDemoCmdContext_t demoCmdCtx;
    ParserDemoCmdContext_t* cmd = NULL;

    if (argc == 1) {
        show_usage();
        mpp_log("vpu api demo complete directly\n");
        return 0;
    }

    //mpp_env_set_u32("buf_slot_debug", 0x10000010, 0);
    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    cmd = &demoCmdCtx;
    memset((void*)cmd, 0, sizeof(ParserDemoCmdContext_t));
    if ((ret = parse_options(argc, argv, cmd)) != 0) {
        if (ret == PARSER_DEMO_PARSE_HELP_OK) {
            return 0;
        }

        mpp_log("parse_options fail\n\n");
        show_usage();
        return 0;
    }
    hevc_parser_test(cmd);
    return 0;
}
