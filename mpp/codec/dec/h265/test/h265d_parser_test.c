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
/*
* @file       h265d_parser_test.c
* @brief
* @author      csy(csy@rock-chips.com)

* @version     1.0.0
* @history
*   2015.7.15 : Create
*/

#define MODULE_TAG "h265_test"

#include "h265d_api.h"
#include "hal_h265d_api.h"

#include "mpp_log.h"
#include <string.h>
#include <stdlib.h>
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_frame.h"
#include "openHevcWrapper.h"
#include "mpp_buf_slot.h"
#include "rk_type.h"
#include "mpp_common.h"
#include "mpp_dec.h"
#include "h265d_api.h"
#include "hal_h265d_api.h"


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

typedef struct ParserCmd {
    RK_U8* name;
    RK_U8* argname;
    RK_U8* help;
} ParserCmd_t;

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


static ParserCmd_t parserCmd[] = {
    {(RK_U8*)"i",               (RK_U8*)"input_file",           (RK_U8*)"input bitstream file"},
    {(RK_U8*)"o",               (RK_U8*)"output_file",          (RK_U8*)"output bitstream file, "},
    {(RK_U8*)"w",               (RK_U8*)"width",                (RK_U8*)"the width of input bitstream"},
    {(RK_U8*)"h",               (RK_U8*)"height",               (RK_U8*)"the height of input bitstream"},
    {(RK_U8*)"vframes",         (RK_U8*)"number",               (RK_U8*)"set the number of video frames to record"},
    {(RK_U8*)"ss",              (RK_U8*)"time_off",             (RK_U8*)"set the start time offset, use Ms as the unit."},
    {(RK_U8*)"d",               (RK_U8*)"disable",              (RK_U8*)"disable the debug output info."},
};

static void show_usage()
{
    mpp_log("usage: parserDemo [options] input_file, \n\n");

    mpp_log("Getting help:\n");
    mpp_log("-help  --print options of vpu api demo\n");
}

static RK_S32 show_help()
{
    RK_U32 i = 0;
    RK_U32 n = sizeof(parserCmd) / sizeof(ParserCmd_t);

    mpp_log("usage: parserDemo [options] input_file, \n\n");
    for (i = 0; i < n; i++) {
        mpp_log("-%s  %s\t\t%s\n",
                parserCmd[i].name, parserCmd[i].argname, parserCmd[i].help);
    }

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


static RK_S32 poll_task(void *hal, MppBufSlots slots, HalDecTask *dec)
{
    HalTask syn;
    RK_U32 i;
    syn.dec = *dec;
    hal_h265d_wait(hal, &syn);
    mpp_err("dec->output = %d", dec->output);
    mpp_buf_slot_clr_hw_dst(slots, dec->output);
    for (i = 0; i < MPP_ARRAY_ELEMS(dec->refer); i++) {
        RK_S32 id;
        id = dec->refer[i];
        if (id >= 0)
            mpp_buf_slot_dec_hw_ref(slots, id);
    }

    return MPP_OK;
}


RK_S32 hevc_parser_test(ParserDemoCmdContext_t *cmd)
{
    FILE* pInFile = NULL;
    RK_U8 * buf = NULL;
    RK_S32 nal_len = 0;
    void *mpp_codex_ctx = NULL;
    void *hal = NULL;
//   void *openHevcHandle = NULL;
    MppBufSlots         slots;
    ParserCfg parser_cfg;
    MppHalCfg hal_cfg;
    MppBufferGroup  mFrameGroup = NULL;
    MppBufferGroup  mStreamGroup = NULL;
    MppPacket rkpkt = NULL;
    MppFrame frame = NULL;

    MppBuffer prestrem = NULL;
    MppBuffer currentstrem = NULL;

    HalDecTask *cutask = NULL;
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

    cutask = mpp_calloc(HalDecTask, 1);
    pretask =  mpp_calloc(HalDecTask, 1);

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
    mpp_buf_slot_setup(slots, 20, cmd->width * cmd->height * 2, 0);
    if (NULL == slots) {
        mpp_err("could not init buffer slot\n");
        return MPP_ERR_UNKNOW;
    }
    if (mFrameGroup == NULL) {
        ret = mpp_buffer_group_normal_get(&mFrameGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
        //mpp_buffer_group_limit_config(mFrameGroup,cmd->width*cmd->height*2,20);
    }

    if (mStreamGroup == NULL) {
        ret = mpp_buffer_group_normal_get(&mStreamGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("h265d mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    mpp_buffer_get(mStreamGroup, &currentstrem, 1024 * 1024);
    mpp_buffer_get(mStreamGroup, &prestrem, 2 * 1024 * 1024);

    parser_cfg.frame_slots = slots;
    hal_cfg.frame_slots = slots;
    h265d_init(mpp_codex_ctx, &parser_cfg);
    hal_h265d_init(hal, &hal_cfg);
    mpp_log("mallc out \n");
    if (buf == NULL) {
        mpp_log("malloc fail for input buf");
    }
    //buf[0] = 0;
    while (!feof(pInFile)) {
        RK_U32 index;
        RK_U8 *tmpbuf = buf;
        RK_U8 *pos = NULL;

        nal_len = get_next_nal(pInFile, buf);
        mpp_err("get nal len from file %d", nal_len);
        do {
            mpp_packet_init(&rkpkt, tmpbuf, nal_len);
            memset(cutask, 0, sizeof(HalDecTask));
            memset(&cutask->refer, -1, sizeof(cutask->refer));
            cutask->stmbuf = currentstrem;
            h265d_prepare(mpp_codex_ctx, rkpkt, cutask);
            pos = (RK_U8 *)mpp_packet_get_pos(rkpkt);
            if (pos < (tmpbuf + nal_len)) {
                tmpbuf = pos;
                nal_len = (RK_S32)((tmpbuf + nal_len) - pos);
                mpp_err("nal_len = %d", nal_len);
            } else {
                nal_len = 0;
            }
            if (cutask->valid) {
                if (wait_task) {
                    poll_task(hal, slots, pretask);
                    wait_task = 0;
                }
                cutask->valid = 0;
                h265d_parse(mpp_codex_ctx, cutask);
            }
            if (cutask->valid) {
                HalTask syn;
                syn.dec = *cutask;
                mpp_buf_slot_get_hw_dst(slots, &index);

                if (NULL == mpp_buf_slot_get_buffer(slots, index)) {
                    MppBuffer buffer = NULL;
                    RK_U32 size = mpp_buf_slot_get_size(slots);
                    mpp_err("size = %d", size);
                    mpp_buffer_get(mFrameGroup, &buffer, size);
                    if (buffer)
                        mpp_buf_slot_set_buffer(slots, index, buffer);
                }

                hal_h265d_gen_regs(hal, &syn);

                hal_h265d_start(hal, &syn);
                {
                    MppBuffer tmp = NULL;
                    HalDecTask *task = NULL;

                    tmp = currentstrem;
                    currentstrem = prestrem;
                    prestrem = tmp;

                    task = cutask;
                    cutask = pretask;
                    pretask = task;
                }
                wait_task = 1;
            }

            do {
                ret = mpp_buf_slot_get_display(slots, &frame);
                if (ret == MPP_OK) {
                    mpp_log("get_display for ");
                }
                if (frame) {
#if 1//def DUMP
                    RK_U32 stride_w, stride_h;
                    void *ptr = NULL;
                    MppBuffer framebuf;
                    stride_w = mpp_frame_get_hor_stride(frame);
                    stride_h = mpp_frame_get_ver_stride(frame);
                    framebuf = mpp_frame_get_buffer(frame);
                    ptr = mpp_buffer_get_ptr(framebuf);
                    fwrite(ptr, 1, stride_w * stride_h * 3 / 2, fp);
                    fflush(fp);
#endif
                    mpp_frame_deinit(&frame);
                    frame = NULL;
                }
            } while (ret == MPP_OK);
            mpp_packet_deinit(&rkpkt);
        } while ( nal_len );
    }

    if (wait_task) {
        poll_task(hal, slots, pretask);
        wait_task = 0;
    }
    h265d_flush((void*)mpp_codex_ctx);
    do {
        ret = mpp_buf_slot_get_display(slots, &frame);
        if (ret == MPP_OK) {
            mpp_log("get_display for ");
        }
        if (frame) {
            mpp_frame_deinit(&frame);
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
    if (slots != NULL)
        mpp_buf_slot_deinit(slots);

    if (mFrameGroup != NULL) {
        mpp_buffer_group_put(mFrameGroup);
    }
    if (currentstrem)
        mpp_buffer_put(currentstrem);
    if (prestrem)
        mpp_buffer_put(prestrem);

    if (mStreamGroup != NULL) {
        mpp_buffer_group_put(mStreamGroup);
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
