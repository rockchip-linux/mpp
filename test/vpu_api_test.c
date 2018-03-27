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

#define MODULE_TAG "vpu_api_demo"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mpp_log.h"
#include "mpp_time.h"

#include "vpu_api.h"
#include "utils.h"

#define FOR_TEST_ENCODE 1

static RK_U32 VPU_API_DEMO_DEBUG_DISABLE = 0;

#define BSWAP32(x) \
    ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
     (((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))

#define DEMO_ERR_RET(err) do { ret = err; goto DEMO_OUT; } while (0)
#define DECODE_ERR_RET(err) do { ret = err; goto DECODE_OUT; } while (0)
#define ENCODE_ERR_RET(err) do { ret = err; goto ENCODE_OUT; } while (0)


typedef enum VPU_API_DEMO_RET {
    VPU_DEMO_OK             = 0,
    VPU_DEMO_PARSE_HELP_OK  = 1,

    VPU_DEMO_ERROR_BASE     = -100,
    ERROR_INVALID_PARAM     = VPU_DEMO_ERROR_BASE - 1,
    ERROR_INVALID_STREAM    = VPU_DEMO_ERROR_BASE - 2,
    ERROR_IO                = VPU_DEMO_ERROR_BASE - 3,
    ERROR_MEMORY            = VPU_DEMO_ERROR_BASE - 4,
    ERROR_INIT_VPU          = VPU_DEMO_ERROR_BASE - 5,

    ERROR_VPU_DECODE        = VPU_DEMO_ERROR_BASE - 90,
} VPU_API_DEMO_RET;

typedef struct VpuApiDemoCmdContext {
    RK_U32  width;
    RK_U32  height;
    CODEC_TYPE  codec_type;
    OMX_RK_VIDEO_CODINGTYPE coding;
    char    input_file[200];
    char    output_file[200];
    RK_U8   have_input;
    RK_U8   have_output;
    RK_U8   disable_debug;
    RK_U32  record_frames;
    RK_S64  record_start_ms;
} VpuApiDemoCmdContext_t;

typedef struct VpuApiEncInput {
    EncInputStream_t stream;
    RK_U32 capability;
} VpuApiEncInput;

static OptionInfo vpuApiCmd[] = {
    { "i",       "input_file",  "input bitstream file" },
    { "o",       "output_file", "output bitstream file, " },
    { "w",       "width",       "the width of input bitstream" },
    { "h",       "height",      "the height of input bitstream" },
    { "t",       "codec_type",  "the codec type, dec: deoder, enc: encoder, default: decoder" },
    { "coding",  "coding_type", "encoding type of the bitstream" },
    { "vframes", "number",      "set the number of video frames to record" },
    { "ss",      "time_off",    "set the start time offset, use Ms as the unit." },
    { "d",       "disable",     "disable the debug output info." },
};

static void show_usage()
{
    mpp_log("usage: vpu_apiDemo [options] input_file, \n\n");

    mpp_log("Getting help:\n");
    mpp_log("-help  --print options of vpu api demo\n");
}

static RK_S32 show_help()
{
    mpp_log("usage: vpu_apiDemo [options] input_file, \n\n");
    show_options(vpuApiCmd);
    return 0;
}

static RK_S32 parse_options(int argc, char **argv, VpuApiDemoCmdContext_t *cmdCxt)
{
    char *opt;
    RK_S32 optindex, handleoptions = 1, ret = 0;

    if ((argc < 2) || (cmdCxt == NULL)) {
        mpp_log("vpu api demo, input parameter invalid\n");
        show_usage();
        return ERROR_INVALID_PARAM;
    }

    /* parse options */
    optindex = 1;
    while (optindex < argc) {
        opt = argv[optindex++];

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
                    ret = VPU_DEMO_PARSE_HELP_OK;
                    goto PARSE_OPINIONS_OUT;
                } else if (argv[optindex]) {
                    cmdCxt->height = atoi(argv[optindex]);
                } else {
                    mpp_log("input height is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }
                break;
            case 't':
                if (argv[optindex]) {
                    cmdCxt->codec_type = atoi(argv[optindex]);
                    break;
                } else {
                    mpp_log("input codec_type is invalid\n");
                    ret = -1;
                    goto PARSE_OPINIONS_OUT;
                }

            default:
                if ((*(opt + 1) != '\0') && argv[optindex]) {
                    if (!strncmp(opt, "coding", 6)) {
                        mpp_log("coding, argv[optindex]: %s",
                                argv[optindex]);
                        cmdCxt->coding = atoi(argv[optindex]);
                    } else if (!strncmp(opt, "vframes", 7)) {
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
        return ERROR_INVALID_PARAM;
    }
    return ret;
}

static RK_S32 readBytesFromFile(RK_U8 *buf, RK_S32 aBytes, FILE *file)
{
    RK_S32 ret = 0;

    if ((NULL == buf) || (NULL == file) || (0 == aBytes)) {
        return -1;
    }

    ret = (RK_S32)fread(buf, 1, aBytes, file);
    if (ret != aBytes) {
        mpp_log("read %d bytes from file fail\n", aBytes);
        return -1;
    }

    return 0;
}

static RK_S32 vpu_encode_demo(VpuApiDemoCmdContext_t *cmd)
{
    FILE *pInFile = NULL;
    FILE *pOutFile = NULL;
    struct VpuCodecContext *ctx = NULL;
    RK_S32 nal = 0x00000001;
    RK_S32 fileSize, ret, size;
    RK_U32 readOneFrameSize = 0;
    EncoderOut_t    enc_out_yuv;
    EncoderOut_t *enc_out = NULL;
    VpuApiEncInput enc_in_strm;
    VpuApiEncInput *api_enc_in = &enc_in_strm;
    EncInputStream_t *enc_in = NULL;
    EncParameter_t *enc_param = NULL;
    RK_S64 fakeTimeUs = 0;
    RK_U32 w_align = 0;
    RK_U32 h_align = 0;

    int Format = ENC_INPUT_YUV420_PLANAR;

    if (cmd == NULL) {
        return -1;
    }

    if ((cmd->have_input == 0) || (cmd->width <= 0) || (cmd->height <= 0)
        || (cmd->coding <= OMX_RK_VIDEO_CodingAutoDetect)) {
        mpp_log("Warning: missing needed parameters for vpu api demo\n");
    }

    if (cmd->have_input) {
        mpp_log("input bitstream w: %d, h: %d, coding: %d(%s), path: %s\n",
                cmd->width, cmd->height, cmd->coding,
                cmd->codec_type == CODEC_DECODER ? "decode" : "encode",
                cmd->input_file);

        pInFile = fopen(cmd->input_file, "rb");
        if (pInFile == NULL) {
            mpp_log("input file not exsist\n");
            ENCODE_ERR_RET(ERROR_INVALID_PARAM);
        }
    } else {
        mpp_log("please set input bitstream file\n");
        ENCODE_ERR_RET(ERROR_INVALID_PARAM);
    }

    if (cmd->have_output) {
        mpp_log("vpu api demo output file: %s\n",
                cmd->output_file);
        pOutFile = fopen(cmd->output_file, "wb");
        if (pOutFile == NULL) {
            mpp_log("can not write output file\n");
            ENCODE_ERR_RET(ERROR_INVALID_PARAM);
        }
    }

#ifdef FOR_TEST_ENCODE
    ctx = (struct VpuCodecContext *)malloc(sizeof(struct VpuCodecContext));
    if (!ctx) {
        mpp_err("Input context has not been properly allocated");
        return -1;
    }
    memset(ctx, 0, sizeof(struct VpuCodecContext));

    ctx->videoCoding = OMX_RK_VIDEO_CodingAVC;
    ctx->codecType = CODEC_ENCODER;
    ctx->width  = cmd->width;
    ctx->height = cmd->height;
#endif

    fseek(pInFile, 0L, SEEK_END);
    fileSize = ftell(pInFile);
    fseek(pInFile, 0L, SEEK_SET);

    memset(&enc_in_strm, 0, sizeof(VpuApiEncInput));
    enc_in = &enc_in_strm.stream;
    enc_in->buf = NULL;

    memset(&enc_out_yuv, 0, sizeof(EncoderOut_t));
    enc_out = &enc_out_yuv;
    enc_out->data = (RK_U8 *)malloc(cmd->width * cmd->height);
    if (enc_out->data == NULL) {
        ENCODE_ERR_RET(ERROR_MEMORY);
    }

    ret = vpu_open_context(&ctx);
    if (ret || (ctx == NULL)) {
        ENCODE_ERR_RET(ERROR_MEMORY);
    }

    /*
     ** now init vpu api context. codecType, codingType, width ,height
     ** are all needed before init.
    */
    ctx->codecType = cmd->codec_type;
    ctx->videoCoding = cmd->coding;
    ctx->width = cmd->width;
    ctx->height = cmd->height;
    ctx->no_thread = 1;

    ctx->private_data = malloc(sizeof(EncParameter_t));
    memset(ctx->private_data, 0, sizeof(EncParameter_t));

    enc_param = (EncParameter_t *)ctx->private_data;
    enc_param->width        = cmd->width;
    enc_param->height       = cmd->height;
    enc_param->format       = ENC_INPUT_YUV420_PLANAR;
    enc_param->rc_mode      = 0;
    enc_param->bitRate      = 4000000;
    enc_param->framerate    = 25;
    enc_param->enableCabac  = 1;
    enc_param->cabacInitIdc = 0;
    enc_param->intraPicRate = 30;
    enc_param->profileIdc = 66;
    enc_param->levelIdc = 40;

    if ((ret = ctx->init(ctx, NULL, 0)) != 0) {
        mpp_log("init vpu api context fail, ret: 0x%X\n", ret);
        ENCODE_ERR_RET(ERROR_INIT_VPU);
    }

    /*
     ** init of VpuCodecContext while running encode, it returns
     ** sps and pps of encoder output, you need to save sps and pps
     ** after init.
    */
    mpp_log("encode init ok, sps len: %d\n", ctx->extradata_size);
    if (pOutFile && (ctx->extradata_size > 0)) {
        mpp_log("dump %d bytes enc output stream to file\n",
                ctx->extradata_size);

        /* save sps and pps */
        fwrite(ctx->extradata, 1, ctx->extradata_size, pOutFile);
        fflush(pOutFile);
    }

    ret = ctx->control(ctx, VPU_API_ENC_SETFORMAT, &Format);
    if (ret)
        mpp_err("VPU_API_ENC_SETFORMAT ret %d\n", ret);

    ret = ctx->control(ctx, VPU_API_ENC_GETCFG, enc_param);
    if (ret)
        mpp_log("VPU_API_ENC_GETCFG ret %d\n", ret);

    enc_param->rc_mode = 1;

    ret = ctx->control(ctx, VPU_API_ENC_SETCFG, enc_param);
    if (ret)
        mpp_log("VPU_API_ENC_SETCFG ret %d\n", ret);

    /*
     ** vpu api encode process.
    */
    mpp_log("init vpu api context ok, input yuv stream file size: %d\n", fileSize);
    w_align = ((ctx->width + 15) & (~15));
    h_align = ((ctx->height + 15) & (~15));
    size = w_align * h_align * 3 / 2;
    readOneFrameSize = ctx->width * ctx->height * 3 / 2;
    mpp_log("%d %d %d %d %d", ctx->width, ctx->height, w_align, h_align, size);
    nal = BSWAP32(nal);

    do {
        if (ftell(pInFile) >= fileSize) {
            mpp_log("read end of file, complete\n");
            break;
        }

        if (enc_in && (enc_in->size == 0)) {
            if (enc_in->buf == NULL) {
                enc_in->buf = (RK_U8 *)(malloc)(size);
                if (enc_in->buf == NULL) {
                    ENCODE_ERR_RET(ERROR_MEMORY);
                }
                api_enc_in->capability = size;
            }

            if (api_enc_in->capability < ((RK_U32)size)) {
                enc_in->buf = (RK_U8 *)(realloc)((void *)(enc_in->buf), size);
                if (enc_in->buf == NULL) {
                    ENCODE_ERR_RET(ERROR_MEMORY);
                }
                api_enc_in->capability = size;
            }

            if (readBytesFromFile(enc_in->buf, readOneFrameSize, pInFile)) {
                break;
            } else {
                enc_in->size = size;
                enc_in->timeUs = fakeTimeUs;
                fakeTimeUs += 40000;
            }

            mpp_log("read one frame, size: %d, timeUs: %lld, filePos: %ld\n",
                    enc_in->size, enc_in->timeUs, ftell(pInFile));
        }

        if ((ret = ctx->encode(ctx, enc_in, enc_out)) < 0) {
            ENCODE_ERR_RET(ERROR_VPU_DECODE);
        } else {
            enc_in->size = 0;  // TODO encode completely, and set enc_in->size to 0
            mpp_log("vpu encode one frame, out len: %d, left size: %d\n",
                    enc_out->size, enc_in->size);

            /*
             ** encoder output stream is raw bitstream, you need to add nal
             ** head by yourself.
            */
            if ((enc_out->size) && (enc_out->data)) {
                if (pOutFile) {
                    mpp_log("dump %d bytes enc output stream to file\n",
                            enc_out->size);
                    //fwrite((RK_U8*)&nal, 1, 4, pOutFile);  // because output stream have start code, so here mask this code
                    fwrite(enc_out->data, 1, enc_out->size, pOutFile);
                    fflush(pOutFile);
                }

                enc_out->size = 0;
            }
        }

        msleep(3);
    } while (1);

ENCODE_OUT:
    if (enc_in && enc_in->buf) {
        free(enc_in->buf);
        enc_in->buf = NULL;
    }
    if (enc_out && (enc_out->data)) {
        free(enc_out->data);
        enc_out->data = NULL;
    }
    if (ctx) {
        if (ctx->private_data) {
            free(ctx->private_data);
            ctx->private_data = NULL;
        }
        vpu_close_context(&ctx);
        ctx = NULL;
    }
    if (pInFile) {
        fclose(pInFile);
        pInFile = NULL;
    }
    if (pOutFile) {
        fclose(pOutFile);
        pOutFile = NULL;
    }

    if (ret) {
        mpp_log("encode demo fail, err: %d\n", ret);
    } else {
        mpp_log("encode demo complete OK.\n");
    }
    return ret;

}

static RK_S32 vpu_decode_demo(VpuApiDemoCmdContext_t *cmd)
{
    FILE *pInFile = NULL;
    FILE *pOutFile = NULL;
    struct VpuCodecContext *ctx = NULL;
    RK_S32 fileSize = 0, pkt_size = 0;
    RK_S32 ret = 0;
    RK_U32 frame_count = 0;
    DecoderOut_t    decOut;
    VideoPacket_t demoPkt;
    VideoPacket_t *pkt = NULL;
    DecoderOut_t *pOut = NULL;
    VPU_FRAME *frame = NULL;
    RK_S64 fakeTimeUs = 0;
    RK_U8 *pExtra = NULL;
    RK_U32 extraSize = 0;
    RK_U32 wAlign16  = 0;
    RK_U32 hAlign16  = 0;
    RK_U32 frameSize = 0;

    if (cmd == NULL) {
        return -1;
    }

    if ((cmd->have_input == 0) || (cmd->width <= 0) || (cmd->height <= 0)
        || (cmd->coding <= OMX_RK_VIDEO_CodingAutoDetect)) {
        mpp_log("Warning: missing needed parameters for vpu api demo\n");
    }

    if (cmd->have_input) {
        mpp_log("input bitstream w: %d, h: %d, coding: %d(%s), path: %s\n",
                cmd->width, cmd->height, cmd->coding,
                cmd->codec_type == CODEC_DECODER ? "decode" : "encode",
                cmd->input_file);

        pInFile = fopen(cmd->input_file, "rb");
        if (pInFile == NULL) {
            mpp_log("input file not exsist\n");
            DECODE_ERR_RET(ERROR_INVALID_PARAM);
        }
    } else {
        mpp_log("please set input bitstream file\n");
        DECODE_ERR_RET(ERROR_INVALID_PARAM);
    }

    if (cmd->have_output) {
        mpp_log("vpu api demo output file: %s\n",
                cmd->output_file);
        pOutFile = fopen(cmd->output_file, "wb");
        if (pOutFile == NULL) {
            mpp_log("can not write output file\n");
            DECODE_ERR_RET(ERROR_INVALID_PARAM);
        }
        if (cmd->record_frames == 0)
            cmd->record_frames = 5;
    }

    fseek(pInFile, 0L, SEEK_END);
    fileSize = ftell(pInFile);
    fseek(pInFile, 0L, SEEK_SET);

    memset(&demoPkt, 0, sizeof(VideoPacket_t));
    pkt = &demoPkt;
    pkt->data = NULL;
    pkt->pts = VPU_API_NOPTS_VALUE;
    pkt->dts = VPU_API_NOPTS_VALUE;

    memset(&decOut, 0, sizeof(DecoderOut_t));
    pOut = &decOut;

    ret = vpu_open_context(&ctx);
    if (ret || (ctx == NULL)) {
        DECODE_ERR_RET(ERROR_MEMORY);
    }

    /*
     ** read codec extra data from input stream file.
    */
    if (readBytesFromFile((RK_U8 *)(&extraSize), 4, pInFile)) {
        DECODE_ERR_RET(ERROR_IO);
    }

    mpp_log("codec extra data size: %d\n", extraSize);

    pExtra = (RK_U8 *)(malloc)(extraSize);
    if (pExtra == NULL) {
        DECODE_ERR_RET(ERROR_MEMORY);
    }
    memset(pExtra, 0, extraSize);

    if (readBytesFromFile(pExtra, extraSize, pInFile)) {
        DECODE_ERR_RET(ERROR_IO);
    }

    /*
     ** now init vpu api context. codecType, codingType, width ,height
     ** are all needed before init.
    */
    ctx->codecType = cmd->codec_type;
    ctx->videoCoding = cmd->coding;
    ctx->width = cmd->width;
    ctx->height = cmd->height;
    ctx->no_thread = 1;

    if ((ret = ctx->init(ctx, pExtra, extraSize)) != 0) {
        mpp_log("init vpu api context fail, ret: 0x%X\n", ret);
        DECODE_ERR_RET(ERROR_INIT_VPU);
    }

    /*
     ** vpu api decoder process.
    */
    mpp_log("init vpu api context ok, fileSize: %d\n", fileSize);

    do {
        if (ftell(pInFile) >= fileSize) {
            mpp_log("read end of file, complete\n");
            break;
        }

        if (pkt && (pkt->size == 0)) {
            if (readBytesFromFile((RK_U8 *)(&pkt_size), 4, pInFile)) {
                break;
            }

            if (pkt->data == NULL) {
                pkt->data = (RK_U8 *)(malloc)(pkt_size);
                if (pkt->data == NULL) {
                    DECODE_ERR_RET(ERROR_MEMORY);
                }
                pkt->capability = pkt_size;
            }

            if (pkt->capability < ((RK_U32)pkt_size)) {
                pkt->data = (RK_U8 *)(realloc)((void *)(pkt->data), pkt_size);
                if (pkt->data == NULL) {
                    DECODE_ERR_RET(ERROR_MEMORY);
                }
                pkt->capability = pkt_size;
            }

            if (readBytesFromFile(pkt->data, pkt_size, pInFile)) {
                break;
            } else {
                pkt->size = pkt_size;
                pkt->pts = fakeTimeUs;
                fakeTimeUs += 40000;
            }

            mpp_log("read one packet, size: %d, pts: %lld, filePos: %ld\n",
                    pkt->size, pkt->pts, ftell(pInFile));
        }

        /* note: must set out put size to 0 before do decoder. */
        pOut->size = 0;

        if (ctx->decode_sendstream(ctx, pkt) != 0) {
            mpp_log("send packet failed");
            DECODE_ERR_RET(ERROR_VPU_DECODE);
        }


        if ((ret = ctx->decode_getframe(ctx, pOut)) != 0) {
            mpp_log("get decoded data failed\n");
            DECODE_ERR_RET(ERROR_VPU_DECODE);
        } else {
            mpp_log("vpu decode one frame, out len: %d, left size: %d\n",
                    pOut->size, pkt->size);

            /*
             ** both virtual and physical address of the decoded frame are contained
             ** in structure named VPU_FRAME, if you want to use virtual address, make
             ** sure you have done VPUMemLink before.
            */
            if ((pOut->size) && (pOut->data)) {
                frame = (VPU_FRAME *)(pOut->data);
                VPUMemLink(&frame->vpumem);
                wAlign16 = ((frame->DisplayWidth + 15) & (~15));
                hAlign16 = ((frame->DisplayHeight + 15) & (~15));
                frameSize = wAlign16 * hAlign16 * 3 / 2;

                if (pOutFile && (frame_count++ < cmd->record_frames)) {
                    mpp_log("write %d frame(yuv420sp) data, %d bytes to file\n",
                            frame_count, frameSize);

                    fwrite((RK_U8 *)(frame->vpumem.vir_addr), 1, frameSize, pOutFile);
                    fflush(pOutFile);
                }

                /*
                 ** remember use VPUFreeLinear to free, other wise memory leak will
                 ** give you a surprise.
                */
                VPUFreeLinear(&frame->vpumem);
                // NOTE: pOut->data is malloc from vpu_api we need to free it.
                free(pOut->data);
                pOut->data = NULL;
                pOut->size = 0;
            }
        }

        msleep(3);
    } while (!(ctx->decoder_err));

DECODE_OUT:
    if (pkt && pkt->data) {
        free(pkt->data);
        pkt->data = NULL;
    }
    if (pOut && (pOut->data)) {
        free(pOut->data);
        pOut->data = NULL;
    }
    if (pExtra) {
        free(pExtra);
        pExtra = NULL;
    }
    if (ctx) {
        vpu_close_context(&ctx);
        ctx = NULL;
    }
    if (pInFile) {
        fclose(pInFile);
        pInFile = NULL;
    }
    if (pOutFile) {
        fclose(pOutFile);
        pOutFile = NULL;
    }

    if (ret) {
        mpp_log("decode demo fail, err: %d\n", ret);
    } else {
        mpp_log("encode demo complete OK.\n");
    }
    return ret;
}


int main(int argc, char **argv)
{
    RK_S32 ret = 0;
    VpuApiDemoCmdContext_t demoCmdCtx;
    VpuApiDemoCmdContext_t *cmd = NULL;
    VPU_API_DEMO_DEBUG_DISABLE = 0;

    mpp_log("/*******  vpu api demo in *******/\n");
    if (argc == 1) {
        show_usage();
        mpp_log("vpu api demo complete directly\n");
        return 0;
    }

    cmd = &demoCmdCtx;
    memset(cmd, 0, sizeof(VpuApiDemoCmdContext_t));
    cmd->codec_type = CODEC_DECODER;
    if ((ret = parse_options(argc, argv, cmd)) != 0) {
        if (ret == VPU_DEMO_PARSE_HELP_OK) {
            return 0;
        }

        mpp_log("parse_options fail\n\n");
        show_usage();
        DEMO_ERR_RET(ERROR_INVALID_PARAM);
    }

    if (cmd->disable_debug) {
        VPU_API_DEMO_DEBUG_DISABLE = 1;
    }

    switch (cmd->codec_type) {
    case CODEC_DECODER:
        ret = vpu_decode_demo(cmd);
        break;
    case CODEC_ENCODER:
        ret = vpu_encode_demo(cmd);
        break;

    default:
        ret = ERROR_INVALID_PARAM;
        break;
    }

DEMO_OUT:
    if (ret) {
        mpp_log("vpu api demo fail, err: %d\n", ret);
    } else {
        mpp_log("vpu api demo complete OK.\n");
    }
    return ret;
}
