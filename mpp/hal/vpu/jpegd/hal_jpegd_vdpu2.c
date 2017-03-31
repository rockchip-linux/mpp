/*
 *
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
#define MODULE_TAG "HAL_JPEG_VDPU2"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "mpp_buffer.h"
#include "mpp_log.h"
#include "mpp_err.h"
#include "mpp_mem.h"
#include "mpp_bitread.h"
#include "mpp_dec.h"
#include "mpp_device.h"
#include "mpp_buffer.h"
#include "mpp_env.h"
#include "mpp_bitput.h"

#include "jpegd_api.h"
#include "jpegd_syntax.h"
#include "hal_jpegd_common.h"
#include "hal_jpegd_vdpu2.h"
#include "hal_jpegd_vdpu2_reg.h"


static void
jpegd_write_len_bits(JpegSyntaxParam *pSyntax, JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    ScanInfo         *JPG_SCN = &pSyntax->scan;
    HuffmanTables    *JPG_VLC = &pSyntax->vlc;
    JpegdIocRegInfo *info = (JpegdIocRegInfo *)pCtx->regs;
    JpegRegSet *reg = &(info->regs);

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
    reg->reg134.sw_ac1_code1_cnt = pTable1->bits[0];
    reg->reg134.sw_ac1_code2_cnt = pTable1->bits[1];
    reg->reg134.sw_ac1_code3_cnt = pTable1->bits[2];
    reg->reg134.sw_ac1_code4_cnt = pTable1->bits[3];
    reg->reg134.sw_ac1_code5_cnt = pTable1->bits[4];
    reg->reg134.sw_ac1_code6_cnt = pTable1->bits[5];

    reg->reg135.sw_ac1_code7_cnt = pTable1->bits[6];
    reg->reg135.sw_ac1_code8_cnt = pTable1->bits[7];
    reg->reg135.sw_ac1_code9_cnt = pTable1->bits[8];
    reg->reg135.sw_ac1_code10_cnt = pTable1->bits[9];

    reg->reg136.sw_ac1_code11_cnt = pTable1->bits[10];
    reg->reg136.sw_ac1_code12_cnt = pTable1->bits[11];
    reg->reg136.sw_ac1_code13_cnt = pTable1->bits[12];
    reg->reg136.sw_ac1_code14_cnt = pTable1->bits[13];

    reg->reg137.sw_ac1_code15_cnt = pTable1->bits[14];
    reg->reg137.sw_ac1_code16_cnt = pTable1->bits[15];

    /* table AC2 (the not-luma table) */
    reg->reg137.sw_ac2_code1_cnt = pTable2->bits[0];
    reg->reg137.sw_ac2_code2_cnt = pTable2->bits[1];
    reg->reg137.sw_ac2_code3_cnt = pTable2->bits[2];
    reg->reg137.sw_ac2_code4_cnt = pTable2->bits[3];

    reg->reg138.sw_ac2_code5_cnt = pTable2->bits[4];
    reg->reg138.sw_ac2_code6_cnt = pTable2->bits[5];
    reg->reg138.sw_ac2_code7_cnt = pTable2->bits[6];
    reg->reg138.sw_ac2_code8_cnt = pTable2->bits[7];

    reg->reg139.sw_ac2_code9_cnt = pTable2->bits[8];
    reg->reg139.sw_ac2_code10_cnt = pTable2->bits[9];
    reg->reg139.sw_ac2_code11_cnt = pTable2->bits[10];
    reg->reg139.sw_ac2_code12_cnt = pTable2->bits[11];

    reg->reg140.sw_ac2_code13_cnt = pTable2->bits[12];
    reg->reg140.sw_ac2_code14_cnt = pTable2->bits[13];
    reg->reg140.sw_ac2_code15_cnt = pTable2->bits[14];
    reg->reg140.sw_ac2_code16_cnt = pTable2->bits[15];

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
    reg->reg141.sw_dc1_code1_cnt = pTable1->bits[0];
    reg->reg141.sw_dc1_code2_cnt = pTable1->bits[1];
    reg->reg141.sw_dc1_code3_cnt = pTable1->bits[2];
    reg->reg141.sw_dc1_code4_cnt = pTable1->bits[3];
    reg->reg141.sw_dc1_code5_cnt = pTable1->bits[4];
    reg->reg141.sw_dc1_code6_cnt = pTable1->bits[5];
    reg->reg141.sw_dc1_code7_cnt = pTable1->bits[6];
    reg->reg141.sw_dc1_code8_cnt = pTable1->bits[7];

    reg->reg142.sw_dc1_code9_cnt = pTable1->bits[8];
    reg->reg142.sw_dc1_code10_cnt = pTable1->bits[9];
    reg->reg142.sw_dc1_code11_cnt = pTable1->bits[10];
    reg->reg142.sw_dc1_code12_cnt = pTable1->bits[11];
    reg->reg142.sw_dc1_code13_cnt = pTable1->bits[12];
    reg->reg142.sw_dc1_code14_cnt = pTable1->bits[13];
    reg->reg142.sw_dc1_code15_cnt = pTable1->bits[14];
    reg->reg142.sw_dc1_code16_cnt = pTable1->bits[15];

    /* table DC2 (the not-luma table) */
    reg->reg143.sw_dc2_code1_cnt = pTable2->bits[0];
    reg->reg143.sw_dc2_code2_cnt = pTable2->bits[1];
    reg->reg143.sw_dc2_code3_cnt = pTable2->bits[2];
    reg->reg143.sw_dc2_code4_cnt = pTable2->bits[3];
    reg->reg143.sw_dc2_code5_cnt = pTable2->bits[4];
    reg->reg143.sw_dc2_code6_cnt = pTable2->bits[5];
    reg->reg143.sw_dc2_code7_cnt = pTable2->bits[6];
    reg->reg143.sw_dc2_code8_cnt = pTable2->bits[7];

    reg->reg144.sw_dc2_code9_cnt = pTable2->bits[8];
    reg->reg144.sw_dc2_code10_cnt = pTable2->bits[9];
    reg->reg144.sw_dc2_code11_cnt = pTable2->bits[10];
    reg->reg144.sw_dc2_code12_cnt = pTable2->bits[11];
    reg->reg144.sw_dc2_code13_cnt = pTable2->bits[12];
    reg->reg144.sw_dc2_code14_cnt = pTable2->bits[13];
    reg->reg144.sw_dc2_code15_cnt = pTable2->bits[14];
    reg->reg144.sw_dc2_code16_cnt = pTable2->bits[15];

    FUN_TEST("Exit");
    return;
}

static void jpegd_set_stream_params(JpegSyntaxParam *pSyntax,
                                    JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    StreamStorage *JPG_STR = &pSyntax->stream;
    JpegdIocRegInfo *info = (JpegdIocRegInfo *)pCtx->regs;
    JpegRegSet *reg = &(info->regs);

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

    reg->reg64_rlc_vlc_base = JPG_STR->streamBus | (addrTmp << 10);
    JPEGD_INFO_LOG("reg64_rlc_vlc_base: 0x%08x\n", reg->reg64_rlc_vlc_base);

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

    reg->reg122.sw_strm_start_bit = JPG_STR->bitPosInByte;

    /* set up stream length for HW.
     * length = size of original buffer - stream we already decoded in SW */
    JPG_STR->pCurrPos = (RK_U8 *) ((uintptr_t) JPG_STR->pCurrPos & (~7));

    if (pSyntax->info.inputStreaming) {
        JPEGD_VERBOSE_LOG("inputStreaming-1, inputBufferLen:%d",
                          pSyntax->info.inputBufferLen);
        amountOfStream = (pSyntax->info.inputBufferLen -
                          (RK_U32) (JPG_STR->pCurrPos - JPG_STR->pStartOfStream));

        reg->reg51_stream_info.sw_stream_len = amountOfStream;
        pSyntax->info.streamEnd = 1;
    } else {
        JPEGD_VERBOSE_LOG("inputStreaming-2");
        amountOfStream = (JPG_STR->streamLength -
                          (RK_U32) (JPG_STR->pCurrPos
                                    - JPG_STR->pStartOfStream));

        reg->reg51_stream_info.sw_stream_len = amountOfStream;

        /* because no input streaming, frame should be ready during decoding this buffer */
        pSyntax->info.streamEnd = 1;
    }

    reg->reg122.sw_jpeg_stream_all = pSyntax->info.streamEnd;

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
    JpegdIocRegInfo *info = (JpegdIocRegInfo *)pCtx->regs;
    JpegRegSet *reg = &(info->regs);

    /* this trick is done because hw always wants luma table as ac hw table 1 */
    if (JPG_SCN->Ta[0] == 0) {
        reg->reg122.sw_cr_ac_vlctable = JPG_SCN->Ta[2];
        reg->reg122.sw_cb_ac_vlctable = JPG_SCN->Ta[1];
    } else {
        if (JPG_SCN->Ta[0] == JPG_SCN->Ta[1])
            reg->reg122.sw_cb_ac_vlctable = 0;
        else
            reg->reg122.sw_cb_ac_vlctable = 1;

        if (JPG_SCN->Ta[0] == JPG_SCN->Ta[2])
            reg->reg122.sw_cr_ac_vlctable = 0;
        else
            reg->reg122.sw_cr_ac_vlctable = 1;
    }

    /* Third DC table selectors */
    if (pSyntax->info.operationType != JPEGDEC_PROGRESSIVE) {
        JPEGD_INFO_LOG("NON_PROGRESSIVE");
        if (JPG_SCN->Td[0] == 0) {
            reg->reg122.sw_cr_dc_vlctable = JPG_SCN->Td[2];
            reg->reg122.sw_cb_dc_vlctable = JPG_SCN->Td[1];
        } else {
            if (JPG_SCN->Td[0] == JPG_SCN->Td[1])
                reg->reg122.sw_cb_dc_vlctable = 0;
            else
                reg->reg122.sw_cb_dc_vlctable = 1;

            if (JPG_SCN->Td[0] == JPG_SCN->Td[2])
                reg->reg122.sw_cr_dc_vlctable = 0;
            else
                reg->reg122.sw_cr_dc_vlctable = 1;
        }

        reg->reg122.sw_cr_dc_vlctable3 = 0;
        reg->reg122.sw_cb_dc_vlctable3 = 0;
    } else {
        JPEGD_INFO_LOG("JPEGDEC_PROGRESSIVE");
    }

    FUN_TEST("Exit");
    return;
}

static MPP_RET jpegd_set_extra_info(JpegHalContext *ctx, RK_U32 offset)
{
    FUN_TEST("Enter");
    if (NULL == ctx) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    JpegdIocRegInfo *reg_info = (JpegdIocRegInfo *)ctx->regs;
    JpegdIocExtInfo *info = &(reg_info->extra_info);
    JpegdIocExtInfoSlot *slot = NULL;
    info->cnt = 1;

    slot = &(info->slots[0]);
    slot->offset = offset;

    if (ctx->set_output_fmt_flag) {
        /* pp enable */
        slot->reg_idx = 22;
    } else {
        slot->reg_idx = 131;
    }

    FUN_TEST("Exit");
    return MPP_OK;
}

static MPP_RET jpegd_set_post_processor(JpegHalContext *pCtx,
                                        JpegSyntaxParam *pSyntax)
{
    FUN_TEST("Enter");
    if (NULL == pCtx || NULL == pSyntax) {
        JPEGD_ERROR_LOG("NULL pointer");
        return MPP_ERR_NULL_PTR;
    }

    PostProcessInfo *ppInfo = &(pSyntax->ppInfo);
    JpegdIocRegInfo *info = (JpegdIocRegInfo *)pCtx->regs;
    JpegRegSet *reg = &(info->regs);
    int inColor = pSyntax->ppInputFomart;
    int outColor = ppInfo->outFomart;
    int dither = ppInfo->shouldDither;
    int inWidth = pSyntax->imageInfo.outputWidth;
    int inHeigth = pSyntax->imageInfo.outputHeight;
    int outWidth = pSyntax->ppScaleW;
    int outHeight = pSyntax->ppScaleH;

    reg->reg0.sw_pp_axi_rd_id = 0;
    reg->reg0.sw_pp_axi_wr_id = 0;
    reg->reg0.sw_pp_scmd_dis = 1;
    reg->reg0.sw_pp_max_burst = 16;

    reg->reg18_pp_in_lu_base = 0;

    reg->reg34.sw_ext_orig_width = inWidth >> 4;

    reg->reg37.sw_pp_in_a2_endsel = 1;
    reg->reg37.sw_pp_in_a1_swap32 = 1;
    reg->reg37.sw_pp_in_a1_endian = 1;
    reg->reg37.sw_pp_in_swap32_e = 1;
    reg->reg37.sw_pp_in_endian = 1;
    reg->reg37.sw_pp_out_endian = 1;
    reg->reg37.sw_pp_out_swap32_e = 1;

    reg->reg41.sw_pp_clk_gate_e = 1;
    reg->reg41.sw_pp_ahb_hlock_e = 1;
    reg->reg41.sw_pp_data_disc_e = 1;

    if (ppInfo->cropW <= 0) {
        reg->reg34.sw_pp_in_w_ext = (((inWidth / 16) & 0xE00) >> 9);
        reg->reg34.sw_pp_in_width = ((inWidth / 16) & 0x1FF);
        reg->reg34.sw_pp_in_h_ext = (((inHeigth / 16) & 0x700) >> 8);
        reg->reg34.sw_pp_in_height = ((inHeigth / 16) & 0x0FF);
    } else {
        reg->reg34.sw_pp_in_w_ext = (((ppInfo->cropW / 16) & 0xE00) >> 9);
        reg->reg34.sw_pp_in_width = ((ppInfo->cropW / 16) & 0x1FF);
        reg->reg34.sw_pp_in_h_ext = (((ppInfo->cropH / 16) & 0x700) >> 8);
        reg->reg34.sw_pp_in_height = ((ppInfo->cropH / 16) & 0x0FF);

        reg->reg14.sw_crop_startx_ext = (((ppInfo->cropX / 16) & 0xE00) >> 9);
        reg->reg14.sw_crop_startx = ((ppInfo->cropX / 16) & 0x1FF);
        reg->reg14.sw_crop_starty_ext = (((ppInfo->cropY / 16) & 0x700) >> 8);
        reg->reg14.sw_crop_starty = ((ppInfo->cropY / 16) & 0x0FF);

        if (ppInfo->cropW & 0x0F) {
            reg->reg14.sw_pp_crop8_r_e = 1;
        } else {
            reg->reg14.sw_pp_crop8_r_e = 0;
        }
        if (ppInfo->cropH & 0x0F) {
            reg->reg14.sw_pp_crop8_d_e = 1;
        } else {
            reg->reg14.sw_pp_crop8_d_e = 0;
        }
        inWidth = ppInfo->cropW;
        inHeigth = ppInfo->cropH;
    }

    reg->reg39.sw_display_width = outWidth;
    reg->reg35.sw_pp_out_width = outWidth;
    reg->reg35.sw_pp_out_height = outHeight;
    reg->reg21_pp_out_lu_base = pSyntax->asicBuff.outLumaBuffer.phy_addr;

    switch (inColor) {
    case    PP_IN_FORMAT_YUV422INTERLAVE:
        reg->reg38.sw_pp_in_format = 0;
        break;
    case    PP_IN_FORMAT_YUV420SEMI:
        reg->reg38.sw_pp_in_format = 1;
        break;
    case    PP_IN_FORMAT_YUV420PLANAR:
        reg->reg38.sw_pp_in_format = 2;
        break;
    case    PP_IN_FORMAT_YUV400:
        reg->reg38.sw_pp_in_format = 3;
        break;
    case    PP_IN_FORMAT_YUV422SEMI:
        reg->reg38.sw_pp_in_format = 4;
        break;
    case    PP_IN_FORMAT_YUV420SEMITIELED:
        reg->reg38.sw_pp_in_format = 5;
        break;
    case    PP_IN_FORMAT_YUV440SEMI:
        reg->reg38.sw_pp_in_format = 6;
        break;
    case    PP_IN_FORMAT_YUV444_SEMI:
        reg->reg38.sw_pp_in_format = 7;
        reg->reg38.sw_pp_in_format_es = 0;
        break;
    case    PP_IN_FORMAT_YUV411_SEMI:
        reg->reg38.sw_pp_in_format = 0;
        reg->reg38.sw_pp_in_format_es = 1;
        break;
    default:
        JPEGD_ERROR_LOG("unsupported format:%d", inColor);
        return -1;
    }

#define VIDEORANGE 1    //0 or 1
    int videoRange = VIDEORANGE;

    reg->reg15.sw_rangemap_coef_y = 9;
    reg->reg15.sw_rangemap_coef_c = 9;
    /*  brightness */
    reg->reg3.sw_pp_color_coefff = BRIGHTNESS;

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

            reg->reg15.sw_ycbcr_range = videoRange;
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

            reg->reg31.sw_contrast_thr1 = thr1;
            reg->reg31.sw_contrast_thr2 = thr2;
            reg->reg32.sw_contrast_off1 = off1;
            reg->reg32.sw_contrast_off2 = off2;

            reg->reg1.sw_color_coeffa1 = a1;
            reg->reg1.sw_color_coeffa2 = a2;
        } else {
            reg->reg31.sw_contrast_thr1 = 55;
            reg->reg31.sw_contrast_thr2 = 165;
            reg->reg32.sw_contrast_off1 = 0;
            reg->reg32.sw_contrast_off2 = 0;

            tmp = a;
            if (tmp > 1023)
                tmp = 1023;
            else if (tmp < 0)
                tmp = 0;

            reg->reg1.sw_color_coeffa1 = tmp;
            reg->reg1.sw_color_coeffa2 = tmp;
        }

        reg->reg37.sw_pp_out_endian = 0;

        /* saturation */
        satur = 64 + SATURATION;

        tmp = (satur * (int) b) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        reg->reg1.sw_color_coeffb = (unsigned int) tmp;

        tmp = (satur * (int) c) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        reg->reg2.sw_color_coeffc = (unsigned int) tmp;

        tmp = (satur * (int) d) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        reg->reg2.sw_color_coeffd = (unsigned int) tmp;

        tmp = (satur * (int) e) / 64;
        if (tmp > 1023)
            tmp = 1023;
        else if (tmp < 0)
            tmp = 0;
        reg->reg2.sw_color_coeffe = (unsigned int) tmp;
    }

    switch (outColor) {
    case    PP_OUT_FORMAT_RGB565:
        reg->reg9_r_mask = 0xF800F800;
        reg->reg10_g_mask = 0x07E007E0;
        reg->reg11_b_mask = 0x001F001F;
        reg->reg16.sw_rgb_r_padd = 0;
        reg->reg16.sw_rgb_g_padd = 5;
        reg->reg16.sw_rgb_b_padd = 11;
        if (dither) { //always do dither
            JPEGD_VERBOSE_LOG("we do dither.");
            reg->reg36.sw_dither_select_r = 2;
            reg->reg36.sw_dither_select_g = 3;
            reg->reg36.sw_dither_select_b = 2;
        } else {
            JPEGD_VERBOSE_LOG("we do not dither.");
        }
        reg->reg37.sw_rgb_pix_in32 = 1;
        reg->reg37.sw_pp_out_swap16_e = 1;
        reg->reg38.sw_pp_out_format = 0;
        break;
    case    PP_OUT_FORMAT_ARGB:
        reg->reg9_r_mask = 0x000000FF | (0xff << 24);
        reg->reg10_g_mask = 0x0000FF00 | (0xff << 24);
        reg->reg11_b_mask = 0x00FF0000 | (0xff << 24);

        reg->reg16.sw_rgb_r_padd = 24;
        reg->reg16.sw_rgb_g_padd = 16;
        reg->reg16.sw_rgb_b_padd = 8;

        reg->reg37.sw_rgb_pix_in32 = 0;
        reg->reg38.sw_pp_out_format = 0;
        break;
    case    PP_OUT_FORMAT_YUV422INTERLAVE:
        reg->reg38.sw_pp_out_format = 3;
        break;
    case    PP_OUT_FORMAT_YUV420INTERLAVE: {
        RK_U32 phy_addr = pSyntax->asicBuff.outLumaBuffer.phy_addr;
        if (pCtx->set_output_fmt_flag) {
            /* pp enable */
            jpegd_set_extra_info(pCtx, outWidth * outHeight);
            reg->reg22_pp_out_ch_base = phy_addr;
        } else {
            reg->reg22_pp_out_ch_base = (phy_addr | ((outWidth * outHeight) << 10));
        }
        reg->reg38.sw_pp_out_format = 5;
        JPEGD_INFO_LOG("outWidth:%d, outHeight:%d", outWidth, outHeight);
        JPEGD_INFO_LOG("outLumaBuffer:%x, reg22:%x", phy_addr,
                       reg->reg22_pp_out_ch_base);
    }
    break;
    default:
        JPEGD_ERROR_LOG("unsuppotred format:%d", outColor);
        return -1;
    }

    reg->reg38.sw_rotation_mode = 0;

    unsigned int        inw, inh;
    unsigned int        outw, outh;

    inw = inWidth - 1;
    inh = inHeigth - 1;
    outw = outWidth - 1;
    outh = outHeight - 1;

    if (inw < outw) {
        reg->reg4.sw_hor_scale_mode = 1;
        reg->reg4.sw_scale_wratio = (outw << 16) / inw;
        reg->reg6.sw_wscale_invra = (inw << 16) / outw;
    } else if (inw > outw) {
        reg->reg4.sw_hor_scale_mode = 2;
        reg->reg6.sw_wscale_invra = ((outw + 1) << 16) / (inw + 1);
    } else
        reg->reg4.sw_hor_scale_mode = 0;

    if (inh < outh) {
        reg->reg4.sw_ver_scale_mode = 1;
        reg->reg5.sw_scale_hratio = (outh << 16) / inh;
        reg->reg6.sw_hscale_invra = (inh << 16) / outh;
    } else if (inh > outh) {
        reg->reg4.sw_ver_scale_mode = 2;
        reg->reg6.sw_hscale_invra = ((outh + 1) << 16) / (inh + 1) + 1;
    } else
        reg->reg4.sw_ver_scale_mode = 0;

    reg->reg41.sw_pp_pipeline_e = ppInfo->enable;
    FUN_TEST("Exit");
    return 0;
}

static MPP_RET
jpegd_configure_regs(JpegSyntaxParam *pSyntax, JpegHalContext *pCtx)
{
    FUN_TEST("Enter");
    JpegdIocRegInfo *info = (JpegdIocRegInfo *)pCtx->regs;
    JpegRegSet *reg = &(info->regs);

    reg->reg50_dec_ctrl.sw_filtering_dis = 1;

    reg->reg53_dec_mode = DEC_MODE_JPEG;

    reg->reg57_enable_ctrl.sw_dec_out_dis = 0;
    reg->reg57_enable_ctrl.sw_rlc_mode_e = pSyntax->info.rlcMode;

    /* frame size, round up the number of mbs */
    reg->reg120.sw_pic_mb_h_ext = ((((pSyntax->info.Y) >> (4)) & 0x700) >> 8);
    reg->reg120.sw_pic_mb_w_ext = ((((pSyntax->info.X) >> (4)) & 0xE00) >> 9);
    reg->reg120.sw_pic_mb_width = ((pSyntax->info.X) >> (4)) & 0x1FF;
    reg->reg120.sw_pic_mb_hight_p = ((pSyntax->info.Y) >> (4)) & 0x0FF;

    reg->reg121.sw_pjpeg_fildown_e = pSyntax->info.fillBottom;
    /* Set spectral selection start coefficient */
    reg->reg121.sw_pjpeg_ss = pSyntax->scan.Ss;
    /* Set spectral selection end coefficient */
    reg->reg121.sw_pjpeg_se = pSyntax->scan.Se;
    /* Set the point transform used in the preceding scan */
    reg->reg121.sw_pjpeg_ah = pSyntax->scan.Ah;
    /* Set the point transform value */
    reg->reg121.sw_pjpeg_al = pSyntax->scan.Al;

    reg->reg122.sw_jpeg_qtables = pSyntax->info.amountOfQTables;
    reg->reg122.sw_jpeg_mode = pSyntax->info.yCbCrMode;
    reg->reg122.sw_jpeg_filright_e = pSyntax->info.fillRight;

    reg->reg148.sw_slice_h = pSyntax->info.sliceHeight;
    /* Set bit 21 of reg148 to 1, notifying hardware to decode jpeg
     * including DRI segment */
    reg->reg148.sw_syn_marker_e = 1;

    /* tell hardware that height is 8-pixel aligned, but not 16-pixel aligned */
    if ((pSyntax->frame.Y % 16) &&
        (pSyntax->info.yCbCrMode == JPEGDEC_YUV422 ||
         pSyntax->info.yCbCrMode == JPEGDEC_YUV444 ||
         pSyntax->info.yCbCrMode == JPEGDEC_YUV411)) {
        reg->reg148.sw_jpeg_height8_flag = 1;
    }

    /* Set JPEG operation mode */
    if (pSyntax->info.operationType != JPEGDEC_PROGRESSIVE) {
        reg->reg57_enable_ctrl.sw_pjpeg_e = 0;
    } else {
        reg->reg57_enable_ctrl.sw_pjpeg_e = 1;
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
    reg->reg61_qtable_base = mpp_buffer_get_fd(pCtx->pTableBase);

    /* set up stream position for HW decode */
    jpegd_set_stream_params(pSyntax, pCtx);

    /* set restart interval */
    if (pSyntax->frame.Ri) {
        reg->reg122.sw_sync_marker_e = 1;
        /* If exists DRI segment, bit 0 to bit 15 of reg123 is set
         * to restart interval */
        reg->reg123.sw_pjpeg_rest_freq = pSyntax->frame.Ri;
    } else {
        reg->reg122.sw_sync_marker_e = 0;
    }

    /* PP depending register writes */
    if (pSyntax->ppInstance != NULL && pSyntax->ppControl.usePipeline) {
        JPEGD_INFO_LOG("ppInstance is not NULL!");
        reg->reg57_enable_ctrl.sw_dec_out_dis = 1;

        /* set output to zero, because of pp */
        /* Luminance output */
        reg->reg63_dec_out_base = 0;

        /* Chrominance output */
        if (pSyntax->image.sizeChroma) {
            reg->reg131_jpg_ch_out_base = 0;
        }

        pSyntax->info.pipeline = 1;
    } else {
        JPEGD_INFO_LOG("ppInstance is NULL!");

        if (pSyntax->info.operationType == JPEGDEC_BASELINE) {
            /* Luminance output */
            JPEGD_INFO_LOG("Luma virtual: %p, phy_addr: %x\n",
                           pSyntax->asicBuff.outLumaBuffer.vir_addr,
                           pSyntax->asicBuff.outLumaBuffer.phy_addr);
            reg->reg63_dec_out_base = pSyntax->asicBuff.outLumaBuffer.phy_addr;

            /* Chrominance output */
            if (pSyntax->image.sizeChroma) {
                JPEGD_INFO_LOG("Chroma virtual: %p, phy_addr: %x\n",
                               pSyntax->asicBuff.outChromaBuffer.vir_addr,
                               pSyntax->asicBuff.outChromaBuffer.phy_addr);
                if (!pCtx->set_output_fmt_flag) {
                    /* pp disable */
                    jpegd_set_extra_info(pCtx, pSyntax->image.sizeLuma);
                    reg->reg131_jpg_ch_out_base =
                        (pSyntax->asicBuff.outChromaBuffer.phy_addr & 0x3ff);
                }
            }
        } else {
            JPEGD_INFO_LOG("NON_JPEGDEC_BASELINE");
        }

        pSyntax->info.pipeline = 0;
    }

    pSyntax->info.sliceStartCount = 1;

    /* Enable jpeg mode and set slice mode */
    reg->reg57_enable_ctrl.sw_dec_e = 1;

    FUN_TEST("Exit");
    return MPP_OK;
}

static MPP_RET jpegd_regs_init(JpegRegSet *reg)
{
    FUN_TEST("Enter");
    reg->reg50_dec_ctrl.sw_dec_out_tiled_e = 0;
    reg->reg50_dec_ctrl.sw_dec_scmd_dis = DEC_SCMD_DISABLE;
    reg->reg50_dec_ctrl.sw_dec_latency = DEC_LATENCY_COMPENSATION;

    reg->reg54_endian.sw_dec_in_endian = DEC_BIG_ENDIAN;
    reg->reg54_endian.sw_dec_out_endian = DEC_LITTLE_ENDIAN;
    reg->reg54_endian.sw_dec_strendian_e = DEC_LITTLE_ENDIAN;
    reg->reg54_endian.sw_dec_outswap32_e = DEC_LITTLE_ENDIAN;
    reg->reg54_endian.sw_dec_inswap32_e = 1;
    reg->reg54_endian.sw_dec_strswap32_e = 1;

    reg->reg55_Interrupt.sw_dec_irq_dis = 0;

    reg->reg56_axi_ctrl.sw_dec_axi_rn_id = 0xff;
    reg->reg56_axi_ctrl.sw_dec_axi_wr_id = 0;
    reg->reg56_axi_ctrl.sw_dec_max_burst = DEC_BUS_BURST_LENGTH_16;
    reg->reg56_axi_ctrl.sw_dec_data_disc_e = DEC_DATA_DISCARD_ENABLE;

    reg->reg57_enable_ctrl.sw_dec_timeout_e = 1;
    reg->reg57_enable_ctrl.sw_dec_clk_gate_e = 1;

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
        do { /* loop until decoding control should return for user */
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

MPP_RET hal_jpegd_vdpu2_init(void *hal, MppHalCfg *cfg)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
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
        JpegHalCtx->vpu_socket = mpp_device_init(MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG, 0);
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

    //init regs
    JpegdIocRegInfo *info = NULL;
    JpegRegSet *reg = NULL;
    info = mpp_calloc(JpegdIocRegInfo, 1);
    if (info == NULL) {
        mpp_err_f("allocate jpegd ioctl info failed\n");
        return MPP_ERR_NOMEM;
    }
    memset(info, 0, sizeof(JpegdIocRegInfo));
    info->extra_info.magic = EXTRA_INFO_MAGIC;
    JpegHalCtx->regs = (void *)info;

    reg = &(info->regs);
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

    //init dbg stuff
    JpegHalCtx->hal_debug_enable = 0;
    JpegHalCtx->frame_count = 0;
    JpegHalCtx->output_yuv_count = 0;

    FUN_TEST("Exit");
    return MPP_OK;
}

MPP_RET hal_jpegd_vdpu2_deinit(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;

#ifdef RKPLATFORM
    if (JpegHalCtx->vpu_socket >= 0) {
        mpp_device_deinit(JpegHalCtx->vpu_socket);
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

MPP_RET hal_jpegd_vdpu2_gen_regs(void *hal,  HalTaskInfo *syn)
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
            mpp_device_deinit(JpegHalCtx->vpu_socket);
            JpegHalCtx->vpu_socket = 0;

            JpegHalCtx->vpu_socket = mpp_device_init(MPP_CTX_DEC, MPP_VIDEO_CodingMJPEG, 1);
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
            RK_U32 *pRegs = (RK_U32 *) & (JpegHalCtx->regs);
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

MPP_RET hal_jpegd_vdpu2_start(void *hal, HalTaskInfo *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;

#ifdef RKPLATFORM
    RK_U32 *p_regs = (RK_U32 *)JpegHalCtx->regs;
    ret = mpp_device_send_reg(JpegHalCtx->vpu_socket, p_regs,
                              sizeof(JpegdIocRegInfo) / sizeof(RK_U32));
    if (ret != 0) {
        JPEGD_ERROR_LOG("mpp_device_send_reg Failed!!!\n");
        return MPP_ERR_VPUHW;
    }
#endif
    (void)JpegHalCtx;
    (void)task;
    FUN_TEST("Exit");
    return ret;
}

MPP_RET hal_jpegd_vdpu2_wait(void *hal, HalTaskInfo *task)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    (void)JpegHalCtx;
    (void)task;

#ifdef RKPLATFORM
    JpegRegSet *reg_out = NULL;
    RK_U32 errinfo = 1;
    MppFrame tmp = NULL;
    RK_U32 reg[184];

    ret = mpp_device_wait_reg(JpegHalCtx->vpu_socket, reg,
                              sizeof(reg) / sizeof(RK_U32));

    reg_out = (JpegRegSet *)reg;
    if (reg_out->reg55_Interrupt.sw_dec_bus_int) {
        JPEGD_ERROR_LOG("IRQ BUS ERROR!");
    } else if (reg_out->reg55_Interrupt.sw_dec_error_int) {
        JPEGD_ERROR_LOG("IRQ STREAM ERROR!");
    } else if (reg_out->reg55_Interrupt.sw_dec_timeout) {
        JPEGD_ERROR_LOG("IRQ TIMEOUT!");
    } else if (reg_out->reg55_Interrupt.sw_dec_buffer_int) {
        JPEGD_ERROR_LOG("IRQ BUFFER EMPTY!");
    } else if (reg_out->reg55_Interrupt.sw_dec_irq) {
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

MPP_RET hal_jpegd_vdpu2_reset(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;
    JpegHalContext *JpegHalCtx = (JpegHalContext *)hal;
    (void)JpegHalCtx;

    return ret;
}

MPP_RET hal_jpegd_vdpu2_flush(void *hal)
{
    FUN_TEST("Enter");
    MPP_RET ret = MPP_OK;

    (void)hal;
    return ret;
}

MPP_RET hal_jpegd_vdpu2_control(void *hal, RK_S32 cmd_type,
                                void *param)
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
