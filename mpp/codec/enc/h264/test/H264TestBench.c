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

/*------------------------------------------------------------------------------
    1. Include headers
------------------------------------------------------------------------------*/
// add by lance 2016.05.06
#include "mpp_log.h"
#include "mpp_buffer.h"
#include "mpp_mem.h"
#include "hal_h264e_api.h"
#include "h264e_syntax.h"
#include "h264e_api.h"

/* For SW/HW shared memory allocation */
#include "ewl.h"

/* For command line structure */
#include "H264TestBench.h"

/* For parameter parsing */
#include "EncGetOption.h"

/* For accessing the EWL instance inside the encoder */
#include "H264Instance.h"

/* For compiler flags, test data, debug and tracing */
#include "enccommon.h"

/* For Rockchip H.264 encoder */
#include "h264encapi.h"

#ifdef INTERNAL_TEST
#include "h264encapi_ext.h"
#endif

/* For printing and file IO */
#include <stdio.h>

/* For dynamic memory allocation */
#include <stdlib.h>

/* For memset, strcpy and strlen */
#include <string.h>

#ifdef USE_EFENCE
#include "efence.h"
#endif

//#include "vpu_mem.h"  // mask by lance 2016.05.06
#include <pthread.h>

/*------------------------------------------------------------------------------
    2. External compiler flags
--------------------------------------------------------------------------------

NO_OUTPUT_WRITE: Output stream is not written to file. This should be used
                 when running performance simulations.
NO_INPUT_READ:   Input frames are not read from file. This should be used
                 when running performance simulations.
PSNR:            Enable PSNR calculation with --psnr option, only works with
                 system model

--------------------------------------------------------------------------------
    3. Module defines
------------------------------------------------------------------------------*/

#define H264ERR_OUTPUT  //stdout
#define MAX_GOP_LEN 150
/* The amount of entries in the NAL unit size table, max one NALU / MB row */
#define NALU_TABLE_SIZE     ((1920/16) + 3)

// add by lance 2016.05.11 for compare
//#define WRITE_CMD_INFO_FOR_COMPARE 1
#ifdef WRITE_CMD_INFO_FOR_COMPARE
FILE *cmdCompareFile = NULL;
int cmdFlagTest = 0;
#endif

#ifdef PSNR
float log10f(float x);
float roundf(float x);
#endif

/* Global variables */

i32 testParam;

static char input[] = "/data/test/Bus_352x288_25.yuv";
static char output[] = "/data/test/stream.h264";
static char nal_sizes_file[] = "nal_sizes.txt";

static char *streamType[2] = { "BYTE_STREAM", "NAL_UNITS" };

// mask by lance 2016.05.12
#if 0
static option_s option[] = {
    {"help", 'H', 1},
    {"firstPic", 'a', 1},
    {"lastPic", 'b', 1},
    {"width", 'x', 1},
    {"height", 'y', 1},
    {"lumWidthSrc", 'w', 1},
    {"lumHeightSrc", 'h', 1},
    {"horOffsetSrc", 'X', 1},
    {"verOffsetSrc", 'Y', 1},
    {"outputRateNumer", 'f', 1},
    {"outputRateDenom", 'F', 1},
    {"inputRateNumer", 'j', 1},
    {"inputRateDenom", 'J', 1},
    {"inputFormat", 'l', 1},    /* Input image format */
    {"colorConversion", 'O', 1},    /* RGB to YCbCr conversion type */
    {"input", 'i', 1},          /* "input" must be after "inputFormat" */
    {"output", 'o', 1},
    {"videoRange", 'k', 1},
    {"rotation", 'r', 1},   /* Input image rotation */
    {"intraPicRate", 'I', 1},
    {"constIntraPred", 'T', 1},
    {"disableDeblocking", 'D', 1},
    {"filterOffsetA", 'W', 1},
    {"filterOffsetB", 'E', 1},

    {"trans8x8", '8', 1}, /* adaptive 4x4, 8x8 transform */
    {"enableCabac", 'K', 1},
    {"cabacInitIdc", 'p', 1},
    {"mbPerSlice", 'V', 1},

    {"bitPerSecond", 'B', 1},
    {"picRc", 'U', 1},
    {"mbRc", 'u', 1},
    {"picSkip", 's', 1},    /* Frame skiping */
    {"gopLength", 'g', 1}, /* group of pictures length */
    {"qpMin", 'n', 1},  /* Minimum frame header qp */
    {"qpMax", 'm', 1},  /* Maximum frame header qp */
    {"qpHdr", 'q', 1},  /* Defaul qp */
    {"chromaQpOffset", 'Q', 1}, /* Chroma qp index offset */
    {"hrdConformance", 'C', 1}, /* HDR Conformance (ANNEX C) */
    {"cpbSize", 'c', 1},    /* Coded Picture Buffer Size */
    {"intraQpDelta", 'A', 1},    /* QP adjustment for intra frames */
    {"fixedIntraQp", 'G', 1},    /* Fixed QP for all intra frames */

    {"userData", 'z', 1},  /* SEI User data file */
    {"level", 'L', 1},  /* Level * 10  (ANNEX A) */
    {"byteStream", 'R', 1},     /* Byte stream format (ANNEX B) */
    {"sei", 'S', 1},    /* SEI messages */
    {"videoStab", 'Z', 1},  /* video stabilization */
    {"bpsAdjust", '1', 1},  /* Setting bitrate on the fly */
    {"mbQpAdjustment", '2', 1},  /* MAD based MB QP adjustment */

    {"testId", 'e', 1},
    {"burstSize", 'N', 1},
    {"burstType", 't', 1},
    {"quarterPixelMv", 'M', 1},
    {"trigger", 'P', 1},
    {"psnr", 'd', 0},
    {"testParam", 'v', 1},
    {0, 0, 0}
};
#endif

/* SW/HW shared memories for input/output buffers */
// mask by lance 2016.05.06
/*static VPUMemLinear_t pictureMem;
static VPUMemLinear_t pictureStabMem;
static VPUMemLinear_t outbufMem;*/
static MppBufferGroup memGroup;
static MppBuffer pictureMem;
static MppBuffer pictureStabMem;
static MppBuffer outbufMem;

static FILE *yuvFile = NULL;
static u32 file_size;

i32 trigger_point = -1;      /* Logic Analyzer trigger point */

/*------------------------------------------------------------------------------
    4. Local function prototypes
------------------------------------------------------------------------------*/
static int AllocRes(commandLine_s * cmdl/*, H264EncInst enc*/);  // mask by lance 2016.05.12
static void FreeRes(/*H264EncInst enc*/);  // mask by lance 2016.05.12
static int OpenEncoder(commandLine_s * cml, h264Instance_s* pEnc);
//static i32 Encode(H264EncInst inst, commandLine_s * cml);  // mask by lance 2016.05.12
static void CloseEncoder(H264EncInst encoderForClose);
static i32 NextPic(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
                   i32 outputRateDenom, i32 frameCnt, i32 firstPic);
static int ReadPic(u8 * image, i32 size, i32 nro, char *name, i32 width,
                   i32 height, i32 format);
static u8* ReadUserData(H264EncInst encoder, char *name);
static int Parameter(commandLine_s * ep);
//static void Help(void);  // mask by lance 2016.05.12
static void WriteStrm(FILE * fout, u32 * outbuf, u32 size, u32 endian);
//static int ChangeInput(i32 argc, char **argv, char **name, option_s * option);  // mask by lance 2016.05.12
static void PrintNalSizes(const u32 *pNaluSizeBuf/*, const u8 *pOutBuf,
                                                   u32 strmSize, i32 byteStream*/);  // mask by lance 2016.05.12

static void WriteNalSizesToFile(const char *file, const u32 * pNaluSizeBuf,
                                u32 buffSize);
static void PrintErrorValue(const char *errorDesc, u32 retVal);
static u32 PrintPSNR(/*u8 *a, u8 *b, i32 scanline, i32 wdh, i32 hgt*/);  // mask by lance 2016.05.12
static int H264encodeInit(void);  // add by lance 2016.05.12
static int H264encodeOneFrame(void *ctx, HalTaskInfo *task_info, H264EncInst encoder, commandLine_s * cml, h264e_syntax *syntax_data);  // add by lance 2016.05.12
static int H264encodeExit(H264EncInst encoder, commandLine_s * cml);  // add by lance 2016.05.12

h264Instance_s h264encInst;
H264EncInst encoder;
commandLine_s cmdl;
//H264EncIn encIn;  // add by lance 2016.06.01
//H264EncOut encOut;  // add by lance 2016.06.01
H264EncRateCtrl rc;
int intraPeriodCnt = 0, codedFrameCnt = 0, next = 0, src_img_size;
u32 frameCnt = 0;
u32 streamSize = 0;
u32 bitrate = 0;
u32 psnrSum = 0;
u32 psnrCnt = 0;
u32 psnr = 0;
i32 i;
u8 *pUserData;
FILE *fout = NULL;

// ----------------
// add for hal part modify by lance 2016.05.07
static void h264e_hal_test_init(void **ctx, HalTaskInfo *task_info, h264e_syntax *syn)
{
    *ctx = mpp_calloc_size(void, hal_api_h264e.ctx_size);  // add by lance 2016.05.07
    memset((*ctx), 0, /*sizeof(h264e_hal_context)*/hal_api_h264e.ctx_size);  // modify by lance 2016.05.07
    memset(task_info, 0, sizeof(HalTaskInfo));
    task_info->enc.syntax.data = (void *)syn;
}

static void h264e_hal_test_deinit(void **ctx, HalTaskInfo *task_info)
{
    mpp_free(*ctx);  // add by lance 2016.05.07
    (void)task_info;
}
// ------------

/*------------------------------------------------------------------------------

    main

------------------------------------------------------------------------------*/
int main(void)
{
    i32 ret;
    HalTaskInfo task_info;
    h264e_syntax syntax_data;
    void *ctx = NULL;
    MppHalCfg hal_cfg;
    // add by lance 2016.05.11
    // ----------------
#ifdef WRITE_CMD_INFO_FOR_COMPARE
    cmdFlagTest = 0;
    if (cmdCompareFile == NULL) {
        if ((cmdCompareFile = fopen("/data/test/cmd_compare.file", "w")) == NULL)
            mpp_err("cmd original File open failed!");
    }
#endif
    // ----------------

    //enc initial
    if (H264encodeInit() != 0)
        mpp_log("H264encodeInit fail!!");

    h264e_hal_test_init(&ctx, &task_info, &syntax_data);  // add for hal part by lance 2016.05.07
    hal_cfg.hal_int_cb.callBack = h264e_callback;
    hal_cfg.hal_int_cb.opaque = (void*)&h264encInst; //control context
    hal_cfg.device_id = HAL_VEPU;  // use HAL_VEPU  modify by lance 2016.05.12
    hal_h264e_init(ctx, &hal_cfg);

    //enc main loop
    do {
        ret = H264encodeOneFrame(ctx, &task_info, encoder, &cmdl, &syntax_data);
//        getchar();
    } while (ret == H264ENC_FRAME_READY);  // only encode one frame  modify by lance 2016.05.06

    if (ret == -10)
        mpp_log("file has read end!");
    if (ret == -11)
        mpp_log("file read error!");

    //enc end and exit, release some resource!
    H264encodeExit(encoder, &cmdl);

    FreeRes(/*encoder*/);  // mask by lance 2016.05.12
    h264e_hal_test_deinit(&ctx, &task_info);  // add for hal part by lance 2016.05.07
    CloseEncoder(encoder);

    return ret;
}

int H264encodeInit(void)
{
    commandLine_s *cml = &cmdl;
    H264EncRet ret;
    MppBufferInfo outInfo;  // add by lance 2016.05.06
    encoder = NULL;  // add by lance 2016.05.06
    memset(&h264encInst, 0, sizeof(h264encInst));  // init 0 in h264encInst  modify by lance 2016.05.06
    encoder = (H264EncInst)&h264encInst;
    H264EncIn *encIn = &(h264encInst.encIn);
    H264EncOut *encOut = &(h264encInst.encOut);

    // mask by lance 2016.05.06
    /*{
        pthread_t tid;
        ALOGV("vpu_service starting\n");
        pthread_create(&tid, NULL, (void *)vpu_service, NULL);
    }*/

    /* Parse command line parameters */
    if (Parameter(&cmdl) != 0) {
        mpp_log( "Input parameter error\n");
        return -1;
    }

    /* Encoder initialization */
    if (OpenEncoder(&cmdl, &h264encInst) != 0) {
        mpp_log( "Open Encoder failure\n");
        return -1;
    }

    /* Set the test ID for internal testing,
     * the SW must be compiled with testing flags */
    H264EncSetTestId(encoder, cmdl.testId);

    /* Allocate input and output buffers */
    if (AllocRes(&cmdl/*, encoder*/) != 0) {  // mask by lance 2016.05.12
        mpp_err( "Failed to allocate the external resources!\n");
        FreeRes(/*encoder*/);  // mask by lance 2016.05.12
        CloseEncoder(encoder);
        return -1;
    }

    //h264encode init
    encIn->pNaluSizeBuf = NULL;
    encIn->naluSizeBufSize = 0;

    // mask and add by lance 2016.05.06
    //encIn.pOutBuf = outbufMem.vir_addr;
    encIn->pOutBuf = (u32*)mpp_buffer_get_ptr(outbufMem);
    // mask and add by lance 2016.05.06
    //encIn.busOutBuf = outbufMem.phy_addr;
    encIn->busOutBuf = mpp_buffer_get_fd(outbufMem);
    // mask and add by lance 2016.05.06
    //encIn.outBufSize = outbufMem.size;
    mpp_buffer_info_get(outbufMem, &outInfo);
    encIn->outBufSize = outInfo.size;

    /* Allocate memory for NAL unit size buffer, optional */

    encIn->naluSizeBufSize = NALU_TABLE_SIZE * sizeof(u32);
    encIn->pNaluSizeBuf = (u32 *) malloc(encIn->naluSizeBufSize);

    if (!encIn->pNaluSizeBuf) {
        mpp_err("WARNING! Failed to allocate NAL unit size buffer.\n");
    }

    /* Source Image Size */
    if (cml->inputFormat <= 1) {
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc +
                       2 * (((cml->lumWidthSrc + 1) >> 1) *
                            ((cml->lumHeightSrc + 1) >> 1));
    } else if (cml->inputFormat <= 9) {
        /* 422 YUV or 16-bit RGB */
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc * 2;
    } else {
        /* 32-bit RGB */
        src_img_size = cml->lumWidthSrc * cml->lumHeightSrc * 4;
    }

    mpp_log("Reading input from file <%s>, frame size %d bytes.\n",
            cml->input, src_img_size);


    /* Start stream */
    ret = H264EncStrmStart(encoder, encIn, encOut);
    if (ret != H264ENC_OK) {
        PrintErrorValue("H264EncStrmStart() failed.", ret);
        return -1;
    }

    fout = fopen(cml->output, "wb");
    if (fout == NULL) {
        mpp_err( "Failed to create the output file %s.\n", cml->output);
        return -1;
    }

    //WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);  // mask by lance 2016.05.06
    WriteStrm(fout, (u32*)mpp_buffer_get_ptr(outbufMem), encOut->streamSize, 0);  // add by lance 2016.05.06
    if (cml->byteStream == 0) {
        WriteNalSizesToFile(nal_sizes_file, encIn->pNaluSizeBuf,
                            encIn->naluSizeBufSize);
    }

    streamSize += encOut->streamSize;

    H264EncGetRateCtrl(encoder, &rc);

    /* Allocate a buffer for user data and read data from file */
    pUserData = ReadUserData(encoder, cml->userData);

    mpp_log("\n");
    mpp_log("Input | Pic | QP | Type |  BR avg  | ByteCnt (inst) |");
    if (cml->psnr)
        mpp_log(" PSNR  | NALU sizes\n");
    else
        mpp_log(" NALU sizes\n");

    mpp_log("------------------------------------------------------------------------\n");
    mpp_log("      |     | %2d | HDR  |          | %7i %6i | ",
            rc.qpHdr, streamSize, encOut->streamSize);
    if (cml->psnr)
        mpp_log("      | ");
    // mask and add by lance 2016.05.06
    /*PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.vir_addr,
                  encOut.streamSize, cml->byteStream);*/
    PrintNalSizes(encIn->pNaluSizeBuf/*, (u8 *) mpp_buffer_get_ptr(outbufMem),
                                      encOut.streamSize, cml->byteStream*/);  // mask by lance 2016.05.12
    mpp_log("\n");

    /* Setup encoder input */
    {
        u32 w = (cml->lumWidthSrc + 15) & (~0x0f);

        encIn->busLuma = mpp_buffer_get_fd(pictureMem)/*pictureMem.phy_addr*/;

        encIn->busChromaU = encIn->busLuma | ((w * cml->lumHeightSrc) << 10);
        encIn->busChromaV = encIn->busChromaU |
                            ((((w + 1) >> 1) * ((cml->lumHeightSrc + 1) >> 1)) << 10);
    }

    /* First frame is always intra with time increment = 0 */
    encIn->codingType = H264ENC_INTRA_FRAME;
    encIn->timeIncrement = 0;

    encIn->busLumaStab = mpp_buffer_get_fd(pictureStabMem)/*pictureStabMem.phy_addr*/;

    intraPeriodCnt = cml->intraPicRate;

    return 0;
}

int H264encodeExit(H264EncInst encoderInst, commandLine_s * cml)
{
    H264EncRet ret;
    h264Instance_s *pInst = (h264Instance_s *)encoderInst;
    H264EncIn *encIn = &(pInst->encIn);  // add by lance 2016.05.31
    H264EncOut *encOut = &(pInst->encOut);

    /* End stream */
    ret = H264EncStrmEnd(encoderInst, encIn, encOut);
    if (ret != H264ENC_OK) {
        PrintErrorValue("H264EncStrmEnd() failed.", ret);
    } else {
        streamSize += encOut->streamSize;
        mpp_log("      |     |    | END  |          | %7i %6i | ",
                streamSize, encOut->streamSize);
        if (cml->psnr)
            mpp_log("      | ");
        // mask and add by lance 2016.05.06
        /*PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.vir_addr,
                      encOut.streamSize, cml->byteStream);*/
        PrintNalSizes(encIn->pNaluSizeBuf/*, (u8 *) mpp_buffer_get_ptr(outbufMem),
                                          encOut.streamSize, cml->byteStream*/);  // mask by lance 2016.05.12
        mpp_log("\n");

//        WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);

        if (cml->byteStream == 0) {
            WriteNalSizesToFile(nal_sizes_file, encIn->pNaluSizeBuf,
                                encIn->naluSizeBufSize);
        }
    }

    // add by lance 2016.05.31
    // for free the malloced memory
    if (encIn->pNaluSizeBuf != NULL) {
        free(encIn->pNaluSizeBuf);
        encIn->pNaluSizeBuf = NULL;
    }

    mpp_log("\nBitrate target %d bps, actual %d bps (%d%%).\n",
            rc.bitPerSecond, bitrate,
            (rc.bitPerSecond) ? bitrate * 100 / rc.bitPerSecond : 0);
    mpp_log("Total of %d frames processed, %d frames encoded, %d bytes.\n",
            frameCnt, codedFrameCnt, streamSize);

    if (psnrCnt)
        mpp_log("Average PSNR %d.%02d\n",
                (psnrSum / psnrCnt) / 100, (psnrSum / psnrCnt) % 100);

    return 0;
}

/*------------------------------------------------------------------------------

    H264encodeOneFrame

    Do the encoding.

    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        encoderInst - encoder instance
        cml     - processed comand line options

    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int H264encodeOneFrame(void *ctx, HalTaskInfo *task_info, H264EncInst encoderInst, commandLine_s * cml, h264e_syntax *syntax_data)
{
    H264EncRet ret;
    MPP_RET vpuWaitResult = MPP_OK;
    h264Instance_s *pInst = (h264Instance_s *)encoderInst;
    H264EncIn *encIn = &(pInst->encIn);  // add by lance 2016.05.31
    H264EncOut *encOut = &(pInst->encOut);  // add by lance 2016.05.31

    /* Main encoding loop */
    if ((next = NextPic(cml->inputRateNumer, cml->inputRateDenom,
                        cml->outputRateNumer, cml->outputRateDenom, frameCnt,
                        cml->firstPic)) <= cml->lastPic) {
#ifdef EVALUATION_LIMIT
        if (frameCnt >= EVALUATION_LIMIT)
            break;
#endif

#ifndef NO_INPUT_READ
        /* Read next frame */
        if (ReadPic((u8 *) mpp_buffer_get_ptr(pictureMem)/*pictureMem.vir_addr*/,  // mask and add by lance 2016.05.06
                    src_img_size, next, cml->input,
                    cml->lumWidthSrc, cml->lumHeightSrc, cml->inputFormat) != 0)
            return -11;

        if (cml->videoStab > 0) {
            /* Stabilize the frame after current frame */
            i32 nextStab = NextPic(cml->inputRateNumer, cml->inputRateDenom,
                                   cml->outputRateNumer, cml->outputRateDenom, frameCnt + 1,
                                   cml->firstPic);

            if (ReadPic((u8 *) mpp_buffer_get_ptr(pictureStabMem)/*pictureStabMem.vir_addr*/,
                        src_img_size, nextStab, cml->input,
                        cml->lumWidthSrc, cml->lumHeightSrc,
                        cml->inputFormat) != 0)
                return -11;
        }
#endif

        for (i = 0; i < MAX_BPS_ADJUST; i++)
            if (cml->bpsAdjustFrame[i] &&
                (codedFrameCnt == cml->bpsAdjustFrame[i])) {
                rc.bitPerSecond = cml->bpsAdjustBitrate[i];
                mpp_log("Adjusting bitrate target: %d\n", rc.bitPerSecond);
                if ((ret = H264EncSetRateCtrl(encoderInst, &rc)) != H264ENC_OK) {
                    PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
                }
            }


        /* Select frame type */
        if ((cml->intraPicRate != 0) && (intraPeriodCnt >= cml->intraPicRate))
            encIn->codingType = H264ENC_INTRA_FRAME;
        else
            encIn->codingType = H264ENC_PREDICTED_FRAME;

        if (encIn->codingType == H264ENC_INTRA_FRAME)
            intraPeriodCnt = 0;

        ret = H264EncStrmEncode(encoderInst, encIn, encOut, syntax_data);

        hal_h264e_gen_regs(ctx, task_info);  // add by lance 2016.05.08
        hal_h264e_start(ctx, task_info);  // add by lance 2016.05.08
        vpuWaitResult = hal_h264e_wait(ctx, task_info);  // add by lance 2016.05.08

        ret = H264EncStrmEncodeAfter(encoderInst, encIn, encOut, vpuWaitResult);  // add by lance 2016.05.07

        H264EncGetRateCtrl(encoderInst, &rc);

        streamSize += encOut->streamSize;

        /* Note: This will overflow if large output rate numerator is used */
        if ((frameCnt + 1) && cml->outputRateDenom)
            bitrate = 8 * ((streamSize / (frameCnt + 1)) *
                           (u32) cml->outputRateNumer / (u32) cml->outputRateDenom);

        switch (ret) {
        case H264ENC_FRAME_READY:

            mpp_log("%5i | %3i | %2i | %s | %8u | %7i %6i | ",
                    next, frameCnt, rc.qpHdr,
                    encOut->codingType == H264ENC_INTRA_FRAME ? " I  " :
                    encOut->codingType == H264ENC_PREDICTED_FRAME ? " P  " : "skip",
                    bitrate, streamSize, encOut->streamSize);
            if (cml->psnr)
                psnr = PrintPSNR(/*(u8 *)
                                 (((h264Instance_s *)encoderInst)->asic.regs.inputLumBase +
                                  ((h264Instance_s *)encoderInst)->asic.regs.inputLumaBaseOffset),
                                 (u8 *)
                                 (((h264Instance_s *)encoderInst)->asic.regs.internalImageLumBaseR),
                                 cml->lumWidthSrc, cml->width, cml->height*/);  // mask by lance 2016.05.12
            if (psnr) {
                psnrSum += psnr;
                psnrCnt++;
            }
            // mask and add by lance 2016.05.06
            /*PrintNalSizes(encIn.pNaluSizeBuf, (u8 *) outbufMem.vir_addr,
              encOut.streamSize, cml->byteStream);*/
            PrintNalSizes(encIn->pNaluSizeBuf/*, (u8 *) mpp_buffer_get_ptr(outbufMem),
                                              encOut.streamSize, cml->byteStream*/);  // mask by lance 2016.05.12
            mpp_log("\n");

            // mask and add by lance 2016.05.06
            //WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);
            WriteStrm(fout, (u32*)mpp_buffer_get_ptr(outbufMem), encOut->streamSize, 0);

            if (cml->byteStream == 0) {
                WriteNalSizesToFile(nal_sizes_file, encIn->pNaluSizeBuf,
                                    encIn->naluSizeBufSize);
            }

            if (pUserData) {
                /* We want the user data to be written only once so
                 * we disable the user data and free the memory after
                 * first frame has been encoded. */
                H264EncSetSeiUserData(encoderInst, NULL, 0);
                free(pUserData);
                pUserData = NULL;
            }

            break;

        case H264ENC_OUTPUT_BUFFER_OVERFLOW:
            mpp_log("%5i | %3i | %2i | %s | %8u | %7i %6i | \n",
                    next, frameCnt, rc.qpHdr, "lost",
                    bitrate, streamSize, encOut->streamSize);
            break;

        default:
            PrintErrorValue("H264EncStrmEncode() failed.", ret);
            /* For debugging, can be removed */
            // mask and add by lance 2016.05.06
            //WriteStrm(fout, outbufMem.vir_addr, encOut.streamSize, 0);
            WriteStrm(fout, (u32*)mpp_buffer_get_ptr(outbufMem), encOut->streamSize, 0);
            /* We try to continue encoding the next frame */
            break;
        }

        encIn->timeIncrement = cml->outputRateDenom;

        frameCnt++;

        if (encOut->codingType != H264ENC_NOTCODED_FRAME) {
            intraPeriodCnt++; codedFrameCnt++;
        }

    } else {
        return -10;
    }

    return ret;
}

/*------------------------------------------------------------------------------

    AllocRes

    Allocation of the physical memories used by both SW and HW:
    the input pictures and the output stream buffer.

    NOTE! The implementation uses the EWL instance from the encoder
          for OS independence. This is not recommended in final environment
          because the encoder will release the EWL instance in case of error.
          Instead, the memories should be allocated from the OS the same way
          as inside EWLMallocLinear().

------------------------------------------------------------------------------*/
int AllocRes(commandLine_s * cmdlForAlloc/*, H264EncInst enc*/)  // mask by lance 2016.05.12
{
    i32 ret;
    u32 pictureSize;
    u32 outbufSize;
    //MppBufferInfo pictureMemInfo;  // mask by lance 2016.05.12

    if (cmdlForAlloc->inputFormat <= 1) {
        /* Input picture in planar YUV 4:2:0 format */
        pictureSize =
            ((cmdlForAlloc->lumWidthSrc + 15) & (~15)) * cmdlForAlloc->lumHeightSrc * 3 / 2;
    } else if (cmdlForAlloc->inputFormat <= 9) {
        /* Input picture in YUYV 4:2:2 or 16-bit RGB format */
        pictureSize =
            ((cmdlForAlloc->lumWidthSrc + 15) & (~15)) * cmdlForAlloc->lumHeightSrc * 2;
    } else {
        /* Input picture in 32-bit RGB format */
        pictureSize =
            ((cmdlForAlloc->lumWidthSrc + 15) & (~15)) * cmdlForAlloc->lumHeightSrc * 4;
    }

    mpp_log("Input %dx%d encoding at %dx%d\n", cmdlForAlloc->lumWidthSrc,
            cmdlForAlloc->lumHeightSrc, cmdlForAlloc->width, cmdlForAlloc->height);

    memGroup = NULL;  // add by lance 2016.05.06
    //pictureMem.vir_addr = NULL;  // mask by lance 2016.05.06
    pictureMem = NULL;  // add by lance 2016.05.06
    outbufMem = NULL;
    //pictureStabMem.vir_addr = NULL;  // mask by lance 2016.05.06
    pictureStabMem = NULL;  // add by lance 2016.05.06

    // add by lance 2016.05.06
    if (memGroup == NULL) {
        ret = mpp_buffer_group_get_internal(&memGroup, MPP_BUFFER_TYPE_ION);
        if (MPP_OK != ret) {
            mpp_err("memGroup mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    /* Here we use the EWL instance directly from the encoder
     * because it is the easiest way to allocate the linear memories */
    // mask and add by lance 2016.05.06
    /*ret = VPUMallocLinear(&pictureMem, pictureSize);
    if (ret != EWL_OK) {
        ALOGV( "Failed to allocate input picture!\n");
        pictureMem.vir_addr = NULL;
        return 1;
    }

    if (cmdl->videoStab > 0) {
        ret = VPUMallocLinear(&pictureStabMem, pictureSize);
        if (ret != EWL_OK) {
            ALOGV( "Failed to allocate stab input picture!\n");
            pictureStabMem.vir_addr = NULL;
            return 1;
        }
    }*/
    ret = mpp_buffer_get(memGroup, &pictureMem, pictureSize);
    if (ret != MPP_OK) {
        mpp_err( "Failed to allocate pictureMem buffer!\n");
        pictureMem = NULL;
        return ret;
    }

    if (cmdlForAlloc->videoStab > 0) {
        ret = mpp_buffer_get(memGroup, &pictureStabMem, pictureSize);
        if (ret != MPP_OK) {
            mpp_err( "Failed to allocate pictureStabMem buffer!\n");
            pictureStabMem = NULL;
            return ret;
        }
    }

    // mask by lance 2016.05.12
    //mpp_buffer_info_get(pictureMem, &pictureMemInfo);  // add by lance 2016.05.06
    //outbufSize = 4 * /*pictureMem.size*/pictureMemInfo.size < (1024 * 1024 * 3 / 2) ?
    //             4 * /*pictureMem.size*/pictureMemInfo.size : (1024 * 1024 * 3 / 2);
    outbufSize = 2 * 1024 * 1024;  // add by lance 2016.05.12
    // mask and add by lance 2016.05.06
    /*ret = VPUMallocLinear(&outbufMem, outbufSize);
    if (ret != EWL_OK) {
        ALOGV( "Failed to allocate output buffer!\n");
        outbufMem.vir_addr = NULL;
        return 1;
    }*/
    ret = mpp_buffer_get(memGroup, &outbufMem, outbufSize);
    if (ret != MPP_OK) {
        mpp_err( "Failed to allocate output buffer!\n");
        outbufMem = NULL;
        return 1;
    }

    // mask by lance 2016.05.06
    /*ALOGV("Input buffer size:          %d bytes\n", pictureMem.size);
    ALOGV("Input buffer bus address:   0x%08x\n", pictureMem.phy_addr);
    ALOGV("Input buffer user address:  0x%08x\n", (u32) pictureMem.vir_addr);
    ALOGV("Output buffer size:         %d bytes\n", outbufMem.size);
    ALOGV("Output buffer bus address:  0x%08x\n", outbufMem.phy_addr);
    ALOGV("Output buffer user address: 0x%08x\n", (u32) outbufMem.vir_addr);*/

    return 0;
}

/*------------------------------------------------------------------------------

    FreeRes

    Release all resources allcoated byt AllocRes()

------------------------------------------------------------------------------*/
void FreeRes(/*H264EncInst enc*/)  // mask by lance 2016.05.12
{
    // mask and add by lance 2016.05.06
    /*if (pictureMem.vir_addr != NULL)
        VPUFreeLinear(&pictureMem);
    if (pictureStabMem.vir_addr != NULL)
        VPUFreeLinear(&pictureStabMem);*/
    if (pictureMem != NULL) {
        mpp_buffer_put(pictureMem);
        pictureMem = NULL;
    }
    if (pictureStabMem != NULL) {
        mpp_buffer_put(pictureStabMem);
        pictureStabMem = NULL;
    }
    // mask by lance 2016.05.06
    /*if (outbufMem.vir_addr != NULL)
        VPUFreeLinear(&outbufMem);*/
    if (outbufMem != NULL) {
        mpp_buffer_put(outbufMem);
        outbufMem = NULL;
    }

    // add by lance 2016.05.06
    if (memGroup != NULL) {
        mpp_err("memGroup deInit");
        mpp_buffer_group_put(memGroup);
    }
}

/*------------------------------------------------------------------------------

    OpenEncoder
        Create and configure an encoder instance.

    Params:
        cml     - processed comand line options
        pEnc    - place where to save the new encoder instance
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int OpenEncoder(commandLine_s * cml, h264Instance_s * pEnc)
{
    H264EncRet ret;
    H264EncConfig cfg;
    H264EncCodingCtrl codingCfg;
    H264EncRateCtrl rcCfg;
    H264EncPreProcessingCfg preProcCfg;

    H264EncInst encoderOpen;
    //h264Instance_s *pEnc264;  // mask by lance 2016.05.12

    /* Encoder initialization */
    if (cml->width == DEFAULT)
        cml->width = cml->lumWidthSrc;

    if (cml->height == DEFAULT)
        cml->height = cml->lumHeightSrc;

    /* outputRateNumer */
    if (cml->outputRateNumer == DEFAULT) {
        cml->outputRateNumer = cml->inputRateNumer;
    }

    /* outputRateDenom */
    if (cml->outputRateDenom == DEFAULT) {
        cml->outputRateDenom = cml->inputRateDenom;
    }

    if (cml->rotation) {
        cfg.width = cml->height;
        cfg.height = cml->width;
    } else {
        cfg.width = cml->width;
        cfg.height = cml->height;
    }

    cfg.frameRateDenom = cml->outputRateDenom;
    cfg.frameRateNum = cml->outputRateNumer;
    if (cml->byteStream == 0)
        cfg.streamType = H264ENC_BYTE_STREAM;
    else if (cml->byteStream == 1)
        cfg.streamType = H264ENC_NAL_UNIT_STREAM;

    cfg.level = H264ENC_LEVEL_3;

    if (cml->level != DEFAULT && cml->level != 0)
        cfg.level = (H264EncLevel)cml->level;

    mpp_log("Init config: size %dx%d   %d/%d fps  %s P&L %d\n",
            cfg.width, cfg.height, cfg.frameRateNum,
            cfg.frameRateDenom, streamType[cfg.streamType], cfg.level);
    // add by lance 2016.05.11
    // ----------------
#ifdef WRITE_CMD_INFO_FOR_COMPARE
    if (cmdFlagTest == 0 && cmdCompareFile != NULL) {
        int iCmdCount = 0;
        fprintf(cmdCompareFile, "cmd->input 					  %s\n", cml->input);
        fprintf(cmdCompareFile, "cmd->output					  %s\n", cml->output);
        fprintf(cmdCompareFile, "cmd->userData					  %s\n", cml->userData);
        fprintf(cmdCompareFile, "cmd->firstPic					  %d\n", cml->firstPic);
        fprintf(cmdCompareFile, "cmd->lastPic					  %d\n", cml->lastPic);
        fprintf(cmdCompareFile, "cmd->width 					  %d\n", cml->width);
        fprintf(cmdCompareFile, "cmd->lumWidthSrc				  %d\n", cml->lumWidthSrc);
        fprintf(cmdCompareFile, "cmd->lumHeightSrc				  %d\n", cml->lumHeightSrc);
        fprintf(cmdCompareFile, "cmd->horOffsetSrc				  %d\n", cml->horOffsetSrc);
        fprintf(cmdCompareFile, "cmd->verOffsetSrc				  %d\n", cml->verOffsetSrc);
        fprintf(cmdCompareFile, "cmd->outputRateNumer			  %d\n", cml->outputRateNumer);
        fprintf(cmdCompareFile, "cmd->outputRateDenom			  %d\n", cml->outputRateDenom);
        fprintf(cmdCompareFile, "cmd->inputRateNumer			  %d\n", cml->inputRateNumer);
        fprintf(cmdCompareFile, "cmd->inputRateDenom			  %d\n", cml->inputRateDenom);
        fprintf(cmdCompareFile, "cmd->level 					  %d\n", cml->level);
        fprintf(cmdCompareFile, "cmd->hrdConformance			  %d\n", cml->hrdConformance);
        fprintf(cmdCompareFile, "cmd->cpbSize					  %d\n", cml->cpbSize);
        fprintf(cmdCompareFile, "cmd->intraPicRate				  %d\n", cml->intraPicRate);
        fprintf(cmdCompareFile, "cmd->constIntraPred			  %d\n", cml->constIntraPred);
        fprintf(cmdCompareFile, "cmd->disableDeblocking 		  %d\n", cml->disableDeblocking);
        fprintf(cmdCompareFile, "cmd->mbPerSlice				  %d\n", cml->mbPerSlice);
        fprintf(cmdCompareFile, "cmd->qpHdr 					  %d\n", cml->qpHdr);
        fprintf(cmdCompareFile, "cmd->qpMin 					  %d\n", cml->qpMin);
        fprintf(cmdCompareFile, "cmd->qpMax 					  %d\n", cml->qpMax);
        fprintf(cmdCompareFile, "cmd->bitPerSecond				  %d\n", cml->bitPerSecond);
        fprintf(cmdCompareFile, "cmd->picRc 					  %d\n", cml->picRc);
        fprintf(cmdCompareFile, "cmd->mbRc						  %d\n", cml->mbRc);
        fprintf(cmdCompareFile, "cmd->picSkip					  %d\n", cml->picSkip);
        fprintf(cmdCompareFile, "cmd->rotation					  %d\n", cml->rotation);
        fprintf(cmdCompareFile, "cmd->inputFormat				  %d\n", cml->inputFormat);
        fprintf(cmdCompareFile, "cmd->colorConversion			  %d\n", cml->colorConversion);
        fprintf(cmdCompareFile, "cmd->videoBufferSize			  %d\n", cml->videoBufferSize);
        fprintf(cmdCompareFile, "cmd->videoRange				  %d\n", cml->videoRange);
        fprintf(cmdCompareFile, "cmd->chromaQpOffset			  %d\n", cml->chromaQpOffset);
        fprintf(cmdCompareFile, "cmd->filterOffsetA 			  %d\n", cml->filterOffsetA);
        fprintf(cmdCompareFile, "cmd->filterOffsetB 			  %d\n", cml->filterOffsetB);
        fprintf(cmdCompareFile, "cmd->trans8x8					  %d\n", cml->trans8x8);
        fprintf(cmdCompareFile, "cmd->enableCabac				  %d\n", cml->enableCabac);
        fprintf(cmdCompareFile, "cmd->cabacInitIdc				  %d\n", cml->cabacInitIdc);
        fprintf(cmdCompareFile, "cmd->testId					  %d\n", cml->testId);
        fprintf(cmdCompareFile, "cmd->burst 					  %d\n", cml->burst);
        fprintf(cmdCompareFile, "cmd->bursttype 				  %d\n", cml->bursttype);
        fprintf(cmdCompareFile, "cmd->quarterPixelMv			  %d\n", cml->quarterPixelMv);
        fprintf(cmdCompareFile, "cmd->sei						  %d\n", cml->sei);
        fprintf(cmdCompareFile, "cmd->byteStream				  %d\n", cml->byteStream);
        fprintf(cmdCompareFile, "cmd->videoStab 				  %d\n", cml->videoStab);
        fprintf(cmdCompareFile, "cmd->gopLength 				  %d\n", cml->gopLength);
        fprintf(cmdCompareFile, "cmd->intraQpDelta				  %d\n", cml->intraQpDelta);
        fprintf(cmdCompareFile, "cmd->fixedIntraQp				  %d\n", cml->fixedIntraQp);
        fprintf(cmdCompareFile, "cmd->mbQpAdjustment			  %d\n", cml->mbQpAdjustment);
        fprintf(cmdCompareFile, "cmd->testParam 				  %d\n", cml->testParam);
        fprintf(cmdCompareFile, "cmd->psnr						  %d\n", cml->psnr);
        for (iCmdCount = 0; iCmdCount < MAX_BPS_ADJUST; ++iCmdCount) {
            fprintf(cmdCompareFile, "cmd->bpsAdjustFrame[%2d]		   %d\n", iCmdCount, cml->bpsAdjustFrame[iCmdCount]);
        }
        for (iCmdCount = 0; iCmdCount < MAX_BPS_ADJUST; ++iCmdCount) {
            fprintf(cmdCompareFile, "cmd->bpsAdjustBitrate[%2d] 	   %d\n", iCmdCount, cml->bpsAdjustBitrate[iCmdCount]);
        }
        fprintf(cmdCompareFile, "cmd->framerateout				  %d\n", cml->framerateout);
        fflush(cmdCompareFile);
        ++cmdFlagTest;
        if (cmdCompareFile != NULL && (cmdFlagTest != 0)) {
            fclose(cmdCompareFile);
            cmdCompareFile = NULL;
        }
    }
#endif
    // ----------------

    if ((ret = H264EncInit(&cfg, pEnc)) != H264ENC_OK) {
        PrintErrorValue("H264EncInit() failed.", ret);
        return -1;
    }

    encoderOpen = (H264EncInst)pEnc;  // modify by lance 2016.05.06

    //get socket
    //pEnc264 = (h264Instance_s *)encoderOpen;  // mask by lance 2016.05.12
    //pEnc264->asic.regs.socket = VPUClientInit(VPU_ENC);  // because socket is inited by hal part, so mask the code  by lance 2016.05.09

    /* Encoder setup: rate control */
    if ((ret = H264EncGetRateCtrl(encoderOpen, &rcCfg)) != H264ENC_OK) {
        PrintErrorValue("H264EncGetRateCtrl() failed.", ret);
        CloseEncoder(encoderOpen);
        return -1;
    } else {
        mpp_log("Get rate control: qp %2d [%2d, %2d]  %8d bps  "
                "pic %d mb %d skip %d  hrd %d\n  cpbSize %d gopLen %d "
                "intraQpDelta %2d\n",
                rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
                rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
                rcCfg.hrdCpbSize, rcCfg.gopLen, rcCfg.intraQpDelta);

        if (cml->qpHdr != DEFAULT)
            rcCfg.qpHdr = cml->qpHdr;
        if (cml->qpMin != DEFAULT)
            rcCfg.qpMin = cml->qpMin;
        if (cml->qpMax != DEFAULT)
            rcCfg.qpMax = cml->qpMax;
        if (cml->picSkip != DEFAULT)
            rcCfg.pictureSkip = cml->picSkip;
        if (cml->picRc != DEFAULT)
            rcCfg.pictureRc = cml->picRc;
        if (cml->mbRc != DEFAULT)
            rcCfg.mbRc = cml->mbRc != 0 ? 1 : 0;
        if (cml->bitPerSecond != DEFAULT)
            rcCfg.bitPerSecond = cml->bitPerSecond;

        if (cml->hrdConformance != DEFAULT)
            rcCfg.hrd = cml->hrdConformance;

        if (cml->cpbSize != DEFAULT)
            rcCfg.hrdCpbSize = cml->cpbSize;

        if (cml->intraPicRate != 0)
            rcCfg.gopLen = MIN(cml->intraPicRate, MAX_GOP_LEN);

        if (cml->gopLength != DEFAULT)
            rcCfg.gopLen = cml->gopLength;

        if (cml->intraQpDelta != DEFAULT)
            rcCfg.intraQpDelta = cml->intraQpDelta;

        rcCfg.fixedIntraQp = cml->fixedIntraQp;
        rcCfg.mbQpAdjustment = cml->mbQpAdjustment;

        mpp_log("Set rate control: qp %2d [%2d, %2d] %8d bps  "
                "pic %d mb %d skip %d  hrd %d\n"
                "  cpbSize %d gopLen %d intraQpDelta %2d "
                "fixedIntraQp %2d mbQpAdjustment %d\n",
                rcCfg.qpHdr, rcCfg.qpMin, rcCfg.qpMax, rcCfg.bitPerSecond,
                rcCfg.pictureRc, rcCfg.mbRc, rcCfg.pictureSkip, rcCfg.hrd,
                rcCfg.hrdCpbSize, rcCfg.gopLen, rcCfg.intraQpDelta,
                rcCfg.fixedIntraQp, rcCfg.mbQpAdjustment);

        if ((ret = H264EncSetRateCtrl(encoderOpen, &rcCfg)) != H264ENC_OK) {
            PrintErrorValue("H264EncSetRateCtrl() failed.", ret);
            CloseEncoder(encoderOpen);
            return -1;
        }
    }

    /* Encoder setup: coding control */
    if ((ret = H264EncGetCodingCtrl(encoderOpen, &codingCfg)) != H264ENC_OK) {
        PrintErrorValue("H264EncGetCodingCtrl() failed.", ret);
        CloseEncoder(encoderOpen);
        return -1;
    } else {
        if (cml->mbPerSlice != DEFAULT) {
            codingCfg.sliceSize = cml->mbPerSlice / (cfg.width / 16);
        }

        if (cml->constIntraPred != DEFAULT) {
            if (cml->constIntraPred != 0)
                codingCfg.constrainedIntraPrediction = 1;
            else
                codingCfg.constrainedIntraPrediction = 0;
        }

        if (cml->disableDeblocking != 0)
            codingCfg.disableDeblockingFilter = 1;
        else
            codingCfg.disableDeblockingFilter = 0;

        if ((cml->disableDeblocking != 1) &&
            ((cml->filterOffsetA != 0) || (cml->filterOffsetB != 0)))
            codingCfg.disableDeblockingFilter = 1;

        if (cml->enableCabac != DEFAULT) {
            codingCfg.enableCabac = cml->enableCabac;
            if (cml->cabacInitIdc != DEFAULT)
                codingCfg.cabacInitIdc = cml->cabacInitIdc;
        }

        codingCfg.transform8x8Mode = cml->trans8x8;

        if (cml->videoRange != DEFAULT) {
            if (cml->videoRange != 0)
                codingCfg.videoFullRange = 1;
            else
                codingCfg.videoFullRange = 0;
        }

        if (cml->sei)
            codingCfg.seiMessages = 1;
        else
            codingCfg.seiMessages = 0;

        mpp_log("Set coding control: SEI %d Slice %5d   deblocking %d "
                "constrained intra %d video range %d\n"
                "  cabac %d cabac initial idc %d Adaptive 8x8 transform %d\n",
                codingCfg.seiMessages, codingCfg.sliceSize,
                codingCfg.disableDeblockingFilter,
                codingCfg.constrainedIntraPrediction, codingCfg.videoFullRange, codingCfg.enableCabac,
                codingCfg.cabacInitIdc, codingCfg.transform8x8Mode );

        if ((ret = H264EncSetCodingCtrl(encoderOpen, &codingCfg)) != H264ENC_OK) {
            PrintErrorValue("H264EncSetCodingCtrl() failed.", ret);
            CloseEncoder(encoderOpen);
            return -1;
        }

#ifdef INTERNAL_TEST
        /* Set some values outside the product API for internal
         * testing purposes */

        H264EncSetChromaQpIndexOffset(encoderOpen, cml->chromaQpOffset);
        mpp_log("Set ChromaQpIndexOffset: %d\n", cml->chromaQpOffset);

        H264EncSetHwBurstSize(encoderOpen, cml->burst);
        mpp_log("Set HW Burst Size: %d\n", cml->burst);
        H264EncSetHwBurstType(encoderOpen, cml->bursttype);
        mpp_log("Set HW Burst Type: %d\n", cml->bursttype);
        if (codingCfg.disableDeblockingFilter == 1) {
            H264EncFilter advCoding;

            H264EncGetFilter(encoderOpen, &advCoding);

            advCoding.disableDeblocking = cml->disableDeblocking;

            advCoding.filterOffsetA = cml->filterOffsetA * 2;
            advCoding.filterOffsetB = cml->filterOffsetB * 2;

            mpp_log
            ("Set filter params: disableDeblocking %d filterOffsetA = %i filterOffsetB = %i\n",
             advCoding.disableDeblocking, advCoding.filterOffsetA,
             advCoding.filterOffsetB);

            ret = H264EncSetFilter(encoderOpen, &advCoding);
            if (ret != H264ENC_OK) {
                PrintErrorValue("H264EncSetFilter() failed.", ret);
                CloseEncoder(encoderOpen);
                return -1;
            }
        }
        if (cml->quarterPixelMv != DEFAULT) {
            H264EncSetQuarterPixelMv(encoderOpen, cml->quarterPixelMv);
            mpp_log("Set Quarter Pixel MV: %d\n", cml->quarterPixelMv);
        }
#endif

    }

    /* PreP setup */
    if ((ret = H264EncGetPreProcessing(encoderOpen, &preProcCfg)) != H264ENC_OK) {
        PrintErrorValue("H264EncGetPreProcessing() failed.", ret);
        CloseEncoder(encoderOpen);
        return -1;
    }
    mpp_log
    ("Get PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
     ": stab %d : cc %d\n",
     preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
     preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
     preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    preProcCfg.inputType = (H264EncPictureFormat)cml->inputFormat;
    preProcCfg.rotation = (H264EncPictureRotation)cml->rotation;

    preProcCfg.origWidth =
        cml->lumWidthSrc /*(cml->lumWidthSrc + 15) & (~0x0F) */ ;
    preProcCfg.origHeight = cml->lumHeightSrc;

    if (cml->horOffsetSrc != DEFAULT)
        preProcCfg.xOffset = cml->horOffsetSrc;
    if (cml->verOffsetSrc != DEFAULT)
        preProcCfg.yOffset = cml->verOffsetSrc;
    if (cml->videoStab != DEFAULT)
        preProcCfg.videoStabilization = cml->videoStab;
    if (cml->colorConversion != DEFAULT)
        preProcCfg.colorConversion.type =
            (H264EncColorConversionType)cml->colorConversion;
    if (preProcCfg.colorConversion.type == H264ENC_RGBTOYUV_USER_DEFINED) {
        preProcCfg.colorConversion.coeffA = 20000;
        preProcCfg.colorConversion.coeffB = 44000;
        preProcCfg.colorConversion.coeffC = 5000;
        preProcCfg.colorConversion.coeffE = 35000;
        preProcCfg.colorConversion.coeffF = 38000;
    }

    mpp_log
    ("Set PreP: input %4dx%d : offset %4dx%d : format %d : rotation %d "
     ": stab %d : cc %d\n",
     preProcCfg.origWidth, preProcCfg.origHeight, preProcCfg.xOffset,
     preProcCfg.yOffset, preProcCfg.inputType, preProcCfg.rotation,
     preProcCfg.videoStabilization, preProcCfg.colorConversion.type);

    if ((ret = H264EncSetPreProcessing(encoderOpen, &preProcCfg)) != H264ENC_OK) {
        PrintErrorValue("H264EncSetPreProcessing() failed.", ret);
        CloseEncoder(encoderOpen);
        return -1;
    }
    return 0;
}

/*------------------------------------------------------------------------------

    CloseEncoder
       Release an encoder insatnce.

   Params:
        encoderForClose - the instance to be released
------------------------------------------------------------------------------*/
void CloseEncoder(H264EncInst encoderForClose)
{
    H264EncRet ret;

    if ((ret = H264EncRelease(encoderForClose)) != H264ENC_OK) {
        PrintErrorValue("H264EncRelease() failed.", ret);
    }
}

/*------------------------------------------------------------------------------

    Parameter
        Process the testbench calling arguments.

    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        cml     - processed comand line options
    Return:
        0   - for success
        -1  - error

------------------------------------------------------------------------------*/
int Parameter(commandLine_s * cml)
{
    //i32 ret/*, i*/;  // mask by lance 2016.05.12
    //char *optArg;  // mask by lance 2016.05.12
    //argument_s argument;  // mask by lance 2016.05.12
    //i32 status = 0;  // mask by lance 2016.05.12
    //i32 bpsAdjustCount = 0;  // mask by lance 2016.05.12

    memset(cml, 0, sizeof(commandLine_s));

    cml->input = input;
    cml->output = output;
    cml->firstPic = 0;
    cml->lastPic = 65535;//100;    // modify 100 to 65535 by lance 2016.05.11
    cml->lumWidthSrc = 352/*1280*/;
    cml->lumHeightSrc = 288/*720*/;

    cml->width = 352/*1280*/;  //output width
    cml->height = 288/*720*/;  //output height
    cml->horOffsetSrc = 0;  //Output image horizontal offset
    cml->verOffsetSrc = 0;  //Output image vertical offset

    cml->outputRateNumer = 25;//30;    // modify 30 to 25 by lance 2016.05.11
    cml->outputRateDenom = 1;
    cml->inputRateNumer = 25;//30;    // modify 30 to 25 by lance 2016.05.11
    cml->inputRateDenom = 1;

    cml->level = 40;
    cml->byteStream = 1;    //nal stream:1; byte stream:0;

    cml->sei = 0;       //Enable/Disable SEI messages.[0]

    cml->rotation = 0;
    cml->inputFormat = 0;   //YUV420
    cml->colorConversion = 0;   //RGB to YCbCr color conversion type---BT.601
    cml->videoRange = 0;    //0..1 Video range.

    cml->videoStab = 0;    //Video stabilization. n > 0 enabled [0]

    cml->constIntraPred = 0;   //0=OFF, 1=ON Constrained intra pred flag [0]
    cml->disableDeblocking = 0;   //0..2 Value of disable_deblocking_filter_idc [0]
    cml->intraPicRate = 30;//0;   //Intra picture rate. [0]  // modify 0 to 30 by lance 2016.05.11
    cml->mbPerSlice = 0;   //Slice size in macroblocks. Should be a multiple of MBs per row. [0]


    cml->trans8x8 = 0;          //0=OFF, 1=Adaptive, 2=ON [0]\n"
    cml->enableCabac = 0;       //0=OFF, 1=ON [0]\n"
    cml->cabacInitIdc = 0;      //0..2\n");


    cml->bitPerSecond  = 100000;//64000;         //Bitrate, 10000..levelMax [64000]\n"    // modify 64000 to 100000 by lance 2016.05.11
    cml->picRc = 1;                      //0=OFF, 1=ON Picture rate control. [1]\n"
    cml->mbRc = 1;                       //0=OFF, 1=ON Mb rc (Check point rc). [1]\n"
    cml->hrdConformance =  0 ;        //0=OFF, 1=ON HRD conformance. [0]\n"
    cml->cpbSize       =   30000000;//0;       //HRD Coded Picture Buffer size in bits. [0]\n"    // modify 0 to 30000000 by lance 2016.05.11
    cml->gopLength     =   cml->intraPicRate;//1;       //GOP length, 1..150 [intraPicRate]\n");  // modify 1 to cml->intraPicRate by lance 2016.05.11


    cml->qpHdr = 26;
    cml->qpMin = 18;//10;    //  modify 10 to 18 by lance 2016.05.11
    cml->qpMax = 51;
    cml->intraQpDelta = 4;//-3; //Intra QP delta  // modify -3 to 4 by lance 2016.05.11
    cml->fixedIntraQp = 0;    //0..51, Fixed Intra QP, 0 = disabled. [0]
    cml->mbQpAdjustment = 0;    //-8..7, MAD based MB QP adjustment, 0 = disabled

    cml->userData  = NULL;        //SEI User data file name.\n"
//    cml->bpsAdjust = 0;        // Frame:bitrate for adjusting bitrate on the fly.\n"
    cml->psnr      = 0;        //Enables PSNR calculation for each frame.\n");


    cml->chromaQpOffset = 2;
    cml->filterOffsetA = 0;
    cml->filterOffsetB = 0;
    cml->burst = 16;        //burstSize
    cml->bursttype = 0;     //0=SIGLE, 1=INCR HW bus burst type. [0]
    cml->quarterPixelMv = 1;//0;    // modify 0 to 1 by lance 2016.05.11
    cml->testId = 0;
    trigger_point = -1;      //Logic Analyzer trigger at picture <n>. [-1]

    cml->picSkip = 0;
    testParam = cml->testParam = 0;

    return 0;
}

/*------------------------------------------------------------------------------

    ReadPic

    Read raw YUV 4:2:0 or 4:2:2 image data from file

    Params:
        image   - buffer where the image will be saved
        size    - amount of image data to be read
        nro     - picture number to be read
        name    - name of the file to read
        width   - image width in pixels
        height  - image height in pixels
        format  - image format (YUV 420/422/RGB16/RGB32)

    Returns:
        0 - for success
        non-zero error code
------------------------------------------------------------------------------*/
int ReadPic(u8 * image, i32 size, i32 nro, char *name, i32 width, i32 height,
            i32 format)
{
    int ret = 0;

    if (yuvFile == NULL) {
        yuvFile = fopen(name, "rb");

        if (yuvFile == NULL) {
            mpp_log( "\nUnable to open YUV file: %s\n", name);
            ret = -1;
            goto end;
        }

        fseek(yuvFile, 0, SEEK_END);
        file_size = ftell(yuvFile);
    }

    /* Stop if over last frame of the file */
    if ((u32)size * (nro + 1) > file_size) {
        mpp_log("\nCan't read frame, EOF\n");
        ret = -1;
        goto end;
    }

    if (fseek(yuvFile, (u32)size * nro, SEEK_SET) != 0) {
        mpp_log( "\nI can't seek frame no: %i from file: %s\n",
                 nro, name);
        ret = -1;
        goto end;
    }

    if ((width & 0x0f) == 0)
        fread(image, 1, size, yuvFile);
    else {
        i32 iCount;
        u8 *buf = image;
        i32 scan = (width + 15) & (~0x0f);

        /* Read the frame so that scan (=stride) is multiple of 16 pixels */

        if (format == 0) { /* YUV 4:2:0 planar */
            /* Y */
            for (iCount = 0; iCount < height; iCount++) {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
            /* Cb */
            for (iCount = 0; iCount < (height / 2); iCount++) {
                fread(buf, 1, width / 2, yuvFile);
                buf += scan / 2;
            }
            /* Cr */
            for (iCount = 0; iCount < (height / 2); iCount++) {
                fread(buf, 1, width / 2, yuvFile);
                buf += scan / 2;
            }
        } else if (format == 1) { /* YUV 4:2:0 semiplanar */
            /* Y */
            for (iCount = 0; iCount < height; iCount++) {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
            /* CbCr */
            for (iCount = 0; iCount < (height / 2); iCount++) {
                fread(buf, 1, width, yuvFile);
                buf += scan;
            }
        } else if (format <= 9) { /* YUV 4:2:2 interleaved or 16-bit RGB */
            for (iCount = 0; iCount < height; iCount++) {
                fread(buf, 1, width * 2, yuvFile);
                buf += scan * 2;
            }
        } else { /* 32-bit RGB */
            for (iCount = 0; iCount < height; iCount++) {
                fread(buf, 1, width * 4, yuvFile);
                buf += scan * 4;
            }
        }

    }

end:

    return ret;
}

/*------------------------------------------------------------------------------

    ReadUserData
        Read user data from file and pass to encoder

    Params:
        name - name of file in which user data is located

    Returns:
        NULL - when user data reading failed
        pointer - allocated buffer containing user data

------------------------------------------------------------------------------*/
u8* ReadUserData(H264EncInst encoderForRead, char *name)
{
    FILE *file = NULL;
    i32 byteCnt;
    u8 *data;

    if (name == NULL) {
        return NULL;
    }

    /* Get user data length from file */
    file = fopen(name, "rb");
    if (file == NULL) {
        mpp_log( "Unable to open User Data file: %s\n", name);
        return NULL;
    }
    fseek(file, 0L, SEEK_END);
    byteCnt = ftell(file);
    rewind(file);

    /* Minimum size of user data */
    if (byteCnt < 16)
        byteCnt = 16;

    /* Maximum size of user data */
    if (byteCnt > 2048)
        byteCnt = 2048;

    /* Allocate memory for user data */
    if ((data = (u8 *) malloc(sizeof(u8) * byteCnt)) == NULL) {
        fclose(file);
        mpp_log( "Unable to alloc User Data memory\n");
        return NULL;
    }

    /* Read user data from file */
    fread(data, sizeof(u8), byteCnt, file);
    fclose(file);

    mpp_log("User data: %d bytes [%d %d %d %d ...]\n",
            byteCnt, data[0], data[1], data[2], data[3]);

    /* Pass the data buffer to encoder
     * The encoder reads the buffer during following H264EncStrmEncode() calls.
     * User data writing must be disabled (with SetSeiUserData(enc, 0, 0)) */
    H264EncSetSeiUserData(encoderForRead, data, byteCnt);

    return data;
}

/*------------------------------------------------------------------------------

    Help
        Print out some instructions about usage.
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
void Help(void)
{
    mpp_log( "Usage:  %s [options] -i inputfile\n\n", "h264_testenc");

    mpp_log(
        "  -i[s] --input             Read input from file. [input.yuv]\n"
        "  -o[s] --output            Write output to file. [stream.h264]\n"
        "  -a[n] --firstPic          First picture of input file. [0]\n"
        "  -b[n] --lastPic           Last picture of input file. [100]\n"
        "  -w[n] --lumWidthSrc       Width of source image. [176]\n"
        "  -h[n] --lumHeightSrc      Height of source image. [144]\n");
    mpp_log(
        "  -x[n] --width             Width of output image. [--lumWidthSrc]\n"
        "  -y[n] --height            Height of output image. [--lumHeightSrc]\n"
        "  -X[n] --horOffsetSrc      Output image horizontal offset. [0]\n"
        "  -Y[n] --verOffsetSrc      Output image vertical offset. [0]\n");
    mpp_log(
        "  -f[n] --outputRateNumer   1..65535 Output picture rate numerator. [30]\n"
        "  -F[n] --outputRateDenom   1..65535 Output picture rate denominator. [1]\n"
        "  -j[n] --inputRateNumer    1..65535 Input picture rate numerator. [30]\n"
        "  -J[n] --inputRateDenom    1..65535 Input picture rate denominator. [1]\n");

    mpp_log( "\n  -L[n] --level             10..40, H264 Level. [30]\n");
    mpp_log( "\n  -R[n] --byteStream        Stream type. [1]\n"
             "                            1 - byte stream according to Annex B.\n"
             "                            0 - NAL units. Nal sizes returned in <nal_sizes.txt>\n");
    mpp_log(
        "\n  -S[n] --sei               Enable/Disable SEI messages.[0]\n");

    mpp_log(
        "\n  -r[n] --rotation          Rotate input image. [0]\n"
        "                                0 - disabled\n"
        "                                1 - 90 degrees right\n"
        "                                2 - 90 degrees right\n"
        "  -l[n] --inputFormat       Input YUV format. [0]\n"
        "                                0 - YUV420\n"
        "                                1 - YUV420 semiplanar\n"
        "                                2 - YUYV422\n"
        "                                3 - UYVY422\n"
        "                                4 - RGB565\n"
        "                                5 - BGR565\n"
        "                                6 - RGB555\n"
        "                                7 - BGR555\n"
        "                                8 - RGB444\n"
        "                                9 - BGR444\n"
        "                                10 - RGB888\n"
        "                                11 - BGR888\n"
        "                                12 - RGB101010\n"
        "                                13 - BGR101010\n"
        "  -O[n] --colorConversion   RGB to YCbCr color conversion type. [0]\n"
        "                                0 - BT.601\n"
        "                                1 - BT.709\n"
        "                                2 - User defined\n"
        "  -k[n] --videoRange        0..1 Video range. [0]\n\n");

    mpp_log(
        "  -Z[n] --videoStab         Video stabilization. n > 0 enabled [0]\n\n");

    mpp_log(
        "  -T[n] --constIntraPred    0=OFF, 1=ON Constrained intra pred flag [0]\n"
        "  -D[n] --disableDeblocking 0..2 Value of disable_deblocking_filter_idc [0]\n"
        "  -I[n] --intraPicRate      Intra picture rate. [0]\n"
        "  -V[n] --mbPerSlice        Slice size in macroblocks. Should be a\n"
        "                            multiple of MBs per row. [0]\n\n");
    mpp_log(
        "  -8[n] --trans8x8          0=OFF, 1=Adaptive, 2=ON [0]\n"
        "  -K[n] --enableCabac       0=OFF, 1=ON [0]\n"
        "  -p[n] --cabacInitIdc      0..2\n");
    mpp_log(
        "\n  -B[n] --bitPerSecond      Bitrate, 10000..levelMax [64000]\n"
        "  -U[n] --picRc             0=OFF, 1=ON Picture rate control. [1]\n"
        "  -u[n] --mbRc              0=OFF, 1=ON Mb rc (Check point rc). [1]\n"
        "  -C[n] --hrdConformance    0=OFF, 1=ON HRD conformance. [0]\n"
        "  -c[n] --cpbSize           HRD Coded Picture Buffer size in bits. [0]\n"
        "  -g[n] --gopLength         GOP length, 1..150 [intraPicRate]\n");

    mpp_log(
        "\n  -s[n] --picSkip           0=OFF, 1=ON Picture skip rate control. [0]\n"
        "  -q[n] --qpHdr             -1..51, Default frame header qp. [26]\n"
        "                             -1=Encoder calculates initial QP\n"
        "  -n[n] --qpMin             0..51, Minimum frame header qp. [10]\n"
        "  -m[n] --qpMax             0..51, Maximum frame header qp. [51]\n"
        "  -A[n] --intraQpDelta      -12..12, Intra QP delta. [-3]\n"
        "  -G[n] --fixedIntraQp      0..51, Fixed Intra QP, 0 = disabled. [0]\n"
        "  -2[n] --mbQpAdjustment    -8..7, MAD based MB QP adjustment, 0 = disabled. [0]\n");

    mpp_log(
        "\n  -z[n] --userData          SEI User data file name.\n"
        "  -1[n]:[n] --bpsAdjust     Frame:bitrate for adjusting bitrate on the fly.\n"
        "  -d    --psnr              Enables PSNR calculation for each frame.\n");

    mpp_log(
        "\nTesting parameters that are not supported for end-user:\n"
        "  -Q[n] --chromaQpOffset    -12..12 Chroma QP offset. [2]\n"
        "  -W[n] --filterOffsetA     -6..6 Deblockiong filter offset A. [0]\n"
        "  -E[n] --filterOffsetB     -6..6 Deblockiong filter offset B. [0]\n"
        "  -N[n] --burstSize          0..63 HW bus burst size. [16]\n"
        "  -t[n] --burstType          0=SIGLE, 1=INCR HW bus burst type. [0]\n"
        "  -M[n] --quarterPixelMv     0=Disable, 1=Enable.\n"
        "  -e[n] --testId            Internal test ID. [0]\n"
        "  -P[n] --trigger           Logic Analyzer trigger at picture <n>. [-1]\n"
        "\n");
    ;
}
#endif

/*------------------------------------------------------------------------------

    WriteStrm
        Write encoded stream to file

    Params:
        fout    - file to write
        strbuf  - data to be written
        size    - amount of data to write
        endian  - data endianess, big or little

------------------------------------------------------------------------------*/
void WriteStrm(FILE * outFile, u32 * strmbuf, u32 size, u32 endian)
{

#ifdef NO_OUTPUT_WRITE
    return;
#endif

    /* Swap the stream endianess before writing to file if needed */
    if (endian == 1) {
        u32 iCount = 0, words = (size + 3) / 4;

        while (words) {
            u32 val = strmbuf[iCount];
            u32 tmp = 0;

            tmp |= (val & 0xFF) << 24;
            tmp |= (val & 0xFF00) << 8;
            tmp |= (val & 0xFF0000) >> 8;
            tmp |= (val & 0xFF000000) >> 24;
            strmbuf[iCount] = tmp;
            words--;
            iCount++;
        }

    }

    /* Write the stream to file */
    fwrite(strmbuf, 1, size, outFile);
}

/*------------------------------------------------------------------------------

    NextPic

    Function calculates next input picture depending input and output frame
    rates.

    Input   inputRateNumer  (input.yuv) frame rate numerator.
            inputRateDenom  (input.yuv) frame rate denominator
            outputRateNumer (stream.mpeg4) frame rate numerator.
            outputRateDenom (stream.mpeg4) frame rate denominator.
            frameCount        Frame counter.
            firstPic        The first picture of input.yuv sequence.

    Return  next    The next picture of input.yuv sequence.

------------------------------------------------------------------------------*/
i32 NextPic(i32 inputRateNumer, i32 inputRateDenom, i32 outputRateNumer,
            i32 outputRateDenom, i32 frameCount, i32 firstPic)
{
    u32 sift;
    u32 skip;
    u32 numer;
    u32 denom;
    u32 nextP;

    numer = (u32) inputRateNumer * (u32) outputRateDenom;
    denom = (u32) inputRateDenom * (u32) outputRateNumer;

    if (numer >= denom) {
        sift = 9;
        do {
            sift--;
        } while (((numer << sift) >> sift) != numer);
    } else {
        sift = 17;
        do {
            sift--;
        } while (((numer << sift) >> sift) != numer);
    }
    skip = (numer << sift) / denom;
    nextP = (((u32) frameCount * skip) >> sift) + (u32) firstPic;

    return (i32) nextP;
}

/*------------------------------------------------------------------------------

    ChangeInput
        Change input file.
    Params:
        argc    - number of arguments to the application
        argv    - argument list as provided to the application
        optionChangeInput  - list of accepted cmdline options

    Returns:
        0 - for success
------------------------------------------------------------------------------*/
// mask by lance 2016.05.12
#if 0
int ChangeInput(i32 argc, char **argv, char **name, option_s * optionChangeInput)
{
    i32 ret;
    argument_s argument;
    i32 enable = 0;

    argument.optCnt = 1;
    while ((ret = EncGetOption(argc, argv, optionChangeInput, &argument)) != -1) {
        if ((ret == 1) && (enable)) {
            *name = argument.optArg;
            mpp_log("\nNext file: %s\n", *name);
            return 1;
        }
        if (argument.optArg == *name) {
            enable = 1;
        }
    }

    return 0;
}
#endif

/*------------------------------------------------------------------------------

    API tracing
        TRacing as defined by the API.
    Params:
        msg - null terminated tracing message
------------------------------------------------------------------------------*/
// mask by lance 2016.05.06
/*void H264EncTrace(const char *msg)
{
    static FILE *fp = NULL;

    if (fp == NULL)
        fp = fopen("api.trc", "wt");

    if (fp)
        fprintf(fp, "%s\n", msg);
}*/

/*------------------------------------------------------------------------------
    Get out pure NAL units from byte stream format (one picture data)
    This is an example!

    Params:
        pNaluSizeBuf - buffer where the individual NAL size are (return by API)
        pStream- buffre containign  the whole picture data
------------------------------------------------------------------------------*/
void NalUnitsFromByteStream(const u32 * pNaluSizeBuf, const u8 * pStream)
{
    u32 /*nalSize, */nal;  // mask by lance 2016.05.12
    const u8 *pNalBase;

    nal = 0;
    pNalBase = pStream + 4; /* skip the 4-byte startcode */

    while (pNaluSizeBuf[nal] != 0) { /* after the last NAL unit size we have a zero */
        //nalSize = pNaluSizeBuf[nal] - 4;  // mask by lance 2016.05.12

        /* now we have the pure NAL unit, do something with it */
        /* DoSomethingWithThisNAL(pNalBase, nalSize); */

        pNalBase += pNaluSizeBuf[nal];  /* next NAL data base address */
        nal++;  /* next NAL unit */
    }
}

/*------------------------------------------------------------------------------

------------------------------------------------------------------------------*/
void PrintNalSizes(const u32 *pNaluSizeBuf/*, const u8 *pOutBuf, u32 strmSize,*/  // mask by lance 2016.05.12
                   /*i32 byteStream*/)  // mask by lance 2016.05.12
{
    u32 nalu = 0, naluSum = 0;

    /* Step through the NALU size buffer */
    while (pNaluSizeBuf && pNaluSizeBuf[nalu] != 0) {
#ifdef INTERNAL_TEST
        /* In byte stream format each NAL must start with
         * start code: any number of leading zeros + 0x000001 */
        if (byteStream) {
            int zero_count = 0;
            const u8 *pTmp = pOutBuf + naluSum;

            /* count zeros, shall be at least 2 */
            while (*pTmp++ == 0x00)
                zero_count++;

            if (zero_count < 2 || pTmp[-1] != 0x01)
                mpp_log
                ("Error: NAL unit %d at %d doesn't start with '00 00 01'  ",
                 nalu, naluSum);
        }
#endif

        naluSum += pNaluSizeBuf[nalu];
        mpp_log("%d  ", pNaluSizeBuf[nalu++]);
    }

#ifdef INTERNAL_TEST
    /* NAL sum must be the same as the whole frame stream size */
    if (naluSum && naluSum != strmSize)
        mpp_log("Error: Sum of NAL size (%d) does not match stream size\n",
                naluSum);
#endif
}

/*------------------------------------------------------------------------------
    WriteNalSizesToFile
        Dump NAL size to a file

    Params:
        file         - file name where toi dump
        pNaluSizeBuf - buffer where the individual NAL size are (return by API)
        buffSize     - size in bytes of the above buffer
------------------------------------------------------------------------------*/
void WriteNalSizesToFile(const char *file, const u32 * pNaluSizeBuf,
                         u32 buffSize)
{
    FILE *fo;
    u32 offset = 0;

    fo = fopen(file, "at");

    if (fo == NULL) {
        mpp_log("FAILED to open NAL size tracing file <%s>\n", file);
        return;
    }

    while (offset < buffSize && *pNaluSizeBuf != 0) {
        //mpp_log(fo, "%d\n", *pNaluSizeBuf++);  // mask by lance 2016.05.12
        offset += sizeof(u32);
    }

    fclose(fo);
}

/*------------------------------------------------------------------------------
    PrintErrorValue
        Print return error value
------------------------------------------------------------------------------*/
void PrintErrorValue(const char *errorDesc, u32 retVal)
{
    char * str;

    switch (retVal) {
    case H264ENC_ERROR: str = "H264ENC_ERROR"; break;
    case H264ENC_NULL_ARGUMENT: str = "H264ENC_NULL_ARGUMENT"; break;
    case H264ENC_INVALID_ARGUMENT: str = "H264ENC_INVALID_ARGUMENT"; break;
    case H264ENC_MEMORY_ERROR: str = "H264ENC_MEMORY_ERROR"; break;
    case H264ENC_EWL_ERROR: str = "H264ENC_EWL_ERROR"; break;
    case H264ENC_EWL_MEMORY_ERROR: str = "H264ENC_EWL_MEMORY_ERROR"; break;
    case H264ENC_INVALID_STATUS: str = "H264ENC_INVALID_STATUS"; break;
    case H264ENC_OUTPUT_BUFFER_OVERFLOW: str = "H264ENC_OUTPUT_BUFFER_OVERFLOW"; break;
    case H264ENC_HW_BUS_ERROR: str = "H264ENC_HW_BUS_ERROR"; break;
    case H264ENC_HW_DATA_ERROR: str = "H264ENC_HW_DATA_ERROR"; break;
    case H264ENC_HW_TIMEOUT: str = "H264ENC_HW_TIMEOUT"; break;
    case H264ENC_HW_RESERVED: str = "H264ENC_HW_RESERVED"; break;
    case H264ENC_SYSTEM_ERROR: str = "H264ENC_SYSTEM_ERROR"; break;
    case H264ENC_INSTANCE_ERROR: str = "H264ENC_INSTANCE_ERROR"; break;
    case H264ENC_HRD_ERROR: str = "H264ENC_HRD_ERROR"; break;
    case H264ENC_HW_RESET: str = "H264ENC_HW_RESET"; break;
    default: str = "UNDEFINED";
    }

    mpp_log( "%s Return value: %s\n", errorDesc, str);
}

/*------------------------------------------------------------------------------
    PrintPSNR
        Calculate and print frame PSNR
------------------------------------------------------------------------------*/
u32 PrintPSNR(/*u8 *a, u8 *b, i32 scanline, i32 wdh, i32 hgt*/)  // mask by lance 2016.05.12
{
#ifdef PSNR
    float mse = 0.0;
    u32 tmp, i, j;

    for (j = 0 ; j < hgt; j++) {
        for (i = 0; i < wdh; i++) {
            tmp = a[i] - b[i];
            tmp *= tmp;
            mse += tmp;
        }
        a += scanline;
        b += wdh;
    }
    mse /= wdh * hgt;

    if (mse == 0.0) {
        mpp_log("--.-- | ");
    } else {
        mse = 10.0 * log10f(65025.0 / mse);
        mpp_log("%5.2f | ", mse);
    }

    return (u32)roundf(mse * 100);
#else
    mpp_log("xx.xx | ");
    return 0;
#endif
}
