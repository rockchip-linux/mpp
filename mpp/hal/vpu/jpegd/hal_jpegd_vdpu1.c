/*
 *
 * Copyright 2017 Rockchip Electronics Co. LTD
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
#define MODULE_TAG "HAL_JPEGD_VDPU1"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mpp_buffer.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"
#include "vpu.h"
#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_bitput.h"

#include "jpegd_api.h"
#include "jpegd_syntax.h"
#include "hal_jpegd_common.h"
#include "hal_jpegd_vdpu1.h"
#include "hal_jpegd_vdpu1_reg.h"

static void
jpegd_write_len_bits(JpegSyntaxParam *pSyntax, JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    ScanInfo         *JPG_SCN = &pSyntax->scan;
    HuffmanTables    *JPG_VLC = &pSyntax->vlc;
    JpegRegSet *reg = pCtx->regs;

    VlcTable *pTable1 = NULL;
    VlcTable *pTable2 = NULL;

    /* first select the table we'll use */

    /* this trick is done because hw always wants luma table as ac hw table 1 */
    if (JPG_SCN->Ta[0] == 0) {
        pTable1 = &(JPG_VLC->acTable0);
        pTable2 = &(JPG_VLC->acTable1);
    } else {
        pTable1 = &(JPG_VLC->acTable1);
        pTable2 = &(JPG_VLC->acTable0);
    }

    JPEGD_ASSERT(pTable1);
    JPEGD_ASSERT(pTable2);

    /* write AC table 1 (luma) */
    reg->reg16.sw_ac1_code1_cnt = pTable1->bits[0];
    reg->reg16.sw_ac1_code2_cnt = pTable1->bits[1];
    reg->reg16.sw_ac1_code3_cnt = pTable1->bits[2];
    reg->reg16.sw_ac1_code4_cnt = pTable1->bits[3];
    reg->reg16.sw_ac1_code5_cnt = pTable1->bits[4];
    reg->reg16.sw_ac1_code6_cnt = pTable1->bits[5];

    reg->reg17.sw_ac1_code7_cnt = pTable1->bits[6];
    reg->reg17.sw_ac1_code8_cnt = pTable1->bits[7];
    reg->reg17.sw_ac1_code9_cnt = pTable1->bits[8];
    reg->reg17.sw_ac1_code10_cnt = pTable1->bits[9];

    reg->reg18.sw_ac1_code11_cnt = pTable1->bits[10];
    reg->reg18.sw_ac1_code12_cnt = pTable1->bits[11];
    reg->reg18.sw_ac1_code13_cnt = pTable1->bits[12];
    reg->reg18.sw_ac1_code14_cnt = pTable1->bits[13];

    reg->reg19.sw_ac1_code15_cnt = pTable1->bits[14];
    reg->reg19.sw_ac1_code16_cnt = pTable1->bits[15];

    /* table AC2 (the not-luma table) */
    reg->reg19.sw_ac2_code1_cnt = pTable2->bits[0];
    reg->reg19.sw_ac2_code2_cnt = pTable2->bits[1];
    reg->reg19.sw_ac2_code3_cnt = pTable2->bits[2];
    reg->reg19.sw_ac2_code4_cnt = pTable2->bits[3];

    reg->reg20.sw_ac2_code5_cnt = pTable2->bits[4];
    reg->reg20.sw_ac2_code6_cnt = pTable2->bits[5];
    reg->reg20.sw_ac2_code7_cnt = pTable2->bits[6];
    reg->reg20.sw_ac2_code8_cnt = pTable2->bits[7];

    reg->reg21.sw_ac2_code9_cnt = pTable2->bits[8];
    reg->reg21.sw_ac2_code10_cnt = pTable2->bits[9];
    reg->reg21.sw_ac2_code11_cnt = pTable2->bits[10];
    reg->reg21.sw_ac2_code12_cnt = pTable2->bits[11];

    reg->reg22.sw_ac2_code13_cnt = pTable2->bits[12];
    reg->reg22.sw_ac2_code14_cnt = pTable2->bits[13];
    reg->reg22.sw_ac2_code15_cnt = pTable2->bits[14];
    reg->reg22.sw_ac2_code16_cnt = pTable2->bits[15];

    if (JPG_SCN->Td[0] == 0) {
        pTable1 = &(JPG_VLC->dcTable0);
        pTable2 = &(JPG_VLC->dcTable1);
    } else {
        pTable1 = &(JPG_VLC->dcTable1);
        pTable2 = &(JPG_VLC->dcTable0);
    }

    JPEGD_ASSERT(pTable1);
    JPEGD_ASSERT(pTable2);

    /* write DC table 1 (luma) */
    reg->reg23.sw_dc1_code1_cnt = pTable1->bits[0];
    reg->reg23.sw_dc1_code2_cnt = pTable1->bits[1];
    reg->reg23.sw_dc1_code3_cnt = pTable1->bits[2];
    reg->reg23.sw_dc1_code4_cnt = pTable1->bits[3];
    reg->reg23.sw_dc1_code5_cnt = pTable1->bits[4];
    reg->reg23.sw_dc1_code6_cnt = pTable1->bits[5];
    reg->reg23.sw_dc1_code7_cnt = pTable1->bits[6];
    reg->reg23.sw_dc1_code8_cnt = pTable1->bits[7];

    reg->reg24.sw_dc1_code9_cnt = pTable1->bits[8];
    reg->reg24.sw_dc1_code10_cnt = pTable1->bits[9];
    reg->reg24.sw_dc1_code11_cnt = pTable1->bits[10];
    reg->reg24.sw_dc1_code12_cnt = pTable1->bits[11];
    reg->reg24.sw_dc1_code13_cnt = pTable1->bits[12];
    reg->reg24.sw_dc1_code14_cnt = pTable1->bits[13];
    reg->reg24.sw_dc1_code15_cnt = pTable1->bits[14];
    reg->reg24.sw_dc1_code16_cnt = pTable1->bits[15];

    /* table DC2 (the not-luma table) */
    reg->reg25.sw_dc2_code1_cnt = pTable2->bits[0];
    reg->reg25.sw_dc2_code2_cnt = pTable2->bits[1];
    reg->reg25.sw_dc2_code3_cnt = pTable2->bits[2];
    reg->reg25.sw_dc2_code4_cnt = pTable2->bits[3];
    reg->reg25.sw_dc2_code5_cnt = pTable2->bits[4];
    reg->reg25.sw_dc2_code6_cnt = pTable2->bits[5];
    reg->reg25.sw_dc2_code7_cnt = pTable2->bits[6];
    reg->reg25.sw_dc2_code8_cnt = pTable2->bits[7];

    reg->reg26.sw_dc2_code9_cnt = pTable2->bits[8];
    reg->reg26.sw_dc2_code10_cnt = pTable2->bits[9];
    reg->reg26.sw_dc2_code11_cnt = pTable2->bits[10];
    reg->reg26.sw_dc2_code12_cnt = pTable2->bits[11];
    reg->reg26.sw_dc2_code13_cnt = pTable2->bits[12];
    reg->reg26.sw_dc2_code14_cnt = pTable2->bits[13];
    reg->reg26.sw_dc2_code15_cnt = pTable2->bits[14];
    reg->reg26.sw_dc2_code16_cnt = pTable2->bits[15];

    FUN_TEST("Exit");
    return;
}

static void jpegd_set_stream_params(JpegSyntaxParam *pSyntax,
                                    JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    StreamStorage *JPG_STR = &pSyntax->stream;
    JpegRegSet *reg = pCtx->regs;

    RK_U32 addrTmp = 0;
    RK_U32 amountOfStream = 0;

    JPEGD_INFO_LOG("read %d bits\n", JPG_STR->readBits);
    JPEGD_INFO_LOG("read %d bytes\n", JPG_STR->readBits / 8);
    JPEGD_INFO_LOG("Stream physical address start: 0x%08x\n",
                   JPG_STR->streamBus);
    JPEGD_INFO_LOG("Stream virtual address start: %p\n",
                   JPG_STR->pStartOfStream);

    /* calculate and set stream start address to hw */
    addrTmp = (((uintptr_t) JPG_STR->pStartOfStream & 0x3) +
               (RK_U32)(JPG_STR->pCurrPos - JPG_STR->pStartOfStream)) & (~7);

    JPEGD_INFO_LOG("pStartOfStream data: 0x%x, 0x%x, 0x%x, 0x%x",
                   JPG_STR->pStartOfStream[JPG_STR->streamLength - 4],
                   JPG_STR->pStartOfStream[JPG_STR->streamLength - 3],
                   JPG_STR->pStartOfStream[JPG_STR->streamLength - 2],
                   JPG_STR->pStartOfStream[JPG_STR->streamLength - 1]);

    if (VPUClientGetIOMMUStatus() <= 0) {
        reg->reg12_input_stream_base = JPG_STR->streamBus + addrTmp;
    } else {
        reg->reg12_input_stream_base = JPG_STR->streamBus | (addrTmp << 10);
    }
    JPEGD_INFO_LOG("reg12_rlc_vlc_base: 0x%08x\n",
                   reg->reg12_input_stream_base);

    /* calculate and set stream start bit to hw */

    /* change current pos to bus address style */
    /* remove three lowest bits and add the difference to bitPosInWord */
    /* used as bit pos in word not as bit pos in byte actually... */
    switch ((uintptr_t) JPG_STR->pCurrPos & (7)) {
    case 0:
        break;
    case 1:
        JPG_STR->bitPosInByte += 8;
        break;
    case 2:
        JPG_STR->bitPosInByte += 16;
        break;
    case 3:
        JPG_STR->bitPosInByte += 24;
        break;
    case 4:
        JPG_STR->bitPosInByte += 32;
        break;
    case 5:
        JPG_STR->bitPosInByte += 40;
        break;
    case 6:
        JPG_STR->bitPosInByte += 48;
        break;
    case 7:
        JPG_STR->bitPosInByte += 56;
        break;
    default:
        JPEGD_ASSERT(0);
        break;
    }

    reg->reg5.sw_strm0_start_bit = JPG_STR->bitPosInByte;

    /* set up stream length for HW.
     * length = size of original buffer - stream we already decoded in SW */
    JPG_STR->pCurrPos = (RK_U8 *) ((uintptr_t) JPG_STR->pCurrPos & (~7));

    if (pSyntax->info.inputStreaming) {
        JPEGD_VERBOSE_LOG("inputStreaming-1, inputBufferLen:%d",
                          pSyntax->info.inputBufferLen);
        amountOfStream = (pSyntax->info.inputBufferLen -
                          (RK_U32) (JPG_STR->pCurrPos - JPG_STR->pStartOfStream));

        reg->reg6_stream_info.sw_stream_len = amountOfStream;
        pSyntax->info.streamEnd = 1;
    } else {
        JPEGD_VERBOSE_LOG("inputStreaming-2");
        amountOfStream = (JPG_STR->streamLength -
                          (RK_U32) (JPG_STR->pCurrPos
                                    - JPG_STR->pStartOfStream));

        reg->reg6_stream_info.sw_stream_len = amountOfStream;

        /* because no input streaming, frame should be ready during decoding this buffer */
        pSyntax->info.streamEnd = 1;
    }

    reg->reg5.sw_jpeg_stream_all = pSyntax->info.streamEnd;

    JPEGD_INFO_LOG("streamLength: %d\n", JPG_STR->streamLength);
    JPEGD_INFO_LOG("pCurrPos: %p\n", JPG_STR->pCurrPos);
    JPEGD_INFO_LOG("pStartOfStream: %p\n", JPG_STR->pStartOfStream);
    JPEGD_INFO_LOG("bitPosInByte: 0x%08x\n", JPG_STR->bitPosInByte);

    FUN_TEST("Exit");
    return;
}

static void jpegd_select_chroma_table(JpegSyntaxParam *pSyntax,
                                      JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    ScanInfo         *JPG_SCN = &pSyntax->scan;
    JpegRegSet *reg = pCtx->regs;

    /* this trick is done because hw always wants luma table as ac hw table 1 */
    if (JPG_SCN->Ta[0] == 0) {
        reg->reg5.sw_cr_ac_vlctable = JPG_SCN->Ta[2];
        reg->reg5.sw_cb_ac_vlctable = JPG_SCN->Ta[1];
    } else {
        if (JPG_SCN->Ta[0] == JPG_SCN->Ta[1])
            reg->reg5.sw_cb_ac_vlctable = 0;
        else
            reg->reg5.sw_cb_ac_vlctable = 1;

        if (JPG_SCN->Ta[0] == JPG_SCN->Ta[2])
            reg->reg5.sw_cr_ac_vlctable = 0;
        else
            reg->reg5.sw_cr_ac_vlctable = 1;
    }

    /* Third DC table selectors */
    if (pSyntax->info.operationType != JPEGDEC_PROGRESSIVE) {
        JPEGD_INFO_LOG("NON_PROGRESSIVE");
        if (JPG_SCN->Td[0] == 0) {
            reg->reg5.sw_cr_dc_vlctable = JPG_SCN->Td[2];
            reg->reg5.sw_cb_dc_vlctable = JPG_SCN->Td[1];
        } else {
            if (JPG_SCN->Td[0] == JPG_SCN->Td[1])
                reg->reg5.sw_cb_dc_vlctable = 0;
            else
                reg->reg5.sw_cb_dc_vlctable = 1;

            if (JPG_SCN->Td[0] == JPG_SCN->Td[2])
                reg->reg5.sw_cr_dc_vlctable = 0;
            else
                reg->reg5.sw_cr_dc_vlctable = 1;
        }

        reg->reg5.sw_cr_dc_vlctable3 = 0;
        reg->reg5.sw_cb_dc_vlctable3 = 0;
    } else {
        JPEGD_INFO_LOG("JPEGDEC_PROGRESSIVE");
    }

    FUN_TEST("Exit");
    return;
}

#define VIDEORANGE 1

static MPP_RET
jpegd_set_post_processor(JpegHalContext *pCtx, JpegSyntaxParam *pSyntax)
{
    FUN_TEST("Enter");
    if (NULL == pCtx || NULL == pSyntax) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    PostProcessInfo *ppInfo = &(pSyntax->ppInfo);
    JpegRegSet *regs = pCtx->regs;
    post_processor_reg *post = &regs->post;

    int inColor = pSyntax->ppInputFomart;
    int outColor = ppInfo->outFomart;
    int dither = ppInfo->shouldDither;
    int inWidth = pSyntax->imageInfo.outputWidth;
    int inHeigth = pSyntax->imageInfo.outputHeight;
    int outWidth = pSyntax->ppScaleW;
    int outHeight = pSyntax->ppScaleH;

    int videoRange = VIDEORANGE;

    post->reg61_dev_conf.sw_pp_axi_rd_id = 0;
    post->reg61_dev_conf.sw_pp_axi_wr_id = 0;
    post->reg61_dev_conf.sw_pp_scmd_dis = 1;
    post->reg61_dev_conf.sw_pp_max_burst = 16;
    post->reg61_dev_conf.sw_pp_in_a2_endsel = 1;
    post->reg61_dev_conf.sw_pp_in_a1_swap32 = 1;
    post->reg61_dev_conf.sw_pp_in_a1_endian = 1;
    post->reg61_dev_conf.sw_pp_in_swap32_e = 1;
    post->reg61_dev_conf.sw_pp_in_endian = 1;
    post->reg61_dev_conf.sw_pp_out_endian = 1;
    post->reg61_dev_conf.sw_pp_out_swap32_e = 1;

    post->reg63_pp_in_lu_base = 0;

    post->reg88_mask_1_size.sw_ext_orig_width = inWidth >> 4;

    post->reg61_dev_conf.sw_pp_clk_gate_e = 1;
    post->reg61_dev_conf.sw_pp_ahb_hlock_e = 1;
    post->reg61_dev_conf.sw_pp_data_disc_e = 1;

    if (ppInfo->cropW <= 0) {
        post->reg92_display.sw_pp_in_w_ext = (((inWidth / 16) & 0xE00) >> 9);
        post->reg72_crop.sw_pp_in_width = ((inWidth / 16) & 0x1FF);
        post->reg92_display.sw_pp_in_h_ext = (((inHeigth / 16) & 0x700) >> 8);
        post->reg72_crop.sw_pp_in_height = ((inHeigth / 16) & 0x0FF);
    } else {
        post->reg92_display.sw_pp_in_w_ext =
            (((ppInfo->cropW / 16) & 0xE00) >> 9);
        post->reg72_crop.sw_pp_in_width = ((ppInfo->cropW / 16) & 0x1FF);
        post->reg92_display.sw_pp_in_h_ext =
            (((ppInfo->cropH / 16) & 0x700) >> 8);
        post->reg72_crop.sw_pp_in_height = ((ppInfo->cropH / 16) & 0x0FF);

        post->reg92_display.sw_crop_startx_ext =
            (((ppInfo->cropX / 16) & 0xE00) >> 9);
        post->reg71_color_coeff_1.sw_crop_startx =
            ((ppInfo->cropX / 16) & 0x1FF);
        post->reg92_display.sw_crop_starty_ext =
            (((ppInfo->cropY / 16) & 0x700) >> 8);
        post->reg72_crop.sw_crop_starty =
            ((ppInfo->cropY / 16) & 0x0FF);

        if (ppInfo->cropW & 0x0F) {
            post->reg85_ctrl.sw_pp_crop8_r_e = 1;
        } else {
            post->reg85_ctrl.sw_pp_crop8_r_e = 0;
        }
        if (ppInfo->cropH & 0x0F) {
            post->reg85_ctrl.sw_pp_crop8_d_e = 1;
        } else {
            post->reg85_ctrl.sw_pp_crop8_d_e = 0;
        }
        inWidth = ppInfo->cropW;
        inHeigth = ppInfo->cropH;
    }

    post->reg92_display.sw_display_width = outWidth;
    post->reg85_ctrl.sw_pp_out_width = outWidth;
    post->reg85_ctrl.sw_pp_out_height = outHeight;
    post->reg66_pp_out_lu_base = pSyntax->asicBuff.outLumaBuffer.phy_addr;

    switch (inColor) {
    case    PP_IN_FORMAT_YUV422INTERLAVE:
        post->reg85_ctrl.sw_pp_in_format = 0;
        break;
    case    PP_IN_FORMAT_YUV420SEMI:
        post->reg85_ctrl.sw_pp_in_format = 1;
        break;
    case    PP_IN_FORMAT_YUV420PLANAR:
        post->reg85_ctrl.sw_pp_in_format = 2;
        break;
    case    PP_IN_FORMAT_YUV400:
        post->reg85_ctrl.sw_pp_in_format = 3;
        break;
    case    PP_IN_FORMAT_YUV422SEMI:
        post->reg85_ctrl.sw_pp_in_format = 4;
        break;
    case    PP_IN_FORMAT_YUV420SEMITIELED:
        post->reg85_ctrl.sw_pp_in_format = 5;
        break;
    case    PP_IN_FORMAT_YUV440SEMI:
        post->reg85_ctrl.sw_pp_in_format = 6;
        break;
    case    PP_IN_FORMAT_YUV444_SEMI:
        post->reg85_ctrl.sw_pp_in_format = 7;
        post->reg86_mask_1.sw_pp_in_format_es = 0;
        break;
    case    PP_IN_FORMAT_YUV411_SEMI:
        post->reg85_ctrl.sw_pp_in_format = 0;
        post->reg86_mask_1.sw_pp_in_format_es = 1;
        break;
    default:
        JPEGD_ERROR_LOG("unsupported format:%d", inColor);
        return -1;
    }

    post->reg72_crop.sw_rangemap_coef_y = 9;
    post->reg86_mask_1.sw_rangemap_coef_c = 9;
    /*  brightness */
    post->reg71_color_coeff_1.sw_color_coefff = BRIGHTNESS;

    if (outColor <= PP_OUT_FORMAT_ARGB) {
        /*Bt.601*/
        unsigned int    a = 298;
        unsigned int    b = 409;
        unsigned int    c = 208;
        unsigned int    d = 100;
        unsigned int    e = 516;

        /*Bt.709
        unsigned int    a = 298;
        unsigned int    b = 459;
        unsigned int    c = 137;
        unsigned int    d = 55;
        unsigned int    e = 544;*/

        int satur = 0, tmp;
        if (videoRange != 0) {
            /*Bt.601*/
            a = 256;
            b = 350;
            c = 179;
            d = 86;
            e = 443;
            /*Bt.709
            a = 256;
            b = 403;
            c = 120;
            d = 48;
            e = 475;*/

            post->reg79_scaling_0.sw_ycbcr_range = videoRange;
        }
        int contrast = CONTRAST;
        if (contrast != 0) {
            int thr1y, thr2y, off1, off2, thr1, thr2, a1, a2;
            if (videoRange == 0) {
                int tmp1, tmp2;
                /* Contrast */
                thr1 = (219 * (contrast + 128)) / 512;
                thr1y = (219 - 2 * thr1) / 2;
                thr2 = 219 - thr1;
                thr2y = 219 - thr1y;

                tmp1 = (thr1y * 256) / thr1;
                tmp2 = ((thr2y - thr1y) * 256) / (thr2 - thr1);
                off1 = ((thr1y - ((tmp2 * thr1) / 256)) * a) / 256;
                off2 = ((thr2y - ((tmp1 * thr2) / 256)) * a) / 256;

                tmp1 = (64 * (contrast + 128)) / 128;
                tmp2 = 256 * (128 - tmp1);
                a1 = (tmp2 + off2) / thr1;
                a2 = a1 + (256 * (off2 - 1)) / (thr2 - thr1);
            } else {
                /* Contrast */
                thr1 = (64 * (contrast + 128)) / 128;
                thr1y = 128 - thr1;
                thr2 = 256 - thr1;
                thr2y = 256 - thr1y;
                a1 = (thr1y * 256) / thr1;
                a2 = ((thr2y - thr1y) * 256) / (thr2 - thr1);
                off1 = thr1y - (a2 * thr1) / 256;
                off2 = thr2y - (a1 * thr2) / 256;
            }

            if (a1 > 1023)
                a1 = 1023;
            else if (a1 < 0)
                a1 = 0;

            if (a2 > 1023)
                a2 = 1023;
            else if (a2 < 0)
                a2 = 0;

            if (thr1 > 255)
                thr1 = 255;
            else if (thr1 < 0)
                thr1 = 0;

            if (thr2 > 255)
                thr2 = 255;
            else if (thr2 < 0)
                thr2 = 0;

            if (off1 > 511)
                off1 = 511;
            else if (off1 < -512)
                off1 = -512;

            if (off2 > 511)
                off2 = 511;
            else if (off2 < -512)
                off2 = -512;

            post->reg68_contrast_adjust.sw_contrast_thr1 = thr1;
            post->reg69.sw_contrast_thr2 = thr2;
            post->reg68_contrast_adjust.sw_contrast_off1 = off1;
            post->reg68_contrast_adjust.sw_contrast_off2 = off2;

            post->reg69.sw_color_coeffa1 = a1;
            post->reg69.sw_color_coeffa2 = a2;
        } else {
            post->reg68_contrast_adjust.sw_contrast_thr1 = 55;
            post->reg69.sw_contrast_thr2 = 165;
            post->reg68_contrast_adjust.sw_contrast_off1 = 0;
            post->reg68_contrast_adjust.sw_contrast_off2 = 0;

            tmp = a;
            if (tmp > 1023)
                tmp = 1023;
            else if (tmp < 0)
                tmp = 0;

            post->reg69.sw_color_coeffa1 = tmp;
            post->reg69.sw_color_coeffa2 = tmp;
        }

        post->reg61_dev_conf.sw_pp_out_endian = 0;

        /* saturation */
        satur = 64 + SATURATION;

        tmp = (satur * (int) b) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        post->reg70_color_coeff_0.sw_color_coeffb = (unsigned int) tmp;

        tmp = (satur * (int) c) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        post->reg70_color_coeff_0.sw_color_coeffc = (unsigned int) tmp;

        tmp = (satur * (int) d) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        post->reg70_color_coeff_0.sw_color_coeffd = (unsigned int) tmp;

        tmp = (satur * (int) e) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        post->reg71_color_coeff_1.sw_color_coeffe = (unsigned int) tmp;
    }

    switch (outColor) {
    case PP_OUT_FORMAT_RGB565:
        post->reg82_r_mask = 0xF800F800;
        post->reg83_g_mask = 0x07E007E0;
        post->reg84_b_mask = 0x001F001F;
        post->reg79_scaling_0.sw_rgb_r_padd = 0;
        post->reg79_scaling_0.sw_rgb_g_padd = 5;
        post->reg80_scaling_1.sw_rgb_b_padd = 11;
        if (dither) { //always do dither
            JPEGD_VERBOSE_LOG("we do dither.");
            post->reg91_pip_2.sw_dither_select_r = 2;
            post->reg91_pip_2.sw_dither_select_g = 3;
            post->reg91_pip_2.sw_dither_select_b = 2;
        } else {
            JPEGD_VERBOSE_LOG("we do not dither.");
        }
        post->reg79_scaling_0.sw_rgb_pix_in32 = 1;
        post->reg85_ctrl.sw_pp_out_swap16_e = 1;
        post->reg85_ctrl.sw_pp_out_format = 0;
        break;
    case PP_OUT_FORMAT_ARGB:
        post->reg82_r_mask = 0x000000FF | (0xff << 24);
        post->reg83_g_mask = 0x0000FF00 | (0xff << 24);
        post->reg84_b_mask = 0x00FF0000 | (0xff << 24);

        post->reg79_scaling_0.sw_rgb_r_padd = 24;
        post->reg79_scaling_0.sw_rgb_g_padd = 16;
        post->reg80_scaling_1.sw_rgb_b_padd = 8;

        post->reg79_scaling_0.sw_rgb_pix_in32 = 0;
        post->reg85_ctrl.sw_pp_out_format = 0;
        break;
    case PP_OUT_FORMAT_YUV422INTERLAVE:
        post->reg85_ctrl.sw_pp_out_format = 3;
        break;
    case PP_OUT_FORMAT_YUV420INTERLAVE: {
        RK_U32 phy_addr = pSyntax->asicBuff.outLumaBuffer.phy_addr;
        if (VPUClientGetIOMMUStatus() <= 0) {
            post->reg67_pp_out_ch_base = (phy_addr + outWidth * outHeight);
        } else {
            if (outWidth * outHeight > 0x400000) {
                JPEGD_ERROR_LOG
			("out offset big than 22bit iommu map may be error");
            } else {
                post->reg67_pp_out_ch_base = (phy_addr
                                              | ((outWidth * outHeight)  << 10));
            }
        }
        post->reg85_ctrl.sw_pp_out_format = 5;
        JPEGD_INFO_LOG("outWidth:%d, outHeight:%d", outWidth, outHeight);
        JPEGD_INFO_LOG("outLumaBuffer:%x, ch reg67:%x", phy_addr,
                       post->reg67_pp_out_ch_base);
    }
    break;
    default:
        JPEGD_ERROR_LOG("unsuppotred format:%d", outColor);
        return -1;
    }

    post->reg71_color_coeff_1.sw_rotation_mode = 0;

    unsigned int        inw, inh;
    unsigned int        outw, outh;

    inw = inWidth - 1;
    inh = inHeigth - 1;
    outw = outWidth - 1;
    outh = outHeight - 1;

    if (inw < outw) {
        post->reg80_scaling_1.sw_hor_scale_mode = 1;
        post->reg79_scaling_0.sw_scale_wratio = (outw << 16) / inw;
        post->reg81_scaling_2.sw_wscale_invra = (inw << 16) / outw;
    } else if (inw > outw) {
        post->reg80_scaling_1.sw_hor_scale_mode = 2;
        post->reg81_scaling_2.sw_wscale_invra = ((outw + 1) << 16) / (inw + 1);
    } else
        post->reg80_scaling_1.sw_hor_scale_mode = 0;

    if (inh < outh) {
        post->reg80_scaling_1.sw_ver_scale_mode = 1;
        post->reg80_scaling_1.sw_scale_hratio = (outh << 16) / inh;
        post->reg81_scaling_2.sw_hscale_invra = (inh << 16) / outh;
    } else if (inh > outh) {
        post->reg80_scaling_1.sw_ver_scale_mode = 2;
        post->reg81_scaling_2.sw_hscale_invra =
            ((outh + 1) << 16) / (inh + 1) + 1;
    } else
        post->reg80_scaling_1.sw_ver_scale_mode = 0;

    post->reg60_interrupt.sw_pp_pipeline_e = ppInfo->enable;
    FUN_TEST("Exit");
    return 0;
}

static JpegDecRet
jpegd_configure_regs(JpegSyntaxParam *pSyntax, JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    JpegRegSet *reg = pCtx->regs;

    reg->reg3.sw_filtering_dis = 1;

    /* JPEG decoder */
    reg->reg3.sw_dec_mode = 3;

    reg->reg3.sw_dec_out_dis = 0;
    reg->reg3.sw_rlc_mode_e = pSyntax->info.rlcMode;

    /* frame size, round up the number of mbs */
    reg->reg4.sw_pic_mb_h_ext = ((((pSyntax->info.Y) >> (4)) & 0x700) >> 8);
    reg->reg4.sw_pic_mb_w_ext = ((((pSyntax->info.X) >> (4)) & 0xE00) >> 9);
    reg->reg4.sw_pic_mb_width = ((pSyntax->info.X) >> (4)) & 0x1FF;
    reg->reg4.sw_pic_mb_height_p = ((pSyntax->info.Y) >> (4)) & 0x0FF;

    reg->reg7.sw_pjpeg_fildown_e = pSyntax->info.fillBottom;
    /* Set spectral selection start coefficient */
    reg->reg7.sw_pjpeg_ss = pSyntax->scan.Ss;
    /* Set spectral selection end coefficient */
    reg->reg7.sw_pjpeg_se = pSyntax->scan.Se;
    /* Set the point transform used in the preceding scan */
    reg->reg7.sw_pjpeg_ah = pSyntax->scan.Ah;
    /* Set the point transform value */
    reg->reg7.sw_pjpeg_al = pSyntax->scan.Al;

    reg->reg5.sw_jpeg_qtables = pSyntax->info.amountOfQTables;
    reg->reg5.sw_jpeg_mode = pSyntax->info.yCbCrMode;
    reg->reg5.sw_jpeg_filright_e = pSyntax->info.fillRight;

    reg->reg15.sw_jpeg_slice_h = pSyntax->info.sliceHeight;
    /*
     * Set bit 21 of reg148 to 1, notifying hardware to decode
     * jpeg including DRI segment
     */
    reg->reg5.sw_sync_marker_e = 1;

    /*
     * FIXME tell hardware that height is 8-pixel aligned,
     * but not 16-pixel aligned
     */

    /* Set JPEG operation mode */
    if (pSyntax->info.operationType != JPEGDEC_PROGRESSIVE) {
        reg->reg3.sw_pjpeg_e = 0;
    } else {
        reg->reg3.sw_pjpeg_e = 1;
    }

    if (pSyntax->info.operationType == JPEGDEC_BASELINE) {
        /* write "length amounts" */
        JPEGD_VERBOSE_LOG("Write VLC length amounts to register\n");
        jpegd_write_len_bits(pSyntax, pCtx);

        /* Create AC/DC/QP tables for HW */
        JPEGD_VERBOSE_LOG("Write AC,DC,QP tables to base\n");
        jpegd_write_tables(pSyntax, pCtx);
    } else if (pSyntax->info.operationType == JPEGDEC_NONINTERLEAVED) {
        JPEGD_INFO_LOG("JPEGDEC_NONINTERLEAVED");
    } else {
        JPEGD_INFO_LOG("other operation type");
    }

    /* Select which tables the chromas use */
    jpegd_select_chroma_table(pSyntax, pCtx);

    /* write table base */
    reg->reg40_qtable_base = mpp_buffer_get_fd(pCtx->pTableBase);

    /* set up stream position for HW decode */
    jpegd_set_stream_params(pSyntax, pCtx);

    /* set restart interval */
    if (pSyntax->frame.Ri) {
        reg->reg5.sw_sync_marker_e = 1;
        /*
        * If exists DRI segment, bit 0 to bit 15 of reg8
         * is set to restart interval
         */
        reg->reg8.sw_pjpeg_rest_freq = pSyntax->frame.Ri;
    } else {
        reg->reg5.sw_sync_marker_e = 0;
    }

    /* PP depending register writes */
    if (pSyntax->ppInstance != NULL && pSyntax->ppControl.usePipeline) {
        JPEGD_INFO_LOG("ppInstance is not NULL!");
        reg->reg3.sw_dec_out_dis = 1;

        /* set output to zero, because of pp */
        /* Luminance output */
        reg->reg13_cur_pic_base = 0;

        /* Chrominance output */
        if (pSyntax->image.sizeChroma) {
            reg->reg14_sw_jpg_ch_out_base = 0;
        }

        if (pSyntax->info.sliceStartCount == JPEGDEC_SLICE_START_VALUE) {
            /* Enable pp */
        }

        pSyntax->info.pipeline = 1;
    } else {
        JPEGD_INFO_LOG("ppInstance is NULL!");

        if (pSyntax->info.operationType == JPEGDEC_BASELINE) {
            /* Luminance output */
            JPEGD_INFO_LOG("Luma virtual: %p, phy_addr: %x\n",
                           pSyntax->asicBuff.outLumaBuffer.vir_addr,
                           pSyntax->asicBuff.outLumaBuffer.phy_addr);
            reg->reg13_cur_pic_base = pSyntax->asicBuff.outLumaBuffer.phy_addr;

            /* Chrominance output */
            if (pSyntax->image.sizeChroma) {
                JPEGD_INFO_LOG("Chroma virtual: %p, phy_addr: %x\n",
                               pSyntax->asicBuff.outChromaBuffer.vir_addr,
                               pSyntax->asicBuff.outChromaBuffer.phy_addr);
                reg->reg14_sw_jpg_ch_out_base =
                    pSyntax->asicBuff.outChromaBuffer.phy_addr;
            }
        } else {
            JPEGD_ERROR_LOG ("Output address is NULL");
            JPEGD_INFO_LOG("NON_JPEGDEC_BASELINE");
        }

        pSyntax->info.pipeline = 0;
    }

    pSyntax->info.sliceStartCount = 1;

    /* Enable jpeg mode and set slice mode */
    reg->reg1_interrupt.sw_dec_e = 1;

    FUN_TEST("Exit");
    return MPP_OK;
}

static MPP_RET jpegd_regs_init(JpegRegSet *reg)
{
    FUN_TEST("Enter");
    reg->reg2_dec_ctrl.sw_dec_out_tiled_e = 0;
    reg->reg2_dec_ctrl.sw_dec_scmd_dis = DEC_VDPU1_SCMD_DISABLE;
    reg->reg2_dec_ctrl.sw_dec_latency = DEC_VDPU1_LATENCY_COMPENSATION;

    reg->reg2_dec_ctrl.sw_dec_in_endian = DEC_VDPU1_BIG_ENDIAN;
    reg->reg2_dec_ctrl.sw_dec_out_endian = DEC_VDPU1_LITTLE_ENDIAN;
    reg->reg2_dec_ctrl.sw_dec_strendian_e = DEC_VDPU1_LITTLE_ENDIAN;
    reg->reg2_dec_ctrl.sw_dec_outswap32_e = DEC_VDPU1_LITTLE_ENDIAN;
    reg->reg2_dec_ctrl.sw_dec_inswap32_e = 1;
    reg->reg2_dec_ctrl.sw_dec_strswap32_e = 1;

    reg->reg1_interrupt.sw_dec_irq_dis = 0;

    reg->reg2_dec_ctrl.sw_dec_axi_rn_id = 0xff;
    reg->reg3.sw_dec_axi_wr_id = 0;
    reg->reg2_dec_ctrl.sw_dec_max_burst = DEC_VDPU1_BUS_BURST_LENGTH_16;
    reg->reg2_dec_ctrl.sw_dec_data_disc_e = DEC_VDPU1_DATA_DISCARD_ENABLE;

    reg->reg2_dec_ctrl.sw_dec_timeout_e = 1;
    reg->reg2_dec_ctrl.sw_dec_clk_gate_e = 1;

    FUN_TEST("Exit");
    return MPP_OK;
}

static MPP_RET jpegd_gen_regs(JpegHalContext *ctx, JpegSyntaxParam *syntax)
{
    FUN_TEST("Enter");
    if (NULL == ctx || NULL == syntax) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }
    JpegDecRet retCode;
    JpegHalContext *pCtx = ctx;
    JpegSyntaxParam *pSyntax = syntax;

    retCode = JPEGDEC_OK;
    if (pSyntax->image.headerReady) {
        JPEGD_INFO_LOG("image header is ready");
        do {
            /* loop until decoding control should return for user */
            if (pSyntax->ppInstance != NULL) {
		/* if pp enabled ==> set pp control */
                pSyntax->ppControl.usePipeline = 1;
            }

            /* check if we had to load input buffer or not */
            if (!pSyntax->info.inputBufferEmpty) {
                /* Start HW or continue after pause */
                if (!pSyntax->info.SliceReadyForPause) {
                    if (!pSyntax->info.progressiveScanReady
                        || pSyntax->info.nonInterleavedScanReady) {
                        JPEGD_INFO_LOG("Start to configure registers\n");
                        retCode = jpegd_configure_regs(pSyntax, pCtx);

                        pSyntax->info.nonInterleavedScanReady = 0;
                        if (retCode != JPEGDEC_OK) {
                            return retCode;
                        }
                    } else {
                        JPEGD_INFO_LOG("Continue HW decoding after progressive scan ready");
                        pSyntax->info.progressiveScanReady = 0;
                    }
                } else {
                    JPEGD_INFO_LOG("Continue HW decoding after slice ready");
                }

                pSyntax->info.SliceCount++;
            } else {
                JPEGD_INFO_LOG("Continue HW decoding after input buffer has been loaded");

                /* buffer loaded ==> reset flag */
                pSyntax->info.inputBufferEmpty = 0;
            }
            /* TODO: when to end this loop */
        } while (0 /*!pSyntax->image.imageReady*/);
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET hal_jpegd_vdpu1_init(void *hal, MppHalCfg *cfg)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    JpegRegSet *reg = NULL;
    if (NULL == JpegHalCtx) {
        JpegHalCtx = (JpegHalContext *)mpp_calloc(JpegHalContext, 1);
        if (NULL == JpegHalCtx) {
            JPEGD_ERROR_LOG("NULL pointer");
            return MPP_ERR_NULL_PTR;
        }
    }

    //configure
    JpegHalCtx->packet_slots = cfg->packet_slots;
    JpegHalCtx->frame_slots = cfg->frame_slots;

    //get vpu socket
#ifdef RKPLATFORM
    if (JpegHalCtx->vpu_socket <= 0) {
        JpegHalCtx->vpu_socket = VPUClientInit(VPU_DEC);
        if (JpegHalCtx->vpu_socket <= 0) {
            JPEGD_ERROR_LOG("get vpu_socket(%d) <= 0, failed. \n",
                            JpegHalCtx->vpu_socket);
            return MPP_ERR_UNKNOW;
        } else {
            JPEGD_VERBOSE_LOG("get vpu_socket(%d), success. \n",
                              JpegHalCtx->vpu_socket);
        }
    }
#endif

    /* allocate regs buffer */
    if (JpegHalCtx->regs == NULL) {
        JpegHalCtx->regs = mpp_calloc_size(void, sizeof(JpegRegSet));
        if (JpegHalCtx->regs == NULL) {
            mpp_err("hal jpegd reg alloc failed\n");

            FUN_TEST("FUN_OUT");
            return MPP_ERR_NOMEM;
        }
    }
    reg = JpegHalCtx->regs;
    memset(reg, 0, sizeof(JpegRegSet));
    jpegd_regs_init(reg);

    //malloc hw buf
    if (JpegHalCtx->group == NULL) {
#ifdef RKPLATFORM
        JPEGD_VERBOSE_LOG("mpp_buffer_group_get_internal used ion in");
        ret = mpp_buffer_group_get_internal(&JpegHalCtx->group,
                                            MPP_BUFFER_TYPE_ION);
#else
        ret = mpp_buffer_group_get_internal(&JpegHalCtx->group,
                                            MPP_BUFFER_TYPE_NORMAL);
#endif
        if (MPP_OK != ret) {
            JPEGD_ERROR_LOG("mpp_buffer_group_get failed\n");
            return ret;
        }
    }

    ret = mpp_buffer_get(JpegHalCtx->group, &JpegHalCtx->frame_buf,
                         JPEGD_STREAM_BUFF_SIZE);
    if (MPP_OK != ret) {
        JPEGD_ERROR_LOG("get buffer failed\n");
        return ret;
    }

    ret = mpp_buffer_get(JpegHalCtx->group, &JpegHalCtx->pTableBase,
                         JPEGDEC_BASELINE_TABLE_SIZE);
    if (MPP_OK != ret) {
        JPEGD_ERROR_LOG("get buffer failed\n");
        return ret;
    }

    JpegHalCtx->output_fmt = MPP_FMT_YUV420SP;
    JpegHalCtx->set_output_fmt_flag = 0;

    /* init dbg stuff */
    JpegHalCtx->hal_debug_enable = 0;
    JpegHalCtx->frame_count = 0;
    JpegHalCtx->output_yuv_count = 0;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET hal_jpegd_vdpu1_deinit(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;

#ifdef RKPLATFORM
    if (JpegHalCtx->vpu_socket >= 0) {
        VPUClientRelease(JpegHalCtx->vpu_socket);
    }
#endif

    if (JpegHalCtx->frame_buf) {
        ret = mpp_buffer_put(JpegHalCtx->frame_buf);
        if (MPP_OK != ret) {
            JPEGD_ERROR_LOG("put buffer failed\n");
            return ret;
        }
    }

    if (JpegHalCtx->pTableBase) {
        ret = mpp_buffer_put(JpegHalCtx->pTableBase);
        if (MPP_OK != ret) {
            JPEGD_ERROR_LOG("put buffer failed\n");
            return ret;
        }
    }

    if (JpegHalCtx->group) {
        ret = mpp_buffer_group_put(JpegHalCtx->group);
        if (MPP_OK != ret) {
            JPEGD_ERROR_LOG("group free buffer failed\n");
            return ret;
        }
    }

    if (JpegHalCtx->regs) {
        mpp_free(JpegHalCtx->regs);
        JpegHalCtx->regs = NULL;
    }

    JpegHalCtx->output_fmt = MPP_FMT_YUV420SP;
    JpegHalCtx->set_output_fmt_flag = 0;
    JpegHalCtx->hal_debug_enable = 0;
    JpegHalCtx->frame_count = 0;
    JpegHalCtx->output_yuv_count = 0;

    return MPP_OK;
    FUN_TEST("Exit");
}

MPP_RET hal_jpegd_vdpu1_gen_regs(void *hal,  HalTaskInfo *syn)
{
    FUN_TEST("Enter");
    if (NULL == hal || NULL == syn) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    JpegSyntaxParam *pSyntax = (JpegSyntaxParam *)syn->dec.syntax.data;
#ifdef RKPLATFORM
    MppBuffer streambuf = NULL;
    MppBuffer outputBuf = NULL;
#endif

    if (syn->dec.valid) {
        syn->dec.valid = 0;
        jpegd_set_output_format(JpegHalCtx, pSyntax);

#ifdef RKPLATFORM
        if (JpegHalCtx->set_output_fmt_flag && (JpegHalCtx->vpu_socket > 0)) {
            VPUClientRelease(JpegHalCtx->vpu_socket);
            JpegHalCtx->vpu_socket = 0;

            JpegHalCtx->vpu_socket = VPUClientInit(VPU_DEC_PP);
            if (JpegHalCtx->vpu_socket <= 0) {
                JPEGD_ERROR_LOG("get vpu_socket(%d) <= 0, failed. \n",
                                JpegHalCtx->vpu_socket);
                return MPP_ERR_UNKNOW;
            } else {
                JPEGD_VERBOSE_LOG("get vpu_socket(%d), success. \n",
                                  JpegHalCtx->vpu_socket);
            }
        }

        mpp_buf_slot_get_prop(JpegHalCtx->packet_slots, syn->dec.input,
                              SLOT_BUFFER, &streambuf);
        pSyntax->stream.streamBus = mpp_buffer_get_fd(streambuf);

        mpp_buf_slot_get_prop(JpegHalCtx->frame_slots, syn->dec.output,
                              SLOT_BUFFER, &outputBuf);
        pSyntax->asicBuff.outLumaBuffer.phy_addr = mpp_buffer_get_fd(outputBuf);

        jpegd_allocate_chroma_out_buffer(pSyntax);
#endif

        ret = jpegd_set_post_processor(JpegHalCtx, pSyntax);

        ret = jpegd_gen_regs(JpegHalCtx, pSyntax);

        if (JpegHalCtx->hal_debug_enable && JpegHalCtx->frame_count < 3) {
            RK_U32 idx = 0;
            RK_U32 *pRegs = (RK_U32 *) JpegHalCtx->regs;
            JPEGD_INFO_LOG("sizeof(JpegRegSet)=%d, sizeof(pRegs[0])=%d",
                           sizeof(JpegRegSet), sizeof(pRegs[0]));

            JpegHalCtx->frame_count++;
            for (idx = 0; idx < sizeof(JpegRegSet) / sizeof(pRegs[0]); idx++) {
                JPEGD_INFO_LOG("frame_%d_reg[%d]=0x%08x",
                               JpegHalCtx->frame_count, idx, pRegs[idx]);
            }
        }

        syn->dec.valid = 1;
    }

    return ret;
    FUN_TEST("Exit");
}

MPP_RET hal_jpegd_vdpu1_start(void *hal, HalTaskInfo *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;

#ifdef RKPLATFORM
    RK_U32 *p_regs = (RK_U32 *)JpegHalCtx->regs;
    ret = VPUClientSendReg(JpegHalCtx->vpu_socket, p_regs, JPEGD_REG_NUM);
    if (ret != 0) {
        JPEGD_ERROR_LOG("VPUClientSendReg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }
#endif
    (void)JpegHalCtx;
    (void)task;
    FUN_TEST("Exit");
    return ret;
}

MPP_RET hal_jpegd_vdpu1_wait(void *hal, HalTaskInfo *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    (void)JpegHalCtx;
    (void)task;

#ifdef RKPLATFORM
    JpegRegSet reg_out;
    VPU_CMD_TYPE cmd = 0;
    RK_S32 length = 0;
    RK_U32 errinfo = 1;
    MppFrame tmp = NULL;
    memset(&reg_out, 0, sizeof(JpegRegSet));

    ret = VPUClientWaitResult(JpegHalCtx->vpu_socket, (RK_U32 *)&reg_out,
                              JPEGD_REG_NUM, &cmd, &length);
    if (reg_out.reg1_interrupt.sw_dec_bus_int) {
        JPEGD_ERROR_LOG("IRQ BUS ERROR!");
    } else if (reg_out.reg1_interrupt.sw_dec_error_int) {
        JPEGD_ERROR_LOG("IRQ STREAM ERROR!");
    } else if (reg_out.reg1_interrupt.sw_dec_timeout) {
        JPEGD_ERROR_LOG("IRQ TIMEOUT!");
    } else if (reg_out.reg1_interrupt.sw_dec_buffer_int) {
        JPEGD_ERROR_LOG("IRQ BUFFER EMPTY!");
    } else if (reg_out.reg1_interrupt.sw_dec_irq) {
        errinfo = 0;
        JPEGD_INFO_LOG("DECODE SUCCESS!");
    }

    mpp_buf_slot_get_prop(JpegHalCtx->frame_slots, task->dec.output,
                          SLOT_FRAME_PTR, &tmp);
    mpp_frame_set_errinfo(tmp, errinfo);

    /* debug information */
    if (JpegHalCtx->hal_debug_enable && JpegHalCtx->output_yuv_count < 3) {
        static FILE *jpg_file;
        static char name[32];
        MppBuffer outputBuf = NULL;
        void *pOutYUV = NULL;
        mpp_buf_slot_get_prop(JpegHalCtx->frame_slots, task->dec.output,
                              SLOT_BUFFER, &outputBuf);
        pOutYUV = mpp_buffer_get_ptr(outputBuf);

        snprintf(name, sizeof(name), "/tmp/output%02d.yuv",
                 JpegHalCtx->output_yuv_count);
        jpg_file = fopen(name, "wb+");
        if (jpg_file) {
            JpegSyntaxParam *pTmpSyn = (JpegSyntaxParam *)task->dec.syntax.data;
            RK_U32 width = pTmpSyn->frame.hwX;
            RK_U32 height = pTmpSyn->frame.hwY;

            JPEGD_INFO_LOG("output YUV(%d*%d) saving to %s\n",
                           width, height, name);
            JPEGD_INFO_LOG("pOutYUV:%p", pOutYUV);
            fwrite(pOutYUV, width * height * 3 / 2, 1, jpg_file);
            fclose(jpg_file);
            JpegHalCtx->output_yuv_count++;
        }
    }
#endif

    FUN_TEST("Exit");
    return ret;
}

MPP_RET hal_jpegd_vdpu1_reset(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    (void)JpegHalCtx;

    return ret;
}

MPP_RET hal_jpegd_vdpu1_flush(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_jpegd_vdpu1_control(void *hal, RK_S32 cmd_type, void *param)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    if (NULL == JpegHalCtx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    switch (cmd_type) {
    case MPP_DEC_SET_OUTPUT_FORMAT: {
        JpegHalCtx->output_fmt = *((MppFrameFormat *)param);
        JpegHalCtx->set_output_fmt_flag = 1;
        JPEGD_INFO_LOG("output_format:%d\n", JpegHalCtx->output_fmt);
    } break;
    default :
        ret = MPP_NOK;
    }
    FUN_TEST("Exit");
    return  ret;
}
