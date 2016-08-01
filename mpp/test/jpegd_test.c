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

#define MODULE_TAG "JPEGD_TEST"

#include <string.h>

#include "mpp_env.h"
#include "mpp_err.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_common.h"

#include "mpp_dec.h"
#include "mpp_frame.h"

#include "jpegd_api.h"
#include "jpegd_syntax.h"
#include "hal_jpegd_api.h"

#include "utils.h"


#define JPEGD_STREAM_BUFF_SIZE (512*1024)

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

typedef struct parserDemoCmdCtx {
    char    input_file[200];
    char    output_file[200];
    RK_U32  width;
    RK_U32  height;
    RK_U8   have_input;
    RK_U8   have_output;
    RK_U8   disable_debug;
    RK_U32  record_frames;
    RK_S64  record_start_ms;
} parserDemoCmdCtx;

typedef struct jpegdDemoCtx {
    parserDemoCmdCtx   *cfg;
    MppDec         api;
    HalDecTask     task;
    MppBuffer      pkt_buf;
    MppBuffer      pic_buf;
    MppBufferGroup frmbuf_grp;
    MppBufferGroup strmbuf_grp;
    MppPacket      pkt;

    FILE*          pOutFile;

    RK_U8          *strmbuf;
    RK_U32         strmbytes;
    RK_U32         dec_frm_num;
} jpegdDemoCtx;

static OptionInfo jpeg_parserCmd[] = {
    {"i",               "input_file",           "input bitstream file"},
    {"o",               "output_file",          "output bitstream file, "},
    {"w",               "width",                "the width of input bitstream"},
    {"h",               "height",               "the height of input bitstream"},
    {"vframes",         "number",               "set the number of video frames to record"},
    {"ss",              "time_off",             "set the start time offset, use Ms as the unit."},
    {"d",               "disable",              "disable the debug output info."},
};

MPP_RET jpegd_readbytes_from_file(RK_U8* buf, RK_S32 aBytes, FILE* fp)
{
    MPP_RET ret = MPP_OK;
    if ((NULL == buf) || (NULL == fp) || (0 == aBytes)) {
        return -1;
    }

    RK_S32 rd_bytes = fread(buf, 1, aBytes, fp);
    if (rd_bytes != aBytes) {
        mpp_log("read %u bytes from file fail, actually only %#x bytes is read.", (RK_U32)aBytes, rd_bytes);
        return -1;
    }
    mpp_log("read %d bytes from file sucessfully, the first 4 bytes: %08x", rd_bytes, *((RK_U32*)buf));

    return ret;
}

static void jpeg_show_usage()
{
    mpp_log("usage: parserDemo [options] input_file, \n\n");

    mpp_log("Getting help:\n");
    mpp_log("-help  --print options of vpu api demo\n");
}

static void jpeg_show_options(int count, OptionInfo *options)
{
    int i;
    for (i = 0; i < count; i++) {
        mpp_log("-%s  %-16s\t%s\n",
                options[i].name, options[i].argname, options[i].help);
    }
}

static RK_S32 jpeg_show_help()
{
    mpp_log("usage: parserDemo [options] input_file, \n\n");
    jpeg_show_options(sizeof(jpeg_parserCmd) / sizeof(OptionInfo), jpeg_parserCmd);
    return 0;
}

static RK_S32 jpeg_parse_options(int argc, char **argv, parserDemoCmdCtx* cmdCxt)
{
    mpp_log("jpeg_parse_options enter\n");
    const char *opt;
    RK_S32 optindex, handleoptions = 1, ret = 0;

    if ((argc < 2) || (cmdCxt == NULL)) {
        mpp_log("parser demo, input parameter invalid\n");
        jpeg_show_usage();
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
                    jpeg_show_help();
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
        jpeg_show_usage();
        return  MPP_ERR_STREAM;
    }
    mpp_log("jpeg_parse_options exit\n");
    return ret;
}

MPP_RET jpegd_test_deinit(jpegdDemoCtx *ctx)
{
    FUN_TEST("Enter");
    MppDec *pApi = &(ctx->api);
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

    if (ctx->pic_buf) {
        mpp_buffer_put(ctx->pic_buf);
    }
    if (ctx->frmbuf_grp) {
        mpp_err("frmbuf_grp deinit");
        mpp_buffer_group_put(ctx->frmbuf_grp);
    }
    if (ctx->strmbuf_grp) {
        mpp_err("strmbuf_grp deinit");
        mpp_buffer_group_put(ctx->strmbuf_grp);
    }
    if (ctx->strmbuf) {
        mpp_err("strmbuf free");
        mpp_free(ctx->strmbuf);
    }
    if (ctx->pOutFile) {
        mpp_err("close output file");
        fclose(ctx->pOutFile);
        ctx->pOutFile = NULL;
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET jpegd_test_init(parserDemoCmdCtx *cmd, jpegdDemoCtx *ctx)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    MppDec *pMppDec = NULL;
    ParserCfg parser_cfg;
    MppHalCfg hal_cfg;

    memset(ctx, 0, sizeof(jpegdDemoCtx));
    ctx->cfg = cmd;

    //demo configure
    ctx->pOutFile = fopen("/data/spurs.yuv", "wb+");
    if (NULL == ctx->pOutFile) {
        JPEGD_ERROR_LOG("create spurs.yuv failed");
    }

    //malloc buffers for software
    CHECK_MEM(ctx->strmbuf = mpp_malloc_size(RK_U8, JPEGD_STREAM_BUFF_SIZE));

    //malloc buffers for hardware
    if (ctx->frmbuf_grp == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->frmbuf_grp, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("frmbuf_grp: jpegd mpp_buffer_group_get_internal failed\n");
            goto __FAILED;
        }
    }
    if (ctx->strmbuf_grp == NULL) {
        ret = mpp_buffer_group_get_internal(&ctx->strmbuf_grp, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("strmbuf_grp: jpegd mpp_buffer_group_get_internal failed\n");
            goto __FAILED;
        }
    }

    //api config
    pMppDec = &ctx->api;
    pMppDec->coding = MPP_VIDEO_CodingMJPEG;

    CHECK_FUN(mpp_buf_slot_init(&pMppDec->frame_slots));
    CHECK_MEM(pMppDec->frame_slots);
    mpp_buf_slot_setup(pMppDec->frame_slots, 2);

    CHECK_FUN(mpp_buf_slot_init(&pMppDec->packet_slots));
    CHECK_MEM(pMppDec->packet_slots);
    mpp_buf_slot_setup(pMppDec->packet_slots, 2);

    //parser config
    memset(&parser_cfg, 0, sizeof(parser_cfg));
    parser_cfg.coding = pMppDec->coding;
    parser_cfg.frame_slots = pMppDec->frame_slots;
    parser_cfg.packet_slots = pMppDec->packet_slots;
    parser_cfg.task_count = 2;
    parser_cfg.need_split = 0;
    CHECK_FUN(parser_init(&pMppDec->parser, &parser_cfg));

    //hal config
    memset(&hal_cfg, 0, sizeof(hal_cfg));
    hal_cfg.type = MPP_CTX_DEC;
    hal_cfg.coding = pMppDec->coding;
    hal_cfg.work_mode = HAL_MODE_LIBVPU;
    {
        RK_U32 hal_device_id = 0;
        //mpp_env_get_u32("h264d_chg_org", &hal_device_id, 1);
        hal_device_id = 0;
        if (hal_device_id == 1) {
            hal_cfg.device_id = HAL_RKVDEC;
        } else {
            hal_cfg.device_id = HAL_VDPU;
        }
    }
    hal_cfg.frame_slots = pMppDec->frame_slots;
    hal_cfg.packet_slots = pMppDec->packet_slots;
    hal_cfg.task_count = parser_cfg.task_count;
    CHECK_FUN(mpp_hal_init(&pMppDec->hal, &hal_cfg));
    pMppDec->tasks = hal_cfg.tasks;

    memset(&ctx->task, 0, sizeof(ctx->task));
    memset(ctx->task.refer, -1, sizeof(ctx->task.refer));
    ctx->task.input = -1;

    FUN_TEST("Exit");
    return MPP_OK;
__FAILED:
    FUN_TEST("Exit");
    return ret;
}

MPP_RET jpegd_parser_test(parserDemoCmdCtx *cmd)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    MppDec *pMppDec = NULL;
    HalDecTask *curtask = NULL;
    jpegdDemoCtx DemoCtx;
    FILE* pInFile = NULL;
    RK_S32 fileSize = 0;

    // 1.jpegd_test_init
    jpegd_test_init(cmd, &DemoCtx);

    pMppDec = &DemoCtx.api;
    curtask = &DemoCtx.task;

    do {
        RK_S32 slot_idx = 0;

        if (cmd->have_input) {
            mpp_log("input bitstream w: %d, h: %d, path: %s\n",
                    cmd->width, cmd->height, cmd->input_file);

            pInFile = fopen(cmd->input_file, "rb");
            if (pInFile == NULL) {
                mpp_log("input file not exsist\n");
                goto __FAILED;
            }
        } else {
            mpp_log("please set input bitstream file\n");
            goto __FAILED;
        }

        fseek(pInFile, 0L, SEEK_END);
        fileSize = ftell(pInFile);
        fseek(pInFile, 0L, SEEK_SET);

        // 2.jpegd_readbytes_from_file
        ret = jpegd_readbytes_from_file(DemoCtx.strmbuf, fileSize, pInFile);
        if (ret != MPP_OK) {
            mpp_log("read bytes from file failed\n");
            goto __FAILED;
        }
        mpp_packet_init(&DemoCtx.pkt, DemoCtx.strmbuf, fileSize);

        // 3.parser_prepare
        CHECK_FUN(parser_prepare(pMppDec->parser, DemoCtx.pkt, curtask)); // jpegd_parser_prepare

        if (-1 == curtask->input) {
            if (MPP_OK == mpp_buf_slot_get_unused(pMppDec->packet_slots, &slot_idx) ) {
                MppBuffer buffer = NULL;
                curtask->input = slot_idx;

                mpp_buf_slot_get_prop(pMppDec->packet_slots, slot_idx, SLOT_BUFFER, &buffer);
                if (NULL == buffer) {
                    RK_U32 size = (RK_U32)mpp_buf_slot_get_size(pMppDec->packet_slots);
                    if (size == 0) {
                        size = (1024 * 1024);
                    }

                    mpp_buffer_get(DemoCtx.strmbuf_grp, &buffer, size);
                    if (buffer != NULL) {
                        mpp_err("mpp_buf_slot_get_prop, buffer:%p, size:%d", buffer, size);
                        mpp_buf_slot_set_prop(pMppDec->packet_slots, slot_idx, SLOT_BUFFER, buffer);
                    }
                }

                mpp_buffer_write(buffer, 0,
                                 mpp_packet_get_data(curtask->input_packet),
                                 mpp_packet_get_size(curtask->input_packet));

                mpp_err("%s Line %d, (*input_packet):%p, length:%d", __func__, __LINE__,
                        mpp_packet_get_data(curtask->input_packet),
                        mpp_packet_get_size(curtask->input_packet));
                DemoCtx.pkt_buf = buffer;

                mpp_buf_slot_set_flag(pMppDec->packet_slots, curtask->input, SLOT_CODEC_READY);
                mpp_buf_slot_set_flag(pMppDec->packet_slots, curtask->input, SLOT_HAL_INPUT);
            }
        }

        CHECK_FUN(parser_parse(pMppDec->parser, curtask)); // jpegd_parser_parse

        if (curtask->valid) {
            HalTaskInfo task_info;
            MppBuffer buffer = NULL;
            task_info.dec = *curtask;
            RK_S32 index = curtask->output;

            if (mpp_buf_slot_is_changed(pMppDec->frame_slots)) {
                mpp_buf_slot_ready(pMppDec->frame_slots);
            }

            mpp_buf_slot_get_prop(pMppDec->frame_slots, index, SLOT_BUFFER, &buffer);
            if (NULL == buffer) {
                RK_U32 size = (RK_U32)mpp_buf_slot_get_size(pMppDec->frame_slots);
                if (size == 0) {
                    size = 1920 * 1080 * 5;
                }

                mpp_buffer_get(DemoCtx.frmbuf_grp, &buffer, size);
                if (buffer) {
                    mpp_buf_slot_set_prop(pMppDec->frame_slots, index, SLOT_BUFFER, buffer);
                    mpp_err("frame_slots, buffer:%p, size:%d", buffer, size);
                }

                DemoCtx.pic_buf = buffer;
            }

            mpp_hal_reg_gen(pMppDec->hal, &task_info);  // jpegd_hal_gen_regs
            mpp_hal_hw_start(pMppDec->hal, &task_info); // jpegd_hal_start
            mpp_hal_hw_wait(pMppDec->hal, &task_info); // jpegd_hal_wait

            parser_reset(pMppDec->parser); //[TEMP] jpegd_parser_reset

            void* pOutYUV = NULL;
            pOutYUV = mpp_buffer_get_ptr(DemoCtx.pic_buf);
            if (pOutYUV) {
                JPEGD_INFO_LOG("pOutYUV:%p", pOutYUV);
                JpegSyntaxParam *pTmpSyn = (JpegSyntaxParam *)curtask->syntax.data;
                RK_U32 width = pTmpSyn->frame.hwX;
                RK_U32 height = pTmpSyn->frame.hwY;

                JPEGD_INFO_LOG("Output Image: %d*%d", width, height);
                if (DemoCtx.pOutFile) {
                    fwrite(pOutYUV, 1, width * height * 3 / 2, DemoCtx.pOutFile);
                    fflush(DemoCtx.pOutFile);
                }
            }
        }

        /*** parser packet deinit ***/
        if (DemoCtx.pkt_buf) {
            mpp_buffer_put(DemoCtx.pkt_buf);
        }
        if (DemoCtx.pic_buf) {
            mpp_buffer_put(DemoCtx.pic_buf);
        }

        mpp_buf_slot_clr_flag(pMppDec->packet_slots, curtask->input, SLOT_HAL_INPUT);
        mpp_buf_slot_clr_flag(pMppDec->frame_slots, curtask->output, SLOT_HAL_OUTPUT);

        memset(curtask, 0, sizeof(HalDecTask));
        memset(&curtask->refer, -1, sizeof(curtask->refer));
        curtask->input = -1;

        DemoCtx.dec_frm_num ++;
    } while (0);

    CHECK_FUN(mpp_dec_flush(pMppDec));

__FAILED:
    if (pInFile) {
        fclose(pInFile);
        pInFile = NULL;
    }
    jpegd_test_deinit(&DemoCtx);

    FUN_TEST("Exit");
    return MPP_OK;
}

int main(int argc, char **argv)
{
    FUN_TEST("Enter");

    RK_S32 ret = 0;
    parserDemoCmdCtx demoCmdCtx;
    parserDemoCmdCtx* cmd = NULL;

    if (argc == 1) {
        jpeg_show_usage();
        mpp_log("jpegd test demo complete directly\n");
        return MPP_OK;
    }

    mpp_env_get_u32("mpp_debug", &mpp_debug, 0);

    cmd = &demoCmdCtx;
    memset((void*)cmd, 0, sizeof(parserDemoCmdCtx));
    if ((ret = jpeg_parse_options(argc, argv, cmd)) != 0) {
        if (ret == PARSER_DEMO_PARSE_HELP_OK) {
            return MPP_OK;
        }

        mpp_log("parse_options fail\n\n");
        jpeg_show_usage();
        return MPP_OK;
    }
    jpegd_parser_test(cmd);

    FUN_TEST("Exit");
    return MPP_OK;
}
